#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb/usb_config.h"
#include "usb/device/cdc.h"
#include "app_config.h"
#include "os/os_api.h"
#include "webusb.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[WEBUSB]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_WEBUSB_ENABLE

/* ======================== 常量定义 ======================== */
#define     MAXP_SIZE_WEBUSBIN      64
#define     MAXP_SIZE_WEBUSBOUT     64
#define     CTRL_MAX_PAYLOAD        64  /* USB 1.1 EP0 max packet size */

/* Vendor Request IDs */
#define     WEBUSB_VENDOR_ECHO_CTRL     0x20
#define     WEBUSB_VENDOR_SET_ECHO      0x21
#define     WEBUSB_VENDOR_GET_ECHO      0x22
#define     WEBUSB_VENDOR_CUSTOM_CMD    0x23
#define     WEBUSB_VENDOR_BULK_TX       0x24

/* MS OS 2.0 / WebUSB 描述符常量 */
#define MS_OS_20_SET_HEADER_DESCRIPTOR           0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION     0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION          0x02
#define MS_OS_20_FEATURE_COMPATIBLE_ID           0x03
#define MS_OS_20_FEATURE_REG_PROPERTY            0x04
#define MS_OS_20_DESCRIPTOR_LENGTH 0xb2
#define MS_OS20_OFFSET_BFIRSTINTERFACE           (0x0A + 0x08 + 0x04)
/* ======================== BOS / 描述符 Hook ======================== */

#define WEBUSB_REQUEST_GET_URL         0x02
#define MS_OS_20_REQUEST_DESCRIPTOR    0x07
#define     WEBUSB_REQ_URL          (1)
#define     WEBUSB_REQ_MS_DESC      (2)

#define     WEBUSB_iInterface       0

/* ======================== 数据结构 ======================== */
typedef void (*webusb_rx_handle_t)(void *hdl, u8 *buffer, u32 len);

struct webusb_handle_t {
    u8 *ep_out_dmabuffer;
    u8 *ep_in_dmabuffer;
    void (*wakeup_handle)(struct usb_device_t *usb_device);
    void *priv_hdl;
    webusb_rx_handle_t rx_hook;
    u8 echo_enable;       /* Echo 模式状态 */
    u32 ctrl_file_total;  /* Control Transfer 文件传输状态 */
    u32 ctrl_file_seq;
    u8 ctrl_echo_buf[CTRL_MAX_PAYLOAD]; /* Control Transfer Echo 缓冲 */
    u8 ctrl_echo_len;
    u32 bulk_tx_remaining; /* Bulk TX 状态（固件主动发数据） */
    u32 bulk_tx_seq;
};

static struct webusb_handle_t *webusb_handle = NULL;
static u8 webusb_interface_num;

static void webusb_epout_isr(struct usb_device_t *usb_device, u32 ep);
static void webusb_epin_isr(struct usb_device_t *usb_device, u32 ep);

/* ======================== USB 描述符 ======================== */

static const u8 sWebUSBDescriptor[] = {
    /* Interface Descriptor */
    0x09,                           // bLength
    0x04,                           // bDescriptorType = Interface
    0x00,                           // bInterfaceNumber (运行时填充)
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints = 2
    0xFF,                           // bInterfaceClass = Vendor Specific
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol
    WEBUSB_iInterface,              // iInterface

    /* Endpoint Descriptor IN */
    USB_DT_ENDPOINT_SIZE,           // bLength
    USB_DT_ENDPOINT,                // bDescriptorType
    USB_DIR_IN | WEBUSB_EP_IN,      // bEndpointAddress
    USB_ENDPOINT_XFER_BULK,         // bmAttributes
    LOBYTE(MAXP_SIZE_WEBUSBIN),
    HIBYTE(MAXP_SIZE_WEBUSBIN),     // wMaxPacketSize
    0,                              // bInterval

    /* Endpoint Descriptor OUT */
    USB_DT_ENDPOINT_SIZE,           // bLength
    USB_DT_ENDPOINT,                // bDescriptorType
    WEBUSB_EP_OUT,                  // bEndpointAddress
    USB_ENDPOINT_XFER_BULK,         // bmAttributes
    LOBYTE(MAXP_SIZE_WEBUSBOUT),
    HIBYTE(MAXP_SIZE_WEBUSBOUT),    // wMaxPacketSize
    0,                              // bInterval
};

