#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_photo.data.bss")
#pragma data_seg(".video_photo.data")
#pragma const_seg(".video_photo.text.const")
#pragma code_seg(".video_photo.text")
#endif
#include "system/includes.h"
#include "app_main.h"
#include "app_video.h"
#include "app_config.h"
#include "app_action.h"
#include "app_tone.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "video_photo.h"
#include "key_driver.h"
#include "key_event_deal.h"
#include "video_photo_ui.h"
#include "lv_port_video_disp.h"
#include "jlgpu_image_filters.h"  /* video滤镜枚举 jlgpu_filter_mode_t (上游重构从 lv_port_video_disp.h 迁入) */
#include "fs/fs.h"
#include "media_file_list.h"

/* 摄像头源二选一: DVP(内部 sensor) 或 USB Host UVC(外接摄像头)。
 * UVC 打开要求 TCFG_HOST_UVC_ENABLE 与 TCFG_USB_HOST_ENABLE 同时为 1, 缺一不可。 */
#if TCFG_CAMERA_DVP_ENABLE
#include "app_camera_mpi.h"
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
#include "os/os_api.h"
#include "usb_host.h"
#include "host_uvc.h"
#include "uvc_device.h"
#include "mpi_common.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "mpi_vo.h"
#include "mpi_sys.h"
#include "fb.h"
#endif

#define LOG_TAG             "[PHOTO]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"
#if TCFG_APP_PHOTO_EN

/* 弱默认函数: 当 kid_camera LVGL UI 未被编译时提供默认值 */
_WEAK_
u8 kid_settings_get_img_quality_idx(void)
{
    return 6;  /* 默认图片质量中间档 */
}

_WEAK_
u8 kid_settings_get_photo_timed_sec(void)
{
    return 0;  /* 默认关闭定时拍照 */
}

/* 最后一张照片的实际文件名, 拍照完成时由 app_camera 层填入 (按名入库用, 同录像) */
static char g_last_photo_name[128];

static void photo_flm_add_new(void)
{
    if (g_last_photo_name[0] == '\0') {
        return;
    }
    if (flm_add_by_name(MEDIA_PHOTO_DIR, g_last_photo_name) != 0) {
        log_error("[PHOTO] flm_add_by_name failed: %s\n", g_last_photo_name);
    }
}

#define PHOTO_STICKER_COUNT   10u
#define PHOTO_AI_WIDTH           640u
#define PHOTO_AI_HEIGHT          480u
#define PHOTO_AI_QUALITY         70u
#define PHOTO_SENSOR_WIDTH       640u
#define PHOTO_SENSOR_HEIGHT      480u
#define PHOTO_PATH_MAX           96      /* 照片文件路径缓冲 (DVP/UVC 共用) */
extern void rtc_read_time(struct sys_time *time);
/* 内部上下文 */
typedef struct {
    u8  state;
    u8  mode;
    u8  capturing;
    u8  ready;          // 模式已稳定可接受事件 (0=未就绪, 1=就绪)
    u8  zoom_tenths;    // zoom * 10 (10=1.0x, 40=4.0x)
    u8  burst_cnt;
    u8  timer_left;
    u8  effect;
    u8  sticker_idx;
    u32 total;
    u32 index;
} photo_ctx_t;

static photo_ctx_t g_ctx;

#if TCFG_CAMERA_DVP_ENABLE
/* 摄像头预览句柄 (persistent, across capture) */
static app_camera_handle_t g_photo_cam = NULL;

/* 显示属性 (LCD preview): 摄像头预览保持 320x240(传感器 4:3 等比画面),
 * 预览 fb 与 LVGL 视频控件同尺寸 */
static const app_camera_display_attr_t g_photo_disp_attr = {
    .fb_name = "fb5",
    .fb_num = 2,
    .width = 320,
    .height = 240,
    .left = 0,
    .top = 0,
};
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
/* UVC 显示属性 (LCD preview, 与 DVP 共用 fb5 预览窗口) */
typedef struct {
    const char *fb_name;
    u8  fb_num;
    u16 width;
    u16 height;
    u16 left;
    u16 top;
} photo_uvc_disp_attr_t;

static const photo_uvc_disp_attr_t g_photo_disp_attr = {
    .fb_name = "fb5",
    .fb_num = 2,
    .width = 320,
    .height = 240,
    .left = 0,
    .top = 0,
};
#endif

static u8 g_photo_ai_tools;
/* sensor 固定输出 640x480(GC0308); 只留 VGA 档(≥720P 大图整帧 615KB 超物理池预算)。
 * 顺序必须与 kid_settings.c 的 OPT_IMG_QUALITY 和 ui_action_sys_menu.c 的 roller 一致。 */
#define PHOTO_RES_DEFAULT_INDEX  0u
static const u16 PHOTO_RES[][2] = {
    { 640,  480},
};
int photo_precheck(void);
// ── 工具 ────────────────────────────────────────────────────
static int photo_make_filename(char *buf, int len)
{
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    extern u8 uvc_host_is_h264_format(void);
    if (uvc_host_is_h264_format()) {
        /* UVC H264 直通拍照: 存原始 IDR 关键帧. 扩展名 3 字符 .H26;
         * 主名必须 ≤8 字符: H264 + **** = 8, 刚好卡 8.3 上限 (不能再加下划线,
         * 否则 9 字符超长导致 JLFS 的 **** 递增失效, 每次同名覆盖) */
        snprintf(buf, len, PHOTO_FILE_PATH "H264****.H26");
        return 0;
    }
#endif
    /* JLFS 8.3短文件名: JPG_0001.JPG, 文件系统自动递增序号 */
    snprintf(buf, len, PHOTO_FILE_PATH "JPG_****.JPG");
    return 0;
}
// ── 存储 ────────────────────────────────────────────────────
/* SD 检测 fget_partition 优先, 失败回退 mnt/sd0, 与 video_dec 一致防 fopen 误判 */

static int photo_find_partition(struct vfs_partition **out)
{
    struct vfs_partition *p = fget_partition(CONFIG_ROOT_PATH);
    if (p) {
        *out = p;
        return 0;
    }
    p = fget_partition("mnt/"SDX_DEV);
    if (p) {
        *out = p;
        return 0;
    }
    *out = NULL;
    return -1;
}
int photo_storage_info(u32 *total_kb, u32 *free_kb)
{
    struct vfs_partition *p = NULL;
    if (photo_find_partition(&p) != 0 || p == NULL) {
        *total_kb = 8 * 1024 * 1024;
        *free_kb  = 4 * 1024 * 1024;
        return 0;
    }
    *total_kb = p->total_size;
    u32 free_space = 0;
    if (fget_free_space(CONFIG_ROOT_PATH, &free_space) != 0) {
        fget_free_space("mnt/"SDX_DEV, &free_space);
    }
    *free_kb = free_space;
    return 0;
}
/* 删除 PHOTO 目录下最旧的一张 JPG（按文件名顺序），返回 0=成功, -1=无可删 */
static int photo_delete_oldest_file(void)
{
    struct vfscan *fs = fscan(PHOTO_FILE_PATH, "-tJPG -sn", 1);
    struct vfs_attr attr = {0};
    char oldest_name[64] = {0};
    FILE *fp;

    if (!fs) {
        log_info("[PHOTO] cannot scan %s\n", PHOTO_FILE_PATH);
        return -1;
    }

    for (fp = fselect(fs, FSEL_FIRST_FILE, 0); fp != NULL;
         fp = fselect(fs, FSEL_NEXT_FILE, 0)) {
        char name[64] = {0};
        fget_name(fp, (u8 *)name, sizeof(name));
        if (strstr(name, ".jpg") == NULL && strstr(name, ".JPG") == NULL) {
            fclose(fp);
            continue;
        }
        if (fget_attrs(fp, &attr) != 0) {
            fclose(fp);
            continue;
        }
        if (attr.attr & F_ATTR_RO) {
            fclose(fp);
            continue;
        }
        /* 设备时间可能未联网同步，创建时间不可靠；
         * 文件名由 JLFS 按 JPG_**** 顺序自动生成，
         * 因此按文件名字典序选最旧（最小）的一个。 */
        if (oldest_name[0] == '\0' || strcmp(name, oldest_name) < 0) {
            strncpy(oldest_name, name, sizeof(oldest_name) - 1);
        }
        fclose(fp);
    }
    fscan_release(fs);

    if (oldest_name[0] == '\0') {
        log_info("[PHOTO] no JPG to delete\n");
        return -1;
    }

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", PHOTO_FILE_PATH, oldest_name);
    log_info("[PHOTO] deleting oldest: %s\n", full_path);
    if (flm_delete_file(full_path) != 0) {
        log_error("[PHOTO] delete failed: %s\n", full_path);
        return -1;
    }
    return 0;
}

