#ifndef __SHM_INFO_STORAGE_H__
#define __SHM_INFO_STORAGE_H__

#include "typedef.h"
typedef struct exercise_heart_rate_mode {
    u8 max_heart_rate;
    u8 heart_rate_type;
} e_heart_rate;

typedef struct fall_detection_mode {
    u8 phone_len;
    u8 phone_num[20];
} fall_detection_t;

#pragma pack(1)
typedef struct sport_info_personal_info {
    u16 height;
    u16 weight;

    u16 birth_y;
    u8  birth_m;
    u8  birth_d;

    u8  gender;
} personal_information;
#pragma pack()

typedef struct raise_wrist_mode {
    u8 begin_time_hour;
    u8 begin_time_min;
    u8 end_time_hour;
    u8 end_time_min;
} raise_wrist_t;

typedef struct sedentary_mode {
    u8 nop_mode;
    u8 begin_time_hour;
    u8 begin_time_min;
    u8 end_time_hour;
    u8 end_time_min;
} sedentary_t;

typedef struct sleep_detection_mode {
    u8 begin_time_hour;
    u8 begin_time_min;
    u8 end_time_hour;
    u8 end_time_min;
} sleep_detection_t;

#pragma pack(1)
// 女性健康/经期提醒 线格式（APP 侧需按同样布局解析）
typedef struct sport_info_mense {
    u8  mense_period_days;    // 经期天数
    u8  physiological_days;   // 生理周期天数
    u16 start_year;           // 最近经期开始:年
    u8  start_month;          // 月
    u8  start_day;            // 日
    u8  resp_enable;          // 提醒总开关 0/1
    u8  mense_prev_day;       // 经期提前提醒天数
    u8  ovulation_prev_day;   // 排卵提前提醒天数
    u8  easy_prev_day;        // 易孕提前提醒天数
    u8  resp_hour;            // 提醒时
    u8  resp_min;             // 提醒分
} sport_info_mense_t;
#pragma pack()



int sport_info_write_vm(int vm_id, u8 *data, u16 data_len);
int sport_info_read_vm(int vm_id, u8 *data, u16 data_len);

void sport_info_switch_record_update(u8 switch_type, u8 switch_state, u8 write_vm);
u32 sport_info_swtich_record_get(u8 switch_type);
void sport_info_mode_record_update(u8 mode_type, u8 mode);
u16 sport_info_record_get(u8 mode_type, u8 *mode_data[]);

int sport_exercise_heart_rate_get(e_heart_rate *heart_rate);
int sport_fall_detection_get(fall_detection_t *fall_detect);
int sport_personal_info_get(personal_information *info);
int sport_raise_wrist_get(raise_wrist_t *raise_wrist);
int sport_sedentary_get(sedentary_t *sedentary);
int sport_sleep_detection_get(sleep_detection_t *sleep_detection);

/* 开关薄封装：供 UI 层读取/写入开关状态，避免 UI 层引入 sport_info_opt.h 的枚举 */
int sport_info_sedentary_switch_get(void);        // 久坐提醒开关 0/1
int sport_info_sedentary_switch_set(u8 on);
int sport_info_raise_wrist_switch_get(void);      // 抬腕亮屏开关 0/1
int sport_info_raise_wrist_switch_set(u8 on);
/* 检测链路是否应使能：比 switch_get 多算「定时时段开启」位；VM 无记录时按默认值处理 */
int sport_info_raise_wrist_enabled(void);
/* 开关变更后把状态下发到检测链路（跨核 enable/disable），异步、不阻塞调用方 */
int sport_info_raise_wrist_switch_apply(void);
/* 老机器升级迁移：仅当无抬腕记录时用 legacy 值（原 UI 开关）初始化；返回 1 = 已迁移 */
int sport_info_raise_wrist_switch_migrate(u8 legacy_on);

#endif

