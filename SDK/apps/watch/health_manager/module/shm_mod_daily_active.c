#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "data_storage/data_storage.h"
#include "rtc/rtc_dev.h"


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

#define DAILY_ACTIVE_FILE_TYPE			F_TYPE_STEP			/* 步数文件：整点落盘，跨天按日期新建 */
#define DAILY_ACTIVE_FILE_VERSION		(0)
#define DAILY_ACTIVE_FILE_INTERVAL		(0Xff)
static struct daily_active daily_active_value;
static struct daily_active daily_active_storage;
/* 最近一次成功更新算法值时的完整时刻，供跨天收尾定位"上一天"
 * （只存 day 无法区分跨月/跨年同号的情况） */
static struct sys_time s_last_time;
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

/* 把某日某时刻的步数快照写入该日期的文件（不存在则新建）。
 * 供整点落盘与关机落盘复用，避免两处重复维护文件格式。 */
static int shm_daily_active_write_record(u16 year, u8 month, u8 day, u8 hour, u8 min, u32 steps)
{
    int file_len = 0;
    void *fp = sport_health_file_open_by_time(DAILY_ACTIVE_FILE_TYPE, year, month, day);
    if (!fp) {
        /* 该日期文件不存在：新建并写入文件总头 */
        struct health_file_total_head file_head = {
            .type = DAILY_ACTIVE_FILE_TYPE,
            .year = year,
            .month = month,
            .day = day,
            .crc = hour,/* 仅需每次不同，不做实际校验（对齐 bo_mode_file_save） */
            .interval = DAILY_ACTIVE_FILE_INTERVAL,
            .version = DAILY_ACTIVE_FILE_VERSION,
            .reserve = 0,
        };
        fp = sport_health_file_open(DAILY_ACTIVE_FILE_TYPE, 0);
        if (!fp) {
            log_error("%s file open fail", __func__);
            return -SHM_ERR_FILE_NOT_FIND;
        }
        sport_health_common_swapX((u8 *)&file_head.year, (u8 *)&file_head.year, 2);
        file_len = sport_health_file_write(fp, (u8 *)&file_head, file_len, sizeof(struct health_file_total_head));
        if (file_len <= 0) {
            sport_health_file_close(fp);
            return -SHM_ERR_DATA_ERR;
        }
    } else {
        file_len = sport_health_file_get_len(fp);
    }

    /* 一条数据头（时刻）+ 该时刻的累计步数 */
    struct health_file_data_head data_head = {
        .hour = hour,
        .min = min,
        .len = sizeof(u32),
    };
    sport_health_common_swapX((u8 *)&data_head.len, (u8 *)&data_head.len, 2);
    file_len = sport_health_file_write(fp, (u8 *)&data_head, file_len, sizeof(struct health_file_data_head));
    if (file_len <= 0) {
        sport_health_file_close(fp);
        return -SHM_ERR_DATA_ERR;
    }
    file_len = sport_health_file_write(fp, (u8 *)&steps, file_len, sizeof(steps));
    if (file_len <= 0) {
        sport_health_file_close(fp);
        return -SHM_ERR_DATA_ERR;
    }
    sport_health_file_close(fp);

    log_info("fun:<%s> [%d-%d-%d %d:%d] steps:%d file_len:%d", __func__, year, month, day, hour, min, steps, file_len);
    return SHM_ERR_OK;
}

/* 整点把当日步数落盘到按日期的文件（跨天新建），供 daily active 历史记录 */
static int shm_daily_active_hour_file()
{
    static u8 last_hour = 0xff;/* 去重：UPDATE_SEC 每秒触发，整点这一分钟内只允许落盘一次 */
    static u8 last_day  = 0xff;
    struct sys_time time;

    rtc_read_time(&time);
    if (time.min != 0) {
        return SHM_ERR_OK;/* 仅整点（0 分）执行 */
    }
    if ((time.hour == last_hour) && (time.day == last_day)) {
        return SHM_ERR_OK;/* 该整点已落盘 */
    }

    int ret = shm_daily_active_write_record(time.year, time.month, time.day, time.hour, 0, __value->steps);
    if (ret == SHM_ERR_OK) {
        /* 仅落盘成功才记去重标记：失败时本分钟内（time.min==0）会继续重试 */
        last_hour = time.hour;
        last_day  = time.day;
    }
    return ret;
}

/* 关机/主动保存：不受整点门控限制，按当前时刻写一条快照。 */
static int shm_daily_active_flush_now()
{
    struct sys_time time;

    rtc_read_time(&time);
    return shm_daily_active_write_record(time.year, time.month, time.day,
                                         time.hour, time.min, __value->steps);
}

/* 跨天收尾：把上一天最后一组步数补写进上一天的文件。
 * 调用点在 shm_daily_active_update() 清零 __storage 之前，此时 __value 仍是上一天
 * 的最终值；用 s_last_time 定位"上一天"，跨月/跨年也正确。 */
static void shm_daily_active_close_prev_day()
{
    if ((s_last_time.year == 0) || (s_last_time.day == 0)) {
        return;/* 本次开机尚未成功更新过，没有"上一天"可收尾 */
    }
    shm_daily_active_write_record(s_last_time.year, s_last_time.month, s_last_time.day,
                                  s_last_time.hour, s_last_time.min, __value->steps);
}

static int shm_daily_active_update()
{
    int ret;
    struct algo_value algo_out_value;
    struct sys_time time;
    rtc_read_time(&time);
    ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo_out_value);
    if (!ret) {
        /* 跨天判据用 s_last_time 而非 __storage->day（后者 memset 后不回填，会令同一
         * 开机内第二次跨天漏收尾）；比较完整日期，跨月/跨年同号也能识别。
         * 放在 !ret 分支内：取数失败时不收尾、不累加、不推进 s_last_time。 */
        if (s_last_time.year && s_last_time.day &&
            ((s_last_time.year != time.year) || (s_last_time.month != time.month) ||
             (s_last_time.day != time.day))) {
            /* 跨天：先把上一天最后一组数据补写进上一天的文件，再清零当日累计。
             * 此时 __value 仍是上一天的最终值。 */
            shm_daily_active_close_prev_day();
            memset(__storage, 0, sizeof(struct daily_active));
            /* 基线日期与当日累计口径保持一致，供日志与后续判据参考 */
            __storage->day = time.day;
        }
        __value->steps = __storage->steps + algo_out_value.steps;
        __value->calories = __storage->calories + algo_out_value.calories;
        __value->distance = __storage->distance + DISTANCE_MAP(algo_out_value.distance);
        __value->day = time.day;
        /* 记录最近一次成功更新的完整时刻，供跨天收尾定位"上一天" */
        memcpy(&s_last_time, &time, sizeof(struct sys_time));
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
        shm_daily_active_hour_file();/* 整点（0 分）落盘当日步数（跨天按日期新建） */
        break;
    case SHM_CMD_TARGET_SET:
        ret = shm_daily_active_target_set(priv);
        break;
    case  SHM_CMD_CLEAR_VALUE:
        ret =  shm_daily_active_clear_value();
        break;
    case SHM_CMD_SAVE_SINGLE:
        /* 关机/主动保存——除 VM 掉电保存外，再按当前时刻写一条文件快照，
         * 不受整点门控限制。 */
        ret = shm_daily_active_save();
        shm_daily_active_flush_now();
        break;
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
