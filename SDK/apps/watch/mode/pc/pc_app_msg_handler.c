#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_app_msg_handler.data.bss")
#pragma data_seg(".pc_app_msg_handler.data")
#pragma const_seg(".pc_app_msg_handler.text.const")
#pragma code_seg(".pc_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "audio_config.h"
#include "usb/device/hid.h"
#include "common_api.h"
#include "video_api.h"

#if TCFG_APP_PC_EN
int pc_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_PC)) {
        return 0;
    }

    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
#if TCFG_USB_SLAVE_HID_ENABLE
    case APP_MSG_MUSIC_PP:
        printf("APP_MSG_MUSIC_PP\n");
        hid_key_handler(0, USB_AUDIO_PP);
        break;
    case APP_MSG_MUSIC_PREV:
        printf("APP_MSG_MUSIC_PREV\n");
        hid_key_handler(0, USB_AUDIO_PREFILE);
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("APP_MSG_MUSIC_NEXT\n");
        hid_key_handler(0, USB_AUDIO_NEXTFILE);
        break;
    case APP_MSG_VOL_UP:
        printf("APP_MSG_VOL_UP\n");
        hid_key_handler(0, USB_AUDIO_VOLUP);
        printf(">>>pc vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        break;
    case APP_MSG_VOL_DOWN:
        printf("APP_MSG_VOL_DOWN\n");
        hid_key_handler(0, USB_AUDIO_VOLDOWN);
        printf(">>>pc vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        break;
#endif
    case APP_MSG_CURRENT_BAT:
        printf("[PC_KEY]:APP_MSG_CURRENT_BAT\n");
        common_api_get_bat_percent();
        break;
    case APP_MSG_POWER_OFF:
        printf("[PC_KEY]:APP_MSG_POWER_OFF\n");
        common_api_reboot_or_shutdown(0, 0);
        break;
    case APP_MSG_DISCON_RESET_BT:
        printf("[PC_KEY]:APP_MSG_DISCON_RESET_BT\n");
        common_api_disconnect_all_restore();
        break;
    case APP_MSG_START_PAIR:
        printf("[PC_KEY]:APP_MSG_START_PAIR\n");
        common_api_disconnect_start_pair();
        break;
    case APP_MSG_TAKE_PHOTO:
        printf("[PC_KEY]:APP_MSG_TAKE_PHOTO\n");
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
        camera_api_take_photo_or_stop_rec();
#endif
        break;
    case APP_MSG_RECORD_START:
        printf("[PC_KEY]:APP_MSG_RECORD_START\n");
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
        camera_api_record_video();
#endif
        break;
    default:
        break;
    }

    return 0;
}

#endif
