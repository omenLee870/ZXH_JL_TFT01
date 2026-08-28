#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "uvc_device.h"
#include "usb_config.h"
#include "host_uvc.h"
#include "video_ioctl.h"
#include "video.h"
#include "videobuf.h"
#include "sdk_config.h"

#include "debug.h"
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE



#define UVC_JPEG_BUF_SIZE  (30 * 1024) 	//限制jpeg 大小为30KB,(need enable)

extern void jpeg_dec_flush(void);

u8 *jluvc_jpeg_buf = NULL;
int jluvc_jpeg_buf_len = 0;

static u8 jluvc_start = 0;
static OS_SEM jluvc_sem;

struct uvc_cam_jpeg_t {
    volatile u8 streamon;
    u16 cur_width;
    u16 cur_height;
    void (*ui_refresh_cb)(void);
};
struct uvc_cam_jpeg_t _uvc_cam_jpeg = {0};

#define __this   (&_uvc_cam_jpeg)

static int find_reso(u16 ref_width, u16 ref_height, struct uvc_capability *uvc_cap)
{
    u8 find_width = 0;
    u8 find_height = 0;
    int w_index = 0;
    int h_index = 0;
    int i;

    while (h_index < uvc_cap->reso_num) {
        for (i = w_index; i < uvc_cap->reso_num; i++) {
            if (ref_width == uvc_cap->reso[i].width) {
                find_width = 1;
                w_index = i;
            }
        }

        for (i = h_index; i < uvc_cap->reso_num; i++) {
            if (ref_height == uvc_cap->reso[i].height) {
                find_height = 1;
                h_index = i;
                break;
            }
        }

        if (find_width && find_height) {
            if (uvc_cap->reso[h_index].width == ref_width) {
                return h_index;
            } else if (uvc_cap->reso[w_index].height == ref_height) {
                return w_index;
            } else {
                w_index++;
                h_index++;
                continue;
            }
        } else if (find_height) {
            return h_index;
        } else if (find_width) {
            return w_index;
        } else {
            return 0;
        }
    }

    return 0;
}

/**
 * @brief 设置JLUVC运行状态
 *
 * 设置JLUVC的运行状态，0表示未运行，1表示运行中
 *
 * @param status 要设置的运行状态
 */
void jlset_uvc_status(u8 status)
{
    log_debug("set_uvc_status: %d\n", status);
    jluvc_start = status;
}

/**
 * @brief 获取JLUVC运行状态
 *
 * 返回JLUVC当前的运行状态，0表示未运行，1表示运行中
 *
 * @return u8 JLUVC当前的运行状态
 */
u8 jlget_uvc_status(void)
{
    return jluvc_start;
}

/**
 * @brief 创建JLUVC信号量
 *
 * 初始化并创建JLUVC使用的信号量资源
 */
void jluvc_sem_create(void)
{
    os_sem_create(&jluvc_sem, 0);
}

/**
 * @brief 等待JLUVC信号量
 *
 * 阻塞当前线程，直到JLUVC信号量的计数大于0。
 * 当信号量计数大于0时，线程被唤醒并继续执行。
 */
void jluvc_sem_pend(void)
{
    os_sem_pend(&jluvc_sem, 0);
}

/**
 * @brief 释放JLUVC信号量
 *
 * 通过增加信号量计数来释放信号量，允许等待该信号量的线程继续执行
 */
void jluvc_sem_post(void)
{
    os_sem_post(&jluvc_sem);
}

/**
 * @brief 删除JLUVC信号量
 *
 * 释放并删除JLUVC使用的信号量资源
 */
void jluvc_sem_del(void)
{
    os_sem_del(&jluvc_sem, 0);
}

/**
 * @brief 初始化JLUVC缓冲区
 *
 * 分配指定大小的内存块用于JLUVC操作
 *
 * @param size 要分配的内存大小(字节)
 * @return 成功时返回指向分配内存的指针，失败时返回NULL
 */
void *jluvc_buf_init(u16 size)
{
    log_debug("-----%s>>\n", __func__);
    void *ptr = NULL;
    if (ptr == NULL) {
        ptr = malloc(size);
    }
    return ptr;
}

/**
 * @brief 释放JLUVC缓冲区
 *
 * 释放之前分配的JPEG缓冲区内存，并将缓冲区指针置为NULL
 */
