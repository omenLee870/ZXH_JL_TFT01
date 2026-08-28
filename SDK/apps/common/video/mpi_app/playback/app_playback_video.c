#include "app_playback_mpi_priv.h"
#include "video_rx_player.h"

#define APP_PLAYBACK_VIDEO_THREAD_PRIO    10
#define APP_PLAYBACK_VIDEO_THREAD_STACK   2048
#define APP_PLAYBACK_VIDEO_BUF_DEFAULT    (200u * 1024u)
#define APP_PLAYBACK_AUDIO_BUF_DEFAULT    (64u * 1024u)
#define APP_PLAYBACK_VIDEO_DEFAULT_FPS    20u

extern uint32_t timer_get_ms(void);

static app_playback_ret_t app_playback_video_post_event(app_playback_handle_t handle,
        int event)
{
    int argv[1];

    argv[0] = event;
    return app_playback_post_event(handle, ARRAY_SIZE(argv), argv);
}

static app_playback_ret_t app_playback_video_post_event_value(app_playback_handle_t handle,
        int event,
        int value)
{
    int argv[2];

    argv[0] = event;
    argv[1] = value;
    return app_playback_post_event(handle, ARRAY_SIZE(argv), argv);
}

static uint32_t app_playback_video_total_ms(app_playback_handle_t handle)
{
    double fps;

    if (handle == NULL || handle->demux.info.total_frames <= 0) {
        return 0;
    }

    fps = handle->demux.info.fps;
    if (fps <= 0.0) {
        fps = APP_PLAYBACK_VIDEO_DEFAULT_FPS;
    }

    return (uint32_t)(((double)handle->demux.info.total_frames * 1000.0) / fps);
}


static uint32_t app_playback_video_frame_interval_ms(double fps)
{
    uint32_t frame_interval_ms;

    if (fps <= 0.0) {
        fps = APP_PLAYBACK_VIDEO_DEFAULT_FPS;
    }

    frame_interval_ms = (uint32_t)(1000.0 / fps);
    return frame_interval_ms ? frame_interval_ms : 1u;
}

static int app_playback_video_time_after_eq(uint32_t now_ms,
        uint32_t target_ms)
{
    return (int32_t)(now_ms - target_ms) >= 0;
}

static int app_playback_video_frame_expired(uint32_t now_ms,
        uint32_t target_ms,
        uint32_t frame_interval_ms)
{
    if (!app_playback_video_time_after_eq(now_ms, target_ms)) {
        return 0;
    }

    return (uint32_t)(now_ms - target_ms) >= frame_interval_ms;
}

static void app_playback_video_wait_until(app_playback_handle_t ctx,
        uint32_t target_ms)
{
    while (!ctx->video_thread_exit_flag) {
        uint32_t now_ms = timer_get_ms();

        if (app_playback_video_time_after_eq(now_ms, target_ms)) {
            break;
        }

        os_time_dly(1);
    }
}

