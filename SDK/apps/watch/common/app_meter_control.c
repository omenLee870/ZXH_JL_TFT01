/**
 * @file app_meter_control.c
 * @brief 在 app_core 与 UI 线程间协调仪表开关机、媒体退出及本机操作。
 * @note 协议入口仅登记请求；不在协议锁内操作屏幕、播放器或等待其他线程。
 *       开关机全部阶段完成后才确认成功；提交前可取消，开始收尾后必须完成或回滚。
 */
#ifndef APP_METER_CONTROL_TEST
#include "app_config.h"
#include "system/includes.h"
#include "app_tone.h"
#include "zxh_ble.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "audio_config.h"
#if TCFG_NETWORK_ENABLE && TCFG_NET_SCR_ENABLE
#include "screen_mirror.h"
#endif
#include "screen_mirror_bt_h264.h"
#endif
#include "app_meter_control.h"
#ifndef APP_METER_CONTROL_TEST
#include "meter_settings.h"
#endif
#include "app_vehicle.h"
#include <string.h>

extern int youyibao_sif_ui_meter_apply(int power_on, int light_on, int mode);
extern int ui_update_page_is_active(void);
extern void youyibao_sif_ui_update_wake(void);
enum meter_phase { METER_IDLE, METER_QUEUED, METER_WAIT_UI, METER_UI_APPLYING,
                   METER_WAIT_AUDIO, METER_SERVICES, METER_ROLLBACK, METER_DONE };
static OS_SEM meter_gate;
static volatile unsigned meter_initialized, meter_ui_online, meter_stopped;
static volatile unsigned short meter_timer;
static struct app_meter_state meter_state = {.power_on=1, .mode=1, .power_state=APP_METER_RUNNING};
static volatile int meter_power = APP_METER_RUNNING, meter_finder_tone;
static struct {
    enum meter_phase phase;
    struct zxh_request request;
    struct app_meter_state desired, before;
    void *client;
    uint32_t token, started;
    int result, cancelled, committed;
} meter_op;
static uint32_t meter_audio_sequence, meter_audio_play, meter_audio_at, meter_audio_duration;
static volatile uint32_t meter_audio_started, meter_audio_stopped;

/** @brief 播放线程只发布事件标记，禁止在播放器锁内回调 APP 协议或停止音频。 */
static int meter_audio_event(void *priv, enum stream_event event)
{
    uint32_t sequence = (uint32_t)(uintptr_t)priv;
    if (event == STREAM_EVENT_START) meter_audio_started = sequence;
    if (event == STREAM_EVENT_STOP) meter_audio_stopped = sequence;
    return 0;
}

/** @brief 停止本模块的寻车提示，不关闭其他来源的提示音或蓝牙音频。 */
static void meter_audio_stop(void)
{
    uint32_t sequence = meter_audio_play;
    meter_finder_tone = 0;
    meter_audio_play = 0;
    if (sequence) tone_player_stop_by_callback((void *)(uintptr_t)sequence, meter_audio_event);
    os_sem_pend(&meter_gate, 0); meter_state.finding = 0; os_sem_post(&meter_gate);
}

static int meter_audio_play_once(void)
{
    if (++meter_audio_sequence == 0) ++meter_audio_sequence;
    meter_audio_play = meter_audio_sequence;
    /* normal.sin 在当前中英文提示音配置中均已启用，避免引用未打包的音频资源。 */
    return play_tone_file_alone_callback_psram(get_tone_files()->normal,
        (void *)(uintptr_t)meter_audio_play, meter_audio_event);
}

/** @brief 锁内发布统一电源状态，过渡态不得继续提供实时数据。 */
static void meter_set_power(int state)
{
    meter_power = meter_state.power_state = state;
    meter_state.power_on = state == APP_METER_RUNNING;
    if (!meter_state.power_on) meter_state.light_on = 0;
}

/** @brief 未提交硬件变更时恢复完整原状态，避免失败后图标状态与实际显示不同。 */
static void meter_restore_power(void)
{
    meter_state = meter_op.before;
    meter_power = meter_state.power_state;
}

int app_meter_control_media_allowed(void)
{
    return !meter_initialized || meter_stopped || meter_power == APP_METER_RUNNING;
}

int app_meter_control_tone_allowed(void *priv, void *callback)
{
    return app_meter_control_media_allowed() || (meter_finder_tone &&
        priv == (void *)(uintptr_t)meter_audio_play && callback == (void *)meter_audio_event);
}

