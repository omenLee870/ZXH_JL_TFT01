
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "rtc/rtc_dev.h"
#include "sensor_hub.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM_BO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif

#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_BLOOD_OXYGEN)

#define HEALTH_RATE_DATA_TEST 		0				//使用模拟数据测试（默认关闭，走 pmc 小核真实数据）
#define HEALTH_FILE_TYPE			F_TYPE_BLOOD_OXYGEN
#define HEALTH_FILE_INVERVAL		(5)				//采样间隔(分钟)：整点对齐，每小时 12 点
#define HEALTH_FILE_STORE_INTERVAL	(60)			//存储间隔(分钟)：满一小时整点落盘一次
#define HEALTH_FILE_VERSION			(0)				//文件格式版本（与日常活动同口径）
#define HEALTH_SAMPLE_ENABLE		(1)				//硬件采样使能（默认开：后台独立采样，不进页面也采）
#define HEALTH_SAMPLE_DURATION_SEC	(30)			//单次采样时长(秒)
#define HEALTH_SAMPLE_NUM			(HEALTH_FILE_STORE_INTERVAL / HEALTH_FILE_INVERVAL)	//每小时采样点数(12)
#define HEALTH_SAMPLE_RING_LEN		(16)			//环形采样缓存长度(2 的幂，取模用掩码)

#define HEALTH_FILE_TEST			0
/* 采样执行方：1=采样窗在 pmc 小核完成，大核整点拉取整包后落盘（必须 P11 双核）；
 * 0=大核本地采样（单核/非 P11 平台回退）。本地采样代码整块保留，切宏即可回退比对。 */
#if TCFG_SENSOR_HUB_P11
#define HEALTH_SAMPLE_SRC_PMC		(1)
#else
#define HEALTH_SAMPLE_SRC_PMC		(0)
#endif
/* pmc 采样总开关：1=大核整点向 pmc 拉取采样整包落盘（须 P11 双核，且小核同侧
 * TCFG_SENSOR_SPO2_SAMPLE_ENABLE 也打开）；0=不拉取，大核不做后台周期采样。
 * 默认关闭——两侧开关须同时打开，只开单侧时另一侧的请求只会换来错误回码。 */
#define HEALTH_PMC_SAMPLE_ENABLE	(0)
/* 实际是否走 pmc 采样链路：架构支持且总开关打开。下面一律用本宏做条件编译 */
#define HEALTH_USE_PMC_SAMPLE		(HEALTH_SAMPLE_SRC_PMC && HEALTH_PMC_SAMPLE_ENABLE)
/* 整点落盘允许偏差(分钟)：1 分钟以内视为节拍正常抖动，超过即认定整点已不可信，
 * 本段采样整段丢弃。与 common/health_manager.c 的同名宏保持一致。 */
#define SHM_HOUR_DRIFT_TOLERATE_MIN	(1)
/* 自测快进：不等真实整点，改为固定周期拉取 + 槽位号递增，一次烧录即可在几分钟内
 * 覆盖「拉取→跨核回传→新建文件→续写文件→读回」全链路。交付态必须为 0。 */
#define HEALTH_SAMPLE_TEST_FAST		(0)
/* 文件布局：总头 + 数据头 + 整点槽位（槽位号 = 小时 0~23，每小时 HEALTH_SAMPLE_NUM 字节） */

/* 采样状态：采样点写进环形缓存，整点在环内取「最近 HEALTH_SAMPLE_NUM 个刻度」落盘；
 * 环内旧点由新点自然覆盖，取数永远是最新的。 */
struct bo_sample_data {
    u8 buf[HEALTH_SAMPLE_RING_LEN];	//环形采样缓存（连续 HEALTH_SAMPLE_NUM 格 = 最近 HEALTH_SAMPLE_NUM 个刻度）
    u8 wr;							//写指针：下一个采样点的落位
    u8 cnt;							//环内已写入点数（饱和到 RING_LEN）
    u8 slot;						//上次启动采样的刻度号(0xff=本小时尚未启动)
    u8 running;						//采样进行中
    u8 tick;						//本次采样已持续秒数
    u8 algo_on;						//算法当前是否由本模块开启
    u8 user_enable;					//页面/上层使能（算法需常开）
#if HEALTH_USE_PMC_SAMPLE
    u8 ready;						//pmc 采样包已到，待下一个 1s 节拍落盘
    u8 req_hour;					//发起拉取的小时：落盘槽位以发起时刻为准
#endif
};

