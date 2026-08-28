/**
 * @file video_player.h
 * @brief 统一视频播放模块 — 封装 app_playback_mpi 管线 + LRU 帧缓存
 *
 * 提供 AVI/MJPEG 视频播放能力:
 * - 单次播放 / 循环播放
 * - 带音频 / 静音播放
 * - 仅解码首帧（缩略图）
 * - 暂停/恢复帧推送
 * - LRU 帧缓存 (数量可动态配置, 默认 10)
 *
 * 内部使用 VDEC→VPSS→VO→fb 硬件管线,
 * fb_data_updata 回调驱动帧就绪通知。
 *
 * @author PM
 * @date   2026-08-05
 */
#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include "system/includes.h"

/* ------------------------------------------------------------------*/
/* 视频表盘分辨率宏 (用在 ui_action_dial.c)                          */
/* ------------------------------------------------------------------*/
#define VIDEO_DIAL_WIDTH    464
#define VIDEO_DIAL_HEIGHT   348

/* ------------------------------------------------------------------*/
/* 播放模式 */
/* ------------------------------------------------------------------*/
typedef enum {
    VIDEO_PLAYER_MODE_ONCE = 0,         /* 单次播放 (播完停止)    */
    VIDEO_PLAYER_MODE_LOOP,             /* 循环播放 (播完自动重播) */
} video_player_mode_t;

/* ------------------------------------------------------------------*/
/* 音频模式 */
/* ------------------------------------------------------------------*/
typedef enum {
    VIDEO_PLAYER_AUDIO_OFF = 0,         /* 静音 (不播放音频) */
    VIDEO_PLAYER_AUDIO_ON,              /* 带音频播放         */
} video_player_audio_t;

/* ------------------------------------------------------------------*/
/* 句柄 (opaque) */
/* ------------------------------------------------------------------*/
typedef struct video_player_ctx *video_player_handle_t;

/* ------------------------------------------------------------------*/
/* 帧就绪回调类型 (通过 os_taskq_post Q_CALLBACK 在任务上下文调用)   */
/* ------------------------------------------------------------------*/
typedef void (*video_player_frame_cb_t)(void *h);

/* ==================================================================*/
/* 生命周期                                                          */
/* ==================================================================*/

/**
 * @brief 打开视频播放器并创建硬件解码管线
 *
 * @param path      AVI/MJPEG 文件路径
 * @param mode      播放模式 (ONCE / LOOP)
 * @param audio     音频开关 (OFF / ON)
 * @param fb_index  输出 framebuffer 索引 (如 5 对应 fb5)
 * @param width     显示宽度
 * @param height    显示高度
 *
 * @return handle  成功返回句柄, 失败返回 NULL
 */
video_player_handle_t video_player_open(const char *path,
                                        video_player_mode_t mode,
                                        video_player_audio_t audio,
                                        int fb_index,
                                        int width,
                                        int height);

/**
 * @brief 关闭播放器, 释放所有资源 (包括帧缓存)
 */
void video_player_close(video_player_handle_t h);

/* ==================================================================*/
/* 缓存配置                                                          */
/* ==================================================================*/

/**
 * @brief 设置帧缓存最大数量 (默认 10)
 *
 * @param h     句柄
 * @param max   最大缓存帧数 (>=1)
 */
void video_player_set_cache_max(video_player_handle_t h, int max);

/* ==================================================================*/
/* 播放控制                                                          */
/* ==================================================================*/

/**
 * @brief 设置音频模式 (在 start/play_file 之前调用生效)
 *
 * @param h     句柄
 * @param audio 音频模式 (AUDIO_ON / AUDIO_OFF)
 */
void video_player_set_audio(video_player_handle_t h, video_player_audio_t audio);

/**
 * @brief 开始/恢复视频解码
 *
 * @return 0=成功, <0=失败
 */
int video_player_start(video_player_handle_t h);

/**
 * @brief 停止视频解码, 释放硬件管线
 *
 * @return 0=成功, <0=失败
 */
int video_player_stop(video_player_handle_t h);

/**
 * @brief 停止视频解码但保留 VDEC 管线 (用于缩略图复用场景)
 *
 * 仅调用 app_playback_video_finish, 不关闭硬件管线,
 * 后续 video_player_play_file 可快速解码不同文件。
 */
int video_player_stop_keep_pipeline(video_player_handle_t h);

/**
 * @brief 解码指定文件的首帧 (相机缩略图场景)
 *
 * 更新路径后启动解码, AUDIO_OFF 模式自动静音。
 * 调用方通过 video_player_get_frame_data() 获取帧数据。
 *
 * 内部基于文件路径 djb2 hash 做帧缓存校验:
 * 若同一文件的首帧已在 LRU 缓存中, 则跳过 demux/decode,
 * 直接返回 1 通知调用方帧已就绪。
 *
 * @param h     句柄
 * @param path  新文件路径
 * @return  1=缓存命中(帧已就绪, 无需等待 fb 中断),
 *          0=正常启动解码(需等待 fb 中断回调后取帧),
 *         <0=失败
 */
