#include "app_config.h"
#include "system/includes.h"
#include "jlui_app/ui_sys_param.h"
#include "jlui_app/watch_syscfg_manage.h"
#include "periph/pwm_interface.h"
#include "audio_config.h"
#include "automute.h"
#include "jlui_app/ui_style.h"
#include "ui_api.h"
#include "cat1/cat1_common.h"
#include "ui_page_switch.h"
#include "a2dp_player.h"
#include "vol_sync.h"
#include "avctp_user.h"
#include "rtc.h"
#include "data_storage.h"
#include "le/ble_api.h"
#include "font/language_list.h"
#include "alarm.h"
#include "smartbox_info_manager.h"
#include "smartbox_user_app.h"
#if TCFG_PAY_ALIOS_ENABLE
#include "alipay.h"
#endif


#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI_SYS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_UI_ENABLE

#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))

extern const int SCALE_EFFECT_WITHOUT_PSRAM_ENABLE;
extern int music_is_play(void);
extern int sport_health_manager_release();
struct set_info set = {0};
int UIInfo_w_vm_timer = 0;

static u16 moto_time_id = 0;
static u8 moto_tmr_cnt = 0;
static u8 moto_mode = 0;

struct sys_vm_param ui_sys_param;
struct pwm_if_handle moto_pwm_p_data;

struct sys_param sys_defalut_param[SYS_PARAM_NUM] = {
    {CardSetNum,          0},
    {LightLevel,          5},
    {DarkTime,            0},
    {ShortcutKey,         0},
    {LastSysVol,          60},
    {LightAlwayEn,        0},
    {LightTime,           0},
    {AllDayUndisturbEn,   0},
    {TimeUndisturbEn,     0},
    {UndisturbStimeH,     0},
    {UndisturbStimeM,     0},
    {UndisturbEtimeH,     0},
    {UndisturbEtimeM,     0},
    {TrainAutoEn,         0},
    {ConnNewPhone,        0},
    {Language,            Chinese_Simplified},   //修复: 原为 0，而语言ID从 1 起算；0 会让 ui_language_set() 算 BIT(-1) 未定义行为
    {MenuStyle,           0},
    {MotoMode,           80},
    {twenty_four_time,    1},
    {move_mode,           0},
    {raise_hand_screen_on, 0},
    {screen_off_dial,     0},
    {bed_light,           0},
    {health_tips,         0},
    {low_power_mode,      0},
    {curr_sel_dial,		  0},
    {shortcut_info_sel,	  0},
    {smartwin_en,		  0},
    {LastDarkTime,		  0},
    {LastLightLevel,	  5},
    {VoiceMute,           0},
};

// 应用列表类型
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
const static int app_list_type_tab[] = {
    //ID_WINDOW_MENU_LIST,// 列表
    //ID_WINDOW_MENU_STAR,// 蜂窝
    ID_WINDOW_H_LIST,   //弧形菜单
    ID_WINDOW_FOOTBALL,// 3D足球
};
#else
const static int app_list_type_tab[] = {
    ID_WINDOW_MENU_LIST,// 列表
    ID_WINDOW_MENU_STAR,// 蜂窝
    ID_WINDOW_FOOTBALL,// 3D足球
};
#endif

// 卡片过渡动画类型
const static int card_anim_type_tab[] = {
    PAGE_MOVE_MODE_DOUBLE_PAGE,     // 平移
#if TCFG_UI_EFFECT_USED_DEMO
    PAGE_MOVE_MODE_USER,			//缩放+alpha
#else
    PAGE_MOVE_MODE_SCALE_DOUBLE,    // 缩放
#endif
    PAGE_MOVE_MODE_FLIP,            // 翻页
    PAGE_MOVE_MODE_CUBE,            // 3D立方体
    PAGE_MOVE_MODE_CENTER_FLIP,     // 中心轴翻转
    PAGE_MOVE_MODE_HEXAGON,         // 3D灯笼
    PAGE_MOVE_MODE_EDGE_FLIP,	    // 边沿翻转
    PAGE_MOVE_MODE_CUBE_FLIP,       // 立方体翻页(+ 倒影)
    PAGE_MOVE_MODE_REFLECTION,      // 3D灯笼 + 倒影
    PAGE_MOVE_MODE_CUBE_REFLECTION, // 立方体(灯笼) + 倒影
    PAGE_MOVE_MODE_BOARD_FLIP,      // 翻板翻转特效
    PAGE_MOVE_MODE_DRIFT_FLIP,      // 漂移翻页(+ 倒影)
    //PAGE_MOVE_MODE_BOARD_SLICING_FLIP,  // 切片翻板翻转特效( TODO, 暂时无对应的 UI 前端 )
};


extern void sys_enter_soft_poweroff(void *priv);
extern u8 get_ui_page_list_total_num();
static u8 ui_style_vm_init(void);
static u8 ui_card_anim_vm_init(void);
/* 静音开关的出厂复位，定义见本文件 voice_mute 段 */
void ui_voice_mute_reset(void);

extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;

static s16 get_tone_vol()
{
    return app_audio_get_volume(APP_AUDIO_STATE_WTONE);
}

int ui_check_list_tyep(int page_id)
{
    for (int i = 0; i < ARRAY_SIZE(app_list_type_tab); i++) {
        if (app_list_type_tab[i] == page_id) {
            return true;
        }
    }
    return false;
}

