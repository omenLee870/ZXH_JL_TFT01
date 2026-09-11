#include "system/includes.h"
#include "ai_audio_sink.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#include "audio_def.h"
#include "ai_audio_player.h"

#include <stdlib.h>
#include <string.h>

#define AI_AUDIO_SINK_DEFAULT_BUFFER_SIZE   (16 * 1024)
#define AI_AUDIO_SINK_DEFAULT_VOLUME        100
#define AI_AUDIO_SINK_SOURCE                0xA0
#define AI_AUDIO_SINK_FRAME_BYTES           1024
#define AI_AUDIO_SINK_WRITE_WAIT_MS         500

#if TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void free_psram(void *ptr);
#endif

struct ai_audio_sink_default_ctx {
    char *buf;
    cbuffer_t cbuf;
    struct ai_audio_sink_config cfg;
    int opened;
    int closed;
    int draining;
    int player_started;
    int error;
};

static struct ai_audio_sink_default_ctx ai_audio_sink_default;
static const struct ai_audio_sink_ops *ai_audio_sink_ops;
static void *ai_audio_sink_ctx;
static int ai_audio_sink_is_open;

static void *ai_audio_sink_malloc(unsigned int size)
{
#if TCFG_PSRAM_DEV_ENABLE
    return malloc_psram(size);
#else
    return malloc(size);
#endif
}

static void ai_audio_sink_free(void *ptr)
{
#if TCFG_PSRAM_DEV_ENABLE
    free_psram(ptr);
#else
    free(ptr);
#endif
}

static u32 ai_audio_sink_coding_type(enum ai_audio_sink_format format)
{
    switch (format) {
    case AI_AUDIO_SINK_PCM16:
        return AUDIO_CODING_PCM;
    case AI_AUDIO_SINK_OPUS:
        return AUDIO_CODING_OPUS;
    case AI_AUDIO_SINK_MP3:
        return AUDIO_CODING_STREAM_MP3;
    case AI_AUDIO_SINK_AAC:
        return AUDIO_CODING_AAC;
    default:
        return AUDIO_CODING_UNKNOW;
    }
}

static enum stream_node_state ai_audio_sink_rx_frame(void *file, struct stream_frame **pframe)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    struct ai_audio_sink_default_ctx *ctx = &ai_audio_sink_default;
    struct stream_frame *frame;
    unsigned int data_len;
    unsigned int read_size;
    int rlen;

    *pframe = NULL;
    if (!ctx->buf) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    data_len = cbuf_get_data_size(&ctx->cbuf);
    read_size = ctx->cfg.frame_bytes ? ctx->cfg.frame_bytes : AI_AUDIO_SINK_FRAME_BYTES;
    if (data_len < read_size && !ctx->closed && !ctx->draining) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    if (data_len < read_size) {
        read_size = data_len;
    }
    if (!read_size) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame = jlstream_get_frame(hdl->node->oport, read_size);
    if (!frame) {
        return NODE_STA_RUN;
    }

    rlen = cbuf_read(&ctx->cbuf, frame->data, read_size);
    if (rlen <= 0) {
        jlstream_free_frame(frame);
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame->len = rlen;
    *pframe = frame;
    return NODE_STA_RUN;
}

static void ai_audio_sink_default_reset(struct ai_audio_sink_default_ctx *ctx)
{
    if (!ctx) {
        return;
    }

    ctx->opened = 0;
    ctx->closed = 0;
    ctx->draining = 0;
    ctx->player_started = 0;
    ctx->error = 0;
    memset(&ctx->cfg, 0, sizeof(ctx->cfg));
}

static unsigned int ai_audio_sink_sample_bytes(const struct ai_audio_sink_default_ctx *ctx)
{
    unsigned int channels;
    unsigned int bytes_per_sample;

    if (!ctx || ctx->cfg.format != AI_AUDIO_SINK_PCM16) {
        return 1;
    }

    channels = ctx->cfg.channels ? ctx->cfg.channels : 1;
    bytes_per_sample = ctx->cfg.bits_per_sample ? ctx->cfg.bits_per_sample / 8 : 2;
    return channels * bytes_per_sample;
}

