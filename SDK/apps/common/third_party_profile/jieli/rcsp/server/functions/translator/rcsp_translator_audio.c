/* Translation audio: SDK microphone uplink + bounded AI_RX OPUS.
 * Runtime resource operations and the service timer run in app_core.
 * The stream callback only consumes the SPSC ring or records an event.
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_translator_audio.data.bss")
#pragma data_seg(".rcsp_translator_audio.data")
#pragma const_seg(".rcsp_translator_audio.text.const")
#pragma code_seg(".rcsp_translator_audio.text")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_functions/rcsp_define.h"
#include "rcsp_manage.h"
#include "JL_rcsp_protocol.h"
#include "app_protocol_common.h"
#include "ai_rx_player.h"
#include "media_config.h"
#include "btstack/avctp_user.h"
#include "rcsp_translator.h"
#include "rcsp_translator_audio.h"

#if RCSP_MODE && RCSP_TRANSLATOR_ENABLE

#define TRANSLATOR_CACHE_SIZE       8192
#define TRANSLATOR_PLAYER_SOURCE    0xe0 /* Internal ID, not a wire Source. */
#define TRANSLATOR_SERVICE_MS       20
#define TRANSLATOR_STALL_MS         20000

enum translator_record_request {
    TRANSLATOR_RECORD_NONE,
    TRANSLATOR_RECORD_SEND_START,
    TRANSLATOR_RECORD_WAIT_START,
    TRANSLATOR_RECORD_START_READY,
    TRANSLATOR_RECORD_SEND_STOP,
    TRANSLATOR_RECORD_WAIT_STOP,
    TRANSLATOR_RECORD_STOP_READY,
};

static struct {
    cbuffer_t cache;
    u8 *buffer;
    struct stream_node *volatile node;
    u32 generation;
    u32 play_generation;
    u32 last_rx_ms;
    volatile u32 last_progress_ms;
    u32 cache_wait_ms;
    u16 timer;
    u16 packet_len;
    u16 ble_handle;
    u8 spp_addr[6];
    u8 spp_addr_valid;
    u8 mode;
    u8 playing;
    volatile u8 closing;
    u8 notify_pending; /* BIT(source): each waiting MIC source needs op5. */
    u8 a2dp_suspended;
    volatile u8 stream_failed;
    volatile u8 stream_stopped;
    u8 record_failed;
    u8 record_request;
    u8 record_stop_requested;
    u32 record_request_ms;
} g_audio;

static int translator_source_supported(u8 source)
{
    /* Protocol 1.4.3.4 and SDK source_to_ch_remap: both MIC sources share
     * the first playback cache. Source describes origin, not mode access.
     * Keep unsupported file/A2DP/eSCO audio paths closed. */
    return (source == RCSP_TRANSLATOR_SOURCE_DEVICE_MIC ||
            source == RCSP_TRANSLATOR_SOURCE_PHONE_MIC) &&
           (g_audio.mode == RCSP_TRANSLATOR_MODE_RECORD_TRANSLATION ||
            g_audio.mode == RCSP_TRANSLATOR_MODE_FACE_TO_FACE ||
            g_audio.mode == RCSP_TRANSLATOR_MODE_SIMULTANEOUS);
}

