#ifndef __USB_STD_CLASS_DEF_H__
#define __USB_STD_CLASS_DEF_H__

/**************************************************************************/
/*
               CLASS  BITMAP
    7   |   6   |   5   |   4   |   3   |   2   |   1   |   0
                                   HID    AUDIO  SPEAKER   Mass Storage
*/
/**************************************************************************/
#define     MASSSTORAGE_CLASS   0x00000001
#define     SPEAKER_CLASS       0x00000002
#define     MIC_CLASS           0x00000004
#define     AUDIO_CLASS         (SPEAKER_CLASS|MIC_CLASS)
#define     HID_CLASS           0x00000008
#define     CDC_CLASS           0x00000010
#define     CUSTOM_HID_CLASS    0x00000020
#define     MIDI_CLASS          0x00000040
#define     PRINTER_CLASS       0x00000080
#define     MTP_CLASS           0x00000100
#define     IAP_CLASS           0x00000200
#define     WEBUSB_CLASS        0x00000400
#define     HCI_CLASS           0x00000800
#define     XBOX360_CLASS       0x00001000
#define     HID_KBMOUSE_CLASS   0x00002000
#define     TEST_CLASS          0x80000000


#ifndef FUSB_MODE
#define FUSB_MODE  1
#endif

#if defined(FUSB_MODE) && (FUSB_MODE == 0)
#define USB_IS_HS_MODE 1
#else
#define USB_IS_HS_MODE 0
#endif

///////////MassStorage Class
#ifndef MSD_BULK_EP_OUT
#define MSD_BULK_EP_OUT             1
#endif
#ifndef MSD_BULK_EP_IN
#define MSD_BULK_EP_IN              1
#endif
#ifndef MAXP_SIZE_BULKOUT_FS
#define MAXP_SIZE_BULKOUT_FS        64
#endif
#ifndef MAXP_SIZE_BULKOUT_HS
#define MAXP_SIZE_BULKOUT_HS        512
#endif
#ifndef MAXP_SIZE_BULKOUT
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_HS
#endif
#endif
#ifndef MAXP_SIZE_BULKIN_FS
#define MAXP_SIZE_BULKIN_FS        64
#endif
#ifndef MAXP_SIZE_BULKIN_HS
#define MAXP_SIZE_BULKIN_HS        512
#endif
#ifndef MAXP_SIZE_BULKIN
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_HS
#endif
#endif
#ifndef MSD_STR_INDEX
#define MSD_STR_INDEX               7
#endif

#ifndef IAP_STR_INDEX
#define IAP_STR_INDEX               10
#endif

///////////HID class
#ifndef HID_EP_IN
#define HID_EP_IN                   2
#endif
#ifndef HID_EP_OUT
#define HID_EP_OUT                  2
#endif
#ifndef HID_EP_IN_2
#define HID_EP_IN_2                 3
#endif
#ifndef HID_EP_OUT_2
#define HID_EP_OUT_2                3
#endif
#ifndef MAXP_SIZE_HIDOUT
#define MAXP_SIZE_HIDOUT            16
#endif
#ifndef MAXP_SIZE_HIDIN
#define MAXP_SIZE_HIDIN             16
#endif
#ifndef HID_INTR_INTERVAL_FS
#define HID_INTR_INTERVAL_FS        1
#endif
#ifndef HID_INTR_INTERVAL_HS
#define HID_INTR_INTERVAL_HS        4
#endif
#ifndef HID_INTR_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define HID_INTR_INTERVAL           HID_INTR_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define HID_INTR_INTERVAL           HID_INTR_INTERVAL_HS
#endif
#endif

/////////////Audio Class
/**
 * 注意事项：
 * 1. uac 2.0需要使能spk的多采样率以及mic的多采样率才能被正确枚举
 * 2. Microsoft从Windodws 10 1703开始才支持uac 2.0，Linux则较早就支持，基于性能
 *    考虑，还是基于兼容性考虑，请谨慎选用
 * 3. 切换版本后，为避免出现未预料的问题，请先在设备管理器卸载一次
 * 4. 当mic声道>2时，windows会将采样率锁定在16kHz；spk声道>2时，则仍然可以设48kHz
 */
