#include "app_config.h"
#include "system/spinlock.h"
#include "os/os_api.h"
#include "task.h"
#include "lbuf.h"
#include "spp_transfer.h"
#include "vstream_common.h"
#include "kid_wt_app.h"
#include "crc.h"

#if 1
#define LOG_TAG_CONST      	VSTREAM_SENDER
//#define LOG_TAG             "[KWT_VSTREAM_SEND]"
#define LOG_ERROR_ENABLE
//#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else
#define log_info(...)
#define log_debug(...)
#define log_error(...)
#endif
#include "kid_mem_dbg.h"

#if TCFG_VSTREAM_TRANSFER



struct lbuf_data_head {
    int len;
    u8 type;
    u8 data[0];
};
struct lbuf_manager {
    struct lbuff_head *hd;
    u8 *ptr;
    OS_MUTEX lock;
};
struct stream_info {
    u8 task_exit;
    struct lbuf_manager audio;
    struct lbuf_manager video;
    struct lbuf_manager file;
    struct lbuf_manager cmd;

    u8 *udp_send_buf;  //udp发送缓冲
    u32 seq;//seq自加
    /* struct lbuf_manager audio; */
    /* struct lbuf_manager video; */
    /* struct lbuf_manager file; */
    /* struct lbuf_manager cmd; */
};

static struct stream_info *g_stream_info = NULL;

/* ============================================================================
 * Transfer instrumentation: real (success-only) send FPS + per-stage timing.
 *
 * The send task drains cmd/audio/video/file lbufs and hands each fragment to
 * spp_send.  Per-fragment wall time is accumulated and dumped once per second
 * from the send task (success-only: a video frame is counted only when every
 * fragment reached spp_send).  frame_ms = avg wall time per video frame;
 * pkt_ms = avg wall time per fragment; spp_ms = avg time inside spp_send.
 * ============================================================================ */
struct send_stats {
    u32 video_frames;   /* video frames fully written (all fragments sent) */
    u32 video_bytes;
    u32 video_frags;    /* fragments across those video frames */
    u32 write_ms;       /* wall time inside vstream_write_data (video) */
    u32 spp_ms;         /* wall time inside spp_send */
    u32 frags_all;      /* total fragments sent, all types */
    u32 last_dump_ms;
};
static struct send_stats g_send_stats;

static void send_stats_dump(void)
{
    u32 now = jiffies_msec();
    u32 vf, nf, na;

    if (jiffies_msec2offset(g_send_stats.last_dump_ms, now) < 1000) {
        return;
    }
    vf = g_send_stats.video_frames ? g_send_stats.video_frames : 1;
    nf = g_send_stats.video_frags  ? g_send_stats.video_frags  : 1;
    na = g_send_stats.frags_all     ? g_send_stats.frags_all     : 1;
    log_info("[SENDSTAT] fps=%u B/s=%u frame_ms=%u pkt_ms=%u spp_ms=%u pkt/s=%u",
             g_send_stats.video_frames,
             g_send_stats.video_bytes,
             g_send_stats.write_ms / vf,
             g_send_stats.write_ms / nf,
             g_send_stats.spp_ms / na,
             g_send_stats.frags_all);
    g_send_stats.video_frames = 0;
    g_send_stats.video_bytes  = 0;
    g_send_stats.video_frags  = 0;
    g_send_stats.write_ms     = 0;
    g_send_stats.spp_ms       = 0;
    g_send_stats.frags_all    = 0;
    g_send_stats.last_dump_ms = now;
}

static int vstream_write_data(struct stream_info *info, u8 *buffer, size_t len, u8 type);

extern u32 get_bredr_tx_remain_size();

static int vstream_lbuf_manager_is_init(struct lbuf_manager *lm)
{
    if (lm && lm->ptr) {
        return true;
    }
    /* log_error("%s lm%x ptr:%x",__func__,lm,lm->ptr); */
    return false;
}

