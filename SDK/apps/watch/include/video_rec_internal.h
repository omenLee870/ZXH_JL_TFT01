/**
 * @file video_rec_internal.h
 * @brief 录像模块内部头文件 (ctx定义 + 事件类型 + 状态机 + worker接口)
 *
 * 仅供 video_rec.c 和 video_rec_worker.c 使用，不对外暴露
 *
 * ## 状态机与回调埋点 (COD1-P0)
 *
 *   video_rec.c 维护单一权威状态机，所有状态切换必须经过
 *   video_rec_transition_to()，由它统一完成：
 *     1. 状态字段更新 (g_rec_ctx.current_state + 同步 recording/paused 位)
 *     2. UI 通知     -> video_rec_ui_on_state_change(old, new)
 *     3. 计时器启停  -> RECORDING 时启动 500ms sys_timer (video_rec_ui_tick_500ms)
 *                      非 RECORDING 时停止该 timer
 *
 *   触发源 (均在 video_rec.c 中):
 *     - video_rec_start()  (worker 线程) : IDLE -> RECORDING
 *     - video_rec_stop()   (主线程)     : (PAUSED|RECORDING) -> IDLE
 *     - video_rec_worker() 退出路径     : RECORDING -> IDLE
 *     - video_rec_pause()  (主线程)     : RECORDING -> PAUSED
 *     - video_rec_resume() (主线程)     : PAUSED -> RECORDING
 *
 * ## Alert 弹窗埋点 (COD1-P1)
 *
 *   video_rec.c 在以下错误路径调用 video_rec_ui_show_alert(type) 提示用户:
 *
 *     | 触发位置                          | type | 弹窗文案          | 触发场景                  |
 *     |-----------------------------------|------|-------------------|---------------------------|
 *     | video_rec_mode_init()             |  1   | "请插入SD卡"      | 摄像头 sensor 打开失败     |
 *     | video_rec_mode_init()             |  2   | "电量不足"        | 显示通道启动失败           |
 *     | video_rec_start()                 |  2   | "电量不足"        | 状态异常 (重复 start)      |
 *     | video_rec_start()                 |  0   | "存储已满"        | 存储空间 < 10MB           |
 *     | video_rec_start()                 |  2   | "电量不足"        | 任务创建失败               |
 *     | video_rec_resume()                |  2   | "电量不足"        | 编码器/录制恢复失败         |
 *     | video_rec_worker() (worker 线程)  |  2   | "电量不足"        | 编码器/录制启动失败         |
 *     | video_rec_worker() 退出路径       |  3   | "保存完成"        | AVI 文件已保存 (2s 自动消失) |
 *
 *   说明:
 *     - type=2 在 UI 实现里是"电量不足"，这里复用作通用系统错误.
 *       type 编号的扩展是后续 P2 阶段 UI 一起更新的工作.
 *     - alert 弹窗由 worker 线程直接调用 (与 photo.c 中 photo_ui_show_alert
 *       一致), 当前 UI stub 是空函数, 真实 LVGL 接入时再考虑主线程派发.
 */
#ifndef VIDEO_REC_INTERNAL_H
#define VIDEO_REC_INTERNAL_H

#include "video_rec.h"
#include "key_driver.h"
#include "video_rec_ui.h"

/*==============================================================================
 * 内部事件类型
 *==============================================================================*/
typedef enum {
    VIDEO_REC_EVT_START        = 0x01,
    VIDEO_REC_EVT_STOP         = 0x02,
    VIDEO_REC_EVT_STOP_FORCE   = 0x03,
    VIDEO_REC_EVT_PAUSE        = 0x04,
    VIDEO_REC_EVT_RESUME       = 0x05,
    VIDEO_REC_EVT_TOGGLE        = 0x06,  /* CLICK toggle: 录制中->STOP, 空闲->START */
    VIDEO_REC_EVT_MODE_SWITCH  = 0x07,  /* 长按切模式: IOKEY HOLD -> 退出录像模式 */
    VIDEO_REC_EVT_SD_OUT       = 0x10,
    VIDEO_REC_EVT_SD_FULL      = 0x11,
    VIDEO_REC_EVT_LOW_BATTERY  = 0x12,
    VIDEO_REC_EVT_ENC_ERROR    = 0x20,
    VIDEO_REC_EVT_WORKER_ABORT = 0x21,
} video_rec_event_t;

