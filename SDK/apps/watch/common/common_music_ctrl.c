#include "common_music_ctrl.h"

#if TCFG_APP_BT_EN
#include "bt_event_func.h"
#include "bt_key_func.h"
#include "a2dp_player.h"
#include "btstack/avctp_user.h"
#endif

#if TCFG_APP_MUSIC_EN
#include "app_music.h"
#include "file_player.h"
#include "music_player.h"
#endif

#include "app_mode_manager/app_mode_manager.h"
#include "app_msg.h"
#include "app_task.h"
#include "audio_config.h"
#include "vol_sync.h"
#include "jiffies.h"
#include "video_api.h"

/* #define LOG_TAG_CONST       MUSIC_CTRL */
#define LOG_TAG             "[MUSIC-CTRL]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

/* ================================================================
 *                         内部辅助函数
 * ================================================================*/

/**
 * @brief 获取当前活跃的音乐源
 */
static music_ctrl_source_t __music_ctrl_detect_source(void)
{
#if TCFG_APP_BT_EN && TCFG_APP_MUSIC_EN
    /* 连接优先：蓝牙已连 → BT，否则 → 本地 SD */
    if (bt_get_connect_status() != BT_STATUS_WAITINT_CONN) {
        return MUSIC_CTRL_SOURCE_BT;
    }
    return MUSIC_CTRL_SOURCE_LOCAL;
#elif TCFG_APP_BT_EN
    return MUSIC_CTRL_SOURCE_BT;
#elif TCFG_APP_MUSIC_EN
    return MUSIC_CTRL_SOURCE_LOCAL;
#else
    return MUSIC_CTRL_SOURCE_AUTO;
#endif
}

/**
 * @brief 获取当前模式对应的音乐模式ID
 */
static int __music_ctrl_get_target_mode(music_ctrl_source_t source)
{
#if TCFG_APP_BT_EN && TCFG_APP_MUSIC_EN
    if (source == MUSIC_CTRL_SOURCE_AUTO) {
        source = __music_ctrl_detect_source();
    }
    return (source == MUSIC_CTRL_SOURCE_BT) ? APP_MODE_BT : APP_MODE_MUSIC;
#elif TCFG_APP_BT_EN
    return APP_MODE_BT;
#elif TCFG_APP_MUSIC_EN
    return APP_MODE_MUSIC;
#else
    return -1;
#endif
}

/**
 * @brief 检查当前是否处于音乐相关模式
 */
static bool __music_ctrl_in_music_mode(void)
{
    int mode = app_get_current_mode_name();
#if TCFG_APP_BT_EN
    if (mode == APP_MODE_BT) {
        return true;
    }
#endif
#if TCFG_APP_MUSIC_EN
    if (mode == APP_MODE_MUSIC) {
        return true;
    }
#endif
    return false;
}

/* ================================================================
 *                         播放控制
 * ================================================================*/

int music_ctrl_play_pause_source(music_ctrl_source_t source)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return -1;
    }
#endif
    int target_mode = __music_ctrl_get_target_mode(source);
    int cur_mode = app_get_current_mode_name();

    log_info("<%s> source:%d, cur_mode:%d, target_mode:%d",
             __func__, source, cur_mode, target_mode);

    if (target_mode < 0) {
        log_error("<%s> no music mode available", __func__);
        return -1;
    }

    /* 如果不在目标模式，先切换模式（切换后会自动开始播放） */
    if (cur_mode != target_mode) {
#if TCFG_APP_MUSIC_EN
        if (target_mode == APP_MODE_MUSIC) {
            /* 本地音乐：设置从断点播放 */
            music_task_set_parm(MUSIC_TASK_START_BY_NORMAL, 0);
        }
#endif
        app_send_message(APP_MSG_GOTO_MODE, target_mode);
        return 0;
    }

    /* 已在目标模式，直接发送播放/暂停切换消息 */
    app_send_message(APP_MSG_MUSIC_PP, 0);
    return 0;
}

int music_ctrl_play_pause(void)
{
    return music_ctrl_play_pause_source(MUSIC_CTRL_SOURCE_AUTO);
}

int music_ctrl_play_source(music_ctrl_source_t source)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return -1;
    }
