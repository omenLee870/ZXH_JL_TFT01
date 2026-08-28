#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb_ctrl_transfer.h"
#include "usb/test_usb.h"

#if TCFG_USB_TEST_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB-TEST-H]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define TEST_TASK_NAME      "usb_test"
#define TEST_CTRL_MAXP      64
#define TEST_STATS_INTERVAL 10000 /* 打印统计间隔(ms) */
#define BULK_BATCH_SIZE     64   /* BULK每批包数 */

/* ========== Task message ========== */
#define USB_TEST_MSG_START       1
#define USB_TEST_MSG_CTRL_TEST   2
#define USB_TEST_MSG_BULK_RESUME 3

/* ========== Per-endpoint channel state ========== */
struct test_ep_channel {
    u8 host_ep_in;
    u8 host_ep_out;
    u8 target_ep_in;
    u8 target_ep_out;
    u16 maxp;
    u8 interval;
    u8 type;
    u8 *ep_in_dma;
    u8 *ep_out_dma;
    struct test_ep_stats rx_stats;
    struct test_ep_stats tx_stats;
    u32 tx_seq;
    u16 bulk_batch;
    u8 bulk_paused;
};

/* ========== Device state ========== */
struct test_host_device {
    struct usb_host_device *parent;
    struct test_ep_channel channels[TEST_CHANNEL_MAX];
    u8 interface_num[TEST_CHANNEL_MAX];
};

static struct test_host_device test_dev;

/* ========== interface_ctrl ========== */

static int test_set_power(struct usb_host_device *host_dev, u32 value)
{
    return 0;
}

static int test_get_power(struct usb_host_device *host_dev, u32 value)
{
    return 0;
}

static int test_release(struct usb_host_device *host_dev);

static const struct interface_ctrl test_ctrl = {
    .interface_class = 0xFF,
    .set_power = test_set_power,
    .get_power = test_get_power,
    .ioctl = NULL,
    .release = test_release,
};

/* ========== interface_info table ========== */
static const struct usb_interface_info test_if_info = {
    .ctrl = (struct interface_ctrl *) &test_ctrl,
    .dev.p = &test_dev,
};

/* ========== Data helpers ========== */

static u32 fill_frame(u8 *buf, u32 max_len, u32 seq, u8 pattern)
{
    u32 data_len = (seq % (max_len - FRAME_HDR_SIZE)) + 1;
    memcpy(buf, &seq, sizeof(u32));
    memcpy(buf + 4, &data_len, sizeof(u32));
    return FRAME_HDR_SIZE + data_len;
}

/* ========== RX ISR (host IN EP, device -> host) ========== */

static void test_rx_isr(struct usb_host_device *host_dev, u32 ep)
{
    const usb_dev usb_id = host_device2id(host_dev);
    struct test_ep_channel *ch = NULL;
    int ch_idx = -1;

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if ((test_dev.channels[i].host_ep_in) == ep) {
            ch = &test_dev.channels[i];
            ch_idx = i;
            break;
        }
    }
    if (!ch || !ch->ep_in_dma) {
        return;
    }

    static u8 buf[3072];
    u32 total_len = usb_h_ep_read_async(usb_id, ep, ch->target_ep_in,
                                        buf, ch->maxp, ch->type, 0);
    if (!total_len) {
        if (ch->type != TEST_XFER_BULK || ch->bulk_batch < BULK_BATCH_SIZE) {
            usb_h_ep_read_async(usb_id, ep, ch->target_ep_in,
                                buf, ch->maxp, ch->type, 1);
        }
        return;
    }

    ch->rx_stats.rx_bytes += total_len;

    u32 offset = 0;
    while (offset + FRAME_HDR_SIZE <= total_len) {
        u32 rx_seq, data_len;
        memcpy(&rx_seq, buf + offset, sizeof(u32));
        memcpy(&data_len, buf + offset + 4, sizeof(u32));
        u32 frame_len = FRAME_HDR_SIZE + data_len;
        if (offset + frame_len > total_len) {
            break;
        }

        ch->rx_stats.rx_packets++;
        u32 expected_seq = ch->rx_stats.last_seq + 1;

        if (rx_seq != expected_seq) {
            ch->rx_stats.rx_errors++;
            if (rx_seq > expected_seq) {
                ch->rx_stats.rx_lost += rx_seq - expected_seq;
            }
        }
        for (u32 i = 0; i < data_len; i++) {
            if (buf[offset + FRAME_HDR_SIZE + i] != (0x80 | ch->target_ep_in)) {
                ch->rx_stats.rx_errors++;
                break;
            }
        }
        ch->rx_stats.last_seq = rx_seq;
        offset += frame_len;
    }

    if (ch->type == TEST_XFER_BULK && ch->bulk_batch >= BULK_BATCH_SIZE) {
        if (!ch->bulk_paused) {
            ch->bulk_paused = 1;
            os_taskq_post_msg(TEST_TASK_NAME, 2, USB_TEST_MSG_BULK_RESUME, ch_idx);
        }
        return;
    }
    if (ch->type == TEST_XFER_BULK) {
        ch->bulk_batch++;
    }
    usb_h_ep_read_async(usb_id, ep, ch->target_ep_in,
                        buf, ch->maxp, ch->type, 1);
}

