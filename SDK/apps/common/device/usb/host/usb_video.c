#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")

#include <string.h>
#include "usb_ctrl_transfer.h"
#include "usb_host.h"
#include "usb_ctrl_transfer.h"
#include "host_uvc.h"
#include "usb_stack.h"
#include "device/device_drive.h"
#include "app_config.h"
/* #include "usb/host/usb_host.h" */

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */    /* uncomment to enable hexdump */

#ifdef LOG_DUMP_ENABLE
#define UVC_HEXDUMP(x, y)  log_debug_hexdump(x, y)
#else
#define UVC_HEXDUMP(x, y)
#endif
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_HOST_UVC_ENABLE

/*static	USB_DEV uvc_ep3;*/
/*static	USB_DEV uvc_ep4;*/
#define     USB_DT_INTERFACE_HEADER_DESCRIPTOR_SIZE	0x0d
#define     STOP_TRANSFER   1
#define 	UVC_SUPPORT_MJEPG_ENABLE		1
#define 	UVC_SUPPORT_H264_ENABLE			1
#define 	UVC_SUPPORT_YUYV_ENABLE			0//707不支持
#if (UVC_SUPPORT_MJEPG_ENABLE+UVC_SUPPORT_H264_ENABLE+UVC_SUPPORT_YUYV_ENABLE)<1
#error "uvc config error: no format enabled!!!!!"
#endif

#define time_get(t,ms)	t = jiffies + msecs_to_jiffies(ms)
#define time_out(a,b)  (((long)(a) - (long)(b)) > 0)

#define  check_ret(ret)\
    if ((int)(ret) < DEV_ERR_NONE) {\
        log_error("err ret = %d\n",ret);\
        return ret;\
    }
#define UVC_BULK_READ_MULT   8
#define UVC_BULK_SAVE_SIZE   (512 * UVC_BULK_READ_MULT)

struct usb_video_manager {
    u8 online;
    u8 open;
    volatile u8 in_irq;
    u8 itf_base;
    const char *name;
    u8 *buffer;
    u8 save_buf[UVC_BULK_SAVE_SIZE];
    u32 frame_cnt;
    struct device dev;
};

static struct usb_video_manager uvc_manager[USB_MAX_HW_NUM];

/* BULK statistics (global, printed at recording end) */
static u32 g_bulk_rx_pkts = 0;
static u32 g_bulk_rx_bytes = 0;
static u32 g_bulk_rx_errs = 0;
static u32 g_bulk_stream_errs = 0;
static u32 g_bulk_rx_min = 0xFFFFFFFF;
static u32 g_bulk_rx_max = 0;
static u32 g_error_frame_pay_drops = 0;  // DIAG: non-header H264 pkts dropped by error_frame
static u16 g_target_width = 0;
static u16 g_target_height = 0;

#define     usbpriv_to_usbid(priv)	((usb_dev)(priv->usb_id))

static u8 usbdev_to_usbid(struct device *device)
{
    struct usb_video_manager *hdl = container_of(device, struct usb_video_manager, dev);
    return hdl->name[3] - '0';

}
/**
 * @brief Set UVC target resolution from board platform data
 */
void uvc_host_set_target_reso(u16 width, u16 height)
{
    g_target_width = width;
    g_target_height = height;
    log_debug("uvc_set_target: %dx%d", width, height);
}

int uvc_host_online(void)
{
    int id = 0;
    for (id = 0; id < 2; id++) {
        if (uvc_manager[id].online) {
            return id;
        }
    }
    return -EINVAL;
}
static u32 uvc_host_get_frame(usb_dev usb_id)
{
    return uvc_manager[usb_id].frame_cnt;
}
static int uvc_set_power(struct usb_host_device *host_dev, u32 value)
{
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    hdl->online = 0;
    return 0;
}
static int uvc_get_power(struct usb_host_device *host_dev, u32 value)
{
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    return hdl->online;
}
static int uvc_ioctl(struct usb_host_device *host_dev, u32 cmd, u32 arg)
{
    return 0;
}
static const struct interface_ctrl uvc_host_ops = {
    .interface_class = USB_CLASS_VIDEO,

    .set_power = uvc_set_power,
    .get_power = uvc_get_power,
    .ioctl = uvc_ioctl,
};
static struct usb_interface_info uvc_host_inf[USB_MAX_HW_NUM] = {
    {
        .ctrl = (struct interface_ctrl *) &uvc_host_ops,
        .dev.uvc = NULL,
    },
};

#define device_to_uvc(device)     (uvc_host_inf[usb_id].dev.uvc)

/**
 * @brief       usb video 初始化
 *
 * @param:     usb_id : usb端口ID
 *             0: usb0 fusb
 *             1: usb1 husb
 * @param:     sub_id : 同一usb端口的 子设备id
 *             目前支持husb端口上挂最多2个子设备
 *
 * @return:    0: 成功  非0: 失败
 **/
int usb_host_video_init(const usb_dev usb_id, const u8 sub_id)
{
    int ret = 0;
    struct device *device;
    struct usb_host_device *host_dev;
    struct usb_uvc *uvc;
    u32 maxpsize;
    struct usb_video_manager *hdl = &uvc_manager[sub_id];

    device = &hdl->dev;
    if (!device) {
        return -DEV_ERR_OFFLINE;
    }
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -DEV_ERR_OFFLINE;
    }
    /* uvc = device_to_uvc(device); */
    uvc = uvc_host_inf[sub_id].dev.uvc;
    if (!uvc) {
        return -DEV_ERR_OFFLINE;
    }
    log_debug("[UVC_DBG] video_init: uvc=%p ep=%d host_ep=%d vs_itf_num=%d wMaxPacketSize=%d xfer_type=%d format=0x%x",
              uvc, uvc->ep, uvc->host_ep, uvc->vs_itf_num, uvc->wMaxPacketSize, uvc->xfer_type, uvc->format);
    /* 修改原因: 在 video_init 入口处添加关键参数打印,便于确认 UVC 设备初始化时 ep/vs_itf_num/format 等是否正确解析,尤其对双摄设备(两个 VS interface)至关重要。 */
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
        if (usb_id == 0) {
            hdl->buffer = malloc(maxpsize);
        } else if (usb_id == 1) {
            hdl->buffer = malloc(maxpsize * ISO_INTR_CNT);
        }
        if (!hdl->buffer) {
            log_error("err no mem in hdl->buffer");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
        uvc->host_ep = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_ISOC);
        if ((int)uvc->host_ep < 0) {
            log_error("uvc get ep_num fail");
            ret = -DEV_ERR_INUSE;
            goto __exit_fail;
        }
        if (usb_id == 0) {
            uvc->ep_buffer = usb_h_alloc_ep_buffer(usb_id, uvc->host_ep | USB_DIR_IN, maxpsize);
        } else if (usb_id == 1) {
            uvc->ep_buffer = usb_h_alloc_ep_buffer(usb_id, uvc->host_ep | USB_DIR_IN, maxpsize * ISO_INTR_CNT);
        }
        if (!uvc->ep_buffer) {
            log_error("uvc alloc ep buffer fail");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
        hdl->buffer = malloc(maxpsize * UVC_BULK_READ_MULT * 2);  //第二份用于保存bulk的上一包
        if (!hdl->buffer) {
            log_error("err no mem in hdl->buffer");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
        if (maxpsize * UVC_BULK_READ_MULT > UVC_BULK_SAVE_SIZE) {
            log_error("FATAL: save_buf too small!");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
        uvc->host_ep = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_BULK);
        if ((int)uvc->host_ep < 0) {
            log_error("uvc get ep_num fail");
            ret = -DEV_ERR_INUSE;
            goto __exit_fail;
        }
        uvc->ep_buffer = usb_h_alloc_ep_buffer(usb_id, uvc->host_ep | USB_DIR_IN, maxpsize * UVC_BULK_READ_MULT * 2);  //bulk传输dma使用double buffer
        if (!uvc->ep_buffer) {
            log_error("uvc alloc ep buffer fail");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
    }

    {
        u8 stream_alt = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_if_alt_num : uvc->mjpeg_if_alt_num;
        if (stream_alt != 0) {
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, uvc->vs_itf_num, NULL, 0);
            if (ret < DEV_ERR_NONE) {
                log_error("USB_REQ_SET_INTERFACE %d to 0 fail", uvc->vs_itf_num);
                goto __exit_fail;
            }
        }
    }

    hdl->online = 1;
    log_debug("[UVC_FLOW] video_init SUCCESS, uvc online=1");

    return DEV_ERR_NONE;

__exit_fail:
    if (uvc->ep_buffer) {
        usb_h_free_ep_buffer(usb_id, uvc->ep_buffer);
        uvc->ep_buffer = NULL;
    }
    if (hdl->buffer) {
        free(hdl->buffer);
        hdl->buffer = NULL;
    }
    return ret;
}

static struct device *uvc_host_device_find(const char *name)
{
    if (!name) {
        return NULL;
    }
    usb_dev usb_id = name[3] - '0';
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    if (usb_id >= 0 && usb_id <= 1 && name && hdl->name) {
        if (!strcmp(hdl->name, name)) {
            return &hdl->dev;
        }
    }
    return NULL;
}

static int uvc_dummy_stream_out(void *fd, int cnt, void *stream_list, int eof)
{
    return 0;
}

