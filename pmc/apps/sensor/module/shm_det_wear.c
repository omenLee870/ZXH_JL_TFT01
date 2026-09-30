#include "health_manager.h"
/* 段放置：随 hr LEVEL（1s 路径走 vc30fx 佩戴检测） */
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

#define LOG_TAG_CONST       SENSOR_MODULE
#define LOG_TAG     		"[DET_WEAR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"


#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE && TCFG_SENSOR_DET_WEAR_ENABLE)

/* module 层缓存：1s 节拍把 service 实时值存入，供大核 INFO / REAL_VALUE 拉取
 * （wear 无专用结构体，与 service 层一致按 4 字节 int 传递） */
static int g_wear_data = 0;

static int shm_det_wear_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        sensor_service_enable(SHM_MOD_WEAR_DETECTION);
        break;
    case SHM_CMD_DISABLE:
        sensor_service_disable(SHM_MOD_WEAR_DETECTION);
        break;
    case SHM_CMD_UPDATE:
    case SHM_CMD_UPDATE_SEC:
    case SHM_CMD_UPDATE_ALL: {
        int wear = 0;
        if (!sensor_service_get_result(SHM_MOD_WEAR_DETECTION, SHM_GET_TYPE_REAL_VALUE, &wear)) {
            g_wear_data = wear;
        }
        /* service 未注册 / 器件未使能时保持上次值，不阻断 1s 节拍 */
    }
    break;
    default:
        break;
    }
    return ret;
}

static int shm_det_wear_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
    case SHM_GET_TYPE_REAL_VALUE:
        if (priv != NULL) {
            *(int *)priv = g_wear_data;
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


REGISTER_SPORT_HEALTH_MODULE(wear)
{
    .module    = SHM_MOD_WEAR_DETECTION,
    .io_ctrl   = shm_det_wear_crtl,
    .get_value = shm_det_wear_get,
};


#endif
