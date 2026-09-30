/**
 * @file descriptor.c
 * @brief overwrite usb device descriptor
 * @version 1.00
 * @date 2019-05-06
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb/device/descriptor.h"
#include "usb/device/uac_audio.h"
#include "tzflash_api.h"
#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#include "usb/device/iap.h"
#endif

#include "app_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[DESC]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE

#ifdef TCFG_USB_VID

#define USB_VID     TCFG_USB_VID
#define USB_PID     TCFG_USB_PID

#if TCFG_USB_ISERIALNUMBER
#define USB_ISERIALNUMBER   3      // String index = 3
#else
#define USB_ISERIALNUMBER   0
#endif

#define USB_STRING_UTF8_ENABLE   TCFG_USB_STRING_UTF8_ENABLE

#define USB_MANUFACTURER_DESC    TCFG_USB_MANUFACTURER_DESC
#define USB_PRODUCT_DESC         TCFG_USB_PRODUCT_DESC
#define USB_SPEAKER_DESC         TCFG_USB_SPEAKER_DESC
#define USB_MIC_DESC             TCFG_USB_SPEAKER_DESC

#else   /* 默认值 */

#define USB_VID                  0x3654
#define USB_PID                  0x1234
#define USB_ISERIALNUMBER        0
#define USB_STRING_UTF8_ENABLE   0

#define USB_MANUFACTURER_DESC    "Jieli Technology Co.,LTD"
#define USB_PRODUCT_DESC         "Jieli USB Audio"
#define USB_SPEAKER_DESC         "Jieli USB Speaker"
#define USB_MIC_DESC             "Jieli USB Microphone"

#endif

#define     IAP_interface_string "iAP Interface"


#define MAX_UTF16_CHARS  48

static const u8 sDeviceDescriptor[] = { //<Device Descriptor
    USB_DT_DEVICE_SIZE,      // bLength: Size of descriptor
    USB_DT_DEVICE,       // bDescriptorType: Device
#if defined(FUSB_MODE) && FUSB_MODE
#if TCFG_USB_WEBUSB_ENABLE
    0x10, 0x02,     // bcdUSB: USB 2.1
#else
    0x10, 0x01,     // bcdUSB: USB 1.1
#endif
#elif defined(FUSB_MODE) && (FUSB_MODE ==0 )
    0x00, 0x02,     // bcdUSB: USB 2.0
#else
#error "USB_SPEED_MODE not defined"
#endif
    0x00,       // bDeviceClass: none
    0x00,       // bDeviceSubClass: none
    0x00,       // bDeviceProtocol: none
    EP0_SETUP_LEN,//EP0_LEN,      // bMaxPacketSize0: 8/64 bytes
    LOBYTE(USB_VID), HIBYTE(USB_VID),     // idVendor: 0x3654 - Zhuhai Jieli Technology
    LOBYTE(USB_PID), HIBYTE(USB_PID),     // idProduct
#if defined(FUSB_MODE) && FUSB_MODE
    0x00, 0x01,     // bcdDevice: version 1.00
#else
    0x02, 0x01,     // bcdDevice: version 1.02，高速模式下与参考设备保持一致
#endif
    0x01,       // iManufacturer: Index to string descriptor that contains the string <Your Name> in Unicode
    0x02,       // iProduct: Index to string descriptor that contains the string <Your Product Name> in Unicode
    USB_ISERIALNUMBER,       // iSerialNumber: none
    0x01        // bNumConfigurations: 1
};

static const u8 sConfigDescriptor[] = {	//<Config Descriptor
//ConfiguraTIon
    USB_DT_CONFIG_SIZE,    //bLength
    USB_DT_CONFIG,    //DescriptorType : ConfigDescriptor
    0, 0, //TotalLength
    0,//bNumInterfaces: 在set_descriptor函数里面计算
    0x01,    //bConfigurationValue - ID of this configuration
    0x00,    //Unused
#if USB_REMOTE_WAKEUP_EN
    0xA0,    //Attributes:Bus Power remotewakeup
#else
    0x80,    //Attributes:Bus Power
#endif
    0xfa,     //MaxPower * 2ma
};


void get_device_descriptor(u8 *ptr)
{
    memcpy(ptr, sDeviceDescriptor, USB_DT_DEVICE_SIZE);
}

/* PID 的 class 偏移只覆盖 class 位图的低 8 位，每置起一个 bit，idProduct 加
 * (bit + 1) * USB_PID_CLASS_STEP，即累加到 idProduct 的高字节上。 */
#define USB_PID_CLASS_BIT_NUM   8
#define USB_PID_CLASS_STEP      0x100

u16 usb_get_vid(void)
{
    return USB_VID;
}

/* 从机模式编译期使能的 class 位图，内容与 usb_start() 下发给 usb_device_mode()
 * 的 class 一致。用编译期配置而不是运行时 wDeviceClass，是为了让拔线后
 * （usb_pause() 会清零 wDeviceClass）上报的 PID 不随插拔状态变化。 */
