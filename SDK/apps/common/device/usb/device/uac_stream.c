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
#if CONFIG_USB_ESD_RECOVERY
#include "usb/device/usb_suspend_resume.h"
#include "usb/otg.h"
#endif
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
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define UAC_SPEAKER_STREAM_DETECT_INTERVAL  2
#define UAC_SPEAKER_STREAM_DETECT_COUNT     5

/* ------------------------------------------------------------------ */
/* 音频层多路 stream 适配：
 * 通过 UAC_AUDIO_STREAM_ID_SUPPORT 选择音频层接口是否支持 stream_id：
 *   =1：音频层接口带 uac_id/stream_id（pc_spk_set_fmt、
 *       pc_spk_data_isr_cb、pcspk_set_volume_by_taskq），直接透传
 *       stream_id，按真实 stream 分路；
 *   =0：音频层仅单路，接口不带 stream_id，忽略 stream_id 按
 *       stream 0 降级调用（旧音频层兼容）。
 * 当前音频层已支持 id，默认开启；构建配置中定义该宏为 0 即可回退。  */
#ifndef UAC_AUDIO_STREAM_ID_SUPPORT
#define UAC_AUDIO_STREAM_ID_SUPPORT  0
#endif
/* ------------------------------------------------------------------ */
static inline int uac_spk_set_volume_by_taskq(u8 stream_id)
{
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#if UAC_AUDIO_STREAM_ID_SUPPORT
    return pcspk_set_volume_by_taskq(stream_id);
#else
    (void)stream_id;
    return pcspk_set_volume_by_taskq();
#endif
#else
    (void)stream_id;
    return 0;
#endif
}

static inline void uac_spk_set_fmt(u8 stream_id, u8 channel, u8 bit, u32 sample_rate)
{
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#if UAC_AUDIO_STREAM_ID_SUPPORT
    pc_spk_set_fmt(stream_id, channel, bit, sample_rate);
#else
    (void)stream_id;
    pc_spk_set_fmt(channel, bit, sample_rate);
#endif
#endif
}

static inline void uac_spk_data_isr_cb(u8 stream_id, void *buf, u32 len)
{
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#if UAC_AUDIO_STREAM_ID_SUPPORT
    pc_spk_data_isr_cb(stream_id, buf, len);
#else
    (void)stream_id;
    pc_spk_data_isr_cb(buf, len);
#endif
#endif
}

static inline void uac_player_close(u32 release)
{
#if UAC_SPK_MIC_LOOPBACK
    /* skip player close in loopback mode */
    (void)release;
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

typedef enum {
    SPK_STATE_CLOSED = 0,     // 资源未就绪，handler未注册
    SPK_STATE_READY,          // 资源就绪，handler已注册，等待数据触发启动
    SPK_STATE_RUNNING,        // 播放器已启动并持续喂数据
    SPK_STATE_DELAY_CLOSE,    // 延迟关闭窗口：允许补0，避免变调
} spk_state_t;

struct uac_spk_ctx_t {
    u8   stream_id;                    /* stream identifier */
    u8   ep_out;                       /* ISO OUT EP number */
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

    u8   fill_stream_always_keep;      /* keep stream open even on alt setting 0 */
};

static struct uac_spk_ctx_t *g_spk[UAC_MAX_STREAMS];

#if CONFIG_USB_ESD_RECOVERY
static struct {
    u8   active;                       /* any stream has received data */
    u8   recovery_active;
    u16  recovery_timer_id;
    u16  reconnect_timer_id;
    u32  last_reconnect_time;
} g_esd;
#endif

/* ---------- SPK→MIC Loopback (USB data-path self-test) ---------- */
/* Uncomment or define UAC_SPK_MIC_LOOPBACK in build config to enable */
#define UAC_SPK_MIC_LOOPBACK 0
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
        log_warn("loopback overflow %u/%u", wlen, len);
    }
}

