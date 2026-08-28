/**
 * @brief  KWT Video — Camera → JPEG encode → vstream sender bridge.
 *
 *         Opens the camera, sets up a VPSS→VENC(JPEG) hardware pipeline,
 *         polls encoded frames, and feeds them into vstream_send_frame()
 *         for transmission over SPP to the remote KWT device.
 *
 *         Reference: apps/common/video/mpi_app/camera/app_camera_net_stream.c
 *         (Uses the same hardware pipeline; net_rt_send_frame→UDP replaced
 *          by vstream_send_frame→SPP.)
 */

#include "system/includes.h"
#include "app_config.h"
#include "app_camera_mpi.h"
#include "app_camera_mpi_priv.h"   /* for direct access to ctx->vpss_grp */
#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "vstream_common.h"
#include "kid_wt_video.h"
#include "app_playback_mpi.h"
#include "lvgl_video_init.h"
#include "lv_port_video_disp.h"   /* lv_port_video_disp_get_instance_by_fb_index() */
#include "asm/dcache.h"   /* DcuFlushinvRegion — flush CPU-written fb before putmap */
#include "fb.h"           /* fb_draw_open/getmap/putmap/close */
#if KWT_USE_H264
#include "mpi_vdec.h"   /* SOFT_H264 VDEC (replaces video_1 decode) */
#include "mpi_vo.h"     /* VO hardware producer for fb5 */
#endif

#define LOG_TAG             "[KWT_VIDEO]"
#define LOG_ERROR_ENABLE
//#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_VSTREAM_TRANSFER

/* ============================================================================
 * Context
 * ============================================================================ */

typedef struct {
    app_camera_handle_t cam_handle;     /* camera context from app_camera_open() */
    mpi_vpss_grp_t      vpss_grp;       /* cached VPSS group for worker thread */
    mpi_vpss_chn_t      vpss_chn;       /* VPSS output channel → VENC */
    mpi_venc_chn_t      venc_chn;       /* JPEG encoder channel */
    u8                  exit_flag;      /* signal worker thread to stop */
    int                 thread_pid;     /* worker thread handle */
    void               *pip_fb_draw;        /* fb_draw producer handle for PIP (fb6) */
    u8                  pip_fb_opened;       /* fb_draw_open() succeeded */
} kwt_xsmt_ctx_t;

static kwt_xsmt_ctx_t g_xsmt_ctx;

/* cached params (mirrors mpi_video_net.c pattern) */
static int g_xsmt_last_width  = KWT_CAMERA_WIDTH;
static int g_xsmt_last_height = KWT_CAMERA_HEIGHT;
static int g_xsmt_last_fps    = KWT_CAMERA_FPS;
static int g_xsmt_last_abr    = KWT_CAMERA_ABR_KBPS;
static OS_SEM g_frames_held;

/* ============================================================================
 * Internal: VPSS output channel for VENC
 *
 * Creates a VPSS ONLINE output channel that feeds directly into the JPEG
 * encoder.  Pattern from app_camera_vpss_venc_net_open().
 * ============================================================================ */

static int kwt_xsmt_vpss_venc_open(app_camera_handle_t ctx)
{
    int ret;

    /* --- find a free VPSS channel in the camera's VPSS group --- */
    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &g_xsmt_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: no free VPSS channel (ret=%d)\n", __func__, ret);
        return -1;
    }

    /* --- configure VPSS channel (OFFLINE: app reads frames, manual encode) ---
     * buf_size must be > 0 for local-queue mode; sink-frame mode (size=0)
     * cannot serve mpi_vpss_get_chn_frame().  2× for double-buffering. */
    mpi_vpss_chn_attr_t vpss_attr = {
        .width       = KWT_CAMERA_WIDTH,
        .height      = KWT_CAMERA_HEIGHT,
#if KWT_USE_H264
        /* SOFT_264 requires YUV420SP (semi-planar NV12). yuv_order is ignored
         * for 420SP. buf_size = W*H*3/2*2 + 1024 (1.5 B/px, double-buffered). */
        .format      = MPI_PIXEL_FMT_YUV420SP,
        .output_mode = MPI_VPSS_OUTPUT_OFFLINE,
        .hw_id       = MPI_VPSS_HW_AUTO,
        .buf_addr    = NULL,
        .buf_size    = (u32)KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT * 3u / 2u * 2u + 1024u,
#else
        .format      = MPI_PIXEL_FMT_YUV422_PACKED,
        .yuv_order   = MPI_YUV_ORDER_YUYV,
        .output_mode = MPI_VPSS_OUTPUT_OFFLINE,
        .hw_id       = MPI_VPSS_HW_AUTO,
        .buf_addr    = NULL,
        .buf_size    = (u32)KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT * 2u * 2u + 1024u,
#endif
    };

    ret = mpi_vpss_create_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn, &vpss_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_create_chn failed (ret=%d)\n", __func__, ret);
        return -1;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_enable_chn failed (ret=%d)\n", __func__, ret);
        mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
        return -1;
    }

    /* --- find a free VENC channel --- */
    ret = mpi_venc_find_free_chn(&g_xsmt_ctx.venc_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: no free VENC channel (ret=%d)\n", __func__, ret);
        goto err_vpss;
    }

    /* --- configure VENC (JPEG encoder, OFFLINE / MANU mode) ---
     * OFFLINE uses JPEG_CODEC_ENC_MANU: semaphore acquired+released per
     * frame, allowing VDEC decoder to interleave for full-duplex on the
     * single JPEG hardware block.  See mpi_video_net.c UVC path for the
     * canonical time-division multiplex pattern.
     */
    mpi_venc_chn_attr_t venc_attr = {
        .hw_dev          = 0,
#if KWT_USE_H264
        .type            = MPI_VENC_TYPE_SOFT_264,
        .input_mode      = MPI_VENC_INPUT_OFFLINE,
        .in_width        = KWT_CAMERA_WIDTH,
        .in_height       = KWT_CAMERA_HEIGHT,
        .in_format       = MPI_PIXEL_FMT_YUV420SP,
        /* in_yuv_order omitted (422-only field). buf_addr/buf_size omitted
         * (JPEG-photo fields; soft264 validate_attr never reads them). */
        .fps             = KWT_CAMERA_FPS,
        .abr_kbps        = KWT_CAMERA_ABR_KBPS,
        .stream_buf_size = KWT_CAMERA_STREAM_BUF_SZ,
        .gop             = KWT_CAMERA_H264_GOP,
        .qp_min          = KWT_CAMERA_H264_QP_MIN,
        .qp_max          = KWT_CAMERA_H264_QP_MAX,
        .thread_num      = KWT_CAMERA_H264_THREAD_NUM,
        .thread_prio     = KWT_CAMERA_H264_THREAD_PRIO,
#else
        .type            = MPI_VENC_TYPE_JPEG,
        .input_mode      = MPI_VENC_INPUT_OFFLINE,
        .quality         = KWT_CAMERA_JPEG_QUALITY,
        .in_width        = KWT_CAMERA_WIDTH,
        .in_height       = KWT_CAMERA_HEIGHT,
        .in_format       = MPI_PIXEL_FMT_YUV422_PACKED,
        .in_yuv_order    = MPI_YUV_ORDER_YUYV,
        .buf_addr        = NULL,
        .buf_size        = KWT_CAMERA_STREAM_BUF_SZ,
        .fps             = KWT_CAMERA_FPS,
        .abr_kbps        = KWT_CAMERA_ABR_KBPS,
        .stream_buf_size = KWT_CAMERA_STREAM_BUF_SZ,
#endif
    };

    ret = mpi_venc_create_chn(g_xsmt_ctx.venc_chn, &venc_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_venc_create_chn failed (ret=%d)\n", __func__, ret);
        goto err_vpss;
    }

    ret = mpi_venc_start_chn(g_xsmt_ctx.venc_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_venc_start_chn failed (ret=%d)\n", __func__, ret);
        mpi_venc_destroy_chn(g_xsmt_ctx.venc_chn);
        goto err_vpss;
    }

    /* cache vpss_grp for worker thread use */
    g_xsmt_ctx.vpss_grp = ctx->vpss_grp;

    log_info("%s: VPSS grp=%d chn=%d (OFFLINE) VENC chn=%d (OFFLINE/MANU) ok\n",
             __func__, ctx->vpss_grp, g_xsmt_ctx.vpss_chn, g_xsmt_ctx.venc_chn);

    os_sem_create(&g_frames_held, 0);

    return 0;