int photo_storage_ok(void)
{
    u32 total, free_kb;
    int retries = PHOTO_DELETE_MAX_FILES;

    photo_storage_info(&total, &free_kb);
    while (free_kb < PHOTO_MIN_FREE_MB * 1024 && retries-- > 0) {
        if (photo_delete_oldest_file() != 0) {
            break;
        }
        photo_storage_info(&total, &free_kb);
    }
    if (free_kb < PHOTO_MIN_FREE_MB * 1024) {
        log_error("[PHOTO] storage full: %uKB free\n", free_kb);
        return -1;
    }
    return 0;
}
int photo_sd_ok(void)
{
    if (flm_sd_online() != 0) {
        log_error("[PHOTO] no SD card\n");
        return -1;
    }
    return 0;
}
// ── 变焦 (内部用整数tenths, 消除浮点精度问题) ──────────────
static u8 photo_zoom_to_tenths(float level)
{
    if (level < 1.0f) {
        return 10;
    }
    if (level > 4.0f) {
        return 40;
    }
    return (u8)((level - 1.0f) * 10.0f + 0.5001f) + 10;
}

static int photo_zoom_apply(u8 zoom_tenths)
{
#if TCFG_CAMERA_DVP_ENABLE
    app_camera_ret_t ret;

    if (!g_photo_cam) {
        return -1;
    }

    if (zoom_tenths <= 10) {
        ret = app_camera_disable_digital_zoom(g_photo_cam);
    } else {
        app_camera_zoom_param_t param = {0};
        u32 crop_w = (PHOTO_SENSOR_WIDTH * 10u / zoom_tenths) & ~3u;
        u32 crop_h = (PHOTO_SENSOR_HEIGHT * 10u / zoom_tenths) & ~3u;

        param.crop_w = crop_w;
        param.crop_h = crop_h;
        param.x_start = ((PHOTO_SENSOR_WIDTH - crop_w) / 2u) & ~3u;
        param.y_start = ((PHOTO_SENSOR_HEIGHT - crop_h) / 2u) & ~3u;
        ret = app_camera_set_digital_zoom(g_photo_cam, &param);
        if (ret == APP_CAMERA_OK) {
            log_info("[PHOTO] crop=(%u,%u,%u,%u)\n",
                     param.x_start, param.y_start,
                     param.crop_w, param.crop_h);
        }
    }

    if (ret != APP_CAMERA_OK) {
        log_error("[PHOTO] zoom apply failed: %d\n", ret);
        return -1;
    }
    return 0;
#else
    /* UVC 摄像头不做数字变焦, 保持 1.0x */
    (void)zoom_tenths;
    return -1;
#endif
}

int photo_zoom_set(float level)
{
    u8 zoom_tenths = photo_zoom_to_tenths(level);

    if (photo_zoom_apply(zoom_tenths) != 0) {
        return -1;
    }
    g_ctx.zoom_tenths = zoom_tenths;
    log_info("[PHOTO] zoom = %.1fx\n", g_ctx.zoom_tenths / 10.0f);
    return 0;
}
float photo_zoom_get(void)
{
    return g_ctx.zoom_tenths / 10.0f;
}
float photo_zoom_in(void)
{
    if (g_ctx.zoom_tenths < 40) {
        if (photo_zoom_set((g_ctx.zoom_tenths + 1) / 10.0f) != 0) {
            return photo_zoom_get();
        }
    }
    return photo_zoom_get();
}
float photo_zoom_out(void)
{
    if (g_ctx.zoom_tenths > 10) {
        if (photo_zoom_set((g_ctx.zoom_tenths - 1) / 10.0f) != 0) {
            return photo_zoom_get();
        }
    }
    return photo_zoom_get();
}

/* 长按连续变焦
 *
 * 按键表的 LONG 只会来一次, HOLD 的上报频率不可控, 所以不靠 HOLD 步进:
 * LONG 启动这里的步进状态, 由已有的 photo_periodic_tick (100ms) 推进,
 * 按键 UP 发 APP_MSG_VIDEO_PHOTO_ZOOM_STOP 停下。到顶/到底自动停。
 *
 * PHOTO_ZOOM_REPEAT_TICKS: 步进间隔(单位 100ms)。2 => 0.1x/200ms,
 * 1.0x→4.0x 全程约 6s。
 */
#define PHOTO_ZOOM_REPEAT_TICKS   2
static int s_photo_zoom_repeat_dir = 0;
static u32 s_photo_zoom_repeat_ticks = 0;

void photo_zoom_repeat_start(int direction)
{
    /* 已经顶到边界就不必启动了 */
    if ((direction > 0 && g_ctx.zoom_tenths >= 40) ||
        (direction < 0 && g_ctx.zoom_tenths <= 10) ||
        direction == 0) {
        s_photo_zoom_repeat_dir = 0;
        return;
    }
    s_photo_zoom_repeat_dir = direction > 0 ? 1 : -1;
    s_photo_zoom_repeat_ticks = 0;
}

void photo_zoom_repeat_stop(void)
{
    s_photo_zoom_repeat_dir = 0;
    s_photo_zoom_repeat_ticks = 0;
}

// ── UI效果 (P0: 音效+闪光, 保留扩展点) ──────────────────────
static void photo_effect_shutter(void)
{
    log_info("[PHOTO] shutter sound\n");
}
static void photo_effect_flash(void)
{
    log_info("[PHOTO] flash\n");
}
static void photo_effect_thumbnail(const char *path)
{
    log_info("[PHOTO] thumbnail: %s\n", path);
}
static void photo_effect_show_current(void)
{
    const char *path = NULL;

    if (g_ctx.effect == JLGPU_FILTER_STICKER) {
        path = photo_ui_get_sticker_path_by_idx(g_ctx.sticker_idx);
    }
    photo_ui_trigger_effect(g_ctx.effect, path);
}
// ── 特效切换 (桩函数, P0预留) ────────────────────────────────
int photo_effect_next(void)
{
    if (g_ctx.effect == JLGPU_FILTER_STICKER) {
        if ((u8)(g_ctx.sticker_idx + 1u) < PHOTO_STICKER_COUNT) {
            g_ctx.sticker_idx++;
        } else {
            g_ctx.effect = (g_ctx.effect + 1u) % (JLGPU_FILTER_NEGATIVE + 1u);
            g_ctx.sticker_idx = 0;
        }
    } else {
        g_ctx.effect = (g_ctx.effect + 1u) % (JLGPU_FILTER_NEGATIVE + 1u);
        if (g_ctx.effect == JLGPU_FILTER_STICKER) {
            g_ctx.sticker_idx = 0;
        }
    }
    photo_effect_show_current();
    log_info("[PHOTO] effect next: %d sticker=%u\n", g_ctx.effect, g_ctx.sticker_idx);
    return g_ctx.effect;
}
int photo_effect_prev(void)
{
    if (g_ctx.effect == JLGPU_FILTER_STICKER) {
        if (g_ctx.sticker_idx > 0u) {
            g_ctx.sticker_idx--;
        } else {
            g_ctx.effect = (g_ctx.effect + JLGPU_FILTER_NEGATIVE) % (JLGPU_FILTER_NEGATIVE + 1u);
            if (g_ctx.effect == JLGPU_FILTER_STICKER) {
                g_ctx.sticker_idx = PHOTO_STICKER_COUNT - 1u;
            }
        }
    } else {
        g_ctx.effect = (g_ctx.effect + JLGPU_FILTER_NEGATIVE) % (JLGPU_FILTER_NEGATIVE + 1u);
        if (g_ctx.effect == JLGPU_FILTER_STICKER) {
            g_ctx.sticker_idx = PHOTO_STICKER_COUNT - 1u;
        }
    }
    photo_effect_show_current();
    log_info("[PHOTO] effect prev: %d sticker=%u\n", g_ctx.effect, g_ctx.sticker_idx);
    return g_ctx.effect;
}
int photo_effect_get(void)
{
    return g_ctx.effect;
}

/* ============================================================================
 * UVC Camera 路径 (USB Host 外接摄像头)
 *
 *   帧回调 -> 双帧缓冲池 -> pipeline 线程:
 *     - 显示: VDEC(JPEG/SoftH264 自适应) -> VPSS(display chn) -> VO -> fb5
 *     - 拍照: JPEG 直接存原始帧 / H264 走 VDEC->VENC(JPEG) 转码
 *
 * 参考 apps/watch/wifi/stream/mpi_video_net.c 的 buf_pool/vdec/vpss/venc 封装。
 * ============================================================================ */
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)

/* host_uvc.h 未导出的接口 (实现在 usb_video.c) */
extern u8    uvc_host_is_h264_format(void);
extern void  uvc_host_set_target_reso(u16 width, u16 height);

#define PHOTO_UVC_BUF_SIZE          (128 * 1024)
#define PHOTO_UVC_BUF_POOL_COUNT    2
#define PHOTO_UVC_VDEC_TIMEOUT_MS   5000
#define PHOTO_UVC_VPSS_TIMEOUT_MS   5000
#define PHOTO_UVC_VENC_TIMEOUT_MS   5000
#define PHOTO_UVC_H264_FRAME_POOL   (512 * 1024)
#define PHOTO_UVC_H264_STREAM_BUF   (64 * 1024)

