#ifndef _UI_SYS_PARAM__H_
#define _UI_SYS_PARAM__H_

#include "ui/ui_api.h"

#define MAX_CARD_SELECT					(16)
#define MAX_CARD_BACKUP_SELECT			(14)

#if (defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
#define LANGUAGE_NUM					(16)
#else
#define LANGUAGE_NUM					(4)
#endif

#define SYS_PARAM_NUM                   (31)

#define	TCFG_MOTO_PWM_H					(80)
#define	TCFG_MOTO_PWM_L					(30)
#define	TCFG_MOTO_PWM_NULL				(0)

#define POWERON_PASSWORD_LEN            (5)

#define MAX_LIGHTLEVEL                  (5)
#define MIN_LIGHTLEVEL                  (0)

#define UI_LIGHT_LEVEL_MAX              (10)
#define UI_LIGHT_LEVEL_MIN              (1)

enum {
    UI_PAGE_LIST_DEL,
    UI_PAGE_LIST_ADD,
    UI_PAGE_LIST_MOVE,
};

enum {
    CardSetNum,
    LightLevel,
    DarkTime,
    ShortcutKey,
    LastSysVol,
    LightAlwayEn,
    LightTime,
    AllDayUndisturbEn,
    TimeUndisturbEn,
    UndisturbStimeH,
    UndisturbStimeM,
    UndisturbEtimeH,
    UndisturbEtimeM,
    TrainAutoEn,
    ConnNewPhone,
    Language,
    MenuStyle,
    MotoMode,
    twenty_four_time,        // 24小时制
    move_mode,               // 转场动画
    raise_hand_screen_on,    // 抬手亮屏
    screen_off_dial,         // 熄屏表盘
    bed_light,               // 床头灯
    health_tips,             // 健康提醒
    low_power_mode,          // 省电模式
    curr_sel_dial,			 // 当前表盘
    shortcut_info_sel,			//快捷
    smartwin_en,				//灵动岛使能
    LastDarkTime,            // 恢复上次熄屏时长
    LastLightLevel,          // 恢复上次亮度
    VoiceMute,               // 静音开关
};

struct sys_param {
    u8 label;
    int value;
};

struct sys_vm_param {
    u8 valid;
    u8 card_select[MAX_CARD_SELECT];
    struct sys_param sys_param_table[SYS_PARAM_NUM];
};

struct set_info {
    int show_layout;

    u8 last_card_set_num;
    u8 card[MAX_CARD_BACKUP_SELECT];
    u8 vlist_card_index[4];

    u8 vslider_percent;

    /* u8 last_shake_level_sel; */

    u8 make_sure;
};

struct password_t {      // 用于密码的设置
    u8 is_password_open;
    u8 index;
    u8 status;
    u8 time_cnt;
    u32 timer_id;
    char final_password[POWERON_PASSWORD_LEN]; // 保存的密码
    char password[POWERON_PASSWORD_LEN];     // 用于键盘输入
    char new_password[POWERON_PASSWORD_LEN];     // 用于保存暂存的新密码
};

extern struct set_info set;
extern struct sys_vm_param ui_sys_param;

int write_UIInfo_to_vm(void *info);
int read_UIInfo_from_vm();
void erase_UIInfo_in_vm();
void restore_sys_settings();
void set_ui_sys_param(u8 label, int value);
int get_ui_sys_param(u8 label);
void ui_set_dark_time(u8 sel);
void screen_light_alway_switch(u8 on);
int get_light_level();
void ui_ajust_light(u8 level);
void ui_set_voice(int precent);
int ui_voice_to_percent(s8 volume);
void ui_volume_up(u8 step);
void ui_volume_down(u8 step);
void ui_voice_mute(u8 en);
u8 ui_get_voice_mute(void);
void ui_set_voice_mute(u8 en);
/* 恢复出厂时把静音复位为「不静音」并同步参数表与 VM */
void ui_voice_mute_reset(void);
/* 恢复出厂时复位密码/密码开关/静音（UI 路径与蓝牙路径共用） */
int factory_reset_user_secrets(void);
void ui_page_list_update(u32 *id_list, u8 num);
void ui_moto_init(int gpio);
void ui_moto_set_H_L(u8 mode);
void ui_moto_run(u8 run_mode);
void watch_reboot_or_shutdown(u8 flag, u8 erase);
int ui_check_list_tyep(int page_id);
u8 ui_show_minus();
u8 ui_show_plus();
u8 ui_show_shortcut_key();
bool ui_show_menu_check_win(int win_id);
int ui_show_menu_get_index(int *out_win_id);
bool ui_show_menu_force(void);
bool ui_show_menu_sw(int idx);
bool ui_show_menu_page();
bool ui_show_menu_list();
int ui_get_card_anim_by_idex(int index);
bool ui_card_anim_sw(int idx);
bool ui_card_anim_next();
u8 ui_key_shutdown_or_reboot(u8 flag);
int sys_check_time_is_undisturb(void);
int ui_show_menu_total_num(void);
void ui_backlight_restore(void);
u8 lcd_user_brightness_percent(void);

#if (defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
void csc_set_ui_language_type(u8 index);
u8 csc_get_ui_language_type(void);
#endif

#if (defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
void csc_set_ui_language_type(u8 index);
u8 csc_get_ui_language_type(void);
#endif

#endif