static void app_playback_video_thread(void *parm)
{
    app_playback_handle_t ctx = (app_playback_handle_t)parm;
    uint32_t base_ms = timer_get_ms();
    uint32_t frame_interval_ms = app_playback_video_frame_interval_ms(ctx->demux.info.fps);
    uint8_t first_frame = 1;
    int natural_finish = 0;
    int play_sec = -1;
    int cur_sec;

    while (!ctx->video_thread_exit_flag) {
        app_playback_frame_hdr_t *hdr;
        mpi_stream_t stream = {0};
        uint32_t now_ms;
        uint32_t target_ms;

        hdr = app_playback_demux_frame_pop(ctx);
        if (hdr == NULL) {
            if (ctx->demux.thread_finished) {
                printf("video playback finished\n");
                natural_finish = 1;
                break;
            }
            app_playback_demux_frame_wait(ctx);
            continue;
        }

        now_ms = timer_get_ms();
        target_ms = base_ms + hdr->pts;
        if (app_playback_video_frame_expired(now_ms, target_ms, frame_interval_ms)) {
            printf("drop expired video frame, pts=%u\n", hdr->pts);
            app_playback_demux_frame_free(ctx, hdr);
            continue;
        }

        app_playback_video_wait_until(ctx, target_ms);
        if (ctx->video_thread_exit_flag) {
            app_playback_demux_frame_free(ctx, hdr);
            break;
        }

        stream.addr = (const uint8_t *)hdr->data;
        stream.len = hdr->len;

        if (app_playback_image_decode_stream(ctx, &stream) != APP_PLAYBACK_OK) {
            printf("failed to decode video frame\n");
            app_playback_video_post_event(ctx, APP_PLAYBACK_EVENT_ERROR);
            app_playback_demux_frame_free(ctx, hdr);
            break;
        }

        if (first_frame) {
            first_frame = 0;
            app_playback_video_post_event(ctx, APP_PLAYBACK_EVENT_FIRST_FRAME);
        }

        //post cur play time
        cur_sec = hdr->pts / 1000;
        if (play_sec != cur_sec) {
            play_sec = cur_sec;
            app_playback_video_post_event_value(ctx, APP_PLAYBACK_EVENT_CUR_TIME, (int)hdr->pts);
        }


        app_playback_demux_frame_free(ctx, hdr);
    }

    if (natural_finish && !ctx->video_thread_exit_flag) {
        app_playback_video_post_event(ctx, APP_PLAYBACK_EVENT_FINISHED);
    }
}


/*==============================================================================
 * 音频播放(独立线程, 独立avi句柄): 通过 video_rx_player 接入 jlstream 流水线。
 * 流水线自动处理解码(PCM透传/MP3解码)、采样率转换、立体声混音和 DAC 输出。
 * 与视频共享播放起点, DAC硬件实时定速自然与视频(pts墙钟定速)同步。
 *============================================================================*/
#define APP_PLAYBACK_AUDIO_THREAD_PRIO   10
#define APP_PLAYBACK_AUDIO_THREAD_STACK  3072   /* 音频线程内调AVI_open_input_file(重解析)+DAC, 原1024栈溢出冲坏demux句柄video_frames导致回放截断 */

/* 文件级静态变量: 音频播放线程与 rx_func 回调之间的数据桥 */

static avi_t *g_audio_avi = NULL;
static volatile int *g_audio_exit_flag = NULL;

/* AVI wFormatTag -> AUDIO_CODING_* 映射 */
static u32 app_playback_audio_codec_map(long avi_fmt)
{
    switch (avi_fmt) {
    case 0x0055:
        return AUDIO_CODING_STREAM_MP3;
    case WAVE_FORMAT_PCM:
    default:
        return AUDIO_CODING_PCM;
    }
}

/* 回调函数: 在流水线线程中执行, 读取原始 AVI 音频数据送入流水线 */
static enum stream_node_state app_playback_audio_rx_func(void *file,
        struct stream_frame **pframe)
{
    struct video_rx_file_handle *hdl = (struct video_rx_file_handle *)file;
    struct stream_frame *frame;
    int rlen;
    u16 fram_len = 640;  /* 16kHz 16-bit mono, 20ms = 640 bytes */

    if (g_audio_avi == NULL || g_audio_exit_flag == NULL || *g_audio_exit_flag) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }

    frame = jlstream_get_frame(hdl->node->oport, fram_len);
    if (frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }

    rlen = (int)AVI_read_audio(g_audio_avi, (char *)frame->data, fram_len);
    if (rlen <= 0) {
        jlstream_free_frame(frame);
        *pframe = NULL;
        return NODE_STA_RUN;  /* 音频数据读完, 流水线自然停止 */
    }
    frame->len = rlen;
    *pframe = frame;
    return NODE_STA_RUN;
}

