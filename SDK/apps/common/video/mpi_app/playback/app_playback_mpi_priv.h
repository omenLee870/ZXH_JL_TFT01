#ifndef APP_PLAYBACK_MPI_PRIV_H
#define APP_PLAYBACK_MPI_PRIV_H

#include "system/includes.h"
#include "app_playback_mpi.h"
#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_vdec.h"
#include "mpi_vpss.h"
#include "mpi_vo.h"
#include "fb.h"
#include "lbuf.h"
#include "app_playback_demux.h"

#define APP_PLAYBACK_EVENT_TASK_NAME    "mpi_playback_event_task"
#define APP_PLAYBACK_EVENT_TASK_PRIO    10
#define APP_PLAYBACK_EVENT_TASK_STACK   1024
#define APP_PLAYBACK_EVENT_TASK_Q_SIZE  1024

typedef struct {
    uint16_t align_width;
    uint16_t align_height;
    mpi_pixel_format_t format;
} app_playback_image_info_t;

struct app_playback_ctx {
    /* display */
    app_playback_display_attr_t display_attr;
    char display_fb_name[10];

    /* pipeline */
    mpi_vdec_dev_t vdec_dev;
    mpi_vdec_chn_t vdec_chn;
    mpi_vpss_grp_t vpss_grp;
    mpi_vpss_chn_t vpss_chn;
    mpi_vo_chn_t vo_chn;

    /* video */
    void *video_lbuf_mem;
    struct lbuff_head *video_lbuf;
    uint32_t video_buffer_size;
    uint32_t audio_buffer_size;
    char video_thread_name[16];
    int video_thread_pid;
    int video_thread_exit_flag;
    app_playback_demux_ctx_t demux;

    /* audio playback: 独立avi句柄读PCM -> DAC, 与视频同步 */
    char audio_thread_name[16];
    int audio_thread_pid;
    volatile int audio_thread_exit_flag;
    uint8_t audio_started;
    char play_path[128];

    /* cur dec info */
    app_playback_image_info_t image_info;

    /* state */
    uint8_t opened;
    uint8_t pipeline_started;
    uint8_t video_started;

    /* event */
    app_playback_event_handler_t event_handler;
    void *event_priv;
    const char *event_owner;
};

app_playback_ret_t app_playback_image_decode_stream(app_playback_handle_t handle,
        const mpi_stream_t *stream);
app_playback_ret_t app_playback_image_pipeline_close(app_playback_handle_t ctx);
app_playback_ret_t app_playback_post_event(app_playback_handle_t handle,
        int argc,
        int *argv);

#endif /* APP_PLAYBACK_MPI_PRIV_H */

