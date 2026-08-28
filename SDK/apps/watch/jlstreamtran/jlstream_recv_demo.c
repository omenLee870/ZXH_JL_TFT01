/**
 * @file    jlstream_recv_demo.c
 * @brief   流媒体 Push 模式接收端实现 (SDK → DEVICE)
 *
 * 与发送端 (jlstream_send_demo.c) 完全解耦，各自注册独立的回调处理不同方向。
 *
 * 架构:
 *   ┌─────────────────────────────────────────────┐
 *   │  BLE 回调上下文 (att_write_callback)          │
 *   │    └─ jlstm_rx_push_stream_data()           │
 *   │         └─ jlstream_recv_demo_data_cb()     │  ← REGISTER_JLSTM_RX_PUSH_OPS
 *   │              malloc + 拷贝 → os_taskq_post   │
 *   └──────────────────┬──────────────────────────┘
 *                      │
 *   ┌──────────────────▼──────────────────────────┐
 *   │  接收任务线程 (jlstm_push_recv_task)         │
 *   │    os_taskq_pend → 组包/校验/显示             │
 *   └─────────────────────────────────────────────┘
 *   ┌─────────────────────────────────────────────┐
 *   │  控制命令响应                                 │
 *   │    jlstream_recv_demo_resp_cb()             │  ← REGISTER_JLSTM_TX_RESPONSE_OPS
 *   │      start(dir=1) → jlstream_recv_demo_init()│
 *   │      stop → jlstream_recv_demo_deinit()      │
 *   └─────────────────────────────────────────────┘
 */

#include "system/includes.h"
#include "stream_media.h"
#include "video_1_decode_api.h"

#define LOG_TAG_CONST       JLSTM_RECV
#define LOG_TAG             "[JLSTM RECV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* ==================== 内部类型定义 ==================== */

/* BLE 回调 → 任务线程的消息包 */
typedef struct {
    /* ── 首包扩展信息 (仅 offset==0 时有效) ── */
    u8  index;
    u32 frame_size;
    u32 timestamp;
    u16 crc;

    /* ── 每包基础信息 ── */
    u8 codec_type;		//编码类型(0x00:jpeg;0x01:H264)
    u8  data_type;
    u16 seq;
    u32 offset;
    u16 payload_len;
    u8  payload[0];  /* 变长负载数据 */
} jlstm_recv_msg_t;

/* 帧组装上下文 */
typedef struct {
    u8  *frame_buf;         /* 当前帧组装缓冲区 (不含header时为纯数据，含header时=header+data) */
    u32  frame_size;        /* 当前帧总大小 (data-only模式 = 纯数据大小; combined模式 = header_size + data_size) */
    u32  recv_total;        /* 已接收数据总量 */
    u16  current_seq;       /* 当前帧序号 */
    u8   active_index;      /* 当前活动的 index */
    u8   active;            /* 当前帧是否活跃 */

    /* JPEG header分离模式 (data_type:0x01 + 0x02) */
    u8  *header_buf;        /* 保存的JPEG头部 */
    u32  header_size;       /* 头部长度 */
    u8   has_header;        /* 是否已收到头部 */

    /* 用于data-only模式的原始数据信息 (CRC计算用) */
    u32  data_frame_size;   /* 原始数据长度 (不含header) */
    u16  data_crc;          /* 原始数据CRC */
} jlstm_recv_frame_ctx_t;

/* h264解码 */
typedef struct {
    void 	  *dec_storage;         /* 解码器实例 */
    u8        *stream_buf;          /* 码流累积缓冲区 */
    u32        stream_capacity;     /* 缓冲区容量 */
    u32        stream_len;          /* 当前累积数据长度(已接收最大偏移) */
    u8         decoder_inited;      /* 解码器是否已初始化 */
    u32        frame_count;         /* 已解码帧计数 */
    u32        pic_width;           /* 当前图像宽度 */
    u32        pic_height;          /* 当前图像高度 */

    /* 当前包数据 */
    u32        pkt_size;
    u16        pkt_crc;
    u16        pkt_head_seq;
    /* 统计 */
    u32        seq_gap_cnt;         /* seq 不连续计数 */
    u32        crc_err_cnt;         /* CRC 校验失败计数 */
    u32        pkt_incomplete_cnt;  /* 收包不完整计数 */

} jlstm_recv_h264_ctx_t;

/* 接收框架句柄 */
typedef struct {
    char                  task_name[32];
    jlstm_recv_frame_ctx_t frame_ctx;			/* jpeg解码上下文 */
    jlstm_recv_h264_ctx_t   h264_ctx;        /* H.264 解码上下文 */
    void (*ui_refresh_cb)(void *arg);   /* UI 刷新回调 */
} jlstm_recv_handle_t;

typedef struct {
    void (*ui_refresh_cb)(void *arg);   /* UI 刷新回调 */
} jlstm_recv_ui;

#define H264_DEBUG_FROM_SD  0
/* ==================== 内部接口声明 ==================== */
//jpeg相关
static void recv_jpeg_feed(jlstm_recv_frame_ctx_t *ctx,
                           jlstm_recv_msg_t     *pkt,
                           u8 dt_header, u8 dt_data, u8 dt_last);

//h264相关
static void *recv_h264_malloc(int size, u32 ram_type, u32 module_type);
static void recv_h264_free(void *buf, u32 ram_type, u32 module_type);
static int recv_h264_init(jlstm_recv_h264_ctx_t *ctx);
static void recv_h264_deinit(jlstm_recv_h264_ctx_t *ctx);
static int recv_h264_dpb_dump(jlstm_recv_h264_ctx_t *ctx);
static int recv_h264_feed(jlstm_recv_h264_ctx_t *ctx,
                          const u8 *data, u32 len,
                          u32 offset, u32 frame_size,
                          u16 seq, u16 crc, u8 is_last);

