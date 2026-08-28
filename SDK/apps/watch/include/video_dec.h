/**
 * @file video_dec.h
 * @brief 视频解码模块公共头文件
 */
#ifndef CONFIG_VIDEO_DEC_H
#define CONFIG_VIDEO_DEC_H
#include "system/includes.h"
enum video_dec_state {
    VIDEO_DEC_STATE_IDLE = 0, VIDEO_DEC_STATE_SCANNING = 1,
    VIDEO_DEC_STATE_LIST = 2, VIDEO_DEC_STATE_PHOTO_VIEW = 3,
    VIDEO_DEC_STATE_VIDEO_PLAY = 4, VIDEO_DEC_STATE_DELETE_CONFIRM = 5,
};
typedef enum {
    VIDEO_DEC_EVT_ENTER, VIDEO_DEC_EVT_EXIT, VIDEO_DEC_EVT_SCAN_DONE,
    VIDEO_DEC_EVT_SELECT_ITEM, VIDEO_DEC_EVT_SWIPE_LEFT,
    VIDEO_DEC_EVT_SWIPE_RIGHT, VIDEO_DEC_EVT_LONG_PRESS,
    VIDEO_DEC_EVT_DELETE_CONFIRM, VIDEO_DEC_EVT_DELETE_CANCEL,
    VIDEO_DEC_EVT_LOCK_TOGGLE, VIDEO_DEC_EVT_VIDEO_PLAY,
    VIDEO_DEC_EVT_VIDEO_PAUSE, VIDEO_DEC_EVT_VIDEO_STOP, VIDEO_DEC_EVT_BACK,
} video_dec_event_t;
#define VIDEO_DEC_TYPE_PHOTO 0
#define VIDEO_DEC_TYPE_VIDEO 1
#define VIDEO_DEC_THUMB_WIDTH 128
#define VIDEO_DEC_THUMB_HEIGHT 128
#define VIDEO_DEC_THUMB_CACHE_MAX 20
#define VIDEO_DEC_GRID_COLS 3
#define VIDEO_DEC_GRID_ROWS 3
#define VIDEO_DEC_GRID_COUNT (VIDEO_DEC_GRID_COLS * VIDEO_DEC_GRID_ROWS)
#define VIDEO_DEC_PHOTO_DIR CONFIG_ROOT_PATH"DCIM/PHOTO/"
#define VIDEO_DEC_VIDEO_DIR CONFIG_ROOT_PATH"DCIM/VIDEO/"
#define VIDEO_DEC_VDEC_DEV 0
#define VIDEO_DEC_VO_DEV 0
#include "media_file_list.h"
typedef flm_file_t video_dec_file_t;
typedef struct {
    u32 total_kb;
    u32 free_kb;
    u32 photo_count;
    u32 video_count;
} video_dec_storage_t;
struct app_mode *app_enter_video_dec_mode(int arg);
int video_dec_init(void);
void video_dec_exit(void);
int video_dec_scan_files(video_dec_file_t **list, int *count);
void video_dec_free_file_list(video_dec_file_t *list, int count);
int video_dec_generate_thumbnail(video_dec_file_t *file);
int video_dec_get_storage_info(video_dec_storage_t *info);
int video_dec_delete_file(const char *path);
int video_dec_lock_file(const char *path);
int video_dec_unlock_file(const char *path);
int video_dec_is_locked(const char *path);
int video_dec_view_photo(const char *path);
int video_dec_play_video(const char *path);
void video_dec_refresh_after_insert(void);  /* 插卡后重扫+自动显示第一个 */
void video_dec_clear_on_card_out(void);  /* 拔卡清理: 停播/清显示/清索引 */
int video_dec_pause_video(void);
int video_dec_resume_video(void);
int video_dec_stop_video(void);
int video_dec_video_progress(u32 *current_sec, u32 *total_sec);
enum video_dec_state video_dec_get_state(void);
int video_dec_get_current_index(void);
int video_dec_get_file_count(void);
int video_dec_open(void);
void video_dec_dispatch_play(void);
void video_dec_dispatch_next(void);
void video_dec_dispatch_prev(void);
/* P2: 音量调节 (仅 VIDEO_PLAY 状态有效) */
int video_dec_volume_up(void);
int video_dec_volume_down(void);
int video_dec_volume_get(void);
/* P2: 批量删除 / 一键清空 (跳过加锁文件) */
int video_dec_delete_selected(void);
int video_dec_delete_all(void);
/* 按键消息处理 */
extern int video_dec_app_msg_handler(int *msg);

#endif
