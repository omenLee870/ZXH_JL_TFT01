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

#if TCFG_USB_SLAVE_MIDI_ENABLE && (USB_MIDI_VERSION == USB_MIDI_VERSION_2_0)

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[MIDI2]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/*********************************************************/
/*
    USB MIDI 2.0 Descriptor Subtype Constants
    Per Universal Serial Bus Device Class Definition
    for MIDI Devices Release 2.0 (May 5, 2020)
*/
/*********************************************************/
#define MS_HEADER               0x01
#define MS_GENERAL              0x01    /* MIDI 1.0 endpoint descriptor subtype */
#define MS_GENERAL_2_0          0x02    /* MIDI 2.0 endpoint descriptor subtype */

#define CS_GR_TRM_BLOCK         0x26
#define GR_TRM_BLOCK_HEADER     0x01
#define GR_TRM_BLOCK            0x02

#define CS_INTERFACE            0x24
#define CS_ENDPOINT             0x25

static u8 *midi_dma_tx_buffer;
static u8 *midi_dma_rx_buffer;

/*********************************************************/
/*
    Alternate Setting state for MIDIStreaming Interface.
    0 = MIDI 1.0 compatible (Alt Setting 0)
    1 = MIDI 2.0 (Alt Setting 1, default)
*/
/*********************************************************/
static u8 midi2_alternate_setting = 1;

/*********************************************************/
/*
    AudioControl Interface Descriptors (18 bytes)

    Provides the mandatory AudioControl interface that
    precedes the MIDIStreaming interface. Required for
    host OS USB audio class driver binding.

    Layout:
      - Standard AC Interface Descriptor          (9 bytes)
      - Class-Specific AC Interface Header        (9 bytes)
*/
/*********************************************************/
static const u8 sMidi2ACDescriptor[] = {
///Standard AC Interface Descriptor
    0x09,        // bLength = 9
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0 (filled at runtime)
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0x01,        // bInterfaceClass (Audio)
    0x01,        // bInterfaceSubClass (Audio Control)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

///Class-Specific AC Interface Header Descriptor (Table B-4)
    0x09,        // bLength = 9
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    0x01,        // bDescriptorSubtype (HEADER)
    0x00, 0x01,  // bcdADC = 0x0100 (Audio Class 1.0)
    0x09, 0x00,  // wTotalLength = 9
    0x01,        // bInCollection = 1
    0x00,        // baInterfaceNr(1) (filled at runtime -> MS interface)
};