#if H264_DEBUG_FROM_SD
static void recv_h264_sd_feed_one(jlstm_recv_h264_ctx_t *ctx);
#endif

/* ==================== 模块状态 ==================== */

static jlstm_recv_handle_t *g_recv;
static jlstm_recv_ui recv_ui;

enum {
    Q_JLSTM_RECV_DATA = (Q_USER + 130),
    Q_JLSTM_RECV_KILL,
};

static void jlstm_push_recv_task(void *p);

/* ==================== 帧缓冲区管理 ==================== */

static void jlstream_recv_ui_reflush(void *arg)
{
    if (recv_ui.ui_refresh_cb) {
        recv_ui.ui_refresh_cb(arg);
    }
}

static void recv_abort_frame(void)
{
    if (!g_recv) {
        return;
    }
    jlstm_recv_frame_ctx_t *ctx = &g_recv->frame_ctx;
    if (ctx->frame_buf) {
        free(ctx->frame_buf);
        ctx->frame_buf = NULL;
    }
    ctx->active        = 0;
    ctx->recv_total    = 0;
    ctx->frame_size    = 0;
    ctx->data_frame_size = 0;
    ctx->data_crc      = 0;
    /* 注意: header_buf 不清除，跨帧保留 */
}

int jlstream_recv_demo_init(void);
int jlstream_recv_demo_deinit(void);
int jlstream_recv_ui_refresh_register(void (*cb)(void *arg));

/* ==================== 数据回调 (BLE 上下文) ==================== */

/**
 * @brief AE03 流数据回调 —— 在 BLE att_write_callback 上下文中执行
 *
 * 不做任何阻塞操作，仅拷贝数据并投递消息给接收任务线程。
 */
static void jlstream_recv_demo_data_cb(const jlstm_push_frame_t *frame)
{
    if (!frame || !g_recv) {
        return;
    }

    jlstm_recv_msg_t *msg = (jlstm_recv_msg_t *)malloc(sizeof(*msg) + frame->payload_len);
    if (!msg) {
        log_error("msg alloc fail");
        return;
    }

    msg->index       = frame->index;
    msg->frame_size  = frame->frame_size;
    msg->timestamp   = frame->timestamp;
    msg->crc         = frame->crc;
    msg->codec_type	 = frame->codec_type;
    msg->data_type   = frame->data_type;
    msg->seq         = frame->seq;
    msg->offset      = frame->offset;
    msg->payload_len = frame->payload_len;
    memcpy(msg->payload, frame->payload, frame->payload_len);

    int arg = (int)msg;
    int err = os_taskq_post_type(g_recv->task_name, Q_JLSTM_RECV_DATA, 1, &arg);
    if (err) {
        log_error("post fail: %d", err);
        free(msg);
    }
}

REGISTER_JLSTM_RX_PUSH_OPS(jlstream_recv_demo_rx_ops) = {
    .on_push_data = jlstream_recv_demo_data_cb,
};

/* ==================== 控制命令响应回调 ==================== */

/**
 * @brief 0x37 控制命令响应回调
 *
 * 仅处理 dir=1 (Push 模式) 的 start/stop。
 * dir=0 (Pull 模式) 由 jlstream_send_demo.c 注册的独立回调处理。
 */
static void jlstream_recv_demo_resp_cb(u8 option, u8 type, u8 index, u8 status, u8 num)
{
    u8 dir = (option >> 4) & 0x01;
    u8 base = option & 0x0F;

    /* 只处理 Push 方向 */
    if (dir != 1 && base == JLSTM_OPTION_START_STREAM) {
        return;
    }

    switch (base) {
    case JLSTM_OPTION_START_STREAM:
        if (status == 0x00) {
            log_info("Push start: idx=%d type=0x%02x", index, type);
            if (jlstream_recv_demo_init() == 0 && g_recv) {
                g_recv->frame_ctx.active_index = index;
            }
        } else {
            log_error("Push start fail: idx=%d status=0x%02x", index, status);
        }
        break;

    case JLSTM_OPTION_STOP_STREAM:
        log_info("Push stop: idx=%d", index);
        jlstream_recv_demo_deinit();
        break;

    default:
        break;
    }
}

REGISTER_JLSTM_TX_RESPONSE_OPS(jlstream_recv_demo_tx_ops) = {
    .callback = jlstream_recv_demo_resp_cb,
};

/* ==================== 接收任务 (独立线程) ==================== */

/**
 * @brief 接收任务主循环
 *
 * 支持三种 data_type:
 *   0x03 (JLSTM_DT_HAS_HEADER|JLSTM_DT_HAS_DATA): 完整JPEG，直接显示
 *   0x01 (JLSTM_DT_HAS_HEADER): 仅JPEG头部，保存用于后续拼接
 *   0x02 (JLSTM_DT_HAS_DATA):  仅裸数据，需拼接之前保存的头部
 */
