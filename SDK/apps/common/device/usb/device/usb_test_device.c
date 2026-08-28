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
#include "usb/device/descriptor.h"
#include "usb/scsi.h"
#include "usb/test_usb.h"

#if TCFG_USB_TEST_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB-TEST-D]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

/* ========== Channel state ========== */
struct test_ep_channel {
    u8 ep;
    u16 maxp;
    u8 type;
    u8 *ep_in_dma;
    u8 *ep_out_dma;
    struct test_ep_stats in_stats;
    struct test_ep_stats out_stats;
    u32 in_seq;
};

/* ========== Config table ========== */
static const struct test_ch_cfg ch_cfg[TEST_CHANNEL_MAX] = { TEST_CHANNEL_CONFIG };
static struct test_ep_channel channels[TEST_CHANNEL_MAX];
static u8 ch_itf_num[TEST_CHANNEL_MAX];

#if TEST_EP_CTRL_ENABLE
static struct test_ep_stats ctrl_in_stats;
static struct test_ep_stats ctrl_out_stats;
static u32 ctrl_in_seq;
static u32 ctrl_recv_offset;
static u8 ctrl_buf[TEST_CTRL_MAXP];
#endif

/* ========== Data helpers ========== */

static u32 fill_frame(u8 *buf, u32 max_len, u32 seq, u8 pattern)
{
    if (!buf || max_len <= FRAME_HDR_SIZE) {
        return 0;
    }
    u32 data_len = (seq % (max_len - FRAME_HDR_SIZE)) + 1;
    memcpy(buf, &seq, sizeof(u32));
    memcpy(buf + 4, &data_len, sizeof(u32));
    return FRAME_HDR_SIZE + data_len;
}

/* ========== Channel lookup ========== */

static struct test_ep_channel *find_channel_by_ep(u8 ep)
{
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (channels[i].ep == ep) {
            return &channels[i];
        }
    }
    return NULL;
}

/* ========== TX helper ========== */

static void start_in_channel(const usb_dev usb_id, struct test_ep_channel *ch)
{
    if (!ch->ep_in_dma) {
        log_error("ep%d in_dma NULL", ch->ep);
        return;
    }
    ch->in_seq++;
    u32 frame_len = fill_frame(ch->ep_in_dma, ch->maxp, ch->in_seq, 0x80 | ch->ep);
    if (!frame_len) {
        log_error("ep%d fill_frame 0, maxp=%d", ch->ep, ch->maxp);
        return;
    }
    ch->in_stats.tx_bytes += frame_len;
    ch->in_stats.tx_packets++;
    int r = usb_g_ep_write(usb_id, ch->ep, NULL, frame_len);
    if (r != frame_len) {
        log_error("g_write error %d", r);
    }
}

/* ========== EP0 CTRL receive callback ========== */
#if TEST_EP_CTRL_ENABLE
static u32 test_ctrl_recv_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 len = MIN(req->wLength, TEST_CTRL_MAXP);

    usb_read_ep0(usb_id, ctrl_buf + ctrl_recv_offset, len);
    ctrl_recv_offset += len;

    req->wLength -= len;
    if (req->wLength) {
        return USB_EP0_STAGE_OUT;
    }

    /* All data received */
    ctrl_out_stats.rx_bytes += ctrl_recv_offset;
    ctrl_out_stats.rx_packets++;
    u32 expected_seq = ctrl_out_stats.last_seq + 1;
    u32 rx_seq, data_len;
    memcpy(&rx_seq, ctrl_buf, sizeof(u32));
    memcpy(&data_len, ctrl_buf + 4, sizeof(u32));
    if (rx_seq != expected_seq || FRAME_HDR_SIZE + data_len != ctrl_recv_offset) {
        ctrl_out_stats.rx_errors++;
    } else {
        for (u32 i = 0; i < data_len; i++) {
            if (ctrl_buf[FRAME_HDR_SIZE + i] != 0x00) {
                ctrl_out_stats.rx_errors++;
                break;
            }
        }
    }
    ctrl_out_stats.last_seq = rx_seq;

    return USB_EP0_STAGE_SETUP;
}
#endif