#define USB_AUDIO_VERSION_1_0       0x10
#define USB_AUDIO_VERSION_2_0       0x20
#ifndef USB_AUDIO_VERSION
#define USB_AUDIO_VERSION           USB_AUDIO_VERSION_1_0
#endif
#ifndef UAC_ISO_INTERVAL_FS
#define UAC_ISO_INTERVAL_FS         1
#endif
#ifndef UAC_ISO_INTERVAL_HS
#define UAC_ISO_INTERVAL_HS         4
#endif
#ifndef UAC_ISO_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define UAC_ISO_INTERVAL            UAC_ISO_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define UAC_ISO_INTERVAL            UAC_ISO_INTERVAL_HS
#endif
#endif
#ifndef UAC_24BIT_IN_4BYTE
//0: 24bit in 3byte, 1: 24 bit in 4byte
//Windows不支持这种数据格式，安卓支持这种数据格式
#define UAC_24BIT_IN_4BYTE          0
#endif
//speaker class
#ifndef SPK_AUDIO_RATE_NUM
#define SPK_AUDIO_RATE_NUM          1
#endif
#ifndef SPK_AUDIO_RATE_1
#define SPK_AUDIO_RATE_1            48000
#endif
#ifndef SPK_AUDIO_RATE_2
#define SPK_AUDIO_RATE_2            44100
#endif
#ifndef SPK_AUDIO_RATE_3
#define SPK_AUDIO_RATE_3            96000
#endif
#if SPK_AUDIO_RATE_NUM  == 1
#define SPK_AUDIO_RATE              SPK_AUDIO_RATE_1
#elif SPK_AUDIO_RATE_NUM == 2
#define SPK_AUDIO_RATE              SPK_AUDIO_RATE_2
#elif SPK_AUDIO_RATE_NUM == 3
#define SPK_AUDIO_RATE              SPK_AUDIO_RATE_3
#else
#error "SPK_AUDIO_RATE_NUM must be 1, 2, or 3"
#endif
#ifndef SPK_AUDIO_RES
#define SPK_AUDIO_RES               16
#endif
#ifndef SPK_AUDIO_RES_2
#define SPK_AUDIO_RES_2             0//24
#endif
#ifndef SPK_CHANNEL
#define SPK_CHANNEL                 2
#endif
#ifndef SPK_FRAME_LEN
#define SPK_FRAME_LEN               (((SPK_AUDIO_RATE) * SPK_AUDIO_RES / 8 * SPK_CHANNEL)/1000+4)
#endif

/* ---------- UAC multi-stream config ---------- */
#ifndef UAC_MAX_STREAMS
#define UAC_MAX_STREAMS  1
#endif

#if defined(MAX_EP_RX) && ((UAC_MAX_STREAMS * 2) > (MAX_EP_RX - 1))
#error "UAC_MAX_STREAMS exceeds available endpoints"
#endif

#ifndef SPK_PCM_Type
#define SPK_PCM_Type                (SPK_AUDIO_RES >> 4)                // 0=8 ,1=16
#endif
#ifndef SPK_AUDIO_TYPE
#define SPK_AUDIO_TYPE              (0x02 - SPK_PCM_Type)           // TYPE1_PCM16
#endif
#ifndef SPK_ISO_EP_OUT
#ifdef CONFIG_CPU_BR18
#define SPK_ISO_EP_OUT              2
#else
#define SPK_ISO_EP_OUT              3
#endif
#endif
#ifndef SPEAKER_STR_INDEX
#define SPEAKER_STR_INDEX           5
#endif
#define SPK_INPUT_TERMINAL_ID(n)   (1  + (n) * 10)
#define SPK_FEATURE_UNIT_ID(n)     (2  + (n) * 10)
#define SPK_SELECTOR_UNIT_ID(n)    (3  + (n) * 10)
#define SPK_OUTPUT_TERMINAL_ID(n)  (4  + (n) * 10)

/////////////Microphone Class
#ifndef MIC_AUDIO_RATE_NUM
#define MIC_AUDIO_RATE_NUM          1
#endif

