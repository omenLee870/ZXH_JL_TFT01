/**
 * @file video_player.c
 * @brief 统一视频播放模块 — 封装 app_playback_mpi 硬件解码管线 + LRU 帧缓存
 *
 * 内部基于 VDEC→VPSS→VO→fb 硬件管线,
 * 通过 fb_data_updata 回调驱动帧就绪通知。
 * 帧缓存基于 LRU 策略, 缓存数量可动态配置。
 */
#include "system/includes.h"
#include "app_config.h"
#include "video_player.h"
#include "playback/app_playback_mpi.h"
#include "playback/app_playback_mpi_priv.h"
#include "gpu_port.h"
#include "generic/list.h"
#include "system/spinlock.h"
#include "os/os_api.h"

#define LOG_TAG             "[VIDEO_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* ------------------------------------------------------------------*/
/* 常量 */
/* ------------------------------------------------------------------*/
#define VIDEO_PLAYER_FB_NAME_PREFIX   "fb"
#define VIDEO_PLAYER_FB_NUM           2
#define VIDEO_PLAYER_GPU_FORMAT       GPU_FORMAT_YUV420_BT601
#define VIDEO_PLAYER_CACHE_KEY_MAX    32

/* ------------------------------------------------------------------*/
/* 内存申请封装: PSRAM 优先, 否则用标准 malloc */
/* ------------------------------------------------------------------*/
#if TCFG_PSRAM_DEV_ENABLE
#define vp_malloc(s)   malloc_psram(s)
#define vp_zalloc(s)   ({ void *_p = malloc_psram(s); if (_p) memset(_p, 0, s); _p; })
#define vp_free(p)     free_psram(p)
#else
#define vp_malloc(s)   malloc(s)
#define vp_zalloc(s)   zalloc(s)
#define vp_free(p)     free(p)
#endif

/* ------------------------------------------------------------------*/
/* LRU 帧缓存 */
/* ------------------------------------------------------------------*/
struct vp_cache_node {
    struct list_head entry;
    u32 hash;               /* key hash */
    u8 *data;
    u32 data_len;
    u16 width;
    u16 height;
    u8  format;
};

struct vp_cache_mgr {
    struct list_head head;
    int count;
    int max;                /* 可动态配置的最大数量 */
    spinlock_t lock;
};

/* ------------------------------------------------------------------*/
/* 核心数据结构 */
/* ------------------------------------------------------------------*/
struct video_player_ctx {
    struct list_head entry;                 /* 全局链表节点 */
    app_playback_handle_t pb_handle;
    video_player_mode_t mode;
    video_player_audio_t audio;
    int fb_index;
    int width;
    int height;
    char fb_name[8];
    char path[128];
    u32 file_hash;                          /* 当前文件路径的 djb2 hash (用于缓存 key 去耦, 同一文件切换 path 后帧仍可定位) */
    u32 frame_size;
    int frame_cnt;                          /* 帧计数器 (作为缓存 key 的一部分) */
    struct vp_cache_mgr cache;              /* LRU 帧缓存管理器 */
    volatile int started;
    volatile int paused;
    int loop_interval_ms;                   /* LOOP 循环间隔(ms), 0=立即重播 */
    u16 loop_restart_timer;                 /* 间隔定时器句柄 */
    int loop_count;                         /* LOOP 循环次数 */
    volatile int closing;                   /* 正在关闭标记 (防止定时器回调 use-after-free) */
    video_player_frame_cb_t frame_cb;
};

/* ------------------------------------------------------------------*/
/* 异步回调 payload: 避免 ctx 中 seq 被重入覆盖                          */
/* ------------------------------------------------------------------*/
#define VP_CB_POOL_SIZE 16

struct vp_cb_payload {
    struct video_player_ctx *ctx;
    u32 seq;
    u32 post_time;
};

static struct vp_cb_payload g_cb_pool[VP_CB_POOL_SIZE];
static volatile u32 g_cb_seq;              /* 全局投递序号 */
static volatile int g_cb_pool_idx;         /* pool 写指针 */
static volatile int g_cb_pending;          /* 未处理回调数 */