void set_ui_sys_param(u8 label, int value)
{
    u8 i;
    for (i = 0; i < SYS_PARAM_NUM; i++) {
        if (label == ui_sys_param.sys_param_table[i].label) {
            ui_sys_param.sys_param_table[i].value = value;
            return;
        }
    }

    printf("not found sys param\n");
}

int get_ui_sys_param(u8 label)
{
    u8 i;
    for (i = 0; i < SYS_PARAM_NUM; i++) {
        if (label == ui_sys_param.sys_param_table[i].label) {
            return ui_sys_param.sys_param_table[i].value;
        }
    }

    printf("not found sys param\n");
    return -1;
}

int write_UIInfo_to_vm(void *info)
{
    int ret = 0;
    g_printf("write_UIInfo_to_vm");
    if (UIInfo_w_vm_timer != 0) {
        sys_timer_del(UIInfo_w_vm_timer);
        UIInfo_w_vm_timer = 0;
    }
    ui_sys_param.valid = 1;
    if ((int)info == (int)SYSCFG_WRITE_ERASE_STATUS) {
        ui_sys_param.valid = 0;
    }
    ret = syscfg_write(VM_UI_SYS_INFO, &ui_sys_param, sizeof(ui_sys_param));
    if (ret != sizeof(ui_sys_param)) {
        printf("write ui_sysinfo VM err\n");
        return -1;
    }

    return ret;
}

int read_UIInfo_from_vm()
{
    int ret = 0;
    u8 ui_sys_param_vaild = 0;
    g_printf("read_UIInfo_from_vm");
    ret = syscfg_read(VM_UI_SYS_INFO, &ui_sys_param_vaild, sizeof(ui_sys_param_vaild));
    if ((ret != sizeof(ui_sys_param_vaild)) || (ui_sys_param_vaild == 0)) {
        printf("ui_sysinfo invalid\n");
        memcpy(&ui_sys_param.sys_param_table, &sys_defalut_param, sizeof(sys_defalut_param));
        ui_style_vm_init();
        ui_card_anim_vm_init();
        return -1;
    }
    ret = syscfg_read(VM_UI_SYS_INFO, &ui_sys_param, sizeof(ui_sys_param));
    if (ret != sizeof(ui_sys_param)) {
        printf("read ui_sysinfo VM err\n");
        memcpy(&ui_sys_param.sys_param_table, &sys_defalut_param, sizeof(sys_defalut_param));
        ui_style_vm_init();
        ui_card_anim_vm_init();
        return -2;
    }
    //set_ui_sys_param(LastSysVol, app_audio_get_volume(APP_AUDIO_STATE_WTONE));
    ui_style_vm_init();
    ui_card_anim_vm_init();
    return ret;
}

void erase_UIInfo_in_vm()
{
    int ret = 0;
    ui_sys_param.valid = 0;
    ret = syscfg_write(VM_UI_SYS_INFO, &ui_sys_param, sizeof(ui_sys_param));
    if (ret != sizeof(ui_sys_param)) {
        printf("erase ui_sysinfo VM err\n");
    }
}

/* 恢复出厂设置：复位"用户私密状态"。
 * 密码与密码开关存在独立 VM id 上，erase_UIInfo_in_vm() 只擦 VM_UI_SYS_INFO(22)，
 * 覆盖不到它们，必须显式复位：
 *   - USER_PASSWORD(163)    开机密码
 *   - USER_PASSWORD_ON(164) 开机密码开关
 * 静音已纳入 sys_param 参数表，由 erase_UIInfo_in_vm() 置 valid=0 一并覆盖；
 * 这里仍调 ui_voice_mute_reset() 让运行时参数表与 VM 立即一致。
 * UI 路径（restore_sys_settings）与蓝牙路径（common_api_restore_sys_settings）
 * 共用本函数，避免两条路径复位内容不一致。
 * 返回 0 表示全部写入成功，非 0 表示有失败。 */
int factory_reset_user_secrets(void)
{
    u8 pwd_buf[POWERON_PASSWORD_LEN] = {0};
    u8 pwd_off = 0;
    int ret_pwd = 0;
    int ret_on = 0;

    ret_pwd = syscfg_write(USER_PASSWORD, pwd_buf, sizeof(pwd_buf));
    ret_on  = syscfg_write(USER_PASSWORD_ON, &pwd_off, sizeof(pwd_off));
    /* 注意：syscfg_write 的返回值即写入字节数，不等于 sizeof 即为失败。
     * 失败时 VM 中旧密码会残留，恢复出厂后开机仍会要求输入密码。 */
    ui_voice_mute_reset();

    return ((ret_pwd == (int)sizeof(pwd_buf)) && (ret_on == (int)sizeof(pwd_off))) ? 0 : -1;
}

