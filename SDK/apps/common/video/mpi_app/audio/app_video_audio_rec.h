#ifndef APP_VIDEO_AUDIO_REC_H
#define APP_VIDEO_AUDIO_REC_H

#include <stdint.h>

typedef struct app_video_audio_rec *app_video_audio_rec_t;

typedef struct {
    uint32_t buffer_size;
    uint32_t frame_interval_ms;
    uint32_t sample_rate;       /* 0 = default (8000) */
    uint32_t codec_type;        /* 0 = PCM, use AUDIO_CODING_MP3 etc. */
    uint32_t bit_rate;          /* MP3/OPUS bitrate, 0 = default */
} app_video_audio_rec_attr_t;

typedef struct {
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
} app_video_audio_rec_format_t;

typedef struct {
    uint32_t len;
    uint32_t timestamp_ms;
    uint8_t data[0];
} app_video_audio_rec_frame_t;

int app_video_audio_rec_open(app_video_audio_rec_t *handle,
                             const app_video_audio_rec_attr_t *attr);
int app_video_audio_rec_start(app_video_audio_rec_t handle);
void app_video_audio_rec_stop(app_video_audio_rec_t handle);
void app_video_audio_rec_close(app_video_audio_rec_t *handle);
int app_video_audio_rec_get_frame(app_video_audio_rec_t handle,
                                  app_video_audio_rec_frame_t **frame);
void app_video_audio_rec_release_frame(app_video_audio_rec_frame_t *frame);
int app_video_audio_rec_get_format(app_video_audio_rec_t handle,
                                   app_video_audio_rec_format_t *format);

#endif /* APP_VIDEO_AUDIO_REC_H */
