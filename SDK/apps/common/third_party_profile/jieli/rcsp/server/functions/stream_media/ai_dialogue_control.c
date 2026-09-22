#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ai_dialogue_control.data.bss")
#pragma data_seg(".ai_dialogue_control.data")
#pragma const_seg(".ai_dialogue_control.text.const")
#pragma code_seg(".ai_dialogue_control.text")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_functions/rcsp_define.h"
#include "rcsp_manage.h"
#include "../mass_data/mass_data.h"
#include "ai_dialogue_control.h"
#include "JL_rcsp_protocol.h"
#include "btstack/avctp_user.h"
#include "rcsp_photo_notify.h"
#include "audio_video_control.h"
#include "video_api.h"
#include "translator/rcsp_translator.h"

#define LOG_TAG_CONST       RCSP
#define LOG_TAG             "[rcsp ai dialogue]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if RCSP_AI_DIALOGUE_ENABLE

#define RCSP_AI_OP_DIALOGUE                 0x01

#define RCSP_AI_FLAG_STATE_GET              0x00
#define RCSP_AI_FLAG_ENTER                  0x01
#define RCSP_AI_FLAG_EXIT                   0x02
#define RCSP_AI_FLAG_STATE_NOTIFY           0x03
#define RCSP_AI_FLAG_ACTION_STATE           0x04

#define RCSP_AI_ACTION_MASS_DATA            0x01
#define RCSP_AI_ACTION_LARGE_FILE           0x02
#define RCSP_AI_ACTION_CLOUD_PROCESSING      0x03

/* 非协议值: action_state() 用它表示「本次不需要发状态通知」 */
#define RCSP_AI_STATE_NO_CHANGE             0xff

#define RCSP_AI_RESULT_SUCCESS              0x00
#define RCSP_AI_RESULT_BUSY                 0x01
#define RCSP_AI_RESULT_UNSUPPORTED          0x02
#define RCSP_AI_RESULT_IN_PROGRESS          0x03

/*
 * 会话超时(秒)。flow_timer 只在 WAITING 状态倒计时, 计的是
 * "APP 完成下一步小动作"的窗口:
 *   - 语音对话: ENTER 应答 / 0x04 起录 / 0x05 停录, 都是秒级动作
 *   - 按键识图: 拍完照发 0x38 资源通知后, 等 APP 回 action=1 开始拉图
 * 识图那条窗口要等 BLE 把资源通知送到 APP 再回包, 给到 15s 留余量;
 * 云端处理(28s 量级)期间状态是 PROCESSING, 不参与倒计时。
 */
#define RCSP_AI_DEFAULT_TIMEOUT_SEC         15
/*
 * Cfg1 取值: bit0=下发识别文本, bit1=下发应答文本,
 *            bit2=需要应答 TTS, bit3=下发应答 TTS(0=A2DP 本地播, 1=走大数据下发)。
 * 当前阶段只要文本, 故 bit2/bit3 均不置位; 等要做语音播报时再打开 bit2。
 * SUPPORTED_CFG 同步收窄, 避免 APP 回包里的 TTS 位被采纳。
 */
#define RCSP_AI_DEFAULT_CFG                 0x03
#define RCSP_AI_SUPPORTED_CFG               0x03
#define RCSP_AI_CFG_TTS_REQUIRED            BIT(2)
#define RCSP_AI_CFG_TTS_TO_DEVICE           BIT(3)
#define RCSP_AI_TEXT_TYPE_RESPONSE          0x01
#define RCSP_AI_TEXT_LOG_MAX_BYTES          255
#define RCSP_AI_EXIT_USER_CANCEL            0x00
#define RCSP_AI_EXIT_TIMEOUT                0x01
#define RCSP_AI_EXIT_RECORD_FAILED          0x02
#define RCSP_AI_EXIT_RES_MISS               0x03
#define RCSP_AI_EXIT_SESSION_MISMATCH        0x07
#define RCSP_AI_COMMAND_TIMEOUT_MS          10000
#define RCSP_AI_RESTART_DELAY_MS            100

#define RCSP_AI_A2DP_EVENT_STOP             0
#define RCSP_AI_A2DP_EVENT_START            1
#define RCSP_AI_A2DP_EVENT_RECHECK          2

/* 会话模式：同一套 0x32 状态机承载两种触发来源 */
#define RCSP_AI_MODE_VOICE                  0   /* 场景 A：语音对话(拾音/播报) */
#define RCSP_AI_MODE_PHOTO                  1   /* 场景 B：按键识图(拍照/上传) */

struct rcsp_ai_dialogue_handle {
    u8 session_id;
    u8 state;
    u8 mode;                /* RCSP_AI_MODE_xxx */
    u8 cfg;
    u8 pending;
    u8 pending_flag;
    u8 active_action;
    u8 cancel_pending;
    u8 app_photo_pending;   /* APP 已请求拍照, 动作还在 app_core 队列里 */
    u8 restart_requested;
    u8 a2dp_tts_expected;
    u8 a2dp_tts_playing;
    u8 a2dp_stream_active;
    u16 timeout_sec;
    u16 command_timer;
    u16 command_generation;
    u16 flow_timer;
    u16 flow_generation;
    u16 restart_timer;
    u16 lifecycle_generation;
};

/* All state mutation runs in app_core: RCSP parsing is dispatched there by
 * JL_rcsp_recieve_resume(), link-state events and keys use the same task.
 * Stream callbacks only post events; timers explicitly target app_core.
 * Do not call the command/result/cleanup entry points from a stream callback. */
static struct rcsp_ai_dialogue_handle g_ai_dialogue;
static void rcsp_ai_dialogue_post_a2dp_event(u8 event);
#if !TCFG_AI_INTERACTION_ENABLE
static void rcsp_ai_dialogue_text_receive(void *priv, u8 *data, int len);
#endif