static void jlstm_push_recv_task(void *p)
{
    int msg[8];
    log_info("task started");

    while (1) {
        if (os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) {
        case Q_JLSTM_RECV_DATA: {
            jlstm_recv_msg_t *pkt = (jlstm_recv_msg_t *)msg[1];
            if (!pkt) {
                break;
            }

            jlstm_recv_frame_ctx_t *ctx = &g_recv->frame_ctx;
            u8 dt_header = pkt->data_type & JLSTM_DT_HAS_HEADER;  // Bit0
            u8 dt_data   = pkt->data_type & JLSTM_DT_HAS_DATA;    // Bit1
            u8 dt_last   = pkt->data_type & JLSTM_DT_LAST_FRAG;   // Bit7
            if (pkt->codec_type == 0x00) {
                /* 数据编码类型:jpeg */
                recv_jpeg_feed(ctx, pkt, dt_header, dt_data, dt_last);
            } else if (pkt->codec_type == 0x01) {
                /* 数据编码类型:H264 */
#if H264_DEBUG_FROM_SD
                /* 调试模式: 读SD卡文件, 忽略BLE数据 */
                if (!g_recv->h264_ctx.decoder_inited) {
                    if (recv_h264_init(&g_recv->h264_ctx) != 0) {
                        log_error("h264 init fail");
                        free(pkt);
                        break;
                    }
                }
                recv_h264_sd_feed_one(&g_recv->h264_ctx);
#else
                /* 首次收到 H.264 数据时初始化解码器 */
                if (!g_recv->h264_ctx.decoder_inited) {
                    if (recv_h264_init(&g_recv->h264_ctx) != 0) {
                        log_error("h264 init fail");
                        free(pkt);
                        break;
                    }
                }

                /* 喂入数据 → 组包 → dt_last 校验 CRC+完整性 → 解码 → 送显 */
                int decoded = recv_h264_feed(&g_recv->h264_ctx,
                                             pkt->payload, pkt->payload_len,
                                             pkt->offset, pkt->frame_size,
                                             pkt->seq, pkt->crc,
                                             dt_last ? 1 : 0);

                extern void wdt_clear(void);
                wdt_clear();
                os_time_dly(1);
#endif
            }


            free(pkt);
            break;
        }

        case Q_JLSTM_RECV_KILL:
            log_info("task kill");
            recv_abort_frame();
            /* 清空残留消息，防止内存泄漏 */
            while (os_taskq_accept(ARRAY_SIZE(msg), msg) == OS_TASKQ) {
                if (msg[0] == Q_JLSTM_RECV_DATA && msg[1]) {
                    free((void *)msg[1]);
                }
            }
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(-1);
            return;

        default:
            break;
        }
    }
}
/* ==================== JPEG 图像帧处理  ==================== */

/**
 * @brief 处理 JPEG 图像帧数据
 *
 * 场景A (0x01, header-only): 缓存 JPEG 头部，供后续 data-only 帧拼接
 * 场景B (0x02, data-only): 与缓存头部拼接成完整 JPEG
 * 场景C (0x03, complete):  完整 JPEG 帧 (含头部+数据)
 *
 * @note 不负责 free(pkt)，由调用者统一释放
 */
