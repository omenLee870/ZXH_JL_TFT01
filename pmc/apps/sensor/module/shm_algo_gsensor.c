
#include "health_manager.h"
#if 1
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
#define LOG_TAG     		"[ALGO_GSENSOR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"




#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&&TCFG_SENSOR_ALGO_GSENSOR_ENABLE)

#define SPORT_HEALTH_ALGO_TEST_ENABLE	0

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
    sensor_service_init(SHM_MOD_GSENSOR_ALGO);
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
    printf("%s %d", __func__, __LINE__);
    struct algo_value algo;
    sensor_service_get_result(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_REAL_VALUE, &algo);

    algo.steps += gsensor_algo_info.steps;
    algo.calories += gsensor_algo_info.calories;
    algo.distance += gsensor_algo_info.distance;

    memcpy(&gsensor_algo_info, &algo, sizeof(algo_out));

#endif
    return ret;
}
static  int shm_algo_gsensor_set(void *priv)
{
    int ret = SHM_ERR_OK;
    //更新配置信息
#if !SPORT_HEALTH_ALGO_TEST_ENABLE
    sensor_service_set_config(SHM_MOD_GSENSOR_ALGO, SHM_CMD_INFO_SET, NULL);
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
#if  0//取数后清0，由master做累加
            memset(&gsensor_algo_info, 0, sizeof(algo_out));
#endif
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