void rcsp_ai_dialogue_reset(void)
{
    u16 command_generation = g_ai_dialogue.command_generation + 1;
    u16 flow_generation = g_ai_dialogue.flow_generation + 1;
    u16 lifecycle_generation = g_ai_dialogue.lifecycle_generation + 1;

    if (g_ai_dialogue.command_timer) {
        sys_timeout_del(g_ai_dialogue.command_timer);
    }
    if (g_ai_dialogue.flow_timer) {
        sys_timeout_del(g_ai_dialogue.flow_timer);
    }
    if (g_ai_dialogue.restart_timer) {
        sys_timeout_del(g_ai_dialogue.restart_timer);
    }
    memset(&g_ai_dialogue, 0, sizeof(g_ai_dialogue));
    /* Invalidate callbacks already queued before disconnect/reset. */
    g_ai_dialogue.command_generation = command_generation;
    g_ai_dialogue.flow_generation = flow_generation;
    g_ai_dialogue.lifecycle_generation = lifecycle_generation;
#if !TCFG_AI_INTERACTION_ENABLE
    mass_data_recv_register(MASS_DATA_TYPE_AI_TEXT, NULL,
                            rcsp_ai_dialogue_text_receive);
#endif
}

void rcsp_ai_dialogue_disconnect(void)
{
    int record_stopped = rcsp_ai_dialogue_record_force_stop();

    /* The link is already down, so only clear local 0x30 receive state. */
    mass_data_recv_reset(0);
    rcsp_ai_dialogue_reset();
    log_i("disconnect cleanup, record_stopped:%d\n", record_stopped);
}

static void rcsp_ai_dialogue_interrupt_current(u8 restart);
static int rcsp_ai_dialogue_send_enter(void);
static int rcsp_ai_dialogue_request_exit(u8 reason);
static void rcsp_ai_dialogue_schedule_restart(void);

static void rcsp_ai_dialogue_command_timeout_in_app_core(void *priv)
{
    u8 pending_flag;
    u8 restart;
    u16 generation = (u16)(int)priv;

    if (!g_ai_dialogue.pending ||
        generation != g_ai_dialogue.command_generation) {
        return;
    }

    g_ai_dialogue.command_timer = 0;
    pending_flag = g_ai_dialogue.pending_flag;
    restart = g_ai_dialogue.restart_requested;
    g_ai_dialogue.pending = 0;
    log_e("command timeout, flag:%d, session:%d\n",
          pending_flag, g_ai_dialogue.session_id);

    if (pending_flag == RCSP_AI_FLAG_EXIT) {
        rcsp_ai_dialogue_interrupt_current(restart);
    } else if (pending_flag == RCSP_AI_FLAG_ENTER) {
        g_ai_dialogue.state = RCSP_AI_DIALOGUE_STATE_IDLE;
        if (restart) {
            rcsp_ai_dialogue_schedule_restart();
        }
    }
}

static int rcsp_ai_dialogue_start_command_timer(void)
{
    if (g_ai_dialogue.command_timer) {
        sys_timeout_del(g_ai_dialogue.command_timer);
    }
    g_ai_dialogue.command_generation++;
    g_ai_dialogue.command_timer = sys_timeout_add_2_task(
                                      (void *)(int)g_ai_dialogue.command_generation,
                                      rcsp_ai_dialogue_command_timeout_in_app_core,
                                      RCSP_AI_COMMAND_TIMEOUT_MS, "app_core");
    if (!g_ai_dialogue.command_timer) {
        log_e("command timer add failed\n");
        return false;
    }
    return true;
}

static void rcsp_ai_dialogue_stop_command_timer(void)
{
    if (g_ai_dialogue.command_timer) {
        sys_timeout_del(g_ai_dialogue.command_timer);
        g_ai_dialogue.command_timer = 0;
    }
    g_ai_dialogue.command_generation++;
}

static u8 rcsp_ai_dialogue_result_to_status(u8 result)
{
    switch (result) {
    case RCSP_AI_RESULT_SUCCESS:
        return JL_PRO_STATUS_SUCCESS;
    case RCSP_AI_RESULT_BUSY:
    case RCSP_AI_RESULT_IN_PROGRESS:
        return JL_PRO_STATUS_BUSY;
    default:
        return JL_PRO_STATUS_PARAM_ERR;
    }
}

static JL_ERR rcsp_ai_dialogue_notify_state(u8 session_id, u8 state)
{
    u8 data[4] = {
        RCSP_AI_OP_DIALOGUE,
        RCSP_AI_FLAG_STATE_NOTIFY,
        session_id,
        state,
    };

    log_i("notify state, session:%d, state:%d\n", session_id, state);
    return JL_CMD_send(JL_OPCODE_AI_DIAL, data, sizeof(data),
                       JL_NOT_NEED_RESPOND, 0, NULL);
}

static void rcsp_ai_dialogue_stop_flow_timer(void)
{
    if (g_ai_dialogue.flow_timer) {
        sys_timeout_del(g_ai_dialogue.flow_timer);
        g_ai_dialogue.flow_timer = 0;
    }
    g_ai_dialogue.flow_generation++;
}

static void rcsp_ai_dialogue_flow_timeout_in_app_core(void *priv)
{
    u16 generation = (u16)(int)priv;

    if (generation != g_ai_dialogue.flow_generation ||
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING) {
        return;
    }

    g_ai_dialogue.flow_timer = 0;
    log_e("flow timeout, session:%d, state:%d\n",
          g_ai_dialogue.session_id, g_ai_dialogue.state);
    rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_TIMEOUT);
}

static int rcsp_ai_dialogue_refresh_flow_timer(void)
{
    rcsp_ai_dialogue_stop_flow_timer();

    /* The timeout is the watchdog between two APP-controlled small turns. */
    if (!g_ai_dialogue.timeout_sec ||
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING) {
        return true;
    }

    g_ai_dialogue.flow_timer = sys_timeout_add_2_task(
                                   (void *)(int)g_ai_dialogue.flow_generation,
                                   rcsp_ai_dialogue_flow_timeout_in_app_core,
                                   (u32)g_ai_dialogue.timeout_sec * 1000,
                                   "app_core");
    if (!g_ai_dialogue.flow_timer) {
        log_e("flow timer add failed\n");
        return false;
    }
    return true;
}

/* 只改本地状态, 不向 APP 发状态通知 */
static void rcsp_ai_dialogue_set_local_state(u8 state)
{
    g_ai_dialogue.state = state;
    rcsp_ai_dialogue_refresh_flow_timer();
}