/* ========== TX helper ========== */

static void start_tx_channel(const usb_dev usb_id, struct test_ep_channel *ch)
{
    ch->tx_seq++;
    u32 frame_len = fill_frame(ch->ep_out_dma, ch->maxp, ch->tx_seq, ch->target_ep_out);
    ch->tx_stats.tx_bytes += frame_len;
    ch->tx_stats.tx_packets++;
    usb_h_ep_write_async(usb_id, ch->host_ep_out, ch->maxp,
                         ch->target_ep_out, ch->ep_out_dma, frame_len, ch->type, 1);
}

/* ========== TX ISR (host OUT EP, host -> device) ========== */

static void test_tx_isr(struct usb_host_device *host_dev, u32 ep)
{
    const usb_dev usb_id = host_device2id(host_dev);
    struct test_ep_channel *ch = NULL;
    int ch_idx = -1;

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (test_dev.channels[i].host_ep_out == ep) {
            ch = &test_dev.channels[i];
            ch_idx = i;
            break;
        }
    }
    if (!ch) {
        return;
    }

    if (ch->type == TEST_XFER_BULK && ch->bulk_batch >= BULK_BATCH_SIZE) {
        if (!ch->bulk_paused) {
            ch->bulk_paused = 1;
            os_taskq_post_msg(TEST_TASK_NAME, 2, USB_TEST_MSG_BULK_RESUME, ch_idx);
        }
        return;
    }
    if (ch->type == TEST_XFER_BULK) {
        ch->bulk_batch++;
    }
    start_tx_channel(usb_id, ch);
}

/* ========== Parser ========== */

int usb_test_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
    pBuf += sizeof(struct usb_interface_descriptor);
    int len = sizeof(struct usb_interface_descriptor);

    u8 subclass = interface->bInterfaceSubClass;
    if (subclass >= TEST_CHANNEL_MAX) {
        log_error("unknown test subclass %d", subclass);
        return len;
    }

    host_dev->interface_info[interface_num] = &test_if_info;
    test_dev.parent = host_dev;
    test_dev.interface_num[subclass] = interface_num;

    struct test_ep_channel *ch = &test_dev.channels[subclass];

    for (int endnum = 0; endnum < interface->bNumEndpoints; endnum++) {
        struct usb_endpoint_descriptor *end_desc = (struct usb_endpoint_descriptor *)pBuf;

        if (end_desc->bDescriptorType != USB_DT_ENDPOINT ||
            end_desc->bLength != USB_DT_ENDPOINT_SIZE) {
            log_error("invalid ep descriptor");
            return -USB_DT_ENDPOINT;
        }

        len += USB_DT_ENDPOINT_SIZE;
        pBuf += USB_DT_ENDPOINT_SIZE;

        if (end_desc->bEndpointAddress & USB_DIR_IN) {
            ch->target_ep_in = end_desc->bEndpointAddress & 0x0f;
            ch->maxp = end_desc->wMaxPacketSize;
        } else {
            ch->target_ep_out = end_desc->bEndpointAddress & 0x0f;
        }

        ch->type = end_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
        ch->interval = end_desc->bInterval;
    }

    log_debug("test parser: subclass %d, ep_in %d, ep_out %d, maxp %d",
              subclass, ch->target_ep_in, ch->target_ep_out, ch->maxp);

    return len;
}

int _usb_test_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    return usb_test_parser(host_dev, interface_num, pBuf);
}

/* ========== Process (post-enumeration setup) ========== */