/* ---- BOS Descriptor (WebUSB + MS OS 2.0) ---- */
static const uint8_t bos_descriptor[] = {
    /* BOS Descriptor */
    0x05,       // bLength
    0x0F,       // bDescriptorType = BOS
    0x39, 0x00, // wTotalLength = 57
    0x02,       // bNumDeviceCaps = 2

    /* WebUSB Platform Capability Descriptor */
    0x18,       // bLength = 24
    0x10,       // bDescriptorType = Device Capability
    0x05,       // bDevCapabilityType = PLATFORM
    0x00,       // bReserved
    0x38, 0xB6, 0x08, 0x34,
    0xA9, 0x09,
    0xA0, 0x47,
    0x8B, 0xFD,
    0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
    0x00, 0x01,                 // bcdVersion 1.0
    WEBUSB_REQ_URL,             // bVendorCode (for WebUSB requests url)
    0x01,                       // iLandingPage

    /* Microsoft OS 2.0 Platform Capability Descriptor */
    0x1C,       // bLength = 28
    0x10,       // bDescriptorType = Device Capability
    0x05,       // bDevCapabilityType = PLATFORM
    0x00,       // bReserved
    0xDF, 0x60, 0xDD, 0xD8,
    0x89, 0x45,
    0xC7, 0x4C,
    0x9C, 0xD2,
    0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,     // dwWindowsVersion
    MS_OS_20_DESCRIPTOR_LENGTH, 0x00,
    WEBUSB_REQ_MS_DESC,          // bMS_VendorCode
    0x00,                        // bAltEnumCode
};

/* ---- Microsoft OS 2.0 Descriptor Set ---- */
static const uint8_t ms_os_20_descriptor[] = {
    0x0A, 0x00,                                          // wLength
    MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00,                // wDescriptorType
    0x00, 0x00, 0x03, 0x06,                              // dwWindowsVersion
    MS_OS_20_DESCRIPTOR_LENGTH, 0x00,                    // wTotalLength

    0x08, 0x00,             // wLength
    0x01, 0x00,             // wDescriptorType = Configuration Subset Header
    0x00,                   // bConfigurationValue
    0x00,                   // bReserved
    0xA8, 0x00,             // wTotalLength for subset (示例，文档填 B2)
    0x08, 0x00,             // wLength
    0x02, 0x00,             // wDescriptorType = Function Subset Header
    0x00,                   // bFirstInterface = 1
    0x00,                   // bReserved
    0xA0, 0x00,             // wSubsetLength = 168 bytes (示例文档)
    0x14, 0x00,                                          // wLength
    MS_OS_20_FEATURE_COMPATIBLE_ID, 0x00,                // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,           // compatible ID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // sub-compatible ID
    0x84, 0x00,             // wLength = 132 bytes (0x84)
    0x04, 0x00,             // wDescriptorType = Registry Property
    0x07, 0x00,             // wPropertyDataType = REG_MULTI_SZ
    0x2A, 0x00,             // wNameLength = 42 bytes
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0,
    'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0,
    0x00, 0x00, // null terminator
    0x50, 0x00,             // wPropertyDataLength = 0x50 = 80 bytes
    '{', 0, 'F', 0, '2', 0, '3', 0, 'A', 0, '1', 0, 'C', 0, '4', 0, 'D', 0, '-', 0,
    '7', 0, 'E', 0, '8', 0, 'B', 0, '-', 0,
    '4', 0, 'D', 0, '9', 0, '2', 0, '-', 0,
    '9', 0, 'A', 0, '0', 0, 'F', 0, '-', 0,
    '1', 0, 'B', 0, '2', 0, 'C', 0, '3', 0, 'D', 0, '4', 0, 'E', 0, '5', 0, 'F', 0, '6', 0, '0', 0, '}', 0,
    0x00, 0x00, 0x00, 0x00      // 双 null 终止符
};

