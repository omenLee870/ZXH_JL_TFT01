/* Copyright(C) JieLi Technology
 * Xbox360 手柄（XInput）从机驱动
 *
 * 描述符按参考设备逐字节复刻，参考 apps/common/device/usb/docs/usb_device_class_guide.md。
 * 三个接口：
 *   IF0  XID 厂商接口（FF/5D/01）+ 17 字节 0x21 安全描述符   EP1 IN/OUT 32B
 *   IF1  厂商 HID 接口（Usage Page 0xFF10）                  EP2 IN/OUT 64B
 *   IF2  厂商 HID 接口（Usage Page 0xFF70）                  EP4 IN/OUT 64B
 * 标准键鼠接口由 hid_kbmouse.c 提供（EP3），排在最后。
 *
 * 注意：本驱动独占 EP1~EP4，不能与 UAC/MSD/HID 等其他从机 class 同时使能。
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
#include "usb/device/hid.h"     /* 复用 USB_HID_DT_* / USB_REQ_*_IDLE 等 HID 类请求定义 */
#include "usb/device/xbox360.h"
/* 带 gamepad/ 前缀，靠各 app 已有的 -I$(ROOT)/apps/common/device/ 解析，
 * 这样新增 device/gamepad/ 目录不需要动各 app 的 include_dir.mk */
#include "gamepad/gamepad_input.h"
/* 只为了在进 USB 模式时把「本机摇杆」这个常驻输入源拉起来（TCFG 宏守护）。
 * 运行期用哪个源由应用层调 gamepad_input_select() 决定，与本驱动无关 */
#include "gamepad/joystick_gpadc.h"
#include "app_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[XBOX360]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_XBOX360_ENABLE

/* 厂商请求：XID 初始化 */
#define XBOX360_REQ_XID             0x01
/* 厂商请求：MS OS 扩展兼容 ID 描述符 */
#define XBOX360_REQ_MS_OS           0x90
/* MS OS 扩展兼容 ID 描述符的 wIndex */
#define XBOX360_MS_OS_INDEX         0x0004
/* MS OS 扩展属性描述符的 wIndex */
#define XBOX360_MS_OS_PROP_INDEX    0x0005
/* MS OS 字符串描述符索引 */
#define XBOX360_MS_OS_STR_INDEX     0xEE

#define XBOX360_ITF_NUM             3       /* 本驱动占用的接口数 */
#define XBOX360_HID_ITF_NUM         2       /* 其中的厂商 HID 接口数 */

/* 输入报文里的固定前两字节：类型 0x00、长度 20 */
#define XBOX360_REPORT_TYPE         0x00

/* ---------------- 配置描述符（config 头之后的 104 字节） ----------------
 * 取自参考设备 cfg.bin 的 cfg[9..80]（IF0 + IF1）与 cfg[113..144]（IF3）。
 * 标准键鼠接口 cfg[81..112] 由 hid_kbmouse.c 提供，接口号重排后本表内容不变。
 *
 * 本表的字节值是靠协议标准宏拼出来的：能用 ch9.h / hid.h 的宏就用宏，
 * 没有现成宏的（厂商类、Xbox 安全描述符等）保留字面量并逐字段注释。
 * 绝对不能出现"值对不上宏名"的情况 —— 描述符已与参考设备逐字节比对过。 */

/* 接口号在运行期按 itf_base 回填，对应下面各接口描述符的 bInterfaceNumber */
#define XB_OFF_IFNUM_0              2
#define XB_OFF_IFNUM_1              42
#define XB_OFF_IFNUM_2              74
/* 两个 HID 描述符的起始偏移，9 字节 */
#define XB_OFF_HIDDESC_1            49
#define XB_OFF_HIDDESC_2            81
/* HID 描述符内 wDescriptorLength 的偏移：
 * 从描述符起始数第 7 字节（bLength/type/bcdHID×2/count/type 之后），
 * 也就是 HID 描述符长度减去最后两个字节 */
#define XB_OFF_REPLEN_REL           (USB_HID_DESC_SIZE - 2)

/* ---- Xbox360 专有描述符 ----
 * Interface Association 风格的 0x21：XID 厂商接口的安全描述符，共 17 字节。
 * 前 6 字节固定，后 11 字节是设备侧的随机/校验数据，照抄参考设备取值。 */
#define XBSEC_DESC_LEN              17
#define XBSEC_DT                    0x21
#define XBSEC_MAGIC_0               0x00
#define XBSEC_MAGIC_1               0x01
#define XBSEC_MAGIC_2               0x01
#define XBSEC_MAGIC_3               0x25