int video_player_play_file(video_player_handle_t h, const char *path);

/**
 * @brief 暂停帧推送到显示 (解码继续在后台运行)
 *
 * @return 0=成功, <0=失败
 */
int video_player_pause(video_player_handle_t h);

/**
 * @brief 恢复帧推送到显示
 *
 * @return 0=成功, <0=失败
 */
int video_player_resume(video_player_handle_t h);

/* ==================================================================*/
/* 首帧解码                                                          */
/* ==================================================================*/

/**
 * @brief 仅解码第一帧 (用于生成缩略图)
 *
 * 阻塞等待首帧就绪后自动停止视频播放,
 * 调用方通过 video_player_get_frame_data() 获取帧数据。
 *
 * @return 0=成功, <0=失败
 */
int video_player_decode_first_frame(video_player_handle_t h);

/* ==================================================================*/
/* 状态查询                                                          */
/* ==================================================================*/

int video_player_is_playing(video_player_handle_t h);
int video_player_is_paused(video_player_handle_t h);
int video_player_get_width(video_player_handle_t h);
int video_player_get_height(video_player_handle_t h);

/* ==================================================================*/
/* 帧数据获取                                                        */
/* ==================================================================*/

/**
 * @brief 获取最新解码帧数据指针 (YUV420SP 格式)
 *
 * @return 帧数据指针, 未就绪返回 NULL
 */
u8 *video_player_get_frame_data(video_player_handle_t h);

/**
 * @brief 获取帧像素格式 (用于 GPU 渲染)
 *
 * @return GPU 格式枚举值
 */
int video_player_get_frame_format(video_player_handle_t h);

/* ==================================================================*/
/* 回调注册                                                          */
/* ==================================================================*/

/**
 * @brief 注册帧就绪回调 (在 fb_data_updata 中调用)
 *
 * 调用方通过此回调触发 UI 刷新 (ui_redraw)。
 */
void video_player_set_frame_callback(video_player_handle_t h,
                                     video_player_frame_cb_t cb);

/* ==================================================================*/
/* 循环间隔配置                                                      */
/* ==================================================================*/

/**
 * @brief 设置 LOOP 模式循环间隔时间
 *
 * 仅在 VIDEO_PLAYER_MODE_LOOP 模式下生效。
 * 播放完一个循环后暂停 interval_ms 毫秒，再自动重新播放。
 * 间隔期间 UI 显示最后一帧画面（静止），按键响应不受影响。
 *
 * @param h            句柄
 * @param interval_ms  间隔毫秒数, 0=立即重播（默认行为）
 *
 * @note  在 video_player_start 之前调用即可生效
 */
void video_player_set_loop_interval(video_player_handle_t h, int interval_ms);

/**
 * @brief 获取当前循环间隔配置
 *
 * @param h  句柄
 * @return   间隔毫秒数, 0=立即重播
 */
int video_player_get_loop_interval(video_player_handle_t h);

/* ==================================================================*/
/* taskq 延迟追踪 (调试用)                                           */
/* ==================================================================*/

/** @brief 未处理回调上限: 超过此值后 deliver_frame 不再投递新的 Q_CALLBACK,
 *  等 UI 消费掉积压回调后再恢复投递, 防止 taskq 积压导致按键延迟 */
#ifndef VIDEO_PLAYER_MAX_PENDING_CB
#define VIDEO_PLAYER_MAX_PENDING_CB  3
#endif

/**
 * @brief 标记一个回调已被 UI 消费 (递减 cb_pending, 内部使用)
 */
void video_player_cb_consumed(void);

/**
 * @brief 帧回调投递 payload. deliver_frame 填入 seq+time 后通过
 *        taskq Q_CALLBACK 传递给 dial_video_frame_cb,
 *        避免 ctx 中 seq 被连续投递重入覆盖.
 */
struct video_player_cb_payload {
    video_player_handle_t ctx;      /* 真正的 player 句柄 */
    u32 seq;                        /* 投递序号 */
    u32 post_time;                  /* 投递时刻 (jiffies_msec) */
};

/* ==================================================================*/
/* fb_data_updata 集成                                               */
/* ==================================================================*/

/**
 * @brief 全局帧数据处理入口 (由 fb_data_updata 调用)
 *
 * 遍历所有活跃的 player handle, 将帧数据分发到匹配 fb_index 的 player。
 *
 * @param fb_index  framebuffer 索引
 * @param baddr     帧数据虚拟地址
 *
 * @return 1=有 player 处理了此帧, 0=无匹配 player
 */
int video_player_fb_handler(u8 fb_index, u8 *baddr);

#endif /* VIDEO_PLAYER_H */
