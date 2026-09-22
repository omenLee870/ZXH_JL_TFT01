#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".kws_voice_event_deal.data.bss")
#pragma data_seg(".kws_voice_event_deal.data")
#pragma const_seg(".kws_voice_event_deal.text.const")
#pragma code_seg(".kws_voice_event_deal.text")
#endif
#include "event.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "asr/kws_event.h"
#include "key_driver.h"
#include "app_msg.h"
#include "bt_key_func.h"
#include "ui/ui_api.h"
#include "jlui_app/ui_sys_param.h"
#include "app_mode_manager/app_mode_manager.h"
#include "app_task.h"
#include "esco_player.h"
#include "app_common.h"
#include "music/music_player.h"
/* #include "media/file_decoder.h" */
#include "data_storage.h"
#include "audio_config.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#define LOG_TAG             "[KWS_VOICE_EVENT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if ((defined TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE) && (TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE))

#define KWS_CHECK_EVENT						BIT(0)
#define KWS_CHECK_ID						BIT(1)
#define KWS_CHECK_MODE						BIT(2)
#define KWS_CHECK_BT						BIT(3)
#define KWS_CHECK_PHONE_BT					BIT(4)
#define KWS_CHECK_UI						BIT(5)

#define KWS_EVENT_ERR_CHECK_EVENT			(-1)
#define KWS_EVENT_ERR_CHECK_ID				(-2)
#define KWS_EVENT_ERR_CHECK_MODE			(-3)
#define KWS_EVENT_ERR_CHECK_BT				(-4)
#define KWS_EVENT_ERR_CHECK_UI				(-5)
#define KWS_EVENT_ERR_UI_SHOW				(-6)


extern u8 is_bredr_close();
extern void bt_init_bredr();
extern void bredr_conn_last_dev();
extern void bt_close_bredr();
extern void set_call_log_type(u8 type);

extern int music_pp(void);
extern int music_prev(void);
extern int music_next(void);

extern void volume_up();
extern void volume_down();
extern void volume_set(u8 vol);

extern int watch_set_style(int style);
extern int watch_get_items_num();
extern char *watch_get_item(int style);
extern int watch_version_juge(char *watch_item);
extern int watch_get_style();

extern void kws_hold_time_enable(void);
extern int bt_must_work(void);
extern void music_set_start_auto_play(u8 on);
extern u8 create_control_by_menu_set(u8 en);
extern void ui_auto_shut_down_re_run(void);
extern void setting_write_UIInfo_to_vm(void *info);
extern void save_ui_info_to_vm();

static int kws_event_common_deal(u16 event, u32 id);
static int kws_event_app_list_deal(u16 event, u32 id);
static int kws_event_music_deal(u16 event, u32 id);
static int kws_event_call_deal(u16 event, u32 id);
static int kws_event_volume_deal(u16 event, u32 id);
static int kws_event_brightness_deal(u16 event, u32 id);
static int kws_event_bt_setting_deal(u16 event, u32 id);
static int kws_event_start_photos_deal(u16 event, u32 id);
static int kws_event_switch_dial_deal(u16 event, u32 id);
static int kws_event_switch_style_deal(u16 event, u32 id);
static int kws_event_ui_show_ID(u32 id, u8 checkid);

static int kws_event_ai_glasses_xiaojie_deal(u16 event, u32 id);
static int kws_event_ai_glasses_music_deal(u16 event, u32 id);
static int kws_event_ai_glasses_volume_deal(u16 event, u32 id);
static int kws_event_ai_glasses_anc_deal(u16 event, u32 id);

extern int UIInfo_w_vm_timer;

struct jl_kws_event_hdl {
    u32 last_event;
    u32 last_event_jiffies;
    u32 last_ui_id;
};

static struct jl_kws_event_hdl kws_hdl = {
    .last_event = 0,
    .last_event_jiffies = 0,
    .last_ui_id = 0,
};

#define __this 		(&kws_hdl)


struct jl_kws_event_ui_key {
    u16 event;	// 事件
    u32 ui_id;	// UI界面
    int (*deal)(u16 event, u32 ui_id);	// 执行处理
};

