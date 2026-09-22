
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
#include "kid_wt_app.h"    /* kwt_get_spp_connect() — gate TX on the SPP link */
#include "lvgl_video_init.h"
#include "lv_port_video_disp.h"   /* lv_port_video_disp_get_instance_by_fb_index() */
#include "asm/dcache.h"   /* DcuFlushinvRegion — flush CPU-written fb before putmap */
#include "fb.h"           /* fb_draw_open/getmap/putmap/close */
#include "tjpgd.h"        /* software JPEG decode (recv path, JPEG branch) */
#include "mpi_vo.h"     /* VO hardware producer: fb5 (H264 recv) / fb6 PIP (JPEG send) */

#define LOG_TAG             "[KWT_VIDEO]"
#define LOG_ERROR_ENABLE
//#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_VSTREAM_TRANSFER

typedef struct {
    app_camera_handle_t cam_handle;     /* camera context from app_camera_open() */
    mpi_vpss_grp_t      vpss_grp;       /* cached VPSS group for worker thread */
    mpi_vpss_chn_t      vpss_chn;       /* VPSS output channel → VENC (ONLINE) */
    mpi_vpss_chn_t      pip_chn;        /* VPSS 2nd channel, sink-frame 80x60 → VO → fb6 */
    mpi_venc_chn_t      venc_chn;       /* JPEG encoder channel */
    mpi_vo_chn_t        vo_chn;         /* JPEG: VO channel producing fb6 (local PIP) */
    u8                  pip_vo_bound;   /* JPEG: pip_chn → VO bind succeeded */
    void               *pip_fb_draw;    /* H264: fb_draw producer handle for PIP (fb6) */
    u8                  pip_fb_opened;  /* H264: fb_draw_open() succeeded */
    u8                  exit_flag;      /* signal worker thread to stop */
    int                 thread_pid;     /* worker thread handle */
} kwt_xsmt_ctx_t;

static kwt_xsmt_ctx_t g_xsmt_ctx;

