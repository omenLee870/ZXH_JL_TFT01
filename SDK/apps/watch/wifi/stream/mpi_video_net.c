#include "system/includes.h"
#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE

#if (defined TCFG_CAMERA_DVP_ENABLE) && (TCFG_CAMERA_DVP_ENABLE == 1)

/* ============================================================================
 * DVP Camera 路径: 使用 app_camera_* API (VI -> VPSS -> VENC -> UDP 推流)
 * ============================================================================ */

#include "camera/app_camera_mpi.h"


static app_camera_handle_t g_camera_hdl = NULL;
static int g_stream_status = 0;

/* cached params from last open, for VIDEO_CTRL reuse */
static char g_last_path[APP_CAMERA_REC_PATH_MAX];
static int g_last_width = 640;
static int g_last_height = 480;
static int g_last_fps = 25;
static int g_last_abr = 2000;


int mpi_video_net_open_stream(const char *path, int w, int h, int fps, int abr)
{
    app_camera_ret_t ret;
    app_camera_net_stream_attr_t attr;

    if (g_stream_status) {
        printf("mpi_video_net: stream already opened\n");
        return g_stream_status;
    }

    /* open camera on first use */
    if (g_camera_hdl == NULL) {
        ret = app_camera_open(0, "GC0308", &g_camera_hdl);
        if (ret != APP_CAMERA_OK) {
            printf("mpi_video_net: failed to open camera, ret=%d\n", ret);
            return -1;
        }
        printf("mpi_video_net: camera opened\n");
    }

    /* cache params for VIDEO_CTRL */
    if (path && path[0]) {
        strncpy(g_last_path, path, sizeof(g_last_path) - 1);
        g_last_path[sizeof(g_last_path) - 1] = '\0';
    }
    g_last_width = w;
    g_last_height = h;
    g_last_fps = fps;
    g_last_abr = abr;

    /* build attr and start stream */
    memset(&attr, 0, sizeof(attr));
    strncpy(attr.path, path, sizeof(attr.path) - 1);
    attr.width = (uint16_t)w;
    attr.height = (uint16_t)h;
    attr.fps = (uint32_t)fps;
    attr.abr_kbps = (uint16_t)abr;
    attr.stream_buf_size = 200 * 1024;
    attr.audio_enable = 0;

    ret = app_camera_net_stream_start(g_camera_hdl, &attr);
    if (ret != APP_CAMERA_OK) {
        printf("mpi_video_net: failed to start net stream, ret=%d\n", ret);
        return -1;
    }

    g_stream_status = 1;
    printf("mpi_video_net: stream started, path=%s, %dx%d@%d, abr=%dkbps\n",
           path, w, h, fps, abr);
    return 0;
}

int mpi_video_net_close_stream(void)
{
    app_camera_ret_t ret;

    if (!g_stream_status) {
        printf("mpi_video_net: stream not opened\n");
        return 0;
    }

    ret = app_camera_net_stream_stop(g_camera_hdl);
    if (ret != APP_CAMERA_OK) {
        printf("mpi_video_net: failed to stop net stream, ret=%d\n", ret);
        return -1;
    }

    g_stream_status = 0;
    printf("mpi_video_net: stream stopped\n");
    return 0;
}

int mpi_video_net_get_status(void)
{
    return g_stream_status;
}

const char *mpi_video_net_get_last_path(void)
{
    return g_last_path;
}

int mpi_video_net_get_last_params(int *w, int *h, int *fps, int *abr)
{
    if (w) {
        *w = g_last_width;
    }
    if (h) {
        *h = g_last_height;
    }
    if (fps) {
        *fps = g_last_fps;
    }
    if (abr) {
        *abr = g_last_abr;
    }
    return 0;
}

/* recording: under DVP, rec == stream (use cached params) */
int mpi_video_net_start_rec(const char *path)
{
    (void)path;
    if (g_stream_status) {
        return 0;
    }
    return mpi_video_net_open_stream(g_last_path, g_last_width,
                                     g_last_height, g_last_fps, g_last_abr);
}

void mpi_video_net_stop_rec(void)
{
    mpi_video_net_close_stream();
}

int mpi_video_net_get_rec_status(void)
{
    return g_stream_status;
}

/* DVP 模式下无 UVC host，分辨率查询接口返回空列表/默认分辨率 */
int mpi_video_net_get_uvc_resolution(int *w, int *h)
{
    if (w) {
        *w = g_last_width;
    }
    if (h) {
        *h = g_last_height;
    }
    return -1;
}

int mpi_video_net_get_reso_count(void)
{
    return 0;
}

int mpi_video_net_get_reso_by_index(int index, int *w, int *h)
{
    (void)index;
    if (w) {
        *w = 0;
    }
    if (h) {
        *h = 0;
    }
    return -1;
}

#elif (defined TCFG_HOST_UVC_ENABLE) && (TCFG_HOST_UVC_ENABLE == 1)

/* ============================================================================
 * UVC Camera 路径:
 *
 *   CONFIG_NET_JPEG  → UVC JPEG → VDEC → VPSS → VENC → {AVI, UDP推流}
 *   !CONFIG_NET_JPEG → UVC H264 → passthrough → {libmov MOV, UDP推流}
 *
 * 双帧缓冲池:
 *   - 2个128KB缓冲区, UVC回调写入非锁定的buf
 *   - Pipeline线程取出, JPEG走编解码流水线, H264直通
 * ============================================================================ */

#include "fs/fs.h"
#include "avilib.h"
#include "mp4_muxer.h"
#include "sock_api/sock_api.h"
#include "lwip/sockets.h"
#include "os/os_api.h"
#include "generic/lbuf.h"
#include "system/spinlock.h"
#include "rt_stream_pkg.h"
#include "usb_host.h"
#include "host_uvc.h"
#include "uvc_device.h"
#include "mpi_common.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "system/timer.h"
#include "rtc/rtc_dev.h"
#include "sys_time.h"
#include "app_camera_osd.h"

/* libmov keyframe flag (from mov-format.h) */
#ifndef MOV_AV_FLAG_KEYFRAME
#define MOV_AV_FLAG_KEYFRAME  0x0001
#endif

