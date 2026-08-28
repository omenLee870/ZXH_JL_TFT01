#include "health_manager.h"

#define LOG_TAG_CONST       SENSOR_MODULE
#define LOG_TAG     		"[ALGO_SPO2]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif

#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&&TCFG_SENSOR_ALGO_SPO2_ENABLE)



static int shm_spo2_crtl(int cmd, void *priv)
{
    // log_info("%s cmd=%d",__func__,cmd);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        sensor_service_enable(SHM_MOD_SPO2_ALGO);
        break;
    case SHM_CMD_DISABLE:
        sensor_service_disable(SHM_MOD_SPO2_ALGO);
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
    case SHM_GET_TYPE_REAL_VALUE:
        ret =  sensor_service_get_result(SHM_MOD_SPO2_ALGO, type, priv);
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


