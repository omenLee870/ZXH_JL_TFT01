#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "app_config.h"
#include "system/includes.h"
#include "printf.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "generic/circular_buf.h"
#include "system/event.h"

#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "audio_config.h"
#include "pc_spk_player.h"
#include "pc_spk_file.h"
#include "pc_mic_recoder.h"
#include "app_msg.h"
#include "lbuf.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define UAC_SPEAKER_STREAM_DETECT_INTERVAL  2
#define UAC_SPEAKER_STREAM_DETECT_COUNT     5

typedef enum {
    SPK_STATE_CLOSED = 0,     // 资源未就绪，handler未注册
    SPK_STATE_READY,          // 资源就绪，handler已注册，等待数据触发启动
    SPK_STATE_RUNNING,        // 播放器已启动并持续喂数据
    SPK_STATE_DELAY_CLOSE,    // 延迟关闭窗口：允许补0，避免变调
} spk_state_t;

struct uac_spk_ctx_t {
    u8  *iso_packet_buf;           // ISO packet buffer（1ms 内最大包）
    u32  max_iso_packet_bytes;     // 每个 ISO packet 最大字节数
    u32  min_iso_packet_bytes;     // 每个 ISO packet 最小字节数

    u32  opened_timestamp;

    u32  samplerate_hz;            // 采样率（Hz）
    u8   num_channels;             // 声道数
    u8   bits_per_sample;          // 每声道采样位宽

    u8   state;                    // spk_state_t: CLOSED/READY/RUNNING/DELAY_CLOSE
    u8   stream_halt_count;

    u16  halt_check_timer_id;
    u16  close_delay_tid;
};

static struct uac_spk_ctx_t *g_spk = NULL;

/* ---------- SPK→MIC Loopback (USB data-path self-test) ---------- */
/* Uncomment or define UAC_SPK_MIC_LOOPBACK in build config to enable */
#define UAC_SPK_MIC_LOOPBACK 1
#if UAC_SPK_MIC_LOOPBACK
#define UAC_LOOPBACK_BUF_SIZE   768
#define UAC_LOOPBACK_DELAY_MS   0          /* MIC 回复延时(ms)，0=无延时 */
static cbuffer_t g_loopback_cbuf;
static u8 g_loopback_buf[UAC_LOOPBACK_BUF_SIZE] __attribute__((aligned(4)));
static u8 g_loopback_inited;

static void uac_loopback_init(void)
{
    if (!g_loopback_inited) {
        cbuf_init(&g_loopback_cbuf, g_loopback_buf, sizeof(g_loopback_buf));
        g_loopback_inited = 1;
        log_info("loopback cbuf init size=%u", UAC_LOOPBACK_BUF_SIZE);
    }
}

static void uac_loopback_spk_write(const u8 *data, u32 len)
{
    uac_loopback_init();
    u32 wlen = cbuf_write(&g_loopback_cbuf, (void *)data, len);
    if (wlen < len) {
        log_error("loopback overflow %u/%u", wlen, len);
    }
}

static u32 uac_loopback_mic_read(u8 *buf, u32 len)
{
    uac_loopback_init();

#if UAC_LOOPBACK_DELAY_MS > 0
    if (g_spk) {
        u32 bytes_per_ms = g_spk->samplerate_hz / 1000 *
                           g_spk->num_channels *
                           (g_spk->bits_per_sample / 8);
        u32 threshold = bytes_per_ms * UAC_LOOPBACK_DELAY_MS;
        if (g_loopback_cbuf.data_len < threshold + len) {
            memset(buf, 0, len);
            return len;
        }
    }
#endif

    u32 rlen = cbuf_read(&g_loopback_cbuf, buf, len);
    if (rlen < len) {
        memset(buf + rlen, 0, len - rlen);
    }
    return len;
}
#endif /* UAC_SPK_MIC_LOOPBACK */

static void uac_speaker_stream_halt_detect(void *priv);

