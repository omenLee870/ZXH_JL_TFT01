#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_msg.h"
#include "lv_ui.h"
#include "app_config.h"
#include "fs/fs.h"
#include "avctp_user.h"
#include "custom.h"
#include "pet_dashboard_music.h"
/*#include "video/logo_show.h"*/


static u32 toltal_time = 0;

//UI 按钮事件
void res_event_music_next(lv_event_t *e)
{
#if !LV_USE_GUIBUILDER_SIMULATOR
    /*app_send_message(APP_MSG_BT_MUSIC_NEXT, 0);*/

    app_send_message(APP_MSG_MUSIC_NEXT, 0);
#endif
}
void res_event_music_prev(lv_event_t *e)
{
#if !LV_USE_GUIBUILDER_SIMULATOR
    /*app_send_message(APP_MSG_BT_MUSIC_PREV, 0);*/
    app_send_message(APP_MSG_MUSIC_PREV, 0);
#endif
}
void res_event_music_pause_play(lv_event_t *e)
{
#if !LV_USE_GUIBUILDER_SIMULATOR

    app_send_message(APP_MSG_BT_EMITTER_PAUSE, 0);
    if (bt_get_total_connect_dev() == 0) {
        ui_music_pause_handler(); //没有设备连接，重置UI按钮
    }
#endif
}

void res_event_music_process_change(lv_event_t *e)
{
#if !LV_USE_GUIBUILDER_SIMULATOR

#endif
}

// UI处理SDK消息回调
void ui_music_pause_handler(void)
{
    lvgl_module_msg_send_value(GUI_MUSIC_MSG_ID_MUSIC_STATE, LV_STATE_CHECKED, 0);
}

void ui_music_on_handler(void)
{
    lvgl_module_msg_send_value(GUI_MUSIC_MSG_ID_MUSIC_STATE, LV_STATE_DEFAULT, 0);
}

void ui_music_tol_time_handler(u32 tol_time)
{
    uint8_t min = tol_time / 1000 / 60;
    uint8_t sec = tol_time / 1000 - (min * 60);
    toltal_time = tol_time;
    char *tol_time_ptr =  lvgl_module_msg_get_ptr(GUI_MUSIC_MSG_ID_TOL_TIME, 8);
    sprintf(tol_time_ptr, "%02u:%02u", min, sec);
    lvgl_module_msg_send_ptr(tol_time_ptr, 0); //发送成功后内部会释放指针，无需再释放
}

void ui_music_cur_time_handler(u32 cur_time)
{
    uint8_t min = cur_time / 1000 / 60;
    uint8_t sec = cur_time / 1000 - (min * 60);
    char *cur_time_ptr =  lvgl_module_msg_get_ptr(GUI_MUSIC_MSG_ID_CUR_TIME, 8);
    sprintf(cur_time_ptr, "%02u:%02u", min, sec);
    lvgl_module_msg_send_ptr(cur_time_ptr, 0); //发送成功后内部会释放指针，无需再释放
    ui_music_process_handler(cur_time);
}

void ui_music_process_handler(u32 cur_time)
{
    if (toltal_time == 0) {
        return;
    }
    uint32_t progress_percent = 0;
    if (cur_time <= toltal_time) {
        progress_percent = (cur_time * 100) / toltal_time;
    } else {
        progress_percent = 100;
    }
    lvgl_module_msg_send_value(GUI_MUSIC_MSG_ID_MUSIC_PROCESS, progress_percent, 0);
}

void ui_music_artist_handler(char *song_info)
{
}

void ui_music_lyrics_handler(char *song_lyrics)
{
    if (!song_lyrics) {
        return;
    }
    gui_scr_t *current_screen = gui_scr_get_act();
    if (!current_screen) {
        printf("No active screen found\n");
        return;
    }
    int32_t current_screen_id = gui_scr_get_id(current_screen);
    char *lyrics = lvgl_module_msg_get_ptr(GUI_MUSIC_MSG_ID_MUSIC_LYRICS, strlen(song_lyrics) + 1);
    strncpy(lyrics, song_lyrics, strlen(song_lyrics));
    lyrics[strlen(song_lyrics)] = '\0';
    lvgl_module_msg_send_ptr(lyrics, 0);
}

void ui_music_volume_handler(int volume)
{
    //lvgl_module_msg_send_value(GUI_SONGS_MSG_ID_PLAYER_VOLUME, volume, 0);
}

//注册页面加载卸载回调
static int gui_src_action_page_music(int action)
{
    printf("--->%s()----->%d, action: %d\n", __func__, __LINE__, action);

    switch (action) {
    case GUI_SCREEN_ACTION_LOAD:
        // UI主动获取当前音乐播放状态
        if (bt_get_total_connect_dev() > 0) {
            static lv_timer_t *music_album_pic_rotate = NULL;
            if (music_album_pic_rotate == NULL) {
                music_album_pic_rotate = lv_timer_create(page_music_rotate_timer_cb, 200, NULL);
            }
            lv_timer_set_repeat_count(music_album_pic_rotate, -1);
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_PLAY_TIME, 0, NULL); //获取音乐播放等信息
            pet_dashboard_music_album_request(); // 与仪表共用取图去重
        }
        break;
    case GUI_SCREEN_ACTION_UNLOAD:
        lv_ui_page_music *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
        if (!ui_scr) {
            return -1;
        }
        lv_image_set_src(ui_scr->page_music_img_3, NULL);

        break;
    }
    return 0;
}
REGISTER_UI_SCREEN_ACTION_HANDLER(GUI_SCREEN_PAGE_MUSIC)
.onchange = gui_src_action_page_music,
};

//注册页面加载卸载回调
static int gui_src_action_page_app_select(int action)
{
    printf("--->%s()----->%d, action: %d\n", __func__, __LINE__, action);

    switch (action) {
    case GUI_SCREEN_ACTION_LOAD:
        // UI主动获取当前音乐播放状态
        if (bt_get_total_connect_dev() > 0) {
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_PLAY_TIME, 0, NULL); //获取音乐播放等信息
            pet_dashboard_music_album_request(); // 与仪表共用取图去重
        }
        break;
    case GUI_SCREEN_ACTION_UNLOAD:
        lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
        if (!ui_scr) {
            return -1;
        }
        lv_image_set_src(ui_scr->page_app_select_img_3, NULL);

        break;
    }
    return 0;
}
REGISTER_UI_SCREEN_ACTION_HANDLER(GUI_SCREEN_PAGE_APP_SELECT)
.onchange = gui_src_action_page_app_select,
};


#endif