static u32 uac_loopback_mic_read(u8 *buf, u32 len)
{
    uac_loopback_init();

#if UAC_LOOPBACK_DELAY_MS > 0
    if (g_spk[0]) {
        u32 bytes_per_ms = g_spk[0]->samplerate_hz / 1000 *
                           g_spk[0]->num_channels *
                           (g_spk[0]->bits_per_sample / 8);
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

#if CONFIG_USB_ESD_RECOVERY
/* ---------- ESD (Electrostatic Discharge) Recovery ---------- */

static void esd_recovery_timer_cb(void *priv);
static void esd_reconnect_timer_cb(void *priv);

static void esd_clear_recovery(void)
{
    local_irq_disable();
    if (!g_esd.recovery_active) {
        local_irq_enable();
        return;
    }

    g_esd.recovery_active = 0;
    u16 tid = g_esd.recovery_timer_id;
    g_esd.recovery_timer_id = 0;
    local_irq_enable();

    if (tid) {
        sys_hi_timeout_del(tid);
    }

    log_info("[ESD] recovery success -- Iso OUT data received");
}

static void esd_start_recovery(const char *reason)
{
    /* 调用者已持有 irq_disable */
    if (!g_esd.active) {
        return;
    }

    if (g_esd.recovery_active) {
        /* 已在恢复监控中，刷新3s timer */
        if (g_esd.recovery_timer_id) {
            sys_hi_timeout_modify(g_esd.recovery_timer_id, 3000);
        }
        log_info("[ESD] %s: already monitoring, refresh timer", reason);
        return;
    }

    /* 30s冷却检查 */
    if (g_esd.last_reconnect_time != 0) {
        if (jiffies_msec2offset(g_esd.last_reconnect_time, jiffies_msec()) < 30000) {
            log_info("[ESD] %s: cooldown active, skip", reason);
            return;
        }
    }

    g_esd.recovery_active = 1;
    g_esd.recovery_timer_id = sys_hi_timeout_add(NULL, esd_recovery_timer_cb, 3000);
    log_info("[ESD] %s: start 3s recovery monitor, timer=%u", reason, g_esd.recovery_timer_id);
}

static void esd_suspend_status_cb(const usb_dev usb_id, usb_slave_status status)
{
    if (status != USB_SLAVE_SUSPEND) {
        return;
    }

    local_irq_disable();
    {
        int any = 0;
        for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
            if (g_spk[i]) {
                any = 1;
                break;
            }
        }
        if (!any) {
            local_irq_enable();
            return;
        }
    }
    esd_start_recovery("suspend");
    local_irq_enable();
}

void uac_speaker_notify_reset(void)
{
    local_irq_disable();
    {
        int any = 0;
        for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
            if (g_spk[i]) {
                any = 1;
                break;
            }
        }
        if (!any) {
            local_irq_enable();
            return;
        }
    }
    esd_start_recovery("reset");
    local_irq_enable();
}

static void esd_recovery_timer_cb(void *priv)
{
    (void)priv;

    local_irq_disable();

    g_esd.recovery_timer_id = 0;

    /* 如果恢复已被清除（数据刚好在timer触发前到达），放弃 */
    if (!g_esd.recovery_active) {
        local_irq_enable();
        log_info("[ESD] recovery already cleared, skip disconnect");
        return;
    }

    g_esd.recovery_active = 0;
    g_esd.last_reconnect_time = (u32)jiffies_msec();
    local_irq_enable();

    log_warn("[ESD] recovery failed, disconnecting USB");

    /* Step 1: 断开USB连接 */
    usb_otg_suspend(0, OTG_UNINSTALL);

    /* Step 2: 200ms后重新使能热插拔检测 */
    local_irq_disable();
    g_esd.reconnect_timer_id = sys_hi_timeout_add(NULL, esd_reconnect_timer_cb, 200);
    log_info("[ESD] reconnect delay timer=%u", g_esd.reconnect_timer_id);
    local_irq_enable();
}

static void esd_reconnect_timer_cb(void *priv)
{
    (void)priv;

    local_irq_disable();
    g_esd.reconnect_timer_id = 0;
    local_irq_enable();

    log_info("[ESD] re-enabling USB hotplug detection");
    usb_otg_resume(0);
}