/*==============================================================================
 * 事件 dispatch: 按键 -> 内部事件
 *
 *   童趣相机只有单 IOKEY 键, 所以这里只映射 KEY_IO_NUM0:
 *     - CLICK  -> VIDEO_REC_EVT_MODE_SWITCH (退出录像模式, t_313c3290)
 *     - HOLD   -> VIDEO_REC_EVT_TOGGLE      (t_7bb40022: caller 改用此返回值记标志,
 *                                             抬起 UP 时才执行 toggle)
 *     - 其它   -> -1 (忽略)
 *
 *   注意: KEY_ACTION_LONG 也触发此函数 (走 HOLD 分支上行等同).
 *         KEY_ACTION_UP 不在本函数处理 — caller 通过 static long_pressed
 *         标志判断是否执行 toggle (t_7bb40022).
 *
 *   底层 key_driver 已经做了消抖/防误触, 这里不做任何额外过滤.
 *==============================================================================*/
static inline int video_rec_map_key_to_event(u8 key_value, u8 key_action)
{
    if (key_value != KEY_IO_NUM0) {
        return -1;
    }
    if (key_action == KEY_ACTION_CLICK) {
        return VIDEO_REC_EVT_MODE_SWITCH;
    }
    if (key_action == KEY_ACTION_HOLD) {
        return VIDEO_REC_EVT_TOGGLE;
    }
    return -1;
}

/*==============================================================================
 * 状态机切换 (COD1-P0)
 *
 * 调用方:
 *   video_rec.c 内的 start/stop/pause/resume 函数,
 *   以及 video_rec_worker 线程退出路径
 *
 * 行为:
 *   - 若 new_state == current_state，直接返回 (幂等)
 *   - 原子地更新 g_rec_ctx.current_state 及同步 recording/paused 位
 *   - 调用 video_rec_ui_on_state_change() 通知 UI
 *   - 若新状态为 RECORDING，启动 500ms 计时器;
 *     否则停止该计时器并清零句柄
 *
 * @param new_state 目标状态 (enum video_rec_state)
 *==============================================================================
 */
void video_rec_transition_to(enum video_rec_state new_state);

/*==============================================================================
 * 500ms UI 计时回调
 *
 * 由 video_rec_transition_to() 在进入 RECORDING 时通过 sys_timer_add() 启动，
 * 离开 RECORDING 时通过 sys_timer_del() 停止。
 * 每 500ms 调用 video_rec_ui_update_timer() 刷新 UI 上的录像计时。
 *
 * @param priv  sys_timer 私有参数 (未使用)
 *==============================================================================
 */
void video_rec_ui_tick_500ms(void *priv);

/* 在 app_core 线程执行 UI 500ms 定时器的启停（由 APP_MSG_VIDEO_REC_UI_TIMER_SYNC 触发） */
void video_rec_ui_timer_sync(int new_state);

/*==============================================================================
 * 全局上下文 extern
 *==============================================================================*/
extern video_rec_ctx_t g_rec_ctx;

/*==============================================================================
 * Worker 接口 (实现在 video_rec_worker.c)
 *==============================================================================*/
/* AVI 修复 */
int video_rec_repair_avi(const char *path);

/* 文件数量限制 (Phase 1): 计数 + 超 MAX_FILES 时删除最旧 */
int video_rec_count_files(void);
int video_rec_check_file_count(void);

/* 存储管理: 声明在 video_rec.h */
/* 文件加锁: 声明在 video_rec.h */
/* 循环录像配置: 声明在 video_rec.h */
/* 获取当前文件路径: 声明在 video_rec.h */

#endif /* VIDEO_REC_INTERNAL_H */
