#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rtc_app_msg_handler.data.bss")
#pragma data_seg(".rtc_app_msg_handler.data")
#pragma const_seg(".rtc_app_msg_handler.text.const")
#pragma code_seg(".rtc_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "rtc.h"

#if TCFG_APP_RTC_EN

int rtc_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_RTC)) {
        return 0;
    }

    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("APP_MSG_CHANGE_MODE\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_RTC_UP:
        printf("APP_MSG_RTC_UP \n");
        set_rtc_up();
        break;
    case APP_MSG_RTC_DOWN:
        printf("APP_MSG_RTC_DOWN \n");
        set_rtc_down();
        break;
    case APP_MSG_RTC_SW:
        printf("APP_MSG_RTC_SW \n");
        set_rtc_sw();
        break;
    case APP_MSG_RTC_SW_POS:
        printf("APP_MSG_RTC_SW_POS \n");
        set_rtc_pos();
        break;
    default:
        break;
    }

    return 0;
}

#endif
