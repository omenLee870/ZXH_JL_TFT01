
/*****************************************************************
>file name : uvc_dev.c
>author : lichao
>create time : Tue 13 Jun 2017 04:20:02 PM HKT
*****************************************************************/
#include "device/usb_stack.h"
#include "uvc_device.h"
#include "usb_config.h"
#include "host_uvc.h"
#include "video_ioctl.h"
#include "videobuf.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "os/os_type.h"
#include "app_config.h"
#include <stdio.h>

#if TCFG_HOST_UVC_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UVC_HOST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define UVC_CHANNEL_MAX			2
#define UVC_REC_JPG_HEAD_SIZE	8
#define UVC_REC_JPG_ALIGN     	512
#define UVC_JPEG_HEAD 			0xE0FFD8FF
#define UVC_JPEG_HEAD1 			0xC0FFD8FF
/* Fix6v5: JPEG marker-aware SOS finder.
 * Parses marker segments by their length fields (skipping APP/DQT/DHT/SOF data)
 * to avoid false FF DA matches inside marker payloads.
 * Returns byte offset of FF DA, or -1 if not found.
 * Assumes data starts with SOI (FF D8). */
static int jpeg_find_sos(const u8 *data, u32 len)
{
    if (!data || len < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        return -1;
    }
    u32 pos = 2;
    while (pos + 1 < len) {
        if (data[pos] != 0xFF) {
            pos++;
            continue;
        }
        u8 m = data[pos + 1];
        if (m == 0x00) {
            pos += 2;
            continue;
        }
        if (m >= 0xD0 && m <= 0xD7) {
            pos += 2;
            continue;
        }
        if (m == 0xD8) {
            pos += 2;
            continue;
        }
        if (m == 0xD9) {
            return -1;
        }
        if (m == 0xDA) {
            return (int)pos;
        }
        if (pos + 3 < len) {
            u32 seg_len = ((u32)data[pos + 2] << 8) | data[pos + 3];
            if (seg_len < 2) {
                seg_len = 2;
            }
            pos += 2 + seg_len;
        } else {
            pos += 2;
        }
    }
    return -1;
}

#define USB_DMA_CP_ENABLE       (0)// NOTE: HW DMA copy must use async receive push, otherwise fast data causes corruption
#if TCFG_PSRAM_DEV_ENABLE
#define UVC_MALLOC(s)   malloc_psram(s)
#define UVC_FREE(p)     free_psram(p)
#else
#define UVC_MALLOC(s)   malloc(s)
#define UVC_FREE(p)     free(p)
#endif

static u8 *__persist_buf = NULL;  /* Persistent PSRAM buffer for UVC RX, allocated once on first use */
#define UVC_PERSIST_BUF_INIT_SIZE  (1 * 1024 * 1024)   /* 1MB initial */
#define UVC_PERSIST_BUF_MAX_SIZE   (3 * 1024 * 1024)   /* 3MB max on 4MB PSRAM */
static u32 __persist_buf_size = 0;  /* Current size, grows on demand */

#ifdef CONFIG_UI_ENABLE
#define UVC_RECV_BUFF_SHARE		0 // 1: UVC RX data uses shared frame independent of app-layer buffers; register uvc_host_jpg_callback_register for data, app handles display etc.
#else
#define UVC_RECV_BUFF_SHARE        0
#endif
#define UVC_RECV_BUFF_SHARE_SIZE	(32*1024) // UVC single JPEG frame size, recommend 100KB

#define ADDR_ALIGNE(addr,align) ((addr + (align - 1)) / align * align)

/* Callback for JPEG frame output to app layer */
static void (*uvc_host_jpg_cb)(char *jpg_buf, u32 jpg_len);
static void uvc_host_jpg_callback_register(void (*cb)(char *jpg_buf, u32 jpg_len))
{
    uvc_host_jpg_cb = cb;
}

/* Callback for raw frame output (MJPEG/H264) to app layer */
static void (*uvc_host_frame_cb)(const u8 *data, u32 len, u8 is_h264);
void uvc_host_frame_cb_register(void (*cb)(const u8 *data, u32 len, u8 is_h264))
{
    uvc_host_frame_cb = cb;
}

static int uvc_jpeg_head_check(u32 head)
{
    if (uvc_host_get_fmt()) {
        return true;
    }

    if (head == UVC_JPEG_HEAD) {
        return true;
    } else if (head == UVC_JPEG_HEAD1) {
        return true;
    }
    return 0;
}

struct uvc_dev_control {
    OS_SEM sem;
    struct list_head dev_list;
};
struct mjpg_uvc_image {
    u8 *buf;
    u32 size;
};
struct uvc_fh {
    u8 id;
    u8 eof;
    u8 fps;
    u8 real_fps;
    u8 drop_frame;
    u8 streamon;
    volatile u8 streamoff;
    u8 uvc_out;
    u8 channel;
    u8 alloc_channel;
    u8 open[UVC_CHANNEL_MAX];
    u8 streamon_ch[UVC_CHANNEL_MAX];
    u32 frame_cnt;
    u32 drop_frame_cnt;
    struct list_head entry;
    int free_size;
    u8 *buf;
    u8 *share_buf;
    int b_offset;
    u8 buf_is_malloc;
    /* dma_user_task_t dma_list[16]; */
    u8 bfmode;
    u8 uvc_video_q;

    void *ppbuf;
    struct videobuf_queue video_q;
    struct video_device *video_dev[UVC_CHANNEL_MAX];
    struct device device[UVC_CHANNEL_MAX];
    void *private_data;
    OS_SEM sem;
    OS_SEM img_sem;
    struct mjpg_uvc_image img;
    volatile u8 image_req;
};
/* Singleton UVC device control: semaphore + device list */
static struct uvc_dev_control uvc_dev;

#define __this  (&uvc_dev)

