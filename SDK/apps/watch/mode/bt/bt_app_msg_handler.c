#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_app_msg_handler.data.bss")
#pragma data_seg(".bt_app_msg_handler.data")
#pragma const_seg(".bt_app_msg_handler.text.const")
#pragma code_seg(".bt_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "bt_key_func.h"
#include "low_latency.h"
#include "a2dp_player.h"
#include "linein_player.h"
#include "app_tone.h"
#include "audio_config.h"
#include "vol_sync.h"
#include "bt.h"
#include "classic/tws_api.h"
#include "rtc/rtc_dev.h"
#include "common_api.h"
#include "common_music_ctrl.h"

#if TCFG_APP_BT_EN

int bt_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_BT)) {
        return 0;
    }

    u8 msg_type = msg[0];

    printf("bt_app_msg type:0x%x", msg[0]);
    switch (msg_type) {
    case APP_MSG_CHANGE_MODE:
        puts("app msg key change mode\n");
        /*一些情况不希望退出蓝牙模式*/
        if (bt_app_exit_check() == 0) {
            puts("bt_background can not enter\n");
            return 0;
        }
#if TCFG_USER_TWS_ENABLE && !TCFG_BT_BACKGROUND_ENABLE
#if CONFIG_TWS_USE_COMMMON_ADDR == 0
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            tws_api_detach(TWS_DETACH_BY_POWEROFF, 5000);   //这里从机用TWS_DETACH_BY_POWEROFF断开不会进行主从切换
            g_bt_hdl.ignore_discon_tone = 1;
        }
#endif
        if (msg[1] == APP_KEY_MSG_FROM_TWS) { //非后台不响应来自tws的切换模式消息
            return 0;
        }
#endif
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_MUSIC_PP:
        puts("app msg music pp\n");
        bt_key_music_pp();
        break;
    case APP_MSG_MUSIC_NEXT:
        puts("app msg music next\n");
        bt_key_music_next();
        break;
    case APP_MSG_MUSIC_PREV:
        puts("app msg music prev\n");
        bt_key_music_prev();
        break;
    case APP_MSG_VOL_UP:
        puts("app msg vol up\n");
        bt_key_vol_up();
        break;
    case APP_MSG_VOL_DOWN:
        puts("app msg vol down\n");
        bt_key_vol_down();
        break;
    case APP_MSG_CALL_HANGUP:
        puts("app msg call HangUp\n");
        bt_key_call_hang_up();
        break;
    case APP_MSG_CALL_LAST_NO:
        puts("app msg call last on\n");
        bt_key_call_last_on();
        break;
    case APP_MSG_OPEN_SIRI:
        puts("app msg open siri\n");
        bt_key_call_siri();
        break;
    case APP_MSG_HID_CONTROL:
        puts("app msg hid control\n");
        bt_key_hid_control();
        break;
    case APP_MSG_LOW_LANTECY:
        puts("app msg low lantecy\n");
        int state = bt_get_low_latency_mode();
        bt_set_low_latency_mode(!state);
        break;
    case APP_MSG_ADD_LINEIN_STREAM:
#if TCFG_APP_LINEIN_EN
        //工具的蓝牙流程图需要添加对应的linein数据流!!
        puts("app msg add linein stream\n");
        if (linein_player_runing()) {
            linein_player_close();
        } else {
            linein_player_open();
        }
#endif
        break;
    case APP_MSG_BT_A2DP_START:
#if TCFG_BT_BACKGROUND_ENABLE
        /*这里处理有些设备切到后台一直不推a2dp stop，手动切到蓝牙模式后能量检测还在跑，这时候点击设备播放按钮之后，
          能量检测有数据之后结束推APP_MSG_BT_A2DP_START，这种情况需要在这里打开解码*/
        u8 *bt_addr = (u8 *)&msg[1];
        ASSERT(bt_addr);
        u8 dev_vol = bt_get_music_volume(bt_addr);
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
        if (dev_vol > 127) {
            dev_vol = app_audio_bt_volume_update(bt_addr, APP_AUDIO_STATE_MUSIC);
        }
        set_music_device_volume(dev_vol);
        int err = a2dp_player_open(bt_addr);
        if (err == -EBUSY) {
            printf("bt_app_msg_handler open a2dp_player failed\n");
        }
#endif
        break;

    case APP_MSG_CALL_THREE_WAY_ANSWER1:
        bt_key_call_three_way_answer1();
        break;

    case APP_MSG_CALL_THREE_WAY_ANSWER2:
        bt_key_call_three_way_answer2();
        break;

    case APP_MSG_CALL_SWITCH:
        bt_key_call_switch();
        break;

    case APP_MSG_PITCH_UP:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg a2dp pitch up\n");
        if (a2dp_player_runing()) {
            app_var.pitch_mode = a2dp_file_pitch_up(); //返回当前变调模式
        }
#endif
        break;
    case APP_MSG_PITCH_DOWN:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg a2dp pitch down\n");
        if (a2dp_player_runing()) {
            app_var.pitch_mode = a2dp_file_pitch_down();
        }
#endif
        break;
    case APP_MSG_MUSIC_CTRL_NEXT:
        printf("[BT_KEY]:APP_MSG_MUSIC_CTRL_NEXT\n");
        music_ctrl_local_play(0);
        break;
    case APP_MSG_MUSIC_CTRL_PREV:
        printf("[BT_KEY]:APP_MSG_MUSIC_CTRL_PREV\n");
        music_ctrl_local_play(1);
        break;
    case APP_MSG_INCALL_REC:
        printf("[BT_KEY]:APP_MSG_INCALL_REC\n");
        common_api_call_income_hangup();
        break;
    default:
        app_common_key_msg_handler(msg);
        printf("unknow msg type:%d", msg_type);
        break;
    }

    return 0;
}

#endif /* #if TCFG_APP_BT_EN */
