#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "sensor_hub.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-ALGO_SPO2]"
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

#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SPORT_HEALTH_P11_ALGO_SPO2)

static struct spo2_data data = {0};


static int shm_spo2_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        /* 大核→小核跨核下发 ENABLE（与 HR 同修） */
        sensor_driver_p11_ctrl(SHM_MOD_SPO2_ALGO, SHM_CMD_ENABLE);
        break;
    case SHM_CMD_DISABLE:
        sensor_driver_p11_ctrl(SHM_MOD_SPO2_ALGO, SHM_CMD_DISABLE);
        break;
    case SHM_CMD_UPDATE_VAILE_P11:
        if (priv != NULL) {
            memcpy(&data, priv, sizeof(struct spo2_data));
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
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
            sensor_driver_p11_get_data(SHM_MOD_SPO2_ALGO, type);
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

REGISTER_SPORT_HEALTH_MODULE(spo2)
{
    .module    = SHM_MOD_SPO2_ALGO,
    .io_ctrl   = shm_spo2_crtl,
    .get_value = shm_spo2_get,
};
#endif