int uac_get_spk_vol()
{
    int max_vol = app_audio_volume_max_query(AppVol_USB);
    int vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol * 100 / max_vol < 100) {
        return vol * 100 / max_vol;
    } else {
        return 99;
    }
    return 0;
}

void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol)
{
    static u32 last_spk_l_vol = (u32) - 1, last_spk_r_vol = (u32) - 1;
    static u32 last_mic_vol = (u32) - 1;

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
        pc_mic_set_volume_by_taskq(l_vol);
#endif
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;

        log_info(">> PC, r_vol:%d, l_vol:%d\n", r_vol, l_vol);
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        pcspk_set_volume_by_taskq();
#endif
        break;
    default:
        break;
    }
}

static const char *spk_state_name(u8 s)
{
    switch (s) {
    case SPK_STATE_CLOSED:
        return "CLOSED";
    case SPK_STATE_READY:
        return "READY";
    case SPK_STATE_RUNNING:
        return "RUNNING";
    case SPK_STATE_DELAY_CLOSE:
        return "DELAY_CLOSE";
    default:
        return "UNKNOWN";
    }
}

static void spk_state_set(u8 new_state, const char *reason, const char *func, int line)
{
    struct uac_spk_ctx_t *spk;
    u8 old_state;

    local_irq_disable();

    spk = g_spk;
    if (!spk) {
        local_irq_enable();

        log_error("(null) -> %s @%s:%d reason=%s",
                  spk_state_name(new_state), func, line,
                  reason ? reason : "N/A");
        return;
    }

    old_state = spk->state;

    if (old_state != new_state) {
        spk->state = new_state;
    }

    local_irq_enable();

    if (old_state != new_state) {
        log_info("%s -> %s @%s:%d reason=%s",
                 spk_state_name(old_state),
                 spk_state_name(new_state),
                 func, line,
                 reason ? reason : "N/A");
    } else {
        log_debug("%s -> %s (same) @%s:%d reason=%s",
                  spk_state_name(old_state),
                  spk_state_name(new_state),
                  func, line,
                  reason ? reason : "N/A");
    }
}
#define SPK_SET_STATE(_new, _reason)  spk_state_set((_new), (_reason), __func__, __LINE__)

static void spk_dbg_log_ctx(const char *tag)
{
    if (!g_spk) {
        log_debug("[%s] g_spk=NULL", tag);
        return;
    }

    log_debug("[%s] state=%s sr=%u ch=%u bits=%u max_pkt=%u halt_cnt=%u open_ts=%u t_halt=%u t_close=%u",
              tag,
              spk_state_name(g_spk->state),
              g_spk->samplerate_hz,
              g_spk->num_channels,
              g_spk->bits_per_sample,
              g_spk->max_iso_packet_bytes,
              g_spk->stream_halt_count,
              g_spk->opened_timestamp,
              g_spk->halt_check_timer_id,
              g_spk->close_delay_tid);
}

u8 uac_audio_is_24bit_in_4byte()
{
    return UAC_24BIT_IN_4BYTE;
}

static inline bool is_timeout(u32 start_ms, u32 timeout_ms)
{
    return (u32)jiffies_msec2offset(start_ms, jiffies_msec()) >= timeout_ms;
}

static void spk_player_start()
{
    if (!g_spk) {
        return;
    }

    log_info("player_start enter, cur_state=%s", spk_state_name(g_spk->state));


    if (g_spk->state == SPK_STATE_RUNNING) {
        // 已运行，先关再开以应用新参数（保险）
        log_debug("player already running, close before reopen");
#if !UAC_SPK_MIC_LOOPBACK
        app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
    }
#if UAC_SPK_MIC_LOOPBACK
    uac_loopback_init();
#else
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    pc_spk_set_fmt(g_spk->num_channels, g_spk->bits_per_sample, g_spk->samplerate_hz);
#endif
    app_send_message(APP_MSG_PC_AUDIO_PLAY_OPEN, 0);
#endif


    if (g_spk->halt_check_timer_id == 0) {
        g_spk->halt_check_timer_id = sys_hi_timer_add(NULL,
                                     uac_speaker_stream_halt_detect,
                                     UAC_SPEAKER_STREAM_DETECT_INTERVAL);
        log_info("halt_detect timer add id=%u interval=%dms",
                 g_spk->halt_check_timer_id, UAC_SPEAKER_STREAM_DETECT_INTERVAL);
    }

    g_spk->stream_halt_count = 0;
    SPK_SET_STATE(SPK_STATE_RUNNING, "player_start");
}


