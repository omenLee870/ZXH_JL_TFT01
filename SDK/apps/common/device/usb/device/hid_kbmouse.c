/* Copyright(C) JieLi Technology
 * 标准键鼠 HID 从机驱动
 *
 * 一个可参数化的标准 HID 接口实现：报告描述符、子类/协议、端点对、包长、
 * bInterval、bcdHID 全部由 usb_std_class_def.h 里的宏配置，便于复用到其他产品。
 * 当前实例化为「Boot Mouse + Report ID 2 键盘」的复合描述符（97 字节）。
 *
 * 描述符按参考设备逐字节复刻，见 apps/common/device/usb/docs/usb_device_class_guide.md。
 * 本驱动使用 EP3，不能与同样占用 EP3 的 UAC 同时使能。
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
#include "usb/device/hid_kbmouse.h"
#include "app_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[HID_KB]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_HID_KBMOUSE_ENABLE

/* 取自参考设备 cfg.bin 的 cfg[81..112]，共 32 字节。
 * 接口号与 report 描述符长度在运行期回填。 */
#define HID_KBMOUSE_OFF_IFNUM       2
#define HID_KBMOUSE_OFF_HIDDESC     9
/* HID 描述符内 wDescriptorLength 的偏移（HID 描述符内第 7 字节） */
#define HID_KBMOUSE_OFF_REPLEN      (HID_KBMOUSE_OFF_HIDDESC + 7)

static const u8 sHidKbMouseConfig[] = {
    /* 接口描述符：bInterfaceNumber 运行期回填 */
    USB_DT_INTERFACE_SIZE,      // bLength
    USB_DT_INTERFACE,           // bDescriptorType
    0x00,                       // bInterfaceNumber（运行期回填）
    0x00,                       // bAlternateSetting
    0x02,                       // bNumEndpoints（IN + OUT）
    USB_CLASS_HID,              // bInterfaceClass
    HID_KBMOUSE_SUBCLASS,       // bInterfaceSubClass
    HID_KBMOUSE_PROTOCOL,       // bInterfaceProtocol
    0x00,                       // iInterface（无字符串）
    /* HID 描述符：wDescriptorLength 运行期回填 */
    USB_HID_DESC_SIZE,          // bLength
    USB_HID_DT_HID,             // bDescriptorType = 0x21
    LOBYTE(HID_KBMOUSE_BCD_HID), HIBYTE(HID_KBMOUSE_BCD_HID),   // bcdHID
    0x00,                       // bCountryCode = 0（不本地化）
    0x01,                       // bNumDescriptors
    USB_HID_DT_REPORT,          // bDescriptorType = Report
    0x00, 0x00,                 // wDescriptorLength（运行期回填）
    /* EP IN：中断 */
    USB_DT_ENDPOINT_SIZE,                       // bLength
    USB_DT_ENDPOINT,                            // bDescriptorType
    (USB_DIR_IN | HID_KBMOUSE_EP_IN),           // bEndpointAddress
    USB_ENDPOINT_XFER_INT,                      // bmAttributes
    LOBYTE(HID_KBMOUSE_MAXP), HIBYTE(HID_KBMOUSE_MAXP),         // wMaxPacketSize
    HID_KBMOUSE_INTR_INTERVAL,                  // bInterval
    /* EP OUT：中断 */
    USB_DT_ENDPOINT_SIZE,                       // bLength
    USB_DT_ENDPOINT,                            // bDescriptorType
    HID_KBMOUSE_EP_OUT,                         // bEndpointAddress
    USB_ENDPOINT_XFER_INT,                      // bmAttributes
    LOBYTE(HID_KBMOUSE_MAXP), HIBYTE(HID_KBMOUSE_MAXP),         // wMaxPacketSize
    HID_KBMOUSE_INTR_INTERVAL_OUT,              // bInterval
};

/* 报告描述符：Report ID 1 = 鼠标（3 按键 + 5bit 填充 + X/Y/Wheel），
 *              Report ID 2 = 键盘（8 修饰键 + 7 键码） */
