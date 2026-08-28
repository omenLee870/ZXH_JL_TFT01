#include "system/includes.h"
#include "ai_audio_source.h"
#include "app_config.h"
#include "audio_def.h"
#include "ai_voice_recoder.h"

#include <stdlib.h>
#include <string.h>

#define AI_AUDIO_SOURCE_DEFAULT_BUFFER_SIZE (16 * 1024)
#define AI_AUDIO_SOURCE_DEFAULT_FORMAT      "opus"
#define AI_AUDIO_SOURCE_DEFAULT_RATE        16000
#define AI_AUDIO_SOURCE_DEFAULT_CH          1
#define AI_AUDIO_SOURCE_DROP_CHUNK          256

#if TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void free_psram(void *ptr);
#endif

struct ai_audio_source_default_ctx {
    char *buf;
    cbuffer_t cbuf;
    struct ai_audio_source_config cfg;
    int is_open;
    int is_closed;
    int is_error;
    int write_short_logged;
    volatile int is_end;
    unsigned int drop_bytes;
    unsigned int drop_frames;
    unsigned int drop_events;
    unsigned int last_drop_log_ms;
    /* The JieLi cbuffer is single-producer/single-consumer lockless, but here the
     * recorder callback (producer) also reads to drop-oldest on overflow, while
     * the upload thread (consumer) reads concurrently — two readers. This mutex
     * serializes every cbuf access. The recorder callback runs in task context
     * (it calls printf/sys_timer_get_ms), so a blocking mutex is safe there. */
    OS_MUTEX cbuf_mutex;
    int cbuf_mutex_initialized;
};

static struct ai_audio_source_default_ctx ai_audio_source_default;
static const struct ai_audio_source_ops *ai_audio_source_ops;
static void *ai_audio_source_ctx;
static int ai_audio_source_started;

static void ai_audio_source_cbuf_lock(struct ai_audio_source_default_ctx *ctx)
{
    if (ctx && ctx->cbuf_mutex_initialized) {
        os_mutex_pend(&ctx->cbuf_mutex, 0);
    }
}

static void ai_audio_source_cbuf_unlock(struct ai_audio_source_default_ctx *ctx)
{
    if (ctx && ctx->cbuf_mutex_initialized) {
        os_mutex_post(&ctx->cbuf_mutex);
    }
}

static void *ai_audio_source_malloc(unsigned int size)
{
#if TCFG_PSRAM_DEV_ENABLE
    return malloc_psram(size);
#else
    return malloc(size);
#endif
}

static void ai_audio_source_free(void *ptr)
{
#if TCFG_PSRAM_DEV_ENABLE
    free_psram(ptr);
#else
    free(ptr);
#endif
}

static int ai_audio_source_default_cbuf_init(struct ai_audio_source_default_ctx *ctx,
        unsigned int buffer_size)
{
    if (!ctx) {
        return -1;
    }

    if (!ctx->buf) {
        ctx->buf = (char *)ai_audio_source_malloc(buffer_size);
        if (!ctx->buf) {
            return -1;
        }
    }

    cbuf_init(&ctx->cbuf, ctx->buf, buffer_size);
    return 0;
}

static void ai_audio_source_default_cbuf_exit(struct ai_audio_source_default_ctx *ctx)
{
    if (!ctx || !ctx->buf) {
        return;
    }

    cbuf_clear(&ctx->cbuf);
    ai_audio_source_free(ctx->buf);
    ctx->buf = NULL;
}

static u32 ai_audio_source_code_type(const char *format)
{
    if (!format || !strcmp(format, "opus")) {
        return AUDIO_CODING_OPUS;
    }
    if (!strcmp(format, "speex")) {
        return AUDIO_CODING_SPEEX;
    }
    if (!strcmp(format, "pcm") || !strcmp(format, "pcm16")) {
        return AUDIO_CODING_PCM;
    }
    return 0;
}