void restore_sys_settings()
{
#if TCFG_PAY_ALIOS_ENABLE
    extern void alipay_power_on(void);
    alipay_power_on();
    alipay_unbinding();
#endif /* #if TCFG_PAY_ALIPAY_ENABLE */
#if TCFG_APP_RTC_EN
    struct sys_time def_sys_time = {  //重置系统时间
        .year = 2020,
        .month = 1,
        .day = 1,
        .hour = 0,
        .min = 0,
        .sec = 0,
    };
    rtc_write_time(&def_sys_time);
#endif
#if TCFG_USER_BLE_ENABLE
    ble_list_clear_all();
#endif
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
#endif
    small_file_del_all();//清外挂flash 存储
    /* 修复恢复出厂后语言未复位：原先只清 VM，重启后语言依赖默认表内容，
     * 而默认表里 Language 曾是 0（非法ID）。这里显式写回默认语言，
     * 与 sys_defalut_param[Language] 保持一致。 */
    set_ui_sys_param(Language, Chinese_Simplified);
    /* 修复恢复出厂后仍要求输入开机密码：密码与密码开关存放在独立 VM id
     * （USER_PASSWORD=163 / USER_PASSWORD_ON=164），erase_UIInfo_in_vm() 只擦
     * VM_UI_SYS_INFO(22)，覆盖不到这两个 id，必须显式复位。
     * 必须排在 erase_UIInfo_in_vm() 之前：本函数内部经 ui_voice_mute_reset()
     * 调 write_UIInfo_to_vm(NULL)，而 write_UIInfo_to_vm() 会无条件把
     * ui_sys_param.valid 置 1 写回 VM；若放在擦除之后，valid 会被重新置 1，
     * 重启后 read_UIInfo_from_vm() 走正常读取路径、不加载 sys_defalut_param，
     * 亮度/音量/表盘/语言/免打扰全部不会回退默认值，恢复出厂失效。 */
    (void)factory_reset_user_secrets();
    erase_UIInfo_in_vm();
    watch_reboot_or_shutdown(1, 1);
}

void ui_set_dark_time(u8 sel)
{
#if TCFG_UI_SHUT_DOWN_TIME
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
    const u16 dark_time[4] = {30, 10, 30, 60};
#else
    const u16 dark_time[5] = {10, 15, 20, 30, 60};
#endif
    if (get_ui_sys_param(LightAlwayEn) == 0) {
        if (sel >= ARRAY_SIZE(dark_time)) {   //修复: 原为 sizeof(dark_time)(=10)，与数组元素个数(=5)不符，越界读
            return;
        }
        ui_set_shut_down_time(dark_time[sel]);
        ui_auto_shut_down_modify();
    }
#endif
}

#if TCFG_UI_SHUT_DOWN_TIME

static u32 ui_get_dark_time(void)
{
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
    const u16 dark_time[4] = {5, 10, 30, 60};
#else
    const u16 dark_time[5] = {10, 15, 20, 30, 60};
#endif
    u8 sel = get_ui_sys_param(DarkTime);

    if (get_ui_sys_param(LightAlwayEn) == 0) {
        if (sel >= ARRAY_SIZE(dark_time)) {   //修复: 原为 sizeof(dark_time)(=10)，与数组元素个数(=5)不符，越界读
            return 0;
        }
        return dark_time[sel];
    }
    return 0;
}


static u16 alway_light_reset_to_id = 0;
static void alway_light_reset_to(void *priv)
{
    if (alway_light_reset_to_id == 0) {
        return ;
    }
    // 恢复
    set_ui_sys_param(LightAlwayEn, 0);
    ui_set_dark_time(get_ui_sys_param(DarkTime));
}
#endif /* #if TCFG_UI_SHUT_DOWN_TIME */

void screen_light_alway_switch(u8 on)
{
#if TCFG_UI_SHUT_DOWN_TIME
    //5min 10min 15min 20min
    u16 light_time[4] = {5 * 60, 10 * 60, 15 * 60, 20 * 60};
    u8 light_time_sel = get_ui_sys_param(LightTime);

    if (alway_light_reset_to_id) {
        sys_timeout_del(alway_light_reset_to_id);
        alway_light_reset_to_id = 0;
    }
    if (on) {
        if (light_time_sel >= ARRAY_SIZE(light_time)) {   //修复: 原为 sizeof(light_time)，同型越界读缺陷
            g_printf("light_time_sel >= sizeof(light_time)");
            return;
        }
        ui_set_shut_down_time(light_time[light_time_sel]);
        ui_auto_shut_down_modify();
        set_ui_sys_param(LightAlwayEn, 1);
        u32 to = (light_time[light_time_sel] - ui_get_dark_time()) * 1000;
        alway_light_reset_to_id = sys_timeout_add(NULL, alway_light_reset_to, to);
    } else {
        set_ui_sys_param(LightAlwayEn, 0);
        ui_set_dark_time(get_ui_sys_param(DarkTime));
    }
#endif
}

int get_light_level()
{
    return get_ui_sys_param(LightLevel);
}

void ui_ajust_light(u8 level)
{
    if (level > UI_LIGHT_LEVEL_MAX) {
        level = UI_LIGHT_LEVEL_MAX;
    }
    if (level <= UI_LIGHT_LEVEL_MIN) {
        level = UI_LIGHT_LEVEL_MIN;
    }
    extern int lcd_drv_backlight_ctrl(u8 percent);
    extern int lcd_backlight_status();
    if (lcd_backlight_status()) { //开机没有亮背光之前不能亮
        lcd_drv_backlight_ctrl(level * 10);
    }
}

/* 把 ui_ajust_light 语义的 level 钳位到 [UI_LIGHT_LEVEL_MIN, UI_LIGHT_LEVEL_MAX]。
 * 与 ui_ajust_light() 内联的钳位逻辑一致，供下面两条恢复路径复用。 */