#ifndef MIC_AUDIO_RATE_1
#define MIC_AUDIO_RATE_1            48000
#endif
#ifndef MIC_AUDIO_RATE_2
#define MIC_AUDIO_RATE_2            16000
#endif
#ifndef MIC_AUDIO_RATE_3
#define MIC_AUDIO_RATE_3            96000
#endif
#if MIC_AUDIO_RATE_NUM   == 1
#define MIC_AUDIO_RATE              MIC_AUDIO_RATE_1
#elif MIC_AUDIO_RATE_NUM == 2
#define MIC_AUDIO_RATE              MIC_AUDIO_RATE_2
#elif MIC_AUDIO_RATE_NUM == 3
#define MIC_AUDIO_RATE              MIC_AUDIO_RATE_3
#else
#error "MIC_AUDIO_RATE_NUM must be 1, 2, or 3"
#endif

#ifndef MIC_AUDIO_RES
#define MIC_AUDIO_RES               16
#endif
#ifndef MIC_AUDIO_RES_2
#define MIC_AUDIO_RES_2             0//24
#endif
#ifndef MIC_CHANNEL
#define MIC_CHANNEL                 1
#endif

/* ---------- wChannelConfig 值（u16，供 g_stream_configs 使用） ---------- */
/* 位定义见 USB Audio Class Spec：FL=0x1 FR=0x2 FC=0x4 LFE=0x8
 * BL=0x10 BR=0x20 SL=0x200 SR=0x400 ... */
#if SPK_CHANNEL == 1
#define SPK_CHANNEL_CONFIG_VAL       0x0004
#elif SPK_CHANNEL == 2
#define SPK_CHANNEL_CONFIG_VAL       0x0003
#elif SPK_CHANNEL == 3
#define SPK_CHANNEL_CONFIG_VAL       0x000B
#elif SPK_CHANNEL == 4
#define SPK_CHANNEL_CONFIG_VAL       0x0033
#elif SPK_CHANNEL == 5
#define SPK_CHANNEL_CONFIG_VAL       0x0037
#elif SPK_CHANNEL == 6
#define SPK_CHANNEL_CONFIG_VAL       0x003F
#elif SPK_CHANNEL == 7
#define SPK_CHANNEL_CONFIG_VAL       0x0637
#elif SPK_CHANNEL == 8
#define SPK_CHANNEL_CONFIG_VAL       0x063F
#else
#error "unsupport usb speaker channel number"
#endif

#if MIC_CHANNEL == 1
#define MIC_CHANNEL_CONFIG_VAL       0x0000
#elif MIC_CHANNEL == 2
#define MIC_CHANNEL_CONFIG_VAL       0x0003
#elif MIC_CHANNEL == 3
#define MIC_CHANNEL_CONFIG_VAL       0x000B
#elif MIC_CHANNEL == 4
#define MIC_CHANNEL_CONFIG_VAL       0x0033
#elif MIC_CHANNEL == 5
#define MIC_CHANNEL_CONFIG_VAL       0x0037
#elif MIC_CHANNEL == 6
#define MIC_CHANNEL_CONFIG_VAL       0x003F
#elif MIC_CHANNEL == 7
#define MIC_CHANNEL_CONFIG_VAL       0x0637
#elif MIC_CHANNEL == 8
#define MIC_CHANNEL_CONFIG_VAL       0x063F
#else
#error "unsupport usb mic channel number"
#endif

#ifndef MIC_FRAME_LEN
#define MIC_FRAME_LEN               ((MIC_AUDIO_RATE * MIC_AUDIO_RES / 8 * MIC_CHANNEL)/1000)
#endif
#ifndef MIC_PCM_TYPE
#define MIC_PCM_TYPE                (MIC_AUDIO_RES >> 4)                // 0=8 ,1=16
#endif
#ifndef MIC_AUDIO_TYPE
#define MIC_AUDIO_TYPE              (0x02 - MIC_PCM_TYPE)
#endif
#ifndef MIC_ISO_EP_IN
#define MIC_ISO_EP_IN               3
#endif
#ifndef MIC_STR_INDEX
#define MIC_STR_INDEX               6
#endif

/* ---------- UAC Stream 1 string indices ---------- */
#ifndef UAC_STREAM_1_SPEAKER_STR_INDEX
#define UAC_STREAM_1_SPEAKER_STR_INDEX  0  /* 0 = OS auto-generates name */
#endif
#ifndef UAC_STREAM_1_MIC_STR_INDEX
#define UAC_STREAM_1_MIC_STR_INDEX      0
#endif

