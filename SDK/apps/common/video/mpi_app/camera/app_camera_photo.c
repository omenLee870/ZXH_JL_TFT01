#include "app_camera_mpi_priv.h"
#include "app_config.h"
#include "asm/dcache.h"
#include "fb.h"
#include "jpeg_codec.h"
#if ((defined CONFIG_JL_UI_ENABLE)&& CONFIG_JL_UI_ENABLE)
#include "jlgpu_def.h"
#include "jlgpu_image_filters.h"
#endif
#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
#include "jlgpu_image_filters.h"
#include "lv_port_video_disp.h"
/* 拍照 VENC 编码期间暂停/恢复 LVGL 刷屏 (lv_port_disp.c), 防 GPU top 联动渲染卡死 */
extern void disp_disable_update(void);
extern void disp_enable_update(void);
/* 拍照编码期间持有 JLGPU 互斥锁, 阻止 LVGL C3/JLGPU 中途复位 JPEG 硬件 */
extern void lv_jlgpu_mutex_pend(void);
extern void lv_jlgpu_mutex_post(void);
#endif

#define APP_CAMERA_PHOTO_FORMAT             MPI_PIXEL_FMT_YUV422_PACKED
#define APP_CAMERA_PHOTO_YUV_ORDER          MPI_YUV_ORDER_UYVY
#define APP_CAMERA_PHOTO_DEFAULT_QUALITY    80u
#define APP_CAMERA_PHOTO_STREAM_BUF_SIZE    (100* 1024u)
#define APP_CAMERA_PHOTO_TIMEOUT_MS         300
#define APP_CAMERA_PHOTO_DONE_TIMEOUT_MS    5000
#define APP_CAMERA_PHOTO_ONLINE_LINE_NUM    16u
#define APP_CAMERA_PHOTO_LBUF_EXTRA         1024u
#define APP_CAMERA_PHOTO_VENC_HW_DEV        0
#define APP_CAMERA_PHOTO_THUMB_WIDTH        320u
#define APP_CAMERA_PHOTO_THUMB_HEIGHT       240u
#define APP_CAMERA_PHOTO_THUMB_BUF_SIZE     (64 * 1024u)


#ifdef CONFIG_LVGL_UI_ENABLE
static int app_camera_photo_gpu_format(int format)
{
    return format == MPI_PIXEL_FMT_YUV420SP ? JLGPU_FORMAT_YUV420_BT601 : JLGPU_FORMAT_YUV422_BT601;
}

static void app_camera_photo_dump_frame(const char *path, const mpi_video_frame_info_t *frame)
{
    if (!path || !frame || !frame->y_addr || frame->y_len == 0) {
        return;
    }

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        printf("open %s failed\n", path);
        return;
    }
    fwrite(frame->y_addr, 1, frame->y_len, fp);
    if (frame->c_addr && frame->c_len) {
        fwrite(frame->c_addr, 1, frame->c_len, fp);
    }
    fclose(fp);
    printf("dump %s w=%u h=%u fmt=%d y=%u c=%u\n", path, frame->width, frame->height,
           frame->format, frame->y_len, frame->c_len);
}


static void app_camera_photo_apply_effect(mpi_video_frame_info_t *frame, const app_camera_photo_attr_t *attr)
{
    int mode;
    const char *sticker_path = NULL;

    if (!frame || !frame->y_addr || !attr) {
        return;
    }

    mode = attr->effect;
    if (mode < 0) {
        return;
    }

    if (mode == JLGPU_FILTER_STICKER) {
        sticker_path = attr->sticker_path;
        if (!sticker_path || !sticker_path[0]) {
            sticker_path = "storage/res_nor_mode/C/lvgl/4b000013.zip";
        }
    }

#if 0
    app_camera_photo_dump_frame("storage/sd1/C/filter_before.yuv", frame);
#endif
#if CONFIG_LVGL_UI_ENABLE && LV_USE_DRAW_JLGPU_LIST
    int gpu_format = app_camera_photo_gpu_format(frame->format);
    printf("gpu_format:%d\n", gpu_format);
    jlgpu_filter_session_t *sess = jlgpu_filter_session_init(frame->width, frame->height, gpu_format, gpu_format);
    if (!sess) {
        printf("photo filter session init failed\n");
        return;
    }

    jlgpu_sticker_t sticker = {
        .x = 0,
        .y = 0,
        .path = sticker_path,
    };
    jlgpu_filter_session_process(sess, (const uint8_t *)frame->y_addr, (uint8_t *)frame->y_addr, (jlgpu_filter_mode_t)mode,
                                 mode == JLGPU_FILTER_STICKER ? &sticker : NULL);
#if 0
    app_camera_photo_dump_frame("storage/sd1/C/filter_after.yuv", frame);
#endif
    jlgpu_filter_session_uninit(sess);
#endif
}
#endif

