#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "os/os_api.h"
#include "usb/device/usb_stack.h"
#include "usb_config.h"

#include "app_config.h"

#if TCFG_USB_SLAVE_MIDI_ENABLE && (USB_MIDI_VERSION == USB_MIDI_VERSION_1_0)

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[MIDI]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

static u8 *midi_dma_tx_buffer;
static u8 *midi_dma_rx_buffer;

static const u8 sMidiDescriptor[] = {
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0x01,        // bInterfaceClass (Audio)
    0x01,        // bInterfaceSubClass (Audio Control)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
    0x00, 0x01,  // bcdADC 1.00
    0x09, 0x00,  // wTotalLength 9
    0x01,        // binCollection 0x01
    0x01,        // baInterfaceNr 1

    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x01,        // bInterfaceNumber 1
    0x00,        // bAlternateSetting
    0x02,        // bNumEndpoints 2
    0x01,        // bInterfaceClass (Audio)
    0x03,        // bInterfaceSubClass
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x07,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x01, 0x00, 0x01, 0x25, 0x00,
    0x06,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02, 0x01, 0x01, 0x00,
    0x06,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02, 0x02, 0x02, 0x00,
    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00,
    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00,

    0x07,        // bLength
    0x05,        // bDescriptorType (See Next Line)
    MIDI_EP_IN | USB_DIR_IN,        // bEndpointAddress (IN/D2H)
    USB_ENDPOINT_XFER_BULK,        // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPIN), HIBYTE(MAXP_SIZE_MIDI_EPIN),  // wMaxPacketSize 64
    0x00,        // bInterval 0 (unit depends on device speed)

    0x05,        // bLength
    0x25,        // bDescriptorType (See Next Line)
    0x01, 0x01, 0x03,

    0x07,        // bLength
    0x05,        // bDescriptorType (See Next Line)
    MIDI_EP_OUT,                    // bEndpointAddress (OUT/H2D)
    USB_ENDPOINT_XFER_BULK,         // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPOUT), HIBYTE(MAXP_SIZE_MIDI_EPOUT),  // wMaxPacketSize 64
    0x00,        // bInterval 0 (unit depends on device speed)

    0x05,        // bLength
    0x25,        // bDescriptorType (See Next Line)
    0x01, 0x01, 0x01,
// 88 bytes
};

static void (*usb_midi_rx_notify)(const usb_dev usb_id);

void midi_register_rx_notify(void (*func)(const usb_dev usb_id))
{
    usb_midi_rx_notify = func;
}
static void midi_ep_rx_handler(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (usb_midi_rx_notify) {
        usb_midi_rx_notify(usb_id);
    }
}
static void midi_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    ep_buffer = midi_dma_tx_buffer;
    usb_g_ep_config(usb_id, MIDI_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 0, ep_buffer, MAXP_SIZE_MIDI_EPIN);

    ep_buffer = midi_dma_rx_buffer;
    usb_g_ep_config(usb_id, MIDI_EP_OUT, USB_ENDPOINT_XFER_BULK, 1, ep_buffer, MAXP_SIZE_MIDI_EPOUT);
    usb_g_set_intr_hander(usb_id, MIDI_EP_OUT, midi_ep_rx_handler);

    usb_enable_ep(usb_id, MIDI_EP_IN);
    //log_info("midi tx dma buffer %x", ep_buffer);
}
static void midi_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_info("midi_reset");
    midi_endpoint_init(usb_device, itf);
}
u32 midi_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len)
{
    log_debug_hexdump((u8 *)buffer, len);
    return usb_g_bulk_write(usb_id, MIDI_EP_IN, buffer, len);
}
u32 midi_rx_data(const usb_dev usb_id, u8 *buffer)
{
    return usb_g_bulk_read(usb_id, MIDI_EP_OUT, buffer, MAXP_SIZE_MIDI_EPOUT, 0);
}
static u32 midi_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        break;
    }
    return 0;
}
u32 midi_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_info("midi interface num:%d\n", *cur_itf_num);
    memcpy(ptr, sMidiDescriptor, sizeof(sMidiDescriptor));
    ptr[2] = *cur_itf_num;
    ptr[9 + 8] = *cur_itf_num + 1;
    ptr[9 + 9 + 2] = *cur_itf_num + 1;
    if (usb_set_interface_hander(usb_id, *cur_itf_num, midi_itf_hander) != *cur_itf_num) {
        ASSERT(0, "midi set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, midi_reset) != *cur_itf_num) {
        ASSERT(0, "midi set interface_reset_hander fail");
    }

    *cur_itf_num = *cur_itf_num + 2;
    return sizeof(sMidiDescriptor) ;
}

void midi_register(const usb_dev usb_id)
{
    if (!midi_dma_tx_buffer) {
        midi_dma_tx_buffer = usb_alloc_ep_dmabuffer(usb_id, MIDI_EP_IN | USB_DIR_IN, MAXP_SIZE_MIDI_EPIN);
    }
    if (!midi_dma_rx_buffer) {
        midi_dma_rx_buffer = usb_alloc_ep_dmabuffer(usb_id, MIDI_EP_OUT, MAXP_SIZE_MIDI_EPOUT);
    }
}

void midi_release(const usb_dev usb_id)
{
    if (midi_dma_tx_buffer) {
        usb_free_ep_dmabuffer(usb_id, midi_dma_tx_buffer);
        midi_dma_tx_buffer = NULL;
    }
    if (midi_dma_rx_buffer) {
        usb_free_ep_dmabuffer(usb_id, midi_dma_rx_buffer);
        midi_dma_rx_buffer = NULL;
    }
}

USB_CLASS_REGISTER(midi_desc_config, midi_register, midi_release, TCFG_USB_MIDI_INDEX, MIDI_CLASS, 0);
#endif
