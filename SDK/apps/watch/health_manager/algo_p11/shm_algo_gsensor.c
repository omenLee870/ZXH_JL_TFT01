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

#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SPORT_HEALTH_P11_ALGO_GSENSOR)

static algo_out gsensor_algo_info = {0};
static u8 gsensor_dir_cache = 0;/* 小核方向缓存（REAL_VALUE 通道回填） */

/* 收到小核方向变化时更新缓存并打印（事件型，不刷屏） */
void shm_algo_gsensor_dir_update(u8 dir)
{
    if (dir != gsensor_dir_cache) {
        log_info("dir:%d", dir);
    }
    gsensor_dir_cache = dir;
}

/* bedside 方向由小核计算上报（P11 模式；master 模式由 sensor_service_motion.c 提供） */
u8 gsensor_dir_get(void)
{
    u8 dir = 0;
    int ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_REAL_VALUE, &dir);

    if (ret != SHM_ERR_OK) {
        log_error("%s get dir fail:%d", __func__, ret);
        return 0;
    }
    return dir;
}

static int shm_algo_gsensor_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        sensor_driver_p11_ctrl(SHM_MOD_GSENSOR_ALGO, SHM_CMD_ENABLE);/* 大核控制小核开启 gsensor 算法 */
        break;
    case SHM_CMD_DISABLE:
        sensor_driver_p11_ctrl(SHM_MOD_GSENSOR_ALGO, SHM_CMD_DISABLE);/* 大核控制小核关闭 gsensor 算法 */
        break;
    case SHM_CMD_DIR_ENABLE:
        /* 床头钟方向计算开关：透传到小核 sensor_service_motion_dir_enable()，
         * 不触发 SHM_CMD_ENABLE（计步算法总闸） */
        sensor_driver_p11_ctrl(SHM_MOD_GSENSOR_ALGO, SHM_CMD_DIR_ENABLE);
        break;
    case SHM_CMD_DIR_DISABLE:
        sensor_driver_p11_ctrl(SHM_MOD_GSENSOR_ALGO, SHM_CMD_DIR_DISABLE);
        break;
    case SHM_CMD_UPDATE_VAILE_P11:
        if (priv != NULL) {
            memcpy(&gsensor_algo_info, priv, sizeof(algo_out));
            gsensor_algo_info.update.all = 1;
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
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
            sensor_driver_p11_get_data(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO);
            /* 请求-应答异步：m2p 发出后小核回填 UPDATE_VAILE_P11 更新 gsensor_algo_info */
            memcpy(priv, &gsensor_algo_info, sizeof(algo_out));
        } else {
            ret = -SHM_ERR_MOD_INVALID_PARAM;
        }
        break;
    case SHM_GET_TYPE_REAL_VALUE:
        if (priv != NULL) {
            sensor_driver_p11_get_data(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_REAL_VALUE);
            *(u8 *)priv = gsensor_dir_cache;
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

