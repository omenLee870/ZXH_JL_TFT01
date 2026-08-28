#include "app_playback_mpi_priv.h"

#define APP_PLAYBACK_DEFAULT_FB_NAME       "fb5"
#define APP_PLAYBACK_DEFAULT_FB_NUM        2

app_playback_ret_t app_playback_open(app_playback_handle_t *handle,
                                     const app_playback_display_attr_t *display)
{
    app_playback_handle_t ctx;
    const char *fb_name;
    size_t fb_name_len;

    if (display == NULL || handle == NULL || display->width == 0 || display->height == 0) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    fb_name = display->fb_name;
    if (fb_name == NULL || fb_name[0] == '\0') {
        fb_name = APP_PLAYBACK_DEFAULT_FB_NAME;
    }
    fb_name_len = strlen(fb_name);
    if (fb_name_len == 0 || fb_name_len >= sizeof(ctx->display_fb_name)) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    ctx = (app_playback_handle_t)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        return APP_PLAYBACK_ERR_NOMEM;
    }
    memset(ctx, 0, sizeof(*ctx));

    memcpy(ctx->display_fb_name, fb_name, fb_name_len + 1u);
    ctx->display_attr.fb_name = ctx->display_fb_name;
    ctx->display_attr.fb_num = display->fb_num ? display->fb_num : APP_PLAYBACK_DEFAULT_FB_NUM;
    ctx->display_attr.width = display->width;
    ctx->display_attr.height = display->height;
    ctx->display_attr.left = display->left;
    ctx->display_attr.top = display->top;
    ctx->display_attr.rotate = display->rotate;
    ctx->display_attr.mirror = display->mirror;

    ctx->opened = 1;
    *handle = ctx;

    return APP_PLAYBACK_OK;
}

app_playback_ret_t app_playback_close(app_playback_handle_t *handle)
{
    app_playback_ret_t app_ret = APP_PLAYBACK_OK;

    if (handle == NULL || *handle == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    if ((*handle)->video_started) {
        app_ret = app_playback_video_stop(*handle);
    } else if ((*handle)->pipeline_started) {
        app_ret = app_playback_image_pipeline_close(*handle);
    }

    free(*handle);
    *handle = NULL;

    return app_ret;
}