static u8 ui_light_level_clamp(u8 level)
{
    if (level > UI_LIGHT_LEVEL_MAX) {
        level = UI_LIGHT_LEVEL_MAX;
    }
    if (level <= UI_LIGHT_LEVEL_MIN) {
        level = UI_LIGHT_LEVEL_MIN;
    }
    return level;
}

/* 开机按 VM 档位恢复亮度，不走 ui_ajust_light() 的 lcd_backlight_status() 守卫，避免依赖其 100% 设置的副作用 */
void ui_backlight_restore(void)
{
    extern int lcd_drv_backlight_ctrl(u8 percent);
    u8 level = ui_light_level_clamp(get_ui_sys_param(LightLevel) * 2);

    lcd_drv_backlight_ctrl(level * 10);
}

/* 覆盖 lcd_drive.c 的 weak 定义：返回用户设定亮度百分比(0~100)，供 lcd_sleep_ctrl() 唤醒与 lcd_bl_open() 取用 */
u8 lcd_user_brightness_percent(void)
{
    return ui_light_level_clamp(get_ui_sys_param(LightLevel) * 2) * 10;
}

void ui_set_voice(int percent)
{
    s8 volume = percent;

    if (volume > app_audio_get_max_volume()) {
        volume = app_audio_get_max_volume();
    }

    /* printf("%s %d--volume:%d, max_volume:%d",__FUNCTION__,__LINE__,volume,app_audio_get_max_volume()); */
    app_audio_set_volume(APP_AUDIO_STATE_WTONE, volume, 1);
    if (volume > 0) {
        set_ui_sys_param(LastSysVol, volume);
    }
}

static int ui_voice_to_percent(s8 volume)
{
    return volume * 100 / get_tone_vol();
}

void ui_moto_init(int gpio)
{
    if (gpio != NO_CONFIG_PORT) {
        pwm_if_open(&moto_pwm_p_data, gpio, 0, NULL);
    }
    moto_mode = get_ui_sys_param(MotoMode);
}

static void ui_moto_set_duty(int duty)
{
    if (duty > 100) {
        duty = 100;
    }
    pwm_if_set_duty(&moto_pwm_p_data, duty);
}

void ui_moto_set_H_L(u8 mode)
{
    set_ui_sys_param(MotoMode, mode);

    if (mode == TCFG_MOTO_PWM_NULL) {
        ui_moto_set_duty(TCFG_MOTO_PWM_NULL * 100);
    }
}

int sys_check_time_is_undisturb(void)
{
    u8 old_start_hour;
    u8 old_start_min;
    u8 old_end_hour;
    u8 old_end_min;
    u32 old_start_all_time;
    u32 old_end_all_time;
    u32 cur_all_time;
    struct sys_time cur_time;

    old_start_hour = get_ui_sys_param(UndisturbStimeH);
    old_start_min  = get_ui_sys_param(UndisturbStimeM);
    old_end_hour   = get_ui_sys_param(UndisturbEtimeH);
    old_end_min    = get_ui_sys_param(UndisturbEtimeM);
    old_start_all_time = old_start_hour * 60 + old_start_min;
    old_end_all_time = old_end_hour * 60 + old_end_min;
    rtc_read_time(&cur_time);
    cur_all_time = cur_time.hour * 60 + cur_time.min;

    printf(">>>>>>old_start_hour >>>>>>>>>= %d", old_start_hour);
    printf(">>>>>>old_start_min  >>>>>>>>>= %d", old_start_min);
    printf(">>>>>>old_end_hour   >>>>>>>>>= %d", old_end_hour);
    printf(">>>>>>old_end_min    >>>>>>>>>= %d", old_end_min);
    printf(">>>>>>>>>>>>>>>old_start_all_time = %d", old_start_all_time);
    printf(">>>>>>>>>>>>>>>old_end_all_time = %d", old_end_all_time);
    printf(">>>>>>>>>>>>>>>cur_all_time= %d", cur_all_time);

    if (old_start_all_time < old_end_all_time) {
        if ((old_start_all_time <= cur_all_time) && (cur_all_time < old_end_all_time)) { //判断为在勿扰时间内
            return true;
        }
    } else if (old_start_all_time > old_end_all_time) {
        if ((old_start_all_time <= cur_all_time) || (cur_all_time < old_end_all_time)) { //判断为在勿扰时间内
            return true;
        }
    }

    return false;
}

static void ui_moto_out_sleep(void)
{
    printf("mcpwm_open");
    /* mcpwm_start(moto_pwm_p_data.ch); */
    /* JL_MCPWM->CH1_CON0 = 0x24; */
}

static void ui_moto_in_sleep(void)
{
    printf("mcpwm_close");
    /* mcpwm_pause(moto_pwm_p_data.ch); */
    /* JL_MCPWM->CH1_CON0 = 0x00; */
}

static void moto_play(void *mode)
{
    if (moto_time_id == 0) {
        return ;
    }
    printf("moto_run ");
    if (moto_tmr_cnt & 0x01) {
        if (moto_mode == TCFG_MOTO_PWM_H) {
            ui_moto_out_sleep();
            ui_moto_set_duty(TCFG_MOTO_PWM_H * 100);
        } else if (moto_mode == TCFG_MOTO_PWM_L) {
            ui_moto_out_sleep();
            ui_moto_set_duty(TCFG_MOTO_PWM_L * 100);
        }
    } else {
        ui_moto_set_duty(TCFG_MOTO_PWM_NULL * 100);
        ui_moto_in_sleep();
    }
    if (moto_tmr_cnt) {
        moto_tmr_cnt --;
    } else {
        if (moto_time_id) {
            sys_timer_del(moto_time_id);
            moto_time_id = 0;
        }
    }
}

