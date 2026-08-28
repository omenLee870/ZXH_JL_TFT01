#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_photo_app_msg_handler.data.bss")
#pragma data_seg(".video_photo_app_msg_handler.data")
#pragma const_seg(".video_photo_app_msg_handler.text.const")
#pragma code_seg(".video_photo_app_msg_handler.text")
#endif
#include "app_main.h"
#include "system/includes.h"
#include "video_photo.h"
#include "video_photo_ui.h"
#include "app_config.h"
#include "media_file_list.h"

#if TCFG_APP_PHOTO_EN


int video_photo_app_msg_handler(int *msg)
{
    int msg_type = msg[0];
    printf("[PHOTO_HANDLER] msg_type=%d", msg_type);

    switch (msg_type) {
    case APP_MSG_VIDEO_PHOTO_CAPTURE:
        /* P1: 定时倒计时中按拍照键 -> 取消定时 (不拍照)。
         * photo_state_get 返回 COUNTDOWN 时 photo_timer_cancel 清零 timer_left
         * 并回到 SINGLE 模式, 用户可重新设置定时或正常拍照。 */
        if (photo_state_get() == PHOTO_STATE_COUNTDOWN) {
            printf("[PHOTO_HANDLER] -> cancel timer (countdown active)");
            photo_timer_cancel();
        } else {
            extern void photo_start_timed(void);
            printf("[PHOTO_HANDLER] -> capture (timed if set)");
            photo_start_timed();   /* 菜单设了定时则倒计时, 否则直接拍 */
        }
        break;
    case APP_MSG_ZOOM_IN:
        printf("[PHOTO_HANDLER] -> zoom in");
        photo_zoom_in();
        photo_ui_update_zoom(photo_zoom_get());
        photo_zoom_repeat_start(1);
        break;
    case APP_MSG_ZOOM_OUT:
        printf("[PHOTO_HANDLER] -> zoom out");
        photo_zoom_out();
        photo_ui_update_zoom(photo_zoom_get());
        photo_zoom_repeat_start(-1);
        break;
    case APP_MSG_VIDEO_PHOTO_ZOOM_STOP:
        printf("[PHOTO_HANDLER] -> zoom stop");
        photo_zoom_repeat_stop();
        break;
    case APP_MSG_EFFECT_NEXT:
        printf("[PHOTO_HANDLER] -> effect next");
        photo_effect_next();
        break;
    case APP_MSG_EFFECT_PREV:
        printf("[PHOTO_HANDLER] -> effect prev");
        photo_effect_prev();
        break;
    case APP_MSG_VIDEO_PHOTO_BACK:
        /* 左键返回: 清理连拍/定时后必须切回 IDLE 模式, 关闭 camera;
         * 否则拍照循环后台继续跑, camera 占用, 后续进录像/拍照会冲突死机 */
        printf("[PHOTO_HANDLER] -> back to home (stop timer/burst + switch to idle)");
        photo_zoom_repeat_stop();
        {
            u8 m = photo_mode_get();
            if (m == PHOTO_MODE_BURST) {
                photo_burst_stop();
            }
            photo_timer_cancel();
        }
        app_task_switch_to(APP_MODE_IDLE, 0);
        break;
    case APP_MSG_CHANGE_MODE:
        printf("[PHOTO_HANDLER] -> goto next mode");
        photo_zoom_repeat_stop();
        {
            u8 m = photo_mode_get();
            if (m == PHOTO_MODE_BURST) {
                photo_burst_stop();
            }
            photo_timer_cancel();
        }
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    default:
        printf("[PHOTO_HANDLER] unhandled msg=%d", msg_type);
        break;
    }
    return 0;
}
#endif /* #if TCFG_APP_PHOTO_EN */