static uint32_t photo_calc_frame_buf_size(uint16_t width, uint16_t height, int format)
{
    if (format == MPI_PIXEL_FMT_YUV420SP) {
        return (uint32_t)width * height * 3u / 2u + APP_CAMERA_PHOTO_LBUF_EXTRA;
    } else {
        return (uint32_t)width * height * 2u + APP_CAMERA_PHOTO_LBUF_EXTRA;
    }
}

static uint32_t photo_calc_online_buf_size(uint16_t width, int format)
{
    if (format == MPI_PIXEL_FMT_YUV420SP) {
        return (uint32_t)width * APP_CAMERA_PHOTO_ONLINE_LINE_NUM * 3u / 2u;
    } else {
        return (uint32_t)width * APP_CAMERA_PHOTO_ONLINE_LINE_NUM * 2u;
    }
}

/* 暂停显示通路: unbind VO → stop/destroy VO → disable VPSS display chn,
 * 释放 IMC SRC0 的 UB。
 * 返回 1=暂停成功(需 resume), 0=无显示可拆(安全, 无需 resume), -1=失败。 */
static int app_camera_photo_display_suspend(app_camera_handle_t ctx)
{
#if !TCFG_UI_ENABLE
    if (!app_camera_display_is_started(ctx)) {   // 即 !ctx->vpss_vo_enabled
        return 0;                                 // 无显示占用 UB, 整帧离线缓冲可直接用, 无需挂起/resume
    }
#endif
    mpi_chn_t vpss_ch = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_display};
    mpi_chn_t vo_ch   = {.mod_id = MPI_MOD_ID_VO, .dev_id = 0, .chn_id = ctx->vo_chn};

#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    /* 拆显示通路前先停 LVGL 刷屏: 编码窗口内任意一次 GPU top 联动渲染都会永久卡死 */
    disp_disable_update();
#endif

    // 先挂LVGL/退fb5控件(否则VO省不了225KB), 再停VO

#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    extern void lvgl_video_exit_sync(void *handle);
    extern void lvgl_ui_task_suspend_resume(uint8_t state);
    lvgl_ui_task_suspend_resume(0);
    lv_video_disp_handle_t *v_hdl = lv_port_video_disp_get_instance_by_fb_index(5);   /* fb5 */
    if (v_hdl) {
        lvgl_video_exit_sync(v_hdl);
    }
#endif

    (void)mpi_sys_unbind(&vpss_ch, &vo_ch);
    (void)mpi_vo_stop_chn(ctx->vo_chn);
    (void)mpi_vo_destroy_chn(ctx->vo_chn);

    if (mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_display) != MPI_SUCCESS) {
        printf("failed to disable display chn, UB may overflow\n");
#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
        extern void lvgl_ui_task_suspend_resume(uint8_t state);
        lvgl_ui_task_suspend_resume(1);   /* 失败抖底: 解挂LVGL防冻死 */
        disp_enable_update();
#endif
        return -1;
    }
    return 1;
}

/* 供 photo 流程使用: 拉起拍照 VENC JPEG, 等待编码完成,
 * 将码流输出到指定 buf 中 */
static mpi_ret_t app_camera_wait_photo_output(app_camera_handle_t ctx,
        const app_camera_photo_attr_t *attr)
{
    mpi_stream_t stream = {0};
    mpi_ret_t ret;

    if (!attr->jpeg_buf) {
        return mpi_venc_wait_photo_done(ctx->venc_chn_photo, APP_CAMERA_PHOTO_DONE_TIMEOUT_MS);
    }

    ret = mpi_venc_get_stream(ctx->venc_chn_photo, &stream, APP_CAMERA_PHOTO_DONE_TIMEOUT_MS);
    if (ret != MPI_SUCCESS) {
        printf("failed to get encoded photo stream: %d\n", ret);
        return ret;
    }
    if (stream.len > attr->jpeg_buf_size) {
        printf("encoded photo too large: %u/%u\n", stream.len, attr->jpeg_buf_size);
        mpi_venc_release_stream(ctx->venc_chn_photo, &stream);
        return MPI_ERR_FULL;
    }

    memcpy(attr->jpeg_buf, stream.addr, stream.len);
    *attr->jpeg_size = stream.len;
    return mpi_venc_release_stream(ctx->venc_chn_photo, &stream);
}