#define UVC_JPEG_WIDTH                  640
#define UVC_JPEG_HEIGHT                 480
#define UVC_JPEG_FPS                    25
#define UVC_BUF_SIZE                    (128 * 1024)
#define UVC_BUF_POOL_COUNT              2
#define VDEC_FRAME_BUF_SIZE             (640 * 480 * 2)
#define VDEC_TIMEOUT_MS                 5000
#define VPSS_TIMEOUT_MS                 5000
#define VENC_QUALITY                    50
#define VENC_BUF_SIZE                   (128 * 1024)
#define VENC_STREAM_BUF_SIZE            (200 * 1024)
#define VENC_TIMEOUT_MS                 5000
#define VO_AVI_MAX_FRAMES               1000
#define VO_UDP_PORT                     2224
#define UDP_SEND_BUF_SIZE               (4 * 1460)
#define MAX_PAYLOAD                     (1460 - sizeof(struct frm_head))
#define NET_LBUF_COUNT                  3

#define TAG  "[UVC_NET]"

static char g_last_path[96];
static int  g_last_width  = 640;
static int  g_last_height = 480;
static int  g_last_fps    = 25;
static int  g_last_abr    = 2000;

/* ============================================================================
 * Layer 1: 双帧缓冲池
 * ============================================================================ */

typedef struct {
    u8              *data;
    volatile u32    len;
    u32             capacity;
    volatile u8     locked;
    volatile u8     ready;
    volatile u8     is_h264;
} frame_buf_t;

typedef struct {
    frame_buf_t     bufs[UVC_BUF_POOL_COUNT];
    OS_SEM          frame_sem;
    volatile u32    frame_cnt;
    volatile u32    drop_cnt;
    volatile u32    write_cnt;
} buf_pool_t;

static buf_pool_t *g_pool = NULL;
static volatile u32 g_cb_cnt = 0;

static void uvc_jpeg_frame_cb(const u8 *data, u32 len, u8 is_h264)
{
    buf_pool_t *p = g_pool;
    g_cb_cnt++;
    if (!p || len == 0) {
        return;
    }
    p->frame_cnt++;

    frame_buf_t *t = NULL;
    int i;
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++) {
        if (!p->bufs[i].locked) {
            t = &p->bufs[i];
            break;
        }
    }
    if (!t) {
        p->drop_cnt++;
        return;
    }

    if (len > t->capacity) {
        u8 *n = (u8 *)realloc(t->data, len);
        if (!n) {
            p->drop_cnt++;
            return;
        }
        t->data = n;
        t->capacity = len;
    }

    memcpy(t->data, data, len);
    t->len = len;
    t->is_h264 = is_h264;
    t->ready = 1;
    p->write_cnt++;
    os_sem_post(&p->frame_sem);
}

static int buf_pool_get_frame(buf_pool_t *p, u8 **data, u32 *len, u8 *is_h264, int to_ms)
{
    int i;
    if (os_sem_pend(&p->frame_sem, to_ms) != 0) {
        return -1;
    }
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++) {
        if (p->bufs[i].ready && !p->bufs[i].locked) {
            p->bufs[i].locked = 1;
            p->bufs[i].ready = 0;
            *data = p->bufs[i].data;
            *len = p->bufs[i].len;
            if (is_h264) {
                *is_h264 = p->bufs[i].is_h264;
            }
            return 0;
        }
    }
    return -1;
}

static void buf_pool_unlock(buf_pool_t *p, u8 *data)
{
    int i;
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++)
        if (p->bufs[i].data == data) {
            p->bufs[i].locked = 0;
            return;
        }
}

static int buf_pool_init(buf_pool_t **pp)
{
    buf_pool_t *p = calloc(1, sizeof(buf_pool_t));
    int i;
    if (!p) {
        return -1;
    }
    os_sem_create(&p->frame_sem, 0);
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++) {
        p->bufs[i].data = malloc(UVC_BUF_SIZE);
        if (!p->bufs[i].data) {
            goto err;
        }
        p->bufs[i].capacity = UVC_BUF_SIZE;
    }
    g_pool = p;
    *pp = p;
    uvc_host_frame_cb_register(uvc_jpeg_frame_cb);
    printf(TAG " [BUF] pool: %d x %d bytes\n", UVC_BUF_POOL_COUNT, UVC_BUF_SIZE);
    return 0;
err:
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    free(p);
    return -1;
}

