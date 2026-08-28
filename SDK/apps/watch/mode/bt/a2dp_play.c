#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".a2dp_play.data.bss")
#pragma data_seg(".a2dp_play.data")
#pragma const_seg(".a2dp_play.text.const")
#pragma code_seg(".a2dp_play.text")
#endif
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "os/os_api.h"
#include "bt_slience_detect.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "app_main.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "btstack/a2dp_media_codec.h"
#include "bt.h"
#include "effect/effects_default_param.h"
#include "scene_switch.h"

#if TCFG_APP_BT_EN
#if(TCFG_USER_TWS_ENABLE == 0)
//开关a2dp后，是否保持变调状态
#define A2DP_PLAYBACK_PITCH_KEEP          0

enum {
    CMD_A2DP_PLAY = 1,
    CMD_A2DP_SLIENCE_DETECT,
    CMD_A2DP_CLOSE,
    CMD_SET_A2DP_VOL,
};

static u8 g_play_addr[6];
static u8 g_slience_addr[6];



/**@brief   a2dp
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_get_a2dp_en_status()
{
    return g_bt_hdl.a2dp_en;
}


void bt_set_a2dp_en_status(u8 on)
{
    printf("%s need todo \n", __FUNCTION__);
    g_bt_hdl.a2dp_en = !!on;
}

static int bt_init_a2dp_en_status(void)
{
    /* #if TCFG_USER_EMITTER_ENABLE */
    /* g_bt_hdl.a2dp_en = 0; */
    /* #else */
    g_bt_hdl.a2dp_en = 1;
    /* #endif */
    return 0;
}
platform_initcall(bt_init_a2dp_en_status);

void a2dp_play_close(u8 *bt_addr)
{
    puts("a2dp_play_close\n");
    put_buf(bt_addr, 6);
    a2dp_player_close(bt_addr);
    bt_stop_a2dp_slience_detect(bt_addr);
    a2dp_media_close(bt_addr);
}