static void ai_audio_sink_player_event(u8 source, int event, void *priv)
{
    struct ai_audio_sink_default_ctx *ctx = (struct ai_audio_sink_default_ctx *)priv;

    if (ctx && source == AI_AUDIO_SINK_SOURCE && event == STREAM_EVENT_NEGOTIATE_FAILD) {
        ctx->error = 1;
        printf("[ai_audio_sink] format negotiation failed\n");
    }
}

static unsigned int ai_audio_sink_prebuffer_bytes(const struct ai_audio_sink_default_ctx *ctx)
{
    unsigned int frame_bytes;
    unsigned int prebuffer_bytes;

    frame_bytes = ctx->cfg.frame_bytes ? ctx->cfg.frame_bytes : AI_AUDIO_SINK_FRAME_BYTES;
    if (frame_bytes > ctx->cbuf.total_len) {
        frame_bytes = ctx->cbuf.total_len;
    }
    prebuffer_bytes = ctx->cfg.prebuffer_bytes;
    if (!prebuffer_bytes) {
        prebuffer_bytes = ctx->cbuf.total_len > frame_bytes ? ctx->cbuf.total_len - frame_bytes : frame_bytes;
    }
    if (prebuffer_bytes < frame_bytes) {
        prebuffer_bytes = frame_bytes;
    }
    if (prebuffer_bytes > ctx->cbuf.total_len) {
        prebuffer_bytes = ctx->cbuf.total_len;
    }
    return prebuffer_bytes;
}

static int ai_audio_sink_start_player(struct ai_audio_sink_default_ctx *ctx)
{
    struct ai_rx_player_param param = {0};
    int ret;

    if (ctx->player_started) {
        return 0;
    }

    param.coding_type = ai_audio_sink_coding_type(ctx->cfg.format);
    param.sample_rate = ctx->cfg.sample_rate ? ctx->cfg.sample_rate : 16000;
    param.channel_mode = ctx->cfg.channels == 2 ? AUDIO_CH_LR : AUDIO_CH_MIX;
    param.type = AI_SERVICE_VOICE;
    param.packet_len = ctx->cfg.frame_bytes;

    ret = ai_audio_player_open(NULL, AI_AUDIO_SINK_SOURCE, &param, ai_audio_sink_rx_frame,
                               ai_audio_sink_player_event, ctx);
    if (ret) {
        ctx->error = 1;
        return ret;
    }

    extern void audio_app_volume_set(u8 state, s16 volume, u8 fade);
    audio_app_volume_set(1, ctx->cfg.volume ? ctx->cfg.volume : AI_AUDIO_SINK_DEFAULT_VOLUME, 1);
    ctx->player_started = 1;
    printf("[ai_audio_sink] player start: buffered=%u/%u\n",
           cbuf_get_data_size(&ctx->cbuf), ctx->cbuf.total_len);
    return 0;
}

static int ai_audio_sink_default_open(const struct ai_audio_sink_config *cfg, void **out_ctx)
{
    struct ai_audio_sink_default_ctx *ctx = &ai_audio_sink_default;
    unsigned int buffer_size;

    if (!cfg || !out_ctx || ctx->opened) {
        return -1;
    }

    if (ai_audio_sink_coding_type(cfg->format) == AUDIO_CODING_UNKNOW) {
        return -1;
    }
    if (cfg->format == AI_AUDIO_SINK_PCM16 &&
        ((cfg->bits_per_sample && cfg->bits_per_sample != 16) || cfg->channels > 2)) {
        return -1;
    }
    buffer_size = cfg->buffer_size ? cfg->buffer_size : AI_AUDIO_SINK_DEFAULT_BUFFER_SIZE;
    ctx->buf = (char *)ai_audio_sink_malloc(buffer_size);
    if (!ctx->buf) {
        return -1;
    }
    cbuf_init(&ctx->cbuf, ctx->buf, buffer_size);
    ctx->cfg = *cfg;
    ctx->closed = 0;
    ctx->draining = 0;
    ctx->player_started = 0;
    ctx->error = 0;
    ctx->opened = 1;

    if (cfg->format != AI_AUDIO_SINK_PCM16 && ai_audio_sink_start_player(ctx)) {
        ai_audio_sink_free(ctx->buf);
        ctx->buf = NULL;
        ai_audio_sink_default_reset(ctx);
        return -1;
    }
    *out_ctx = ctx;
    return 0;
}