static void buf_pool_deinit(buf_pool_t *p)
{
    int i;
    if (!p) {
        return;
    }
    uvc_host_frame_cb_register(NULL);
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    for (i = 0; i < UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    printf(TAG " [BUF] deinit: cb=%u writes=%u drops=%u\n",
           p->frame_cnt, p->write_cnt, p->drop_cnt);
    free(p);
    g_pool = NULL;
}

/* ============================================================================
 * Layer 2: VDEC — JPEG 解码 (OFFLINE, 非阻塞)
 * ============================================================================ */

typedef struct {
    mpi_vdec_chn_t  chn;
    uint8_t         created, started;
    OS_SEM          sem;
    volatile int    done;
} vdec_t;

static void vdec_notify(void *priv, const mpi_stream_t *s, mpi_ret_t ret)
{
    vdec_t *c = (vdec_t *)priv;
    c->done = (ret == MPI_SUCCESS) ? 1 : -1;
    os_sem_post(&c->sem);
}

static int vdec_create(vdec_t *c, u32 frame_buf_size)
{
    mpi_ret_t r;
    mpi_vdec_chn_attr_t a;
    os_sem_create(&c->sem, 0);
    c->done = 0;
    r = mpi_vdec_find_free_chn(&c->chn);
    if (r) {
        return -1;
    }
    memset(&a, 0, sizeof(a));
    a.hw_dev = 0;
    a.type = MPI_VDEC_TYPE_JPEG;
    a.out_yuv_order = MPI_YUV_ORDER_YUYV;
    a.output_mode = MPI_VDEC_OUTPUT_FRAME;
    a.frame_buf_size = frame_buf_size + 1024;
    r = mpi_vdec_create_chn(c->chn, &a);
    if (r) {
        return -1;
    }
    c->created = 1;
    mpi_vdec_set_block_mode(c->chn, 0, vdec_notify, c);
    r = mpi_vdec_start_chn(c->chn);
    if (r) {
        return -1;
    }
    c->started = 1;
    printf(TAG " [VDEC] chn=%d OFFLINE\n", c->chn);
    return 0;
}

static int vdec_decode(vdec_t *c, u8 *data, u32 len, mpi_video_frame_info_t *f, int to_ms)
{
    mpi_stream_t s;
    int ret;
    c->done = 0;
    os_sem_set(&c->sem, 0);
    memset(&s, 0, sizeof(s));
    s.addr = data;
    s.len = len;
    s.type = MPI_STREAM_TYPE_JPEG;
    ret = mpi_vdec_send_stream(c->chn, &s);
    if (ret) {
        printf(TAG " [VDEC] send: %d\n", ret);
        return -1;
    }
    ret = os_sem_pend(&c->sem, to_ms);
    if (ret || c->done <= 0) {
        printf(TAG " [VDEC] timeout\n");
        return -1;
    }
    memset(f, 0, sizeof(*f));
    ret = mpi_vdec_get_frame(c->chn, f, VDEC_TIMEOUT_MS);
    if (ret) {
        printf(TAG " [VDEC] get_frame: %d\n", ret);
        return -1;
    }
    return 0;
}

static void vdec_destroy(vdec_t *c)
{
    if (c->started) {
        mpi_vdec_stop_chn(c->chn);
    }
    if (c->created) {
        mpi_vdec_destroy_chn(c->chn);
    }
    os_sem_del(&c->sem, OS_DEL_ALWAYS);
}

/* ============================================================================
 * Layer 3: VPSS — OFFLINE 输出
 * ============================================================================ */

typedef struct {
    mpi_vpss_grp_t  grp;
    mpi_vpss_chn_t  chn;
    uint8_t         created, started, chn_enabled;
} vpss_t;

static u32 vpss_buf_sz(u16 w, u16 h, mpi_pixel_format_t fmt)
{
    if (fmt == MPI_PIXEL_FMT_YUV420SP)      {
        return (u32)w * h * 3u / 2u * 2u + 1024u;
    }
    if (fmt == MPI_PIXEL_FMT_YUV422_PACKED) {
        return (u32)w * h * 2u * 2u + 1024u;
    }
    return (u32)w * h * 2u + 1024u;
}

static int vpss_create(vpss_t *c, u16 w, u16 h, u32 fps)
{
    mpi_ret_t r;
    mpi_vpss_grp_attr_t ga;
    mpi_vpss_chn_attr_t ca;

    r = mpi_vpss_find_free_grp(&c->grp);
    if (r) {
        return -1;
    }
    memset(&ga, 0, sizeof(ga));
    ga.src_width = w;
    ga.src_height = h;
    ga.src_format = MPI_VPSS_SRC_FMT_YUV;
    ga.src_pixel_format = MPI_PIXEL_FMT_YUV422_PACKED;
    ga.src_yuv_order = MPI_YUV_ORDER_YUYV;
    ga.src_fps = (u16)fps;
    r = mpi_vpss_create_grp(c->grp, &ga);
    if (r) {
        return -1;
    }
    c->created = 1;
    r = mpi_vpss_start_grp(c->grp);
    if (r) {
        return -1;
    }
    c->started = 1;

    r = mpi_vpss_find_free_chn(c->grp, &c->chn);
    if (r) {
        return -1;
    }
    memset(&ca, 0, sizeof(ca));
    ca.width = w;
    ca.height = h;
    ca.format = MPI_PIXEL_FMT_YUV422_PACKED;
    ca.yuv_order = MPI_YUV_ORDER_YUYV;
    ca.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ca.hw_id = MPI_VPSS_HW_IMC_DST0;
    ca.buf_size = vpss_buf_sz(w, h, ca.format);

    r = mpi_vpss_create_chn(c->grp, c->chn, &ca);
    if (r) {
        return -1;
    }
    r = mpi_vpss_enable_chn(c->grp, c->chn);
    if (r) {
        return -1;
    }
    c->chn_enabled = 1;
    printf(TAG " [VPSS] grp=%d chn=%d OFFLINE %dx%d@%d\n", c->grp, c->chn, w, h, fps);
    return 0;
}

static int vpss_send(vpss_t *c, const mpi_video_frame_info_t *f, int to_ms)
{
    return mpi_vpss_send_frame(c->grp, f, to_ms);
}

static int vpss_get_frame(vpss_t *c, mpi_video_frame_info_t *f, int to_ms)
{
    memset(f, 0, sizeof(*f));
    return mpi_vpss_get_chn_frame(c->grp, c->chn, f, to_ms);
}

static int vpss_release_frame(vpss_t *c, const mpi_video_frame_info_t *f)
{
    return mpi_vpss_release_chn_frame(c->grp, c->chn, f);
}

static void vpss_destroy(vpss_t *c)
{
    if (c->chn_enabled) {
        mpi_vpss_disable_chn(c->grp, c->chn);
    }
    mpi_vpss_destroy_chn(c->grp, c->chn);
    if (c->started) {
        mpi_vpss_stop_grp(c->grp);
    }
    if (c->created) {
        mpi_vpss_destroy_grp(c->grp);
    }
}

/* ============================================================================
 * Layer 4: VENC — JPEG 编码 (OFFLINE 输入)
 * ============================================================================ */

typedef struct {
    mpi_venc_chn_t  chn;
    mpi_venc_dev_t  hw_dev;
    uint8_t         created, started;
} venc_t;

static int venc_create(venc_t *c, u16 w, u16 h, u32 fps)
{
    mpi_ret_t r;
    mpi_venc_chn_attr_t a;
    r = mpi_venc_find_free_chn(&c->chn);
    if (r) {
        return -1;
    }
    c->hw_dev = 0;

    memset(&a, 0, sizeof(a));
    a.hw_dev = c->hw_dev;
    a.type = MPI_VENC_TYPE_JPEG;
    a.input_mode = MPI_VENC_INPUT_OFFLINE;
    a.quality = VENC_QUALITY;
    a.in_width = w;
    a.in_height = h;
    a.in_format = MPI_PIXEL_FMT_YUV422_PACKED;
    a.in_yuv_order = MPI_YUV_ORDER_YUYV;
    a.buf_size = VENC_BUF_SIZE;
    a.fps = fps;
    a.stream_buf_size = VENC_STREAM_BUF_SIZE;

    r = mpi_venc_create_chn(c->chn, &a);
    if (r) {
        return -1;
    }
    c->created = 1;
    r = mpi_venc_start_chn(c->chn);
    if (r) {
        return -1;
    }
    c->started = 1;
    printf(TAG " [VENC] chn=%d OFFLINE %dx%d@%d q=%d\n", c->chn, w, h, fps, VENC_QUALITY);
    return 0;
}

static int venc_encode(venc_t *c, const mpi_video_frame_info_t *f, mpi_stream_t *out, int to_ms)
{
    int r = mpi_venc_send_frame(c->chn, f);
    if (r) {
        printf(TAG " [VENC] send_frame: %d\n", r);
        return -1;
    }
    memset(out, 0, sizeof(*out));
    r = mpi_venc_get_stream(c->chn, out, to_ms);
    if (r) {
        printf(TAG " [VENC] get_stream: %d\n", r);
        return -1;
    }
    return 0;
}

static void venc_destroy(venc_t *c)
{
    if (c->started) {
        mpi_venc_stop_chn(c->chn);
    }
    if (c->created) {
        mpi_venc_destroy_chn(c->chn);
    }
}

/* ============================================================================
 * Layer 5: Pipeline 状态 + AVI/NET 输出
 * ============================================================================ */

static volatile int g_rec_active    = 0;
static volatile int g_stream_active = 0;
static volatile int g_pipeline_exit = 0;
static int          g_pipeline_pid  = 0;

struct avi_file_s {
    char path[96];
    avi_t *avi;
    u16 w, h;
    u32 fps, frames, bytes, max;
};
typedef struct avi_file_s  avi_file_t;

struct lbuf_data_head {
    int len;
    u8 type;
    u8 data[0];
};

typedef struct {
    void *fd;
    struct sockaddr_in addr;
    u8 task_exit;
    char task_name[32];
    int task_pid;
    u8 *video_lbuf_ptr;
    struct lbuff_head *video_lbuf_handle;
    u8 *udp_send_buf;
    u32 seq, frames, bytes, err, drop;
} net_t;

static avi_file_t g_avi = {{0}};
static net_t     *g_net = NULL;

/* ---- OSD 时间水印 (双缓冲, 避免硬件读脏数据) ---- */
#define OSD_TIME_FMT     "%04d-%02d-%02d %02d:%02d:%02d"
#define OSD_BUF_COUNT    2

static app_camera_osd_bitmap_t g_osd_bmp[OSD_BUF_COUNT];
static volatile int            g_osd_bmp_idx = 0;   /* 当前硬件正在读的 buf index */
static uint16_t                g_osd_timer = 0;
static mpi_vpss_grp_t          g_osd_grp = -1;
static mpi_vpss_chn_t          g_osd_chn = -1;

static void osd_apply(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn,
                      app_camera_osd_bitmap_t *bmp, int update)
{
    mpi_vpss_osd_attr_t attr = {0};
    attr.mode      = MPI_VPSS_OSD_MODE_2BIT;
    attr.width     = bmp->width;
    attr.height    = bmp->height;
    attr.color[0]  = 0x057d88u;
    attr.color[1]  = 0xe20095u;
    attr.color[2]  = 0xe20095u;
    attr.color[3]  = 0xe20095u;
    attr.buf_addr  = bmp->buf;
    attr.buf_size  = bmp->buf_size;

    if (update) {
        mpi_vpss_update_chn_osd_attr(grp, chn, &attr);
    } else {
        mpi_vpss_set_chn_osd_attr(grp, chn, &attr);
    }
}

static void osd_timer_cb(void *priv)
{
    struct sys_time time;
    char buf[32];
    int next = g_osd_bmp_idx ^ 1;   /* 渲染到另一个 buf */
    (void)priv;

    rtc_read_time(&time);
    snprintf(buf, sizeof(buf), OSD_TIME_FMT,
             time.year, time.month, time.day, time.hour, time.min, time.sec);

    /* 释放备用的旧位图, 重新渲染 */
    app_camera_osd_bitmap_free(&g_osd_bmp[next]);
    if (app_camera_osd_text_parse(buf, &g_osd_bmp[next]) != APP_CAMERA_OK) {
        return;
    }

    osd_apply(g_osd_grp, g_osd_chn, &g_osd_bmp[next], 1);
    g_osd_bmp_idx = next;   /* 更新完成后, 硬件下一帧切到新 buf */
}

static int osd_start(mpi_vpss_grp_t grp, mpi_vpss_chn_t chn)
{
    struct sys_time time;
    char buf[32];

    if (g_osd_timer) {
        return 0;
    }

    g_osd_grp = grp;
    g_osd_chn = chn;

    rtc_read_time(&time);
    snprintf(buf, sizeof(buf), OSD_TIME_FMT,
             time.year, time.month, time.day, time.hour, time.min, time.sec);

    if (app_camera_osd_text_parse(buf, &g_osd_bmp[0]) != APP_CAMERA_OK) {
        printf(TAG " OSD text parse fail\n");
        return -1;
    }

    osd_apply(grp, chn, &g_osd_bmp[0], 0);
    mpi_vpss_enable_chn_osd(grp, chn);

    g_osd_bmp_idx = 0;
    g_osd_timer = sys_timer_add(NULL, osd_timer_cb, 1000);
    printf(TAG " [OSD] start: %s\n", buf);
    return 0;
}

static void osd_stop(void)
{
    int i;
    if (g_osd_timer) {
        sys_timer_del(g_osd_timer);
        g_osd_timer = 0;
    }
    if (g_osd_grp != (mpi_vpss_grp_t) - 1) {
        mpi_vpss_disable_chn_osd(g_osd_grp, g_osd_chn);
    }
    for (i = 0; i < OSD_BUF_COUNT; i++) {
        app_camera_osd_bitmap_free(&g_osd_bmp[i]);
    }
    printf(TAG " [OSD] stop\n");
}
/* ---- OSD end ---- */

/* ---- NET ---- */

static int net_path_analyze(net_t *c, const char *path)
{
    char *t = strstr(path, "udp://"), ip[16] = {0};
    u16 port;
    if (!t) {
        return -1;
    }
    t += 6;
    char *c2 = strchr(t, ':');
    if (!c2) {
        return -1;
    }
    strncpy(ip, t, c2 - t);
    port = (u16)atoi(c2 + 1);
    c->addr.sin_family = AF_INET;
    c->addr.sin_addr.s_addr = inet_addr(ip);
    c->addr.sin_port = htons(port);
    return 0;
}

static int net_write_data(net_t *c, u8 *buf, size_t len, u8 type)
{
    struct frm_head frame_head = {0};
    u16 payload_len = 0;
    u32 total_udp_send = 0;
    int ret;

    frame_head.offset = 0;
    frame_head.frm_sz = len;
    frame_head.type &= ~LAST_FREG_MAKER;
    frame_head.type |= type;
    frame_head.timestamp = 0;
    frame_head.seq = c->seq++;

    u32 remain_len = len;
    while (remain_len) {
        if (remain_len < MAX_PAYLOAD) {
            payload_len = remain_len;
            frame_head.type |= LAST_FREG_MAKER;
        } else {
            payload_len = MAX_PAYLOAD;
        }

        frame_head.payload_size = payload_len;
        memcpy(c->udp_send_buf + total_udp_send, &frame_head, sizeof(struct frm_head));
        total_udp_send += sizeof(struct frm_head);

        memcpy(c->udp_send_buf + total_udp_send, buf + frame_head.offset, payload_len);
        total_udp_send += payload_len;

        if (total_udp_send == UDP_SEND_BUF_SIZE || payload_len < MAX_PAYLOAD) {
            ret = sock_sendto(c->fd, c->udp_send_buf, total_udp_send, 0,
                              (struct sockaddr *)&c->addr, sizeof(struct sockaddr_in));
            if (ret != (int)total_udp_send) {
                puts("net_stream_send error!\n");
                c->err++;
                return -1;
            }
            total_udp_send = 0;
        }

        remain_len -= payload_len;
        frame_head.offset += payload_len;
    }

    c->frames++;
    c->bytes += len;
    return (int)len;
}

static int net_rt_send_frame(net_t *c, u8 *buf, size_t len, u8 type)
{
    if (!c) {
        return 0;
    }

    struct lbuf_data_head *lbuf_data = lbuf_alloc(c->video_lbuf_handle, len);
    if (lbuf_data) {
        lbuf_data->len = len;
        lbuf_data->type = type;
        memcpy(lbuf_data->data, buf, len);
        lbuf_push(lbuf_data, BIT(0));
    } else {
        c->drop++;
    }
    return (int)len;
}

static void net_pkg_send_task(void *priv)
{
    net_t *c = (net_t *)priv;
    while (!c->task_exit) {
        struct lbuf_data_head *lbuf_data = lbuf_pop(c->video_lbuf_handle, BIT(0));
        if (lbuf_data) {
            net_write_data(c, lbuf_data->data, lbuf_data->len, lbuf_data->type);
            lbuf_free(lbuf_data);
        } else {
            os_time_dly(2);
        }
    }
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    net_t *c = (net_t *)priv;
    if (c->task_exit) {
        return -1;
    }
    return 0;
}

static net_t *net_open(const char *path, u32 abr, u32 fps)
{
    net_t *c = calloc(1, sizeof(net_t));
    int sz;
    if (!c) {
        return NULL;
    }

    if (net_path_analyze(c, path)) {
        goto err;
    }

    c->fd = sock_reg(AF_INET, SOCK_DGRAM, 0, sock_cb_func, c);
    if (!c->fd) {
        goto err;
    }

    if (!abr) {
        abr = 4000;
    }
    if (!fps) {
        fps = UVC_JPEG_FPS;
    }
    sz = (int)(abr * 1024 / fps / 8 * NET_LBUF_COUNT);
    if (sz < 65536) {
        sz = 65536;
    }

    c->video_lbuf_ptr = malloc(sz);
    if (!c->video_lbuf_ptr) {
        goto err;
    }
    c->video_lbuf_handle = lbuf_init(c->video_lbuf_ptr, sz, 4, sizeof(struct lbuf_data_head));

    c->udp_send_buf = malloc(UDP_SEND_BUF_SIZE);
    if (!c->udp_send_buf) {
        goto err;
    }

    snprintf(c->task_name, sizeof(c->task_name), "net_pkg_send");
    if (thread_fork(c->task_name, 15, 2048, 2048,
                    &c->task_pid, net_pkg_send_task, c)) {
        goto err;
    }

    return c;

err:
    if (c->udp_send_buf) {
        free(c->udp_send_buf);
    }
    if (c->video_lbuf_ptr) {
        free(c->video_lbuf_ptr);
    }
    if (c->fd) {
        sock_unreg(c->fd);
    }
    free(c);
    return NULL;
}

static void net_close(net_t *c)
{
    if (!c) {
        return;
    }
    if (c->task_pid) {
        c->task_exit = 1;
        thread_kill(&c->task_pid, KILL_WAIT);
    }
    if (c->fd) {
        sock_unreg(c->fd);
    }
    if (c->video_lbuf_ptr) {
        free(c->video_lbuf_ptr);
    }
    if (c->udp_send_buf) {
        free(c->udp_send_buf);
    }
    printf(TAG " [NET] close: f=%u b=%u e=%u d=%u\n", c->frames, c->bytes, c->err, c->drop);
    free(c);
}

/* ---- AVI ---- */

/* ---- forward declarations (libmov MOV muxer) ---- */
void mov_rec_init(const char *path, u16 w, u16 h, u32 fps);
void mov_rec_write(const u8 *data, u32 len, int flags);
void mov_rec_finish(void);

static int avi_open(avi_file_t *c, const char *path, u16 w, u16 h, u32 fps)
{
    FILE *fp;
    memset(c, 0, sizeof(*c));
    strncpy(c->path, path, 95);
    c->w = w;
    c->h = h;
    c->fps = fps;
    fp = fopen(path, "wb");
    if (fp) {
        fclose(fp);
    }
    c->avi = AVI_open_output_file(path);
    if (!c->avi) {
        printf(TAG " [AVI] open fail\n");
        return -1;
    }
    AVI_set_video(c->avi, w, h, (double)fps, "MJPG");
    return 0;
}

static int avi_write(avi_file_t *c, const u8 *d, u32 len)
{
    if (!c || !c->avi || !d || !len) {
        return -1;
    }
    if (AVI_write_frame(c->avi, (const char *)d, (long)len, 1)) {
        return -1;
    }
    c->frames++;
    c->bytes += len;
    return 0;
}

static void avi_close(avi_file_t *c)
{
    if (!c || !c->avi) {
        return;
    }
    AVI_close(c->avi);
    c->avi = NULL;
    printf(TAG " [AVI] close: %u f %u b\n", c->frames, c->bytes);
}

/* ============================================================================
 * Pipeline 线程
 * ============================================================================ */

static void uvc_pipeline_thread(void *arg)
{
    int ret;
    buf_pool_t *pool = NULL;
#ifdef CONFIG_NET_JPEG
    vdec_t vdec = {0};
    vpss_t vpss = {0};
    venc_t venc = {0};
    mpi_video_frame_info_t vdec_yuv = {0};
    mpi_video_frame_info_t vpss_yuv = {0};
    mpi_stream_t           enc_jpeg = {0};
#endif
    u8  *raw = NULL;
    u32 raw_len = 0;
    u8  is_h264 = 0;

    (void)arg;

#ifdef CONFIG_NET_JPEG
    printf(TAG " === UVC JPEG Pipeline: VDEC->VPSS->VENC->{AVI,STREAM} ===\n");
#else
    printf(TAG " === UVC H264 Pipeline: passthrough -> {MOV,STREAM} ===\n");
#endif

    ret = buf_pool_init(&pool);
    if (ret) {
        return;
    }

#if 0
    /* 动态获取摄像头实际分辨率 */
    int rw = UVC_JPEG_WIDTH, rh = UVC_JPEG_HEIGHT;
    uvc_host_get_cur_resolution(&rw, &rh);
    g_last_width  = rw;
    g_last_height = rh;
    printf(TAG "resolution=%dx%d\n", rw, rh);
#endif

#ifdef CONFIG_NET_JPEG
    ret = vdec_create(&vdec, (u32)g_last_width * g_last_height * 2);
    if (ret) {
        goto e1;
    }
    ret = vpss_create(&vpss, (u16)g_last_width, (u16)g_last_height, UVC_JPEG_FPS);
    if (ret) {
        goto e2;
    }
    ret = venc_create(&venc, (u16)g_last_width, (u16)g_last_height, UVC_JPEG_FPS);
    if (ret) {
        goto e3;
    }

    osd_start(vpss.grp, vpss.chn);
#endif

    /* 摄像头开始收数据 */
    extern void uvc_host_set_target_reso(u16 width, u16 height);
    uvc_host_set_target_reso(g_last_width, g_last_height);
    uvc_host_start_camera();
    printf(TAG " === Pipeline running ===\n");

    while (!g_pipeline_exit) {
        if (!g_rec_active && !g_stream_active) {
            os_time_dly(100);
            continue;
        }

        if (buf_pool_get_frame(pool, &raw, &raw_len, &is_h264, 500)) {
            os_time_dly(1);
            continue;
        }

#ifdef CONFIG_NET_JPEG
        if (vdec_decode(&vdec, raw, raw_len, &vdec_yuv, VDEC_TIMEOUT_MS)) {
            buf_pool_unlock(pool, raw);
            continue;
        }
        buf_pool_unlock(pool, raw);

        ret = vpss_send(&vpss, &vdec_yuv, VPSS_TIMEOUT_MS);
        mpi_vdec_release_frame(vdec.chn, &vdec_yuv);
        if (ret) {
            continue;
        }

        ret = vpss_get_frame(&vpss, &vpss_yuv, VPSS_TIMEOUT_MS);
        if (ret) {
            continue;
        }

        ret = venc_encode(&venc, &vpss_yuv, &enc_jpeg, VENC_TIMEOUT_MS);
        vpss_release_frame(&vpss, &vpss_yuv);
        if (ret) {
            continue;
        }

        if (g_rec_active && g_avi.avi) {
            avi_write(&g_avi, (const u8 *)enc_jpeg.addr, enc_jpeg.len);
        }
        if (g_stream_active && g_net) {
            net_rt_send_frame(g_net, (u8 *)enc_jpeg.addr, enc_jpeg.len, JPEG_TYPE_VIDEO);
        }
        mpi_venc_release_stream(venc.chn, &enc_jpeg);
#else
        /* H264 path: passthrough to net + libmov MOV recording */
        {
            static u8 h264_rec_wait_idr;

            /* wait for IDR after rec starts */
            if (g_rec_active && !h264_rec_wait_idr) {
                h264_rec_wait_idr = 1;
                mov_rec_init(g_last_path, (u16)g_last_width, (u16)g_last_height, (u32)g_last_fps);
            }
            if (!g_rec_active) {
                h264_rec_wait_idr = 0;
            }

            if (g_rec_active && h264_rec_wait_idr == 1) {
                u8 idr = 0;
                u32 off = 0;
                while (off + 4 < raw_len) {
                    u8 sl;
                    if (raw[off] == 0 && raw[off + 1] == 0 && raw[off + 2] == 0 && raw[off + 3] == 1) {
                        sl = 4;
                    } else if (raw[off] == 0 && raw[off + 1] == 0 && raw[off + 2] == 1) {
                        sl = 3;
                    } else {
                        off++;
                        continue;
                    }
                    if ((raw[off + sl] & 0x1f) == 5) {
                        idr = 1;
                        break;
                    }
                    off += sl + 1;
                }
                if (idr) {
                    h264_rec_wait_idr = 2;
                    mov_rec_write(raw, raw_len, MOV_AV_FLAG_KEYFRAME);
                }
            } else if (g_rec_active && h264_rec_wait_idr == 2) {
                int is_idr = 0;
                u32 off = 0;
                while (off + 4 < raw_len) {
                    u8 sl;
                    if (raw[off] == 0 && raw[off + 1] == 0 && raw[off + 2] == 0 && raw[off + 3] == 1) {
                        sl = 4;
                    } else if (raw[off] == 0 && raw[off + 1] == 0 && raw[off + 2] == 1) {
                        sl = 3;
                    } else {
                        off++;
                        continue;
                    }
                    if ((raw[off + sl] & 0x1f) == 5) {
                        is_idr = 1;
                        break;
                    }
                    off += sl + 1;
                }
                mov_rec_write(raw, raw_len, is_idr ? MOV_AV_FLAG_KEYFRAME : 0);
            }
        }
        if (g_stream_active && g_net) {
            net_rt_send_frame(g_net, raw, raw_len, H264_TYPE_VIDEO);
        }
        buf_pool_unlock(pool, raw);
#endif
    }

    printf(TAG " === Pipeline exit: AVI=%u/%ub NET: f=%u b=%u cb=%u drop=%u ===\n",
           g_avi.frames, g_avi.bytes,
           g_net ? g_net->frames : 0, g_net ? g_net->bytes : 0,
           g_cb_cnt, pool->drop_cnt);

#ifdef CONFIG_NET_JPEG
    osd_stop();
    venc_destroy(&venc);
e3:
    vpss_destroy(&vpss);
e2:
    vdec_destroy(&vdec);
e1:
#endif
    buf_pool_deinit(pool);
    printf(TAG " === Pipeline thread exit ===\n");
}

/* ---- Internal helpers ---- */

static int pipeline_start(void)
{
    if (g_pipeline_pid) {
        return 0;
    }
    g_pipeline_exit = 0;
    if (thread_fork("uvc_jpeg_pipe", 20, 8192, 20,
                    &g_pipeline_pid, uvc_pipeline_thread, NULL)) {
        printf(TAG " pipeline thread fork fail\n");
        return -1;
    }
    return 0;
}

static void pipeline_stop(void)
{
    if (!g_pipeline_pid) {
        return;
    }
    g_rec_active = 0;
    g_stream_active = 0;
    g_pipeline_exit = 1;
    thread_kill(&g_pipeline_pid, KILL_WAIT);
    g_pipeline_pid = 0;
}

static int stream_start(const char *udp_path, u32 abr, u32 fps)
{
    if (g_stream_active) {
        return 0;
    }
    if (!g_pipeline_pid && pipeline_start()) {
        return -1;
    }
    g_net = net_open(udp_path, abr, fps);
    if (!g_net) {
        return -1;
    }
    g_stream_active = 1;
    printf(TAG " [STREAM] start: %s abr=%u fps=%u\n", udp_path, abr, fps);
    return 0;
}

static void stream_stop(void)
{
    if (!g_stream_active) {
        return;
    }
    g_stream_active = 0;
    net_close(g_net);
    g_net = NULL;
    printf(TAG " [STREAM] stop\n");
}

/* ============================================================================
 * 对外 API
 * ============================================================================ */

int mpi_video_net_open_stream(const char *path, int w, int h, int fps, int abr)
{
    /* cache params */
    if (path && path[0]) {
        strncpy(g_last_path, path, sizeof(g_last_path) - 1);
        g_last_path[sizeof(g_last_path) - 1] = '\0';
    }
    g_last_width = w;
    g_last_height = h;
    g_last_fps = fps;
    g_last_abr = abr;

    return stream_start(path, (u32)abr, (u32)fps);
}

int mpi_video_net_close_stream(void)
{
    stream_stop();
    return 0;
}

int mpi_video_net_get_status(void)
{
    return g_stream_active;
}

const char *mpi_video_net_get_last_path(void)
{
    return g_last_path;
}

int mpi_video_net_get_last_params(int *w, int *h, int *fps, int *abr)
{
    if (w)  {
        *w   = g_last_width;
    }
    if (h)  {
        *h   = g_last_height;
    }
    if (fps) {
        *fps = g_last_fps;
    }
    if (abr) {
        *abr = g_last_abr;
    }
    return 0;
}

/* ---- UVC camera info queries ---- */

int mpi_video_net_get_uvc_resolution(int *w, int *h)
{
    int ret = uvc_host_get_cur_resolution(w, h);
    if (ret < 0) {
        if (w) {
            *w = g_last_width;
        }
        if (h) {
            *h = g_last_height;
        }
    }
    return ret;
}

int mpi_video_net_get_reso_count(void)
{
    return uvc_host_get_reso_count();
}

int mpi_video_net_get_reso_by_index(int index, int *w, int *h)
{
    u16 width = 0, height = 0;
    int ret = uvc_host_get_reso_by_index(index, &width, &height);
    if (ret == 0) {
        if (w) {
            *w = width;
        }
        if (h) {
            *h = height;
        }
    }
    return ret;
}

/* ---- file naming: DCIM/2/VID_0001.ext, auto-increment ---- */
#define REC_BASE_DIR   CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_1  /* DCIM/2 */

static int rec_file_next_index(const char *ext)
{
    int max_idx = 0;
    /* fscan pattern: "-tAVI" or "-tMP4" */
    char pattern[8];
    int plen = 0;
    pattern[plen++] = '-';
    pattern[plen++] = 't';
    {
        const char *e = ext;
        while (*e) {
            pattern[plen++] = *e++;
        }
    }
    pattern[plen] = '\0';

    struct vfscan *fs = fscan(REC_BASE_DIR, pattern, 1);
    if (fs) {
        FILE *fp;
        while ((fp = fselect(fs, FSEL_NEXT_FILE, 0)) != NULL) {
            char name[64] = {0};
            fget_name(fp, (u8 *)name, sizeof(name));
            if (strncmp(name, "VID_", 4) == 0) {
                int idx = atoi(name + 4);
                if (idx > max_idx) {
                    max_idx = idx;
                }
            }
            fclose(fp);
        }
        fscan_release(fs);
    }
    return max_idx + 1;
}

static void rec_file_build_path(char *out, int out_size, const char *ext)
{
    int idx = rec_file_next_index(ext);
    snprintf(out, out_size, REC_BASE_DIR"/VID_%04d.%s", idx, ext);
}

/* recording: JPEG→AVI(MJPG), H264→MOV via libmov */
int mpi_video_net_start_rec(const char *path)
{
    if (g_rec_active) {
        return 0;
    }
    if (!g_pipeline_pid && pipeline_start()) {
        return -1;
    }

    /* auto-generate filename: DCIM/2/VID_0001.avi or .mp4 */
    (void)path;
#ifdef CONFIG_NET_JPEG
    rec_file_build_path(g_last_path, sizeof(g_last_path), "avi");
#else
    rec_file_build_path(g_last_path, sizeof(g_last_path), "mp4");
#endif

    g_rec_active = 1;
    printf(TAG " [REC] start: %s %dx%d@%d\n", g_last_path, g_last_width, g_last_height, g_last_fps);
    return 0;
}

void mpi_video_net_stop_rec(void)
{
    if (!g_rec_active) {
        return;
    }
    g_rec_active = 0;
#ifdef CONFIG_NET_JPEG
    avi_close(&g_avi);
#else
    mov_rec_finish();
#endif
    printf(TAG " [REC] stop\n");
}

int mpi_video_net_get_rec_status(void)
{
    return g_rec_active;
}

/* ---- libmov MP4 muxer (H264 path) ---- */

static mp4_muxer_t *g_mov_muxer  = NULL;
static u32 g_mov_frames  = 0;
static u32 g_mov_fps_val = 25;
static u16 g_mov_w = 640, g_mov_h = 480;
static u8  g_mov_avcc[256 + 128 + 16];  /* SPS+PPS concatenated for avcC */
static u32 g_mov_avcc_len = 0;

void mov_rec_init(const char *path, u16 w, u16 h, u32 fps)
{
    if (g_mov_muxer) {
        return;
    }
    g_mov_frames   = 0;
    g_mov_fps_val  = fps;
    g_mov_w        = w;
    g_mov_h        = h;
    g_mov_avcc_len = 0;

    /* 从路径提取 sd 设备名，永久禁用低功耗 */
    char sd_dev[4] = {0};
    const char *sd = strstr(path, "storage/");
    if (sd) {
        sd += 8;
        const char *end = strchr(sd, '/');
        int len = end ? (int)(end - sd) : (int)strlen(sd);
        if (len > 0 && len < 4) {
            memcpy(sd_dev, sd, len);
        }
    }
    if (sd_dev[0]) {
        extern void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);
        sdmmc_dev_lowpower_disable(sd_dev, 1);
    }

    g_mov_muxer = mp4_muxer_open(path, 0);
    if (g_mov_muxer) {
        printf(TAG " [MOV] opened %s %dx%d@%d\n", path, w, h, fps);
    } else {
        printf(TAG " [MOV] open failed\n");
    }
}