void jluvc_buf_deinit(void)
{
    log_debug("-----%s>>\n", __func__);
    if (jluvc_jpeg_buf) {
        free(jluvc_jpeg_buf);
        jluvc_jpeg_buf = NULL;
    }
}

/**
 * @brief 刷新JLUVC界面显示
 *
 * 该函数用于向UI任务发送刷新消息，触发UVC JPEG测试界面的刷新操作。
 * 通过向"ui"任务发送Q_CALLBACK类型消息来实现界面更新。
 *
 * @note 该函数通过任务间通信机制实现界面刷新，不直接操作UI组件
 */
static void jluvc_ui_reflush(void)
{
    if (__this && __this->ui_refresh_cb) {
        __this->ui_refresh_cb();
    }
}
int jluvc_set_refresh_cb(void (*cb)(void))
{
    if (!__this) {
        log_error("%s not find\n", __func__);
        return -1;
    }
    __this->ui_refresh_cb = cb;
    return 0;
}

/**
 * @brief UVC JPEG接收任务处理函数
 * 该函数负责初始化UVC设备，接收JPEG格式的视频流数据，并进行处理。
 * 主要功能包括：设备初始化、视频流控制、JPEG数据接收和处理等。
 * @param arg 任务参数指针，未使用
 * @return 无返回值
 * @note 该任务为循环执行的任务，在系统退出时会进行资源释放
 */
static void uvc_jpeg_recv_task(void *arg)
{
    extern void video_sd_write_init(void);
    video_sd_write_init();
    void *fd = NULL;
    struct uvc_reqbufs breq = {0};
    struct video_buffer b = {0};
    b.timeout = 50;
    b.noblock = 0;
    struct uvc_capability uvc_cap;
    int err = -1;
    int msg[32];
    u32 i = 0;
    if (dev_online("uvc")) {
        const struct video_platform_data *video_data = NULL;
        struct uvc_platform_data *uvc_info = NULL;
        struct video_var_param_info info;
        struct video_format video_f;
        char video_name[16] = {0};
        sprintf(video_name, "video%d", 0);
        extern void *device_platform_data_get(const char *name);
        video_data = device_platform_data_get(video_name);
        if (video_data && video_data->data) {
            for (int j = 0; j < video_data->num; j++) {
                if (video_data->data[j].tag == VIDEO_TAG_UVC) {
                    uvc_info = video_data->data[j].data;
                    break;
                }
            }
        }
        video_f.uvc_id  = 0;
        info.f = &video_f;
        fd = dev_open("uvc", (void *)&info); //指定uvc设备0
        if (fd) {
            dev_ioctl(fd, UVCIOC_QUERYCAP, (unsigned int)&uvc_cap);

            for (int k = 0; k < uvc_cap.reso_num; k++) {
            }

            u8 fmt = uvc_cap.fmt;
            int reso_index = find_reso(uvc_info->width, uvc_info->height, &uvc_cap);
            u16 width = uvc_cap.reso[reso_index].width;
            u16 height = uvc_cap.reso[reso_index].height;
            log_debug("fmt :%d reso : %d, ref size : %d x %d, size : %d x %d.\n", fmt, reso_index, uvc_info->width, uvc_info->height, width, height);

            dev_ioctl(fd, UVCIOC_SET_CAP_SIZE, (unsigned int)(reso_index + 1));

            breq.buf = NULL;
            breq.size = uvc_info->mem_size;

            err = dev_ioctl(fd, UVCIOC_REQBUFS, (unsigned int)&breq);
            if (!err) {
                dev_ioctl(fd, UVCIOC_STREAM_ON, 0);
            } else {
                err = -1;
                log_error("video open err1");
            }
        }
    }
    jluvc_sem_create();//create sem
    u8 usr_wdt_cnt = 0;//定时喂狗
    while (1) {
        os_taskq_accept(ARRAY_SIZE(msg), msg);
        /* os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); */
        if (msg[0] == Q_MSG) {
            if (msg[1] == 0xaa) {
                if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
                    if (fd) {
                        dev_ioctl(fd, UVCIOC_STREAM_OFF, 0);
                        dev_close(fd);
                    }
                    os_task_del_res(OS_TASK_SELF);
                }
            }
        }
        if (usr_wdt_cnt++ >= 20) {
            usr_wdt_cnt = 0;
            wdt_clear();
        }
        //从uvc 缓存中请求一帧图像
        if (fd) {
            err = dev_ioctl(fd, UVCIOC_DQBUF, (unsigned int)&b);
        }
        if (err || b.len <= 0) { //请求失败
            os_time_dly(1);
            continue;
        }
        //请求成功
        log_debug("[msg]>>>>>>>>>>>b.len=%d", b.len);
        /* if (jluvc_jpeg_buf [> && (b.len < UVC_JPEG_BUF_SIZE)<]) { //UI刷新数据 */
        if (1 /* && (b.len < UVC_JPEG_BUF_SIZE)*/) { //UI刷新数据
            jlset_uvc_status(1);

            /* memset(jluvc_jpeg_buf, 0, b.len); */
            /* memcpy(jluvc_jpeg_buf, (u8 *)b.baddr, b.len); */
            /* jluvc_jpeg_buf_len = b.len; */
#if TCFG_UI_ENABLE
            int jljpeg_stream_src_data_copy(u8 * buf, int size);
            if (!jljpeg_stream_src_data_copy((u8 *)b.baddr, b.len)) {
                jluvc_ui_reflush();
            }
#endif
            /* jluvc_sem_post(); */
        }

        // put_buf((u8 *)b.baddr+ b.len -512 ,512);
        /* if (f) { */
        /* fwrite(f, b.baddr, b.len); */
        /* } */

        dev_ioctl(fd, UVCIOC_QBUF, (unsigned int)&b);
    }

}