/* ---- WebUSB URL Descriptor ---- */
static const uint8_t webusb_url_descriptor[] = {
    3 + 16,     // bLength
    0x03,       // bDescriptorType = URL
    0x01,       // bScheme = https://
    'd', 'o', 'c', '.', 'z', 'h', '-', 'j', 'i', 'e', 'l', 'i', '.', 'c', 'o', 'm'
};

/* ---- Device Qualifier Descriptor ---- */
static const uint8_t device_qualifier_descriptor[] = {
    0x0A,       // bLength
    0x06,       // bDescriptorType = DEVICE_QUALIFIER
    0x10, 0x02, // bcdUSB = 2.00
    0x00,       // bDeviceClass
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0 = 64
    0x01,       // bNumConfigurations
    0x00        // bReserved
};




u32 webusb_setup_device_hook(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    if (webusb_handle == NULL) {
        return 0;
    }
    u32 ret = 1;
    u32 h_wValue = HIBYTE(req->wValue);
    u32 l_wValue = LOBYTE(req->wValue);
    u8 *tx_payload = usb_get_setup_buffer(usb_device);

    if (req->bRequest == USB_REQ_GET_DESCRIPTOR) {
        if (h_wValue == USB_DT_BOS) {
            usb_set_data_payload(usb_device, req, bos_descriptor, sizeof(bos_descriptor));

        } else if (h_wValue == USB_DT_DEVICE_QUALIFIER) {
            usb_set_data_payload(usb_device, req, device_qualifier_descriptor, sizeof(device_qualifier_descriptor));

        } else {
            ret = 0;
        }
    } else if (req->bRequest == WEBUSB_REQ_MS_DESC && req->wIndex == MS_OS_20_REQUEST_DESCRIPTOR) {
        memcpy(tx_payload, ms_os_20_descriptor, sizeof(ms_os_20_descriptor));
        tx_payload[MS_OS20_OFFSET_BFIRSTINTERFACE] = webusb_interface_num;
        usb_set_data_payload(usb_device, req, tx_payload, sizeof(ms_os_20_descriptor));
    } else if (req->bRequest == WEBUSB_REQ_URL && req->wIndex == WEBUSB_REQUEST_GET_URL) {
        usb_set_data_payload(usb_device, req, webusb_url_descriptor, sizeof(webusb_url_descriptor));
    } else {
        ret = 0;
    }

    return ret;
}

/* ======================== 端点初始化 ======================== */

static void webusb_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_g_set_intr_hander(usb_id, WEBUSB_EP_OUT, webusb_epout_isr);
    usb_g_ep_config(usb_id, WEBUSB_EP_OUT, USB_ENDPOINT_XFER_BULK, 1, webusb_handle->ep_out_dmabuffer, MAXP_SIZE_WEBUSBOUT);

    usb_g_set_intr_hander(usb_id, WEBUSB_EP_IN | USB_DIR_IN, webusb_epin_isr);
    usb_g_ep_config(usb_id, WEBUSB_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, webusb_handle->ep_in_dmabuffer, MAXP_SIZE_WEBUSBIN);
    usb_enable_ep(usb_id, WEBUSB_EP_IN);
}

/* ======================== 公共 API ======================== */

u32 webusb_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len)
{
    if (webusb_handle == NULL) {
        return -1;
    }
    if (len > MAXP_SIZE_WEBUSBIN) {
        len = MAXP_SIZE_WEBUSBIN;
    }
    return usb_g_bulk_write(usb_id, WEBUSB_EP_IN, buffer, len);
}