#if TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
static const struct jl_kws_event_ui_key kws_event_tab[] = {
    /*音乐关键词*/
    {KWS_EVENT_PLAY_MUSIC,   	 	ID_WINDOW_MUSIC_PLAYER,     kws_event_music_deal},
    {KWS_EVENT_STOP_MUSIC,   	 	0,							kws_event_music_deal},
    {KWS_EVENT_PAUSE_MUSIC,  	 	0,							kws_event_music_deal},
    {KWS_EVENT_PREV_SONG,    	 	0,							kws_event_music_deal},
    {KWS_EVENT_NEXT_SONG,    	 	0,							kws_event_music_deal},

    /*通话关键词*/
    {KWS_EVENT_CALL_ACTIVE,  	 	0,							kws_event_call_deal},
    {KWS_EVENT_CALL_HANGUP,  	 	0,							kws_event_call_deal},

    /*音量关键词*/
    {KWS_EVENT_VOLUME_UP,    	 	0,							kws_event_volume_deal},
    {KWS_EVENT_VOLUME_DOWN,  	 	0,							kws_event_volume_deal},
    {KWS_EVENT_VOLUME_MUTE,			0,							kws_event_volume_deal},
    {KWS_EVENT_VOLUME_UNMUTE, 		0,							kws_event_volume_deal},
    {KWS_EVENT_VOLUME_MAX, 			0,							kws_event_volume_deal},

    /*亮度调整关键词*/
    {KWS_EVENT_BRIGHTNESS_ALWAYS,	0,							kws_event_brightness_deal},
    {KWS_EVENT_BRIGHTNESS_UP,		0,							kws_event_brightness_deal},
    {KWS_EVENT_BRIGHTNESS_DOWN,		0,							kws_event_brightness_deal},
    {KWS_EVENT_BRIGHTNESS_AUTO,		0,							kws_event_brightness_deal},

    /*健康测量关键词*/
    {KWS_EVENT_DETECTION_HEART,		ID_WINDOW_HEART,			kws_event_common_deal},
    {KWS_EVENT_DETECTION_OXYGEN,	ID_WINDOW_OXYGEN,		    kws_event_common_deal},

    /*蓝牙应用关键词*/
    {KWS_EVENT_OPEN_EDR,			0,		                    kws_event_bt_setting_deal},
    {KWS_EVENT_FIND_PHONE,			ID_WINDOW_FINDPHONE,		kws_event_common_deal},
    {KWS_EVENT_START_PHOTOS,		ID_WINDOW_PHOTOGRAGH,	    kws_event_common_deal},

    /*表盘应用关键词*/
    {KWS_EVENT_SWITCH_DIAL,			ID_WINDOW_DIAL,				kws_event_switch_dial_deal},
    {KWS_EVENT_SWITCH_STYLE,		0,							kws_event_switch_style_deal},

    /*记录查看关键词*/
    {KWS_EVENT_SEE_SPORT_RECORD,	ID_WINDOW_SPORT_RESULT,		kws_event_common_deal},
    {KWS_EVENT_SEE_ACTION_RECORD,	ID_WINDOW_MOMENTUM,		    kws_event_common_deal},
    {KWS_EVENT_SEE_SLEEP_RECORD,	ID_WINDOW_SLEEP,			kws_event_common_deal},
    {KWS_EVENT_SEE_CALL_REDORD,		ID_WINDOW_PHONE,		    kws_event_common_deal},
    {KWS_EVENT_SEE_TRAIN_RECORD,	ID_WINDOW_SPORTING,		    kws_event_common_deal},
    {KWS_EVENT_SEE_HEAT,			ID_WINDOW_HEAT,			    kws_event_common_deal},

    /*打开功能页面关键词*/
    {KWS_EVENT_OPEN_SPORT,			ID_WINDOW_OUTDOOR_SPORTS,   kws_event_common_deal},
    {KWS_EVENT_OPEN_TRAIN,		 	ID_WINDOW_SPORT_TARGET,	    kws_event_common_deal},
    {KWS_EVENT_OPEN_CALCULAGRAPH,	ID_WINDOW_TIMER,            kws_event_common_deal},
    {KWS_EVENT_OPEN_CALL_DIAL,	 	ID_WINDOW_PHONE_KEYPAD,		kws_event_common_deal},
    {KWS_EVENT_OPEN_PHONEBOOK,	 	ID_WINDOW_PHONE,		    kws_event_common_deal},
    {KWS_EVENT_OPEN_ALARM,		 	ID_WINDOW_ALARM,			kws_event_common_deal},
    {KWS_EVENT_OPEN_STOPWATCH,	 	ID_WINDOW_STOPWATCH,		kws_event_common_deal},
    {KWS_EVENT_OPEN_WEATHER,		ID_WINDOW_WEATHER,			kws_event_common_deal},
    {KWS_EVENT_OPEN_MESS,		 	ID_WINDOW_NOTICE,			kws_event_common_deal},
    {KWS_EVENT_OPEN_SET,			ID_WINDOW_SETTING,			kws_event_common_deal},
    {KWS_EVENT_OPEN_APP_LIST,	 	0,							kws_event_app_list_deal},
    {KWS_EVENT_OPEN_BREATH_TRAIN, 	ID_WINDOW_BREATH_TRAIN,		kws_event_common_deal},
    {KWS_EVENT_OPEN_BARO,		 	0,				            kws_event_common_deal},
    {KWS_EVENT_OPEN_COMPASS,	 	ID_WINDOW_COMPASS,			kws_event_common_deal},
    {KWS_EVENT_OPEN_CARD_BAG,	 	0,			                kws_event_common_deal},
    {KWS_EVENT_OPEN_ALIPAY,		 	ID_WINDOW_ALIPAY,			kws_event_common_deal},
    {KWS_EVENT_OPEN_FLASHLIGHT,	 	ID_WINDOW_FLASHLIGHT,		kws_event_common_deal},
    {KWS_EVENT_OPEN_CALENDAR,	 	ID_WINDOW_CALENDAR,			kws_event_common_deal},
    {KWS_EVENT_OPEN_CALCULATOR,	 	ID_WINDOW_CALCULATOR,		kws_event_common_deal},

    {0, 0, 0},
};
#else
static const struct jl_kws_event_ui_key kws_event_tab[] = {
};
#endif //CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE || CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE

