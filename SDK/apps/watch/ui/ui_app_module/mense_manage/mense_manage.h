
#ifndef _MENSE_MANAGE_H_
#define _MENSE_MANAGE_H_

#include "generic/typedef.h"
#include "app_config.h"
#include "system/timer.h"
#include "jiffies.h"
#include "sys_time.h"
#include "system/includes.h"
#include "timestamp/timestamp.h"
typedef enum {
    MENSE_SHOW_TYPE_SAFETY_PREIOD,              //生理间期1（经前）
    MENSE_SHOW_TYPE_MENSTRUATION,        //经期
    MENSE_SHOW_TYPE_OVULATION,           //排卵
    MENSE_SHOW_TYPE_EASY_PREGNANCY,      //易孕期1（排卵前）
} MENSE_SHOW_TYPE;

typedef enum {
    MENSE_TYPE_PREV_MENSTRUATION,   //经前
    MENSE_TYPE_MENSTRUATION,        //经期
    MENSE_TYPE_POST_MENSTRUATION,	//经后
    MENSE_TYPE_PREV_OVULATION,      //易孕期1（排卵前）
    MENSE_TYPE_OVULATION,           //排卵
    MENSE_TYPE_POST_OVULATION,		//易孕期2（排卵后）
    MENSE_TYPE_MAX,
} MENSE_TYPE;

typedef enum {
    MENSE_RESP_TYPE_NONE,               //不提醒
    MENSE_RESP_TYPE_MENSTRUATION,       //经期提醒
    MENSE_RESP_TYPE_OVULATION,          //排卵提醒
    MENSE_RESP_TYPE_EASY_PREGNANCY,		//易孕提醒
} MENSE_RESP_TYPE;

struct mense_resp {
    u32 mense_prev_day: 5;					//经期提前提醒x天数
    u32 ovulation_prev_day: 5;				//排卵
    u32 easy_prev_day: 5;					//易孕
    u32 resp_hour: 5;
    u32 resp_min: 6;
    u32 enable: 1;
};//u32

struct personal_mense_info {
    u8 mense_period_days;                   	//经期天数
    u8 physiological_days;                  	//生理周期
    struct sys_time menstruation_sday;      	//经期开始日期
    struct mense_resp resp;                 	//提醒功能
};
typedef struct personal_mense_info PERSONAL_MENSE;
struct mense_info {
    struct sys_time prev_menstruation_day;		//
    struct sys_time menstruation_day;      		//经期
    struct sys_time post_menstruation_day;
    struct sys_time prev_ovulation_day;    	//易孕期开始时间
    struct sys_time ovulation_day;          	//排卵日
    struct sys_time post_ovulation_day;    	//易孕期开始时间
    struct sys_time today;
    u8 mense_type: 3;							//当前类型，用于ui
    u8 mense_resp_type: 3;						//提醒记录
    u8 mense_resp_flag: 1;
    u8 mense_forecast_en: 1;						//预测
};
typedef struct mense_info MENSE_INFO;



//func:
int mense_personal_info_get(void *param);
int mense_personal_info_set(void *param);
int mense_day_info_get(void *param);
int mense_next_status_get(int status);
int mense_manage_init(void);
int mense_manage_release(void);
int mense_time_day_len(struct sys_time *time1, struct sys_time *time2);
void mense_time_offset_day(struct sys_time *ptime, struct sys_time *ntime, int x);
int memset_resp_status_clr();
int mense_resp_status_get();
int mense_resp_check();
int mense_status_time_update(int curr_type);
int mense_status_end_time_get(int curr_status, struct sys_time *time);
int mense_status_begin_time_get(int curr_type, struct sys_time *time);
int mense_status_map_show_type(int status);
int mense_now_status_get(void);
#endif //_MENSE_MANAGE_H_