static void spk_player_stop()
{
    local_irq_disable();
    if (!g_spk) {
        local_irq_enable();
        return;
    }
    u8 state = g_spk->state;
    local_irq_enable();

    if (state == SPK_STATE_CLOSED || state == SPK_STATE_READY) {
        log_error("player_stop called in invalid state=%s", spk_state_name(state));
    }

#if !UAC_SPK_MIC_LOOPBACK
    app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
    SPK_SET_STATE(SPK_STATE_READY, "player_stop");
}

static void usb_sof_spk_write_callback(struct usb_device_t *usb_device,
                                       u32 ep, u32 frame)
{
    if (!g_spk) {
        return;
    }

    if (g_spk->state == SPK_STATE_CLOSED) {
        return;
    }

    const usb_dev usb_id = usb_device2id(usb_device);

    if (g_spk->state == SPK_STATE_READY && usb_g_data_size(usb_id, SPK_ISO_EP_OUT)) {
        log_debug("READY trigger start, frame=%u ep=%u", frame, ep);
        spk_player_start();
    }

    u8  *iso_packet_buf       = g_spk->iso_packet_buf;
    u32  max_iso_packet_bytes = g_spk->max_iso_packet_bytes;
    u32  data_valid           = 0;
    for (int i = 0; i < UAC_MAX_PKTS_PER_SOF; i++) {
        int packet_len = usb_g_iso_read(usb_id,
                                        SPK_ISO_EP_OUT,
                                        iso_packet_buf,
                                        max_iso_packet_bytes,
                                        0);
        if (packet_len < g_spk->min_iso_packet_bytes) {
            break;
        }

        data_valid = 1;
#if UAC_SPK_MIC_LOOPBACK
        uac_loopback_spk_write(iso_packet_buf, packet_len);
#elif TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        pc_spk_data_isr_cb(iso_packet_buf, packet_len);
#endif
    }

    if (!data_valid) {
        if (g_spk->state == SPK_STATE_DELAY_CLOSE) {
            memset(iso_packet_buf, 0, max_iso_packet_bytes);
#if UAC_SPK_MIC_LOOPBACK
            uac_loopback_spk_write(iso_packet_buf, max_iso_packet_bytes);
#elif TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
            pc_spk_data_isr_cb(iso_packet_buf, max_iso_packet_bytes);
#endif
            g_spk->stream_halt_count = 0;
            log_debug("no data in DELAY_CLOSE, pad zero %u bytes", max_iso_packet_bytes);
        } else {
#if UAC_SPK_MIC_LOOPBACK
            /* no downstream to notify */
#elif TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
            pc_spk_data_isr_cb(NULL, 0);
#endif
        }
    } else {
        if (g_spk->stream_halt_count > 2) {
            log_debug("data resumed, halt_count %u -> 0", g_spk->stream_halt_count);
        }
        g_spk->stream_halt_count = 0;
    }
}


void uac_speaker_stream_write(const u8 *obuf, u32 len)
{
    /* 当前 UAC speaker 数据由 SOF 回调路径驱动（usb_sof_spk_write_callback）
     * 此接口保留不用，避免误导：不要在任务上下文直接喂数据
     */
    (void)obuf;
    (void)len;
}

