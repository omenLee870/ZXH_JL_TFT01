#include "includes.h"
#include "health_manager.h"
#include "sensor_service_wrist_tilt.h"
/* L0/L1/L2 → RAM；L3 → Flash（ALGO 类同向，函数级 AT 覆盖） */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#endif
#pragma const_seg(".sensor_const")
#include "sensor_seg.h"
#define LOG_TAG_CONST       SENSOR_MODULE
#define LOG_TAG     		"[DET_WRIST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

enum {
    MSYS_STATE_NORMAL = 0,
    MSYS_STATE_PWR_DOWN,
    MSYS_STATE_PWR_OFF,
    MSYS_STATE_SOFT_PWR_OFF,
    MSYS_STATE_SOFT_PWR_OFF_KEEP_NVRAM,
    MSYS_STATE_LIGHT_PWR_DOWN,
};

#define MSYS_STATE_GET()            (PMC_SYSTEM->PMC_SYS_CON0 & 0xf)
#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&& TCFG_SENSOR_DET_WRIST_ENABLE)
static u8 p11_algo_event;
/* 默认放行：大核下发 ENABLE 前有约 1 秒空窗，期间算法已出事件，初值 0 会全部丢弃；
 * 开关语义由大核闸门保证，关闭时大核下发 DISABLE 置 0。代价是开机 1 秒内可能误亮一次。 */
static u8 wrist_enable = 1;
AT_SENSOR_ALGO static int shm_det_wrist_update_deal(u8 event)
{
    p11_algo_event = event;

    u8 low_power_mode  = MSYS_STATE_GET() != MSYS_STATE_NORMAL;
    //翻腕且
    if (p11_algo_event == ALGO_WRIST_DOWN && low_power_mode) {
        return 0;
    }
    log_info("%s event:%d en:%d", __func__, event, wrist_enable);
    if (!wrist_enable) {
        return 0;
    }
    //发送 算法事件 给大核
    int msg[2];
    msg[0] = MSG_P11_ALGORITHM_EVENT;
    msg[1] = p11_algo_event;
    p2m_post_msg(MSG_APP, 0, (u8 *)msg, sizeof(msg));
    return 0;
}


AT_SENSOR_ALGO static int shm_det_wrist_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        wrist_enable = 1;
        break;
    case SHM_CMD_DISABLE:
        wrist_enable = 0;
        break;
    case SHM_CMD_UPDATE:
        shm_det_wrist_update_deal((u8)(int)priv);
        break;
    case SHM_CMD_UPDATE_SEC:
        /* 每秒读 GSENSOR 算法输出的 wrist_event，非 ALGO_NOTHING 时回传大核；
         * update_sec_all 按 module 升序遍历，GSENSOR_ALGO(1) 先于 DET_WRIST */
    {
        struct algo_value algo;
        if (!sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo)) {
#if (CONFIG_SPORT_HEALTH_MANAGER_DEBUG & BIT(0))
            /* mock: 每 10s 周期交替注入 UP(前 5s)/DOWN(后 5s) */
            static u8 mock_wrist_cnt = 0;
            mock_wrist_cnt++;
            if (mock_wrist_cnt >= 10) {
                mock_wrist_cnt = 0;
            }
            algo.wrist_event = (mock_wrist_cnt < 5) ? ALGO_WRIST_UP : ALGO_WRIST_DOWN;
#endif
            if (algo.wrist_event != ALGO_NOTHING) {
                shm_det_wrist_update_deal(algo.wrist_event);
            }
        }
    }
    break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}


/* T-005：补齐 get_value，使 sport_health_manager_value_get(SHM_MOD_DET_WRIST, ...) 可用。
 * 无专用结构体，REAL_VALUE/INFO 统一按 4 字节 int 返回最近一次翻腕事件。 */
AT_SENSOR_ALGO static int shm_det_wrist_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
    case SHM_GET_TYPE_REAL_VALUE:
        if (priv != NULL) {
            *(int *)priv = (int)p11_algo_event;
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(wrist)
{
    .module = SHM_MOD_DET_WRIST,
    .io_ctrl = shm_det_wrist_io_crtl,
    .get_value = shm_det_wrist_get_value,
};
#endif