#endif
    int target_mode = __music_ctrl_get_target_mode(source);
    int cur_mode = app_get_current_mode_name();

    log_info("<%s> source:%d, cur_mode:%d, target_mode:%d",
             __func__, source, cur_mode, target_mode);

    if (target_mode < 0) {
        log_error("<%s> no music mode available", __func__);
        return -1;
    }

    /* 不在目标模式，直接切换即可，进入模式后会自动播放 */
    if (cur_mode != target_mode) {
        return music_ctrl_play_pause_source(source);
    }

    /* 已在目标模式，根据状态决定操作 */
    music_ctrl_state_t state = music_ctrl_get_state();

    switch (state) {
    case MUSIC_CTRL_STATE_PLAYING:
        return 0;

    case MUSIC_CTRL_STATE_PAUSED:
        app_send_message(APP_MSG_MUSIC_PP, 0);
        return 0;

    case MUSIC_CTRL_STATE_STOPPED:
    case MUSIC_CTRL_STATE_UNKNOWN:
        return music_ctrl_play_pause_source(source);

    default:
        return -1;
    }
}

int music_ctrl_play(void)
{
    return music_ctrl_play_source(MUSIC_CTRL_SOURCE_AUTO);
}

int music_ctrl_pause(void)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return 0;
    }
#endif
    music_ctrl_state_t state = music_ctrl_get_state();

    log_info("<%s> current state:%d", __func__, state);

    if (state == MUSIC_CTRL_STATE_PLAYING) {
        /* 播放中，发送PP切换暂停 */
        app_send_message(APP_MSG_MUSIC_PP, 0);
        return 0;
    }

    /* 已暂停或停止，无需操作 */
    return 0;
}

int music_ctrl_prev(void)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return -1;
    }
#endif
    if (!__music_ctrl_in_music_mode()) {
        log_error("<%s> not in music mode", __func__);
        return -1;
    }

    app_send_message(APP_MSG_MUSIC_PREV, 0);
    return 0;
}

int music_ctrl_next(void)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return -1;
    }
#endif
    if (!__music_ctrl_in_music_mode()) {
        log_error("<%s> not in music mode", __func__);
        return -1;
    }

    app_send_message(APP_MSG_MUSIC_NEXT, 0);
    return 0;
}

/* ================================================================
 *                         状态查询
 * ================================================================*/

music_ctrl_state_t music_ctrl_get_state(void)
{
#if TCFG_APP_BT_EN
    if (app_get_current_mode_name() == APP_MODE_BT) {
        u8 status = bt_a2dp_get_status();
        if (status == BT_MUSIC_STATUS_STARTING) {
            return MUSIC_CTRL_STATE_PLAYING;
        } else {
            return MUSIC_CTRL_STATE_PAUSED;
        }
    }
#endif

#if TCFG_APP_MUSIC_EN
    if (app_get_current_mode_name() == APP_MODE_MUSIC) {
        if (music_player_get_play_status()) {
            return MUSIC_CTRL_STATE_PLAYING;
        } else {
            return MUSIC_CTRL_STATE_PAUSED;
        }
    }
#endif

    return MUSIC_CTRL_STATE_UNKNOWN;
}

bool music_ctrl_is_playing(void)
{
    return (music_ctrl_get_state() == MUSIC_CTRL_STATE_PLAYING);
}

music_ctrl_source_t music_ctrl_get_source(void)
{
    return __music_ctrl_detect_source();
}

/* ================================================================
 *                         音量控制
 * ================================================================*/

void music_ctrl_volume_up(void)
{
#if TCFG_APP_BT_EN || TCFG_APP_MUSIC_EN
#if TCFG_BT_VOL_SYNC_ENABLE
    s16 volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    opid_play_vol_sync_fun(&volume, 1);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#endif
#else
    app_audio_volume_up(1);
#endif
#endif
}

void music_ctrl_volume_down(void)
{
#if TCFG_APP_BT_EN || TCFG_APP_MUSIC_EN
#if TCFG_BT_VOL_SYNC_ENABLE
    s16 volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    opid_play_vol_sync_fun(&volume, 0);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#endif
#else
    app_audio_volume_down(1);
#endif
#endif
}