/* ------------------------------------------------------------------*/
/* 全局 player 链表 */
/* ------------------------------------------------------------------*/
static struct list_head g_player_list = LIST_HEAD_INIT(g_player_list);
static struct video_player_ctx *g_player_by_fb[8];

/* ==================================================================*/
/* 内部: LRU 缓存操作                                                */
/* ==================================================================*/

static u32 vp_cache_calc_hash(const char *key)
{
    u32 hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + (u32)c;
    }
    return hash;
}

static struct vp_cache_node *vp_cache_find(struct vp_cache_mgr *mgr, const char *key)
{
    u32 hash = vp_cache_calc_hash(key);
    struct vp_cache_node *node, *n;
    struct vp_cache_node *found = NULL;

    spin_lock(&mgr->lock);
    list_for_each_entry_safe(node, n, &mgr->head, entry) {
        if (node->hash == hash) {
            list_del(&node->entry);
            list_add(&node->entry, &mgr->head);
            found = node;
            break;
        }
    }
    spin_unlock(&mgr->lock);
    return found;
}

static int vp_cache_add(struct vp_cache_mgr *mgr, const char *key,
                        u8 *data, u32 data_len,
                        u16 width, u16 height, u8 format)
{
    struct vp_cache_node *node;
    struct vp_cache_node *old;

    node = vp_zalloc(sizeof(*node));
    if (!node) {
        return -1;
    }
    node->data = vp_malloc(data_len);
    if (!node->data) {
        vp_free(node);
        return -1;
    }
    memcpy(node->data, data, data_len);
    node->data_len = data_len;
    node->width    = width;
    node->height   = height;
    node->format   = format;
    node->hash     = vp_cache_calc_hash(key);

    spin_lock(&mgr->lock);
    while (mgr->count >= mgr->max && mgr->max > 0) {
        if (list_empty(&mgr->head)) {
            break;
        }
        old = list_entry(mgr->head.prev, struct vp_cache_node, entry);
        list_del(&old->entry);
        mgr->count--;
        spin_unlock(&mgr->lock);
        if (old->data) {
            vp_free(old->data);
        }
        vp_free(old);
        spin_lock(&mgr->lock);
    }
    list_add(&node->entry, &mgr->head);
    mgr->count++;
    spin_unlock(&mgr->lock);
    return 0;
}

static void vp_cache_clear(struct vp_cache_mgr *mgr)
{
    struct vp_cache_node *node, *n;

    spin_lock(&mgr->lock);
    list_for_each_entry_safe(node, n, &mgr->head, entry) {
        list_del(&node->entry);
        spin_unlock(&mgr->lock);
        if (node->data) {
            vp_free(node->data);
        }
        vp_free(node);
        spin_lock(&mgr->lock);
    }
    mgr->count = 0;
    spin_unlock(&mgr->lock);
}

static void vp_cache_init(struct vp_cache_mgr *mgr)
{
    INIT_LIST_HEAD(&mgr->head);
    mgr->count = 0;
    mgr->max   = 10;  /* 默认 10 帧 */
    spin_lock_init(&mgr->lock);
}

/*
 * 计算文件路径的 djb2 hash, 生成与 path 解耦的唯一标识。
 * 用于帧缓存 key, 确保同一文件的帧在不同 path 切换后仍可通过 hash 定位。
 */
static u32 vp_calc_file_hash(const char *path)
{
    u32 hash = 5381;
    int c;
    while ((c = *path++)) {
        hash = ((hash << 5) + hash) + (u32)c;
    }
    return hash;
}

/*
 * 根据 file_hash + frame_cnt 构造缓存 key。
 * key 格式: "hash值_帧序号", 例如 "1a2b3c4d_1"
 * 缓冲区最小需要 20 字节 (u32 hex 8位 + _ + int 最多10位).
 */
static void vp_cache_get_key(struct video_player_ctx *ctx, char *key, int size,
                             u32 file_hash, int frame_cnt)
{
    snprintf(key, size, "%08x_%d", (unsigned int)file_hash, frame_cnt);
}

static void vp_cache_get_latest_key(struct video_player_ctx *ctx, char *key, int size)
{
    vp_cache_get_key(ctx, key, size, ctx->file_hash, ctx->frame_cnt);
}