static void a2dp_play_in_task(u8 *data)
{
    u8 btaddr[6];
    u8 dev_vol;
    u8 *bt_addr = data + 2;

    switch (data[0]) {
    case CMD_A2DP_SLIENCE_DETECT:
        puts("CMD_A2DP_SLIENCT_DETECE\n");
        put_buf(bt_addr, 6);
        a2dp_play_close(bt_addr);
        bt_start_a2dp_slience_detect(bt_addr, 50);     //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        memset(g_slience_addr, 0xff, 6);
        break;
    case CMD_A2DP_PLAY:
        puts("app_msg_bt_a2dp_play\n");
        put_buf(bt_addr, 6);
#if (TCFG_BT_A2DP_PLAYER_ENABLE == 0)
        break;
#endif
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
        dev_vol = data[8];
        //更新一下音量再开始播放
        if (dev_vol > 127) {    //返回值0xff说明不支持音量同步
            y_printf("device no support sync vol, use sys volume:%d\n", app_var.music_volume);
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
            app_audio_set_volume_def_state(0);
        } else {
            set_music_device_volume(dev_vol);
        }
        bt_stop_a2dp_slience_detect(bt_addr);
        int err = a2dp_player_open(bt_addr);
        if (err == -EBUSY) {
            bt_start_a2dp_slience_detect(bt_addr, 50); //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        }
        memset(g_play_addr, 0xff, 6);
        music_vocal_remover_update_parm();
        break;
    case CMD_A2DP_CLOSE:
        a2dp_play_close(bt_addr);
        break;
    case CMD_SET_A2DP_VOL:
        dev_vol = data[8];
        set_music_device_volume(dev_vol);
        break;
    }
}

static void a2dp_play_send_cmd(u8 cmd, u8 *_data, u8 len, u8 tx_do_action)
{
    u8 data[16];
    data[0] = cmd;
    data[1] = 2;
    memcpy(data + 2, _data, len);
    a2dp_play_in_task(data);
}

static void a2dp_play(u8 *bt_addr, bool tx_do_action)
{
    u8 data[8];
    memcpy(data, bt_addr, 6);
    data[6] = bt_get_music_volume(bt_addr);
    /* if (data[6] > 127) { */
    /*     data[6] = app_audio_bt_volume_update(bt_addr, APP_AUDIO_STATE_MUSIC); */
    /* } */
    a2dp_play_send_cmd(CMD_A2DP_PLAY, data, 7, tx_do_action);
}

static void a2dp_play_slience_detect(u8 *bt_addr, bool tx_do_action)
{
    a2dp_play_send_cmd(CMD_A2DP_SLIENCE_DETECT, bt_addr, 6, tx_do_action);
}


static int a2dp_bt_status_event_handler(int *event)
{
    int ret;
    u8 data[8];
    u8 btaddr[6];
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case BT_STATUS_A2DP_MEDIA_START:
        puts("BT_STATUS_A2DP_MEDIA_START\n");
        put_buf(bt->args, 6);
        if (app_var.goto_poweroff_flag) {
            break;
        }
        void *file = a2dp_open_media_file(bt->args);
        if (file == NULL) {
            printf("open a2dp file error \n");
            break;
        }
        a2dp_close_media_file(file);
        if (bt_get_call_status_for_addr(bt->args) == BT_CALL_INCOMING) {
            //小米11来电挂断偶现没有hungup过来，hfp链路异常，重新断开hfp再连接
            puts("<<<<<<<<waring a2dp start hfp_incoming\n");
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_DISCONNECT, 0, NULL);
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_CMD_CONN, 0, NULL);
        }
        if (esco_player_runing()) {
            a2dp_media_close(bt->args);
            break;
        }
        if (a2dp_player_get_btaddr(btaddr)) {
            if (memcmp(btaddr, bt->args, 6) == 0) {
                a2dp_play(bt->args, 1);
            } else {
                a2dp_play_slience_detect(bt->args, 1);
            }
        } else {
            a2dp_play(bt->args, 1);
        }
#if (TCFG_PITCH_SPEED_NODE_ENABLE && A2DP_PLAYBACK_PITCH_KEEP)
        audio_pitch_default_parm_set(app_var.pitch_mode);
        a2dp_file_pitch_mode_init(app_var.pitch_mode);
#endif
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        puts("BT_STATUS_A2DP_MEDIA_STOP\n");
        a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt->args, 6, 1);
        break;
    case BT_STATUS_AVRCP_VOL_CHANGE:
        //判断是当前地址的音量值才更新
        ret = a2dp_player_get_btaddr(data);
        if (ret && memcmp(data, bt->args, 6) == 0) {
            data[6] = bt->value;
            a2dp_play_send_cmd(CMD_SET_A2DP_VOL, data, 7, 1);
        }
        break;
    }
    return 0;
}

APP_MSG_HANDLER(a2dp_stack_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = a2dp_bt_status_event_handler,
};


static int a2dp_bt_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        a2dp_play_close(bt->args);
        break;
    }

    return 0;
}
APP_MSG_HANDLER(a2dp_hci_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = a2dp_bt_hci_event_handler,
};

static int a2dp_app_msg_handler(int *msg)
{
    u8 *bt_addr = (u8 *)(msg +  1);

    switch (msg[0]) {
    case APP_MSG_BT_A2DP_PAUSE:
        puts("app_msg_bt_a2dp_pause\n");
        if (a2dp_player_is_playing(bt_addr)) {
            a2dp_play_slience_detect(bt_addr, 1);
        }
        break;
    case APP_MSG_BT_A2DP_PLAY:
        puts("app_msg_bt_a2dp_play\n");
        a2dp_play(bt_addr, 1);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_app_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = a2dp_app_msg_handler,
};
#endif
#endif /* #if TCFG_APP_BT_EN */
