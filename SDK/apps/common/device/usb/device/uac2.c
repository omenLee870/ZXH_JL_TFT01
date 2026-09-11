/* Copyright(C) JieLi Technology
 * usb audio 2.0
 * All right reserved
 *
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/device/uac_audio.h"
#include "usb/device/uac_audio_v2.h"
#include "app_config.h"
#include "uac_stream.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC2]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE) && (USB_AUDIO_VERSION == USB_AUDIO_VERSION_2_0)

/* Windows UAC2驱动要求Clock Source RANGE至少包含2个子范围才能正常枚举，
 * 单采样率下Windows会请求完采样率表后直接suspend设备。
 * 请将SPK_AUDIO_RATE_NUM和MIC_AUDIO_RATE_NUM至少配置为2。 */
#if SPK_AUDIO_RATE_NUM < 2
#error "UAC2: SPK_AUDIO_RATE_NUM must be at least 2 (Windows UAC2 driver requires >=2 sample rates)"
#endif
#if MIC_AUDIO_RATE_NUM < 2
#error "UAC2: MIC_AUDIO_RATE_NUM must be at least 2 (Windows UAC2 driver requires >=2 sample rates)"
#endif

/* Clock Source IDs — per-stream, independent from terminal IDs */
#define CLOCK_ID_SPK(n)  (20 + (n) * 10)
#define CLOCK_ID_MIC(n)  (21 + (n) * 10)

/* g_stream_configs — multi-stream config table */
const struct uac_stream_config g_stream_configs[UAC_MAX_STREAMS] = {
    {
        .spk_rates     = {SPK_AUDIO_RATE_1, SPK_AUDIO_RATE_2, SPK_AUDIO_RATE_3},
        .spk_rate_num  = SPK_AUDIO_RATE_NUM,
        .spk_res       = SPK_AUDIO_RES,
        .spk_res_2     = SPK_AUDIO_RES_2,
        .spk_channels  = SPK_CHANNEL,
        .spk_channel_config = SPK_CHANNEL_CONFIG_VAL,
        .spk_controls  = {0},         /* filled by builder */
        .mic_rates     = {MIC_AUDIO_RATE_1, MIC_AUDIO_RATE_2, MIC_AUDIO_RATE_3},
        .mic_rate_num  = MIC_AUDIO_RATE_NUM,
        .mic_res       = MIC_AUDIO_RES,
        .mic_res_2     = MIC_AUDIO_RES_2,
        .mic_channels  = MIC_CHANNEL,
        .mic_channel_config = MIC_CHANNEL_CONFIG_VAL,
        .mic_controls  = {0},         /* filled by builder */
        .spk_ep        = SPK_ISO_EP_OUT,
        .mic_ep        = MIC_ISO_EP_IN,
        .spk_str_index = SPEAKER_STR_INDEX,
        .mic_str_index = MIC_STR_INDEX,
    },
#if UAC_MAX_STREAMS >= 2
    {
        .spk_rates     = {UAC_STREAM_1_SPK_RATE_1, UAC_STREAM_1_SPK_RATE_2, UAC_STREAM_1_SPK_RATE_3},
        .spk_rate_num  = UAC_STREAM_1_SPK_RATE_NUM,
        .spk_res       = UAC_STREAM_1_SPK_RES,
        .spk_res_2     = UAC_STREAM_1_SPK_RES_2,
        .spk_channels  = UAC_STREAM_1_SPK_CHANNELS,
        .spk_channel_config = UAC_STREAM_1_SPK_CHANNEL_CONFIG,
        .spk_controls  = {0},
        .mic_rates     = {0},
        .mic_rate_num  = UAC_STREAM_1_MIC_RATE_NUM,
        .mic_res       = 0,
        .mic_res_2     = 0,
        .mic_channels  = 0,
        .mic_channel_config = 0,
        .mic_controls  = {0},
        .spk_ep        = UAC_STREAM_1_SPK_EP,
        .mic_ep        = UAC_STREAM_1_MIC_EP,
        .spk_str_index = UAC_STREAM_1_SPEAKER_STR_INDEX,
        .mic_str_index = UAC_STREAM_1_MIC_STR_INDEX,
    },
#endif
};


//bmChannelConfig
#define UAC_V2_CHANNEL_FRONT_LEFT               BIT(0)
#define UAC_V2_CHANNEL_FRONT_RIGHT              BIT(1)
#define UAC_V2_CHANNEL_FRONT_CENTER             BIT(2)
#define UAC_V2_CHANNEL_LOW_FREQ_EFFECT          BIT(3)
#define UAC_V2_CHANNEL_BACK_LEFT                BIT(4)
#define UAC_V2_CHANNEL_BACK_RIGHT               BIT(5)
#define UAC_V2_CHANNEL_FRONT_LEFT_OF_C          BIT(6)
#define UAC_V2_CHANNEL_FRONT_RIGHT_OF_C         BIT(7)
#define UAC_V2_CHANNEL_BACK_CENTER              BIT(8)
#define UAC_V2_CHANNEL_SIDE_LEFT                BIT(9)
#define UAC_V2_CHANNEL_SIDE_RIGHT               BIT(10)
#define UAC_V2_CHANNEL_TOP_CENTER               BIT(11)
#define UAC_V2_CHANNEL_TOP_FRONT_LEFT           BIT(12)
#define UAC_V2_CHANNEL_TOP_FRONT_CENTER         BIT(13)
#define UAC_V2_CHANNEL_TOP_FRONT_RIGHT          BIT(14)
#define UAC_V2_CHANNEL_TOP_BACK_LEFT            BIT(15)
#define UAC_V2_CHANNEL_TOP_BACK_CENTER          BIT(16)
#define UAC_V2_CHANNEL_TOP_BACK_RIGHT           BIT(17)
#define UAC_V2_CHANNEL_TOP_FRONT_LEFT_OF_C      BIT(18)
#define UAC_V2_CHANNEL_TOP_FRONT_RIGHT_OF_C     BIT(19)
#define UAC_V2_CHANNEL_LEFT_LFE                 BIT(20)
#define UAC_V2_CHANNEL_RIGHT_LFE                BIT(21)
#define UAC_V2_CHANNEL_TOP_SIDE_LEFT            BIT(22)
#define UAC_V2_CHANNEL_TOP_SIDE_RIGHT           BIT(23)
#define UAC_V2_CHANNEL_BOTTOM_CENTER            BIT(24)
#define UAC_V2_CHANNEL_BACK_LEFT_OF_C           BIT(25)
#define UAC_V2_CHANNEL_BACK_RIGHT_OF_C          BIT(26)

//feature unit bmControls
#define UAC_V2_FU_CONTROL_UNDEF                 (0x00)
#define UAC_V2_FU_CONTROL_MUTE                  (0x03 << 0)
#define UAC_V2_FU_CONTROL_VOLUME                (0x03 << 2)
#define UAC_V2_FU_CONTROL_BASS                  (0x03 << 4)
#define UAC_V2_FU_CONTROL_MID                   (0x03 << 6)
#define UAC_V2_FU_CONTROL_TREBLE                (0x03 << 8)
#define UAC_V2_FU_CONTROL_EQUALIZER             (0x03 << 10)
#define UAC_V2_FU_CONTROL_AGC                   (0x03 << 12)
#define UAC_V2_FU_CONTROL_DELAY                 (0x03 << 14)
#define UAC_V2_FU_CONTROL_BASS_BOOST            (0x03 << 16)
#define UAC_V2_FU_CONTROL_LOUDNESS              (0x03 << 18)
#define UAC_V2_FU_CONTROL_INP_GAIN              (0x03 << 20)
#define UAC_V2_FU_CONTROL_INP_GAIN_PAD          (0x03 << 22)
#define UAC_V2_FU_CONTROL_PHASE_INVERT          (0x03 << 24)
#define UAC_V2_FU_CONTROL_UNDERFLOW             (0x03 << 26)
#define UAC_V2_FU_CONTROL_OVERFLOW              (0x03 << 28)

//format type
#define UAC_V2_FORMAT_TYPE_I_PCM                0x00000001
#define UAC_V2_FORMAT_TYPE_I_PCM8               0x00000002
#define UAC_V2_FORMAT_TYPE_I_IEEE_FLOAT         0x00000004
#define UAC_V2_FORMAT_TYPE_I_ALAW               0x00000008
#define UAC_V2_FORMAT_TYPE_I_MULAW              0x00000010


#if SPK_CHANNEL == 1

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_CENTER \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 2

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 3  //2.1

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 4

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 5

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 6  //5.1

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 7

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 8  //7.1

#define SPK_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(UAC_V2_FU_CONTROL_VOLUME)

#else
#error "unsupport usb speaker channel number"
#endif


#if MIC_CHANNEL == 1

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        0 \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 2

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 3  //2.1

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 4

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 5

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 6  //5.1

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 7

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#elif MIC_CHANNEL == 8  //7.1

