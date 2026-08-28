#ifndef EVENTS_ADAPTER_H_
#define EVENTS_ADAPTER_H_
#ifdef __cplusplus
extern "C" {
#endif



// #include "ui/ui_api.h"
// #include "ui/ui.h"
#include "smartbox_user_app.h"
void usr_screen_on(u8 on);




// UI >>> 蓝牙
typedef enum {
    smartbox_EQ_MODE_NORMAL = 0,
    smartbox_EQ_MODE_ROCK,
    smartbox_EQ_MODE_POP,
    smartbox_EQ_MODE_CLASSIC,
    smartbox_EQ_MODE_JAZZ,
} smartbox_EQ_MODE;

typedef enum {
    CALL_CONTROL_ANSWER = 1,
    CALL_CONTROL_HANG_UP,
    CALL_CONTROL_MUTE_OFF,
    CALL_CONTROL_MUTE_ON,
} CALL_CONTROL;

typedef enum {
    LANGUAGE_CHINESE = 1,
    LANGUAGE_ENGLISH,
} LANGUAGE_LIST;


typedef enum {
    HID_TIKTOP_PRV,
    HID_TIKTOP_NEXT,
    HID_TIKTOP_LEFT_MOVE,
    HID_TIKTOP_RIGHT_MOVE,
    HID_TIKTOP_LIKE,
    HID_TAKE_PHOTO,
} HID;

typedef enum {
    ANC_MODE_OFF = 0X01,
    ANC_MODE_ON,
    ANC_TRANSPARENCY_MODE,
    ANC_ADAPTIVE_MODE,
} ANC;

#define CMD_FIND_MY_L   (0x10)
#define CMD_FIND_MY_R   (0x01)
#define CMD_FIND_MY_LR  (0x11)
#define CMD_FIND_CLOSE  (0x03)

#define CMD_ALARM_RING  (0x12)

#define CMD_BLE_NO_LANTACY_ON   (0xf1)
#define CMD_BLE_NO_LANTACY_OFF  (0xf0)

struct s_box_info_send_cb {
    // 播放模式用于让耳机关机
    void (*smartbox_earphone_poweroff_cmd_send)(void);   // 0x12 闹钟响起

    // 播放闹钟
    void (*smartbox_alarm_ring_cmd_send)(u8 data);        // 0x12 闹钟响起

    // 查找耳机
    void (*smartbox_find_my_cmd_send)(u8 data);           // 0x10查找L | 0x01查找R | 0x11查找LR | 0x03查找关闭

    // 音量
    void (*smartbox_volume_up_cmd_send)(void);           // 音量增加
    void (*smartbox_volume_down_cmd_send)(void);         // 音量减小

    // 音乐
    void (*custom_client_send_music_ctrl)(u8 state);

    //设置ANC
    void (*custom_client_send_anc_mode)(u8 mode);
    // EQ 模式控制
    void (*smartbox_eq_mode_cmd_send)(u8 data);

    //设置接听挂断
    void (*smartbox_call_status_cmd_send)(u8 data);    // 控制接听挂断  1接听 2挂断

    //设置耳机BLE进入 no lantacy发送
    void (*smartbox_ble_setting_cmd_send)(u8 data);      // 0xf1 设置 0xf0退出
    void (*smartbox_box_all_info_cmd_send)(void);

    //抖音控制
    void (*custom_client_send_ctrl_tiktop)(u8 data);         //抖音控制

    // 拍照
    void (*smartbox_snap_cmd_send)(u8 data);           // 拍照

    // 切换语言
    void (*smartbox_language_switch_cmd_send)(LANGUAGE_LIST data);    // 1中文，2英文

    // 语音助手控制
    void (*smartbox_siri_cmd_send)(u8 data);  // 0打开，1关闭

    // 播出电话
    void (*smartbox_phoneout_send_cmd)(u8 *buf);

    // 用户自定义命令
    void (*smartbox_user_add_cmd_send)(void);
};

extern const struct s_box_info_send_cb box_info_send_cb;



// 蓝牙 >>> UI
typedef enum {
    CALL_STATUS_HANGUP = 1,
    CALL_STATUS_INCOME,
    CALL_STATUS_ACTIVE,
    CALL_STATUS_OUTGOING,
} CALL_STATUS;


typedef enum {
    MUTE_STATUS_MUTE_OFF = 3,
    MUTE_STATUS_MUTE_ON,
} MUTE_STATUS;


typedef enum {
    PHONE_CTRL_ANSWER = 1,
    PHONE_CTRL_HANGUP,
    PHONE_CTRL_MUTE_OFF,
    PHONE_CTRL_MUTE_ON,
} PHONE_CTRL;

typedef enum {
    BLE_DISCONNET = 0,
    BLE_CONNECTED,
} BLE_STATUS;

typedef enum {
    BT_DISCONNET = 0,
    BT_CONNECTED,
} BT_STATUS;


typedef enum {
    ANC_STATUS_NONE = 0,
    ANC_STATUS_CLOSE,
    ANC_STATUS_OPEN,
    ANC_STATUS_TRANSPARENCY,
} ANC_STATUS;

typedef enum {
    MUSIC_STATE_NULL = 0,
    MUSIC_STATE_PLAY, //音乐播放
    MUSIC_STATE_PAUSE, //音乐暂停
    MUSIC_STATE_PRV,
    MUSIC_STATE_NEXT,
} MUSIC_STATUS;

typedef enum {
    SIRI_OPEN = 0,
    SIRI_CLOSE,
} SIRI_STATUS;



struct s_box_info_base_cb {
    u8(*smartbox_app_state_get)(void);
    u8(*smartbox_phone_state_get)(void);
    u8(*smartbox_earphone_state_get)(void);
    u8(*smartbox_l_ear_bat_get)(void);
    u8(*smartbox_r_ear_bat_get)(void);
    u8(*smartbox_box_bat_get)(void);
    u8(*smartbox_music_state_get)(void);
    u8(*smartbox_earphone_vol_get)(void);
    u8(*smartbox_anc_mode_get)(void);
    u8(*smartbox_eq_mode_get)(void);
    u8(*smartbox_ui_bl_levl_get)(void);
    u8(*smartbox_l_charging_get)(void);
    u8(*smartbox_r_charging_get)(void);
    u8(*smartbox_box_charging_get)(void);
    u8(*smartbox_l_finding_get)(void);
    u8(*smartbox_r_finding_get)(void);
    u8(*smartbox_box_clid_status_get)(void);
    u8(*smartbox_language_get)(void);
    u8(*smartbox_low_power_get)(void);
    u8(*smartbox_phone_call_state_get)(void);
    u8(*smartbox_phone_call_mute_get)(void);
    u8(*smartbox_box_hour_get)(void);
    u8(*smartbox_box_minute_get)(void);
    u8(*smartbox_box_second_get)(void);
    u8(*smartbox_box_time_get)(struct sys_time *time);
    u8(*smartbox_local_page_get)(void);
    u8(*smartbox_emitter_state_get)(void);
    u8(*smartbox_r_inbox_get)(void);
    u8(*smartbox_l_inbox_get)(void);

    void (*smartbox_phone_state_set)(u8 data);
    void (*smartbox_earphone_state_set)(u8 data);
    void (*smartbox_l_ear_bat_set)(u8 data);
    void (*smartbox_r_ear_bat_set)(u8 data);
    void (*smartbox_box_bat_set)(u8 data);
    void (*smartbox_music_state_set)(u8 data);
    void (*smartbox_earphone_vol_set)(u8 data);
    void (*smartbox_anc_mode_set)(u8 data);
    void (*smartbox_eq_mode_set)(u8 data);
    void (*smartbox_ui_bl_levl_set)(u8 data);
    void (*smartbox_l_charging_set)(u8 data);
    void (*smartbox_r_charging_set)(u8 data);
    void (*smartbox_box_charging_set)(u8 data);
    void (*smartbox_l_finding_set)(u8 data);
    void (*smartbox_r_finding_set)(u8 data);
    void (*smartbox_box_clid_status_set)(u8 data);
    void (*smartbox_language_set)(u8 data);
    void (*smartbox_low_power_set)(u8 data);
    void (*smartbox_phone_call_state_set)(u8 data);
    void (*smartbox_phone_call_mute_set)(u8 data);
    void (*smartbox_box_hour_set)(u8 data);
    void (*smartbox_box_minute_set)(u8 data);
    void (*smartbox_local_page_set)(u8 data);
    void (*smartbox_emitter_state_set)(u8 state);
    void (*smartbox_r_inbox_set)(u8 data);
    void (*smartbox_l_inbox_set)(u8 data);

    void (*smartbox_storage_mode_enter)(void);
};
extern const struct s_box_info_base_cb box_info_base_cb;

// enum {
//     UI_MSG_OTHER,
//     UI_MSG_KEY,
//     UI_MSG_TOUCH,
//     UI_MSG_SHOW,
//     UI_MSG_HIDE,
//     UI_MSG_UPDATE,
// };

enum {
    BR_INIT,
    BR_CONNECTED,
    BR_DISCONNECTED,
    BR_PAGE_TIMEROUT,
    BR_EXIT,
};

enum {
    UI_UPDATE_TIME,
    UI_UPDATE_BR_STATUS,
};

enum {
    LOCAL_PLAY_PC_MUSIC_PREV,
    LOCAL_PLAY_PC_MUSIC_NEXT,
    LOCAL_PLAY_PC_MUSIC_PP,
    LOCAL_PLAY_PC_CLOSE,
    LOCAL_PLAY_PC_START,
    LOCAL_PLAY_PC_STOP,
    LOCAL_PLAY_MUSIC_MODE,
    LOCAL_PLAY_MASK_MODE,

};

#ifdef __cplusplus
}
#endif
#endif /* EVENTS_ADAPTER_H_ */