/* XID 接口固定声明为 2 个中断端点：EP1 IN（发报文）+ EP1 OUT（收 LED/震动） */
#define XBOX360_IF0_NUM_EP          2

static const u8 sXbox360Config[] = {
    /* ---------- IF0: XID 厂商接口（bInterfaceNumber 运行期回填） ---------- */
    USB_DT_INTERFACE_SIZE,          // bLength
    USB_DT_INTERFACE,               // bDescriptorType
    0x00,                           // bInterfaceNumber（运行期回填）
    0x00,                           // bAlternateSetting
    XBOX360_IF0_NUM_EP,             // bNumEndpoints（IN + OUT）
    USB_CLASS_VENDOR_SPEC,          // bInterfaceClass = 0xFF
    0x5D,                           // bInterfaceSubClass = 0x5D（XID 专有）
    0x01,                           // bInterfaceProtocol = 0x01（XID 专有）
    0x00,                           // iInterface（无字符串）
    /* 安全描述符，17 字节 */
    XBSEC_DESC_LEN,                 // bLength
    XBSEC_DT,                       // bDescriptorType = 0x21
    XBSEC_MAGIC_0, XBSEC_MAGIC_1, XBSEC_MAGIC_2, XBSEC_MAGIC_3,
    0x81, 0x14,                     // 以下按参考设备取值
    0x00, 0x00, 0x00, 0x00,
    0x13, 0x01, 0x08, 0x00, 0x00,
    /* EP1 IN：中断，32B，bInterval 1 */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    (USB_DIR_IN | XBOX360_EP_IN_0),         // bEndpointAddress
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_IF0), HIBYTE(XBOX360_MAXP_IF0),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_0,                // bInterval
    /* EP1 OUT：中断，32B，bInterval 7（HS 下 2^6×125us = 8ms） */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    XBOX360_EP_IN_0,                        // bEndpointAddress（OUT）
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_IF0), HIBYTE(XBOX360_MAXP_IF0),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_0_OUT,            // bInterval
    /* ---------- IF1: 厂商 HID 接口（耳机，bInterfaceNumber 运行期回填） ---------- */
    USB_DT_INTERFACE_SIZE,          // bLength
    USB_DT_INTERFACE,               // bDescriptorType
    0x01,                           // bInterfaceNumber（运行期回填）
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    USB_CLASS_HID,                  // bInterfaceClass
    0x00,                           // bInterfaceSubClass（无子类）
    0x00,                           // bInterfaceProtocol（无协议）
    0x00,                           // iInterface（无字符串）
    /* HID 描述符：bcdHID 0x0111，wDescriptorLength 运行期回填 */
    USB_HID_DESC_SIZE,              // bLength
    USB_HID_DT_HID,                 // bDescriptorType = 0x21
    LOBYTE(USB_HID_BCD_1_11), HIBYTE(USB_HID_BCD_1_11),     // bcdHID
    0x00,                           // bCountryCode = 0（不本地化）
    0x01,                           // bNumDescriptors
    USB_HID_DT_REPORT,              // bDescriptorType = Report
    0x1E, 0x00,                     // wDescriptorLength（运行期回填）
    /* EP2 IN：中断，64B，bInterval 4（HS 下 1ms） */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    (USB_DIR_IN | XBOX360_EP_IN_1),         // bEndpointAddress
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_HID), HIBYTE(XBOX360_MAXP_HID),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_1,                // bInterval
    /* EP2 OUT：中断，64B，bInterval 8（HS 下 16ms） */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    XBOX360_EP_IN_1,                        // bEndpointAddress（OUT）
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_HID), HIBYTE(XBOX360_MAXP_HID),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_1_OUT,            // bInterval
    /* ---------- IF2: 厂商 HID 接口（聊天板，bInterfaceNumber 运行期回填） ---------- */
    USB_DT_INTERFACE_SIZE,          // bLength
    USB_DT_INTERFACE,               // bDescriptorType
    0x03,                           // bInterfaceNumber（运行期回填）
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints
    USB_CLASS_HID,                  // bInterfaceClass
    0x00,                           // bInterfaceSubClass（无子类）
    0x00,                           // bInterfaceProtocol（无协议）
    0x00,                           // iInterface（无字符串）
    /* HID 描述符：bcdHID 0x0111，wDescriptorLength 运行期回填 */
    USB_HID_DESC_SIZE,              // bLength
    USB_HID_DT_HID,                 // bDescriptorType = 0x21
    LOBYTE(USB_HID_BCD_1_11), HIBYTE(USB_HID_BCD_1_11),     // bcdHID
    0x00,                           // bCountryCode = 0（不本地化）
    0x01,                           // bNumDescriptors
    USB_HID_DT_REPORT,              // bDescriptorType = Report
    0x22, 0x00,                     // wDescriptorLength（运行期回填）
    /* EP4 IN：中断，64B，bInterval 1（HS 下 125us） */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    (USB_DIR_IN | XBOX360_EP_IN_2),         // bEndpointAddress
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_HID), HIBYTE(XBOX360_MAXP_HID),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_2,                // bInterval
    /* EP4 OUT：中断，64B，bInterval 1 */
    USB_DT_ENDPOINT_SIZE,                   // bLength
    USB_DT_ENDPOINT,                        // bDescriptorType
    XBOX360_EP_IN_2,                        // bEndpointAddress（OUT）
    USB_ENDPOINT_XFER_INT,                  // bmAttributes
    LOBYTE(XBOX360_MAXP_HID), HIBYTE(XBOX360_MAXP_HID),     // wMaxPacketSize
    XBOX360_INTR_INTERVAL_2_OUT,            // bInterval
};