err_vpss:
    mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    return -1;
}

/* ============================================================================
 * Internal: VPSS→VENC teardown
 * ============================================================================ */

static void kwt_xsmt_vpss_venc_close(app_camera_handle_t ctx)
{
    /* 排空 VENC 后再 stop: mpi_venc_stop_chn 在 offline_busy/frame_queue 非空时
     * 返回 MPI_ERR_BUSY 并跳过 jpeg codec close(泄漏 ENC_MANU 状态, 重连后解码器
     * 断言 mode err 3)。先等离线 worker 处理完残留帧、取回所有已编码 stream, 再
     * stop; 若仍 busy 则短暂等待重试, 避免静默泄漏。 */
    mpi_stream_t stream;
    int i = 0;
    int stop_ret = MPI_ERR_BUSY;
    int destroy_ret = MPI_ERR_BUSY;

    for (i = 0; i < 100; i++) {
        while (mpi_venc_get_stream(g_xsmt_ctx.venc_chn, &stream, 0) == MPI_SUCCESS) {
            mpi_venc_release_stream(g_xsmt_ctx.venc_chn, &stream);
        }
        stop_ret = mpi_venc_stop_chn(g_xsmt_ctx.venc_chn);
        if (stop_ret == MPI_SUCCESS) {
            break;
        }
        os_time_dly(1);
    }
    if (i >= 100) {
        log_error("%s: venc stop still busy after retries (stop_ret=%d)\n", __func__, stop_ret);
    } else {
        log_info("%s: venc stop ok (retries=%d)\n", __func__, i);
    }

    destroy_ret = mpi_venc_destroy_chn(g_xsmt_ctx.venc_chn);
    if (destroy_ret != MPI_SUCCESS) {
        /* destroy 失败 => VENC 通道泄漏(started 未清), 重连后 mpi_venc_find_free_chn
         * 拿不到该 chn, 且离线 worker / frame_queue / stream_queue 均未释放 */
        log_error("%s: venc destroy failed ret=%d (channel LEAKED)\n", __func__, destroy_ret);
    } else {
        log_info("%s: venc destroy ok\n", __func__);
    }

    /* Stop the VI->VPSS source BEFORE disabling the VPSS channel. The camera
     * keeps feeding the group while we tear it down, so mpi_vpss_destroy_chn
     * intermittently returns -3 (held_count != 0). That leaks the VPSS group +
     * VI device, and the NEXT reconnect fails ("find free VI device" -> the fb6
     * producer never opens -> fb_get_fbuffer_idle: no available buffer).
     * Unbinding unlinks the IMC src, so the channel drains deterministically. */
    {
        mpi_chn_t vi_src   = {.mod_id = MPI_MOD_ID_VI,   .dev_id = ctx->vi_dev,   .chn_id = 0};
        mpi_chn_t vpss_dst = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = 0};
        (void)mpi_sys_unbind(&vi_src, &vpss_dst);
    }

    /* disable + destroy VPSS channel (source already stopped -> drains cleanly) */
    int vpss_ret = MPI_ERR_BUSY;
    vpss_ret = mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    log_info("%s: vpss disable ret=%d\n", __func__, vpss_ret);
    vpss_ret = mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    log_info("%s: vpss destroy ret=%d\n", __func__, vpss_ret);

    /* stop + destroy the VPSS group, then disable the VI device. Previously
     * these ran inside app_camera_src_close() AFTER this function, but that
     * path early-returns (mpi_sys_unbind already done above -> -4) and skips
     * mpi_vi_disable_dev, leaking the VI device. Doing it here keeps the order
     * correct and marks the source closed so app_camera_src_close no-ops. */
    {
        mpi_ret_t grp_ret = mpi_vpss_stop_grp(ctx->vpss_grp);
        log_info("%s: vpss stop grp ret=%d\n", __func__, grp_ret);
        grp_ret = mpi_vpss_destroy_grp(ctx->vpss_grp);
        log_info("%s: vpss destroy grp ret=%d\n", __func__, grp_ret);
        mpi_ret_t vi_ret = mpi_vi_disable_dev(ctx->vi_dev);
        log_info("%s: vi disable dev ret=%d\n", __func__, vi_ret);
        ctx->vi_vpss_src_enabled = 0;
    }

    os_sem_del(&g_frames_held, 0);

    log_info("%s: done\n", __func__);
}

/* ============================================================================
 * Internal: local camera preview PIP (fb6, overlaid in front of the spp fb5)
 *
 * Software producer: the sender cap thread already CPU-reads the VPSS OFFLINE
 * YUYV 320x240 frame for VENC; it also downscales that same frame to 80x60 NV12
 * and pushes it to fb6 via fb_draw_open/getmap/putmap.  No VPSS/VO channel is
 * created, so the sender still consumes only 1 IMC-dst (VENC) and the receiver
 * keeps its own VPSS — the 2-unit BR33 pool is not exceeded.  This runs in
 * kid_wt_video_xsmt_start().  The fb6 LVGL widget is created by
 * ui_action_video_intercom.c on the intercom screen LOAD; the two files
 * coordinate only via the shared fb name (dev_open is name-keyed/idempotent),
 * so this producer can open fb6 independently of the widget's create order.
 * ============================================================================ */