static void uac_speaker_stream_halt_detect(void *priv)
{
    struct uac_spk_ctx_t *spk;
    struct uac_spk_ctx_t bk;

    (void)priv;

    local_irq_disable();

    spk = g_spk;
    if (!spk) {
        local_irq_enable();
        return;
    }

    bk = *spk;

    local_irq_enable();

    if (bk.state != SPK_STATE_RUNNING) {
        return;
    }

    bk.stream_halt_count++;

    if (bk.stream_halt_count >= UAC_SPEAKER_STREAM_DETECT_COUNT) {
        log_error("stream halt detected, stop player");
        spk_player_stop();
    }

    local_irq_disable();

    if (g_spk == spk && g_spk) {
        g_spk->stream_halt_count = bk.stream_halt_count;
    }

    local_irq_enable();
}

void uac_speaker_stream_get_volume(u16 *l_vol, u16 *r_vol)
{
    uac_get_cur_vol(0, l_vol, r_vol);
}

u8 uac_speaker_stream_status(void)
{
    return g_spk != NULL;
}

static int uac_speaker_alloc_iso_buffer(void)
{
    u32 bits_per_sample = g_spk->bits_per_sample;

#if UAC_24BIT_IN_4BYTE
    if (g_spk->bits_per_sample == 24) {
        bits_per_sample = 32;
    }
#endif

    u32 bytes_per_sample_frame   = g_spk->num_channels * (bits_per_sample / 8);
    u32 min_sample_frames_per_ms = g_spk->samplerate_hz / 1000;
    u32 max_sample_frames_per_ms = (g_spk->samplerate_hz + 999) / 1000;

    g_spk->min_iso_packet_bytes = bytes_per_sample_frame * min_sample_frames_per_ms;
    g_spk->max_iso_packet_bytes = bytes_per_sample_frame * max_sample_frames_per_ms;

    log_info("max_iso_packet_bytes:%d num_channels:%d samplerate_hz:%d bits_per_sample:%d",
             g_spk->max_iso_packet_bytes, g_spk->num_channels,
             g_spk->samplerate_hz, g_spk->bits_per_sample);

    // 分配 buffer，如果已经分配则释放重分配
    if (g_spk->iso_packet_buf) {
        free(g_spk->iso_packet_buf);
        g_spk->iso_packet_buf = NULL;
    }

    g_spk->iso_packet_buf = malloc(g_spk->max_iso_packet_bytes);
    assert_d(g_spk->iso_packet_buf != NULL);

    return 0;
}

// 配置采样率/声道/位宽，可在流已经打开时动态调用
static void uac_speaker_configure(u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    int need_update = 0;

    g_spk->opened_timestamp = jiffies_msec();

    if (g_spk->samplerate_hz != samplerate_hz ||
        g_spk->num_channels  != num_channels ||
        g_spk->bits_per_sample != bits_per_sample) {

        log_info("change %u/%u/%u -> %u/%u/%u",
                 g_spk->samplerate_hz, g_spk->num_channels, g_spk->bits_per_sample,
                 samplerate_hz, num_channels, bits_per_sample);

        g_spk->samplerate_hz    = samplerate_hz;
        g_spk->num_channels     = (u8)num_channels;
        g_spk->bits_per_sample  = (u8)bits_per_sample;
        need_update = 1;
    }

    if (need_update == 0) {
        log_debug("unchanged");
        return;
    }

    uac_speaker_alloc_iso_buffer();
}


void uac_speaker_stream_open(u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    if (g_spk) {
        log_info("speaker already opened, change config");

        if (g_spk->close_delay_tid) {
            log_info("[SPK] cancel pending close_delay tid=%u because reopen", g_spk->close_delay_tid);
            sys_hi_timeout_del(g_spk->close_delay_tid);
            g_spk->close_delay_tid = 0;
        }

        spk_dbg_log_ctx("open_reconfig_before");

        u8 old_state = g_spk->state;
        uac_speaker_configure(samplerate_hz, num_channels, bits_per_sample);

        if (old_state == SPK_STATE_RUNNING) {
            spk_player_stop();
        } else if (old_state == SPK_STATE_DELAY_CLOSE) {
            SPK_SET_STATE(SPK_STATE_READY, "reopen during delay_close");
        } else {
            SPK_SET_STATE(SPK_STATE_READY, "reconfig keep armed");
        }

        spk_dbg_log_ctx("open_reconfig_after");
        return;
    }

    g_spk = zalloc(sizeof(struct uac_spk_ctx_t));
    assert_d(g_spk != NULL);


    g_spk->state = SPK_STATE_CLOSED;
    uac_speaker_configure(samplerate_hz, num_channels, bits_per_sample);

    SPK_SET_STATE(SPK_STATE_READY, "stream_open init ready");

    usb_g_set_sof_hander(0, SPK_ISO_EP_OUT, usb_sof_spk_write_callback);
    spk_dbg_log_ctx("open_done");
}