#define MIC_CHANNEL_CONFIG \
        UAC_DWORD( \
        UAC_V2_CHANNEL_FRONT_LEFT | \
        UAC_V2_CHANNEL_FRONT_RIGHT | \
        UAC_V2_CHANNEL_FRONT_CENTER | \
        UAC_V2_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V2_CHANNEL_BACK_LEFT | \
        UAC_V2_CHANNEL_BACK_RIGHT | \
        UAC_V2_CHANNEL_SIDE_LEFT | \
        UAC_V2_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_DWORD(UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0), \
        UAC_DWORD(0)

#else
#error "unsupport usb mic channel number"
#endif



/*********************************************************/
/*
				   Audio Class
*/
/*********************************************************/
///Standard Interface Association Descriptor
static const u8 uac_audio_interface_association[] = {
    USB_DT_INTERFACE_ASSOCIATION_SIZE, //Size of this descriptor in bytes: 8
    USB_DT_INTERFACE_ASSOCIATION,//INTERFACE ASSOCIATION Descriptor
    0,//Interface number of the first interface that is associated with this function
    0,//Number of contiguous interfaces that are associated with this function.
    USB_CLASS_AUDIO,//AUDIO_FUNCTION Function Class code
    0,//FUNCTION_SUBCLASS_UNDEFINED
    UAC_VERSION_2,//AF_VERSION_02_00 Function Protocol code.
    SPEAKER_STR_INDEX,//Index of a string descriptor that describes this interface.
};
static const u8 uac_ac_standard_interface_desc[] = {
///standard interface AC descriptor(Interface 0, Alternate Setting 0)
    USB_DT_INTERFACE_SIZE,    //Length
    USB_DT_INTERFACE,     //DescriptorType:Inerface
    0x00,     //InterfaceNum
    0x00,       //AlternateSetting
    0x00,       //NumEndpoint
    USB_CLASS_AUDIO,       //InterfaceClass:audio
    USB_SUBCLASS_AUDIOCONTROL,       //InterfaceSubClass:audio ctl
    UAC_VERSION_2,       //InterfaceProtocol
    SPEAKER_STR_INDEX,      //Interface String
};

struct spk_info_t {
    u16 spk_left_vol;
    u16 spk_right_vol;
    u16 spk_max_vol;
    u16 spk_min_vol;
    u16 spk_def_vol;
    u16 spk_vol_res;
    u8 *spk_dma_buffer;
    u32 spk_sampling_rate;
    u16 spk_padding_interval;
    u16 spk_padding_cnt;
    u16 res;
    u16 sof_cnt;
    u8  spk_mute;
    u8  ps4_mode;
    u8  alternate_setting;
    u8  downstream_halt;
};

struct mic_info_t {
    u16 mic_max_vol;
    u16 mic_min_vol;
    u16 mic_def_vol;
    u16 mic_vol_res;
    u16 mic_vol;
    u8 *mic_dma_buffer;
    u32 mic_sampling_rate;
    u16 mic_padding_interval;
    u16 mic_padding_cnt;
    u16 res;
    u16 sof_cnt;
    u8  mic_mute;
    u8  bAGC;
    u8  ps4_mode;
    u8  alternate_setting;
    u8  mic_no_data;
    u8  upstream_halt;
};

struct spk_info_t *spk_info;
struct mic_info_t *mic_info;

struct stream_if_map {
    u8 stream_id;
    u8 if_num_ac;
    u8 if_num_as;
};
static struct stream_if_map *g_stream_if_map;

/* 通过 AS interface number 反查 stream_id（避免直接用 if_num 作数组下标越界） */
static u8 uac_stream_id_from_as_if(u8 if_num)
{
    if (!g_stream_if_map) {
        return 0xFF;
    }
    for (u8 i = 0; i < UAC_MAX_STREAMS * 2; i++) {
        if (g_stream_if_map[i].if_num_as == if_num) {
            return g_stream_if_map[i].stream_id;
        }
    }
    return 0xFF;
}

static void open_spk(struct usb_device_t *usb_device, u8 stream_id);
static void open_mic(struct usb_device_t *usb_device, u8 stream_id);

#define AUDIO_SAMPLE_FREQ_NUM(num) \
    DW1BYTE(num), DW2BYTE(num)

#define AUDIO_SAMPLE_FREQ_4B(freq) \
    DW1BYTE(freq), DW2BYTE(freq), DW3BYTE(freq), DW4BYTE(freq)

static const u8 speaker_default_sampling_freq_table[] = {
    AUDIO_SAMPLE_FREQ_NUM(SPK_AUDIO_RATE_NUM),
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_1),     //MIN
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_1),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#if SPK_AUDIO_RATE_NUM > 1
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_2),     //MIN
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_2),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#endif
#if SPK_AUDIO_RATE_NUM > 2
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_3),     //MIN
    AUDIO_SAMPLE_FREQ_4B(SPK_AUDIO_RATE_3),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#endif
};

static const u8 mic_default_sampling_freq_table[] = {
    AUDIO_SAMPLE_FREQ_NUM(MIC_AUDIO_RATE_NUM),
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_1),     //MIN
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_1),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#if MIC_AUDIO_RATE_NUM > 1
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_2),     //MIN
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_2),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#endif
#if MIC_AUDIO_RATE_NUM > 2
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_3),     //MIN
    AUDIO_SAMPLE_FREQ_4B(MIC_AUDIO_RATE_3),     //MAX
    AUDIO_SAMPLE_FREQ_4B(0x00),                 //RES
#endif

};

static u32 vol_convert(u16 v)
{
    const u16 vol_table[] = {
        0xe3a0, 0xe461, 0xe519, 0xe5c8, 0xe670, 0xe711, 0xe7ac, 0xe840, 0xe8cf, 0xe959,
        0xe9df, 0xea60, 0xeadc, 0xeb55, 0xebca, 0xec3c, 0xecab, 0xed16, 0xed7f, 0xede5,
        0xee48, 0xeea9, 0xef08, 0xef64, 0xefbe, 0xf016, 0xf06c, 0xf0c0, 0xf113, 0xf164,
        0xf1b3, 0xf200, 0xf24c, 0xf297, 0xf2e0, 0xf328, 0xf36e, 0xf3b4, 0xf3f8, 0xf43a,
        0xf47c, 0xf4bd, 0xf4fc, 0xf53b, 0xf579, 0xf5b5, 0xf5f1, 0xf62c, 0xf666, 0xf69f,
        0xf6d7, 0xf70e, 0xf745, 0xf77b, 0xf7b0, 0xf7e5, 0xf818, 0xf84b, 0xf87e, 0xf8b0,
        0xf8e1, 0xf911, 0xf941, 0xf970, 0xf99f, 0xf9cd, 0xf9fb, 0xfa28, 0xfa55, 0xfa81,
        0xfaad, 0xfad8, 0xfb03, 0xfb2d, 0xfb56, 0xfb80, 0xfba9, 0xfbd1, 0xfbf9, 0xfc21,
        0xfc48, 0xfc6f, 0xfc95, 0xfcbb, 0xfce1, 0xfd06, 0xfd2b, 0xfd50, 0xfd74, 0xfd98,
        0xfdbc, 0xfddf, 0xfe02, 0xfe25, 0xfe47, 0xfe69, 0xfe8b, 0xfead, 0xfece, 0xfeef,
        0xff0f,
    };

    if (v <= 100) {
        return vol_table[v];
    }

    for (int i = 0; i < sizeof(vol_table) / 2; i++) {
        if (v <= vol_table[i]) {
            return i;
        }
    }

    return 0;
}

