#include "app_playback_mpi_priv.h"

extern void *phy_malloc(size_t size);
extern void phy_free(void *pv);

#define APP_PLAYBACK_DEMUX_THREAD_PRIO   10
#define APP_PLAYBACK_DEMUX_THREAD_STACK  2048
#define APP_PLAYBACK_LBUF_ALIGN          4
#define APP_PLAYBACK_DEMUX_DEFAULT_FPS   20u

/* 帧队列缓冲池: 仿 video_dec.c 的 fb5 池(raw/size/used/enable 四字段)。
 * 本板无 PSRAM(TCFG_PSRAM_DEV_ENABLE=0), 帧队列与 VO fb 同抢内部物理池, 且该块
 * 要经 mpi_vdec_send_stream 交给硬件解码 ⇒ 必须物理连续, 不能换堆。
 * 由 app_playback_demux_frame_queue_prealloc() 在池子干净时先取走, 之后跨模式复用
 * 同一块, 不再每轮进模式 malloc/free 切碎物理池。 */
static struct {
    u8    enable;    /* 仅预建成功后置位, 未预建时行为与原来一致 */
    u8    used;      /* 已交给某个回放句柄 */
    u32   size;      /* 实际分配大小, 复用判据 */
    void *raw;       /* 申请到的原始指针, 供释放 */
} g_fq_pool;

/* 帧队列预建: 内存宽裕时先取走缓冲。返回 0 成功, -1 失败(调用方走原路径) */
int app_playback_demux_frame_queue_prealloc(uint32_t size)
{
    if (size == 0) {
        return -1;
    }
    if (g_fq_pool.raw != NULL && g_fq_pool.size >= size) {
        g_fq_pool.enable = 1;       /* 已有够大的常驻块: 直接复用 */
        return 0;
    }
    if (g_fq_pool.used) {
        return -1;                  /* 池块正被句柄占用: 不换块 */
    }
    if (g_fq_pool.raw != NULL) {
        phy_free(g_fq_pool.raw);     /* 旧块不够大: 换块 */
        g_fq_pool.raw = NULL;
        g_fq_pool.size = 0;
    }
    g_fq_pool.raw = phy_malloc(size);
    if (g_fq_pool.raw == NULL) {
        g_fq_pool.enable = 0;
        return -1;
    }
    g_fq_pool.size = size;
    g_fq_pool.used = 0;
    g_fq_pool.enable = 1;
    return 0;
}

/* 回放退出时释放常驻帧队列块: 录像不需要帧队列, 留着等于永久占 128KB 物理池。
 * 走 phy_free 真还物理池(malloc/free 只还给堆, 堆不把页还 pmm)。 */
void app_playback_demux_frame_queue_pool_release(void)
{
    if (g_fq_pool.used) {
        return;                      /* 仍被句柄占用: 不能拆 */
    }
    if (g_fq_pool.raw != NULL) {
        phy_free(g_fq_pool.raw);
        g_fq_pool.raw = NULL;
    }
    g_fq_pool.size = 0;
    g_fq_pool.enable = 0;
}

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

    /* 先取复用池(命中则零 malloc), 未命中回落原路径 */
    if (g_fq_pool.enable && !g_fq_pool.used && g_fq_pool.raw != NULL &&
        g_fq_pool.size >= size) {
        ctx->video_lbuf_mem = g_fq_pool.raw;
        g_fq_pool.used = 1;
    } else {
        ctx->video_lbuf_mem = malloc(size);
    }
    /* MEMDBG 取证: 帧队列尺寸与成败(事后整段清理) */
    printf("[APP_PLAYBACK][MEMDBG] fq_open size=%u %s\n", size,
           ctx->video_lbuf_mem ? "ok" : "FAIL");
    if (ctx->video_lbuf_mem == NULL) {
        return APP_PLAYBACK_ERR_NOMEM;
    }

    ctx->video_lbuf = lbuf_init(ctx->video_lbuf_mem,
                                size,
                                APP_PLAYBACK_LBUF_ALIGN,
                                sizeof(app_playback_frame_hdr_t));
    if (ctx->video_lbuf == NULL) {
        if (g_fq_pool.enable && g_fq_pool.raw == ctx->video_lbuf_mem) {
            g_fq_pool.used = 0;
        } else {
            free(ctx->video_lbuf_mem);
        }
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
        if (g_fq_pool.enable && g_fq_pool.raw == ctx->video_lbuf_mem) {
            g_fq_pool.used = 0;     /* 池块: 只归还池, 不 free(跨模式复用) */
        } else {
            free(ctx->video_lbuf_mem);
        }
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

    /* getIndex=1 必须保留: 试过改 0(想省索引扫描时间), 结果起播耗时没降(play 仍 2360ms),
     * 反而把正常文件判成 CORRUPTED(首帧/损坏检测依赖索引) => 已回退, 勿再改。 */
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