#define list_for_each_uvc(fh) \
        list_for_each_entry(fh, &__this->dev_list, entry)

#define list_add_uvc(fh) \
        list_add(&fh->entry, &__this->dev_list)

#define list_del_uvc(fh) \
        list_del(&fh->entry);

static int uvc_img_cap(void *_fh, u32 arg)
{
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    struct video_image_capture *icap = (struct video_image_capture *)arg;
    u32 *head;
    int err = 0;
    if (!icap) {
        return -EINVAL;
    }
    if (fh) {
        fh->img.buf = icap->baddr;
        fh->img.size = icap->size;
        os_sem_create(&fh->img_sem, 0);
        err = os_sem_pend(&fh->img_sem, 500);
        if (err) {
            log_error("image err");
        }
        fh->image_req = false;
        head = (u32 *)(fh->img.buf + 8);
        if (uvc_jpeg_head_check(*head)) {
            fh->img.size -= 8;
            memcpy(icap->baddr, icap->baddr + 8, fh->img.size);
        }
        os_sem_del(&fh->img_sem, OS_DEL_ALWAYS);
        if (icap->size < fh->img.size) {
            log_error("image capture size is too small !!!");
        } else if (!err) {
            log_info("image capture : %d.%dKB", fh->img.size / 1024, (fh->img.size % 1024) * 10 / 1024);
        }
        icap->size = fh->img.size;
        return err;
    } else {
        log_error("image err in on open uvc");
    }
    return err;
}
static void *uvc_buf_malloc(struct uvc_fh *fh, u32 size, u8 channel)// UVC video stream buffer allocation
{
    void *b;
    if (fh->uvc_video_q) {
        struct videobuf_buffer *vb;
        vb = videobuf_stream_alloc(&fh->video_q, size);
        if (!vb) {
            return NULL;
        }
        vb->len = size;
        return vb->data;
    }
    if (!channel) {
        for (int i = 0; i < UVC_CHANNEL_MAX; i++) {
            if (fh->open[i] && fh->streamon_ch[i]) {
                channel = i + 1;
                break;
            }
        }
        if (!channel) {
            return NULL;
        }
    }
    fh->alloc_channel = channel;
    if (!fh->open[channel - 1] || !fh->streamon_ch[channel - 1]) {
        return NULL;
    }
    struct video_device *video_dev = (struct video_device *)fh->video_dev[fh->alloc_channel - 1];
    if (!video_dev) {
        return NULL;
    }
    b = video_buf_malloc(video_dev, size);  /* Alloc via video_buf subsystem */
    return b;
}
static void *uvc_buf_realloc(struct uvc_fh *fh, void *buf, int size, u8 channel)// UVC video stream buffer reallocation
{
    if (fh->uvc_video_q) {
        struct videobuf_buffer *b = container_of(buf, struct videobuf_buffer, data);

        b->len = size;
        videobuf_stream_realloc(&fh->video_q, b, size);

        return b->data;
    }
    if (!channel || !fh->open[channel - 1] || !fh->streamon_ch[channel - 1]) {
        return NULL;
    }
    struct video_device *video_dev = (struct video_device *)fh->video_dev[channel - 1];
    if (!video_dev) {
        return NULL;
    }
    return video_buf_realloc(video_dev, buf, size);
}
static void uvc_buf_free(struct uvc_fh *fh, void *buf, u8 channel)// UVC video stream buffer free
{
    if (fh->uvc_video_q) {
        struct videobuf_buffer *b = container_of(buf, struct videobuf_buffer, data);

        ASSERT(buf != NULL, "uvc_buf_free\n");

        videobuf_stream_free(&fh->video_q, b);

        return;
    }
    if (!channel || !fh->open[channel - 1] || !fh->streamon_ch[channel - 1]) {
        return;
    }
    struct video_device *video_dev = (struct video_device *)fh->video_dev[channel - 1];
    if (!video_dev) {
        return ;
    }
    return video_buf_free(video_dev, buf);
}
static u32 uvc_buf_free_space(struct uvc_fh *fh, u8 channel)// UVC video stream remaining free space
{
    if (fh->uvc_video_q) {
        return videobuf_stream_free_space(&fh->video_q);
    }
    if (!channel) {
find_open:
        for (int i = 0; i < UVC_CHANNEL_MAX; i++) {
            if (fh->open[i] && fh->streamon_ch[i]) {
                channel = i + 1;
                break;
            }
        }
        if (!channel) {
            return 0;
        }
        fh->alloc_channel = channel;
    }
    if (!fh->open[channel - 1] || !fh->streamon_ch[channel - 1]) {
        return 0;
    }
    struct video_device *video_dev = (struct video_device *)fh->video_dev[channel - 1];
    if (!video_dev) {
        channel = 0;
        goto find_open;
    }
    return video_buf_free_space(video_dev);
}
static void uvc_buf_stream_finish(struct uvc_fh *fh, void *buf, u8 channel)// UVC video stream push to app layer
{
    if (fh->uvc_video_q) {
        struct videobuf_buffer *b = container_of(buf, struct videobuf_buffer, data);
        videobuf_stream_finish(&fh->video_q, b);
        return;
    }
    if (!channel || !buf || !fh->open[channel - 1] || !fh->streamon_ch[channel - 1]) {
        return;
    }
    struct video_device *video_dev = (struct video_device *)fh->video_dev[channel - 1];
    if (!video_dev) {
        return;
    }
    u32 size;
    if (os_sem_valid(&fh->img_sem) && fh->image_req) {// If photo capture requested, copy data to photo buffer
        size = video_buf_size(buf);
        size = MIN(size, fh->img.size);
        memcpy(fh->img.buf, buf, size);
        fh->img.size = size;
        os_sem_post(&fh->img_sem);
        fh->image_req = false;
    }
    video_buf_stream_finish(video_dev, buf);// lbuf push stream
}
static int uvc_dev_reqbufs(void *_fh, struct uvc_reqbufs *b)
{

    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    struct video_reqbufs vb_req = {0};

    vb_req.buf = b->buf;
    vb_req.size = b->size;

    videobuf_queue_init(&fh->video_q, 32, "uvc_dev");
    fh->uvc_video_q = 1;
    return videobuf_reqbufs(&fh->video_q, (struct video_reqbufs *)&vb_req);

}