void uac_get_cur_vol(u8 stream_id, u16 *l_vol, u16 *r_vol)
{
    if (l_vol) {
        *l_vol = 0;
    }
    if (r_vol) {
        *r_vol = 0;
    }
    if (!spk_info || stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    if (spk_info[stream_id].spk_dma_buffer) {
        if (!spk_info[stream_id].spk_mute) {
            if (l_vol) {
                *l_vol = vol_convert(spk_info[stream_id].spk_left_vol);
            }
            if (r_vol) {
                *r_vol = vol_convert(spk_info[stream_id].spk_right_vol);
            }
        }
    }
}
u16 uac_get_mic_vol(u8 stream_id)
{
    if (!mic_info || stream_id >= UAC_MAX_STREAMS) {
        return 0;
    }
    if (mic_info[stream_id].mic_dma_buffer) {
        if (!mic_info[stream_id].mic_mute) {
            return vol_convert(mic_info[stream_id].mic_vol);
        }
    }
    return 0;
}

static u32 uac_get_mic_sameplerate(void *priv)
{
    u32 sample_rate = MIC_AUDIO_RATE;
    if (mic_info && mic_info[0].mic_dma_buffer) {
        sample_rate = mic_info[0].mic_sampling_rate;
    }
    return sample_rate;
}

static u32 uac_rate_clamp(u32 rate, const u8 *rate_table)
{
    u8 num = rate_table[0];
    u32 closest = 0;
    u32 min_diff = 0xFFFFFFFF;

    for (u8 i = 0; i < num; i++) {
        u32 r;
        //dMIN at offset 2+12*i
        memcpy(&r, &rate_table[2 + 12 * i], 4);
        if (rate == r) {
            return rate;
        }
        u32 diff = (rate > r) ? (rate - r) : (r - rate);
        if (diff < min_diff) {
            min_diff = diff;
            closest = r;
        }
    }
    return closest;
}

static u32 uac_vol_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[64];
    u8 mute;
    u16 volume = 0;

    u8 unit_id = HIBYTE(setup->wIndex);
    /* Clock Source IDs 20-31 */
    u8 clock_id_spk0 = CLOCK_ID_SPK(0);
    u8 clock_id_mic0 = CLOCK_ID_MIC(0);

    usb_read_ep0(usb_id, read_ep, setup->wLength);
    ret = USB_EP0_STAGE_SETUP;

    if (!spk_info || !mic_info) {
        return ret;
    }

    if (unit_id >= clock_id_mic0) {
        /* MIC Clock Source */
        u8 stream_id = (unit_id - clock_id_mic0) / 10;
        if (stream_id >= UAC_MAX_STREAMS) {
            return USB_EP0_SET_STALL;
        }
        if (!mic_info[stream_id].mic_dma_buffer) {
            return ret;
        }
        switch (HIBYTE(setup->wValue)) {
        case UAC2_CS_CONTROL_SAM_FREQ: {
            u32 freq;
            memcpy(&freq, read_ep, 4);
            freq = uac_rate_clamp(freq, mic_default_sampling_freq_table);
            mic_info[stream_id].mic_sampling_rate = freq;
            log_info("Clock Source Control Set Sampling Frequency to %d", mic_info[stream_id].mic_sampling_rate);
            break;
        }
        case UAC2_CS_CONTROL_CLOCK_VALID:
            log_debug("Clock Source Control Set Valid to %d", read_ep[0]);
            break;
        }
        return ret;
    }

    if (unit_id >= clock_id_spk0) {
        /* SPK Clock Source */
        u8 stream_id = (unit_id - clock_id_spk0) / 10;
        if (stream_id >= UAC_MAX_STREAMS) {
            return USB_EP0_SET_STALL;
        }
        if (!spk_info[stream_id].spk_dma_buffer) {
            return ret;
        }
        switch (HIBYTE(setup->wValue)) {
        case UAC2_CS_CONTROL_SAM_FREQ: {
            u32 freq;
            memcpy(&freq, read_ep, 4);
            freq = uac_rate_clamp(freq, speaker_default_sampling_freq_table);
            spk_info[stream_id].spk_sampling_rate = freq;
            log_info("Clock Source Control Set Sampling Frequency to %d", spk_info[stream_id].spk_sampling_rate);
            break;
        }
        case UAC2_CS_CONTROL_CLOCK_VALID:
            log_debug("Clock Source Control Set Valid to %d", read_ep[0]);
            break;
        }
        return ret;
    }

    /* Feature Unit / Selector Unit handling */
    u8 stream_id = UAC_UNIT_ID_STREAM(unit_id);
    int is_mic = UAC_UNIT_ID_IS_MIC(unit_id);
    int is_spk_selector = 0;
    int is_mic_selector = 0;

    if (is_mic) {
        is_mic_selector = (unit_id == MIC_SELECTOR_UNIT_ID(stream_id));
    } else {
        is_spk_selector = (unit_id == SPK_SELECTOR_UNIT_ID(stream_id));
    }

    if (stream_id >= UAC_MAX_STREAMS) {
        return USB_EP0_SET_STALL;
    }

    if (is_spk_selector) {
        if (!spk_info[stream_id].spk_dma_buffer) {
            return ret;
        }
        switch (HIBYTE(setup->wValue)) {
        case 0:
            log_debug("SPK SELECTOR_UNIT_ID");
            spk_info[stream_id].ps4_mode = read_ep[0];
            if (read_ep[0]) {
                spk_info[stream_id].spk_left_vol = vol_convert(100);
                spk_info[stream_id].spk_right_vol = vol_convert(100);
                uac_mute_volume(SPK_FEATURE_UNIT_ID(stream_id), 100, 100);
            }
            break;
        }
        return ret;
    }

    if (is_mic_selector) {
        if (!mic_info[stream_id].mic_dma_buffer) {
            return ret;
        }
        switch (HIBYTE(setup->wValue)) {
        case 0:
            log_debug("MIC SELECTOR_UNIT_ID");
            break;
        }
        return ret;
    }

    switch (HIBYTE(setup->wValue)) {
    case UAC_FU_MUTE:
        if (!is_mic) {
            if (!spk_info[stream_id].spk_dma_buffer) {
                break;
            }
            memcpy((u8 *)&mute, read_ep, 1);
            if (mute) {
                uac_mute_volume(SPK_FEATURE_UNIT_ID(stream_id), 0, 0);
                spk_info[stream_id].spk_mute = 1;
            } else {
                uac_mute_volume(SPK_FEATURE_UNIT_ID(stream_id),
                                vol_convert(spk_info[stream_id].spk_left_vol),
                                vol_convert(spk_info[stream_id].spk_right_vol));
                spk_info[stream_id].spk_mute = 0;
            }
        } else {
            if (!mic_info[stream_id].mic_dma_buffer) {
                break;
            }
            memcpy((u8 *)&mute, read_ep, 1);
            if (mute) {
                uac_mute_volume(MIC_FEATURE_UNIT_ID(stream_id), 0, 0);
                mic_info[stream_id].mic_mute = 1;
            } else {
                uac_mute_volume(MIC_FEATURE_UNIT_ID(stream_id), vol_convert(mic_info[stream_id].mic_vol), 0);
                mic_info[stream_id].mic_mute = 0;
            }
        }
        break;
    case UAC_FU_VOLUME:
        if (!is_mic) {
            if (!spk_info[stream_id].spk_dma_buffer) {
                break;
            }
            memcpy((u8 *)&volume, read_ep, setup->wLength);
            if (LOBYTE(setup->wValue) == 0) {
                spk_info[stream_id].spk_left_vol = volume;
                spk_info[stream_id].spk_right_vol = volume;
            } else if (LOBYTE(setup->wValue) == 1) {
                spk_info[stream_id].spk_left_vol = volume;
            } else if (LOBYTE(setup->wValue) == 2) {
                spk_info[stream_id].spk_right_vol = volume;
            }
            if (spk_info[stream_id].ps4_mode) {
                spk_info[stream_id].spk_left_vol = vol_convert(100);
                spk_info[stream_id].spk_right_vol = vol_convert(100);
            }
            if (usb_get_host_type(usb_id) == HOST_TYPE_ANDROID) {
                //安卓没有音量同步，需要把uac端音量设最大
                spk_info[stream_id].spk_left_vol = vol_convert(100);
                spk_info[stream_id].spk_right_vol = vol_convert(100);
            }
            if (!spk_info[stream_id].spk_mute && !spk_info[stream_id].ps4_mode) {
                uac_mute_volume(SPK_FEATURE_UNIT_ID(stream_id),
                                vol_convert(spk_info[stream_id].spk_left_vol),
                                vol_convert(spk_info[stream_id].spk_right_vol));
            }
        } else {
            if (!mic_info[stream_id].mic_dma_buffer) {
                break;
            }
            memcpy((u8 *)&mic_info[stream_id].mic_vol, read_ep, sizeof(mic_info[stream_id].mic_vol));
            if (!mic_info[stream_id].mic_mute) {
                uac_mute_volume(MIC_FEATURE_UNIT_ID(stream_id), vol_convert(mic_info[stream_id].mic_vol), 0);
            }
        }
        break;
    case UAC_FU_AUTOMATIC_GAIN:
        if (is_mic) {
            if (!mic_info[stream_id].mic_dma_buffer) {
                break;
            }
            if (read_ep[0]) {
                mic_info[stream_id].bAGC = 1;
            } else {
                mic_info[stream_id].bAGC = 0;
            }
        }
        break;

    default :
        ret = USB_EP0_SET_STALL;
        break;
    }

    return ret;
}
#if USB_ROOT2
static u8 SetInterface_0_Lock;
#endif
static u32 audio_ac_itf_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    u8 mute;

    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;
    u8 uac2_range_param[2 + 12];  //layout 3 parameter block

    u8 unit_id = HIBYTE(setup->wIndex);
    u8 clock_id_spk0 = CLOCK_ID_SPK(0);
    u8 clock_id_mic0 = CLOCK_ID_MIC(0);
    u8 stream_id;
    int is_mic = 0;

    if (unit_id >= clock_id_mic0) {
        stream_id = (unit_id - clock_id_mic0) / 10;
    } else if (unit_id >= clock_id_spk0) {
        stream_id = (unit_id - clock_id_spk0) / 10;
    } else {
        stream_id = UAC_UNIT_ID_STREAM(unit_id);
        is_mic = UAC_UNIT_ID_IS_MIC(unit_id);
    }

    if (!spk_info || !mic_info) {
        return ret;
    }

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (setup->bRequest) {
        case USB_REQ_SET_INTERFACE:
#if USB_ROOT2
            SetInterface_0_Lock = 1;
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);//no alt setting
            }