/* 厂商自定义 HID 的 Usage Page，与本驱动是同一厂家私有约定。
 * 多字节 item 按 SDK 惯例写成逗号分隔的字节对（同 hid.h 的 BALANCE_LEFT），
 * 写成单个 0xFF10 会被当成一个 u8 元素截断成 0x10 */
#define XBOX360_PAGE_HEADSET        0x10, 0xFF  //Vendor 0xFF10
#define XBOX360_PAGE_CHATPAD        0x70, 0xFF  //Vendor 0xFF70

/* IF1 的 report 描述符：厂商页 0xFF10，Report ID 0x81 输入 / 0x80 输出，各 63 字节 */
static const u8 sReportDescHeadset[] = {
    USAGE_PAGE(2, XBOX360_PAGE_HEADSET),
    USAGE(1, 0x01),                         //Usage 0x01（耳机）
    COLLECTION(1, APPLICATION),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0xFF),
    USAGE(1, 0x02),
    REPORT_ID(1, 0x81),
    REPORT_SIZE(1, 0x08),
    REPORT_COUNT(1, 0x3F),                  //63 字节
    INPUT(1, 0x03),                         //Const,Var,Abs
    USAGE(1, 0x03),
    REPORT_ID(1, 0x80),
    REPORT_COUNT(1, 0x3F),
    OUTPUT(1, 0x03),                        //Const,Var,Abs
    END_COLLECTION,
};

/* IF2 的 report 描述符：厂商页 0xFF70，输入/输出各 64 字节 */
static const u8 sReportDescChatpad[] = {
    USAGE_PAGE(2, XBOX360_PAGE_CHATPAD),
    USAGE(1, 0x71),                         //Usage 0x71（聊天板）
    COLLECTION(1, APPLICATION),
    USAGE(1, 0x62),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(2, 0xFF, 0x00),             //两字节小端，最高位在前
    REPORT_COUNT(1, 0x40),                  //64 字节
    REPORT_SIZE(1, 0x08),
    INPUT(1, 0x02),                         //Data,Var,Abs
    USAGE(1, 0x63),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(2, 0xFF, 0x00),
    REPORT_COUNT(1, 0x40),
    REPORT_SIZE(1, 0x08),
    OUTPUT(1, 0x02),                        //Data,Var,Abs
    END_COLLECTION,
};

static const u8 *const sHidReportDesc[XBOX360_HID_ITF_NUM] = {
    sReportDescHeadset,
    sReportDescChatpad,
};
static const u16 sHidReportDescLen[XBOX360_HID_ITF_NUM] = {
    sizeof(sReportDescHeadset),
    sizeof(sReportDescChatpad),
};
static const u16 sHidDescOffset[XBOX360_HID_ITF_NUM] = {
    XB_OFF_HIDDESC_1,
    XB_OFF_HIDDESC_2,
};

/* MS OS 字符串描述符：GET_DESCRIPTOR(String, 0xEE) 时返回。
 * 参考设备抓包里没有这条请求（主机的 MS OS 缓存已存在），但冷机首次插入
 * Windows 必定先问它，拿不到就取不到后面的 CompatibleID "XUSB10"。
 * 结构见 MS OS 1.0 规范：bLength + bDescriptorType + "MSFT100"(UTF-16LE)
 *                        + bMS_VendorCode + bPad。
 * 长度 = 2 + 7*2 + 1 + 1 = 18，数组建好后用 _Static_assert 兜底。
 * 注意不能用 sizeof(数组自身) 写 bLength —— 声明未完成时类型不完整。 */