/* 用 display_attr 重建 VO chn, (重)开启 VPSS display chn, 并 bind */
static int app_camera_photo_display_resume(app_camera_handle_t ctx)
{
#if !TCFG_UI_ENABLE
    if (!app_camera_display_is_started(ctx)) {
        return -1;   // 无显示可恢复
    }
#endif
    const app_camera_display_attr_t *da = &ctx->display_attr;
    mpi_vo_chn_attr_t vo_attr;

    if (mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_display) != MPI_SUCCESS) {
        printf("failed to enable display chn\n");
        goto err_disp_enable;
    }

    memset(&vo_attr, 0, sizeof(vo_attr));
    vo_attr.fb_num        = da->fb_num;
    strncpy(vo_attr.fb_name, da->fb_name, sizeof(vo_attr.fb_name) - 1);
    vo_attr.fb_width      = da->width;
    vo_attr.fb_height     = da->height;
    vo_attr.window_width  = da->width;
    vo_attr.window_height = da->height;
    vo_attr.left          = da->left;
    vo_attr.top           = da->top;
    vo_attr.fb_format     = FB_COLOR_FORMAT_YUV420P;
    vo_attr.rotate        = da->rotate;
    vo_attr.mirror        = da->mirror;
    vo_attr.combine       = 1;

    if (mpi_vo_create_chn(ctx->vo_chn, &vo_attr) != MPI_SUCCESS) {
        (void)mpi_vo_find_free_chn(&ctx->vo_chn);
        if (mpi_vo_create_chn(ctx->vo_chn, &vo_attr) != MPI_SUCCESS) {
            printf("failed to create VO chn\n");
            goto err_disp_enable;
        }
    }
    (void)mpi_vo_start_chn(ctx->vo_chn);

    {
        mpi_chn_t vpss_ch = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_display};
        mpi_chn_t vo_ch   = {.mod_id = MPI_MOD_ID_VO, .dev_id = 0, .chn_id = ctx->vo_chn};

        if (mpi_sys_bind(&vpss_ch, &vo_ch) != MPI_SUCCESS) {
            printf("failed to rebind display VPSS->VO\n");
            goto err_disp_enable;
        }
    }
    if (ctx->digital_zoom_enabled &&
        app_camera_set_digital_zoom(ctx, &ctx->digital_zoom_param) != APP_CAMERA_OK) {
        printf("failed to restore display crop\n");
        goto err_disp_enable;
    }

#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    /* 恢复显示层: 先恢复LVGL任务(视频控件创建依赖rpc), 再重建fb5视频控件 */
    extern void lvgl_ui_task_suspend_resume(uint8_t state);
    extern void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name);
    lvgl_ui_task_suspend_resume(1);
    void *new_v_hdl = lvgl_video_init(da->width, da->height, da->left, da->top, "fb5");
    printf("fb5 video overlay rebuilt for photo: %p\n", new_v_hdl);

    /* 显示通路重建完成后再恢复 LVGL 刷屏, 避免 flush 在显示未就绪时触发 GPU top 联动而永久超时 */
    disp_enable_update();
#endif
    return 0;

err_disp_enable:
#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    extern void lvgl_ui_task_suspend_resume(uint8_t state);
    lvgl_ui_task_suspend_resume(1);   /* 兜底: 恢复失败也必须解挂LVGL, 防屏幕冻死 */
    disp_enable_update();
#endif
    return -1;
}

