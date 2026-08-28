#ifndef APP_PLAYBACK_DEMUX_H
#define APP_PLAYBACK_DEMUX_H

#include <stdint.h>

#include "system/includes.h"
#include "app_playback_mpi.h"
#include "avilib.h"

struct lbuff_head;

typedef struct {
    uint32_t len;
    uint32_t pts;
    u8 data[0];
} app_playback_frame_hdr_t;

typedef struct {
    long total_frames;
    double fps;
    uint8_t has_audio;
    uint8_t audio_channels;
    uint8_t audio_bits;
    long audio_rate;
    long audio_format;   /* AVI wFormatTag, e.g. 0x0001=PCM, 0x0055=MP3 */
    long audio_chunks;
    long audio_bytes;
} app_playback_demux_info_t;

typedef struct {
    avi_t *avi;
    app_playback_demux_info_t info;
    uint32_t video_frame_index;
    char thread_name[16];
    int thread_pid;
    int thread_exit_flag;
    int thread_finished;
    OS_SEM frame_sem;
    OS_SEM space_sem;
} app_playback_demux_ctx_t;

struct app_playback_ctx;

app_playback_ret_t app_playback_demux_open(struct app_playback_ctx *ctx,
        const char *path);
void app_playback_demux_close(struct app_playback_ctx *ctx);
app_playback_ret_t app_playback_demux_start(struct app_playback_ctx *ctx);
void app_playback_demux_stop(struct app_playback_ctx *ctx);
app_playback_ret_t app_playback_demux_frame_queue_open(struct app_playback_ctx *ctx,
        uint32_t size);
void app_playback_demux_frame_queue_close(struct app_playback_ctx *ctx);
app_playback_frame_hdr_t *app_playback_demux_frame_pop(struct app_playback_ctx *ctx);
void app_playback_demux_frame_wait(struct app_playback_ctx *ctx);
void app_playback_demux_frame_wakeup(struct app_playback_ctx *ctx);
void app_playback_demux_frame_free(struct app_playback_ctx *ctx,
                                   app_playback_frame_hdr_t *frame);

#endif /* APP_PLAYBACK_DEMUX_H */