static int vstream_lbuf_manager_init(struct lbuf_manager *lm, int size)
{
    if (!lm) {
        log_error("%s hd null\n", __func__);
        return -1;
    }
    if (vstream_lbuf_manager_is_init(lm)) {
        log_error("%s is init\n", __func__);
        return -2;
    }
    os_mutex_create(&lm->lock);
    os_mutex_pend(&lm->lock, 0);
    if (!lm->ptr) {
        lm->ptr = malloc(size);
        if (lm->ptr) {
            memset(lm->ptr, 0, size);   /* malloc is not zeroed */
        }
    }
    if (!lm->ptr) {
        log_error("%s malloc(%d) failed\n", __func__, size);
        KWT_MEM_STATS("sender.lbuf_init malloc fail");
    }
    log_debug("%s %d ptr:%x", __func__, __LINE__, lm->ptr);
    if (lm->ptr) {
        lm->hd = lbuf_init(lm->ptr, size, 4, sizeof(struct lbuf_data_head));
    }
    log_debug("%s %d ptr%x", __func__, __LINE__, lm->ptr);
    os_mutex_post(&lm->lock);
    return 0;
}

static int vstream_lbuf_manager_deinit(struct lbuf_manager *lm)
{
    if (!lm) {
        log_error("%s hd null\n", __func__);
        return -1;
    }
    if (!vstream_lbuf_manager_is_init(lm)) {
        log_error("%s is not init\n", __func__);
        return -2;
    }
    os_mutex_pend(&lm->lock, 0);
    log_debug("%s %d ptr:%x", __func__, __LINE__, lm->ptr);
    if (lm->ptr) {
        free(lm->ptr);
        lm->ptr = NULL;
        lm->hd = NULL;   /* head 由 lbuf_init 落在被释放的 buffer 内, 一并置空避免悬垂 */
    }
    log_debug("%s %d ptr:%x", __func__, __LINE__, lm->ptr);
    os_mutex_post(&lm->lock);
    return 0;
}
static void vstream_lbuf_manaager_lock(struct lbuf_manager *lm)
{
    if (!lm) {
        log_error("%s hd null\n", __func__);
        return ;
    }
    os_mutex_pend(&lm->lock, 0);

}
static void vstream_lbuf_manaager_unlock(struct lbuf_manager *lm)
{
    if (!lm) {
        log_error("%s hd null\n", __func__);
        return ;
    }
    os_mutex_post(&lm->lock);
}
static struct lbuf_manager *vstream_type_get_lbuf_hd(int type)
{
    struct lbuf_manager *lm = NULL;
    struct stream_info *info = (struct stream_info *)g_stream_info;
    switch (type) {
    case VSTREAM_JPEG_TYPE_VIDEO:
    case VSTREAM_H264_TYPE_VIDEO:
        lm = &info->video;
        break;
    case  VSTREAM_PCM_TYPE_AUDIO:
        lm = &info->audio;
        break;
    case  VSTREAM_TYPE_CMD:
        lm = &info->cmd;
        break;
    case  VSTREAM_TYPE_FILE:
        lm = &info->file;
        break;
    default:
        break;
    }
    if (!os_mutex_valid(&lm->lock)) {
        os_mutex_create(&lm->lock);
    }
    if (vstream_lbuf_manager_is_init(lm)) {
        /* log_debug("%s type:%d succ",__func__,type); */
        return lm;
    }
    return NULL;
}