static int kwt_xsmt_vpss_venc_open(app_camera_handle_t ctx)
{
    int ret;
    u8 pip_created  = 0;   /* JPEG: 80x60 PIP chn created+enabled */
    u8 chn0_enabled = 0;   /* chn0 passed mpi_vpss_enable_chn */
    u8 venc_started = 0;   /* VENC passed mpi_venc_start_chn */
    u8 vo_started = 0;
    u8 chn0_bound = 0;

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &g_xsmt_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: no free VPSS channel (ret=%d)\n", __func__, ret);
        return -1;
    }

    ret = mpi_vpss_create_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn,
    &(mpi_vpss_chn_attr_t) {
        .width       = KWT_CAMERA_WIDTH,
        .height      = KWT_CAMERA_HEIGHT,
        .format      = MPI_PIXEL_FMT_YUV422_PACKED,
        .yuv_order   = MPI_YUV_ORDER_YUYV,
        .output_mode = MPI_VPSS_OUTPUT_ONLINE,
        .hw_id       = MPI_VPSS_HW_AUTO,
        .buf_addr    = NULL,
        .buf_size    = (u32)KWT_CAMERA_WIDTH * KWT_VPSS_LINE_NUM * 2u,
        .dst_fps     = KWT_CAMERA_FPS
    });
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_create_chn failed (ret=%d)\n", __func__, ret);
        return -1;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_enable_chn failed (ret=%d)\n", __func__, ret);
        goto err;
    }
    chn0_enabled = 1;

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &g_xsmt_ctx.pip_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: no free VPSS channel for PIP (ret=%d)\n", __func__, ret);
        goto err;
    }

    ret = mpi_vpss_create_chn(ctx->vpss_grp,
                              g_xsmt_ctx.pip_chn,
    &(mpi_vpss_chn_attr_t) {
        .width       = KWT_PIP_WIDTH,
        .height      = KWT_PIP_HEIGHT,
        .format      = MPI_PIXEL_FMT_YUV420SP,   /* NV12 → fb6 (YUV420P) */
                       .output_mode = MPI_VPSS_OUTPUT_OFFLINE,  /* buf_size=0 → sink-frame, VO supplies DMA target */
                                      .hw_id       = MPI_VPSS_HW_AUTO,
                                      .buf_addr    = NULL,
                                      .buf_size    = 0,   /* sink-frame: no VPSS local queue, zero CPU copy */
    });
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_create_chn(pip) failed (ret=%d)\n", __func__, ret);
        goto err;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vpss_enable_chn(pip) failed (ret=%d)\n", __func__, ret);
        mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
        goto err;
    }

    pip_created = 1;

    ret = mpi_venc_find_free_chn(&g_xsmt_ctx.venc_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: no free VENC channel (ret=%d)\n", __func__, ret);
        goto err;
    }

    ret = mpi_venc_create_chn(g_xsmt_ctx.venc_chn,
    &(mpi_venc_chn_attr_t) {
        .hw_dev          = 0,
        .type            = MPI_VENC_TYPE_JPEG,
        .input_mode      = MPI_VENC_INPUT_ONLINE,
        .quality         = KWT_CAMERA_JPEG_QUALITY,
        .in_width        = KWT_CAMERA_WIDTH,
        .in_height       = KWT_CAMERA_HEIGHT,
        .in_format       = MPI_PIXEL_FMT_YUV422_PACKED,
        .in_yuv_order    = MPI_YUV_ORDER_YUYV,
        .fps             = KWT_CAMERA_FPS,
        .abr_kbps        = KWT_CAMERA_ABR_KBPS,
        .stream_buf_size = KWT_CAMERA_STREAM_BUF_SZ,
    });
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_venc_create_chn failed (ret=%d)\n", __func__, ret);
        goto err;
    }

    ret = mpi_venc_start_chn(g_xsmt_ctx.venc_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_venc_start_chn failed (ret=%d)\n", __func__, ret);
        mpi_venc_destroy_chn(g_xsmt_ctx.venc_chn);
        goto err;
    }
    venc_started = 1;

    ret = mpi_vo_find_free_chn(&g_xsmt_ctx.vo_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_find_free_chn failed (ret=%d)\n", __func__, ret);
        goto err;
    }

    mpi_vo_chn_attr_t vo_attr = {
        .fb_num        = 2,
        .fb_width      = KWT_PIP_WIDTH,
        .fb_height     = KWT_PIP_HEIGHT,
        .window_width  = KWT_PIP_WIDTH,
        .window_height = KWT_PIP_HEIGHT,
        .fb_format     = FB_COLOR_FORMAT_YUV420P,   /* NV12, matches the fb6 LVGL widget */
        .combine       = 1
    };
    strncpy(vo_attr.fb_name, KWT_PIP_FB_NAME, sizeof(vo_attr.fb_name) - 1);
    ret = mpi_vo_create_chn(g_xsmt_ctx.vo_chn, &vo_attr);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_create_chn failed (ret=%d)\n", __func__, ret);
        goto err;
    }

    ret = mpi_vo_start_chn(g_xsmt_ctx.vo_chn);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_vo_start_chn failed (ret=%d)\n", __func__, ret);
        mpi_vo_destroy_chn(g_xsmt_ctx.vo_chn);
        goto err;
    }
    vo_started = 1;

    mpi_chn_t pip_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.pip_chn};
    mpi_chn_t vo_dst  = {.mod_id = MPI_MOD_ID_VO,   .dev_id = 0,            .chn_id = g_xsmt_ctx.vo_chn  };
    ret = mpi_sys_bind(&pip_src, &vo_dst);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_sys_bind pip->vo failed (ret=%d)\n", __func__, ret);
        goto err;
    }
    g_xsmt_ctx.pip_vo_bound = 1;

    mpi_chn_t vpss_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.vpss_chn};
    mpi_chn_t venc_dst = {.mod_id = MPI_MOD_ID_VENC, .dev_id = 0,            .chn_id = g_xsmt_ctx.venc_chn};
    ret = mpi_sys_bind(&vpss_src, &venc_dst);
    if (ret != MPI_SUCCESS) {
        log_error("%s: mpi_sys_bind vpss->venc failed (ret=%d)\n", __func__, ret);
        goto err;
    }
    chn0_bound = 1;

    /* cache vpss_grp for worker thread use */
    g_xsmt_ctx.vpss_grp = ctx->vpss_grp;

    log_info("%s: VPSS grp=%d chn=%d (ONLINE) pip chn=%d (sink->VO chn=%d) VENC chn=%d (ONLINE) ok\n",
             __func__, ctx->vpss_grp, g_xsmt_ctx.vpss_chn, g_xsmt_ctx.pip_chn, g_xsmt_ctx.vo_chn, g_xsmt_ctx.venc_chn);

    return 0;

