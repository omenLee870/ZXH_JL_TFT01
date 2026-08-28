#include "app_camera_mpi_priv.h"
#include "app_config.h"


app_camera_ret_t app_camera_open(uint8_t camera_id,
                                 const char *sensor_name,
                                 app_camera_handle_t *handle)
{
    app_camera_handle_t ctx;

    if (handle == NULL || sensor_name == NULL) {
        printf("handle or sensor_name is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    ctx = (app_camera_handle_t)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        printf("failed to allocate context\n");
        return APP_CAMERA_ERR_NOMEM;
    }
    memset(ctx, 0, sizeof(*ctx));

    ctx->camera_id = camera_id;
    strncpy(ctx->sensor_name, sensor_name, sizeof(ctx->sensor_name) - 1);

    ctx->opened = 1;

    *handle = ctx;

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_close(app_camera_handle_t *handle)
{
    app_camera_handle_t ctx;
    app_camera_ret_t app_ret = APP_CAMERA_OK;

    if (handle == NULL || *handle == NULL) {
        printf("handle is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    ctx = *handle;

    if (app_camera_rec_is_started(ctx)) {
        printf("record is still started, stop it first\n");
        app_ret = app_camera_rec_stop(ctx);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to stop record\n");
            return app_ret;
        }
    }
#if TCFG_BT_NET_ENABLE || TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE || TCFG_LTE_PHY_ENABLE || TCFG_RTL8733BS_ENABLE
    if (app_camera_net_stream_is_started(ctx)) {
        printf("net stream is still started, stop it first\n");
        app_ret = app_camera_net_stream_stop(ctx);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to stop net stream\n");
            return app_ret;
        }
    }
#endif /* 无网络时不存在网络流 */
    if (app_camera_display_is_started(ctx)) {
        printf("display is still started, stop it first\n");
        app_ret = app_camera_display_stop(ctx);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to stop display\n");
            return app_ret;
        }
    }
    if (app_camera_src_is_opened(ctx)) {
        printf("camera source is still opened, close it first\n");
        app_ret = app_camera_src_close(ctx);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to close camera source\n");
            return app_ret;
        }
    }

    free(ctx);

    *handle = NULL;

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_set_digital_zoom(app_camera_handle_t handle,
        const app_camera_zoom_param_t *param)
{
    mpi_vpss_crop_param_t crop_param;
    mpi_ret_t ret;

    if (handle == NULL || param == NULL) {
        printf("invalid argument\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    memset(&crop_param, 0, sizeof(crop_param));
    crop_param.x_start = param->x_start;
    crop_param.y_start = param->y_start;
    crop_param.crop_w  = param->crop_w;
    crop_param.crop_h  = param->crop_h;

    /* Always apply zoom to display path immediately for preview */
    ret = mpi_vpss_chn_set_crop(handle->vpss_grp,
                                handle->vpss_chn_display,
                                &crop_param);
    if (ret != MPI_SUCCESS) {
        printf("failed to set display crop: %d\n", ret);
        return APP_CAMERA_ERR_STATE;
    }

    /* Store zoom param for later use by record/photo paths */
    memcpy(&handle->digital_zoom_param, param, sizeof(*param));
    handle->digital_zoom_enabled = 1;

    printf("digital zoom set: crop(%d,%d,%d,%d)\n",
           param->x_start, param->y_start,
           param->crop_w, param->crop_h);

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_disable_digital_zoom(app_camera_handle_t handle)
{
    mpi_ret_t ret;

    if (handle == NULL) {
        printf("invalid argument\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    handle->digital_zoom_enabled = 0;

    /* Always restore display to full view */
    ret = mpi_vpss_chn_disable_crop(handle->vpss_grp,
                                    handle->vpss_chn_display);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable display crop: %d\n", ret);
        return APP_CAMERA_ERR_STATE;
    }

    /* If recording is active, restore encode path too */
    if (app_camera_rec_is_started(handle)) {
        ret = mpi_vpss_chn_disable_crop(handle->vpss_grp,
                                        handle->vpss_chn_rec);
        if (ret != MPI_SUCCESS) {
            printf("failed to disable encode crop: %d\n", ret);
            return APP_CAMERA_ERR_STATE;
        }
    }

    printf("digital zoom disabled\n");

    return APP_CAMERA_OK;
}

void app_camera_set_file_done_callback(app_camera_handle_t handle,
                                       const char *dir_path,
                                       app_camera_file_done_cb cb,
                                       void *priv)
{
    if (!handle) {
        return;
    }
    handle->file_done_cb   = cb;
    handle->file_done_priv = priv;
    if (dir_path) {
        strncpy(handle->file_dir_path, dir_path, sizeof(handle->file_dir_path) - 1);
        handle->file_dir_path[sizeof(handle->file_dir_path) - 1] = '\0';
    } else {
        handle->file_dir_path[0] = '\0';
    }
}

void app_camera_set_last_video_file_buffer(app_camera_handle_t handle, char *ptr)
{
    if (handle) {
        handle->last_video_file = ptr;
    }
}

void app_camera_set_last_photo_file_buffer(app_camera_handle_t handle, char *ptr)
{
    if (handle) {
        handle->last_photo_file = ptr;
    }
}