#else
            if (setup->wValue == 0) { //alt 0
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
#endif
            break;
        case USB_REQ_GET_INTERFACE:
#if USB_ROOT2
            if (usb_root2_testing()) {
                SetInterface_0_Lock = 0;
            }
#endif
            if (setup->wValue || (setup->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        default:
            ret = 1 ;
            break;
        }
        break;

    case USB_TYPE_CLASS:
        switch (setup->bRequest) {
        case UAC2_CS_CUR:
            if ((setup->bRequestType & USB_DIR_IN) == 0) {
                usb_set_setup_recv(usb_device, uac_vol_handler);
            } else {
                if (unit_id >= clock_id_mic0) {
                    if (stream_id >= UAC_MAX_STREAMS || !mic_info[stream_id].mic_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC2_CS_CONTROL_SAM_FREQ:
                        tx_payload = (u8 *)&mic_info[stream_id].mic_sampling_rate;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    case UAC2_CS_CONTROL_CLOCK_VALID:
                        tx_payload[0] = 1;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                if (unit_id >= clock_id_spk0) {
                    if (stream_id >= UAC_MAX_STREAMS || !spk_info[stream_id].spk_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC2_CS_CONTROL_SAM_FREQ:
                        tx_payload = (u8 *)&spk_info[stream_id].spk_sampling_rate;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    case UAC2_CS_CONTROL_CLOCK_VALID:
                        tx_payload[0] = 1;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                if (is_mic) {
                    if (stream_id >= UAC_MAX_STREAMS || !mic_info[stream_id].mic_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC_FU_MUTE:
                        mute = mic_info[stream_id].mic_mute ;
                        tx_payload = &mute;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    case UAC_FU_VOLUME:
                        tx_payload = (u8 *)&mic_info[stream_id].mic_vol;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    case UAC_FU_AUTOMATIC_GAIN:
                        if (mic_info[stream_id].bAGC) {
                            tx_payload[0] = 0x01;
                        } else {
                            tx_payload[0] = 0x00;
                        }
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                /* SPK Feature Unit GET_CUR */
                if (stream_id >= UAC_MAX_STREAMS || !spk_info[stream_id].spk_dma_buffer) {
                    ret = 1;
                    break;
                }
                switch (HIBYTE(setup->wValue)) {
                case UAC_FU_MUTE:
                    mute = spk_info[stream_id].spk_mute ;
                    tx_payload = &mute;
                    tx_len = setup->wLength;
                    usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                    break;
                case UAC_FU_VOLUME:
                    if (LOBYTE(setup->wValue) == 0) {
                        tx_payload = (u8 *)&spk_info[stream_id].spk_left_vol;
                    } else if (LOBYTE(setup->wValue) == 1) {
                        tx_payload = (u8 *)&spk_info[stream_id].spk_left_vol;
                    } else if (LOBYTE(setup->wValue) == 2) {
                        tx_payload = (u8 *)&spk_info[stream_id].spk_right_vol;
                    }
                    tx_len = setup->wLength;
                    usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                    break;
                default:
                    ret = 1 ;
                    break;
                }
            }
            break;

        case UAC2_CS_RANGE:
            if ((setup->bRequestType & USB_DIR_IN) == 0) {

            } else {
                if (unit_id >= clock_id_mic0) {
                    if (stream_id >= UAC_MAX_STREAMS || !mic_info[stream_id].mic_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC2_CS_CONTROL_SAM_FREQ:
                        memcpy(tx_payload,
                               mic_default_sampling_freq_table,
                               sizeof(mic_default_sampling_freq_table));
                        tx_len = sizeof(mic_default_sampling_freq_table);
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                if (unit_id >= clock_id_spk0) {
                    if (stream_id >= UAC_MAX_STREAMS || !spk_info[stream_id].spk_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC2_CS_CONTROL_SAM_FREQ:
                        memcpy(tx_payload,
                               speaker_default_sampling_freq_table,
                               sizeof(speaker_default_sampling_freq_table));
                        tx_len = sizeof(speaker_default_sampling_freq_table);
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                if (is_mic) {
                    if (stream_id >= UAC_MAX_STREAMS || !mic_info[stream_id].mic_dma_buffer) {
                        ret = 1;
                        break;
                    }
                    switch (HIBYTE(setup->wValue)) {
                    case UAC_FU_VOLUME:
                        memset(uac2_range_param, 0, sizeof(uac2_range_param));
                        uac2_range_param[0] = 1;
                        memcpy(&uac2_range_param[2], &mic_info[stream_id].mic_min_vol, 2);
                        memcpy(&uac2_range_param[4], &mic_info[stream_id].mic_max_vol, 2);
                        memcpy(&uac2_range_param[6], &mic_info[stream_id].mic_vol_res, 2);
                        tx_payload = (u8 *)uac2_range_param;
                        tx_len = setup->wLength;
                        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                        break;
                    default:
                        ret = 1 ;
                        break;
                    }
                    break;
                }

                /* SPK Feature Unit RANGE */
                if (stream_id >= UAC_MAX_STREAMS || !spk_info[stream_id].spk_dma_buffer) {
                    ret = 1;
                    break;
                }
                switch (HIBYTE(setup->wValue)) {
                case UAC_FU_VOLUME:
                    memset(uac2_range_param, 0, sizeof(uac2_range_param));
                    uac2_range_param[0] = 1;
                    memcpy(&uac2_range_param[2], &spk_info[stream_id].spk_min_vol, 2);
                    memcpy(&uac2_range_param[4], &spk_info[stream_id].spk_max_vol, 2);
                    memcpy(&uac2_range_param[6], &spk_info[stream_id].spk_vol_res, 2);
                    tx_payload = (u8 *)uac2_range_param;
                    tx_len = setup->wLength;
                    usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
                    break;
                default:
                    ret = 1 ;
                    break;
                }
            }
            break;

        default:
            ret = 1 ;
            break;
        }
        break;

    default:
        ret = 1 ;
        break;
    }
    return ret;
}


static u32 spk_get_max_sampling_rate(const struct uac_stream_config *cfg)
{
    u32 sr = 0;
    for (int i = 0; i < cfg->spk_rate_num; i++) {
        if (sr < cfg->spk_rates[i]) {
            sr = cfg->spk_rates[i];
        }
    }
    return sr;
}
// 按给定位宽计算 1ms 内的最大数据字节数（endpoint wMaxPacketSize 用）
static u32 spk_get_frame_len_by_res(const struct uac_stream_config *cfg, u32 res)
{
    u32 sr = spk_get_max_sampling_rate(cfg);

#if UAC_24BIT_IN_4BYTE
    if (res == 24) {
        res = 32;
    }
#endif
    u32 byte_per_sample = res / 8;
    u32 len = sr * byte_per_sample * cfg->spk_channels / 1000;
    len += (sr % 1000) ? byte_per_sample * cfg->spk_channels : 0;
    return len;
}

// 全局最大 frame len（EP buffer 分配用，取所有 alt 里最大的位宽）
static u32 spk_get_max_frame_len(const struct uac_stream_config *cfg)
{
    u32 res = cfg->spk_res;
    if (cfg->spk_res_2 > res) {
        res = cfg->spk_res_2;
    }
    return spk_get_frame_len_by_res(cfg, res);
}
static u32 mic_get_max_sampling_rate(const struct uac_stream_config *cfg)
{
    u32 sr = 0;
    for (int i = 0; i < cfg->mic_rate_num; i++) {
        if (sr < cfg->mic_rates[i]) {
            sr = cfg->mic_rates[i];
        }
    }
    return sr;
}
// 按给定位宽计算 1ms 内的最大数据字节数（endpoint wMaxPacketSize 用）
static u32 mic_get_frame_len_by_res(const struct uac_stream_config *cfg, u32 res)
{
    u32 sr = mic_get_max_sampling_rate(cfg);

#if UAC_24BIT_IN_4BYTE
    if (res == 24) {
        res = 32;
    }
#endif
    u32 byte_per_sample = res / 8;
    u32 len = sr * byte_per_sample * cfg->mic_channels / 1000;
    len += (sr % 1000) ? byte_per_sample * cfg->mic_channels : 0;
    return len;
}

// 全局最大 frame len（EP buffer 分配用，取所有 alt 里最大的位宽）
static u32 mic_get_max_frame_len(const struct uac_stream_config *cfg)
{
    u32 res = cfg->mic_res;
    if (cfg->mic_res_2 > res) {
        res = cfg->mic_res_2;
    }
    return mic_get_frame_len_by_res(cfg, res);
}

/* ---------- Descriptor builder helpers ---------- */

static inline void bld_put_le16(u16 val, u8 *buf)
{
    buf[0] = (u8)(val & 0xFF);
    buf[1] = (u8)((val >> 8) & 0xFF);
}

static inline void bld_put_le32(u32 val, u8 *buf)
{
    buf[0] = (u8)(val & 0xFF);
    buf[1] = (u8)((val >> 8) & 0xFF);
    buf[2] = (u8)((val >> 16) & 0xFF);
    buf[3] = (u8)((val >> 24) & 0xFF);
}

/* ---------- Clock Source builder ---------- */

static u8 *build_clock_source(u8 *buf, u8 clock_id, u8 rate_num)
{
    buf[0] = 8;
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC2_CLOCK_SOURCE;
    buf[3] = clock_id;
    buf[4] = 0x00;       /* bmAttributes: synchronized to USB SOF */
    buf[5] = (rate_num > 1) ? 0x03 : 0x00;  /* bmControls: freq+valid control if multiple rates */
    buf[6] = 0x00;
    buf[7] = 0x00;
    return buf + 8;
}

/* ---------- UAC2 Topology builder functions ---------- */

static u8 *build_spk_input_terminal_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    buf[0] = 0x11;             // bLength: 17
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_INPUT_TERMINAL;
    buf[3] = SPK_INPUT_TERMINAL_ID(stream_id);
    bld_put_le16(UAC_TERMINAL_STREAMING, &buf[4]);
    buf[6] = 0x00;             // bAssocTerminal
    buf[7] = CLOCK_ID_SPK(stream_id);  // bCSourceID
    buf[8] = cfg->spk_channels;
    bld_put_le32(cfg->spk_channel_config, &buf[9]);
    buf[13] = 0x00;            // iChannelNames
    bld_put_le16(0x0000, &buf[14]); // bmControls
    buf[16] = cfg->spk_str_index;  // iTerminal
    return buf + 17;
}

static u8 *build_spk_feature_unit_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    u8 ch = cfg->spk_channels;
    buf[0] = 6 + (ch + 1) * 4; // bLength
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_FEATURE_UNIT;
    buf[3] = SPK_FEATURE_UNIT_ID(stream_id);
    buf[4] = SPK_INPUT_TERMINAL_ID(stream_id);
    // bmaControls: channel 0 (mute) + per-channel (volume)
    for (int i = 0; i <= ch; i++) {
        u32 ctl = (i == 0) ? UAC_V2_FU_CONTROL_MUTE : UAC_V2_FU_CONTROL_VOLUME;
        bld_put_le32(ctl, &buf[5 + i * 4]);
    }
    buf[5 + (ch + 1) * 4] = 0x00; // iFeature
    return buf + 6 + (ch + 1) * 4;
}

static u8 *build_spk_output_terminal_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    buf[0] = 0x0c;             // bLength: 12
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_OUTPUT_TERMINAL;
    buf[3] = SPK_OUTPUT_TERMINAL_ID(stream_id);
    bld_put_le16(UAC_OUTPUT_TERMINAL_SPEAKER, &buf[4]);
    buf[6] = 0x00;             // bAssocTerminal
    buf[7] = SPK_FEATURE_UNIT_ID(stream_id);  // bSourceID
    buf[8] = CLOCK_ID_SPK(stream_id);  // bCSourceID
    bld_put_le16(0x0000, &buf[9]); // bmControls
    buf[11] = cfg->spk_str_index;  // iTerminal
    return buf + 12;
}

static u8 *build_mic_input_terminal_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    buf[0] = 0x11;             // bLength: 17
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_INPUT_TERMINAL;
    buf[3] = MIC_INPUT_TERMINAL_ID(stream_id);
    bld_put_le16(UAC_INPUT_TERMINAL_MICROPHONE, &buf[4]);
    buf[6] = 0x00;             // bAssocTerminal
    buf[7] = CLOCK_ID_MIC(stream_id);  // bCSourceID
    buf[8] = cfg->mic_channels;
    bld_put_le32(cfg->mic_channel_config, &buf[9]);
    buf[13] = 0x00;            // iChannelNames
    bld_put_le16(0x0000, &buf[14]); // bmControls
    buf[16] = cfg->mic_str_index;  // iTerminal
    return buf + 17;
}

static u8 *build_mic_feature_unit_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    u8 ch = cfg->mic_channels;
    buf[0] = 6 + (ch + 1) * 4; // bLength
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_FEATURE_UNIT;
    buf[3] = MIC_FEATURE_UNIT_ID(stream_id);
    buf[4] = MIC_INPUT_TERMINAL_ID(stream_id);
    // bmaControls: channel 0 (mute+volume) + per-channel (0)
    for (int i = 0; i <= ch; i++) {
        u32 ctl = (i == 0) ? (UAC_V2_FU_CONTROL_MUTE | UAC_V2_FU_CONTROL_VOLUME) : 0;
        bld_put_le32(ctl, &buf[5 + i * 4]);
    }
    buf[5 + (ch + 1) * 4] = 0x00; // iFeature
    return buf + 6 + (ch + 1) * 4;
}

static u8 *build_mic_output_terminal_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 stream_id)
{
    buf[0] = 0x0c;             // bLength: 12
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_OUTPUT_TERMINAL;
    buf[3] = MIC_OUTPUT_TERMINAL_ID(stream_id);
    bld_put_le16(UAC_TERMINAL_STREAMING, &buf[4]);
    buf[6] = 0x00;             // bAssocTerminal
    buf[7] = MIC_FEATURE_UNIT_ID(stream_id);  // bSourceID
    buf[8] = CLOCK_ID_MIC(stream_id);  // bCSourceID
    bld_put_le16(0x0000, &buf[9]); // bmControls
    buf[11] = cfg->mic_str_index;  // iTerminal
    return buf + 12;
}

/* ---------- UAC2 Format builder ---------- */

static u8 *build_spk_format_type_i_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 alt_setting)
{
    u8 res = (alt_setting == 1) ? cfg->spk_res : cfg->spk_res_2;
    u8 subframe = res / 8;
#if UAC_24BIT_IN_4BYTE
    if (res == 24) {
        subframe = 4;
    }
#endif

    buf[0] = 0x06;
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_FORMAT_TYPE;
    buf[3] = UAC_FORMAT_TYPE_I;
    buf[4] = subframe;
    buf[5] = res;
    return buf + 6;
}

static u8 *build_mic_format_type_i_uac2(u8 *buf, const struct uac_stream_config *cfg, u8 alt_setting)
{
    u8 res = (alt_setting == 1) ? cfg->mic_res : cfg->mic_res_2;
    u8 subframe = res / 8;
#if UAC_24BIT_IN_4BYTE
    if (res == 24) {
        subframe = 4;
    }
#endif

    buf[0] = 0x06;
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_FORMAT_TYPE;
    buf[3] = UAC_FORMAT_TYPE_I;
    buf[4] = subframe;
    buf[5] = res;
    return buf + 6;
}

/* ---------- UAC2 AS descriptor builder ---------- */

static u8 *build_spk_as_descriptors_uac2(u8 *buf, u8 if_num,
        const struct uac_stream_config *cfg,
        u8 term_link, u8 clock_id)
{
    /* Alt 0 (zero-bandwidth) */
    buf[0] = 9;
    buf[1] = USB_DT_INTERFACE;
    buf[2] = if_num;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = USB_CLASS_AUDIO;
    buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
    buf[7] = UAC_VERSION_2;
    buf[8] = cfg->spk_str_index;
    buf += 9;

    /* Alt 1 (active) */
    buf[0] = 9;
    buf[1] = USB_DT_INTERFACE;
    buf[2] = if_num;
    buf[3] = 1;
    buf[4] = 1;
    buf[5] = USB_CLASS_AUDIO;
    buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
    buf[7] = UAC_VERSION_2;
    buf[8] = cfg->spk_str_index;
    buf += 9;

    /* AS_GENERAL (16 bytes for UAC2) */
    buf[0] = 0x10;
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_AS_GENERAL;
    buf[3] = term_link;
    buf[4] = 0x01;             // bmControls
    buf[5] = UAC_FORMAT_TYPE_I; // bFormatType
    bld_put_le32(UAC_V2_FORMAT_TYPE_I_PCM, &buf[6]); // bmFormat
    buf[10] = cfg->spk_channels;
    bld_put_le32(cfg->spk_channel_config, &buf[11]);
    buf[15] = 0;               // iChannelNames
    buf += 16;

    /* FORMAT_TYPE_I alt 1 (6 bytes for UAC2) */
    buf = build_spk_format_type_i_uac2(buf, cfg, 1);

    /* ISO EP OUT alt 1 (7 bytes for UAC2) */
    u32 max_pkt = spk_get_frame_len_by_res(cfg, cfg->spk_res);
    buf[0] = 0x07;
    buf[1] = USB_DT_ENDPOINT;
    buf[2] = USB_DIR_OUT | cfg->spk_ep;
    buf[3] = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE;
    bld_put_le16(max_pkt, &buf[4]);
    buf[6] = UAC_ISO_INTERVAL;
    buf += 7;

    /* CS Endpoint alt 1 (8 bytes for UAC2) */
    buf[0] = 0x08;
    buf[1] = USB_DT_CS_ENDPOINT;
    buf[2] = UAC_AS_GENERAL;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf += 8;

    if (cfg->spk_res_2) {
        /* Alt 2 (alternate resolution) */
        buf[0] = 9;
        buf[1] = USB_DT_INTERFACE;
        buf[2] = if_num;
        buf[3] = 2;
        buf[4] = 1;
        buf[5] = USB_CLASS_AUDIO;
        buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
        buf[7] = UAC_VERSION_2;
        buf[8] = cfg->spk_str_index;
        buf += 9;

        /* AS_GENERAL alt 2 */
        buf[0] = 0x10;
        buf[1] = USB_DT_CS_INTERFACE;
        buf[2] = UAC_AS_GENERAL;
        buf[3] = term_link;
        buf[4] = 0x01;
        buf[5] = UAC_FORMAT_TYPE_I;
        bld_put_le32(UAC_V2_FORMAT_TYPE_I_PCM, &buf[6]);
        buf[10] = cfg->spk_channels;
        bld_put_le32(cfg->spk_channel_config, &buf[11]);
        buf[15] = 0;
        buf += 16;

        /* FORMAT_TYPE_I alt 2 */
        buf = build_spk_format_type_i_uac2(buf, cfg, 2);

        /* ISO EP OUT alt 2 */
        max_pkt = spk_get_frame_len_by_res(cfg, cfg->spk_res_2);
        buf[0] = 0x07;
        buf[1] = USB_DT_ENDPOINT;
        buf[2] = USB_DIR_OUT | cfg->spk_ep;
        buf[3] = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE;
        bld_put_le16(max_pkt, &buf[4]);
        buf[6] = UAC_ISO_INTERVAL;
        buf += 7;

        /* CS Endpoint alt 2 */
        buf[0] = 0x08;
        buf[1] = USB_DT_CS_ENDPOINT;
        buf[2] = UAC_AS_GENERAL;
        buf[3] = 0x00;
        buf[4] = 0x00;
        buf[5] = 0x00;
        buf[6] = 0x00;
        buf[7] = 0x00;
        buf += 8;
    }

    return buf;
}

static u8 *build_mic_as_descriptors_uac2(u8 *buf, u8 if_num,
        const struct uac_stream_config *cfg,
        u8 term_link, u8 clock_id)
{
    /* Alt 0 (zero-bandwidth) */
    buf[0] = 9;
    buf[1] = USB_DT_INTERFACE;
    buf[2] = if_num;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = USB_CLASS_AUDIO;
    buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
    buf[7] = UAC_VERSION_2;
    buf[8] = cfg->mic_str_index;
    buf += 9;

    /* Alt 1 (active) */
    buf[0] = 9;
    buf[1] = USB_DT_INTERFACE;
    buf[2] = if_num;
    buf[3] = 1;
    buf[4] = 1;
    buf[5] = USB_CLASS_AUDIO;
    buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
    buf[7] = UAC_VERSION_2;
    buf[8] = cfg->mic_str_index;
    buf += 9;

    /* AS_GENERAL (16 bytes for UAC2) */
    buf[0] = 0x10;
    buf[1] = USB_DT_CS_INTERFACE;
    buf[2] = UAC_AS_GENERAL;
    buf[3] = term_link;
    buf[4] = 0x01;             // bmControls
    buf[5] = UAC_FORMAT_TYPE_I; // bFormatType
    bld_put_le32(UAC_V2_FORMAT_TYPE_I_PCM, &buf[6]); // bmFormat
    buf[10] = cfg->mic_channels;
    bld_put_le32(cfg->mic_channel_config, &buf[11]);
    buf[15] = 0;               // iChannelNames
    buf += 16;

    /* FORMAT_TYPE_I alt 1 (6 bytes for UAC2) */
    buf = build_mic_format_type_i_uac2(buf, cfg, 1);

    /* ISO EP IN alt 1 (7 bytes for UAC2) */
    u32 max_pkt = mic_get_frame_len_by_res(cfg, cfg->mic_res);
    buf[0] = 0x07;
    buf[1] = USB_DT_ENDPOINT;
    buf[2] = USB_DIR_IN | cfg->mic_ep;
    buf[3] = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC;
    bld_put_le16(max_pkt, &buf[4]);
    buf[6] = UAC_ISO_INTERVAL;
    buf += 7;

    /* CS Endpoint alt 1 (8 bytes for UAC2) */
    buf[0] = 0x08;
    buf[1] = USB_DT_CS_ENDPOINT;
    buf[2] = UAC_AS_GENERAL;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf += 8;

    if (cfg->mic_res_2) {
        /* Alt 2 (alternate resolution) */
        buf[0] = 9;
        buf[1] = USB_DT_INTERFACE;
        buf[2] = if_num;
        buf[3] = 2;
        buf[4] = 1;
        buf[5] = USB_CLASS_AUDIO;
        buf[6] = USB_SUBCLASS_AUDIOSTREAMING;
        buf[7] = UAC_VERSION_2;
        buf[8] = cfg->mic_str_index;
        buf += 9;

        /* AS_GENERAL alt 2 */
        buf[0] = 0x10;
        buf[1] = USB_DT_CS_INTERFACE;
        buf[2] = UAC_AS_GENERAL;
        buf[3] = term_link;
        buf[4] = 0x01;
        buf[5] = UAC_FORMAT_TYPE_I;
        bld_put_le32(UAC_V2_FORMAT_TYPE_I_PCM, &buf[6]);
        buf[10] = cfg->mic_channels;
        bld_put_le32(cfg->mic_channel_config, &buf[11]);
        buf[15] = 0;
        buf += 16;

        /* FORMAT_TYPE_I alt 2 */
        buf = build_mic_format_type_i_uac2(buf, cfg, 2);

        /* ISO EP IN alt 2 */
        max_pkt = mic_get_frame_len_by_res(cfg, cfg->mic_res_2);
        buf[0] = 0x07;
        buf[1] = USB_DT_ENDPOINT;
        buf[2] = USB_DIR_IN | cfg->mic_ep;
        buf[3] = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC;
        bld_put_le16(max_pkt, &buf[4]);
        buf[6] = UAC_ISO_INTERVAL;
        buf += 7;

        /* CS Endpoint alt 2 */
        buf[0] = 0x08;
        buf[1] = USB_DT_CS_ENDPOINT;
        buf[2] = UAC_AS_GENERAL;
        buf[3] = 0x00;
        buf[4] = 0x00;
        buf[5] = 0x00;
        buf[6] = 0x00;
        buf[7] = 0x00;
        buf += 8;
    }

    return buf;
}

/* 根据 EP 号反查 stream_id（去方向位后匹配 g_stream_configs） */
static u8 uac_spk_stream_id_from_ep(u32 ep)
{
    u8 e = ep & 0x7F;
    for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
        if (UAC_CFG_HAS_SPK(&g_stream_configs[i]) && g_stream_configs[i].spk_ep == e) {
            return i;
        }
    }
    return 0;
}

static u8 uac_mic_stream_id_from_ep(u32 ep)
{
    u8 e = ep & 0x7F;
    for (u8 i = 0; i < UAC_MAX_STREAMS; i++) {
        if (UAC_CFG_HAS_MIC(&g_stream_configs[i]) && g_stream_configs[i].mic_ep == e) {
            return i;
        }
    }
    return 0;
}

static void spk_transfer(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 spk_res = 0;
    u32 spk_frame_len;
    u32 rx_len;
    u8 stream_id = uac_spk_stream_id_from_ep(ep);

    if (!spk_info || !spk_info[stream_id].spk_dma_buffer) {
        return;
    }

    if (usb_ep_rx_is_unmask_irq(usb_id, ep) == 0) {
        ep_buffer = spk_info[stream_id].spk_dma_buffer;
    }
    spk_res = spk_info[stream_id].res;
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 24) {
        spk_res = 32;
    }
#endif
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    spk_frame_len = spk_info[stream_id].spk_sampling_rate * (spk_res / 8) * cfg->spk_channels / 1000;
    spk_frame_len += ((spk_info[stream_id].spk_sampling_rate % 1000) ? (spk_res / 8) * cfg->spk_channels : 0);

    if (usb_ep_rx_is_unmask_irq(usb_id, ep) == 0) {
        rx_len = usb_g_iso_read(usb_id, cfg->spk_ep, NULL, spk_frame_len, 0);
        uac_speaker_stream_write(ep_buffer, rx_len);
    } else {
        u8 frame_buf[spk_frame_len];
        ep_buffer = frame_buf;
        for (int i = 0; i < 3; i++) {
            rx_len = usb_g_iso_read(usb_id, cfg->spk_ep, ep_buffer, spk_frame_len, 0);
            if (rx_len == 0) {
                break;
            }
            uac_speaker_stream_write(ep_buffer, rx_len);
        }
    }

}
static void spk_feed_data_when_downstream_halt(struct usb_device_t *usb_device, u32 ep, u32 frame)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 spk_res;
    u32 spk_frame_len;
    u32 padding_len = 0;
    u8 stream_id = uac_spk_stream_id_from_ep(ep);

    if (!spk_info || !spk_info[stream_id].spk_dma_buffer) {
        return;
    }
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    //由于uac_speaker_stream_close_delay()的机制，实际上音频流不会
    //立刻关闭，如果这段时间内又open，中间缺失了数据，给到音频流的
    //数据量会不够，变采样处理不过来声音就会变调。所以需要手工补够
    //数据
    if (uac_speaker_stream_status_ex(stream_id) == 0) {
        usb_g_set_sof_hander(usb_id, cfg->spk_ep, NULL);
        return;
    }
#ifdef USB_HW_20
#if FUSB_MODE == 0
    //接到high-speed host时每8个微帧补偿一次；接到full-speed host时每个帧补偿
    //一次
    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        if ((++spk_info[stream_id].sof_cnt % 8) != 0) {
            return;
        }
    }
