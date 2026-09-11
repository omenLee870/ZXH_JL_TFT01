#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_rec_app_msg_handler.data.bss")
#pragma data_seg(".video_rec_app_msg_handler.data")
#pragma const_seg(".video_rec_app_msg_handler.text.const")
#pragma code_seg(".video_rec_app_msg_handler.text")
#endif
#include "app_main.h"
#include "system/includes.h"
#include "video_rec.h"
#include "video_rec_ui.h"
#include "app_config.h"

#if TCFG_APP_VIDEO_REC_EN

extern void video_rec_ui_sticker_next(void);
extern void video_rec_ui_update_zoom(float level);
extern float video_rec_zoom_in(void);
extern float video_rec_zoom_out(void);
extern void video_rec_slowmo_toggle(void);
extern u8 video_rec_slowmo_get(void);
extern void video_rec_ui_update_slowmo(u8 on);
extern void video_rec_storage_recalc(void);
extern void video_rec_ui_timer_sync(int new_state);
extern int video_rec_flm_rebuild_done(void);

int video_rec_app_msg_handler(int *msg)
{
    int msg_type = msg[0];
    printf("[REC_HANDLER] msg_type=%d", msg_type);

    switch (msg_type) {
    case APP_MSG_VIDEO_REC_PP: {
        /* 中键: 开始/停止 */
        printf("[REC_HANDLER] toggle start/stop");
        enum video_rec_state before = video_rec_get_state();
        if (before == VIDEO_STATE_RECORDING || before == VIDEO_STATE_PAUSED) {
            video_rec_stop();
        } else {
            video_rec_start();
        }
        video_rec_ui_on_state_change(before, video_rec_get_state());
        break;
    }
    case APP_MSG_VIDEO_REC_PAUSE_RESUME: {
        /* 右键: 暂停/继续; IDLE 下不动作, 开始录像只能按中键 */
        printf("[REC_HANDLER] toggle pause/resume");
        enum video_rec_state before = video_rec_get_state();
        if (before == VIDEO_STATE_RECORDING) {
            video_rec_pause();
        } else if (before == VIDEO_STATE_PAUSED) {
            video_rec_resume();
        } else {
            printf("[REC_HANDLER] pause/resume ignored in IDLE\n");
        }
        video_rec_ui_on_state_change(before, video_rec_get_state());
        break;
    }
    case APP_MSG_VIDEO_REC_START: {
        /* 长按开始录像; 若已在录像/暂停, start 内部防重入返回 -1, 安全。 */
        printf("[REC_HANDLER] long-press start");
        enum video_rec_state before = video_rec_get_state();
        video_rec_start();
        video_rec_ui_on_state_change(before, video_rec_get_state());
        break;
    }
    case APP_MSG_VIDEO_REC_STOP: {
        /* 松开停止录像; IDLE 时 stop 内部 no-op 返回 0, 安全。 */
        printf("[REC_HANDLER] release stop");
        enum video_rec_state before = video_rec_get_state();
        video_rec_stop();
        video_rec_ui_on_state_change(before, video_rec_get_state());
        break;
    }
    case APP_MSG_EFFECT_NEXT:
        printf("[REC_HANDLER] sticker next");
        video_rec_ui_sticker_next();
        break;
    case APP_MSG_ZOOM_IN:
        printf("[REC_HANDLER] zoom in");
        video_rec_ui_update_zoom(video_rec_zoom_in());
        break;
    case APP_MSG_ZOOM_OUT:
        printf("[REC_HANDLER] zoom out");
        video_rec_ui_update_zoom(video_rec_zoom_out());
        break;
    case APP_MSG_EFFECT_PREV:
        printf("[REC_HANDLER] slowmo toggle");
        video_rec_slowmo_toggle();
        video_rec_ui_update_slowmo(video_rec_slowmo_get());
        break;
    case APP_MSG_VIDEO_REC_BACK:
        /* 左键返回: 停录后必须切回 IDLE 模式, 让录像模式循环退出并关闭 camera;
         * 否则后台循环继续跑, 插卡事件还会触发自动录像, 且 camera 占用导致后续模式异常 */
        printf("[REC_HANDLER] -> back to home (stop rec + switch to idle)");
        {
            enum video_rec_state st = video_rec_get_state();
            if (st == VIDEO_STATE_RECORDING || st == VIDEO_STATE_PAUSED) {
                video_rec_stop();
            }
        }
        app_task_switch_to(APP_MODE_IDLE, 0);
        break;
    case APP_MSG_CHANGE_MODE:
        printf("[REC_HANDLER] -> goto next mode");
        {
            enum video_rec_state st = video_rec_get_state();
            if (st == VIDEO_STATE_RECORDING || st == VIDEO_STATE_PAUSED) {
                video_rec_stop();
            }
        }
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_VIDEO_REC_RECALC_STORAGE:
        /* fget_free_space 遍历 FAT 慢, 在 app_core 线程算,
         * 不阻塞 LVGL ui 绘制(否则进录像屏长时间背景/白屏) */
        video_rec_storage_recalc();
        break;
    case APP_MSG_VIDEO_REC_UI_TIMER_SYNC:
        /* UI 500ms 定时器统一在 app_core 线程启停 */
        video_rec_ui_timer_sync(msg[1]);
        break;
    case APP_MSG_FLM_DB_CHANGED:
        /* 重建完成: 补做被挂起的 SD IN 自动开录(无 pending 时 no-op) */
        printf("[REC_HANDLER] flm rebuild done");
        video_rec_flm_rebuild_done();
        break;
    default:
        printf("[REC_HANDLER] unhandled msg=%d", msg_type);
        break;
    }
    return 0;
}
#endif /* #if TCFG_APP_VIDEO_REC_EN */