static void rcsp_ai_dialogue_set_state(u8 state)
{
    int watchdog_ready;

    g_ai_dialogue.state = state;
    watchdog_ready = rcsp_ai_dialogue_refresh_flow_timer();
    rcsp_ai_dialogue_notify_state(g_ai_dialogue.session_id, state);
    if (!watchdog_ready) {
        /* Fail closed rather than leave an unguarded WAITING session. */
        rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_USER_CANCEL);
    }
}

#if !TCFG_AI_INTERACTION_ENABLE
static void rcsp_ai_dialogue_print_text(u8 text_type,
                                        const u8 *text, u16 text_len)
{
    char text_buf[RCSP_AI_TEXT_LOG_MAX_BYTES + 1];
    u16 print_len = text_len;

    if (!text || !text_len) {
        return;
    }
    if (print_len > RCSP_AI_TEXT_LOG_MAX_BYTES) {
        print_len = RCSP_AI_TEXT_LOG_MAX_BYTES;
        /* Do not cut a UTF-8 character in the middle. */
        while (print_len && (text[print_len] & 0xc0) == 0x80) {
            print_len--;
        }
    }

    memcpy(text_buf, text, print_len);
    text_buf[print_len] = '\0';
    printf("[AI dialogue] text[%d]: %s%s\n", text_type, text_buf,
           print_len < text_len ? "...[truncated]" : "");
}

static void rcsp_ai_dialogue_text_receive(void *priv, u8 *data, int len)
{
    u8 version;
    u8 text_type;
    u16 payload_len;

    (void)priv;
    if (!data || len < 4) {
        log_e("AI text header invalid, len:%d\n", len);
        return;
    }

    version = data[0] & 0x0f;
    text_type = data[0] >> 4;
    payload_len = READ_BIG_U16(data + 2);
    if (version != 0 || payload_len != (len - 4)) {
        log_e("AI text format invalid, version:%d, payload:%d, len:%d\n",
              version, payload_len, len);
        return;
    }
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_PROCESSING ||
        g_ai_dialogue.active_action != RCSP_AI_ACTION_MASS_DATA) {
        log_i("drop inactive AI text, type:%d, len:%d\n",
              text_type, payload_len);
        return;
    }

    /* Headless glasses only validate/consume the text transport for now. */
    log_i("AI text received, type:%d, supplier:%d, len:%d\n",
          text_type, data[1], payload_len);
    rcsp_ai_dialogue_print_text(text_type, data + 4, payload_len);

    if (text_type == RCSP_AI_TEXT_TYPE_RESPONSE &&
        (g_ai_dialogue.cfg & RCSP_AI_CFG_TTS_REQUIRED) &&
        !(g_ai_dialogue.cfg & RCSP_AI_CFG_TTS_TO_DEVICE)) {
        g_ai_dialogue.a2dp_tts_expected = 1;
        log_i("wait APP A2DP TTS, session:%d\n",
              g_ai_dialogue.session_id);
        if (g_ai_dialogue.a2dp_stream_active) {
            log_i("A2DP already active, recheck AI TTS\n");
            rcsp_ai_dialogue_post_a2dp_event(
                RCSP_AI_A2DP_EVENT_RECHECK);
        }
    }
}
#endif

static void rcsp_ai_dialogue_a2dp_event_in_app_core(void *priv)
{
    u32 event = (u32)(int)priv;
    u16 generation = event >> 16;
    u8 session_id = (event >> 8) & 0xff;
    u8 event_type = event & 0xff;

    if (generation != g_ai_dialogue.lifecycle_generation ||
        !session_id || session_id != g_ai_dialogue.session_id ||
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE) {
        return;
    }

    if (event_type == RCSP_AI_A2DP_EVENT_START) {
        g_ai_dialogue.a2dp_stream_active = 1;
    } else if (event_type == RCSP_AI_A2DP_EVENT_STOP) {
        g_ai_dialogue.a2dp_stream_active = 0;
        if (!g_ai_dialogue.a2dp_tts_playing) {
            return;
        }

        g_ai_dialogue.a2dp_tts_playing = 0;
        g_ai_dialogue.a2dp_tts_expected = 0;
        if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_PLAYING) {
            rcsp_ai_dialogue_set_state(g_ai_dialogue.active_action ==
                                       RCSP_AI_ACTION_MASS_DATA ?
                                       RCSP_AI_DIALOGUE_STATE_PROCESSING :
                                       RCSP_AI_DIALOGUE_STATE_WAITING);
        }
        return;
    } else if (event_type != RCSP_AI_A2DP_EVENT_RECHECK) {
        return;
    }

    if (!g_ai_dialogue.a2dp_stream_active ||
        !g_ai_dialogue.a2dp_tts_expected ||
        !(g_ai_dialogue.cfg & RCSP_AI_CFG_TTS_REQUIRED) ||
        (g_ai_dialogue.cfg & RCSP_AI_CFG_TTS_TO_DEVICE)) {
        return;
    }
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING &&
        !(g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_PROCESSING &&
          g_ai_dialogue.active_action == RCSP_AI_ACTION_MASS_DATA)) {
        log_i("ignore A2DP TTS start, state:%d, action:%d\n",
              g_ai_dialogue.state, g_ai_dialogue.active_action);
        return;
    }

    g_ai_dialogue.a2dp_tts_playing = 1;
    g_ai_dialogue.a2dp_tts_expected = 0;
    if (g_ai_dialogue.active_action == RCSP_AI_ACTION_MASS_DATA) {
        /* action=1 end will publish PLAYING after the text is acknowledged. */
        return;
    }
    rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_PLAYING);
}

static void rcsp_ai_dialogue_post_a2dp_event(u8 event)
{
    int msg[3];
    u8 session_id = g_ai_dialogue.session_id;

    if (!session_id || g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE) {
        return;
    }

    msg[0] = (int)rcsp_ai_dialogue_a2dp_event_in_app_core;
    msg[1] = 1;
    msg[2] = ((u32)g_ai_dialogue.lifecycle_generation << 16) |
             ((u16)session_id << 8) | event;
    if (os_taskq_post_type("app_core", Q_CALLBACK, 3, msg)) {
        log_e("post A2DP TTS event failed, event:%d, session:%d\n",
              event, session_id);
    }
}

void rcsp_ai_dialogue_a2dp_play_start(void)
{
    rcsp_ai_dialogue_post_a2dp_event(RCSP_AI_A2DP_EVENT_START);
}