int usb_uvc_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    int len = 0;
    int cur_len;
    int cur_type;
    int cur_sub_type;
    int loop = 1;
    int bControlSize;
    int formart_num;
    u32 frame_num;
    u16 wTotalLength = 0;
    u32 wMaxPacketSize = 0;
    u32 AlternateSetting = 0;
    u8 assocition_num = 0;
    u8 format_support = 0;
    struct usb_endpoint_descriptor end_desc;
    struct usb_uvc *real_uvc = NULL;
    struct usb_private_data *private_data = &host_dev->private_data;
    usb_dev usb_id = private_data->usb_id;
    if (interface_num) {
        //同一个usb口下另外一个uvc 设备
        usb_id = 0;
    }
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    u8 int_ep = 0;
    u8 ItfSubClass = 0;
    u8 *pformat;

    struct usb_uvc *uvc = (struct usb_uvc *)zalloc(sizeof(struct usb_uvc));
    if (!uvc) {
        log_error("err no mem in usb_uvc_parser\n\n");
        len = -DEV_ERR_INVALID_BUF;
        goto __err;
    }
    uvc->bfh = 0;
    uvc->cur_frame = 1;
    uvc->stream_out = uvc_dummy_stream_out;

    while (loop) {
        cur_len = *(pBuf + len);
        cur_type = *(pBuf + len + 1);
        cur_sub_type = *(pBuf + len + 2);

        if (!cur_len) {
            break;
        }
        /* UVC_HEXDUMP((u8 *)(pBuf + len), cur_len); */

        u8 cur_vs_itf_num = 0;  /* track current VS interface number for ep sync */
        u8 cur_vs_ep = 0;       /* track current VS endpoint for per-format save */
        /* 修改原因: 原代码只有一个全局的 uvc->ep 和 0 // uvc->vs_itf_num,在双摄设备上,MJPEG 和 H264 分别属于不同的 VS interface,各自有不同的 endpoint。新增这两个局部变量在描述符遍历过程中跟踪当前正在处理的 VS interface 信息。 */

        if (cur_type == USB_DT_INTERFACE_ASSOCIATION) {
            if (*(pBuf + len + 4) != USB_CLASS_VIDEO ||
                *(pBuf + len + 5) != UVC_SC_VIDEO_INTERFACE_COLLECTION) {
                break;
            }
            //	composite device

            log_debug("USB_DT_INTERFACE_ASSOCIATION");
            UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
            if (++assocition_num >= 1) {
                //有2个uvc设备
                break;
            }
        } else if (cur_type == USB_DT_INTERFACE) {
            if (cur_len != USB_DT_INTERFACE_SIZE) {
                len = -USB_DT_INTERFACE;
                goto __err;
            }
            if (*(pBuf + len + 5) != USB_CLASS_VIDEO) {
                break;
            }
            log_debug("USB_DT_INTERFACE");
            UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
            cur_vs_itf_num = *(pBuf + len + 2);  // Track current VS interface number
            uvc->vs_itf_num = cur_vs_itf_num;
            /* 修改原因: 在解析 USB_DT_INTERFACE 描述符时,记录当前 VS interface 的接口号。原代码没有跟踪此信息,导致多 VS interface 设备无法区分 MJPEG 和 H264 各自属于哪个 interface。 */
            AlternateSetting = *(pBuf + len + 3);
            log_debug("AlternateSetting:%d", AlternateSetting);

            if (*(pBuf + len + 6) == UVC_SC_VIDEOCONTROL) {
                ItfSubClass = UVC_SC_VIDEOCONTROL;
                hdl->itf_base = *(pBuf + len + 2);
            } else if (*(pBuf + len + 6) == UVC_SC_VIDEOSTREAMING) {
                ItfSubClass = UVC_SC_VIDEOSTREAMING;
            } else {
                log_error("invalid video subclass");
                len = -USB_DT_INTERFACE;
                goto __err;
            }
        } else if (cur_type == USB_DT_CS_INTERFACE) {
            if (ItfSubClass == UVC_SC_VIDEOCONTROL) {
                switch (cur_sub_type) {
                case UVC_VC_HEADER:
                    log_debug("VC_HEADER:%d %d, uvc ver %02x%02x", len, *(pBuf + len), *(pBuf + len + 4), *(pBuf + len + 3));
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                case UVC_VC_INPUT_TERMINAL:
                    bControlSize = *(pBuf + len + 14);
                    log_debug("VC_INPUT_TERMINAL:%d %d %d", len, *(pBuf + len), bControlSize);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                case UVC_VC_PROCESSING_UNIT:
                    bControlSize = *(pBuf + len + 7);
                    log_debug("VC_PROCESSING_UNIT:%d %d %d", len, *(pBuf + len), bControlSize);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                case UVC_VC_OUTPUT_TERMINAL:
                    bControlSize = *(pBuf + len + 8);
                    log_debug("VC_OUTPUT_TERMINAL:%d %d %d", len, *(pBuf + len), bControlSize);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                case UVC_VC_SELECTOR_UNIT:
                    break;

                case UVC_VC_EXTENSION_UNIT:
                    bControlSize = *(pBuf + len + 20);
                    log_debug("VC_EXTENSION_UNIT:%d %d %d %d",
                              len,
                              *(pBuf + len),
                              bControlSize,
                              *(pBuf + len + 22 + bControlSize));
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                default :
                    log_debug("Unrecognized VC UNIT:%d %d", len, *(pBuf + len));
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;
                }
            } else if (ItfSubClass == UVC_SC_VIDEOSTREAMING) {
                switch (cur_sub_type) {
                case UVC_VS_INPUT_HEADER:
                    formart_num = *(pBuf + len + 3);
                    bControlSize = *(pBuf + len + 12);
                    memcpy(&wTotalLength, pBuf + len + 4, 2);
                    log_debug("VS_INPUT_HEADER:%d %d %d",
                              wTotalLength,
                              formart_num,
                              bControlSize);
                    UVC_HEXDUMP((u8 *)(pBuf + len), *(pBuf + len));
                    wTotalLength -= *(pBuf + len);
                    break;

                case UVC_VS_FORMAT_MJPEG:
                    format_support = UVC_SUPPORT_MJEPG_ENABLE;
                    pformat = (u8 *)(pBuf + len);
                    frame_num = pformat[4];
                    uvc->mjpeg_format_index = pformat[3];
                    wTotalLength -= pformat[0];
                    uvc->format += UVC_VS_FORMAT_MJPEG;
                    /* Per-format ep/vs_itf_num saved at end of parser after all endpoints parsed */
                    log_debug("find mjpeg format descriptor %d %d %d",
                              frame_num,

                              pformat[3],
                              wTotalLength);
                    UVC_HEXDUMP(pformat, pformat[0]);
                    uvc->cur_frame = pformat[6];
                    log_debug("default frame id %d", uvc->cur_frame);
                    pformat += pformat[0];
                    uvc->reso_num = frame_num;
                    real_uvc = (struct usb_uvc *)zalloc(sizeof(struct usb_uvc) + sizeof(struct uvc_frame_info) * (1 + frame_num));
                    if (!real_uvc) {
                        log_error("err no mem in usb_uvc_parser\n\n");
                        len = -DEV_ERR_INVALID_BUF;
                        goto __err;
                    }
                    for (int i = 0; i < frame_num; i++) {
                        memcpy(&real_uvc->pframe[i].width, &pformat[5], 2);
                        memcpy(&real_uvc->pframe[i].height, &pformat[7], 2);
                        log_debug("frame info %d %d x %d",
                                  1 + i,
                                  real_uvc->pframe[i].width,
                                  real_uvc->pframe[i].height);
                        UVC_HEXDUMP(pformat, pformat[0]);
                        if (g_target_width && g_target_height && real_uvc->pframe[i].width == g_target_width && real_uvc->pframe[i].height == g_target_height) {
                            uvc->cur_frame = i + 1;
                            log_info("MJPEG select board cfg %dx%d frame %d", g_target_width, g_target_height, i + 1);
                        }
                        u32 f_interval = 0;
                        u32 f_interval_step = 0;
                        memcpy(&f_interval, pformat + 21, 4);
                        log_debug("default frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                        uvc->fps = (f_interval ? (10000000 / f_interval) : 0);
                        if (pformat[25] == 0) {
                            memcpy(&f_interval, pformat + 26, 4);
                            log_debug("min frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval, pformat + 30, 4);
                            log_debug("max frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval_step, pformat + 34, 4);
                            log_debug("frame interval step %d, fps step %d", f_interval_step,
                                      f_interval_step ? (10000000 / f_interval_step) : 0);
                        } else {
                            for (int icnt = 0; icnt < pformat[25]; icnt++) {
                                memcpy(&f_interval, pformat + 26 + 4 * icnt, 4);
                                log_debug("frame interval %d %d, fps %d", icnt, f_interval, 10000000 / f_interval);
                            }
                        }
                        wTotalLength -= pformat[0];
                        cur_len += pformat[0];
                        pformat += pformat[0];
                    }
                    log_info("[UVC] MJPEG selected %dx%d frame %d", real_uvc->pframe[uvc->cur_frame - 1].width, real_uvc->pframe[uvc->cur_frame - 1].height, uvc->cur_frame);
                    break;

                case UVC_VS_FORMAT_UNCOMPRESSED:
                    format_support = UVC_SUPPORT_YUYV_ENABLE;
                    pformat = (u8 *)(pBuf + len);
                    frame_num = pformat[4];
                    uvc->yuyv_format_index = pformat[3];
                    wTotalLength -= pformat[0];
                    uvc->format += UVC_VS_FORMAT_UNCOMPRESSED;
                    log_debug("find yuy2 frame descriptor %d %d %d",
                              frame_num,
                              pformat[3],
                              wTotalLength);
                    pformat += pformat[0];
                    uvc->reso_num = frame_num;
                    real_uvc = (struct usb_uvc *)malloc(sizeof(struct usb_uvc) + sizeof(struct uvc_frame_info) * (1 + frame_num));
                    memset(real_uvc, 0, sizeof(struct usb_uvc) + sizeof(struct uvc_frame_info) * (1 + frame_num));
                    for (int i = 0; i < frame_num; i++) {
                        /* u16 width, heigth; */
                        /* memcpy(&width, &pformat[5], 2); */
                        /* memcpy(&heigth, &pformat[7], 2); */
                        /* log_debug("frame info %d %d*%d", 1 + i, width, */
                        /* heigth); */
                        memcpy(&real_uvc->pframe[i].width, &pformat[5], 2);
                        memcpy(&real_uvc->pframe[i].height, &pformat[7], 2);
                        log_debug("frame info %d %d*%d", 1 + i, real_uvc->pframe[i].width,
                                  real_uvc->pframe[i].height);
                        if (g_target_width && g_target_height &&
                            real_uvc->pframe[i].width == g_target_width &&
                            real_uvc->pframe[i].height == g_target_height) {
                            uvc->cur_frame = i + 1;
                            log_info("YUV select board cfg %dx%d frame %d",
                                     g_target_width, g_target_height, i + 1);
                        }
                        if (!g_target_width || !g_target_height) {
                            if ((real_uvc->pframe[i].width == 1280) &&
                                (real_uvc->pframe[i].height == 720)) {
                                uvc->cur_frame = i + 1;
                                log_info("YUV select fallback 1280x720 frame %d", i + 1);
                            } else if ((real_uvc->pframe[i].width == 640) &&
                                       (real_uvc->pframe[i].height == 480)) {
                                uvc->cur_frame = i + 1;
                                log_info("YUV select fallback 640x480 frame %d", i + 1);
                            }
                        }
                        wTotalLength -= pformat[0];
                        cur_len += pformat[0];
                        pformat += pformat[0];
                    }
                    break;

                case UVC_VS_STILL_IMAGE_FRAME:
                    wTotalLength -= cur_len;
                    log_debug("find still image format descriptor %d", wTotalLength);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;
                case UVC_VS_FORMAT_FRAME_BASED:
                    format_support = UVC_SUPPORT_H264_ENABLE;
                    pformat = (u8 *)(pBuf + len);
                    frame_num = pformat[4];
                    uvc->h264_format_index = pformat[3];
                    wTotalLength -= pformat[0];
                    uvc->format += UVC_VS_FORMAT_FRAME_BASED;
                    /* Per-format ep/vs_itf_num saved at end of parser after all endpoints parsed */
                    log_debug("find format frame base descriptor %d %d %d",
                              frame_num,
                              pformat[3],

                              wTotalLength);
                    UVC_HEXDUMP(pformat, pformat[0]);
                    uvc->cur_frame = pformat[6];
                    log_debug("default frame id %d", uvc->cur_frame);
                    pformat += pformat[0];
                    uvc->reso_num = frame_num;
                    real_uvc = (struct usb_uvc *)zalloc(sizeof(struct usb_uvc) + sizeof(struct uvc_frame_info) * (1 + frame_num));
                    if (!real_uvc) {
                        log_error("err no mem in usb_uvc_parser\n\n");
                        len = -DEV_ERR_INVALID_BUF;
                        goto __err;
                    }
                    int h264_1080p_sel = 0;
                    for (int i = 0; i < frame_num; i++) {
                        memcpy(&real_uvc->pframe[i].width, &pformat[5], 2);
                        memcpy(&real_uvc->pframe[i].height, &pformat[7], 2);
                        log_debug("frame info %d %d x %d",
                                  1 + i,
                                  real_uvc->pframe[i].width,
                                  real_uvc->pframe[i].height);
                        UVC_HEXDUMP(pformat, pformat[0]);
                        if (g_target_width && g_target_height &&
                            real_uvc->pframe[i].width == g_target_width &&
                            real_uvc->pframe[i].height == g_target_height) {
                            uvc->cur_frame = i + 1;
                            h264_1080p_sel = 1;
                            log_debug("H264 select board cfg %dx%d frame %d",
                                      g_target_width, g_target_height, i + 1);
                        } else if (!h264_1080p_sel && (real_uvc->pframe[i].width == 640) &&
                                   (real_uvc->pframe[i].height == 480)) {
                            uvc->cur_frame = i + 1;
                            log_debug("H264 select 640x480 frame %d", i + 1);
                        } else if (!h264_1080p_sel && (real_uvc->pframe[i].width == 480) &&
                                   (real_uvc->pframe[i].height == 854)) {
                            uvc->cur_frame = i + 1;
                            log_debug("H264 select 480x854 frame %d", i + 1);
                        }
                        u32 f_interval = 0;
                        u32 f_interval_step = 0;
                        memcpy(&f_interval, pformat + 21, 4);
                        log_debug("default frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                        uvc->fps = (f_interval ? (10000000 / f_interval) : 0);
                        if (pformat[25] == 0) {
                            memcpy(&f_interval, pformat + 26, 4);
                            log_debug("min frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval, pformat + 30, 4);
                            log_debug("max frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval_step, pformat + 34, 4);
                            log_debug("frame interval step %d, fps step %d", f_interval_step,
                                      f_interval_step ? (10000000 / f_interval_step) : 0);
                        } else {
                            for (int icnt = 0; icnt < pformat[25]; icnt++) {
                                memcpy(&f_interval, pformat + 26 + 4 * icnt, 4);
                                log_debug("frame interval %d %d, fps %d", icnt, f_interval, 10000000 / f_interval);
                            }
                        }
                        wTotalLength -= pformat[0];
                        cur_len += pformat[0];
                        pformat += pformat[0];
                    }
                    break;

                case UVC_VS_COLORFORMAT:
                    wTotalLength -= cur_len;
                    log_debug("find colorformat descriptor %d", wTotalLength);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;

                default:
                    wTotalLength -= cur_len;
                    log_debug("unrecognized vs descriptor %d", wTotalLength);
                    UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
                    break;
                }
            }
        } else if (cur_type == USB_DT_CS_ENDPOINT) {
            if (cur_len != 5) {
                len = -USB_DT_CS_ENDPOINT;
                goto __err;
            }

            log_debug("USB_DT_CS_ENDPOINT");
            UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
        } else if (cur_type == USB_DT_ENDPOINT) {
            if (cur_len != USB_DT_ENDPOINT_SIZE) {
                len = -USB_DT_ENDPOINT;
                goto __err;
            }

            log_debug("USB_DT_ENDPOINT");
            UVC_HEXDUMP((u8 *)(pBuf + len), cur_len);
            memcpy(&end_desc, pBuf + len, USB_DT_ENDPOINT_SIZE);

            if (end_desc.bEndpointAddress & USB_DIR_IN) {
                log_debug("Endpoint IN %x", end_desc.bEndpointAddress);
            } else {
                log_debug("Endpoint OUT %x", end_desc.bEndpointAddress);
            }

            log_debug("wMaxPacketSize  = %d, %d additional transaction", end_desc.wMaxPacketSize & 0x7ff, end_desc.wMaxPacketSize >> 11);

            if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                log_debug(">>>>>XFER_BULK\n");
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
                log_debug(">>>>>XFER_INT = %d\n", end_desc.bInterval);
                int_ep = 1;
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
                log_debug(">>>>>XFER_ISOC = %d\n", end_desc.bInterval);
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_CONTROL) {
                log_debug(">>>>>XFER_CONTROL\n");
            }

            if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
                uvc->int_ep.epin = end_desc.bEndpointAddress & 0x0f;
                uvc->int_ep.interval = end_desc.bInterval;
                uvc->int_ep.wMaxPacketSize = end_desc.wMaxPacketSize;
                uvc->int_ep.enable = 1;

            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
                cur_vs_ep = end_desc.bEndpointAddress & 0x0f;
                /* 修改原因: 在解析端点描述符时,将当前端点地址保存到 cur_vs_ep,供后续 FORMAT 描述符解析时使用。原代码没有保存此信息。 */
                if (end_desc.wMaxPacketSize > wMaxPacketSize) {
                    wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->ep = end_desc.bEndpointAddress & 0x0f;
                    uvc->wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->interval = end_desc.bInterval;
                    uvc->if_alt_num = AlternateSetting;
                    uvc->xfer_type = USB_ENDPOINT_XFER_ISOC;
                }
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                cur_vs_ep = end_desc.bEndpointAddress & 0x0f;
                /* 修改原因: 同上,BULK 传输类型也需要保存端点信息。 */
                if (end_desc.wMaxPacketSize > wMaxPacketSize) {
                    wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->ep = end_desc.bEndpointAddress & 0x0f;
                    uvc->wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->interval = end_desc.bInterval;
                    uvc->if_alt_num = AlternateSetting;
                    uvc->xfer_type = USB_ENDPOINT_XFER_BULK;
                }
            }
            /*log_d("=======[%s - %d] >>>> \nif_alt_num : %d\ninterval : %d\nwMaxPacketSize : %d\n", __FUNCTION__,*/
            /*__LINE__, AlternateSetting, end_desc.bInterval, end_desc.wMaxPacketSize);*/
        } else {
            log_error("error %x", len);
            UVC_HEXDUMP((u8 *)(pBuf + len), 16);
            loop = 0;
            cur_len = 0;
        }
        len += cur_len;
    }

    log_debug("[UVC_DBG] parser done: uvc=%p real_uvc=%p", uvc, real_uvc);
    log_debug("[UVC_DBG] ep=%d vs_itf_num=%d wMaxPacketSize=%d xfer_type=%d format=0x%x",
              uvc->ep, uvc->vs_itf_num, uvc->wMaxPacketSize, uvc->xfer_type, uvc->format);
    log_debug("[UVC_DBG] host_ep=%d if_alt_num=%d interval=%d fps=%d cur_frame=%d",
              uvc->host_ep, uvc->if_alt_num, uvc->interval, uvc->fps, uvc->cur_frame);

    if (uvc->format == UVC_VS_FORMAT_MJPEG) {
        log_debug("[UVC_DBG] Using MJPEG endpoint: ep=%d vs_itf=%d", uvc->ep, uvc->vs_itf_num);
        uvc->mjpeg_ep = uvc->ep;
        uvc->mjpeg_vs_itf_num = uvc->vs_itf_num;
        uvc->mjpeg_if_alt_num = uvc->if_alt_num;
    } else if (uvc->format == UVC_VS_FORMAT_FRAME_BASED) {
        log_debug("[UVC_DBG] Using H264 endpoint: ep=%d vs_itf=%d", uvc->ep, uvc->vs_itf_num);
        uvc->h264_ep = uvc->ep;
        uvc->h264_vs_itf_num = uvc->vs_itf_num;
        uvc->h264_if_alt_num = uvc->if_alt_num;
    } else {
        uvc->mjpeg_if_alt_num = uvc->if_alt_num;
    }

    if (real_uvc) {
        memcpy(real_uvc, uvc, sizeof(*uvc));
    }
    free(uvc);
    uvc = NULL;
    struct usb_uvc *uvc_old = uvc_host_inf[usb_id].dev.uvc;
    if (real_uvc) {
        if (!format_support && uvc_old) {
            uvc_old->ep = real_uvc->ep;
            free(real_uvc);
        } else {
            if (uvc_old) {
                if (!real_uvc->mjpeg_ep && uvc_old->mjpeg_ep) {
                    real_uvc->mjpeg_ep = uvc_old->mjpeg_ep;
                    real_uvc->mjpeg_vs_itf_num = uvc_old->mjpeg_vs_itf_num;
                    real_uvc->mjpeg_if_alt_num = uvc_old->mjpeg_if_alt_num;
                }
                if (!real_uvc->h264_ep && uvc_old->h264_ep) {
                    real_uvc->h264_ep = uvc_old->h264_ep;
                    real_uvc->h264_vs_itf_num = uvc_old->h264_vs_itf_num;
                    real_uvc->h264_if_alt_num = uvc_old->h264_if_alt_num;
                }
                free(uvc_old);
            }
            uvc_host_inf[usb_id].dev.uvc = real_uvc;
        }
    }
    host_dev->interface_info[interface_num] = &uvc_host_inf[usb_id];
    hdl->dev.private_data = host_dev;
    hdl->name = usb_id ? "uvc1" : "uvc0";
    hdl->frame_cnt = 0;

    return len;

__err:
    log_error("error in usb_uvc_parser");
    if (real_uvc) {
        free(real_uvc);
        real_uvc = NULL;
    }
    if (uvc) {
        free(uvc);
        uvc = NULL;
    }
    return len;
}

#define     USB_GET_REQ		(USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE)
#define     USB_SET_REQ		(USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE)

int uvc_host_req_processing_unit(void *fd, struct uvc_processing_unit *pu)
{
    struct device *device = (struct device *)fd;
    int err = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);

    err = usb_control_msg(host_dev, pu->request, pu->type, pu->value, pu->index, (u8 *)pu->buf, pu->len);

    return err;
}

static u8 Video_Probe_Commit_Control[26] = {
    0x00, 0x00,                     //bmHint
    0x01,                           //FormatIndex
    0x01,                           //FrameIndex
    DW1BYTE(FRAME_FPS_0), DW2BYTE(FRAME_FPS_0), DW3BYTE(FRAME_FPS_0), DW4BYTE(FRAME_FPS_0), //dwFrameInterval Frame interval in 100 ns units.
    0x00, 0x00,                     //KeyFrameRate
    0x00, 0x00,                     //PFrameRate
    0x00, 0x27,                     //CompQuanlity
    0x00, 0x00,                     //CompWindowsSize
    0x00, 0x00, //0x32,0x00,                      //Delay
    DW1BYTE(300 * 1024), DW2BYTE(300 * 1024), DW3BYTE(300 * 1024), DW4BYTE(300 * 1024), //MaxVideoFrameSize
    LOBYTE(512), HIBYTE(512), 0x00, 0x00, //MaxPayLoadTransferSize
};

static u32 StreamInterfaceNum_get_cur(struct device *device, u16 id)
{
    u32 ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    struct usb_uvc *uvc = device_to_uvc(device);
    if (!uvc) {
        return -1;
    }
    id = cpu_to_be16(id);
    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, id, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    //log_debug("%s:%x\n",__FUNCTION__,id);
    check_ret(ret);
    return DEV_ERR_NONE;
}
static u32 StreamInterfaceNum_set_cur(struct device *device, u16 id)
{
    u32 ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    struct usb_uvc *uvc = device_to_uvc(device);
    if (!uvc) {
        return -1;
    }
    id = cpu_to_be16(id);
    u8 set_fmt_idx = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_format_index : uvc->mjpeg_format_index;
    Video_Probe_Commit_Control[2] = set_fmt_idx;
    Video_Probe_Commit_Control[3] = uvc->cur_frame;
    log_debug("%s:fmt_idx:%x cur_frame:%x\n", __func__, set_fmt_idx, uvc->cur_frame);
    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, id, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    //log_debug("%s:%x\n",__FUNCTION__,usb_id);
    check_ret(ret);
    return DEV_ERR_NONE;
}
static void Stream_head_analysis(u8 *rx_addr)
{
#if 0
    u32 frame_cnt = 0, scr = 0;
    u16 sof_count = 0;
    /// Stream Head
    rx_addr[0x00] = 0x0C;//Head Length
    rx_addr[0x01] = hdr->bfh_val; //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
    /* rx_addr[0x1] &= ~(BIT(2) | BIT(3) | BIT(5) | BIT(6)); */
    frame_cnt = hdr->frame_cnt;
    rx_addr[0x02] = frame_cnt ;//PTS[7:0]
    rx_addr[0x03] = (frame_cnt >> 8); //PTS[15:8]
    rx_addr[0x04] = (frame_cnt >> 16) ; //PTS[23:16]
    rx_addr[0x05] = (frame_cnt >> 24) ; //PTS[31:24]
    scr = hdr->scr;
    rx_addr[0x06] = DW1BYTE(scr);//SCR[7:0]
    rx_addr[0x07] = DW2BYTE(scr);//SCR[15:8]
    rx_addr[0x08] = DW3BYTE(scr);//SCR[23:16]
    rx_addr[0x09] = DW4BYTE(scr);//SCR[31:24]
    sof_count = hdr->sof_cnt;
    rx_addr[0x0A] = LOBYTE(sof_count); //SCR[42:32] 1KHz SOF token counter
    rx_addr[0x0B] = HIBYTE(sof_count); //SCR[47:42] res set to 0.
#endif
}

static struct uvc_stream_list uvc_global_l[USB_MAX_HW_NUM][ISO_INTR_CNT];

static void vs_iso_handler(struct usb_host_device *host_dev, u32 ep)
{
    usb_dev usb_id;
    int rx_len;
    u8 *start_addr;
    u8 error_flag = 0;
    /* u8 stream_state = 0; */
    u32 maxpsize;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    struct device *device;
    struct usb_uvc *uvc;
    if (!host_dev) {
        return;
    }
    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    //fix 连续拔插
    if (!hdl->open) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    rx_len = usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, hdl->buffer, maxpsize, USB_ENDPOINT_XFER_ISOC, 0);
    if (rx_len < 0) {
        uvc->error_frame = 1;
        {
            static int rxn = 0;
            if (rxn < 3) {
                log_error("[RX_NEG] rx_len<0 set error_frame=1\n");
                rxn++;
            }
        }
        if (uvc->stream_out) {
            g_bulk_stream_errs++;
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    } else {
        trans_err_cnt[usb_id] = 0;
    }
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
    start_addr = hdl->buffer;
    struct uvc_stream_list *uvc_list = uvc_global_l[usb_id];
    if (rx_len > 0 && rx_len > start_addr[0]) {
        rx_len -= start_addr[0];
        error_flag = start_addr[1];
        /* if (!uvc->error_frame) { */
        uvc_list->addr = start_addr + start_addr[0];
        uvc_list->length = rx_len;
        /* } */
        if (start_addr[0] != 0x0c && start_addr[0] != 0x02) {
            log_debug("start_addr = %x", start_addr[0]);
            uvc->error_frame = 1;
            if (uvc->stream_out) {
                g_bulk_stream_errs++;
                uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
            }
        }
        //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
        if ((error_flag & BIT(6)) && uvc->error_frame == 0) {
            g_bulk_stream_errs++;
        }
        if (error_flag & BIT(1)) { //endof frame
            if (!uvc->error_frame) {
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_EOF);
                    /* stream_state = STREAM_EOF; */
                    hdl->frame_cnt++;
                }
            }
            uvc->error_frame = 0;//clear error_frame pending
        } else {
            if (!uvc->error_frame) {
                if (uvc->bfh != (error_flag & BIT(0))) { //new frame
                    uvc->bfh = error_flag & BIT(0);//clear error_frame pending
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                        /* stream_state = STREAM_SOF; */
                    }
                } else {
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                        /* stream_state = STREAM_NO_ERR; */
                    }
                }
            }
        }
    }

    hdl->in_irq = 0;
}
static void vs_iso_burst_handler(struct usb_host_device *host_dev, u32 ep)
{
#if ISO_INTR_MODE == 2

    usb_dev usb_id;
    int rx_len;
    int total_rx_len;
    u8 *start_addr;
    u8 error_flag = 0;
    /* u8 stream_state = 0; */
    u32 maxpsize;
    u32 rlen_table[8];
    u32 uvc_pos = 0;
    u32 valid_cnt = 0;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    struct device *device;
    struct usb_uvc *uvc;

    if (!host_dev) {
        return;
    }
    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    total_rx_len = usb_h_ep_burst_read_async(usb_id, uvc->host_ep, uvc->ep, ISO_INTR_CNT, rlen_table, hdl->buffer, maxpsize * ISO_INTR_CNT, USB_ENDPOINT_XFER_ISOC, 0);
    if (total_rx_len < 0) {
        uvc->error_frame = 1;
        if (uvc->stream_out) {
            g_bulk_stream_errs++;
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    } else {
        trans_err_cnt[usb_id] = 0;
        /* printf("total_len %d\n", total_rx_len); */
        /* printf_buf(hdl->buffer, total_rx_len); */
    }
    //触发下一次接收
    usb_h_ep_burst_read_async(usb_id, uvc->host_ep, uvc->ep, 0, NULL, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
    start_addr = hdl->buffer;
    struct uvc_stream_list *uvc_list = uvc_global_l[usb_id];
    if (total_rx_len > 0) {
        for (int i = 0; i < ISO_INTR_CNT; i++) {
            rx_len = rlen_table[i];
            /* printf("r %d\n", rx_len); */
            /* printf_buf(start_addr, rx_len); */
            if (rx_len > start_addr[0]) {
                rx_len -= start_addr[0];
            } else {
                /* start_addr += rx_len; */
                start_addr += (rx_len + 3) / 4 * 4;
                continue;
            }
            error_flag = start_addr[1];
            if (!uvc->error_frame) {
                uvc_list[uvc_pos + valid_cnt].addr = start_addr + start_addr[0];
                uvc_list[uvc_pos + valid_cnt].length = rx_len;
                valid_cnt++;
            }
            if (start_addr[0] != 0x0c && start_addr[0] != 0x02) {
                log_debug("start_addr = %x", start_addr[0]);
                uvc->error_frame = 1;
                valid_cnt = 0;
                if (uvc->stream_out) {
                    g_bulk_stream_errs++;
                    uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                }
            }
            //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
            if ((error_flag & BIT(6)) && uvc->error_frame == 0) {
                g_bulk_stream_errs++;
            }
            if ((error_flag & BIT(1)) && !(uvc->format & UVC_VS_FORMAT_FRAME_BASED)) { //endof frame
                if (!uvc->error_frame) {
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, valid_cnt, &uvc_list[uvc_pos], STREAM_EOF);
                        /* stream_state = STREAM_EOF; */
                        hdl->frame_cnt++;
                        uvc_pos += valid_cnt;
                    }

                }
                valid_cnt = 0;
                uvc->error_frame = 0;//clear error_frame pending
            }
            if (uvc->bfh != (error_flag & BIT(0))) { //new frame
                uvc->bfh = error_flag & BIT(0);//clear error_frame pending
                if (!uvc->error_frame) {
                    uvc_pos += (valid_cnt - 1);
                    valid_cnt = 1;
                } else {
                    uvc_pos += valid_cnt;
                    valid_cnt = 0;
                }
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, -1, NULL, STREAM_SOF);
                    /* stream_state = STREAM_SOF; */
                }
                uvc->error_frame = 0;//clear error_frame pending
            }

            start_addr += rx_len + start_addr[0];
        }
        if (!uvc->error_frame && valid_cnt) {
            if (uvc->stream_out) {
                uvc->stream_out(uvc->priv, valid_cnt, &uvc_list[uvc_pos], STREAM_NO_ERR);
                /* stream_state = STREAM_NO_ERR; */
            }
        }
    }
    hdl->in_irq = 0;
