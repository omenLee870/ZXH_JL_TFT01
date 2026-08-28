#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_default_msg_handler.data.bss")
#pragma data_seg(".app_default_msg_handler.data")
#pragma const_seg(".app_default_msg_handler.text.const")
#pragma code_seg(".app_default_msg_handler.text")
#endif
#include "app_main.h"
#include "init.h"
#include "app_config.h"
#include "app_default_msg_handler.h"
#include "dev_status.h"
#include "alarm.h"
#include "idle.h"
#include "audio_config.h"
#include "app_tone.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "bt.h"
#include "node_param_update.h"
#include "bass_treble.h"
#include "usb/device/usb_stack.h"
#include "asm/charge.h"
#include "asm/sdmmc.h"
#include "record.h"
#include "mix_record_api.h"
#include "common_api.h"
#include "video_api.h"
#include "video_rec.h"
#include "media_file_list.h"

static u32 input_number = 0;
static u16 input_number_timer = 0;
static void input_number_timeout(void *p)
{
#if TCFG_APP_MUSIC_EN
    input_number_timer = 0;
    printf("input_number = %d\n", input_number);
    if (app_in_mode(APP_MODE_MUSIC)) {
        app_send_message(APP_MSG_MUSIC_PLAY_BY_NUM, (int)input_number);
    }
    input_number = 0;
#endif
}

static void input_number_deal(u32 num)
{
#if TCFG_APP_MUSIC_EN
    input_number = input_number * 10 + num;
    if (input_number > 9999) {
        input_number = num;
    }
    printf("num = %d, input_number = %d, input_number_timer = %d\n", num, input_number, input_number_timer);
    if (input_number_timer == 0) {
        input_number_timer = sys_timeout_add(NULL, input_number_timeout, 1000);
    } else {
        sys_timer_modify(input_number_timer, 1000);
    }
    app_send_message(APP_MSG_INPUT_FILE_NUM, input_number);
#endif
}
static u8 sys_audio_mute_statu = 0;//记录 audio dac mute

u8 get_sys_aduio_mute_statu(void)
{
    return 	app_audio_get_dac_digital_mute();
}
void app_common_key_msg_handler(int *msg)
{


    int from_tws = msg[1];

    switch (msg[0]) {
    case APP_MSG_VOL_UP:
        app_audio_volume_up(1);
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume()) {
            if (tone_player_runing() == 0) {
#if TCFG_MAX_VOL_PROMPT
                play_tone_file(get_tone_files()->max_vol);
#endif
            }
        }
        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
    case APP_MSG_VOL_DOWN:
        app_audio_volume_down(1);
        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
#if AUD_MEDIA_EFx_SCENE_SWITCH_ENABLE
    case APP_MSG_SWITCH_SOUND_EFFECT:
        effect_scene_switch();
        break;
#endif
#if TCFG_MIC_EFFECT_ENABLE
    case APP_MSG_MIC_EFFECT_ON_OFF://混响开关

        if (from_tws == APP_KEY_MSG_FROM_TWS) {
            break;
        }

#if TCFG_APP_BT_EN
        if (bt_get_call_status() != BT_CALL_HANGUP) {//通话中
            break;
        }
#endif
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        } else {
            mic_effect_player_open();
        }
        break;
#if AUD_MIC_EFx_SCENE_SWITCH_ENABLE
    case APP_MSG_SWITCH_MIC_EFFECT://混响音效场景切换
        mic_effect_scene_switch();
        break;
