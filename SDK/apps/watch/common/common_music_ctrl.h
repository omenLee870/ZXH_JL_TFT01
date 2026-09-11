#ifndef _COMMON_MUSIC_CTRL_H
#define _COMMON_MUSIC_CTRL_H

#include "app_config.h"
#include "generic/typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 * TYPEDEFS
 *********************/

/** 音乐源类型 */
typedef enum {
    MUSIC_CTRL_SOURCE_AUTO  = 0,    /**< 自动检测当前活跃源 */
    MUSIC_CTRL_SOURCE_BT,           /**< 蓝牙 A2DP 播放 */
    MUSIC_CTRL_SOURCE_LOCAL,        /**< 本地 SD 卡播放 */
} music_ctrl_source_t;

/** 播放状态 */
typedef enum {
    MUSIC_CTRL_STATE_STOPPED = 0,   /**< 停止 */
    MUSIC_CTRL_STATE_PLAYING,       /**< 播放中 */
    MUSIC_CTRL_STATE_PAUSED,        /**< 暂停 */
    MUSIC_CTRL_STATE_UNKNOWN,       /**< 未知（未初始化或不在音乐相关模式） */
} music_ctrl_state_t;

/**********************
 * API
 *********************/

/* ========== 播放控制 ========== */

/**
 * @brief 播放/暂停切换
 * @note  自动检测当前出声源（BT 或本地音乐），如果不在对应模式则先切换模式再操作
 * @return 0 成功, -1 失败
 */
int music_ctrl_play_pause(void);

/**
 * @brief 播放/暂停切换（指定源）
 * @param source  MUSIC_CTRL_SOURCE_AUTO=自动检测, BT=蓝牙, LOCAL=SD卡
 * @note  当 source 为 AUTO 时行为同 music_ctrl_play_pause()
 * @return 0 成功, -1 失败
 */
int music_ctrl_play_pause_source(music_ctrl_source_t source);

/**
 * @brief 开始播放
 * @note  如果当前已在播放则无操作；如果暂停则恢复；如果停止则开始播放
 * @return 0 成功, -1 失败
 */
int music_ctrl_play(void);

/**
 * @brief 开始播放（指定源）
 * @param source  MUSIC_CTRL_SOURCE_AUTO=自动检测, BT=蓝牙, LOCAL=SD卡
 * @note  当 source 为 AUTO 时行为同 music_ctrl_play()
 * @return 0 成功, -1 失败
 */
int music_ctrl_play_source(music_ctrl_source_t source);

/**
 * @brief 暂停播放
 * @note  如果已暂停或无播放则无操作
 * @return 0 成功, -1 失败
 */
int music_ctrl_pause(void);

/**
 * @brief 去电通话打断本地音乐：暂停本地播放
 * @note  仅当处于本地音乐模式且正在播放时才暂停，并记录恢复标志供通话结束后恢复
 * @return 0 成功, -1 失败
 */
int music_ctrl_pause_for_call(void);

/**
 * @brief 通话结束后恢复因通话而暂停的本地音乐
 * @note  仅在之前由 music_ctrl_pause_for_call 暂停过时才恢复
 * @return 0 成功, -1 失败
 */
int music_ctrl_resume_after_call(void);

/**
 * @brief 上一曲
 * @return 0 成功, -1 失败
 */
int music_ctrl_prev(void);

/**
 * @brief 下一曲
 * @return 0 成功, -1 失败
 */
int music_ctrl_next(void);

/* ========== 状态查询 ========== */

/**
 * @brief 获取当前播放状态
 * @return 播放状态枚举
 */
music_ctrl_state_t music_ctrl_get_state(void);

/**
 * @brief 是否正在播放
 * @return true 播放中, false 未播放
 */
bool music_ctrl_is_playing(void);

/**
 * @brief 获取当前活跃的音乐源
 * @return 音乐源类型
 */
music_ctrl_source_t music_ctrl_get_source(void);

/* ========== 音量控制 ========== */

/**
 * @brief 音量加
 */
void music_ctrl_volume_up(void);

/**
 * @brief 音量减
 */
void music_ctrl_volume_down(void);

/**
 * @brief 设置音量百分比
 * @param percent 0~100
 */
void music_ctrl_volume_set(int percent);

/**
 * @brief 获取当前音量百分比
 * @return 0~100
 */
int music_ctrl_volume_get_percent(void);

/**
 * @brief 一键自动播放：蓝牙已连→播蓝牙，未连→播本地SD卡
 * @note 去电-不处理；来电-接听；通话-挂断
 */
void music_ctrl_local_play(u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_MUSIC_CTRL_H */