#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-DAILY]"
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

#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_DAILY_ACTIVE)
static struct daily_active daily_active_value;
static struct daily_active daily_active_storage;
static struct daily_active daily_active_target = {
    .steps = 50000,
    .calories = 200,
    .distance = 40000,
    .stand_times = 14,
};


#define __value (&daily_active_value)
#define __storage (&daily_active_storage)
#define __target  (&daily_active_target)

static int shm_daily_active_init()
{
    int ret = SHM_ERR_OK;
    int len;
    struct daily_active tmp;
    struct sys_time time;
    rtc_read_time(&time);

    len	= syscfg_read(VM_SHM_DAILY_ACTIVE, &tmp, sizeof(struct daily_active));
    log_info("%s %d %d %d %d", __func__, len, (u32)sizeof(struct daily_active), tmp.day, time.day);
    if ((len == sizeof(struct daily_active)) && (tmp.day == time.day)) {
        memcpy(__storage, &tmp, sizeof(struct daily_active));
    } else {
        memset(__storage, 0, sizeof(struct daily_active));
    }

    len = syscfg_read(VM_SHM_DAILY_ACTIVE_TARGET, &tmp, sizeof(struct daily_active));
    if (len == sizeof(struct daily_active)) {
        memcpy(__target, &tmp, sizeof(struct daily_active));
    }

    memset(__value, 0, sizeof(struct daily_active));
    __value->day = time.day;
    __value->init = 1;

    log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __value->steps, __value->calories, __value->distance, __value->day);
    log_debug("fun:<%s> storage steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __storage->steps, __storage->calories, __storage->distance, __storage->day);
    log_debug("fun:<%s> target steps:%d cal:%d dist:%d \n", \
              __func__, __target->steps, __target->calories, __target->distance);
    return ret;
}
static int shm_daily_active_release()
{
    syscfg_write(VM_SHM_DAILY_ACTIVE, __value, sizeof(struct daily_active));
    __value->init = 0;
    log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __value->steps, __value->calories, __value->distance, __value->day);
    return SHM_ERR_OK;
}
static int shm_daily_active_save()
{
    syscfg_write(VM_SHM_DAILY_ACTIVE, __value, sizeof(struct daily_active));
    log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __value->steps, __value->calories, __value->distance, __value->day);
    return SHM_ERR_OK;
}
static int shm_daily_active_update()
{
    int ret;
    struct algo_value algo_out_value;
    struct sys_time time;
    rtc_read_time(&time);
    ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo_out_value);
    if ((__storage->day) && (time.day != __storage->day)) {
        memset(__storage, 0, sizeof(struct daily_active));
    }
    if (!ret) {
        __value->steps = __storage->steps + algo_out_value.steps;
        __value->calories = __storage->calories + algo_out_value.calories;
        __value->distance = __storage->distance + DISTANCE_MAP(algo_out_value.distance);
        __value->day = time.day;
    }

    log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __value->steps, __value->calories, __value->distance, __value->day);
    log_debug("fun:<%s> storage steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __storage->steps, __storage->calories, __storage->distance, __storage->day);
    return ret;
}
static int shm_daily_active_target_set(void *priv)
{
    int ret = SHM_ERR_OK;
    memcpy(__target, priv, sizeof(struct daily_active));
    syscfg_write(VM_SHM_DAILY_ACTIVE_TARGET, __target, sizeof(struct daily_active));
    log_debug("fun:<%s> target steps:%d cal:%d dist:%d \n", \
              __func__, __target->steps, __target->calories, __target->distance);
    return ret;
}
static int shm_daily_active_clear_value()
{
    int ret = SHM_ERR_OK;
    memset(__storage, 0, sizeof(struct daily_active));
    memset(__value, 0, sizeof(struct daily_active));
    struct sys_time time;
    rtc_read_time(&time);
    __value->day = time.day;

    log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
              __func__, __value->steps, __value->calories, __value->distance, __value->day);
    return ret;
}
static int shm_daily_active_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        ret = shm_daily_active_init();
        break;
    case SHM_CMD_RELEASE:
        ret = shm_daily_active_release();
        break;
    case SHM_CMD_UPDATE_SEC:
        ret = shm_daily_active_update();
        break;
    case SHM_CMD_TARGET_SET:
        ret = shm_daily_active_target_set(priv);
        break;
    case  SHM_CMD_CLEAR_VALUE:
        ret =  shm_daily_active_clear_value();
        break;
    case SHM_CMD_SAVE_SINGLE:
    case SHM_CMD_SAVE_CONTINUE:
        ret = shm_daily_active_save();
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}

static int shm_daily_active_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
        memcpy(priv, __value, sizeof(struct daily_active));
        log_debug("fun:<%s> real value steps:%d cal:%d dist:%d day:%d\n", \
                  __func__, __value->steps, __value->calories, __value->distance, __value->day);
        break;
    case SHM_GET_TYPE_TARGET:
        memcpy(priv, __target, sizeof(struct daily_active));
        log_debug("fun:<%s> target steps:%d cal:%d dist:%d \n", \
                  __func__, __target->steps, __target->calories, __target->distance);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(daily_active)
{
    .module =  SHM_MOD_DAILY_ACTIVE,
     .io_ctrl =  shm_daily_active_io_crtl,
      .get_value =  shm_daily_active_get_value,
};

#endif