static int len_to_align(int type, int len)
{
    int alloc_len  = 0;
    switch (type) {
    case VSTREAM_JPEG_TYPE_VIDEO:
    case VSTREAM_H264_TYPE_VIDEO:
        alloc_len  = ((len / VIDEO_STREAM_BUF_ALIGN + 1) * VIDEO_STREAM_BUF_ALIGN);
        break;
    case  VSTREAM_PCM_TYPE_AUDIO:
        alloc_len  = ((len / AUDIO_STREAM_BUF_ALIGN + 1) * AUDIO_STREAM_BUF_ALIGN);
        break;
    case  VSTREAM_TYPE_CMD:
        alloc_len  = ((len / CMD_STREAM_BUF_ALIGN + 1) * CMD_STREAM_BUF_ALIGN);
        break;
    case  VSTREAM_TYPE_FILE:
        alloc_len  = ((len / FILE_STREAM_BUF_ALIGN + 1) * FILE_STREAM_BUF_ALIGN);
        break;
    default:
        break;
    }
    return alloc_len;
}
static void vstream_pkg_send_task(void *priv)
{
    int msg[32];
    int ret;
    struct stream_info *info  = priv;

    u8 send_something = 0;
    while (!info->task_exit) {

        /* log_info("%d send_something:%d",  __LINE__, send_something); */
        if (!send_something) {
            ret = os_taskq_pend_timeout(NULL, msg, ARRAY_SIZE(msg), 5);
        }
        send_something = 0;
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            os_task_del_res(OS_TASK_SELF);
        }

        struct lbuf_data_head *lbuf_data  = NULL;
        struct lbuf_manager *lm = vstream_type_get_lbuf_hd(VSTREAM_TYPE_CMD);
        if (lm) {
            lbuf_data = lbuf_pop(lm->hd, BIT(0));
            if (lbuf_data) {
                if (vstream_write_data(info, lbuf_data->data, lbuf_data->len, lbuf_data->type) != lbuf_data->len) {
                    log_error("cmd write data err\n");
                }
                send_something = 1;
                lbuf_free(lbuf_data);
                continue;
            }
        }
        lm = vstream_type_get_lbuf_hd(VSTREAM_PCM_TYPE_AUDIO);
        if (lm) {
            vstream_lbuf_manaager_lock(lm);
            lbuf_data = lbuf_pop(lm->hd, BIT(0));
            vstream_lbuf_manaager_unlock(lm);
            if (lbuf_data) {
                //write (outside the lock: spp_send is slow; do not block the producer)
                if (vstream_write_data(info, lbuf_data->data, lbuf_data->len, lbuf_data->type) != lbuf_data->len) {
                    log_error("audio write data err\n");
                }
                send_something = 1;
                vstream_lbuf_manaager_lock(lm);
                lbuf_free(lbuf_data);
                vstream_lbuf_manaager_unlock(lm);
                continue;
            }
        }
        lm = vstream_type_get_lbuf_hd(VSTREAM_JPEG_TYPE_VIDEO);
        if (lm) {
            vstream_lbuf_manaager_lock(lm);
            lbuf_data = lbuf_pop(lm->hd, BIT(0));
            vstream_lbuf_manaager_unlock(lm);
            if (lbuf_data) {
                u8 *data_ptr = lbuf_data->data;
                u32 remain_len = lbuf_data->len;
#if !KWT_USE_H264
                //head offset (JPEG 8-byte header strip; H264 Annex-B has no such header)
                if (data_ptr[0] != 0xFF || data_ptr[1] != 0xD8) {
                    data_ptr += 8;
                    remain_len -= 8;
                }
#endif
                //write (outside the lock: spp_send is slow; do not block the producer)
                if (vstream_write_data(info, data_ptr, remain_len, lbuf_data->type) != remain_len) {
                    log_error("video write data err\n");
                } else {
                    extern int fps_win_counter_update(u32 now_ms);
                    fps_win_counter_update(jiffies_msec());
                }
                send_something = 1;
                vstream_lbuf_manaager_lock(lm);
                lbuf_free(lbuf_data);
                vstream_lbuf_manaager_unlock(lm);
                continue;
            }
        }
        lm = vstream_type_get_lbuf_hd(VSTREAM_TYPE_FILE);
        if (lm) {
            vstream_lbuf_manaager_lock(lm);
            lbuf_data = lbuf_pop(lm->hd, BIT(0));
            vstream_lbuf_manaager_unlock(lm);
            if (lbuf_data) {
                //write (outside the lock: spp_send is slow; do not block the producer)
                if (vstream_write_data(info, lbuf_data->data, lbuf_data->len, lbuf_data->type) != lbuf_data->len) {
                    struct chat_data_mult_packet *file_head = (struct chat_data_mult_packet *)lbuf_data->data;
                    log_error("file write data err drop:%d\n", file_head->offset);
                }
                send_something = 1;
                vstream_lbuf_manaager_lock(lm);
                lbuf_free(lbuf_data);
                vstream_lbuf_manaager_unlock(lm);
            }
        }
    }
    os_time_dly(-1);
}