#endif
}
static void vs_bulk_handler(struct usb_host_device *host_dev, u32 ep)
{
    log_verb("[UVC_DBG] bulk_irq triggered ep=%d", ep);
    usb_dev usb_id;
    u8 _header;
    int rx_len;
    u8 *start_addr;
    u8 error_flag = 0;
    /* u8 stream_state = 0; */
    u8 *last_buf;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    static u32 last_len[USB_MAX_HW_NUM];
    static u32 frame_len[USB_MAX_HW_NUM];
    static u32 bulk_rx_bytes[USB_MAX_HW_NUM] = {0};
    static u8 mjpeg_active1[USB_MAX_HW_NUM] = {0};
    u32 maxpsize;
    struct device *device;
    struct usb_uvc *uvc;
    if (!host_dev) {
        return;
    }
    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    rx_len = usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, hdl->buffer, maxpsize * UVC_BULK_READ_MULT, USB_ENDPOINT_XFER_BULK, 0);//异步读取当前数据包
    if (rx_len > 0) {
        memcpy(hdl->save_buf, hdl->buffer, rx_len);
    }
    start_addr = hdl->save_buf;
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
    {
        /* update global stats */
        if (rx_len > 0) {
            g_bulk_rx_pkts++;
            g_bulk_rx_bytes += rx_len;
            if (rx_len < g_bulk_rx_min) {
                g_bulk_rx_min = rx_len;
            }
            if (rx_len > g_bulk_rx_max) {
                g_bulk_rx_max = rx_len;
            }
        } else if (rx_len < 0) {
            g_bulk_rx_errs++;
        }
    }
    if (rx_len < 0) {
        uvc->error_frame = 1;
        {
            static int rxn = 0;
            if (rxn < 3) {
                log_error("[RX_NEG] rx_len<0 set error_frame=1\n");
                rxn++;
            }
        }
        if (uvc->stream_out) {
            g_bulk_stream_errs++;
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    } else {
        trans_err_cnt[usb_id] = 0;
    }

    struct uvc_stream_list *uvc_list = uvc_global_l[usb_id];
    if (rx_len > 0) {
        _header = 0;
        last_buf = &hdl->buffer[maxpsize];
        if (rx_len == 0 || rx_len > maxpsize) {
            static int rx0_cnt = 0;
            if (rx_len == 0 && rx0_cnt < 5) {
                // printf("[RX0] rx_len=0 early return #%d (maxpsize=%d)\n", rx0_cnt, maxpsize);
                rx0_cnt++;
            }
            hdl->in_irq = 0;
            return;
        }
        if ((uvc->format & UVC_VS_FORMAT_FRAME_BASED) && rx_len <= 12) {
            int all_zero = 1;
            for (int zi = 0; zi < rx_len; zi++) {
                if (start_addr[zi]) {
                    all_zero = 0;
                    break;
                }
            }
            if (all_zero) {
                hdl->in_irq = 0;
                return;
            }
        }
        if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
            if (start_addr[0] == 0x0c) {
                if ((start_addr[1] & 0x80) == 0x80) {
                    if (start_addr[1] == 0x8f && rx_len <= start_addr[0]) {
                        hdl->in_irq = 0;
                        return;
                    }
                    if ((start_addr[0x0c] == 0x00) &&
                        (start_addr[0x0d] == 0x00) &&
                        (start_addr[0x0e] == 0x00) &&
                        (start_addr[0x0f] == 0x01)) {
                        _header = 1;
                    }
                }
            } else if (start_addr[0] == 0x02) {
                if ((start_addr[1] & 0x80) == 0x80) {
                    if ((start_addr[0x02] == 0x00) &&
                        (start_addr[0x03] == 0x00) &&
                        (start_addr[0x04] == 0x00) &&
                        (start_addr[0x05] == 0x01)) {
                        _header = 1;
                    }
                }
            }
        } else {
            if (start_addr[0] == 0x0c) {
                if ((start_addr[1] & 0x8c) == 0x8c || (start_addr[1] & 0x8c) == 0x80) {
                    if (start_addr[1] == 0x8f && rx_len <= 0x0c) {
                        hdl->in_irq = 0;
                        return;

                    }
                    _header = 1;
                    if (start_addr[0x0c] == 0xff && start_addr[0x0d] == 0xd8) {
                        _header = 1;
                    }
                    if (*(u32 *)(&start_addr[0x0c]) == 0x1000000) {
                        _header = 1;
                    }
                } else if (((start_addr[1] & 0x0c) == 0x0c) || ((start_addr[1] & 0x0c) == 0x08)) {
                    /* Sony-style header: BIT(3) or BIT(2) set, BIT(7) clear.
                     * For EOF-only pkt (12B): accept directly.
                     * For data pkt (>12B): confirm with SOI at offset 12. */
                    if (rx_len == 0x0c || (rx_len > 0x0c && start_addr[0x0c] == 0xff && start_addr[0x0d] == 0xd8)) {
                        _header = 1;
                        if (rx_len > 0x0c) {
                            mjpeg_active1[usb_id] = 1;
                        }
                    } else if (mjpeg_active1[usb_id]) {
                        _header = 1;
                    }
                } else if (start_addr[1] == 0x02 || start_addr[1] == 0x03) {
                    if (*(u32 *)(&start_addr[0x0c]) == 0x1000000) {
                        _header = 1;
                    } else {
                        _header = 1; /* BIT(1) EOF header, no H264 start code */
                        {
                            static int hh_cnt = 0;
                            if (hh_cnt < 5) {
                                // printf("[HDR_FIX] _header\n");
                                hh_cnt++;
                            }
                        }
                    }
                } else {
                    /* BULK: 12-byte header with unrecognized bmHeaderInfo.
                     * In BULK mode every packet starts with a UVC header,
                     * so accept it unconditionally. */
                    _header = 1;
                    if (rx_len > 0x0c && start_addr[0x0c] == 0xff && start_addr[0x0d] == 0xd8) {
                        mjpeg_active1[usb_id] = 1;
                    }
                }
            } else if (start_addr[0] == 0x02) {
                /* BULK 2-byte UVC header: bHeaderLength=2 means this IS a header.
                 * Don't require EOH(bit7) - many devices don't set it. */
                _header = 1;
                if (rx_len > 2 && start_addr[0x02] == 0xff && start_addr[0x03] == 0xd8) {
                    mjpeg_active1[usb_id] = 1;
                }
            } else {
                /* BULK: unknown bHeaderLength - if in MJPEG stream, treat as header. */
                if (mjpeg_active1[usb_id]) {
                    start_addr[0] = 0;
                    _header = 1;
                }
            }
        }
        if (_header) {
            if (rx_len >= start_addr[0]) {
                rx_len -= start_addr[0];
            }
            uvc_list->addr = start_addr + start_addr[0];
            uvc_list->length = rx_len;
            error_flag = start_addr[1];
            if ((error_flag & BIT(6)) && (uvc->error_frame == 0)) {
                /* For MJPEG BULK: many cameras set ERR(bit6) unreliably.
                 * Ignore it to avoid dropping valid frames. */
                if (!mjpeg_active1[usb_id]) {
                    {
                        static int b6_cnt = 0;
                        if (b6_cnt < 10) {
                            // printf("[STATE] BIT6 ERR ef=%02x len=%d fcnt=%d\n", error_flag, rx_len, hdl->frame_cnt);
                            b6_cnt++;
                        }
                    }
                    uvc->error_frame = 1;
                    frame_len[usb_id] = 0;
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                    }
                    g_bulk_stream_errs++;
                }
            }
            if ((error_flag & BIT(1)) && !(uvc->format & UVC_VS_FORMAT_FRAME_BASED)) {
                if (mjpeg_active1[usb_id] && (error_flag & BIT(6))) {
                    /* ERR+EOF: segment boundary, not frame end. Keep accumulating. */
                    if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                    }
                    bulk_rx_bytes[usb_id] += rx_len;
                    frame_len[usb_id] += rx_len;
                    // {static u32 seg1=0;if(seg1<200){printf("[SEG_END1] ef=%02x flen=%d\r\n",error_flag,frame_len[usb_id]);seg1++;}}
                } else {
                    /* EOF without ERR (or non-MJPEG): real frame end. */
                    if (!uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_EOF);
                            hdl->frame_cnt++;
                        }
                    }
                    {
                        static int eof_dbg = 0;
                        if (eof_dbg < 20) {
                            // printf("[EOF_DBG] BULK EOF ef=%02x len=%d frame_cnt=%d\n", error_flag, rx_len, hdl->frame_cnt);
                            eof_dbg++;
                        }
                    }
                    {
                        // printf("[EOF_SUM] fr#%d pkts=%d hdr_ok=%d hdr_bad=%d\r\n",
                    }
                    uvc->error_frame = 0;
                    frame_len[usb_id] = 0;
                    mjpeg_active1[usb_id] = 0;
                }
            } else if (uvc->bfh != (error_flag & BIT(0))) {
                uvc->bfh = error_flag & BIT(0);
                u8 orig_hdr_len = start_addr[0];
                if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
                    if (!uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                            hdl->frame_cnt++;
                        }
                    }
                    uvc->error_frame = 0;
                    if (uvc_list->length) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                        }
                        bulk_rx_bytes[usb_id] += rx_len;
                        frame_len[usb_id] = rx_len;
                    }
                    {
                        static int fid1 = 0;
                        if (fid1 < 10) {
                            // printf("[STATE] FID-CHG ef=%02x rx_len=%d fcnt=%d\n", error_flag, rx_len, hdl->frame_cnt);
                            fid1++;
                        }
                    }
                } else {
                    if ((last_len[usb_id] >= 2 &&
                         last_buf[last_len[usb_id] - 2] == 0xff &&
                         last_buf[last_len[usb_id] - 1] == 0xd9) ||
                        (last_len[usb_id] == 1 && last_buf[last_len[usb_id] - 1] == 0xd9) ||
                        (last_len[usb_id] >= 4 && last_buf[last_len[usb_id] - 4] == 0x55 && last_buf[last_len[usb_id] - 3] == 0xAA && last_buf[last_len[usb_id] - 2] == 0x66 && last_buf[last_len[usb_id] - 1] == 0xCC)) {
                        if (!uvc->error_frame) {
                            if (uvc->stream_out) {
                                uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                                hdl->frame_cnt++;
                            }
                        }
                        if (uvc_list->length) {
                            if (uvc->stream_out) {
                                uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            }
                            uvc->error_frame = 0;
                            bulk_rx_bytes[usb_id] += rx_len;
                            frame_len[usb_id] = rx_len;
                        }
                    } else if (last_len[usb_id] >= 2) {
                        if (!uvc->error_frame) {
                            if (uvc->stream_out) {
                                uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                                hdl->frame_cnt++;
                            }
                        }
                        if (uvc_list->length) {
                            {
                                static int sf_cnt = 0;
                                if (sf_cnt < 10) {
                                    // printf("[STATE] SOF ef=%02x len=%d fcnt=%d\n", error_flag, rx_len, hdl->frame_cnt);
                                    sf_cnt++;
                                }
                            }
                            if (uvc->stream_out) {
                                uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            }
                            uvc->error_frame = 0;
                            bulk_rx_bytes[usb_id] += rx_len;
                            frame_len[usb_id] = rx_len;
                        }
                    } else {
                        if (last_len[usb_id] == 0 && uvc_list->length) {
                            if (uvc->stream_out) {
                                uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            }
                            uvc->error_frame = 0;
                            bulk_rx_bytes[usb_id] += rx_len;
                            frame_len[usb_id] = rx_len;
                        } else {
                            uvc->error_frame = 1;
                            if (uvc->stream_out) {
                                g_bulk_stream_errs++;
                                uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                            }
                            frame_len[usb_id] = 0;
                        }
                    }
                }
            } else if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
                {
                    static int nc_cnt = 0;
                    if (nc_cnt < 10) {
                        // printf("[STATE] NOCHG ef=%02x len=%d fcnt=%d\n", error_flag, rx_len, hdl->frame_cnt);
                        nc_cnt++;
                    }
                }
                if (!uvc->error_frame && uvc_list->length) {
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                    }
                    bulk_rx_bytes[usb_id] += rx_len;
                    frame_len[usb_id] += rx_len;
                }
            } else {
                if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                    uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                }
                bulk_rx_bytes[usb_id] += rx_len;
                frame_len[usb_id] += rx_len;
            }
            last_len[usb_id] = rx_len;
            memcpy(last_buf, start_addr + start_addr[0], rx_len);
        } else {
            uvc_list->addr = start_addr;
            uvc_list->length = rx_len;
            /* Non-header H264 packets are pure payload - no UVC header to strip. */

            if (!uvc->error_frame) {
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                }
                bulk_rx_bytes[usb_id] += rx_len;
                frame_len[usb_id] += rx_len;
            } else if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
                g_error_frame_pay_drops++;
                {
                    static int pd = 0;
                    if (pd < 10) {
                        pd++;
                        log_error("[PAY_DROP] err_frame=1 drop len=%d\n", rx_len);
                    }
                }
            }
            last_len[usb_id] = rx_len;
            memcpy(last_buf, start_addr, rx_len);
        }
    }
    hdl->in_irq = 0;
}