#endif /* CONFIG_USB_ESD_RECOVERY */

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

    // 从 Feature Unit ID 反推 stream_id（与 uac1.c/uac2.c 一致）
    u8 stream_id = UAC_UNIT_ID_STREAM(type);
    u8 is_mic = UAC_UNIT_ID_IS_MIC(type);

    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    if (is_mic) { //MIC
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
        pc_mic_set_volume_by_taskq(l_vol);
#endif
    } else { //SPK
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;

        log_info(">> PC, r_vol:%d, l_vol:%d\n", r_vol, l_vol);
        uac_spk_set_volume_by_taskq(stream_id);
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

static void spk_state_set(u8 stream_id, u8 new_state, const char *reason, const char *func, int line)
{
    struct uac_spk_ctx_t *spk;
    u8 old_state;

    local_irq_disable();

    spk = g_spk[stream_id];
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
        log_info("%s -> %s (same) @%s:%d reason=%s",
                 spk_state_name(old_state),
                 spk_state_name(new_state),
                 func, line,
                 reason ? reason : "N/A");
    }
}
#define SPK_SET_STATE(id, _new, _reason)  spk_state_set((id), (_new), (_reason), __func__, __LINE__)

static void spk_dbg_log_ctx_ex(const char *tag, u8 stream_id)
{
    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    if (!spk) {
        log_warn("[%s] g_spk[%u]=NULL", tag, stream_id);
        return;
    }

#if CONFIG_USB_ESD_RECOVERY
    log_info("[%s] stream=%u state=%s sr=%u ch=%u bits=%u max_pkt=%u halt_cnt=%u open_ts=%u t_halt=%u t_close=%u esd_active=%u esd_recov=%u esd_timer=%u esd_reconn_t=%u esd_ts=%u",
             tag,
             stream_id,
             spk_state_name(spk->state),
             spk->samplerate_hz,
             spk->num_channels,
             spk->bits_per_sample,
             spk->max_iso_packet_bytes,
             spk->stream_halt_count,
             spk->opened_timestamp,
             spk->halt_check_timer_id,
             spk->close_delay_tid,
             g_esd.active,
             g_esd.recovery_active,
             g_esd.recovery_timer_id,
             g_esd.reconnect_timer_id,
             g_esd.last_reconnect_time);
#else
    log_info("[%s] stream=%u state=%s sr=%u ch=%u bits=%u max_pkt=%u halt_cnt=%u open_ts=%u t_halt=%u t_close=%u",
             tag,
             stream_id,
             spk_state_name(spk->state),
             spk->samplerate_hz,
             spk->num_channels,
             spk->bits_per_sample,
             spk->max_iso_packet_bytes,
             spk->stream_halt_count,
             spk->opened_timestamp,
             spk->halt_check_timer_id,
             spk->close_delay_tid);
#endif
}

u8 uac_audio_is_24bit_in_4byte()
{
    return UAC_24BIT_IN_4BYTE;
}

static inline bool is_timeout(u32 start_ms, u32 timeout_ms)
{
    return (u32)jiffies_msec2offset(start_ms, jiffies_msec()) >= timeout_ms;
}

static void spk_player_start(u8 stream_id)
{
    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    if (!spk) {
        return;
    }

    log_info("player_start enter, cur_state=%s", spk_state_name(spk->state));


    if (spk->state == SPK_STATE_RUNNING) {
        // 已运行，先关再开以应用新参数（保险）
        log_warn("player already running, close before reopen");
#if !UAC_SPK_MIC_LOOPBACK
        app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
    }

#if UAC_SPK_MIC_LOOPBACK
    uac_loopback_init();
#elif TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    uac_spk_set_fmt(stream_id, spk->num_channels, spk->bits_per_sample, spk->samplerate_hz);
    app_send_message(APP_MSG_PC_AUDIO_PLAY_OPEN, 0);
#endif


    if (spk->halt_check_timer_id == 0) {
        spk->halt_check_timer_id = sys_hi_timer_add(NULL,
                                   uac_speaker_stream_halt_detect,
                                   UAC_SPEAKER_STREAM_DETECT_INTERVAL);
        log_info("halt_detect timer add id=%u interval=%dms",
                 spk->halt_check_timer_id, UAC_SPEAKER_STREAM_DETECT_INTERVAL);
    }

    spk->stream_halt_count = 0;
    SPK_SET_STATE(stream_id, SPK_STATE_RUNNING, "player_start");
#if CONFIG_USB_ESD_RECOVERY
    g_esd.active = 1;
#endif
}


