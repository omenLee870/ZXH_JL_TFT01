#include "app_playback_mpi_priv.h"

#define APP_PLAYBACK_DEMUX_THREAD_PRIO   10
#define APP_PLAYBACK_DEMUX_THREAD_STACK  2048
#define APP_PLAYBACK_LBUF_ALIGN          4
#define APP_PLAYBACK_DEMUX_DEFAULT_FPS   20u

static uint32_t app_playback_demux_video_pts(uint32_t frame_index,
        double fps)
{
    if (fps <= 0.0) {
        fps = APP_PLAYBACK_DEMUX_DEFAULT_FPS;
    }

    return (uint32_t)(((double)frame_index * 1000.0) / fps);
}

app_playback_ret_t app_playback_demux_frame_queue_open(app_playback_handle_t ctx,
        uint32_t size)
{
    if (ctx == NULL || size == 0) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (ctx->video_lbuf != NULL || ctx->video_lbuf_mem != NULL) {
        return APP_PLAYBACK_ERR_STATE;
    }

    ctx->video_lbuf_mem = malloc(size);
    if (ctx->video_lbuf_mem == NULL) {
        return APP_PLAYBACK_ERR_NOMEM;
    }

    ctx->video_lbuf = lbuf_init(ctx->video_lbuf_mem,
                                size,
                                APP_PLAYBACK_LBUF_ALIGN,
                                sizeof(app_playback_frame_hdr_t));
    if (ctx->video_lbuf == NULL) {
        free(ctx->video_lbuf_mem);
        ctx->video_lbuf_mem = NULL;
        return APP_PLAYBACK_ERR_IO;
    }

    os_sem_create(&ctx->demux.frame_sem, 0);
    os_sem_create(&ctx->demux.space_sem, 0);

    return APP_PLAYBACK_OK;
}

app_playback_frame_hdr_t *app_playback_demux_frame_pop(app_playback_handle_t ctx)
{
    if (ctx == NULL || ctx->video_lbuf == NULL) {
        return NULL;
    }

    return (app_playback_frame_hdr_t *)lbuf_pop(ctx->video_lbuf, 1u);
}

void app_playback_demux_frame_wait(app_playback_handle_t ctx)
{
    if (ctx == NULL || ctx->video_lbuf == NULL) {
        return;
    }

    os_sem_pend(&ctx->demux.frame_sem, 0);
}

void app_playback_demux_frame_wakeup(app_playback_handle_t ctx)
{
    if (ctx == NULL || ctx->video_lbuf == NULL) {
        return;
    }

    os_sem_post(&ctx->demux.frame_sem);
    os_sem_post(&ctx->demux.space_sem);
}

void app_playback_demux_frame_free(app_playback_handle_t ctx,
                                   app_playback_frame_hdr_t *frame)
{
    if (frame == NULL) {
        return;
    }

    lbuf_free(frame);
    if (ctx != NULL && ctx->video_lbuf != NULL) {
        os_sem_post(&ctx->demux.space_sem);
    }
}

void app_playback_demux_frame_queue_close(app_playback_handle_t ctx)
{
    app_playback_frame_hdr_t *hdr;

    if (ctx == NULL) {
        return;
    }

    while ((hdr = app_playback_demux_frame_pop(ctx)) != NULL) {
        app_playback_demux_frame_free(ctx, hdr);
    }
    if (os_sem_valid(&ctx->demux.frame_sem)) {
        os_sem_set(&ctx->demux.frame_sem, 0);
        os_sem_del(&ctx->demux.frame_sem, OS_DEL_ALWAYS);
    }
    if (os_sem_valid(&ctx->demux.space_sem)) {
        os_sem_set(&ctx->demux.space_sem, 0);
        os_sem_del(&ctx->demux.space_sem, OS_DEL_ALWAYS);
    }

    ctx->video_lbuf = NULL;

    if (ctx->video_lbuf_mem != NULL) {
        free(ctx->video_lbuf_mem);
        ctx->video_lbuf_mem = NULL;
    }
}

static int app_playback_video_is_mjpg(const char *compressor)
{
    if (compressor == NULL) {
        return 0;
    }

    return ((compressor[0] == 'M' || compressor[0] == 'm') &&
            (compressor[1] == 'J' || compressor[1] == 'j') &&
            (compressor[2] == 'P' || compressor[2] == 'p') &&
            (compressor[3] == 'G' || compressor[3] == 'g'));
}