static app_camera_ret_t app_camera_encode_offline_photo(app_camera_handle_t ctx,
        const app_camera_photo_attr_t *attr,
        const mpi_video_frame_info_t *frame)
{
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_ret_t ret;

    ctx->venc_dev_photo = APP_CAMERA_PHOTO_VENC_HW_DEV;
    ret = mpi_venc_find_free_chn(&ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free offline photo VENC channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    venc_attr.hw_dev = ctx->venc_dev_photo;
    venc_attr.type = MPI_VENC_TYPE_JPEG;
    venc_attr.input_mode = MPI_VENC_INPUT_OFFLINE;
    venc_attr.quality = attr->quality ? attr->quality : APP_CAMERA_PHOTO_DEFAULT_QUALITY;
    venc_attr.in_width = frame->width;
    venc_attr.in_height = frame->height;
    venc_attr.in_format = frame->format;
    venc_attr.in_yuv_order = frame->yuv_order;
    venc_attr.stream_buf_size = attr->stream_buf_size ? attr->stream_buf_size : APP_CAMERA_PHOTO_STREAM_BUF_SIZE;
    ret = mpi_venc_create_chn(ctx->venc_chn_photo, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create offline photo VENC channel\n");
        return APP_CAMERA_ERR_IO;
    }

    ret = mpi_venc_start_chn(ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to start offline photo VENC channel\n");
        goto err_destroy_venc;
    }

    ret = mpi_venc_send_frame(ctx->venc_chn_photo, frame);
    if (ret != MPI_SUCCESS) {
        printf("failed to send frame to offline photo VENC\n");
        goto err_stop_venc;
    }

    ret = app_camera_wait_photo_output(ctx, attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to get offline encoded photo\n");
        goto err_stop_venc;
    }

    mpi_venc_stop_chn(ctx->venc_chn_photo);
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
    return APP_CAMERA_OK;

err_stop_venc:
    mpi_venc_stop_chn(ctx->venc_chn_photo);
err_destroy_venc:
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
    return APP_CAMERA_ERR_IO;
}

static void app_camera_photo_file_created(void *fp, void *priv)
{
    app_camera_handle_t ctx = (app_camera_handle_t)priv;
    if (ctx->last_photo_file) {
        fget_name((FILE *)fp, (u8 *)ctx->last_photo_file, 128);
    }
    if (ctx->file_done_cb) {
        ctx->file_done_cb((FILE *)fp, ctx->file_dir_path, ctx->file_done_priv);
    }
}

/* 用 replay group 编一帧: VPSS channel(IMC缩放) + VENC + bind + send + wait。
 * jpeg_buf 非空时走 get_stream 拿流(photo_mode=0), 为空时写文件(photo_mode=1)。 */
static app_camera_ret_t app_camera_replay_encode_once(app_camera_handle_t ctx,
        const mpi_video_frame_info_t *frame,
        uint16_t out_width, uint16_t out_height,
        uint8_t thumbnail_create,
        uint8_t *head_app1, uint32_t head_app1_len,
        const app_camera_photo_attr_t *attr,
        uint8_t *jpeg_buf, uint32_t jpeg_buf_size, uint32_t *jpeg_size)
{
    mpi_ret_t ret;
    app_camera_ret_t app_ret = APP_CAMERA_ERR_IO;
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_chn_t vpss_chn_src;
    mpi_chn_t venc_chn_dst;

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp_photo, &ctx->vpss_chn_photo_large);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS photo channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    chn_attr.width = out_width;
    chn_attr.height = out_height;
    chn_attr.format = frame->format;
    chn_attr.yuv_order = frame->yuv_order;
    chn_attr.mirror = 0;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_ONLINE;
    chn_attr.buf_size = photo_calc_online_buf_size(out_width, chn_attr.format);
    ret = mpi_vpss_create_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VPSS photo channel\n");
        return APP_CAMERA_ERR_IO;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS photo channel\n");
        goto err_destroy_vpss_chn;
    }

    ctx->venc_dev_photo = APP_CAMERA_PHOTO_VENC_HW_DEV;
    ret = mpi_venc_find_free_chn(&ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VENC photo channel\n");
        goto err_disable_vpss_chn;
    }

    venc_attr.hw_dev = ctx->venc_dev_photo;
    venc_attr.type = MPI_VENC_TYPE_JPEG;
    venc_attr.input_mode = MPI_VENC_INPUT_ONLINE;
    venc_attr.quality = attr->quality ? attr->quality : APP_CAMERA_PHOTO_DEFAULT_QUALITY;
    venc_attr.in_width = out_width;
    venc_attr.in_height = out_height;
    venc_attr.in_format = chn_attr.format;
    venc_attr.in_yuv_order = chn_attr.yuv_order;
    venc_attr.stream_buf_size = attr->stream_buf_size ? attr->stream_buf_size : APP_CAMERA_PHOTO_STREAM_BUF_SIZE;
    venc_attr.photo_mode = jpeg_buf ? 0 : 1;
    venc_attr.photo_path = jpeg_buf ? NULL : attr->path;
    venc_attr.photo_file_cb = app_camera_photo_file_created;
    venc_attr.photo_file_cb_priv = ctx;
    venc_attr.thumbnail_create = thumbnail_create;
    venc_attr.head_app1 = head_app1;
    venc_attr.head_app1_len = head_app1_len;
    ret = mpi_venc_create_chn(ctx->venc_chn_photo, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VENC photo channel\n");
        goto err_disable_vpss_chn;
    }

    ret = mpi_venc_start_chn(ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VENC photo channel\n");
        goto err_destroy_venc_chn;
    }

    vpss_chn_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_chn_src.dev_id = ctx->vpss_grp_photo;
    vpss_chn_src.chn_id = ctx->vpss_chn_photo_large;
    venc_chn_dst.mod_id = MPI_MOD_ID_VENC;
    venc_chn_dst.dev_id = ctx->venc_dev_photo;
    venc_chn_dst.chn_id = ctx->venc_chn_photo;

    ret = mpi_sys_bind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS and VENC photo channel\n");
        goto err_stop_venc_chn;
    }

    ret = mpi_vpss_send_frame(ctx->vpss_grp_photo, frame, APP_CAMERA_PHOTO_TIMEOUT_MS);
    if (ret != MPI_SUCCESS) {
        printf("failed to send frame to VPSS group\n");
        goto err_unbind_vpss_venc;
    }

    if (jpeg_buf) {
        mpi_stream_t stream = {0};
        ret = mpi_venc_get_stream(ctx->venc_chn_photo, &stream, APP_CAMERA_PHOTO_DONE_TIMEOUT_MS);
        if (ret != MPI_SUCCESS) {
            printf("failed to get encoded stream: %d\n", ret);
            goto err_unbind_vpss_venc;
        }
        if (stream.len > jpeg_buf_size) {
            printf("encoded stream too large: %u/%u\n", stream.len, jpeg_buf_size);
            mpi_venc_release_stream(ctx->venc_chn_photo, &stream);
            goto err_unbind_vpss_venc;
        }
        memcpy(jpeg_buf, stream.addr, stream.len);
        if (jpeg_size) {
            *jpeg_size = stream.len;
        }
        mpi_venc_release_stream(ctx->venc_chn_photo, &stream);
    } else {
        ret = mpi_venc_wait_photo_done(ctx->venc_chn_photo, APP_CAMERA_PHOTO_DONE_TIMEOUT_MS);
        if (ret != MPI_SUCCESS) {
            printf("failed to wait VENC photo done\n");
            goto err_unbind_vpss_venc;
        }
    }

    app_ret = APP_CAMERA_OK;

    mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
    mpi_venc_stop_chn(ctx->venc_chn_photo);
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
    mpi_vpss_disable_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large);
    mpi_vpss_destroy_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large);
    return app_ret;