static int uvc_dev_qbuf(void *_fh, struct video_buffer *b)
{

    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    return videobuf_qbuf(&fh->video_q, b);

}

static int uvc_dev_dqbuf(void *_fh, struct video_buffer *b)
{

    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    if (fh->uvc_out) {
        return -ENODEV;
    }
    if (!fh->streamon) {
        return -ENODEV;
    }
    return videobuf_dqbuf(&fh->video_q, b);

}


/* Grow __persist_buf to at least needed_size, preserving accumulated frame data.
 * Called from ISR context during MJPEG accumulation when buffer overflows.
 * Returns 0 on success, -1 on failure. */
static int persist_buf_grow(struct uvc_fh *fh, u32 needed_size)
{
    if (needed_size > UVC_PERSIST_BUF_MAX_SIZE) {
        /* printf("[MJPEG_OVF] #%d accum=%dKB needed=%dKB max=%dKB\r\n", fh->frame_cnt+1, (int)fh->b_offset/1024, (int)needed_size/1024, UVC_PERSIST_BUF_MAX_SIZE/1024); */
        log_error("[PERSIST] needed %d > max %d\n", needed_size, UVC_PERSIST_BUF_MAX_SIZE);
        return -1;
    }
    u32 new_size = __persist_buf_size ? __persist_buf_size : UVC_PERSIST_BUF_INIT_SIZE;
    while (new_size < needed_size) {
        new_size *= 2;
    }
    if (new_size > UVC_PERSIST_BUF_MAX_SIZE) {
        new_size = UVC_PERSIST_BUF_MAX_SIZE;
    }
    u8 *new_buf = (u8 *)UVC_MALLOC(new_size);
    if (!new_buf) {
        log_error("[PERSIST] grow to %d FAILED\n", new_size);
        return -1;
    }
    u32 copy_bytes = UVC_REC_JPG_HEAD_SIZE + fh->b_offset;
    if (__persist_buf && copy_bytes > 0) {
        memcpy(new_buf, __persist_buf, copy_bytes);
    }
    if (__persist_buf) {
        UVC_FREE(__persist_buf);
    }
    __persist_buf = new_buf;
    __persist_buf_size = new_size;
    fh->buf = __persist_buf;
    fh->free_size = new_size;
    log_info("[PERSIST] grown to %d (needed %d)\n", new_size, needed_size);
    return 0;
}

