#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"
#include "rtc/rtc_dev.h"
#include "health_manager/health_manager.h"


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


#define BIG_LITTLE_SWAP16(x)        ( (((*(u16*)x) & 0xff00) >> 8) | \
                                      (((*(u16*)x) & 0x00ff) << 8) )

#define BIG_LITTLE_SWAP32(x)        ( (((*(u32*)x) & 0xff000000) >> 24) | \
                                      (((*(u32*)x) & 0x00ff0000) >> 8) | \
                                      (((*(u32*)x) & 0x0000ff00) << 8) | \
                                      (((*(u32*)x) & 0x000000ff) << 24) )

void sport_health_common_swapX(const uint8_t *src, uint8_t *dst, int32_t len)
{
    if (len  == 2) {
        BIG_LITTLE_SWAP16(dst);
    } else if (len  == 4) {
        BIG_LITTLE_SWAP32(dst);
    } else {
        ASSERT(0);
    }

}

int sport_health_get_value_daily_active(struct daily_active *data)
{
    ASSERT(data);
    return sport_health_manager_value_get(SHM_MOD_DAILY_ACTIVE, SHM_GET_TYPE_INFO, data);

}
int sport_health_get_value_daily_steps()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active(&data)) {
        return data.steps;
    }
    return 0;
}
int sport_health_get_value_daily_calories()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active(&data)) {
        return data.calories;
    }
    return 0;
}
int sport_health_get_value_daily_distance()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active(&data)) {
        return data.distance / 100; //转m
    }
    return 0;
}
int sport_health_get_value_daily_stand_times()
{
    /* struct daily_active data; */
    /* if(!sport_health_get_value_daily_active(&data)){ */
    /* return data.stand_times; */
    /* } */
    return 0;
}
int sport_health_get_value_daily_active_target(struct daily_active *data)
{
    ASSERT(data);
    return sport_health_manager_value_get(SHM_MOD_DAILY_ACTIVE, SHM_GET_TYPE_TARGET, data);

}
int sport_health_get_value_daily_target_steps()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active_target(&data)) {
        return data.steps;
    }
    return 0;
}
int sport_health_get_value_daily_target_calories()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active_target(&data)) {
        return data.calories;
    }
    return 0;
}
int sport_health_get_value_daily_target_distance()
{
    struct daily_active data;
    if (!sport_health_get_value_daily_active_target(&data)) {
        return data.distance;
    }
    return 0;
}
int sport_health_get_value_daily_target_stand_times()
{
    /* struct daily_active data; */
    /* if(!sport_health_get_value_daily_active_target(&data)){ */
    /* return data.stand_times; */
    /* } */
    return 0;
}
int sport_health_set_daily_active_target(struct daily_active *data)
{
    return sport_health_manager_msg_post(SHM_MOD_DAILY_ACTIVE, SHM_CMD_TARGET_SET, (void *)data, 1);

}
int sport_health_ctrl_sport_start_with_type(int type)
{
    //设置运动类型
    int ret = sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_INFO_SET, (void *)type, 1);
    ASSERT(!ret, "ret:%d", ret);
    //开始运动
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_START, NULL, 1);
}
int sport_health_ctrl_sport_start()
{
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_START, NULL, 1);
}
int sport_health_ctrl_sport_pause()
{
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_PAUSE, NULL, 1);
}
int sport_health_ctrl_sport_continue()
{
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_CONTINUE, NULL, 1);
}
int sport_health_ctrl_sport_stop()
{
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_STOP, NULL, 1);
}
int sport_health_set_sport_type(int type)
{
    return sport_health_manager_msg_post(SHM_MOD_SPORT, SHM_CMD_INFO_SET, (void *)type, 1);
}
int sport_health_get_sport_info(struct sport_value *data)
{
    ASSERT(data);
    return sport_health_manager_value_get(SHM_MOD_SPORT, SHM_GET_TYPE_INFO, (void *)data);
}
int sport_health_get_sport_status()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.status;
    }
    return 0;
}
int sport_health_get_sport_type()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.type;
    }
    return 0;
}
int sport_health_get_sport_steps()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.steps_c;
    }
    return 0;
}
int sport_health_get_sport_calories()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.calories_c;
    }
    return 0;
}
/*根据rcsp协议，单位统一为0.01公里*/
int sport_health_get_sport_distance()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.distance_c / 1000;
    }
    return 0;
}
int sport_health_get_sport_freq()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.step_freq_c;
    }
    return 0;
}
int sport_health_get_sport_time()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.run_sec;
    }
    return 0;
}
int sport_health_get_sport_file_total()
{
    int total = 0;
    sport_health_manager_value_get(SHM_MOD_SPORT, SHM_GET_TYPE_STORAGE_TOTAL, &total);
    return total;
}
int sport_health_get_sport_file_value(struct sport_value *value)
{
    return sport_health_manager_value_get(SHM_MOD_SPORT, SHM_GET_TYPE_INFO_STORAGE, value);
}
int sport_health_get_sport_hr_real()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.heart_real;
    }
    return 0;
}
int sport_health_get_sport_hr_max()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.heart_max;
    }
    return 0;
}
int sport_health_get_sport_hr_min()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.heart_min;
    }
    return 0;
}
int sport_health_get_sport_hr_arg()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.heart_val;
    }
    return 0;
}
int sport_health_get_sport_speed()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.speed_c * 3.6; //cm/s to 0.01km/h
    }
    return 0;
}
int sport_health_get_sport_step_stride()
{
    struct sport_value info;
    int ret =  sport_health_get_sport_info(&info);
    if (!ret) {
        return info.step_stride_c;
    }
    return 0;
}
int sport_health_get_sport_file_id()
{
    u16 file_id = 0;
    sport_health_manager_value_get(SHM_MOD_SPORT, SHM_GET_TYPE_STORAGE_ID, &file_id);
    return file_id;
}
int sport_health_get_sport_file_size()
{
    u16 file_id = 0;
    sport_health_manager_value_get(SHM_MOD_SPORT, SHM_GET_TYPE_STORAGE_ID, &file_id);
    if (file_id) {
        void *fp  = sport_health_file_open(F_TYPE_SPORTRECORD, file_id);
        int file_len = 0;
        if (fp) {
            file_len = sport_health_file_get_len(fp);
            sport_health_file_close(fp);
        }
        return file_len;
    }
    return 0;
}
int sport_health_gsensor_algo_cfg_update()
{
    return sport_health_manager_msg_post(SHM_MOD_GSENSOR_ALGO, SHM_CMD_INFO_SET, NULL, 1);
}