/* ---- 双帧缓冲池 ---- */
typedef struct {
    u8              *data;
    volatile u32    len;
    u32             capacity;
    volatile u8     locked;
    volatile u8     ready;
    volatile u8     is_h264;
} photo_uvc_frame_buf_t;

typedef struct {
    photo_uvc_frame_buf_t bufs[PHOTO_UVC_BUF_POOL_COUNT];
    OS_SEM          frame_sem;
    volatile u32    frame_cnt;
    volatile u32    drop_cnt;
    volatile u32    write_cnt;
} photo_uvc_buf_pool_t;

/* ---- VDEC ---- */
typedef struct {
    mpi_vdec_chn_t  chn;
    u8              created, started;
    OS_SEM          sem;
    volatile int    done;
} photo_uvc_vdec_t;

/* ---- VPSS(display chn bind VO) + VO ---- */
typedef struct {
    mpi_vpss_grp_t  grp;
    mpi_vpss_chn_t  disp_chn;   /* 显示通道, bind VO -> fb5 */
    mpi_vo_chn_t    vo_chn;
    u8              created, started;
    u8              disp_enabled;
} photo_uvc_vpss_t;

static photo_uvc_buf_pool_t *g_uvc_pool = NULL;
static photo_uvc_vdec_t      g_uvc_vdec;
static photo_uvc_vpss_t      g_uvc_vpss;
static int    g_uvc_pipeline_pid = 0;
static volatile u8 g_uvc_exit        = 0;
static volatile u8 g_uvc_running     = 0;
static volatile u8 g_uvc_capture_req = 0;
static volatile u8 g_uvc_capture_ok  = 0;
static volatile u8 g_uvc_capture_is_file = 1;
static char  g_uvc_photo_path[96];
static unsigned char *g_uvc_jpeg_buf = NULL;
static unsigned int  g_uvc_jpeg_buf_size = 0;
static unsigned int *g_uvc_jpeg_size = NULL;
static u8    g_uvc_is_h264 = 0;
static u16   g_uvc_width  = 640;
static u16   g_uvc_height = 480;
static u16   g_uvc_capture_w = 1280;  /* 拍照目标分辨率 (菜单档位, IMC 缩放) */
static u16   g_uvc_capture_h = 960;
static u8    g_uvc_capture_q = 80;    /* 拍照 JPEG 质量 (菜单档位) */

/* ---- 帧回调 (UVC ISO 接收线程上下文) ---- */
static void photo_uvc_frame_cb(const u8 *data, u32 len, u8 is_h264)
{
    photo_uvc_buf_pool_t *p = g_uvc_pool;
    photo_uvc_frame_buf_t *t = NULL;
    int i;

    if (!p || len == 0) {
        return;
    }
    p->frame_cnt++;

    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++) {
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

static int photo_uvc_buf_pool_init(photo_uvc_buf_pool_t **pp)
{
    photo_uvc_buf_pool_t *p = (photo_uvc_buf_pool_t *)calloc(1, sizeof(*p));
    int i;

    if (!p) {
        return -1;
    }
    os_sem_create(&p->frame_sem, 0);
    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++) {
        p->bufs[i].data = (u8 *)malloc(PHOTO_UVC_BUF_SIZE);
        if (!p->bufs[i].data) {
            goto err;
        }
        p->bufs[i].capacity = PHOTO_UVC_BUF_SIZE;
    }
    g_uvc_pool = p;
    *pp = p;
    uvc_host_frame_cb_register(photo_uvc_frame_cb);
    printf("[PHOTO_UVC] buf pool: %d x %d bytes\n", PHOTO_UVC_BUF_POOL_COUNT, PHOTO_UVC_BUF_SIZE);
    return 0;
err:
    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    free(p);
    return -1;
}

static void photo_uvc_buf_pool_deinit(photo_uvc_buf_pool_t *p)
{
    int i;
    if (!p) {
        return;
    }
    uvc_host_frame_cb_register(NULL);
    os_sem_del(&p->frame_sem, OS_DEL_ALWAYS);
    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++) if (p->bufs[i].data) {
            free(p->bufs[i].data);
        }
    free(p);
    g_uvc_pool = NULL;
}

static int photo_uvc_buf_pool_get(photo_uvc_buf_pool_t *p, u8 **data, u32 *len,
                                  u8 *is_h264, int to_ms)
{
    int i;
    if (os_sem_pend(&p->frame_sem, to_ms) != 0) {
        return -1;
    }
    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++) {
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

static void photo_uvc_buf_pool_unlock(photo_uvc_buf_pool_t *p, u8 *data)
{
    int i;
    for (i = 0; i < PHOTO_UVC_BUF_POOL_COUNT; i++)
        if (p->bufs[i].data == data) {
            p->bufs[i].locked = 0;
            return;
        }
}

/* ---- VDEC (OFFLINE, 非阻塞) ---- */
static void photo_uvc_vdec_notify(void *priv, const mpi_stream_t *s, mpi_ret_t ret)
{
    photo_uvc_vdec_t *c = (photo_uvc_vdec_t *)priv;
    (void)s;
    c->done = (ret == MPI_SUCCESS) ? 1 : -1;
    os_sem_post(&c->sem);
}

static int photo_uvc_vdec_create(photo_uvc_vdec_t *c, u8 is_h264)
{
    mpi_ret_t r;
    mpi_vdec_chn_attr_t a;

    memset(c, 0, sizeof(*c));
    os_sem_create(&c->sem, 0);
    r = mpi_vdec_find_free_chn(&c->chn);
    if (r) {
        return -1;
    }
    memset(&a, 0, sizeof(a));
    a.hw_dev = 0;
    a.type = is_h264 ? MPI_VDEC_TYPE_SOFT_H264 : MPI_VDEC_TYPE_JPEG;
    a.out_yuv_order = MPI_YUV_ORDER_YUYV;
    a.output_mode = MPI_VDEC_OUTPUT_FRAME;
    if (is_h264) {
        a.frame_buf_size = PHOTO_UVC_H264_FRAME_POOL;
        a.stream_buf_size = PHOTO_UVC_H264_STREAM_BUF;
    } else {
        a.frame_buf_size = (u32)g_uvc_width * g_uvc_height * 2 + 1024;
    }
    r = mpi_vdec_create_chn(c->chn, &a);
    if (r) {
        return -1;
    }
    c->created = 1;
    mpi_vdec_set_block_mode(c->chn, 0, photo_uvc_vdec_notify, c);
    r = mpi_vdec_start_chn(c->chn);
    if (r) {
        return -1;
    }
    c->started = 1;
    printf("[PHOTO_UVC] vdec chn=%d %s\n", c->chn, is_h264 ? "H264" : "JPEG");
    return 0;
}

static int photo_uvc_vdec_decode(photo_uvc_vdec_t *c, u8 *data, u32 len,
                                 mpi_video_frame_info_t *f, int to_ms)
{
    mpi_stream_t s;
    int ret;

    c->done = 0;
    os_sem_set(&c->sem, 0);
    memset(&s, 0, sizeof(s));
    s.addr = data;
    s.len = len;
    s.type = g_uvc_is_h264 ? MPI_STREAM_TYPE_H264 : MPI_STREAM_TYPE_JPEG;
    ret = mpi_vdec_send_stream(c->chn, &s);
    if (ret) {
        printf("[PHOTO_UVC] vdec send: %d\n", ret);
        return -1;
    }
    ret = os_sem_pend(&c->sem, to_ms);
    if (ret || c->done <= 0) {
        printf("[PHOTO_UVC] vdec timeout\n");
        return -1;
    }
    memset(f, 0, sizeof(*f));
    ret = mpi_vdec_get_frame(c->chn, f, PHOTO_UVC_VDEC_TIMEOUT_MS);
    if (ret) {
        printf("[PHOTO_UVC] vdec get_frame: %d\n", ret);
        return -1;
    }
    return 0;
}

static void photo_uvc_vdec_destroy(photo_uvc_vdec_t *c)
{
    if (c->started) {
        mpi_vdec_stop_chn(c->chn);
    }
    if (c->created) {
        mpi_vdec_destroy_chn(c->chn);
    }
    os_sem_del(&c->sem, OS_DEL_ALWAYS);
    memset(c, 0, sizeof(*c));
}

/* ---- VPSS + VO (display chn 绑定 fb5 预览; 拍照 chn 由 capture_do 动态建) ---- */
static int photo_uvc_vpss_create(photo_uvc_vpss_t *c, u16 w, u16 h, u16 disp_w, u16 disp_h)
{
    mpi_ret_t r;
    mpi_vpss_grp_attr_t ga;
    mpi_vpss_chn_attr_t ca;
    mpi_vo_chn_attr_t vo_attr;
    mpi_chn_t vpss_src, vo_dst;

    memset(c, 0, sizeof(*c));
    c->grp = -1;
    c->disp_chn = -1;

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

    /* display chn -> VO -> fb5 */
    r = mpi_vpss_find_free_chn(c->grp, &c->disp_chn);
    if (r) {
        return -1;
    }
    memset(&ca, 0, sizeof(ca));
    ca.width = disp_w;
    ca.height = disp_h;
    ca.format = MPI_PIXEL_FMT_YUV420SP;
    ca.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ca.hw_id = MPI_VPSS_HW_IMC_DST1;
    r = mpi_vpss_create_chn(c->grp, c->disp_chn, &ca);
    if (r) {
        return -1;
    }
    r = mpi_vpss_enable_chn(c->grp, c->disp_chn);
    if (r) {
        return -1;
    }
    c->disp_enabled = 1;

    r = mpi_vo_find_free_chn(&c->vo_chn);
    if (r) {
        return -1;
    }
    memset(&vo_attr, 0, sizeof(vo_attr));
    strncpy(vo_attr.fb_name, "fb5", sizeof(vo_attr.fb_name) - 1);
    vo_attr.fb_num = 2;
    vo_attr.fb_width = disp_w;
    vo_attr.fb_height = disp_h;
    vo_attr.window_width = disp_w;
    vo_attr.window_height = disp_h;
    vo_attr.fb_format = FB_COLOR_FORMAT_YUV420P;
    vo_attr.combine = 1;
    r = mpi_vo_create_chn(c->vo_chn, &vo_attr);
    if (r) {
        return -1;
    }
    r = mpi_vo_start_chn(c->vo_chn);
    if (r) {
        return -1;
    }
    vpss_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_src.dev_id = c->grp;
    vpss_src.chn_id = c->disp_chn;
    vo_dst.mod_id = MPI_MOD_ID_VO;
    vo_dst.dev_id = 0;
    vo_dst.chn_id = c->vo_chn;
    r = mpi_sys_bind(&vpss_src, &vo_dst);
    if (r) {
        return -1;
    }

    printf("[PHOTO_UVC] vpss grp=%d disp=%d %dx%d->%dx%d\n",
           c->grp, c->disp_chn, w, h, disp_w, disp_h);
    return 0;
}

static void photo_uvc_vpss_destroy(photo_uvc_vpss_t *c)
{
    mpi_chn_t vpss_src, vo_dst;

    if (c->disp_enabled) {
        vpss_src.mod_id = MPI_MOD_ID_VPSS;
        vpss_src.dev_id = c->grp;
        vpss_src.chn_id = c->disp_chn;
        vo_dst.mod_id = MPI_MOD_ID_VO;
        vo_dst.dev_id = 0;
        vo_dst.chn_id = c->vo_chn;
        mpi_sys_unbind(&vpss_src, &vo_dst);
        mpi_vo_stop_chn(c->vo_chn);
        mpi_vo_destroy_chn(c->vo_chn);
        mpi_vpss_disable_chn(c->grp, c->disp_chn);
        mpi_vpss_destroy_chn(c->grp, c->disp_chn);
    }
    if (c->started) {
        mpi_vpss_stop_grp(c->grp);
    }
    if (c->created) {
        mpi_vpss_destroy_grp(c->grp);
    }
    memset(c, 0, sizeof(*c));
}

/* ---- 写文件 / 写 buffer ---- */
static int photo_uvc_write_jpeg_file(const char *path, const u8 *data, u32 len)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        log_error("[PHOTO] UVC open file failed: %s\n", path);
        return -1;
    }
    size_t w = fwrite((void *)data, 1, len, fp);
    fclose(fp);
    if (w != len) {
        log_error("[PHOTO] UVC write file failed: %s (%d/%d)\n", path, (int)w, (int)len);
        return -1;
    }
    return 0;
}