/* ========== ISR callbacks ========== */

static void test_in_isr(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 ep_num = ep & 0x0f;
    if (ep_num == 0) {
        return;
    }
    struct test_ep_channel *ch = find_channel_by_ep(ep_num);
    if (!ch || !ch->ep_in_dma) {
        return;
    }
    start_in_channel(usb_id, ch);
}

static void test_out_isr(struct usb_device_t *usb_device, u32 ep)
{
    if (ep == 0) {
        return;
    }
    const usb_dev usb_id = usb_device2id(usb_device);
    struct test_ep_channel *ch = find_channel_by_ep(ep);
    if (!ch || !ch->ep_in_dma) {
        return;
    }

    static u8 buf[3072];
    u32 total_len = usb_g_ep_read(usb_id, ep, buf, ch->maxp, 0);
    if (!total_len) {
        return;
    }

    ch->out_stats.rx_bytes += total_len;

    u32 offset = 0;
    while (offset + FRAME_HDR_SIZE <= total_len) {
        u32 rx_seq, data_len;
        memcpy(&rx_seq, buf + offset, sizeof(u32));
        memcpy(&data_len, buf + offset + 4, sizeof(u32));
        u32 frame_len = FRAME_HDR_SIZE + data_len;
        if (offset + frame_len > total_len) {
            break;
        }

        ch->out_stats.rx_packets++;
        u32 expected_seq = ch->out_stats.last_seq + 1;

        if (rx_seq != expected_seq) {
            ch->out_stats.rx_errors++;
            if (rx_seq > expected_seq) {
                ch->out_stats.rx_lost += rx_seq - expected_seq;
            }
        }
        if (FRAME_HDR_SIZE + data_len != frame_len) {
            ch->out_stats.rx_errors++;
        }
        for (u32 i = 0; i < data_len; i++) {
            if (buf[offset + FRAME_HDR_SIZE + i] != ch->ep) {
                ch->out_stats.rx_errors++;
                break;
            }
        }
        ch->out_stats.last_seq = rx_seq;
        offset += frame_len;
    }
}

/* ========== Interface handler (EP0 vendor requests) ========== */

static u32 test_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    switch (req->bRequest) {

#if TEST_EP_CTRL_ENABLE
    case TEST_VENDOR_REQ_SEND:
        ctrl_recv_offset = 0;
        usb_set_setup_recv(usb_device, test_ctrl_recv_handler);
        break;

    case TEST_VENDOR_REQ_RECV: {
        ctrl_in_seq++;
        memset(ctrl_buf + FRAME_HDR_SIZE, 0x80, TEST_CTRL_MAXP - FRAME_HDR_SIZE);
        u32 frame_len = fill_frame(ctrl_buf, TEST_CTRL_MAXP, ctrl_in_seq, 0x80);
        ctrl_in_stats.tx_bytes += frame_len;
        ctrl_in_stats.tx_packets++;
        usb_set_data_payload(usb_device, req, ctrl_buf, frame_len);
        break;
    }
#endif

    case TEST_VENDOR_REQ_GET_STAT: {
        u8 ch_idx = req->wValue & 0xFF;
        u8 dir = req->wValue >> 8;
        struct test_ep_stats *stats = NULL;
        if (ch_idx == 0xFF) {
#if TEST_EP_CTRL_ENABLE
            stats = dir ? &ctrl_in_stats : &ctrl_out_stats;
#endif
        } else if (ch_idx < TEST_CHANNEL_MAX && ch_cfg[ch_idx].enable) {
            stats = dir ? &channels[ch_idx].in_stats : &channels[ch_idx].out_stats;
        }
        if (stats) {
            usb_set_data_payload(usb_device, req, (u8 *)stats, sizeof(struct test_ep_stats));
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
        }
        break;
    }

    case TEST_VENDOR_REQ_START: {
        const usb_dev usb_id = usb_device2id(usb_device);
        for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
            if (ch_cfg[i].enable) {
                start_in_channel(usb_id, &channels[i]);
                log_info("CH%d IN : start ep %d", i, channels[i].ep);
            }
        }
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
        break;
    }

    case TEST_VENDOR_REQ_RESET:
        for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
            if (ch_cfg[i].enable) {
                memset(&channels[i].in_stats, 0, sizeof(struct test_ep_stats));
                memset(&channels[i].out_stats, 0, sizeof(struct test_ep_stats));
                channels[i].in_seq = 0;
            }
        }