void ui_moto_run(u8 run_mode)
{
    u32 rets;//
    __asm__ volatile("%0 = rets":"=r"(rets));
    printf("__func__ %s %x\n", __func__, rets);


    static u8 run_key = 0;
    static u8 moto_key = 0;
    struct sys_time new_time;
    u8 old_start_hour;
    u8 old_start_min;
    u8 old_end_hour;
    u8 old_end_min;
    u32 old_start_all_time;
    u32 old_end_all_time;
    u32 new_all_time;

    if (run_mode == 3) { //全天勿扰加锁
        moto_key = 1;
        run_key = 0;
        ui_moto_set_duty(TCFG_MOTO_PWM_NULL * 100);
        ui_moto_in_sleep();
        if (moto_time_id) {
            sys_timer_del(moto_time_id);
            moto_time_id = 0;
        }
    } else if (run_mode  == 4) {
        moto_key = 0;
    } else if (run_mode == 7) {
        moto_key = 0;
        run_key = 0;
    }

    if (moto_key == 0) { //非全天勿扰
        if (run_mode == 5) { //定时勿扰判处理
            printf("run_mode == 5");
            run_key = 5;
            return ;
        } else if (run_mode  == 6) {
            printf("run_mode == 6");
            run_key = 0;
            moto_key = 0;
        }
    }

    if (run_key == 5) {
        printf("run_mode == 5");
        old_start_hour = get_ui_sys_param(UndisturbStimeH);
        old_start_min  = get_ui_sys_param(UndisturbStimeM);
        old_end_hour   = get_ui_sys_param(UndisturbEtimeH);
        old_end_min    = get_ui_sys_param(UndisturbEtimeM);
        old_start_all_time = old_start_hour * 60 + old_start_min;
        old_end_all_time = old_end_hour * 60 + old_end_min;
        rtc_read_time(&new_time);
        new_all_time = new_time.hour * 60 + new_time.min;

        printf(">>>>>>old_start_hour >>>>>>>>>= %d", old_start_hour);
        printf(">>>>>>old_start_min  >>>>>>>>>= %d", old_start_min);
        printf(">>>>>>old_end_hour   >>>>>>>>>= %d", old_end_hour);
        printf(">>>>>>old_end_min    >>>>>>>>>= %d", old_end_min);
        printf(">>>>>>>>>>>>>>>old_start_all_time = %d", old_start_all_time);
        printf(">>>>>>>>>>>>>>>old_end_all_time = %d", old_end_all_time);
        printf(">>>>>>>>>>>>>>>new_all_time = %d", new_all_time);

        if (old_start_all_time == old_end_all_time) {
            moto_key = 0;
        }

        if (old_start_all_time < old_end_all_time) { //首先判断是开始时间大还是结束时间大确认是否超24小时
            if ((old_start_all_time <= new_all_time) && (new_all_time < old_end_all_time)) { //判断为在勿扰时间内
                printf("moto_run_disable");
                moto_key = 1;
            } else {
                printf("moto_run_enable");
                moto_key = 0;
            }
        } else { //这种情况不做处理 //开始时间 大于 结束时间

        }
    }

    printf("moto_key  == %d", moto_key);

    if (moto_key == 0) {
        moto_mode = get_ui_sys_param(MotoMode);
        if (run_mode == 1) { //间隔震动
            printf("moto_run_mode = 1 ");
            moto_tmr_cnt = 10;
            if (moto_time_id == 0) {
                moto_time_id = sys_timer_add(NULL, moto_play, 1000);
            }
        } else if (run_mode == 2) { //震动一次
            printf("moto_run_mode = 2 ");
            if (moto_time_id == 0) {
                moto_tmr_cnt = 1;
                moto_time_id = sys_timer_add(NULL, moto_play, 1000);
            }
        } else { //为静音模式
            printf("moto_run_mode = 0 ");
            if (moto_time_id) {
                sys_timer_del(moto_time_id);
                moto_time_id = 0;
            }
            ui_moto_set_duty(TCFG_MOTO_PWM_NULL * 100);
            ui_moto_in_sleep();
        }
    }
}

void ui_moto_run_test(u8 percent)
{
    if (0 == percent) {
        ui_moto_set_duty(0);
        ui_moto_in_sleep();
    } else {
        ui_moto_out_sleep();
        ui_moto_set_duty(percent * 100);
    }
}

static void ui_volume_up(u8 step)
{
    s8 volume = app_audio_get_volume(APP_AUDIO_STATE_WTONE);
    volume += step;
    if (volume > app_audio_get_max_volume()) {
        volume = app_audio_get_max_volume();
    }
    /* printf("%s %d--volume:%d, max_volume:%d",__FUNCTION__,__LINE__,volume,app_audio_get_max_volume()); */
    app_audio_set_volume(APP_AUDIO_STATE_WTONE, volume, 1);
    if (volume > 0) {
        set_ui_sys_param(LastSysVol, volume);
    }
}

