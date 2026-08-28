#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc.data.bss")
#pragma data_seg(".pc.data")
#pragma const_seg(".pc.text.const")
#pragma code_seg(".pc.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "audio_config.h"
#include "usb/device/usb_stack.h"
#include "usb/usb_task.h"
#include "usb/device/hid.h"
#include "usb/device/msd.h"
#include "uac_stream.h"
#include "pc.h"
#include "tone_player.h"
#include "app_tone.h"
#include "user_cfg.h"
#include "app_task.h"
#include "app_main.h"
#include "ui/ui_api.h"
#include "ui_manage.h"

#define LOG_TAG_CONST       PC
#define LOG_TAG             "[PC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_PC_EN

struct pc_opr {
    u8 volume;
    u8 onoff : 1;
};

static struct pc_opr pc_hdl = {0};
#define __this 	(&pc_hdl)
static u8 pc_idle_flag = 1;

#define TCFG_PC_TONE_EN		1

//*----------------------------------------------------------------------------*/
/**@brief    pc 在线检测  切换模式判断使用
  @param    无
  @return   1 linein设备在线 0 设备不在线
  @note
 */
/*----------------------------------------------------------------------------*/
static int app_pc_check(void)
{
#if ((defined TCFG_PC_BACKMODE_ENABLE) && (TCFG_PC_BACKMODE_ENABLE))
    return false;
#endif//TCFG_PC_BACKMODE_ENABLE

    u32 r = usb_otg_online(0);
    log_info("pc_app_check %d", r);
    if ((r == OTG_STATE_SLAVE_MODE) ||
        (r == OTG_STATE_SLAVE_WAIT_CONFIRMATION) ||
        (r == OTG_STATE_SLAVE_MODE_SUSPEND)) {
        return true;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 打开
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void pc_task_start(void)
{
    if (__this->onoff) {
        log_info("PC is start ");
        return ;
    }
    log_info("App Start - PC");

    /* app_status_handler(APP_STATUS_PC); */
#if TCFG_PC_ENABLE
    usb_message_to_stack(USBSTACK_START, 0, 1);
#endif

    __this->onoff = 1;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 关闭
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void pc_task_stop(void)
{
    if (!__this->onoff) {
        log_info("PC is stop ");
        return ;
    }
    __this->onoff = 0;
    u32 state = usb_otg_online(0);
    if (state != OTG_STATE_SLAVE_MODE && state != OTG_STATE_SLAVE_WAIT_CONFIRMATION) {
        log_info("App Stop - PC");
#if TCFG_PC_ENABLE
        usb_message_to_stack(USBSTACK_STOP, 0, 1);
#endif
    } else {
        log_info("App Hold- PC");
#if TCFG_PC_ENABLE
        usb_message_to_stack(USBSTACK_PAUSE, 0, 1);
#endif
    }

    /* tone_play_stop(); */
    /* tone_play_stop_by_path(tone_table[IDEX_TONE_PC]);//停止播放提示音 */

#if (TCFG_DEV_MANAGER_ENABLE)
    dev_manager_list_check_mount();
#endif/*TCFG_DEV_MANAGER_ENABLE*/

}

#if ((defined TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)&&(TCFG_PC_MODE_USB_DETECT_DURING_CHARGING))
void pc_charge_usb_exit_check(void *p)
{
    //如果没有退出pc模式，则退出
    if (app_in_mode(APP_MODE_PC) == true) {
        log_info("%s re_exit!\n", __func__);
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
    }
}
void pc_charge_usb_detect_start()
{
    //恢复usb io(主机)
    /* usb_otg_io_resume(0); */
    //打印usb检测定时器
    usb_detect_timer_add();
}
void pc_charge_usb_detect_stop()
{
    //关闭usb检测定时器，优化功耗
    usb_detect_timer_del();
    //挂起usb io（主机）
    /* usb_otg_io_suspend(0); */
    //检测是否已经退出pc模式
    pc_charge_usb_exit_check(NULL);
}
#endif// TCFG_PC_MODE_USB_DETECT_DURING_CHARGING

static int pc_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_PC)) {
        return 0;
    }
    switch (event) {
    case STREAM_EVENT_NONE:
    case STREAM_EVENT_STOP:
        pc_task_start();
        break;
    default:
        break;
    }
    return 0;
}

static void app_pc_init()
{
    pc_idle_flag = 0;
    /* ui_update_status(STATUS_PC_MODE); */
    __this->volume =  app_audio_get_volume(APP_AUDIO_STATE_MUSIC);//记录下当前音量
#if TCFG_PC_TONE_EN
    tone_player_stop();
    int ret = play_tone_file_callback(get_tone_files()->pc_mode, NULL, pc_tone_play_end_callback);
    if (ret) {
        log_error("pc tone play err!!!");
        pc_task_start();
    }
#else
    pc_task_start();
#endif
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_PC);
#if TCFG_PC_MODE_LOCK_UI_ENABLE
    UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_PC	, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PC);
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    pc模式 退出
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void app_pc_exit()
{
#if TCFG_PC_MODE_LOCK_UI_ENABLE
    UI_WINDOW_PREEMPTION_POP(ID_WINDOW_PC);
#endif
    pc_task_stop();
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif
    pc_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_PC);
}

struct app_mode *app_enter_pc_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_pc_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), pc_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            pc_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_pc_exit();

    return next_mode;
}

static int pc_mode_try_enter(int arg)
{
    if (true == app_pc_check()) {
        return 0;
    }
    return 1;
}

static int pc_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops pc_mode_ops = {
    .try_enter = pc_mode_try_enter,
    .try_exit  = pc_mode_try_exit,
};

/* 注册pc模式 */
REGISTER_APP_MODE(pc_mode) = {
    .name 	= APP_MODE_PC,
    .index  = APP_MODE_PC_INDEX,
    .ops 	= &pc_mode_ops,
};

static u8 pc_idle_query(void)
{
    return pc_idle_flag;
}

REGISTER_LP_TARGET(pc_lp_target) = {
    .name = "pc",
    .is_idle = pc_idle_query,
};

#endif