/* ==================================================================*/
/* 内部: LOOP 间隔定时器回调                                          */
/* ==================================================================*/

static void video_player_loop_restart_cb(void *priv)
{
    struct video_player_ctx *ctx = (struct video_player_ctx *)priv;
    if (!ctx) {
        return;
    }

    /* 防御 use-after-free: ctx 正在被 close 释放 */
    if (ctx->closing || !ctx->pb_handle) {
        if (ctx->loop_restart_timer) {
            ctx->loop_restart_timer = 0;
        }
        return;
    }

    /* 删除一次性定时器 (从定时器回调中安全删除自身) */
    if (ctx->loop_restart_timer) {
        u16 t = ctx->loop_restart_timer;
        ctx->loop_restart_timer = 0;
        sys_timer_del(t);
    }

    log_info("%s: restart after interval=%dms, loop=%d\n",
             __func__, ctx->loop_interval_ms, ctx->loop_count);

    /* 关闭 demux, 强制 video_start 走 fresh demux_open 路径,
     * 使 OPENED 事件在音频线程创建前触发, has_audio=0 才能生效.
     * 详见 FINISHED handler 中同名注释. */
    {
        struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
        if (pb->video_thread_pid) {
            thread_kill(&pb->video_thread_pid, KILL_WAIT);
            pb->video_thread_pid = 0;
        }
        app_playback_demux_frame_queue_close(pb);
        app_playback_demux_close(pb);
        pb->video_started = 0;
        pb->video_thread_exit_flag = 0;
        pb->demux.info.has_audio = 0;
    }

    app_playback_ret_t ret = app_playback_video_start(ctx->pb_handle, ctx->path);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: LOOP delayed restart failed\n", __func__);
        ctx->started = 0;
        return;
    }

    ctx->started = 1;
    ctx->paused = 0;

    /* AUDIO_OFF: 立即杀死 video_start 创建的 play_aud 线程 */
    if (ctx->audio == VIDEO_PLAYER_AUDIO_OFF) {
        struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
        pb->demux.info.has_audio = 0;
        if (pb->audio_thread_pid) {
            pb->audio_thread_exit_flag = 1;
            thread_kill(&pb->audio_thread_pid, KILL_WAIT);
            pb->audio_thread_pid = 0;
            pb->audio_started = 0;
            pb->audio_thread_exit_flag = 0;
            log_info("%s: audio killed immediately after delayed LOOP\n", __func__);
        }
    }
}

/* ==================================================================*/
/* 内部: 帧数据分发                                                  */
/* ==================================================================*/

static void video_player_deliver_frame(struct video_player_ctx *ctx, u8 *baddr)
{
    if (!ctx || !baddr) {
        return;
    }
    if (ctx->paused) {
        return;
    }

    if (!ctx->frame_size) {
        ctx->frame_size = ctx->width * ctx->height * 3 / 2;
    }

    ctx->frame_cnt++;

    char key[VIDEO_PLAYER_CACHE_KEY_MAX];
    vp_cache_get_latest_key(ctx, key, sizeof(key));

    vp_cache_add(&ctx->cache, key, baddr, ctx->frame_size,
                 ctx->width, ctx->height, VIDEO_PLAYER_GPU_FORMAT);

    if (ctx->frame_cb) {
        /* 跳帧: 当 UI 积压超过阈值时停止投递, 防止 taskq 雪崩 */
        if (g_cb_pending >= VIDEO_PLAYER_MAX_PENDING_CB) {
            return;
        }
        g_cb_pending++;

        /* 从 pool 取一个 payload, 避免 ctx 字段被重入覆盖 */
        struct vp_cb_payload *p = &g_cb_pool[g_cb_pool_idx++ % VP_CB_POOL_SIZE];
        p->ctx = ctx;
        p->seq = ++g_cb_seq;
        p->post_time = jiffies_msec();

        int msg[3] = {0};
        msg[0] = (int)ctx->frame_cb;
        msg[1] = 1;
        msg[2] = (int)p;          /* 回调用 payload 指针代替 ctx */
        os_taskq_post_type("ui", Q_CALLBACK, 3, msg);
    }
}

