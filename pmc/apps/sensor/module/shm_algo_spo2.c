#include "health_manager.h"
#include "sdk_config.h"

#define LOG_TAG_CONST       SENSOR_MODULE
#define LOG_TAG     		"[ALGO_SPO2]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"


#if TCFG_SENSOR_HR_CODE_IN_RAM
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#pragma const_seg(".sensor_const")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#pragma const_seg(".sensor_const")
#endif

#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&&TCFG_SENSOR_ALGO_SPO2_ENABLE)

/* module 层缓存：1s 节拍把 service 实时值存入，供大核 INFO / REAL_VALUE 拉取 */
static struct spo2_data g_spo2_data = {0};

#if TCFG_SENSOR_SPO2_SAMPLE_ENABLE
/* ------------------------------------------------------------------------------------*/
/* 后台硬件采样：采样过程在 pmc 完成，大核整点拉取整包后落盘。
 *
 * 小核无墙钟（rtc_read_time 是空桩），刻度只能按本地秒数计；采样点写进环形缓存，
 * 大核任何时刻来拉都取「最近 SHM_SAMPLE_NUM 个刻度」，不要求拉取节奏与刻度节奏成固定比例。
 *
 * 心率与血氧共用同一片 vc30fx，物理上不能同时采（WORK_MODE_HRSPO2 无实现），
 * 故按整窗轮换：心率占偶数刻度、血氧占奇数刻度，各 30s 完整窗口，每模态 6 点/小时。 */
/* ------------------------------------------------------------------------------------*/
#define SHM_SAMPLE_INTERVAL_SEC     (SHM_SAMPLE_INTERVAL_MIN * 60)	//刻度长度(秒，300)

/* 模拟数据自测：不接 vc30fx，用小核生成的假值跑通「采样→跨核→落盘→读回」链路。
 * 打开后刻度缩到 5s、采样窗 1s，约 60s 即可填满整组 12 个刻度。交付态必须为 0。 */
#define SHM_SAMPLE_TEST_ENABLE      (0)

#if SHM_SAMPLE_TEST_ENABLE
#define SAMPLE_INTERVAL_SEC         (5)
#define SAMPLE_DURATION_SEC         (1)
/* 模拟血氧 90~99 循环递增：与心率的 70~99 取值区间错开，便于在日志里区分两路数据 */
static u8 g_spo2_test_val = 0;
#define SAMPLE_SPO2_VALUE()         (90 + (g_spo2_test_val++ % 10))
#else
#define SAMPLE_INTERVAL_SEC         (SHM_SAMPLE_INTERVAL_SEC)
#define SAMPLE_DURATION_SEC         (SHM_SAMPLE_DURATION_SEC)
#define SAMPLE_SPO2_VALUE()         (g_spo2_data.real)
#endif
#endif	/* TCFG_SENSOR_SPO2_SAMPLE_ENABLE */

/* 算法开关状态：页面路径与采样路径共用，与采样链路是否编译无关 */
static u8 g_spo2_algo_on = 0;
static u8 g_spo2_user_enable = 0;

#if TCFG_SENSOR_SPO2_SAMPLE_ENABLE

/* 采样点环形缓存：写指针随刻度持续前进，环内旧点由新点自然覆盖，取数永远是最新的 */
#define SHM_SAMPLE_RING_LEN         SHM_SAMPLE_DATA_LEN	//环形长度(16，2 的幂，取模用掩码)

struct spo2_sample_ring {
    u8 buf[SHM_SAMPLE_RING_LEN];	//环形采样缓存（连续 SHM_SAMPLE_NUM 格 = 最近 SHM_SAMPLE_NUM 个刻度）
    u8 wr;							//写指针：下一个采样点的落位
    u8 cnt;							//环内已写入刻度数（饱和到 RING_LEN）
    u8 slot;						//当前刻度号(0..SHM_SAMPLE_NUM-1)，仅用于奇偶轮换
    u8 running;						//采样窗进行中
    u8 tick;						//本次采样窗已持续秒数
    u8 pend_val;					//本刻度采到的值（窗口刚结束，待刻度收尾时入环）
    u8 pend_valid;					//本刻度是否采到值（0=让路/未采，收尾补 0 占位）
    u8 algo_on;						//算法当前是否已开启（避免重复 enable）
    u8 user_enable;					//页面/上层使能（算法需保持开启）
    u16 sec;						//当前刻度内已过秒数（刻度 300s，u8 装不下）
};

