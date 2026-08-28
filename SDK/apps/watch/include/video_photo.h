#ifndef CONFIG_VIDEO_PHOTO_H
#define CONFIG_VIDEO_PHOTO_H

#include "system/includes.h"

/*==============================================================================
 * 拍照状态
 *==============================================================================
 */
enum photo_state {
    PHOTO_STATE_IDLE      = 0,  // 空闲预览
    PHOTO_STATE_BUSY      = 1,  // 拍照/编码/保存中
    PHOTO_STATE_COUNTDOWN = 2,  // 定时倒计时中
    PHOTO_STATE_BURST     = 3,  // 连拍中
};

/*==============================================================================
 * 拍照模式
 *==============================================================================
 */
enum photo_mode {
    PHOTO_MODE_SINGLE   = 0,
    PHOTO_MODE_TIMER_3S = 1,
    PHOTO_MODE_TIMER_5S = 2,
    PHOTO_MODE_TIMER_10S = 3,
    PHOTO_MODE_BURST    = 4,
};

/*==============================================================================
 * 拍照事件 — 状态机驱动
 *==============================================================================
 */
enum photo_event {
    PHOTO_EVT_SHUTTER,          // 短按拍照键
    PHOTO_EVT_SHUTTER_LONG,     // 长按拍照键(连拍)
    PHOTO_EVT_ZOOM_IN,          // 音量+
    PHOTO_EVT_ZOOM_OUT,         // 音量-
    PHOTO_EVT_MODE_SWITCH,      // 切换模式
    PHOTO_EVT_CAPTURE_DONE,     // 拍照完成
    PHOTO_EVT_CAPTURE_FAIL,     // 拍照失败
    PHOTO_EVT_TIMER_TICK,       // 定时器tick(每秒)
    PHOTO_EVT_TIMER_CANCEL,     // 取消定时
    PHOTO_EVT_SD_FULL,          // SD卡满
    PHOTO_EVT_SD_REMOVED,       // SD卡拔出
};

/*==============================================================================
 * 配置宏 — PRD 3.6
 *==============================================================================
 */
#define PHOTO_WIDTH              1280
#define PHOTO_HEIGHT             720
#define PHOTO_JPEG_QUALITY       95
#define PHOTO_ENTER_AI_TOOLS     0x4149
// 实际JPEG帧33~44KB; 256KB占1024物理页>空闲975页会vmalloc死机
#define PHOTO_STREAM_BUF_SIZE    (64 * 1024)

#define PHOTO_ZOOM_MIN           1.0f
#define PHOTO_ZOOM_MAX           4.0f
#define PHOTO_ZOOM_STEP          0.1f
#define PHOTO_ZOOM_QUALITY_WARN  3.0f

#define PHOTO_FILE_PATH          CONFIG_ROOT_PATH"DCIM/PHOTO/"
#define PHOTO_FILE_PREFIX        "IMG_"
#define PHOTO_FILE_EXT           ".JPG"

#define PHOTO_BURST_INTERVAL_MS  200
#define PHOTO_BURST_MAX          20

#define PHOTO_TIMER_3S           3
#define PHOTO_TIMER_5S           5
#define PHOTO_TIMER_10S          10

#define PHOTO_MIN_FREE_MB        10
#define PHOTO_DELETE_MAX_FILES   10   // 单次拍照前空间不足时最多删除的旧照片数
#define PHOTO_THUMBNAIL_MS       1500

/*==============================================================================
 * 模式生命周期 — 由 app_main 调用
 *==============================================================================
 */
struct app_mode *app_enter_photo_mode(int arg);
int  photo_init(void);
void photo_exit(void);
int  photo_timer_tick(void);         /* 主循环每秒调用，驱动倒计时/连拍 */

/*==============================================================================
 * 拍照操作
 *==============================================================================
 */
int  photo_capture(void);
int  photo_capture_jpeg_buffer(unsigned char *jpeg_buf, unsigned int jpeg_buf_size,
                               unsigned int *jpeg_size);

/*==============================================================================
 * 变焦 — 1.0x-4.0x，步进0.1x
 *==============================================================================
 */
int   photo_zoom_set(float level);
float photo_zoom_get(void);
float photo_zoom_in(void);
float photo_zoom_out(void);
/* 长按连续变焦: direction>0 放大, <0 缩小。
 * 由 photo_periodic_tick (100ms) 驱动步进, 到达 1.0x/4.0x 自动停。 */
void  photo_zoom_repeat_start(int direction);
void  photo_zoom_repeat_stop(void);

/*==============================================================================
 * 模式切换
 *==============================================================================
 */
int photo_mode_set(u8 mode);
u8  photo_mode_get(void);
int photo_timer_cancel(void);
int photo_burst_start(void);
int photo_burst_stop(void);

/*==============================================================================
 * 存储
 *==============================================================================
 */
int photo_storage_info(u32 *total_kb, u32 *free_kb);

/*==============================================================================
 * 状态查询
 *==============================================================================
 */
enum photo_state photo_state_get(void);
u32  photo_total(void);
/* 特效切换 */
extern int photo_effect_next(void);
extern int photo_effect_get(void);
extern int photo_effect_prev(void);
/* 按键消息处理 */
extern int video_photo_app_msg_handler(int *msg);

#endif /* CONFIG_VIDEO_PHOTO_H */