#define MSOS_STR_MAGIC          "MSFT100"
#define MSOS_STR_LEN            18

static const u8 sMsOsStringDesc[] = {
    MSOS_STR_LEN,                   // bLength
    USB_DT_STRING,                  // bDescriptorType
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,     // "MSFT100" UTF-16LE
    '1', 0x00, '0', 0x00, '0', 0x00,
    XBOX360_REQ_MS_OS,              // bMS_VendorCode
    0x00                            // bPad
};
_Static_assert(sizeof(MSOS_STR_MAGIC) - 1 == 7, "MSFT100 魔数应为 7 个字符");
_Static_assert(sizeof(sMsOsStringDesc) == MSOS_STR_LEN, "MS OS 字符串描述符长度不符");

/* MS OS 扩展兼容 ID 描述符：bRequest=0x90 / wIndex=4 时返回。
 * 头 16 字节与参考设备抓包逐字节一致；function 段的后 16 字节
 * （SubCompatibleID + Reserved2）按 MS 规范必须全 0，抓包被截断未见，此处补 0。
 * CompatibleID "XUSB10" 是让 Windows 加载 xusb22.sys 的关键。
 * dwLength = 16(头) + 1 + 1 + 8(CompatibleID) + 8(SubCompatibleID) + 6(保留) = 40 */
#define MSOS_COMPAT_ID          "XUSB10"
#define MSOS_COMPAT_LEN         40

static const u8 sMsExtendedCompatId[] = {
    /* ---------- 头 16 字节 ---------- */
    MSOS_COMPAT_LEN, 0x00, 0x00, 0x00,  // dwLength
    0x00, 0x01,                     // bcdVersion = 0x0100
    0x04, 0x00,                     // wIndex = 4
    0x01,                           // bCount = 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // Reserved[7]
    /* ---------- function section ---------- */
    0x00,                           // bFirstInterfaceNumber
    0x04,                           // Reserved1，照抄参考设备取值
    'X', 'U', 'S', 'B', '1', '0', 0x00, 0x00,   // CompatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SubCompatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // Reserved2
};
_Static_assert(sizeof(MSOS_COMPAT_ID) - 1 == 6, "XUSB10 应为 6 个字符");
_Static_assert(sizeof(sMsExtendedCompatId) == MSOS_COMPAT_LEN, "Compat ID 描述符长度不符");

/* MS OS 扩展属性描述符：bRequest=0x90 / wIndex=5 时返回。
 * 头 10 字节即全部内容（wCount=0 表示没有属性）。不实现时主机会收到 STALL——
 * 按 MS 规范这是合法的"不支持"答复，不影响枚举，但会在总线上多 4 次失败交互
 * （wValue 0~3），实现后可以消掉。
 * dwLength = 4(dwLength) + 4(dwVersion) + 2(wCount) = 10 */
#define MSOS_PROP_LEN           10

static const u8 sMsExtendedProperties[] = {
    MSOS_PROP_LEN, 0x00, 0x00, 0x00,    // dwLength
    0x00, 0x01, 0x00, 0x00,         // dwVersion = 0x0100
    0x00, 0x00,                     // wCount = 0
};
_Static_assert(sizeof(sMsExtendedProperties) == MSOS_PROP_LEN, "Extended Properties 长度不符");

/* XID 能力描述符：20 字节。抓包中 20B/8B/4B 三次响应正好是本数组的前缀，
 * usb_set_data_payload() 会按 wLength 自动截断，故只需一个数组。 */
static const u8 sXidCapabilities[] = {
    0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
};

struct xbox360_hdl {
    u8  itf_base;                               /* 本 class 的第一个接口号 */
    u8  cfg_done;                               /* SET_CONFIGURATION 完成 */
    u8  ep_in_ready;                            /* 主机已取走首包，说明在轮询 */
    u8  hid_idle[XBOX360_HID_ITF_NUM];
    /* IF0 当前要上报的输入报文。由 EP1 的 TX 完成中断持续发出，
     * 上层只更新内容，不直接写端点（见 xbox360_ep_in_isr） */
    u8  report[XBOX360_INPUT_REPORT_LEN];
    u8 *ep_in_dma[XBOX360_ITF_NUM];
    u8 *ep_out_dma[XBOX360_ITF_NUM];
    void *output_priv;
    xbox360_output_cb_t output_cb;
};