static int kwt_xsmt_local_display_start(app_camera_handle_t ctx)
{
    struct fb_draw_info info;

    (void)ctx;

    /* fb6 LVGL 控件由 ui_action_video_intercom.c 在 GUI_SCREEN_VIDEO_INTERCOM
     * LOAD 时创建。此处只打开 fb6 软件生产者; 其 dev_open 按名字复用同一设备
     * (幂等), 故与控件创建的先后顺序无关。 */

    memset(&info, 0, sizeof(info));
    info.name    = KWT_PIP_FB_NAME;
    info.width   = KWT_PIP_WIDTH;
    info.height  = KWT_PIP_HEIGHT;
    info.format  = FB_COLOR_FORMAT_YUV420P;   /* NV12 semi-planar */
    info.fb_num  = 2;                         /* double buffering */
    info.combine = 1;                         /* composite via fb_out */

    g_xsmt_ctx.pip_fb_draw = fb_draw_open(&info);
    if (!g_xsmt_ctx.pip_fb_draw) {
        log_error("%s: fb_draw_open(%s) failed\n", __func__, KWT_PIP_FB_NAME);
        return -1;
    }
    g_xsmt_ctx.pip_fb_opened = 1;

    log_info("%s: local PIP %s %dx%d@(%d,%d) ok (software)\n", __func__, KWT_PIP_FB_NAME,
             KWT_PIP_WIDTH, KWT_PIP_HEIGHT, KWT_PIP_LEFT, KWT_PIP_TOP);
    return 0;
}

static void kwt_xsmt_local_display_stop(app_camera_handle_t ctx)
{
    (void)ctx;

    /* close the fb6 software producer, then destroy the LVGL widget (by index) */
    if (g_xsmt_ctx.pip_fb_draw) {
        fb_draw_close(g_xsmt_ctx.pip_fb_draw);
        g_xsmt_ctx.pip_fb_draw = NULL;
    }
    g_xsmt_ctx.pip_fb_opened = 0;

    {
        lv_video_disp_handle_t *handle = lv_port_video_disp_get_instance_by_fb_index(6);
        if (handle) {
            lvgl_video_exit(handle);
        }
    }

    log_info("%s: done\n", __func__);
}

/* ============================================================================
 * Internal: YUYV 320x240 -> NV12 80x60 downscale (nearest-neighbor 4x decimation)
 *
 * Source is the packed YUYV frame the cap thread reads from the VPSS OFFLINE
 * channel (row pitch = src_stride bytes; 320/80 = 4, 240/60 = 4).  Output is
 * NV12 semi-planar: a Y plane (dst_w*dst_h) then the interleaved UV plane
 * (dst_w/2 * dst_h/2).  Each output pixel samples the top-left Y of a 4x4
 * source block; sx is always even so the Y0/U/V bytes line up.  U/V are already
 * 2x2-subsampled in YUYV, so sampling the block origin is a correct chroma
 * decimation.
 * ============================================================================ */
static void kwt_yuyv_to_nv12_scale(const u8 *yuyv, u32 src_w, u32 src_h,
                                   u32 src_stride,
                                   u8 *dst_y, u8 *dst_uv,
                                   u32 dst_w, u32 dst_h)
{
    u32 sx = src_w / dst_w;   /* 4 */
    u32 sy = src_h / dst_h;   /* 4 */
    u32 uv_w = dst_w / 2;
    u32 dy, dx;

    for (dy = 0; dy < dst_h; dy++) {
        const u8 *row = yuyv + dy * sy * src_stride;
        u8 *y_out = dst_y + dy * dst_w;
        for (dx = 0; dx < dst_w; dx++) {
            const u8 *p = row + dx * sx * 2;   /* YUYV: 2 bytes/px */
            y_out[dx] = p[0];                  /* Y0 of the sampled pixel */
            if ((dx & 1) == 0 && (dy & 1) == 0) {
                u32 uv_off = (dy >> 1) * uv_w * 2 + (dx >> 1) * 2;
                dst_uv[uv_off + 0] = p[1];     /* U */
                dst_uv[uv_off + 1] = p[3];     /* V */
            }
        }
    }
}

/* ============================================================================
 * Internal: NV12 320x240 -> NV12 80x60 downscale (nearest-neighbor 4x decimation)
 *
 * H264 counterpart of kwt_yuyv_to_nv12_scale(): in H264 mode the VPSS OFFLINE
 * channel emits semi-planar NV12 (YUV420SP) — a Y plane (stride x height)
 * followed by an interleaved UV plane (stride x height/2) at
 * c_addr = y_addr + y_len (mpi_vpss_fill_active_frame_info).  Output is NV12
 * (Y plane + interleaved UV).  4x decimation: each output Y samples the top-left
 * of a 4x4 source block; each output UV pair samples the decimated source pair
 * (U/V are already 2x2-subsampled, so sampling the block origin is correct).
 * ============================================================================ */
static void kwt_nv12_to_nv12_scale(const u8 *src_y, const u8 *src_uv,
                                   u32 src_w, u32 src_h, u32 src_stride,
                                   u8 *dst_y, u8 *dst_uv,
                                   u32 dst_w, u32 dst_h)
{
    u32 sx = src_w / dst_w;   /* 4 */
    u32 sy = src_h / dst_h;   /* 4 */
    u32 uv_w = dst_w / 2;
    u32 uv_h = dst_h / 2;
    u32 dy, dx;

    /* Y plane */
    for (dy = 0; dy < dst_h; dy++) {
        const u8 *row = src_y + dy * sy * src_stride;
        u8 *y_out = dst_y + dy * dst_w;
        for (dx = 0; dx < dst_w; dx++) {
            y_out[dx] = row[dx * sx];
        }
    }

    /* interleaved UV plane (source UV row pitch == src_stride) */
    for (dy = 0; dy < uv_h; dy++) {
        const u8 *row = src_uv + dy * sy * src_stride;
        u8 *uv_out = dst_uv + dy * uv_w * 2;
        for (dx = 0; dx < uv_w; dx++) {
            const u8 *p = row + (dx * sx) * 2;
            uv_out[dx * 2]     = p[0];   /* U */
            uv_out[dx * 2 + 1] = p[1];   /* V */
        }
    }
}

/* ============================================================================
 * Worker thread: polls VENC for encoded JPEG frames → vstream_send_frame()
 * ============================================================================ */