#elif TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES
static const struct jl_kws_event_ui_key kws_event_tab[] = {
    {KWS_EVENT_XIAOJIE,   	 	    0,                          kws_event_ai_glasses_xiaojie_deal},
    {KWS_EVENT_PLAY_MUSIC,   	 	0,                          kws_event_ai_glasses_music_deal},
    {KWS_EVENT_STOP_MUSIC,   	 	0,                          kws_event_ai_glasses_music_deal},
    {KWS_EVENT_PAUSE_MUSIC,   	 	0,                          kws_event_ai_glasses_music_deal},
    {KWS_EVENT_VOLUME_UP,   	 	0,                          kws_event_ai_glasses_volume_deal},
    {KWS_EVENT_VOLUME_DOWN,   	 	0,                          kws_event_ai_glasses_volume_deal},
    {KWS_EVENT_PREV_SONG,   	 	0,                          kws_event_ai_glasses_music_deal},
    {KWS_EVENT_NEXT_SONG,   	 	0,                          kws_event_ai_glasses_music_deal},
    {KWS_EVENT_ANC_ON,   	 	    0,                          kws_event_ai_glasses_anc_deal},
    {KWS_EVENT_ANC_OFF,   	 	    0,                          kws_event_ai_glasses_anc_deal},
    {KWS_EVENT_TRANSARENT_ON,   	0,                          kws_event_ai_glasses_anc_deal},
    {0, 0, 0},
};

#else
static const struct jl_kws_event_ui_key kws_event_tab[] = {
};
#endif //TCFG_SUB_CASE_TYPE