static void app_playback_audio_thread(void *parm)
{
    app_playback_handle_t ctx = (app_playback_handle_t)parm;
    avi_t *avi = NULL;
    struct video_rx_player_param param = {0};
    int rate = ctx->demux.info.audio_rate;
    int err;

    avi = AVI_open_input_file(ctx->play_path, 1);
    if (avi == NULL) {
        printf("audio playback: open AVI failed: %s\n", ctx->play_path);
        goto out;
    }

    g_audio_avi = avi;
    g_audio_exit_flag = &ctx->audio_thread_exit_flag;

    param.coding_type = app_playback_audio_codec_map(ctx->demux.info.audio_format);
    param.channel_mode = AUDIO_CH_MIX;
    param.sample_rate = rate > 0 ? rate : 16000;
    param.bit_rate = param.sample_rate * 2;  /* 16-bit mono */
    param.type = VIDEO_SERVICE_VOICE;
    param.frame_dms = 200;  /* 20ms */

    err = video_rx_player_open(0, &param, app_playback_audio_rx_func);
    if (err) {
        printf("audio playback: video_rx_player_open failed: %d\n", err);
        goto out;
    }

    while (!ctx->audio_thread_exit_flag) {
        os_time_dly(10);
    }

    video_rx_player_close(0);

out:
    g_audio_avi = NULL;
    g_audio_exit_flag = NULL;
    if (avi) {
        AVI_close(avi);
    }
    ctx->audio_thread_pid = 0;
}

static app_playback_ret_t app_playback_audio_start(app_playback_handle_t handle)
{
    if (!handle->demux.info.has_audio) {
        return APP_PLAYBACK_OK;   /* 无音频轨, 跳过(纯视频文件) */
    }
    handle->audio_thread_exit_flag = 0;
    snprintf(handle->audio_thread_name, sizeof(handle->audio_thread_name), "play_aud");
    if (thread_fork(handle->audio_thread_name,
                    APP_PLAYBACK_AUDIO_THREAD_PRIO,
                    APP_PLAYBACK_AUDIO_THREAD_STACK,
                    0,
                    &handle->audio_thread_pid,
                    app_playback_audio_thread,
                    (void *)handle) != 0) {
        printf("failed to create audio playback thread\n");
        handle->audio_thread_pid = 0;
        return APP_PLAYBACK_ERR_IO;
    }
    handle->audio_started = 1;
    return APP_PLAYBACK_OK;
}

static void app_playback_audio_stop(app_playback_handle_t handle)
{
    if (!handle->audio_started) {
        return;
    }
    handle->audio_thread_exit_flag = 1;
    if (handle->audio_thread_pid) {
        thread_kill(&handle->audio_thread_pid, KILL_WAIT);
        handle->audio_thread_pid = 0;
    }
    handle->audio_started = 0;
    handle->audio_thread_exit_flag = 0;
}

app_playback_ret_t app_playback_video_start(app_playback_handle_t handle,
        const char *path)
{
    app_playback_ret_t app_ret;

    if (handle == NULL || path == NULL || path[0] == '\0') {
        printf("handle or video path is invalid\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (handle->video_started) {
        printf("playback is busy\n");
        return APP_PLAYBACK_ERR_BUSY;
    }
    snprintf(handle->play_path, sizeof(handle->play_path), "%s", path);

    if (handle->audio_buffer_size == 0) {
        handle->audio_buffer_size = APP_PLAYBACK_AUDIO_BUF_DEFAULT;
    }
    if (handle->video_buffer_size == 0) {
        handle->video_buffer_size = APP_PLAYBACK_VIDEO_BUF_DEFAULT;
    }

    app_ret = app_playback_demux_frame_queue_open(handle, handle->video_buffer_size);
    if (app_ret != APP_PLAYBACK_OK) {
        printf("failed to open video frame queue\n");
        app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_ERROR);
        return app_ret;
    }

    app_ret = app_playback_demux_open(handle, path);
    if (app_ret != APP_PLAYBACK_OK) {
        app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_ERROR);
        goto err_close_lbuf;
    }
    app_playback_video_post_event_value(handle,
                                        APP_PLAYBACK_EVENT_OPENED,
                                        (int)app_playback_video_total_ms(handle));

    handle->video_started = 1;
    handle->video_thread_exit_flag = 0;

    app_ret = app_playback_demux_start(handle);
    if (app_ret != APP_PLAYBACK_OK) {
        printf("failed to start demux\n");
        app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_ERROR);
        goto err_close_demux;
    }

    snprintf(handle->video_thread_name,
             sizeof(handle->video_thread_name),
             "play_avi");
    if (thread_fork(handle->video_thread_name,
                    APP_PLAYBACK_VIDEO_THREAD_PRIO,
                    APP_PLAYBACK_VIDEO_THREAD_STACK,
                    0,
                    &handle->video_thread_pid,
                    app_playback_video_thread,
                    (void *)handle) != 0) {
        printf("failed to create video playback thread: %s\n", handle->video_thread_name);
        app_ret = APP_PLAYBACK_ERR_IO;
        app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_ERROR);
        goto err_stop_demux;
    }

    app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_STARTED);

    app_playback_audio_start(handle);  /* 有音频轨则起音频播放线程(可选, 失败不影响视频) */

    return APP_PLAYBACK_OK;