/*********************************************************/
/*
    MIDIStreaming Interface Descriptors (USB MIDI 1.0)
    Alternate Setting 0 — backward compatible with
    USB MIDI 1.0 hosts.

    Structure (74 bytes total):
      - Standard MS Interface Descriptor             (9 bytes)
      - Class-Specific MS Interface Header           (7 bytes)
      - MIDI IN Jack (Embedded)                      (6 bytes)
      - MIDI IN Jack (External)                      (6 bytes)
      - MIDI OUT Jack (Embedded)                     (9 bytes)
      - MIDI OUT Jack (External)                     (9 bytes)
      - Standard MS Bulk OUT Endpoint (9 bytes)      (9 bytes)
      - Class-Specific MS Bulk OUT Endpoint          (5 bytes, MS_GENERAL)
      - Standard MS Bulk IN Endpoint (9 bytes)       (9 bytes)
      - Class-Specific MS Bulk IN Endpoint           (5 bytes, MS_GENERAL)
*/
/*********************************************************/
static const u8 sMidi2MS10Descriptor[] = {
///Standard MS Interface Descriptor
    0x09,        // bLength = 9
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber (filled at runtime)
    0x00,        // bAlternateSetting = 0 (USB MIDI 1.0)
    0x02,        // bNumEndpoints 2
    0x01,        // bInterfaceClass (Audio)
    0x03,        // bInterfaceSubClass (MIDIStreaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

///Class-Specific MS Interface Header Descriptor
    0x07,        // bLength = 7
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    MS_HEADER,   // bDescriptorSubtype (MS_HEADER)
    0x00, 0x01,  // bcdMSC = 0x0100 (USB MIDI 1.0)
    0x25, 0x00,  // wTotalLength = 37 (7 + 6 + 6 + 9 + 9)

///MIDI IN Jack Descriptor (Embedded)
    0x06,        // bLength = 6
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    0x02,        // bDescriptorSubtype (MIDI_IN_JACK)
    0x01,        // bJackType (EMBEDDED)
    0x01,        // bJackID = 1
    0x00,        // iJack

///MIDI IN Jack Descriptor (External)
    0x06,        // bLength = 6
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    0x02,        // bDescriptorSubtype (MIDI_IN_JACK)
    0x02,        // bJackType (EXTERNAL)
    0x02,        // bJackID = 2
    0x00,        // iJack

///MIDI OUT Jack Descriptor (Embedded)
    0x09,        // bLength = 9
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    0x03,        // bDescriptorSubtype (MIDI_OUT_JACK)
    0x01,        // bJackType (EMBEDDED)
    0x03,        // bJackID = 3
    0x01,        // bNrInputPins = 1
    0x02,        // baSourceID(1) = 2 (External IN Jack)
    0x01,        // baSourcePin(1) = 1
    0x00,        // iJack

///MIDI OUT Jack Descriptor (External)
    0x09,        // bLength = 9
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    0x03,        // bDescriptorSubtype (MIDI_OUT_JACK)
    0x02,        // bJackType (EXTERNAL)
    0x04,        // bJackID = 4
    0x01,        // bNrInputPins = 1
    0x01,        // baSourceID(1) = 1 (Embedded IN Jack)
    0x01,        // baSourcePin(1) = 1
    0x00,        // iJack

///Standard MS Bulk IN Endpoint Descriptor (9 bytes, MIDI 1.0 with bRefresh/bSynchAddress)
    0x09,        // bLength = 9
    0x05,        // bDescriptorType (Endpoint)
    0x00,        // bEndpointAddress (filled at runtime)
    0x02,        // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPIN), HIBYTE(MAXP_SIZE_MIDI_EPIN),  // wMaxPacketSize
    0x00,        // bInterval (ignored for Bulk)
    0x00,        // bRefresh
    0x00,        // bSynchAddress

///Class-Specific MS Bulk IN Endpoint Descriptor (5 bytes, MS_GENERAL)
    0x05,        // bLength = 4 + n
    CS_ENDPOINT, // bDescriptorType (CS_ENDPOINT)
    MS_GENERAL,  // bDescriptorSubtype (MS_GENERAL)
    0x01,        // bNumEmbMIDIJack = 1
    0x03,        // baAssocJackID(1) = 3 (Embedded OUT Jack)

///Standard MS Bulk OUT Endpoint Descriptor (9 bytes, MIDI 1.0 with bRefresh/bSynchAddress)
    0x09,        // bLength = 9
    0x05,        // bDescriptorType (Endpoint)
    0x00,        // bEndpointAddress (filled at runtime)
    0x02,        // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPOUT), HIBYTE(MAXP_SIZE_MIDI_EPOUT),  // wMaxPacketSize
    0x00,        // bInterval (ignored for Bulk)
    0x00,        // bRefresh
    0x00,        // bSynchAddress

///Class-Specific MS Bulk OUT Endpoint Descriptor (5 bytes, MS_GENERAL)
    0x05,        // bLength = 4 + n
    CS_ENDPOINT, // bDescriptorType (CS_ENDPOINT)
    MS_GENERAL,  // bDescriptorSubtype (MS_GENERAL)
    0x01,        // bNumEmbMIDIJack = 1
    0x01,        // baAssocJackID(1) = 1 (Embedded IN Jack)
// 74 bytes
};