#endif
#endif
    spk_res = spk_info[stream_id].res;
    if (spk_info[stream_id].spk_padding_interval) {
        if (++spk_info[stream_id].spk_padding_cnt == spk_info[stream_id].spk_padding_interval) {
            spk_info[stream_id].spk_padding_cnt = 0;
            padding_len = (spk_res / 8) * cfg->spk_channels;
        }
    }
    ep_buffer = spk_info[stream_id].spk_dma_buffer;
    if (spk_info[stream_id].downstream_halt >= 5) {
        spk_info[stream_id].downstream_halt = 5;  //avoid numerical overflow
        spk_frame_len = spk_info[stream_id].spk_sampling_rate * (spk_res / 8) * cfg->spk_channels / 1000;
        spk_frame_len += padding_len;
        memset(ep_buffer, 0, spk_frame_len);
        uac_speaker_stream_write(ep_buffer, spk_frame_len);
    }
    spk_info[stream_id].downstream_halt++;
}
static void open_spk(struct usb_device_t *usb_device, u8 stream_id)
{
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 spk_frame_len;
    u8 *ep_buffer;
    u32 spk_res = 0;

    if (!spk_info || !spk_info[stream_id].spk_dma_buffer) {
        return;
    }
    log_info("open_spk stream=%d", stream_id);
    if (spk_info[stream_id].spk_sampling_rate == 0) {
        spk_info[stream_id].spk_sampling_rate = spk_get_max_sampling_rate(cfg);
    }
    if (spk_info[stream_id].alternate_setting == 1) {
        spk_res = cfg->spk_res;
    } else if (spk_info[stream_id].alternate_setting == 2) {
        spk_res = cfg->spk_res_2;
    } else {
        return;
    }
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 24) {
        spk_res = 32;
    }