static int kws_get_event_index(u16 event)
{
    int index = -1;
    for (int i = 0; i < ARRAY_SIZE(kws_event_tab); i++) {
        if (kws_event_tab[i].event == event) {
            index = i;
            break;
        }
    }
    return index;
}


static int kws_admittance_check_mode(void)
{
    u8 cur_task = app_get_current_mode_name();
    switch (cur_task) {
    case APP_MODE_POWERON:
    case APP_MODE_UPDATE:
    case APP_MODE_SMARTBOX:
        // 这些模式不支持跳转
        log_e("cur task:%d no support swtich \n", cur_task);
        return false;
    /* break; */
    default:
        break;
    }
    return true;
}

static int kws_admittance_check_phone_bt(void)
{
    if (bt_must_work()) {
        log_e("phone bt busy \n");
        return false;
    }
    return true;
}

static int kws_admittance_check_bt(void)
{
    if (esco_player_runing()) {
        log_e("bt busy \n");
        return false;
    }
    return true;
}

static int kws_admittance_check_ui(void)
{
    if (UI_WINDOW_PREEMPTION_CHECK()) {
        log_e("ui busy \n");
        return false;
    }
    return true;
}

static int kws_admittance_check(u16 event, u32 id, u32 check_type)
{
    int index = 0;
    if (check_type & KWS_CHECK_EVENT) {
        index = kws_get_event_index(event);
        if (index < 0) {
            log_e("event index err \n");
            return KWS_EVENT_ERR_CHECK_EVENT;
        }
    }
    if ((check_type & KWS_CHECK_ID) && (id == 0)) {
        return KWS_EVENT_ERR_CHECK_ID;
    }
    if ((check_type & KWS_CHECK_MODE) && (!kws_admittance_check_mode())) {
        return KWS_EVENT_ERR_CHECK_MODE;
    }
    if ((check_type & KWS_CHECK_BT) && (!kws_admittance_check_bt())) {
        return KWS_EVENT_ERR_CHECK_BT;
    }
    if ((check_type & KWS_CHECK_PHONE_BT) && (!kws_admittance_check_phone_bt())) {
        return KWS_EVENT_ERR_CHECK_BT;
    }
    if (((check_type & KWS_CHECK_UI) && !kws_admittance_check_ui())) {
        return KWS_EVENT_ERR_CHECK_UI;
    }
    return index;
}

static void kws_event_ui_show_push(int (*callback)(int))
{
    int argv[3];
    argv[0] = (int)callback;
    argv[1] = 1;
    argv[2] = (int)0;
    os_taskq_post_type("ui", Q_CALLBACK, ARRAY_SIZE(argv), argv);
}

#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
static int kws_event_ui_show_ID(u32 id, u8 checkid)
{
    if (__this->last_ui_id) {
        UI_WINDOW_BACK_DEL(__this->last_ui_id);
    }
    __this->last_ui_id = id;

    if (get_screen_saver_status()) {
        /* ui_screen_recover(0); */
        ui_auto_shut_down_enable();
        /* UI_HIDE_CURR_WINDOW(); */
        if (id == ID_WINDOW_NOTICE) {
            create_control_by_menu_set(1);
        }
        UI_SHOW_WINDOW(id);
    } else {
        ui_auto_shut_down_re_run();
        if ((UI_GET_WINDOW_ID() != id) || (!checkid)) {
            UI_HIDE_CURR_WINDOW();
            if (id == ID_WINDOW_NOTICE) {
                create_control_by_menu_set(1);
            }
            UI_SHOW_WINDOW(id);
        }
    }
    return 0;
}
#else
static int kws_event_ui_show_ID(u32 id, u8 checkid)
{
    return 0;
}
#endif

static int kws_event_common_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_ID | KWS_CHECK_MODE | KWS_CHECK_BT | KWS_CHECK_UI);
    if (ret) {
        return ret;
    }
    kws_event_ui_show_ID(id, 1);
    return 0;
}