void webusb_set_rx_hook(void *priv, void (*rx_hook)(void *priv, u8 *buf, u32 len))
{
    if (webusb_handle == NULL) {
        return;
    }
    webusb_handle->priv_hdl = priv;
    webusb_handle->rx_hook = rx_hook;
}

/* ======================== Reset ======================== */

static void webusb_reset(struct usb_device_t *usb_device, u32 itf)
{
    webusb_endpoint_init(usb_device, itf);
}

/* ======================== Bulk OUT ISR（接收 + Echo 回传）======================== */

static void webusb_epout_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 buf[MAXP_SIZE_WEBUSBOUT];
    int len = usb_g_bulk_read(usb_id, ep, buf, sizeof(buf), 0);

    if (len <= 0) {
        return;
    }

    if (webusb_handle->rx_hook) {
        webusb_handle->rx_hook(webusb_handle->priv_hdl, buf, len);
    } else {
        log_debug("webusb_recv: len = %d\n", len);
        /* Echo 模式：将收到的数据原样通过 IN 端点回传 */
        if (webusb_handle->echo_enable && len > 0) {
            usb_g_bulk_write(usb_id, WEBUSB_EP_IN, buf, len);
        }
    }
}
/* ---- Bulk TX 发一包，返回包长度 ---- */
static u32 bulk_tx_send_packet(const usb_dev usb_id)
{
    if (webusb_handle->bulk_tx_remaining == 0) {
        return 0;
    }
    u8 buf[MAXP_SIZE_WEBUSBIN];
    u32 len = webusb_handle->bulk_tx_remaining > MAXP_SIZE_WEBUSBIN ? MAXP_SIZE_WEBUSBIN : webusb_handle->bulk_tx_remaining;
    memset(buf, webusb_handle->bulk_tx_seq, len);
    webusb_handle->bulk_tx_seq += len;
    webusb_handle->bulk_tx_remaining -= len;
    usb_g_bulk_write(usb_id, WEBUSB_EP_IN, buf, len);
    if (webusb_handle->bulk_tx_remaining == 0 && len == MAXP_SIZE_WEBUSBIN) {
        usb_g_bulk_write(usb_id, WEBUSB_EP_IN, buf, 0);
    }
    return len;
}

/* ======================== Bulk IN ISR（发送完成）======================== */

static void webusb_epin_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    bulk_tx_send_packet(usb_id);
}

/* ======================== Vendor Request: Echo OUT 回调 ======================== */

static u32 webusb_vendor_echo_out_cb(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 len = MIN(setup->wLength, sizeof(webusb_handle->ctrl_echo_buf));
    usb_read_ep0(usb_id, webusb_handle->ctrl_echo_buf, len);
    webusb_handle->ctrl_echo_len = (u8)len;
    log_debug("vendor echo out: len=%d\n", len);

    setup->wLength -= len;
    if (setup->wLength) {
        return USB_EP0_STAGE_OUT;
    }
    return USB_EP0_STAGE_SETUP;
}

static u32 webusb_ctrl_file_recv_cb(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 discard[CTRL_MAX_PAYLOAD];
    u32 len = MIN(setup->wLength, sizeof(discard));
    usb_read_ep0(usb_id, discard, len);
    webusb_handle->ctrl_file_total += len;
    log_debug("ctrl file recv: len=%d, total=%d\n", len, webusb_handle->ctrl_file_total);

    setup->wLength -= len;
    if (setup->wLength) {
        return USB_EP0_STAGE_OUT;
    }
    return USB_EP0_STAGE_SETUP;
}

/* ======================== Vendor Request: Bulk TX 启动回调 ======================== */