err_unbind_vpss_venc:
    mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
err_stop_venc_chn:
    mpi_venc_stop_chn(ctx->venc_chn_photo);
err_destroy_venc_chn:
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp_photo, ctx->vpss_chn_photo_large);
    return APP_CAMERA_ERR_IO;
}

static app_camera_ret_t app_camera_take_small_photo(app_camera_handle_t ctx,
        const app_camera_photo_attr_t *attr)
{
    mpi_ret_t ret;
    app_camera_ret_t app_ret;
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_chn_t vpss_chn_src;
    mpi_chn_t venc_chn_dst;

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS photo channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    chn_attr.width = attr->width;
    chn_attr.height = attr->height;
    chn_attr.format = attr->yuv420 ? MPI_PIXEL_FMT_YUV420SP : APP_CAMERA_PHOTO_FORMAT;
    chn_attr.yuv_order = APP_CAMERA_PHOTO_YUV_ORDER;
    chn_attr.mirror = 0;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_ONLINE;
    chn_attr.buf_size = photo_calc_online_buf_size(attr->width, chn_attr.format);
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn_photo, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set VPSS photo channel attributes\n");
        return APP_CAMERA_ERR_IO;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS photo channel\n");
        goto err_destroy_vpss_chn;
    }

    if (ctx->digital_zoom_enabled) {
        mpi_vpss_crop_param_t crop_param = {0};
        crop_param.x_start = ctx->digital_zoom_param.x_start;
        crop_param.y_start = ctx->digital_zoom_param.y_start;
        crop_param.crop_w  = ctx->digital_zoom_param.crop_w;
        crop_param.crop_h  = ctx->digital_zoom_param.crop_h;
        ret = mpi_vpss_chn_set_crop(ctx->vpss_grp,
                                    ctx->vpss_chn_photo,
                                    &crop_param);
        if (ret != MPI_SUCCESS) {
            printf("failed to apply digital zoom to small photo: %d\n", ret);
            goto err_disable_vpss_chn;
        }
    }

    ctx->venc_dev_photo = APP_CAMERA_PHOTO_VENC_HW_DEV;
    ret = mpi_venc_find_free_chn(&ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VENC photo channel\n");
        goto err_disable_vpss_chn;
    }

    venc_attr.hw_dev = ctx->venc_dev_photo;
    venc_attr.type = MPI_VENC_TYPE_JPEG;
    venc_attr.input_mode = MPI_VENC_INPUT_ONLINE;
    venc_attr.quality = attr->quality ? attr->quality : APP_CAMERA_PHOTO_DEFAULT_QUALITY;
    venc_attr.in_width = attr->width;
    venc_attr.in_height = attr->height;
    venc_attr.in_format = chn_attr.format;
    venc_attr.in_yuv_order = chn_attr.yuv_order;
    venc_attr.stream_buf_size = attr->stream_buf_size ? attr->stream_buf_size : APP_CAMERA_PHOTO_STREAM_BUF_SIZE;
    venc_attr.photo_mode = attr->jpeg_buf ? 0 : 1;
    venc_attr.photo_path = attr->jpeg_buf ? NULL : attr->path;
    venc_attr.photo_file_cb = app_camera_photo_file_created;
    venc_attr.photo_file_cb_priv = ctx;
    ret = mpi_venc_create_chn(ctx->venc_chn_photo, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VENC photo channel\n");
        goto err_disable_vpss_chn;
    }

    ret = mpi_venc_start_chn(ctx->venc_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VENC photo channel\n");
        goto err_destroy_venc_chn;
    }

    vpss_chn_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_chn_src.dev_id = ctx->vpss_grp;
    vpss_chn_src.chn_id = ctx->vpss_chn_photo;
    venc_chn_dst.mod_id = MPI_MOD_ID_VENC;
    venc_chn_dst.dev_id = ctx->venc_dev_photo;
    venc_chn_dst.chn_id = ctx->venc_chn_photo;

    ret = mpi_sys_bind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS and VENC photo channel\n");
        goto err_stop_venc_chn;
    }

    ret = app_camera_wait_photo_output(ctx, attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to wait VENC photo done\n");
        goto err_unbind_vpss_venc;
    }

    app_ret = APP_CAMERA_OK;

    mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
    mpi_venc_stop_chn(ctx->venc_chn_photo);
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    return app_ret;