static void uac_speaker_stream_close_delay(void *priv)
{
    if (!g_spk) {
        return;
    }

    int release = (int)priv;

    log_info("close_delay fire, release=%d, state=%s",
             release, spk_state_name(g_spk->state));

    g_spk->close_delay_tid = 0;

    usb_g_set_sof_hander(0, SPK_ISO_EP_OUT, NULL);

    if (g_spk->halt_check_timer_id) {
        sys_hi_timer_del(g_spk->halt_check_timer_id);
        log_info("halt_detect timer del id=%u", g_spk->halt_check_timer_id);
        g_spk->halt_check_timer_id = 0;
    }


    if (g_spk->state == SPK_STATE_RUNNING ||
        g_spk->state == SPK_STATE_DELAY_CLOSE) {
#if UAC_SPK_MIC_LOOPBACK
        /* skip player close in loopback mode */
#else
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        if (release) {
            pc_spk_player_close();
        } else {
            pcspk_close_player_by_taskq();
        }
#endif
        app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
    }

    if (g_spk->iso_packet_buf) {
        free(g_spk->iso_packet_buf);
        g_spk->iso_packet_buf = NULL;
    }

    SPK_SET_STATE(SPK_STATE_CLOSED, "close_delay finalize free ctx");

    free(g_spk);
    g_spk = NULL;
}


void uac_speaker_stream_close(int release)
{
    if (!g_spk) {
        return;
    }

    log_info("close request release=%d, state=%s ts=%d",
             release, spk_state_name(g_spk->state), jiffies_msec());

    if (is_timeout(g_spk->opened_timestamp, 150) || release) {
        if (g_spk->close_delay_tid) {
            sys_hi_timeout_del(g_spk->close_delay_tid);
            g_spk->close_delay_tid = 0;
        }
        uac_speaker_stream_close_delay((void *)release);
    } else {
        if (g_spk->state == SPK_STATE_RUNNING ||
            g_spk->state == SPK_STATE_READY) {
            SPK_SET_STATE(SPK_STATE_DELAY_CLOSE, "close request within holdoff window");
        }

        if (g_spk->close_delay_tid == 0) {
            g_spk->close_delay_tid = sys_hi_timeout_add(NULL,
                                     uac_speaker_stream_close_delay, 150);
            log_info("close_delay timer add id=%u timeout=150ms", g_spk->close_delay_tid);
        } else {
            sys_hi_timeout_modify(g_spk->close_delay_tid, 150);
            log_debug("close_delay timer modify id=%u timeout=150ms", g_spk->close_delay_tid);
        }
    }
}




struct uac_mic_handle {
    int (*tx_handler)(void *, void *, int);
    void *tx_handler_priv;
    u32  opened_timestamp;
    u32  samplerate_hz;            // 采样率（Hz）
    u8   num_channels;             // 声道数
    u8   bits_per_sample;          // 每声道采样位宽
    u16  close_delay_tid;
};

static struct uac_mic_handle *g_mic;

int uac_mic_stream_read(u8 *buf, u32 len)
{
#if UAC_SPK_MIC_LOOPBACK
    return uac_loopback_mic_read(buf, len);
#endif
    if (g_mic == NULL || (g_mic->tx_handler == NULL)) {
        memset(buf, 0, len);
        return len;
    }
    return g_mic->tx_handler(g_mic->tx_handler_priv, buf, len);
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(void *, void *, int))
{
    if (g_mic == NULL) {
        return ;
    }
    g_mic->tx_handler = tx_handler;
    g_mic->tx_handler_priv = priv;
}