int uvc_mjpg_stream_out(void *fd, int cnt, void *stream_list, int state)
{
    struct uvc_fh *fh = (struct uvc_fh *)fd;
    struct uvc_stream_list *list = (struct uvc_stream_list *)stream_list;
    int i;
    int err = 0;
    u32 tmp_jiffies = 0, req_size;

    if (!fh) {
        return -EINVAL;
    }

    if ((cnt < 0) || !list) {
        //{ static u32 _ec=0; if(_ec<30){ printf("[EARLY] cnt=%d list=%p state=%d buf=%p drop=%d boff=%d free=%d\r\n", cnt,list,state,fh->buf,fh->drop_frame,(int)fh->b_offset,(int)fh->free_size); _ec++; } }
        /*putchar('E');*/
        if (fh->buf) {
            if (fh->buf != fh->share_buf) {// Only free if no multi-channel shared memory
                uvc_buf_free(fh, fh->buf, fh->alloc_channel);
            }
            fh->buf = NULL;
            if (state != STREAM_SOF) { // eof == 2 next frame start
                fh->drop_frame = 1;
            }

            err = -EINVAL;
        } else if (state == STREAM_SOF) {
            fh->drop_frame = 0;
            fh->b_offset = 0;
        }
        goto _exit;
    }

    if (fh->drop_frame) {
        if (state == STREAM_EOF) {
            fh->drop_frame = 0;
        }
        goto _exit;
    }

    if (!fh->buf) {
        if (fh->share_buf) {// If shared memory available, use shared size (typically when 2 channels or UI enabled)
            fh->buf = fh->share_buf;
            fh->free_size = uvc_host_get_fmt() ? UVC_RECV_BUFF_SHARE_SIZE : UVC_RECV_BUFF_SHARE_SIZE - UVC_REC_JPG_ALIGN;
            fh->alloc_channel = 0;
        } else {// No shared memory: request free space and allocate if sufficient
            fh->free_size = uvc_buf_free_space(fh, fh->alloc_channel);
            if (fh->free_size > 1024) {
                fh->buf = uvc_buf_malloc(fh, fh->free_size, fh->alloc_channel);
                if (!uvc_host_get_fmt()) {
                    fh->free_size -= UVC_REC_JPG_ALIGN;// Subtract 512 to prevent realloc 512-alignment assertion
                }
            }
        }
        if (!fh->buf) {
            if (!__persist_buf) {
                __persist_buf = (u8 *)UVC_MALLOC(UVC_PERSIST_BUF_INIT_SIZE);
                if (!__persist_buf) {
                    log_error("[FLOW7] persist UVC_MALLOC(%d) FAILED\n", UVC_PERSIST_BUF_INIT_SIZE);
                    err = -ENOMEM;
                    goto _exit;
                }
                __persist_buf_size = UVC_PERSIST_BUF_INIT_SIZE;
            }
            fh->buf = __persist_buf;
            fh->free_size = __persist_buf_size;
            fh->buf_is_malloc = 0;
        }
        fh->b_offset = 0;
    }

    for (i = 0; i < cnt; i++) {
        if ((fh->b_offset + list[i].length + UVC_REC_JPG_HEAD_SIZE) > fh->free_size) {// Data exceeds available memory space
            /* Try dynamic grow for persist_buf before dropping */
            if (fh->buf == __persist_buf) {
                u32 needed = fh->b_offset + list[i].length + UVC_REC_JPG_HEAD_SIZE;
                if (persist_buf_grow(fh, needed) == 0) {
                    /* Grow succeeded; fh->buf and fh->free_size updated, continue */
                } else {
                    goto _drop_frame;
                }
            } else {
_drop_frame:
                if (fh->buf_is_malloc) {
                    free(fh->buf);
                } else if (fh->buf != fh->share_buf) {
                    uvc_buf_free(fh, fh->buf, fh->alloc_channel);
                }
                fh->buf = NULL;
                fh->buf_is_malloc = 0;
                fh->drop_frame = 1;
                err = -EFAULT;
                goto _exit;
            }
        }
        // Copy data to buf cache
#if USB_DMA_CP_ENABLE
        fh->dma_list[i].src_addr = list[i].addr;
        fh->dma_list[i].dst_addr = fh->buf + UVC_REC_JPG_HEAD_SIZE + fh->b_offset;
        fh->dma_list[i].len = list[i].length;

#else
        memcpy(fh->buf + UVC_REC_JPG_HEAD_SIZE + fh->b_offset, list[i].addr, list[i].length);
#endif
        fh->b_offset += list[i].length;
    }

#if USB_DMA_CP_ENABLE
    dma_task_list_copy(fh->dma_list, cnt);// NOTE: HW DMA copy must use async receive push, otherwise fast data causes corruption
#endif

    if (state == STREAM_EOF) {// USB End-Of-Frame: one complete image frame received
#ifdef CONFIG_UVC_DROP_FRAME_ENABLE
        if (fh->real_fps && fh->fps && fh->real_fps < fh->fps) {// Check if USB frame rate differs from app-layer requested FPS, drop frames if needed
            u32 drop = fh->frame_cnt * (fh->fps - fh->real_fps) / fh->fps;
            if (fh->drop_frame_cnt != drop) {// Mismatch detected, drop current frame
                fh->drop_frame_cnt = drop;
                if (fh->buf != fh->share_buf) {// Drop frame
                    uvc_buf_free(fh, fh->buf, fh->alloc_channel);
                }
                fh->buf = NULL;
                goto _exit;
            }
        }
#endif
        if (uvc_host_get_fmt()) {
            req_size = fh->b_offset + UVC_REC_JPG_HEAD_SIZE;
        } else {
            req_size = ADDR_ALIGNE(fh->b_offset + UVC_REC_JPG_HEAD_SIZE, UVC_REC_JPG_ALIGN);// JPEG image data requires 512-byte alignment
        }
        u8 *save_ptr = fh->buf + UVC_REC_JPG_HEAD_SIZE;
        u32 save_sz = fh->b_offset;
        /* log_debug("[MJPEG_SZ] #%d raw=%dKB persist=%dKB\r\n", fh->frame_cnt+1, (int)save_sz/1024, (int)__persist_buf_size/1024); */

        /* [FIX_SOI] Find LAST SOI (0xFFD8) in accumulated data.
         * Multi-segment MJPEG: each FID segment may have its own SOI.
         * The last SOI marks the start of the complete JPEG frame.
         * Discard everything before it. */

        {
            u32 soi_off = 0;
            int soi_found = 0;
            int soi_count = 0;
            for (u32 off = 0; off + 1 < save_sz; off++) {
                if (save_ptr[off] == 0xFF && save_ptr[off + 1] == 0xD8) {
                    soi_off = off;
                    soi_found = 1;
                    soi_count++;
                }
            }
            if (soi_found) {
                if (soi_off > 0) {
                    save_ptr += soi_off;
                    save_sz -= soi_off;
                }
            } else {
                fh->b_offset = 0;
                goto _exit;
            }
        }

        /* Discard tiny frames: frames <1KB are likely corrupt stubs. */
        if (save_sz < 1024) {
            fh->b_offset = 0;
            goto _exit;
        }

        /* Discard incomplete frames: no EOI (FF D9) found in last 4KB means corrupted JPEG.
         * Defense-in-depth: even if ISR sent STREAM_EOF, verify JPEG data completeness. */
        {
            int eoi_found = 0;
            u32 eoi_scan = (save_sz > 4096) ? 4096 : (save_sz - 1);
            for (u32 o = 0; o < eoi_scan; o++) {
                u32 idx = save_sz - 2 - o;
                if (save_ptr[idx] == 0xFF && save_ptr[idx + 1] == 0xD9) {
                    eoi_found = 1;
                    break;
                }
            }
            if (!eoi_found) {
                fh->b_offset = 0;
                goto _exit;
            }
        }

        /* Persist_buf: data valid during callbacks; b_offset reset after callbacks below.
         * No __save_tmp copy needed -- callbacks run synchronously in ISR context. */

        if (uvc_host_jpg_cb) {// If callback registered, invoke it first
            uvc_host_jpg_cb((char *)save_ptr, save_sz);
        }
        if (save_sz > 0 && uvc_host_frame_cb) {
            uvc_host_frame_cb(save_ptr, save_sz, 0);
        }
        /* Reset b_offset now that callbacks have consumed the data */
        if (fh->buf == __persist_buf) {
            fh->b_offset = 0;
        }
        fh->frame_cnt++;
        if (fh->buf == fh->share_buf && fh->share_buf) {// Shared data: multi-channel mode
            for (int ch = 1; ch <= UVC_CHANNEL_MAX; ch++) {// Check all channels
                if (uvc_buf_free_space(fh, ch) > 1024) {// Check sufficient free space before allocating and copying data
                    u8 *pbuf = uvc_buf_malloc(fh, req_size, ch);
                    if (pbuf) {
                        memcpy(pbuf + UVC_REC_JPG_HEAD_SIZE, fh->buf + UVC_REC_JPG_HEAD_SIZE, fh->b_offset);
                        fh->buf = pbuf;
                        if (fh->buf) {
                            uvc_buf_stream_finish(fh, fh->buf, ch);// Push stream to app layer
                        }
                    }
                }
            }
        } else {// Single channel
            if (fh->buf == __persist_buf) {
                /* b_offset already reset after callbacks */
            } else if (fh->buf_is_malloc) {
                UVC_FREE(fh->buf);
                fh->buf = NULL;
                fh->buf_is_malloc = 0;
            } else {
                fh->buf = uvc_buf_realloc(fh, fh->buf, req_size, fh->alloc_channel);
                if (fh->buf) {
                    uvc_buf_stream_finish(fh, fh->buf, fh->alloc_channel);
                }
                fh->buf = NULL;
                fh->buf_is_malloc = 0;
            }
        }
    }

_exit:
    fh->streamoff = 0;
    return err;
}

