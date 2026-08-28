#include "app_config.h"
#include "sport_info_sync.h"
#include "ui/ui_api.h"
#include "system/init.h"
#include "health_manager/health_manager.h"
#include "rtc/rtc_dev.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[sport]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (JL_RCSP_SENSORS_DATA_OPT)

static void sport_ui_show(u8 status)
{
    switch (status) {
    case RCSP_SPORT_STATUS_START:
        UI_SHOW_WINDOW(ID_WINDOW_SPORTING);
        break;
    case RCSP_SPORT_STATUS_PAUSE:
        if (UI_GET_WINDOW_ID() != ID_WINDOW_SPORTING) {
            UI_SHOW_WINDOW(ID_WINDOW_SPORTING);
        }
        UI_MSG_POST("sport_pause", NULL);
        break;
    case RCSP_SPORT_STATUS_CONTINNUE:
        if (UI_GET_WINDOW_ID() != ID_WINDOW_SPORTING) {
            UI_SHOW_WINDOW(ID_WINDOW_SPORTING);
        }
        UI_MSG_POST("sport_continue", NULL);
        break;
    case RCSP_SPORT_STATUS_STOP:
        UI_SHOW_WINDOW(ID_WINDOW_SPORT_RESULT);
        break;
    }
}

int execise_ctrl_status_set(unsigned char sport_mode, unsigned char status)
{
    log_info("sport_mode=%d status= %d", sport_mode, status);

    switch (status) {
    case RCSP_SPORT_STATUS_START:
        sport_health_ctrl_sport_start_with_type(sport_mode);
        break;
    case RCSP_SPORT_STATUS_PAUSE:
        sport_health_ctrl_sport_pause();
        break;
    case RCSP_SPORT_STATUS_CONTINNUE:
        sport_health_ctrl_sport_continue();
        break;
    case RCSP_SPORT_STATUS_STOP:
        sport_health_ctrl_sport_stop();
        break;
    }
    sport_ui_show(status);
    return 0;
}

u8 execise_ctrl_status_get(void)
{
    u8 run_status =	sport_health_get_sport_status();
    log_info("%s %d", __func__, run_status);
    return run_status;
}

int execise_ctrl_status_clr(void)
{

    return 0;
}

u8 execise_mode_get(void)
{
    u8 mode = sport_health_get_sport_type();
    log_info("%s %d", __func__, mode);
    if (mode > 2) {
        mode = 2;
    }
    return mode;
}

int execise_info_get_data(u8 info_type)
{
    int result;
    switch (info_type) {
    case RCSP_SPORT_DATA_STEPS:
        result = sport_health_get_sport_steps();
        break;
    case RCSP_SPORT_DATA_DISTANCE:
        result = sport_health_get_sport_distance();
        break;
    case RCSP_SPORT_DATA_KCAL:
        result = sport_health_get_sport_calories();
        break;
    case RCSP_SPORT_DATA_EXERCISE_INTENSITY:
        int hr_arg = sport_health_get_sport_hr_arg();
        if (hr_arg > 150) {
            result = 5;
        } else if (hr_arg > 140) {
            result = 4;
        } else if (hr_arg > 130) {
            result = 3;
        } else if (hr_arg > 110) {
            result = 2;
        } else if (hr_arg > 90) {
            result = 1;
        } else {
            result = 0;
        }
        break;
    case RCSP_SPORT_DATA_HR:
        result  = sport_health_get_sport_hr_real();
        break;
    case RCSP_SPORT_DATA_STEP_STRIDE:
        result =  sport_health_get_sport_step_stride();
        break;
    case RCSP_SPORT_DATA_STEP_FREQ:
        result =  sport_health_get_sport_freq();
        break;
    case RCSP_SPORT_DATA_MOTION_TIME:
        result = sport_health_get_sport_time();
        break;
    case RCSP_SPORT_DATA_SPEED:
        result = sport_health_get_sport_speed();
        break;
    default:
        result = 0;
        /* log_error("%s %d",__func__,info_type); */
        break;
    }
    return result;
}

void execise_info_clr(void)
{

}

u32 get_sport_start_time(struct sys_time *t)
{
    u32 time;
    struct sys_time *stime = malloc(sizeof(struct sys_time));

#if TCFG_APP_RTC_EN
    rtc_read_time(stime);
#endif
    time = ((stime->sec	& 0x3F) | \
            ((stime->min	& 0x3F) << 6) | \
            ((stime->hour	& 0x1F) << 12) | \
            ((stime->day	& 0x1F) << 17) | \
            ((stime->month	& 0x0F) << 22) | \
            ((stime->year	& 0x3F) << 26));

    free(stime);

    return time;
}
u32 get_sport_end_time(struct sys_time *t)
{


    return 0;
}
u16 get_sport_recode_id(void)
{

    return sport_health_get_sport_file_id();
}

u16 get_sport_recode_size(void)
{

    return sport_health_get_sport_file_size();
}

//运动总时间
int ui_sport_get_total_time(struct sys_time *t)
{
    return 0;
}

int heart_rate_last_data_get(void)
{

    return 0;
}
int execise_info_get_intensity_time(int *intensity_time, int intensity_time_size)
{
    return 0;
}

struct rcsp_watch_execise watch_execise_hd = {
    //设置运动开始/暂停/继续/结束，输入参数为运动类型
    .execise_ctrl_status_set = execise_ctrl_status_set,
    //获取当前运动状态，用于APP与UI同步
    .execise_ctrl_status_get = execise_ctrl_status_get,
    //清除当前运动状态，结束后调用
    .execise_ctrl_status_clr = execise_ctrl_status_clr,
    //获取当前运动类型，户外、室内
    .execise_mode_get = execise_mode_get,
    //运动开始到结束累积的数据
    .execise_info_get_data = execise_info_get_data,
    .execise_info_clr = execise_info_clr,
    //清除缓存运动数据
    .get_sport_start_time = get_sport_start_time,
    .get_sport_end_time = get_sport_end_time,
    .get_sport_recode_id = get_sport_recode_id,
    .get_sport_recode_size = get_sport_recode_size,
    .ui_sport_get_total_time = ui_sport_get_total_time,
    .heart_rate_last_data_get = heart_rate_last_data_get,
    .execise_info_get_intensity_time = execise_info_get_intensity_time,
};


int rcsp_sport_data_sync(void)
{
    rcsp_register_sport_info_sync_interface(&watch_execise_hd);
    return 0;
}

late_initcall(rcsp_sport_data_sync);

#endif /* if (TCFG_SPORT_HEALTH_ENABLE && JL_RCSP_SENSORS_DATA_OPT) */