void music_ctrl_volume_set(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }

    s16 volume_max = app_audio_volume_max_query(AppVol_BT_MUSIC);
    s16 volume = volume_max * percent / 100;

    log_info("<%s> percent:%d, volume:%d/%d", __func__, percent, volume, volume_max);

    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, volume, 1);

#if TCFG_APP_BT_EN
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#endif
}

int music_ctrl_volume_get_percent(void)
{
    s16 volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    s16 volume_max = app_audio_volume_max_query(AppVol_BT_MUSIC);

    if (volume_max <= 0) {
        return 0;
    }

    return volume * 100 / volume_max;
}

/* ================================================================
 *                  快捷操作
 * ================================================================*/

#define AUTO_PLAY_REENTRY_GUARD_MS   3000    /**< 3秒防重入窗口 */

/**
 * @brief 判断是否正在通话中（来电或接通）
 */
static int __music_ctrl_is_in_call(void)
{
#if TCFG_APP_BT_EN
    u8 call_status = bt_get_call_status();
    if (call_status == BT_CALL_INCOMING ||
        call_status == BT_CALL_ACTIVE ||
        call_status == BT_CALL_OUTGOING) {
        return call_status;
    }
#endif
    return -1;
}

/**
 * @brief 一键自动播放：蓝牙已连→播蓝牙，未连→播本地SD卡
 * @note 去电-不处理；来电-接听；通话-挂断
 */
void music_ctrl_local_play(u8 flag)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        log_info("<%s> video recording, skip", __func__);
        return;
    }
#endif
    static u32 s_last_call_tick = 0;
    u32 now = jiffies_msec();

    /* 来电或通话中 → 去电不处理，来电接听，通话挂断 */
#if TCFG_APP_BT_EN
    int call_status = __music_ctrl_is_in_call();
    if (call_status == BT_CALL_OUTGOING) {
        log_info("<%s> out call, skip", __func__);
        return;
    } else if (call_status == BT_CALL_INCOMING) {
        log_info("<%s> in call, answer", __func__);
        bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        return;
    } else if (call_status == BT_CALL_ACTIVE) {
        log_info("<%s> calling, hangup", __func__);
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    }
#endif

    /* 防重入：3秒内重复调用直接忽略 */
    if (s_last_call_tick != 0 && (u32)jiffies_msec2offset(s_last_call_tick, now) < AUTO_PLAY_REENTRY_GUARD_MS) {
        log_info("<%s> reentry guarded, last:%d ms ago",
                 __func__, (int)(now - s_last_call_tick));
        return;
    }
    s_last_call_tick = now;

    /* 决定播放源：蓝牙已连接 → BT，否则 → 本地SD */
    music_ctrl_source_t source;
#if TCFG_APP_BT_EN && TCFG_APP_MUSIC_EN
    if (bt_get_connect_status() != BT_STATUS_WAITINT_CONN) {
        source = MUSIC_CTRL_SOURCE_BT;
    } else {
        source = MUSIC_CTRL_SOURCE_LOCAL;
    }
#elif TCFG_APP_BT_EN
    source = MUSIC_CTRL_SOURCE_BT;
#elif TCFG_APP_MUSIC_EN
    source = MUSIC_CTRL_SOURCE_LOCAL;
#else
    log_error("<%s> no music source available", __func__);
    return;
#endif

    int cur_mode = app_get_current_mode_name();
    int target_mode = __music_ctrl_get_target_mode(source);

    log_info("<%s> source:%d, cur_mode:%d, target_mode:%d, is_playing:%d",
             __func__, source, cur_mode, target_mode, music_ctrl_is_playing());

    /* 不在目标模式 → 切换模式（进入后自动播放） */
    if (cur_mode != target_mode) {
        music_ctrl_play_pause_source(source);
        return;
    }

    /* 已在目标模式 → 正在播放切下一曲，否则恢复/开始播放 */
    if (music_ctrl_is_playing()) {
        if (flag == 0) {
            music_ctrl_next();
        } else if (flag == 1) {
            music_ctrl_prev();
        }
    } else {
        music_ctrl_play_source(source);
    }
}