static int kws_event_app_list_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE | KWS_CHECK_BT | KWS_CHECK_UI);
    if (ret) {
        return ret;
    }
    if (false == ui_check_list_tyep(UI_GET_WINDOW_ID())) {
        ret = ui_show_menu_page();
        if (ret == false) {
            return KWS_EVENT_ERR_UI_SHOW;
        }
        ui_auto_shut_down_re_run();
    }
    return 0;
}

static int kws_event_music_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE | KWS_CHECK_BT | KWS_CHECK_PHONE_BT);
    if (ret) {
        return ret;
    }
    u8 cur_task = app_get_curr_task();
    if (0) {
#if TCFG_APP_MUSIC_EN
    } else if (cur_task == APP_MUSIC_TASK) {
        switch (event) {
        case KWS_EVENT_PLAY_MUSIC:
            if (music_player_get_play_status() != FILE_PLAYER_START) {
                app_send_message(APP_MSG_MUSIC_PP, 0);
            }
            break;
        case KWS_EVENT_STOP_MUSIC:
        case KWS_EVENT_PAUSE_MUSIC:
            if (music_player_get_play_status() == FILE_PLAYER_START) {
                app_send_message(APP_MSG_MUSIC_PP, 0);
            }
            break;
        case KWS_EVENT_PREV_SONG:
            app_send_message(APP_MSG_MUSIC_PREV, 0);
            break;
        case KWS_EVENT_NEXT_SONG:
            app_send_message(APP_MSG_MUSIC_NEXT, 0);
            break;
        default :
            break;
        }
#endif /* #if TCFG_APP_MUSIC_EN */
    } else if (cur_task == APP_BT_TASK) {
        u8 a2dp_state = bt_a2dp_get_status();
        switch (event) {
        case KWS_EVENT_PLAY_MUSIC:
            if (a2dp_state != BT_MUSIC_STATUS_STARTING) {
#if TCFG_APP_MUSIC_EN
                if (bt_get_connect_status() ==  BT_STATUS_WAITINT_CONN) {
                    log_info("switch music mode\n");
                    music_set_start_auto_play(1);
                    app_task_switch_to(APP_MODE_MUSIC, 0);
                    break;
                }
#endif /* #if TCFG_APP_MUSIC_EN */
                log_info("send PLAY cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            }
            break;
        case KWS_EVENT_STOP_MUSIC:
            if (a2dp_state == BT_MUSIC_STATUS_STARTING) {
                log_info("send STOP cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_STOP, 0, NULL);
            }
            break;
        case KWS_EVENT_PAUSE_MUSIC:
            if (a2dp_state == BT_MUSIC_STATUS_STARTING) {
                log_info("send PAUSE cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
            }
            break;
        case KWS_EVENT_PREV_SONG:
            log_info("Send PREV cmd");
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
            break;
        case KWS_EVENT_NEXT_SONG:
            log_info("Send NEXT cmd");
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
            break;
        default :
            break;
        }
    } else {
        if (event == KWS_EVENT_PLAY_MUSIC) {
#if TCFG_APP_MUSIC_EN
            if (app_get_current_mode_name() == APP_MODE_BT) {
                log_info("switch music mode\n");
                music_set_start_auto_play(1);
                app_task_switch_to(APP_MODE_MUSIC, 0);
            }
#endif /* #if TCFG_APP_MUSIC_EN */
            if (app_get_current_mode_name() == APP_MODE_MUSIC) {
                log_info("switch bt mode, send PLAY cmd\n");
                app_task_switch_to(APP_MODE_BT, 0);
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            }
        }
    }
    if ((event == KWS_EVENT_PLAY_MUSIC) && id) {
        ret = kws_admittance_check(event, id, KWS_CHECK_UI);
        if (ret == 0) {
            kws_event_ui_show_ID(id, 1);
        }
    }
    return 0;
}

static int kws_event_call_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE);
    if (ret) {
        return ret;
    }
    switch (event) {
    case KWS_EVENT_CALL_ACTIVE:
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            log_info("Send ANSWER cmd");
            call_ctrl_answer();
        }
        break;

    case KWS_EVENT_CALL_HANGUP:
        log_info("Send HANG UP cmd");
        if ((bt_get_call_status() >= BT_CALL_INCOMING) && (bt_get_call_status() <= BT_CALL_ALERT)) {
            small_file_call_log_set_type(CALL_INCOME_REJECT);
            call_ctrl_hangup();
        }
        break;
    default :
        break;
    }

    return 0;
}