static struct xbox360_hdl *xbox360_hdl;

/* 每帧采样回调放在文件作用域而不是句柄里：它是配置项而非实例状态，
 * 这样上层可以在 USB 启动之前就注册，不必等 xbox360_register() 分配句柄 */
static void *s_sample_priv;
static xbox360_sample_cb_t s_sample_cb;

static void xbox360_pad_to_report(const struct xbox360_gamepad *pad, u8 *report);

static const u8 sXbox360EpIn[XBOX360_ITF_NUM] = {
    XBOX360_EP_IN_0, XBOX360_EP_IN_1, XBOX360_EP_IN_2,
};
static const u8 sXbox360Maxp[XBOX360_ITF_NUM] = {
    XBOX360_MAXP_IF0, XBOX360_MAXP_HID, XBOX360_MAXP_HID,
};

static u32 xbox360_report_input_raw(const usb_dev usb_id, u8 idx, const u8 *buf, u32 len);

/* ---------------- 端点中断 ---------------- */

static void xbox360_ep_in_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 len = sizeof(xbox360_hdl->report);

    if (xbox360_hdl == NULL || ep != sXbox360EpIn[0]) {
        return;
    }

    /* 注意：这里不能调 usb_clr_intr_txe()。本端点靠 TX 完成中断自维持上报，
     * 关掉 TXE 就再也收不到中断、流会断掉（做法同 webusb.c 的 webusb_epin_isr）。 */
    xbox360_hdl->ep_in_ready = 1;

    /* 每帧取一次最新状态。回调只做取数与换算，可安全在中断里执行；
     * 没注册回调就沿用 report 里的现有内容（xbox360_report_input 写入的） */
    if (s_sample_cb) {
        struct xbox360_gamepad pad = {0};
        s_sample_cb(s_sample_priv, &pad);
        xbox360_pad_to_report(&pad, xbox360_hdl->report);
    }

    if (usb_g_intr_write(usb_id, ep, xbox360_hdl->report, len) != len) {
        /* 续包失败：关掉 TXE 主动退出循环，避免 TXPktRdy 长时间为 0 造成中断风暴；
         * 下次枚举/复位时 xbox360_ep_init() 会重新打开 */
        log_warn("xbox360 ep%d tx fail, stop streaming", ep);
        usb_clr_intr_txe(usb_id, ep);
    }
}

static void xbox360_ep_out_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 buf[64];
    u32 len;

    /* 中断上下文，block 必须为 0 */
    len = usb_g_intr_read(usb_id, ep, buf, sizeof(buf), 0);
    if (len == 0 || xbox360_hdl == NULL || xbox360_hdl->output_cb == NULL) {
        return;
    }
    xbox360_hdl->output_cb(xbox360_hdl->output_priv, (u8)(ep - 1), buf, len);
}

/* ---------------- 端点配置 ---------------- */

static void xbox360_ep_init(struct usb_device_t *usb_device, u8 idx)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 ep = sXbox360EpIn[idx];
    u8 maxp = sXbox360Maxp[idx];

    usb_g_ep_config(usb_id, ep | USB_DIR_IN, USB_ENDPOINT_XFER_INT,
                    0, xbox360_hdl->ep_in_dma[idx], maxp);
    usb_g_set_intr_hander(usb_id, ep | USB_DIR_IN, xbox360_ep_in_isr);
    usb_enable_ep(usb_id, ep);
    usb_set_intr_txe(usb_id, ep);

    /* OUT 端点与 IN 端点同号 */
    usb_g_ep_config(usb_id, ep, USB_ENDPOINT_XFER_INT,
                    0, xbox360_hdl->ep_out_dma[idx], maxp);
    usb_g_set_intr_hander(usb_id, ep, xbox360_ep_out_isr);
    usb_enable_ep(usb_id, ep);
    usb_set_intr_rxe(usb_id, ep);
}

/* 只在当前接口的端点上操作，避免 flush 其他接口正在通信的端点 */
static void xbox360_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 idx;

    if (xbox360_hdl == NULL || itf < xbox360_hdl->itf_base) {
        return;
    }
    idx = (u8)(itf - xbox360_hdl->itf_base);
    if (idx >= XBOX360_ITF_NUM) {
        return;
    }
    log_info("xbox360_reset itf=%d idx=%d", itf, idx);

    xbox360_hdl->ep_in_ready = 0;
    xbox360_ep_init(usb_device, idx);
    xbox360_hdl->cfg_done = 1;

    /* 必须主动发第一包：只有发出去且被主机取走，才会产生 TX 完成中断，
     * 中断里再续下一包，从而形成持续上报（同 hid.c 的 hid_endpoint_init） */
    if (idx == 0) {
        usb_g_intr_write(usb_id, sXbox360EpIn[0], xbox360_hdl->report, sizeof(xbox360_hdl->report));
    }
}

