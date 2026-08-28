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

#endif

