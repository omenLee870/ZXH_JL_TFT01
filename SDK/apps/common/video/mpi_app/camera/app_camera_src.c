#include "app_camera_mpi_priv.h"

static app_camera_ret_t app_camera_vi_open(app_camera_handle_t ctx, uint8_t camera_id, const char *sensor_name)
{
    mpi_vi_dev_attr_t vi_attr;
    mpi_ret_t ret;

    ret = mpi_vi_find_free_dev(&ctx->vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VI device\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    memset(&vi_attr, 0, sizeof(vi_attr));
    vi_attr.camera_id = camera_id;
    vi_attr.sensor_name = (char *)sensor_name;

    ret = mpi_vi_set_dev_attr(ctx->vi_dev, &vi_attr);
    if (ret != MPI_SUCCESS) {
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    ret = mpi_vi_enable_dev(ctx->vi_dev);
    if (ret != MPI_SUCCESS) {
        return APP_CAMERA_ERR_NOT_FOUND;
    }
    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_vi_close(app_camera_handle_t ctx)
{
    mpi_ret_t ret;

    ret = mpi_vi_disable_dev(ctx->vi_dev);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable VI device\n");
        return APP_CAMERA_ERR_IO;
    }
    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_vi_vpss_bind_open(app_camera_handle_t ctx)
{
    mpi_vpss_grp_attr_t vpss_grp_attr;
    mpi_vi_output_info_t vi_output_info;
    mpi_ret_t ret;
    mpi_chn_t vi_src;
    mpi_chn_t vpss_grp_dst;

    ret = mpi_vpss_find_free_grp(&ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS group\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    memset(&vpss_grp_attr, 0, sizeof(vpss_grp_attr));
    ret = mpi_vi_get_output_info(ctx->vi_dev, &vi_output_info);
    if (ret != MPI_SUCCESS) {
        printf("failed to get VI output info\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    printf("VI output info: enabled=%d, is_raw=%d, width=%d, height=%d, fps=%d\n",
           vi_output_info.enabled, vi_output_info.is_raw, vi_output_info.width, vi_output_info.height, vi_output_info.fps);

    vpss_grp_attr.src_width = vi_output_info.width;
    vpss_grp_attr.src_height = vi_output_info.height;
    vpss_grp_attr.src_format = vi_output_info.is_raw ? MPI_VPSS_SRC_FMT_RAW : MPI_VPSS_SRC_FMT_YUV;
    vpss_grp_attr.src_fps = vi_output_info.fps;

    ret = mpi_vpss_create_grp(ctx->vpss_grp, &vpss_grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VPSS group\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }
    ret = mpi_vpss_start_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VPSS group\n");
        mpi_vpss_destroy_grp(ctx->vpss_grp);
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    vi_src.mod_id = MPI_MOD_ID_VI;
    vi_src.dev_id = ctx->vi_dev;
    vi_src.chn_id = 0;
    vpss_grp_dst.mod_id = MPI_MOD_ID_VPSS;
    vpss_grp_dst.dev_id = ctx->vpss_grp;
    vpss_grp_dst.chn_id = 0;

    ret = mpi_sys_bind(&vi_src, &vpss_grp_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VI and VPSS group\n");
        mpi_vpss_stop_grp(ctx->vpss_grp);
        mpi_vpss_destroy_grp(ctx->vpss_grp);
        return APP_CAMERA_ERR_BIND;
    }

    printf("VI and VPSS group bound successfully\n");

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_vi_vpss_bind_close(app_camera_handle_t ctx)
{
    mpi_ret_t ret;
    mpi_chn_t vi_src;
    mpi_chn_t vpss_grp_dst;

    vi_src.mod_id = MPI_MOD_ID_VI;
    vi_src.dev_id = ctx->vi_dev;
    vi_src.chn_id = 0;
    vpss_grp_dst.mod_id = MPI_MOD_ID_VPSS;
    vpss_grp_dst.dev_id = ctx->vpss_grp;
    vpss_grp_dst.chn_id = 0;

    ret = mpi_sys_unbind(&vi_src, &vpss_grp_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to unbind VI and VPSS group\n");
        return APP_CAMERA_ERR_IO;
    }

    ret = mpi_vpss_stop_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VPSS group\n");
        return APP_CAMERA_ERR_IO;
    }
    ret = mpi_vpss_destroy_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS group\n");
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_src_open(app_camera_handle_t ctx)
{
    app_camera_ret_t app_ret;

    if (ctx->vi_vpss_src_enabled) {
        printf("camera source is already enabled\n");
        return APP_CAMERA_ERR_STATE;
    }

    app_ret = app_camera_vi_open(ctx, ctx->camera_id, ctx->sensor_name);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to open VI device\n");
        return app_ret;
    }
    app_ret = app_camera_vi_vpss_bind_open(ctx);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to open VI to VPSS group binding\n");
        app_camera_vi_close(ctx);
        return app_ret;
    }
    ctx->vi_vpss_src_enabled = 1;

    printf("camera source open successfully\n");

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_src_close(app_camera_handle_t ctx)
{
    app_camera_ret_t app_ret;

    if (!ctx->vi_vpss_src_enabled) {
        printf("camera source is not enabled\n");
        return APP_CAMERA_ERR_STATE;
    }

    app_ret = app_camera_vi_vpss_bind_close(ctx);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to close VI to VPSS group binding\n");
        return app_ret;
    }

    app_ret = app_camera_vi_close(ctx);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to close VI device\n");
        return app_ret;
    }

    ctx->vi_vpss_src_enabled = 0;

    printf("camera source close successfully\n");

    return APP_CAMERA_OK;
}

int app_camera_src_is_opened(app_camera_handle_t ctx)
{
    return ctx->vi_vpss_src_enabled;
}

int app_camera_src_need_stop(app_camera_handle_t ctx)
{
    return ctx->vi_vpss_src_enabled && !ctx->vpss_vo_enabled && !ctx->vpss_venc_rec_enabled && !ctx->vpss_venc_net_enabled;
}