void rcsp_ai_dialogue_a2dp_play_stop(void)
{
    rcsp_ai_dialogue_post_a2dp_event(RCSP_AI_A2DP_EVENT_STOP);
}

static u8 rcsp_ai_dialogue_next_session(void)
{
    g_ai_dialogue.session_id++;
    if (g_ai_dialogue.session_id == 0) {
        g_ai_dialogue.session_id = 1;
    }
    return g_ai_dialogue.session_id;
}

static void rcsp_ai_dialogue_restart_timeout(void *priv)
{
    if ((u16)(int)priv != g_ai_dialogue.lifecycle_generation) {
        return;
    }
    g_ai_dialogue.restart_timer = 0;

    if (!g_ai_dialogue.restart_requested) {
        return;
    }
    if (!get_rcsp_connect_status() || g_ai_dialogue.pending ||
        g_ai_dialogue.cancel_pending ||
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        log_e("restart aborted, state:%d, pending:%d, cancel:%d\n",
              g_ai_dialogue.state, g_ai_dialogue.pending,
              g_ai_dialogue.cancel_pending);
        g_ai_dialogue.restart_requested = 0;
        return;
    }

    g_ai_dialogue.restart_requested = 0;
    log_i("restart delay complete\n");
    rcsp_ai_dialogue_send_enter();
}

static void rcsp_ai_dialogue_schedule_restart(void)
{
    if (!g_ai_dialogue.restart_requested || g_ai_dialogue.restart_timer ||
        g_ai_dialogue.pending || g_ai_dialogue.cancel_pending ||
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        return;
    }
    if (!get_rcsp_connect_status()) {
        g_ai_dialogue.restart_requested = 0;
        return;
    }

    g_ai_dialogue.restart_timer = sys_timeout_add_2_task(
                                      (void *)(int)g_ai_dialogue.lifecycle_generation,
                                      rcsp_ai_dialogue_restart_timeout,
                                      RCSP_AI_RESTART_DELAY_MS,
                                      "app_core");
    if (!g_ai_dialogue.restart_timer) {
        log_e("restart timer add failed\n");
        g_ai_dialogue.restart_requested = 0;
        return;
    }
    log_i("restart scheduled, delay:%dms\n", RCSP_AI_RESTART_DELAY_MS);
}

static void rcsp_ai_dialogue_cancel_complete(void *priv, int result)
{
    u8 restart;

    (void)priv;
    if (!g_ai_dialogue.cancel_pending) {
        return;
    }

    g_ai_dialogue.cancel_pending = 0;
    restart = g_ai_dialogue.restart_requested;
    log_i("old data cancel complete, result:%d, restart:%d\n",
          result, restart);
    if (restart) {
        rcsp_ai_dialogue_schedule_restart();
    }
}

static void rcsp_ai_dialogue_interrupt_current(u8 restart)
{
    int stop_result = true;
    u8 cancel_mass_data;

    if (restart) {
        g_ai_dialogue.restart_requested = 1;
    }

    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE) {
        if (!g_ai_dialogue.cancel_pending && g_ai_dialogue.restart_requested) {
            rcsp_ai_dialogue_schedule_restart();
        }
        return;
    }

    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_RECORDING) {
        stop_result = rcsp_ai_dialogue_record_force_stop();
    }

    cancel_mass_data =
        (g_ai_dialogue.active_action == RCSP_AI_ACTION_MASS_DATA);
    g_ai_dialogue.active_action = 0;
    g_ai_dialogue.a2dp_tts_expected = 0;
    g_ai_dialogue.a2dp_tts_playing = 0;
    g_ai_dialogue.a2dp_stream_active = 0;
    /* Every accepted EXIT completes with the old session's IDLE notification. */
    rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_IDLE);
    if (!stop_result) {
        log_e("interrupt record stop failed\n");
        g_ai_dialogue.restart_requested = 0;
    }

    if (g_ai_dialogue.cancel_pending) {
        return;
    }

    if (!cancel_mass_data) {
        if (g_ai_dialogue.restart_requested) {
            rcsp_ai_dialogue_schedule_restart();
        }
        return;
    }

    /* Only interrupt an active 0x30 download before starting a new session. */
    g_ai_dialogue.cancel_pending = 1;
    if (!mass_data_active_cancel_ex(0, NULL,
                                    rcsp_ai_dialogue_cancel_complete)) {
        g_ai_dialogue.cancel_pending = 0;
        mass_data_recv_reset(0);
        if (g_ai_dialogue.restart_requested) {
            rcsp_ai_dialogue_schedule_restart();
        }
    }
}

static int rcsp_ai_dialogue_request_exit(u8 reason)
{
    u8 data[3] = {
        RCSP_AI_OP_DIALOGUE,
        RCSP_AI_FLAG_EXIT,
        reason,
    };
    int err;

    if (g_ai_dialogue.pending ||
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE) {
        return false;
    }

    g_ai_dialogue.pending = 1;
    g_ai_dialogue.pending_flag = RCSP_AI_FLAG_EXIT;
    rcsp_ai_dialogue_stop_flow_timer();
    /* Reserve the watchdog before sending a command that needs a reply. */
    if (!rcsp_ai_dialogue_start_command_timer()) {
        g_ai_dialogue.pending = 0;
        g_ai_dialogue.restart_requested = 0;
        rcsp_ai_dialogue_interrupt_current(0);
        return false;
    }
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    err = JL_CMD_send(JL_OPCODE_AI_DIAL, data, sizeof(data),
                      JL_NEED_RESPOND, 0, NULL);
    if (err) {
        rcsp_ai_dialogue_stop_command_timer();
        g_ai_dialogue.pending = 0;
        log_e("exit send failed:%d, reason:%d\n", err, reason);
        rcsp_ai_dialogue_interrupt_current(0);
        return false;
    }

    log_i("exit sent, reason:%d\n", reason);
    return true;
}

