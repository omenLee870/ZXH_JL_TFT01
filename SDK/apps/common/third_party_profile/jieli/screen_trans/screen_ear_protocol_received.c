#include "ble_user.h"
#include "events_adapter.h"
#include "data_call_log_storage.h"
#include "data_phonebook_storage.h"
#include "ui_api.h"
#include "rtc/rtc_dev.h"
#include "smartbox_user_app.h"
#include "smartbox_info_manager.h"
#include "third_party/screen_trans_lib/screen_ear_interface.h"
/*
*   充电仓和耳机通讯事件回调文件
*   用户层修改和调用
*
*/

#define LOG_TAG_CONST       EARPHONE_PROT
#define LOG_TAG     		"[EARPHONE_PROT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "utils/debug.h"

#if TCFG_EARPHONE_PROTOCOL

void smartbox_app_receive_handle(u8 cmd, u8 *data, u8 len)
{
    log_info("%s cmd:%x len:%d\n", __func__, cmd, len);
    put_buf(data, len);
    switch (cmd) {
    case CUSTOM_CALL_STATE_CMD :
        log_info("CUSTOM_CALL_STATE_CMD");
        if ((data[0] < CALL_STATUS_HANGUP) || (data[0] > CALL_STATUS_OUTGOING)) {
            log_error("%s data[0] error", __func__);
            return;
        }

        sbox_phone_call_state_set(data[0]);

        if (data[0] == CALL_STATUS_INCOME) {
            //暂时获取耳机仓时间，后续可能需要耳机下发时间
            /* small_file_call_log_set_date(); */
            /* small_file_call_log_set_type(CALL_INCOME); */
            /* small_file_call_log_set_sel(CALL_SEL_ERA_BLE); */
            /* small_file_call_log_save(); */

            UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_PHONE_CALL_STATUS, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PHONE);
        } else if (data[0] == CALL_STATUS_OUTGOING) {
            //暂时获取耳机仓时间，后续可能需要耳机下发时间
            /* small_file_call_log_set_date(); */
            /* small_file_call_log_set_type(CALL_OUT); */
            /* small_file_call_log_set_sel(CALL_SEL_ERA_BLE); */
            /* small_file_call_log_save(); */

            UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_PHONE_CALL_STATUS, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PHONE);
        } else if (data[0] == CALL_STATUS_ACTIVE) {
            UI_MSG_POST("phone:state =%4", data[0]);
        } else if (data[0] == CALL_STATUS_HANGUP) {
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_PHONE_CALL_STATUS);
        }

        break;
    case CUSTOM_PHONE_CALL_INFO_CMD:
        log_info("CUSTOM_PHONE_CALL_INFO_CMD data[0]:%d", data[0]);
        if ((data[0] < CALL_STATUS_HANGUP) || (data[0] > CALL_STATUS_OUTGOING)) {
            log_error("data[0] error");
            return;
        }
        UI_WINDOW_BACK_SHOW(2);
        sbox_phone_call_state_set(data[0]);
        UI_MSG_POST("phone:state =%4", data[0]);
        break;
    case CUSTOM_ALL_INFO_CMD: { // 0xff
        log_info("CUSTOM_ALL_INFO_CMD data[0]:%d", data[0]);
        sbox_bt_connect_flag_set(data[0]);
        sbox_anc_mode_set(data[1] + 1);
        sbox_music_ui_state_set(data[2]);
        sbox_volume_set(data[3]);
        sbox_battery_left_set(data[4]);
        sbox_battery_right_set(data[5]);
        sbox_equalizer_mode_set(data[6]);
        sbox_eq_gain_set(data + 7);
        sbox_key_info_set(data + 17);
        break;
    }
    case CUSTOM_EQ_DATE_CMD: { // 0x5
        log_info("CUSTOM_EQ_DATE_CMD data[0]:%d", data[0]);
        sbox_equalizer_mode_set(data[0]);
        // UI_MSG_POST("bt_status:status=%4", data[0]); // 往ui task推消息，更新连接状态
        break;
    }
    case CUSTOM_ANC_DATE_CMD: { // 0x6
        log_info("CUSTOM_ANC_DATE_CMD data[0]:%d", data[0]);
        sbox_anc_mode_set(data[0] + 1);
        // UI_MSG_POST("bt_status:status=%4", data[0]); // 往ui task推消息，更新连接状态
        break;
    }
    case CUSTOM_BT_CONNECT_STATE_CMD: { // 0x1
        log_info("CUSTOM_BT_CONNECT_STATE_CMD data[0]:%d", data[0]);
        sbox_bt_connect_flag_set(data[0]);
        // UI_MSG_POST("bt_status:status=%4", data[0]); // 往ui task推消息，更新连接状态
        break;
    }
    case CUSTOM_BLE_CONNECT_STATE_CMD: { // 0x2
        log_info("CUSTOM_BLE_CONNECT_STATE_CMD data[0]:%d", data[0]);
        // UI_MSG_POST("ble_status:status=%4", data[0]);
        break;
    }
    case CUSTOM_BLE_BATTERY_STATE_CMD: { // 0x3
        log_info("CUSTOM_BLE_BATTERY_STATE_CMD data[L]:%d.data[R]:%d", data[0], data[1]);
        // todo  更新电量值
        sbox_battery_left_set(data[0]);
        sbox_battery_right_set(data[1]);
        // UI_MSG_POST("vbat_status");
        break;
    }
    case CUSTOM_BLE_VOLUMEN_CMD: { // 0x4
        log_info("CUSTOM_BLE_VOLUMEN_CMD data[0]:%d", data[0]);
        sbox_volume_set(data[0]);
        break;
    }
    case CUSTOM_BLE_TIME_DATE_CMD:
        log_info("CUSTOM_BLE_TIME_DATE_CMD:");
        sbox_phone_time_set((data[1] << 8) + data[0], data[2], data[3], data[4], data[5], data[6]);
        log_info("box_info.phone_time.year: %d", (data[1] << 8) + data[0]);

        struct sys_time time = {0};
        sbox_box_time_get(&time);
        extern void rtc_update_time_api(struct sys_time * time);
        rtc_update_time_api(&time);
        break;
    // }

    case CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD: { // 0x33
        log_info("CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD data[0]:%d", data[0]);
        sbox_music_ui_state_set(data[0]);
    }
    case CUSTOM_BLE_ANC_MODE_CONTROL_CMD: { // 0x34
#if TCFG_AUDIO_ANC_ENABLE
        if (data[0] == 0x01) { // ANC on
            anc_mode_switch(ANC_ON, ANC_USER_TRAIN_TONE_MODE); // 默认需要播提示音
        } else if (data[0] == 0x02) { // 通透
            anc_mode_switch(ANC_TRANSPARENCY, ANC_USER_TRAIN_TONE_MODE);
        } else {
            anc_mode_switch(ANC_OFF, ANC_USER_TRAIN_TONE_MODE);
            log_info("CUSTOM_BLE_ANC_MODE_CONTROL_CMD volue:%x valid!!!!\n", data[0]);
        }
#endif
        break;
    }
    case CUSTOM_BLE_EQ_MODE_CONTROL_CMD: { // 0x35
        log_info("CUSTOM_BLE_EQ_MODE_CONTROL_CMD");
        // todo
        if (data[0] == 0x01) { // eq效果1

        } else if (data[0] == 0x02) { // eq效果2

        } else if (data[0] == 0x03) { // eq效果3

        } else {
            log_info("CUSTOM_BLE_VOL_CONTROL_CMD volue:%x valid!!!!\n", data[0]);
        }
        break;
    }
    case CUSTOM_EDR_CLEAR_COMP: { // 0x35
        log_info("CUSTOM_EDR_CLEAR_COMP");
        // extern void set_earphone_wait_conn(u8 en);
        // set_earphone_wait_conn(1);
        break;
    }
    default:
        log_info("ble_notify_recv_data_handler cmd err:%d", cmd);
        break;
    }
}
#endif