/*********************************************************/
/*
    MIDIStreaming Interface Descriptors (USB MIDI 2.0)
    Alternate Setting 1 — preferred implementation
    supporting UMP and Group Terminal Blocks.

    Structure (40 bytes total):
      - Standard MS Interface Descriptor             (9 bytes)
      - Class-Specific MS Interface Header           (7 bytes)
      - Standard MS Bulk OUT Endpoint Descriptor     (7 bytes)
      - Class-Specific MS Bulk OUT Endpoint          (5 bytes, MS_GENERAL_2_0)
      - Standard MS Bulk IN Endpoint Descriptor      (7 bytes)
      - Class-Specific MS Bulk IN Endpoint           (5 bytes, MS_GENERAL_2_0)

    Group Terminal Block descriptors are retrieved
    separately via GET_DESCRIPTOR(CS_GR_TRM_BLOCK).
*/
/*********************************************************/
static const u8 sMidi2MS20Descriptor[] = {
///Standard MS Interface Descriptor (Table 5-1)
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber (filled at runtime)
    0x01,        // bAlternateSetting = 1 (USB MIDI 2.0)
    0x02,        // bNumEndpoints 2
    0x01,        // bInterfaceClass (Audio)
    0x03,        // bInterfaceSubClass (MIDIStreaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

///Class-Specific MS Interface Header Descriptor (Table 5-2)
    0x07,        // bLength
    CS_INTERFACE,// bDescriptorType (CS_INTERFACE)
    MS_HEADER,   // bDescriptorSubtype (MS_HEADER)
    0x00, 0x02,  // bcdMSC = 0x0200 (USB MIDI 2.0)
    0x07, 0x00,  // wTotalLength = 7 (equals bLength in USB MIDI 2.0)

///Standard MS Bulk OUT Endpoint Descriptor (Table 5-3)
    0x07,        // bLength (7 in USB MIDI 2.0, no bRefresh/bSynchAddress)
    0x05,        // bDescriptorType (Endpoint)
    0x00,        // bEndpointAddress (filled at runtime)
    0x02,        // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPOUT), HIBYTE(MAXP_SIZE_MIDI_EPOUT),  // wMaxPacketSize
    0x00,        // bInterval (ignored for Bulk)

///Class-Specific MS Bulk OUT Endpoint Descriptor (Table 5-4)
    0x05,        // bLength = 4 + n
    CS_ENDPOINT, // bDescriptorType (CS_ENDPOINT)
    MS_GENERAL_2_0, // bDescriptorSubtype (MS_GENERAL_2_0)
    0x01,        // bNumGrpTrmBlock = 1
    0x01,        // baAssoGrpTrmBlkID(1) = 1

///Standard MS Bulk IN Endpoint Descriptor (Table 5-3)
    0x07,        // bLength (7 in USB MIDI 2.0)
    0x05,        // bDescriptorType (Endpoint)
    0x00,        // bEndpointAddress (filled at runtime)
    0x02,        // bmAttributes (Bulk)
    LOBYTE(MAXP_SIZE_MIDI_EPIN), HIBYTE(MAXP_SIZE_MIDI_EPIN),  // wMaxPacketSize
    0x00,        // bInterval (ignored for Bulk)

///Class-Specific MS Bulk IN Endpoint Descriptor (Table 5-4)
    0x05,        // bLength = 4 + n
    CS_ENDPOINT, // bDescriptorType (CS_ENDPOINT)
    MS_GENERAL_2_0, // bDescriptorSubtype (MS_GENERAL_2_0)
    0x01,        // bNumGrpTrmBlock = 1
    0x01,        // baAssoGrpTrmBlkID(1) = 1
// 40 bytes
};

/*********************************************************/
/*
    Group Terminal Block Descriptors
    Retrieved via GET_DESCRIPTOR(CS_GR_TRM_BLOCK = 0x26)

    Structure:
      - Group Terminal Block Header Descriptor  (5 bytes)
      - Group Terminal Block Descriptor          (13 bytes)
*/
/*********************************************************/
static const u8 sMidi2GroupTerminalBlockDesc[] = {
///Class-Specific Group Terminal Block Header Descriptor (Table 5-5)
    0x05,              // bLength = 5
    CS_GR_TRM_BLOCK,   // bDescriptorType = 0x26
    GR_TRM_BLOCK_HEADER, // bDescriptorSubtype = 0x01
    0x12, 0x00,        // wTotalLength = 18 (5 + 13)

///Group Terminal Block Descriptor (Table 5-6)
    0x0D,              // bLength = 13
    CS_GR_TRM_BLOCK,   // bDescriptorType = 0x26
    GR_TRM_BLOCK,      // bDescriptorSubtype = 0x02
    0x01,              // bGrpTrmBlkID = 1
    0x00,              // bGrpTrmBlkType = 0x00 (Bidirectional, for MIDI-CI)
    0x00,              // nGroupTrm = 0 (Group Terminal 1)
    0x01,              // nNumGroupTrm = 1
    0x00,              // iBlockItem (no string descriptor)
    0x00,              // bMIDIProtocol = 0x00 (Use MIDI-CI)
    0x01, 0x00,        // wMaxInputBandwidth (0x0001 = ~32kb/s)
    0x01, 0x00,        // wMaxOutputBandwidth (0x0001 = ~32kb/s)
};