static const u8 sHidKbMouseReportDesc[] = {
    /* ---- 鼠标集合 ---- */
    USAGE_PAGE(1, GENERIC_DESKTOP_CTRLS),   //Generic Desktop
    USAGE(1, MOUSE),
    COLLECTION(1, APPLICATION),

    REPORT_ID(1, HID_KBMOUSE_REPORT_MOUSE),

    USAGE(1, POINTER),
    COLLECTION(1, PHYSICAL),
    USAGE_PAGE(1, BUTTON),
    USAGE_MIN(1, 0x01),
    USAGE_MAX(1, 0x03),                     //3 个按键
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0x01),
    REPORT_SIZE(1, 0x01),
    REPORT_COUNT(1, 0x03),
    INPUT(1, 0x02),                         //Data,Var,Abs

    REPORT_SIZE(1, 0x05),
    REPORT_COUNT(1, 0x01),
    INPUT(1, 0x01),                         //Const,Array,Abs —— 填充到字节边界

    USAGE_PAGE(1, GENERIC_DESKTOP_CTRLS),
    USAGE(1, X_AXIS),
    USAGE(1, Y_AXIS),
    USAGE(1, WHEEL),
    LOGICAL_MIN(1, -127),
    LOGICAL_MAX(1, 127),
    REPORT_SIZE(1, 0x08),
    REPORT_COUNT(1, 0x03),
    INPUT(1, 0x06),                         //Data,Var,Rel
    END_COLLECTION,
    END_COLLECTION,

    /* ---- 键盘集合 ---- */
    USAGE_PAGE(1, GENERIC_DESKTOP_CTRLS),
    USAGE(1, KEYBOARD),
    COLLECTION(1, APPLICATION),

    REPORT_ID(1, HID_KBMOUSE_REPORT_KEYBOARD),

    USAGE_PAGE(1, KEYBOARD_PAGE),
    USAGE_MIN(1, LEFT_CONTROL),
    USAGE_MAX(1, RIGHT_GUI),                //8 个修饰键
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0x01),
    REPORT_SIZE(1, 0x01),
    REPORT_COUNT(1, 0x08),
    INPUT(1, 0x02),                         //Data,Var,Abs

    REPORT_COUNT(1, 0x07),                  //保留 1 字节 + 6 个键码
    REPORT_SIZE(1, 0x08),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(2, 0xFF, 0x00),             //两字节小端，最高位在前
    USAGE_PAGE(1, KEYBOARD_PAGE),
    USAGE_MIN(1, 0x00),
    USAGE_MAX(2, 0xFF, 0x00),
    INPUT(1, 0x00),                         //Data,Array,Abs
    END_COLLECTION,
};

struct hid_kbmouse_hdl {
    u8  ep_in_ready;
    u8  ready;
    u8 *ep_in_dma;
    u8 *ep_out_dma;
};

static struct hid_kbmouse_hdl *hid_kbmouse_hdl;

static u32 hid_kbmouse_report_raw(const usb_dev usb_id, const u8 *buf, u32 len);

/* ---------------- 端点中断 ---------------- */

static void hid_kbmouse_ep_in_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    usb_clr_intr_txe(usb_id, ep);
    if (hid_kbmouse_hdl) {
        hid_kbmouse_hdl->ep_in_ready = 1;
    }
}

static void hid_kbmouse_ep_out_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 buf[HID_KBMOUSE_MAXP];

    /* 中断上下文，block 必须为 0；键鼠输出报文（LED/键盘灯）当前不处理 */
    (void)usb_g_intr_read(usb_id, ep, buf, sizeof(buf), 0);
}

static void hid_kbmouse_ep_init(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    usb_g_ep_config(usb_id, HID_KBMOUSE_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT,
                    0, hid_kbmouse_hdl->ep_in_dma, HID_KBMOUSE_MAXP);
    usb_g_set_intr_hander(usb_id, HID_KBMOUSE_EP_IN | USB_DIR_IN, hid_kbmouse_ep_in_isr);
    usb_enable_ep(usb_id, HID_KBMOUSE_EP_IN);
    usb_set_intr_txe(usb_id, HID_KBMOUSE_EP_IN);

    usb_g_ep_config(usb_id, HID_KBMOUSE_EP_OUT, USB_ENDPOINT_XFER_INT,
                    0, hid_kbmouse_hdl->ep_out_dma, HID_KBMOUSE_MAXP);
    usb_g_set_intr_hander(usb_id, HID_KBMOUSE_EP_OUT, hid_kbmouse_ep_out_isr);
    usb_enable_ep(usb_id, HID_KBMOUSE_EP_OUT);
    usb_set_intr_rxe(usb_id, HID_KBMOUSE_EP_OUT);
}

static void hid_kbmouse_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    /* 全 0 的鼠标报文即中立状态 */
    const u8 idle_report[HID_KBMOUSE_REPORT_LEN] = { HID_KBMOUSE_REPORT_MOUSE, 0, 0, 0 };

    if (hid_kbmouse_hdl == NULL) {
        return;
    }
    log_info("hid_kbmouse_reset itf=%d", itf);

    hid_kbmouse_hdl->ep_in_ready = 0;
    hid_kbmouse_ep_init(usb_device);
    hid_kbmouse_hdl->ready = 1;

    /* 先送一包中立报文，主机取走后 TX 完成中断会置 ep_in_ready */
    hid_kbmouse_report_raw(usb_id, idle_report, sizeof(idle_report));
}

/* ---------------- EP0 请求 ---------------- */

