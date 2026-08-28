#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".alarm_user.data.bss")
#pragma data_seg(".alarm_user.data")
#pragma const_seg(".alarm_user.text.const")
#pragma code_seg(".alarm_user.text")
#endif
#include "system/includes.h"
#include "alarm.h"
#include "system/timer.h"
#include "app_main.h"
#include "tone_player.h"
#include "app_task.h"
#include "app_tone.h"
#include "app_config.h"
#include "bt.h"
#include "ui/ui_api.h"
#include "call/call_common.h"
#include "rtc.h"
#include "smartbox_user_app.h"

#if TCFG_APP_RTC_EN

#define ALARM_RING_MAX 50
volatile u16 g_alarm_ring_cnt = 0;
static u16 g_ring_playing_timer = 0;

/*************************************************************
  此文件函数主要是用户主要修改的文件

  void set_rtc_default_time(struct sys_time *t)
  设置默认时间的函数

  void alm_wakeup_isr(void)
  闹钟到达的函数

  void alarm_event_handler(struct sys_event *event, void *priv)
  监听按键消息接口,应用于随意按键停止闹钟

  int alarm_sys_event_handler(struct sys_event *event)
  闹钟到达响应接口

  void alarm_ring_start()
  闹钟铃声播放接口

void alarm_stop(u8 reason)
  闹钟铃声停止播放接口
 **************************************************************/

static u8 rtc_can_run(void)
{
    printf("%s", __func__);
    u8 call_status = call_ctrl_get_status();
    if ((call_status == BT_CALL_ACTIVE) ||
        (call_status == BT_CALL_OUTGOING) ||
        (call_status == BT_CALL_ALERT) ||
        (call_status == BT_CALL_INCOMING)) {
        //通话过程不允许开rtc
        return false;
    }
    return true;
}

/* u8 set_rtc_default_time(struct sys_time *t, struct _rtc_trim *lrc_trim) */
/* { */
/* #if (defined(CONFIG_CPU_BR28)) */
/*     if (read_p11_sys_time(t, lrc_trim)) { */
/*         g_printf("rtc_read_sys_time: %d-%d-%d %d:%d:%d\n", */
/*                  t->year, t->month, t->day, t->hour, t->min, t->sec); */
/*         return 1; */
/*     } */
/* #else */
/*     t->year = 2019; */
/*     t->month = 5; */
/*     t->day = 5; */
/*     t->hour = 18; */
/*     t->min = 18; */
/*     t->sec = 18; */
/* #endif */
/*     return 0; */
/* } */

__attribute__((weak))
u8 rtc_app_alarm_ring_play(u8 alarm_state)
{
    return 0;
}


void alarm_ring_cnt_clear(void)
{
    g_alarm_ring_cnt = 0;
}

void alarm_stop(u8 reason)
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s rets:0x%x", __func__, rets_addr);
    printf("ALARM_STOP !!!\n");
    UI_MOTO_RUN(0);
#if TCFG_EARPHONE_PROTOCOL
    custom_client_send_alarm_earphone(0x03);
#endif

    if (reason == 1) {            //闹钟铃声播放超时
#if TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE
        if (get_need_password() == 1) {
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_ALARM_RINGING);
            UI_SHOW_WINDOW(ID_WINDOW_POWERON_PASSWORD);
        } else {
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_ALARM_RINGING);
        }
#endif
        alarm_snooze();    // 若播放铃声超时直接设置贪睡模式
    }
#if TCFG_USER_EMITTER_ENABLE
    emitter_close(1);
#endif
    alarm_active_flag_set(0);
    alarm_ring_cnt_clear();
    rtc_app_alarm_ring_play(0);
}

void alarm_play_timer_del(void)
{
    if (g_ring_playing_timer) {
        sys_timeout_del(g_ring_playing_timer);
        g_ring_playing_timer = 0;
    }
}

static void  __alarm_ring_play(void *p)
{
    if (alarm_active_flag_get()) {
        if (g_alarm_ring_cnt > 0) {
            if (!tone_player_runing()) {
                if (!rtc_app_alarm_ring_play(1)) {
                    play_tone_file_alone(get_tone_files()->normal);
                    g_alarm_ring_cnt--;
                }
                sys_timeout_add(NULL, __alarm_ring_play, 500);
            } else {
                sys_timeout_add(NULL, __alarm_ring_play, 500);
            }
        } else {
            alarm_stop(1);
        }
    }
}

void alarm_ring_start()
{
    UI_MOTO_RUN(1);
#if TCFG_EARPHONE_PROTOCOL
    custom_client_send_alarm_earphone(0x12);
#endif

    printf("ALARM_RING_START !!!\n");
    if (g_alarm_ring_cnt == 0) {
        g_alarm_ring_cnt = ALARM_RING_MAX;
#if TCFG_USER_EMITTER_ENABLE
        emitter_open(1);
#endif
        sys_timeout_add(NULL, __alarm_ring_play, 500);
    }
}

#if TCFG_UI_ENABLE

static int alarm_ui_entry(int page)
{
    alarm_ring_start();
    return false;
}

static int alarm_ui_exit(int page)
{
    alarm_stop(0);
    return false;
}
#endif /* #if TCFG_UI_ENABLE */

void alm_wakeup_isr(void)
{
    int msg[2];
    msg[0] = (u32)DEVICE_EVENT_FROM_ALM;
    alarm_active_flag_set(true);
    /*如果关机后由闹钟唤醒，不想立马执行闹钟回调
      则使用该判断，留到alarm_init()再去判断时间发消息执行*/
    if (app_var.start_time == 0) {
        return;
    }
    msg[1] = DEVICE_EVENT_IN;
    app_send_message_from(MSG_FROM_DEVICE, sizeof(msg), msg);
}

static int alarm_event_handler(int *msg)
{
    int ret = 0;
    if (msg[0] == DEVICE_EVENT_FROM_ALM) {
        switch (msg[1]) {
        case DEVICE_EVENT_IN:
            printf("<%s>", __func__);
            alarm_update_info_after_isr();
            UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_ALARM_RINGING, alarm_ui_entry, alarm_ui_exit, UI_WINDOW_PREEMPTION_TYPE_ALARM);
            ret = 1;
            break;
        default:
            break;
        }
    }
    return ret; /* 1:截获alarm消息,app应用不会收到消息 0:消息继续分发 */
}

APP_MSG_PROB_HANDLER(alarm_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_DEVICE,
    .handler    = alarm_event_handler,
};

#endif

