#ifndef __SPORT_INFO_SYNC_H__
#define __SPORT_INFO_SYNC_H__

#include "typedef.h"
#include "sys_time.h"

enum {
    RCSP_SPORT_STATUS_NULL,
    RCSP_SPORT_STATUS_START,
    RCSP_SPORT_STATUS_PAUSE,
    RCSP_SPORT_STATUS_CONTINNUE,
    RCSP_SPORT_STATUS_STOP
};//SPORT_STATUS
enum {
    RCSP_SPORT_DATA_NULL,
    RCSP_SPORT_DATA_STEPS,
    RCSP_SPORT_DATA_DISTANCE,
    RCSP_SPORT_DATA_MOTION_TIME,
    RCSP_SPORT_DATA_SPEED,
    RCSP_SPORT_DATA_KCAL,
    RCSP_SPORT_DATA_STEP_FREQ,
    RCSP_SPORT_DATA_STEP_STRIDE,
    RCSP_SPORT_DATA_EXERCISE_INTENSITY,
    RCSP_SPORT_DATA_HR,
};


//运动
struct rcsp_watch_execise {
    int (*execise_ctrl_status_set)(unsigned char execise_mode, unsigned char status);			//设置运动开始/暂停/继续/结束，输入参数为运动类型
    unsigned char (*execise_ctrl_status_get)(void);					//获取当前运动状态，用于APP与UI同步
    int (*execise_ctrl_status_clr)(void);					//清除当前运动状态，结束后调用
    unsigned char (*execise_mode_get)(void);							//获取当前运动类型，户外、室内
    int (*execise_info_get_data)(u8 info_type);		//运动开始到结束累积的数据
    void (*execise_info_clr)(void);							//清除缓存运动数据
    u32(*get_sport_start_time)(struct sys_time *t);
    u32(*get_sport_end_time)(struct sys_time *t);
    u16(*get_sport_recode_id)(void);
    u16(*get_sport_recode_size)(void);
    int (*ui_sport_get_total_time)(struct sys_time *t);
    int (*heart_rate_last_data_get)(void);
    int (*execise_info_get_intensity_time)(int *intensity_time, int intensity_time_size);
};


int JL_rcsp_sports_info_sync_funciton(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);
int sport_info_sync_keep_exercise_rcsp(void);
int sport_info_sync_pause_exercise_rcsp(void);
int sport_info_sync_end_exercise_by_fw(void);

// 当data是NULL时，就表示固件主动推送数据给app
int sport_info_sync_start_exercise_rcsp(u8 *data, u16 *data_len);

int rcsp_register_sport_info_sync_interface(struct rcsp_watch_execise *sport_info_sync_interface);
#endif