static unsigned int ai_audio_source_drop_oldest(struct ai_audio_source_default_ctx *ctx,
        unsigned int need,
        unsigned int align)
{
    unsigned char drop[AI_AUDIO_SOURCE_DROP_CHUNK];
    unsigned int dropped = 0;

    /* Round the drop up to a whole frame (align == recorder frame size). Dropping
     * an arbitrary byte count would shift every subsequent read off the frame
     * boundary — corrupting all following opus packets / 16-bit PCM samples until
     * the stream is restarted. Rounding up keeps the buffer frame-aligned. */
    if (align > 1) {
        need = ((need + align - 1) / align) * align;
    }

    while (dropped < need && cbuf_get_data_size(&ctx->cbuf) > 0) {
        unsigned int chunk = need - dropped;
        int rlen;

        if (chunk > sizeof(drop)) {
            chunk = sizeof(drop);
        }
        rlen = cbuf_read(&ctx->cbuf, drop, chunk);
        if (rlen <= 0) {
            break;
        }
        dropped += rlen;
    }

    return dropped;
}

static int ai_audio_source_default_write(u8 *voice_buf, u32 voice_len)
{
    struct ai_audio_source_default_ctx *ctx = &ai_audio_source_default;
    unsigned int written;
    unsigned int dropped = 0;
    unsigned int lost = 0;
    int overflow = 0;
    int wlen;

    if (!ctx->buf || !voice_buf || !voice_len) {
        return voice_len;
    }

    ai_audio_source_cbuf_lock(ctx);
    wlen = cbuf_write(&ctx->cbuf, voice_buf, voice_len);
    written = wlen > 0 ? (unsigned int)wlen : 0;
    if (written < voice_len) {
        int retry;

        /* Drop oldest in whole-frame units to make room, then finish writing the
         * rest of THIS frame so it is stored intact (its two pieces are contiguous
         * at the tail; the drop happens at the head). */
        dropped = ai_audio_source_drop_oldest(ctx, voice_len - written, voice_len);
        retry = cbuf_write(&ctx->cbuf, voice_buf + written, voice_len - written);
        if (retry > 0) {
            written += retry;
        }
        lost = voice_len - written;
        ctx->drop_bytes += dropped + lost;
        ctx->drop_frames++;
        ctx->drop_events++;
        overflow = 1;
    }
    ai_audio_source_cbuf_unlock(ctx);

    if (overflow) {
        unsigned int now_ms = sys_timer_get_ms();

        if (!ctx->write_short_logged || (unsigned int)(now_ms - ctx->last_drop_log_ms) >= 1000) {
            printf("[ai_audio] cbuf overflow drop=%u lost=%u total=%u events=%u\n",
                   dropped,
                   lost,
                   ctx->drop_bytes,
                   ctx->drop_events);
            ctx->write_short_logged = 1;
            ctx->last_drop_log_ms = now_ms;
        }
    }

    return voice_len;
}

static void ai_audio_source_default_reset(struct ai_audio_source_default_ctx *ctx)
{
    if (!ctx) {
        return;
    }

    memset(&ctx->cfg, 0, sizeof(ctx->cfg));
    ctx->is_open = 0;
    ctx->is_closed = 1;
    ctx->is_error = 0;
    ctx->write_short_logged = 0;
    ctx->is_end = 1;
    ctx->drop_bytes = 0;
    ctx->drop_frames = 0;
    ctx->drop_events = 0;
    ctx->last_drop_log_ms = 0;
}