static void kwt_xsmt_cap_thread(void *priv)
{
    mpi_video_frame_info_t vpss_frame;
    mpi_stream_t stream;
    int ret;
    int frame_held = 0;   /* 已 pop 出 VPSS 帧但尚未 release, 退出线程前必须归还 */

    log_info("%s: started\n", __func__);

    while (!g_xsmt_ctx.exit_flag) {
        /* 1. Get YUV frame from VPSS (OFFLINE output, CPU-accessible) */
        ret = mpi_vpss_get_chn_frame(g_xsmt_ctx.vpss_grp,
                                     g_xsmt_ctx.vpss_chn,
                                     &vpss_frame, 100);
        if (ret != MPI_SUCCESS) {
            os_time_dly(1);
            continue;
        }

        /* 源端背压(Fix A): TX 剩余空间不足以再吞一帧时, 直接归还 VPSS 帧并跳过,
         * 不编码、不发送 —— 避免连接初期突发把 TX 缓冲打到有界等待区(高延迟)。
         * 同时按 ~10fps(100ms) 限速, 匹配链路可持续速率, 让需求不超供给。 */
        extern u32 get_bredr_tx_remain_size(void);
        extern u32 timer_get_ms(void);
        static u32 last_prod_ms = 0;
        u32 now_ms = timer_get_ms();
        if (get_bredr_tx_remain_size() < (4 * 1024)
            || (now_ms - last_prod_ms) < 100) {
            mpi_vpss_release_chn_frame(g_xsmt_ctx.vpss_grp,
                                       g_xsmt_ctx.vpss_chn, &vpss_frame);
            os_time_dly(1);
            continue;
        }
        last_prod_ms = now_ms;

        /* 1.5. local camera PIP: downscale this frame to 80x60 NV12 and push it
         *      to fb6.  Must run BEFORE mpi_vpss_release_chn_frame() — the VPSS
         *      buffer is recycled to the IMC on release. */
#if KWT_USE_H264
        /* H264: VPSS frame is NV12 (YUV420SP). Downscale NV12 -> NV12. */
        if (g_xsmt_ctx.pip_fb_draw) {
            struct fb_map_user map;
            memset(&map, 0, sizeof(map));
            if (fb_draw_getmap(g_xsmt_ctx.pip_fb_draw, &map) == 0) {
                u8 *y_buf  = map.baddr;
                u8 *uv_buf = map.uv_baddr ? map.uv_baddr
                             : (map.baddr + KWT_PIP_WIDTH * KWT_PIP_HEIGHT);
                if (y_buf && uv_buf) {
                    kwt_nv12_to_nv12_scale(vpss_frame.y_addr, vpss_frame.c_addr,
                                           KWT_CAMERA_WIDTH, KWT_CAMERA_HEIGHT,
                                           vpss_frame.stride,
                                           y_buf, uv_buf,
                                           KWT_PIP_WIDTH, KWT_PIP_HEIGHT);
                    /* cacheable PSRAM: flush CPU writes so the display DMA
                     * sees them (separate vs contiguous Y/UV buffers) */
                    if (map.uv_baddr) {
                        DcuFlushinvRegion((void *)map.baddr,
                                          KWT_PIP_WIDTH * KWT_PIP_HEIGHT);
                        DcuFlushinvRegion((void *)map.uv_baddr,
                                          (KWT_PIP_WIDTH * KWT_PIP_HEIGHT) / 2);
                    } else {
                        DcuFlushinvRegion((void *)map.baddr,
                                          (KWT_PIP_WIDTH * KWT_PIP_HEIGHT * 3) / 2);
                    }
                }
                fb_draw_putmap(g_xsmt_ctx.pip_fb_draw, &map);
            }
        }
#else
        if (g_xsmt_ctx.pip_fb_draw) {
            struct fb_map_user map;
            memset(&map, 0, sizeof(map));
            if (fb_draw_getmap(g_xsmt_ctx.pip_fb_draw, &map) == 0) {
                u8 *y_buf  = map.baddr;
                u8 *uv_buf = map.uv_baddr ? map.uv_baddr
                             : (map.baddr + KWT_PIP_WIDTH * KWT_PIP_HEIGHT);
                if (y_buf && uv_buf) {
                    kwt_yuyv_to_nv12_scale(vpss_frame.y_addr,
                                           KWT_CAMERA_WIDTH, KWT_CAMERA_HEIGHT,
                                           vpss_frame.stride,
                                           y_buf, uv_buf,
                                           KWT_PIP_WIDTH, KWT_PIP_HEIGHT);
                    /* cacheable PSRAM: flush CPU writes so the display DMA
                     * sees them (separate vs contiguous Y/UV buffers) */
                    if (map.uv_baddr) {
                        DcuFlushinvRegion((void *)map.baddr,
                                          KWT_PIP_WIDTH * KWT_PIP_HEIGHT);
                        DcuFlushinvRegion((void *)map.uv_baddr,
                                          (KWT_PIP_WIDTH * KWT_PIP_HEIGHT) / 2);
                    } else {
                        DcuFlushinvRegion((void *)map.baddr,
                                          (KWT_PIP_WIDTH * KWT_PIP_HEIGHT * 3) / 2);
                    }
                }
                fb_draw_putmap(g_xsmt_ctx.pip_fb_draw, &map);
            }
        }
#endif

        /* 2. Send YUV to VENC for manual JPEG encoding.
         *    mpi_venc_send_frame() 只是把 VPSS 帧浅拷贝进离线 worker 的 frame_queue,
         *    真正的编码(读取同一 VPSS 缓冲)由 mpi_venc_offline_worker 异步执行。
         *    因此 VPSS 帧必须等 mpi_venc_get_stream() 返回(编码完成)之后才能归还,
         *    否则 worker 会读到已回收的缓冲(use-after-free → 残帧/花屏)。 */
        extern u32 timer_get_ms(void);
        u32 begin = timer_get_ms();
        ret = mpi_venc_send_frame(g_xsmt_ctx.venc_chn, &vpss_frame);
        log_info("mpi_venc_send_frame: %ums", timer_get_ms() - begin);

        if (ret != MPI_SUCCESS) {
            log_error("mpi_venc_send_frame failed");
            /* 入队失败(队列满): 该帧不会被编码, 立即归还 VPSS 帧避免泄漏 */
            mpi_vpss_release_chn_frame(g_xsmt_ctx.vpss_grp,
                                       g_xsmt_ctx.vpss_chn, &vpss_frame);
            frame_held = 0;
            os_time_dly(1);
            continue;
        }
        /* 帧已入队给 VENC 离线 worker(浅拷贝, worker 会读同一 VPSS 缓冲),
         * 在 get_stream 确认编码完成前该帧保持 held, 需在本线程退出前归还。 */
        frame_held = 1;

        /* 3. 等待异步编码完成。共享 codec 可能被全双工解码器占用, 编码会阻塞数百
         *    ms; 只有 get_stream 成功(证明编码已读完 VPSS 缓冲)后才归还 VPSS 帧,
         *    否则 worker 会读到已回收的缓冲(use-after-free)。超时则继续等待;
         *    exit_flag 置位时退出循环, 由循环结束后的 frame_held 检查统一归还。 */
        while (!g_xsmt_ctx.exit_flag) {
            begin = timer_get_ms();
            ret = mpi_venc_get_stream(g_xsmt_ctx.venc_chn, &stream, 100);
            log_info("mpi_venc_get_stream: %ums", timer_get_ms() - begin);
            if (ret == MPI_SUCCESS) {
                /* 编码已完成(get_stream 成功即证明离线 worker 已读完 VPSS 缓冲), 立即
                 * 归还 VPSS 帧, 不再被下面慢速的 vstream_send_frame(可能阻塞在 SPP 发送
                 * 互斥锁上)占用, 避免 2 缓冲的 VPSS 离线队列被占满而丢帧。 */
                begin = timer_get_ms();
                mpi_vpss_release_chn_frame(g_xsmt_ctx.vpss_grp,
                                           g_xsmt_ctx.vpss_chn, &vpss_frame);
                frame_held = 0;
                log_info("mpi_vpss_release_chn_frame: %ums", timer_get_ms() - begin);

                begin = timer_get_ms();
#if KWT_USE_H264
                vstream_send_frame((u8 *)stream.addr, stream.len,
                                   VSTREAM_H264_TYPE_VIDEO);
#else
                vstream_send_frame((u8 *)stream.addr, stream.len,
                                   VSTREAM_JPEG_TYPE_VIDEO);
#endif
                log_info("vstream_send_frame: %ums", timer_get_ms() - begin);

                mpi_venc_release_stream(g_xsmt_ctx.venc_chn, &stream);

                break;
            }
            os_time_dly(1);
        }
    }

    /* exit_flag 退出时, 若仍持有一帧已 pop 但未 release 的 VPSS 帧, 这里归还。
     * 否则 held_count 永不为 0, mpi_vpss_destroy_chn 返回 BUSY, 级联导致
     * VI 设备/VPSS group 泄漏, 重连后相机无法重新打开(远端画面卡死)。 */
    if (frame_held) {
        mpi_vpss_release_chn_frame(g_xsmt_ctx.vpss_grp,
                                   g_xsmt_ctx.vpss_chn, &vpss_frame);
        frame_held = 0;
        log_info("%s: released held VPSS frame on exit\n", __func__);
    }

    log_info("%s: stopped\n", __func__);
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int kid_wt_video_xsmt_is_running(void)
{
    return (g_xsmt_ctx.cam_handle != NULL);
}

int kid_wt_video_xsmt_get_status(void)
{
    return (g_xsmt_ctx.cam_handle != NULL) ? 1 : 0;
}

void kid_wt_video_xsmt_get_last_params(int *w, int *h, int *fps, int *abr)
{
    if (w) {
        *w   = g_xsmt_last_width;
    }
    if (h) {
        *h   = g_xsmt_last_height;
    }
    if (fps) {
        *fps = g_xsmt_last_fps;
    }
    if (abr) {
        *abr = g_xsmt_last_abr;
    }
}

int kid_wt_video_xsmt_start(void)
{
    int ret;

    log_info("%s: enter\n", __func__);

#if KWT_DISABLE_ENCODE
    log_info("%s: encode disabled by KWT_DISABLE_ENCODE, skip\n", __func__);
    return 0;
#endif

    /* cache params (mirrors mpi_video_net.c pattern) */
    g_xsmt_last_width  = KWT_CAMERA_WIDTH;
    g_xsmt_last_height = KWT_CAMERA_HEIGHT;
    g_xsmt_last_fps    = KWT_CAMERA_FPS;
    g_xsmt_last_abr    = KWT_CAMERA_ABR_KBPS;

    /* already started? */
    if (g_xsmt_ctx.cam_handle != NULL) {
        log_info("%s: already started\n", __func__);
        return 0;
    }

    /* --- 1. open camera --- */
    ret = app_camera_open(0, (char *)KWT_CAMERA_SENSOR, &g_xsmt_ctx.cam_handle);
    if (ret != APP_CAMERA_OK) {
        log_error("%s: app_camera_open failed (ret=%d)\n", __func__, ret);
        g_xsmt_ctx.cam_handle = NULL;
        return -1;
    }

    /* --- 2. open camera source (VI + VPSS group) --- */
    if (!app_camera_src_is_opened(g_xsmt_ctx.cam_handle)) {
        ret = app_camera_src_open(g_xsmt_ctx.cam_handle);
        if (ret != APP_CAMERA_OK) {
            log_error("%s: app_camera_src_open failed (ret=%d)\n", __func__, ret);
            app_camera_close(&g_xsmt_ctx.cam_handle);
            return -1;
        }
    }

    /* --- 3. open VPSS→VENC JPEG pipeline --- */
    ret = kwt_xsmt_vpss_venc_open(g_xsmt_ctx.cam_handle);
    if (ret != 0) {
        log_error("%s: kwt_xsmt_vpss_venc_open failed\n", __func__);
        app_camera_src_close(g_xsmt_ctx.cam_handle);
        app_camera_close(&g_xsmt_ctx.cam_handle);
        return -1;
    }

    /* --- 3.5. local camera preview PIP (fb6, overlaid in front of spp fb5) --- */
    ret = kwt_xsmt_local_display_start(g_xsmt_ctx.cam_handle);
    if (ret != 0) {
        log_error("%s: kwt_xsmt_local_display_start failed\n", __func__);
        kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);
        app_camera_src_close(g_xsmt_ctx.cam_handle);
        app_camera_close(&g_xsmt_ctx.cam_handle);
        return -1;
    }

    /* --- 4. allocate video lbuf in the sender --- */
    ret = vstream_sender_video_init();
    if (ret != 0) {
        log_error("%s: vstream_sender_video_init failed\n", __func__);
        kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);
        kwt_xsmt_local_display_stop(g_xsmt_ctx.cam_handle);
        app_camera_src_close(g_xsmt_ctx.cam_handle);
        app_camera_close(&g_xsmt_ctx.cam_handle);
        return -1;
    }

    /* --- 5. launch worker thread --- */
    g_xsmt_ctx.exit_flag = 0;
    ret = thread_fork("kwt_video_cap",
                      20,
                      2048,
                      0,
                      &g_xsmt_ctx.thread_pid,
                      kwt_xsmt_cap_thread,
                      NULL);
    if (ret != 0) {
        log_error("%s: thread_fork failed (ret=%d)\n", __func__, ret);
        vstream_sender_video_deinit();
        kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);
        kwt_xsmt_local_display_stop(g_xsmt_ctx.cam_handle);
        app_camera_src_close(g_xsmt_ctx.cam_handle);
        app_camera_close(&g_xsmt_ctx.cam_handle);
        return -1;
    }

    log_info("%s: done\n", __func__);
    return 0;
}

