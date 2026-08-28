#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".power_on.data.bss")
#pragma data_seg(".power_on.data")
#pragma const_seg(".power_on.text.const")
#pragma code_seg(".power_on.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "tone_player.h"
#include "app_tone.h"
#include "power_on.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "jlui_app/ui_style.h"

#define LOG_TAG             "[APP_IDLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern u8 get_charge_online_flag(void);
extern uint32_t timer_get_ms(void);
#define POWER_ON_TIME     2//最小开机时间
static u32 ui_logo_time = 0;
extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;

_WEAK_
void set_need_password(u8 flag)
{
    (void)flag;  /* 默认空实现: 当 jlui password UI 未编译时无操作 */
}

#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA)
extern u8 app_get_kid_camera_boot_mode(void);
#endif

static void power_on_ui_enter()
{
    UI_SHOW_WINDOW(ID_WINDOW_POWER_ON);
    ui_logo_time = timer_get_ms();

}
static void power_on_ui_exit()
{
#if TCFG_UI_ENABLE
    while (timer_get_ms() - ui_logo_time <= POWER_ON_TIME * 1000) { //显示开机logo
        os_time_dly(2);
    }
#endif

    u8 poweron_password = 0;
    int ret = syscfg_read(USER_PASSWORD_ON, &poweron_password, sizeof(poweron_password));
    if ((ret == sizeof(poweron_password)) && poweron_password) {
#if TCFG_UI_ENABLE
#if !CONFIG_LVGL_UI_ENABLE
        set_need_password(1);
#endif
#endif
        UI_SHOW_WINDOW(ID_WINDOW_POWERON_PASSWORD);
    } else {
        if (!get_charge_online_flag()) {
            if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                UI_SHOW_MULTI_PAGE();
            }
#if TCFG_PT_JL_TEST_ENABLE
            UI_SHOW_WINDOW(ID_WINDOW_PT);
#else
            UI_SHOW_WINDOW(ID_WINDOW_DIAL);
#endif
        }//非充电状态进表盘，充电床头由充电post页面显示
    }
}
//*----------------------------------------------------------------------------*/
/**@brief   开机启动
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void poweron_task_start()
{
    power_on_ui_exit();

#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA)
    /* 开机进 IDLE 待机/主界面, 不直接进任何功能模式;
     * 由 UI 选择后再 app_send_message(APP_MSG_GOTO_MODE, APP_MODE_VIDEO_REC/PHOTO/VIDEO_DEC) 进入。
     * (原 app_get_kid_camera_boot_mode() 会直接进 VIDEO_REC, 已废弃) */
    if (app_goto_mode_consume_pending()) {
        /* 用户已提前选模式，不再发 IDLE 挤掉它 */
        return;
    }
    app_send_message(APP_MSG_GOTO_MODE, APP_MODE_IDLE);
#elif TCFG_APP_BT_EN
    app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
#elif (TCFG_APP_MUSIC_EN || TCFG_APP_LINEIN_EN || TCFG_APP_FM_EN || TCFG_APP_PC_EN || TCFG_APP_RECORD_EN)
    app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
#else
    app_send_message(APP_MSG_GOTO_MODE, APP_MODE_IDLE);
#endif


}

static int poweron_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (!app_in_mode(APP_MODE_POWERON)) {
        return 0;
    }

    if (event == STREAM_EVENT_STOP) {
        poweron_task_start();
    }
    return 0;
}

static int app_poweron_init()
{
    log_info("power on");
    power_on_ui_enter();

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_POWERON);

    if (app_var.play_poweron_tone) {
        int ret = play_tone_file_callback(get_tone_files()->power_on, NULL, poweron_tone_play_end_callback);
        if (ret) {
            log_error("power on tone play err!!!");
            poweron_task_start();
        }
    } else {
        poweron_task_start();
    }

    return 0;
}

static void app_poweron_exit()
{
    log_info("exit power on mode");
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_POWERON);
}

struct app_mode *app_enter_poweron_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_poweron_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), NULL)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_poweron_exit();

    return next_mode;
}

static int poweron_mode_try_enter(int arg)
{
    return 0;
}

static int poweron_mode_try_exit()
{
    //上电提示音播放结束前不退出模式
#if TCFG_BT_BACKGROUND_ENABLE
    if (tone_player_runing()) {
        return 1;
    } else {
        return 0;
    }
#else
    return 0;
#endif
}

static const struct app_mode_ops poweron_mode_ops = {
    .try_enter          = poweron_mode_try_enter,
    .try_exit           = poweron_mode_try_exit,
};

/*
 * 注册power_on模式
 */
REGISTER_APP_MODE(poweron_mode) = {
    .name 	= APP_MODE_POWERON,
    .index  = 0xff,
    .ops 	= &poweron_mode_ops,
};