static void photo_uvc_fill_last_name(const char *path)
{
    const char *name = strrchr(path, '/');
    snprintf(g_last_photo_name, sizeof(g_last_photo_name), "%s",
             name ? name + 1 : path);
}

/* ---- 拍照: 解码后的 YUV 帧经独立 VPSS group(IMC 缩放) -> VENC(JPEG) 编码 ----
 * 完全对齐 DVP 大图 app_camera_replay_encode_once 的做法:
 *   - 独立 photo group (src = VDEC 解码帧尺寸/格式)
 *   - cap chn 用 ONLINE 输出 (行缓冲, buf 极小), 而非 OFFLINE 整帧队列
 *   - VENC 用 ONLINE 输入, bind VPSS chn -> VENC, send 一帧后 get_stream
 * 每次拍照动态建/销毁, 目标分辨率随菜单档位变化。ONLINE 模式 buf = w*16*2,
 * 720P 仅约 40KB, 不会像 OFFLINE 整帧队列(3.7MB)那样 alloc 失败。 */
static int photo_uvc_capture_do(const mpi_video_frame_info_t *src_frame,
                                u16 cap_w, u16 cap_h, u8 quality,
                                u8 is_file, const char *path,
                                u8 *jpeg_buf, u32 buf_size, u32 *jpeg_size)
{
    mpi_vpss_grp_t photo_grp = -1;
    mpi_vpss_chn_t photo_chn = -1;
    mpi_venc_chn_t venc_chn = -1;
    mpi_vpss_grp_attr_t pga;
    mpi_vpss_chn_attr_t pca;
    mpi_venc_chn_attr_t va;
    mpi_chn_t vpss_src, venc_dst;
    mpi_stream_t enc_jpeg;
    u32 online_buf;
    int ok = -1;
    int r;

    /* 1. 独立 photo group (src = VDEC 解码帧) */
    r = mpi_vpss_find_free_grp(&photo_grp);
    if (r) {
        printf("[PHOTO_UVC] photo grp find fail\n");
        return -1;
    }
    memset(&pga, 0, sizeof(pga));
    pga.src_width = src_frame->width;
    pga.src_height = src_frame->height;
    pga.src_format = MPI_VPSS_SRC_FMT_YUV;
    pga.src_pixel_format = src_frame->format;
    pga.src_yuv_order = src_frame->yuv_order;
    pga.replay_isp_mode = 0;
    r = mpi_vpss_create_grp(photo_grp, &pga);
    if (r) {
        return -1;
    }
    r = mpi_vpss_start_grp(photo_grp);
    if (r) {
        mpi_vpss_destroy_grp(photo_grp);
        return -1;
    }

    /* 2. cap chn (目标分辨率, ONLINE 行缓冲) */
    r = mpi_vpss_find_free_chn(photo_grp, &photo_chn);
    if (r) {
        goto out_grp;
    }
    memset(&pca, 0, sizeof(pca));
    pca.width = cap_w;
    pca.height = cap_h;
    pca.format = src_frame->format;
    pca.yuv_order = src_frame->yuv_order;
    pca.output_mode = MPI_VPSS_OUTPUT_ONLINE;
    online_buf = (src_frame->format == MPI_PIXEL_FMT_YUV420SP)
                 ? (u32)cap_w * 16 * 3 / 2 : (u32)cap_w * 16 * 2;
    pca.buf_size = online_buf;
    r = mpi_vpss_create_chn(photo_grp, photo_chn, &pca);
    if (r) {
        goto out_grp;
    }
    r = mpi_vpss_enable_chn(photo_grp, photo_chn);
    if (r) {
        mpi_vpss_destroy_chn(photo_grp, photo_chn);
        goto out_grp;
    }

    /* 3. VENC(JPEG, ONLINE 输入) */
    r = mpi_venc_find_free_chn(&venc_chn);
    if (r) {
        goto out_chn;
    }
    memset(&va, 0, sizeof(va));
    va.hw_dev = 0;
    va.type = MPI_VENC_TYPE_JPEG;
    va.input_mode = MPI_VENC_INPUT_ONLINE;
    va.quality = quality;
    va.in_width = cap_w;
    va.in_height = cap_h;
    va.in_format = pca.format;
    va.in_yuv_order = pca.yuv_order;
    va.stream_buf_size = PHOTO_STREAM_BUF_SIZE;   /* 1MB, 与 DVP 大图一致 */
    r = mpi_venc_create_chn(venc_chn, &va);
    if (r) {
        goto out_chn;
    }
    r = mpi_venc_start_chn(venc_chn);
    if (r) {
        mpi_venc_destroy_chn(venc_chn);
        goto out_chn;
    }

    /* 4. bind VPSS chn -> VENC */
    vpss_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_src.dev_id = photo_grp;
    vpss_src.chn_id = photo_chn;
    venc_dst.mod_id = MPI_MOD_ID_VENC;
    venc_dst.dev_id = 0;
    venc_dst.chn_id = venc_chn;
    r = mpi_sys_bind(&vpss_src, &venc_dst);
    if (r) {
        goto out_venc;
    }

    /* 5. send 一帧 -> IMC 缩放 -> VENC */
    r = mpi_vpss_send_frame(photo_grp, src_frame, PHOTO_UVC_VPSS_TIMEOUT_MS);
    if (r) {
        printf("[PHOTO_UVC] photo send: %d\n", r);
        goto out_unbind;
    }

    /* 6. 取编码流 */
    memset(&enc_jpeg, 0, sizeof(enc_jpeg));
    r = mpi_venc_get_stream(venc_chn, &enc_jpeg, PHOTO_UVC_VENC_TIMEOUT_MS);
    if (r) {
        printf("[PHOTO_UVC] venc get_stream: %d\n", r);
        goto out_unbind;
    }

    if (is_file) {
        ok = photo_uvc_write_jpeg_file(path, (const u8 *)enc_jpeg.addr, enc_jpeg.len);
    } else if (jpeg_buf && enc_jpeg.len <= buf_size) {
        memcpy(jpeg_buf, enc_jpeg.addr, enc_jpeg.len);
        if (jpeg_size) {
            *jpeg_size = enc_jpeg.len;
        }
        ok = 0;
    }
    mpi_venc_release_stream(venc_chn, &enc_jpeg);

out_unbind:
    mpi_sys_unbind(&vpss_src, &venc_dst);
out_venc:
    mpi_venc_stop_chn(venc_chn);
    mpi_venc_destroy_chn(venc_chn);
out_chn:
    mpi_vpss_disable_chn(photo_grp, photo_chn);
    mpi_vpss_destroy_chn(photo_grp, photo_chn);
out_grp:
    mpi_vpss_stop_grp(photo_grp);
    mpi_vpss_destroy_grp(photo_grp);
    return ok;
}