/**
 * @brief       uvc bulk 第2路接收
 *              usb0 bulk将走这个函数,usb1上的第2路uvc设备接收也会走这个函数
 *
 * @param:      host_dev: usb句柄
 * @param:      ep: 接收端点
 *
 * @return:     null
 **/
static void vs_bulk_handler2(struct usb_host_device *host_dev, u32 ep)
{
    /* log_debug("[UVC_DBG] bulk_irq2 triggered ep=%d", ep); */

    usb_dev usb_id;
    u8 _header;
    int rx_len;
    u8 *start_addr;
    u8 error_flag = 0;
    /* u8 stream_state = 0; */
    u8 *last_buf;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    static u32 last_len[USB_MAX_HW_NUM];
    static u32 frame_len[USB_MAX_HW_NUM];
    static u32 frame_pkt_cnt[USB_MAX_HW_NUM] = {0};
    static u32 bulk_rx_bytes2[USB_MAX_HW_NUM] = {0};
    static u8 mjpeg_active2[USB_MAX_HW_NUM] = {0};
    static u8 frame_done2[USB_MAX_HW_NUM] = {0};
    static u8 prev_ff2[USB_MAX_HW_NUM] = {0};
    static u8 prev_was_short[USB_MAX_HW_NUM] = {1};

    u32 maxpsize;
    struct device *device;
    struct usb_uvc *uvc;
    if (!host_dev) {
        return;
    }
    /* putchar('2'); */

    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = uvc_host_inf[usb_id].dev.uvc;//device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    rx_len = usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, hdl->buffer, maxpsize * UVC_BULK_READ_MULT, USB_ENDPOINT_XFER_BULK, 0);
    {
        /* update global stats */
        if (rx_len > 0) {
            g_bulk_rx_pkts++;
            g_bulk_rx_bytes += rx_len;
            if (rx_len < g_bulk_rx_min) {
                g_bulk_rx_min = rx_len;
            }
            if (rx_len > g_bulk_rx_max) {
                g_bulk_rx_max = rx_len;
            }
        } else if (rx_len < 0) {
            g_bulk_rx_errs++;
        }
    }
    if (rx_len < 0) {
        uvc->error_frame = 1;
        {
            static int rxn = 0;
            if (rxn < 3) {
                log_error("[RX_NEG] rx_len<0 set error_frame=1\n");
                rxn++;
            }
        }
        if (uvc->stream_out) {
            g_bulk_stream_errs++;
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    } else {
        trans_err_cnt[usb_id] = 0;
    }

    if (rx_len > 0) {
        memcpy(hdl->save_buf, hdl->buffer, rx_len);
    }
    start_addr = hdl->save_buf;
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
    struct uvc_stream_list *uvc_list = uvc_global_l[0/*usb_id*/];
    if (rx_len > 0) {
        _header = 0;
        last_buf = &hdl->buffer[maxpsize];
        if (rx_len == 0 || rx_len > maxpsize) {
            goto __bulk2_exit;
        }
        if ((uvc->format & UVC_VS_FORMAT_FRAME_BASED) && rx_len <= 12) {
            int all_zero = 1;
            for (int zi = 0; zi < rx_len; zi++) {
                if (start_addr[zi]) {
                    all_zero = 0;
                    break;
                }
            }
            if (all_zero) {
                goto __bulk2_exit;
            }
        }

        u8 orig_hdr_len = start_addr[0];
        if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
            if (start_addr[0] == 0x0c) {
                if ((start_addr[1] & 0x80) == 0x80) {
                    if (start_addr[1] == 0x8f && rx_len <= start_addr[0]) {
                        goto __bulk2_exit;
                    }
                    if ((start_addr[0x0c] == 0x00) &&
                        (start_addr[0x0d] == 0x00) &&
                        (start_addr[0x0e] == 0x00) &&
                        (start_addr[0x0f] == 0x01)) {
                        _header = 1;
                    }
                }
            } else if (start_addr[0] == 0x02) {
                if ((start_addr[1] & 0x80) == 0x80) {
                    if ((start_addr[0x02] == 0x00) &&
                        (start_addr[0x03] == 0x00) &&
                        (start_addr[0x04] == 0x00) &&
                        (start_addr[0x05] == 0x01)) {
                        _header = 1;
                    }
                }
            }
        } else {
            if (mjpeg_active2[usb_id] && !prev_was_short[usb_id] && rx_len == maxpsize) {
                _header = 0;
            } else if (start_addr[0] == 0x0c) {
                if ((start_addr[1] & 0x8c) == 0x8c || (start_addr[1] & 0x8c) == 0x80) {
                    if (start_addr[1] == 0x8f && rx_len <= 0x0c) {
                        goto __bulk2_exit;
                    }
                    _header = 1;
                } else if (((start_addr[1] & 0x0c) == 0x0c) || ((start_addr[1] & 0x0c) == 0x08)) {
                    u8 was_mjpeg_active = mjpeg_active2[usb_id];
                    if (!mjpeg_active2[usb_id] && !frame_done2[usb_id]) {
                        mjpeg_active2[usb_id] = 1;
                    }
                    if (rx_len == 0x0c || (rx_len > 0x0c && start_addr[0x0c] == 0xff && start_addr[0x0d] == 0xd8)) {
                        _header = 1;
                        if (rx_len > 0x0c) {
                            if (was_mjpeg_active) {
                                start_addr[0] += 2;
                            }
                        }
                    } else {
                        _header = 1;
                    }
                } else if (start_addr[1] == 0x02 || start_addr[1] == 0x03) {
                    _header = 1;
                } else {
                    _header = 1;
                }
            } else if (start_addr[0] == 0x02) {
                _header = 1;
            } else {
                if (mjpeg_active2[usb_id]) {
                    _header = 0;
                } else {
                    start_addr[0] = 0x0c;
                    _header = 1;
                }
            }
        }
        if (_header) {
            if (rx_len >= start_addr[0]) {
                rx_len -= start_addr[0];
            }
            uvc_list->addr = start_addr + start_addr[0];
            uvc_list->length = rx_len;
            error_flag = start_addr[1];

            if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
                /* ---- H264: FID-based frame boundary ---- */
                if ((error_flag & BIT(6)) && (uvc->error_frame == 0)) {
                    uvc->error_frame = 1;
                    frame_len[usb_id] = 0;
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                    }
                    g_bulk_stream_errs++;
                }
                if (uvc->bfh != (error_flag & BIT(0))) {
                    uvc->bfh = error_flag & BIT(0);
                    if (!uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                            hdl->frame_cnt++;
                        }
                    }
                    uvc->error_frame = 0;
                    if (uvc_list->length) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                        }
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] = rx_len;
                    }
                } else {
                    if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                    }
                    bulk_rx_bytes2[usb_id] += rx_len;
                    frame_len[usb_id] += rx_len;
                }
                last_len[usb_id] = rx_len;
                memcpy(last_buf, start_addr + start_addr[0], rx_len);
            } else {
                /* ---- MJPEG: SOI/EOI-based frame boundary ---- */
                {
                    u8 *payload = (u8 *)uvc_list->addr;
                    u8 has_soi = (rx_len >= 2 && payload[0] == 0xff && payload[1] == 0xd8);
                    u32 soi_pos = 0;
                    if (!has_soi && frame_done2[usb_id] && rx_len >= 2) {
                        for (int _k = 0; _k < rx_len - 1; _k++) {
                            if (payload[_k] == 0xff && payload[_k + 1] == 0xd8) {
                                has_soi = 1;
                                soi_pos = _k;
                                break;
                            }
                        }
                    }
                    u8 has_eoi = 0;
                    if (mjpeg_active2[usb_id] && (error_flag & BIT(1))) {
                        has_eoi = 1;
                    }
                    if (rx_len >= 2 && !has_eoi) {
                        if (payload[rx_len - 2] == 0xff && payload[rx_len - 1] == 0xd9) {
                            has_eoi = 1;
                        }
                    }
                    if (!has_eoi && prev_ff2[usb_id] && rx_len >= 1 && payload[0] == 0xd9) {
                        has_eoi = 1;
                    }
                    prev_ff2[usb_id] = (rx_len >= 1 && payload[rx_len - 1] == 0xff) ? 1 : 0;
                    prev_was_short[usb_id] = (rx_len < (maxpsize - start_addr[0])) ? 1 : 0;

                    if (has_eoi && mjpeg_active2[usb_id] && !uvc->error_frame) {
                        if (uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                        }
                        frame_pkt_cnt[usb_id]++;
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                            hdl->frame_cnt++;
                        }
                        mjpeg_active2[usb_id] = 0;
                        frame_done2[usb_id] = 1;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_pkt_cnt[usb_id] = 0;
                        frame_len[usb_id] = 0;
                    } else if (frame_done2[usb_id] && has_soi) {
                        u32 adj_len = rx_len - soi_pos;
                        uvc_list->addr = payload + soi_pos;
                        uvc_list->length = adj_len;
                        if (!uvc->error_frame && adj_len > 0 && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            frame_pkt_cnt[usb_id]++;
                        }
                        mjpeg_active2[usb_id] = 1;
                        frame_done2[usb_id] = 0;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += adj_len;
                        frame_len[usb_id] = adj_len;
                    } else if (!mjpeg_active2[usb_id] && !frame_done2[usb_id] && has_soi) {
                        if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            frame_pkt_cnt[usb_id]++;
                        }
                        mjpeg_active2[usb_id] = 1;
                        frame_done2[usb_id] = 0;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] = rx_len;
                    } else if (mjpeg_active2[usb_id]) {
                        if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                            frame_pkt_cnt[usb_id]++;
                        }
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] += rx_len;
                    } else if (frame_done2[usb_id]) {
                        /* Between frames: discard data until next SOI */
                    } else {
                        if (uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            frame_pkt_cnt[usb_id]++;
                        }
                        mjpeg_active2[usb_id] = 1;
                        frame_done2[usb_id] = 0;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] = rx_len;
                    }
                }
                last_len[usb_id] = rx_len;
                memcpy(last_buf, start_addr + start_addr[0], rx_len);
            }
        } else {
            uvc_list->addr = start_addr;
            uvc_list->length = rx_len;

            if (uvc->format & UVC_VS_FORMAT_FRAME_BASED) {
                /* ---- H264: non-header payload ---- */
                if (!uvc->error_frame) {
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                    }
                    bulk_rx_bytes2[usb_id] += rx_len;
                    frame_len[usb_id] += rx_len;
                } else {
                    g_error_frame_pay_drops++;
                }
                last_len[usb_id] = rx_len;
                memcpy(last_buf, start_addr, rx_len);
            } else {
                /* ---- MJPEG: non-header SOI/EOI ---- */
                {
                    u8 *payload = start_addr;
                    u8 has_soi = (rx_len >= 2 && payload[0] == 0xff && payload[1] == 0xd8);
                    u32 soi_pos = 0;
                    if (!has_soi && frame_done2[usb_id] && rx_len >= 2) {
                        for (int _k = 0; _k < rx_len - 1; _k++) {
                            if (payload[_k] == 0xff && payload[_k + 1] == 0xd8) {
                                has_soi = 1;
                                soi_pos = _k;
                                break;
                            }
                        }
                    }
                    u8 has_eoi = 0;
                    if (rx_len >= 2 && !has_eoi) {
                        if (payload[rx_len - 2] == 0xff && payload[rx_len - 1] == 0xd9) {
                            has_eoi = 1;
                        }
                    }
                    if (!has_eoi && prev_ff2[usb_id] && rx_len >= 1 && payload[0] == 0xd9) {
                        has_eoi = 1;
                    }
                    prev_ff2[usb_id] = (rx_len >= 1 && payload[rx_len - 1] == 0xff) ? 1 : 0;
                    prev_was_short[usb_id] = (rx_len < maxpsize) ? 1 : 0;

                    if (has_eoi && mjpeg_active2[usb_id] && !uvc->error_frame) {
                        if (uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                        }
                        frame_pkt_cnt[usb_id]++;
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                            hdl->frame_cnt++;
                        }
                        mjpeg_active2[usb_id] = 0;
                        frame_done2[usb_id] = 1;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_pkt_cnt[usb_id] = 0;
                        frame_len[usb_id] = 0;
                    } else if (frame_done2[usb_id] && has_soi) {
                        u32 adj_len = rx_len - soi_pos;
                        uvc_list->addr = payload + soi_pos;
                        uvc_list->length = adj_len;
                        if (!uvc->error_frame && adj_len > 0 && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            frame_pkt_cnt[usb_id]++;
                        }
                        mjpeg_active2[usb_id] = 1;
                        frame_done2[usb_id] = 0;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += adj_len;
                        frame_len[usb_id] = adj_len;
                    } else if (!mjpeg_active2[usb_id] && !frame_done2[usb_id] && has_soi) {
                        if (!uvc->error_frame && uvc_list->length && uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            frame_pkt_cnt[usb_id]++;
                        }
                        mjpeg_active2[usb_id] = 1;
                        frame_done2[usb_id] = 0;
                        uvc->error_frame = 0;
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] = rx_len;
                    } else if (mjpeg_active2[usb_id] && !uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                            frame_pkt_cnt[usb_id]++;
                        }
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] += rx_len;
                    } else if (frame_done2[usb_id]) {
                        /* Between frames: discard data until next SOI */
                    } else if (!uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                            frame_pkt_cnt[usb_id]++;
                        }
                        bulk_rx_bytes2[usb_id] += rx_len;
                        frame_len[usb_id] += rx_len;
                    }
                }
                last_len[usb_id] = rx_len;
                memcpy(last_buf, start_addr, rx_len);
            }
        }
    }