#endif
    case APP_MSG_MIC_VOL_UP:
        mic_effect_dvol_up();
        break;
    case APP_MSG_MIC_VOL_DOWN:
        mic_effect_dvol_down();
        break;
    case APP_MSG_MIC_BASS_UP:
        puts("\n APP_MSG_MIC_BASS_UP \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_LOW, 20);
        break;
    case APP_MSG_MIC_BASS_DOWN:
        puts("\n APP_MSG_MIC_BASS_DOWN \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_LOW, -20);
        break;
    case APP_MSG_MIC_TREBLE_UP:
        puts("\n APP_MSG_MIC_TREBLE_UP \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_HIGH, 20);
        break;
    case APP_MSG_MIC_TREBLE_DOWN:
        puts("\n APP_MSG_MIC_TREBLE_DOWN \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_HIGH, -20);
        break;
#endif
    case APP_MSG_VOCAL_REMOVE:
#if TCFG_APP_BT_EN
        if (bt_get_call_status() != BT_CALL_HANGUP) {//通话中
            break;
        }
#endif
        music_vocal_remover_switch();
        break;
    case APP_MSG_MUSIC_CHANGE_EQ:
        //  for test eq切换接口,工具上需要配置多份EQ配置
        u8 music_eq_preset_index = get_music_eq_preset_index();
        music_eq_preset_index ^= 1;
        set_music_eq_preset_index(music_eq_preset_index);
        eq_update_parm(get_current_scene(), "Eq0Media", music_eq_preset_index);
        app_send_message(APP_MSG_EQ_CHANGED, get_music_eq_preset_index() + 1); //显示EQ从1开始
        printf("APP_MSG_MUSIC_CHANGE_EQ:%d\n", music_eq_preset_index);
        break;

    case APP_MSG_SYS_MUTE:
        sys_audio_mute_statu = app_audio_get_dac_digital_mute() ^ 1;
        if (sys_audio_mute_statu) {
            app_audio_mute(AUDIO_MUTE_DEFAULT);
        } else {
            app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        }
        app_send_message(APP_MSG_MUTE_CHANGED, sys_audio_mute_statu);
        break;
    case APP_MSG_REC_PP:
#if TCFG_MIX_RECORD_ENABLE
        if (app_get_current_mode()->name == APP_MODE_MUSIC) {
            r_printf("[Error]Music Mode no support record!\n");
            break;
        } else if (app_get_current_mode()->name == APP_MODE_PC) {
            r_printf("[Error]PC Mode no support record!\n");
            break;
        }

        printf("\n APP_MSG_REC_PP \n");
        if (get_mix_recorder_status()) {
            mix_recorder_stop();
        } else {
            mix_recorder_start();
        }
#endif // TCFG_MIX_RECORD_ENABLE
        break;
    case APP_MSG_IR_NUM0:
    case APP_MSG_IR_NUM1:
    case APP_MSG_IR_NUM2:
    case APP_MSG_IR_NUM3:
    case APP_MSG_IR_NUM4:
    case APP_MSG_IR_NUM5:
    case APP_MSG_IR_NUM6:
    case APP_MSG_IR_NUM7:
    case APP_MSG_IR_NUM8:
    case APP_MSG_IR_NUM9:
        input_number_deal(msg[0]  - APP_MSG_IR_NUM0);
        break;
    case APP_MSG_CURRENT_BAT:
        printf("[BT_KEY]:APP_MSG_CURRENT_BAT\n");
        common_api_get_bat_percent();
        break;
    case APP_MSG_POWER_OFF:
        printf("[KEY]:APP_MSG_POWER_OFF\n");
        common_api_reboot_or_shutdown(0, 0);
        break;
    case APP_MSG_DISCON_RESET_BT:
        printf("[KEY]:APP_MSG_DISCON_RESET_BT\n");
        common_api_disconnect_all_restore();
        break;
    case APP_MSG_START_PAIR:
        printf("[KEY]:APP_MSG_START_PAIR\n");
        common_api_disconnect_start_pair();
        break;
    case APP_MSG_TAKE_PHOTO:
        printf("[BT_KEY]:APP_MSG_TAKE_PHOTO\n");
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
        camera_api_take_photo_or_stop_rec();
#endif
        break;
    case APP_MSG_RECORD_START:
        printf("[BT_KEY]:APP_MSG_RECORD_START\n");
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
        camera_api_record_video();
#endif
        break;
    }
}

void app_common_device_event_handler(int *msg)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch (msg[0]) {
    case DEVICE_EVENT_FROM_OTG:
        usb_msg = (const char *)msg[2];
        if (usb_msg[0] == 's') {
#if TCFG_USB_SLAVE_ENABLE
            ret = pc_device_event_handler(msg);
            if (ret == 1) {
                if (true != app_in_mode(APP_MODE_PC)) {
                    app = APP_MODE_PC;
                }
            } else if (ret == 2) {
                app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
            }
#endif
            break;
        } else if (usb_msg[0] == 'h') {
            ///是主机, 统一于SD卡等响应主机处理，这里不break
        } else {
            break;
        }
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
        printf("[UVC_FLOW] app_default_msg_handler: USB_HOST event, msg1=%d msg2=%s\n", msg[1], (char *)msg[2]);
#if TCFG_HOST_UVC_ENABLE
        if (!strncmp((char *)msg[2], "uvc", 3) && msg[1] == DEVICE_EVENT_IN) {
            printf("[UVC_FLOW] UVC device online, notifying video subsystem\n");
            extern int uvc_dev_notify_online(char *name);
            uvc_dev_notify_online((char *)msg[2]);
        }
#endif
#if TCFG_APP_MUSIC_EN
        if (true == app_in_mode(APP_MODE_MUSIC)) {
            music_device_msg_handler(msg);
        }
#endif
        ret = dev_status_event_filter(msg);///设备上下线，设备挂载等处理
        if (ret == true) {
            if (msg[1] == DEVICE_EVENT_IN) {
#if TCFG_APP_MUSIC_EN
                if (true != app_in_mode(APP_MODE_MUSIC)) {
                    app = APP_MODE_MUSIC;
                }
#endif
            }
        }
#if (TCFG_SD0_ENABLE && TCFG_SD0_ALWAY_ONLINE_ENABLE)
        sdmmc_dev_dect_scan_timer_suspend("sd0");
#endif
#if (TCFG_SD1_ENABLE && TCFG_SD1_ALWAY_ONLINE_ENABLE)
        sdmmc_dev_dect_scan_timer_suspend("sd1");
#endif
#if TCFG_MIX_RECORD_ENABLE
        mix_record_device_msg_handler(msg);
#endif // TCFG_MIX_RECORD_ENABLE
#if (TCFG_APP_VIDEO_REC_EN || TCFG_APP_VIDEO_DEC_EN || TCFG_APP_PHOTO_EN)
        /* 挂载成功(ret==true)才重建, 重复上线/失败跳过防写半死卡 */
        if (msg[1] == DEVICE_EVENT_IN && ret == true) {
            flm_rebuild_async();
        }
        app_send_message(APP_MSG_SD_EVENT, msg[1]);
#endif
        break;
    case DEVICE_EVENT_FROM_LINEIN:
#if TCFG_APP_LINEIN_EN
        if (msg[1] == DEVICE_EVENT_IN) {
            printf("LINEIN ONLINE");
            if (true != app_in_mode(APP_MODE_LINEIN)) {
                app = APP_MODE_LINEIN;
            }
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            printf("LINEIN OFFLINE");
            if (true == app_in_mode(APP_MODE_LINEIN)) {
                app_send_message(APP_MSG_CHANGE_MODE, 0);
            }
        }
#endif
        break;
    case DEVICE_EVENT_FROM_ALM:
#if TCFG_APP_RTC_EN
        if (msg[1] == DEVICE_EVENT_IN) {
            printf("ALM ONLINE");
            alarm_update_info_after_isr();
            if (true != app_in_mode(APP_MODE_RTC)) {
                app = APP_MODE_RTC;
            } else {
                alarm_ring_start();
            }
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            printf("ALM OFFLINE");
        }
#endif
        break;

    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        break;
    }

    if (app != 0xff) {
        /*一些情况不希望退出蓝牙模式*/
#if TCFG_APP_BT_EN
        if (bt_app_exit_check() == 0) {
            puts("bt_background can not enter\n");
            return;
        }
#endif
        //PC 不响应因为设备上线引发的模式切换
        if ((true != app_in_mode(APP_MODE_PC)) || alarm_flag) {
            //闹钟响起直接切到rtc模式
            if (alarm_flag) {
                app_send_message2(APP_MSG_GOTO_MODE, app, 0);
                return;
            }

#if (TCFG_CHARGE_ENABLE && (!TCFG_CHARGE_POWERON_ENABLE))
            if (get_charge_online_flag()) {
                return;
            }
#endif

#if TWFG_APP_POWERON_IGNORE_DEV
            int msec = jiffies_msec2offset(app_var.start_time, jiffies_msec());
            if (msec > TWFG_APP_POWERON_IGNORE_DEV)
#endif
            {
#if 0//defined(TCFG_BROADCAST_ENABLE) && (TCFG_BROADCAST_ENABLE)
                if (get_broadcast_role() != BROADCAST_ROLE_RECEIVER) {
                    app_send_message2(APP_MSG_GOTO_MODE, app, 0);
                }
#else
                app_send_message2(APP_MSG_GOTO_MODE, app, 0);
#endif
            }
        }
    }
}

static void app_common_app_event_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_KEY_POWER_OFF:
    case APP_MSG_KEY_POWER_OFF_HOLD:
        if (msg[1] == APP_KEY_MSG_FROM_TWS) { //来自tws的按键关机消息不响应
            break;
        }
        power_off_deal(APP_MSG_KEY_POWER_OFF);
        break;
    case APP_MSG_KEY_POWER_OFF_RELEASE:
        goto_poweroff_first_flag = 0;
        break;
    case APP_MSG_KEY_POWER_OFF_INSTANTLY:
        power_off_instantly();
        break;
    default:
        break;
    }
}