static struct spo2_sample_ring g_spo2_ring = {0};

/* 采样点入环：写指针环形前进，最旧的点被自然覆盖 */
static void spo2_ring_push(u8 val)
{
    g_spo2_ring.buf[g_spo2_ring.wr] = val;
    g_spo2_ring.wr = (g_spo2_ring.wr + 1) & (SHM_SAMPLE_RING_LEN - 1);
    if (g_spo2_ring.cnt < SHM_SAMPLE_RING_LEN) {
        g_spo2_ring.cnt++;
    }
}

/* 取环内最新 n 个点（由旧到新）写入 out，返回实际取到的点数（0=环内尚空） */
static u8 spo2_ring_take(u8 *out, u8 n)
{
    u8 i, start;

    if (n > SHM_SAMPLE_RING_LEN) {
        n = SHM_SAMPLE_RING_LEN;
    }
    if (n > g_spo2_ring.cnt) {
        n = g_spo2_ring.cnt;		/* 环内还不足 n 个刻度时，只回传已有的 */
    }
    start = (g_spo2_ring.wr - n) & (SHM_SAMPLE_RING_LEN - 1);
    for (i = 0; i < n; i++) {
        out[i] = g_spo2_ring.buf[(start + i) & (SHM_SAMPLE_RING_LEN - 1)];
    }
    return n;
}
#endif	/* TCFG_SENSOR_SPO2_SAMPLE_ENABLE */

/* 算法开关：页面使能与后台采样窗两条路径共用，任一方需要即保持开启。
 * 与采样链路解耦：采样宏关闭时页面开关路径仍完整可用。 */
static void spo2_algo_switch(int on)
{
    int want = (on || g_spo2_user_enable) ? 1 : 0;
    if (want == g_spo2_algo_on) {
        return;
    }
    g_spo2_algo_on = want;
    if (want) {
        sensor_service_enable(SHM_MOD_SPO2_ALGO);
    } else {
        sensor_service_disable(SHM_MOD_SPO2_ALGO);
    }
    log_info("%s on:%d", __func__, want);
}

#if TCFG_SENSOR_SPO2_SAMPLE_ENABLE
/* 刻度起点尝试开启采样窗 */
static void spo2_sample_try_start(void)
{
    if (g_spo2_ring.running) {
        return;							/* 上一个采样窗未结束，本刻度跳过 */
    }
    if (!(g_spo2_ring.slot & 1)) {
        return;							/* 偶数刻度归心率（整窗轮换） */
    }
    /* 页面正在用别的模态时让路，避免切 vc30fx 工作模式打断用户测量 */
    if ((shm_page_hold_get() != SHM_PAGE_HOLD_NONE)
        && (shm_page_hold_get() != SHM_MOD_SPO2_ALGO)) {
        return;
    }
    g_spo2_ring.running = 1;
    g_spo2_ring.tick    = 0;
    spo2_algo_switch(1);
    log_debug("%s slot:%d", __func__, g_spo2_ring.slot);	/* 过程级：采到值由 done 记录 */
}