u16 usb_slave_class_config(void)
{
    u16 class = 0;

#if TCFG_USB_SLAVE_MSD_ENABLE
    class |= MASSSTORAGE_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    class |= SPEAKER_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    class |= MIC_CLASS;
#endif
#if TCFG_USB_SLAVE_HID_ENABLE
    class |= HID_CLASS;
#endif
#if TCFG_USB_SLAVE_CDC_ENABLE
    class |= CDC_CLASS;
#endif
#if TCFG_USB_CUSTOM_HID_ENABLE
    class |= CUSTOM_HID_CLASS;
#endif
#if TCFG_USB_SLAVE_MTP_ENABLE
    class |= MTP_CLASS;
#endif
#if TCFG_USB_SLAVE_MIDI_ENABLE
    class |= MIDI_CLASS;
#endif
#if TCFG_USB_SLAVE_PRINTER_ENABLE
    class |= PRINTER_CLASS;
#endif
#if TCFG_USB_APPLE_DOCK_EN
    class |= IAP_CLASS;
#endif
#if TCFG_USB_WEBUSB_ENABLE
    class |= WEBUSB_CLASS;
#endif
#if TCFG_USB_SLAVE_HCI_ENABLE
    class |= HCI_CLASS;
#endif
    /* Xbox360 手柄（XInput）与其配套的标准键鼠 HID。
     * 注意：xbox360 独占 EP1~EP4、hid_kbmouse 占 EP3，与 MSD(EP1)/HID(EP2)/UAC(EP3)
     * 的端点硬冲突，必须互斥使能；同时开启会导致端点被覆盖、描述符与端点不符。 */
#if TCFG_USB_SLAVE_XBOX360_ENABLE
    class |= XBOX360_CLASS;
#endif
#if TCFG_USB_SLAVE_HID_KBMOUSE_ENABLE
    class |= HID_KBMOUSE_CLASS;
#endif

    return class;
}

/* 按 class 组合推出 PID，算法与 user_setup.c 在 GET_DESCRIPTOR 时对
 * idProduct 高字节的运行时累加完全相同，保证主机枚举到的 PID 与上报值一致。 */
u16 usb_pid_from_class(u16 class_config)
{
    u16 pid = USB_PID;

    for (u8 i = 0; i < USB_PID_CLASS_BIT_NUM; i++) {
        if (class_config & BIT(i)) {
            pid += (u16)((i + 1) * USB_PID_CLASS_STEP);
        }
    }

    return pid;
}

static const char hex_table[] = "0123456789ABCDEF";

void get_chip_uuid_string(char *uuid_hex)
{
#if USB_ROOT2
    const char *root2_string = "00006F64096B22E7";
    memcpy(uuid_hex, root2_string, strlen(root2_string));
#else

    u8 *uuid_bin = tzflash_get_flash_uuid();

    for (u32 i = 0; i < 16; i++) {
        uuid_hex[i * 2]     = hex_table[(uuid_bin[i] >> 4) & 0x0F];
        uuid_hex[i * 2 + 1] = hex_table[ uuid_bin[i]       & 0x0F];
    }

    uuid_hex[32] = '\0';
#endif
}

void get_iserialnumber_str(u8 *ptr)
{
#if USB_ROOT2
    const u8 serial_string[] = {
        0x22, 0x03, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x36, 0x00, 0x46, 0x00, 0x36, 0x00,
        0x34, 0x00, 0x30, 0x00, 0x39, 0x00, 0x36, 0x00,
        0x42, 0x00, 0x32, 0x00, 0x32, 0x00, 0x45, 0x00,
        0x37, 0x00
    };
    memcpy(ptr, serial_string, serial_string[0]);
#else

#endif
}

#if USB_ROOT2
static const u8 ee_string[] = {0x12, 0x03, 0x4D, 0x00, 0x53, 0x00, 0x46, 0x00, 0x54,
                               0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00, 0x90, 0x00
                              };
void get_string_ee(u8 *ptr)
{
    memcpy(ptr, ee_string, ee_string[0]);
}
#endif


const u8 *usb_get_config_desc()
{
    return sConfigDescriptor;
}


typedef struct {
    u8     id;
    const char *str;
} string_desc_t;

static const u8 langid_desc[] = {
    0x04,                 // bLength
    USB_DT_STRING,        // bDescriptorType
    0x09, 0x04            // 0x0409 (English, LE)
    /* 0x04, 0x08,           // Chinese (Simplified) */
};