void app_default_msg_handler(int *msg)
{
    const struct app_msg_handler *handler;
    struct app_mode *mode;

    mode = app_get_current_mode();
    //消息继续分发

#if (TCFG_BT_BACKGROUND_ENABLE)
    if (!bt_background_msg_forward_filter(msg))
#endif
    {
        for_each_app_msg_handler(handler) {
            if (handler->from != msg[0]) {
                continue;
            }
            if (mode && mode->name == handler->owner) {
                continue;
            }

            /*蓝牙后台情况下，消息仅转发给后台处理*/
            handler->handler(msg + 1);
        }
    }

    switch (msg[0]) {
    case MSG_FROM_DEVICE:
        app_common_device_event_handler(msg + 1);
        break;
    case MSG_FROM_APP:
        app_common_app_event_handler(msg + 1);
        break;
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#if (defined TCFG_CHARGE_BOX_ENABLE && TCFG_CHARGE_BOX_ENABLE)
    case MSG_FROM_CHGBOX: {
        extern int charge_box_ctrl_event_handler(int *msg);
        charge_box_ctrl_event_handler(msg + 1);
    }
    break;
    case MSG_FROM_CHGBOX_EAR: {
        extern void app_chargebox_task_handler(int *msg);
        app_chargebox_task_handler(msg + 1);
    }
    break;
#endif
#endif
    default:
        break;
    }
}