/* ---------------- EP0 接口请求 ---------------- */

/* IF0：XID 厂商接口 */
static u32 xbox360_itf0_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);

    if (bRequestType == USB_TYPE_VENDOR && req->bRequest == XBOX360_REQ_XID) {
        /* wLength 20 / 8 / 4 都会按 wLength 截断，同一份数据即可 */
        usb_set_data_payload(usb_device, req, sXidCapabilities, sizeof(sXidCapabilities));
        return 0;
    }

    if (bRequestType == USB_TYPE_STANDARD) {
        switch (req->bRequest) {
        case USB_REQ_SET_INTERFACE:
            if (req->wValue == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            return 0;
        case USB_REQ_GET_INTERFACE:
            if (req->wValue || req->wLength != 1) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, 1);
            }
            return 0;
        }
    }

    /* XID 接口不响应其余请求，避免影响主机的探测时序 */
    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    return 0;
}

/* IF1 / IF2：厂商 HID 接口（idx：0 = IF1，1 = IF2） */
static u32 xbox360_hid_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *req, u8 idx)
{
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            if (HIBYTE(req->wValue) == USB_HID_DT_REPORT) {
                usb_set_data_payload(usb_device, req, sHidReportDesc[idx], sHidReportDescLen[idx]);
            } else if (HIBYTE(req->wValue) == USB_HID_DT_HID) {
                memcpy(tx_payload, sXbox360Config + sHidDescOffset[idx], 9);
                usb_set_data_payload(usb_device, req, tx_payload, 9);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            return 0;
        case USB_REQ_SET_INTERFACE:
            if (req->wValue == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            return 0;
        case USB_REQ_GET_INTERFACE:
            if (req->wValue || req->wLength != 1) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, 1);
            }
            return 0;
        }
        break;

    case USB_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            xbox360_hdl->hid_idle[idx] = HIBYTE(req->wValue);
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            return 0;
        case USB_REQ_GET_IDLE:
            tx_payload[0] = xbox360_hdl->hid_idle[idx];
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 0;
        case USB_REQ_GET_PROTOCOL:
            tx_payload[0] = 0x01;   /* 非 boot 协议 */
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 0;
        case USB_REQ_SET_PROTOCOL:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            return 0;
        }
        break;
    }

    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    return 0;
}

static u32 xbox360_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u8 idx;

    if (xbox360_hdl == NULL) {
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        return 0;
    }
    idx = (u8)(LOBYTE(req->wIndex) - xbox360_hdl->itf_base);
    if (idx == 0) {
        return xbox360_itf0_handler(usb_device, req);
    }
    if (idx - 1 < XBOX360_HID_ITF_NUM) {
        return xbox360_hid_handler(usb_device, req, idx - 1);
    }
    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    return 0;
}

/* ---------------- 设备收件人请求（MS OS / XID） ---------------- */

u8 xbox360_usb_mode_active(void)
{
    return (xbox360_hdl != NULL) ? 1 : 0;
}

u32 xbox360_setup_device_hook(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    /* 未处于手柄模式时不接管任何设备收件人请求 */
    if (xbox360_hdl == NULL) {
        return 0;
    }

    /* MS OS 1.0 握手：GET_DESCRIPTOR(String, 0xEE) */
    if (req->bRequest == USB_REQ_GET_DESCRIPTOR
        && HIBYTE(req->wValue) == USB_DT_STRING
        && LOBYTE(req->wValue) == XBOX360_MS_OS_STR_INDEX) {
        usb_set_data_payload(usb_device, req, sMsOsStringDesc, sizeof(sMsOsStringDesc));
        return 1;
    }

    /* MS OS 扩展兼容 ID 描述符：wLength 16 / 40 都按 wLength 截断 */
    if (req->bRequest == XBOX360_REQ_MS_OS && req->wIndex == XBOX360_MS_OS_INDEX) {
        usb_set_data_payload(usb_device, req, sMsExtendedCompatId, sizeof(sMsExtendedCompatId));
        return 1;
    }

    /* MS OS 扩展属性描述符：只回 10 字节头，wCount = 0 表示没有属性 */
    if (req->bRequest == XBOX360_REQ_MS_OS && req->wIndex == XBOX360_MS_OS_PROP_INDEX) {
        usb_set_data_payload(usb_device, req, sMsExtendedProperties, sizeof(sMsExtendedProperties));
        return 1;
    }

    /* XID 能力描述符（设备收件人，wLength = 4） */
    if (req->bRequest == XBOX360_REQ_XID && req->wValue == 0x0000) {
        usb_set_data_payload(usb_device, req, sXidCapabilities, sizeof(sXidCapabilities));
        return 1;
    }

    return 0;
}