/* extract SPS (NAL type 7) and PPS (NAL type 8) from Annex-B H264 data.
 * returns concatenated avcC extra_data (without the avcC box header). */
static void mov_extract_sps_pps(const u8 *data, u32 len,
                                u8 *sps, u32 *sps_len,
                                u8 *pps, u32 *pps_len)
{
    u32 off = 0;
    *sps_len = *pps_len = 0;
    while (off + 4 < len) {
        u8 sl;
        if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 0 && data[off + 3] == 1) {
            sl = 4;
        } else if (data[off] == 0 && data[off + 1] == 0 && data[off + 2] == 1) {
            sl = 3;
        } else {
            off++;
            continue;
        }
        u8 nt = data[off + sl] & 0x1f;
        u32 nal_start = off + sl;
        u32 nal_end = nal_start;
        while (nal_end + 2 < len &&
               !(data[nal_end] == 0 && data[nal_end + 1] == 0 &&
                 (data[nal_end + 2] == 1 || (nal_end + 3 < len && data[nal_end + 2] == 0 && data[nal_end + 3] == 1)))) {
            nal_end++;
        }
        u32 nal_len = nal_end - nal_start;
        if (nt == 7 && nal_len <= 256) {
            memcpy(sps, data + nal_start, nal_len);
            *sps_len = nal_len;
        }
        if (nt == 8 && nal_len <= 128) {
            memcpy(pps, data + nal_start, nal_len);
            *pps_len = nal_len;
        }
        off = nal_end;
    }
}