static void recv_jpeg_feed(jlstm_recv_frame_ctx_t *ctx,
                           jlstm_recv_msg_t     *pkt,
                           u8 dt_header, u8 dt_data, u8 dt_last)
{
    /*  场景A: 仅头部 (0x01)  可能分包发送  */
    if (dt_header && !dt_data) {
        if (pkt->offset == 0) {
            /* 首包: 分配 header_buf */
            if (ctx->header_buf) {
                free(ctx->header_buf);
            }
            ctx->header_buf = (u8 *)malloc(pkt->frame_size);
            if (!ctx->header_buf) {
                log_error("header_buf alloc fail, size=%lu", pkt->frame_size);
                return;
            }
            memcpy(ctx->header_buf, pkt->payload, pkt->payload_len);
            ctx->header_size  = pkt->frame_size;
            ctx->recv_total   = pkt->payload_len;
            ctx->current_seq  = pkt->seq;

            log_info("jpeg header: seq=%d off=%d plen=%d size=%lu",
                     pkt->seq, pkt->offset, pkt->payload_len, ctx->header_size);
        } else {
            /* 续包: 追加到 header_buf */
            if (pkt->offset + pkt->payload_len > ctx->header_size) {
                log_error("header overflow: off=%lu plen=%d hsize=%lu",
                          pkt->offset, pkt->payload_len, ctx->header_size);
                free(ctx->header_buf);
                ctx->header_buf = NULL;
                ctx->header_size = 0;
                ctx->recv_total = 0;
                return;
            }
            memcpy(ctx->header_buf + pkt->offset, pkt->payload, pkt->payload_len);
            ctx->recv_total += pkt->payload_len;

            log_info("jpeg header: seq=%d off=%d plen=%d total=%lu/%lu",
                     pkt->seq, pkt->offset, pkt->payload_len,
                     ctx->recv_total, ctx->header_size);
        }

        if (dt_last) {
            if (ctx->recv_total != ctx->header_size) {
                log_warn("header incomplete: got=%lu expected=%lu",
                         ctx->recv_total, ctx->header_size);
                free(ctx->header_buf);
                ctx->header_buf = NULL;
                ctx->header_size = 0;
                ctx->recv_total = 0;
                ctx->has_header = 0;
            } else {
                ctx->has_header = 1;
                log_info("jpeg header complete: size=%lu", ctx->header_size);
            }
        }
        return;
    }

    /*  场景B/C: data-only 或 完整JPEG  */

    /* ── 首包 ── */
    if (pkt->offset == 0) {

        /* --- 场景B: 仅裸数据 (0x02)，需拼接之前保存的头部 --- */
        if (!dt_header && dt_data) {
            if (!ctx->has_header) {
                log_error("no header cached for data frame");
                return;
            }
            /* 上一帧未完成就收到新帧 → 丢弃旧帧 */
            recv_abort_frame();

            u32 combined_size = ctx->header_size + pkt->frame_size;
            ctx->frame_buf = (u8 *)malloc(combined_size);
            if (!ctx->frame_buf) {
                log_error("frame_buf alloc fail, size=%lu", combined_size);
                return;
            }
            /* 拷贝头部到缓冲区开头 */
            memcpy(ctx->frame_buf, ctx->header_buf, ctx->header_size);

            /* 拷贝第一包数据负载 */
            memcpy(ctx->frame_buf + ctx->header_size, pkt->payload, pkt->payload_len);

            ctx->frame_size     = combined_size;
            ctx->data_frame_size = pkt->frame_size;
            ctx->data_crc       = pkt->crc;
            ctx->recv_total     = ctx->header_size + pkt->payload_len;
            ctx->current_seq    = pkt->seq;
            ctx->active         = 1;

            log_info("new data-only frame: seq=%d data_size=%lu combined=%lu",
                     pkt->seq, pkt->frame_size, combined_size);
            goto process_last;
        }

        /* --- 场景C: 完整JPEG (0x03) --- */
        /* 上一帧未完成就收到新帧 → 丢弃旧帧 */
        if (ctx->active && pkt->seq != ctx->current_seq) {
            log_warn("new frame seq=%d before prev seq=%d done, discard",
                     pkt->seq, ctx->current_seq);
            recv_abort_frame();
        }

        ctx->frame_buf = (u8 *)malloc(pkt->frame_size);
        if (!ctx->frame_buf) {
            log_error("frame_buf alloc fail, size=%lu", pkt->frame_size);
            return;
        }
        ctx->frame_size     = pkt->frame_size;
        ctx->data_frame_size = pkt->frame_size;
        ctx->data_crc       = pkt->crc;
        ctx->recv_total     = 0;
        ctx->current_seq    = pkt->seq;
        ctx->active         = 1;

        log_info("new frame: seq=%d size=%lu ts=%lu",
                 pkt->seq, pkt->frame_size, pkt->timestamp);
    }

    /* ── 拷贝分包数据 ── */
    {
        u32 offset = pkt->offset;
        /* data-only模式(0x02)时，数据放于header之后 */
        if (dt_data && !dt_header) {
            offset += ctx->header_size;
        }

        if (ctx->active && ctx->frame_buf &&
            offset + pkt->payload_len <= ctx->frame_size) {
            memcpy(ctx->frame_buf + offset, pkt->payload, pkt->payload_len);
            ctx->recv_total += pkt->payload_len;
        } else {
            log_error("overflow: off=%lu plen=%d fsize=%lu",
                      offset, pkt->payload_len, ctx->frame_size);
            recv_abort_frame();
            return;
        }
    }

process_last:
    /* ── 末包：校验并交付 ── */
    if (dt_last) {
        /* 帧完整性校验 */
        if (ctx->recv_total != ctx->frame_size) {
            log_warn("frame incomplete: got=%lu expected=%lu",
                     ctx->recv_total, ctx->frame_size);
            recv_abort_frame();
            return;
        }

        /* CRC16 校验 (仅校验原始数据部分) */
        if (ctx->data_crc != 0) {
            u32 crc_offset = (dt_data && !dt_header) ? ctx->header_size : 0;
            u16 calc_crc = CRC16_with_initval(
                               ctx->frame_buf + crc_offset,
                               ctx->data_frame_size, 0);
            if (calc_crc != ctx->data_crc) {
                log_warn("CRC mismatch: calc=0x%04x recv=0x%04x",
                         calc_crc, ctx->data_crc);
                recv_abort_frame();
                return;
            }
        }

        log_info("frame complete: seq=%d size=%lu data_size=%lu",
                 ctx->current_seq, ctx->frame_size, ctx->data_frame_size);

#if 0
        /* 将JPEG帧送入共享缓冲区供UI显示 */
        jljpeg_stream_src_data_copy(ctx->frame_buf, ctx->frame_size);

        /* 通知UI刷新 */
        jlstream_recv_ui_reflush(0);
#endif
        /* 帧处理完毕，释放缓冲区 */
        recv_abort_frame();
    }
}



/* ==================== H264 图像帧处理  ==================== */
/* H264解码帧信息:通过ui_refresh_cb传给UI层(送显) */
typedef struct {
    const u8 *yuv;        /* YUV420 Planar 数据指针 (回调内有效，拷贝后失效) */
    u32       width;
    u32       height;
    u32       frame_idx;  /* 解码帧序号 */
} jlstm_frame_info_t;

static void *recv_h264_malloc(int size, u32 ram_type, u32 module_type)
{
    (void)module_type;

    if (size <= 0) {
        return NULL;
    }

#if 0
    if (ram_type == UI_RAM_PSRAM) {
        return malloc_psram((size_t)size);
    }
#endif

    return malloc((size_t)size);
}

static void recv_h264_free(void *buf, u32 ram_type, u32 module_type)
{
    (void)module_type;

    if (buf == NULL) {
        return;
    }

#if 0
    if (ram_type == UI_RAM_PSRAM) {
        free_psram(buf);
        return;
    }
#endif

    free(buf);
}


/**
 * @brief 初始化 H.264 解码器
 */