static u32 hid_kbmouse_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            if (HIBYTE(req->wValue) == USB_HID_DT_REPORT) {
                usb_set_data_payload(usb_device, req, sHidKbMouseReportDesc,
                                     sizeof(sHidKbMouseReportDesc));
            } else if (HIBYTE(req->wValue) == USB_HID_DT_HID) {
                memcpy(tx_payload, sHidKbMouseConfig + HID_KBMOUSE_OFF_HIDDESC, 9);
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
        case USB_REQ_SET_PROTOCOL:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            return 0;
        case USB_REQ_GET_IDLE:
            tx_payload[0] = 0x00;
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 0;
        case USB_REQ_GET_PROTOCOL:
            tx_payload[0] = 0x01;   /* 非 boot 协议 */
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 0;
        }
        break;
    }

    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    return 0;
}

/* ---------------- 标准三函数 ---------------- */

u32 hid_kbmouse_register(const usb_dev usb_id)
{
    if (hid_kbmouse_hdl) {
        return 0;
    }
    hid_kbmouse_hdl = zalloc(sizeof(*hid_kbmouse_hdl));
    if (hid_kbmouse_hdl == NULL) {
        return -1;
    }
    hid_kbmouse_hdl->ep_in_dma =
        usb_alloc_ep_dmabuffer(usb_id, HID_KBMOUSE_EP_IN | USB_DIR_IN, HID_KBMOUSE_MAXP);
    hid_kbmouse_hdl->ep_out_dma =
        usb_alloc_ep_dmabuffer(usb_id, HID_KBMOUSE_EP_OUT, HID_KBMOUSE_MAXP);
    return 0;
}

u32 hid_kbmouse_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    if (hid_kbmouse_hdl == NULL) {
        return 0;
    }

    memcpy(ptr, sHidKbMouseConfig, sizeof(sHidKbMouseConfig));

    ptr[HID_KBMOUSE_OFF_IFNUM] = (u8)(*cur_itf_num);
    ptr[HID_KBMOUSE_OFF_REPLEN]     = LOBYTE(sizeof(sHidKbMouseReportDesc));
    ptr[HID_KBMOUSE_OFF_REPLEN + 1] = HIBYTE(sizeof(sHidKbMouseReportDesc));

    log_info("hid_kbmouse desc_config itf=%d", *cur_itf_num);

    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_kbmouse_itf_hander) != *cur_itf_num) {
        ASSERT(0, "hid_kbmouse set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_kbmouse_reset) != *cur_itf_num) {
        ASSERT(0, "hid_kbmouse set interface_reset_hander fail");
    }

    *cur_itf_num += 1;
    return sizeof(sHidKbMouseConfig);
}

void hid_kbmouse_release(const usb_dev usb_id)
{
    if (hid_kbmouse_hdl == NULL) {
        return;
    }
    usb_free_ep_dmabuffer(usb_id, hid_kbmouse_hdl->ep_in_dma);
    usb_free_ep_dmabuffer(usb_id, hid_kbmouse_hdl->ep_out_dma);
    free(hid_kbmouse_hdl);
    hid_kbmouse_hdl = NULL;
}

/* ---------------- 对外 API ---------------- */

static u32 hid_kbmouse_report_raw(const usb_dev usb_id, const u8 *buf, u32 len)
{
    if (hid_kbmouse_hdl == NULL || buf == NULL) {
        return 0;
    }
    if (len > HID_KBMOUSE_MAXP) {
        len = HID_KBMOUSE_MAXP;
    }
    return usb_g_intr_write(usb_id, HID_KBMOUSE_EP_IN, buf, len);
}

u32 hid_kbmouse_report(const usb_dev usb_id, const u8 *report, u32 len)
{
    return hid_kbmouse_report_raw(usb_id, report, len);
}

u32 hid_kbmouse_report_mouse(const usb_dev usb_id, u8 buttons, s8 x, s8 y, s8 wheel)
{
    u8 report[HID_KBMOUSE_REPORT_LEN];

    report[0] = HID_KBMOUSE_REPORT_MOUSE;
    report[1] = buttons;
    report[2] = (u8)x;
    report[3] = (u8)y;
    report[4] = (u8)wheel;

    return hid_kbmouse_report_raw(usb_id, report, sizeof(report));
}

u32 hid_kbmouse_report_keyboard(const usb_dev usb_id, u8 modifier, const u8 *keycode)
{
    u8 report[HID_KBMOUSE_KEYBOARD_LEN] = {0};
    u8 i;

    report[0] = HID_KBMOUSE_REPORT_KEYBOARD;
    report[1] = modifier;
    /* report[2] 是保留字节，固定为 0 */
    if (keycode) {
        for (i = 0; i < HID_KBMOUSE_KEYCODE_MAX; i++) {
            report[3 + i] = keycode[i];
        }
    }
    return hid_kbmouse_report_raw(usb_id, report, sizeof(report));
}

u8 hid_kbmouse_is_ready(void)
{
    return (hid_kbmouse_hdl && hid_kbmouse_hdl->ready && hid_kbmouse_hdl->ep_in_ready) ? 1 : 0;
}

USB_CLASS_REGISTER(hid_kbmouse_desc_config, hid_kbmouse_register, hid_kbmouse_release,
                   TCFG_USB_HID_KBMOUSE_INDEX, HID_KBMOUSE_CLASS, 0);
#endif