static u32 webusb_vendor_bulk_tx_cb(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 param[4];
    u32 len = MIN(setup->wLength, sizeof(param));
    usb_read_ep0(usb_id, param, len);

    u32 total = param[0] | ((u32)param[1] << 8) | ((u32)param[2] << 16) | ((u32)param[3] << 24);
    webusb_handle->bulk_tx_remaining = total;
    webusb_handle->bulk_tx_seq = 0;
    log_info("bulk tx cmd: total=%d\n", total);

    if (webusb_handle->bulk_tx_remaining > 0) {
        bulk_tx_send_packet(usb_id);
    }

    setup->wLength -= len;
    if (setup->wLength) {
        return USB_EP0_STAGE_OUT;
    }
    return USB_EP0_STAGE_SETUP;
}

/* ======================== Interface Request Handler ======================== */

static u32 webusb_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;

    switch (bRequestType) {

    /* ---- Standard Requests ---- */
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;

        case USB_REQ_GET_INTERFACE:
            if (req->wLength) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;

        case USB_REQ_GET_STATUS:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            break;

        default:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;

    /* ---- Class Requests ---- */
    case USB_TYPE_CLASS:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;

    /* ---- Vendor Requests (调试用) ---- */
    case USB_TYPE_VENDOR:
        switch (req->bRequest) {

        /* 0x20: Echo Control Transfer — OUT 存储数据 / IN 回传数据 */
        case WEBUSB_VENDOR_ECHO_CTRL:
            if (req->bRequestType & USB_DIR_IN) {
                /* 主机读取 → 返回之前存储的数据 */
                if (webusb_handle->ctrl_echo_len > 0) {
                    tx_len = MIN(req->wLength, webusb_handle->ctrl_echo_len);
                    usb_set_data_payload(usb_device, req, webusb_handle->ctrl_echo_buf, tx_len);
                } else {
                    /* 未存储过数据，返回请求长度的填充字节 */
                    tx_len = MIN(req->wLength, CTRL_MAX_PAYLOAD);
                    memset(tx_payload, 0, tx_len);
                    usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                }
                log_debug("echo ctrl IN: len=%d\n", tx_len);
            } else {
                /* 主机写入 → 存储数据 */
                usb_set_setup_recv(usb_device, webusb_vendor_echo_out_cb);
            }
            break;

        /* 0x21: 设置 Bulk Echo 模式（wValue: 0=关闭, 1=开启）*/
        case WEBUSB_VENDOR_SET_ECHO:
            webusb_handle->echo_enable = LOBYTE(req->wValue) ? 1 : 0;
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            log_debug("set echo: %d\n", webusb_handle->echo_enable);
            break;

        /* 0x22: 查询 Bulk Echo 模式状态 */
        case WEBUSB_VENDOR_GET_ECHO:
            tx_payload[0] = webusb_handle->echo_enable;
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            log_debug("get echo: %d\n", webusb_handle->echo_enable);
            break;

        /* 0x23: 自定义测试数据（wValue=模式, wLength=字节数）*/
        case WEBUSB_VENDOR_CUSTOM_CMD: {
            u16 pattern = req->wValue;
            tx_len = MIN(req->wLength, CTRL_MAX_PAYLOAD);
            if (tx_len == 0) {
                tx_len = CTRL_MAX_PAYLOAD;    /* wLength=0 时返回满包 */
            }
            for (u32 i = 0; i < tx_len; i++) {
                switch (pattern) {
                case 0: /* 递增序列 */
                    tx_payload[i] = i & 0xFF;
                    break;
                case 1: /* 0xAA 填充 */
                    tx_payload[i] = 0xAA;
                    break;
                case 2: /* 0x55 填充 */
                    tx_payload[i] = 0x55;
                    break;
                case 3: /* Walking-1 */
                    tx_payload[i] = 1 << (i % 8);
                    break;
                default: /* 自定义: (pattern + i) & 0xFF */
                    tx_payload[i] = (pattern + i) & 0xFF;
                    break;
                }
            }
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            log_debug("custom cmd: pattern=%d, len=%d\n", pattern, tx_len);
            break;
        }

        /* 0x24: 启动 Bulk TX（OUT 4字节 uint32 长度 → 固件链式发送）*/
        case WEBUSB_VENDOR_BULK_TX:
            usb_set_setup_recv(usb_device, webusb_vendor_bulk_tx_cb);
            break;

        /* 0x25: Control Transfer 文件接收（OUT → 丢弃数据，只计数）*/
        case 0x25:
            if (req->bRequestType & USB_DIR_IN) {
                tx_payload[0] = webusb_handle->ctrl_file_total & 0xFF;
                tx_payload[1] = (webusb_handle->ctrl_file_total >> 8) & 0xFF;
                tx_payload[2] = (webusb_handle->ctrl_file_total >> 16) & 0xFF;
                tx_payload[3] = (webusb_handle->ctrl_file_total >> 24) & 0xFF;
                usb_set_data_payload(usb_device, req, tx_payload, 4);
                log_debug("ctrl file recv total: %d\n", webusb_handle->ctrl_file_total);
            } else {
                webusb_handle->ctrl_file_total = 0;
                usb_set_setup_recv(usb_device, webusb_ctrl_file_recv_cb);
            }
            break;

        /* 0x26: Control Transfer 文件发送（IN → 自增序列）*/
        case 0x26:
            if (req->bRequestType & USB_DIR_IN) {
                tx_len = MIN(req->wLength, CTRL_MAX_PAYLOAD);
                if (tx_len == 0) {
                    tx_len = CTRL_MAX_PAYLOAD;
                }
                for (u32 i = 0; i < tx_len; i++) {
                    tx_payload[i] = (webusb_handle->ctrl_file_seq + i) & 0xFF;
                }
                webusb_handle->ctrl_file_seq += tx_len;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                log_debug("ctrl file send: seq=%d, len=%d\n", webusb_handle->ctrl_file_seq, tx_len);
            } else {
                webusb_handle->ctrl_file_seq = 0;
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            }
            break;

        default:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            break;
        }
        break;

    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }

    return 0;
}