err_unbind_vpss_venc:
    mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
err_stop_venc_chn:
    mpi_venc_stop_chn(ctx->venc_chn_photo);
err_destroy_venc_chn:
    mpi_venc_destroy_chn(ctx->venc_chn_photo);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    return APP_CAMERA_ERR_IO;
}

static app_camera_ret_t app_camera_take_large_photo(app_camera_handle_t ctx,
        const app_camera_photo_attr_t *attr)
{
    mpi_ret_t ret;
    app_camera_ret_t app_ret;
    mpi_vpss_grp_attr_t src_grp_attr = {0};
    mpi_vpss_chn_attr_t src_chn_attr = {0};
    mpi_vpss_grp_attr_t photo_grp_attr = {0};
    mpi_video_frame_info_t frame = {0};
    int display_stopped = 0;
#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    int jlgpu_locked = 0;
#endif

    ret = mpi_vpss_get_grp_attr(ctx->vpss_grp, &src_grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to get VPSS source group attributes\n");
        return APP_CAMERA_ERR_IO;
    }

    // 取帧需整帧离线缓冲(615KB), 先停显示否则取帧即P_MEM死机

    display_stopped = app_camera_photo_display_suspend(ctx);
    if (display_stopped < 0) {
        printf("failed to suspend display before large photo capture\n");
        return APP_CAMERA_ERR_IO;
    }
    if (display_stopped) {
        printf("display suspended before large photo, IMC SRC0 UB released\n");
    }

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find source photo channel\n");
        app_camera_photo_display_resume(ctx);  /* 入口已挂起显示, 失败必须恢复 */
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    src_chn_attr.width = attr->jpeg_buf ? attr->width : src_grp_attr.src_width;
    src_chn_attr.height = attr->jpeg_buf ? attr->height : src_grp_attr.src_height;
    src_chn_attr.format = attr->yuv420 ? MPI_PIXEL_FMT_YUV420SP : APP_CAMERA_PHOTO_FORMAT;
    src_chn_attr.yuv_order = APP_CAMERA_PHOTO_YUV_ORDER;
    src_chn_attr.mirror = 0;
    src_chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    src_chn_attr.buf_addr = NULL;
    src_chn_attr.buf_size = photo_calc_frame_buf_size(src_chn_attr.width,
                            src_chn_attr.height,
                            src_chn_attr.format);
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn_photo, &src_chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set source photo channel attributes\n");
        app_camera_photo_display_resume(ctx);  /* 入口已挂起显示, 失败必须恢复 */
        return APP_CAMERA_ERR_IO;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable source photo channel\n");
        goto err_destroy_src_chn;
    }

    /* Crop the transient offline photo frame before GPU effects; replay only resizes. */
    if (!attr->jpeg_buf && ctx->digital_zoom_enabled) {
        mpi_vpss_crop_param_t crop_param = {0};
        crop_param.x_start = ctx->digital_zoom_param.x_start;
        crop_param.y_start = ctx->digital_zoom_param.y_start;
        crop_param.crop_w  = ctx->digital_zoom_param.crop_w;
        crop_param.crop_h  = ctx->digital_zoom_param.crop_h;
        ret = mpi_vpss_chn_set_crop(ctx->vpss_grp,
                                    ctx->vpss_chn_photo,
                                    &crop_param);
        if (ret != MPI_SUCCESS) {
            printf("failed to apply digital zoom before photo effect: %d\n", ret);
            goto err_disable_src_chn;
        }
    }
    ret = mpi_vpss_get_chn_frame(ctx->vpss_grp,
                                 ctx->vpss_chn_photo,
                                 &frame,
                                 APP_CAMERA_PHOTO_TIMEOUT_MS);
    if (ret != MPI_SUCCESS) {
        printf("failed to get source photo frame\n");
        goto err_disable_src_chn;
    }
    printf("got source photo frame, width=%u, height=%u, format=%d , yaddr=0x%p ysize=%u\n",
           frame.width, frame.height, frame.format, (void *)frame.y_addr, frame.y_len);