static void spk_player_stop(u8 stream_id)
{
    local_irq_disable();
    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    if (!spk) {
        local_irq_enable();
        return;
    }
    u8 state = spk->state;
    local_irq_enable();

    if (state == SPK_STATE_CLOSED || state == SPK_STATE_READY) {
        log_warn("player_stop called in invalid state=%s", spk_state_name(state));
    }

#if !UAC_SPK_MIC_LOOPBACK
    app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
    SPK_SET_STATE(stream_id, SPK_STATE_READY, "player_stop");
}

static void usb_sof_spk_write_callback(struct usb_device_t *usb_device,
                                       u32 ep, u32 frame)
{
    struct uac_spk_ctx_t *spk = NULL;
    u8 stream_id;
    for (stream_id = 0; stream_id < UAC_MAX_STREAMS; stream_id++) {
        if (g_spk[stream_id] && g_spk[stream_id]->ep_out == ep) {
            spk = g_spk[stream_id];
            break;
        }
    }
    if (!spk) {
        return;
    }

    if (spk->state == SPK_STATE_CLOSED) {
        return;
    }

    const usb_dev usb_id = usb_device2id(usb_device);

    if (spk->state == SPK_STATE_READY && usb_g_data_size(usb_id, ep)) {
        log_info("READY trigger start, frame=%u ep=%u", frame, ep);
        spk_player_start(stream_id);
    }

    u8  *iso_packet_buf       = spk->iso_packet_buf;
    u32  max_iso_packet_bytes = spk->max_iso_packet_bytes;
    u32  data_valid           = 0;
    for (int i = 0; i < UAC_MAX_PKTS_PER_SOF; i++) {
        int packet_len = usb_g_iso_read(usb_id,
                                        ep,
                                        iso_packet_buf,
                                        max_iso_packet_bytes,
                                        0);
        if (packet_len < spk->min_iso_packet_bytes) {
            break;
        }
        data_valid = 1;
#if UAC_SPK_MIC_LOOPBACK
        uac_loopback_spk_write(iso_packet_buf, packet_len);
#else
        uac_spk_data_isr_cb(stream_id, iso_packet_buf, packet_len);
#endif
    }

    if (!data_valid) {
        if (spk->state == SPK_STATE_DELAY_CLOSE) {
            memset(iso_packet_buf, 0, max_iso_packet_bytes);
#if UAC_SPK_MIC_LOOPBACK
            uac_loopback_spk_write(iso_packet_buf, max_iso_packet_bytes);
#else
            uac_spk_data_isr_cb(stream_id, iso_packet_buf, max_iso_packet_bytes);
#endif
            spk->stream_halt_count = 0;
            // log_info("no data in DELAY_CLOSE, pad zero %u bytes", max_iso_packet_bytes);
        } else {
#if UAC_SPK_MIC_LOOPBACK
            /* no downstream to notify */
#else
            uac_spk_data_isr_cb(stream_id, NULL, 0);
#endif
        }
    } else {
        if (spk->stream_halt_count > 2) {
            log_info("data resumed, halt_count %u -> 0", spk->stream_halt_count);
        }
        spk->stream_halt_count = 0;
#if CONFIG_USB_ESD_RECOVERY
        esd_clear_recovery();
        g_esd.active = 1;
#endif
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
    (void)priv;

    u8 stream_id;
    for (stream_id = 0; stream_id < UAC_MAX_STREAMS; stream_id++) {
        struct uac_spk_ctx_t *spk;
        struct uac_spk_ctx_t bk;

        local_irq_disable();

        spk = g_spk[stream_id];
        if (!spk) {
            local_irq_enable();
            continue;
        }

        bk = *spk;

        local_irq_enable();

        if (bk.state != SPK_STATE_RUNNING) {
            continue;
        }

        bk.stream_halt_count++;

        if (bk.stream_halt_count >= UAC_SPEAKER_STREAM_DETECT_COUNT) {
            log_warn("stream halt detected, stop player");
            spk_player_stop(stream_id);
        }

        local_irq_disable();

        if (g_spk[stream_id] == spk && g_spk[stream_id]) {
            g_spk[stream_id]->stream_halt_count = bk.stream_halt_count;
        }

        local_irq_enable();
    }
}

void uac_speaker_stream_get_volume_ex(u8 stream_id, u16 *l_vol, u16 *r_vol)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        if (l_vol) {
            *l_vol = 0;
        }
        if (r_vol) {
            *r_vol = 0;
        }
        return;
    }
    uac_get_cur_vol(stream_id, l_vol, r_vol);
}