static int rcsp_ai_dialogue_send_enter(void)
{
    u8 data[6];
    int err;

#if RCSP_TRANSLATOR_ENABLE
    if (rcsp_translator_is_busy()) {
        return false;
    }
#endif

    if (!get_rcsp_connect_status()) {
        log_e("enter rejected: rcsp disconnected\n");
        return false;
    }
    if (g_ai_dialogue.pending || g_ai_dialogue.cancel_pending ||
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        log_e("enter rejected: state:%d, pending:%d, cancel:%d\n",
              g_ai_dialogue.state, g_ai_dialogue.pending,
              g_ai_dialogue.cancel_pending);
        return false;
    }

    g_ai_dialogue.lifecycle_generation++;
    rcsp_ai_dialogue_next_session();
    g_ai_dialogue.timeout_sec = RCSP_AI_DEFAULT_TIMEOUT_SEC;
    g_ai_dialogue.cfg = RCSP_AI_DEFAULT_CFG;
    g_ai_dialogue.active_action = 0;
    g_ai_dialogue.a2dp_tts_expected = 0;
    g_ai_dialogue.a2dp_tts_playing = 0;
    g_ai_dialogue.a2dp_stream_active = 0;

    data[0] = RCSP_AI_OP_DIALOGUE;
    data[1] = RCSP_AI_FLAG_ENTER;
    data[2] = g_ai_dialogue.session_id;
    WRITE_BIG_U16(data + 3, g_ai_dialogue.timeout_sec);
    data[5] = g_ai_dialogue.cfg;

    g_ai_dialogue.pending = 1;
    g_ai_dialogue.pending_flag = RCSP_AI_FLAG_ENTER;
    if (!rcsp_ai_dialogue_start_command_timer()) {
        g_ai_dialogue.pending = 0;
        g_ai_dialogue.restart_requested = 0;
        return false;
    }
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    err = JL_CMD_send(JL_OPCODE_AI_DIAL, data, sizeof(data),
                      JL_NEED_RESPOND, 0, NULL);
    if (err) {
        rcsp_ai_dialogue_stop_command_timer();
        g_ai_dialogue.pending = 0;
        g_ai_dialogue.state = RCSP_AI_DIALOGUE_STATE_IDLE;
        log_e("enter send failed:%d, session:%d\n", err,
              g_ai_dialogue.session_id);
        return false;
    }

    log_i("key enter sent, session:%d, timeout:%d, cfg:0x%x\n",
          g_ai_dialogue.session_id, g_ai_dialogue.timeout_sec,
          g_ai_dialogue.cfg);
    return true;
}

/* ---------------- 识图拍照（场景 B） ---------------- */

static void rcsp_ai_dialogue_photo_done(int result, const char *path);

#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
static void rcsp_ai_dialogue_defer_photo(void *p)
{
    (void)p;
    /* 相机初始化/编码/写卡必须在 app_core 任务里做，不能在协议栈上下文 */
    camera_api_take_photo_ai(rcsp_ai_dialogue_photo_done);
}
#endif

static void rcsp_ai_dialogue_trigger_photo(void)
{
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    int msg[4] = {(int)rcsp_ai_dialogue_defer_photo, 1, 0};
    if (os_taskq_post_type("app_core", Q_CALLBACK, 3, msg)) {
        log_e("post AI photo failed\n");
        rcsp_ai_dialogue_photo_done(-1, NULL);
    }
#else
    /* 无相机能力：立即以失败收尾，避免会话挂到看门狗超时 */
    rcsp_ai_dialogue_photo_done(-1, NULL);
#endif
}

/*
 * 拍照结束回调（app_core 任务上下文）。
 * 成功：回报 WAITING + 0x38 资源通知，引导 APP 按 0x24 簇号拉图。
 * 失败：以 EXIT(资源缺失) 收尾，不让会话挂到看门狗超时。
 */
static void rcsp_ai_dialogue_photo_done(int result, const char *path)
{
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_CAPTURING) {
        /* 会话已被取消/退出，丢弃本次结果 */
        log_i("photo done but state:%d, drop\n", g_ai_dialogue.state);
        return;
    }

    if (result != 0 || !path) {
        log_e("AI photo failed, exit session\n");
        rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_RES_MISS);
        return;
    }

    /* 先回报 WAITING，再发资源通知。session_id 复用当前会话 ID，
     * 保证 APP 能把这张照片关联到本次对话。 */
    rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_WAITING);
    rcsp_photo_notify_resource(path, g_ai_dialogue.session_id);
}

/* ---------------- APP 请求拍照（0x38 op=0x01） ---------------- */

static void rcsp_ai_dialogue_app_photo_in_app_core(void *priv)
{
    u8 session_id = (u8)(int)priv;

    g_ai_dialogue.app_photo_pending = 0;

    /* 排到 app_core 之前会话可能已被取消/退出 */
    if (session_id != g_ai_dialogue.session_id ||
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE ||
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_CAPTURING) {
        log_i("drop app photo, session:%d->%d, state:%d\n",
              session_id, g_ai_dialogue.session_id, g_ai_dialogue.state);
        return;
    }

    log_i("app photo start, session:%d\n", session_id);
    /*
     * 拍照会顶掉正在进行的小动作。APP 判定出拍照意图后就不再走原流程,
     * 之前那条 action(通常是云端处理)不会再有结束通知, 若不清掉,
     * active_action 会一直挂着, 后续 APP 发的 action=2 大文件传输会被
     * 当成冲突回 BUSY, 触发无休止重试直到看门狗超时。
     */
    if (g_ai_dialogue.active_action) {
        log_i("drop stale action:%d for photo\n", g_ai_dialogue.active_action);
        g_ai_dialogue.active_action = 0;
    }
    rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_CAPTURING);
    rcsp_ai_dialogue_trigger_photo();
}

/*
 * 0x38 op=0x01 拍照请求(APP -> 设备)回调, 由协议栈同步调用。
 *
 * 这里只做校验并把动作排到 app_core: 相机初始化/编码/写卡不能在协议栈
 * 上下文做。状态切换也一并放到 app_core, 这样 0x0038 应答(协议栈同步发出)
 * 会先于 0x8032 CAPTURING 通知, 与 RCSP_AUDIO_VIDEO_RESPONSE_FIRST 的约定一致。
 *
 * 语音流程(场景 A)在 PROCESSING(云端语义理解)期间下发该请求, 识图流程
 * (场景 B)在 WAITING 期间下发, 两种状态都接受。
 */