static int ai_audio_sink_default_write(void *sink_ctx, const unsigned char *data, unsigned int len)
{
    struct ai_audio_sink_default_ctx *ctx = (struct ai_audio_sink_default_ctx *)sink_ctx;
    unsigned int align;
    unsigned int capacity;
    unsigned int wait_start_ms;
    int wlen;

    if (!ctx || !ctx->opened || ctx->closed || ctx->error || !data || !len) {
        return -1;
    }

    align = ai_audio_sink_sample_bytes(ctx);
    if (align > 1 && len % align) {
        printf("[ai_audio_sink] reject unaligned pcm: len=%u align=%u\n", len, align);
        return -1;
    }

    capacity = ctx->cbuf.total_len;
    if (ctx->cfg.format != AI_AUDIO_SINK_PCM16) {
        if (len > capacity) {
            printf("[ai_audio_sink] compressed frame exceeds buffer: len=%u capacity=%u\n",
                   len, capacity);
            return -1;
        }
        wlen = cbuf_write(&ctx->cbuf, (void *)data, len);
        if (wlen != (int)len) {
            printf("[ai_audio_sink] compressed stream overflow: len=%u buffered=%u/%u\n",
                   len, cbuf_get_data_size(&ctx->cbuf), capacity);
            return -1;
        }
        return (int)len;
    }

    if (len > capacity) {
        printf("[ai_audio_sink] pcm frame exceeds buffer: len=%u capacity=%u\n", len, capacity);
        return -1;
    }

    /* Let TCP backpressure slow bursty realtime output; deleting old PCM is audible as a skip. */
    wait_start_ms = sys_timer_get_ms();
    while (len > capacity - cbuf_get_data_size(&ctx->cbuf)) {
        if (ctx->closed || ctx->error) {
            return -1;
        }
        if ((unsigned int)(sys_timer_get_ms() - wait_start_ms) >= AI_AUDIO_SINK_WRITE_WAIT_MS) {
            printf("[ai_audio_sink] pcm backpressure timeout: len=%u buffered=%u/%u wait=%u ms\n",
                   len, cbuf_get_data_size(&ctx->cbuf), capacity,
                   AI_AUDIO_SINK_WRITE_WAIT_MS);
            return -1;
        }
        os_time_dly(1);
    }

    wlen = cbuf_write(&ctx->cbuf, (void *)data, len);
    if (wlen != (int)len) {
        printf("[ai_audio_sink] pcm write failed: %d/%u\n", wlen, len);
        return -1;
    }

    if (!ctx->player_started &&
        cbuf_get_data_size(&ctx->cbuf) >= ai_audio_sink_prebuffer_bytes(ctx) &&
        ai_audio_sink_start_player(ctx)) {
        return -1;
    }

    return (int)len;
}