void uac_speaker_stream_get_volume(u16 *l_vol, u16 *r_vol)
{
    uac_speaker_stream_get_volume_ex(0, l_vol, r_vol);
}

u8 uac_speaker_stream_status_ex(u8 stream_id)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return 0;
    }
    return g_spk[stream_id] != NULL;
}

static int uac_speaker_alloc_iso_buffer(u8 stream_id)
{
    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    u32 bits_per_sample = spk->bits_per_sample;

#if UAC_24BIT_IN_4BYTE
    if (spk->bits_per_sample == 24) {
        bits_per_sample = 32;
    }
#endif

    u32 bytes_per_sample_frame   = spk->num_channels * (bits_per_sample / 8);
    u32 min_sample_frames_per_ms = spk->samplerate_hz / 1000;
    u32 max_sample_frames_per_ms = (spk->samplerate_hz + 999) / 1000;

    spk->min_iso_packet_bytes = bytes_per_sample_frame * min_sample_frames_per_ms;
    spk->max_iso_packet_bytes = bytes_per_sample_frame * max_sample_frames_per_ms;

    log_info("max_iso_packet_bytes:%d num_channels:%d samplerate_hz:%d bits_per_sample:%d",
             spk->max_iso_packet_bytes, spk->num_channels,
             spk->samplerate_hz, spk->bits_per_sample);

    // 分配 buffer，如果已经分配则释放重分配
    if (spk->iso_packet_buf) {
        free(spk->iso_packet_buf);
        spk->iso_packet_buf = NULL;
    }

    spk->iso_packet_buf = malloc(spk->max_iso_packet_bytes);
    assert_d(spk->iso_packet_buf != NULL);

    return 0;
}

// 配置采样率/声道/位宽，可在流已经打开时动态调用
// 返回 1 表示格式发生变化，0 表示未变化
static int uac_speaker_configure(u8 stream_id, u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    int need_update = 0;

    spk->opened_timestamp = jiffies_msec();

    if (spk->samplerate_hz != samplerate_hz ||
        spk->num_channels  != num_channels ||
        spk->bits_per_sample != bits_per_sample) {

        log_info("change %u/%u/%u -> %u/%u/%u",
                 spk->samplerate_hz, spk->num_channels, spk->bits_per_sample,
                 samplerate_hz, num_channels, bits_per_sample);

        spk->samplerate_hz    = samplerate_hz;
        spk->num_channels     = (u8)num_channels;
        spk->bits_per_sample  = (u8)bits_per_sample;
        need_update = 1;
    }

    if (need_update == 0) {
        log_info("unchanged");
        return 0;
    }

    // 立即把新格式通知音频模块(pc_spk_fmt)，不能等数据到达 spk_player_start 才设置，
    // 否则 open/close/open 之间没有数据流时，DELAY_CLOSE 补 0 路径会用旧格式打开播放器，
    // 新参数无法生效
    uac_spk_set_fmt(stream_id, spk->num_channels, spk->bits_per_sample, spk->samplerate_hz);

    uac_speaker_alloc_iso_buffer(stream_id);

    return 1;
}