static void ui_volume_down(u8 step)
{
    s8 volume = app_audio_get_volume(APP_AUDIO_STATE_WTONE);
    volume -= step;
    if (volume < 0) {
        volume = 0;
    }
    /* printf("%s %d--volume:%d, max_volume:%d",__FUNCTION__,__LINE__,volume,app_audio_get_max_volume()); */
    app_audio_set_volume(APP_AUDIO_STATE_WTONE, volume, 1);
    if (volume > 0) {
        set_ui_sys_param(LastSysVol, volume);
    }
}

void ui_music_volume_up(void)
{
    s16 volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    opid_play_vol_sync_fun(&volume, 1);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#endif
    syscfg_write(CFG_MUSIC_VOL, &volume, 2);
}

void ui_music_volume_down(void)
{
    s16 volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    opid_play_vol_sync_fun(&volume, 0);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#endif
    syscfg_write(CFG_MUSIC_VOL, &volume, 2);
}

/* 静音开关。原实现用 app_audio_set_mute_state(APP_AUDIO_STATE_WTONE, ...)，但本板
 * WARNING_TONE_VOL_FIXED=1（audio/common/audio_volume_mixer.c，TCFG_SUB_CASE_TYPE=WATCH
 * 走 else 分支），该接口在写状态前直接 return；而 app_audio_get_mute_state() 没有同样的
 * #if 保护 → 读回恒 0，导致开关既无效也无法关闭。
 * 改用 DAC 全局数字静音 app_audio_mute()（内部走 audio_dac_digital_mute，不受
 * WARNING_TONE_VOL_FIXED 影响），状态存入 sys_param 参数表的 VoiceMute 项随整表持久化。 */
static void voice_mute_apply(u8 en)
{
    app_audio_mute(en ? AUDIO_MUTE_DEFAULT : AUDIO_UNMUTE_DEFAULT);
}

void ui_set_voice_mute(u8 en)
{
    en = !!en;
    if (get_ui_sys_param(VoiceMute) == en) {
        /* 状态未变也保证 DAC 一致（例如开机恢复后首次被调用） */
        voice_mute_apply(en);
        return;
    }
    set_ui_sys_param(VoiceMute, en);
    voice_mute_apply(en);
    /* 静音切换是低频事件，直接落 VM，避免关机前状态丢失 */
    write_UIInfo_to_vm(NULL);
}

u8 ui_get_voice_mute()
{
    return (get_ui_sys_param(VoiceMute) > 0) ? 1 : 0;
}

/* 恢复出厂用：把静音状态清回「不静音」并落 VM。
 * 不走 ui_set_voice_mute(0)：它带「状态未变就 return」的短路，当前恰好已是不静音时
 * 不会覆写 VM。这里无条件写入，保证复位后 VM 中该项必为 0。 */
void ui_voice_mute_reset(void)
{
    set_ui_sys_param(VoiceMute, 0);
    voice_mute_apply(0);
    write_UIInfo_to_vm(NULL);
}

u8 ui_show_shortcut_key()
{
    if (UI_WINDOW_PREEMPTION_CHECK()) {
        return false;
    }
    int id = ui_get_current_window_id();
    if (id == ID_WINDOW_DIAL) {
        if (get_need_password() == 1) {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_POWERON_PASSWORD);
            return true;
        }
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_SHORTCUT_MENU);
    } else if (id == ID_WINDOW_SHORTCUT_MENU) {
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_DIAL);
    } else if (id == ID_WINDOW_POWERON_PASSWORD) {
        return true;
    } else if (id != ID_WINDOW_STOPWATCH && id != ID_WINDOW_PHONE_CALL_STATUS &&
               id != ID_WINDOW_INDOOR_SPORTS && id != ID_WINDOW_OUTDOOR_SPORTS) {
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_SHORTCUT_MENU);
    }

    return true;
}
static u8 ui_style_vm_init(void)
{
    u8 menu_style = get_ui_sys_param(MenuStyle);
    printf("%s style=%d", __func__, menu_style);
    if (menu_style < ARRAY_SIZE(app_list_type_tab)) {
    } else {
        set_ui_sys_param(MenuStyle, 0);
        return -1;
    }
    return 0;
}

static u8 ui_card_anim_vm_init(void)
{
    int ret = 0;
    u8 card_anim = get_ui_sys_param(move_mode);
    printf("%s card_anim=%d", __func__, card_anim);
    if (card_anim >= ARRAY_SIZE(card_anim_type_tab)) {
        card_anim = 0;
        ret = -1;
        goto __card_anim_set;
    }
    if (card_anim_type_tab[card_anim] == -1) {
        card_anim = 0;
        ret = -1;
        goto __card_anim_set;
    }
__card_anim_set:
    ui_card_set_move_mode(card_anim_type_tab[card_anim]);
    set_ui_sys_param(move_mode, card_anim);
    return ret;
}