app_playback_ret_t app_playback_demux_open(app_playback_handle_t ctx,
        const char *path)
{
    app_playback_demux_ctx_t *demux;
    char *compressor;

    if (ctx == NULL || path == NULL || path[0] == '\0') {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    demux = &ctx->demux;
    memset(&demux->info, 0, sizeof(demux->info));
    demux->avi = NULL;
    demux->video_frame_index = 0;
    demux->thread_exit_flag = 0;
    demux->thread_finished = 0;

    demux->avi = AVI_open_input_file(path, 1);
    if (demux->avi == NULL) {
        printf("failed to open AVI file: %s\n", path);
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    compressor = AVI_video_compressor(demux->avi);
    if (!app_playback_video_is_mjpg(compressor)) {
        printf("unsupported AVI compressor: %s\n", compressor ? compressor : "NULL");
        AVI_close(demux->avi);
        demux->avi = NULL;
        memset(&demux->info, 0, sizeof(demux->info));
        return APP_PLAYBACK_ERR_NOT_SUPPORT;
    }

    demux->info.total_frames = AVI_video_frames(demux->avi);
    demux->info.fps = AVI_frame_rate(demux->avi);
    demux->info.audio_channels = (uint8_t)AVI_audio_channels(demux->avi);
    demux->info.audio_bits = (uint8_t)AVI_audio_bits(demux->avi);
    demux->info.audio_rate = AVI_audio_rate(demux->avi);
    demux->info.audio_format = AVI_audio_format(demux->avi);
    demux->info.audio_chunks = AVI_audio_chunks(demux->avi);
    demux->info.audio_bytes = AVI_audio_bytes(demux->avi);
    demux->info.has_audio = (demux->info.audio_channels > 0 &&
                             demux->info.audio_rate > 0 &&
                             demux->info.audio_chunks > 0);

    if (demux->info.total_frames <= 0) {
        printf("invalid AVI file, frames=%ld\n", demux->info.total_frames);
        AVI_close(demux->avi);
        demux->avi = NULL;
        memset(&demux->info, 0, sizeof(demux->info));
        return APP_PLAYBACK_ERR_IO;
    }

    return APP_PLAYBACK_OK;
}

void app_playback_demux_close(app_playback_handle_t ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->demux.avi != NULL) {
        AVI_close(ctx->demux.avi);
    }
    ctx->demux.avi = NULL;
    ctx->demux.video_frame_index = 0;
    memset(&ctx->demux.info, 0, sizeof(ctx->demux.info));
}

static void app_playback_demux_thread(void *parm)
{
    app_playback_handle_t ctx = (app_playback_handle_t)parm;
    app_playback_demux_ctx_t *demux = &ctx->demux;

    while (!demux->thread_exit_flag) {
        app_playback_frame_hdr_t *hdr;
        long frame_size;
        int keyframe = 0;
        long len;

        frame_size = AVI_frame_size(demux->avi, demux->video_frame_index);
        if (frame_size <= 0) {
            /* AVI_frame_size 返回0有两种情况, 需区分:
             *   (a) frame >= video_frames  -> 真EOF
             *   (b) video_index[frame].len == 0 -> 中间的0长度视频帧(录像dup补帧/编码器
             *       hiccup产生), 不是结束! PC播放器会跳过它。
             * 之前一律当EOF -> 遇到第一个0长度帧就提前停(20多秒视频只播几秒)。
             * 修: 只有到达总帧数才是真EOF; 否则跳过该0长度帧(推进AVI内部video_pos
             * 与本地index保持同步), 继续读后面的帧。 */
            if (frame_size < 0 ||
                (long)demux->video_frame_index >= demux->info.total_frames) {
                printf("video demux finished\n");
                break;
            }
            {
                int kf = 0;
                (void)AVI_read_video(demux->avi, NULL, -1, &kf);  /* NULL buf: 仅推进video_pos */
            }
            demux->video_frame_index++;
            continue;
        }
        if (lbuf_empty(ctx->video_lbuf) &&
            lbuf_avaliable(ctx->video_lbuf, (int)frame_size) <= 0) {
            printf("video frame is too large for lbuf, frame=%ld\n", frame_size);
            break;
        }

        hdr = (app_playback_frame_hdr_t *)lbuf_alloc(ctx->video_lbuf, (u32)frame_size);
        if (hdr == NULL) {
            os_sem_pend(&demux->space_sem, 0);
            continue;
        }

        len = AVI_read_video(demux->avi, (char *)hdr->data, frame_size, &keyframe);
        if (len <= 0) {
            lbuf_free(hdr);
            printf("video demux finished\n");
            break;
        }

        hdr->len = (uint32_t)len;
        hdr->pts = app_playback_demux_video_pts(demux->video_frame_index,
                                                demux->info.fps);
        demux->video_frame_index++;
        lbuf_push(hdr, 1u);
        os_sem_post(&demux->frame_sem);
    }

    demux->thread_finished = 1;
    app_playback_demux_frame_wakeup(ctx);
    demux->thread_pid = 0;
}

app_playback_ret_t app_playback_demux_start(app_playback_handle_t ctx)
{
    app_playback_demux_ctx_t *demux;

    if (ctx == NULL || ctx->demux.avi == NULL || ctx->video_lbuf == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    demux = &ctx->demux;
    demux->thread_exit_flag = 0;
    demux->thread_finished = 0;
    snprintf(demux->thread_name,
             sizeof(demux->thread_name),
             "demux_%d",
             ctx->vo_chn);
    if (thread_fork(demux->thread_name,
                    APP_PLAYBACK_DEMUX_THREAD_PRIO,
                    APP_PLAYBACK_DEMUX_THREAD_STACK,
                    0,
                    &demux->thread_pid,
                    app_playback_demux_thread,
                    (void *)ctx) != 0) {
        printf("failed to create demux thread: %s\n", demux->thread_name);
        demux->thread_finished = 1;
        return APP_PLAYBACK_ERR_IO;
    }

    return APP_PLAYBACK_OK;
}

void app_playback_demux_stop(app_playback_handle_t ctx)
{
    app_playback_demux_ctx_t *demux;

    if (ctx == NULL) {
        return;
    }

    demux = &ctx->demux;
    if (demux->thread_pid) {
        demux->thread_exit_flag = 1;
        app_playback_demux_frame_wakeup(ctx);
        thread_kill(&demux->thread_pid, KILL_WAIT);
        demux->thread_pid = 0;
    }

    demux->thread_exit_flag = 0;
}