/* H264 Annex-B 流里检测 IDR 帧 (NAL type 5) */
static int photo_uvc_is_idr(const u8 *data, u32 len)
{
    u32 off = 0;
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
        if ((data[off + sl] & 0x1f) == 5) {
            return 1;
        }
        off += sl + 1;
    }
    return 0;
}

/* ---- pipeline 线程 ---- */
static void photo_uvc_pipeline_thread(void *arg)
{
    photo_uvc_buf_pool_t *pool = NULL;
    mpi_video_frame_info_t yuv = {0};
    u8  *raw = NULL;
    u32 raw_len = 0;
    u8  is_h264 = 0;
    int ret;

    (void)arg;

    ret = photo_uvc_buf_pool_init(&pool);
    if (ret) {
        printf("[PHOTO_UVC] buf pool init failed\n");
        goto out;
    }

    /* 先收一帧确定格式 */
    if (photo_uvc_buf_pool_get(pool, &raw, &raw_len, &is_h264, PHOTO_UVC_VDEC_TIMEOUT_MS)) {
        printf("[PHOTO_UVC] no first frame\n");
        goto out_pool;
    }
    g_uvc_is_h264 = is_h264;
    photo_uvc_buf_pool_unlock(pool, raw);

    if (!g_uvc_is_h264) {
        /* JPEG: VDEC 解码 + VPSS 显示; H264 直通不解码不显示, 两样都不建 */
        if (photo_uvc_vdec_create(&g_uvc_vdec, 0)) {
            printf("[PHOTO_UVC] vdec create failed\n");
            goto out_pool;
        }
        if (photo_uvc_vpss_create(&g_uvc_vpss, g_uvc_width, g_uvc_height,
                                  g_photo_disp_attr.width, g_photo_disp_attr.height)) {
            printf("[PHOTO_UVC] vpss create failed\n");
            goto out_vdec;
        }
    }

    g_uvc_running = 1;
    printf("[PHOTO_UVC] pipeline running (%s %dx%d)\n",
           g_uvc_is_h264 ? "H264" : "JPEG", g_uvc_width, g_uvc_height);

    while (!g_uvc_exit) {
        if (photo_uvc_buf_pool_get(pool, &raw, &raw_len, NULL, 500)) {
            os_time_dly(1);
            continue;
        }

        if (g_uvc_is_h264) {
            /* H264 直通拍照: 不解码不显示, 拍照请求到达后抓下一个 IDR 关键帧直存 */
            if (g_uvc_capture_req) {
                if (photo_uvc_is_idr(raw, raw_len)) {
                    if (g_uvc_capture_is_file) {
                        if (photo_uvc_write_jpeg_file(g_uvc_photo_path, raw, raw_len) == 0) {
                            /* .h264 不入媒体库, 清空 last_name 避免 photo_flm_add_new 报 failed */
                            g_last_photo_name[0] = '\0';
                        }
                    } else if (g_uvc_jpeg_buf && raw_len <= g_uvc_jpeg_buf_size) {
                        memcpy(g_uvc_jpeg_buf, raw, raw_len);
                        if (g_uvc_jpeg_size) {
                            *g_uvc_jpeg_size = raw_len;
                        }
                    }
                    g_uvc_capture_ok = 1;
                    g_uvc_capture_req = 0;
                }
                /* 非 IDR 帧继续等下一个 IDR */
            }
        } else {
            /* JPEG: 解码一帧 YUV */
            if (photo_uvc_vdec_decode(&g_uvc_vdec, raw, raw_len, &yuv,
                                      PHOTO_UVC_VDEC_TIMEOUT_MS) == 0) {
                if (g_uvc_capture_req) {
                    /* 拍照: 动态建 cap chn(IMC 缩放) + VENC, send 一帧同时预览+缩放编码 */
                    int ok = photo_uvc_capture_do(&yuv, g_uvc_capture_w, g_uvc_capture_h, g_uvc_capture_q,
                                                  g_uvc_capture_is_file, g_uvc_photo_path,
                                                  g_uvc_jpeg_buf, g_uvc_jpeg_buf_size, g_uvc_jpeg_size);
                    if (ok == 0 && g_uvc_capture_is_file) {
                        photo_uvc_fill_last_name(g_uvc_photo_path);
                    }
                    g_uvc_capture_ok = (ok == 0) ? 1 : 0;
                    g_uvc_capture_req = 0;
                } else {
                    /* 仅预览: 送 VPSS -> VO */
                    mpi_vpss_send_frame(g_uvc_vpss.grp, &yuv, PHOTO_UVC_VPSS_TIMEOUT_MS);
                }

                mpi_vdec_release_frame(g_uvc_vdec.chn, &yuv);
            }
        }

        photo_uvc_buf_pool_unlock(pool, raw);
    }

    printf("[PHOTO_UVC] pipeline exit\n");

    if (!g_uvc_is_h264) {
        photo_uvc_vpss_destroy(&g_uvc_vpss);
    }
out_vdec:
    if (!g_uvc_is_h264) {
        photo_uvc_vdec_destroy(&g_uvc_vdec);
    }
out_pool:
    photo_uvc_buf_pool_deinit(pool);
out:
    g_uvc_running = 0;
    g_uvc_pipeline_pid = 0;
}

/* ---- 拍照到文件 (photo_capture 调用) ---- */
static int photo_uvc_capture_to_file(const char *path, u16 cap_w, u16 cap_h, u8 quality)
{
    int wait = 0;
    int wait_max = 300;

    if (!g_uvc_running) {
        log_error("[PHOTO] UVC preview not ready\n");
        return -1;
    }

    if (g_uvc_is_h264) {
        /* H264 直通: 抓 IDR 关键帧直接存 (文件名已由 photo_make_filename 生成
         * H264****.H26), 忽略分辨率/质量 */
        strncpy(g_uvc_photo_path, path, sizeof(g_uvc_photo_path) - 1);
        g_uvc_photo_path[sizeof(g_uvc_photo_path) - 1] = '\0';

        /* 禁用 SD 低功耗: 否则拍照写文件瞬间 SD 可能处于低功耗, 数据不落盘 */
        {
            char sd_dev[4] = {0};
            const char *sd = strstr(path, "storage/");
            if (sd) {
                sd += 8;
                const char *end = strchr(sd, '/');
                int slen = end ? (int)(end - sd) : (int)strlen(sd);
                if (slen > 0 && slen < 4) {
                    memcpy(sd_dev, sd, slen);
                }
            }
            if (sd_dev[0]) {
                extern void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);
                sdmmc_dev_lowpower_disable(sd_dev, 1);
            }
        }

        wait_max = 3000;  /* 等 IDR 帧: 摄像头 IDR 间隔可能 1~2s */
    } else {
        g_uvc_capture_w = cap_w;
        g_uvc_capture_h = cap_h;
        g_uvc_capture_q = quality;
        strncpy(g_uvc_photo_path, path, sizeof(g_uvc_photo_path) - 1);
        g_uvc_photo_path[sizeof(g_uvc_photo_path) - 1] = '\0';
    }

    g_uvc_capture_is_file = 1;
    g_uvc_capture_ok = 0;
    g_uvc_capture_req = 1;

    while (g_uvc_capture_req && wait++ < wait_max) {
        os_time_dly(1);   /* 1ms 步进 */
    }
    if (g_uvc_capture_req) {
        log_error("[PHOTO] UVC capture timeout\n");
        g_uvc_capture_req = 0;
        return -1;
    }
    return g_uvc_capture_ok ? 0 : -1;
}