int vstream_sender_open(struct vstream_info *s_info)
{
    int flags = 0;

    if (g_stream_info) {
        log_info("stream already opened \n");
        return 0;
    }

    struct stream_info *info = (struct stream_info *)calloc(1, sizeof(struct stream_info));

    if (info == NULL) {
        log_error("%s %d->Error in zalloc()\n", __func__, __LINE__);
        KWT_MEM_STATS("sender.open calloc fail");
        return -1;
    }

    info->udp_send_buf = (u8 *)zalloc(UDP_SEND_BUF_SIZE);
    if (info->udp_send_buf == NULL) {
        log_error("%s %d->Error in zalloc()\n", __func__, __LINE__);
        KWT_MEM_STATS("sender.open udp_buf zalloc fail");
        goto err;
    }
    int cmd_lbuf_size = CMD_STREAM_PACKET_NUM * (CMD_STREAM_BUF_ALIGN + 32) + 64;
    if (vstream_lbuf_manager_init(&info->cmd, cmd_lbuf_size) != 0) {
        log_error("%s %d->Error cmd lbuf init\n", __func__, __LINE__);
        goto err;
    }

    /* 必须在 task_create 之前设置，防止任务立即运行时看到 NULL */
    g_stream_info = info;

    if (task_create(vstream_pkg_send_task, info, "vstream_pkg_send_task")) {
        log_error("%s %d->Error task create\n", __func__, __LINE__);
        g_stream_info = NULL;  /* 回退 */
        goto err;
    }
    log_info("%s succ!\n", __func__);
    KWT_MEM_STATS("sender.open done");
    return 0;
err:

    if (info->udp_send_buf) {
        free(info->udp_send_buf);
    }
    vstream_lbuf_manager_deinit(&info->cmd);
    free(info);

    return -1;
}
int vstream_sender_close(void)
{
    if (!g_stream_info) {
        log_error("stream already close \n");
        return 0;
    }

    struct stream_info *info  = g_stream_info;
    g_stream_info = NULL;  /* 立即置空，防止重入/并发访问 */

    while (OS_TASK_NOT_EXIST != os_task_del_req("vstream_pkg_send_task")) {
        os_taskq_post("vstream_pkg_send_task", 1, 0);
        os_time_dly(1);
    }
    info->task_exit = 1;

    if (info->udp_send_buf) {
        free(info->udp_send_buf);
        info->udp_send_buf = NULL;
    }
    vstream_lbuf_manager_deinit(&info->video);
    vstream_lbuf_manager_deinit(&info->audio);
    vstream_lbuf_manager_deinit(&info->file);
    vstream_lbuf_manager_deinit(&info->cmd);
    free(info);

    log_info("%s succ!\n", __func__);
    KWT_MEM_STATS("sender.close done");
    return 0;
}

/* 重连后复位发送端序号, 与首次上电一致。视频/音频 lbuf 已由
 * kid_wt_video_xsmt_stop/audio_app_stop 里的 *_deinit 重建, 这里只需复位跨会话
 * 单调递增的 seq。在 kwt_stream_start(媒体启动前)调用, 此时无并发发送。 */