struct blood_oxygen_data {
    u8 status;
    u8 bo_cur;
    u8 bo_max;
    u8 bo_min;
    u8 bo_avg;
    u16 avg_cnt;
    struct bo_sample_data sample;	//后台硬件采样状态
};

#define BO_MAX_DEFAULT  (0x0)
#define BO_MIN_DEFAULT  (0xff)
#define BO_CUR_DEFAULT  (0x0)

static struct blood_oxygen_data __info = {0};

/* 采样点入环：写指针环形前进，最旧的点被自然覆盖 */
#if (!HEALTH_SAMPLE_SRC_PMC || HEALTH_USE_PMC_SAMPLE)/* 至少一条后台采样链路启用时才需要 */
static void bo_mod_ring_push(u8 val)
{
    __info.sample.buf[__info.sample.wr] = val;
    __info.sample.wr = (__info.sample.wr + 1) & (HEALTH_SAMPLE_RING_LEN - 1);
    if (__info.sample.cnt < HEALTH_SAMPLE_RING_LEN) {
        __info.sample.cnt++;
    }
}
#endif

/* 取环内最新 n 个点（由旧到新）写入 out，返回实际取到的点数（0=环内尚空） */
static u8 bo_mod_ring_take(u8 *out, u8 n)
{
    u8 i, start;

    if (n > HEALTH_SAMPLE_RING_LEN) {
        n = HEALTH_SAMPLE_RING_LEN;
    }
    if (n > __info.sample.cnt) {
        n = __info.sample.cnt;		/* 环内还不足 n 个点时，只取已有的 */
    }
    start = (__info.sample.wr - n) & (HEALTH_SAMPLE_RING_LEN - 1);
    for (i = 0; i < n; i++) {
        out[i] = __info.sample.buf[(start + i) & (HEALTH_SAMPLE_RING_LEN - 1)];
    }
    return n;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mod_value_clr 重置统计数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mod_value_clr()
{
    /* if(__info.status == SHM_MOD_STA_ENABLE){ */
    /* return -SHM_ERR_MOD_STA_ERR; */
    /* } */
    __info.bo_cur = BO_CUR_DEFAULT;
    __info.bo_min = BO_MIN_DEFAULT;
    __info.bo_max = BO_MAX_DEFAULT;
    __info.bo_avg = BO_CUR_DEFAULT;
    __info.avg_cnt = 0;
    return  SHM_ERR_OK;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mod_algo_switch 统一算法开关
 *
 * @param on 本次请求（0=请求关闭，1=请求打开）
 * @note 页面使能与后台采样两条路径共用：只要任一方需要，算法保持打开，
 *       避免两条路径互相覆盖对方的开关状态。
 */
/* ------------------------------------------------------------------------------------*/
static void bo_mod_algo_switch(int on)
{
    int want = (on || __info.sample.user_enable) ? 1 : 0;

    if (want == __info.sample.algo_on) {
        return;
    }
    __info.sample.algo_on = want;
    __info.status = want ? SHM_MOD_STA_ENABLE : SHM_MOD_STA_DISABLE;
    sport_health_manager_msg_post_self(SHM_MOD_SPO2_ALGO,
                                       want ? SHM_CMD_ENABLE : SHM_CMD_DISBALE, NULL);
    log_info("%s on:%d", __func__, want);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_file_read  读取文件记录
 *
 * @param p
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_file_read(struct health_file_data_info *p)
{
    struct sys_time time;
#if 0//只读当天
    rtc_read_time(&time);
#else//由句柄指定日期
    memcpy(&time, &p->file_time, sizeof(struct sys_time));
#endif
    void *fp  = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);
    if (!fp) {
        return -SHM_ERR_FILE_NOT_FIND;
    }
    int file_len = sport_health_file_get_len(fp);
    int data_offset = sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);

    /* 采样间隔（相邻采样点间隔分钟数），供上层换算时间轴 */
    p->interval = HEALTH_FILE_INVERVAL;
    p->rlen = file_len - data_offset;
    p->rbuf = zalloc(p->rlen);
    sport_health_file_read(fp, p->rbuf, data_offset, p->rlen);
    return SHM_ERR_OK;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_file_save  保存数据到文件
 *
 * @param p_time 落盘时刻（NULL=取当前 RTC）
 * @note 整点槽位：每小时一组 HEALTH_SAMPLE_NUM 个采样值，槽位号 = 小时(0~23)。
 *       缓存不足时补 0，保持槽位定长便于读侧解析。
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_file_save(struct sys_time *p_time)
{
    int ret = SHM_ERR_OK;
    int file_len = 0;
    struct sys_time time;
    int data_offset = 0;
    int time_offset = 0;
    u8 samp[HEALTH_SAMPLE_NUM] = {0};	/* 环内最新 HEALTH_SAMPLE_NUM 格（不足补 0，保持槽位定长） */

    if (p_time) {
        memcpy(&time, p_time, sizeof(struct sys_time));
    } else {
        rtc_read_time(&time);
    }

    /* 整点槽位：每小时一组采样值，槽位号即小时(0~23) */
    time_offset  = time.hour * HEALTH_SAMPLE_NUM;
    /* 取环内最新 HEALTH_SAMPLE_NUM 个点落盘；环内不足则补 0，保持槽位定长便于读侧解析 */
    bo_mod_ring_take(samp, HEALTH_SAMPLE_NUM);
    log_info("%s save hour:%d min:%d ring_cnt:%d", __func__, time.hour, time.min, __info.sample.cnt);
    struct health_file_total_head file_head = {
        .type = HEALTH_FILE_TYPE,
        .year = time.year,
        .month = time.month,
        .day  = time.day,
        .crc  = 0xffff,
        .version = HEALTH_FILE_VERSION,
        .reserve = 0,
        .interval = HEALTH_FILE_INVERVAL,
    };

    struct health_file_data_head data_head = {
        .hour = time.hour,
        .min = 0,
        .len = time_offset + HEALTH_SAMPLE_NUM,
    };

    void *fp  = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);
    if (!fp) {
        fp = sport_health_file_open(HEALTH_FILE_TYPE, 0);
        if (!fp) {
            log_error("%s file open fail", __func__);
            return -SHM_ERR_FILE_NOT_FIND;
        }
        sport_health_common_swapX((u8 *) &file_head.year, (u8 *) &file_head.year, 2);
        file_len = sport_health_file_write(fp, (u8 *) &file_head, file_len, sizeof(struct health_file_total_head));

        sport_health_common_swapX((u8 *) &data_head.len, (u8 *) &data_head.len, 2);
        file_len = sport_health_file_write(fp, (u8 *) &data_head, file_len, sizeof(struct health_file_data_head));

        //写入数据（整组采样值）
        data_offset = time_offset +  sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);
        file_len = sport_health_file_write(fp, samp, data_offset, HEALTH_SAMPLE_NUM);
        if (file_len != data_offset + HEALTH_SAMPLE_NUM) {
            log_error("%s write fail offset:%d len:%d", __func__, data_offset, file_len);
            ret = -SHM_ERR_DATA_ERR;
        }

        //更新校验值//只需要crc不同，即可，不做实际校验。
        file_head.crc =  time_offset;
        sport_health_file_update(fp, (u8 *) &file_head, 0, sizeof(struct health_file_total_head));
    } else {
        //续写
        file_len = sport_health_file_get_len(fp);
        data_offset = time_offset +  sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);
        /* 槽位号逐 HEALTH_SAMPLE_NUM 递增：data_offset 等于文件尾即本整点未落盘；
         * 同一整点重复触发时 data_offset 小于文件尾，自然幂等。 */
        if (data_offset >= file_len) {
            //写入数据（整组采样值）
            file_len = sport_health_file_write(fp, samp, data_offset, HEALTH_SAMPLE_NUM);
            if (file_len != data_offset + HEALTH_SAMPLE_NUM) {
                log_error("%s write fail offset:%d len:%d", __func__, data_offset, file_len);
                ret = -SHM_ERR_DATA_ERR;
            }
            //更新校验值//只需要crc不同，即可，不做实际校验。
            file_head.crc =  time_offset;
            sport_health_common_swapX((u8 *) &file_head.year, (u8 *) &file_head.year, 2);
            sport_health_file_update(fp, (u8 *) &file_head, 0, sizeof(struct health_file_total_head));
            /* 数据头原地更新（len = 已覆盖槽位数 * 每组字节数），与新建分支布局一致 */
            sport_health_common_swapX((u8 *) &data_head.len, (u8 *) &data_head.len, 2);
            sport_health_file_update(fp, (u8 *) &data_head, sizeof(struct health_file_total_head),
                                     sizeof(struct health_file_data_head));
        }
    }