void uac_speaker_stream_open_ex(u8 stream_id, u32 samplerate_hz,
                                u32 num_channels, u32 bits_per_sample)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    u8 ep_out = g_stream_configs[stream_id].spk_ep;

    if (g_spk[stream_id]) {
        log_warn("speaker already opened, change config");

        if (g_spk[stream_id]->close_delay_tid) {
            log_info("[SPK] cancel pending close_delay tid=%u because reopen", g_spk[stream_id]->close_delay_tid);
            sys_hi_timeout_del(g_spk[stream_id]->close_delay_tid);
            g_spk[stream_id]->close_delay_tid = 0;
        }

        spk_dbg_log_ctx_ex("open_reconfig_before", stream_id);

        u8 old_state = g_spk[stream_id]->state;
        int fmt_changed = uac_speaker_configure(stream_id, samplerate_hz, num_channels, bits_per_sample);

        if (old_state == SPK_STATE_RUNNING) {
            spk_player_stop(stream_id);
        } else if (old_state == SPK_STATE_DELAY_CLOSE) {
            if (fmt_changed) {
                // delay_close 期间播放器并未真正关闭，格式变化必须真正关闭后重开，
                // 否则新格式不会通知到音频模块，继续用旧格式喂新数据会产生噪音
                spk_player_stop(stream_id);
            } else {
                SPK_SET_STATE(stream_id, SPK_STATE_READY, "reopen during delay_close");
            }
        } else {
            SPK_SET_STATE(stream_id, SPK_STATE_READY, "reconfig keep armed");
        }

        spk_dbg_log_ctx_ex("open_reconfig_after", stream_id);
        return;
    }

    g_spk[stream_id] = zalloc(sizeof(struct uac_spk_ctx_t));
    assert_d(g_spk[stream_id] != NULL);

    g_spk[stream_id]->stream_id = stream_id;
    g_spk[stream_id]->ep_out    = ep_out;
    g_spk[stream_id]->state     = SPK_STATE_CLOSED;
    uac_speaker_configure(stream_id, samplerate_hz, num_channels, bits_per_sample);

    SPK_SET_STATE(stream_id, SPK_STATE_READY, "stream_open init ready");

    usb_g_set_sof_hander(0, ep_out, usb_sof_spk_write_callback);

#if CONFIG_USB_ESD_RECOVERY
    usb_slave_set_status_hander(esd_suspend_status_cb);
#endif

    spk_dbg_log_ctx_ex("open_done", stream_id);
}


static void uac_speaker_stream_close_delay(void *priv)
{
    u8 stream_id = (uintptr_t)priv & 0xFF;
    int release  = ((uintptr_t)priv >> 8) & 1;

    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    if (!spk) {
        return;
    }

    log_info("close_delay fire, release=%d, state=%s",
             release, spk_state_name(spk->state));

    spk->close_delay_tid = 0;

    usb_g_set_sof_hander(0, spk->ep_out, NULL);

    if (spk->halt_check_timer_id) {
        sys_hi_timer_del(spk->halt_check_timer_id);
        log_info("halt_detect timer del id=%u", spk->halt_check_timer_id);
        spk->halt_check_timer_id = 0;
    }


    if (spk->state == SPK_STATE_RUNNING ||
        spk->state == SPK_STATE_DELAY_CLOSE) {

        uac_player_close(release);

    }

    if (spk->iso_packet_buf) {
        free(spk->iso_packet_buf);
        spk->iso_packet_buf = NULL;
    }

    SPK_SET_STATE(stream_id, SPK_STATE_CLOSED, "close_delay finalize free ctx");

#if CONFIG_USB_ESD_RECOVERY
    /* 释放前停止ESD恢复timer */
    if (g_esd.recovery_timer_id) {
        sys_hi_timeout_del(g_esd.recovery_timer_id);
        g_esd.recovery_timer_id = 0;
    }
    if (g_esd.reconnect_timer_id) {
        sys_hi_timeout_del(g_esd.reconnect_timer_id);
        g_esd.reconnect_timer_id = 0;
    }
    /* 只有所有stream都关闭时取消suspend状态回调注册 */
    {
        int any = 0;
        for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
            if (i != stream_id && g_spk[i]) {
                any = 1;
                break;
            }
        }
        if (!any) {
            usb_slave_set_status_hander(NULL);
        }
    }
#endif

    free(spk);
    g_spk[stream_id] = NULL;
}


void uac_speaker_stream_close_ex(u8 stream_id, int release)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    struct uac_spk_ctx_t *spk = g_spk[stream_id];
    if (!spk) {
        return;
    }

#if CONFIG_USB_ESD_RECOVERY
    /* 只有最后一个活跃流关闭时才清除 g_esd.active */
    {
        int any = 0;
        for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
            if (i != stream_id && g_spk[i]) {
                any = 1;
                break;
            }
        }
        if (!any) {
            g_esd.active = 0;
        }
    }
    /* 清除任何进行中的ESD恢复 */
    if (g_esd.recovery_active) {
        g_esd.recovery_active = 0;
        if (g_esd.recovery_timer_id) {
            sys_hi_timeout_del(g_esd.recovery_timer_id);
            g_esd.recovery_timer_id = 0;
            log_info("[ESD] recovery cancelled on stream close");
        }
    }
