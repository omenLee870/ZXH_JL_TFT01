#include "sdk_config.h"
#include "sensor_driver.h"
#include "includes.h"
#include "sensor_service_wrist_tilt.h"
#include "health_manager.h"
#define LOG_TAG_CONST       SENSOR_SERVICE
#define LOG_TAG     		"[WRIST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"
#if TCFG_SENSOR_SERVICE_WRITE_ENABLE


enum {
    MSYS_STATE_NORMAL = 0,
    MSYS_STATE_PWR_DOWN,
    MSYS_STATE_PWR_OFF,
    MSYS_STATE_SOFT_PWR_OFF,
    MSYS_STATE_SOFT_PWR_OFF_KEEP_NVRAM,
    MSYS_STATE_LIGHT_PWR_DOWN,
};

#define MSYS_STATE_GET()            (PMC_SYSTEM->PMC_SYS_CON0 & 0xf)


static sensor_info_t sensor_info = {
    .type = SENSOR_ALGO_WRIST_TILT,
    .name = "JL GESTURE V1.0",
};

static s8 wrist_tilt_online(void)
{
    return RET_OK;
}

static void wrist_tilt_event_response(u32 event)
{
    //获取算法数据
    struct algo_value algo_out_value;
    int ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo_out_value);
    void *priv = (void *)algo_out_value.wrist_event;
    //输出结果
    sport_health_manager_msg_post_self(SHM_MOD_DET_WRIST, SHM_CMD_UPDATE, priv);
}

s8 wrist_tilt_run(void *arg, u16 *len)
{
    //jl_motion_run包含手势算法
    return RET_OK;
}

static s8 wrist_tilt_open(u16 range, u16 odr)
{
    return RET_OK;
}

static s8 wrist_tilt_close(void)
{
    return RET_OK;
}


REGISTER_SENSOR(wrist_tilt) = {
    .info    = &sensor_info,
    .online  = wrist_tilt_online,
    .open    = wrist_tilt_open,
    .close   = wrist_tilt_close,
    .run     = wrist_tilt_run,
};

#endif
