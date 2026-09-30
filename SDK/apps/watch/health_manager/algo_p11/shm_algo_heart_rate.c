#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "sensor_hub.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-ALGO_HEART_RATE]"
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

#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SPORT_HEALTH_P11_ALGO_HEART_RATE)

static struct heart_data data = {0};


static int hrs_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        /* 大核→小核跨核下发 ENABLE：小核收到后走 msg_post_self → hrs_crtl →
         * sensor_service_enable → vc30fxStart */
        sensor_driver_p11_ctrl(SHM_MOD_HEART_RATE_ALGO, SHM_CMD_ENABLE);
        break;
    case SHM_CMD_DISABLE:
        sensor_driver_p11_ctrl(SHM_MOD_HEART_RATE_ALGO, SHM_CMD_DISABLE);
        break;
    case SHM_CMD_UPDATE_VAILE_P11:
        if (priv != NULL) {
            memcpy(&data, priv, sizeof(struct heart_data));
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
        break;
    default:
        break;
    }
    return ret;
}
static int hrs_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
    case SHM_GET_TYPE_REAL_VALUE:
        if (priv != NULL) {
            sensor_driver_p11_get_data(SHM_MOD_HEART_RATE_ALGO, type);
            /* 大核 app 模块只读 real 字段（u8），返回 data.real */
            *(u8 *)priv = data.real;
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


REGISTER_SPORT_HEALTH_MODULE(hrm)
{
    .module   = SHM_MOD_HEART_RATE_ALGO,
    .io_ctrl   = hrs_crtl,
    .get_value = hrs_get,
};


#endif