#endif

    log_info("close request release=%d, state=%s ts=%d",
             release, spk_state_name(spk->state), jiffies_msec());

    if (is_timeout(spk->opened_timestamp, 150) || release) {
        if (spk->close_delay_tid) {
            sys_hi_timeout_del(spk->close_delay_tid);
            spk->close_delay_tid = 0;
        }
        uintptr_t packed = stream_id | (release ? 0x100 : 0);
        uac_speaker_stream_close_delay((void *)packed);
    } else {
        if (spk->state == SPK_STATE_RUNNING ||
            spk->state == SPK_STATE_READY) {
            SPK_SET_STATE(stream_id, SPK_STATE_DELAY_CLOSE, "close request within holdoff window");
        }

        uintptr_t packed = stream_id | (0 << 8);
        if (spk->close_delay_tid == 0) {
            spk->close_delay_tid = sys_hi_timeout_add((void *)packed,
                                   uac_speaker_stream_close_delay, 150);
            log_info("close_delay timer add id=%u timeout=150ms", spk->close_delay_tid);
        } else {
            sys_hi_timeout_modify(spk->close_delay_tid, 150);
            log_info("close_delay timer modify id=%u timeout=150ms", spk->close_delay_tid);
        }
    }
}

void uac_spk_stream_set_always_keep(u8 stream_id, u8 enable)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }
    if (g_spk[stream_id]) {
        g_spk[stream_id]->fill_stream_always_keep = enable;
    }
}

u8 uac_spk_stream_get_always_keep(u8 stream_id)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return 0;
    }
    if (g_spk[stream_id]) {
        return g_spk[stream_id]->fill_stream_always_keep;
    }
    return 0;
}


struct uac_mic_handle {
    u8   stream_id;                    /* stream identifier */
    u8   ep_in;                        /* ISO IN EP number */
    int (*tx_handler)(void *, void *, int);
    void *tx_handler_priv;
    u32  opened_timestamp;
    u32  samplerate_hz;            // 采样率（Hz）
    u8   num_channels;             // 声道数
    u8   bits_per_sample;          // 每声道采样位宽
    u16  close_delay_tid;
};

static struct uac_mic_handle *g_mic[UAC_MAX_STREAMS];

int uac_mic_stream_read_ex(u8 stream_id, u8 *buf, u32 len)
{
#if UAC_SPK_MIC_LOOPBACK
    return uac_loopback_mic_read(buf, len);
#endif
    if (stream_id >= UAC_MAX_STREAMS) {
        memset(buf, 0, len);
        return len;
    }
    if (g_mic[stream_id] == NULL || (g_mic[stream_id]->tx_handler == NULL)) {
        memset(buf, 0, len);
        return len;
    }
    return g_mic[stream_id]->tx_handler(g_mic[stream_id]->tx_handler_priv, buf, len);
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(void *, void *, int))
{
    if (g_mic[0] == NULL) {
        return ;
    }
    g_mic[0]->tx_handler = tx_handler;
    g_mic[0]->tx_handler_priv = priv;
}

void uac_mic_stream_get_volume_ex(u8 stream_id, u16 *vol)
{
    if (!vol) {
        return;
    }
    if (stream_id >= UAC_MAX_STREAMS) {
        *vol = 0;
        return;
    }
    *vol = uac_get_mic_vol(stream_id);
}

void uac_mic_stream_get_volume(u16 *vol)
{
    uac_mic_stream_get_volume_ex(0, vol);
}

u8 uac_get_mic_stream_status_ex(u8 stream_id)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return 0;
    }
    return g_mic[stream_id] != NULL;
}

