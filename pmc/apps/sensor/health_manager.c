#include "health_manager.h"
#include "usr_timer.h"
#include "sdk_config.h"
#include "sensor_seg.h"
#include "init.h"
/* L0/L1/L2 → RAM；L3 → Flash（FIFO 类同向，函数级 AT 覆盖） */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
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
#define LOG_TAG_CONST       SENSOR_MANAGER
#define LOG_TAG     		"[SENSOR_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if CONFIG_SPORT_HEALTH_MANAGER_ENABLE

/* 历史测试标记宏，当前无源码消费者；模拟数据总闸为 CONFIG_SPORT_HEALTH_MANAGER_DEBUG */
#define SENSOR_MANAGER_TEST_ENABLE			0

#if CONFIG_SPORT_HEALTH_MANAGER_DEBUG
static int test_steps = 0;
static int test_hr  = 0;
static int tets_spo2 = 0;
#endif

#if TCFG_SENSOR_SERVICE_MOTION_ENABLE
extern u8 gsensor_dir_get(void);
#endif

/* 页面占用登记：大核 UI 使能某模态期间记录该模块号（未占用为 0xff）。
 * 心率/血氧共用 vc30fx，后台采样窗口若在页面使用期间切工作模式会打断用户测量，
 * 采样状态机据此让路（见 shm_algo_heart_rate.c / shm_algo_spo2.c）。 */
static u8 shm_page_hold_mod = SHM_PAGE_HOLD_NONE;

u8 shm_page_hold_get(void)
{
    return shm_page_hold_mod;
}

void shm_page_hold_set(u8 mod)
{
    shm_page_hold_mod = mod;
}


AT_SENSOR_ALGO int sport_health_manager_value_get(int module, int type, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = -SHM_ERR_MOD_NOT_FIND;
    if (module >= SHM_MOD_END) {
        goto __ret;
    }
    if (type >= SHM_GET_TYPE_MAX) {
        ret =  -SHM_ERR_TYPE_NOT_DEFINE;
        goto __ret;
    }

    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        if (mod->module == module) {
            if (mod->get_value) {
                ret = mod->get_value(type, priv);
            } else {
                ret = -	SHM_ERR_MOD_CB_NOT_DEFINE;
                goto __ret;
            }
        }
    }

__ret:
    if (ret) {
        log_error("%s err:%d rets:0x%x module:0x%x type:0x%x", __func__, ret, rets, module, type);
    } else {
        log_debug("%s module:0x%x type:0x%x succ", __func__, module, type);
    }
    return ret;
}

AT_SENSOR_ALGO int sport_health_manager_get_value_m2p(int module, int type)
{
    int ret = -1;
    health_data_msg_t msg = {0};
    if (type == SHM_CMD_ENABLE || type == SHM_CMD_DISABLE) {
        sport_health_manager_msg_post_self(module, type, NULL);
        return 0;
    }
    switch (module) {
    case SHM_MOD_GSENSOR_ALGO:
#if TCFG_SENSOR_SERVICE_MOTION_ENABLE
        if (type == SHM_GET_TYPE_REAL_VALUE) {
            msg.data.dir = gsensor_dir_get();
            static u8 last_dir = 0xff;
            if (msg.data.dir != last_dir) {
                log_info("dir:%d", msg.data.dir);
                last_dir = msg.data.dir;
            }
            ret = 0;
        } else
#endif
            /* 模拟数据分支必须与变量定义同受 #if 守卫：DEBUG=0 时 test_xxx 未定义 */
#if CONFIG_SPORT_HEALTH_MANAGER_DEBUG
            if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(0)) {
                test_steps ++;
                msg.data.acceler.steps = test_steps;
                ret = 0;
            } else
#endif
#if TCFG_SENSOR_SERVICE_MOTION_ENABLE
            {
                struct algo_value algo;
                if (!sport_health_manager_value_get(module, SHM_GET_TYPE_INFO, (void *)&algo)) {
                    msg.data.acceler = algo;
                    ret = 0;
                }
            }
#else
            {
            }
#endif
        break;
    case SHM_MOD_HEART_RATE_ALGO:
#if CONFIG_SPORT_HEALTH_MANAGER_DEBUG
        if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(1)) {
            test_hr++;
            test_hr %= 20;
            msg.data.hr_data.real = 90 + test_hr;
            ret = 0;
        } else
#endif
#if TCFG_SENSOR_ALGO_HEART_RATE_EN
        {
            if (!sport_health_manager_value_get(module, type, (void *)&msg.data.hr_data)) {
                ret = 0;
            }
        }
#else
        {
        }
#endif
        break;
    case SHM_MOD_SPO2_ALGO:
#if CONFIG_SPORT_HEALTH_MANAGER_DEBUG
        if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(2)) {
            tets_spo2++;
            tets_spo2 %= 10;
            msg.data.spo2_data.real = 90 + tets_spo2;
            ret = 0;
        } else
#endif
#if TCFG_SENSOR_ALGO_SPO2_ENABLE
        {
            if (!sport_health_manager_value_get(module, type, (void *)&msg.data.spo2_data)) {
                ret = 0;
            }
        }
#else
        {
        }