/* ---- 拍照到 buffer (photo_capture_jpeg_buffer 调用) ---- */
static int photo_uvc_capture_to_buffer(unsigned char *buf, unsigned int buf_size,
                                       unsigned int *size, u16 cap_w, u16 cap_h, u8 quality)
{
    int wait = 0;
    int wait_max = 300;

    if (!g_uvc_running) {
        return -1;
    }

    if (g_uvc_is_h264) {
        /* H264 直通: 抓 IDR 关键帧原始数据, 忽略分辨率/质量 */
        wait_max = 3000;  /* 等 IDR 帧 */
    } else {
        g_uvc_capture_w = cap_w;
        g_uvc_capture_h = cap_h;
        g_uvc_capture_q = quality;
    }
    g_uvc_jpeg_buf = buf;
    g_uvc_jpeg_buf_size = buf_size;
    g_uvc_jpeg_size = size;
    g_uvc_capture_is_file = 0;
    g_uvc_capture_ok = 0;
    g_uvc_capture_req = 1;

    while (g_uvc_capture_req && wait++ < wait_max) {
        os_time_dly(1);
    }
    if (g_uvc_capture_req) {
        g_uvc_capture_req = 0;
        return -1;
    }
    return g_uvc_capture_ok ? 0 : -1;
}

/* ---- 打开 / 关闭 ---- */
static int photo_uvc_open(void)
{
    int rw = 640, rh = 480;

    g_uvc_is_h264 = uvc_host_is_h264_format() ? 1 : 0;
    uvc_host_get_cur_resolution(&rw, &rh);
    if (rw > 0 && rh > 0) {
        g_uvc_width = (u16)rw;
        g_uvc_height = (u16)rh;
    }

    g_uvc_exit = 0;
    if (thread_fork("photo_uvc_pipe", 20, 8192, 20,
                    &g_uvc_pipeline_pid, photo_uvc_pipeline_thread, NULL)) {
        printf("[PHOTO_UVC] pipeline fork fail\n");
        return -1;
    }

    /* 先启动摄像头收流: 帧回调 -> buf_pool -> pipeline 收第一帧确定格式。
     * 若在 pipeline 就绪后才 start_camera, 会死锁(pipeline 等第一帧, start 等就绪)。 */
    uvc_host_set_target_reso(g_uvc_width, g_uvc_height);
    uvc_host_start_camera();

    /* 再等 pipeline 就绪 (收到第一帧后创建 VDEC/VPSS/VENC) */
    {
        int wait = 0;
        while (!g_uvc_running && wait++ < 500) {
            os_time_dly(1);
        }
    }
    if (!g_uvc_running) {
        printf("[PHOTO_UVC] pipeline not ready\n");
        return -1;
    }

    printf("[PHOTO_UVC] camera started (%s %dx%d)\n",
           g_uvc_is_h264 ? "H264" : "JPEG", g_uvc_width, g_uvc_height);
    return 0;
}

static void photo_uvc_close(void)
{
    g_uvc_exit = 1;
    if (g_uvc_pipeline_pid) {
        thread_kill(&g_uvc_pipeline_pid, KILL_WAIT);
        g_uvc_pipeline_pid = 0;
    }
    g_uvc_running = 0;
    g_uvc_capture_req = 0;
    printf("[PHOTO_UVC] closed\n");
}

#endif /* TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE */

// ── 拍照核心 ────────────────────────────────────────────────
/* 复用 init 打开的 g_photo_cam 预览句柄, 直接 take_photo (每次 open 会双句柄冲突) */

int photo_capture(void)
{
    if (g_ctx.capturing) {
        log_error("[PHOTO] busy\n");
        return -1;
    }
#if TCFG_CAMERA_DVP_ENABLE
    /* 复用持久预览句柄; 不再 open/close */
    if (!g_photo_cam) {
        log_error("[PHOTO] no camera handle (preview not started)\n");
        return -1;
    }
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    if (!g_uvc_running) {
        log_error("[PHOTO] uvc preview not started\n");
        return -1;
    }
#endif
    /* 无卡/存储不足弹窗提示 (photo_precheck 静默, 用户要求有提示) */
    if (photo_sd_ok() != 0) {
        photo_ui_show_alert(1);  /* 请插入SD卡 */
        return -1;
    }
    if (flm_sd_is_exfat()) {
        log_error("[PHOTO] exFAT card not supported, please use FAT32\n");
        photo_ui_show_alert(3);  /* 请使用FAT32格式的SD卡 */
        return -1;
    }
    if (photo_storage_ok() != 0) {
        photo_ui_show_alert(0);  /* 存储已满 */
        return -1;
    }
    g_ctx.capturing = 1;
    char path[PHOTO_PATH_MAX];
    photo_make_filename(path, sizeof(path));
    g_ctx.index++;
    log_info("[PHOTO] -> %s\n", path);
#if TCFG_CAMERA_DVP_ENABLE
    /* 画质菜单档位(0=48M..12=1M)映射 JPEG 压缩质量: 高像素档->高质量(95), 低档->较低(75)。
     * 只调压缩率, 不改分辨率(GC0308 固定输出, 改分辨率有运行时风险)。 */
    u8 q_idx = kid_settings_get_img_quality_idx();
    if (q_idx >= ARRAY_SIZE(PHOTO_RES)) {
        q_idx = PHOTO_RES_DEFAULT_INDEX;
    }
    // 统一50: 映射out_buf=0.5*w*h(154KB), 80则0.7(215KB)超预算
    u8 jpeg_q = 50;
    (void)q_idx;
    u16 photo_w = PHOTO_RES[q_idx][0];
    u16 photo_h = PHOTO_RES[q_idx][1];
    /* 内存预算(关网+DMM后+~200KB): 640x480 拍照 out_buf 215KB+stream 64KB 可过;
     * >640x480(1M及以上档)无意义且超预算, 钳到 VGA */
    if (photo_w > 640 || photo_h > 480) {
        photo_w = 640;
        photo_h = 480;
        log_info("[PHOTO] clamp to 640x480 (VGA)\n");
    }
    log_info("[PHOTO] size idx=%u %ux%u quality=%u\n",
             q_idx, photo_w, photo_h, jpeg_q);
    app_camera_photo_attr_t attr = {
        .width   = photo_w,
        .height  = photo_h,
        .quality = jpeg_q,
        .stream_buf_size = PHOTO_STREAM_BUF_SIZE,
        .effect = g_ctx.effect,
        /* 无PSRAM: thumbnail_enable=1 必走全帧大图流程(need 615KB), 必须关闭 */
        .thumbnail_enable = 0,
        .sticker_path = (g_ctx.effect == JLGPU_FILTER_STICKER) ? photo_ui_get_sticker_save_path_by_idx(g_ctx.sticker_idx, photo_w, photo_h) : NULL,
    };
    strncpy(attr.path, path, sizeof(attr.path) - 1);
    app_camera_ret_t ret = app_camera_take_photo(g_photo_cam, &attr);
    if (ret != APP_CAMERA_OK) {
        log_error("[PHOTO] take_photo failed: %d\n", ret);
        g_ctx.capturing = 0;
        /* P0: JPG编码失败提示 — type=2 系统错误弹窗 (与 video_rec 约定一致)。
         * index 回滚: 编码失败不计入序号, 避免文件名空洞。 */
        if (g_ctx.index > 0) {
            g_ctx.index--;
        }
        photo_ui_show_alert(2);
        return -1;
    }
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    if (g_uvc_is_h264) {
        /* H264 直通: 抓 IDR 关键帧存 .h264, 忽略分辨率/质量档位 */
        log_info("[PHOTO] uvc h264 passthrough capture\n");
        if (photo_uvc_capture_to_file(path, 0, 0, 0) != 0) {
            log_error("[PHOTO] uvc h264 capture failed\n");
            g_ctx.capturing = 0;
            if (g_ctx.index > 0) {
                g_ctx.index--;
            }
            photo_ui_show_alert(2);
            return -1;
        }
    } else {
        /* UVC JPEG: VDEC 解码 -> VPSS(IMC 缩放) -> VENC, 分辨率/质量均按菜单档位 */
        u8 q_idx = kid_settings_get_img_quality_idx();
        if (q_idx >= ARRAY_SIZE(PHOTO_RES)) {
            q_idx = PHOTO_RES_DEFAULT_INDEX;
        }
        u16 cap_w = PHOTO_RES[q_idx][0];
        u16 cap_h = PHOTO_RES[q_idx][1];
        u8 jpeg_q;
        if (q_idx == 11 || q_idx == 12) {
            jpeg_q = 80;
        } else {
            jpeg_q = (q_idx <= 2) ? 80 : (q_idx <= 6) ? 75 : (q_idx <= 9) ? 70 : 65;
        }
        log_info("[PHOTO] uvc size idx=%u %ux%u q=%u\n", q_idx, cap_w, cap_h, jpeg_q);
        if (photo_uvc_capture_to_file(path, cap_w, cap_h, jpeg_q) != 0) {
            log_error("[PHOTO] uvc capture failed\n");
            g_ctx.capturing = 0;
            if (g_ctx.index > 0) {
                g_ctx.index--;
            }
            photo_ui_show_alert(2);
            return -1;
        }
    }
#endif
    g_ctx.total++;
    g_ctx.capturing = 0;
    photo_effect_shutter();
    photo_effect_flash();
    photo_effect_thumbnail(path);
    photo_flm_add_new();
    log_info("[PHOTO] ok (total=%u)\n", g_ctx.total);
    return 0;
}
int photo_capture_jpeg_buffer(unsigned char *jpeg_buf, unsigned int jpeg_buf_size,
                              unsigned int *jpeg_size)
{
    if (!jpeg_buf || !jpeg_buf_size || !jpeg_size || g_ctx.capturing) {
        return -1;
    }
    *jpeg_size = 0;
#if TCFG_CAMERA_DVP_ENABLE
    app_camera_photo_attr_t attr = {0};
    app_camera_ret_t ret;

    if (!g_photo_cam) {
        log_error("[PHOTO] AI capture: preview not ready\n");
        return -1;
    }

    g_ctx.capturing = 1;
    attr.width = PHOTO_AI_WIDTH;
    attr.height = PHOTO_AI_HEIGHT;
    attr.quality = PHOTO_AI_QUALITY;
    attr.stream_buf_size = jpeg_buf_size;
    attr.yuv420 = 1;
    attr.jpeg_buf = jpeg_buf;
    attr.jpeg_buf_size = jpeg_buf_size;
    attr.jpeg_size = jpeg_size;

    ret = app_camera_take_photo(g_photo_cam, &attr);
    g_ctx.capturing = 0;
    if (ret != APP_CAMERA_OK) {
        log_error("[PHOTO] AI capture failed: %d\n", ret);
        return -1;
    }

    log_info("[PHOTO] AI JPEG ready: %u bytes %ux%u q=%u\n",
             *jpeg_size, PHOTO_AI_WIDTH, PHOTO_AI_HEIGHT, PHOTO_AI_QUALITY);
    return 0;
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    g_ctx.capturing = 1;
    int ret = photo_uvc_capture_to_buffer(jpeg_buf, jpeg_buf_size, jpeg_size,
                                          PHOTO_AI_WIDTH, PHOTO_AI_HEIGHT, PHOTO_AI_QUALITY);
    g_ctx.capturing = 0;
    if (ret != 0) {
        log_error("[PHOTO] UVC AI capture failed\n");
        return -1;
    }
    log_info("[PHOTO] UVC AI JPEG ready: %u bytes\n", *jpeg_size);
    return 0;
#endif
}