/* ==================================================================*/
/* 内部: playback 事件回调                                           */
/* ==================================================================*/

static void video_player_event_handler(void *priv, int argc, int *argv)
{
    struct video_player_ctx *ctx = (struct video_player_ctx *)priv;
    if (!ctx || argc < 1) {
        return;
    }

    app_playback_event_t event = (app_playback_event_t)argv[0];

    switch (event) {
    case APP_PLAYBACK_EVENT_OPENED: {
        log_info("%s: OPENED audio=%s\n", __func__,
                 ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF");
        /*
         * AUDIO_OFF: LOOP 重启时 demux_open 重新解析 AVI header,
         * 将 has_audio 恢复为 1, play_aud 线程在 OPENED 之前已被创建.
         * 此时立即杀死音频线程 (在 DAC 上电前有 ~120ms 窗口),
         * 避免音频管线初始化→销毁的 pop 噪音.
         */
        if (ctx->audio == VIDEO_PLAYER_AUDIO_OFF) {
            struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
            pb->demux.info.has_audio = 0;
            if (pb->audio_thread_pid) {
                pb->audio_thread_exit_flag = 1;
                thread_kill(&pb->audio_thread_pid, KILL_WAIT);
                pb->audio_thread_pid = 0;
                pb->audio_started = 0;
                pb->audio_thread_exit_flag = 0;
                log_info("%s: audio killed in OPENED (AUDIO_OFF)\n", __func__);
            }
        }
    }
    break;
    case APP_PLAYBACK_EVENT_STARTED:
        log_info("%s: STARTED\n", __func__);
        break;
    case APP_PLAYBACK_EVENT_FIRST_FRAME: {
        log_info("%s: FIRST_FRAME audio=%s frame_cnt=%d loop=%d\n",
                 __func__,
                 ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF",
                 ctx->frame_cnt, ctx->loop_count);
        if (ctx->audio == VIDEO_PLAYER_AUDIO_OFF) {
            struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
            if (pb->audio_started) {
                pb->audio_thread_exit_flag = 1;
                if (pb->audio_thread_pid) {
                    thread_kill(&pb->audio_thread_pid, KILL_WAIT);
                    pb->audio_thread_pid = 0;
                }
                pb->audio_started = 0;
                pb->audio_thread_exit_flag = 0;
                pb->demux.info.has_audio = 0;
                log_info("%s: audio stopped (AUDIO_OFF)\n", __func__);
            }
        }
    }
    break;
    case APP_PLAYBACK_EVENT_FINISHED: {
        log_info("%s: FINISHED audio=%s mode=%d frame_cnt=%d loop=%d\n",
                 __func__,
                 ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF",
                 ctx->mode, ctx->frame_cnt, ctx->loop_count);

        if (ctx->mode == VIDEO_PLAYER_MODE_LOOP) {
            /* 先停帧推送 (防止 finish→start 窗口内 fb_data_updata 投递脏帧) */
            ctx->started = 0;
            ctx->loop_count++;

            app_playback_video_finish(ctx->pb_handle);

            /*
             * 关闭 demux, 强制 app_playback_video_start 执行 fresh demux_open.
             * 否则 demux 已打开, video_start 内部创建音频线程后才触发 OPENED,
             * has_audio=0 设置过晚导致 play_aud/jlstream/DAC 被创建→初始化→立即销毁,
             * 产生 pop 噪音并浪费 CPU.
             *
             * 关闭 demux 后 video_start 走 fresh 路径:
             *   demux_open → OPENED(has_audio=0) → 不创建音频线程 ✓
             */
            {
                struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
                if (pb->video_thread_pid) {
                    thread_kill(&pb->video_thread_pid, KILL_WAIT);
                    pb->video_thread_pid = 0;
                }
                app_playback_demux_frame_queue_close(pb);
                app_playback_demux_close(pb);
                pb->video_started = 0;
                pb->video_thread_exit_flag = 0;
                /* 提前设 has_audio=0: OPENED 中还会再设, 双保险 */
                pb->demux.info.has_audio = 0;
            }

            /* 间隔模式: 停帧推送, 等定时器触发后重启 */
            if (ctx->loop_interval_ms > 0) {
                log_info("%s: LOOP waiting %dms before restart (loop=%d)\n",
                         __func__, ctx->loop_interval_ms, ctx->loop_count);
                ctx->loop_restart_timer = sys_timer_add(ctx,
                                                        video_player_loop_restart_cb, ctx->loop_interval_ms);
                if (ctx->loop_restart_timer) {
                    break;  /* 定时器创建成功, 等待回调触发重启 */
                }
                /* 定时器分配失败: 降级为立即重启, 避免播放永久卡住 */
                log_error("%s: timer alloc failed, fallback to immediate restart\n",
                          __func__);
                ctx->loop_restart_timer = 0;
            }

            /* 立即重启 (或定时器分配失败降级) */
            log_info("%s: LOOP immediate restart (loop=%d)\n",
                     __func__, ctx->loop_count);

            app_playback_ret_t ret = app_playback_video_start(ctx->pb_handle, ctx->path);
            if (ret != APP_PLAYBACK_OK) {
                log_error("%s: LOOP restart failed\n", __func__);
                break;
            }

            ctx->started = 1;
            ctx->paused = 0;

            /*
             * AUDIO_OFF: demux 重新打开后 has_audio=1, play_aud 线程已被创建.
             * 在音频管线初始化之前 (DAC 上电前有 ~100ms 窗口) 立即杀死音频线程,
             * 避免 DAC 上电→下电的 pop 噪音.
             */
            if (ctx->audio == VIDEO_PLAYER_AUDIO_OFF) {
                struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
                pb->demux.info.has_audio = 0;
                if (pb->audio_thread_pid) {
                    pb->audio_thread_exit_flag = 1;
                    thread_kill(&pb->audio_thread_pid, KILL_WAIT);
                    pb->audio_thread_pid = 0;
                    pb->audio_started = 0;
                    pb->audio_thread_exit_flag = 0;
                    log_info("%s: audio killed immediately after LOOP restart\n", __func__);
                }
            }
        } else {
            ctx->started = 0;
        }
    }
    break;
    case APP_PLAYBACK_EVENT_ERROR:
        log_error("%s: ERROR audio=%s\n", __func__,
                  ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF");
        ctx->started = 0;
        break;
    default:
        break;
    }
}

/* ==================================================================*/
/* 公开: 生命周期                                                    */
/* ==================================================================*/

video_player_handle_t video_player_open(const char *path,
                                        video_player_mode_t mode,
                                        video_player_audio_t audio,
                                        int fb_index,
                                        int width,
                                        int height)
{
    if (!path || fb_index < 0 || fb_index >= 8) {
        log_error("%s: invalid arg\n", __func__);
        return NULL;
    }

    struct video_player_ctx *ctx = zalloc(sizeof(*ctx));
    if (!ctx) {
        log_error("%s: zalloc ctx failed\n", __func__);
        return NULL;
    }

    ctx->mode = mode;
    ctx->audio = audio;
    ctx->fb_index = fb_index;
    ctx->width = width;
    ctx->height = height;
    ctx->frame_cnt = 0;
    ctx->loop_count = 0;
    ctx->loop_interval_ms = 0;
    ctx->loop_restart_timer = 0;
    ctx->file_hash = vp_calc_file_hash(path);
    strncpy(ctx->path, path, sizeof(ctx->path) - 1);
    ctx->path[sizeof(ctx->path) - 1] = '\0';

    vp_cache_init(&ctx->cache); /* 默认 max=10 */

    snprintf(ctx->fb_name, sizeof(ctx->fb_name), "%s%d", VIDEO_PLAYER_FB_NAME_PREFIX, fb_index);

    app_playback_display_attr_t display_attr = {
        .fb_name = ctx->fb_name,
        .fb_num  = VIDEO_PLAYER_FB_NUM,
        .width   = width,
        .height  = height,
        .left    = 0, .top = 0, .rotate = 0, .mirror = 0,
    };

    app_playback_ret_t ret = app_playback_open(&ctx->pb_handle, &display_attr);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: app_playback_open failed\n", __func__);
        free(ctx);
        return NULL;
    }

    ret = app_playback_register_event_handler(ctx->pb_handle, ctx, video_player_event_handler);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: register event handler failed\n", __func__);
        app_playback_close(&ctx->pb_handle);
        free(ctx);
        return NULL;
    }

    list_add_tail(&ctx->entry, &g_player_list);
    g_player_by_fb[fb_index] = ctx;

    log_info("%s: opened path=%s mode=%d audio=%s fb=%s w=%d h=%d cache_max=%d\n",
             __func__, path, mode,
             audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF",
             ctx->fb_name, width, height, ctx->cache.max);
    return ctx;
}