void usb_test_process(struct usb_host_device *host_dev)
{
    const usb_dev usb_id = host_device2id(host_dev);

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        struct test_ep_channel *ch = &test_dev.channels[i];

        if (ch->target_ep_in == 0 && ch->target_ep_out == 0) {
            continue;
        }

        //IN EP: device -> host, 接收端
        if (ch->target_ep_in) {
            ch->host_ep_in = usb_get_ep_num(usb_id, USB_DIR_IN, ch->type);
            if (ch->host_ep_in == 0xFF) {
                log_error("CH%d IN: no host ep", i);
                ch->target_ep_in = 0;
                ch->target_ep_out = 0;
                continue;
            }
            ch->ep_in_dma = usb_h_alloc_ep_buffer(usb_id, ch->host_ep_in | USB_DIR_IN, ch->maxp * 2);
            usb_write_rxfuncaddr(usb_id, ch->host_ep_in, host_dev->private_data.devnum);
            usb_h_ep_config(usb_id, ch->host_ep_in | USB_DIR_IN, ch->type, 1, ch->interval,
                            ch->ep_in_dma, ch->maxp);
            usb_h_set_ep_isr(host_dev, ch->host_ep_in | USB_DIR_IN, test_rx_isr, NULL);
            //arm首次接收
            usb_h_ep_read_async(usb_id, ch->host_ep_in, ch->target_ep_in,
                                ch->ep_in_dma, ch->maxp, ch->type, 1);
            log_debug("test ep_in: host %d -> target %d", ch->host_ep_in, ch->target_ep_in);
        }

        //OUT EP: host -> device, 发送端
        if (ch->target_ep_out) {
            ch->host_ep_out = usb_get_ep_num(usb_id, USB_DIR_OUT, ch->type);
            if (ch->host_ep_out == 0xFF) {
                log_error("CH%d OUT: no host ep", i);
                ch->target_ep_in = 0;
                ch->target_ep_out = 0;
                continue;
            }
            ch->ep_out_dma = usb_h_alloc_ep_buffer(usb_id, ch->host_ep_out, ch->maxp);
            memset(ch->ep_out_dma, ch->target_ep_out, ch->maxp);
            usb_write_txfuncaddr(usb_id, ch->host_ep_out, host_dev->private_data.devnum);
            usb_h_ep_config(usb_id, ch->host_ep_out, ch->type, 1, ch->interval,
                            ch->ep_out_dma, ch->maxp);
            usb_h_set_ep_isr(host_dev, ch->host_ep_out, test_tx_isr, NULL);
            log_debug("test ep_out: host %d -> target %d", ch->host_ep_out, ch->target_ep_out);
        }
    }

    //发送START命令给从机，触发从机IN EP开始发送
    usb_control_msg(host_dev,
                    TEST_VENDOR_REQ_START, USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                    0, 0, NULL, 0);

    //启动主机所有OUT EP首次发送
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        struct test_ep_channel *ch = &test_dev.channels[i];
        if (ch->target_ep_out) {
            start_tx_channel(usb_id, ch);
        }
    }
}

/* ========== Release ========== */

static u16 test_stats_timer;

static int test_release(struct usb_host_device *host_dev)
{
    const usb_dev usb_id = host_device2id(host_dev);

    if (test_stats_timer) {
        sys_timer_del(test_stats_timer);
        test_stats_timer = 0;
    }

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        struct test_ep_channel *ch = &test_dev.channels[i];

        if (ch->host_ep_in) {
            usb_h_set_ep_isr(host_dev, ch->host_ep_in | USB_DIR_IN, NULL, NULL);
            usb_h_free_ep_buffer(usb_id, ch->ep_in_dma);
            usb_free_ep_num(usb_id, ch->host_ep_in | USB_DIR_IN);
            ch->host_ep_in = 0;
        }
        if (ch->host_ep_out) {
            usb_h_set_ep_isr(host_dev, ch->host_ep_out, NULL, NULL);
            usb_h_free_ep_buffer(usb_id, ch->ep_out_dma);
            usb_free_ep_num(usb_id, ch->host_ep_out);
            ch->host_ep_out = 0;
        }
    }

    memset(&test_dev, 0, sizeof(test_dev));
    log_debug("test host release done");
    return 0;
}

/* ========== Application API ========== */

int usb_test_get_local_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats)
{
    if (type >= TEST_CHANNEL_MAX) {
        return -1;
    }
    memcpy(stats, &test_dev.channels[type].rx_stats, sizeof(struct test_ep_stats));
    stats->tx_bytes = test_dev.channels[type].tx_stats.tx_bytes;
    stats->tx_packets = test_dev.channels[type].tx_stats.tx_packets;
    return 0;
}

int usb_test_get_remote_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats)
{
    return usb_control_msg(host_dev,
                           TEST_VENDOR_REQ_GET_STAT, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                           type, test_dev.interface_num[type],
                           stats, sizeof(struct test_ep_stats));
}

int usb_test_get_remote_in_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats)
{
    return usb_control_msg(host_dev,
                           TEST_VENDOR_REQ_GET_STAT, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                           0x0100 | type, test_dev.interface_num[type],
                           stats, sizeof(struct test_ep_stats));
}