#if TEST_EP_CTRL_ENABLE
        memset(&ctrl_in_stats, 0, sizeof(struct test_ep_stats));
        memset(&ctrl_out_stats, 0, sizeof(struct test_ep_stats));
        ctrl_in_seq = 0;
        ctrl_recv_offset = 0;
#endif
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
        break;

    default:
        usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
        break;
    }

    return 0;
}

/* ========== Stats timer ========== */

#define TEST_STATS_INTERVAL 1000

static u16 test_stats_timer;

static void test_stats_timer_cb(void *priv)
{
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!ch_cfg[i].enable) {
            continue;
        }
        struct test_ep_channel *ch = &channels[i];
        log_info("CH%d IN : tx %d/%dB", i, ch->in_stats.tx_packets, ch->in_stats.tx_bytes);
        log_info("CH%d OUT: rx %d/%dB err %d lost %d",
                 i, ch->out_stats.rx_packets, ch->out_stats.rx_bytes,
                 ch->out_stats.rx_errors, ch->out_stats.rx_lost);
    }
#if TEST_EP_CTRL_ENABLE
    log_info("CTRL IN : tx %d/%dB", ctrl_in_stats.tx_packets, ctrl_in_stats.tx_bytes);
    log_info("CTRL OUT: rx %d/%dB err %d", ctrl_out_stats.rx_packets, ctrl_out_stats.rx_bytes, ctrl_out_stats.rx_errors);
#endif
}

/* ========== Reset handler ========== */

static void test_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    if (test_stats_timer == 0) {
        test_stats_timer = sys_timer_add(NULL, test_stats_timer_cb, TEST_STATS_INTERVAL);
    }

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!ch_cfg[i].enable || ch_itf_num[i] != itf) {
            continue;
        }

        struct test_ep_channel *ch = &channels[i];
        ch->ep   = ch_cfg[i].ep;
        ch->maxp = ch_cfg[i].maxp;
        ch->type = ch_cfg[i].type;

        usb_g_ep_config(usb_id, ch->ep | USB_DIR_IN, ch->type, 1, ch->ep_in_dma, ch->maxp);
        usb_g_set_intr_hander(usb_id, ch->ep | USB_DIR_IN, test_in_isr);
        usb_set_intr_txe(usb_id, ch->ep);

        usb_g_ep_config(usb_id, ch->ep, ch->type, 1, ch->ep_out_dma, ch->maxp);
        usb_g_set_intr_hander(usb_id, ch->ep, test_out_isr);
        usb_set_intr_rxe(usb_id, ch->ep);
    }
}

/* ========== Register ========== */

u32 test_register(const usb_dev usb_id)
{
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!ch_cfg[i].enable) {
            continue;
        }
        channels[i].ep_in_dma  = usb_alloc_ep_dmabuffer(usb_id, ch_cfg[i].ep | USB_DIR_IN, ch_cfg[i].maxp);
        channels[i].ep_out_dma = usb_alloc_ep_dmabuffer(usb_id, ch_cfg[i].ep, ch_cfg[i].maxp);
        if (!channels[i].ep_in_dma || !channels[i].ep_out_dma) {
            log_error("CH%d dma alloc fail: in=%x out=%x", i, (u32)channels[i].ep_in_dma, (u32)channels[i].ep_out_dma);
            continue;
        }
        memset(channels[i].ep_in_dma,  0x80 | ch_cfg[i].ep, ch_cfg[i].maxp);
        memset(channels[i].ep_out_dma, ch_cfg[i].ep,        ch_cfg[i].maxp);
    }
    return 0;
}