#endif
        break;
    case SHM_MOD_DET_WRIST:
        msg.data.dir = 1;
        ret = 0;
        break;
    case SHM_MOD_SLEEP:
        /* 睡眠 stage 取自 gsensor 算法输出（jl_motion_run 的 algo_out.sleep，参考 sleep_type），
         * 原为硬编码占位 1。取数路径与 SHM_MOD_GSENSOR_ALGO 分支一致，
         * 避免小核侧再维护一份独立的睡眠状态机 */
#if TCFG_SENSOR_SERVICE_MOTION_ENABLE
    {
        struct algo_value algo;
        if (!sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, (void *)&algo)) {
            msg.data.acceler.sleep = algo.sleep;
            ret = 0;
        }
    }
#else
    msg.data.acceler.sleep = 1;
    ret = 0;
#endif
    break;
    default:
        break;
    }
    if (ret) {
        ret = sport_health_manager_value_get(module, type, (void *)&msg.data);
    }

    if (!ret) {
        msg.msg = MSG_P11_HEALTH_MANAGER_DATA_P2M;
        msg.module = module;
        msg.type = type;

        int post_ret = p2m_post_msg(MSG_APP, 0, (u8 *)&msg, sizeof(health_data_msg_t));
        log_debug("%s post_ret:%d module:%d type:%d (len:%d)", __func__, post_ret, module, type, (u32)sizeof(health_data_msg_t));
    }
    return ret;
}
AT_SENSOR_ALGO int sport_health_manager_msg_post_self(int module, int cmd, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = SHM_ERR_OK;
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        if (mod->module == module) {
            if (mod->io_ctrl) {
                mod->io_ctrl(cmd, priv);
            }
            continue;
        }
    }

    return ret;
}
AT_SENSOR_FIFO int sport_health_manager_msg_post(int module, int cmd, int priv_len, void *priv)
{
    int rets;
    __asm__ volatile("%0 = rets":"=r"(rets));

    int ret = SHM_ERR_OK;
#define MSG_LEN 7//3+sizeof(addr)
    u8 msg[MSG_LEN];
    msg[0] = MSG_P11_HEALTH_MANAGER_DATA_P2P;
    msg[1] = module;
    msg[2] = cmd;
    ASSERT(priv_len <= (MSG_LEN - 3));
    memcpy(msg + 3, (u8 *)priv, priv_len);
    m2p_post_msg_q32f(MSG_APP, 0, msg, MSG_LEN);
    return ret;
}
AT_SENSOR_COLD static void sport_health_module_init()
{
    log_debug("%s %d 0x%x 0x%x", __func__, __LINE__, (u32)sport_health_module_begin, (u32)sport_health_module_end);
    struct sport_health_module *mod;
    list_for_each_sport_health_module(mod) {
        log_debug("%s %d", __func__, mod->module);
        if (mod->io_ctrl) {
            mod->io_ctrl(SHM_CMD_INIT, NULL);
        }
    }
}

AT_SENSOR_ALGO static void sport_health_manager_update_sec_all()
{
    /* printf("%s %d", __func__, __LINE__); */

    for (int module = SHM_MOD_BEGIN + 1; module < SHM_MOD_END; module++) {
        struct sport_health_module *mod;
        list_for_each_sport_health_module(mod) {
            if (mod->module == module) {
                if (mod->io_ctrl) {
                    mod->io_ctrl(SHM_CMD_UPDATE_SEC, NULL);
                }
                break;
            }
        }
    }
}

AT_SENSOR_ALGO void sport_health_manager_task()
{
    /* dir 不再单独触发（原每分钟/秒一次的 sensor_service_motion_dir_update() 已删除）：
     * 方向计算已并入下面这轮 SHM_MOD_GSENSOR_ALGO 的 update 流程，与计步算法复用同一份
     * accel，避免两者争抢 cbuf index 0。更新周期即 CONFIG_SPORT_HEALTH_UPDATE_PERIOD。 */
    sport_health_manager_update_sec_all();
}
AT_SENSOR_FIFO void sport_health_manager_sec_update(void *priv)
{
    log_debug("%s", __func__);
    task_post_msg(NULL,  MSG_SENSOR, MSG_P11_HEALTH_MANAGER_LOOP);
}
static u16 sec_timer_id = 0;

AT_SENSOR_COLD int sport_health_manager_init()
{
    log_info("%s enter", __func__);
    sensor_driver_check();
    sport_health_module_init();
    sec_timer_id = usr_timer_add(NULL,  sport_health_manager_sec_update, CONFIG_SPORT_HEALTH_UPDATE_PERIOD, 0);
    if (sec_timer_id == 0) {
        log_error("%s usr_timer_add failed: pool full (%dms)", __func__, CONFIG_SPORT_HEALTH_UPDATE_PERIOD);
    }
    log_info("%s done", __func__);
    return 0;
}
module_initcall(sport_health_manager_init);

/* static void health_manager_msg_handler(void *priv, u8 *msg, u32 len) */
/* { */
/* switch (msg[0]) { */

/* } */
/* } */

/* REGISTER_M2P_MSG_HANDLER(0, MSG_SENSOR,  health_manager_msg_handler); */

#endif

