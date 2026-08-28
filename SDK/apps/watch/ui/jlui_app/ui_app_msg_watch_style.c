#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_app_msg_watch_style.data.bss")
#pragma data_seg(".ui_app_msg_watch_style.data")
#pragma const_seg(".ui_app_msg_watch_style.text.const")
#pragma code_seg(".ui_app_msg_watch_style.text")
#endif
#include "app_config.h"
#include "ui_api.h"
#include "ui.h"
#include "app_main.h"
#include "jlui_app/ui_style.h"


#if ((defined TCFG_UI_ENABLE) && (TCFG_UI_ENABLE))
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE))

static int ui_lcd_bt_stack_msg_entry(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    printf("lcd bt stack:0x%x\n", bt->event);
    UI_MSG_POST("bt_status:event=%4", bt->event);

    return 0;
}

//ui 消息处理
static void lcd_enter_mode(u8 mode)
{
    switch (mode) {
    case APP_MODE_IDLE:
        /* UI_SHOW_WINDOW(ID_WINDOW_IDLE); */
        break;
    case APP_MODE_POWERON:
        UI_SHOW_WINDOW(ID_WINDOW_POWER_ON);
        break;
    case APP_MODE_BT:
        /* UI_SHOW_WINDOW(ID_WINDOW_BT); */
        break;
    case APP_MODE_MUSIC:
        /* UI_SHOW_WINDOW(ID_WINDOW_MUSIC); */
        break;
    case APP_MODE_RECORD:
        /* UI_SHOW_WINDOW(ID_WINDOW_REC); */
        break;
    case APP_MODE_RTC:
        /* UI_SHOW_WINDOW(ID_WINDOW_CLOCK); */
        break;
    case APP_MODE_PC:
        /* UI_SHOW_WINDOW(ID_WINDOW_PC); */
        break;
    }
}


//*----------------------------------------------------------------------------*/
/**@brief   ui处理
   @param   msg:主线程转发过来的msg
   @return  0
   @note	集中处理ui信息
*/
/*----------------------------------------------------------------------------*/
static int ui_msg_entry(int *msg)
{
    printf("lcd msg:%d\n", msg[0]);

    if (key_is_ui_takeover()) {
        switch (msg[0]) {
        case APP_MSG_JL_UI_HOME:
        case APP_MSG_JL_UI_SHORTCUT:
        case APP_MSG_JL_UI_POWEROFF:
            ui_key_msg_post(msg[0]);
            return 0;
        default:
            break;
        }
    }

    switch (msg[0]) {
#if 0
    case APP_MSG_ENTER_MODE:
        printf("<<<<<<lcd enter mode:%d\n", msg[1]);
        lcd_enter_mode(msg[1] & 0xff);
        break;
    case APP_MSG_EXIT_MODE:
        printf(">>>>>>lcd exit mode:%d\n", msg[1]);
        UI_HIDE_CURR_WINDOW();
        break;
    case APP_MSG_VOL_CHANGED:
        UI_MSG_POST("music_vol:vol=%4", msg[1]);
        break;
    case APP_MSG_MUSIC_FILE_NUM_CHANGED:
        /* printf("msg[1]:%d,msg[2]:%d\n",msg[1],msg[2]); */
        int analaz =  music_player_lrc_analy_start(music_app_get_cur_hdl());
        char *logo = music_app_get_dev_cur();
        UI_MSG_POST("music_start:show_lyric=%4:dev=%4:filenum=%4:total_filenum=%4", !analaz, logo, msg[1], msg[2]);
        break;
    case APP_MSG_REPEAT_MODE_CHANGED:
        break;
    case APP_MSG_FM_REFLASH:
        UI_REFLASH_WINDOW(true);
        UI_MSG_POST("fm_fre", NULL);
        break;
    case APP_MSG_FM_STATION:
        UI_MSG_POST("fm_fre", NULL);
        break;
    case APP_MSG_INPUT_FILE_NUM:
        break;
    case APP_MSG_RTC_SET:
        break;
#endif
    case APP_MSG_JL_UI_HOME:
        // TODO
        break;
    case APP_MSG_JL_UI_SHORTCUT:
        // TODO
        break;
    case APP_MSG_JL_UI_POWEROFF:
        // TODO
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(lcd_bt_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = ui_lcd_bt_stack_msg_entry,
};

APP_MSG_HANDLER(lcd_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = ui_msg_entry,
};

#endif/* #if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE)) */
#endif /* if ((defined TCFG_UI_ENABLE) && (TCFG_UI_ENABLE)) */