static int recv_h264_init(jlstm_recv_h264_ctx_t *ctx)
{
    if (!ctx || ctx->decoder_inited) {
        return 0;
    }

    // 使用psram时需要注册
    /* video_1_register_alloc(recv_h264_malloc, recv_h264_free); */

    ctx->dec_storage = video_1_alloc();
    if (!ctx->dec_storage) {
        log_error("h264 alloc storage fail");
        return -1;
    }

    u32 ret = video_1_init(ctx->dec_storage);  /* 1=不重排序 */
    if (ret != 0) {
        log_error("h264 init fail: %lu", ret);
        video_1_free(ctx->dec_storage);
        ctx->dec_storage = NULL;
        return -1;
    }

    ctx->stream_buf      = NULL;  /* 按片动态分配，解码后释放 */
    ctx->stream_capacity = 0;
    ctx->stream_len      = 0;
    ctx->frame_count     = 0;
    ctx->pic_width       = 0;
    ctx->pic_height      = 0;
    ctx->decoder_inited  = 1;

    ctx->pkt_size           = 0;
    ctx->pkt_crc            = 0;
    ctx->pkt_head_seq       = 0;
    /* 统计 */
    ctx->seq_gap_cnt        = 0;
    ctx->crc_err_cnt        = 0;
    ctx->pkt_incomplete_cnt = 0;

    log_info("h264 decoder inited");
    return 0;
}

/**
 * @brief 反初始化 H.264 解码器
 *
 * 刷出 DPB 残留帧 → shutdown → free 全部资源
 */
static void recv_h264_deinit(jlstm_recv_h264_ctx_t *ctx)
{
    if (!ctx || !ctx->decoder_inited) {
        return;
    }

    if (ctx->dec_storage) {
        video_1_deinit(ctx->dec_storage);
        video_1_free(ctx->dec_storage);
        ctx->dec_storage = NULL;
    }

    if (ctx->stream_buf) {
        free(ctx->stream_buf);
        ctx->stream_buf = NULL;
    }

    ctx->decoder_inited = 0;
    log_info("h264 decoder deinited, frames=%lu seq_gap=%lu crc_err=%lu incomplete=%lu",
             ctx->frame_count, ctx->seq_gap_cnt,
             ctx->crc_err_cnt, ctx->pkt_incomplete_cnt);
}

/**
 * @brief 从 DPB 中取出所有就绪帧进行处理(可在实现中增加送显)
 */
static int recv_h264_dpb_dump(jlstm_recv_h264_ctx_t *ctx)
{
    if (!ctx || !ctx->dec_storage) {
        return 0;
    }

    int frames = 0;
    struct video_1_output out = {0};
    video_1_decode_output(ctx->dec_storage, &out);

    {
        if (out.frame_buf) {
            ctx->pic_width  = out.width;
            ctx->pic_height = out.height;
            ctx->frame_count++;

            log_info("h264 frame %lu: %lux%lu",
                     ctx->frame_count, ctx->pic_width, ctx->pic_height);

            /* 送显 */
            jlstm_frame_info_t info;
            info.yuv       = out.frame_buf;
            info.width     = ctx->pic_width;
            info.height    = ctx->pic_height;
            info.frame_idx = ctx->frame_count;
            jlstream_recv_ui_reflush(&info);
#if 0
            /* 测试模式: YUV 数据写入 SD 卡 */
            {
                char path[64];
                u32 y_size  = ctx->pic_width * ctx->pic_height;
                u32 uv_size = y_size / 4;
                u32 total   = y_size + uv_size * 2;  /* Y + U + V */

                snprintf(path, sizeof(path),
                         "storage/sd0/C/h264_%03d.yuv",
                         ctx->frame_count);

                FILE *fp = fopen(path, "w+");
                if (fp) {
                    u32 written = (u32)fwrite(out.frame_buf, 1, total, fp);
                    fclose(fp);
                    if (written == total) {
                        log_info("h264 yuv saved: %s (%lu bytes)", path, total);
                    } else {
                        log_error("h264 yuv write fail: %s (wrote %u/%lu)",
                                  path, (u32)written, total);
                    }
                } else {
                    log_error("h264 yuv open fail: %s", path);
                }
            }
#endif

            frames++;
        }
    }
    return frames;
}

/**
 * @brief 喂入 H.264 数据并解码
 *
 * @param offset     分片在整帧数据中的字节偏移 (首包为 0)
 * @param frame_size 整帧总大小 (仅 offset==0 时有效)
 * @param seq        包序号
 * @param crc        整帧数据 CRC16 (仅 offset==0 时有效, 0=不校验)
 */