#endif
    if (spk_info[stream_id].spk_sampling_rate % 1000) {
        spk_info[stream_id].spk_padding_interval = 1000 / (spk_info[stream_id].spk_sampling_rate % 1000);
        spk_info[stream_id].spk_padding_cnt = 0;
    } else {
        spk_info[stream_id].spk_padding_interval = 0;
        spk_info[stream_id].spk_padding_cnt = 0;
    }
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 32) {
        spk_res = 24;
    }
#endif
    spk_info[stream_id].res = spk_res;
    ep_buffer = spk_info[stream_id].spk_dma_buffer;
    uac_speaker_stream_open_ex(stream_id, spk_info[stream_id].spk_sampling_rate,
                               cfg->spk_channels, spk_res);


    /* usb_g_set_intr_hander(usb_id, SPK_ISO_EP_OUT | USB_DIR_OUT, spk_transfer); */


    spk_frame_len = spk_get_max_frame_len(cfg);
    usb_g_ep_config_v2(usb_id, cfg->spk_ep | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, spk_frame_len, spk_frame_len * UAC_MAX_PKTS_PER_SOF);

    usb_enable_ep(usb_id, cfg->spk_ep);
}
static void close_spk(struct usb_device_t *usb_device, u8 stream_id)
{
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];

    log_info("close_spk stream=%d", stream_id);
    const usb_dev usb_id = usb_device2id(usb_device);
    int release = 0;

    if (!spk_info) {
        return;
    }

    usb_clr_intr_rxe(usb_id, cfg->spk_ep);
    usb_g_set_intr_hander(usb_id, cfg->spk_ep, NULL);

    if (spk_info[stream_id].spk_dma_buffer) {
        release = (spk_info[stream_id].alternate_setting == (u8)(-1)) ? 1 : 0;
    }
    uac_speaker_stream_close_ex(stream_id, release);

}
static u32 spk_as_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;

    u8 if_num = LOBYTE(setup->wIndex);
    u8 stream_id = uac_stream_id_from_as_if(if_num);
    if (stream_id >= UAC_MAX_STREAMS) {
        return ret;
    }

    if (!spk_info) {
        return ret;
    }

    switch (setup->bRequest) {
    case USB_REQ_SET_INTERFACE:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            if (spk_info[stream_id].spk_dma_buffer) {
                spk_info[stream_id].alternate_setting = setup->wValue;
            }
#if USB_ROOT2
            if (!usb_root2_testing()) {
                SetInterface_0_Lock = 1;
            }
            if (SetInterface_0_Lock == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                if (setup->wValue == 1) { //alt 1
                    open_spk(usb_device, stream_id);
                } else if (setup->wValue == 0) { //alt 0
                    close_spk(usb_device, stream_id);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
#else
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            if (setup->wValue == 1) { //alt 1
                open_spk(usb_device, stream_id);
            } else if (setup->wValue == 2) { //alt 2
                open_spk(usb_device, stream_id);
            } else if (setup->wValue == 0) { //alt 0
                close_spk(usb_device, stream_id);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
#endif
        }

        break;
    case USB_REQ_GET_INTERFACE:
        if (usb_root2_testing()) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            if (setup->wValue || (setup->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                if (spk_info[stream_id].spk_dma_buffer) {
                    tx_payload[0] = spk_info[stream_id].alternate_setting;
                }
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
        }
        break;
    case USB_REQ_GET_STATUS:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    default:
        break;
    }
    return ret;
}

static void mic_transfer(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res = 0;
    u8 stream_id = uac_mic_stream_id_from_ep(ep);

    if (!mic_info || !mic_info[stream_id].mic_dma_buffer) {
        return;
    }
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    ep_buffer = mic_info[stream_id].mic_dma_buffer;
    mic_res = mic_info[stream_id].res;
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 24) {
        mic_res = 32;
    }
#endif
    mic_frame_len = ((mic_info[stream_id].mic_sampling_rate * mic_res / 8 * cfg->mic_channels) / 1000);
    //非对齐采样率补点
    if (mic_info[stream_id].mic_padding_interval) {
        if (++mic_info[stream_id].mic_padding_cnt == mic_info[stream_id].mic_padding_interval) {
            mic_info[stream_id].mic_padding_cnt = 0;
            mic_frame_len += (mic_res / 8) * cfg->mic_channels;
        }
    }

    int len = uac_mic_stream_read_ex(stream_id, ep_buffer, mic_frame_len);
    if (len) {
        mic_info[stream_id].mic_no_data = 0;
    } else { /* if (mic_info->mic_no_data) */
        len = mic_frame_len;
        memset(ep_buffer, 0, len);
    }

    usb_g_iso_write(usb_id, cfg->mic_ep, NULL, len);

    mic_info[stream_id].upstream_halt = 0;
}
static void mic_drop_data_when_upstream_halt(struct usb_device_t *usb_device, u32 ep, u32 frame)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res;
    u8 stream_id = uac_mic_stream_id_from_ep(ep);

    if (!mic_info || !mic_info[stream_id].mic_dma_buffer) {
        return;
    }

    if (uac_get_mic_stream_status_ex(stream_id) == 0) {
        usb_g_set_sof_hander(usb_id, g_stream_configs[stream_id].mic_ep | USB_DIR_IN, NULL);
        return;
    }
#ifdef USB_HW_20
#if FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        if ((++mic_info[stream_id].sof_cnt % 8) != 0) {
            return;
        }
    }