/* 1s 节拍推进采样状态机（只挂 SHM_CMD_UPDATE_SEC/UPDATE_ALL，不挂器件中断驱动的 UPDATE） */
static void spo2_sample_sec_deal(void)
{
    /* 采样窗计时与刻度计时并行推进；窗口结束后值先暂存，本刻度收尾时统一入环，
     * 使刻度时长恒为 SAMPLE_INTERVAL_SEC。 */
    if (g_spo2_ring.running) {
        if (g_spo2_ring.tick < SAMPLE_DURATION_SEC) {
            g_spo2_ring.tick++;
        } else {
            /* vc30fx OSC 冷启动收敛需十几到几十秒，取窗口末尾的值最稳 */
            g_spo2_ring.pend_val   = SAMPLE_SPO2_VALUE();
            g_spo2_ring.pend_valid = 1;
            g_spo2_ring.running = 0;
            g_spo2_ring.tick    = 0;
            spo2_algo_switch(0);			/* 页面未使能时归还器件 */
            log_info("%s done slot:%d val:%d", __func__, g_spo2_ring.slot, g_spo2_ring.pend_val);
        }
    }

    /* 刻度计时 */
    if (g_spo2_ring.sec == 0) {
        spo2_sample_try_start();		/* 刻度起点（上电即为刻度 0） */
        g_spo2_ring.sec = 1;
        return;
    }
    if (++g_spo2_ring.sec < SAMPLE_INTERVAL_SEC) {
        return;
    }
    /* 刻度收尾：本刻度入环。血氧不占用的刻度补 0 占位，使环内连续格严格对应连续刻度 */
    spo2_ring_push(g_spo2_ring.pend_valid ? g_spo2_ring.pend_val : 0);
    g_spo2_ring.pend_valid = 0;
    g_spo2_ring.sec  = 0;
    g_spo2_ring.slot = (g_spo2_ring.slot + 1) % SHM_SAMPLE_NUM;
}
#endif	/* TCFG_SENSOR_SPO2_SAMPLE_ENABLE */

static int shm_spo2_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        shm_page_hold_set(SHM_MOD_SPO2_ALGO);
        g_spo2_user_enable = 1;
        spo2_algo_switch(1);
        break;
    case SHM_CMD_DISABLE:
        if (shm_page_hold_get() == SHM_MOD_SPO2_ALGO) {
            shm_page_hold_set(SHM_PAGE_HOLD_NONE);
        }
        g_spo2_user_enable = 0;
        spo2_algo_switch(0);
        break;
    case SHM_CMD_UPDATE: {
        /* 器件中断驱动，频率不固定：只刷实时值，绝不推进采样状态机 */
        struct spo2_data spo2 = {0};
        if (!sensor_service_get_result(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_REAL_VALUE, &spo2)) {
            g_spo2_data = spo2;
        }
        /* service 未注册 / 器件未使能时保持上次值，不阻断节拍 */
    }
    break;
    case SHM_CMD_UPDATE_SEC:
    case SHM_CMD_UPDATE_ALL: {
        struct spo2_data spo2 = {0};
        if (!sensor_service_get_result(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_REAL_VALUE, &spo2)) {
            g_spo2_data = spo2;
        }
#if TCFG_SENSOR_SPO2_SAMPLE_ENABLE
        spo2_sample_sec_deal();		/* 后台周期采数（该板未开启时不编译） */
#endif
    }
    break;
    default:
        break;
    }
    return ret;
}

static int shm_spo2_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
    case SHM_GET_TYPE_REAL_VALUE:
        if (priv != NULL) {
            /* 大核消费方按 u8 real 读取；整结构体回填保证 real/max/min/avg 齐备 */
            memcpy(priv, &g_spo2_data, sizeof(g_spo2_data));
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
        break;
#if TCFG_SENSOR_SPO2_SAMPLE_ENABLE
    case SHM_GET_TYPE_SAMPLE_BUF:
        if (priv != NULL) {
            struct shm_sample_pack *pack = (struct shm_sample_pack *)priv;
            /* 取环内最新 SHM_SAMPLE_NUM 格回传（0=环内尚空，供大核判空） */
            pack->data_size  = spo2_ring_take(pack->data, SHM_SAMPLE_NUM);
            pack->sample_int = SHM_SAMPLE_INTERVAL_MIN;
            log_info("%s pack size:%d ring_cnt:%d", __func__,
                     pack->data_size, g_spo2_ring.cnt);
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
        break;
#endif	/* TCFG_SENSOR_SPO2_SAMPLE_ENABLE */
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(spo2)
{
    .module    = SHM_MOD_SPO2_ALGO,
    .io_ctrl   = shm_spo2_crtl,
    .get_value = shm_spo2_get,
};
#endif