/**
 * @brief app_core 逆序停止声音和车辆服务，并保存音量与最终里程。
 * @return ZXH_OK 或 ZXH_EXECUTION_FAILED；存储失败不能伪报关机完成。
 * @note 锁外调用，保留 BLE、消息队列与 UI 控制桥用于后续开机。
 */
static int meter_services_off(void)
{
    int result = 0;
    printf("[METER_POWER] off audio begin t=%u\n", sys_timer_get_ms());
    meter_audio_stop();
    unsigned char address[6];
    tone_player_stop();
    if (a2dp_player_runing() && a2dp_player_get_btaddr(address)) a2dp_player_close(address);
    if (esco_player_runing()) esco_player_close();
    printf("[METER_POWER] off mirror begin t=%u\n", sys_timer_get_ms());
    screen_mirror_bt_h264_deinit();
#if TCFG_NETWORK_ENABLE && TCFG_NET_SCR_ENABLE
    printf("[METER_POWER] off network begin t=%u\n", sys_timer_get_ms());
    result = net_scr_shutdown();
#endif
    printf("[METER_POWER] off vehicle begin t=%u\n", sys_timer_get_ms());
    if (app_vehicle_suspend()) result = -1;
    /* 先停止一线通捕获中断，再写音量 VM，避免关机保存期间仍接收业务脉冲。 */
    printf("[METER_POWER] off volume begin t=%u\n", sys_timer_get_ms());
    app_audio_volume_save_now();
    printf("[METER_POWER] off services done result=%d t=%u\n", result, sys_timer_get_ms());
    return result ? ZXH_EXECUTION_FAILED : ZXH_OK;
}

/** @brief app_core 定时推进生命周期、音频与完成回包，不跨线程直接操作 UI。 */
static void meter_control_tick(void *priv)
{
    uint32_t now = sys_timer_get_ms(), completed = 0;
    int result = ZXH_OK, stop_audio = 0, begin_audio = 0, service_action = 0;
    (void)priv;
    if (!meter_initialized || meter_stopped) return;
    if (meter_audio_play && ui_update_page_is_active()) meter_audio_stop();
    if (meter_audio_play && (uint32_t)(now - meter_audio_at) >= meter_audio_duration)
        meter_audio_stop();
    os_sem_pend(&meter_gate, 0);
    if (meter_op.phase != METER_IDLE && meter_op.phase != METER_DONE &&
        meter_op.phase != METER_UI_APPLYING && !meter_op.committed &&
        (meter_op.cancelled || (uint32_t)(now - meter_op.started) >= 7000u)) {
        meter_op.result = meter_op.cancelled ? ZXH_AUTH_FAILED : ZXH_EXECUTION_TIMEOUT;
        stop_audio = meter_op.phase == METER_WAIT_AUDIO;
        meter_op.phase = METER_DONE;
        meter_restore_power();
    }
    if (meter_op.committed && meter_op.phase == METER_WAIT_UI &&
        (uint32_t)(now - meter_op.started) >= 7000u) {
        meter_op.result = ZXH_EXECUTION_TIMEOUT;
        meter_op.phase = METER_ROLLBACK;
    }
    if (meter_op.phase == METER_QUEUED) {
        if (meter_op.request.action == ZXH_FIND) {
            meter_op.phase = METER_WAIT_AUDIO; begin_audio = 1;
        } else {
            meter_op.desired = meter_state;
            switch (meter_op.request.action) {
            case ZXH_LOCK:
                if (meter_power == APP_METER_OFF) { meter_op.result = ZXH_OK; meter_op.phase = METER_DONE; break; }
                meter_op.desired.power_on = 0; meter_op.desired.light_on = 0;
                meter_set_power(APP_METER_STOPPING); meter_op.phase = METER_WAIT_UI; break;
            case ZXH_UNLOCK:
                if (meter_power == APP_METER_RUNNING) { meter_op.result = ZXH_OK; meter_op.phase = METER_DONE; break; }
                meter_set_power(APP_METER_STARTING); meter_op.desired.power_on = 1;
                meter_op.phase = METER_SERVICES; break;
            case ZXH_LIGHT_SET:
                meter_op.desired.light_on = meter_op.request.value;
                break;
            case ZXH_MODE_SET: meter_op.desired.mode = meter_op.request.value; break;
            default: break;
            }
            if (meter_op.phase == METER_QUEUED) meter_op.phase = METER_WAIT_UI;
        }
    }
    if (meter_op.phase == METER_SERVICES) {
        service_action = meter_op.request.action == ZXH_UNLOCK ? 1 : -1;
        meter_op.committed = 1;
    } else if (meter_op.phase == METER_ROLLBACK) service_action = -2;
    os_sem_post(&meter_gate);
    if (service_action) {
        printf("[METER_POWER] token=%u services=%d begin t=%u\n", meter_op.token, service_action, sys_timer_get_ms());
        int code = service_action == 1 ? (app_vehicle_resume() ? ZXH_EXECUTION_FAILED : ZXH_OK) : meter_services_off();
        printf("[METER_POWER] token=%u services=%d result=%d t=%u\n", meter_op.token, service_action, code, sys_timer_get_ms());
        if (service_action == 1 && code) meter_services_off();
        os_sem_pend(&meter_gate, 0);
        if (service_action == 1 && !code) meter_op.phase = METER_WAIT_UI;
        else {
            meter_set_power(code || service_action == -2 ? APP_METER_FAULT : APP_METER_OFF);
            if (service_action != -2) meter_op.result = code;
            meter_op.phase = METER_DONE;
        }
        os_sem_post(&meter_gate);
    }
    if (stop_audio) meter_audio_stop();
    if (begin_audio) {
        int code;
        if (meter_audio_play || tone_player_runing() || ui_update_page_is_active()) code = ZXH_BUSY;
        else {
            meter_audio_at = now; meter_audio_duration = meter_op.request.value;
            meter_finder_tone = 1;
            code = meter_audio_play_once() ? ZXH_EXECUTION_FAILED : ZXH_OK;
        }
        if (code) {
            if (code == ZXH_EXECUTION_FAILED) meter_audio_stop();
            os_sem_pend(&meter_gate, 0); meter_op.result = code;
            meter_op.phase = METER_DONE; os_sem_post(&meter_gate);
        }
    }
    os_sem_pend(&meter_gate, 0);
    if (meter_op.phase == METER_WAIT_AUDIO) {
        if (meter_audio_play && meter_audio_started == meter_audio_play) {
            meter_state.finding = 1; meter_op.result = ZXH_OK; meter_op.phase = METER_DONE;
        } else if (!meter_audio_play || meter_audio_stopped == meter_audio_play) {
            meter_op.result = ZXH_EXECUTION_FAILED; meter_op.phase = METER_DONE;
        }
    }
    if (meter_op.phase == METER_DONE) {
        completed = meter_op.token; result = meter_op.result;
        stop_audio = result == ZXH_OK && meter_op.request.action == ZXH_LOCK;
        memset(&meter_op, 0, sizeof(meter_op));
    } else stop_audio = 0;
    os_sem_post(&meter_gate);
    if (stop_audio) meter_audio_stop();
    if (completed) {
        printf("[METER] token=%u result=%d\n", completed, result);
        zxh_vehicle_complete(completed, result, result == ZXH_OK);
    }
    /* 短提示播完后在限定时间内重复；不抢占随后出现的电话、倒车等其他提示音。 */
    if (meter_audio_play && meter_audio_started == meter_audio_play &&
        meter_audio_stopped == meter_audio_play) {
        if (tone_player_runing() || meter_audio_play_once()) meter_audio_stop();
    }
}