static int ai_audio_sink_default_drain(void *sink_ctx, int timeout_ms)
{
    struct ai_audio_sink_default_ctx *ctx = (struct ai_audio_sink_default_ctx *)sink_ctx;
    const int poll_tick = 2;
    const int poll_interval_ms = poll_tick * 10;
    int waited = 0;
    int delay;
    int empty_checks = 0;

    if (!ctx) {
        return -1;
    }
    if (!ctx->opened || ctx->closed || ctx->error) {
        return 0;
    }

    ctx->draining = 1;
    if (!ctx->player_started && cbuf_get_data_size(&ctx->cbuf) > 0) {
        if (ai_audio_sink_start_player(ctx)) {
            ctx->draining = 0;
            return -1;
        }
    }
    while (cbuf_get_data_size(&ctx->cbuf) > 0) {
        if (timeout_ms <= 0) {
            ctx->draining = 0;
            return cbuf_get_data_size(&ctx->cbuf);
        }
        if (waited >= timeout_ms) {
            ctx->draining = 0;
            return cbuf_get_data_size(&ctx->cbuf);
        }
        os_time_dly(poll_tick);
        waited += poll_interval_ms;
    }

    while (empty_checks < 3) {
        delay = ai_audio_player_get_delay(AI_AUDIO_SINK_SOURCE);
        if (delay <= 0) {
            empty_checks++;
        } else {
            empty_checks = 0;
        }
        if (empty_checks >= 3) {
            break;
        }
        if (timeout_ms <= 0 || waited >= timeout_ms) {
            ctx->draining = 0;
            if (delay > 0) {
                printf("[ai_audio_sink] drain timeout: player_delay=%d\n", delay);
                return 1;
            }
            return 0;
        }
        os_time_dly(poll_tick);
        waited += poll_interval_ms;
    }

    ctx->draining = 0;
    return 0;
}

static int ai_audio_sink_default_close(void *sink_ctx)
{
    struct ai_audio_sink_default_ctx *ctx = (struct ai_audio_sink_default_ctx *)sink_ctx;

    if (!ctx || !ctx->opened) {
        return 0;
    }

    ctx->closed = 1;
    if (ctx->player_started) {
        ai_audio_player_close(AI_AUDIO_SINK_SOURCE);
    }
    if (ctx->buf) {
        cbuf_clear(&ctx->cbuf);
        ai_audio_sink_free(ctx->buf);
        ctx->buf = NULL;
    }
    ai_audio_sink_default_reset(ctx);
    return 0;
}

static const struct ai_audio_sink_ops ai_audio_sink_default_ops = {
    .open = ai_audio_sink_default_open,
    .write = ai_audio_sink_default_write,
    .drain = ai_audio_sink_default_drain,
    .close = ai_audio_sink_default_close,
};

int ai_audio_sink_register_ops(const struct ai_audio_sink_ops *ops)
{
    if (ai_audio_sink_is_open) {
        return -1;
    }

    ai_audio_sink_ops = ops;
    return 0;
}

int ai_audio_sink_open(const struct ai_audio_sink_config *cfg)
{
    const struct ai_audio_sink_ops *ops = ai_audio_sink_ops ? ai_audio_sink_ops : &ai_audio_sink_default_ops;
    int ret;

    if (!cfg || ai_audio_sink_is_open || !ops->open || !ops->write || !ops->close) {
        return -1;
    }

    ret = ops->open(cfg, &ai_audio_sink_ctx);
    if (ret) {
        ai_audio_sink_ctx = NULL;
        return ret;
    }

    ai_audio_sink_is_open = 1;
    return 0;
}

int ai_audio_sink_write(const unsigned char *data, unsigned int len)
{
    const struct ai_audio_sink_ops *ops = ai_audio_sink_ops ? ai_audio_sink_ops : &ai_audio_sink_default_ops;

    if (!ai_audio_sink_is_open || !ai_audio_sink_ctx || !ops->write || !data || !len) {
        return -1;
    }

    return ops->write(ai_audio_sink_ctx, data, len);
}

int ai_audio_sink_drain(int timeout_ms)
{
    const struct ai_audio_sink_ops *ops = ai_audio_sink_ops ? ai_audio_sink_ops : &ai_audio_sink_default_ops;

    if (!ai_audio_sink_is_open || !ai_audio_sink_ctx) {
        return -1;
    }
    if (!ops->drain) {
        return 0;
    }
    return ops->drain(ai_audio_sink_ctx, timeout_ms);
}

int ai_audio_sink_close(void)
{
    const struct ai_audio_sink_ops *ops = ai_audio_sink_ops ? ai_audio_sink_ops : &ai_audio_sink_default_ops;
    int ret;

    if (!ai_audio_sink_is_open || !ops->close) {
        return 0;
    }

    ret = ops->close(ai_audio_sink_ctx);
    ai_audio_sink_ctx = NULL;
    ai_audio_sink_is_open = 0;
    return ret;
}

#endif