static void (*usb_midi_rx_notify)(const usb_dev usb_id);

void midi_register_rx_notify(void (*func)(const usb_dev usb_id))
{
    usb_midi_rx_notify = func;
}

static void midi2_ep_rx_handler(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (usb_midi_rx_notify) {
        usb_midi_rx_notify(usb_id);
    }
}

static void midi2_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    ep_buffer = midi_dma_tx_buffer;
    usb_g_ep_config(usb_id, MIDI_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 0, ep_buffer, MAXP_SIZE_MIDI_EPIN);

    ep_buffer = midi_dma_rx_buffer;
    usb_g_ep_config(usb_id, MIDI_EP_OUT, USB_ENDPOINT_XFER_BULK, 1, ep_buffer, MAXP_SIZE_MIDI_EPOUT);
    usb_g_set_intr_hander(usb_id, MIDI_EP_OUT, midi2_ep_rx_handler);

    usb_enable_ep(usb_id, MIDI_EP_IN);
}

static void midi2_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_info("midi2_reset");
    midi2_alternate_setting = 0;
    midi2_endpoint_init(usb_device, itf);
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

/*********************************************************/
/*
    AudioControl Interface Handler

    Handles standard requests for the AudioControl
    interface (0 endpoints, no alternate settings).
*/
/*********************************************************/
static u32 midi2_ac_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_SET_INTERFACE:
            if (req->wValue == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            return 1;

        case USB_REQ_GET_INTERFACE: {
            u8 *tx_payload = usb_get_setup_buffer(usb_device);
            tx_payload[0] = 0;
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 1;
        }
        }
        break;
    }
    return 0;
}

/*********************************************************/
/*
    MIDIStreaming Interface Handler

    Handles:
      - Standard requests (SET_INTERFACE, GET_INTERFACE)
      - GET_DESCRIPTOR(CS_GR_TRM_BLOCK) for Group
        Terminal Block descriptors (Section 6)
*/
/*********************************************************/
static u32 midi2_ms_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;

    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        /*
         * Handle GET_DESCRIPTOR(CS_GR_TRM_BLOCK) request per Section 6.
         * Returns the Group Terminal Block descriptors to the Host.
         *
         * Request format:
         *   bmRequestType = 0x81 (Device-to-Host, Standard, Interface)
         *   bRequest      = 0x06 (GET_DESCRIPTOR)
         *   wValue        = 0x2600 (CS_GR_TRM_BLOCK << 8 | alternate setting)
         *   wIndex        = Interface Number
         */
        if (req->bRequest == USB_REQ_GET_DESCRIPTOR && HIBYTE(req->wValue) == CS_GR_TRM_BLOCK) {
            u32 total_len = sizeof(sMidi2GroupTerminalBlockDesc);
            if (req->wLength < 5) {
                /* Only header requested */
                usb_set_data_payload(usb_device, req, (u8 *)sMidi2GroupTerminalBlockDesc, req->wLength);
            } else {
                /* Full descriptors requested */
                usb_set_data_payload(usb_device, req, (u8 *)sMidi2GroupTerminalBlockDesc,
                                     total_len < req->wLength ? total_len : req->wLength);
            }
            return 1;
        }

        switch (req->bRequest) {
        case USB_REQ_SET_INTERFACE:
            if (req->wValue == 0 || req->wValue == 1) {
                midi2_alternate_setting = req->wValue;
                log_info("midi2 SET_INTERFACE alt=%d", req->wValue);
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            return 1;

        case USB_REQ_GET_INTERFACE:
            tx_payload[0] = midi2_alternate_setting;
            usb_set_data_payload(usb_device, req, tx_payload, 1);
            return 1;
        }
        break;
    }
    return 0;
}

