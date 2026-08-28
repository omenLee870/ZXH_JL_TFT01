#include "app_camera_mpi_priv.h"
#include "media/audio_def.h"
#include "fs/fs.h"


#define APP_CAMERA_REC_FORMAT              MPI_PIXEL_FMT_YUV422_PACKED
#define APP_CAMERA_REC_YUV_ORDER           MPI_YUV_ORDER_YUYV
#define APP_CAMERA_REC_LINE_NUM            16u
#define APP_CAMERA_REC_DEFAULT_QUALITY     40u
#define APP_CAMERA_REC_VENC_ABR_KBPS       10000u
#define APP_CAMERA_REC_FRAME_RATE          30u
#define APP_CAMERA_REC_STREAM_BUF_SIZE     (512u * 1024u)
#define APP_CAMERA_REC_WORKER_PRIO         10
#define APP_CAMERA_REC_WORKER_STACK        2048
#define APP_CAMERA_REC_CAPTURE_WORKER_STACK 1024
#define APP_CAMERA_REC_VENC_HW_DEV         0
#define APP_CAMERA_REC_MAX_DUP_FRAMES      3u

#ifndef APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
#define APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE 1
#endif

#ifndef APP_CAMERA_REC_FILE_PREALLOC_ENABLE
#define APP_CAMERA_REC_FILE_PREALLOC_ENABLE 0
#endif

#define APP_CAMERA_REC_PREALLOC_JPEG_DIVISOR 12u
#define APP_CAMERA_REC_PREALLOC_EXTRA_SIZE   (2u * 1024u * 1024u)
#define APP_CAMERA_REC_PREALLOC_ALIGN_SIZE   (1024u * 1024u)
#define APP_CAMERA_REC_PREALLOC_STEP_SIZE    (1024u * 1024u)

extern uint32_t timer_get_ms(void);

static uint32_t calc_online_buf_size(uint16_t width, mpi_pixel_format_t format)
{
    if (format == MPI_PIXEL_FMT_YUV422_PACKED) {
        return (uint32_t)width * APP_CAMERA_REC_LINE_NUM * 2u;
    } else if (format == MPI_PIXEL_FMT_YUV420SP) {
        return (uint32_t)width * APP_CAMERA_REC_LINE_NUM * 3u / 2u;
    } else if (format == MPI_PIXEL_FMT_YUV444_PACKED) {
        return (uint32_t)width * APP_CAMERA_REC_LINE_NUM * 3u;
    } else {
        return (uint32_t)width * APP_CAMERA_REC_LINE_NUM;
    }
}

static app_camera_ret_t app_camera_rec_post_event(app_camera_handle_t handle,
        int event)
{
    int argv[1];

    argv[0] = event;
    return app_camera_post_event(handle, ARRAY_SIZE(argv), argv);
}

static app_camera_ret_t app_camera_rec_post_event_value(app_camera_handle_t handle,
        int event,
        int value)
{
    int argv[2];

    argv[0] = event;
    argv[1] = value;
    return app_camera_post_event(handle, ARRAY_SIZE(argv), argv);
}