int usb_test_get_remote_ctrl_stats(struct usb_host_device *host_dev, struct test_ep_stats *out_stats, struct test_ep_stats *in_stats)
{
    int ret;
    ret = usb_control_msg(host_dev,
                          TEST_VENDOR_REQ_GET_STAT, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                          0x00FF, 0,
                          out_stats, sizeof(struct test_ep_stats));
    if (ret) {
        return ret;
    }
    return usb_control_msg(host_dev,
                           TEST_VENDOR_REQ_GET_STAT, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                           0x01FF, 0,
                           in_stats, sizeof(struct test_ep_stats));
}

int usb_test_reset_stats(struct usb_host_device *host_dev)
{
    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        memset(&test_dev.channels[i].rx_stats, 0, sizeof(struct test_ep_stats));
        memset(&test_dev.channels[i].tx_stats, 0, sizeof(struct test_ep_stats));
        test_dev.channels[i].tx_seq = 0;
    }
    return usb_control_msg(host_dev,
                           TEST_VENDOR_REQ_RESET, USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                           0, 0, NULL, 0);
}

/* ========== CTRL (EP0) test state ========== */
#if TEST_EP_CTRL_ENABLE
static u8 ctrl_buf[TEST_CTRL_MAXP];
static u32 ctrl_tx_seq;
static u32 ctrl_rx_seq;
static struct test_ep_stats ctrl_local_stats;

static u32 ctrl_fill_frame(u8 *buf, u32 max_len, u32 seq)
{
    u32 data_len = (seq % (max_len - FRAME_HDR_SIZE)) + 1;
    memcpy(buf, &seq, sizeof(u32));
    memcpy(buf + 4, &data_len, sizeof(u32));
    return FRAME_HDR_SIZE + data_len;
}

static u32 ctrl_verify_frame(const u8 *buf, u32 rx_len, u32 expected_seq)
{
    u32 rx_seq, data_len;
    memcpy(&rx_seq, buf, sizeof(u32));
    memcpy(&data_len, buf + 4, sizeof(u32));
    if (rx_seq != expected_seq) {
        return 1;
    }
    if (FRAME_HDR_SIZE + data_len != rx_len) {
        return 1;
    }
    for (u32 i = 0; i < data_len; i++) {
        if (buf[FRAME_HDR_SIZE + i] != 0x80) {
            return 1;
        }
    }
    return 0;
}
#endif

/* ========== Timers ========== */

static void test_stats_timer_cb(void *priv)
{
    struct usb_host_device *host_dev = (struct usb_host_device *)priv;
    if (!host_dev || !test_dev.parent) {
        return;
    }

    struct test_ep_stats local, remote_out, remote_in;

    for (int i = 0; i < TEST_CHANNEL_MAX; i++) {
        if (!test_dev.channels[i].target_ep_in && !test_dev.channels[i].target_ep_out) {
            continue;
        }

        usb_test_get_local_stats(host_dev, i, &local);
        usb_test_get_remote_stats(host_dev, i, &remote_out);
        usb_test_get_remote_in_stats(host_dev, i, &remote_in);

        log_debug("CH%d host: tx %d/%dB rx %d/%dB err %d lost %d | "
                  "dev: tx %d/%dB rx %d/%dB err %d lost %d",
                  i,
                  local.tx_packets, local.tx_bytes,
                  local.rx_packets, local.rx_bytes, local.rx_errors, local.rx_lost,
                  remote_in.tx_packets, remote_in.tx_bytes,
                  remote_out.rx_packets, remote_out.rx_bytes,
                  remote_out.rx_errors, remote_out.rx_lost);
    }

#if TEST_EP_CTRL_ENABLE
    {
        struct test_ep_stats ctrl_dev_out, ctrl_dev_in;
        usb_test_get_remote_ctrl_stats(host_dev, &ctrl_dev_out, &ctrl_dev_in);
        log_debug("CTRL host: tx %d/%dB rx %d/%dB err %d | "
                  "dev: tx %d/%dB rx %d/%dB err %d",
                  ctrl_local_stats.tx_packets, ctrl_local_stats.tx_bytes,
                  ctrl_local_stats.rx_packets, ctrl_local_stats.rx_bytes, ctrl_local_stats.rx_errors,
                  ctrl_dev_in.tx_packets, ctrl_dev_in.tx_bytes,
                  ctrl_dev_out.rx_packets, ctrl_dev_out.rx_bytes, ctrl_dev_out.rx_errors);
    }
#endif
}

/* ========== Test task ========== */