err_stop_demux:
    app_playback_demux_stop(handle);
err_close_demux:
    handle->video_started = 0;
    app_playback_demux_close(handle);
err_close_lbuf:
    app_playback_demux_frame_queue_close(handle);
    return app_ret;
}


app_playback_ret_t app_playback_video_finish(app_playback_handle_t handle)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (!handle->video_started) {
        return APP_PLAYBACK_ERR_NOT_READY;
    }

    app_playback_audio_stop(handle);
    app_playback_demux_stop(handle);

    if (handle->video_thread_pid) {
        handle->video_thread_exit_flag = 1;
        app_playback_demux_frame_wakeup(handle);
        thread_kill(&handle->video_thread_pid, KILL_WAIT);
        handle->video_thread_pid = 0;
    }

    app_playback_demux_frame_queue_close(handle);
    app_playback_demux_close(handle);
    handle->video_thread_exit_flag = 0;
    handle->video_started = 0;

    return APP_PLAYBACK_OK;
}

app_playback_ret_t app_playback_video_stop(app_playback_handle_t handle)
{
    app_playback_ret_t app_ret = APP_PLAYBACK_OK;

    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (!handle->video_started) {
        printf("video playback not started\n");
        return APP_PLAYBACK_ERR_NOT_READY;
    }

    app_playback_audio_stop(handle);
    app_playback_demux_stop(handle);

    if (handle->video_thread_pid) {
        handle->video_thread_exit_flag = 1;
        app_playback_demux_frame_wakeup(handle);
        thread_kill(&handle->video_thread_pid, KILL_WAIT);
        handle->video_thread_pid = 0;
    }

    if (handle->pipeline_started) {
        app_ret = app_playback_image_pipeline_close(handle);
        if (app_ret != APP_PLAYBACK_OK) {
            printf("failed to close video playback pipeline\n");
        }
    }

    app_playback_demux_frame_queue_close(handle);
    app_playback_demux_close(handle);
    handle->video_thread_exit_flag = 0;
    handle->video_started = 0;
    app_playback_video_post_event(handle, APP_PLAYBACK_EVENT_STOPPED);

    return app_ret;
}

app_playback_ret_t app_playback_set_video_buffer_size(app_playback_handle_t handle,
        uint32_t size)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (handle->video_started) {
        printf("video playback is busy\n");
        return APP_PLAYBACK_ERR_BUSY;
    }

    handle->video_buffer_size = size;

    return APP_PLAYBACK_OK;
}

app_playback_ret_t app_playback_set_audio_buffer_size(app_playback_handle_t handle,
        uint32_t size)
{
    if (handle == NULL) {
        printf("handle is NULL\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (handle->video_started) {
        printf("video playback is busy\n");
        return APP_PLAYBACK_ERR_BUSY;
    }

    handle->audio_buffer_size = size;

    return APP_PLAYBACK_OK;
}

