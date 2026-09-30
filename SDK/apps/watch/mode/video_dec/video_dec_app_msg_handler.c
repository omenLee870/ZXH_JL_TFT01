#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_dec_app_msg_handler.data.bss")
#pragma data_seg(".video_dec_app_msg_handler.data")
#pragma const_seg(".video_dec_app_msg_handler.text.const")
#pragma code_seg(".video_dec_app_msg_handler.text")
#endif
#include "app_main.h"
#include "system/includes.h"
#include "video_dec.h"
#include "app_config.h"
#include "media_file_list.h"

#if TCFG_APP_VIDEO_DEC_EN

/* video_dec.c exports for navigation/play dispatch */
extern void video_dec_dispatch_next(void);
extern void video_dec_dispatch_prev(void);
extern void video_dec_dispatch_play(void);
extern void video_dec_dispatch_back(void);
extern void video_dec_dispatch_show_del_menu(void);
extern void video_dec_skip_corrupt(u32 gen);
extern void video_dec_err_recover(u32 gen);
extern int video_dec_play(int index);   /* 播放指定文件索引(相册列表 OK 键) */

int video_dec_app_msg_handler(int *msg)
{
    int msg_type = msg[0];
    printf("[DEC_HANDLER] msg_type=%d", msg_type);

    switch (msg_type) {
    case APP_MSG_VIDEO_DEC_PLAY:
        printf("[DEC_HANDLER] -> play current / select");
        // 删除执行/重扫窗口忽略play/select, 防与delete_all交错

        if (video_dec_get_state() == VIDEO_DEC_STATE_SCANNING ||
            video_dec_get_state() == VIDEO_DEC_STATE_DELETE_CONFIRM) {
            printf(" (ignored: busy)\n");
            break;
        }
        video_dec_dispatch_play();
        break;
    case APP_MSG_VIDEO_DEC_PLAY_INDEX:
        printf("[DEC_HANDLER] -> play index %d", msg[1]);
        if (video_dec_get_state() == VIDEO_DEC_STATE_SCANNING ||
            video_dec_get_state() == VIDEO_DEC_STATE_DELETE_CONFIRM) {
            printf(" (ignored: busy)\n");
            break;
        }
        video_dec_play(msg[1]);
        break;
    case APP_MSG_VIDEO_DEC_NEXT:
        printf("[DEC_HANDLER] -> next");
        if (video_dec_get_state() == VIDEO_DEC_STATE_SCANNING ||
            video_dec_get_state() == VIDEO_DEC_STATE_DELETE_CONFIRM) {
            printf(" (ignored: busy)\n");
            break;
        }
        video_dec_dispatch_next();
        break;
    case APP_MSG_VIDEO_DEC_PREV:
        printf("[DEC_HANDLER] -> prev");
        if (video_dec_get_state() == VIDEO_DEC_STATE_SCANNING ||
            video_dec_get_state() == VIDEO_DEC_STATE_DELETE_CONFIRM) {
            printf(" (ignored: busy)\n");
            break;
        }
        video_dec_dispatch_prev();
        break;
    case APP_MSG_VIDEO_DEC_BACK:
        printf("[DEC_HANDLER] -> back");
        video_dec_dispatch_back();
        break;
    case APP_MSG_VIDEO_DEC_VOLUME_UP:
        printf("[DEC_HANDLER] -> volume up");
        video_dec_volume_up();
        break;
    case APP_MSG_VIDEO_DEC_VOLUME_DOWN:
        printf("[DEC_HANDLER] -> volume down");
        video_dec_volume_down();
        break;
    case APP_MSG_VIDEO_DEC_DELETE_SEL:
        printf("[DEC_HANDLER] -> delete selected");
        video_dec_delete_selected();
        break;
    case APP_MSG_VIDEO_DEC_DELETE_ALL:
        printf("[DEC_HANDLER] -> delete all");
        video_dec_delete_all();
        break;
    case APP_MSG_VIDEO_DEC_SHOW_DEL_MENU:
        printf("[DEC_HANDLER] -> show/toggle delete menu");
        video_dec_dispatch_show_del_menu();
        break;
    case APP_MSG_VIDEO_DEC_SKIP_CORRUPT:
        printf("[DEC_HANDLER] -> skip corrupt (from event task)");
        video_dec_skip_corrupt((u32)msg[1]);
        break;
    case APP_MSG_VIDEO_DEC_ERR_RECOVER:
        printf("[DEC_HANDLER] -> err recover (mid-stream corrupt)");
        video_dec_err_recover((u32)msg[1]);
        break;
    case APP_MSG_CHANGE_MODE:
        printf("[DEC_HANDLER] -> goto next mode");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_SD_EVENT:
        /* 插卡刷新改由 APP_MSG_FLM_DB_CHANGED 驱动(重建完成后), 避免 app_core 忙等重建线程 */
        if (msg[1] == DEVICE_EVENT_OUT) {
            video_dec_clear_on_card_out();
        }
        break;
    case APP_MSG_FLM_DB_CHANGED:
        video_dec_refresh_after_insert();
        break;
    default:
        printf("[DEC_HANDLER] unhandled msg=%d", msg_type);
        break;
    }
    return 0;
}

#endif /* #if TCFG_APP_VIDEO_DEC_EN */

