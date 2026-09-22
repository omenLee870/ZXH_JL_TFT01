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
/*==============================================================================
 * 照片缩略图引擎契约(六宫格页共用): 阶段一=JPG 真解码; AVI 首帧阶段二, 本阶段保持占位。
 * 内存纪律(1MB 收紧): 无全局常驻缓存(离开页全释放); 页内 LRU<=6 且 path 键;
 * JPG 整读硬顶 128KB; 解码中间档仅 160x120(禁 320x240 档); 单张峰值预算约 210KB。
 *==============================================================================*/
#define VIDEO_DEC_THUMB_TILE_W        96   /* 格占位区 96x64 */
#define VIDEO_DEC_THUMB_TILE_H        64
/* NV12 直显测试开关: 1=复现 jlgpu 渲染侧滞留物理池问题(2026-09-07), 0=现网 RGB565 */
#define VIDEO_DEC_TILE_NV12_TEST      0
#if VIDEO_DEC_TILE_NV12_TEST
#define VIDEO_DEC_THUMB_TILE_STRIDE   VIDEO_DEC_THUMB_TILE_W              /* NV12: Y 平面行宽 */
#define VIDEO_DEC_THUMB_TILE_BYTES    (VIDEO_DEC_THUMB_TILE_W * VIDEO_DEC_THUMB_TILE_H * 3 / 2)
#else
/* RGB565 行宽/总字节(96x64x2); NV12 直显见 video_dec.c 开关 */
#define VIDEO_DEC_THUMB_TILE_STRIDE   (VIDEO_DEC_THUMB_TILE_W * 2)
#define VIDEO_DEC_THUMB_TILE_BYTES    (VIDEO_DEC_THUMB_TILE_W * VIDEO_DEC_THUMB_TILE_H * 2)
#endif
#define VIDEO_DEC_THUMB_FULL_READ_MAX (128u * 1024u)  /* JPG 整读硬顶(不做 1/4 空闲上浮) */
#define VIDEO_DEC_THUMB_VPSS_W_MAX    160            /* VPSS 中间档上限 */
#define VIDEO_DEC_THUMB_VPSS_H_MAX    120
#define VIDEO_DEC_THUMB_LRU_MAX       6              /* 页内 path 键 LRU 上限 */
#define VIDEO_DEC_THUMB_SLOT_MAX      8              /* 请求快照槽上限(只拷 path/格子号) */
#define VIDEO_DEC_THUMB_WORKER_PRIO   20             /* worker 低优先级 */
#define VIDEO_DEC_THUMB_WORKER_STACK  1024           /* 4KB: thread_fork 栈参数单位=4字节 */
/* AVI 首帧(阶段二)预留防护宏: 本阶段仅定义不参与逻辑 */
#define VIDEO_DEC_AVI_FRAME_MAX       4096
#define VIDEO_DEC_AVI_CHUNK_MAX       (256u * 1024u)
/* 就绪回调: worker 线程调用; UI 侧只允许拷 path/gidx 后经 lvgl_rpc 归队 */
typedef void (*video_dec_thumb_ready_cb_t)(void *priv, const char *path, int gidx);
/* 页请求: 快照 [base, base+n) 的照片槽 (UI 线程, 非阻塞, 命中 LRU 不再解码) */
int video_dec_thumb_request(int base, int n);
/* 请求后台预热某个 index 的文件(FAT 冷路径): 只置标志, 供 UI 在进多宫格/移光标时调用 */
void video_dec_fat_warm_request(int idx);
/* LRU 命中返回 tile 指针(96x64 RGB565), 未命中返回 NULL; 供 UI 线程挂图 */
const void *video_dec_thumb_peek(const char *path);
/* 注册就绪回调(页面生命周期: LOAD 注册 / UNLOAD 摘除) */
void video_dec_thumb_set_ready_cb(video_dec_thumb_ready_cb_t cb, void *priv);
/* 置取消并等当前张解码结束(单张有界); 返回后不再处理旧快照 */
int video_dec_thumb_cancel_wait(void);
/* 异步取消(UI 线程路径用): 只置取消+清 WAIT 槽+唤醒, 不等待——防 UI 卡 */
void video_dec_thumb_cancel_async(void);
/* 全屏回放模式是否激活(激活期缩略图解码被禁, UI 侧据此延迟补发请求) */
int video_dec_thumb_mode_active(void);
/* 失效: 代际号++ + 清快照 + LRU 全部释放归零(删除确认/拔卡/DB_CHANGED/重进页兜底) */
void video_dec_thumb_invalidate(void);
/* 离开六宫格页全释放: 停 worker + 清 LRU/快照/回调(无全局常驻缓存) */
void video_dec_thumb_release_all(void);
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