int video_uvc_jpeg_stream_open(u16 width, u16 height)
{
    log_debug("video_uvc_jpeg_stream_open\n");
    if (__this->streamon) {
        return -1;
    }
    memset(__this, 0x00, sizeof(struct uvc_cam_jpeg_t));
    __this->cur_width = width;
    __this->cur_height = height;
    __this->streamon = 1;
    return os_task_create(uvc_jpeg_recv_task, NULL, 20, 1024, 32, "uvc_jpeg_recv");
}

int video_uvc_jpeg_stream_close()
{
    while (OS_TASK_NOT_EXIST != os_task_del_req((const char *)"uvc_jpeg_recv")) {
        os_taskq_post_msg((const char *)"uvc_jpeg_recv", 1,  0xaa);
        os_time_dly(1);
    }
    jluvc_sem_del();
    jluvc_buf_deinit();
    __this->streamon = 0;

    return 0;
}
// Conditional alloc based on PSRAM enable macro
#if TCFG_PSRAM_DEV_ENABLE
#define UVC_MALLOC(s)   malloc_psram(s)
#define UVC_FREE(p)     free_psram(p)
#else
#define UVC_MALLOC(s)   malloc(s)
#define UVC_FREE(p)     free(p)
#endif


// ==================== SD Card Write Module ====================
#define VIDEO_RING_SIZE (1024 * 1024)
#define VIDEO_FRAME_QUEUE_SIZE 64

static u8 *video_ring_mem = NULL;
static u8 *video_frame_buf = NULL;
static volatile u32 ring_write_pos = 0;
static volatile u32 ring_read_pos = 0;
static u32 video_frame_queue[VIDEO_FRAME_QUEUE_SIZE];
static volatile u32 fq_head = 0;
static volatile u32 fq_tail = 0;
#define VIDEO_FRAME_BUF_SIZE (1024 * 1024)
static OS_SEM video_sd_sem;
static u8 video_sd_task_created = 0;

static u32 video_ring_free_space(void)
{
    u32 w = ring_write_pos;
    u32 r = ring_read_pos;
    if (w >= r) {
        return VIDEO_RING_SIZE - (w - r) - 1;
    } else {
        return r - w - 1;
    }
}

static void video_ring_write(const u8 *data, u32 len)
{
    if (!video_ring_mem || len == 0) {
        return;
    }
    if (len > video_ring_free_space()) {
        static u32 drop_cnt = 0;
        drop_cnt++;
        if (drop_cnt <= 5 || (drop_cnt % 100 == 0)) {
            log_info("[VIDEO_RING] full, drop #%d len=%d free=%d\n", drop_cnt, len, video_ring_free_space());
        }
        return;
    }
    u32 w = ring_write_pos;
    u32 first_chunk = VIDEO_RING_SIZE - w;
    if (first_chunk >= len) {
        memcpy(video_ring_mem + w, data, len);
    } else {
        memcpy(video_ring_mem + w, data, first_chunk);
        memcpy(video_ring_mem, data + first_chunk, len - first_chunk);
    }
    ring_write_pos = (w + len) % VIDEO_RING_SIZE;
    u32 next_tail = (fq_tail + 1) % VIDEO_FRAME_QUEUE_SIZE;
    if (next_tail != fq_head) {
        video_frame_queue[fq_tail] = len;
        fq_tail = next_tail;
    } else {
        log_info("[VIDEO_RING] frame queue full, drop len=%d\n", len);
    }
    os_sem_post(&video_sd_sem);
}