err:
    if (g_xsmt_ctx.pip_vo_bound) {
        mpi_sys_unbind(&(mpi_chn_t) {
            .mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.pip_chn
        },
        &(mpi_chn_t) {
            .mod_id = MPI_MOD_ID_VO,   .dev_id = 0,             .chn_id = g_xsmt_ctx.vo_chn
        });
        (void)mpi_sys_unbind(&pip_src, &vo_dst);
        g_xsmt_ctx.pip_vo_bound = 0;
    }

    if (vo_started) {
        mpi_vo_stop_chn(g_xsmt_ctx.vo_chn);
        mpi_vo_destroy_chn(g_xsmt_ctx.vo_chn);
    }

    if (pip_created) {
        mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
        mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
    }

    if (chn0_bound) {
        mpi_sys_unbind(&(mpi_chn_t) {
            .mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.vpss_chn
        },
        &(mpi_chn_t) {
            .mod_id = MPI_MOD_ID_VENC, .dev_id = 0,             .chn_id = g_xsmt_ctx.venc_chn
        });
    }

    if (venc_started) {
        mpi_venc_stop_chn(g_xsmt_ctx.venc_chn);
        mpi_venc_destroy_chn(g_xsmt_ctx.venc_chn);
    }

    if (chn0_enabled) {
        mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    }

    mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    return -1;
}

/* ============================================================================
 * Internal: VPSS→VENC teardown
 * ============================================================================ */

static void kwt_xsmt_vpss_venc_close(app_camera_handle_t ctx)
{
    int unbind_ret = mpi_sys_unbind(&(mpi_chn_t) {
        .mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.vpss_chn
    },
    &(mpi_chn_t) {
        .mod_id = MPI_MOD_ID_VENC, .dev_id = 0,             .chn_id = g_xsmt_ctx.venc_chn
    });
    log_info("%s: vpss->venc unbind ret=%d\n", __func__, unbind_ret);

    mpi_stream_t stream;
    int i = 0;
    int stop_ret = MPI_ERR_BUSY;
    int destroy_ret = MPI_ERR_BUSY;

    stop_ret = mpi_venc_stop_chn(g_xsmt_ctx.venc_chn);

    destroy_ret = mpi_venc_destroy_chn(g_xsmt_ctx.venc_chn);
    if (destroy_ret != MPI_SUCCESS) {
        log_error("%s: venc destroy failed ret=%d (channel LEAKED)\n", __func__, destroy_ret);
    } else {
        log_info("%s: venc destroy ok\n", __func__);
    }

    mpi_chn_t vi_src   = {.mod_id = MPI_MOD_ID_VI,   .dev_id = ctx->vi_dev,   .chn_id = 0};
    mpi_chn_t vpss_dst = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = 0};
    (void)mpi_sys_unbind(&vi_src, &vpss_dst);

    int vpss_ret = MPI_ERR_BUSY;

    if (g_xsmt_ctx.pip_vo_bound) {
        mpi_chn_t pip_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = g_xsmt_ctx.pip_chn};
        mpi_chn_t vo_dst  = {.mod_id = MPI_MOD_ID_VO,   .dev_id = 0,            .chn_id = g_xsmt_ctx.vo_chn};
        int unbind_ret = mpi_sys_unbind(&pip_src, &vo_dst);
        log_info("%s: pip->vo unbind ret=%d\n", __func__, unbind_ret);
        g_xsmt_ctx.pip_vo_bound = 0;
    }

    int vo_ret = mpi_vo_stop_chn(g_xsmt_ctx.vo_chn);
    log_info("%s: vo stop ret=%d\n", __func__, vo_ret);
    vo_ret = mpi_vo_destroy_chn(g_xsmt_ctx.vo_chn);
    log_info("%s: vo destroy ret=%d\n", __func__, vo_ret);
    vpss_ret = mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
    log_info("%s: vpss pip disable ret=%d\n", __func__, vpss_ret);
    vpss_ret = mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.pip_chn);
    log_info("%s: vpss pip destroy ret=%d\n", __func__, vpss_ret);
    vpss_ret = mpi_vpss_disable_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    log_info("%s: vpss disable ret=%d\n", __func__, vpss_ret);
    vpss_ret = mpi_vpss_destroy_chn(ctx->vpss_grp, g_xsmt_ctx.vpss_chn);
    log_info("%s: vpss destroy ret=%d\n", __func__, vpss_ret);

    mpi_ret_t grp_ret = mpi_vpss_stop_grp(ctx->vpss_grp);
    log_info("%s: vpss stop grp ret=%d\n", __func__, grp_ret);
    grp_ret = mpi_vpss_destroy_grp(ctx->vpss_grp);
    log_info("%s: vpss destroy grp ret=%d\n", __func__, grp_ret);
    mpi_ret_t vi_ret = mpi_vi_disable_dev(ctx->vi_dev);
    log_info("%s: vi disable dev ret=%d\n", __func__, vi_ret);
    ctx->vi_vpss_src_enabled = 0;

    lv_video_disp_handle_t *handle = lv_port_video_disp_get_instance_by_fb_index(6);
    if (handle) {
        /* 拆 fb6 前先排空 LIST==2 在飞异步渲染, 否则释放 buffer 后异步渲染线程仍
         * 读该 buffer → use-after-free(与 lvgl_video_exit_drain 注释同因)。 */
        lvgl_video_exit_drain(handle);
    }

    log_info("%s: done\n", __func__);
}