#endif
#endif

    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    ep_buffer = mic_info[stream_id].mic_dma_buffer;

    if (mic_info[stream_id].upstream_halt >= 5) {
        mic_info[stream_id].upstream_halt = 5;  //avoid numerical overflow
        mic_res = mic_info[stream_id].res;
#if UAC_24BIT_IN_4BYTE
        if (mic_res == 24) {
            mic_res = 32;
        }
#endif
        mic_frame_len = ((mic_info[stream_id].mic_sampling_rate * mic_res / 8 * cfg->mic_channels) / 1000);
        //非对齐采样率补点
        if (mic_info[stream_id].mic_padding_interval) {
            if (++mic_info[stream_id].mic_padding_cnt == mic_info[stream_id].mic_padding_interval) {
                mic_info[stream_id].mic_padding_cnt = 0;
                mic_frame_len += (mic_res / 8) * cfg->mic_channels;
            }
        }

        uac_mic_stream_read_ex(stream_id, ep_buffer, mic_frame_len);
    }
    mic_info[stream_id].upstream_halt++;
}
static void open_mic(struct usb_device_t *usb_device, u8 stream_id)
{
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res = 0;

    if (!mic_info || !mic_info[stream_id].mic_dma_buffer) {
        return;
    }
    log_info("open_mic stream=%d", stream_id);
    mic_info[stream_id].mic_no_data = 1;
    usb_enable_ep(usb_id, cfg->mic_ep);

    ep_buffer = mic_info[stream_id].mic_dma_buffer;

    usb_g_set_intr_hander(usb_id, cfg->mic_ep | USB_DIR_IN, mic_transfer);

    if (mic_info[stream_id].mic_sampling_rate == 0) {
        mic_info[stream_id].mic_sampling_rate = mic_get_max_sampling_rate(cfg);
    }
    if (mic_info[stream_id].alternate_setting == 1) {
        mic_res = cfg->mic_res;
    } else if (mic_info[stream_id].alternate_setting == 2) {
        mic_res = cfg->mic_res_2;
    } else {
        return;
    }
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 24) {
        mic_res = 32;
    }
#endif
    mic_frame_len = ((mic_info[stream_id].mic_sampling_rate * mic_res / 8 * cfg->mic_channels) / 1000);
    mic_frame_len += ((mic_info[stream_id].mic_sampling_rate % 1000) ? (mic_res / 8) * cfg->mic_channels : 0);
    if (mic_info[stream_id].mic_sampling_rate % 1000) {
        mic_info[stream_id].mic_padding_interval = 1000 / (mic_info[stream_id].mic_sampling_rate % 1000);
        mic_info[stream_id].mic_padding_cnt = 0;
    } else {
        mic_info[stream_id].mic_padding_interval = 0;
        mic_info[stream_id].mic_padding_cnt = 0;
    }
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 32) {
        mic_res = 24;
    }
#endif

    mic_info[stream_id].res = mic_res;
    uac_mic_stream_open_ex(stream_id, mic_info[stream_id].mic_sampling_rate,
                           cfg->mic_channels, mic_res);

    mic_info[stream_id].sof_cnt = 0;
    mic_info[stream_id].upstream_halt = 0;
    usb_g_set_sof_hander(usb_id, cfg->mic_ep | USB_DIR_IN, mic_drop_data_when_upstream_halt);

    usb_g_ep_config(usb_id,
                    cfg->mic_ep | USB_DIR_IN,
                    USB_ENDPOINT_XFER_ISOC,
                    1, ep_buffer, mic_frame_len);

    mic_transfer(usb_device, cfg->mic_ep);
}
static void close_mic(struct usb_device_t *usb_device, u8 stream_id)
{
    const struct uac_stream_config *cfg = &g_stream_configs[stream_id];

    log_info("close_mic stream=%d", stream_id);
    const usb_dev usb_id = usb_device2id(usb_device);
    int release = 0;

    if (!mic_info) {
        return;
    }

    usb_clr_intr_txe(usb_id, cfg->mic_ep);
    usb_g_set_intr_hander(usb_id, cfg->mic_ep | USB_DIR_IN, NULL);
    if (mic_info[stream_id].mic_dma_buffer) {
        release = (mic_info[stream_id].alternate_setting == (u8)(-1)) ? 1 : 0;
    }
    uac_mic_stream_close_ex(stream_id, release);
}
static u32 mic_as_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 tx_len;
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;

    u8 if_num = LOBYTE(setup->wIndex);
    u8 stream_id = uac_stream_id_from_as_if(if_num);
    if (stream_id >= UAC_MAX_STREAMS) {
        return ret;
    }

    if (!mic_info) {
        return ret;
    }

    switch (setup->bRequest) {
    case USB_REQ_SET_INTERFACE:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            if (mic_info[stream_id].mic_dma_buffer) {
                mic_info[stream_id].alternate_setting = setup->wValue;
            }
#if USB_ROOT2
            if (!usb_root2_testing()) {
                SetInterface_0_Lock = 1;
            }
            if (SetInterface_0_Lock == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                if (setup->wValue == 1) {//alt 1
                    open_mic(usb_device, stream_id);
                } else if (setup->wValue == 0) { //alt 0
                    close_mic(usb_device, stream_id);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
#else
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            if (setup->wValue == 1) {//alt 1
                open_mic(usb_device, stream_id);
            } else if (setup->wValue == 2) {//alt 2
                open_mic(usb_device, stream_id);
            } else if (setup->wValue == 0) { //alt 0
                close_mic(usb_device, stream_id);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
#endif
        }
        break;
    case USB_REQ_GET_INTERFACE:
        if (usb_root2_testing()) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            if (setup->wValue || (setup->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                if (mic_info[stream_id].mic_dma_buffer) {
                    tx_payload[0] = mic_info[stream_id].alternate_setting;
                }
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
        }
        break;
    case USB_REQ_GET_STATUS:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    default:
        break;
    }
    return ret;
}
static void spk_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    if (!g_stream_if_map || !spk_info) {
        return;
    }

    const usb_dev usb_id = usb_device2id(usb_device);
    log_info("spk_reset");

    u8 stream_id = uac_stream_id_from_as_if(ift_num);
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }

    if (spk_info[stream_id].spk_dma_buffer) {
        const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
        u8 *ep_buffer = spk_info[stream_id].spk_dma_buffer;
        u32 spk_frame_len = spk_get_max_frame_len(cfg);
        usb_g_ep_config_v2(usb_id, cfg->spk_ep | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC,
                           1, ep_buffer, spk_frame_len, spk_frame_len * UAC_MAX_PKTS_PER_SOF);
    }
#if CONFIG_USB_ESD_RECOVERY
    uac_speaker_notify_reset();
#endif
}
static void mic_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    if (!g_stream_if_map || !mic_info) {
        return;
    }

    const usb_dev usb_id = usb_device2id(usb_device);
    log_info("mic_reset");

    u8 stream_id = uac_stream_id_from_as_if(ift_num);
    if (stream_id >= UAC_MAX_STREAMS) {
        return;
    }
#if USB_ROOT2
    usb_disable_ep(usb_id, SPK_ISO_EP_OUT);
    /* uac_release(usb_id); */
#else
    if (mic_info[stream_id].mic_dma_buffer) {
        const struct uac_stream_config *cfg = &g_stream_configs[stream_id];
        u8 *ep_buffer = mic_info[stream_id].mic_dma_buffer;
        usb_g_ep_config(usb_id, cfg->mic_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC,
                        1, ep_buffer, mic_get_max_frame_len(cfg));
    }
#endif
}
static void audio_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    if (!spk_info && !mic_info) {
        return;
    }

    const usb_dev usb_id = usb_device2id(usb_device);
    log_info("audio_reset");
#if USB_ROOT2
    usb_disable_ep(usb_id, SPK_ISO_EP_OUT);
    /* uac_release(usb_id); */
#else
    u8 *ep_buffer;
    for (int i = 0; i < UAC_MAX_STREAMS; i++) {
        if (spk_info && spk_info[i].spk_dma_buffer) {
            const struct uac_stream_config *cfg = &g_stream_configs[i];
            ep_buffer = spk_info[i].spk_dma_buffer;
            u32 spk_frame_len = spk_get_max_frame_len(cfg);
            usb_g_ep_config_v2(usb_id, cfg->spk_ep | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC,
                               1, ep_buffer, spk_frame_len, spk_frame_len * UAC_MAX_PKTS_PER_SOF);
        }

        if (mic_info && mic_info[i].mic_dma_buffer) {
            const struct uac_stream_config *cfg = &g_stream_configs[i];
            ep_buffer = mic_info[i].mic_dma_buffer;
            usb_g_ep_config(usb_id, cfg->mic_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC,
                            1, ep_buffer, mic_get_max_frame_len(cfg));
        }
    }
#endif
}