#if 0
    file_len = sport_health_file_get_len(fp);
    u8 *rw_buf = zalloc(file_len);
    sport_health_file_read(fp, rw_buf, 0, file_len);
    put_buf(rw_buf, file_len);
    free(rw_buf);
#endif
    if (fp) {
        sport_health_file_close(fp);
    }
    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_update_sec_deal 测量时更新数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_update_sec_deal()
{
    u8 blood_oxygen = 0;
    int ret = sport_health_manager_value_get(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_REAL_VALUE, &blood_oxygen);
#if HEALTH_RATE_DATA_TEST
    blood_oxygen = 80 + rand32() % 30;
#endif//HEALTH_RATE_DATA_TEST
    if (blood_oxygen) {
        __info.bo_cur = blood_oxygen;
        __info.bo_max = (__info.bo_cur > __info.bo_max) ? __info.bo_cur : __info.bo_max;
        __info.bo_min = (__info.bo_cur < __info.bo_min) ? __info.bo_cur : __info.bo_min;
        if (__info.avg_cnt) {
            u32 bo_sum  = __info.bo_avg * __info.avg_cnt + __info.bo_cur;
            __info.avg_cnt ++;
            __info.bo_avg = bo_sum / __info.avg_cnt;
        } else {
            __info.avg_cnt ++;
            __info.bo_avg = __info.bo_cur;
        }

        __info.status = SHM_MOD_STA_DATA_UPDATE_SUCC;
    }
    log_debug("%s bo:%d bo_cur:%d min:%d max:%d avg:%d", __func__, blood_oxygen, __info.bo_cur, __info.bo_min, __info.bo_max, __info.bo_avg);
    return  ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mod_sample_deal 后台硬件采样（由 UPDATE_SEC 每秒驱动）
 *
 * @param time 当前时刻
 * @note 采样刻度整点对齐：每小时第 0/5/10…/55 分钟各启动一次采样，持续
 *       HEALTH_SAMPLE_DURATION_SEC 秒后把结果存入缓存，满一小时后整点整组落盘。
 * @note 仅 HEALTH_SAMPLE_SRC_PMC=0（大核本地采样）时编译。
 */
/* ------------------------------------------------------------------------------------*/
#if !HEALTH_SAMPLE_SRC_PMC
static void bo_mod_sample_deal(struct sys_time *time)
{
    u8 slot = time->min / HEALTH_FILE_INVERVAL;

    if (__info.sample.running) {
        /* 采样中：计时到点则收尾存缓存 */
        if (__info.sample.tick < HEALTH_SAMPLE_DURATION_SEC) {
            __info.sample.tick++;
            return;
        }
        __info.sample.running = 0;
        __info.sample.tick = 0;
        /* 顺序入环：环内始终保留最近若干次采样，落盘时取最新 HEALTH_SAMPLE_NUM 个；
         * slot 仅作去重用途——非整点开机时前面几个刻度本就采不到。 */
        bo_mod_ring_push(__info.bo_cur);
        bo_mod_algo_switch(0);/* 页面未使能时归还算法 */
        log_info("%s done ring_cnt:%d bo:%d", __func__, __info.sample.cnt, __info.bo_cur);
        return;
    }

    /* 未到采样刻度不启动 */
    if ((time->min % HEALTH_FILE_INVERVAL) || (slot == __info.sample.slot)) {
        return;
    }
    __info.sample.slot = slot;
    __info.sample.running = 1;
    __info.sample.tick = 0;
    bo_mod_algo_switch(1);
    log_info("%s start slot:%d", __func__, slot);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mod_hour_file_check 整点落盘检查（由 UPDATE_SEC 驱动，仅 0 分执行）
 *
 * @param time 当前时刻（与采样共用同一次读时，避免跨秒不一致）
 *
 * @return
 * @note 与日常活动（shm_daily_active_hour_file）同规则：整点写一笔，槽位号 = 小时。
 * @note 仅 HEALTH_SAMPLE_SRC_PMC=0（大核本地采样）时编译。
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mod_hour_file_check(struct sys_time *time)
{
    static u8 last_hour = 0xff;/* 去重：整点这一分钟内只落盘一次 */
    static u8 last_day  = 0xff;

    if (time->min != 0) {
        return SHM_ERR_OK;/* 仅整点（0 分）执行 */
    }
    if ((time->hour == last_hour) && (time->day == last_day)) {
        return SHM_ERR_OK;/* 该整点已落盘 */
    }

    int ret = bo_mode_file_save(time);
    if (ret == SHM_ERR_OK) {
        /* 仅落盘成功才记去重标记：失败时本分钟内会继续重试 */
        last_hour = time->hour;
        last_day  = time->day;
        /* 本小时采样已落盘。环不清空：后续新点会自然覆盖旧点，取数始终是最新的；
         * 本小时没采到的位置由落盘时的补 0 承担。 */
        __info.sample.slot = 0xff;
    }
    return ret;
}
#endif

#if HEALTH_USE_PMC_SAMPLE
/* ------------------------------------------------------------------------------------*/
#if HEALTH_SAMPLE_TEST_FAST
/* 自测：把刚写的槽位读回打印，核对「小核采样值 → 跨核 → 落盘 → 读回」一致性 */
static void bo_mod_test_readback(void)
{
    struct health_file_data_info *finfo = zalloc(sizeof(struct health_file_data_info));
    if (!finfo) {
        return;
    }
    rtc_read_time(&finfo->file_time);
    if (bo_mode_file_read(finfo) == SHM_ERR_OK) {
        log_info("%s [TEST] readback rlen:%d interval:%d", __func__, finfo->rlen, finfo->interval);
        put_buf(finfo->rbuf, finfo->rlen);
        free(finfo->rbuf);
    } else {
        log_error("%s [TEST] readback fail", __func__);
    }
    free(finfo);
}
#endif

static void bo_mod_pmc_sample_deal(struct sys_time *time)
{
    static u8 last_req_hour = 0xff;/* 去重：一个整点只发起一次拉取 */
    static u8 last_req_day  = 0xff;

    if (__info.sample.ready) {
#if !HEALTH_SAMPLE_TEST_FAST
        /* 整点校验：本段数据由整点拉取而来，只有落在整点窗口内才承认它的时间归属。
         * 1s 节拍在低功耗下周期会漂、时钟也可能被人为调整，一旦拖到整点窗口之外
         * 才轮到落盘，这段数据对应哪个整点已不确定 —— 整段抛弃，不落盘。 */
        if (time->min > SHM_HOUR_DRIFT_TOLERATE_MIN) {
            log_error("%s drop sample: min %d > %d, ring_cnt:%d", __func__,
                      time->min, SHM_HOUR_DRIFT_TOLERATE_MIN, __info.sample.cnt);
            /* 只丢弃本次回传：环内已有数据由后续新点覆盖，不需要额外清空 */
            __info.sample.ready = 0;
            return;
        }
#endif
        /* 回传已到：本秒即落盘。槽位用发起拉取那一刻的小时——数据是「上一小时」
         * 采的，而落盘时刻与发起时刻同属一个整点，取发起时刻可避免回传跨秒错位。 */
        struct sys_time save_time = *time;
        save_time.hour = __info.sample.req_hour;
        save_time.min  = 0;
        __info.sample.ready = 0;
        /* 落盘后不清环：取数始终取最新，旧点由后续入环的新点自然覆盖 */
        if (bo_mode_file_save(&save_time) != SHM_ERR_OK) {
            log_error("%s save fail hour:%d", __func__, save_time.hour);
        }
#if HEALTH_SAMPLE_TEST_FAST
        bo_mod_test_readback();
#endif
        return;
    }

#if HEALTH_SAMPLE_TEST_FAST
    {
        /* 首轮等 70s：小核刻度缩到 5s（见小核 SHM_SAMPLE_TEST_ENABLE），先让它把整组
         * 12 个刻度采满；之后每 20s 一轮。槽位号逐轮递增，覆盖文件新建与续写两个分支。 */
        static u16 fast_tick = 0;
        static u8  fast_slot = 0;
        u16 period = (fast_slot == 0) ? 70 : 20;

        if (++fast_tick < period) {
            return;
        }
        fast_tick = 0;
        __info.sample.req_hour = fast_slot;
        fast_slot = (fast_slot + 1) % 24;
        sensor_driver_p11_get_data(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_SAMPLE_BUF);
        log_info("%s [TEST] req sample slot:%d", __func__, __info.sample.req_hour);
    }
    return;
#else
    if (time->min != 0) {
        return;/* 仅整点发起拉取 */
    }
    if ((time->hour == last_req_hour) && (time->day == last_req_day)) {
        return;/* 本整点已发起，等回传，不重复请求 */
    }
    last_req_hour = time->hour;
    last_req_day  = time->day;
    __info.sample.req_hour = time->hour;
    sensor_driver_p11_get_data(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_SAMPLE_BUF);
    log_info("%s req pmc sample hour:%d", __func__, time->hour);
#endif
}
#endif

static int bo_mod_io_ctrl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        /* 采样状态随 __info 一起复位；slot 用 0xff 表示"本小时尚未启动采样" */
        memset(&__info.sample, 0, sizeof(__info.sample));
        __info.sample.slot = 0xff;
        bo_mod_value_clr();
        break;
    case SHM_CMD_ENABLE:
        __info.sample.user_enable = 1;
        bo_mod_algo_switch(1);
        break;
    case SHM_CMD_DISBALE:
        __info.sample.user_enable = 0;
        bo_mod_algo_switch(0);
        break;
    case SHM_CMD_UPDATE:
    case SHM_CMD_UPDATE_SEC: {
        struct sys_time time;
        rtc_read_time(&time);
        if (__info.status == SHM_MOD_STA_ENABLE || (__info.status == SHM_MOD_STA_DATA_UPDATE_SUCC) || (__info.status == SHM_MOD_STA_DATA_UPDATE_NULL)) {
            bo_mode_update_sec_deal();
        }
#if HEALTH_USE_PMC_SAMPLE
        bo_mod_pmc_sample_deal(&time);/* pmc 采样模式：整点拉取 + 回传落盘 */
#elif !HEALTH_SAMPLE_SRC_PMC
        bo_mod_hour_file_check(&time);/* 整点（0 分）落盘整组采样 */
#if HEALTH_SAMPLE_ENABLE
        bo_mod_sample_deal(&time);/* 采样刻度启动/收尾后台采样 */
#endif
#endif
    }
    break;
#if HEALTH_USE_PMC_SAMPLE
    case SHM_CMD_UPDATE_SAMPLE_P11: {
        /* 小核整点回传的采样整包：此处只暂存，落盘留到下一个 1s 节拍
         * （本回调跑在 pmc 消息线程上下文，不能在这里写 flash） */
        struct shm_sample_pack *pack = (struct shm_sample_pack *)priv;
        u8 i;
        if (pack == NULL) {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
            break;
        }
        if (pack->sample_int != HEALTH_FILE_INVERVAL) {
            log_error("%s sample_int mismatch:%d expect:%d", __func__, pack->sample_int, HEALTH_FILE_INVERVAL);
        }
        /* 顺序入环：环内始终保留最近若干个采样点。小核某轮回传不足整包（开机首轮、
         * 被页面占用让路）时，环内旧点自然补齐，落盘取到的是最近的完整窗口，
         * 而不是半包数据后面接 0。data_size==0 时本次不入环，环内容保持不动。 */
        for (i = 0; i < pack->data_size && i < SHM_SAMPLE_DATA_LEN; i++) {
            bo_mod_ring_push(pack->data[i]);
        }
        __info.sample.ready = 1;
        log_info("%s pack size:%d ring_cnt:%d", __func__, pack->data_size, __info.sample.cnt);
    }
    break;
#endif
    case SHM_CMD_SAVE_SINGLE:
    case SHM_CMD_SAVE_CONTINUE:
#if HEALTH_FILE_INVERVAL
        /* 关机/主动保存：不受整点门控限制，按当前时刻覆盖本小时槽位 */
        bo_mode_file_save(NULL);
#endif
        break;
    case SHM_CMD_CLEAR_VALUE:
        __info.status = SHM_MOD_STA_DATA_UPDATE_NULL;
        bo_mod_value_clr();
        break;
    default:
        break;
    }
    return ret;
}

static int bo_mod_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_cur, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_MAX_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_max, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_MIN_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_min, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_AVG_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_avg, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_REC_VALUE: {
        ret = bo_mode_file_read((struct health_file_data_info *)priv);
    }
    break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(blood_oxygen)
{
    .module = SHM_MOD_BLOOD_OXYGEN,
    .io_ctrl = bo_mod_io_ctrl,
    .get_value = bo_mod_get_value,
};
#endif