static const string_desc_t string_desc_arr[] = {
    { 1, USB_MANUFACTURER_DESC },    // Manufacturer
    { 2, USB_PRODUCT_DESC },         // Product

#if TCFG_USB_ISERIALNUMBER
    { 3, NULL },                     // Serial Number（NULL → 使用芯片 UUID）
#endif

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    { SPEAKER_STR_INDEX, USB_SPEAKER_DESC },   // Speaker

    /* Stream 1: separate string indices for per-stream naming */
#ifdef UAC_STREAM_1_SPEAKER_STR_INDEX
#if UAC_STREAM_1_SPEAKER_STR_INDEX
#ifndef UAC_STREAM_1_SPEAKER_DESC
#define UAC_STREAM_1_SPEAKER_DESC  USB_SPEAKER_DESC  /* fallback */
#endif
    { UAC_STREAM_1_SPEAKER_STR_INDEX, UAC_STREAM_1_SPEAKER_DESC },
#endif
#endif
#endif /* TCFG_USB_SLAVE_AUDIO_SPK_ENABLE */

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    { MIC_STR_INDEX,     USB_MIC_DESC     },   // Microphone

#ifdef UAC_STREAM_1_MIC_STR_INDEX
#if UAC_STREAM_1_MIC_STR_INDEX
#ifndef UAC_STREAM_1_MIC_DESC
#define UAC_STREAM_1_MIC_DESC  USB_MIC_DESC  /* fallback */
#endif
    { UAC_STREAM_1_MIC_STR_INDEX, UAC_STREAM_1_MIC_DESC },
#endif
#endif
#endif /* TCFG_USB_SLAVE_AUDIO_MIC_ENABLE */

#if TCFG_USB_APPLE_DOCK_EN
    { IAP_STR_INDEX, IAP_interface_string },
#endif
};

const char *get_string_by_id(uint8_t id)
{
    for (size_t i = 0; i < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]); i++) {
        if (string_desc_arr[i].id == id) {
            return string_desc_arr[i].str;
        }
    }
    return NULL;
}


static u32 make_utf16le_string(const char *str, u8 *desc)
{
#if USB_STRING_UTF8_ENABLE
    /* ================= UTF-8  ================= */
    u32 utf16_count = 0;
    u32 byte_pos = 2;
    const u8 *p = (const u8 *)str;

    while (*p && utf16_count < MAX_UTF16_CHARS) {
        u32 cp;

        if (p[0] < 0x80) {
            cp = p[0];
            p += 1;
        } else if ((p[0] & 0xE0) == 0xC0) {
            cp = ((p[0] & 0x1F) << 6) |
                 (p[1] & 0x3F);
            p += 2;
        } else if ((p[0] & 0xF0) == 0xE0) {
            cp = ((p[0] & 0x0F) << 12) |
                 ((p[1] & 0x3F) << 6) |
                 (p[2] & 0x3F);
            p += 3;
        } else if ((p[0] & 0xF8) == 0xF0) {
            cp = ((p[0] & 0x07) << 18) |
                 ((p[1] & 0x3F) << 12) |
                 ((p[2] & 0x3F) << 6) |
                 (p[3] & 0x3F);
            p += 4;
        } else {
            cp = 0xFFFD;
            p += 1;
        }

        if (cp <= 0xFFFF) {
            desc[byte_pos++] = cp & 0xFF;
            desc[byte_pos++] = cp >> 8;
            utf16_count++;
        } else {
            if (utf16_count + 1 >= MAX_UTF16_CHARS) {
                break;
            }
            cp -= 0x10000;
            u16 hi = 0xD800 | (cp >> 10);
            u16 lo = 0xDC00 | (cp & 0x3FF);

            desc[byte_pos++] = hi & 0xFF;
            desc[byte_pos++] = hi >> 8;
            desc[byte_pos++] = lo & 0xFF;
            desc[byte_pos++] = lo >> 8;
            utf16_count += 2;
        }
    }

#else
    /* ================= ASCII  ================= */
    u32 len = 0;
    u32 byte_pos = 2;

    while (len < MAX_UTF16_CHARS && str[len]) {
        desc[byte_pos++] = (u8)str[len];
        desc[byte_pos++] = 0x00;
        len++;
    }
#endif

    desc[0] = byte_pos;      // bLength
    desc[1] = USB_DT_STRING;
    return byte_pos;
}

u32 descriptor_string_cb(u32 index, u16 langid, u8 *payload)
{
    (void)langid;

    if (index == 0) {
        memcpy(payload, langid_desc, sizeof(langid_desc));
        return sizeof(langid_desc);
    }

    const char *str = get_string_by_id(index);

    /* Serial 特殊处理 */
    char uuid_hex[32 + 1];
    if (!str && index == 3) {
        get_chip_uuid_string(uuid_hex);
        str = uuid_hex;
    }

    if (!str) {
        return 0;
    }

    return make_utf16le_string(str, payload);
}



void get_device_info_to_ota(void *parm_priv)
{
    u8 *tmp = parm_priv;
    memcpy(tmp, &(sDeviceDescriptor[8]), 4);
    tmp[8]++;
    //cppcheck-suppress knownConditionTrueFalse
    descriptor_string_cb(1, 0x0409, &parm_priv[4]);
}
#endif