static void kwt_xsmt_cap_thread(void *priv)
{
    extern u32 timer_get_ms(void);

    mpi_stream_t stream;
    int ret;
    int tx_frames = 0;
    int last_log = timer_get_ms();

    log_info("%s: started\n", __func__);

    while (!g_xsmt_ctx.exit_flag) {

        ret = mpi_venc_get_stream(g_xsmt_ctx.venc_chn, &stream, 0);
        if (ret != MPI_SUCCESS || stream.len == 0) {
            os_time_dly(1);
            continue;
        }

        /* PIP(fb6) 常显, 相机→VENC→PIP 管线在 init 时已启动, 与连接状态无关;
         * 只有发送动作(占用 SPP/信用)按 SPP 连接状态守卫: 未连接时仅丢弃编码帧,
         * 不断流 VENC, 保证重连后无需重启相机即可恢复发送。RX 路径不变。 */
        if (kwt_get_spp_connect()) {
            vstream_send_frame((u8 *)stream.addr, stream.len, VSTREAM_JPEG_TYPE_VIDEO);
            ++tx_frames;
        }

        mpi_venc_release_stream(g_xsmt_ctx.venc_chn, &stream);

        if (timer_get_ms() - last_log > 1000) {
            log_info("tx fps=%u", tx_frames);
            last_log = timer_get_ms();
            tx_frames = 0;
        }

        os_time_dly(1);
    }

    log_info("%s: completed\n", __func__);
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

int kid_wt_video_xsmt_start(void)
{
    extern void mem_stats(void);
    mem_stats();

    int ret;

    log_info("%s: enter\n", __func__);

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

    /* --- 4. allocate video lbuf in the sender --- */
    ret = vstream_sender_video_init();
    if (ret != 0) {
        log_error("%s: vstream_sender_video_init failed\n", __func__);
        kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);
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

    g_xsmt_ctx.exit_flag = 1;
    thread_kill(&g_xsmt_ctx.thread_pid, KILL_WAIT);
    kwt_xsmt_vpss_venc_close(g_xsmt_ctx.cam_handle);
    app_camera_src_close(g_xsmt_ctx.cam_handle);
    app_camera_close(&g_xsmt_ctx.cam_handle);
    vstream_sender_video_deinit();
    memset(&g_xsmt_ctx, 0, sizeof(g_xsmt_ctx));

    log_info("%s: done\n", __func__);
}

/* ============================================================================
 * Receiver: vstream VIDEO_FRAME → JPEG decode → LVGL display
 *
 * Polls vstream_recv_acquire_frame(VIDEO_FRAME) for JPEG frames received
 * over SPP, decodes them via software TJpgDec → NV12 → fb5 (JPEG branch) or
 * hardware VDEC→VPSS→VO→fb5 (H264 branch), and displays them through the fb5
 * LVGL widget created by ui_action_video_intercom.c (LVGL reads fb5 via
 * FBIOGET_FBUFFER_IDLE).
 * ============================================================================ */

typedef struct {
    void *fb5_draw;                      /* fb_draw producer handle for fb5 (software decode) */
    u8  exit_flag;                       /* signal worker thread to stop */
    int is_running;                      /* worker thread handle */
    u16 fps_tid;                         /* 1Hz FPS-printer sys_timer id (0 = not running) */
    u32 fps_count;                       /* received-frame count since last 1s print */
    u8  pipeline_opened;                 /* fb5 producer open / H264 pipeline open succeeded */
} kwt_video_recv_ctx_t;

static kwt_video_recv_ctx_t g_recv_ctx;

/* ============================================================================
 * Software JPEG decode (recv path): TJpgDec → NV12 → fb5
 *
 * Replaces the hardware app_playback pipeline (VDEC→VPSS→VO) so the JPEG codec
 * block and one IMC DST unit are freed for the send-side ONLINE VPSS→VENC.
 * TJpgDec state is static: the JDEC struct (~100 B) + 12 KB pool never live on
 * the 2048-B recv thread stack.
 *
 * 4:2:0 frames (the hardware VENC output) are written straight to NV12 from the
 * IDCT blocks (kwt_jd_decomp_nv12), skipping tjpgd's RGB888 round-trip; other
 * samplings fall back to the old RGB888→NV12 outfunc.
 * ============================================================================ */

static JDEC g_jd;
/* u32[] so the pool base is 4-byte aligned: tjpgd stores int32_t qttbl tables
 * at pool offsets verbatim (alloc_pool returns the base unaligned).
 * 12 KB: JD_SZBUF 512 + huffman/quant tables + MCU bufs + the JD_FASTDECODE=2
 * huffman LUTs (~6 KB: 2x AC 2 KB + 2x DC 1 KB). */
static u32   g_jd_pool[3072];
static struct fb_map_user *g_recv_fb5_map;    /* fb5 map during one decode */

typedef struct {
    const u8 *data;
    size_t   len;
    size_t   off;
} kwt_jd_in_t;

/* TJpgDec input function: serve bytes from the acquired SPP frame buffer.
 * Return 0 at end of stream.
 * CONTRACT (see tjpgd.c): buf == NULL means "skip ndata bytes, do not copy"
 * (used for unknown segments such as APP0/JFIF). Must still advance off. */
static size_t kwt_jd_infunc(JDEC *jd, u8 *buf, size_t want)
{
    kwt_jd_in_t *in = (kwt_jd_in_t *)jd->device;
    size_t n = in->len - in->off;

    if (n > want) {
        n = want;
    }
    if (buf) {
        memcpy(buf, in->data + in->off, n);
    }
    in->off += n;
    return n;   /* 0 = end of stream */
}

/* TJpgDec output function: convert each MCU from packed RGB888 (tjpgd writes
 * B,G,R byte order, jd_mcu_output tjpgd.c) into the fb5 NV12 buffer.
 * rect.right/bottom are INCLUSIVE (jd_mcu_output rect calc). Chroma: NV12
 * holds one U/V pair per 2x2 block — sample the even-position pixels. */
static int kwt_jd_bgr_to_nv12_outfunc(JDEC *jd, void *bitmap, JRECT *rect)
{
    const u8 *bgr = (const u8 *)bitmap;
    struct fb_map_user *map = g_recv_fb5_map;
    u32 w = (u32)rect->right - rect->left + 1;
    u32 h = (u32)rect->bottom - rect->top + 1;
    u8 *y_plane;
    u8 *uv_plane;
    u32 x, y;

    (void)jd;

    /* fb5 map 无效时中止解码 (返回 0 -> jd_decomp 以 JDR_INTR 干净退出,
     * 而不是向 NULL 写入触发与 infunc 同类别的硬件异常) */
    if (!map || !map->baddr) {
        return 0;
    }
    y_plane  = map->baddr;
    uv_plane = map->uv_baddr ? map->uv_baddr
               : (map->baddr + KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT);

    for (y = 0; y < h; y++) {
        u32 img_y = rect->top + y;
        const u8 *row = bgr + y * w * 3;
        u8 *y_out = y_plane + img_y * KWT_CAMERA_WIDTH;
        for (x = 0; x < w; x++) {
            u32 img_x = rect->left + x;
            const u8 *p = row + x * 3;
            int b = p[0], g = p[1], r = p[2];   /* signed: BT.601 uses negative chroma coefficients */

            y_out[img_x] = (u8)(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
            if ((img_x & 1) == 0 && (img_y & 1) == 0) {
                u32 uv_off = (img_y >> 1) * KWT_CAMERA_WIDTH + (img_x >> 1) * 2;
                uv_plane[uv_off]     = (u8)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
                uv_plane[uv_off + 1] = (u8)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
            }
        }
    }
    return 1;   /* 1 = continue decoding */
}

/* ---- 4:2:0 fast path: IDCT blocks -> NV12, no RGB round-trip ----------
 * tjpgd's mcubuf holds one MCU as 4 Y blocks + Cb + Cr (each 64 int16, values
 * already +128 DC-biased by block_idct). For 4:2:0 (msx==2 && msy==2) Cb/Cr are
 * already at NV12's half resolution, so we store Y verbatim (clipped) and write
 * each Cb/Cr sample directly as the interleaved U/V byte — the old RGB888 path
 * had to downsample 2x2 back to that same sample, at 3 mults/pixel, twice.
 * The hardware VENC (GC0308) emits 4:2:0, so this is the hot path. */
static inline u8 kwt_clip16(int v)
{
    return (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

static void kwt_jd_mcu_to_nv12(JDEC *jd, unsigned int x, unsigned int y)
{
    const int16_t *Y0 = jd->mcubuf;      /* 4 luma blocks, raster ordered */
    const int16_t *Y1 = Y0 + 64;
    const int16_t *Y2 = Y1 + 64;
    const int16_t *Y3 = Y2 + 64;
    const int16_t *Cb = Y3 + 64;         /* chroma blocks (half-res for 4:2:0) */
    const int16_t *Cr = Cb + 64;
    struct fb_map_user *map = g_recv_fb5_map;
    u8 *y_plane  = map->baddr;
    u8 *uv_plane = map->uv_baddr ? map->uv_baddr
                   : (map->baddr + KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT);

    /* right/bottom MCU may be clipped at the 320x240 frame edge */
    unsigned int rx = (x + 16 <= KWT_CAMERA_WIDTH)  ? 16 : KWT_CAMERA_WIDTH  - x;
    unsigned int ry = (y + 16 <= KWT_CAMERA_HEIGHT) ? 16 : KWT_CAMERA_HEIGHT - y;
    unsigned int by, bx;

    for (by = 0; by < ry; by++) {
        u8 *y_out = y_plane + (y + by) * KWT_CAMERA_WIDTH + x;
        unsigned int br = by & 7;                 /* row index inside an 8x8 block */
        const int16_t *blkL, *blkR;
        if (by < 8) {                             /* top half: Y0 | Y1 */
            blkL = Y0 + br * 8;
            blkR = Y1 + br * 8;
        } else {                                  /* bottom half: Y2 | Y3 */
            blkL = Y2 + br * 8;
            blkR = Y3 + br * 8;
        }
        for (bx = 0; bx < 8; bx++) {
            if (bx < rx) {
                y_out[bx] = kwt_clip16(blkL[bx]);
            }
            if (bx + 8 < rx) {
                y_out[bx + 8] = kwt_clip16(blkR[bx]);
            }
        }
    }

    /* chroma: 8x8 blocks -> NV12 interleaved U/V plane */
    for (by = 0; by < ry / 2; by++) {
        u8 *uv_row = uv_plane + ((y >> 1) + by) * KWT_CAMERA_WIDTH + (x >> 1) * 2;
        const int16_t *cb = Cb + by * 8;
        const int16_t *cr = Cr + by * 8;
        for (bx = 0; bx < rx / 2; bx++) {
            uv_row[bx * 2]     = kwt_clip16(cb[bx]);
            uv_row[bx * 2 + 1] = kwt_clip16(cr[bx]);
        }
    }
}

/* Own MCU loop (mirrors jd_decomp's loop, tjpgd.c) but bypasses the RGB888
 * jd_mcu_output(): jd_mcu_load() fills mcubuf with Y/Cb/Cr blocks, then we write
 * them straight to NV12. Handles restart markers; aborts (JDR_INTR) if the fb5
 * map vanished. Only valid for 4:2:0 (msx==2 && msy==2). */
static JRESULT kwt_jd_decomp_nv12(JDEC *jd)
{
    const unsigned int mx = 16, my = 16;   /* 4:2:0 MCU is 16x16 px */
    unsigned int x, y;
    uint16_t rst, rsc;
    JRESULT rc;

    jd->scale = 0;
    jd->dcv[2] = jd->dcv[1] = jd->dcv[0] = 0;
    rst = rsc = 0;

    for (y = 0; y < jd->height; y += my) {
        for (x = 0; x < jd->width; x += mx) {
            if (jd->nrst && rst++ == jd->nrst) {
                rc = jd_restart(jd, rsc++);
                if (rc != JDR_OK) {
                    return rc;
                }
                rst = 1;
            }
            rc = jd_mcu_load(jd);
            if (rc != JDR_OK) {
                return rc;
            }
            kwt_jd_mcu_to_nv12(jd, x, y);
        }
    }
    return JDR_OK;
}


void kwt_video_recv_decode_jpeg(u8 *buf, u32 len)
{
    /* 软件 JPEG 解码: TJpgDec 内联解码 SPP 收到的帧并写入 fb5。
     * 释放硬件 VDEC/VPSS/VO(JPEG codec + IMC DST)给发送端 ONLINE。 */

    kwt_jd_in_t in = { .data = buf, .len = (size_t)len, .off = 0 };
    struct fb_map_user map;
    JRESULT jr;

    memset(&g_jd, 0, sizeof(g_jd));
    jr = jd_prepare(&g_jd, kwt_jd_infunc, g_jd_pool, sizeof(g_jd_pool), &in);
    if (jr != JDR_OK) {
        log_error("jd_prepare failed r=%d len=%d\n", jr, len);
    } else if (g_jd.width > KWT_CAMERA_WIDTH || g_jd.height > KWT_CAMERA_HEIGHT) {
        /* 收到的 JPEG SOF 尺寸超过 fb5(320x240): 以固定 320 步长写 Y/UV 平面,
         * 不校验 jd->width/height, 越界会覆盖 fb5 之外的内存。解码前拒绝,
         * 防止远程畸形 JPEG 触发 OOB 写。 */
        log_error("jd SOF dims out of bounds: %u x %u\n",
                  g_jd.width, g_jd.height);
    } else {
        memset(&map, 0, sizeof(map));
        if (fb_draw_getmap(g_recv_ctx.fb5_draw, &map) != 0) {
            log_error("fb5 getmap failed\n");
        } else {
            g_recv_fb5_map = &map;
            if (g_jd.msx == 2 && g_jd.msy == 2) {
                /* 4:2:0: 直接从 IDCT 块写 NV12, 跳过 RGB888 往返 */
                jr = kwt_jd_decomp_nv12(&g_jd);
            } else {
                /* 4:4:4/4:2:2 等: 回退 RGB888→NV12 旧路径 */
                jr = jd_decomp(&g_jd, kwt_jd_bgr_to_nv12_outfunc, 0);
            }
            g_recv_fb5_map = NULL;
            if (jr != JDR_OK) {
                log_error("jd_decomp failed r=%d\n", jr);
            }
            if (map.uv_baddr) {
                DcuFlushinvRegion((void *)map.baddr, KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT);
                DcuFlushinvRegion((void *)map.uv_baddr, KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT / 2);
            } else {
                DcuFlushinvRegion((void *)map.baddr, KWT_CAMERA_WIDTH * KWT_CAMERA_HEIGHT * 3 / 2);
            }
            fb_draw_putmap(g_recv_ctx.fb5_draw, &map);
        }
    }

}

static void kwt_video_recv_thread(void *priv)
{
    extern u32 timer_get_ms(void);

    u8 *buf = NULL;
    int len = 0;
    int ret;
    u32 recv_cnt = 0;        /* 重连后累计收到的视频帧数, 用于定位卡死位置 */
    u32 last_log = timer_get_ms();

    log_info("thread start, pid=%s\n", os_current_task());

    while (!g_recv_ctx.exit_flag) {
        if (stream_dev_create_video() != 0) {
            os_time_dly(1);
            continue;
        }
        log_info("stream_dev ready, polling frames\n");
        break;
    }

    while (!g_recv_ctx.exit_flag) {
        u8 *buf2;

        ret = vstream_recv_acquire_frame(0, &buf2, &len, VIDEO_FRAME);
        if (ret != 0) {
            os_time_dly(1);
            continue;
        }

        buf = malloc(len);
        if (buf == NULL) {
            log_error("rx task out of memory");
            os_time_dly(1);
            continue;
        }

        memcpy(buf, buf2, len);

        video_stream_recv_release_frame(buf2);
        recv_cnt++;

        extern u32 timer_get_ms(void);
        u32 begin_decode  = timer_get_ms();
        kwt_video_recv_decode_jpeg(buf, len);
        log_info("decode: %ums", timer_get_ms() - begin_decode);

        if (timer_get_ms() - last_log > 1000) {
            log_info("rx fps=%u", recv_cnt);
            mem_stats();
            last_log = timer_get_ms();
            recv_cnt = 0;
        }

        ret = 0;
        free(buf);
        buf = NULL;

        os_time_dly(1);
    }

    log_info("thread exit\n");

    os_task_del_res(OS_TASK_SELF);
}

int kid_wt_video_recv_is_running(void)
{
    return (g_recv_ctx.is_running != 0 && !g_recv_ctx.exit_flag);
}

int kid_wt_video_recv_get_status(void)
{
    return (g_recv_ctx.is_running != 0 && !g_recv_ctx.exit_flag) ? 1 : 0;
}

int kid_wt_video_recv_start(void)
{
#if KWT_DISABLE_DECODE
    log_info("%s: decode disabled by KWT_DISABLE_DECODE, skip\n", __func__);
    return 0;
#endif

    int ret;

    /* idempotent */
    if (kid_wt_video_recv_is_running()) {
        log_info("already started\n");
        return 0;
    }

    /* JPEG: 软件解码。fb5 由 fb_draw 软件生产者打开(LVGL 控件按名字复用
     * 同一设备, 幂等), 替代 app_playback 的硬件 VDEC→VPSS→VO 流水线。 */
    struct fb_draw_info info;
    memset(&info, 0, sizeof(info));
    info.name    = KWT_RECV_FB_NAME;
    info.width   = KWT_CAMERA_WIDTH;
    info.height  = KWT_CAMERA_HEIGHT;
    info.format  = FB_COLOR_FORMAT_YUV420P;   /* NV12, matches the LVGL widget */
    info.fb_num  = 2;                         /* double buffering */
    info.combine = 1;                         /* composite via fb_out */
    g_recv_ctx.fb5_draw = fb_draw_open(&info);
    if (!g_recv_ctx.fb5_draw) {
        log_error("%s: fb_draw_open(%s) failed\n", __func__, KWT_RECV_FB_NAME);
        return -1;
    }
    g_recv_ctx.pipeline_opened = 1;

    /* 3. worker thread */
    g_recv_ctx.exit_flag = 0;
    ret = os_task_create_affinity_core(kwt_video_recv_thread, NULL, 20, 2048, 0, "kwt_video_recv", 1);
    log_info("thread_fork ret=%d\n", ret);
    if (ret != 0) {
        fb_draw_close(g_recv_ctx.fb5_draw);
        g_recv_ctx.fb5_draw = NULL;
        g_recv_ctx.pipeline_opened = 0;
        return -1;
    }

    g_recv_ctx.is_running = 1;

    return 0;
}

void kid_wt_video_recv_stop(void)
{
    if (!kid_wt_video_recv_is_running()) {
        return;
    }

    g_recv_ctx.exit_flag = 1;
    while (OS_TASK_NOT_EXIST != os_task_del_req("kwt_video_recv")) {
        os_time_dly(1);
    }

    log_info("thread killed\n");

    g_recv_ctx.is_running = 0;

    /* 2. close the fb5 software producer (fb_draw) */
    if (g_recv_ctx.fb5_draw) {
        fb_draw_close(g_recv_ctx.fb5_draw);
        g_recv_ctx.fb5_draw = NULL;
        log_info("fb5 software producer closed\n");
    }
    g_recv_ctx.pipeline_opened = 0;

    stream_dev_media_clear();

    lv_video_disp_handle_t *handle = lv_port_video_disp_get_instance_by_fb_index(5);
    if (handle) {
        /* 拆 fb5 前先排空 LIST==2 在飞异步渲染(与其它视频模块一致: video_rec/
         * video_photo/video_dec 均用 lvgl_video_exit_drain)。白屏 overlay 已移除,
         * 不再有遮蔽, 非 drain 删除会在拆 fb5 时暴露绿屏/mmu 异常。 */
        lvgl_video_exit_drain(handle);
        log_info("lvgl_video_exit_drain done\n");
    }

    memset(&g_recv_ctx, 0, sizeof(g_recv_ctx));
}

#endif /* TCFG_VSTREAM_TRANSFER */