static void uac_mic_stream_restart(u8 stream_id, u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    struct uac_mic_handle *mic = g_mic[stream_id];
    int need_update = 0;

    mic->opened_timestamp = jiffies_msec();

    if (mic->samplerate_hz != samplerate_hz ||
        mic->num_channels != num_channels ||
        mic->bits_per_sample != bits_per_sample) {

        log_info("change %u/%u/%u -> %u/%u/%u",
                 mic->samplerate_hz, mic->num_channels, mic->bits_per_sample,
                 samplerate_hz, num_channels, bits_per_sample);

        mic->samplerate_hz   = samplerate_hz;
        mic->num_channels    = num_channels;
        mic->bits_per_sample = bits_per_sample;
        need_update = 1;
    }

    if (!need_update) {
        return;
    }

    mic->tx_handler      = NULL;
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    pc_mic_set_fmt(num_channels, bits_per_sample, samplerate_hz);
    pc_mic_recoder_restart_by_taskq();
#endif
}

// 打开 mic_stream
void uac_mic_stream_open_ex(u8 stream_id, u32 samplerate_hz, u32 num_channels, u32 bits_per_sample)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    u8 ep_in = g_stream_configs[stream_id].mic_ep;

    if (g_mic[stream_id]) {
        log_warn("Mic stream already opened, change config");
        if (g_mic[stream_id]->close_delay_tid) {
            log_info("[MIC] cancel pending close_delay tid=%u because reopen", g_mic[stream_id]->close_delay_tid);
            sys_hi_timeout_del(g_mic[stream_id]->close_delay_tid);
            g_mic[stream_id]->close_delay_tid = 0;
        }
        uac_mic_stream_restart(stream_id, samplerate_hz, num_channels, bits_per_sample);

        return;
    }

    g_mic[stream_id] = zalloc(sizeof(struct uac_mic_handle));

    assert_d(g_mic[stream_id] != NULL);

    g_mic[stream_id]->stream_id = stream_id;
    g_mic[stream_id]->ep_in     = ep_in;
    g_mic[stream_id]->tx_handler = NULL;
    // 首次 open 必须记录格式和时间戳，否则 reopen 的格式判断(基于0)恒判定变化、
    // 且首次 close 的 is_timeout(0,150) 会误判超时跳过 delay_close
    g_mic[stream_id]->opened_timestamp = jiffies_msec();
    g_mic[stream_id]->samplerate_hz    = samplerate_hz;
    g_mic[stream_id]->num_channels     = (u8)num_channels;
    g_mic[stream_id]->bits_per_sample  = (u8)bits_per_sample;
#if UAC_SPK_MIC_LOOPBACK
    uac_loopback_init();
#elif TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    pc_mic_set_fmt(num_channels, bits_per_sample, samplerate_hz);
    pc_mic_recoder_open_by_taskq();
#endif

}

static void uac_mic_stream_close_delay(void *priv)
{
    u8 stream_id = (uintptr_t)priv & 0xFF;
    int release  = ((uintptr_t)priv >> 8) & 1;

    struct uac_mic_handle *mic = g_mic[stream_id];
    if (!mic) {
        return;
    }
    mic->close_delay_tid = 0;

    log_info("## Func:uac_mic_stream_close_delay, Line:%d, Close Mic!!\n", __LINE__);
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    if (release) {
        pc_mic_recoder_close();
    } else {
        pc_mic_recoder_close_by_taskq();
    }
#endif
    free(mic);
    g_mic[stream_id] = NULL;
}

void uac_mic_stream_close_ex(u8 stream_id, int release)
{
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    struct uac_mic_handle *mic = g_mic[stream_id];
    if (mic == NULL) {
        return ;
    }
    //频繁开关mic，导致出现audio或者蓝牙工作异常，
    //收到mic关闭命令后延时150ms再发消息通知audio模块执行关闭动作
    //如果在150ms之内继续收到usb下发的关闭命令，则继续推迟150ms。
    if (is_timeout(mic->opened_timestamp, 150) || release) {
        if (mic->close_delay_tid) {
            sys_hi_timeout_del(mic->close_delay_tid);
            mic->close_delay_tid = 0;
        }
        uintptr_t packed = stream_id | (release ? 0x100 : 0);
        uac_mic_stream_close_delay((void *)packed);
    } else {
        uintptr_t packed = stream_id | (0 << 8);
        if (mic->close_delay_tid == 0) {
            mic->close_delay_tid = sys_hi_timeout_add((void *)packed, uac_mic_stream_close_delay, 150);
        } else {
            sys_hi_timeout_modify(mic->close_delay_tid, 150);
        }
    }
}