#define H264_ACCUM_BUF_SIZE (1 * 1024 * 1024)
/* H264 frame accumulation buffer (PSRAM) */
static u8 *h264_accum_buf = NULL;
/* H264 accumulation buffer write offset (volatile, shared with ISR) */
static volatile u32 h264_accum_offset = 0;

static void h264_accum_buf_init(void)
{
    if (!h264_accum_buf) {
        h264_accum_buf = UVC_MALLOC(H264_ACCUM_BUF_SIZE);
        if (h264_accum_buf) {
            h264_accum_offset = 0;
            log_info("[H264_ACCUM] alloc PSRAM %p size=%d\n", h264_accum_buf, H264_ACCUM_BUF_SIZE);
        } else {
            log_info("[H264_ACCUM] PSRAM alloc FAILED, trying SRAM...\n");
            h264_accum_buf = malloc(H264_ACCUM_BUF_SIZE);
            if (h264_accum_buf) {
                h264_accum_offset = 0;
                log_info("[H264_ACCUM] alloc SRAM %p size=%d\n", h264_accum_buf, H264_ACCUM_BUF_SIZE);
            } else {
                log_error("[H264_ACCUM] alloc FAILED (both pools)\n");
            }
        }
    }
}

/* SPS/PPS cache for frames that arrive without parameter sets */
/* Cached H264 SPS (Sequence Parameter Set) for late-arriving frames */
static u8 g_h264_sps[512];
static volatile u32 g_h264_sps_len = 0;  /* Valid SPS length */
/* Cached H264 PPS (Picture Parameter Set) for late-arriving frames */
static u8 g_h264_pps[256];
static volatile u32 g_h264_pps_len = 0;  /* Valid PPS length */
#define H264_OUT_BUF_SIZE (256 * 1024)
/* H264 output buffer for prepending SPS/PPS before frame data (allocated from PSRAM on first use) */
static u8 *g_h264_out_buf = NULL;

static void cache_h264_sps_pps(const u8 *data, u32 len)
{
    u32 i = 0;
    while (i + 4 < len) {
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            u8 ntype = data[i + 4] & 0x1f;
            if (ntype == 7 || ntype == 8) {
                u32 nal_start = i;
                u32 nal_end = len;
                u32 j = i + 5;
                while (j + 3 < len) {
                    if (data[j] == 0x00 && data[j + 1] == 0x00 &&
                        (data[j + 2] == 0x01 || (data[j + 2] == 0x00 && data[j + 3] == 0x01))) {
                        nal_end = j;
                        break;
                    }
                    j++;
                }
                u32 nal_len = nal_end - nal_start;
                if (ntype == 7 && nal_len <= 512) {
                    memcpy(g_h264_sps, data + nal_start, nal_len);
                    g_h264_sps_len = nal_len;
                } else if (ntype == 8 && nal_len <= 256) {
                    memcpy(g_h264_pps, data + nal_start, nal_len);
                    g_h264_pps_len = nal_len;
                }
                i = j;
            } else {
                i += 5;
            }
        } else {
            i++;
        }
    }
}

static int h264_validate_frame(const u8 *data, u32 len)
{
    int has_sps = 0, has_pps = 0, has_idr = 0;
    u32 i = 0;
    while (i + 4 < len) {
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01) {
            u8 ntype = data[i + 4] & 0x1f;
            if (ntype == 7) {
                has_sps = 1;
            } else if (ntype == 8) {
                has_pps = 1;
            } else if (ntype == 5) {
                has_idr = 1;
            }
            i += 5;
        } else {
            i++;
        }
    }
    return has_sps && has_pps && has_idr;
}