static int ai_audio_source_default_start(const struct ai_audio_source_config *cfg, void **out_ctx)
{
    struct ai_audio_source_default_ctx *ctx = &ai_audio_source_default;
    const char *format;
    int sample_rate;
    int channels;
    unsigned int buffer_size;
    u32 code_type;
    int err;

    if (!out_ctx || ctx->is_open) {
        return -1;
    }

    format = (cfg && cfg->format) ? cfg->format : AI_AUDIO_SOURCE_DEFAULT_FORMAT;
    sample_rate = (cfg && cfg->sample_rate > 0) ? cfg->sample_rate : AI_AUDIO_SOURCE_DEFAULT_RATE;
    channels = (cfg && cfg->channels > 0) ? cfg->channels : AI_AUDIO_SOURCE_DEFAULT_CH;
    buffer_size = (cfg && cfg->buffer_size > 0) ? (unsigned int)cfg->buffer_size : AI_AUDIO_SOURCE_DEFAULT_BUFFER_SIZE;
    code_type = ai_audio_source_code_type(format);
    if (!code_type) {
        printf("[ai_audio] unsupported source format=%s\n", format ? format : "");
        return -1;
    }

    if (ai_audio_source_default_cbuf_init(ctx, buffer_size) != 0) {
        return -1;
    }

    ctx->cfg = cfg ? *cfg : (struct ai_audio_source_config) {
        0
    };
    ctx->is_open = 0;
    ctx->is_closed = 0;
    ctx->is_error = 0;
    ctx->write_short_logged = 0;
    ctx->is_end = 0;

    err = ai_voice_recoder_open(code_type, 0);
    if (err) {
        printf("[ai_audio] ai_voice_recoder_open failed: format=%s code=0x%x err=%d\n",
               format ? format : "", code_type, err);
        ai_audio_source_default_cbuf_exit(ctx);
        ai_audio_source_default_reset(ctx);
        return err;
    }

    /* Create the cbuf mutex before registering the tx callback, so no recorder
     * frame can hit the buffer before the lock exists. */
    if (!ctx->cbuf_mutex_initialized) {
        if (os_mutex_create(&ctx->cbuf_mutex)) {
            ai_voice_recoder_close();
            ai_audio_source_default_cbuf_exit(ctx);
            ai_audio_source_default_reset(ctx);
            return -1;
        }
        ctx->cbuf_mutex_initialized = 1;
    }

    ai_voice_recoder_set_ai_tx_node_func(ai_audio_source_default_write);

    ctx->is_open = 1;
    *out_ctx = ctx;
    printf("[ai_audio] source started format=%s sample=%d channels=%d\n",
           format, sample_rate, channels);
    return 0;
}

static int ai_audio_source_default_stop(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;

    if (!ctx) {
        return 0;
    }

    if (ctx->is_open) {
        ai_voice_recoder_close();
    }

    ai_audio_source_default_cbuf_exit(ctx);
    /* Recorder is closed (no more tx callbacks) and the buffer is freed, so it is
     * safe to tear the mutex down. */
    if (ctx->cbuf_mutex_initialized) {
        os_mutex_del(&ctx->cbuf_mutex, OS_DEL_ALWAYS);
        ctx->cbuf_mutex_initialized = 0;
    }
    ai_audio_source_default_reset(ctx);
    return 0;
}

static int ai_audio_source_default_read(void *source_ctx, unsigned char *buf, unsigned int len)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    unsigned int data_len;
    int rlen;

    if (!ctx || !ctx->buf || !buf || !len) {
        return 0;
    }

    ai_audio_source_cbuf_lock(ctx);
    data_len = cbuf_get_data_size(&ctx->cbuf);
    if (data_len < len && !ctx->is_end) {
        ai_audio_source_cbuf_unlock(ctx);
        return 0;
    }
    if (data_len < len) {
        len = data_len;
    }
    rlen = len ? cbuf_read(&ctx->cbuf, buf, len) : 0;
    ai_audio_source_cbuf_unlock(ctx);

    return rlen;
}

static int ai_audio_source_default_data_len(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    int size;

    if (!ctx || !ctx->buf) {
        return 0;
    }

    ai_audio_source_cbuf_lock(ctx);
    size = cbuf_get_data_size(&ctx->cbuf);
    ai_audio_source_cbuf_unlock(ctx);

    return size;
}

static int ai_audio_source_default_is_end(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    return ctx ? ctx->is_end : 1;
}

static int ai_audio_source_default_is_closed(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    return ctx ? ctx->is_closed : 1;
}

