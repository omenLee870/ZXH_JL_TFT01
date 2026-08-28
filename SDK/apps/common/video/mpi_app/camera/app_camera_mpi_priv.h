#ifndef APP_CAMERA_MPI_PRIV_H
#define APP_CAMERA_MPI_PRIV_H

#include "system/includes.h"
#include "app_camera_mpi.h"
#include "app_camera_osd.h"
#include "mpi_common.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "avilib.h"
#include "app_video_audio_rec.h"

#define APP_CAMERA_EVENT_TASK_NAME    "mpi_camera_event_task"
#define APP_CAMERA_EVENT_TASK_PRIO    10
#define APP_CAMERA_EVENT_TASK_STACK   1024
#define APP_CAMERA_EVENT_TASK_Q_SIZE  1024

struct app_camera_ctx {
    /* sensor */
    uint8_t camera_id;
    char sensor_name[10];

    /* pipeline */
    /* vi src */
    mpi_vi_dev_t vi_dev;
    mpi_vpss_grp_t vpss_grp;

    /* display */
    mpi_vpss_chn_t vpss_chn_display;
    mpi_vo_chn_t vo_chn;

    /* record */
    mpi_vpss_chn_t vpss_chn_rec;
    mpi_venc_dev_t venc_dev_rec;
    mpi_venc_chn_t venc_chn_rec;
    avi_t *avi;
    app_camera_rec_attr_t rec_attr;
    int rec_frame_count;
    int rec_dup_frame_count;
    char rec_thread_name[16];
    int rec_thread_pid;
    char rec_capture_thread_name[16];
    int rec_capture_thread_pid;
    int rec_thread_exit_flag;
    uint8_t rec_paused;     /* 录像暂停: 保留 AVI/编码链路, 丢弃暂停期帧, 恢复后同文件续写 */
    OS_SEM rec_path_sem;
    OS_MUTEX rec_stream_mutex;
    mpi_stream_t rec_latest_stream;
    uint8_t rec_stream_ready;
    uint8_t rec_stream_mutex_ready;
    uint8_t rec_wait_path;
    uint8_t rec_path_ready;
    char rec_next_path[APP_CAMERA_REC_PATH_MAX];
    uint32_t rec_prealloc_target;
    uint32_t rec_prealloc_next;
    uint32_t rec_prealloc_total_ms;
    uint32_t rec_prealloc_max_step_ms;
    uint32_t rec_prealloc_steps;
    app_video_audio_rec_t audio_rec;

    /* photo */
    mpi_vpss_chn_t vpss_chn_photo;
    mpi_vpss_grp_t vpss_grp_photo;
    mpi_vpss_chn_t vpss_chn_photo_large;
    mpi_venc_dev_t venc_dev_photo;
    mpi_venc_chn_t venc_chn_photo;
    uint8_t *photo_thumb_buf;
    uint8_t *photo_app1_buf;
    /* pipeline */

    /* status */
    uint8_t opened;
    uint8_t vi_vpss_src_enabled;
    uint8_t vpss_vo_enabled;
    uint8_t vpss_venc_rec_enabled;
    uint8_t vpss_venc_net_enabled;
    uint8_t photo_busy;

    /* net stream */
    mpi_vpss_chn_t vpss_chn_net;
    mpi_venc_dev_t venc_dev_net;
    mpi_venc_chn_t venc_chn_net;
    char net_thread_name[20];
    int net_thread_pid;
    int net_thread_exit_flag;
    void *net_stream_priv;   /* struct stream_info * */

    /* event */
    app_camera_event_handler_t event_handler;
    void *event_priv;
    const char *event_owner;

    /* osd */
    app_camera_text_osd_t display_osd;
    app_camera_text_osd_t record_osd;

    uint8_t digital_zoom_enabled;
    app_camera_zoom_param_t digital_zoom_param;

    app_camera_display_attr_t display_attr;
    app_camera_file_done_cb file_done_cb;
    void                   *file_done_priv;
    char                    file_dir_path[128];
    char                   *last_video_file;   /* 指向应用层 buffer, 记录刚创建的实际文件名 */
    char                   *last_photo_file;   /* 同上, 照片版 (通配符路径展开后的实际名) */
};

app_camera_ret_t app_camera_src_open(app_camera_handle_t ctx);
app_camera_ret_t app_camera_src_close(app_camera_handle_t ctx);
int app_camera_src_is_opened(app_camera_handle_t ctx);
int app_camera_src_need_stop(app_camera_handle_t ctx);
int app_camera_display_is_started(app_camera_handle_t ctx);
mpi_ret_t app_camera_display_pause(app_camera_handle_t ctx);
mpi_ret_t app_camera_display_resume(app_camera_handle_t ctx);
int app_camera_rec_is_started(app_camera_handle_t ctx);
int app_camera_net_stream_is_started(app_camera_handle_t ctx);
app_camera_ret_t app_camera_vpss_vo_bind_close(app_camera_handle_t ctx);
app_camera_ret_t app_camera_vpss_vo_bind_open(app_camera_handle_t ctx,
        const app_camera_display_attr_t *attr);
app_camera_ret_t app_camera_post_event(app_camera_handle_t handle,
                                       int argc,
                                       int *argv);

#endif /* APP_CAMERA_MPI_PRIV_H */