static int recv_h264_feed(jlstm_recv_h264_ctx_t *ctx,
                          const u8 *data, u32 len,
                          u32 offset, u32 frame_size,
                          u16 seq, u16 crc, u8 is_last)
{
    if (!ctx || !ctx->decoder_inited || !data || len == 0) {
        return 0;
    }

    /* 首包 */
    if (offset == 0) {
        ctx->pkt_size      = frame_size;
        ctx->pkt_crc       = crc;
        ctx->pkt_head_seq  = seq;

        /* 按包的 frame_size 实时分配缓冲区 */
        {
            u32 alloc_size = frame_size + 4096;
            u8 *new_buf = (u8 *)malloc(alloc_size);
            if (!new_buf) {
                log_error("h264 buf alloc fail, size=%lu", frame_size);
                ctx->stream_len = 0;
                ctx->pkt_size   = 0;
                return 0;
            }
            if (ctx->stream_buf) {
                free(ctx->stream_buf);
                ctx->stream_buf = NULL;
            }
            ctx->stream_buf     = new_buf;
            ctx->stream_capacity = alloc_size;
        }
        ctx->stream_len = 0;  /* 新包开始，重置接收长度 */
    }

    /* 偏移量越界保护 */
    if (offset + len > ctx->stream_capacity) {
        log_error("h264 offset overflow: off=%lu len=%lu cap=%lu",
                  offset, len, ctx->stream_capacity);
        free(ctx->stream_buf);
        ctx->stream_buf     = NULL;
        ctx->stream_capacity = 0;
        ctx->stream_len      = 0;
        ctx->pkt_size        = 0;
        return 0;
    }

    /* 按 offset 定位写入，兼容乱序到达 */
    memcpy(ctx->stream_buf + offset, data, len);
    if (offset + len > ctx->stream_len) {
        ctx->stream_len = offset + len;  /* 记录已接收的最大偏移 */
    }

    log_info("h264 pkt,len:%d, offset:%d, frame_size:%d, seq:%d, crc:%x, is_last:%d\n",
             len, offset, frame_size, seq, crc, is_last);


    if (!is_last) {
        return 0;  /* 包未收完整，继续累积 */
    }
#if 0
    /* 测试模式: 原始H.264码流写入SD卡 */
    {
        char path[64];
        snprintf(path, sizeof(path),
                 "storage/sd0/C/%04d.264",
                 seq);   /* +1: 当前帧解码前计数尚未递增 */

        FILE *fp = fopen(path, "w+");
        if (fp) {
            u32 written = (u32)fwrite(ctx->stream_buf, 1, ctx->stream_len, fp);
            fclose(fp);
            if (written == ctx->stream_len) {
                log_info("h264 raw saved: %s (%lu bytes)", path, ctx->stream_len);
            } else {
                log_error("h264 raw write fail: %s (wrote %u/%lu)",
                          path, written, ctx->stream_len);
            }
        } else {
            log_error("h264 raw open fail: %s", path);
        }
    }
#endif


    /* dt_last: 包完整, 执行完整性校验 */

    /* 1. CRC16 校验 */
    if (ctx->pkt_crc != 0 && ctx->stream_len > 0) {
        u16 calc_crc = CRC16_with_initval(ctx->stream_buf,
                                          ctx->stream_len, 0);
        log_info("h264 crc : calc=0x%04x expect=0x%04x , stream_len:%d",
                 calc_crc, ctx->pkt_crc, ctx->stream_len);

        if (calc_crc != ctx->pkt_crc) {
            ctx->crc_err_cnt++;
            log_error("h264 crc mismatch: calc=0x%04x expect=0x%04x cnt=%lu, discard nal",
                      calc_crc, ctx->pkt_crc, ctx->crc_err_cnt);
            /* CRC 失败 → 丢弃该 NAL，释放缓冲区 */
            free(ctx->stream_buf);
            ctx->stream_buf     = NULL;
            ctx->stream_capacity = 0;
            ctx->stream_len      = 0;
            ctx->pkt_size        = 0;
            ctx->pkt_crc         = 0;
            return 0;
        }
    }

    /* ── 送入解码器 ── */
    u32 dec_off = 0;
    int frames  = 0;
    u32 zero_read_count = 0;  /* 防御: read_bytes 连续为0时中止 */
#if 0
    /* 测试模式: 原始H.264码流写入SD卡 */
    {
        char path[64];
        snprintf(path, sizeof(path),
                 "storage/sd0/C/%04d.264",
                 seq);   /* +1: 当前帧解码前计数尚未递增 */

        FILE *fp = fopen(path, "w+");
        if (fp) {
            u32 written = (u32)fwrite(ctx->stream_buf, 1, ctx->stream_len, fp);
            fclose(fp);
            if (written == ctx->stream_len) {
                log_info("h264 raw saved: %s (%lu bytes)", path, ctx->stream_len);
            } else {
                log_error("h264 raw write fail: %s (wrote %u/%lu)",
                          path, written, ctx->stream_len);
            }
        } else {
            log_error("h264 raw open fail: %s", path);
        }
    }
#endif

    while (dec_off < ctx->stream_len) {
        u32 read_bytes = 0;

        u32 ret = video_1_decode(
                      ctx->dec_storage,
                      ctx->stream_buf + dec_off,
                      ctx->stream_len - dec_off,
                      &read_bytes,
                      NULL);

        if (ret == video_1_DEC_PIC_RDY) {
            /* 一帧图像重建完成 → 从 DPB 取出并送显 */
            frames += recv_h264_dpb_dump(ctx);
            zero_read_count = 0;
        } else if (ret == video_1_DEC_HDRS_RDY) {
            /*
             * SPS/PPS 首次激活或参数变化时返回 HDRS_RDY，
             * 此时 read_bytes=0 且 prevBufNotFinished=TRUE，
             * 解码器期望用同一指针再次调用以完成第二阶段初始化
             * (清除 pendingActivation 并继续解码 slice)。
             * 绝不能 break，否则 pendingActivation 残留会导致
             * 后续 P 帧报 "Pending activation not completed" 错误。
             */
            zero_read_count++;
            if (zero_read_count > 4) {
                log_error("h264 hdrs_rdy stuck: off=%lu", dec_off);
                break;
            }
            /* dec_off 不前进, 下次调用同一指针触发 prevBufNotFinished 路径 */
            continue;
        } else if (ret == video_1_DEC_ERROR ||
                   ret == video_1_DEC_PARAM_SET_ERROR ||
                   ret == video_1_DEC_MEMALLOC_ERROR) {
            log_error("h264 decode err: ret=%lu off=%lu",
                      ret, dec_off);
            break;
        }
        if (read_bytes == 0) {
            zero_read_count++;
            if (zero_read_count > 4) {
                log_error("h264 decode stuck: ret=%lu off=%lu read=0",
                          ret, dec_off);
                break;
            }
            continue;
        }
        dec_off += read_bytes;
        zero_read_count = 0;
    }

    /* 本轮数据消费完毕，释放缓冲区等待下一轮 */
    free(ctx->stream_buf);
    ctx->stream_buf     = NULL;
    ctx->stream_capacity = 0;
    ctx->stream_len      = 0;
    ctx->pkt_size        = 0;
    ctx->pkt_crc         = 0;
    return frames;
}