static int ai_audio_source_default_is_error(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    return ctx ? ctx->is_error : 1;
}

static int ai_audio_source_default_is_open(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    return ctx ? ctx->is_open : 0;
}

static int ai_audio_source_default_is_speaking(void *source_ctx)
{
    struct ai_audio_source_default_ctx *ctx = (struct ai_audio_source_default_ctx *)source_ctx;
    /* ponytail: ai_voice visual flow exposes data callbacks here, not VAD events; add node event plumbing if callers need VAD edges. */
    return ctx ? ctx->is_open : 0;
}

static int ai_audio_source_default_is_speak_stopped(void *source_ctx)
{
    (void)source_ctx;
    return 0;
}

static const struct ai_audio_source_ops ai_audio_source_default_ops = {
    .start = ai_audio_source_default_start,
    .stop = ai_audio_source_default_stop,
    .read = ai_audio_source_default_read,
    .data_len = ai_audio_source_default_data_len,
    .is_end = ai_audio_source_default_is_end,
    .is_closed = ai_audio_source_default_is_closed,
    .is_error = ai_audio_source_default_is_error,
    .is_open = ai_audio_source_default_is_open,
    .is_speaking = ai_audio_source_default_is_speaking,
    .is_speak_stopped = ai_audio_source_default_is_speak_stopped,
};

static const struct ai_audio_source_ops *ai_audio_source_active_ops(void)
{
    return ai_audio_source_ops ? ai_audio_source_ops : &ai_audio_source_default_ops;
}

int ai_audio_source_register_ops(const struct ai_audio_source_ops *ops)
{
    if (ai_audio_source_started) {
        return -1;
    }

    ai_audio_source_ops = ops;
    return 0;
}

int ai_audio_source_start(const struct ai_audio_source_config *cfg)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    int ret;

    if (!ops || !ops->start || ai_audio_source_started) {
        return -1;
    }

    ret = ops->start(cfg, &ai_audio_source_ctx);
    if (ret) {
        ai_audio_source_ctx = NULL;
        return ret;
    }

    ai_audio_source_started = 1;
    return 0;
}

int ai_audio_source_stop(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    int ret;

    if (!ai_audio_source_started || !ops || !ops->stop) {
        return 0;
    }

    ret = ops->stop(ai_audio_source_ctx);
    ai_audio_source_ctx = NULL;
    ai_audio_source_started = 0;
    return ret;
}

int ai_audio_source_read(unsigned char *buf, unsigned int len)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();

    if (!ops || !ops->read || !ai_audio_source_started) {
        return 0;
    }

    return ops->read(ai_audio_source_ctx, buf, len);
}

int ai_audio_source_data_len(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();

    if (!ops || !ops->data_len || !ai_audio_source_started) {
        return 0;
    }

    return ops->data_len(ai_audio_source_ctx);
}

int ai_audio_source_is_end(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_end) ? ops->is_end(ai_audio_source_ctx) : 1;
}

int ai_audio_source_is_closed(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_closed) ? ops->is_closed(ai_audio_source_ctx) : 1;
}

int ai_audio_source_is_error(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_error) ? ops->is_error(ai_audio_source_ctx) : 1;
}

int ai_audio_source_is_open(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_open) ? ops->is_open(ai_audio_source_ctx) : 0;
}

int ai_audio_source_is_speaking(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_speaking) ? ops->is_speaking(ai_audio_source_ctx) : 0;
}

int ai_audio_source_is_speak_stopped(void)
{
    const struct ai_audio_source_ops *ops = ai_audio_source_active_ops();
    return (ops && ops->is_speak_stopped) ? ops->is_speak_stopped(ai_audio_source_ctx) : 0;
}

int ai_audio_source_get_drop_stats(struct ai_audio_source_drop_stats *out)
{
    if (!out) {
        return -1;
    }

    out->bytes = ai_audio_source_default.drop_bytes;
    out->frames = ai_audio_source_default.drop_frames;
    out->events = ai_audio_source_default.drop_events;
    return 0;
}