__bulk2_exit:
    hdl->in_irq = 0;
}


static void usb_intr_config(struct device *device, u8 ep, u8 enable)
{
    u8 id = usbpriv_to_usbid(usbdev_to_usbpriv(device_to_usbdev(device)));
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (enable) {
        usb_set_intr_rxe(id, ep);
    } else {
        usb_h_set_ep_isr(host_dev, ep, NULL, NULL);
        usb_clr_intr_rxe(id, ep);
        usb_write_rxcsr(id, ep, RXCSRH_ClrDataTog | RXCSRH_FlushFIFO);
        usb_write_rxinterval(id, ep, 0);
        /* usb_write_fifosize(id, ep, 0); */  //默认都没开
    }
}
static void iso_ep_rx_init(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);
    u8 devnum = host_dev->private_data.devnum;

    usb_write_rxfuncaddr(usb_id, uvc->host_ep, devnum);
    if (usb_id == 0) {
        usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, vs_iso_handler, host_dev);
        usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);
        usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
    } else if (usb_id == 1) {
#if ISO_INTR_MODE == 2
        usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, vs_iso_burst_handler, host_dev);
        usb_write_fifosize(usb_id, uvc->host_ep, BIT(5) | BIT(4) | BIT(3) | (ISO_INTR_CNT - 1));
        usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);
        usb_h_ep_burst_read_async(usb_id, uvc->host_ep, uvc->ep, 0, NULL, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
#else
        usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, vs_iso_handler, host_dev);
        usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);
        usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);