#ifdef CONFIG_LVGL_UI_ENABLE
    app_camera_photo_apply_effect(&frame, attr);
#endif

    ret = mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable source photo channel\n");
        goto err_release_disable_src_frame;
    }

    if (attr->jpeg_buf) {
        app_ret = app_camera_encode_offline_photo(ctx, attr, &frame);
        ret = mpi_vpss_release_chn_frame(ctx->vpss_grp, ctx->vpss_chn_photo, &frame);
        if (ret != MPI_SUCCESS) {
            printf("failed to release source photo frame after offline encode\n");
            app_ret = APP_CAMERA_ERR_IO;
        }
        if (mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_photo) != MPI_SUCCESS) {
            printf("failed to destroy source photo channel after offline encode\n");
            app_ret = APP_CAMERA_ERR_IO;
        }
        app_camera_photo_display_resume(ctx);  /* 入口已挂起显示(jpeg_buf快速路径也要恢复) */
        return app_ret;
    }

    /* 显示已在 take_large_photo 入口暂停(display_stopped==1), 此处不再二次挂起 */

#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    /* 编码期间持有 JLGPU 互斥锁: LVGL C3 加载/重初始化会被阻塞,
     * 避免在 VENC 使用 EVA_JPG0 时被 lv_c3_hw_check_and_reinit() 复位打断 */
    lv_jlgpu_mutex_pend();
    jlgpu_locked = 1;
#endif

    ret = mpi_vpss_find_free_grp(&ctx->vpss_grp_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS photo group\n");
        goto err_release_src_frame;
    }

    photo_grp_attr.src_width = frame.width;
    photo_grp_attr.src_height = frame.height;
    photo_grp_attr.src_format = MPI_VPSS_SRC_FMT_YUV;
    photo_grp_attr.src_pixel_format = frame.format;
    photo_grp_attr.src_yuv_order = frame.yuv_order;
    photo_grp_attr.replay_isp_mode = 0;
    ret = mpi_vpss_create_grp(ctx->vpss_grp_photo, &photo_grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VPSS photo group\n");
        goto err_release_src_frame;
    }

    ret = mpi_vpss_start_grp(ctx->vpss_grp_photo);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VPSS photo group\n");
        goto err_destroy_photo_grp;
    }

    if (attr->thumbnail_enable) {
        uint32_t thumb_len = 0;
        uint32_t app1_len = 0;

        /* 1. 编缩略图: thumbnail_create=1, 拿流到 photo_thumb_buf */
        app_ret = app_camera_replay_encode_once(ctx, &frame,
                                                APP_CAMERA_PHOTO_THUMB_WIDTH, APP_CAMERA_PHOTO_THUMB_HEIGHT,
                                                1, NULL, 0, attr,
                                                ctx->photo_thumb_buf, APP_CAMERA_PHOTO_THUMB_BUF_SIZE, &thumb_len);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to encode thumbnail\n");
            goto err_stop_photo_grp;
        }

        /* 2. 生成 APP1 (含缩略图), IFD0 为空 */
        app1_len = mjpg_generate_app1(ctx->photo_app1_buf, NULL, 0,
                                      ctx->photo_thumb_buf, thumb_len);

        /* 3. 编主图: thumbnail_create=0, 带 head_app1, 写文件 */
        app_ret = app_camera_replay_encode_once(ctx, &frame,
                                                attr->width, attr->height,
                                                0, ctx->photo_app1_buf, app1_len, attr,
                                                NULL, 0, NULL);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to encode main photo\n");
            goto err_stop_photo_grp;
        }
    } else {
        /* 无缩略图: 单次编主图写文件 */
        app_ret = app_camera_replay_encode_once(ctx, &frame,
                                                attr->width, attr->height,
                                                0, NULL, 0, attr,
                                                NULL, 0, NULL);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to encode main photo\n");
            goto err_stop_photo_grp;
        }
    }

    mpi_vpss_stop_grp(ctx->vpss_grp_photo);
    mpi_vpss_destroy_grp(ctx->vpss_grp_photo);

    if (display_stopped) {
        int resume_ret = app_camera_photo_display_resume(ctx);
        if (resume_ret != 0) {
            printf("failed to resume display after large photo\n");
        } else {
            printf("display resumed after large photo\n");
        }
        display_stopped = 0;
    }

#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    /* 显示通路恢复后再释放 JLGPU 互斥锁, 避免编码期间排队的贴纸切换 RPC
     * 在显示未就绪时执行 GPU top 联动而卡死 */
    if (jlgpu_locked) {
        lv_jlgpu_mutex_post();
        jlgpu_locked = 0;
    }