int uvc_h264_stream_out(void *fd, int cnt, void *stream_list, int state)
{
    h264_accum_buf_init();
    if (!h264_accum_buf) {
        log_error("[H264_OUT] FATAL: accum buf alloc failed, dropping\n");
        return -ENOMEM;
    }
    struct uvc_fh *fh = (struct uvc_fh *)fd;
    struct uvc_stream_list *list = (struct uvc_stream_list *)stream_list;
    int i;
    int err = 0;

    if (!fh) {
        return -EINVAL;
    }
    if ((cnt < 0) || !list) {
        if (state != STREAM_SOF) {
            fh->drop_frame = 1;
        }
        h264_accum_offset = 0;
        err = -EINVAL;
        goto _exit;
    }

    if (fh->drop_frame) {
        if (state == STREAM_EOF) {
            fh->drop_frame = 0;
            h264_accum_offset = 0;
        }
        goto _exit;
    }

    for (i = 0; i < cnt; i++) {
        u8 *src = list[i].addr;
        u32 src_len = list[i].length;

        if (src_len > 0 && src_len <= 512) {
            int all_zero = 1;
            for (int zi = 0; zi < src_len; zi++) {
                if (src[zi]) {
                    all_zero = 0;
                    break;
                }
            }
            if (all_zero) {
                continue;
            }
        }
        if ((src_len == 12 || src_len == 2) && (src[0] == 0x0c || src[0] == 0x02) && (src[1] & 0x80) == 0x80) {
            continue;
        }
        if ((h264_accum_offset + src_len) > H264_ACCUM_BUF_SIZE) {
            log_error("[H264_ACCUM] OVERFLOW! off=%d + src=%d > size=%d\n",
                      h264_accum_offset, src_len, H264_ACCUM_BUF_SIZE);
            h264_accum_offset = 0;
            fh->drop_frame = 1;
            err = -EFAULT;
            goto _exit;
        }
        if (!h264_accum_buf) {
            log_error("[H264_ACCUM] buf NULL, force reinit\n");
            h264_accum_buf_init();
            if (!h264_accum_buf) {
                err = -ENOMEM;
                goto _exit;
            }
        }
        memcpy(h264_accum_buf + h264_accum_offset, src, src_len);
        h264_accum_offset += src_len;
    }
    if (state == STREAM_EOF) {
        u32 data_len = h264_accum_offset;
        int found_cc = -1;
        for (int ci = 0; ci + 3 < (int)data_len; ci++) {
            if (h264_accum_buf[ci] == 0x55 && h264_accum_buf[ci + 1] == 0xAA && h264_accum_buf[ci + 2] == 0x66 && h264_accum_buf[ci + 3] == 0xCC) {
                found_cc = ci;
                break;
            }
        }
        if (found_cc >= 0) {
            data_len = found_cc;
            while (data_len > 0 && h264_accum_buf[data_len - 1] == 0x00) {
                data_len--;
            }
        }
        int eof_valid = 0;
        if (data_len > 0) {
            /* Always scan for SPS/PPS, regardless of frame validity.
             * Many H264 cameras send SPS/PPS in separate frames before any IDR. */
            cache_h264_sps_pps(h264_accum_buf, data_len);
            eof_valid = h264_validate_frame(h264_accum_buf, data_len);
            if (eof_valid) {
                cache_h264_sps_pps(h264_accum_buf, data_len);
            }
            if (uvc_host_frame_cb) {
                if (!eof_valid && g_h264_sps_len > 0) {
                    u32 out_off = g_h264_sps_len + g_h264_pps_len;
                    if (out_off + data_len <= H264_OUT_BUF_SIZE) {
                        if (!g_h264_out_buf) {
                            g_h264_out_buf = (u8 *)malloc_psram(H264_OUT_BUF_SIZE);
                            if (!g_h264_out_buf) {
                                log_error("[H264_OUT] PSRAM alloc %d FAILED, skip prepend\n", H264_OUT_BUF_SIZE);
                                goto _exit;
                            }
                        }
                        memcpy(g_h264_out_buf, g_h264_sps, g_h264_sps_len);
                        if (g_h264_pps_len > 0) {
                            memcpy(g_h264_out_buf + g_h264_sps_len, g_h264_pps, g_h264_pps_len);
                        }
                        memcpy(g_h264_out_buf + out_off, h264_accum_buf, data_len);
                        uvc_host_frame_cb(g_h264_out_buf, out_off + data_len, 1);
                    } else {
                        log_error("[H264_OUT] frame too large for prepend: %d+%d > %d\n", data_len, out_off, H264_OUT_BUF_SIZE);
                        uvc_host_frame_cb(h264_accum_buf, data_len, 1);
                    }
                } else {
                    uvc_host_frame_cb(h264_accum_buf, data_len, 1);
                }
            }
        }
        fh->frame_cnt++;
        h264_accum_offset = 0;
    }

_exit:
    fh->streamoff = 0;
    return err;
}
static int uvc_stream_on(void *device, void *_fh, int index)
{
    int err = 0;
    u8 channel = 0;
    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    os_sem_pend(&fh->sem, 0);

    for (int i = 0; i < UVC_CHANNEL_MAX; i++) {
        if ((u32)(&fh->device[i]) == (u32)device) {
            channel = i + 1;
            break;
        }

    }
    ASSERT(channel, "err int uvc_stream_on\n");
    if (fh->uvc_out) {
        os_sem_post(&fh->sem);
        return -EINVAL;
    }

    fh->eof = 1;
    fh->drop_frame = 0;

    if (fh->streamon) {
        fh->streamon++;
        fh->streamon_ch[channel - 1]++;
        os_sem_post(&fh->sem);
        return 0;
    }
    err = uvc_host_open_camera(fh->private_data);// Open USB and configure UVC video stream
    if (err) {
        log_error("uvc_stream_on err");
        os_sem_post(&fh->sem);
        return err;
    }
    fh->streamon++;
    fh->streamon_ch[channel - 1]++;
    if (fh->uvc_video_q) {
        videobuf_streamon(&fh->video_q, (u8 *)&channel);
    }
    os_sem_post(&fh->sem);
    return err;
}
static int uvc_set_real_fps(void *_fh, u8 fp)
{
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    fh->real_fps = fp;
    return 0;
}
static int uvc_stream_off(void *device, void *_fh, int index)
{
    int err = 0;
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    u32 time = jiffies + msecs_to_jiffies(10);
    u8 channel = 0;

    os_sem_pend(&fh->sem, 0);
    for (int i = 0; i < UVC_CHANNEL_MAX; i++) {
        if ((u32)(&fh->device[i]) == (u32)device) {
            channel = i + 1;
            break;
        }
    }
    ASSERT(channel, "err int uvc_stream_off\n");
    fh->streamon_ch[channel - 1]--;
    if (fh->streamon && --fh->streamon) {
        os_sem_post(&fh->sem);
        return 0;
    }
    if (!fh->uvc_out) {
        uvc_host_close_camera(fh->private_data);// Close UVC video stream
    }
    fh->streamoff = 1;
    //cppcheck-suppress knownConditionTrueFalse
    while (fh->streamoff) {
        if (time_after(jiffies, time)) {
            break;
        }
    }

    if (fh->buf) {
        if (fh->buf != fh->share_buf) {
            uvc_buf_free(fh, fh->buf, fh->alloc_channel);
        }
        fh->buf = NULL;
        fh->b_offset = 0;
    }
    if (fh->uvc_video_q) {
        err = videobuf_streamoff(&fh->video_q, 0);
    }
    os_sem_post(&fh->sem);
    return err;

}