static void translator_record_service(u32 now)
{
    u8 param[3] = {RCSP_TRANSLATOR_ENCODE_OPUS, 0x10, 0};
    u8 opcode;
    if (!g_audio.record_request) {
        return;
    }
    if ((u32)(now - g_audio.record_request_ms) >= TRANSLATOR_STALL_MS) {
        rcsp_translator_abort();
        return;
    }
    if (g_audio.record_request == TRANSLATOR_RECORD_START_READY) {
        if (g_audio.record_stop_requested) {
            /* Key released before APP accepted start: never open MIC. */
            g_audio.record_request = TRANSLATOR_RECORD_SEND_STOP;
            g_audio.record_request_ms = now;
        } else {
            g_audio.record_request = TRANSLATOR_RECORD_NONE;
            if (rcsp_translator_record_start(g_audio.ble_handle,
                                             g_audio.spp_addr_valid ?
                                             g_audio.spp_addr : NULL)) {
                rcsp_translator_abort();
            }
            return;
        }
    }
    if (g_audio.record_request == TRANSLATOR_RECORD_STOP_READY) {
        g_audio.record_request = TRANSLATOR_RECORD_NONE;
        g_audio.record_stop_requested = 0;
        return;
    }
    if (g_audio.record_request != TRANSLATOR_RECORD_SEND_START &&
        g_audio.record_request != TRANSLATOR_RECORD_SEND_STOP) {
        return;
    }
    opcode = g_audio.record_request == TRANSLATOR_RECORD_SEND_START ?
             JL_OPCODE_APP_RECORDING : JL_OPCODE_APP_RECORD_END;
    if (opcode == JL_OPCODE_APP_RECORD_END) {
        param[0] = 0; /* Normal release. */
    }
    if (!JL_CMD_send(opcode, param, opcode == JL_OPCODE_APP_RECORDING ? 3 : 1,
                     JL_NEED_RESPOND, g_audio.ble_handle,
                     g_audio.spp_addr_valid ? g_audio.spp_addr : NULL)) {
        g_audio.record_request = opcode == JL_OPCODE_APP_RECORDING ?
                                 TRANSLATOR_RECORD_WAIT_START :
                                 TRANSLATOR_RECORD_WAIT_STOP;
    }
}

static enum stream_node_state translator_rx_frame(void *priv,
        struct stream_frame **pframe)
{
    struct ai_rx_file_handle *file = priv;
    struct stream_frame *frame;
    u16 len = g_audio.packet_len;