void uac_mic_stream_get_volume(u16 *vol)
{
    *vol = uac_get_mic_vol(0);
}

u8 uac_get_mic_stream_status(void)
{
    return g_mic != NULL;
}

static void uac_mic_stream_restart(u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    int need_update = 0;

    g_mic->opened_timestamp = jiffies_msec();

    if (g_mic->samplerate_hz != samplerate_hz ||
        g_mic->num_channels != num_channels ||
        g_mic->bits_per_sample != bits_per_sample) {

        log_info("change %u/%u/%u -> %u/%u/%u",
                 g_mic->samplerate_hz, g_mic->num_channels, g_mic->bits_per_sample,
                 samplerate_hz, num_channels, bits_per_sample);

        g_mic->samplerate_hz   = samplerate_hz;
        g_mic->num_channels    = num_channels;
        g_mic->bits_per_sample = bits_per_sample;
        need_update = 1;
    }

    if (!need_update) {
        return;
    }

    g_mic->tx_handler      = NULL;
#if UAC_SPK_MIC_LOOPBACK
    /* skip mic recorder in loopback mode */
#elif TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    pc_mic_set_fmt(num_channels, bits_per_sample, samplerate_hz);
    pc_mic_recoder_restart_by_taskq();
#endif
}

// 打开 mic_stream
void uac_mic_stream_open(u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    if (g_mic) {
        log_info("Mic stream already opened, change config");
        if (g_mic->close_delay_tid) {
            log_info("[MIC] cancel pending close_delay tid=%u because reopen", g_mic->close_delay_tid);
            sys_hi_timeout_del(g_mic->close_delay_tid);
            g_mic->close_delay_tid = 0;
        }
        uac_mic_stream_restart(samplerate_hz, num_channels, bits_per_sample);

        return;
    }

    g_mic = zalloc(sizeof(struct uac_mic_handle));

    assert_d(g_mic != NULL);

    g_mic->tx_handler      = NULL;
#if UAC_SPK_MIC_LOOPBACK
    uac_loopback_init();
#elif TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    pc_mic_set_fmt(num_channels, bits_per_sample, samplerate_hz);
    pc_mic_recoder_open_by_taskq();
#endif

}

static void uac_mic_stream_close_delay(void *priv)
{
    int release = (u8)priv;
    g_mic->close_delay_tid = 0;

    log_info("## Func:uac_mic_stream_close_delay, Line:%d, Close Mic!!\n", __LINE__);
#if UAC_SPK_MIC_LOOPBACK
    /* skip mic recorder close in loopback mode */
#elif TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    if (release) {
        pc_mic_recoder_close();
    } else {
        pc_mic_recoder_close_by_taskq();
    }
#endif
    free(g_mic);
    g_mic = NULL;
}

void uac_mic_stream_close(int release)
{
    if (g_mic == NULL) {
        return ;
    }
    //频繁开关mic，导致出现audio或者蓝牙工作异常，
    //收到mic关闭命令后延时150ms再发消息通知audio模块执行关闭动作
    //如果在150ms之内继续收到usb下发的关闭命令，则继续推迟150ms。
    if (is_timeout(g_mic->opened_timestamp, 150) || release) {
        if (g_mic->close_delay_tid) {
            sys_hi_timeout_del(g_mic->close_delay_tid);
            g_mic->close_delay_tid = 0;
        }
        uac_mic_stream_close_delay((void *)release);
    } else {
        if (g_mic->close_delay_tid == 0) {
            g_mic->close_delay_tid = sys_hi_timeout_add(NULL, uac_mic_stream_close_delay, 150);
        } else {
            sys_hi_timeout_modify(g_mic->close_delay_tid, 150);
        }
    }
}