u8 ui_show_plus()
{
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
    if (music_is_play() == true || ui_get_current_window_id() == ID_WINDOW_MUSIC_PLAYER) {
#else
    if (music_is_play() == true && ui_get_current_window_id() != ID_WINDOW_MUSIC_PLAYER) {
#endif
        ui_music_volume_up();
    }
    return true;
}

u8 ui_show_minus()
{
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
    if (music_is_play() == true || ui_get_current_window_id() == ID_WINDOW_MUSIC_PLAYER) {
#else
    if (music_is_play() == true && ui_get_current_window_id() != ID_WINDOW_MUSIC_PLAYER) {
#endif
        ui_music_volume_down();
    }
    return true;
}

u8 ui_key_shutdown_or_reboot(u8 flag)
{
    if (ui_get_current_window_id() == ID_WINDOW_POWERON_PASSWORD) {
        return true;
    }
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
    if (sbox_box_charging_get()) {
        return true;
    }
#endif
    if (!lcd_sleep_status()) {
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_RESTART_SHUTDOWN);
    }
    return true;
}

// 检查当前是否在显示列表
bool ui_show_menu_check_win(int win_id)
{
    for (int i = 0; i < ARRAY_SIZE(app_list_type_tab); i++) {
        if (app_list_type_tab[i] == win_id) {
            return true;
        }
    }
    return false;
}

// 获取当前应用列表序号
int ui_show_menu_get_index(int *out_win_id)
{
    u8 menu_style = get_ui_sys_param(MenuStyle);
    if (menu_style < ARRAY_SIZE(app_list_type_tab)) {
        if (out_win_id) {
            *out_win_id = app_list_type_tab[menu_style];
        }
        return menu_style;
    }
    return -1;
}

// 显示应用列表
bool ui_show_menu_force(void)
{
    u8 menu_style = get_ui_sys_param(MenuStyle);
    if (menu_style < ARRAY_SIZE(app_list_type_tab)) {
        ui_menu_enter_anim_enable();
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
        // UI_WINDOW_BACK_CLEAN(); // 卡片
#else
        UI_WINDOW_BACK_CLEAN(); // 卡片
#endif
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(app_list_type_tab[menu_style]);
    } else {
        return false;
    }
    return true;
}
// 应用列表风格切换
bool ui_show_menu_sw(int idx)
{
    if (idx >= ARRAY_SIZE(app_list_type_tab)) {
        return false;
    }
    if (app_list_type_tab[idx] == 0) {
        return false;
    }
    set_ui_sys_param(MenuStyle, idx);
    return true;
}

// 应用列表风格切换及显示
bool ui_show_menu_list()
{
    if (ui_get_current_window_id() == ID_WINDOW_DIAL) {
        if (get_need_password() == 1) {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_POWERON_PASSWORD);
            return true;
        }
    }
    if (ui_get_current_window_id() == ID_WINDOW_POWERON_PASSWORD) {
        return true;
    }
    if (UI_WINDOW_PREEMPTION_CHECK()) {
        return false;
    }
    if (ui_show_menu_check_win(ui_get_current_window_id())) {
        u8 menu_style = get_ui_sys_param(MenuStyle);
        u8 total = ARRAY_SIZE(app_list_type_tab);
        while (--total) {
            menu_style ++;
            if (menu_style >= ARRAY_SIZE(app_list_type_tab)) {
                menu_style  = 0;
            }
            if (ui_show_menu_sw(menu_style)) {
                ui_show_menu_force();
                return true;
            }
        }

    }
    return false;
}

// 根据索引号获取卡片转场风格
int ui_get_card_anim_by_idex(int index)
{
    if (index >= ARRAY_SIZE(card_anim_type_tab)) {
        return -1;
    }
    return card_anim_type_tab[index];
}

// 切换卡片转场风格
bool ui_card_anim_sw(int idx)
{
    if (idx >= ARRAY_SIZE(card_anim_type_tab)) {
        return false;
    }
    if (card_anim_type_tab[idx] == -1) {
        return false;
    }
#if 0
    //特效优化中
    if (card_anim_type_tab[idx] == PAGE_MOVE_MODE_REFLECTION ||
        card_anim_type_tab[idx] == PAGE_MOVE_MODE_CUBE_REFLECTION ||
        card_anim_type_tab[idx] == PAGE_MOVE_MODE_DRIFT_FLIP) {
        return false;
    }
#endif
    ui_card_set_move_mode(card_anim_type_tab[idx]);
    set_ui_sys_param(move_mode, idx);
#if TCFG_UI_EFFECT_USED_DEMO
    if (card_anim_type_tab[idx] == PAGE_MOVE_MODE_USER) {
        void ui_page_user_mode_callback_set(void *cb);
        void ui_page_effect_double_scale_alpha_cb(struct element * curr_elm, struct element * prev_elm, struct element * next_elm, int cur_left);
        ui_page_user_mode_callback_set(ui_page_effect_double_scale_alpha_cb);
    }
#endif
    return true;
}

// 自动切换卡片转场风格
bool ui_card_anim_next()
{
    bool ret = false;
    enum ui_card_run_type type = ui_card_get_status();
    if (type == UI_CARD_RUN_SINGLE_PAGE) {
        return ret;
    } else if (type == UI_CARD_RUN_MULTI_PAGE) {
        ui_card_run_stop();
    }
    int cur_win = ui_get_current_window_id();
    struct ui_page *page = ui_page_search(cur_win);
    if ((cur_win == ID_WINDOW_DIAL) || (page)) {
        u8 idx = get_ui_sys_param(move_mode);
        u8 total = ARRAY_SIZE(card_anim_type_tab);
        while (--total) {
            idx ++;
            if (idx >= ARRAY_SIZE(card_anim_type_tab)) {
                idx = 0;
            }
            if (ui_card_anim_sw(idx)) {
                ret = true;
                break;
            }
        }
    }
    if (type == UI_CARD_RUN_MULTI_PAGE) {
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_DIAL);
    }
    return ret;
}

