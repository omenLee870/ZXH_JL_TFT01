#include "pet_reverse_notice.h"
#include <string.h>
#ifdef PET_REVERSE_NOTICE_TEST
#include <stdio.h>
#endif

#ifndef PET_REVERSE_NOTICE_TEST
#include "app_config.h"
#include "pet_dashboard_music.h"
#include "system/timer.h"
#include "os/os_api.h"
#include "tone_player.h"
#include "fs/resfile.h"
#if TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
#include "btstack/avctp_user.h"
#include "bt_event_func.h"
#include "bt.h"
#endif
#define NOTICE_FILE "tone_zh/reverse.wav"

static bool pet_reverse_notice_media_state(uint8_t address[6], bool *playing)
{
    *playing = false;
#if TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
    if (!get_bt_init_status() || !bt_get_total_connect_dev()) return false;
    u8 *remote = bt_get_current_remote_addr();
    if (!remote) return false;
    memcpy(address, remote, 6);
    *playing = bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING;
    return true;
#else
    (void)address;
    return false;
#endif
}

static int pet_reverse_notice_media_command(bool play)
{
#if TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
    return bt_cmd_prepare(play ? USER_CTRL_AVCTP_OPID_PLAY : USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
#else
    (void)play;
    return -1;
#endif
}

static bool pet_reverse_notice_resource_ready(void)
{
    /* 不把倒车提示排在其他长提示音后面，也不抢占其他提示音。 */
    if (tone_player_runing()) return false;
    void *file = resfile_open(FLASH_RES_PATH NOTICE_FILE);
    if (!file) return false;
    resfile_close(file);
    return true;
}
#endif

#define PAUSE_TIMEOUT_MS 1500U
#define RESTORE_TIMEOUT_MS 2000U
#define NOTICE_TIMEOUT_MS 12000U
#define NOTICE_INTERVAL_MS 3000U /* 每段语音播完后静音等待的时间。 */

enum notice_phase { NOTICE_IDLE, NOTICE_WAIT_PAUSE, NOTICE_SPEAKING, NOTICE_WAIT_INTERVAL, NOTICE_RESTORING };
static enum notice_phase phase;
static uint32_t generation;
/* 音频线程仅写完成标记，不能在播放器互斥锁内操作蓝牙或停止播放器。 */
static volatile uint32_t completed_generation;
static uint32_t phase_at;
static uint32_t restore_at;
static uint8_t paused_address[6];
static bool resume_owned;
static bool camera_active;
static bool restart_pending;
static bool view_hold;
/* 单次写入记录 UI 最新意图；即使退出消息队列满，也能由 100ms 检测取消循环。 */
static volatile int requested_state = -1;

void pet_reverse_notice_done(uint32_t value)
{
    if (value == generation) completed_generation = value;
}

#ifndef PET_REVERSE_NOTICE_TEST
static int notice_callback(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) pet_reverse_notice_done((uint32_t)(uintptr_t)priv);
    return 0;
}

static int pet_reverse_notice_play(uint32_t value)
{
    return play_tone_file_alone_callback_psram(NOTICE_FILE, (void *)(uintptr_t)value, notice_callback);
}

static void pet_reverse_notice_stop(uint32_t value)
{
    tone_player_stop_by_callback((void *)(uintptr_t)value, notice_callback);
}
#endif

static bool same_phone(bool *playing)
{
    uint8_t address[6];
    return pet_reverse_notice_media_state(address, playing) && !memcmp(address, paused_address, 6);
}

static void notice_finish(uint32_t now_ms)
{
    bool playing = false;
    phase = NOTICE_IDLE;
    if (resume_owned && same_phone(&playing) && !playing) {
        int ret = pet_reverse_notice_media_command(true);
        printf("[REVERSE_VOICE] resume ret=%d\n", ret);
        if (!ret) {
            phase = NOTICE_RESTORING;
            phase_at = now_ms;
            view_hold = true;
            restore_at = now_ms;
        }
    }
    resume_owned = false;
    printf("[REVERSE_VOICE] finished\n");
}

static void notice_start(uint32_t now_ms)
{
    /* 每一轮使用新标记，上一轮迟到的 STOP 不能结束新一轮播放。 */
    if (++generation == 0) ++generation;
    completed_generation = 0;
    phase = NOTICE_SPEAKING;
    phase_at = now_ms;
    int ret = pet_reverse_notice_play(generation);
    printf("[REVERSE_VOICE] play generation=%u ret=%d\n", generation, ret);
    if (ret) notice_finish(now_ms);
}

static void notice_begin(uint32_t now_ms)
{
    restart_pending = false;
    if (!pet_reverse_notice_resource_ready()) {
        printf("[REVERSE_VOICE] skipped: resource missing or tone busy\n");
        return;
    }
    bool playing = false;
    bool connected = pet_reverse_notice_media_state(paused_address, &playing);
    resume_owned = false;
    if (connected && playing) {
        int ret = pet_reverse_notice_media_command(false);
        printf("[REVERSE_VOICE] pause ret=%d\n", ret);
        if (ret) return;
        resume_owned = true;
        phase = NOTICE_WAIT_PAUSE;
        phase_at = now_ms;
    } else {
        notice_start(now_ms);
    }
}