static int kws_event_volume_deal(u16 event, u32 id)
{
    /* int ret = kws_admittance_check(event, id, KWS_CHECK_MODE | KWS_CHECK_BT); */
    /* if (ret) { */
    /* return ret; */
    /* } */
    switch (event) {
    case KWS_EVENT_VOLUME_UP:
        log_info("volume up\n");
        volume_up();
        break;
    case KWS_EVENT_VOLUME_DOWN:
        log_info("volume down\n");
        volume_down();
        break;
    case KWS_EVENT_VOLUME_MUTE:
        log_info("volume mute\n");
        if (!ui_get_voice_mute()) {
            ui_set_voice_mute(1);
        }
        break;
    case KWS_EVENT_VOLUME_UNMUTE:
        log_info("volume unmute\n");
        if (ui_get_voice_mute()) {
            ui_set_voice_mute(0);
        }
        break;
    case KWS_EVENT_VOLUME_MAX:
        log_info("volume max\n");
        app_audio_set_volume(APP_AUDIO_STATE_WTONE, app_audio_volume_max_query(SysVol_TONE), 1);
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
        set_ui_sys_param(LastSysVol, 100);
        save_ui_info_to_vm();
#endif
        break;
    default :
        break;
    }

    return 0;
}

static int kws_event_brightness_deal(u16 event, u32 id)
{
    int level;
    /* int ret = kws_admittance_check(event, id, KWS_CHECK_MODE | KWS_CHECK_BT); */
    /* if (ret) { */
    /* return ret; */
    /* } */
    switch (event) {
    case KWS_EVENT_BRIGHTNESS_ALWAYS:
        screen_light_alway_switch(1);
        break;
    case KWS_EVENT_BRIGHTNESS_UP:
        level = get_light_level();
        if (level < 10) {
            level ++;
        }
        set_ui_sys_param(LightLevel, level);
        ui_ajust_light(level);
        break;
    case KWS_EVENT_BRIGHTNESS_DOWN:
        level = get_light_level();
        if (level) {
            level--;
        }
        set_ui_sys_param(LightLevel, level);
        ui_ajust_light(level);
        break;
    case KWS_EVENT_BRIGHTNESS_AUTO:
        level = 10;
        set_ui_sys_param(LightLevel, level);
        ui_ajust_light(level);
        break;
    default :
        return 0;
        /* break; */
    }
    write_UIInfo_to_vm(NULL);
    return 0;
}

static int kws_event_bt_setting_ui_reshow(int priv)
{
    /* ui_pic_show_image_by_id(PIC_EDR_SWITCH, 1); */
    return 0;
}

static int kws_event_bt_setting_deal(u16 event, u32 id)
{
    /* int ret = kws_event_common_deal(event, id); */
    /* if (ret) { */
    /*     return ret; */
    /* } */
    if (is_bredr_close()) {
#if 1
        bredr_conn_last_dev();
#else
        bt_init_bredr();
#endif//自动回连
        /* ui_pic_show_image_by_id(PIC_EDR_SWITCH, 1); */
        kws_event_ui_show_push(kws_event_bt_setting_ui_reshow);
    }
    return 0;
}


static int kws_event_switch_dial_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE);
    if (ret) {
        return ret;
    }
    int items = watch_get_items_num();
    int sel_item = watch_get_style();
    sel_item ++;
    if (sel_item >= items) {
        sel_item = 0;
    }
    ret = watch_version_juge(watch_get_item(sel_item));
    if (ret != 0) {
        log_error("watch_version_juge err %d, %d\n", sel_item, ret);
        return KWS_EVENT_ERR_UI_SHOW;
    }
    ret = watch_set_style(sel_item);
    if (ret != true) {
        log_error("watch_set_style err %d\n", sel_item);
        return KWS_EVENT_ERR_UI_SHOW;
    }

    ret = kws_admittance_check(event, id, KWS_CHECK_BT | KWS_CHECK_UI);
    if (ret == 0) {
        kws_event_ui_show_ID(id, 0);
    }

    return 0;
}