/* ---------- UAC Stream 1 配置默认值（默认继承 stream 0，项目可在 sdk_config.h 覆盖） ---------- */
#ifndef UAC_STREAM_1_SPK_RATE_NUM
#define UAC_STREAM_1_SPK_RATE_NUM       SPK_AUDIO_RATE_NUM
#endif
#ifndef UAC_STREAM_1_SPK_RATE_1
#define UAC_STREAM_1_SPK_RATE_1         SPK_AUDIO_RATE_1
#endif
#ifndef UAC_STREAM_1_SPK_RATE_2
#define UAC_STREAM_1_SPK_RATE_2         SPK_AUDIO_RATE_2
#endif
#ifndef UAC_STREAM_1_SPK_RATE_3
#define UAC_STREAM_1_SPK_RATE_3         SPK_AUDIO_RATE_3
#endif
#ifndef UAC_STREAM_1_SPK_RES
#define UAC_STREAM_1_SPK_RES            SPK_AUDIO_RES
#endif
#ifndef UAC_STREAM_1_SPK_RES_2
#define UAC_STREAM_1_SPK_RES_2          SPK_AUDIO_RES_2
#endif
#ifndef UAC_STREAM_1_SPK_CHANNELS
#define UAC_STREAM_1_SPK_CHANNELS       SPK_CHANNEL
#endif
#ifndef UAC_STREAM_1_SPK_CHANNEL_CONFIG
#define UAC_STREAM_1_SPK_CHANNEL_CONFIG SPK_CHANNEL_CONFIG_VAL
#endif
#ifndef UAC_STREAM_1_SPK_EP
#define UAC_STREAM_1_SPK_EP             (SPK_ISO_EP_OUT + 1)  /* 需与 stream 0 不同，项目应显式配置 */
#endif
#ifndef UAC_STREAM_1_MIC_RATE_NUM
#define UAC_STREAM_1_MIC_RATE_NUM       0   /* stream 1 默认无 MIC */
#endif
#ifndef UAC_STREAM_1_MIC_EP
#define UAC_STREAM_1_MIC_EP             0
#endif

#define MIC_INPUT_TERMINAL_ID(n)   (5  + (n) * 10)
#define MIC_FEATURE_UNIT_ID(n)     (6  + (n) * 10)
#define MIC_SELECTOR_UNIT_ID(n)    (7  + (n) * 10)
#define MIC_OUTPUT_TERMINAL_ID(n)  (8  + (n) * 10)

////////////CDC Class
#ifndef CDC_DATA_EP_IN
#if (USB_MAX_HW_EPNUM <= 4)
#define CDC_DATA_EP_IN              1
#else
#define CDC_DATA_EP_IN              4
#endif
#endif

#ifndef CDC_DATA_EP_OUT
#if (USB_MAX_HW_EPNUM <= 4)
#define CDC_DATA_EP_OUT             1
#else
#define CDC_DATA_EP_OUT             4
#endif
#endif

#ifndef CDC_INTR_EP_IN
#define CDC_INTR_EP_IN              5
#endif
#ifndef MAXP_SIZE_CDC_BULKIN
#define MAXP_SIZE_CDC_BULKIN        64
#endif
#ifndef MAXP_SIZE_CDC_BULKOUT
#define MAXP_SIZE_CDC_BULKOUT       64
#endif
#ifndef MAXP_SIZE_CDC_INTRIN
#define MAXP_SIZE_CDC_INTRIN        8
#endif
#ifndef CDC_INTR_INTERVAL_FS
#define CDC_INTR_INTERVAL_FS        1
#endif
#ifndef CDC_INTR_INTERVAL_HS
#define CDC_INTR_INTERVAL_HS        4
#endif
#ifndef CDC_INTR_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define CDC_INTR_INTERVAL           CDC_INTR_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define CDC_INTR_INTERVAL           CDC_INTR_INTERVAL_HS
#endif
#endif
#ifndef CDC_INTR_EP_ENABLE
#define CDC_INTR_EP_ENABLE          0
#endif