static void video_sd_write_task(void *priv)
{
    extern void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);
    sdmmc_dev_lowpower_disable("sd0", 1);
    static u8 recorder_running = 0;
    if (recorder_running) {
        log_info("[H264_WRITE] recorder already running, skip\n");
        return;
    }
    recorder_running = 1;
    extern u8 uvc_host_is_h264_format(void);
    u8 is_h264 = uvc_host_is_h264_format();
    u32 total = 0;
    u32 frame_cnt = 0;

    static u32 h264_file_idx = 1;
    char h264_fname[64];
    sprintf(h264_fname, "storage/sd0/C/h264_%d.h264", h264_file_idx);
    if (is_h264) {
        FILE *fp = fopen("storage/sd0/C/h264_0.h264", "w+b");
        if (!fp) {
            log_info("[H264_WRITE] FAILED to open storage/sd0/C/h264_0.h264\n");
            while (1) {
                os_time_dly(100);
            }
        }
        log_info("[H264_WRITE] opened ok, fp=%p\n", (void *)fp);
        while (1) {
            os_sem_pend(&video_sd_sem, 0);
            while (fq_head != fq_tail) {
                u32 len = video_frame_queue[fq_head];
                fq_head = (fq_head + 1) % VIDEO_FRAME_QUEUE_SIZE;
                u32 r = ring_read_pos;
                /* Copy out of ring BEFORE slow fwrite, release ring space immediately */
                if (len > VIDEO_FRAME_BUF_SIZE) {
                    log_info("[H264_WRITE] SKIP frame too large: %d > %d\n", len, VIDEO_FRAME_BUF_SIZE);
                    ring_read_pos = (r + len) % VIDEO_RING_SIZE;
                    continue;
                }
                if (r + len <= VIDEO_RING_SIZE) {
                    memcpy(video_frame_buf, video_ring_mem + r, len);
                } else {
                    u32 first = VIDEO_RING_SIZE - r;
                    memcpy(video_frame_buf, video_ring_mem + r, first);
                    memcpy(video_frame_buf + first, video_ring_mem, len - first);
                }
                ring_read_pos = (r + len) % VIDEO_RING_SIZE;
                size_t written = fwrite(video_frame_buf, 1, len, fp);
                if (written != len) {
                    log_info("[H264_WRITE] fwrite ERR wanted=%d written=%d ftell=%d\n", (int)len, (int)written, (int)ftell(fp));
                    total = ftell(fp);
                } else {
                    total += written;
                }
                frame_cnt++;
                if (frame_cnt <= 5 || (frame_cnt % 100 == 0)) {
                    log_info("[H264_WRITE] frame#%d wrote=%d/%d total=%d ftell=%d\n", frame_cnt, (int)written, (int)len, (int)total, (int)ftell(fp));
                }
            }
            if (frame_cnt >= 100) {
                fclose(fp);
                log_info("[H264_WRITE] DONE: %d frames, %d bytes (frame limit)\n", frame_cnt, total);
                while (1) {
                    os_time_dly(100);
                }
            }
        }
    } else {
        u32 file_idx = 0;
        while (1) {
            os_sem_pend(&video_sd_sem, 0);
            while (fq_head != fq_tail) {
                u32 len = video_frame_queue[fq_head];
                fq_head = (fq_head + 1) % VIDEO_FRAME_QUEUE_SIZE;
                u32 r = ring_read_pos;
                /* Copy out of ring BEFORE slow fwrite, release ring space immediately */
                if (len > VIDEO_FRAME_BUF_SIZE) {
                    log_info("[JPEG_WRITE] SKIP frame too large: %d > %d\n", len, VIDEO_FRAME_BUF_SIZE);
                    ring_read_pos = (r + len) % VIDEO_RING_SIZE;
                    frame_cnt++;
                    continue;
                }
                if (r + len <= VIDEO_RING_SIZE) {
                    memcpy(video_frame_buf, video_ring_mem + r, len);
                } else {
                    u32 first = VIDEO_RING_SIZE - r;
                    memcpy(video_frame_buf, video_ring_mem + r, first);
                    memcpy(video_frame_buf + first, video_ring_mem, len - first);
                }
                ring_read_pos = (r + len) % VIDEO_RING_SIZE;
                char fname[48];
                snprintf(fname, sizeof(fname), "storage/sd0/C/img_%04d.jpg", file_idx);
                FILE *fp = fopen(fname, "wb");
                if (!fp) {
                    log_info("[JPEG_WRITE] FAILED to open %s\n", fname);
                    frame_cnt++;
                    continue;
                }
                size_t written = fwrite(video_frame_buf, 1, len, fp);
                if (written != len) {
                    log_info("[JPEG_WRITE] fwrite ERR wanted=%d written=%d\n", (int)len, (int)written);
                }
                fclose(fp);
                total += len;
                frame_cnt++;
                file_idx++;
                if (frame_cnt % 100 == 0) {
                    log_info("[JPEG_WRITE] %d frames, %d bytes, last=%s\n", frame_cnt, total, fname);
                }
            }
        }
    }
}