/** @brief 只在 app_core 初始化一次，启动 50ms 检测；失败时不开放控制能力。 */
int app_meter_control_start(void)
{
    if (meter_stopped || strcmp(os_current_task(), "app_core")) return -1;
    if (!meter_initialized) {
        if (os_sem_create(&meter_gate, 1)) return -1;
        meter_initialized = 1;
    }
    #ifndef APP_METER_CONTROL_TEST
    if (meter_settings_init()) return -1;
    #endif
    if (!meter_timer) meter_timer = sys_timer_add(NULL, meter_control_tick, 50);
    return meter_timer ? 0 : -1;
}

uint32_t app_meter_control_actions(void)
{
    return meter_initialized && meter_timer && meter_ui_online && !meter_stopped ?
        (1u << ZXH_LOCK) | (1u << ZXH_UNLOCK) | (1u << ZXH_LIGHT_SET) |
        (1u << ZXH_FIND) | (1u << ZXH_MODE_SET) : 0;
}

int app_meter_control_snapshot(struct app_meter_state *state)
{
    if (!state || !app_meter_control_actions()) return ZXH_EXECUTION_FAILED;
    if (os_sem_pend(&meter_gate, -1)) return ZXH_BUSY;
    *state = meter_state; os_sem_post(&meter_gate); return ZXH_OK;
}