void kid_wt_video_xsmt_stop(void)
{
    log_info("%s: enter\n", __func__);

    if (g_xsmt_ctx.cam_handle == NULL) {
        return;
    }

    /* 1. signal and wait for worker thread */
    g_xsmt_ctx.exit_flag = 1;
    thread_kill(&g_xsmt_ctx.thread_pid, KILL_WAIT);

    /* 2. tear down VPSS→VENC pipeline */
    kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);

    /* 3. stop local camera preview PIP (fb6 software producer, then LVGL widget) */
    kwt_xsmt_local_display_stop(g_xsmt_ctx.cam_handle);

    /* 4. close camera source (VI + VPSS group) — the software PIP never owned
     *    the source, so this is a real close, not a no-op */
    app_camera_src_close(g_xsmt_ctx.cam_handle);

    /* 5. close camera */
    app_camera_close(&g_xsmt_ctx.cam_handle);

    /* 6. free video lbuf in the sender */
    vstream_sender_video_deinit();

    /* clear state */
    memset(&g_xsmt_ctx, 0, sizeof(g_xsmt_ctx));

    log_info("%s: done\n", __func__);
}

/* ============================================================================
 * Receiver: vstream VIDEO_FRAME → JPEG decode → LVGL display
 *
 * Polls vstream_recv_acquire_frame(VIDEO_FRAME) for JPEG frames received
 * over SPP, decodes them via app_playback_one_image_data() (VDEC→VPSS→VO→fb5),
 * and displays them through the fb5 LVGL widget created by
 * ui_action_video_intercom.c (LVGL reads fb5 via FBIOGET_FBUFFER_IDLE).
 * ============================================================================ */