void vstream_sender_reset(void)
{
    if (!g_stream_info) {
        return;
    }
    struct stream_info *info = (struct stream_info *)g_stream_info;
    info->seq = 0;
    log_info("%s succ!\n", __func__);
}
int vstream_sender_video_init()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    int video_lbuf_size = VIDEO_STREAM_PACKET_NUM * (VIDEO_STREAM_BUF_ALIGN + 32) + 64;
    vstream_lbuf_manager_init(&info->video, video_lbuf_size);
    log_info("%s succ!\n", __func__);
    KWT_MEM_STATS("sender.video_init done");
    return 0;
}
int vstream_sender_video_deinit()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    vstream_lbuf_manager_deinit(&info->video);
    log_info("%s succ!\n", __func__);
    KWT_MEM_STATS("sender.video_deinit done");
    return 0;
}
int vstream_sender_audio_init()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    int audio_lbuf_size = AUDIO_STREAM_PACKET_NUM * (AUDIO_STREAM_BUF_ALIGN + 32) + 64;
    vstream_lbuf_manager_init(&info->audio, audio_lbuf_size);
    log_info("%s succ!\n", __func__);
    return 0;
}
int vstream_sender_audio_deinit()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    vstream_lbuf_manager_deinit(&info->audio);
    log_info("%s succ!\n", __func__);
    return 0;
}
int vstream_sender_file_init()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    int file_lbuf_size = FILE_STREAM_PACKET_NUM * (FILE_STREAM_BUF_ALIGN + 32) + 64;
    vstream_lbuf_manager_init(&info->file, file_lbuf_size);
    log_info("%s succ!\n", __func__);
    return 0;
}
int vstream_sender_file_deinit()
{
    if (!g_stream_info) {
        log_error("%s info is null\n", __func__);
        return -1;
    }
    struct stream_info *info  = g_stream_info;
    vstream_lbuf_manager_deinit(&info->file);
    log_info("%s succ!\n", __func__);
    return 0;
}
int vstream_send_frame(u8 *buffer, size_t len, u8 type)
{
    /* log_debug("%s type:%d enter\n",__func__,type); */
    struct lbuf_data_head *lbuf_data = NULL;
    u8 timeout = 0;

    if (!g_stream_info) {
        log_error("stream info null \n");
        return -1;
    }
    //对齐处理
    int align = len_to_align(type, len);

    struct stream_info *info = (struct stream_info *)g_stream_info;
    //获取lbuf句柄
    struct lbuf_manager *lm;
    //int retry = 0;
    //do {
    lm = vstream_type_get_lbuf_hd(type);
    if (!lm) {
        log_error("lm is null(type:%d)>\n", type);
        return -1;
    }
    os_mutex_pend(&lm->lock, 0);
    if (vstream_lbuf_manager_is_init(lm)) {
        lbuf_data = lbuf_alloc(lm->hd, align);
        if (!lbuf_data) {
#if KWT_USE_H264
            if (type == VSTREAM_JPEG_TYPE_VIDEO || type == VSTREAM_H264_TYPE_VIDEO) {
#else
            if (type ==  VSTREAM_JPEG_TYPE_VIDEO) {
#endif
                //视频直接丢帧
                os_mutex_post(&lm->lock);
                log_error("%s type:%d lbuf_get_error,drop frame\n", __func__, type);
                return 0;
            }
        } else {
            //获取到buf，保留临界区
            //break;
        }
    }
    //os_time_dly(1);
    //}
    //while (retry-- > 0);

    if (lbuf_data) {
        lbuf_data->len = len;
        lbuf_data->type = type;
        memcpy(lbuf_data->data, buffer, len);
        lbuf_push(lbuf_data, BIT(0));
        //释放lbuf的临界
        os_mutex_post(&lm->lock);
        /* 唤醒发送任务: 生产者只 lbuf_push, 发送任务唯一的阻塞点是 5 tick(50ms) 的
         * os_taskq_pend_timeout, 不唤醒的话新入队的帧要等最多 50ms 才被 drain。
         * post 一个空消息打断 pend 即可, 发送任务不消费 msg 内容。 */
        os_taskq_post("vstream_pkg_send_task", 1, 0);
        log_debug("%s type:%d succ\n", __func__, type);
        return len;
    } else {
        log_error("%s type:%d lbuf_get_error,drop frame\n", __func__, type);
    }
    return 0;
}


static int vstream_rt_vpkg_write(struct stream_info *info, u8 *buffer, size_t len)
{
    int ret;
    u32 t1;

    static u32 last_msec = 0;
    u32 cur_msec = jiffies_msec();

    /* 只有 TX 缓冲真正吃紧时才做分片限速(10ms 间隔)。有充足余量时全速发送,
     * 否则固定 10ms 限速会把链路钉死在 ~100 pkts/s(实测 ~50, 因为旧的死信用检查
     * 每分片还要做一次 btstack 线程往返)。真正的背压由 spp_send 内的有界等待负责
     * (get_bredr_tx_remain_size < SPP_TX_MIN_RAM_LIMIT 时等待排空)。 */
    if (get_bredr_tx_remain_size() < SPP_TX_THROTTLE_LIMIT
        && jiffies_msec2offset(last_msec, cur_msec) < VSTREAM_SPP_TX_CTRL_INTERVAL) {
        os_time_dly(1);
    }

    t1 = jiffies_msec();
    ret = spp_send(buffer, len);
    g_send_stats.spp_ms += (jiffies_msec() - t1);
    g_send_stats.frags_all++;

    last_msec = jiffies_msec();
    return ret;
}

static int vstream_write_data(struct stream_info *info, u8 *buffer, size_t len, u8 type)
{
    u16 payload_len = 0;
    u32 total_udp_send = 0;
    struct frm_head frame_head = {0};
    int ret;
    u32 t_start = jiffies_msec();
    u32 nfrags = 0;

    frame_head.offset = 0;
    frame_head.frm_sz = len;
    frame_head.type &= ~VSTREAM_LAST_FREG_MAKER;
    frame_head.type |=  type;
    frame_head.seq = info->seq++;
#if VSTREAM_DATA_CRC_CHECK_EN
    frame_head.crc16 = CRC16(buffer, len);
#else
    frame_head.timestamp = 0;
#endif
    u32 remain_len = len;
    frame_head.block_crc = 0;
    while (remain_len) {
        log_debug("remain_len:%d", remain_len);
        if (remain_len < MAX_PAYLOAD) {
            payload_len = remain_len;
            frame_head.type |= VSTREAM_LAST_FREG_MAKER;
        } else {
            payload_len = MAX_PAYLOAD;
        }

        frame_head.payload_size = payload_len;
#if VSTREAM_DATA_CRC_CHECK_EN
        frame_head.block_crc = CRC16(buffer + frame_head.offset, payload_len);
        vsteam_frame_head_dump(&frame_head, __func__, __LINE__);
#endif
        /* put_buf(buffer, 64); */
        memcpy(info->udp_send_buf + total_udp_send, &frame_head, sizeof(struct frm_head));
        total_udp_send += sizeof(struct frm_head);

        memcpy(info->udp_send_buf + total_udp_send, buffer + frame_head.offset, payload_len);

        total_udp_send += payload_len;

        /* spp_send 内部已做有界等待(TX 缓冲不足时等 SPP_TX_WAIT_TIMEOUT_TICKS),
         * 这里不再叠加 retry 循环——否则卡死时 10 次重试 × 100ms 会把唯一的发送任务
         * 阻塞 ~1s/分片, 既拖慢丢帧判定又占住 pkg_send_task。单次调用即可。 */
        ret = vstream_rt_vpkg_write(info, info->udp_send_buf, total_udp_send);
        if (ret != 0) {
            log_error("%s %d error!\n", __func__, len);
            return -1;
        }
        total_udp_send = 0;
        nfrags++;

        remain_len -= payload_len;
        frame_head.offset += payload_len;
    }
    log_info("%s seq=%u len=%d succ!", __func__, frame_head.seq, len);

    /* success-only send stats: only frames whose every fragment reached spp_send */
    if (type == VSTREAM_JPEG_TYPE_VIDEO || type == VSTREAM_H264_TYPE_VIDEO) {
        g_send_stats.video_frames++;
        g_send_stats.video_bytes += len;
        g_send_stats.video_frags += nfrags;
        g_send_stats.write_ms += (jiffies_msec() - t_start);
        send_stats_dump();
    }
    return len;
}
void vsteam_frame_head_dump(struct frm_head *head, const char *func, int line)
{
#if VSTREAM_DATA_CRC_CHECK_EN
    log_debug("%s call:%s line:%d head:0x%x head_size(%u) type:%d res:%d payload_size:%d seq:%d frm_sz:%d offset:%d block_crc:0x%x crc16:0x%x",
              __func__, func, line, (u32)head, (unsigned int)sizeof(struct frm_head), head->type, head->res, head->payload_size, head->seq, head->frm_sz, head->offset, head->block_crc, head->crc16);
#else
    log_debug("%s call:%s line:%d head:0x%x head_size(%u) type:%d res:%d payload_size:%d seq:%d frm_sz:%d offset:%d time:%d",
              __func__, func, line, (u32)head, (unsigned int)sizeof(struct frm_head), head->type, head->res, head->payload_size, head->seq, head->frm_sz, head->offset, head->timestamp);
#endif
}


#endif /*#if TCFG_VSTREAM_TRANSFER*/

