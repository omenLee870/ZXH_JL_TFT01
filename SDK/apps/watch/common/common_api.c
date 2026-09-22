#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".common_api.data.bss")
#pragma data_seg(".common_api.data")
#pragma const_seg(".common_api.text.const")
#pragma code_seg(".common_api.text")
#endif

#include "btstack/avctp_user.h"
#include  "poweroff.h"
#include  "common_api.h"
#include  "video_api.h"
#include  "app_config.h"
#include "health_manager/health_manager.h"
#include "cat1/cat1_common.h"
#include "jlui_app/watch_syscfg_manage.h"
#include "btstack/le/ble_api.h"
#include "data_storage.h"
#include "battery_manager.h"
#include "tone_player.h"
#include "app_tone.h"
#include "app_msg.h"
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
#include "video_api.h"
#endif
#if TCFG_PAY_ALIOS_ENABLE
#include "alipay.h"
#endif
#if TCFG_MIX_RECORD_ENABLE
#include "mix_record_api.h"
#endif
#include "rcsp_mix_record.h"
#include "record.h"

void common_api_reboot_or_shutdown(u8 flag, u8 erase)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    camera_api_stop_rec_photo();
#endif

#if TCFG_APP_RECORD_EN
    //复位/关机/恢复出厂前停止本地录音，保证录音文件正常封头保存
    if (is_recorder_runing()) {
        app_recorder_stop_public();
    }
#endif
#if TCFG_MIX_RECORD_ENABLE
    //复位/关机/恢复出厂前停止混响录音，保证录音文件正常封头保存
    if (get_mix_recorder_status()) {
        mix_recorder_stop();
    }
    if (rcsp_mix_recorder_get_status()) {
        rcsp_mix_recorder_stop();
    }
#endif

#if (defined CONFIG_JL_UI_ENABLE && CONFIG_JL_UI_ENABLE)
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
#endif

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

#if (defined CONFIG_JL_UI_ENABLE && CONFIG_JL_UI_ENABLE)
    watch_syscfg_write_all(p);
#endif

    if (flag) {
        cpu_reset();
    } else {
        sys_enter_soft_poweroff(0);
    }
}


static void common_api_restore_sys_settings()
{
#if TCFG_PAY_ALIOS_ENABLE
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
    extern u32 rtc_write_time(const struct sys_time * curr_time);
    rtc_write_time(&def_sys_time);
#endif
#if TCFG_USER_BLE_ENABLE
    ble_list_clear_all();
#endif
#if TCFG_USER_BT_CLASSIC_ENABLE
    /* 清除经典蓝牙全部配对记录，避免恢复出厂后继续回连旧手机 */
    bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
    /* 清除上电回连地址列表计数，确保下次开机不再发起回连 */
    bt_clear_current_poweron_memory_search_index(1);
    /* 让蓝牙栈先处理删除命令并落盘，再复位 */
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    os_time_dly(10);
#endif
    small_file_del_all();//清外挂flash 存储

#if (TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE)
    erase_UIInfo_in_vm();
#endif
    common_api_reboot_or_shutdown(1, 1);
}


void common_api_disconnect_all_restore()
{
    play_tone_file(get_tone_files()->factory_reset);
    if (bt_get_curr_channel_state() != 0) { // 断开经典蓝牙
        /* bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL); */
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
    }
    common_api_restore_sys_settings();
    play_tone_file(get_tone_files()->reset_complete);
}


void common_api_disconnect_start_pair()
{
    /* printf("%s NOW STATUS %d\n",__FUNCTION__,bt_get_connect_status()); */
    if (
        (BT_STATUS_TAKEING_PHONE == bt_get_connect_status()) ||
        (BT_STATUS_PLAYING_MUSIC == bt_get_connect_status()) ||
        (bt_get_call_status() == BT_CALL_INCOMING) ||
        (bt_get_call_status() == BT_CALL_OUTGOING)
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
        || (get_camera_status() != CAMERA_STATE_IDLE)
#endif
    ) {
        /* printf("%s IS NOT FREE!\n",__FUNCTION__); */
        return;
    }
    if (bt_get_curr_channel_state() != 0) { // 断开经典蓝牙
        /* bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL); */
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
    }
}


static int poweron_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        u8 raw_level = get_cur_battery_level(); // 获取原始电量值（0-9）
        const u8 battery_thresholds[] = {1, 5, 10, 15, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        u8 actual_percentage;
        if (raw_level >= 9) {
            actual_percentage = 90;  // 9对应90%
        } else {
            actual_percentage = raw_level * 10;  // 其他值乘以10
        }
        int index = 0;
        for (int i = 0; i < sizeof(battery_thresholds) / sizeof(battery_thresholds[0]); i++) {
            if (actual_percentage >= battery_thresholds[i]) {
                index = i;
            } else {
                break;
            }
        }
        play_tone_file(get_tone_files()->battery_lvl[index]);
    }
    return 0;
}



void common_api_get_bat_percent()
{
    play_tone_file_callback(get_tone_files()->battery_status, NULL, poweron_tone_play_end_callback);
}


/*
触摸长按2s：
       空闲/录像/通话:开始/停止录音
       来电:拒接
       蓝牙播歌:返回
*/
void common_api_call_income_hangup()
{
    u8 call_status = bt_get_call_status();

    if (call_status == BT_CALL_INCOMING) {
        printf("[debug_info %s:%d] \n", __func__, __LINE__);
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    } else if (call_status == BT_CALL_ACTIVE) {
        printf("[debug_info %s:%d] \n", __func__, __LINE__);
        app_send_message(APP_MSG_REC_PP, 0);
        return;
    } else if (bt_get_connect_status() == BT_STATUS_PLAYING_MUSIC) {
        return;
    }
    app_send_message(APP_MSG_REC_PP, 0);
}