// ── 连拍 ────────────────────────────────────────────────────
int photo_burst_start(void)
{
    if (g_ctx.state == PHOTO_STATE_BUSY) {
        return -1;
    }
    g_ctx.mode = PHOTO_MODE_BURST;
    g_ctx.burst_cnt = 0;
    log_info("[PHOTO] burst start\n");
    return 0;
}
int photo_burst_stop(void)
{
    log_info("[PHOTO] burst stop (%u photos)\n", g_ctx.burst_cnt);
    g_ctx.mode = PHOTO_MODE_SINGLE;
    g_ctx.burst_cnt = 0;
    return 0;
}
int photo_burst_tick(void)
{
    if (g_ctx.burst_cnt >= PHOTO_BURST_MAX) {
        photo_burst_stop();
        return 0;
    }
    if (photo_capture() == 0) {
        g_ctx.burst_cnt++;
        return g_ctx.burst_cnt;
    }
    log_error("[PHOTO] burst stopped: capture failed at #%u\n", g_ctx.burst_cnt);
    photo_burst_stop();
    return -1;
}
// ── 定时拍照 ────────────────────────────────────────────────
#define PHOTO_EFFECT_COUNT   (JLGPU_FILTER_NEGATIVE + 1)

static u8 g_photo_timed_sec = 0;   /* 定时拍照秒数偏好, 来自设置菜单(0关/2/5/10) */

static const u8 photo_timer_seconds[] = {
    [PHOTO_MODE_TIMER_3S]  = 3,
    [PHOTO_MODE_TIMER_5S]  = 5,
    [PHOTO_MODE_TIMER_10S] = 10,
};
int photo_timer_cancel(void)
{
    log_info("[PHOTO] timer cancelled\n");
    g_ctx.timer_left = 0;
    g_ctx.mode = PHOTO_MODE_SINGLE;
    return 0;
}
/* 按拍照键入口: 菜单设了定时秒数则启动倒计时, 否则直接拍照 */
void photo_start_timed(void)
{
    if (g_photo_timed_sec > 0) {
        g_ctx.timer_left = g_photo_timed_sec;
        log_info("[PHOTO] timed capture: %us countdown\n", g_photo_timed_sec);
    } else {
        photo_capture();
    }
}
int photo_timer_tick(void)
{
    if (g_ctx.timer_left == 0) {
        return 0;
    }
    g_ctx.timer_left--;
    log_info("[PHOTO] countdown: %us\n", g_ctx.timer_left);
    if (g_ctx.timer_left == 0) {
        if (photo_capture() != 0) {
            log_error("[PHOTO] timer capture failed\n");
            photo_ui_show_alert(2); /* 拍照失败 */
        }
        g_ctx.mode = PHOTO_MODE_SINGLE;
    }
    return g_ctx.timer_left;
}
// ── 模式切换 ────────────────────────────────────────────────
int photo_mode_set(u8 mode)
{
    if (mode > PHOTO_MODE_BURST) {
        return -1;
    }
    if (g_ctx.timer_left) {
        photo_timer_cancel();
    }
    if (g_ctx.mode == PHOTO_MODE_BURST && mode != PHOTO_MODE_BURST) {
        photo_burst_stop();
    }
    g_ctx.mode = mode;
    if (mode >= PHOTO_MODE_TIMER_3S && mode <= PHOTO_MODE_TIMER_10S) {
        g_ctx.timer_left = photo_timer_seconds[mode];
        log_info("[PHOTO] timer %us started\n", g_ctx.timer_left);
    } else if (mode == PHOTO_MODE_BURST) {
        photo_burst_start();
    }
    return 0;
}
u8 photo_mode_get(void)
{
    return g_ctx.mode;
}
// ── 状态 ────────────────────────────────────────────────────
enum photo_state photo_state_get(void)
{
    if (g_ctx.capturing) {
        return PHOTO_STATE_BUSY;
    }
    if (g_ctx.timer_left) {
        return PHOTO_STATE_COUNTDOWN;
    }
    if (g_ctx.mode == PHOTO_MODE_BURST) {
        return PHOTO_STATE_BURST;
    }
    return PHOTO_STATE_IDLE;
}
u32 photo_total(void)
{
    return g_ctx.total;
}
// ── 生命周期 ────────────────────────────────────────────────
#if TCFG_CAMERA_DVP_ENABLE
/* 拍照 OSD 叠加日期时间水印, has_time 由 OSD 模块每秒刷新 */