static u32 uac_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    if (!usb_device) {
        return 0;
    }

    if (!g_stream_if_map) {
        g_stream_if_map = zalloc((UAC_MAX_STREAMS * 2 + 1) * sizeof(struct stream_if_map));
        if (!g_stream_if_map) {
            return 0;
        }
    }

    u8  *tptr = ptr;
    u8  map_idx = 0;   /* g_stream_if_map 紧凑下标 */

    log_info("[UAC2] desc_config enter: UAC_MAX_STREAMS=%d, cur_itf_num=%d\n",
             UAC_MAX_STREAMS, *cur_itf_num);

    /* 每个 stream_id 作为一台独立声卡输出（一个 IAD + 一个 AC 接口 + 各自的 AS），
     * 使主机枚举出多台音频设备；单台声卡可同时含 spk 与 mic。 */
    for (int i = 0; i < UAC_MAX_STREAMS; i++) {
        const struct uac_stream_config *cfg = &g_stream_configs[i];
        u8 has_spk = UAC_CFG_HAS_SPK(cfg) ? 1 : 0;
        u8 has_mic = UAC_CFG_HAS_MIC(cfg) ? 1 : 0;
        u8 num_as  = has_spk + has_mic;

        if (num_as == 0) {
            continue;
        }

        log_info("[UAC2] card[%d]: spk_rate_num=%d spk_ep=%d | mic_rate_num=%d mic_ep=%d\n",
                 i, cfg->spk_rate_num, cfg->spk_ep, cfg->mic_rate_num, cfg->mic_ep);

        u8 ac_if_num = (*cur_itf_num);

        /* IAD (8 bytes) */
        memcpy(tptr, uac_audio_interface_association, 8);
        tptr[2] = ac_if_num;
        tptr[3] = 1 + num_as;
        tptr[7] = cfg->spk_str_index;
        log_info("[UAC2] IAD[%d]: bFirstInterface=%d bInterfaceCount=%d\n",
                 i, tptr[2], tptr[3]);
        tptr += 8;

        /* AC Standard Interface (9 bytes) */
        memcpy(tptr, uac_ac_standard_interface_desc, 9);
        tptr[2] = ac_if_num;
        tptr[8] = cfg->spk_str_index;
        log_info("[UAC2] AC[%d] interface num=%d\n", i, ac_if_num);
        (*cur_itf_num)++;
        tptr += 9;

        /* AC Class-Specific Header (9 bytes for UAC2) */
        u8 *ctl_start = tptr;
        tptr[0] = 0x09;
        tptr[1] = USB_DT_CS_INTERFACE;
        tptr[2] = UAC_HEADER;
        bld_put_le16(0x0200, &tptr[3]);
        tptr[5] = 0x00;  // bCategory
        tptr += 9;

        /* Clock Sources and topology for this card */
        if (has_spk) {
            tptr = build_clock_source(tptr, CLOCK_ID_SPK(i), cfg->spk_rate_num);
            tptr = build_spk_input_terminal_uac2(tptr, cfg, i);
            tptr = build_spk_feature_unit_uac2(tptr, cfg, i);
            tptr = build_spk_output_terminal_uac2(tptr, cfg, i);
        }

        if (has_mic) {
            tptr = build_clock_source(tptr, CLOCK_ID_MIC(i), cfg->mic_rate_num);
            tptr = build_mic_input_terminal_uac2(tptr, cfg, i);
            tptr = build_mic_feature_unit_uac2(tptr, cfg, i);
            tptr = build_mic_output_terminal_uac2(tptr, cfg, i);
        }

        /* Backfill wTotalLength in AC header (16-bit for UAC2) */
        u16 ctrl_size = tptr - ctl_start;
        ctl_start[6] = (u8)(ctrl_size & 0xFF);
        ctl_start[7] = (u8)((ctrl_size >> 8) & 0xFF);

        /* Register AC handler */
        if (usb_set_interface_hander(usb_id, ac_if_num, audio_ac_itf_handler) != ac_if_num) {
            ASSERT(0, "uac set interface_hander fail");
        }
        usb_set_reset_hander(usb_id, ac_if_num, audio_reset);

        /* Build AS descriptors for this card */
        if (has_spk) {
            u8 if_num = (*cur_itf_num);
            g_stream_if_map[map_idx].stream_id = i;
            g_stream_if_map[map_idx].if_num_ac = ac_if_num;
            g_stream_if_map[map_idx].if_num_as = if_num;
            map_idx++;

            tptr = build_spk_as_descriptors_uac2(tptr, if_num, cfg,
                                                 SPK_INPUT_TERMINAL_ID(i),
                                                 CLOCK_ID_SPK(i));
            log_info("[UAC2] spk stream[%d]: AS if_num=%d ep=%d(OUT)\n",
                     i, if_num, cfg->spk_ep);

            usb_set_interface_hander(usb_id, if_num, spk_as_itf_hander);
            (*cur_itf_num)++;
        }

        if (has_mic) {
            u8 if_num = (*cur_itf_num);
            g_stream_if_map[map_idx].stream_id = i;
            g_stream_if_map[map_idx].if_num_ac = ac_if_num;
            g_stream_if_map[map_idx].if_num_as = if_num;
            map_idx++;

            tptr = build_mic_as_descriptors_uac2(tptr, if_num, cfg,
                                                 MIC_OUTPUT_TERMINAL_ID(i),
                                                 CLOCK_ID_MIC(i));
            log_info("[UAC2] mic stream[%d]: AS if_num=%d ep=%d(IN)\n",
                     i, if_num, cfg->mic_ep);

            usb_set_interface_hander(usb_id, if_num, mic_as_itf_hander);
            (*cur_itf_num)++;
        }
    }

    log_info("[UAC2] desc_config done: total_len=%d, final cur_itf_num=%d\n",
             (int)(tptr - ptr), *cur_itf_num);
    return tptr - ptr;
}
static u32 uac_spk_register(const usb_dev usb_id)
{
    if (!spk_info) {
        spk_info = zalloc(UAC_MAX_STREAMS * sizeof(struct spk_info_t));
        if (!spk_info) {
            return -1;
        }
    }

    for (int id = 0; id < UAC_MAX_STREAMS; id++) {
        const struct uac_stream_config *cfg = &g_stream_configs[id];
        if (!UAC_CFG_HAS_SPK(cfg)) {
            continue;
        }

        if (!spk_info[id].spk_dma_buffer) {
#if USB_EP_PROTECT
            u32 ep_reserved_len = 1024 - spk_get_max_frame_len(cfg);
#else
            u32 ep_reserved_len = 0;
#endif
            spk_info[id].spk_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, cfg->spk_ep,
                                          spk_get_max_frame_len(cfg) + ep_reserved_len);
        }
        spk_info[id].spk_sampling_rate = spk_get_max_sampling_rate(cfg);
        spk_info[id].spk_def_vol = vol_convert(100);
        spk_info[id].spk_left_vol = spk_info[id].spk_def_vol;
        spk_info[id].spk_right_vol = spk_info[id].spk_def_vol;
        spk_info[id].spk_max_vol = vol_convert(100);
        spk_info[id].spk_min_vol = vol_convert(0);
        spk_info[id].spk_vol_res = 0x30;
        spk_info[id].spk_mute = 0;
        spk_info[id].ps4_mode = 0;
    }
    return 0;
}
static void uac_spk_release(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (!spk_info) {
        return;
    }

    for (int id = 0; id < UAC_MAX_STREAMS; id++) {
        if (!spk_info[id].spk_dma_buffer) {
            continue;
        }
        //卸载时同步关闭音频流
        spk_info[id].alternate_setting = -1;
        close_spk(usb_device, id);
        if (spk_info[id].spk_dma_buffer) {
            usb_free_ep_dmabuffer(usb_id, spk_info[id].spk_dma_buffer);
            spk_info[id].spk_dma_buffer = NULL;
        }
    }

    free(spk_info);
    spk_info = NULL;
}
static u32 uac_mic_register(const usb_dev usb_id)
{
    if (!mic_info) {
        mic_info = zalloc(UAC_MAX_STREAMS * sizeof(struct mic_info_t));
        if (!mic_info) {
            return -1;
        }
    }

    for (int id = 0; id < UAC_MAX_STREAMS; id++) {
        const struct uac_stream_config *cfg = &g_stream_configs[id];
        if (!UAC_CFG_HAS_MIC(cfg)) {
            continue;
        }

        if (!mic_info[id].mic_dma_buffer) {
            mic_info[id].mic_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, cfg->mic_ep | USB_DIR_IN,
                                          mic_get_max_frame_len(cfg));
        }
        mic_info[id].mic_sampling_rate = mic_get_max_sampling_rate(cfg);
        mic_info[id].mic_max_vol = vol_convert(100);
        mic_info[id].mic_min_vol = vol_convert(0);
        mic_info[id].mic_vol_res = 0x30;
        mic_info[id].mic_def_vol = vol_convert(13 / 14.0 * 100);
        mic_info[id].mic_vol = mic_info[id].mic_def_vol;
        mic_info[id].mic_mute = 0;
        mic_info[id].ps4_mode = 0;
    }
    return 0;
}
static void uac_mic_release(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (!mic_info) {
        return;
    }

    for (int id = 0; id < UAC_MAX_STREAMS; id++) {
        if (!mic_info[id].mic_dma_buffer) {
            continue;
        }
        //卸载时同步关闭音频流
        mic_info[id].alternate_setting = -1;
        close_mic(usb_device, id);
        if (mic_info[id].mic_dma_buffer) {
            usb_free_ep_dmabuffer(usb_id, mic_info[id].mic_dma_buffer);
            mic_info[id].mic_dma_buffer = NULL;
        }
    }

    free(mic_info);
    mic_info = NULL;
}


static void uac_register(usb_dev usb_id)
{
    uac_spk_register(usb_id);
    uac_mic_register(usb_id);
}

static void uac_release(usb_dev usb_id)
{
    uac_spk_release(usb_id);
    uac_mic_release(usb_id);

    if (g_stream_if_map) {
        free(g_stream_if_map);
        g_stream_if_map = NULL;
    }
}

USB_CLASS_REGISTER(uac_desc_config, uac_register, uac_release,
                   TCFG_USB_AUDIO_INDEX, AUDIO_CLASS, 1);
#endif