/* ======================== 描述符配置 ======================== */

u32 webusb_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    log_info("webusb interface num:%d\n", *cur_itf_num);
    memcpy(ptr, sWebUSBDescriptor, sizeof(sWebUSBDescriptor));
    ptr[2] = *cur_itf_num;

    if (usb_set_interface_hander(usb_id, *cur_itf_num, webusb_itf_hander) != *cur_itf_num) {
        ASSERT(0, "webusb set interface_hander fail");
    }

    if (usb_set_reset_hander(usb_id, *cur_itf_num, webusb_reset) != *cur_itf_num) {
        ASSERT(0, "webusb set interface_reset_hander fail");
    }

    webusb_interface_num = *cur_itf_num;
    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(sWebUSBDescriptor);
}

/* ======================== 注册 / 释放 ======================== */

u8 is_webusb_register()
{
    return webusb_handle ? 1 : 0;
}

u32 webusb_register(usb_dev usb_id)
{
    if (webusb_handle) {
        return 0;
    }

    webusb_handle = (struct webusb_handle_t *)zalloc(sizeof(struct webusb_handle_t));
    webusb_handle->ep_in_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, WEBUSB_EP_IN | USB_DIR_IN, MAXP_SIZE_WEBUSBIN);
    webusb_handle->ep_out_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, WEBUSB_EP_OUT, MAXP_SIZE_WEBUSBOUT);
    return 0;
}

u32 webusb_release(usb_dev usb_id)
{
    if (webusb_handle == NULL) {
        return 0;
    }
    usb_free_ep_dmabuffer(usb_id, webusb_handle->ep_in_dmabuffer);
    usb_free_ep_dmabuffer(usb_id, webusb_handle->ep_out_dmabuffer);
    free(webusb_handle);
    webusb_handle = NULL;
    return 0;
}

USB_CLASS_REGISTER(webusb_desc_config, webusb_register, webusb_release, TCFG_USB_WEBUSB_INDEX, WEBUSB_CLASS, 0);
#endif /* TCFG_USB_WEBUSB_ENABLE */


