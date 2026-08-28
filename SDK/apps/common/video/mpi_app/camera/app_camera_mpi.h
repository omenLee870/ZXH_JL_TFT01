#ifndef APP_CAMERA_MPI_H
#define APP_CAMERA_MPI_H

#include <stdint.h>

#define APP_CAMERA_REC_PATH_MAX 96

typedef void (*app_camera_video_file_open_cb_t)(void *args);
typedef void (*app_camera_photo_file_open_cb_t)(void *args);

typedef struct app_camera_ctx *app_camera_handle_t;

typedef enum {
    APP_CAMERA_OK = 0,
    APP_CAMERA_ERR_INVALID_ARG = -1,
    APP_CAMERA_ERR_NOMEM = -2,
    APP_CAMERA_ERR_BUSY = -3,
    APP_CAMERA_ERR_NOT_FOUND = -4,
    APP_CAMERA_ERR_NOT_READY = -5,
    APP_CAMERA_ERR_NOT_SUPPORT = -6,
    APP_CAMERA_ERR_EXIST = -7,
    APP_CAMERA_ERR_STATE = -8,
    APP_CAMERA_ERR_BIND = -9,
    APP_CAMERA_ERR_IO = -10,
} app_camera_ret_t;

typedef enum {
    APP_CAMERA_EVENT_REC_STARTED = 0,
    APP_CAMERA_EVENT_REC_TIME,
    APP_CAMERA_EVENT_REC_CYC_FILE,
    APP_CAMERA_EVENT_REC_STOPPED,
    APP_CAMERA_EVENT_REC_ERROR,
} app_camera_event_t;

typedef enum {
    APP_CAMERA_AUDIO_CODEC_PCM = 0,
    APP_CAMERA_AUDIO_CODEC_MP3 = 1,
} app_camera_audio_codec_t;

typedef void (*app_camera_event_handler_t)(void *priv,
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
} app_camera_display_attr_t;

typedef struct {
    char path[APP_CAMERA_REC_PATH_MAX];
    uint16_t width;
    uint16_t height;
    uint32_t fps;
    uint8_t quality;
    uint16_t abr_kbps;
    uint32_t stream_buf_size;
    uint32_t cyc_sec;
    uint8_t audio_enable;
    uint32_t audio_buf_size;
    uint32_t audio_frame_interval_ms;
    app_camera_audio_codec_t audio_codec_type; /* 0=PCM, 1=MP3 */
    uint32_t audio_bit_rate;     /* codec bitrate, e.g. 64000 */
    uint32_t audio_sample_rate;  /* 0 = default (8000) */
} app_camera_rec_attr_t;

typedef struct {
    char path[APP_CAMERA_REC_PATH_MAX];
    uint16_t width;
    uint16_t height;
    uint8_t quality;
    uint32_t stream_buf_size;
    uint8_t effect;
    const char *sticker_path;
    uint8_t yuv420;
    uint8_t *jpeg_buf;
    uint32_t jpeg_buf_size;
    uint32_t *jpeg_size;
    uint8_t thumbnail_enable;
} app_camera_photo_attr_t;

typedef struct {
    char path[APP_CAMERA_REC_PATH_MAX];  // "udp://192.168.1.100:1234"
    uint16_t width;
    uint16_t height;
    uint32_t fps;
    uint16_t abr_kbps;
    uint32_t stream_buf_size;
    uint8_t audio_enable;
} app_camera_net_stream_attr_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    const char *text;
} app_camera_osd_attr_t;

app_camera_ret_t app_camera_open(uint8_t camera_id,
                                 const char *sensor_name,
                                 app_camera_handle_t *handle);
app_camera_ret_t app_camera_close(app_camera_handle_t *handle);
app_camera_ret_t app_camera_display_start(app_camera_handle_t handle,
        const app_camera_display_attr_t *attr);
app_camera_ret_t app_camera_display_stop(app_camera_handle_t handle);
app_camera_ret_t app_camera_rec_start(app_camera_handle_t handle,
                                      const app_camera_rec_attr_t *attr);
app_camera_ret_t app_camera_rec_stop(app_camera_handle_t handle);
app_camera_ret_t app_camera_rec_set_path(app_camera_handle_t handle,
        const char *path);
app_camera_ret_t app_camera_rec_pause(app_camera_handle_t handle);
app_camera_ret_t app_camera_rec_resume(app_camera_handle_t handle);
app_camera_ret_t app_camera_take_photo(app_camera_handle_t handle,
                                       const app_camera_photo_attr_t *attr);
app_camera_ret_t app_camera_set_display_text_osd(app_camera_handle_t handle,
        const app_camera_osd_attr_t *attr);
app_camera_ret_t app_camera_enable_display_osd(app_camera_handle_t handle);
app_camera_ret_t app_camera_disable_display_osd(app_camera_handle_t handle);
app_camera_ret_t app_camera_set_record_text_osd(app_camera_handle_t handle,
        const app_camera_osd_attr_t *attr);
app_camera_ret_t app_camera_enable_record_osd(app_camera_handle_t handle);
app_camera_ret_t app_camera_disable_record_osd(app_camera_handle_t handle);
app_camera_ret_t app_camera_register_event_handler(
    app_camera_handle_t handle,
    void *priv,
    app_camera_event_handler_t handler);
app_camera_ret_t app_camera_net_stream_start(app_camera_handle_t handle,
        const app_camera_net_stream_attr_t *attr);
app_camera_ret_t app_camera_net_stream_stop(app_camera_handle_t handle);
int app_camera_net_stream_is_started(app_camera_handle_t handle);

/**
 * @brief Callback invoked just before the camera closes a created file.
 * @param fp       Open file handle (cast to FILE* in the callback) — all data
 *                 has been written, fclose is imminent.
 * @param dir_path Directory path (e.g. "DCIM/VIDEO/" or "DCIM/PHOTO/").
 * @param priv     Opaque caller-private pointer set during registration.
 */
typedef void (*app_camera_file_done_cb)(void *fp, const char *dir_path, void *priv);

/**
 * @brief Register a callback to be called when a media file is fully written.
 *        Replaces the old buffer-pointer approach (app_camera_set_last_*_file_buffer).
 * @param handle   Camera handle.
 * @param dir_path Directory path associated with this camera session.
 * @param cb       Callback function (may be NULL to unregister).
 * @param priv     Opaque pointer passed through to cb.
 */
void app_camera_set_file_done_callback(app_camera_handle_t handle,
                                       const char *dir_path,
                                       app_camera_file_done_cb cb,
                                       void *priv);
void app_camera_set_last_video_file_buffer(app_camera_handle_t handle, char *ptr);
void app_camera_set_last_photo_file_buffer(app_camera_handle_t handle, char *ptr);

typedef enum {
    APP_CAMERA_ZOOM_TYPE_DISPLAY = 0,  /* For VI->VPSS->VO path */
    APP_CAMERA_ZOOM_TYPE_ENCODE  = 1,  /* For VI->VPSS->VENC path */
} app_camera_zoom_type_t;

typedef struct {
    uint16_t x_start;    /* crop start x */
    uint16_t y_start;    /* crop start y */
    uint16_t crop_w;     /* crop width */
    uint16_t crop_h;     /* crop height */
} app_camera_zoom_param_t;

app_camera_ret_t app_camera_set_digital_zoom(app_camera_handle_t handle,
        const app_camera_zoom_param_t *param);
app_camera_ret_t app_camera_disable_digital_zoom(app_camera_handle_t handle);

#endif /* APP_CAMERA_MPI_H */


