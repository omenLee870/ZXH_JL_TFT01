#include "app_camera_mpi_priv.h"
#include "fb.h"

app_camera_ret_t app_camera_vpss_vo_bind_open(app_camera_handle_t ctx, const app_camera_display_attr_t *attr)
{
    mpi_ret_t ret;
    app_camera_ret_t app_ret;
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_vo_chn_attr_t vo_attr = {0};
    /* YUV420SP 高度需偶数(2对齐)*/
    uint16_t disp_h = (attr->height + 1) & ~1u;

    if (ctx->vpss_vo_enabled) {
        printf("display already started\n");
        return APP_CAMERA_ERR_EXIST;
    }

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn_display);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }
    chn_attr.width = attr->width;
    chn_attr.height = disp_h;
    chn_attr.format = MPI_PIXEL_FMT_YUV420SP;
    chn_attr.mirror = 0;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
#if defined CONFIG_CPU_BR33
    chn_attr.hw_id = ctx->display_osd.enable ? MPI_VPSS_HW_IMC_DST0 : MPI_VPSS_HW_IMC_DST1;
#else
    chn_attr.hw_id = MPI_VPSS_HW_AUTO;
#endif
    chn_attr.buf_addr = NULL;
    chn_attr.buf_size = 0;
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn_display, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set VPSS channel attributes\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    app_ret = app_camera_osd_apply(ctx, &ctx->display_osd, ctx->vpss_chn_display);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to apply display OSD, keep display running ret=%d\n", app_ret);
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_display);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS channel\n");
        goto err_disable_osd;
    }

    ret = mpi_vo_find_free_chn(&ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VO channel\n");
        goto err_disable_osd;
    }
    vo_attr.fb_num = attr->fb_num;
    strncpy(vo_attr.fb_name, attr->fb_name, sizeof(vo_attr.fb_name) - 1);
    vo_attr.fb_width = attr->width;
    vo_attr.fb_height = disp_h;
    vo_attr.window_width = attr->width;
    vo_attr.window_height = disp_h;
    vo_attr.left = attr->left;
    vo_attr.top = attr->top;
    vo_attr.fb_format = FB_COLOR_FORMAT_YUV420P;
    vo_attr.rotate = attr->rotate;
    vo_attr.mirror = attr->mirror;
    vo_attr.combine = 1;
    ret = mpi_vo_create_chn(ctx->vo_chn, &vo_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VO channel\n");
        goto err_disable_osd;
    }
    ret = mpi_vo_start_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VO channel\n");
        goto err_destroy_vo_chn;
    }

    mpi_chn_t vpss_chn_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_display};
    mpi_chn_t vo_chn_dst = {.mod_id = MPI_MOD_ID_VO, .dev_id = 0, .chn_id = ctx->vo_chn};

    ret = mpi_sys_bind(&vpss_chn_src, &vo_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS channel and VO channel\n");
        goto err_stop_vo_chn;
    }
    ctx->vpss_vo_enabled = 1;

    printf("VPSS channel and VO channel bound successfully\n");

    return APP_CAMERA_OK;

err_stop_vo_chn:
    mpi_vo_stop_chn(ctx->vo_chn);
err_destroy_vo_chn:
    mpi_vo_destroy_chn(ctx->vo_chn);
err_disable_osd:
    app_camera_osd_disable(ctx, &ctx->display_osd, ctx->vpss_chn_display);
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_display);
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_display);
    return APP_CAMERA_ERR_NOT_FOUND;
}

app_camera_ret_t app_camera_vpss_vo_bind_close(app_camera_handle_t ctx)
{
    mpi_ret_t ret;
    if (!ctx->vpss_vo_enabled) {
        printf("display not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    mpi_chn_t vpss_chn_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_display};
    mpi_chn_t vo_chn_dst = {.mod_id = MPI_MOD_ID_VO, .dev_id = 0, .chn_id = ctx->vo_chn};

    app_camera_osd_disable(ctx, &ctx->display_osd, ctx->vpss_chn_display);

    /* unbind vpss and vo */
    ret = mpi_sys_unbind(&vpss_chn_src, &vo_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to unbind VPSS and VO channels\n");
        return APP_CAMERA_ERR_IO;
    }

    /* 先停 vpss: 上游还在往 fb 里写, 停了才能释放缓冲 */
    ret = mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_display);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable VPSS channel\n");
        return APP_CAMERA_ERR_IO;
    }
    ret = mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_display);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS channel\n");
        return APP_CAMERA_ERR_IO;
    }

    /* 再关 vo chn, 释放 fb 缓冲 */
    ret = mpi_vo_stop_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VO channel\n");
        return APP_CAMERA_ERR_IO;
    }
    ret = mpi_vo_destroy_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VO channel\n");
        return APP_CAMERA_ERR_IO;
    }

    ctx->vpss_vo_enabled = 0;

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_display_start(app_camera_handle_t handle,
        const app_camera_display_attr_t *attr)
{
    app_camera_ret_t app_ret = APP_CAMERA_OK;
    int once_open_src = 0;
    if (handle == NULL || attr == NULL) {
        printf("handle or attr is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    /* check src */
    if (!app_camera_src_is_opened(handle)) {
        printf("camera source is not opened, open it first\n");
        once_open_src = 1;
        app_ret = app_camera_src_open(handle);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to open camera source\n");
            return app_ret;
        }
    }

    memcpy(&handle->display_attr, attr, sizeof(handle->display_attr));

    /* open vo chn */
    app_ret = app_camera_vpss_vo_bind_open(handle, attr);
    if (app_ret != APP_CAMERA_OK) {
        if (once_open_src) {
            printf("failed to start display, close the camera source opened in this function\n");
            app_camera_src_close(handle);
        }
        printf("failed to start display\n");
        return app_ret;
    }
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_display_stop(app_camera_handle_t handle)
{
    app_camera_ret_t app_ret = APP_CAMERA_OK;
    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    if (!handle->vpss_vo_enabled) {
        printf("display not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    app_ret = app_camera_vpss_vo_bind_close(handle);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to stop display\n");
        return app_ret;
    }

    if (app_camera_src_need_stop(handle)) {
        printf("camera source is no longer needed, stopping it\n");
        app_ret = app_camera_src_close(handle);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to close camera source\n");
            return app_ret;
        }
    }

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_set_display_text_osd(app_camera_handle_t handle, const app_camera_osd_attr_t *attr)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_set_text(&handle->display_osd, attr);
}

app_camera_ret_t app_camera_enable_display_osd(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_enable(handle,
                                 &handle->display_osd,
                                 handle->vpss_chn_display,
                                 handle->vpss_vo_enabled);
}

app_camera_ret_t app_camera_disable_display_osd(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_disable(handle,
                                  &handle->display_osd,
                                  handle->vpss_chn_display);
}



int app_camera_display_is_started(app_camera_handle_t handle)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return 0;
    }
    return handle->vpss_vo_enabled;
}