/*********************************************************/
/*
    midi2_desc_config

    Fills the USB MIDI descriptors into the configuration
    descriptor buffer per Appendix B of the USB MIDI 2.0 spec.

    Layout:
      - Interface N:   AudioControl (no alternate settings)
      - Interface N+1, Alt 0: MIDIStreaming (USB MIDI 1.0 compatible)
      - Interface N+1, Alt 1: MIDIStreaming (USB MIDI 2.0)

    Total descriptor size: 18 + 74 + 40 = 132 bytes
    cur_itf_num advances by 2
*/
/*********************************************************/
u32 midi2_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    u32 itf_num = *cur_itf_num;
    u32 total_len = 0;

    log_info("midi2 interface num:%d\n", itf_num);

    /*
     * 1. AudioControl Interface (Interface N)
     *    Offset 2: bInterfaceNumber = itf_num
     *    Offset 17: baInterfaceNr(1) = itf_num + 1 (point to MS interface)
     */
    memcpy(ptr, sMidi2ACDescriptor, sizeof(sMidi2ACDescriptor));
    ptr[2] = itf_num;
    ptr[17] = itf_num + 1;
    ptr += sizeof(sMidi2ACDescriptor);
    total_len += sizeof(sMidi2ACDescriptor);

    /*
     * 2. MIDIStreaming Interface Alt 0 — USB MIDI 1.0 (Interface N+1)
     *    Offset 2:  bInterfaceNumber = itf_num + 1
     *    Offset 48: bEndpointAddress = MIDI_EP_IN | USB_DIR_IN  (Bulk IN)
     *    Offset 62: bEndpointAddress = MIDI_EP_OUT             (Bulk OUT)
     */
    memcpy(ptr, sMidi2MS10Descriptor, sizeof(sMidi2MS10Descriptor));
    ptr[2] = itf_num + 1;
    ptr[48] = MIDI_EP_IN | USB_DIR_IN;
    ptr[62] = MIDI_EP_OUT;
    ptr += sizeof(sMidi2MS10Descriptor);
    total_len += sizeof(sMidi2MS10Descriptor);

    /*
     * 3. MIDIStreaming Interface Alt 1 — USB MIDI 2.0 (Interface N+1)
     *    Offset 2:  bInterfaceNumber = itf_num + 1 (already 0x00, fill)
     *    Offset 18: bEndpointAddress = MIDI_EP_OUT             (Bulk OUT)
     *    Offset 30: bEndpointAddress = MIDI_EP_IN | USB_DIR_IN  (Bulk IN)
     */
    memcpy(ptr, sMidi2MS20Descriptor, sizeof(sMidi2MS20Descriptor));
    ptr[2] = itf_num + 1;
    ptr[18] = MIDI_EP_OUT;
    ptr[30] = MIDI_EP_IN | USB_DIR_IN;
    ptr += sizeof(sMidi2MS20Descriptor);
    total_len += sizeof(sMidi2MS20Descriptor);

    /* Register handler for AudioControl interface */
    if (usb_set_interface_hander(usb_id, itf_num, midi2_ac_itf_hander) != itf_num) {
        ASSERT(0, "midi2 set ac interface_hander fail");
    }

    /* Register handler and reset for MIDIStreaming interface */
    if (usb_set_interface_hander(usb_id, itf_num + 1, midi2_ms_itf_hander) != itf_num + 1) {
        ASSERT(0, "midi2 set ms interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, itf_num + 1, midi2_reset) != itf_num + 1) {
        ASSERT(0, "midi2 set interface_reset_hander fail");
    }

    *cur_itf_num = itf_num + 2;
    return total_len;
}

void midi2_register(const usb_dev usb_id)
{
    if (!midi_dma_tx_buffer) {
        midi_dma_tx_buffer = usb_alloc_ep_dmabuffer(usb_id, MIDI_EP_IN | USB_DIR_IN, MAXP_SIZE_MIDI_EPIN);
    }
    if (!midi_dma_rx_buffer) {
        midi_dma_rx_buffer = usb_alloc_ep_dmabuffer(usb_id, MIDI_EP_OUT, MAXP_SIZE_MIDI_EPOUT);
    }
}

void midi2_release(const usb_dev usb_id)
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

USB_CLASS_REGISTER(midi2_desc_config, midi2_register, midi2_release, TCFG_USB_MIDI_INDEX, MIDI_CLASS, 0);
#endif