static void photo_osd_enable(app_camera_handle_t cam)
{
    if (!cam) {
        return;
    }
    app_camera_osd_attr_t osd = {
        .x    = 0,                  /* 左下角 (320x240 LCD 视口, 避开模式栏) */
        .y    = 200,
        .text = "hh:mm:ss",
    };
#if 0
    if (app_camera_set_display_text_osd(cam, &osd) != APP_CAMERA_OK) {
        log_error("[PHOTO] set display OSD failed\n");
        return;
    }
    if (app_camera_enable_display_osd(cam) != APP_CAMERA_OK) {
        log_error("[PHOTO] enable display OSD failed\n");
        return;
    }
    log_info("[PHOTO] display OSD (date/time) enabled\n");
#endif
}
static void photo_osd_disable(app_camera_handle_t cam)
{
    if (!cam) {
        return;
    }
    app_camera_disable_display_osd(cam);
    log_info("[PHOTO] display OSD disabled\n");
}
#endif /* TCFG_CAMERA_DVP_ENABLE */
int photo_init(void)
{
    log_info("[PHOTO] init\n");
    memset(&g_ctx, 0, sizeof(g_ctx));
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    /* H264 检测提前到 UI init 之前: H264 不显示预览, 不创建 fb5 视频显示对象 */
    g_uvc_is_h264 = uvc_host_is_h264_format() ? 1 : 0;
#endif
    int show_video = 1;
#if (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    show_video = g_uvc_is_h264 ? 0 : 1;
#endif
    photo_ui_init(g_photo_disp_attr.width, g_photo_disp_attr.height, g_photo_disp_attr.left, g_photo_disp_attr.top, show_video);
    g_ctx.zoom_tenths = 10;
    g_ctx.mode = PHOTO_MODE_SINGLE;
    g_ctx.effect = 0;
    g_photo_timed_sec = kid_settings_get_photo_timed_sec();  /* 定时拍照(菜单) */
    g_ctx.sticker_idx = 0;
    photo_effect_show_current();

#if TCFG_CAMERA_DVP_ENABLE
    /* 打开摄像头并启动 LCD 预览 (持久预览); 任一步失败都向上返回错误 */
    app_camera_ret_t ret = app_camera_open(0, "GC0308", &g_photo_cam);
    if (ret != APP_CAMERA_OK) {
        log_error("[PHOTO] camera open failed: %d\n", ret);
        return -1;
    }
    ret = app_camera_display_start(g_photo_cam, &g_photo_disp_attr);
    if (ret != APP_CAMERA_OK) {
        log_error("[PHOTO] display start failed: %d\n", ret);
        app_camera_close(&g_photo_cam);
        return -1;
    }
    log_info("[PHOTO] camera preview started\n");
    /* 强制 LVGL 刷新视频对象: 避免首次进入时首帧未触发渲染, 背景/白屏盖住摄像头 */
    photo_ui_video_refresh();
    photo_osd_enable(g_photo_cam);
    app_camera_set_last_photo_file_buffer(g_photo_cam, g_last_photo_name);
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    /* 打开 UVC 摄像头并启动 LCD 预览 (pipeline 线程) */
    if (photo_uvc_open() != 0) {
        log_error("[PHOTO] uvc open failed\n");
    }
#endif

    return 0;
}
void photo_exit(void)
{
    log_info("[PHOTO] exit (total=%u)\n", g_ctx.total);
    photo_zoom_repeat_stop();
    photo_ui_deinit();
    if (g_ctx.mode == PHOTO_MODE_BURST) {
        photo_burst_stop();
    }
    if (g_ctx.timer_left) {
        photo_timer_cancel();
    }

    /* A synchronous JPEG capture owns the preview handle until encoding finishes. */
    int capture_wait = 0;
    while (g_ctx.capturing && capture_wait < 600) {
        os_time_dly(1);
        capture_wait++;
    }
    if (g_ctx.capturing) {
        log_error("[PHOTO] capture still busy during exit\n");
    }

#if TCFG_CAMERA_DVP_ENABLE
    /* 关闭摄像头预览 */
    if (g_photo_cam) {
        photo_osd_disable(g_photo_cam);
        app_camera_display_stop(g_photo_cam);
        app_camera_close(&g_photo_cam);

        log_info("[PHOTO] camera preview stopped\n");
    }
#elif (TCFG_HOST_UVC_ENABLE && TCFG_USB_HOST_ENABLE)
    photo_uvc_close();
#endif
}
/* ---------- SD/存储检查 (内部) ---------- */
/* 统一前置检查(卡/存储), show_alert 控制是否弹窗 */

static int photo_precheck_internal(int show_alert)
{
    if (photo_sd_ok() != 0) {
        if (show_alert) {
            photo_ui_show_alert(1);
        }
        return -1;
    }
    if (photo_storage_ok() != 0) {
        if (show_alert) {
            photo_ui_show_alert(0);
        }
        return -1;
    }
    return 0;
}
/* photo_capture() 内部使用: 静默 */
int photo_precheck(void)
{
    return photo_precheck_internal(0);
}
/* 主循环按键使用: 弹窗 */
static int photo_check_sd(void)
{
    return photo_precheck_internal(1);
}
/* ---------- 模式主循环 ---------- */
/* 周期任务走 sys_timer 100ms: app_get_message 阻塞等待使 tick 分支恒不执行(死代码) */

static u16 s_photo_periodic_timer = 0;
static u32 s_photo_tick_100ms = 0;
static enum photo_state s_photo_prev_state = PHOTO_STATE_IDLE;

static void photo_periodic_tick(void *priv)
{
    (void)priv;
    s_photo_tick_100ms++;

    /* 长按连续变焦步进 */
    if (s_photo_zoom_repeat_dir != 0 &&
        ++s_photo_zoom_repeat_ticks >= PHOTO_ZOOM_REPEAT_TICKS) {
        u8 previous_zoom = g_ctx.zoom_tenths;

        s_photo_zoom_repeat_ticks = 0;
        if (s_photo_zoom_repeat_dir > 0) {
            photo_zoom_in();
        } else {
            photo_zoom_out();
        }
        photo_ui_update_zoom(photo_zoom_get());
        /* 到顶/到底(或应用失败)后级别不再变, 停掉避免空转 */
        if (g_ctx.zoom_tenths == previous_zoom) {
            photo_zoom_repeat_stop();
        }
    }

    /* 连拍: 每 200ms (2 * 100ms) */
    if (photo_mode_get() == PHOTO_MODE_BURST && (s_photo_tick_100ms % 2) == 0) {
        int burst_ret = photo_burst_tick();
        if (burst_ret < 0) {
            photo_ui_show_alert(2);
            photo_mode_set(PHOTO_MODE_SINGLE);
            photo_ui_on_state_change(PHOTO_STATE_BURST, PHOTO_STATE_IDLE);
            s_photo_prev_state = PHOTO_STATE_IDLE;
        }
    }

    /* 倒计时 + 电池: 每 1000ms (10 * 100ms) */
    if ((s_photo_tick_100ms % 10) == 0) {
        photo_ui_update_battery();
        int left = photo_timer_tick();
        if (left >= 0) {
            photo_ui_update_countdown(left);
        }
    }

    /* 状态变化 -> UI 通知(每 100ms 检测一次, 及时反映拍照中/连拍中等状态) */
    enum photo_state now = photo_state_get();
    if (now != s_photo_prev_state) {
        photo_ui_on_state_change(s_photo_prev_state, now);
        s_photo_prev_state = now;
    }
}

struct app_mode *app_enter_photo_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;
    log_info("[PHOTO] app_enter_photo_mode, arg=%d\n", arg);
    g_photo_ai_tools = (arg == PHOTO_ENTER_AI_TOOLS);

    /* AI tools borrows the photo pipeline but must not replace boot mode memory. */
    if (!g_photo_ai_tools) {
        u8 _mode = APP_MODE_PHOTO;
        syscfg_write(VM_ID_MODE_MEMORY, &_mode, sizeof(_mode));
    }

    if (photo_init() != 0) {
        log_error("[PHOTO] photo_init failed, back to idle\n");
        photo_exit();
        app_task_switch_to(APP_MODE_IDLE, 0);
        return NULL;
    }
    if (!g_photo_ai_tools) {
        photo_ui_on_state_change(PHOTO_STATE_IDLE, PHOTO_STATE_IDLE);
        photo_ui_update_zoom(photo_zoom_get());
    }
    /* 拍照模式已稳定, 可接受 CLICK 拍照 / HOLD 切模式 */
    g_ctx.ready = 1;
    log_info("[PHOTO] mode ready\n");
    s_photo_tick_100ms = 0;
    s_photo_prev_state = PHOTO_STATE_IDLE;
    if (!g_photo_ai_tools && !s_photo_periodic_timer) {
        s_photo_periodic_timer = sys_timer_add(NULL, photo_periodic_tick, 100);
    }
    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), video_photo_mode_key_table)) {
            /* 倒计时/连拍/状态刷新已移至 photo_periodic_tick (sys_timer_add,
             * 见函数前注释的根因说明), 此分支不再需要处理周期性逻辑。 */
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            if (photo_mode_get() == PHOTO_MODE_BURST) {
                photo_burst_stop();
            }
            if (photo_state_get() == PHOTO_STATE_COUNTDOWN) {
                photo_timer_cancel();
            }
            if (s_photo_periodic_timer) {
                sys_timer_del(s_photo_periodic_timer);
                s_photo_periodic_timer = 0;
            }
            break;
        }
        switch (msg[0]) {
        case MSG_FROM_APP:
            video_photo_app_msg_handler(msg + 1);
            break;
        }
        app_default_msg_handler(msg);
    }
    if (s_photo_periodic_timer) {
        sys_timer_del(s_photo_periodic_timer);
        s_photo_periodic_timer = 0;
    }
    photo_exit();
    if (!g_photo_ai_tools) {
        photo_ui_on_state_change(photo_state_get(), PHOTO_STATE_IDLE);
    }
    g_photo_ai_tools = 0;
    log_info("[PHOTO] exit\n");
    return next_mode;
}
/*==============================================================================
 * 模式try_enter/try_exit
 *==============================================================================
 */
static int photo_mode_try_enter(int arg)
{
    log_info("photo_mode_try_enter\n");
    /* 进拍照(吃内存模式)前释放回放遗留的全库名字缓存(87.8KB): 该缓存跨"多宫格↔全屏"
     * 保持是为了避免每次进回放重建(实测重建 2.3s), 但不能带进拍照/录像这类模式。 */
    flm_names_cache_release();
    return 0;
}
static int photo_mode_try_exit(void)
{
    log_info("photo_mode_try_exit\n");
    return 0;
}
static const struct app_mode_ops photo_mode_ops = {
    .try_enter  = photo_mode_try_enter,
    .try_exit   = photo_mode_try_exit,
};
REGISTER_APP_MODE(photo_mode) = {
    .name   = APP_MODE_PHOTO,
    .index = APP_MODE_PHOTO_INDEX,
    .ops    = &photo_mode_ops,
};
#endif