#endif

    ret = mpi_vpss_release_chn_frame(ctx->vpss_grp, ctx->vpss_chn_photo, &frame);
    if (ret != MPI_SUCCESS) {
        printf("failed to release source photo frame after encode\n");
        app_ret = APP_CAMERA_ERR_IO;
    }
    if (mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_photo) != MPI_SUCCESS) {
        printf("failed to destroy source photo channel\n");
        app_ret = APP_CAMERA_ERR_IO;
    }
    return app_ret;

err_stop_photo_grp:
    mpi_vpss_stop_grp(ctx->vpss_grp_photo);
err_destroy_photo_grp:
    mpi_vpss_destroy_grp(ctx->vpss_grp_photo);
err_release_src_frame:
    // 错误路径: 先销毁取帧通道(held帧释放不掉)再恢复显示, 否则池不足恢复崩溃

    if (mpi_vpss_release_chn_frame(ctx->vpss_grp, ctx->vpss_chn_photo, &frame) != MPI_SUCCESS) {
        printf("failed to release source photo frame\n");
    }
#if ((defined CONFIG_LVGL_UI_ENABLE)&&CONFIG_LVGL_UI_ENABLE)
    if (jlgpu_locked) {
        lv_jlgpu_mutex_post();
        jlgpu_locked = 0;
    }
#endif
    goto err_destroy_src_chn;
err_release_disable_src_frame:
    if (mpi_vpss_release_chn_frame(ctx->vpss_grp, ctx->vpss_chn_photo, &frame) != MPI_SUCCESS) {
        printf("failed to release source photo frame\n");
    }
err_disable_src_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
err_destroy_src_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_photo);
    if (display_stopped) {
        int resume_ret = app_camera_photo_display_resume(ctx);
        if (resume_ret != 0) {
            printf("failed to resume display (error path)\n");
        }
        display_stopped = 0;
    }
    return APP_CAMERA_ERR_IO;
}

app_camera_ret_t app_camera_take_photo(app_camera_handle_t handle,
                                       const app_camera_photo_attr_t *attr)
{
    app_camera_ret_t app_ret;
    mpi_vpss_grp_attr_t grp_attr = {0};
    int once_open_src = 0;

    if (handle == NULL || attr == NULL) {
        printf("handle or photo attr is invalid\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if ((attr->jpeg_buf && (!attr->jpeg_buf_size || !attr->jpeg_size)) ||
        (!attr->jpeg_buf && attr->path[0] == '\0')) {
        printf("photo output target is invalid\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (attr->jpeg_size) {
        *attr->jpeg_size = 0;
    }
    if (attr->width == 0 || attr->height == 0) {
        printf("invalid photo size\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (handle->photo_busy) {
        printf("photo is busy\n");
        return APP_CAMERA_ERR_BUSY;
    }
    if (handle->vpss_venc_rec_enabled) {
        printf("record is started, cannot take photo\n");
        return APP_CAMERA_ERR_BUSY;
    }

    if (!app_camera_src_is_opened(handle)) {
        printf("camera source is not opened, open it first\n");
        app_ret = app_camera_src_open(handle);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to open camera source\n");
            return app_ret;
        }
        once_open_src = 1;
    }

    handle->photo_busy = 1;

    if (attr->thumbnail_enable) {
        handle->photo_thumb_buf = zalloc(APP_CAMERA_PHOTO_THUMB_BUF_SIZE);
        handle->photo_app1_buf = zalloc(APP_CAMERA_PHOTO_THUMB_BUF_SIZE + 128);
        if (!handle->photo_thumb_buf || !handle->photo_app1_buf) {
            printf("failed to alloc thumbnail/app1 buf\n");
            app_ret = APP_CAMERA_ERR_IO;
            goto out;
        }
    }

    if (mpi_vpss_get_grp_attr(handle->vpss_grp, &grp_attr) != MPI_SUCCESS) {
        printf("failed to get VPSS group attributes\n");
        app_ret = APP_CAMERA_ERR_IO;
        goto out;
    }

    if (!attr->jpeg_buf && !attr->thumbnail_enable && attr->effect == 0 && attr->width <= grp_attr.src_width && attr->height <= grp_attr.src_height) {
        app_ret = app_camera_take_small_photo(handle, attr);
    } else {
        app_ret = app_camera_take_large_photo(handle, attr);
    }

out:
    if (handle->photo_thumb_buf) {
        free(handle->photo_thumb_buf);
        handle->photo_thumb_buf = NULL;
    }
    if (handle->photo_app1_buf) {
        free(handle->photo_app1_buf);
        handle->photo_app1_buf = NULL;
    }
    handle->photo_busy = 0;

    if (once_open_src && app_camera_src_need_stop(handle)) {
        app_camera_src_close(handle);
    }

    return app_ret;
}