int sport_health_heart_rate_module_enable()
{
    return sport_health_manager_msg_post(SHM_MOD_HEART_RATE, SHM_CMD_ENABLE, NULL, 1);
}
int sport_health_heart_rate_module_disable()
{
    return sport_health_manager_msg_post(SHM_MOD_HEART_RATE, SHM_CMD_DISABLE, NULL, 1);
}
int sport_health_heart_rate_module_clr()
{
    return sport_health_manager_msg_post(SHM_MOD_HEART_RATE, SHM_CMD_CLEAR_VALUE, NULL, 1);
}
int sport_health_heart_rate_get_cur()
{
    int heart_rate = 0;
    sport_health_manager_value_get(SHM_MOD_HEART_RATE, SHM_GET_TYPE_REAL_VALUE, &heart_rate);
    return heart_rate;
}
int sport_health_heart_rate_get_min()
{
    int heart_rate = 0;
    sport_health_manager_value_get(SHM_MOD_HEART_RATE, SHM_GET_TYPE_MIN_VALUE, &heart_rate);
    return heart_rate;
}
int sport_health_heart_rate_get_max()
{
    int heart_rate = 0;
    sport_health_manager_value_get(SHM_MOD_HEART_RATE, SHM_GET_TYPE_MAX_VALUE, &heart_rate);
    return heart_rate;
}
int sport_health_heart_rate_get_avg()
{
    int heart_rate = 0;
    sport_health_manager_value_get(SHM_MOD_HEART_RATE, SHM_GET_TYPE_AVG_VALUE, &heart_rate);
    return heart_rate;
}
int sport_health_heart_rate_get_rec(struct health_file_data_info *info)
{
    return sport_health_manager_value_get(SHM_MOD_HEART_RATE, SHM_GET_TYPE_REC_VALUE, info);
}


int sport_health_blood_oxygen_module_enable()
{
    return sport_health_manager_msg_post(SHM_MOD_BLOOD_OXYGEN, SHM_CMD_ENABLE, NULL, 1);
}
int sport_health_blood_oxygen_module_disable()
{
    return sport_health_manager_msg_post(SHM_MOD_BLOOD_OXYGEN, SHM_CMD_DISABLE, NULL, 1);
}
int sport_health_blood_oxygen_module_clr()
{
    return sport_health_manager_msg_post(SHM_MOD_BLOOD_OXYGEN, SHM_CMD_CLEAR_VALUE, NULL, 1);
}
int sport_health_blood_oxygen_get_cur()
{
    int blood_oxygen = 0;
    sport_health_manager_value_get(SHM_MOD_BLOOD_OXYGEN, SHM_GET_TYPE_REAL_VALUE, &blood_oxygen);
    return blood_oxygen;
}
int sport_health_blood_oxygen_get_min()
{
    int blood_oxygen = 0;
    sport_health_manager_value_get(SHM_MOD_BLOOD_OXYGEN, SHM_GET_TYPE_MIN_VALUE, &blood_oxygen);
    return blood_oxygen;
}
int sport_health_blood_oxygen_get_max()
{
    int blood_oxygen = 0;
    sport_health_manager_value_get(SHM_MOD_BLOOD_OXYGEN, SHM_GET_TYPE_MAX_VALUE, &blood_oxygen);
    return blood_oxygen;
}
int sport_health_blood_oxygen_get_avg()
{
    int blood_oxygen = 0;
    sport_health_manager_value_get(SHM_MOD_BLOOD_OXYGEN, SHM_GET_TYPE_AVG_VALUE, &blood_oxygen);
    return blood_oxygen;
}
int sport_health_blood_oxygen_get_rec(struct health_file_data_info *info)
{
    return sport_health_manager_value_get(SHM_MOD_BLOOD_OXYGEN, SHM_GET_TYPE_REC_VALUE, info);
}


int sport_health_sleep_data_get(struct sleep_data_analysis *info, int analysis_en, struct sys_time *time)
{
    if (!time) {
        rtc_read_time(&info->file_time);
    } else {
        memcpy(&info->file_time, time, sizeof(struct sys_time));
    }
    int ret = sport_health_manager_value_get(SHM_MOD_SLEEP, SHM_GET_TYPE_INFO_STORAGE, (void *)info);
    if (ret) {
        return ret;
    }
    if (analysis_en) {
        ret = sport_health_manager_value_get(SHM_MOD_SLEEP,  SHM_GET_TYPE_DATA_ANALYSIS, (void *)info);

    }
    return ret;
}