/* build avcC extra_data from SPS+PPS */
static u32 mov_build_avcc(u8 *out, const u8 *sps, u32 sps_len, const u8 *pps, u32 pps_len)
{
    u8 *p = out;
    *p++ = 1;                                         /* configurationVersion */
    *p++ = sps_len >= 4 ? sps[1] : 66;                /* AVCProfileIndication */
    *p++ = sps_len >= 4 ? sps[2] : 0;                 /* profile_compatibility */
    *p++ = sps_len >= 4 ? sps[3] : 30;                /* AVCLevelIndication */
    *p++ = 0xFF;                                      /* 6 bits reserved + lengthSizeMinusOne=3 */
    *p++ = 0xE1;                                      /* 3 bits reserved + numOfSequenceParameterSets=1 */
    *p++ = (u8)(sps_len >> 8);                        /* SPS length */
    *p++ = (u8)(sps_len & 0xFF);
    memcpy(p, sps, sps_len);
    p += sps_len;
    *p++ = 1;                                         /* numOfPictureParameterSets=1 */
    *p++ = (u8)(pps_len >> 8);                        /* PPS length */
    *p++ = (u8)(pps_len & 0xFF);
    memcpy(p, pps, pps_len);
    p += pps_len;
    return (u32)(p - out);
}