static u8 rcsp_ai_dialogue_take_photo_request(u8 session_id, u8 device_number)
{
    int msg[3];

    (void)device_number;

    if (!get_rcsp_connect_status()) {
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE ||
        g_ai_dialogue.session_id == 0) {
        /* 会话已结束: 回「参数不支持」而不是「设备繁忙」, 避免 APP 反复重试 */
        log_e("app photo rejected: no session\n");
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    if (session_id != g_ai_dialogue.session_id) {
        log_e("app photo session mismatch: %d vs %d\n",
              session_id, g_ai_dialogue.session_id);
        return RCSP_AV_RESULT_UNSUPPORTED;
    }
    if (g_ai_dialogue.app_photo_pending ||
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_CAPTURING) {
        return RCSP_AV_RESULT_IN_PROGRESS;
    }
    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_UPLOADING ||
        g_ai_dialogue.active_action == RCSP_AI_ACTION_LARGE_FILE) {
        /* 上一张图还在回传, 不能同时占相机 */
        log_e("app photo rejected: upload busy\n");
        return RCSP_AV_RESULT_BUSY;
    }
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING &&
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_PROCESSING) {
        return RCSP_AV_RESULT_BUSY;
    }

    msg[0] = (int)rcsp_ai_dialogue_app_photo_in_app_core;
    msg[1] = 1;
    msg[2] = session_id;
    if (os_taskq_post_type("app_core", Q_CALLBACK, 3, msg)) {
        log_e("post app photo failed, session:%d\n", session_id);
        return RCSP_AV_RESULT_BUSY;
    }

    g_ai_dialogue.app_photo_pending = 1;
    return RCSP_AV_RESULT_SUCCESS;
}

static const struct rcsp_audio_video_ops rcsp_ai_dialogue_av_ops = {
    .take_photo = rcsp_ai_dialogue_take_photo_request,
};

static int rcsp_ai_dialogue_av_init(void)
{
    /* 与 rcsp_mix_record 的录音回调按字段合并, 互不覆盖 */
    rcsp_audio_video_register_ops(&rcsp_ai_dialogue_av_ops);
    return 0;
}

late_initcall(rcsp_ai_dialogue_av_init);

/*
 * 统一触发入口。mode 区分场景：语音对话 / 按键识图。
 * 两者共用同一套 0x32 会话状态机，只是进入会话后的动作不同。
 */
static int rcsp_ai_dialogue_trigger(u8 mode)
{
    if (!get_rcsp_connect_status()) {
        log_e("trigger rejected: rcsp disconnected\n");
        return false;
    }
    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_UPLOADING) {
        log_e("trigger rejected: large-file upload busy\n");
        return false;
    }

    /* 识图模式下会话已就绪：复用会话直接拍照，不重复 ENTER（协议要求） */
    if (mode == RCSP_AI_MODE_PHOTO &&
        g_ai_dialogue.mode == RCSP_AI_MODE_PHOTO &&
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_WAITING &&
        !g_ai_dialogue.active_action &&
        !g_ai_dialogue.pending && !g_ai_dialogue.cancel_pending) {
        log_i("reuse session for photo, session:%d\n", g_ai_dialogue.session_id);
        rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_CAPTURING);
        rcsp_ai_dialogue_trigger_photo();
        return true;
    }

    /* 记录本次期望的模式，供 send_enter / 延迟重启使用 */
    g_ai_dialogue.mode = mode;

    if (g_ai_dialogue.pending || g_ai_dialogue.cancel_pending ||
        g_ai_dialogue.restart_timer) {
        g_ai_dialogue.restart_requested = 1;
        log_i("trigger queued, pending:%d, cancel:%d, timer:%d\n",
              g_ai_dialogue.pending, g_ai_dialogue.cancel_pending,
              g_ai_dialogue.restart_timer);
        return true;
    }
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        /* 已有其它模式的会话：先退出，restart 时按新 mode 重新 ENTER */
        g_ai_dialogue.restart_requested = 1;
        return rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_USER_CANCEL);
    }
    return rcsp_ai_dialogue_send_enter();
}

int rcsp_ai_dialogue_key_trigger(void)
{
    return rcsp_ai_dialogue_trigger(RCSP_AI_MODE_VOICE);
}

int rcsp_ai_dialogue_photo_trigger(void)
{
    return rcsp_ai_dialogue_trigger(RCSP_AI_MODE_PHOTO);
}

static void rcsp_ai_dialogue_response(u8 OpCode, u8 OpCode_SN, u8 result,
                                      u8 *response_data, u16 response_len,
                                      u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 response[4];

    if (response_len > sizeof(response) - 1 ||
        (response_len && !response_data)) {
        log_e("invalid AI response length:%u\n", response_len);
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_PARAM_ERR, OpCode_SN,
                             NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }
    response[0] = result;
    if (response_len) {
        memcpy(response + 1, response_data, response_len);
    }
    JL_CMD_response_send(OpCode, rcsp_ai_dialogue_result_to_status(result),
                         OpCode_SN, response, response_len + 1,
                         ble_con_handle, spp_remote_addr);
}