///////////CUSTOM_HID class
#ifndef CUSTOM_HID_EP_IN
#define CUSTOM_HID_EP_IN            4
#endif
#ifndef CUSTOM_HID_EP_OUT
#define CUSTOM_HID_EP_OUT           4
#endif
#ifndef MAXP_SIZE_CUSTOM_HIDIN
#define MAXP_SIZE_CUSTOM_HIDIN      64
#endif
#ifndef MAXP_SIZE_CUSTOM_HIDOUT
#define MAXP_SIZE_CUSTOM_HIDOUT     64
#endif
#ifndef CUSTOM_HID_INTERVAL_FS
#define CUSTOM_HID_INTERVAL_FS      1
#endif
#ifndef CUSTOM_HID_INTERVAL_HS
#define CUSTOM_HID_INTERVAL_HS      4
#endif
#ifndef CUSTOM_HID_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define CUSTOM_HID_INTERVAL         CUSTOM_HID_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define CUSTOM_HID_INTERVAL         CUSTOM_HID_INTERVAL_HS
#endif
#endif

///USB MIDI Version
#define USB_MIDI_VERSION_1_0       0x10
#define USB_MIDI_VERSION_2_0       0x20
#ifndef USB_MIDI_VERSION
#define USB_MIDI_VERSION           USB_MIDI_VERSION_1_0
#endif

////////////MIDI Class
#ifndef MIDI_EP_IN
#define MIDI_EP_IN                  3
#endif
#ifndef MIDI_EP_OUT
#define MIDI_EP_OUT                 3
#endif
#ifndef MAXP_SIZE_MIDI_EPIN
#define MAXP_SIZE_MIDI_EPIN         64
#endif
#ifndef MAXP_SIZE_MIDI_EPOUT
#define MAXP_SIZE_MIDI_EPOUT        64
#endif

///////////PRINTER Class
#ifndef PTR_BULK_EP_OUT
#define PTR_BULK_EP_OUT             2
#endif
#ifndef PTR_BULK_EP_IN
#define PTR_BULK_EP_IN              1
#endif
#ifndef MAXP_SIZE_PTR_EPIN
#define MAXP_SIZE_PTR_EPIN          64
#endif
#ifndef MAXP_SIZE_PTR_EPOUT
#define MAXP_SIZE_PTR_EPOUT         64
#endif

///////////MTP Class
#ifndef MTP_BULK_EP_OUT
#define MTP_BULK_EP_OUT             1
#endif
#ifndef MTP_BULK_EP_IN
#define MTP_BULK_EP_IN              1
#endif
#ifndef MTP_INTR_EP_IN
#define MTP_INTR_EP_IN              2
#endif
#ifndef MAXP_SIZE_BULKOUT_FS
#define MAXP_SIZE_BULKOUT_FS        64
#endif
#ifndef MAXP_SIZE_BULKOUT_HS
#define MAXP_SIZE_BULKOUT_HS        512
#endif
#ifndef MAXP_SIZE_BULKOUT
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_HS
#endif
#endif
#ifndef MAXP_SIZE_BULKIN_FS
#define MAXP_SIZE_BULKIN_FS        64
#endif
#ifndef MAXP_SIZE_BULKIN_HS
#define MAXP_SIZE_BULKIN_HS        512
#endif
#ifndef MAXP_SIZE_BULKIN
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_HS
#endif
#endif
#ifndef MTP_STR_INDEX
#define MTP_STR_INDEX              8
#endif

#define     WEBUSB_EP_IN        4
#define     WEBUSB_EP_OUT       4

/* ---------- Xbox360 gamepad (XInput) ----------
 * 4 个 interface、EP1~EP4 各一对双向中断端点。
 * 端点号与接口角色绑定（鼠标 EP3、聊天板 EP4），便于描述符与参考设备逐字节对齐。
 * bInterval 直接取自参考设备描述符：FS 按 ms 解释，HS 按 2^(n-1)*125us 解释，
 * 两种速率下都是同一组字节，无需条件改写。 */
