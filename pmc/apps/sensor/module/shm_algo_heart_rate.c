#include "health_manager.h"

#define LOG_TAG_CONST       SENSOR_MODULE
#define LOG_TAG     		"[ALGO_HR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"


#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&& TCFG_SENSOR_ALGO_HEART_RATE_EN)



static int hrs_crtl(int cmd, void *priv)
{
    // log_info("%s cmd=%d",__func__,cmd);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        sensor_service_enable(SHM_MOD_HEART_RATE_ALGO);
        break;
    case SHM_CMD_DISABLE:
        sensor_service_disable(SHM_MOD_HEART_RATE_ALGO);
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
    case SHM_GET_TYPE_REAL_VALUE:
        ret = sensor_service_get_result(SHM_MOD_HEART_RATE_ALGO, type, priv);
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