static u8 rcsp_ai_dialogue_action_state(u8 action, u8 state, u8 *next_state)
{
    if (state > 1 || !next_state) {
        return RCSP_AI_RESULT_UNSUPPORTED;
    }
    if (g_ai_dialogue.pending || g_ai_dialogue.cancel_pending) {
        return RCSP_AI_RESULT_IN_PROGRESS;
    }
    if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE ||
        g_ai_dialogue.session_id == 0) {
        /* 会话已结束: 回「参数不支持」而不是「设备繁忙」。
         * result=0x01(繁忙) 的语义是「稍后重试」, APP 会以 ~550ms 周期
         * 无限重发 action 请求; 0x02 能让 APP 停止重试。 */
        return RCSP_AI_RESULT_UNSUPPORTED;
    }

    if (action != RCSP_AI_ACTION_MASS_DATA &&
        action != RCSP_AI_ACTION_LARGE_FILE &&
        action != RCSP_AI_ACTION_CLOUD_PROCESSING) {
        return RCSP_AI_RESULT_UNSUPPORTED;
    }

    if (state) {
        if (g_ai_dialogue.active_action) {
            if (g_ai_dialogue.active_action == action) {
                return RCSP_AI_RESULT_IN_PROGRESS;
            }
            /*
             * 大文件传输是照片上传的关键路径, 允许抢占其它动作。
             * APP 会在云端处理(识别文本的语义理解)尚未结束时就开始拉图,
             * 单槽模型若回 BUSY, APP 会以 ~600ms 周期重试到看门狗超时,
             * 照片永远传不出去。被抢掉的 action 不再有结束通知, 由这里
             * 直接覆盖即可。
             */
            if (action != RCSP_AI_ACTION_LARGE_FILE) {
                return RCSP_AI_RESULT_BUSY;
            }
            log_i("large file preempts action:%d\n",
                  g_ai_dialogue.active_action);
        }
        /*
         * 大文件传输在两种状态下都可能到来:
         *   WAITING    - 识图流程, 资源通知后 APP 直接拉图
         *   PROCESSING - 语音流程, APP 先起云端处理再拉图
         * 其余 action 只允许在 WAITING 下发。
         */
        if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING &&
            !(action == RCSP_AI_ACTION_LARGE_FILE &&
              g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_PROCESSING)) {
            return RCSP_AI_RESULT_BUSY;
        }

        g_ai_dialogue.active_action = action;
        if (action == RCSP_AI_ACTION_MASS_DATA) {
            g_ai_dialogue.a2dp_tts_expected = 0;
        }
        *next_state = (action == RCSP_AI_ACTION_LARGE_FILE) ?
                      RCSP_AI_DIALOGUE_STATE_UPLOADING :
                      RCSP_AI_DIALOGUE_STATE_PROCESSING;
    } else {
        if (g_ai_dialogue.active_action != action) {
            /*
             * 迟到的或被抢占的结束通知: 按幂等处理并回成功。
             * 回 0x03(进行中) 会让 APP 反复重发同一条结束通知,
             * 而设备侧已经没有对应动作可以收尾 —— 例如拍照抢占了
             * 云端处理, 它的结束通知随后才到。状态保持不变,
             * 由调用方比较 next_state 决定要不要发状态通知。
             */
            log_i("ignore stale action:%d end, active:%d\n",
                  action, g_ai_dialogue.active_action);
            /* 用非法状态值表示「不需要发状态通知」 */
            *next_state = RCSP_AI_STATE_NO_CHANGE;
            return RCSP_AI_RESULT_SUCCESS;
        }
        g_ai_dialogue.active_action = 0;
        *next_state = (action == RCSP_AI_ACTION_MASS_DATA &&
                       g_ai_dialogue.a2dp_tts_playing) ?
                      RCSP_AI_DIALOGUE_STATE_PLAYING :
                      RCSP_AI_DIALOGUE_STATE_WAITING;
    }
    return RCSP_AI_RESULT_SUCCESS;
}

void rcsp_ai_dialogue_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN,
                               u8 *data, u16 len, u16 ble_con_handle,
                               u8 *spp_remote_addr)
{
    u8 result = RCSP_AI_RESULT_SUCCESS;
    u8 response_data[2];
    u8 next_state = RCSP_AI_DIALOGUE_STATE_IDLE;
    u8 notify_state = 0;
    u8 cancel_after_response = 0;
    u16 response_len = 0;

    (void)priv;

    if (!data || len < 2 || data[0] != RCSP_AI_OP_DIALOGUE) {
#if TCFG_AI_INTERACTION_ENABLE
        if (data && len && data[0] == 0) {
            extern void put_aidial(void *priv, u8 OpCode, u8 OpCode_SN,
                                   u8 * data, u16 len);
            put_aidial(priv, OpCode, OpCode_SN, data, len);
            return;
        }
#endif
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_PARAM_ERR, OpCode_SN,
                             NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }

    log_i("rx, flag:%d, len:%d\n", data[1], len);
    switch (data[1]) {
    case RCSP_AI_FLAG_STATE_GET:
        if (len != 2) {
            result = RCSP_AI_RESULT_UNSUPPORTED;
            break;
        }
        response_data[0] = g_ai_dialogue.session_id;
        response_data[1] = g_ai_dialogue.state;
        response_len = 2;
        break;

    case RCSP_AI_FLAG_ENTER:
        if (len < 6 || data[2] == 0) {
            result = RCSP_AI_RESULT_UNSUPPORTED;
            break;
        }
#if RCSP_TRANSLATOR_ENABLE
        if (rcsp_translator_is_busy()) {
            result = RCSP_AI_RESULT_BUSY;
            break;
        }
#endif
        if (g_ai_dialogue.pending || g_ai_dialogue.cancel_pending) {
            result = RCSP_AI_RESULT_IN_PROGRESS;
            break;
        }
        if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
            result = RCSP_AI_RESULT_BUSY;
            break;
        }
        /* APP ENTER supersedes the queued local restart, including its intent. */
        if (g_ai_dialogue.restart_timer) {
            sys_timeout_del(g_ai_dialogue.restart_timer);
            g_ai_dialogue.restart_timer = 0;
        }
        g_ai_dialogue.restart_requested = 0;
        g_ai_dialogue.lifecycle_generation++;
        g_ai_dialogue.session_id = data[2];
        g_ai_dialogue.timeout_sec = READ_BIG_U16(data + 3);
        g_ai_dialogue.cfg = data[5] & RCSP_AI_SUPPORTED_CFG;
        /* APP 自己发起 ENTER 的是语音对话会话; 识图会话只能由本地按键建立 */
        g_ai_dialogue.mode = RCSP_AI_MODE_VOICE;
        g_ai_dialogue.active_action = 0;
        g_ai_dialogue.a2dp_tts_expected = 0;
        g_ai_dialogue.a2dp_tts_playing = 0;
        g_ai_dialogue.a2dp_stream_active = 0;
        response_data[0] = g_ai_dialogue.cfg;
        response_len = 1;
        break;

    case RCSP_AI_FLAG_EXIT:
        if (len != 3 || data[2] > RCSP_AI_EXIT_SESSION_MISMATCH) {
            result = RCSP_AI_RESULT_UNSUPPORTED;
            break;
        }
        if (g_ai_dialogue.pending || g_ai_dialogue.cancel_pending) {
            result = RCSP_AI_RESULT_IN_PROGRESS;
            break;
        }
        if (g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_UPLOADING) {
            result = RCSP_AI_RESULT_BUSY;
            break;
        }
        if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
            cancel_after_response = 1;
        }
        break;

    case RCSP_AI_FLAG_ACTION_STATE:
        if (len != 4) {
            result = RCSP_AI_RESULT_UNSUPPORTED;
            break;
        }
        result = rcsp_ai_dialogue_action_state(data[2], data[3],
                                               &next_state);
        if (result == RCSP_AI_RESULT_SUCCESS &&
            next_state != RCSP_AI_STATE_NO_CHANGE) {
            notify_state = 1;
        }
        break;

    default:
        result = RCSP_AI_RESULT_UNSUPPORTED;
        break;
    }

    rcsp_ai_dialogue_response(OpCode, OpCode_SN, result,
                              response_data, response_len,
                              ble_con_handle, spp_remote_addr);

    if (result != RCSP_AI_RESULT_SUCCESS) {
        return;
    }
    if (cancel_after_response) {
        rcsp_ai_dialogue_interrupt_current(0);
        return;
    }
    if (data[1] == RCSP_AI_FLAG_ENTER) {
        rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_WAITING);
    } else if (notify_state &&
               g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_CAPTURING) {
        /*
         * 拍照期间不让 action 的起止通知改写状态。
         * 语音流程里 APP 会紧跟着拍照请求补发 action=3 的结束通知,
         * 若照常切到 WAITING, photo_done 会因状态不是 CAPTURING 丢弃照片。
         * 状态由 photo_done 自己推到 WAITING 并发资源通知。
         */
        rcsp_ai_dialogue_set_state(next_state);
    }
    /* APP 主动接管会话时, 丢弃本地排队的重开会话请求 */
    if (g_ai_dialogue.restart_requested && !g_ai_dialogue.pending &&
        !g_ai_dialogue.cancel_pending &&
        g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        g_ai_dialogue.restart_requested = 0;
    }
}