#ifndef XBOX360_EP_IN_0
#define XBOX360_EP_IN_0             1       /* IF0 XID            */
#endif
#ifndef XBOX360_EP_IN_1
#define XBOX360_EP_IN_1             2       /* IF1 厂商 HID 耳机   */
#endif
#ifndef XBOX360_EP_IN_2
#define XBOX360_EP_IN_2             4       /* IF3 厂商 HID 聊天板 */
#endif
#ifndef XBOX360_MAXP_IF0
#define XBOX360_MAXP_IF0            32
#endif
#ifndef XBOX360_MAXP_HID
#define XBOX360_MAXP_HID            64
#endif
#ifndef XBOX360_INTR_INTERVAL_0
#define XBOX360_INTR_INTERVAL_0     1
#endif
#ifndef XBOX360_INTR_INTERVAL_0_OUT
#define XBOX360_INTR_INTERVAL_0_OUT 7
#endif
#ifndef XBOX360_INTR_INTERVAL_1
#define XBOX360_INTR_INTERVAL_1     4
#endif
#ifndef XBOX360_INTR_INTERVAL_1_OUT
#define XBOX360_INTR_INTERVAL_1_OUT 8
#endif
#ifndef XBOX360_INTR_INTERVAL_2
#define XBOX360_INTR_INTERVAL_2     1
#endif
#ifndef XBOX360_INTR_INTERVAL_2_OUT
#define XBOX360_INTR_INTERVAL_2_OUT 1
#endif

/* ---------- 标准键鼠 HID class ----------
 * 参数化实现，本产品实例化为「Boot Mouse + Report ID 2 键盘」。 */
#ifndef HID_KBMOUSE_EP_IN
#define HID_KBMOUSE_EP_IN           3
#endif
#ifndef HID_KBMOUSE_EP_OUT
#define HID_KBMOUSE_EP_OUT          3
#endif
#ifndef HID_KBMOUSE_MAXP
#define HID_KBMOUSE_MAXP            64
#endif
#ifndef HID_KBMOUSE_INTR_INTERVAL
#define HID_KBMOUSE_INTR_INTERVAL   1
#endif
#ifndef HID_KBMOUSE_INTR_INTERVAL_OUT
#define HID_KBMOUSE_INTR_INTERVAL_OUT 8
#endif
#ifndef HID_KBMOUSE_BCD_HID
#define HID_KBMOUSE_BCD_HID         0x0110
#endif
#ifndef HID_KBMOUSE_SUBCLASS
#define HID_KBMOUSE_SUBCLASS        0x01    /* Boot Interface */
#endif
#ifndef HID_KBMOUSE_PROTOCOL
#define HID_KBMOUSE_PROTOCOL        0x02    /* Mouse */
#endif

#ifndef TCFG_USB_MSD_INDEX
#define TCFG_USB_MSD_INDEX         0
#endif
#ifndef TCFG_USB_HID_INDEX
#define TCFG_USB_HID_INDEX         1
#endif
#ifndef TCFG_USB_CDC_INDEX
#define TCFG_USB_CDC_INDEX         2
#endif
#ifndef TCFG_USB_AUDIO_INDEX
#define TCFG_USB_AUDIO_INDEX       3
#endif
#ifndef TCFG_USB_MIDI_INDEX
#define TCFG_USB_MIDI_INDEX        4
#endif
#ifndef TCFG_USB_PRINTER_INDEX
#define TCFG_USB_PRINTER_INDEX     5
#endif
#ifndef TCFG_USB_MTP_INDEX
#define TCFG_USB_MTP_INDEX         6
#endif
#ifndef TCFG_USB_IAP_INDEX
#define TCFG_USB_IAP_INDEX         7
#endif
#ifndef TCFG_USB_WEBUSB_INDEX
#define TCFG_USB_WEBUSB_INDEX      8
#endif
#ifndef TCFG_USB_CUSTOM_HID_INDEX
#define TCFG_USB_CUSTOM_HID_INDEX  9
#endif
#ifndef TCFG_USB_TEST_INDEX
#define TCFG_USB_TEST_INDEX        10
#endif
#ifndef TCFG_USB_HCI_INDEX
#define TCFG_USB_HCI_INDEX         11
#endif
#ifndef TCFG_USB_XBOX360_INDEX
#define TCFG_USB_XBOX360_INDEX     12
#endif
#ifndef TCFG_USB_HID_KBMOUSE_INDEX
#define TCFG_USB_HID_KBMOUSE_INDEX 13
#endif

#endif /* __USB_STD_CLASS_DEF_H__ */