extern void uvc_host_set_target_reso(u16 width, u16 height);

static int uvc_dev_init(const struct dev_node *node, void *_data)
{
    struct uvc_platform_data *data = (struct uvc_platform_data *)_data;
    if (data && data->width && data->height) {
        uvc_host_set_target_reso(data->width, data->height);
    }
    os_sem_create(&__this->sem, 1);
    INIT_LIST_HEAD(&__this->dev_list);
    return 0;
}

static bool uvc_dev_online(const struct dev_node *node)
{
    int id;

    id = uvc_host_online();
    if (id < 0 || id >= USB_MAX_HW_NUM) {
        return false;  /* Invalid or out-of-range USB ID */
    }

    return true;
}

static int uvc_dev_out(void *fd)
{
    struct uvc_fh *fh = (struct uvc_fh *)fd;

    os_sem_pend(&fh->sem, 0);

    fh->uvc_out = 1;

    os_sem_post(&fh->sem);
    return 0;

}

static int uvc_dev_open(const char *_name, struct device **device, void *arg)
{
    struct uvc_fh *fh = NULL;
    struct uvc_host_param param = {0};
    char name[8];
    int id, i;
    u8 channel = 0;

    struct video_var_param_info *info = (struct video_var_param_info *)arg;
    log_info("[UVC_FLOW] uvc_dev_open: _name=%s uvc_id=%d pixelformat=0x%x\n", _name, info->f->uvc_id, info->f->pixelformat);
    id = info->f->uvc_id;
    list_for_each_uvc(fh) {
        if (fh->id == id) {// Already opened: use another channel
            for (i = 0; i < UVC_CHANNEL_MAX; i++) {
                if (!fh->open[i]) {
                    channel = i;
                    break;
                }
            }
            if (i >= UVC_CHANNEL_MAX) {
                log_error("uvc no channel");
                os_sem_post(&__this->sem);
                return -EINVAL;
            }
            *device = &fh->device[channel];
            (*device)->private_data = fh;
            fh->video_dev[channel] = (struct video_device *)info->priv;
            fh->open[channel] = 1;
            fh->channel++;
            os_sem_post(&__this->sem);
            return 0;
        }
    }

    fh = (struct uvc_fh *)zalloc(sizeof(*fh));
    if (!fh) {
        os_sem_post(&__this->sem);
        return -ENOMEM;
    }
    sprintf(name, "uvc%d", id);
    param.name = name;//node->name;
    if (info->f->pixelformat & VIDEO_PIX_FMT_H264) {
        param.uvc_stream_out = uvc_h264_stream_out;
    } else {
        param.uvc_stream_out = uvc_mjpg_stream_out;
    }
    param.uvc_out = uvc_dev_out;
    param.priv = fh;
    log_info("[UVC_FLOW] uvc_host_open: name=%s pixelformat=0x%x", name, info->f->pixelformat);
    fh->private_data = uvc_host_open(&param);// Initialize UVC host
    if (!fh->private_data) {
        free(fh);
        os_sem_post(&__this->sem);
        return -EINVAL;
    }
    if (info->priv) {
        fh->video_dev[channel] = (struct video_device *)info->priv;
    }
#if UVC_RECV_BUFF_SHARE
    if (!fh->share_buf) {
        fh->share_buf = malloc(UVC_RECV_BUFF_SHARE_SIZE);
        if (!fh->share_buf) {
            log_error("uvc share_buf no men");
            free(fh);
            os_sem_post(&__this->sem);
            return -ENOMEM;
        }
    }
#endif
    os_sem_create(&fh->sem, 0);
    list_add_uvc(fh);// Add to list after open; multi-channel open requires list traversal
    os_sem_post(&fh->sem);
    *device = &fh->device[channel];
    (*device)->private_data = fh;
    fh->id = id;
    fh->open[channel] = 1;
    fh->channel++;
    os_sem_post(&__this->sem);
    return 0;
}

static int uvc_querycap(struct uvc_fh *fh, struct uvc_capability *cap)// Get supported resolutions
{
    int num;

    struct uvc_frame_info *reso_table;

    os_sem_pend(&fh->sem, 0);
    num = uvc_host_get_pix_table(fh->private_data, &reso_table);

    if (!reso_table || num <= 0) {
        os_sem_post(&fh->sem);
        return -EINVAL;  /* No resolution table available */
    }
    if (num > (sizeof(cap->reso) / sizeof(cap->reso[0]))) {
        num = sizeof(cap->reso) / sizeof(cap->reso[0]);
    }

    memcpy(cap->reso, reso_table, sizeof(cap->reso[0]) * num);
    cap->fmt = UVC_CAMERA_FMT_MJPG;
    cap->reso_num = num;
    cap->fps = uvc_host_get_fps(fh->private_data);
    fh->fps = cap->fps;
    os_sem_post(&fh->sem);
    return 0;
}