#endif
    }
}
static void bulk_ep_rx_init(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    void *bulk_rx_handler = NULL;
    usb_dev usb_id = usbdev_to_usbid(device);
    if (usb_id == 1) {
        bulk_rx_handler = vs_bulk_handler;
    } else {
        bulk_rx_handler = vs_bulk_handler2;
    }
    struct usb_uvc *uvc = device_to_uvc(device);

    u8 devnum = host_dev->private_data.devnum;
    usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));

    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    log_debug("[UVC_DBG] bulk_ep_rx_init: uvc=%p ep=%d host_ep=%d vs_itf_num=%d maxpsize=%d",
              uvc, uvc->ep, uvc->host_ep, uvc->vs_itf_num, uvc->wMaxPacketSize);

    usb_write_rxfuncaddr(usb_id, uvc->host_ep, devnum);
    log_debug("[UVC_DBG] hw_reg: RXCSR=0x%04x devnum=%d target_ep=%d if_alt_num=%d",
              usb_read_rxcsr(usb_id, uvc->host_ep), devnum, uvc->ep, uvc->if_alt_num);

    usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, bulk_rx_handler, host_dev);
    usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);

    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
}

int uvc_host_close_camera(void *fd)
{
    int ret;
    struct device *device;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;
    u8 status;
    u32 timeout;
    device = (struct device *)fd;
    if (!device) {
        return DEV_ERR_NONE;
    }
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return DEV_ERR_NONE;
    }
    /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
    usb_dev usb_id = usbdev_to_usbid(device);
    uvc = device_to_uvc(device);
    if (!uvc) {
        return DEV_ERR_NONE;
    }
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    status = host_dev_status(host_dev);
    time_get(timeout, 1000);

    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