/* ==================== H264 调试: SD卡文件回放 ==================== */

#if H264_DEBUG_FROM_SD

#define H264_DEBUG_SD_PATH  "storage/sd0/C"
#define H264_DEBUG_DELAY_MS 30  /* 帧间隔 */

/**
 * @brief 从 SD 卡读取下一个 .264 文件并送入解码器 (内部管理文件序号和延时)
 * @param ctx  H.264 解码上下文
 */
static void recv_h264_sd_feed_one(jlstm_recv_h264_ctx_t *ctx)
{
    static int sd_file_idx = 1;
    char path[64];

    snprintf(path, sizeof(path), "%s/%04d.264", H264_DEBUG_SD_PATH, sd_file_idx);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        log_info("h264 sd: no file [%s], restart from 1", path);
        sd_file_idx = 1;
        snprintf(path, sizeof(path), "%s/%04d.264", H264_DEBUG_SD_PATH, sd_file_idx);
        fp = fopen(path, "r");
        if (!fp) {
            return;
        }
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0) {
        log_error("h264 sd: empty file [%s]", path);
        fclose(fp);
        sd_file_idx++;
        return;
    }

    u8 *buf = (u8 *)malloc((size_t)fsize);
    if (!buf) {
        log_error("h264 sd: alloc %ld fail", fsize);
        fclose(fp);
        sd_file_idx++;
        return;
    }

    size_t rd = fread(buf, 1, (size_t)fsize, fp);
    fclose(fp);

    if (rd != (size_t)fsize) {
        log_error("h264 sd: read %d/%ld", (int)rd, fsize);
        free(buf);
        sd_file_idx++;
        return;
    }

    log_info("h264 sd: feed [%s] %ld bytes", path, fsize);

    /* 作为完整一包送入: offset=0, crc=0 跳过校验, is_last=1 触发解码 */
    recv_h264_feed(ctx, buf, (u32)fsize,
                   0, (u32)fsize,    /* offset, frame_size */
                   (u16)sd_file_idx, /* seq */
                   0,                /* crc=0 跳过CRC校验 */
                   1);               /* is_last */
    free(buf);

    sd_file_idx++;
    os_time_dly(H264_DEBUG_DELAY_MS);
}

#endif /* H264_DEBUG_FROM_SD */
/* ==================== 对外接口 ==================== */

/**
 * @brief 初始化 Push 接收框架
 *
 * - 分配句柄
 * - 创建接收任务线程
 * - 多个调用幂等（已初始化时直接返回 0）
 */
int jlstream_recv_demo_init(void)
{
    if (g_recv) {
        log_info("already inited");
        return 0;
    }

    g_recv = (jlstm_recv_handle_t *)malloc(sizeof(*g_recv));
    if (!g_recv) {
        log_error("handle alloc fail");
        return -1;
    }
    memset(g_recv, 0, sizeof(*g_recv));

    snprintf(g_recv->task_name, sizeof(g_recv->task_name), "jlstm_push_recv");

    if (os_task_create_affinity_core(jlstm_push_recv_task, NULL, 15, 1024, 1024, g_recv->task_name, 1)) {
        log_error("task create fail");
        free(g_recv);
        g_recv = NULL;
        return -1;
    }

    log_info("inited");
    return 0;
}

/**
 * @brief 反初始化 Push 接收框架
 *
 * - 清理消息队列
 * - 等待任务退出
 * - 释放帧缓冲区和句柄
 * - 多个调用幂等（未初始化时直接返回 0）
 */
int jlstream_recv_demo_deinit(void)
{
    if (!g_recv) {
        return 0;
    }

    /* 清空待处理的 DATA 消息（避免任务在处理这些消息时被 kill） */
    os_taskq_del_type(g_recv->task_name, Q_JLSTM_RECV_DATA);

    OS_SEM kill_sem;
    os_sem_create(&kill_sem, 0);
    int sem_ptr = (int)&kill_sem;
    os_taskq_post_type(g_recv->task_name, Q_JLSTM_RECV_KILL, 1, &sem_ptr);
    os_sem_pend(&kill_sem, 0);
    os_sem_del(&kill_sem, OS_DEL_ALWAYS);

    task_kill(g_recv->task_name);

    /* 释放jpeg相关 */
    if (g_recv->frame_ctx.frame_buf) {
        free(g_recv->frame_ctx.frame_buf);
    }
    if (g_recv->frame_ctx.header_buf) {
        free(g_recv->frame_ctx.header_buf);
        g_recv->frame_ctx.header_buf = NULL;
    }

    /*释放h264相关*/
    if (g_recv->h264_ctx.dec_storage) {
        video_1_deinit(g_recv->h264_ctx.dec_storage);
        video_1_free(g_recv->h264_ctx.dec_storage);
    }
    if (g_recv->h264_ctx.stream_buf) {
        free(g_recv->h264_ctx.stream_buf);
        g_recv->h264_ctx.stream_buf     = NULL;
    }

    free(g_recv);
    g_recv = NULL;

    log_info("deinited");
    return 0;
}

/* ==================== UI 刷新回调注册 ==================== */

int jlstream_recv_ui_refresh_register(void (*cb)(void *arg))
{
    recv_ui.ui_refresh_cb = cb;
    log_info("registered, cb=%p", cb);
    return 0;
}