typedef struct {
    app_playback_handle_t pb_handle;     /* handle from app_playback_open() */
    u8  exit_flag;                       /* signal worker thread to stop */
    int thread_pid;                      /* worker thread handle */
    u16 fps_tid;                         /* 1Hz FPS-printer sys_timer id (0 = not running) */
    u32 fps_count;                       /* received-frame count since last 1s print */
    u8  pipeline_opened;                 /* app_playback_open() / H264 pipeline open succeeded */
#if KWT_USE_H264
    mpi_vdec_dev_t  vdec_dev;            /* 0 */
    mpi_vdec_chn_t  vdec_chn;            /* from mpi_vdec_find_free_chn */
    mpi_vpss_grp_t  vpss_grp;            /* from mpi_vpss_find_free_grp */
    mpi_vpss_chn_t  vpss_chn;            /* from mpi_vpss_find_free_chn */
    mpi_vo_chn_t    vo_chn;              /* from mpi_vo_find_free_chn */
#endif
} kwt_video_recv_ctx_t;

static kwt_video_recv_ctx_t g_recv_ctx;

/* guards g_recv_ctx.fps_count across kwt_video_recv_thread (increment) and the
 * 1Hz FPS timer callback (read+reset), which run in different task contexts */
static DEFINE_SPINLOCK(g_recv_fps_lock);

#if KWT_USE_H264

/* SOFT_H264 VDEC frame pool + unresolved-tail cache sizes (mirrors
 * apps/common/video/mpi_demo/h264/mpi_h264_vpss_vo.c). */
#define KWT_RECV_VDEC_FRAME_POOL_SIZE   (512u * 1024u)
#define KWT_RECV_VDEC_STREAM_BUF_SIZE   (64u * 1024u)

/* Build the VDEC(SOFT_H264) -> VPSS -> VO bound display pipeline.
 *
 * VDEC decodes the reassembled H264 frame into NV12 (YUV420SP), the VPSS
 * channel forwards it, and VO writes it into fb5 (the LVGL intercom widget
 * consumes fb5 via FBIOGET_FBUFFER_IDLE). Decoded frames auto-forward
 * VDEC->VPSS->VO through the bind mechanism — no manual get_frame/release_frame
 * (those return MPI_ERR_BUSY while bound). Mirrors app_playback_image_pipeline_open()
 * (apps/common/video/mpi_app/playback/app_playback_image.c), with
 * MPI_VDEC_TYPE_SOFT_H264 + MPI_VDEC_OUTPUT_FRAME instead of JPEG. */
static int kwt_recv_h264_pipeline_open(void)
{
    mpi_ret_t ret;
    mpi_vdec_chn_attr_t vdec_attr = {0};
    mpi_vpss_grp_attr_t grp_attr = {0};
    mpi_vpss_chn_attr_t vpss_attr = {0};
    mpi_vo_chn_attr_t vo_attr = {0};
    mpi_chn_t vdec_src;
    mpi_chn_t vpss_dst;
    mpi_chn_t vpss_out;
    mpi_chn_t vo_dst;

    /* --- 1. VDEC channel (SOFT_H264, frame output) --- */
    ret = mpi_vdec_find_free_chn(&g_recv_ctx.vdec_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vdec_find_free_chn failed (ret=%d)\n", __func__, ret);
        return -1;
    }

    g_recv_ctx.vdec_dev = 0;
    vdec_attr.hw_dev          = g_recv_ctx.vdec_dev;
    vdec_attr.type            = MPI_VDEC_TYPE_SOFT_H264;
    vdec_attr.output_mode     = MPI_VDEC_OUTPUT_FRAME;   /* SOFT_H264 only supports frame mode */
    vdec_attr.frame_buf_size  = KWT_RECV_VDEC_FRAME_POOL_SIZE;
    vdec_attr.stream_buf_size = KWT_RECV_VDEC_STREAM_BUF_SIZE;
    ret = mpi_vdec_create_chn(g_recv_ctx.vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vdec_create_chn failed (ret=%d)\n", __func__, ret);
        return -1;
    }

    /* --- 2. VPSS group (src = VDEC NV12 320x240 output) --- */
    ret = mpi_vpss_find_free_grp(&g_recv_ctx.vpss_grp);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_find_free_grp failed (ret=%d)\n", __func__, ret);
        goto err_destroy_vdec;
    }

    grp_attr.src_width        = KWT_CAMERA_WIDTH;
    grp_attr.src_height       = KWT_CAMERA_HEIGHT;
    grp_attr.src_format       = MPI_VPSS_SRC_FMT_YUV;
    grp_attr.src_pixel_format = MPI_PIXEL_FMT_YUV420SP;  /* VDEC SOFT_H264 emits NV12 */
    grp_attr.src_yuv_order    = 0;
    ret = mpi_vpss_create_grp(g_recv_ctx.vpss_grp, &grp_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_create_grp failed (ret=%d)\n", __func__, ret);
        goto err_destroy_vdec;
    }

    ret = mpi_vpss_start_grp(g_recv_ctx.vpss_grp);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_start_grp failed (ret=%d)\n", __func__, ret);
        goto err_destroy_grp;
    }

    /* --- 3. VPSS channel (NV12 out, OFFLINE) --- */
    ret = mpi_vpss_find_free_chn(g_recv_ctx.vpss_grp, &g_recv_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_find_free_chn failed (ret=%d)\n", __func__, ret);
        goto err_stop_grp;
    }

    vpss_attr.width       = KWT_CAMERA_WIDTH;
    vpss_attr.height      = KWT_CAMERA_HEIGHT;
    vpss_attr.format      = MPI_PIXEL_FMT_YUV420SP;   /* NV12 -> fb5 */
    vpss_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ret = mpi_vpss_create_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn, &vpss_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_create_chn failed (ret=%d)\n", __func__, ret);
        goto err_stop_grp;
    }
    ret = mpi_vpss_enable_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_enable_chn failed (ret=%d)\n", __func__, ret);
        goto err_destroy_vpss_chn;
    }

    /* --- 4. VO channel (fb5 hardware producer, NV12) --- */
    ret = mpi_vo_find_free_chn(&g_recv_ctx.vo_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_find_free_chn failed (ret=%d)\n", __func__, ret);
        goto err_disable_vpss_chn;
    }

    memcpy(vo_attr.fb_name, KWT_RECV_FB_NAME, sizeof(vo_attr.fb_name));
    vo_attr.fb_num        = 2;
    vo_attr.fb_width      = KWT_CAMERA_WIDTH;
    vo_attr.fb_height     = KWT_CAMERA_HEIGHT;
    vo_attr.window_width  = KWT_CAMERA_WIDTH;
    vo_attr.window_height = KWT_CAMERA_HEIGHT;
    vo_attr.fb_format     = FB_COLOR_FORMAT_YUV420P;   /* NV12, matches the LVGL widget */
    vo_attr.combine       = 1;
    ret = mpi_vo_create_chn(g_recv_ctx.vo_chn, &vo_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_create_chn failed (ret=%d)\n", __func__, ret);
        goto err_disable_vpss_chn;
    }
    ret = mpi_vo_start_chn(g_recv_ctx.vo_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_start_chn failed (ret=%d)\n", __func__, ret);
        goto err_destroy_vo;
    }

    /* --- 5. bind VPSS chn -> VO, start VDEC, bind VDEC -> VPSS grp --- */
    vdec_src.mod_id = MPI_MOD_ID_VDEC;
    vdec_src.dev_id = g_recv_ctx.vdec_dev;
    vdec_src.chn_id = g_recv_ctx.vdec_chn;
    vpss_dst.mod_id = MPI_MOD_ID_VPSS;
    vpss_dst.dev_id = g_recv_ctx.vpss_grp;
    vpss_dst.chn_id = 0;
    vpss_out.mod_id = MPI_MOD_ID_VPSS;
    vpss_out.dev_id = g_recv_ctx.vpss_grp;
    vpss_out.chn_id = g_recv_ctx.vpss_chn;
    vo_dst.mod_id   = MPI_MOD_ID_VO;
    vo_dst.dev_id   = 0;
    vo_dst.chn_id   = g_recv_ctx.vo_chn;

    ret = mpi_sys_bind(&vpss_out, &vo_dst);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_sys_bind vpss->vo failed (ret=%d)\n", __func__, ret);
        goto err_stop_vo;
    }

    ret = mpi_vdec_start_chn(g_recv_ctx.vdec_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vdec_start_chn failed (ret=%d)\n", __func__, ret);
        goto err_unbind_vpss_vo;
    }

    ret = mpi_sys_bind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_sys_bind vdec->vpss failed (ret=%d)\n", __func__, ret);
        goto err_stop_vdec;
    }

    g_recv_ctx.pipeline_opened = 1;
    log_info("%s: VDEC chn=%d -> VPSS grp=%d chn=%d -> VO chn=%d (fb5) ok\n",
             __func__, g_recv_ctx.vdec_chn, g_recv_ctx.vpss_grp,
             g_recv_ctx.vpss_chn, g_recv_ctx.vo_chn);
    return 0;