/* ========== Descriptor config (dynamic) ========== */

u32 test_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    u8 *start = ptr;

#if TEST_EP_CTRL_ENABLE
    /* CTRL interface: 0 endpoints, subclass 0xFF */
    *ptr++ = USB_DT_INTERFACE_SIZE;
    *ptr++ = USB_DT_INTERFACE;
    *ptr++ = *cur_itf_num;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0xFF;
    *ptr++ = 0xFF;
    *ptr++ = TEST_INTERFACE_PROTOCOL;
    *ptr++ = 0x00;
    usb_set_interface_hander(usb_id, *cur_itf_num, test_itf_hander);
    usb_set_reset_hander(usb_id, *cur_itf_num, test_reset);

    (*cur_itf_num)++;
#endif

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!ch_cfg[i].enable) {
            continue;
        }

        u8 interval = 0;
        if (ch_cfg[i].type == TEST_XFER_ISOC) {
            interval = 8;
        }
        if (ch_cfg[i].type == TEST_XFER_INT)  {
            interval = 3;
        }

        /* Interface descriptor, subclass = channel index */
        *ptr++ = USB_DT_INTERFACE_SIZE;
        *ptr++ = USB_DT_INTERFACE;
        *ptr++ = *cur_itf_num;
        *ptr++ = 0x00;
        *ptr++ = 0x02;
        *ptr++ = 0xFF;
        *ptr++ = i;
        *ptr++ = TEST_INTERFACE_PROTOCOL;
        *ptr++ = 0x00;
        usb_set_interface_hander(usb_id, *cur_itf_num, test_itf_hander);
        usb_set_reset_hander(usb_id, *cur_itf_num, test_reset);
        ch_itf_num[i] = *cur_itf_num;

        /* EP IN */
        *ptr++ = USB_DT_ENDPOINT_SIZE;
        *ptr++ = USB_DT_ENDPOINT;
        *ptr++ = USB_DIR_IN | ch_cfg[i].ep;
        *ptr++ = ch_cfg[i].type;
        *ptr++ = LOBYTE(ch_cfg[i].maxp);
        *ptr++ = HIBYTE(ch_cfg[i].maxp);
        *ptr++ = interval;

        /* EP OUT */
        *ptr++ = USB_DT_ENDPOINT_SIZE;
        *ptr++ = USB_DT_ENDPOINT;
        *ptr++ = ch_cfg[i].ep;
        *ptr++ = ch_cfg[i].type;
        *ptr++ = LOBYTE(ch_cfg[i].maxp);
        *ptr++ = HIBYTE(ch_cfg[i].maxp);
        *ptr++ = interval;


        (*cur_itf_num)++;
    }

    return ptr - start;
}

/* ========== Release ========== */

void test_release(const usb_dev usb_id)
{
    if (test_stats_timer) {
        sys_timer_del(test_stats_timer);
        test_stats_timer = 0;
    }
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!ch_cfg[i].enable) {
            continue;
        }
        if (channels[i].ep_in_dma) {
            usb_free_ep_dmabuffer(usb_id, channels[i].ep_in_dma);
            channels[i].ep_in_dma = NULL;
        }
        if (channels[i].ep_out_dma) {
            usb_free_ep_dmabuffer(usb_id, channels[i].ep_out_dma);
            channels[i].ep_out_dma = NULL;
        }
    }
}

#endif /* TCFG_USB_TEST_ENABLE */
