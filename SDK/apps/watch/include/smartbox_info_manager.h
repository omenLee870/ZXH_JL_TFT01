#ifndef _SMARTBOX_INFO_MANAGER_H_
#define _SMARTBOX_INFO_MANAGER_H_


#include "typedef.h"
#include "utils/sys_time.h"

enum {
    OUT_OF_BOX = 0, // 不在仓
    IN_BOX = 1,     // 在仓
};

enum {
    LID_OPEN = 1,   // 开盖
    LID_CLOSE,      // 关盖
};

struct s_box_info {
    u8 app_state;                   // app连接状态  0:不连接 1：连接
    u8 phone_state ;                // 手机连接状态，0:不连接 1：连接
    u8 earphone_state;              // 耳机连接状态，0:不连接 1：连接
    u8 l_ear_bat ;                  // L耳电量
    u8 r_ear_bat ;                  // R耳电量
    u8 l_inbox;                     // L耳入仓
    u8 r_inbox;                     // R耳入仓
    s8 box_bat ;                    // 充电仓电量
    u8 music_state;                 // 耳机播放状态
    u8 earphone_vol ;               // 耳机音量
    u8 anc_mode ;                   // anc模式，0关 1开 2通透
    u8 eq_mode;                     // eq模式
    u8 ui_bl_levl;                  // 仓背光亮度
    u8 l_charging;                  // L耳充电状态
    u8 r_charging;                  // R耳充电状态
    u8 box_charging;                // 充电仓充电状态
    u8 l_finding;                   // 查找L耳
    u8 r_finding;                   // 查找R耳
    u8 box_clid_status;             // 充电仓开关盖状态
    u8 language;                    // 语言
    u8 low_power;                   // 低电状态
    u8 phone_call_state;            // 通话状态
    u8 phone_call_mute;             // 通话是否静音
    u8 local_page;                  // 本地页面记录
    u8 emitter_status;              // 蓝牙发射器状态
    u8 box_touch_switch;            // 充电仓触摸开关
    u8 box_voice_switch;            // 充电仓触摸开关
    u8 llatenyc_on;                 //  低延时模式
    struct sys_time phone_time;     // 系统时间
    u8 key_info[8];     //按键信息
    u8 eq_gain[10];                 //
};


typedef struct __custom_edr_info {
    u8 emitter_addr[6];

} custom_edr_info __attribute__((aligned(4)));


u8 sbox_bt_app_flag_get(void);
/*耳机与手机连接状态*/
u8 sbox_bt_connect_flag_get(void);
/*仓与耳机连接状态*/
u8 sbox_ble_connect_flag_get(void);
u8 sbox_battery_left_get(void);
u8 sbox_battery_right_get(void);
u8 sbox_battery_box_get(void);
u8 sbox_music_ui_state_get(void);
u8 sbox_volume_get(void);
u8 sbox_anc_mode_get(void);
u8 sbox_equalizer_mode_get(void);
u8 sbox_backlight_level_get(void);
u8 sbox_left_charging_get(void);
u8 sbox_right_charging_get(void);
u8 sbox_box_charging_get(void);
u8 sbox_left_finding_get(void);
u8 sbox_right_finding_get(void);
u8 sbox_box_clid_status_get(void);
u8 sbox_language_ui_get(void);
u8 sbox_low_power_get(void);
u8 sbox_phone_call_state_get(void);
u8 sbox_phone_call_mute_get(void);
u8 sbox_box_hour_get(void);
u8 sbox_box_minute_get(void);
u8 sbox_box_second_get(void);
u8 sbox_box_time_get(struct sys_time *time);
u8 sbox_local_page_get(void);
u8 sbox_emitter_state_get(void);
u8 sbox_r_inbox_get(void);
u8 sbox_l_inbox_get(void);

void sbox_bt_connect_flag_set(u8 data);
void sbox_ble_connect_flag_set(u8 data);
void sbox_battery_left_set(u8 data);
void sbox_battery_right_set(u8 data);
void sbox_battery_box_set(u8 data);
void sbox_music_ui_state_set(u8 data);
void sbox_volume_set(u8 data);
void sbox_anc_mode_set(u8 data);
void sbox_equalizer_mode_set(u8 data);
void sbox_backlight_level_set(u8 data);
void sbox_left_charging_set(u8 data);
void sbox_right_charging_set(u8 data);
void sbox_box_charging_set(u8 data);
void sbox_left_finding_set(u8 data);
void sbox_right_finding_set(u8 data);
void sbox_box_clid_status_set(u8 data);
void sbox_language_ui_set(u8 data);
void sbox_low_power_set(u8 data);
void sbox_phone_call_state_set(u8 data);
void sbox_phone_call_mute_set(u8 data);
void sbox_box_year_set(u8 data);
void sbox_box_month_set(u8 data);
void sbox_box_day_set(u8 data);
void sbox_box_hour_set(u8 data);
void sbox_box_minute_set(u8 data);
void sbox_box_second_set(u8 data);
void sbox_local_page_set(u8 data);
void sbox_emitter_state_set(u8 state);
void sbox_r_inbox_set(u8 state);
void sbox_l_inbox_set(u8 state);
u8 *sbox_emitter_addr_get(void);
void ear_inbox_state_deal(u8 status);
void sbox_phone_time_set(u16 year, u8 month, u8 day, u8 hour, u8 min, u8 sec);


#endif