static int uvc_req_processing_unit(struct uvc_fh *fh, struct uvc_processing_unit *pu)
{
    int err = 0;

    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc_host_req_processing_unit(fh->private_data, pu);
    }

    os_sem_post(&fh->sem);
    return err;
}

static int uvc_dev_reset(struct uvc_fh *fh)
{
    int err = 0;
    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc_force_reset(fh->private_data);
    }

    os_sem_post(&fh->sem);

    return err;
}

static int usb_ioctl(struct uvc_fh *fh, u32 cmd, void *arg)
{
    int err = 0;
    if (!fh) {
        return -EINVAL;
    }
    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc2usb_ioctl(fh->private_data, cmd, arg);
    }

    os_sem_post(&fh->sem);

    return err;
}

static int uvc_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int ret = 0;
    struct uvc_fh *fh = (struct uvc_fh *)device->private_data;

    switch (cmd) {
    case UVCIOC_QUERYCAP:
        ret = uvc_querycap(fh, (struct uvc_capability *)arg);
        break;
    case UVCIOC_SET_CAP_SIZE:
        ret = uvc_host_set_pix(fh->private_data, (int)arg);
        break;
    case UVCIOC_REQBUFS:
        ret = uvc_dev_reqbufs(fh, (struct uvc_reqbufs *)arg);
        break;
    case UVCIOC_DQBUF:
        ret = uvc_dev_dqbuf(fh, (struct video_buffer *)arg);
        break;
    case UVCIOC_QBUF:
        ret = uvc_dev_qbuf(fh, (struct video_buffer *)arg);
        break;
    case UVCIOC_STREAM_ON:
        ret = uvc_stream_on(device, fh, arg);
        break;
    case UVCIOC_STREAM_OFF:
        ret = uvc_stream_off(device, fh, arg);
        break;
    case UVCIOC_GET_IMAGE:
        ret = uvc_img_cap(fh, arg);
        break;
    case UVCIOC_RESET:
        ret = uvc_dev_reset(fh);
        break;
    case UVCIOC_REQ_PROCESSING_UNIT:
        ret = uvc_req_processing_unit(fh, (struct uvc_processing_unit *)arg);
        break;

    case UVCIOC_SET_CUR_GRAY:
        /*set_uvc_gray(arg);*/
        break;
    case UVCIOC_SET_CUR_FPS:
        uvc_set_real_fps(fh, (u8)arg);
        break;
    default:
        ret = usb_ioctl(fh, cmd, (void *)arg);
        break;
    }

    return ret;
}

static int uvc_dev_close(struct device *device)
{
    struct uvc_fh *fh = (struct uvc_fh *)device->private_data;
    if (fh) {
        if (fh->channel) {
            fh->channel--;
        }
    }
    if (fh && fh->channel == 0) {// All channels closed
        os_sem_pend(&fh->sem, 0);
        uvc_host_close(fh->private_data);
        for (int i = 0; i < UVC_CHANNEL_MAX; i++) {// Close all channels
            if (&fh->device[i] == device) {// Close current channel
                fh->open[i] = 0;
                fh->video_dev[i] = NULL;
                memset(&fh->device[i], 0, sizeof(struct device));
            }
        }
        if (fh->uvc_video_q) {
            videobuf_queue_release(&fh->video_q);
        }
        list_del_uvc(fh);
        if (fh->share_buf) {
            free(fh->share_buf);
            fh->share_buf = NULL;
        }
        os_sem_post(&fh->sem);
        free(fh);
    }
    return 0;
}

#if TCFG_HOST_UVC_ENABLE
static u8 uvc_already_opened = 0;
static struct uvc_fh *uvc_notify_fh = NULL;
int uvc_dev_notify_online(char *name)
{
    log_info("[UVC_FLOW] uvc_dev_notify_online: name=%s already_opened=%d\n", name, uvc_already_opened);
    if (uvc_already_opened) {
        log_info("[UVC_FLOW] uvc already opened, skip duplicate open\n");
        return 0;
    }
    uvc_already_opened = 1;
    struct uvc_fh *fh = (struct uvc_fh *)zalloc(sizeof(*fh));
    if (!fh) {
        log_error("[UVC_FLOW] uvc fh alloc failed\n");
        uvc_already_opened = 0;
        return -1;
    }
    os_sem_create(&fh->sem, 0);
    uvc_notify_fh = fh;
    struct uvc_host_param param = {0};
    param.name = name;
    param.priv = fh;
    extern u8 uvc_host_is_h264_format(void);
    if (uvc_host_is_h264_format()) {
        param.uvc_stream_out = uvc_h264_stream_out;
        log_info("[UVC_FLOW] Using H264 stream_out\n");
    } else {
        param.uvc_stream_out = uvc_mjpg_stream_out;
        log_info("[UVC_FLOW] Using MJPEG stream_out\n");
    }
    param.uvc_out = NULL;
    /* extern void video_sd_write_init(void); */
    /* video_sd_write_init(); */
    extern int clock_lock(const char *name, unsigned int clk);
    clock_lock("uvc_stream", 240000000);
    void *fd = uvc_host_open(&param);
    if (fd) {
        fh->private_data = fd;
        /* log_info("[UVC_FLOW] uvc_host_open success, fd=%p fh=%p\n", fd, fh); */
        /* uvc_host_open_camera(fd); */
        /* log_info("[UVC_FLOW] uvc_host_open_camera done\n"); */
        return 0;
    } else {
        log_error("[UVC_FLOW] uvc_host_open FAILED\n");
        uvc_already_opened = 0;
        free(fh);
        uvc_notify_fh = NULL;
        return -1;
    }
}
#endif

const struct device_operations uvc_dev_ops = {
    .init  =  uvc_dev_init,
    .open  =  uvc_dev_open,
    .ioctl =  uvc_dev_ioctl,
    .close =  uvc_dev_close,
    .online = uvc_dev_online,
};
#endif