    *pframe = NULL;
    if (!file || !file->node) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    g_audio.node = file->node;
    if (g_audio.closing || !g_audio.buffer) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    if (cbuf_get_data_len(&g_audio.cache) < len) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    frame = jlstream_get_frame(file->node->oport, len);
    if (!frame) {
        return NODE_STA_RUN;
    }
    frame->len = cbuf_read(&g_audio.cache, frame->data, len);
    if (frame->len != len) {
        /* Short read would hand the CBR decoder a truncated packet. */
        jlstream_free_frame(frame);
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    g_audio.last_progress_ms = sys_timer_get_ms();
    *pframe = frame;
    return NODE_STA_RUN;
}

static void translator_player_event(void *priv, int event)
{
    if ((u32)priv != g_audio.play_generation || g_audio.closing) {
        return;
    }
    if (event == STREAM_EVENT_PREEMPTED ||
        event == STREAM_EVENT_NEGOTIATE_FAILD) {
        g_audio.stream_failed = 1;
    } else if (event == STREAM_EVENT_STOP) {
        g_audio.stream_stopped = 1;
    }
}

static void translator_stop_player(void)
{
    if (!g_audio.playing) {
        return;
    }
    g_audio.closing = 1;
    g_audio.play_generation++;
    ai_rx_player_set_event_callback(TRANSLATOR_PLAYER_SOURCE, NULL, NULL);
    ai_rx_player_close(TRANSLATOR_PLAYER_SOURCE);
    g_audio.node = NULL;
    g_audio.playing = 0;
    g_audio.closing = 0;
    if (g_audio.a2dp_suspended) {
        g_audio.a2dp_suspended = 0;
        if (!ai_mic_is_busy()) {
            a2dp_tws_dec_resume();
        }
    }
}

static void translator_audio_service(void *priv)
{
    u32 used;
    u32 now = sys_timer_get_ms();
    u8 notification[7] = {RCSP_TRANSLATOR_OP_NOTIFY_CACHE, 5};

    if ((u32)priv != g_audio.generation || !g_audio.mode) {
        return;
    }
    if (!get_rcsp_connect_status()) {
        rcsp_translator_disconnect();
        return;
    }
    rcsp_translator_record_poll_events();
    if (!g_audio.mode) {
        return;
    }
    if (bt_get_call_status() != BT_CALL_HANGUP || g_audio.stream_failed ||
        g_audio.record_failed) {
        rcsp_translator_abort();
        return;
    }
    translator_record_service(now);
    if (!g_audio.mode) {
        return;
    }
    if (!g_audio.buffer) {
        return;
    }
    used = cbuf_get_data_len(&g_audio.cache);
    if (g_audio.playing && !g_audio.node &&
        (u32)(now - g_audio.last_progress_ms) >= TRANSLATOR_STALL_MS) {
        rcsp_translator_abort();
        return;
    }
    if (!g_audio.playing && used &&
        (u32)(now - g_audio.last_rx_ms) >= TRANSLATOR_STALL_MS) {
        rcsp_translator_abort();
        return;
    }
    if (g_audio.stream_stopped) {
        rcsp_translator_abort();
        return;
    }
    if (g_audio.playing && g_audio.node) {
        if (used >= g_audio.packet_len) {
            if ((u32)(now - g_audio.last_progress_ms) >= TRANSLATOR_STALL_MS) {
                rcsp_translator_abort();
                return;
            }
            jlstream_wakeup_thread(NULL, g_audio.node, NULL);
        } else if ((u32)(now - g_audio.last_rx_ms) >= TRANSLATOR_STALL_MS) {
            if (used) {
                rcsp_translator_abort();
                return;
            }
            /* Count ends a transport block, not the TTS stream. Wait for
             * further blocks; release only an idle, empty player without
             * exiting translation or stopping the simultaneous MIC. */
            if ((u32)(now - g_audio.last_progress_ms) >= TRANSLATOR_STALL_MS) {
                translator_stop_player();
            }
        }
    }
    if (g_audio.notify_pending &&
        used <= TRANSLATOR_CACHE_SIZE * 2 / 5) {
        u32 free_size = TRANSLATOR_CACHE_SIZE - used;
        notification[3] = free_size >> 24;
        notification[4] = free_size >> 16;
        notification[5] = free_size >> 8;
        notification[6] = free_size;
        /* At most two sends per service tick, never wait synchronously. */
        for (u8 source = RCSP_TRANSLATOR_SOURCE_DEVICE_MIC;
             source <= RCSP_TRANSLATOR_SOURCE_PHONE_MIC; source++) {
            if (!(g_audio.notify_pending & BIT(source))) {
                continue;
            }
            notification[2] = source;
            if (!JL_CMD_send(JL_OPCODE_TRANSLATOR, notification, sizeof(notification),
                             JL_NOT_NEED_RESPOND, g_audio.ble_handle,
                             g_audio.spp_addr_valid ? g_audio.spp_addr : NULL)) {
                g_audio.notify_pending &= ~BIT(source);
            } else if ((u32)(now - g_audio.cache_wait_ms) >= TRANSLATOR_STALL_MS) {
                rcsp_translator_abort();
                return;
            }
        }
    }
}

void rcsp_translator_audio_set_link(u16 handle, const u8 *addr)
{
    g_audio.ble_handle = handle;
    g_audio.spp_addr_valid = addr != NULL;
    memset(g_audio.spp_addr, 0, sizeof(g_audio.spp_addr));
    if (addr) {
        memcpy(g_audio.spp_addr, addr, sizeof(g_audio.spp_addr));
    }
    if (g_audio.mode == RCSP_TRANSLATOR_MODE_SIMULTANEOUS) {
        /* Timer sends 0x04 after the SET_MODE response has been queued. */
        g_audio.record_request = TRANSLATOR_RECORD_SEND_START;
        g_audio.record_request_ms = sys_timer_get_ms();
    }
}

int rcsp_translator_audio_request_record(u8 start)
{
    if (g_audio.mode != RCSP_TRANSLATOR_MODE_FACE_TO_FACE ||
        !get_rcsp_connect_status()) {
        return -EINVAL;
    }
    if (start) {
        if (g_audio.record_request || rcsp_realtime_record_is_busy() ||
            ai_mic_is_busy()) {
            return -EBUSY;
        }
        g_audio.record_stop_requested = 0;
        g_audio.record_request = TRANSLATOR_RECORD_SEND_START;
    } else {
        g_audio.record_stop_requested = 1;
        if (g_audio.record_request == TRANSLATOR_RECORD_SEND_START) {
            g_audio.record_request = TRANSLATOR_RECORD_NONE;
            g_audio.record_stop_requested = 0;
            return 0;
        }
        if (g_audio.record_request) {
            return 0; /* Finish the in-flight start before sending stop. */
        }
        if (!rcsp_realtime_record_is_busy()) {
            g_audio.record_stop_requested = 0;
            return 0;
        }
        if (!rcsp_translator_record_force_stop()) {
            rcsp_translator_abort();
            return -EFAULT;
        }
        g_audio.record_request = TRANSLATOR_RECORD_SEND_STOP;
    }
    g_audio.record_request_ms = sys_timer_get_ms();
    return 0;
}

void rcsp_translator_audio_cancel_record_request(void)
{
    /* An APP 0x04/0x05 takes precedence over a pending device request. */
    g_audio.record_request = TRANSLATOR_RECORD_NONE;
    g_audio.record_stop_requested = 0;
}

void rcsp_translator_audio_cmd_response(u8 opcode, u8 status, u16 handle,
                                        const u8 *addr)
{
    if (!g_audio.mode || handle != g_audio.ble_handle ||
        (addr != NULL) != g_audio.spp_addr_valid ||
        (addr && memcmp(addr, g_audio.spp_addr, sizeof(g_audio.spp_addr)))) {
        return;
    }
    if (!((opcode == JL_OPCODE_APP_RECORDING &&
           g_audio.record_request == TRANSLATOR_RECORD_WAIT_START) ||
          (opcode == JL_OPCODE_APP_RECORD_END &&
           g_audio.record_request == TRANSLATOR_RECORD_WAIT_STOP))) {
        return;
    }
    if (status != JL_PRO_STATUS_SUCCESS) {
        g_audio.record_failed = 1;
    } else {
        g_audio.record_request = opcode == JL_OPCODE_APP_RECORDING ?
                                 TRANSLATOR_RECORD_START_READY :
                                 TRANSLATOR_RECORD_STOP_READY;
    }
}

int rcsp_translator_audio_prepare(u8 mode)
{
    int error = -ENOMEM;
    if (g_audio.mode) {
        return -EBUSY;
    }
    g_audio.generation++;
    g_audio.mode = mode;
    if (mode == RCSP_TRANSLATOR_MODE_RECORD_TRANSLATION ||
        mode == RCSP_TRANSLATOR_MODE_FACE_TO_FACE ||
        mode == RCSP_TRANSLATOR_MODE_SIMULTANEOUS) {
#if TCFG_AI_RX_NODE_ENABLE
        /* Match the existing decoder's fixed CBR packet configuration. */
        if (CONFIG_OGG_OPUS_DEC_SUPPORT) {
            if (CONFIG_OGG_OPUS_DEC_FORMAT_MODE != OPUS_PACK_HEADER_NULL ||
                CONFIG_OGG_OPUS_DEC_SET_CBR_PACKET_LEN != 40) {
                error = -EINVAL;
                goto fail;
            }
            g_audio.packet_len = CONFIG_OGG_OPUS_DEC_SET_CBR_PACKET_LEN;
        } else {
            if (OPUS_SRINDEX != FRAME_LEN_40) {
                error = -EINVAL;
                goto fail;
            }
            g_audio.packet_len = 40; /* OPUS 16 kbps, 20 ms. */
        }
        g_audio.buffer = malloc(TRANSLATOR_CACHE_SIZE);
        if (!g_audio.buffer) {
            goto fail;
        }
        cbuf_init(&g_audio.cache, g_audio.buffer, TRANSLATOR_CACHE_SIZE);
#else
        goto fail;
#endif
    }
    g_audio.timer = sys_timer_add((void *)g_audio.generation,
                                  translator_audio_service, TRANSLATOR_SERVICE_MS);
    if (!g_audio.timer) {
        goto fail;
    }
    return 0;

fail:
    rcsp_translator_audio_close();
    return error;
}

void rcsp_translator_audio_close(void)
{
    if (g_audio.timer) {
        sys_timer_del(g_audio.timer);
        g_audio.timer = 0;
    }
    translator_stop_player(); /* joins the stream before freeing its ring */
    if (g_audio.buffer) {
        /* Clear first: a late rx_frame must see NULL, not a freed pointer. */
        u8 *buffer = g_audio.buffer;
        g_audio.buffer = NULL;
        free(buffer);
    }
    g_audio.generation++;
    g_audio.mode = 0;
    g_audio.notify_pending = 0;
    g_audio.stream_failed = 0;
    g_audio.stream_stopped = 0;
    g_audio.record_failed = 0;
    g_audio.record_request = TRANSLATOR_RECORD_NONE;
    g_audio.record_stop_requested = 0;
    g_audio.record_request_ms = 0;
    g_audio.node = NULL;
    g_audio.packet_len = 0;
}

u32 rcsp_translator_audio_cache(u8 source)
{
    u32 free_size;
    if (!g_audio.buffer || !translator_source_supported(source)) {
        return 0;
    }
    free_size = TRANSLATOR_CACHE_SIZE - cbuf_get_data_len(&g_audio.cache);
    if (free_size < TRANSLATOR_CACHE_SIZE * 3 / 5) {
        if (!g_audio.notify_pending) {
            g_audio.cache_wait_ms = sys_timer_get_ms();
        }
        g_audio.notify_pending |= BIT(source);
    }
    return free_size;
}

int rcsp_translator_audio_receive(u8 source, const u8 *data, u16 len, u8 count)
{
    struct ai_rx_player_param param = {0};
    (void)count;
    if (!g_audio.buffer || !translator_source_supported(source) ||
        (len && !data)) {
        return -EINVAL;
    }
    if (len > TRANSLATOR_CACHE_SIZE - cbuf_get_data_len(&g_audio.cache)) {
        rcsp_translator_abort();
        return -ENOMEM;
    }
    if (len && cbuf_get_data_len(&g_audio.cache) < g_audio.packet_len) {
        /* Time spent waiting for APP data is not decoder stall time. */
        g_audio.last_progress_ms = sys_timer_get_ms();
    }
    if (len && cbuf_write(&g_audio.cache, (void *)data, len) != len) {
        rcsp_translator_abort();
        return -ENOMEM;
    }
    g_audio.last_rx_ms = sys_timer_get_ms();
    if (!g_audio.playing && cbuf_get_data_len(&g_audio.cache) >= g_audio.packet_len) {
        if (ai_rx_player_any_running()) {
            rcsp_translator_abort();
            return -EBUSY;
        }
        if (!ai_mic_is_busy()) {
            if (a2dp_tws_dec_suspend(NULL)) {
                rcsp_translator_abort();
                return -EBUSY;
            }
            g_audio.a2dp_suspended = 1;
        }
        param.type = AI_SERVICE_VOICE;
        param.coding_type = AUDIO_CODING_OPUS;
        param.sample_rate = 16000;
        param.bit_rate = 16000;
        param.channel_mode = AUDIO_CH_MIX;
        param.frame_dms = 200;
        param.packet_len = g_audio.packet_len;
        param.format_mode = OPUS_PACK_HEADER_NULL;
        g_audio.last_progress_ms = sys_timer_get_ms();
        g_audio.play_generation++;
        /* No BT clock reference: translated audio is fed by the APP, so pass
         * NULL like the other ai_rx callers instead of a zero address. */
        if (ai_rx_player_open(NULL, TRANSLATOR_PLAYER_SOURCE,
                              &param, translator_rx_frame)) {
            if (g_audio.a2dp_suspended) {
                g_audio.a2dp_suspended = 0;
                a2dp_tws_dec_resume();
            }
            rcsp_translator_abort();
            return -EFAULT;
        }
        g_audio.playing = 1;
        ai_rx_player_set_event_callback(TRANSLATOR_PLAYER_SOURCE,
                                        translator_player_event,
                                        (void *)g_audio.play_generation);
    }
    return 0;
}

void rcsp_translator_audio_record_result(u8 success)
{
    if (!success) {
        g_audio.record_failed = 1;
        rcsp_translator_abort();
    }
}

#endif