void video_player_close(video_player_handle_t h)
{
    if (!h) {
        return;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    ctx->closing = 1;
    video_player_stop(h);
    vp_cache_clear(&ctx->cache);

    if (ctx->pb_handle) {
        app_playback_close(&ctx->pb_handle);
        ctx->pb_handle = NULL;
    }

    if (g_player_by_fb[ctx->fb_index] == ctx) {
        g_player_by_fb[ctx->fb_index] = NULL;
    }
    list_del(&ctx->entry);

    free(ctx);
    log_info("%s: closed\n", __func__);
}

/* ==================================================================*/
/* 公开: 缓存配置                                                    */
/* ==================================================================*/

void video_player_set_cache_max(video_player_handle_t h, int max)
{
    if (!h || max < 1) {
        return;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;
    ctx->cache.max = max;
    log_info("%s: cache_max=%d\n", __func__, max);
}

/* ==================================================================*/
/* 公开: 循环间隔配置                                                */
/* ==================================================================*/

void video_player_set_loop_interval(video_player_handle_t h, int interval_ms)
{
    if (!h || interval_ms < 0) {
        return;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;
    ctx->loop_interval_ms = interval_ms;
    log_info("%s: loop_interval=%dms\n", __func__, interval_ms);
}

int video_player_get_loop_interval(video_player_handle_t h)
{
    if (!h) {
        return 0;
    }
    return ((struct video_player_ctx *)h)->loop_interval_ms;
}

void video_player_cb_consumed(void)
{
    if (g_cb_pending > 0) {
        g_cb_pending--;
    }
}

/* ==================================================================*/
/* 公开: 播放控制                                                    */
/* ==================================================================*/

void video_player_set_audio(video_player_handle_t h, video_player_audio_t audio)
{
    if (!h) {
        return;
    }
    ((struct video_player_ctx *)h)->audio = audio;
    log_info("%s: audio=%s\n", __func__,
             audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF");
}

int video_player_start(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    if (!ctx->pb_handle) {
        log_error("%s: not opened\n", __func__);
        return -1;
    }
    if (ctx->started) {
        return 0;
    }

    app_playback_ret_t ret = app_playback_video_start(ctx->pb_handle, ctx->path);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: video_start failed\n", __func__);
        return -1;
    }

    ctx->closing = 0;  /* 复位关闭标记, 允许 LOOP 间隔重启 */
    ctx->started = 1;
    ctx->paused = 0;
    log_info("%s: started audio=%s path=%s\n", __func__,
             ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF",
             ctx->path);
    return 0;
}

int video_player_stop(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    if (!ctx->pb_handle || !ctx->started) {
        /* 清理可能残留的间隔定时器 */
        if (ctx->loop_restart_timer) {
            sys_timer_del(ctx->loop_restart_timer);
            ctx->loop_restart_timer = 0;
        }
        return 0;
    }

    /* 清理间隔定时器 */
    if (ctx->loop_restart_timer) {
        sys_timer_del(ctx->loop_restart_timer);
        ctx->loop_restart_timer = 0;
    }

    /* 立即清空回调, 阻止 deliver_frame 继续向 UI 任务队列投递 Q_CALLBACK.
     * 已排队但未处理的回调将由调用方 frame_cb 中的 _dial_video_player NULL 检查跳过. */
    ctx->frame_cb = NULL;

    app_playback_video_finish(ctx->pb_handle);
    app_playback_image_pipeline_close(ctx->pb_handle);
    ctx->started = 0;
    ctx->paused = 0;
    log_info("%s: stopped\n", __func__);
    return 0;
}

int video_player_stop_keep_pipeline(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    if (!ctx->pb_handle || !ctx->started) {
        return 0;
    }

    /* 仅设退出标志, 不等待线程退出.
     * 线程会在 demux EOF 或睡眠唤醒后自然退出,
     * 真正的 thread_kill(KILL_WAIT) 延迟到下一个 play_file 调用 */
    struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;
    pb->video_thread_exit_flag = 1;
    app_playback_demux_stop(pb);
    app_playback_demux_frame_wakeup(pb);
    /* 音频已在 AUDIO_OFF 模式跳过, 无需 app_playback_audio_stop */
    ctx->started = 0;
    ctx->paused = 0;
    log_info("%s: threads signaled (non-blocking)\n", __func__);
    return 0;
}

int video_player_play_file(video_player_handle_t h, const char *path)
{
    if (!h || !path) {
        return -1;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;
    struct app_playback_ctx *pb = (struct app_playback_ctx *)ctx->pb_handle;

    if (!ctx->pb_handle) {
        log_error("%s: not opened\n", __func__);
        return -1;
    }

    /* 计算文件 hash (基于完整路径, 用于缓存 key 去耦) */
    u32 file_hash = vp_calc_file_hash(path);

    /* Step 1: 检查缓存 -- 查找 {hash}_1 (首帧 key, deliver_frame 中 frame_cnt++ 后为 1) */
    char cache_key[VIDEO_PLAYER_CACHE_KEY_MAX];
    snprintf(cache_key, sizeof(cache_key), "%08x_1", (unsigned int)file_hash);
    struct vp_cache_node *cached = vp_cache_find(&ctx->cache, cache_key);

    if (cached && cached->data) {
        /* 缓存命中: 将帧以 {hash}_0 为 key 重新插入 LRU,
         * 后续 get_frame_data (frame_cnt=0) 可找到此帧。
         * 跳过 demux close / decode, 直接返回 1 通知调用方帧已就绪。 */
        log_info("%s: cache HIT, hash=%08x, path=%s\n", __func__, (unsigned int)file_hash, path);

        /* 更新 path 和 hash (保持一致性) */
        strncpy(ctx->path, path, sizeof(ctx->path) - 1);
        ctx->path[sizeof(ctx->path) - 1] = '\0';
        ctx->file_hash = file_hash;
        ctx->frame_cnt = 0;

        /* 将缓存的帧数据以 {hash}_0 为 key 重新加入缓存,
         * 使 frame_cnt=0 的 get_frame_data 能命中 */
        char hit_key[VIDEO_PLAYER_CACHE_KEY_MAX];
        snprintf(hit_key, sizeof(hit_key), "%08x_0", (unsigned int)file_hash);
        vp_cache_add(&ctx->cache, hit_key, cached->data, cached->data_len,
                     cached->width, cached->height, cached->format);

        /* 不启动解码, ctx->started 保持原值,
         * stop_keep_pipeline 会因为 !started 而安全返回 0 */
        return 1;
    }

    /* Step 2: 缓存未命中 -- 走正常解码流程 */
    log_info("%s: cache MISS, hash=%08x, path=%s, will decode\n", __func__, (unsigned int)file_hash, path);

    /* 等待上一个文件的线程退出 (已通过 stop_keep_pipeline 设 exit_flag,
     * 线程通常在 ~5ms 内因 EOF 自然退出, 此时 kill 瞬时返回) */
    if (pb->video_thread_pid) {
        thread_kill(&pb->video_thread_pid, KILL_WAIT);
        pb->video_thread_pid = 0;
    }
    app_playback_demux_frame_queue_close(pb);
    app_playback_demux_close(pb);
    pb->video_started = 0;
    pb->video_thread_exit_flag = 0;

    /* 更新路径和 hash */
    strncpy(ctx->path, path, sizeof(ctx->path) - 1);
    ctx->path[sizeof(ctx->path) - 1] = '\0';
    ctx->file_hash = file_hash;
    ctx->frame_cnt = 0;

    /* AUDIO_OFF: 提前标记无音频 (OPENED 事件中还会再设, 双保险) */
    if (ctx->audio == VIDEO_PLAYER_AUDIO_OFF) {
        pb->demux.info.has_audio = 0;
    }

    app_playback_ret_t ret = app_playback_video_start(ctx->pb_handle, ctx->path);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: start failed\n", __func__);
        return -1;
    }

    ctx->started = 1;
    ctx->paused = 0;
    log_info("%s: playing audio=%s path=%s\n", __func__,
             ctx->audio == VIDEO_PLAYER_AUDIO_ON ? "ON" : "OFF",
             path);
    return 0;
}

int video_player_pause(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    ((struct video_player_ctx *)h)->paused = 1;
    return 0;
}

int video_player_resume(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    ((struct video_player_ctx *)h)->paused = 0;
    return 0;
}

/* ==================================================================*/
/* 公开: 首帧解码                                                    */
/* ==================================================================*/

int video_player_decode_first_frame(video_player_handle_t h)
{
    if (!h) {
        return -1;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    if (!ctx->pb_handle) {
        return -1;
    }

    int prev_cnt = ctx->frame_cnt;
    app_playback_ret_t ret = app_playback_video_start(ctx->pb_handle, ctx->path);
    if (ret != APP_PLAYBACK_OK) {
        log_error("%s: start failed\n", __func__);
        return -1;
    }
    ctx->started = 1;

    int timeout = 200;
    while (ctx->frame_cnt == prev_cnt && timeout > 0) {
        os_time_dly(1);
        timeout--;
    }
    if (ctx->frame_cnt == prev_cnt) {
        log_error("%s: first frame timeout\n", __func__);
        video_player_stop(h);
        return -1;
    }

    app_playback_video_finish(ctx->pb_handle);
    app_playback_image_pipeline_close(ctx->pb_handle);
    ctx->started = 0;
    log_info("%s: first frame ready\n", __func__);
    return 0;
}

/* ==================================================================*/
/* 公开: 状态查询                                                    */
/* ==================================================================*/

int video_player_is_playing(video_player_handle_t h)
{
    if (!h) {
        return 0;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;
    return ctx->started && !ctx->paused;
}

int video_player_is_paused(video_player_handle_t h)
{
    if (!h) {
        return 0;
    }
    return ((struct video_player_ctx *)h)->paused;
}

int video_player_get_width(video_player_handle_t h)
{
    if (!h) {
        return 0;
    }
    return ((struct video_player_ctx *)h)->width;
}

int video_player_get_height(video_player_handle_t h)
{
    if (!h) {
        return 0;
    }
    return ((struct video_player_ctx *)h)->height;
}

/* ==================================================================*/
/* 公开: 帧数据获取 (从 LRU 缓存取最新帧)                            */
/* ==================================================================*/

u8 *video_player_get_frame_data(video_player_handle_t h)
{
    if (!h) {
        return NULL;
    }
    struct video_player_ctx *ctx = (struct video_player_ctx *)h;

    char key[VIDEO_PLAYER_CACHE_KEY_MAX];
    vp_cache_get_latest_key(ctx, key, sizeof(key));
    struct vp_cache_node *node = vp_cache_find(&ctx->cache, key);
    return node ? node->data : NULL;
}

int video_player_get_frame_format(video_player_handle_t h)
{
    return VIDEO_PLAYER_GPU_FORMAT;
}

/* ==================================================================*/
/* 公开: 回调注册                                                    */
/* ==================================================================*/

void video_player_set_frame_callback(video_player_handle_t h, video_player_frame_cb_t cb)
{
    if (!h) {
        return;
    }
    ((struct video_player_ctx *)h)->frame_cb = cb;
}

/* ==================================================================*/
/* 公开: fb_data_updata 集成                                         */
/* ==================================================================*/

int video_player_fb_handler(u8 fb_index, u8 *baddr)
{
    if (fb_index >= 8) {
        return 0;
    }

    struct video_player_ctx *ctx = g_player_by_fb[fb_index];
    if (!ctx || !ctx->started) {
        return 0;
    }

    video_player_deliver_frame(ctx, baddr);
    return 1;
}
