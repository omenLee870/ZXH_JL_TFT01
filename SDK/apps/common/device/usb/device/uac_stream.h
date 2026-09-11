/*****************************************************************
>file name : usb_audio.h
>author : lichao
>create time : Wed 22 May 2019 10:39:35 AM CST
*****************************************************************/
#ifndef _UAC_STREAM_H_
#define _UAC_STREAM_H_
#include "typedef.h"
#define UAC_STREAM_ID   0
#define UAC_DUAL_STREAM_ID   1

/* 根据 terminal/unit ID 反推 stream_id 与 spk/mic 属性。
 * ID 布局：每个 stream 占 10 个号，SPK 用 1..4（IT/FU/SU/OT），MIC 用 5..8。
 * 注意：不能简单用 unit_id 与 MIC_FEATURE_UNIT_ID(0) 比较大小来区分 spk/mic，
 * 因为 SPK stream N(N>=1) 的 unit ID（2+10N）大于 MIC stream 0 的 ID（6）。 */
#define UAC_UNIT_ID_STREAM(id)   (((id) - 1) / 10)
#define UAC_UNIT_ID_IS_MIC(id)   ((((id) - 1) % 10) >= 4)

struct uac_stream_config {
    /* Speaker */
    u32  spk_rates[6];
    u8   spk_rate_num;
    u8   spk_res;
    u8   spk_res_2;
    u8   spk_channels;
    u16  spk_channel_config;
    u8   spk_controls[9];

    /* Microphone (mic_rate_num=0 means no MIC) */
    u32  mic_rates[6];
    u8   mic_rate_num;
    u8   mic_res;
    u8   mic_res_2;
    u8   mic_channels;
    u16  mic_channel_config;
    u8   mic_controls[5];

    /* EP allocation */
    u8   spk_ep;
    u8   mic_ep;

    /* String descriptor indices */
    u8   spk_str_index;  // iInterface / iTerminal for speaker
    u8   mic_str_index;  // iInterface / iTerminal for microphone
};

extern const struct uac_stream_config g_stream_configs[];

/* ---------- stream config helpers ---------- */
#define UAC_CFG_HAS_SPK(cfg)  ((cfg)->spk_rate_num > 0)
#define UAC_CFG_HAS_MIC(cfg)  ((cfg)->mic_rate_num > 0)

enum uac_event {
    USB_AUDIO_PLAY_OPEN = 0x0,
    USB_AUDIO_PLAY_CLOSE,
    USB_AUDIO_MIC_OPEN,
    USB_AUDIO_MIC_CLOSE,
    // USB_AUDIO_MUTE,
    USB_AUDIO_SET_PLAY_VOL,
    USB_AUDIO_SET_MIC_VOL,
};

#define      UAC_MAX_PKTS_PER_SOF           4

/* === Multi-stream API (with stream_id) === */
void uac_speaker_stream_open_ex(u8 stream_id, u32 samplerate_hz,
                                u32 num_channels, u32 bits_per_sample);
void uac_speaker_stream_close_ex(u8 stream_id, int release);
u8   uac_speaker_stream_status_ex(u8 stream_id);

void uac_mic_stream_open_ex(u8 stream_id, u32 samplerate_hz,
                            u32 num_channels, u32 bits_per_sample);
void uac_mic_stream_close_ex(u8 stream_id, int release);
int  uac_mic_stream_read_ex(u8 stream_id, u8 *buf, u32 len);
u8   uac_get_mic_stream_status_ex(u8 stream_id);

void uac_spk_stream_set_always_keep(u8 stream_id, u8 enable);
u8   uac_spk_stream_get_always_keep(u8 stream_id);

/* === Legacy API (unchanged declarations) === */
int uac_get_spk_vol();
u8 uac_audio_is_24bit_in_4byte();
void uac_speaker_stream_write(const u8 *obuf, u32 len);
void uac_speaker_stream_get_volume(u16 *l_vol, u16 *r_vol);
void uac_speaker_stream_get_volume_ex(u8 stream_id, u16 *l_vol, u16 *r_vol);
void set_uac_speaker_rx_handler(void *priv, void (*rx_handler)(int, void *, int));
void uac_mic_stream_get_volume(u16 *vol);
void uac_mic_stream_get_volume_ex(u8 stream_id, u16 *vol);
void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(void *, void *, int));
void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol);

#if CONFIG_USB_ESD_RECOVERY
void uac_speaker_notify_reset(void);
#endif

/* === Compatibility wrappers (stream_id=0) === */
static inline void uac_speaker_stream_open(u32 samplerate_hz,
        u32 num_channels, u32 bits_per_sample)
{
    uac_speaker_stream_open_ex(0, samplerate_hz, num_channels,
                               bits_per_sample);
}

static inline void uac_speaker_stream_close(int release)
{
    uac_speaker_stream_close_ex(0, release);
}

static inline u8 uac_speaker_stream_status(void)
{
    return uac_speaker_stream_status_ex(0);
}

static inline void uac_mic_stream_open(u32 samplerate_hz,
                                       u32 num_channels, u32 bits_per_sample)
{
    uac_mic_stream_open_ex(0, samplerate_hz, num_channels,
                           bits_per_sample);
}

static inline void uac_mic_stream_close(int release)
{
    uac_mic_stream_close_ex(0, release);
}

static inline int uac_mic_stream_read(u8 *buf, u32 len)
{
    return uac_mic_stream_read_ex(0, buf, len);
}

static inline u8 uac_get_mic_stream_status(void)
{
    return uac_get_mic_stream_status_ex(0);
}

#endif