#if (STOP_TRANSFER == 0)
#if CAMERA_CLOSE_BY_IRQ
        while (hdl->in_irq) {
            if (time_out(jiffies, timeout)) {
                log_error("wait uvc exiting isr timeout");
                break;
            }
        }
        usb_intr_config(device, uvc->host_ep, 0);
#endif
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc_dummy_stream_out;
#endif
            return DEV_ERR_NONE;
        }
        if (status) {
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, uvc->vs_itf_num, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to %d", 0x01, 0);
        }

#else  /*STOP_TRANSFER*/
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc_dummy_stream_out;
#endif
#if CAMERA_CLOSE_BY_IRQ
            while (hdl->in_irq) {
                if (time_out(jiffies, timeout)) {
                    log_error("wait uvc exiting isr timeout");
                    break;
                }
            }
            usb_intr_config(device, uvc->host_ep, 0);
#endif
            if (status) {
                usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, uvc->vs_itf_num, NULL, 0);
                log_debug("USB_REQ_SET_INTERFACE %d to %d", 0x01, 0);
            }
            uvc->camera_open = 0;

        }
#endif

    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
#if !CAMERA_CLOSE_BY_IRQ
        uvc->stream_out = uvc_dummy_stream_out;
#endif
        while (hdl->in_irq) {
            if (time_out(jiffies, timeout)) {
                log_error("wait uvc exiting isr timeout");
                break;
            }
        }
        usb_intr_config(device, uvc->host_ep, 0);
        uvc->camera_open = 0;
        if (status) {
            if (uvc->format & UVC_VS_FORMAT_FRAME_BASED && uvc->h264_ep) {
                uvc->ep = uvc->h264_ep;
                uvc->vs_itf_num = uvc->h264_vs_itf_num;
            } else if (uvc->format & UVC_VS_FORMAT_MJPEG && uvc->mjpeg_ep) {
                uvc->ep = uvc->mjpeg_ep;
                uvc->vs_itf_num = uvc->mjpeg_vs_itf_num;
            }
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, uvc->vs_itf_num, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to 0 (close)", uvc->vs_itf_num);
            //clear endpoint feature, halt endpoint
            (void)ret;
            ret = usb_control_msg(host_dev, USB_REQ_CLEAR_FEATURE, 0x02, 0, uvc->ep | USB_DIR_IN, NULL, 0);
            log_debug("USB_REQ_CLEAR_FEATURE");
            check_ret(ret);

            Video_Probe_Commit_Control[0]  = 0;
            Video_Probe_Commit_Control[1]  = 0;
            u8 close_fmt_idx = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_format_index : uvc->mjpeg_format_index;
            Video_Probe_Commit_Control[2]  = close_fmt_idx;
            Video_Probe_Commit_Control[3]  = uvc->cur_frame;
            log_debug("%s:fmt_idx:%x cur_frame:%x\n", __func__, close_fmt_idx, uvc->cur_frame);
            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control SET_CUR");
            UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control GET_CUR");
            UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control SET_CUR");
            UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, uvc->vs_itf_num, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control GET_CUR");
            UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            u8 tmp_buf[10];
            ret = usb_control_msg(host_dev, UVC_GET_DEF, USB_GET_REQ, 0x1400, (0x01 << 8) | hdl->itf_base, tmp_buf, 0x0a);
            check_ret(ret);
            log_debug("CameraTerminal GET_DEF");
            UVC_HEXDUMP(tmp_buf, ret);

            memset(tmp_buf, 0, sizeof(tmp_buf));
            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x1400, (0x01 << 8) | hdl->itf_base, tmp_buf, 0x0a);
            check_ret(ret);
            log_debug("CameraTerminal SET_CUR");
            UVC_HEXDUMP(tmp_buf, ret);
        }
    }
    return DEV_ERR_NONE;
}

static int usb_uvc_info(struct device *device);

int uvc_host_open_camera(void *fd)
{
    u32 ret;
    struct device *device;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;
    log_debug("[UVC_FLOW] uvc_host_open_camera ENTERED fd=%p", fd);
    device = (struct device *)fd;
    if (!device) {
        return -DEV_ERR_OFFLINE;
    }
    /*ret = uvc_host_close_camera(device);*/
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -DEV_ERR_OFFLINE;
    }
    /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
    usb_dev usb_id = usbdev_to_usbid(device);
    uvc = device_to_uvc(device);
    if (!uvc) {
        return -DEV_ERR_OFFLINE;
    }
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    uvc->bfh = 0;
    uvc->error_frame = 0;

    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
        log_debug("USB_ENDPOINT_XFER_ISOC\n");
#if (STOP_TRANSFER == 0)
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc->bak_stream_out;
#endif
            log_debug("warning: UVC ALREADY OPEN\n\n\n");
            return 0;
        }
        u8 bulk_alt = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_if_alt_num : uvc->mjpeg_if_alt_num;
        if (bulk_alt != 0) {
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, bulk_alt, uvc->vs_itf_num, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to %d", uvc->vs_itf_num, uvc->if_alt_num);
            check_ret(ret);
        }
        usb_uvc_info(device);
        iso_ep_rx_init(device);
        uvc->camera_open = 1;
#else  /*STOP_TRANSFER*/
        if (!uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc->bak_stream_out;
#endif
            u8 bulk_alt = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_if_alt_num : uvc->mjpeg_if_alt_num;
            if (bulk_alt != 0) {
                ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, bulk_alt, uvc->vs_itf_num, NULL, 0);
                log_debug("USB_REQ_SET_INTERFACE %d to %d", uvc->vs_itf_num, uvc->if_alt_num);
                check_ret(ret);
            }
            usb_uvc_info(device);
            iso_ep_rx_init(device);
            uvc->camera_open = 1;
        }
#endif

    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
        log_debug("USB_ENDPOINT_XFER_BULK\n");
        if (uvc->format & UVC_VS_FORMAT_FRAME_BASED && uvc->h264_ep) {
            uvc->ep = uvc->h264_ep;
            uvc->vs_itf_num = uvc->h264_vs_itf_num;
        } else if (uvc->format & UVC_VS_FORMAT_MJPEG && uvc->mjpeg_ep) {
            uvc->ep = uvc->mjpeg_ep;
            uvc->vs_itf_num = uvc->mjpeg_vs_itf_num;
        }
        u8 bulk_alt = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_if_alt_num : uvc->mjpeg_if_alt_num;
        if (bulk_alt != 0) {
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, bulk_alt, uvc->vs_itf_num, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to %d (BULK open)", uvc->vs_itf_num, bulk_alt);
            check_ret(ret);
        }
        usb_uvc_info(device);
        bulk_ep_rx_init(device);
#if !CAMERA_CLOSE_BY_IRQ
        uvc->stream_out = uvc->bak_stream_out;
#endif
        uvc->camera_open = 1;
    }
    log_info("uvc usb open ok");
    return DEV_ERR_NONE;
}