static int jlstm_demo_set_device(void)
{
    jlstm_dev_capability_t cap;
    memset(&cap, 0, sizeof(cap));
#if 0
    /* 配置当前支持的设备 */
    cap.cam_count = 2;
    // Camera 0: JPEG, 320x240, 15fps
    cap.cams[0].number = 0;
    cap.cams[0].codec_bitmap = 0x02; // JPEG
    cap.cams[0].max_width = 240;
    cap.cams[0].max_height = 320;
    cap.cams[0].max_fps = 15;
    // Camera 1: JPEG, 320x240, 15fps
    cap.cams[1].number = 1;
    cap.cams[1].codec_bitmap = 0x02; // JPEG
    cap.cams[1].max_width = 240;
    cap.cams[1].max_height = 320;
    cap.cams[1].max_fps = 15;
    //  MIC: None
    cap.mic_count = 0;
#endif

    // Local Device: 本机支持视频显示，不支持音频播放
    cap.local_dev_en = 1;
    cap.local_dev.number = 0x01;
    cap.local_dev.has_video = 1;
    cap.local_dev.video_cfg.codec_bitmap = 0x02; // H264
    cap.local_dev.video_cfg.max_width = 320;
    cap.local_dev.video_cfg.max_height = 240;
    cap.local_dev.video_cfg.max_fps = 15;
    cap.local_dev.has_audio = 0;

    jlstm_set_device_capability(&cap);
    return 0;
}
early_initcall(jlstm_demo_set_device);


/* ==================== 新流媒体协议适配 ==================== */
#if !TCFG_BT_H264_SCR_ENABLE
#include "stream_media_core.h"

/* 当前推流编码类型 (从 START_STREAM 获取) */
static u8 g_sm_codec = 0;

/**
 * @brief 新协议数据回调适配 —— 将 stream_media_data_push_cb_t
 *        的参数打包为 jlstm_recv_msg_t 投递到接收任务
 */
static void test_sm_data_cb(const u8 *data, u16 len,
                            u16 seq, u32 offset,
                            u32 frame_size, u32 timestamp,
                            u16 crc, u8 stream_type, u8 is_last)
{
    if (!g_recv || !data || len == 0) {
        return;
    }

    /* 首包 stream_type 覆盖 g_sm_codec */
    if (stream_type != 0) {
        g_sm_codec = stream_type;
    }

    jlstm_recv_msg_t *msg = (jlstm_recv_msg_t *)malloc(sizeof(*msg) + len);
    if (!msg) {
        log_error("msg alloc fail");
        return;
    }

    msg->index       = 0;
    msg->frame_size  = frame_size;
    msg->timestamp   = timestamp;
    msg->crc         = crc;
    /* msg->crc         = 0; */

    if (g_sm_codec == STREAM_FORMAT_JPEG) {
        msg->codec_type = 0x00;
    } else if (g_sm_codec == STREAM_FORMAT_H264) {
        msg->codec_type = 0x01;
    }
    /*
     * data_type 适配:
     *   新协议不再分离 JPEG header/data，统一按完整帧 (0x03) 处理。
     *   recv_jpeg_feed 会走 Scenario C，不需要 header 缓存。
     *   H264 则直接由 recv_h264_feed 处理，data_type 仅记录末包标志。
     */
    msg->data_type   = (g_sm_codec == STREAM_FORMAT_JPEG)
                       ? (0x03 | (is_last ? 0x80 : 0x00))
                       : (is_last ? 0x80 : 0x00);
    msg->seq         = seq;
    msg->offset      = offset;
    msg->payload_len = len;
    memcpy(msg->payload, data, len);
    log_info("[sm_data] seq=%d off=%d len=%d last=%d codec=0x%02x ts=%d size=%d, crc = 0x%04x",
             seq, offset, len, is_last, g_sm_codec, timestamp, frame_size, crc);

    int arg = (int)msg;
    int err = os_taskq_post_type(g_recv->task_name, Q_JLSTM_RECV_DATA, 1, &arg);
    if (err) {
        log_error("post fail: %d", err);
        free(msg);
    }
}

/**
 * @brief 新协议 START_STREAM 回调 —— 初始化接收框架
 */
static void test_sm_start_cb(const stream_media_stream_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }
    g_sm_codec = cfg->format;

    log_info("[NEW-PROTO] start: %dx%d@%dfps qual=%d fmt=%d mtu=%d",
             cfg->width, cfg->height, cfg->fps,
             cfg->quality, cfg->format, cfg->stream_mtu);

    if (jlstream_recv_demo_init() == 0 && g_recv) {
        g_recv->frame_ctx.active_index = 0;
    }
}

/**
 * @brief 新协议 STOP_STREAM 回调 —— 反初始化接收框架
 */
static void test_sm_stop_cb(void)
{
    log_info("[NEW-PROTO] stop");
    jlstream_recv_demo_deinit();
}

/**
 * @brief 将处理函数注册到新流媒体协议框架
 *
 * 设备信息/能力已由 device_config 直供，无需注册。
 */
static int test_sm_register(void)
{
    stream_media_set_stream_start_cb(test_sm_start_cb);
    stream_media_set_stream_stop_cb(test_sm_stop_cb);
    stream_media_set_data_push_cb(test_sm_data_cb);

    /* 初始化协议核心（幂等：多次调用安全）
     * 复位状态机、清零 SN，确保 BLE 数据到达时已就绪 */
    stream_media_core_init();

    log_info("[NEW-PROTO] callbacks registered + core inited");
    return 0;
}

early_initcall(test_sm_register);


#endif