static void usb_test_task(void *arg)
{
    int ret;
    int msg[16];

    log_debug("test task running");

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ || msg[0] != Q_MSG) {
            continue;
        }

        switch (msg[1]) {
        case USB_TEST_MSG_START: {
            struct usb_host_device *host_dev = (struct usb_host_device *)msg[2];
            if (!host_dev) {
                break;
            }

            usb_test_process(host_dev);

            test_stats_timer = sys_timer_add(host_dev, test_stats_timer_cb, TEST_STATS_INTERVAL);
#if TEST_EP_CTRL_ENABLE
            ctrl_tx_seq = 0;
            ctrl_rx_seq = 0;
            memset(&ctrl_local_stats, 0, sizeof(struct test_ep_stats));
            os_taskq_post_msg(TEST_TASK_NAME, 2, USB_TEST_MSG_CTRL_TEST, (int)host_dev);
#endif
            log_debug("test started");
            break;
        }

#if TEST_EP_CTRL_ENABLE
        case USB_TEST_MSG_CTRL_TEST: {
            struct usb_host_device *host_dev = (struct usb_host_device *)msg[2];
            if (!host_dev || !test_dev.parent) {
                break;
            }

            /* SEND: host -> device */
            ctrl_tx_seq++;
            memset(ctrl_buf + FRAME_HDR_SIZE, 0x00, TEST_CTRL_MAXP - FRAME_HDR_SIZE);
            u32 tx_frame_len = ctrl_fill_frame(ctrl_buf, TEST_CTRL_MAXP, ctrl_tx_seq);
            usb_control_msg(host_dev,
                            TEST_VENDOR_REQ_SEND, USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                            0, 0, ctrl_buf, tx_frame_len);
            ctrl_local_stats.tx_bytes += tx_frame_len;
            ctrl_local_stats.tx_packets++;

            /* RECV: device -> host */
            usb_control_msg(host_dev,
                            TEST_VENDOR_REQ_RECV, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
                            0, 0, ctrl_buf, TEST_CTRL_MAXP);
            u32 data_len;
            memcpy(&data_len, ctrl_buf + 4, sizeof(u32));
            u32 rx_len = FRAME_HDR_SIZE + data_len;
            ctrl_rx_seq++;
            ctrl_local_stats.rx_bytes += rx_len;
            ctrl_local_stats.rx_packets++;
            if (ctrl_verify_frame(ctrl_buf, rx_len, ctrl_rx_seq)) {
                ctrl_local_stats.rx_errors++;
            }

            os_time_dly(1);
            os_taskq_post_msg(TEST_TASK_NAME, 2, USB_TEST_MSG_CTRL_TEST, (int)host_dev);
            break;
        }
#endif

        case USB_TEST_MSG_BULK_RESUME: {
            int ch_idx = msg[2];
            if (ch_idx < 0 || ch_idx >= TEST_CHANNEL_MAX) {
                break;
            }
            struct usb_host_device *host_dev = test_dev.parent;
            if (!host_dev) {
                break;
            }
            const usb_dev usb_id = host_device2id(host_dev);
            struct test_ep_channel *ch = &test_dev.channels[ch_idx];

            os_time_dly(1);
            ch->bulk_batch = 0;
            ch->bulk_paused = 0;

            if (ch->host_ep_in) {
                usb_h_ep_read_async(usb_id, ch->host_ep_in, ch->target_ep_in,
                                    ch->ep_in_dma, ch->maxp, ch->type, 1);
            }
            if (ch->host_ep_out) {
                start_tx_channel(usb_id, ch);
            }
            break;
        }
        }
    }
}

/* ========== Init ========== */

static int usb_test_init(void)
{
    int err = task_create(usb_test_task, NULL, TEST_TASK_NAME);
    if (err != OS_NO_ERR) {
        log_error("test task create fail %x", err);
    }
    return 0;
}
late_initcall(usb_test_init);

/* ========== Public API: post message ========== */

void usb_test_post_start(struct usb_host_device *host_dev)
{
    os_taskq_post_msg(TEST_TASK_NAME, 2, USB_TEST_MSG_START, (int)host_dev);
}

static bool _usb_test_match(const struct usb_device_descriptor *device_desc,
                            const struct usb_interface_descriptor *interface_desc,
                            const u8 *cfg_buf, u32 cfg_len)
{
    return (interface_desc->bInterfaceClass == 0xFF &&
            interface_desc->bInterfaceProtocol == TEST_INTERFACE_PROTOCOL);
}

USB_HOST_CLASS_REGISTER(_usb_test_match, _usb_test_parser,
                        0xFF, "test%d");

#endif /* TCFG_USB_TEST_ENABLE */