#define USE_HUSB    0
static int usb_uvc_info(struct device *device)
{
#if USE_HUSB
    u8 tmp_Video_Probe_Commit_Control[26] = {
        0x00, 0x00,
        0x02,//1 yuv 2 mjpg
        0x01,//6 1280*720 5 640*360 1 640*480
        0x15, 0x16, 0x05, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x20, 0x00,
        0x00, 0x60, 0x09, 0x00,
        0x00, 0x0c, 0x00, 0x00
    };
    StreamInterfaceNum_get_cur(device, 1);
    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 1);

    StreamInterfaceNum_get_cur(device, 1);
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 1);
    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 2);

    //StreamInterfaceNum_get_cur(device,1);
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
#else
    int ret = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    /* usb_dev usb_id = host_device2id(host_dev); */
    usb_dev usb_id = usbdev_to_usbid(device);
    struct usb_uvc *uvc = device_to_uvc(device);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    u8 vs_itf = uvc->vs_itf_num;
    u8 fmt_idx = (uvc->format & UVC_VS_FORMAT_FRAME_BASED) ? uvc->h264_format_index : uvc->mjpeg_format_index;
    log_debug("[UVC_DBG] Probe/Commit: vs_itf=%d fmt_idx=%d format=0x%x", vs_itf, fmt_idx, uvc->format);
    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

    log_debug("Video Probe Control GET_CUR");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    u32 max_payload = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    Video_Probe_Commit_Control[22] = LOBYTE(max_payload);
    Video_Probe_Commit_Control[23] = HIBYTE(max_payload);
    Video_Probe_Commit_Control[24] = 0;
    Video_Probe_Commit_Control[25] = 0;

    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control SET_CUR");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_MIN, USB_GET_REQ, 0x0100, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_MIN");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_MAX, USB_GET_REQ, 0x0100, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_MAX");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_CUR");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    Video_Probe_Commit_Control[2] = fmt_idx;
    Video_Probe_Commit_Control[3] = uvc->cur_frame;
    log_debug("%s:fmt_idx:%x cur_frame:%x\n", __func__, fmt_idx, uvc->cur_frame);
    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0200, vs_itf, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Commit Control SET_CUR");
    UVC_HEXDUMP(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);
    {
        u32 frame_interval = Video_Probe_Commit_Control[4] | (Video_Probe_Commit_Control[5] << 8) |
                             (Video_Probe_Commit_Control[6] << 16) | (Video_Probe_Commit_Control[7] << 24);
        u32 max_frame_size = Video_Probe_Commit_Control[18] | (Video_Probe_Commit_Control[19] << 8) |
                             (Video_Probe_Commit_Control[20] << 16) | (Video_Probe_Commit_Control[21] << 24);
        u32 fps = frame_interval ? (10000000 / frame_interval) : 0;
        log_debug("[UVC_INFO] format=%s fmt_idx=%d frame_idx=%d fps=%d interval=%d max_frame_size=%d\n",
                  (uvc->format & 0x10) ? "H264" : (uvc->format == 0x06) ? "MJPEG" : "YUV",
                  fmt_idx, uvc->cur_frame, fps, frame_interval, max_frame_size);
    }

#endif
    return DEV_ERR_NONE;
}

static void toogle_port()
{

}

struct status_packet_format {
    u8 bStatusType;//bit0-3 : 0=Reserved, 1=VideoControl interface, 2 = VideoStreaming interface
    u8 bOriginator;//ID of Teriminal, Unit or Interface
};

struct status_vc_packet {
    u8 bEvent;//0x0 : Control Change , 0x1 - 0xFF : Reserved
    u8 bSelector;//Report the Control Selector of th control that issued the interrupt
    u8 bAttribute;//0x0:Control value change, 0x1:Control info change, 0x2:Control failure change, 0x3-0xff : Reserved
    u8 bValue;//0x0:Equivalent to the result of a GET_CUR request, 0x1:GET_INFO request, 0x2:GET_CUR request on VC_REQUEST_ERROR_CODE_CONTROL
};

struct status_vs_packet {
    u8 bEvent;//0x0:Button Press, 0x1-0xff:Stream Error
    u8 bValue;//Button Press--0x0:button released, 0x1:button pressed
};

void *uvc_host_open(void *arg)
{
    struct uvc_host_param *info = (struct uvc_host_param *)arg;
    struct usb_uvc *uvc;
    struct device *device;
    struct usb_host_device *host_dev;
    struct usb_video_manager *hdl;
    usb_dev usb_id = 0;

    device = uvc_host_device_find(info->name);
    if (device) {
        atomic_inc(&(device->ref));
        host_dev = device_to_usbdev(device);
        if (!host_dev) {
            return NULL;
        }

        if (info->name) {
            usb_id = info->name[3] - '0';
            /* usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
        }
        hdl = &uvc_manager[usb_id];
        if (hdl->open) {
            log_error("uvc host reopen !!!");
            return device;
        }
        uvc = device_to_uvc(device);
        if (uvc) {
            uvc->priv = info->priv;
#if !CAMERA_CLOSE_BY_IRQ
            if (!uvc->camera_open) {
                uvc->stream_out = info->uvc_stream_out;

            }
            uvc->bak_stream_out = info->uvc_stream_out;
#else
            uvc->stream_out = info->uvc_stream_out;
#endif
            uvc->offline = info->uvc_out;
        }
        hdl->open = 1;
        return device;
    } else {
        log_debug("err uvc_host_open device NULL\n");
        return NULL;
    }
}

int uvc_host_close(void *fd)
{
    struct device *device = (struct device *)fd;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;

    if (device && (device->private_data != NULL)) {
        if (atomic_dec_and_test(&device->ref)) {
            struct usb_video_manager *hdl = container_of(device, struct usb_video_manager, dev);
            host_dev = device_to_usbdev(device);
            if (!host_dev) {
                return DEV_ERR_NONE;
            }
            usb_dev usb_id = hdl->name[3] - '0';
            log_info("uvc_host_close %s\n", hdl->name);
            /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
            uvc = device_to_uvc(device);
            if (uvc) {
                /* usb_intr_config(device, uvc->host_ep, 0); */
                uvc->stream_out = uvc_dummy_stream_out;
#if !CAMERA_CLOSE_BY_IRQ
                uvc->bak_stream_out = uvc_dummy_stream_out;
#endif
                uvc->priv = NULL;
                uvc->offline = NULL;
            }
            /* struct usb_video_manager *hdl = &uvc_manager[usb_id]; */
            hdl->open = 0;
        }
    }
    return DEV_ERR_NONE;
}

int uvc_host_get_pix_table(void *fd, struct uvc_frame_info **pix_table)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
    usb_dev usb_id = usbdev_to_usbid(device);
    struct usb_uvc *uvc = device_to_uvc(device);

    if (uvc && uvc->reso_num) {
        *pix_table = uvc->pframe;
        return uvc->reso_num;
    }
    return 0;
}

int uvc_host_get_fps(void *fd)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
    usb_dev usb_id = usbdev_to_usbid(device);
    struct usb_uvc *uvc = device_to_uvc(device);
    if (uvc) {
        return uvc->fps;
    }
    return 25;
}

int uvc_host_set_pix(void *fd, int index)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    /* usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev)); */
    usb_dev usb_id = usbdev_to_usbid(device);
    struct usb_uvc *uvc = device_to_uvc(device);

    if (uvc) {
        // printf("[DBG_SETPIX] SET cur_frame=%d\n", index);
        uvc->cur_frame = index;
    } else {
        // printf("[DBG_SETPIX] uvc=NULL, NOT set!\n");
    }
    return 0;
}

int uvc_force_reset(void *fd)

{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    uvc_host_close_camera(fd);
    return usb_host_force_reset(usb_id);
}

int uvc2usb_ioctl(void *fd, u32 cmd, void *arg)
{
#if 0
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -ENODEV;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);

    if (!uvc) {
        return -ENODEV;
    }
    switch (cmd) {
    case UVCIOC_SET_EVENT_LISTENER:
        struct uvc_event_listener *listener = (struct uvc_event_listener *)arg;
        uvc->event_priv = listener->priv;
        uvc->event_handler = listener->handler;
        break;
    default:
        return host_device_ioctl(host_dev, cmd, arg);
    }
#endif
    return 0;
}

u32 uvc_host_get_fmt(void)
{
    return ((uvc_host_inf[uvc_host_online()].dev.uvc->format == UVC_VS_FORMAT_UNCOMPRESSED) ? 1 : 0);
}

int uvc_host_get_cur_resolution(int *width, int *height)
{
    int id = uvc_host_online();
    if (id < 0) {
        goto fallback;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        goto fallback;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (!uvc || !uvc->reso_num) {
        goto fallback;
    }
    /* cur_frame is 1-indexed (UVC bFrameIndex), pframe[] is 0-indexed */
    int idx = (int)uvc->cur_frame - 1;
    if (idx < 0 || idx >= uvc->reso_num) {
        idx = 0;
    }
    if (width) {
        *width = uvc->pframe[idx].width;
    }
    if (height) {
        *height = uvc->pframe[idx].height;
    }
    return 0;

fallback:
    if (width) {
        *width = 640;
    }
    if (height) {
        *height = 480;
    }
    return -1;
}

u8 uvc_host_is_h264_format(void)
{
    int id = uvc_host_online();

    if (id < 0) {
        return 0;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (uvc && (uvc->format & UVC_VS_FORMAT_FRAME_BASED)) {
        return 1;
    }
    return 0;
}

#define JPEG_HEAD 	0xE0FFD8FF
#define JPEG_HEAD1 	0xC0FFD8FF

static int net_jpeg_pkg_head_check(u32 head)
{
    if (uvc_host_get_fmt()) {
        return true;
    }

    if (head == JPEG_HEAD) {
        return true;
    } else if (head == JPEG_HEAD1) {
        return true;
    }
    return 0;
}

static int jpeg_pkg_head_check(u32 head)
{
    if (uvc_host_get_fmt()) {
        return true;
    }

    if (head == JPEG_HEAD) {
        return true;
    } else if (head == JPEG_HEAD1) {
        return true;
    }
    return 0;
}

int uvc_host_get_reso_count(void)
{
    int id = uvc_host_online();
    if (id < 0) {
        return 0;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        return 0;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (!uvc || !uvc->reso_num) {
        return 0;
    }
    return uvc->reso_num;
}

int uvc_host_get_reso_by_index(int index, u16 *width, u16 *height)
{
    int id = uvc_host_online();
    if (id < 0 || !width || !height) {
        return -1;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        return -1;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (!uvc || index >= uvc->reso_num) {
        return -1;
    }
    *width  = uvc->pframe[index].width;
    *height = uvc->pframe[index].height;
    return 0;
}

int uvc_host_get_fps_val(void)
{
    int id = uvc_host_online();
    if (id < 0) {
        return 25;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        return 25;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (!uvc) {
        return 25;
    }
    return uvc->fps;
}

int uvc_host_get_fmt_type(void)
{
    int id = uvc_host_online();
    if (id < 0) {
        return 0;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        return 0;
    }
    struct usb_uvc *uvc = uvc_host_inf[id].dev.uvc;
    if (!uvc) {
        return 0;
    }
    return uvc->format;
}

void uvc_host_start_camera(void)
{
    int id = uvc_host_online();
    if (id < 0) {
        return;
    }
    struct usb_video_manager *hdl = &uvc_manager[id];
    if (!hdl->online) {
        return;
    }
    if (!hdl->open) {
        return;
    }
    uvc_host_open_camera(&hdl->dev);
}

#endif // USB_HOST_UVC