/* ---------------- 标准三函数 ---------------- */

u32 xbox360_register(const usb_dev usb_id)
{
    u8 i;

    if (xbox360_hdl) {
        return 0;
    }

    /* 每帧的输入状态统一由汇聚层提供。具体是本机摇杆 ADC 还是无线传送来的，
     * 由应用层运行期调 gamepad_input_select() 决定，本驱动不关心也不做编译期绑定 */
    xbox360_set_sample_cb(NULL, gamepad_input_get);

#if TCFG_JOYSTICK_GPADC_ENABLE
    /* 本机摇杆是常驻的输入源，跟手柄一起初始化。joystick_init() 内部幂等：
     * 重复进入 USB 模式不会重复申请资源；它会在动手前先 adc_suspend()
     * 让 SDK 的 gpadc 驱动释放 ADC 硬件，并向汇聚层注册 GAMEPAD_SRC_LOCAL */
    joystick_init();
#endif

    xbox360_hdl = zalloc(sizeof(*xbox360_hdl));
    if (xbox360_hdl == NULL) {
        return -1;
    }

    /* 中立报文：类型 0x00、长度 20，其余全 0 */
    xbox360_hdl->report[0] = XBOX360_REPORT_TYPE;
    xbox360_hdl->report[1] = XBOX360_INPUT_REPORT_LEN;

    for (i = 0; i < XBOX360_ITF_NUM; i++) {
        u8 ep = sXbox360EpIn[i];
        u8 maxp = sXbox360Maxp[i];
        xbox360_hdl->ep_in_dma[i] = usb_alloc_ep_dmabuffer(usb_id, ep | USB_DIR_IN, maxp);
        xbox360_hdl->ep_out_dma[i] = usb_alloc_ep_dmabuffer(usb_id, ep, maxp);
    }
    return 0;
}

u32 xbox360_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    u8 i;

    if (xbox360_hdl == NULL) {
        return 0;
    }

    memcpy(ptr, sXbox360Config, sizeof(sXbox360Config));

    xbox360_hdl->itf_base = (u8)(*cur_itf_num);
    log_info("xbox360 desc_config itf_base=%d", xbox360_hdl->itf_base);

    /* 接口号按当前起始号回填 */
    ptr[XB_OFF_IFNUM_0] = (u8)(*cur_itf_num + 0);
    ptr[XB_OFF_IFNUM_1] = (u8)(*cur_itf_num + 1);
    ptr[XB_OFF_IFNUM_2] = (u8)(*cur_itf_num + 2);

    /* HID 描述符里的 report 描述符长度 */
    for (i = 0; i < XBOX360_HID_ITF_NUM; i++) {
        u16 off = sHidDescOffset[i] + XB_OFF_REPLEN_REL;
        ptr[off]     = LOBYTE(sHidReportDescLen[i]);
        ptr[off + 1] = HIBYTE(sHidReportDescLen[i]);
    }

    for (i = 0; i < XBOX360_ITF_NUM; i++) {
        if (usb_set_interface_hander(usb_id, *cur_itf_num + i, xbox360_itf_hander) != *cur_itf_num + i) {
            ASSERT(0, "xbox360 set interface_hander fail");
        }
        if (usb_set_reset_hander(usb_id, *cur_itf_num + i, xbox360_reset) != *cur_itf_num + i) {
            ASSERT(0, "xbox360 set interface_reset_hander fail");
        }
    }

    *cur_itf_num += XBOX360_ITF_NUM;
    return sizeof(sXbox360Config);
}

void xbox360_release(const usb_dev usb_id)
{
    u8 i;

    if (xbox360_hdl == NULL) {
        return;
    }
    for (i = 0; i < XBOX360_ITF_NUM; i++) {
        usb_free_ep_dmabuffer(usb_id, xbox360_hdl->ep_in_dma[i]);
        usb_free_ep_dmabuffer(usb_id, xbox360_hdl->ep_out_dma[i]);
    }
    free(xbox360_hdl);
    xbox360_hdl = NULL;
}