void rcsp_ai_dialogue_cmd_response(u8 status, u8 *data, u16 len)
{
    u8 result;
    u8 pending_flag;
    u8 restart;

    if (!g_ai_dialogue.pending) {
        log_i("unexpected response, status:%d, len:%d\n", status, len);
        return;
    }

    rcsp_ai_dialogue_stop_command_timer();

    result = (data && len) ? data[0] : RCSP_AI_RESULT_UNSUPPORTED;
    pending_flag = g_ai_dialogue.pending_flag;
    restart = g_ai_dialogue.restart_requested;
    g_ai_dialogue.pending = 0;
    log_i("response, flag:%d, status:%d, result:%d, len:%d\n",
          pending_flag, status, result, len);

    if (pending_flag == RCSP_AI_FLAG_ENTER) {
        if (status == JL_PRO_STATUS_SUCCESS &&
            result == RCSP_AI_RESULT_SUCCESS) {
            if (len >= 2) {
                g_ai_dialogue.cfg = data[1] & RCSP_AI_SUPPORTED_CFG;
            }
            /*
             * APP 靠 0x8032 状态序列区分两条流程:
             *   语音: IDLE -> WAITING -> RECORDING -> ASR
             *   识图: IDLE -> CAPTURING -> WAITING -> 图片理解
             * 识图会话若先发 WAITING, APP 会当成语音流程并下发 0xC004 开录音,
             * 所以这里必须直接进 CAPTURING, 中间不插 WAITING。
             */
            if (restart) {
                /* 只改本地状态(不通知), 让 request_exit 能发出去;
                 * 若这里补发 WAITING, APP 可能据此起录音。 */
                rcsp_ai_dialogue_set_local_state(RCSP_AI_DIALOGUE_STATE_WAITING);
                rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_USER_CANCEL);
            } else if (g_ai_dialogue.mode == RCSP_AI_MODE_PHOTO) {
                rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_CAPTURING);
                rcsp_ai_dialogue_trigger_photo();
            } else {
                rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_WAITING);
            }
        } else {
            g_ai_dialogue.state = RCSP_AI_DIALOGUE_STATE_IDLE;
            if (restart) {
                rcsp_ai_dialogue_schedule_restart();
            }
        }
    } else if (pending_flag == RCSP_AI_FLAG_EXIT) {
        rcsp_ai_dialogue_interrupt_current(restart);
    }
}

int rcsp_ai_dialogue_is_busy(void)
{
    return g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE ||
           g_ai_dialogue.pending || g_ai_dialogue.cancel_pending ||
           g_ai_dialogue.restart_requested || g_ai_dialogue.restart_timer;
}

int rcsp_ai_dialogue_record_start_allowed(void)
{
    /* 识图会话不接受实时录音: 该流程走 0x38 资源通知 + 大文件传输,
     * 不经过 RECORDING。APP 若误发 0xC004 直接拒掉, 避免状态被劫持。 */
    if (g_ai_dialogue.mode == RCSP_AI_MODE_PHOTO) {
        return false;
    }
    return g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_IDLE ||
           g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_WAITING;
}

void rcsp_ai_dialogue_pause_a2dp_before_record(void)
{
    u8 a2dp_status;

    /* IDLE also permits the legacy 0x04 recording command. Only pause music
     * when this recording belongs to an active AI dialogue. */
    if (g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_WAITING) {
        return;
    }

    a2dp_status = bt_a2dp_get_status();
    if (a2dp_status != BT_MUSIC_STATUS_STARTING) {
        return;
    }

    if (bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL)) {
        log_e("pause A2DP before record failed\n");
    }
}

void rcsp_ai_dialogue_record_start_result(u8 success)
{
    if (success &&
        g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_WAITING) {
        g_ai_dialogue.a2dp_tts_expected = 0;
        g_ai_dialogue.a2dp_tts_playing = 0;
        rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_RECORDING);
    } else if (!success &&
               g_ai_dialogue.state != RCSP_AI_DIALOGUE_STATE_IDLE) {
        rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_RECORD_FAILED);
    }
}

void rcsp_ai_dialogue_record_stop_result(u8 success)
{
    if (success && g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_RECORDING) {
        /* Protocol 1.0.5: RECORDING ends by notifying WAITING. */
        rcsp_ai_dialogue_set_state(RCSP_AI_DIALOGUE_STATE_WAITING);
    } else if (!success &&
               g_ai_dialogue.state == RCSP_AI_DIALOGUE_STATE_RECORDING) {
        rcsp_ai_dialogue_request_exit(RCSP_AI_EXIT_RECORD_FAILED);
    }
}

#endif /* RCSP_AI_DIALOGUE_ENABLE */
