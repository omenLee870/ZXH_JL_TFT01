#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "sensor_hub.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-DET-WEAR]"
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

/* P11 模式用 detect_p11/，master 直连才编本文件（否则与 detect_p11 重复注册 wear） */
#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SPORT_HEALTH_DET_WEAR && !TCFG_SENSOR_HUB_P11)

/* 探索骨架：佩戴检测链路待打通，暂不调用 sensor_service_wear_*（该组接口当前无实现） */
static int shm_wear_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        /* sensor_service_wear_enable(); */
        break;
    case SHM_CMD_DISABLE:
        /* sensor_service_wear_disable(); */
        break;
    default:
        break;
    }
    return ret;
}

static int shm_wear_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        /* ret = sensor_service_wear_get_result(priv); */
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(wear)
{
    .module = SHM_MOD_WEAR_DETECTION,
    .io_ctrl   = shm_wear_crtl,
    .get_value = shm_wear_get,
};


#endif