static int kws_event_switch_style_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE);
    if (ret) {
        return ret;
    }
    u8 menu_style = get_ui_sys_param(MenuStyle);
    menu_style ++;
    if (menu_style > (ui_show_menu_total_num() - 1)) {
        menu_style = 0;
    }
    set_ui_sys_param(MenuStyle, menu_style);
    write_UIInfo_to_vm(NULL);
    ret = kws_admittance_check(event, id, KWS_CHECK_BT | KWS_CHECK_UI);
    if (ret == 0) {
        ret = ui_show_menu_page();
        if (ret == false) {
            return KWS_EVENT_ERR_UI_SHOW;
        }
        ui_auto_shut_down_re_run();
    }
    return 0;
}


#if TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES
static int kws_event_ai_glasses_xiaojie_deal(u16 event, u32 id)
{
    log_info("send SIRI cmd");
    bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);

    return 0;
}

static int kws_event_ai_glasses_music_deal(u16 event, u32 id)
{
    int ret = kws_admittance_check(event, id, KWS_CHECK_MODE | KWS_CHECK_BT | KWS_CHECK_PHONE_BT);
    if (ret) {
        return ret;
    }
    u8 cur_task = app_get_curr_task();
    if (0) {
#if TCFG_APP_MUSIC_EN
    } else if (cur_task == APP_MUSIC_TASK) {
        switch (event) {
        case KWS_EVENT_PLAY_MUSIC:
            if (music_player_get_play_status() != FILE_PLAYER_START) {
                app_send_message(APP_MSG_MUSIC_PP, 0);
            }
            break;
        case KWS_EVENT_STOP_MUSIC:
        case KWS_EVENT_PAUSE_MUSIC:
            if (music_player_get_play_status() == FILE_PLAYER_START) {
                app_send_message(APP_MSG_MUSIC_PP, 0);
            }
            break;
        case KWS_EVENT_PREV_SONG:
            app_send_message(APP_MSG_MUSIC_PREV, 0);
            break;
        case KWS_EVENT_NEXT_SONG:
            app_send_message(APP_MSG_MUSIC_NEXT, 0);
            break;
        default :
            break;
        }
#endif /* #if TCFG_APP_MUSIC_EN */
    } else if (cur_task == APP_BT_TASK) {
        u8 a2dp_state = bt_a2dp_get_status();
        switch (event) {
        case KWS_EVENT_PLAY_MUSIC:
            if (a2dp_state != BT_MUSIC_STATUS_STARTING) {
#if TCFG_APP_MUSIC_EN
                if (bt_get_connect_status() ==  BT_STATUS_WAITINT_CONN) {
                    log_info("switch music mode\n");
                    music_set_start_auto_play(1);
                    app_task_switch_to(APP_MODE_MUSIC, 0);
                    break;
                }
#endif /* #if TCFG_APP_MUSIC_EN */
                log_info("send PLAY cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            }
            break;
        case KWS_EVENT_STOP_MUSIC:
            if (a2dp_state == BT_MUSIC_STATUS_STARTING) {
                log_info("send STOP cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_STOP, 0, NULL);
            }
            break;
        case KWS_EVENT_PAUSE_MUSIC:
            if (a2dp_state == BT_MUSIC_STATUS_STARTING) {
                log_info("send PAUSE cmd\n");
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
            }
            break;
        case KWS_EVENT_PREV_SONG:
            log_info("Send PREV cmd");
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
            break;
        case KWS_EVENT_NEXT_SONG:
            log_info("Send NEXT cmd");
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
            break;
        default :
            break;
        }
    } else {
        if (event == KWS_EVENT_PLAY_MUSIC) {
#if TCFG_APP_MUSIC_EN
            if (app_get_current_mode_name() == APP_MODE_BT) {
                log_info("switch music mode\n");
                music_set_start_auto_play(1);
                app_task_switch_to(APP_MODE_MUSIC, 0);
            }
#endif /* #if TCFG_APP_MUSIC_EN */
            if (app_get_current_mode_name() == APP_MODE_MUSIC) {
                log_info("switch bt mode, send PLAY cmd\n");
                app_task_switch_to(APP_MODE_BT, 0);
                bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            }
        }
    }
    return 0;
}

static int kws_event_ai_glasses_volume_deal(u16 event, u32 id)
{
    switch (event) {
    case KWS_EVENT_VOLUME_UP:
        log_info("volume up\n");
        volume_up();
        break;
    case KWS_EVENT_VOLUME_DOWN:
        log_info("volume down\n");
        volume_down();
        break;
    default :
        break;
    }

    return 0;
}

static int kws_event_ai_glasses_anc_deal(u16 event, u32 id)
{
#if TCFG_AUDIO_ANC_ENABLE
    switch (event) {
    case KWS_EVENT_ANC_ON:
        log_info("anc on\n");
        anc_mode_switch(ANC_ON, 1);
        break;
    case KWS_EVENT_ANC_OFF:
        log_info("anc off\n");
        anc_mode_switch(ANC_OFF, 1);
        break;
    case KWS_EVENT_TRANSARENT_ON:
        log_info("transarent on\n");
        anc_mode_switch(ANC_TRANSPARENCY, 1);
        break;
    default :
        break;
    }
#endif

    return 0;
}
#endif

/* ---------------------------------------------------------------------------- */
/**
 * @brief: 关键词唤醒语音事件处理流程
 *
 * @param event: 系统事件
 *
 * @return : true: 处理该事件; false: 不处理该事件, 由
 */
/* ---------------------------------------------------------------------------- */
int jl_kws_voice_event_handle(int event)
{
    u32 cur_jiffies = jiffies;
    u32 voice_event = event;

    log_info("%s: event: %d", __func__, voice_event);

    if (voice_event == __this->last_event) {
        if (jiffies_to_msecs(cur_jiffies - __this->last_event_jiffies) < 1000) {
            log_info("voice event %d same, ignore", voice_event);
            __this->last_event_jiffies = cur_jiffies;
            return true;
        }
    }
    __this->last_event_jiffies = cur_jiffies;
    __this->last_event = voice_event;

    switch (voice_event) {
#if 0
    case KWS_EVENT_HEY_KEYWORD:
    case KWS_EVENT_XIAOJIE:
        //主唤醒词:
        log_info("send SIRI cmd");
        bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
        break;

    case KWS_EVENT_XIAODU:
        //主唤醒词:
        log_info("send SIRI cmd");
        bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
        break;

#if TCFG_AUDIO_ANC_ENABLE
    case KWS_EVENT_ANC_ON:
        anc_mode_switch(ANC_ON, 1);
        break;
    case KWS_EVENT_TRANSARENT_ON:
        anc_mode_switch(ANC_TRANSPARENCY, 1);
        break;
    case KWS_EVENT_ANC_OFF:
        anc_mode_switch(ANC_OFF, 1);
        break;
#endif
#endif

    case KWS_EVENT_NULL:
        log_info("KWS_EVENT_NULL");
        break;

    default: {
        int index = kws_get_event_index(voice_event);
        if (index < 0) {
            log_error("event index err \n");
            break;
        }
        if (kws_event_tab[index].deal) {
            int ret = kws_event_tab[index].deal(voice_event, kws_event_tab[index].ui_id);
            log_info("event deal index:%d, ret:%d \n", index, ret);
        }

    }
    break;
    }

    return true;
}

static int kws_voice_event_msg_handler(int *msg)
{
    int type = msg[0];

    switch (type) {
    case APP_MSG_SMART_VOICE_EVENT:
        int event = msg[1];
        jl_kws_voice_event_handle(event);
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(kws_voice_event_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = kws_voice_event_msg_handler,
};
#endif
