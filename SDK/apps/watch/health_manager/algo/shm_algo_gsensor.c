#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "sensor_hub.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-ALGO_GSENSOR]"
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

#if (TCFG_SPORT_HEALTH_ENABLE&& TCFG_SPORT_HEALTH_ALGO_GSENSOR)


#define SPORT_HEALTH_ALGO_TEST_ENABLE		(!TCFG_SENSOR_HUB)


static algo_out gsensor_algo_info = {0};
static void shm_algo_test(algo_out *algo)
{
    log_debug("%s %d", __func__, __LINE__);
    algo->steps ++;
    algo->calories = algo->steps * 0.03;
    algo->distance = 80 * algo->steps;
    algo->step_frequency = 60;
}

static int shm_algo_gsensor_init()
{
    int ret = SHM_ERR_OK;
    //get personal info from vm
    log_debug("%s %d", __func__, __LINE__);
#if !SPORT_HEALTH_ALGO_TEST_ENABLE
    sensor_service_motion_init();
#endif
    return ret;
}
static  int shm_algo_gsensor_release()
{
    int ret = SHM_ERR_OK;
    return ret;
}
static  int shm_algo_gsensor_enable()
{
    int ret = SHM_ERR_OK;
    return ret;
}
static  int shm_algo_gsensor_disable()
{
    int ret = SHM_ERR_OK;
    return ret;
}
static  int shm_algo_gsensor_update()
{
    int ret = SHM_ERR_OK;
#if SPORT_HEALTH_ALGO_TEST_ENABLE
    shm_algo_test(&gsensor_algo_info);
#else
    sensor_service_motion_get(&gsensor_algo_info);
#endif
    return ret;
}
static  int shm_algo_gsensor_set(void *priv)
{
    int ret = SHM_ERR_OK;
    //更新配置信息
#if !SPORT_HEALTH_ALGO_TEST_ENABLE
    sensor_service_motion_cfg_update();
#endif
    return ret;
}

static int shm_algo_gsensor_io_crtl(int cmd, void *priv)
{
    log_debug("%s %d", __func__, __LINE__);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        ret =  shm_algo_gsensor_init();
        break;
    case SHM_CMD_RELEASE:
        ret =  shm_algo_gsensor_release();
        break;
    case SHM_CMD_ENABLE:
        ret = shm_algo_gsensor_enable();
        break;
    case SHM_CMD_DISABLE:
        ret = shm_algo_gsensor_disable();
        break;
    case SHM_CMD_UPDATE:
    case SHM_CMD_UPDATE_SEC:
    case SHM_CMD_UPDATE_ALL:
        ret = shm_algo_gsensor_update();
        break;
    case SHM_CMD_INFO_SET:
        ret = shm_algo_gsensor_set(priv);
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}
static int shm_algo_gsensor_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
        if (priv != NULL) {
            memcpy(priv, &gsensor_algo_info, sizeof(algo_out));
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

REGISTER_SPORT_HEALTH_MODULE(gsensor_algo)
{
    .module = SHM_MOD_GSENSOR_ALGO,
     .io_ctrl = shm_algo_gsensor_io_crtl,
      .get_value = shm_algo_gsensor_get_value,
};

#endif

