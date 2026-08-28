#include "includes.h"
#include "health_manager.h"
#include "sensor_service_wrist_tilt.h"
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
static u8 wrist_enable;
static int shm_det_wrist_update_deal(u8 event)
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


static int shm_det_wrist_io_crtl(int cmd, void *priv)
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
        shm_det_wrist_update_deal((u8)priv);
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(wrist)
{
    .module = SHM_MOD_DET_WRIST,
     .io_ctrl = shm_det_wrist_io_crtl,
      .get_value = NULL,
};
#endif