static void uvc_frame_cb(const u8 *data, u32 len, u8 is_h264)
{
    video_ring_write(data, len);
}

static void video_sd_write_init(void)
{
    if (video_sd_task_created) {
        return;
    }
    video_sd_task_created = 1;

    /* memory debug */
    extern void psram_heap_stats(void);
    extern void mem_stats(void);
    log_info("[VIDEO_INIT] === SRAM heap ===\n");
    mem_stats();
    log_info("[VIDEO_INIT] === PSRAM heap ===\n");
    psram_heap_stats();
    log_info("[VIDEO_INIT] requesting %d bytes from PSRAM\n", VIDEO_RING_SIZE);

    video_ring_mem = UVC_MALLOC(VIDEO_RING_SIZE);
    log_info("[VIDEO_INIT] ring_mem=%p size=%d (PSRAM)\n", video_ring_mem, VIDEO_RING_SIZE);
    if (!video_ring_mem) {
        log_info("[VIDEO_INIT] malloc_psram ring FAILED, trying internal SRAM...\n");
        video_ring_mem = malloc(VIDEO_RING_SIZE);
        log_info("[VIDEO_INIT] ring_mem=%p size=%d (SRAM fallback)\n", video_ring_mem, VIDEO_RING_SIZE);
        if (!video_ring_mem) {
            log_info("[VIDEO_INIT] malloc ring FAILED (both pools)\n");
            video_sd_task_created = 0;
            return;
        }
    }
    memset(video_ring_mem, 0, VIDEO_RING_SIZE);
    log_info("[VIDEO_INIT] requesting %d bytes frame buffer from PSRAM\n", VIDEO_FRAME_BUF_SIZE);
    video_frame_buf = UVC_MALLOC(VIDEO_FRAME_BUF_SIZE);
    log_info("[VIDEO_INIT] frame_buf=%p size=%d (PSRAM)\n", video_frame_buf, VIDEO_FRAME_BUF_SIZE);
    if (!video_frame_buf) {
        log_info("[VIDEO_INIT] PSRAM frame FAILED, trying SRAM...\n");
        video_frame_buf = malloc(VIDEO_FRAME_BUF_SIZE);
        log_info("[VIDEO_INIT] frame_buf=%p size=%d (SRAM fallback)\n", video_frame_buf, VIDEO_FRAME_BUF_SIZE);
        if (!video_frame_buf) {
            log_info("[VIDEO_INIT] FAILED to alloc frame buffer!\n");
            free(video_ring_mem);
            video_ring_mem = NULL;
            video_sd_task_created = 0;
            return;
        }
    }
    ring_write_pos = 0;
    ring_read_pos = 0;
    fq_head = 0;
    fq_tail = 0;
    os_sem_create(&video_sd_sem, 0);
    extern void uvc_host_frame_cb_register(void (*cb)(const u8 * data, u32 len, u8 is_h264));
    uvc_host_frame_cb_register(uvc_frame_cb);
    os_task_create(video_sd_write_task, NULL, 10, 2048, 0, "video_sd");
}

