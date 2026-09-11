#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb_config.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb/device/hid.h"
#include "usb/device/uac_audio.h"
#include "usb/device/descriptor.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "app_config.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USER_SETUP]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE


extern void go_mask_usb_updata();


static u8 root2_testing;
u32 usb_root2_testing()
{
#if USB_ROOT2
    return root2_testing;
#else
    return 0;
#endif
}
static u32 setup_endpoint(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 ep = LOBYTE(req->wIndex) & 0x0f;
    if (0) {
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        return 1;// not zero user handle this request
    }

    return 0;
}
_WEAK_
u32 webusb_setup_device_hook(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    return 0;
}
static u32 setup_device(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);

#if TCFG_USB_WEBUSB_ENABLE
    ret =  webusb_setup_device_hook(usb_device, req);
    if (ret) {
        return ret;
    }
#endif

#if TCFG_USB_SLAVE_HCI_ENABLE
    ret = hci_setup_device_hook(usb_device, req);
    if (ret) {
        return ret;
    }
#endif

    switch (req->bRequest) {
    case USB_REQ_GET_DESCRIPTOR:
        switch (HIBYTE(req->wValue)) {
        case USB_DT_STRING:
            break;
        case USB_DT_DEVICE:
#if USB_ROOT2
            if (req->wLength == 0xffff) {
                root2_testing = 1;
            }
#endif
            get_device_descriptor(tx_payload);
            for (int i = 0; i < 8; i++) {
                //为了兼容windows7等老电脑，配置成不同类时应该使用不同pid
                if (usb_device->wDeviceClass & BIT(i)) {
                    tx_payload[11] += i + 1;
                }
            }
            //为了兼容macOS，必须设成复合设备，否则macbook会不认设备
            tx_payload[0x4] = 0xef; //device class should be Multi-interface Function 0xEF
            tx_payload[0x5] = 0x2; //device subclass should be Multi-interface Function 2
            tx_payload[0x6] = 0x1; //device protocol should be Multi-interface Function 1
            usb_set_data_payload(usb_device, req, tx_payload, tx_payload[0]);
            ret = 1;
            break;
        case USB_DT_DEVICE_QUALIFIER:
#if USB_IS_HS_MODE
        {
            static const u8 qualifier[] = {
                10,                         // bLength
                USB_DT_DEVICE_QUALIFIER,    // bDescriptorType
                0x00, 0x02,                 // bcdUSB = 2.00
                0x00,                       // bDeviceClass
                0x00,                       // bDeviceSubClass
                0x00,                       // bDeviceProtocol
                64,                         // bMaxPacketSize0
                1,                          // bNumConfigurations
                0,                          // bReserved
            };
            usb_set_data_payload(usb_device, req, qualifier, sizeof(qualifier));
            ret = 1;
        }
#else
        break;
#endif
        break;
        case USB_DT_CONFIG:
            break;
        }
        break;
    case USB_REQ_SET_CONFIGURATION:
        break;
    case USB_REQ_SET_ADDRESS:
        break;
    }
    return ret;
}

#if TCFG_WLM_PRODUCTION_TEST_ENABLE
_WEAK_
int user_slave_send_data_process(struct usb_ctrlrequest *req, u8 *tx_payload)
{
    return 0;
}
_WEAK_
void user_slave_receive_data_process(struct usb_ctrlrequest *req, u8 *buf, u32 len)
{
}
static u32 receive_data_from_usb(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u8 buf[64];
    u32 len;
    const usb_dev usb_id = usb_device2id(usb_device);
    len = sizeof(buf) > req->wLength ? req->wLength : sizeof(buf);
    usb_read_ep0(usb_id, buf, len);
    g_printf("%s", __func__);
    printf_buf(buf, len);
    user_slave_receive_data_process(req, buf, len);
    return USB_EP0_STAGE_SETUP;
}
#endif

static u32 setup_other(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u8 ack_data[8] = {0};
    u32 ret = 0;
    u32 tx_len = 0;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    switch (req->bRequest) {
    case USB_REQ_GET_STATUS:
#if TCFG_TYPEC_EARPHONE_TEST_FILTER
        tx_payload[0] = 0xfe;
        usb_set_data_payload(usb_device, req, tx_payload, 1);
#endif
        if ((req->wValue == 0xfc) && (req->wIndex == 0xfe)) {
            if (req->wLength == sizeof(ack_data)) { //支持该私有命令响应
                log_info("support private update command\n");
                memset(ack_data, 0xff, sizeof(ack_data));
                usb_set_data_payload(usb_device, req, ack_data, sizeof(ack_data));
            } else if (req->wLength == 0x00) {  //准备跳转升级模式
                log_info("run func go_mask_usb_updata()\n");
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                go_mask_usb_updata();
            }
            ret = 1;
        }
        break;
#if TCFG_WLM_PRODUCTION_TEST_ENABLE
    case 0xf0:  //dir: device -> host
        if ((req->bRequestType & 0x80) != 0) {
            switch (req->bRequestType & USB_TYPE_MASK) {
            case USB_TYPE_VENDOR:
                r_printf("s->h, wValue: 0x%x, wIndex: 0x%x", req->wValue, req->wIndex);
                tx_len = user_slave_send_data_process(req, tx_payload);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                ret = 1;
                break;
            }
        }
        break;
    case 0x70:  //dir: host -> device
        if ((req->bRequestType & 0x80) == 0) {
            switch (req->bRequestType & USB_TYPE_MASK) {
            case USB_TYPE_VENDOR:
                r_printf("h->s, wValue: 0x%x, wIndex: 0x%x", req->wValue, req->wIndex);
                usb_set_setup_recv(usb_device, receive_data_from_usb);
                ret = 1;
                break;
            }
        }
        break;
#endif
    default:
        ret = 1 ;
        break;
    }
    return ret;
}

static u32 user_setup_filter(struct usb_device_t *usb_device, struct usb_ctrlrequest *request)
{
    detect_host_type(usb_device, request);
    // dump_setup_request(request);
    // log_debug_hexdump((u8 *)request, 8);
    u32 ret = 0;
    u32 recip = request->bRequestType & USB_RECIP_MASK;
    switch (recip) {
    case USB_RECIP_DEVICE:
        ret = setup_device(usb_device, request);
        break;
    case USB_RECIP_INTERFACE:
        break;
    case USB_RECIP_ENDPOINT:
        ret = setup_endpoint(usb_device, request);
        break;
    case USB_RECIP_OTHER:
        ret = setup_other(usb_device, request);
        break;
    }
#if 0
    const char *statas[] = {"USB_ATTACHED", "USB_POWERED", "USB_DEFAULT",
                            "USB_ADDRESS", "USB_CONFIGURED", "USB_SUSPENDED"
                           };

    const char *phases[] = {"SETUP", "IN", "OUT",
                            "STALL",
                           };

    printf("state:%s phase: %s", statas[usb_device->bDeviceStates],
           phases[usb_device->bsetup_phase]);
#endif
    return ret;// not zero user handle this request
}
void user_setup_filter_install(struct usb_device_t *usb_device)
{
    usb_set_setup_hook(usb_device, user_setup_filter);
}
#endif