static app_camera_ret_t app_camera_rec_audio_open(app_camera_handle_t ctx,
        const app_camera_rec_attr_t *attr)
{
    app_video_audio_rec_attr_t audio_attr = {0};
    int ret;

    if (!attr->audio_enable) {
        return APP_CAMERA_OK;
    }

    static const u32 codec_map[] = {
        [APP_CAMERA_AUDIO_CODEC_PCM] = AUDIO_CODING_PCM,
        [APP_CAMERA_AUDIO_CODEC_MP3] = AUDIO_CODING_MP3,
    };

    audio_attr.buffer_size = attr->audio_buf_size;
    audio_attr.frame_interval_ms = attr->audio_frame_interval_ms;
    audio_attr.codec_type = codec_map[attr->audio_codec_type];
    audio_attr.bit_rate = attr->audio_bit_rate ? attr->audio_bit_rate : 64000;
    audio_attr.sample_rate = attr->audio_sample_rate;
    ret = app_video_audio_rec_open(&ctx->audio_rec, &audio_attr);
    if (ret != 0) {
        printf("failed to open record audio, ret=%d\n", ret);
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_audio_start(app_camera_handle_t ctx)
{
    int ret;

    if (ctx->audio_rec == NULL) {
        return APP_CAMERA_OK;
    }

    ret = app_video_audio_rec_start(ctx->audio_rec);
    if (ret != 0) {
        printf("failed to start record audio, ret=%d\n", ret);
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

static void app_camera_rec_audio_stop(app_camera_handle_t ctx)
{
    if (ctx->audio_rec != NULL) {
        app_video_audio_rec_stop(ctx->audio_rec);
    }
}

static void app_camera_rec_audio_close(app_camera_handle_t ctx)
{
    if (ctx->audio_rec == NULL) {
        return;
    }

    app_video_audio_rec_close(&ctx->audio_rec);
}

static uint32_t app_camera_rec_prealloc_size(const app_camera_rec_attr_t *attr)
{
    uint64_t bytes;

    if (!APP_CAMERA_REC_FILE_PREALLOC_ENABLE || attr == NULL || attr->cyc_sec == 0) {
        return 0;
    }

    bytes = (uint64_t)attr->width * attr->height * attr->fps * attr->cyc_sec /
            APP_CAMERA_REC_PREALLOC_JPEG_DIVISOR;
    bytes += APP_CAMERA_REC_PREALLOC_EXTRA_SIZE;
    bytes = (bytes + APP_CAMERA_REC_PREALLOC_ALIGN_SIZE - 1u) &
            ~((uint64_t)APP_CAMERA_REC_PREALLOC_ALIGN_SIZE - 1u);
    if (bytes > INT32_MAX) {
        bytes = INT32_MAX;
    }

    return (uint32_t)bytes;
}

static void app_camera_rec_prealloc_reset(app_camera_handle_t ctx)
{
    ctx->rec_prealloc_target = 0;
    ctx->rec_prealloc_next = 0;
    ctx->rec_prealloc_total_ms = 0;
    ctx->rec_prealloc_max_step_ms = 0;
    ctx->rec_prealloc_steps = 0;
}

static void app_camera_rec_prealloc_start(app_camera_handle_t ctx)
{
    uint32_t target = app_camera_rec_prealloc_size(&ctx->rec_attr);

    app_camera_rec_prealloc_reset(ctx);
    if (target == 0) {
        return;
    }

    ctx->rec_prealloc_target = target;
    ctx->rec_prealloc_next = target < APP_CAMERA_REC_PREALLOC_STEP_SIZE ?
                             target : APP_CAMERA_REC_PREALLOC_STEP_SIZE;
}

static void app_camera_rec_prealloc_step(app_camera_handle_t ctx)
{
    uint32_t elapsed_ms;
    uint32_t next;
    uint32_t start_ms;

    if (ctx->avi == NULL || ctx->rec_prealloc_target == 0 ||
        ctx->rec_prealloc_next == 0) {
        return;
    }

    next = ctx->rec_prealloc_next;
    start_ms = timer_get_ms();
    if (AVI_preallocate(ctx->avi, next) != 0) {
        printf("[AVI_PREALLOC] fallback to normal allocation, bytes=%u\n", next);
        app_camera_rec_prealloc_reset(ctx);
        return;
    }

    elapsed_ms = timer_get_ms() - start_ms;
    ctx->rec_prealloc_total_ms += elapsed_ms;
    if (elapsed_ms > ctx->rec_prealloc_max_step_ms) {
        ctx->rec_prealloc_max_step_ms = elapsed_ms;
    }
    ctx->rec_prealloc_steps++;

    if (next >= ctx->rec_prealloc_target) {
        printf("[AVI_PREALLOC] complete bytes=%u steps=%u total_ms=%u max_step_ms=%u\n",
               ctx->rec_prealloc_target,
               ctx->rec_prealloc_steps,
               ctx->rec_prealloc_total_ms,
               ctx->rec_prealloc_max_step_ms);
        app_camera_rec_prealloc_reset(ctx);
        return;
    }

    if (ctx->rec_prealloc_target - next <= APP_CAMERA_REC_PREALLOC_STEP_SIZE) {
        ctx->rec_prealloc_next = ctx->rec_prealloc_target;
    } else {
        ctx->rec_prealloc_next = next + APP_CAMERA_REC_PREALLOC_STEP_SIZE;
    }
}

static void app_camera_rec_flush_wbuf(app_camera_handle_t ctx)
{
    const char *path = ctx->rec_attr.path;
    const char *volume_end;
    char volume_path[APP_CAMERA_REC_PATH_MAX];
    size_t volume_len;

    volume_end = strstr(path, "/C/");
    if (volume_end == NULL) {
        printf("failed to derive record volume for FAT flush: %s\n", path);
        return;
    }

    volume_len = (size_t)(volume_end - path) + 3u;
    if (volume_len >= sizeof(volume_path)) {
        printf("record volume path is too long for FAT flush: %s\n", path);
        return;
    }

    memcpy(volume_path, path, volume_len);
    volume_path[volume_len] = '\0';
    if (f_flush_wbuf(volume_path) != 0) {
        printf("failed to flush record volume: %s\n", volume_path);
    }
}

static app_camera_ret_t app_camera_rec_avi_open(app_camera_handle_t ctx)
{
    app_video_audio_rec_format_t audio_format;

    app_camera_rec_prealloc_reset(ctx);
    ctx->avi = AVI_open_output_file(ctx->rec_attr.path);
    if (ctx->avi == NULL) {
        printf("failed to open AVI file: %s\n", ctx->rec_attr.path);
        return APP_CAMERA_ERR_IO;
    }

    /* 文件名解析已移至 AVI_close 之前（回调中处理），不再在此处获取。 */

    /* 注: 曾试 is_opendml=1 根治 idx1 索引256KB崩溃, 但本SDK avilib 读侧对 opendml 索引
     * 支持不完整, 回放读不全帧(20s视频1.4s播完), 已回退。改由音频块大小修复兜底:
     * 音频 frame_interval 100ms/块=10块/s(原800B/40块/s), 索引增长为 video30+audio10=40/s,
     * 3分钟循环段约7200条索引(~113KB, 最大单次realloc 128KB), 远不到崩溃的12289条/256KB。 */

    AVI_set_video(ctx->avi,
                  ctx->rec_attr.width,
                  ctx->rec_attr.height,
                  (double)ctx->rec_attr.fps,
                  "MJPG");

    if (ctx->audio_rec != NULL) {
        if (app_video_audio_rec_get_format(ctx->audio_rec, &audio_format) != 0) {
            printf("failed to get record audio format\n");
            AVI_close(ctx->avi);
            ctx->avi = NULL;
            return APP_CAMERA_ERR_IO;
        }
        {
            int avi_fmt = (ctx->rec_attr.audio_codec_type == APP_CAMERA_AUDIO_CODEC_MP3) ? 0x0055 : WAVE_FORMAT_PCM;
            int mp3rate = (ctx->rec_attr.audio_codec_type == APP_CAMERA_AUDIO_CODEC_MP3)
                          ? (ctx->rec_attr.audio_bit_rate / 1000) : 0;
            AVI_set_audio(ctx->avi,
                          audio_format.channels,
                          audio_format.sample_rate,
                          audio_format.bits_per_sample,
                          avi_fmt,
                          mp3rate);
        }
    }

    app_camera_rec_prealloc_start(ctx);

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_audio_frame_write(app_camera_handle_t ctx,
        const app_video_audio_rec_frame_t *frame,
        uint32_t file_base_ms)
{
    if (frame == NULL || frame->len == 0) {
        return APP_CAMERA_OK;
    }

    if ((int32_t)(frame->timestamp_ms - file_base_ms) < 0) {
        return APP_CAMERA_OK;
    }

    if (AVI_write_audio(ctx->avi, (const char *)frame->data, (long)frame->len) != 0) {
        printf("failed to write AVI audio, len=%u\n", frame->len);
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return APP_CAMERA_ERR_IO;
    }
    // printf("wrote AVI audio frame, len=%u timestamp=%u\n",
    //        frame->len,
    //        frame->timestamp_ms);

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_count_video_frame(app_camera_handle_t ctx,
        uint32_t *last_rec_sec)
{
    uint32_t rec_ms;
    uint32_t rec_sec;

    ctx->rec_frame_count++;
    rec_ms = (uint32_t)(((uint64_t)ctx->rec_frame_count * 1000u) / ctx->rec_attr.fps);
    rec_sec = rec_ms / 1000u;
    if (rec_sec > *last_rec_sec) {
        *last_rec_sec = rec_sec;
        app_camera_rec_post_event_value(ctx, APP_CAMERA_EVENT_REC_TIME, (int)rec_ms);
    }

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_video_dup_write(app_camera_handle_t ctx,
        uint32_t *last_rec_sec)
{
    if (ctx->rec_frame_count <= 0) {
        return APP_CAMERA_OK;
    }

#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
    if (AVI_dup_frame(ctx->avi) != 0) {
#else
    if (AVI_dup_empty_frame(ctx->avi) != 0) {
#endif
        printf("failed to dup AVI video frame\n");
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return APP_CAMERA_ERR_IO;
    }
    ctx->rec_dup_frame_count++;
    //printf("wrote AVI dup video frame, total dup frames=%d\n", ctx->rec_dup_frame_count);

    return app_camera_rec_count_video_frame(ctx, last_rec_sec);
}

static app_camera_ret_t app_camera_rec_video_write(app_camera_handle_t ctx,
        const mpi_stream_t *stream,
        uint32_t *last_rec_sec,
        uint32_t file_base_ms)
{
#if 1
    static uint32_t diag_start_ms;
    static uint32_t diag_write_count;
    static uint32_t diag_total_ms;
    static uint32_t diag_max_ms;
    static uint32_t diag_over_budget_count;
    uint32_t write_start_ms;
    uint32_t write_end_ms;
    uint32_t write_cost_ms;
    uint32_t frame_budget_ms;
#endif
    int ret = 0;

#if 1
    printf("[REC_FRAME] seq=%u len=%u quality=%u abr_cfg=%u est_kbps=%u\n",
           stream->seq, stream->len, ctx->rec_attr.quality, ctx->rec_attr.abr_kbps,
           ctx->rec_attr.fps ?
           (uint32_t)(((uint64_t)stream->len * ctx->rec_attr.fps * 8u) / 1024u) : 0u);
    write_start_ms = timer_get_ms();
    if (diag_start_ms == 0 || (uint32_t)(write_start_ms - diag_start_ms) > 2000u) {
        diag_start_ms = write_start_ms;
        diag_write_count = 0;
        diag_total_ms = 0;
        diag_max_ms = 0;
        diag_over_budget_count = 0;
    }

#endif
    ret = AVI_write_frame(ctx->avi,
                          (const char *)stream->addr,
                          (long)stream->len,
                          1);
#if 1
    write_end_ms = timer_get_ms();
    write_cost_ms = (uint32_t)(write_end_ms - write_start_ms);
    frame_budget_ms = ctx->rec_attr.fps ? 1000u / ctx->rec_attr.fps : 0;
    diag_write_count++;
    diag_total_ms += write_cost_ms;
    if (write_cost_ms > diag_max_ms) {
        diag_max_ms = write_cost_ms;
    }
    if (frame_budget_ms != 0 && write_cost_ms > frame_budget_ms) {
        diag_over_budget_count++;
    }
    if ((uint32_t)(write_end_ms - diag_start_ms) >= 1000u) {
        printf("[REC_IO] AVI_write_frame writes=%u avg_ms=%u max_ms=%u "
               "over_budget=%u budget_ms=%u last_len=%u\n",
               diag_write_count, diag_total_ms / diag_write_count, diag_max_ms,
               diag_over_budget_count, frame_budget_ms, stream->len);
        diag_start_ms = write_end_ms;
        diag_write_count = 0;
        diag_total_ms = 0;
        diag_max_ms = 0;
        diag_over_budget_count = 0;
    }
#endif
    if (ret != 0) {
        printf("failed to write AVI frame, len=%u ret:%d err:%s  \n\n", stream->len, ret, AVI_strerror());
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return APP_CAMERA_ERR_IO;
    }
    app_camera_rec_count_video_frame(ctx, last_rec_sec);
    // printf("wrote AVI video frame, len=%u timestamp=%u \n",
    //        stream->len,
    //        stream->timestamp_ms);

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_wait_next_file(app_camera_handle_t ctx,
        uint32_t *last_rec_sec,
        uint32_t cyc_count)
{
    app_camera_ret_t app_ret;
    int close_ret;

    /* 分段文件由 CYC_FILE 事件延迟入库, 这里只记段名 (AVI_close 前取) */
    if (ctx->last_video_file) {
        FILE *fp = (FILE *)(long)ctx->avi->fdes;
        fget_name(fp, (u8 *)ctx->last_video_file, 128);
    }
    close_ret = AVI_close(ctx->avi);
    app_camera_rec_flush_wbuf(ctx);
    if (close_ret != 0) {
        printf("failed to close cycle AVI file\n");
        ctx->avi = NULL;
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return APP_CAMERA_ERR_IO;
    }
    printf("closed cycle AVI file, total frames=%d, dup frames=%d\n",
           ctx->rec_frame_count, ctx->rec_dup_frame_count);

    ctx->avi = NULL;

    ctx->rec_wait_path = 1;
    ctx->rec_path_ready = 0;
    ctx->rec_next_path[0] = '\0';
    os_sem_set(&ctx->rec_path_sem, 0);

    app_ret = app_camera_rec_post_event_value(ctx,
              APP_CAMERA_EVENT_REC_CYC_FILE,
              (int)cyc_count);
    if (app_ret != APP_CAMERA_OK) {
        ctx->rec_wait_path = 0;
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return app_ret;
    }

    while (!ctx->rec_thread_exit_flag && !ctx->rec_path_ready) {
        os_sem_pend(&ctx->rec_path_sem, 0);
    }

    if (ctx->rec_thread_exit_flag) {
        ctx->rec_wait_path = 0;
        return APP_CAMERA_OK;
    }

    if (ctx->rec_next_path[0] == '\0') {
        printf("cycle record path is empty\n");
        ctx->rec_wait_path = 0;
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    strncpy(ctx->rec_attr.path, ctx->rec_next_path, sizeof(ctx->rec_attr.path) - 1u);
    ctx->rec_attr.path[sizeof(ctx->rec_attr.path) - 1u] = '\0';
    ctx->rec_wait_path = 0;
    ctx->rec_path_ready = 0;
    ctx->rec_next_path[0] = '\0';
    ctx->rec_frame_count = 0;
    ctx->rec_dup_frame_count = 0;
    *last_rec_sec = 0;

    app_ret = app_camera_rec_avi_open(ctx);
    if (app_ret != APP_CAMERA_OK) {
        app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
        return app_ret;
    }

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_check_cycle(app_camera_handle_t ctx,
        uint32_t *last_rec_sec,
        uint32_t cyc_frame,
        uint32_t *cyc_count,
        uint32_t *file_base_ms,
        uint32_t *sync_base_ms,
        uint8_t *sync_started)
{
    app_camera_ret_t app_ret;

    if (cyc_frame == 0 || ctx->rec_frame_count < cyc_frame) {
        return APP_CAMERA_OK;
    }

    app_ret = app_camera_rec_wait_next_file(ctx,
                                            last_rec_sec,
                                            ++(*cyc_count));
    {
        uint32_t now_ms = timer_get_ms();

        *file_base_ms = now_ms;
        *sync_base_ms = now_ms;
        if (sync_started != NULL) {
            *sync_started = 0;
        }
    }

    return app_ret;
}

static app_camera_ret_t app_camera_rec_video_dup_to_clock(app_camera_handle_t ctx,
        uint32_t *last_rec_sec,
        uint32_t cyc_frame,
        uint32_t *cyc_count,
        uint32_t *file_base_ms,
        uint32_t *sync_base_ms,
        uint8_t *sync_started,
        uint8_t *did_dup)
{
    uint32_t now_ms;
    uint32_t elapsed_ms;
    uint32_t expected_frames;
    uint32_t dup_count;
    app_camera_ret_t app_ret;

    if (did_dup != NULL) {
        *did_dup = 0;
    }

    if (ctx->rec_frame_count <= 0) {
        return APP_CAMERA_OK;
    }

    now_ms = timer_get_ms();
    elapsed_ms = now_ms - *sync_base_ms;
    expected_frames = (uint32_t)(((uint64_t)elapsed_ms * ctx->rec_attr.fps) / 1000u);
    if (expected_frames <= (uint32_t)ctx->rec_frame_count) {
        return APP_CAMERA_OK;
    }

    dup_count = expected_frames - (uint32_t)ctx->rec_frame_count;
    if (dup_count > APP_CAMERA_REC_MAX_DUP_FRAMES) {
        dup_count = APP_CAMERA_REC_MAX_DUP_FRAMES;
    }

    while (dup_count-- > 0 && !ctx->rec_thread_exit_flag) {
        app_ret = app_camera_rec_video_dup_write(ctx, last_rec_sec);
        if (app_ret != APP_CAMERA_OK) {
            return app_ret;
        }
        if (did_dup != NULL) {
            *did_dup = 1;
        }

        app_ret = app_camera_rec_check_cycle(ctx,
                                             last_rec_sec,
                                             cyc_frame,
                                             cyc_count,
                                             file_base_ms,
                                             sync_base_ms,
                                             sync_started);
        if (app_ret != APP_CAMERA_OK || ctx->rec_thread_exit_flag) {
            return app_ret;
        }

        if (ctx->rec_frame_count <= 0) {
            break;
        }
    }

    return APP_CAMERA_OK;
}

static mpi_ret_t app_camera_rec_release_stream(app_camera_handle_t ctx,
        const mpi_stream_t *stream)
{
    mpi_ret_t ret;

    if (ctx->rec_stream_mutex_ready) {
        os_mutex_pend(&ctx->rec_stream_mutex, 0);
        ret = mpi_venc_release_stream(ctx->venc_chn_rec, stream);
        os_mutex_post(&ctx->rec_stream_mutex);
        return ret;
    }

    return mpi_venc_release_stream(ctx->venc_chn_rec, stream);
}

#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
static int app_camera_rec_stream_take(app_camera_handle_t ctx, mpi_stream_t *stream)
{
    int ready = 0;

    os_mutex_pend(&ctx->rec_stream_mutex, 0);
    if (ctx->rec_stream_ready) {
        *stream = ctx->rec_latest_stream;
        ctx->rec_stream_ready = 0;
        ready = 1;
    }
    os_mutex_post(&ctx->rec_stream_mutex);

    return ready;
}

static void app_camera_rec_capture_thread(void *parm)
{
    app_camera_handle_t ctx = (app_camera_handle_t)parm;

    while (!ctx->rec_thread_exit_flag) {
        mpi_stream_t stream = {0};
        mpi_stream_t stale_stream = {0};
        mpi_ret_t ret;
        int has_stale = 0;

        ret = mpi_venc_get_stream(ctx->venc_chn_rec, &stream, 10);
        if (ret == MPI_ERR_EMPTY || ret == MPI_ERR_NOT_READY) {
            continue;
        }
        if (ret != MPI_SUCCESS) {
            if (!ctx->rec_thread_exit_flag) {
                printf("failed to capture VENC stream, ret=%d\n", ret);
                ctx->rec_thread_exit_flag = 1;
                app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
            }
            break;
        }
        if (stream.addr == NULL || stream.len == 0) {
            if (app_camera_rec_release_stream(ctx, &stream) != MPI_SUCCESS) {
                printf("failed to release empty captured VENC stream\n");
                ctx->rec_thread_exit_flag = 1;
                app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
                break;
            }
            continue;
        }

        os_mutex_pend(&ctx->rec_stream_mutex, 0);
        if (ctx->rec_stream_ready) {
            stale_stream = ctx->rec_latest_stream;
            has_stale = 1;
        }
        ctx->rec_latest_stream = stream;
        ctx->rec_stream_ready = 1;
        os_mutex_post(&ctx->rec_stream_mutex);

        if (has_stale &&
            app_camera_rec_release_stream(ctx, &stale_stream) != MPI_SUCCESS) {
            printf("failed to release replaced VENC stream\n");
            ctx->rec_thread_exit_flag = 1;
            app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
            break;
        }
    }
}

static void app_camera_rec_capture_release_pending(app_camera_handle_t ctx)
{
    mpi_stream_t stream = {0};

    if (!ctx->rec_stream_mutex_ready ||
        !app_camera_rec_stream_take(ctx, &stream)) {
        return;
    }
    if (app_camera_rec_release_stream(ctx, &stream) != MPI_SUCCESS) {
        printf("failed to release pending VENC stream\n");
    }
}
#endif

static void app_camera_rec_thread(void *parm)
{
    app_camera_handle_t ctx = (app_camera_handle_t)parm;
    uint32_t cyc_frame = 0;
    uint32_t cyc_count = 0;
    uint32_t last_rec_sec = 0;
    uint32_t file_base_ms = timer_get_ms();
    uint32_t sync_base_ms = file_base_ms;
    uint8_t sync_started = 0;

    if (ctx->rec_attr.cyc_sec > 0) {
        cyc_frame = ctx->rec_attr.cyc_sec * ctx->rec_attr.fps;
    }

    while (!ctx->rec_thread_exit_flag) {
        mpi_stream_t video_stream = {0};
        app_video_audio_rec_frame_t *audio_frame = NULL;
        mpi_ret_t ret;
        int got_video = 0;
        int wrote = 0;

#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
        got_video = app_camera_rec_stream_take(ctx, &video_stream);
#else
        ret = mpi_venc_get_stream(ctx->venc_chn_rec,
                                  &video_stream,
                                  0);
        if (ret == MPI_SUCCESS) {
            if (video_stream.addr != NULL && video_stream.len > 0) {
                got_video = 1;
            } else if (app_camera_rec_release_stream(ctx, &video_stream) != MPI_SUCCESS) {
                printf("failed to release empty VENC stream\n");
                app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
                break;
            }
        } else if (ret != MPI_ERR_EMPTY && ret != MPI_ERR_NOT_READY) {
            printf("failed to get VENC stream, ret=%d\n", ret);
            app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
            break;
        }
#endif

        /* 暂停: 编码链路与 AVI 文件保持打开, 丢弃暂停期间的音视频帧, 恢复后同一文件续写 */
        if (ctx->rec_paused) {
            if (got_video) {
                mpi_ret_t rel = app_camera_rec_release_stream(ctx, &video_stream);
                if (rel != MPI_SUCCESS) {
                    printf("failed to release paused VENC stream, ret=%d\n", rel);
                    app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
                    break;
                }
            }
            if (ctx->audio_rec != NULL) {
                while (app_video_audio_rec_get_frame(ctx->audio_rec, &audio_frame) == 0) {
                    app_video_audio_rec_release_frame(audio_frame);
                }
            }
            sync_started = 0;  /* 恢复后重立时间基线, 防止补帧补偿暂停时长 */
            os_time_dly(1);
            continue;
        }

#if !APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
        if (ctx->audio_rec != NULL) {
            while (1) {
                int audio_ret = app_video_audio_rec_get_frame(ctx->audio_rec, &audio_frame);

                if (audio_ret == 0) {
                    app_camera_ret_t app_ret = app_camera_rec_audio_frame_write(ctx,
                                               audio_frame,
                                               file_base_ms);

                    app_video_audio_rec_release_frame(audio_frame);
                    wrote = 1;
                } else {
                    break;
                }
            }
        }
#endif

        if (got_video) {
            app_camera_ret_t app_ret;

            app_ret = app_camera_rec_video_write(ctx,
                                                 &video_stream,
                                                 &last_rec_sec,
                                                 file_base_ms);
            ret = app_camera_rec_release_stream(ctx, &video_stream);
            if (app_ret != APP_CAMERA_OK) {
                break;
            }
            if (ret != MPI_SUCCESS) {
                printf("failed to release VENC stream, ret=%d\n", ret);
                app_camera_rec_post_event(ctx, APP_CAMERA_EVENT_REC_ERROR);
                break;
            }
        }

#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
        if (ctx->audio_rec != NULL) {
            int audio_ret = app_video_audio_rec_get_frame(ctx->audio_rec, &audio_frame);

            if (audio_ret == 0) {
                app_camera_ret_t app_ret = app_camera_rec_audio_frame_write(ctx,
                                           audio_frame,
                                           file_base_ms);

                app_video_audio_rec_release_frame(audio_frame);
                if (app_ret != APP_CAMERA_OK) {
                    break;
                }
                wrote = 1;
            }
        }
#endif

        if (got_video) {
            app_camera_ret_t app_ret;

            if (!sync_started) {
                uint32_t now_ms = timer_get_ms();

                file_base_ms = now_ms;
                sync_base_ms = now_ms;
                sync_started = 1;
            }
            wrote = 1;

            app_ret = app_camera_rec_check_cycle(ctx,
                                                 &last_rec_sec,
                                                 cyc_frame,
                                                 &cyc_count,
                                                 &file_base_ms,
                                                 &sync_base_ms,
                                                 &sync_started);
            if (app_ret != APP_CAMERA_OK || ctx->rec_thread_exit_flag) {
                break;
            }
        }

        if (!ctx->rec_thread_exit_flag && sync_started) {
            app_camera_ret_t app_ret;
            uint8_t did_dup = 0;

            app_ret = app_camera_rec_video_dup_to_clock(ctx,
                      &last_rec_sec,
                      cyc_frame,
                      &cyc_count,
                      &file_base_ms,
                      &sync_base_ms,
                      &sync_started,
                      &did_dup);
            if (app_ret != APP_CAMERA_OK || ctx->rec_thread_exit_flag) {
                break;
            }
            if (did_dup) {
                wrote = 1;
            }
        }

        if (wrote && !ctx->rec_thread_exit_flag) {
            app_camera_rec_prealloc_step(ctx);
        }

        if (!wrote) {
            os_time_dly(1);
        }
    }
}

static app_camera_ret_t app_camera_vpss_venc_bind_open(app_camera_handle_t ctx,
        const app_camera_rec_attr_t *attr)
{
    mpi_ret_t ret;
    app_camera_ret_t app_ret;
    mpi_vpss_chn_attr_t chn_attr = {0};
    mpi_venc_chn_attr_t venc_attr = {0};
    mpi_chn_t vpss_chn_src;
    mpi_chn_t venc_chn_dst;

    if (ctx->vpss_venc_rec_enabled) {
        printf("record is already started\n");
        return APP_CAMERA_ERR_STATE;
    }

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS record channel\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    chn_attr.width = attr->width;
    chn_attr.height = attr->height;
    chn_attr.format = APP_CAMERA_REC_FORMAT;
    chn_attr.yuv_order = APP_CAMERA_REC_YUV_ORDER;
    chn_attr.mirror = 0;
    chn_attr.output_mode = MPI_VPSS_OUTPUT_ONLINE;
#if defined CONFIG_CPU_BR33
    chn_attr.hw_id = ctx->record_osd.enable ? MPI_VPSS_HW_IMC_DST0 : MPI_VPSS_HW_AUTO;
#else
    chn_attr.hw_id = MPI_VPSS_HW_AUTO;
#endif
    chn_attr.buf_addr = NULL;
    chn_attr.buf_size = calc_online_buf_size(chn_attr.width, chn_attr.format);
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn_rec, &chn_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set VPSS record channel attributes\n");
        return APP_CAMERA_ERR_NOT_FOUND;
    }

    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS record channel\n");
        goto err_destroy_vpss_chn;
    }

    //TODO
    ctx->venc_dev_rec = APP_CAMERA_REC_VENC_HW_DEV;

    ret = mpi_venc_find_free_chn(&ctx->venc_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VENC channel\n");
        goto err_disable_vpss_chn;
    }

    venc_attr.hw_dev = ctx->venc_dev_rec;
    venc_attr.type = MPI_VENC_TYPE_JPEG;
    venc_attr.input_mode = MPI_VENC_INPUT_ONLINE;
    venc_attr.quality = attr->quality ? attr->quality : APP_CAMERA_REC_DEFAULT_QUALITY;
    venc_attr.abr_kbps = attr->abr_kbps ? attr->abr_kbps : APP_CAMERA_REC_VENC_ABR_KBPS;
    venc_attr.fps = attr->fps ? attr->fps : APP_CAMERA_REC_FRAME_RATE;
    venc_attr.stream_buf_size = attr->stream_buf_size ? attr->stream_buf_size : APP_CAMERA_REC_STREAM_BUF_SIZE;
    venc_attr.in_width = attr->width;
    venc_attr.in_height = attr->height;
    venc_attr.in_format = chn_attr.format;
    venc_attr.in_yuv_order = chn_attr.yuv_order;

    ret = mpi_venc_create_chn(ctx->venc_chn_rec, &venc_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VENC channel\n");
        goto err_disable_vpss_chn;
    }

    ret = mpi_venc_start_chn(ctx->venc_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VENC channel\n");
        goto err_destroy_venc_chn;
    }

    app_ret = app_camera_osd_apply(ctx, &ctx->record_osd, ctx->vpss_chn_rec);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to apply record OSD, keep record running ret=%d\n", app_ret);
    }

    vpss_chn_src.mod_id = MPI_MOD_ID_VPSS;
    vpss_chn_src.dev_id = ctx->vpss_grp;
    vpss_chn_src.chn_id = ctx->vpss_chn_rec;

    venc_chn_dst.mod_id = MPI_MOD_ID_VENC;
    venc_chn_dst.dev_id = ctx->venc_dev_rec;
    venc_chn_dst.chn_id = ctx->venc_chn_rec;

    ret = mpi_sys_bind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS channel and VENC channel\n");
        goto err_disable_osd;
    }

    ctx->vpss_venc_rec_enabled = 1;

    return APP_CAMERA_OK;

err_disable_osd:
    app_camera_osd_disable(ctx, &ctx->record_osd, ctx->vpss_chn_rec);
    mpi_venc_stop_chn(ctx->venc_chn_rec);
err_destroy_venc_chn:
    mpi_venc_destroy_chn(ctx->venc_chn_rec);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_rec);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_rec);
    return APP_CAMERA_ERR_IO;
}

static app_camera_ret_t app_camera_rec_avi_start(app_camera_handle_t ctx,
        const app_camera_rec_attr_t *attr)
{
    app_camera_ret_t app_ret;

    memcpy(&ctx->rec_attr, attr, sizeof(app_camera_rec_attr_t));
    ctx->rec_frame_count = 0;
    ctx->rec_paused = 0;  /* 新一段录像必须清暂停标志, 否则 pause→stop→start 后一直丢帧 */
    ctx->rec_wait_path = 0;
    ctx->rec_path_ready = 0;
    ctx->rec_next_path[0] = '\0';
    os_sem_create(&ctx->rec_path_sem, 0);

    app_ret = app_camera_rec_avi_open(ctx);
    if (app_ret != APP_CAMERA_OK) {
        os_sem_del(&ctx->rec_path_sem, OS_DEL_ALWAYS);
    }

    return app_ret;
}

static app_camera_ret_t app_camera_rec_worker_start(app_camera_handle_t ctx)
{
    ctx->rec_thread_exit_flag = 0;
    ctx->rec_capture_thread_pid = 0;
    ctx->rec_thread_pid = 0;
#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
    ctx->rec_stream_ready = 0;
    memset(&ctx->rec_latest_stream, 0, sizeof(ctx->rec_latest_stream));
    if (os_mutex_create(&ctx->rec_stream_mutex) != OS_NO_ERR) {
        printf("failed to create record stream mutex\n");
        return APP_CAMERA_ERR_IO;
    }
    ctx->rec_stream_mutex_ready = 1;
    snprintf(ctx->rec_capture_thread_name,
             sizeof(ctx->rec_capture_thread_name),
             "rec_cap_%d", ctx->camera_id);
    if (thread_fork(ctx->rec_capture_thread_name,
                    APP_CAMERA_REC_WORKER_PRIO,
                    APP_CAMERA_REC_CAPTURE_WORKER_STACK,
                    0,
                    &ctx->rec_capture_thread_pid,
                    app_camera_rec_capture_thread,
                    (void *)ctx) != 0) {
        printf("failed to create record capture thread :%s \n",
               ctx->rec_capture_thread_name);
        os_mutex_del(&ctx->rec_stream_mutex, OS_DEL_ALWAYS);
        ctx->rec_stream_mutex_ready = 0;
        return APP_CAMERA_ERR_IO;
    }
#endif
    snprintf(ctx->rec_thread_name, sizeof(ctx->rec_thread_name), "rec_avi_%d", ctx->camera_id);
    if (thread_fork(ctx->rec_thread_name,
                    APP_CAMERA_REC_WORKER_PRIO,
                    APP_CAMERA_REC_WORKER_STACK,
                    0,
                    &ctx->rec_thread_pid,
                    app_camera_rec_thread,
                    (void *)ctx) != 0) {
        printf("failed to create record thread :%s \n", ctx->rec_thread_name);
#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
        ctx->rec_thread_exit_flag = 1;
        thread_kill(&ctx->rec_capture_thread_pid, KILL_WAIT);
        ctx->rec_capture_thread_pid = 0;
        app_camera_rec_capture_release_pending(ctx);
        os_mutex_del(&ctx->rec_stream_mutex, OS_DEL_ALWAYS);
        ctx->rec_stream_mutex_ready = 0;
#endif
        return APP_CAMERA_ERR_IO;
    }

    return APP_CAMERA_OK;
}

static app_camera_ret_t app_camera_rec_avi_stop(app_camera_handle_t ctx)
{
    app_camera_ret_t app_ret = APP_CAMERA_OK;

    if (ctx->rec_thread_pid || ctx->rec_capture_thread_pid) {
        ctx->rec_thread_exit_flag = 1;
        os_sem_post(&ctx->rec_path_sem);
#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
        if (ctx->rec_capture_thread_pid) {
            thread_kill(&ctx->rec_capture_thread_pid, KILL_WAIT);
            ctx->rec_capture_thread_pid = 0;
        }
#endif
    }
    if (ctx->rec_thread_pid) {
        thread_kill(&ctx->rec_thread_pid, KILL_WAIT);
        ctx->rec_thread_pid = 0;
    }
#if APP_CAMERA_REC_DROP_OLD_FRAMES_ENABLE
    if (ctx->rec_stream_mutex_ready) {
        app_camera_rec_capture_release_pending(ctx);
        os_mutex_del(&ctx->rec_stream_mutex, OS_DEL_ALWAYS);
        ctx->rec_stream_mutex_ready = 0;
    }
#endif
    printf("record stop frame count: %d\n", ctx->rec_frame_count);
    if (ctx->avi) {
        /* 记实际文件名, 入库由应用层延迟 (AVI_close 后立即重开读破坏 JLFS 缓冲) */
        if (ctx->last_video_file) {
            FILE *fp = (FILE *)(long)ctx->avi->fdes;
            fget_name(fp, (u8 *)ctx->last_video_file, 128);
        }
        int ret = AVI_close(ctx->avi);
        app_camera_rec_flush_wbuf(ctx);
        if (ret != 0 && ctx->rec_frame_count > 0) {
            printf("failed to close AVI file, ret=%d\n", ret);
            app_ret = APP_CAMERA_ERR_IO;
        }
        ctx->avi = NULL;
    }
    ctx->rec_wait_path = 0;
    ctx->rec_path_ready = 0;
    ctx->rec_next_path[0] = '\0';
    os_sem_del(&ctx->rec_path_sem, OS_DEL_ALWAYS);

    return app_ret;
}


static app_camera_ret_t app_camera_vpss_venc_bind_close(app_camera_handle_t ctx)
{
    mpi_ret_t ret;
    mpi_chn_t vpss_chn_src = {.mod_id = MPI_MOD_ID_VPSS, .dev_id = ctx->vpss_grp, .chn_id = ctx->vpss_chn_rec};
    mpi_chn_t venc_chn_dst = {.mod_id = MPI_MOD_ID_VENC, .dev_id = ctx->venc_dev_rec, .chn_id = ctx->venc_chn_rec};

    if (!ctx->vpss_venc_rec_enabled) {
        printf("record not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    if (app_camera_osd_disable(ctx, &ctx->record_osd,
                               ctx->vpss_chn_rec) != APP_CAMERA_OK) {
        printf("failed to disable record OSD\n");
    }

    ret = mpi_sys_unbind(&vpss_chn_src, &venc_chn_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to unbind VPSS and VENC\n");
    }

    ret = mpi_venc_stop_chn(ctx->venc_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VENC channel\n");

    }

    ret = mpi_venc_destroy_chn(ctx->venc_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VENC channel\n");
    }

    ret = mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable VPSS record channel\n");
    }
    ret = mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn_rec);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS record channel\n");
    }

    ctx->vpss_venc_rec_enabled = 0;

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_rec_start(app_camera_handle_t handle,
                                      const app_camera_rec_attr_t *attr)
{
    app_camera_ret_t app_ret;
    int once_open_src = 0;

    if (handle == NULL || attr == NULL) {
        printf("handle or record attr is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (attr->width == 0 || attr->height == 0 || attr->fps == 0) {
        printf("invalid record size or fps\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (handle->vpss_venc_rec_enabled) {
        printf("record already started\n");
        return APP_CAMERA_ERR_EXIST;
    }

    if (!app_camera_src_is_opened(handle)) {
        printf("camera source is not opened, open it first\n");
        app_ret = app_camera_src_open(handle);
        if (app_ret != APP_CAMERA_OK) {
            printf("failed to open camera source\n");
            return app_ret;
        }
        once_open_src = 1;
    }

    app_ret = app_camera_rec_audio_open(handle, attr);
    if (app_ret != APP_CAMERA_OK) {
        goto err_out1;
    }

    app_ret = app_camera_rec_avi_start(handle, attr);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to start AVI recording\n");
        goto err_close_audio;
    }

    app_ret = app_camera_vpss_venc_bind_open(handle, attr);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to start record\n");
        goto err_stop_avi;
    }

    if (handle->digital_zoom_enabled) {
        mpi_vpss_crop_param_t crop_param = {0};
        mpi_ret_t r;

        crop_param.x_start = handle->digital_zoom_param.x_start;
        crop_param.y_start = handle->digital_zoom_param.y_start;
        crop_param.crop_w  = handle->digital_zoom_param.crop_w;
        crop_param.crop_h  = handle->digital_zoom_param.crop_h;

        r = mpi_vpss_chn_set_crop(handle->vpss_grp,
                                  handle->vpss_chn_rec,
                                  &crop_param);
        if (r != MPI_SUCCESS) {
            printf("failed to apply digital zoom to encode: %d\n", r);
            goto err_close_venc;
        }
        printf("digital zoom applied to encode\n");
    }

    app_ret = app_camera_rec_audio_start(handle);
    if (app_ret != APP_CAMERA_OK) {
        goto err_close_venc;
    }

    app_ret = app_camera_rec_worker_start(handle);
    if (app_ret != APP_CAMERA_OK) {
        goto err_stop_audio;
    }

    app_camera_rec_post_event(handle, APP_CAMERA_EVENT_REC_STARTED);

    return APP_CAMERA_OK;

err_stop_audio:
    app_camera_rec_audio_stop(handle);
err_close_venc:
    app_camera_vpss_venc_bind_close(handle);
err_stop_avi:
    app_camera_rec_avi_stop(handle);
err_close_audio:
    app_camera_rec_audio_close(handle);
err_out1:
    if (once_open_src) {
        app_camera_src_close(handle);
    }
    return app_ret;
}

app_camera_ret_t app_camera_rec_stop(app_camera_handle_t handle)
{
    app_camera_ret_t app_ret;
    app_camera_ret_t src_ret;

    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    if (!handle->vpss_venc_rec_enabled) {
        printf("record not started\n");
        return APP_CAMERA_ERR_NOT_READY;
    }

    handle->rec_paused = 0;  /* stop 总是结束暂停态, 防止下一次 start 继承 paused 丢帧 */

    app_camera_rec_audio_stop(handle);
    app_ret = app_camera_rec_avi_stop(handle);
    if (app_ret != APP_CAMERA_OK) {
        printf("failed to stop AVI recording\n");
    }
    app_camera_rec_audio_close(handle);

    src_ret = app_camera_vpss_venc_bind_close(handle);
    if (src_ret != APP_CAMERA_OK) {
        printf("failed to stop record\n");
        if (app_ret == APP_CAMERA_OK) {
            app_ret = src_ret;
        }
    }

    if (app_camera_src_need_stop(handle)) {
        printf("camera source is no longer needed, stopping it\n");
        src_ret = app_camera_src_close(handle);
        if (src_ret != APP_CAMERA_OK) {
            printf("failed to close camera source\n");
            if (app_ret == APP_CAMERA_OK) {
                app_ret = src_ret;
            }
        }
    }

    if (app_ret == APP_CAMERA_OK) {
        app_camera_rec_post_event(handle, APP_CAMERA_EVENT_REC_STOPPED);
    }

    return app_ret;
}

app_camera_ret_t app_camera_rec_set_path(app_camera_handle_t handle,
        const char *path)
{
    if (handle == NULL || path == NULL || path[0] == '\0') {
        return APP_CAMERA_ERR_INVALID_ARG;
    }

    if (!handle->rec_wait_path) {
        return APP_CAMERA_ERR_NOT_READY;
    }

    strncpy(handle->rec_next_path, path, sizeof(handle->rec_next_path) - 1u);
    handle->rec_next_path[sizeof(handle->rec_next_path) - 1u] = '\0';
    handle->rec_path_ready = 1;
    os_sem_post(&handle->rec_path_sem);

    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_rec_pause(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (!handle->vpss_venc_rec_enabled) {
        printf("record not started, cannot pause\n");
        return APP_CAMERA_ERR_NOT_READY;
    }
    if (handle->rec_paused) {
        printf("record already paused\n");
        return APP_CAMERA_OK;
    }
    handle->rec_paused = 1;
    printf("record paused\n");
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_rec_resume(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    if (!handle->vpss_venc_rec_enabled) {
        printf("record not started, cannot resume\n");
        return APP_CAMERA_ERR_NOT_READY;
    }
    if (!handle->rec_paused) {
        printf("record not paused\n");
        return APP_CAMERA_OK;
    }
    handle->rec_paused = 0;
    printf("record resumed\n");
    return APP_CAMERA_OK;
}

app_camera_ret_t app_camera_set_record_text_osd(app_camera_handle_t handle,
        const app_camera_osd_attr_t *attr)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_set_text(&handle->record_osd, attr);
}

app_camera_ret_t app_camera_enable_record_osd(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_enable(handle,
                                 &handle->record_osd,
                                 handle->vpss_chn_rec,
                                 handle->vpss_venc_rec_enabled);
}

app_camera_ret_t app_camera_disable_record_osd(app_camera_handle_t handle)
{
    if (handle == NULL) {
        return APP_CAMERA_ERR_INVALID_ARG;
    }
    return app_camera_osd_disable(handle,
                                  &handle->record_osd,
                                  handle->vpss_chn_rec);
}

int app_camera_rec_is_started(app_camera_handle_t handle)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return 0;
    }
    return handle->vpss_venc_rec_enabled;
}