/* ---------------- 对外 API ---------------- */

static u32 xbox360_report_input_raw(const usb_dev usb_id, u8 idx, const u8 *buf, u32 len)
{
    if (xbox360_hdl == NULL || buf == NULL) {
        return 0;
    }
    if (len > sXbox360Maxp[idx]) {
        len = sXbox360Maxp[idx];
    }
    return usb_g_intr_write(usb_id, sXbox360EpIn[idx], buf, len);
}

void usb_xbox360_start(const usb_dev usb_id)
{
    usb_pause(usb_id);
    usb_device_mode(usb_id, XBOX360_CLASS | HID_KBMOUSE_CLASS);
}

void usb_xbox360_stop(const usb_dev usb_id)
{
    usb_device_mode(usb_id, 0);
}

u8 xbox360_is_ready(void)
{
    return (xbox360_hdl && xbox360_hdl->cfg_done && xbox360_hdl->ep_in_ready) ? 1 : 0;
}

u32 xbox360_report_input(const usb_dev usb_id, const u8 *report, u32 len)
{
    if (xbox360_hdl == NULL || report == NULL) {
        return 0;
    }
    if (len > sizeof(xbox360_hdl->report)) {
        len = sizeof(xbox360_hdl->report);
    }
    (void)usb_id;

    /* 只更新内容，实际发送由 EP1 的 TX 完成中断持续驱动。
     * 关中断拷贝，避免中断里取到写了一半的报文 */
    CPU_CRITICAL_ENTER();
    memcpy(xbox360_hdl->report, report, len);
    CPU_CRITICAL_EXIT();

    return len;
}

/* 把手柄字段组包成 20 字节 XInput 输入报文。调用前 report 至少 20 字节 */
static void xbox360_pad_to_report(const struct xbox360_gamepad *pad, u8 *report)
{
    report[0] = XBOX360_REPORT_TYPE;
    report[1] = XBOX360_INPUT_REPORT_LEN;
    report[2] = LOBYTE(pad->buttons);
    report[3] = HIBYTE(pad->buttons);
    report[4] = pad->lt;
    report[5] = pad->rt;
    report[6]  = LOBYTE(pad->lx);
    report[7]  = HIBYTE(pad->lx);
    report[8]  = LOBYTE(pad->ly);
    report[9]  = HIBYTE(pad->ly);
    report[10] = LOBYTE(pad->rx);
    report[11] = HIBYTE(pad->rx);
    report[12] = LOBYTE(pad->ry);
    report[13] = HIBYTE(pad->ry);
}

u32 xbox360_report_gamepad(const usb_dev usb_id, const struct xbox360_gamepad *pad)
{
    u8 report[XBOX360_INPUT_REPORT_LEN] = {0};

    if (pad == NULL) {
        return 0;
    }
    xbox360_pad_to_report(pad, report);

    return xbox360_report_input(usb_id, report, sizeof(report));
}

u32 xbox360_report_input_if(const usb_dev usb_id, u8 if_idx, const u8 *buf, u32 len)
{
    if (if_idx >= XBOX360_ITF_NUM) {
        return 0;
    }
    /* IF0 是自维持上报流，只更新内容；IF1/IF2 按需直接发 */
    if (if_idx == 0) {
        return xbox360_report_input(usb_id, buf, len);
    }
    return xbox360_report_input_raw(usb_id, if_idx, buf, len);
}

void xbox360_set_output_cb(void *priv, xbox360_output_cb_t cb)
{
    if (xbox360_hdl == NULL) {
        return;
    }
    xbox360_hdl->output_priv = priv;
    xbox360_hdl->output_cb = cb;
}

void xbox360_set_sample_cb(void *priv, xbox360_sample_cb_t cb)
{
    /* 先设 priv 再设 cb：中断里判断 cb 非空才会用到 priv。
     * 不依赖句柄，可在 USB 启动前调用 */
    s_sample_priv = priv;
    s_sample_cb = cb;
}

u32 xbox360_parse_output(const u8 *buf, u32 len, xbox360_output_t *out)
{
    if (buf == NULL || out == NULL || len < XBOX360_OUTPUT_REPORT_LEN) {
        return 1;
    }
    out->type = buf[0];
    out->b1   = buf[1];
    out->b2   = buf[2];
    return 0;
}

USB_CLASS_REGISTER(xbox360_desc_config, xbox360_register, xbox360_release,
                   TCFG_USB_XBOX360_INDEX, XBOX360_CLASS, 0);
#endif