// 非表盘界面显示表盘。表盘界面显示应用列表
bool ui_show_menu_page()
{
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
    extern void ui_screen_saver(void *p);
    ui_screen_saver(NULL);
    bool cpc_is_card_page(int mem_id);
#else
    if (ui_get_current_window_id() == ID_WINDOW_DIAL) {
        enum ui_card_run_type type = ui_card_get_status();
        printf("@@@@@ %s, cardtype:%d \n", __func__, type);
        if (get_need_password() == 1) {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_POWERON_PASSWORD);
            return true;
        }
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
        extern void cpc_go_to_history_card();
        cpc_go_to_history_card();
        return true;
    } else if (cpc_is_card_page(ui_get_current_window_id())) {
        return ui_show_menu_force();
    } else if (ui_get_current_window_id() == ID_WINDOW_POWERON_PASSWORD) {
#else
        if (type == UI_CARD_RUN_MULTI_PAGE) {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_DIAL);
            return true;
        }
        if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
            UI_HIDE_MULTI_PAGE();
        }
        return ui_show_menu_force();
    } else if (ui_get_current_window_id() == ID_WINDOW_POWERON_PASSWORD) {
#endif
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_DIAL);
    } else if (ui_get_current_window_id() > 0) {
        if (UI_WINDOW_PREEMPTION_CHECK()) {
            return false;
        }
#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
        void cpc_go_to_history_card();
        cpc_go_to_history_card();
#else
        UI_HIDE_CURR_WINDOW();
        if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
            UI_SHOW_MULTI_PAGE();
        }
        UI_SHOW_WINDOW(ID_WINDOW_DIAL);
#endif
    }
#endif

    return true;
}

int ui_show_menu_total_num(void)
{
    return ARRAY_SIZE(app_list_type_tab);
}

void ui_page_list_update(u32 *id_list, u8 num)
{
    u8 list_total_num = get_ui_page_list_total_num();
    u8 i;
    if (list_total_num < num) {
        for (i = 0; i < list_total_num; i++) {
            ui_page_list_id_modify(i, id_list[i]);
        }
        for (i = list_total_num; i < num; i++) {
            ui_page_add(id_list[i]);
        }
    } else if (list_total_num > num) {
        for (i = 0; i < num; i++) {
            ui_page_list_id_modify(i, id_list[i]);
        }

        for (i = num; i < list_total_num; i++) {
            //ui_page_del_by_num(i);
            ui_page_del_by_num(num);
        }
    } else {
        for (i = 0; i < num; i++) {
            ui_page_list_id_modify(i, id_list[i]);
        }
    }
}

void watch_reboot_or_shutdown(u8 flag, u8 erase)
{
    void *p = NULL;
    if (erase) {
        p = SYSCFG_WRITE_ERASE_STATUS;

#if (defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
        struct sys_time def_sys_time = {
            .year = 2020,
            .month = 1,
            .day = 1,
            .hour = 0,
            .min = 0,
            .sec = 0,
        };
        rtc_update_time_api(&def_sys_time);
        u8 chgbox_addr[6] = {0};
        syscfg_read(CFG_CHGBOX_ADDR, chgbox_addr, 6); /*根据使用逻辑，耳机的mac地址不用清除*/
        vm_eraser(); /*为了清除eq anc之类的配置*/
        syscfg_write(CFG_CHGBOX_ADDR, chgbox_addr, 6);
        log_info("chgbox_addr:");
        log_info_hexdump(chgbox_addr, 6);
#endif
    }
    /* extern void watch_sensor_test(void); */
    /* extern int watch_sensor_close(void); */

#if TCFG_SPORT_HEALTH_ENABLE

    sport_health_manager_release();
#endif
#if TCFG_APP_CAT1_EN
    cat1_close();
#endif
#if TCFG_SYS_LVD_EN
    extern void update_bat_info_vm();
    update_bat_info_vm();
#endif
    watch_syscfg_write_all(p);

    if (flag) {
        cpu_reset();
    } else {
        sys_enter_soft_poweroff(NULL);
    }
}

REGISTER_WATCH_SYSCFG(sys_param_ops) = {
    .name = "sys_param",
    .read = read_UIInfo_from_vm,
    .write = write_UIInfo_to_vm,
};

static u8 ui_sys_idle_query(void)
{
    if (moto_time_id) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(ui_sys_lp_target) = {
    .name = "ui_sys",
    .is_idle = ui_sys_idle_query,
};


#if (defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
#include "events_adapter.h"

const static u8 csc_language_type[LANGUAGE_NUM] = {Chinese_Simplified, Chinese_Traditional, Japanese, Korean, English, French, German, Russian,  Turkey, Arabic, Portuguese, Spanish, Vietnam, Thai, Malay, Italian};

void csc_set_ui_language_type(u8 index)
{
    if (index >= LANGUAGE_NUM) {
        log_error("%s type is error!!!", __func__);
        return;
    }

    set_ui_sys_param(Language, index);
    ui_language_set(csc_language_type[index]);
    sbox_language_ui_set(csc_language_type[index]);
    log_info("%s %d %d ", __func__, index, csc_language_type[index]);
}

u8 csc_get_ui_language_type(void)
{
    return get_ui_sys_param(Language);
}

#endif



#endif /* #if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))*/
#endif/* #if TCFG_UI_ENABLE */

