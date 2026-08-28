#ifndef APP_PLAYBACK_MPI_H
#define APP_PLAYBACK_MPI_H

#include <stdint.h>

typedef struct app_playback_ctx *app_playback_handle_t;

typedef enum {
    APP_PLAYBACK_OK = 0,
    APP_PLAYBACK_ERR_INVALID_ARG = -1,
    APP_PLAYBACK_ERR_NOMEM = -2,
    APP_PLAYBACK_ERR_BUSY = -3,
    APP_PLAYBACK_ERR_NOT_FOUND = -4,
    APP_PLAYBACK_ERR_NOT_READY = -5,
    APP_PLAYBACK_ERR_NOT_SUPPORT = -6,
    APP_PLAYBACK_ERR_EXIST = -7,
    APP_PLAYBACK_ERR_STATE = -8,
    APP_PLAYBACK_ERR_BIND = -9,
    APP_PLAYBACK_ERR_IO = -10,
} app_playback_ret_t;

typedef enum {
    APP_PLAYBACK_EVENT_OPENED = 0,
    APP_PLAYBACK_EVENT_STARTED,
    APP_PLAYBACK_EVENT_FIRST_FRAME,
    APP_PLAYBACK_EVENT_CUR_TIME,
    APP_PLAYBACK_EVENT_FINISHED,
    APP_PLAYBACK_EVENT_STOPPED,
    APP_PLAYBACK_EVENT_ERROR,
} app_playback_event_t;

typedef void (*app_playback_event_handler_t)(void *priv,
        int argc,
        int *argv);

typedef struct {
    const char *fb_name;
    uint8_t fb_num;
    uint16_t width;
    uint16_t height;
    uint16_t left;
    uint16_t top;
    uint16_t rotate;
    uint16_t mirror;
} app_playback_display_attr_t;

app_playback_ret_t app_playback_open(app_playback_handle_t *handle,
                                     const app_playback_display_attr_t *display);
app_playback_ret_t app_playback_close(app_playback_handle_t *handle);

app_playback_ret_t app_playback_one_image_path(app_playback_handle_t handle,
        const char *path);
app_playback_ret_t app_playback_one_image_data(app_playback_handle_t handle,
        const uint8_t *data,
        uint32_t len);

/* 读取 JPG 文件内嵌 EXIF 缩略图（320x240 JPEG）。
 * 成功返回 APP_PLAYBACK_OK，*buf 为 malloc 的缩略图 JPEG，*len 为长度；
 * 调用方使用完必须 free(*buf)。无缩略图返回 APP_PLAYBACK_ERR_NOT_FOUND。 */
app_playback_ret_t app_playback_read_embedded_thumb(const char *path,
        uint8_t **buf, uint32_t *len);

app_playback_ret_t app_playback_video_start(app_playback_handle_t handle,
        const char *path);
app_playback_ret_t app_playback_video_stop(app_playback_handle_t handle);
app_playback_ret_t app_playback_video_finish(app_playback_handle_t handle);
app_playback_ret_t app_playback_set_video_buffer_size(app_playback_handle_t handle,
        uint32_t size);
app_playback_ret_t app_playback_set_audio_buffer_size(app_playback_handle_t handle,
        uint32_t size);
app_playback_ret_t app_playback_register_event_handler(
    app_playback_handle_t handle,
    void *priv,
    app_playback_event_handler_t handler);

#endif /* APP_PLAYBACK_MPI_H */