err_stop_vdec:
    mpi_vdec_stop_chn(g_recv_ctx.vdec_chn);
err_unbind_vpss_vo:
    mpi_sys_unbind(&vpss_out, &vo_dst);
err_stop_vo:
    mpi_vo_stop_chn(g_recv_ctx.vo_chn);
err_destroy_vo:
    mpi_vo_destroy_chn(g_recv_ctx.vo_chn);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn);
err_stop_grp:
    mpi_vpss_stop_grp(g_recv_ctx.vpss_grp);
err_destroy_grp:
    mpi_vpss_destroy_grp(g_recv_ctx.vpss_grp);
err_destroy_vdec:
    mpi_vdec_destroy_chn(g_recv_ctx.vdec_chn);
    return -1;
}

/* Tear down the VDEC->VPSS->VO pipeline downstream->upstream (mirrors
 * app_playback_image_pipeline_close). Gated by g_recv_ctx.pipeline_opened. */
static void kwt_recv_h264_pipeline_close(void)
{
    mpi_chn_t vdec_src;
    mpi_chn_t vpss_dst;
    mpi_chn_t vpss_out;
    mpi_chn_t vo_dst;

    if (!g_recv_ctx.pipeline_opened) {
        return;
    }

    vdec_src.mod_id = MPI_MOD_ID_VDEC;
    vdec_src.dev_id = g_recv_ctx.vdec_dev;
    vdec_src.chn_id = g_recv_ctx.vdec_chn;
    vpss_dst.mod_id = MPI_MOD_ID_VPSS;
    vpss_dst.dev_id = g_recv_ctx.vpss_grp;
    vpss_dst.chn_id = 0;
    vpss_out.mod_id = MPI_MOD_ID_VPSS;
    vpss_out.dev_id = g_recv_ctx.vpss_grp;
    vpss_out.chn_id = g_recv_ctx.vpss_chn;
    vo_dst.mod_id   = MPI_MOD_ID_VO;
    vo_dst.dev_id   = 0;
    vo_dst.chn_id   = g_recv_ctx.vo_chn;

    /* 1. VPSS chn -> VO unbind, then VO */
    mpi_sys_unbind(&vpss_out, &vo_dst);
    mpi_vo_stop_chn(g_recv_ctx.vo_chn);
    mpi_vo_destroy_chn(g_recv_ctx.vo_chn);

    /* 2. VPSS chn (release the IMC dst before the VDEC->VPSS unbind) */
    mpi_vpss_disable_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn);
    mpi_vpss_destroy_chn(g_recv_ctx.vpss_grp, g_recv_ctx.vpss_chn);

    /* 3. VDEC -> VPSS grp unbind, then VPSS grp */
    mpi_sys_unbind(&vdec_src, &vpss_dst);
    mpi_vpss_stop_grp(g_recv_ctx.vpss_grp);
    mpi_vpss_destroy_grp(g_recv_ctx.vpss_grp);

    /* 4. VDEC (stop frees the soft-H264 decoder; destroy frees the frame pool) */
    mpi_vdec_stop_chn(g_recv_ctx.vdec_chn);
    mpi_vdec_destroy_chn(g_recv_ctx.vdec_chn);

    g_recv_ctx.pipeline_opened = 0;
    log_info("%s: done\n", __func__);
}

#endif /* KWT_USE_H264 */