void mov_rec_write(const u8 *data, u32 len, int flags)
{
    if (!g_mov_muxer || !data || !len) {
        return;
    }

    /* lazy init: extract SPS/PPS from first frame, add video track */
    if (g_mov_avcc_len == 0) {
        u8 sps[256], pps[128];
        u32 sps_len, pps_len;
        mov_extract_sps_pps(data, len, sps, &sps_len, pps, &pps_len);
        if (sps_len && pps_len) {
            g_mov_avcc_len = mov_build_avcc(g_mov_avcc, sps, sps_len, pps, pps_len);
            mp4_muxer_add_video(g_mov_muxer, MP4_MUXER_OBJECT_H264,
                                g_mov_w, g_mov_h, g_mov_avcc, g_mov_avcc_len);
        } else {
            /* fallback: no SPS/PPS, add track with empty avcC */
            mp4_muxer_add_video(g_mov_muxer, MP4_MUXER_OBJECT_H264,
                                g_mov_w, g_mov_h, NULL, 0);
            g_mov_avcc_len = 1; /* mark as initialized */
        }
    }

    u32 ts = (u32)((u64)g_mov_frames * 1000 / g_mov_fps_val);
    mp4_muxer_write_video(g_mov_muxer, data, len, ts, ts, flags);
    g_mov_frames++;
}