/** @brief 复制请求并立即返回；一次只执行一项本机操作，避免 UI 与音频相互覆盖。 */
int app_meter_control_submit(void *client, uint32_t token, const struct zxh_request *request)
{
    if (!request || !token || !app_meter_control_actions()) return ZXH_EXECUTION_FAILED;
    if (ui_update_page_is_active()) return ZXH_BUSY;
    if ((unsigned)request->action >= ZXH_ACTION_COUNT ||
        !(app_meter_control_actions() & (1u << request->action))) return ZXH_ACTION_ERROR;
    if ((request->action == ZXH_LIGHT_SET && (request->value < 0 || request->value > 1)) ||
        (request->action == ZXH_MODE_SET && (request->value < 0 || request->value > 2)) ||
        (request->action == ZXH_FIND && (request->value < 1000 || request->value > 10000))) return ZXH_PARAMETER_ERROR;
    if ((request->action == ZXH_LOCK || request->action == ZXH_UNLOCK) && request->value != 0) return ZXH_PARAMETER_ERROR;
    if (os_sem_pend(&meter_gate, -1)) return ZXH_BUSY;
    if (meter_op.phase != METER_IDLE) { os_sem_post(&meter_gate); return ZXH_BUSY; }
    if (meter_power != APP_METER_RUNNING &&
        (request->action == ZXH_LIGHT_SET || request->action == ZXH_MODE_SET)) {
        os_sem_post(&meter_gate); return ZXH_POWER_OFF;
    }
    meter_op.request = *request; meter_op.token = token; meter_op.client = client;
    meter_op.before = meter_state;
    meter_op.started = sys_timer_get_ms(); meter_op.phase = METER_QUEUED;
    os_sem_post(&meter_gate); return ZXH_PENDING;
}

/** @brief LVGL 线程执行真实显示操作，执行成功后才发布电源、图标及模式状态。 */
void app_meter_control_ui_poll(void)
{
    struct app_meter_state desired;
    uint32_t token;
    int result;
    if (!meter_initialized || !meter_timer || meter_stopped) return;
    meter_ui_online = 1;
    if (os_sem_pend(&meter_gate, -1)) return;
    if (meter_op.phase != METER_WAIT_UI || (!meter_op.committed && (meter_op.cancelled ||
        (uint32_t)(sys_timer_get_ms() - meter_op.started) >= 7000u))) {
        os_sem_post(&meter_gate); return;
    }
    token = meter_op.token; desired = meter_op.desired; meter_op.phase = METER_UI_APPLYING;
    os_sem_post(&meter_gate);
    printf("[METER_POWER] token=%u ui power=%d begin t=%u\n", token, desired.power_on, sys_timer_get_ms());
    result = youyibao_sif_ui_meter_apply(desired.power_on, desired.light_on, desired.mode);
    printf("[METER_POWER] token=%u ui result=%d t=%u\n", token, result, sys_timer_get_ms());
    os_sem_pend(&meter_gate, 0);
    if (meter_op.token == token && meter_op.phase == METER_UI_APPLYING) {
        if (!result && meter_op.request.action == ZXH_LOCK) {
            meter_op.committed = 1; meter_op.phase = METER_SERVICES;
        } else if (result && meter_op.request.action == ZXH_UNLOCK && meter_op.committed) {
            meter_op.result = result == -2 ? ZXH_BUSY : ZXH_EXECUTION_FAILED;
            meter_op.phase = METER_ROLLBACK;
        } else {
            if (!result) {
                meter_state.power_on = desired.power_on; meter_state.light_on = desired.light_on;
                meter_state.mode = desired.mode;
                if (meter_op.request.action == ZXH_UNLOCK) meter_set_power(APP_METER_RUNNING);
            } else meter_restore_power();
            meter_op.result = result == -2 ? ZXH_BUSY : result ? ZXH_EXECUTION_FAILED : ZXH_OK;
            meter_op.phase = METER_DONE;
        }
    }
    os_sem_post(&meter_gate);
}

void app_meter_control_disconnect(void *client)
{
    if (!meter_initialized || meter_stopped) return;
    os_sem_pend(&meter_gate, 0);
    if (meter_op.phase != METER_IDLE && meter_op.client == client) meter_op.cancelled = 1;
    os_sem_post(&meter_gate);
}

void app_meter_control_ui_wake_for_update(void)
{
    if (!meter_initialized || !meter_timer || meter_stopped) return;
    /* 升级只临时点亮专用进度页，不绕过业务开机顺序，也不改变逻辑电源状态。 */
    youyibao_sif_ui_update_wake();
}

/** @brief SDK 最终断电或重启前关闭唤醒控制桥；逻辑关机保留此桥以接收再次开机。 */
void app_meter_control_stop(void)
{
    if (!meter_initialized || meter_stopped) return;
    meter_stopped = 1;
    if (meter_timer) { sys_timer_del(meter_timer); meter_timer = 0; }
    meter_audio_stop();
}