void pet_reverse_notice_handle(bool enter, uint32_t now_ms)
{
    camera_active = enter;
    if (enter) {
        if (phase == NOTICE_IDLE) notice_begin(now_ms);
        else if (phase == NOTICE_RESTORING) restart_pending = true;
    } else {
        restart_pending = false;
        if (phase == NOTICE_SPEAKING) {
            pet_reverse_notice_stop(generation);
            notice_finish(now_ms);
        } else if (phase == NOTICE_WAIT_INTERVAL) {
            notice_finish(now_ms);
        }
        /* 等 PAUSE 的反馈再恢复，避免快速退出时 PLAY 先于 PAUSE 生效。 */
    }
}

void pet_reverse_notice_tick(uint32_t now_ms)
{
    if (requested_state == 0 && camera_active) pet_reverse_notice_handle(false, now_ms);
    if (view_hold && (uint32_t)(now_ms - restore_at) >= RESTORE_TIMEOUT_MS) view_hold = false;
    bool playing = false;
    bool same = same_phone(&playing);
    if (resume_owned && !same) resume_owned = false;
    switch (phase) {
    case NOTICE_WAIT_PAUSE:
        if (!same || !playing) {
            if (camera_active) notice_start(now_ms);
            else notice_finish(now_ms);
        } else if ((uint32_t)(now_ms - phase_at) >= PAUSE_TIMEOUT_MS) {
            printf("[REVERSE_VOICE] skipped: pause not confirmed\n");
            /* 发送 PLAY 撤销可能迟到的 PAUSE，不能因为当前仍播放就跳过撤销。 */
            if (resume_owned && same) {
                int ret = pet_reverse_notice_media_command(true);
                printf("[REVERSE_VOICE] cancel pause ret=%d\n", ret);
                view_hold = !ret;
                restore_at = now_ms;
                phase = ret ? NOTICE_IDLE : NOTICE_RESTORING;
                phase_at = now_ms;
                resume_owned = false;
            } else notice_finish(now_ms);
        }
        break;
    case NOTICE_SPEAKING:
        if (completed_generation == generation) {
            if (camera_active) {
                phase = NOTICE_WAIT_INTERVAL;
                phase_at = now_ms;
                printf("[REVERSE_VOICE] interval=%u ms\n", NOTICE_INTERVAL_MS);
            } else notice_finish(now_ms);
        }
        else if ((uint32_t)(now_ms - phase_at) >= NOTICE_TIMEOUT_MS) {
            printf("[REVERSE_VOICE] playback timeout\n");
            pet_reverse_notice_stop(generation);
            notice_finish(now_ms);
        }
        break;
    case NOTICE_WAIT_INTERVAL:
        if (!camera_active) notice_finish(now_ms);
        else if ((uint32_t)(now_ms - phase_at) >= NOTICE_INTERVAL_MS) {
            if (pet_reverse_notice_resource_ready()) notice_start(now_ms);
            else {
                /* 其他提示音正在播放时推迟本轮，不积压重复提示音。 */
                phase_at = now_ms;
                printf("[REVERSE_VOICE] repeat delayed: resource missing or tone busy\n");
            }
        }
        break;
    case NOTICE_RESTORING:
        if (playing || !same || (uint32_t)(now_ms - phase_at) >= RESTORE_TIMEOUT_MS) {
            phase = NOTICE_IDLE;
            if (restart_pending && camera_active) notice_begin(now_ms);
        }
        break;
    case NOTICE_IDLE:
        break;
    }
}

bool pet_reverse_notice_preserve_music_view(void)
{
    return phase != NOTICE_IDLE || view_hold;
}

#ifndef PET_REVERSE_NOTICE_TEST
static void notice_request_callback(void *priv)
{
    (void)priv;
    /* 过期的进入回调不能在 UI 已退出后重新启动循环。 */
    bool enter = requested_state == 1;
    /* 检测定时器未启动时不能暂停手机，否则没有后续恢复机会。 */
    if (enter && pet_dashboard_music_start() != 0) {
        printf("[REVERSE_VOICE] skipped: music timer unavailable\n");
        return;
    }
    pet_reverse_notice_handle(enter, sys_timer_get_ms());
}
#endif

int pet_reverse_notice_request(bool enter)
{
    requested_state = enter ? 1 : 0;
#ifndef PET_REVERSE_NOTICE_TEST
    int argv[3] = {(int)notice_request_callback, 1, enter ? 1 : 0};
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
    if (ret) printf("[REVERSE_VOICE] request enter=%u failed=%d\n", enter, ret);
    return ret;
#else
    (void)enter;
    return -1;
#endif
}