void mov_rec_finish(void)
{
    if (!g_mov_muxer) {
        return;
    }
    mp4_muxer_close(g_mov_muxer);
    g_mov_muxer = NULL;
    printf(TAG " [MOV] done: %u frames\n", g_mov_frames);
}

#else
/* no camera defined */

int mpi_video_net_open_stream(const char *path, int w, int h, int fps, int abr)
{
    (void)path;
    (void)w;
    (void)h;
    (void)fps;
    (void)abr;
    return -1;
}

int mpi_video_net_close_stream(void)
{
    return -1;
}
int mpi_video_net_get_status(void)
{
    return 0;
}
const char *mpi_video_net_get_last_path(void)
{
    return "";
}
int mpi_video_net_get_last_params(int *w, int *h, int *fps, int *abr)
{
    if (w) {
        *w = 640;
    }
    if (h) {
        *h = 480;
    }
    if (fps) {
        *fps = 25;
    }
    if (abr) {
        *abr = 2000;
    }
    return 0;
}

int  mpi_video_net_start_rec(const char *path)
{
    (void)path;
    return -1;
}
void mpi_video_net_stop_rec(void) {}
int  mpi_video_net_get_rec_status(void)
{
    return 0;
}

int mpi_video_net_get_uvc_resolution(int *w, int *h)
{
    if (w) {
        *w = 640;
    }
    if (h) {
        *h = 480;
    }
    return -1;
}

int mpi_video_net_get_reso_count(void)
{
    return 0;
}

int mpi_video_net_get_reso_by_index(int index, int *w, int *h)
{
    (void)index;
    if (w) {
        *w = 0;
    }
    if (h) {
        *h = 0;
    }
    return -1;
}

#endif /* TCFG_CAMERA_DVP_ENABLE / TCFG_HOST_UVC_ENABLE */

#endif /* TCFG_EXT_WIFI_ENABLE */