static void kwt_video_recv_thread(void *priv)
{
    u8 *buf = NULL;
    int len = 0;
    int ret;
    u32 recv_cnt = 0;        /* 重连后累计收到的视频帧数, 用于定位卡死位置 */
    u32 no_frame_ticks = 0;  /* 连续取不到帧的 tick 计数(1 tick = 1ms) */

    log_info("thread start, pid=%s\n", os_current_task());

    while (!g_recv_ctx.exit_flag) {
        /* wait for first SPP data packet to create the stream_dev(ip=0);
         * stream_dev_create_video() returns 0 once it exists and the
         * VIDEO_FRAME lbuf queue has been initialized */
        if (stream_dev_create_video() != 0) {
            os_time_dly(1);
            continue;
        }
        log_info("stream_dev ready, polling frames\n");
        break;
    }

    while (!g_recv_ctx.exit_flag) {
        ret = vstream_recv_acquire_frame(0, &buf, &len, VIDEO_FRAME);
        if (ret == 0 && buf && len > 0) {
            recv_cnt++;
            no_frame_ticks = 0;
            log_info("recv frame cnt=%u len=%d\n", recv_cnt, len);
            spin_lock(&g_recv_fps_lock);
            g_recv_ctx.fps_count++;
            spin_unlock(&g_recv_fps_lock);
#if KWT_USE_H264
            {
                mpi_stream_t stream = { .addr = buf, .len = (u32)len };
                ret = mpi_vdec_send_stream(g_recv_ctx.vdec_chn, &stream);
                log_info("vdec send_stream ret=%d\n", ret);
            }
#else
            ret = app_playback_one_image_data(g_recv_ctx.pb_handle,
                                              buf, (u32)len);
            log_info("decode ret=%d\n", ret);
#endif
            video_stream_recv_release_frame(buf);
#if !KWT_USE_H264
            if (ret == APP_PLAYBACK_ERR_BUSY) {
                os_time_dly(1);
            }
#endif
        } else {
            os_time_dly(1);
            if (++no_frame_ticks >= 1000) {
                /* 连续 1s 取不到帧: 说明对端没发/传输断/解析卡住, 打印出来定位 */
                log_info("recv no frame for 1s, total recv_cnt=%u\n", recv_cnt);
                no_frame_ticks = 0;
            }
        }
    }

    log_info("thread exit\n");
}

/* ============================================================================
 * FPS printer: 1Hz sys_timer callback.  Runs in the app_core context that added
 * the timer (sys_timer is synchronous to the add-thread — see timer.h note),
 * NOT in the recv thread, so it takes g_recv_fps_lock around the read+reset.
 * ============================================================================ */
static void kwt_video_fps_timer_cb(void *priv)
{
    u32 fps;

    (void)priv;

    spin_lock(&g_recv_fps_lock);
    fps = g_recv_ctx.fps_count;
    g_recv_ctx.fps_count = 0;
    spin_unlock(&g_recv_fps_lock);

    log_info("recv fps = %u\n", fps);
}

int kid_wt_video_recv_is_running(void)
{
    return (g_recv_ctx.thread_pid != 0 && !g_recv_ctx.exit_flag);
}

int kid_wt_video_recv_get_status(void)
{
    return (g_recv_ctx.thread_pid != 0 && !g_recv_ctx.exit_flag) ? 1 : 0;
}

int kid_wt_video_recv_start(void)
{
#if KWT_DISABLE_DECODE
    log_info("%s: decode disabled by KWT_DISABLE_DECODE, skip\n", __func__);
    return 0;
#endif

    app_playback_ret_t ret;

    /* idempotent */
    if (kid_wt_video_recv_is_running()) {
        log_info("already started\n");
        return 0;
    }

#if KWT_USE_H264
    /* H264: open the MPI VDEC(SOFT_H264)->VPSS->VO bound pipeline, which
     * produces fb5 via mpi_vo_create_chn (hardware producer). Do NOT also
     * fb_draw_open(fb5) here — the two would double-produce into fb5. */
    if (kwt_recv_h264_pipeline_open() != 0) {
        return -1;
    }
#else
    /* fb5 LVGL 控件由 ui_action_video_intercom.c 在 GUI_SCREEN_VIDEO_INTERCOM
     * LOAD 时创建。此处只打开 playback 流水线; dev_open 按名字复用 fb5 设备。 */
    app_playback_display_attr_t attr = {
        .fb_name = KWT_RECV_FB_NAME,
        .fb_num  = 2,
        .width   = KWT_CAMERA_WIDTH,
        .height  = KWT_CAMERA_HEIGHT,
    };
    ret = app_playback_open(&g_recv_ctx.pb_handle, &attr);
    log_info("app_playback_open ret=%d\n", ret);
    if (ret != APP_PLAYBACK_OK) {
        return -1;
    }
    g_recv_ctx.pipeline_opened = 1;
#endif

    /* 3. worker thread */
    g_recv_ctx.exit_flag = 0;
    ret = thread_fork("kwt_video_recv",
                      20,
                      2048,
                      0,
                      &g_recv_ctx.thread_pid,
                      kwt_video_recv_thread,
                      NULL);
    log_info("thread_fork ret=%d\n", ret);
    if (ret != 0) {
#if KWT_USE_H264
        kwt_recv_h264_pipeline_close();
#else
        app_playback_close(&g_recv_ctx.pb_handle);
        g_recv_ctx.pipeline_opened = 0;
#endif
        return -1;
    }

    /* 4. 1Hz FPS printer timer (app_core context): fires every 1s, reads+resets
     *    fps_count and prints the receiving frame rate. */
    g_recv_ctx.fps_count = 0;
    g_recv_ctx.fps_tid = sys_timer_add(NULL, kwt_video_fps_timer_cb, 1000);
    if (!g_recv_ctx.fps_tid) {
        log_error("%s: sys_timer_add(fps) failed\n", __func__);
    }

    return 0;
}

void kid_wt_video_recv_stop(void)
{
    if (!kid_wt_video_recv_is_running()) {
        return;
    }

    /* 1. stop worker thread */
    g_recv_ctx.exit_flag = 1;
    thread_kill(&g_recv_ctx.thread_pid, KILL_WAIT);
    log_info("thread killed\n");

    /* 1.5. stop the 1Hz FPS printer timer (must precede the memset below, which
     *      wipes fps_tid — deleting with a zeroed id would leak the timer and
     *      trip the "timer func no del" assert on task exit) */
    if (g_recv_ctx.fps_tid) {
        sys_timer_del(g_recv_ctx.fps_tid);
        g_recv_ctx.fps_tid = 0;
    }

#if KWT_USE_H264
    /* 2. teardown the VDEC->VPSS->VO pipeline (fb5 hardware producer) */
    kwt_recv_h264_pipeline_close();
#else
    /* 2. close playback pipeline (VDEC/VPSS/VO teardown) */
    if (g_recv_ctx.pipeline_opened && g_recv_ctx.pb_handle) {
        app_playback_close(&g_recv_ctx.pb_handle);
        g_recv_ctx.pipeline_opened = 0;
        log_info("playback closed\n");
    }
#endif

    /* 2.5 清空接收端视频/音频 lbuf 队列 + 复位解析状态, 避免上一会话残帧
     *     在重连后被 VDEC 误解码(远端画面卡住)。 */
    stream_dev_media_clear();

    /* 3. close LVGL display widget (fb5) — looked up by fb_index */
    {
        lv_video_disp_handle_t *handle = lv_port_video_disp_get_instance_by_fb_index(5);
        if (handle) {
            lvgl_video_exit(handle);
            log_info("lvgl_video_exit posted\n");
        }
    }

    memset(&g_recv_ctx, 0, sizeof(g_recv_ctx));
}

#endif /* TCFG_VSTREAM_TRANSFER */
