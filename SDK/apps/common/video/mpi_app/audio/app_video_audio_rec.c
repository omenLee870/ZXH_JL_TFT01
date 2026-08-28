#include "system/includes.h"
#include "generic/lbuf.h"
#include "jlstream.h"
#include "video_voice_recoder.h"

#include "app_video_audio_rec.h"

extern uint32_t timer_get_ms(void);

#define APP_VIDEO_AUDIO_REC_BUFFER_SIZE_DEFAULT  (64u * 1024u)
#define APP_VIDEO_AUDIO_REC_SAMPLE_RATE          8000u
#define APP_VIDEO_AUDIO_REC_CHANNELS             1u
#define APP_VIDEO_AUDIO_REC_BITS_PER_SAMPLE      16u
#define APP_VIDEO_AUDIO_REC_FRAME_INTERVAL_DEFAULT_MS 50u
#define APP_VIDEO_AUDIO_REC_LBUF_ALIGN           4
#define APP_VIDEO_AUDIO_REC_BYTES_PER_SAMPLE     (APP_VIDEO_AUDIO_REC_CHANNELS * APP_VIDEO_AUDIO_REC_BITS_PER_SAMPLE / 8u)
#define APP_VIDEO_AUDIO_REC_BYTES_PER_SEC        (APP_VIDEO_AUDIO_REC_SAMPLE_RATE * APP_VIDEO_AUDIO_REC_BYTES_PER_SAMPLE)

#define AUDIO_MAX_HANDLES  4

struct app_video_audio_rec {
    struct lbuff_head *lbuf;
    app_video_audio_rec_frame_t *pending_frame;
    uint8_t *buf;
    uint32_t buffer_size;
    uint32_t frame_size;
    uint32_t pending_len;
    uint32_t sample_rate;
    uint32_t codec_type;
    uint32_t bit_rate;
    uint8_t started;
};

/* multi-handle shared hardware management */
static app_video_audio_rec_t g_handles[AUDIO_MAX_HANDLES];
static int g_handle_count;
static int g_hw_refcount;

static app_video_audio_rec_frame_t *app_video_audio_rec_alloc_frame(
    app_video_audio_rec_t rec)
{
    app_video_audio_rec_frame_t *frame;

    frame = (app_video_audio_rec_frame_t *)lbuf_alloc(rec->lbuf, rec->frame_size);
    if (frame == NULL) {
        return NULL;
    }

    frame->len = 0;
    frame->timestamp_ms = timer_get_ms();
    return frame;
}

static void app_video_audio_rec_free_pending(app_video_audio_rec_t rec)
{
    if (rec->pending_frame != NULL) {
        lbuf_free(rec->pending_frame);
        rec->pending_frame = NULL;
    }
    rec->pending_len = 0;
}

static void feed_one_handle(app_video_audio_rec_t rec, const u8 *buf, u32 len)
{
    const u8 *src = buf;
    u32 remain = len;

    if (rec->codec_type != AUDIO_CODING_PCM) {
        /* Non-PCM (MP3 etc.): encoder outputs complete frames, push as-is */
        app_video_audio_rec_frame_t *frame;

        frame = (app_video_audio_rec_frame_t *)lbuf_alloc(rec->lbuf, len);
        if (frame == NULL) {
            return;
        }
        frame->len = len;
        frame->timestamp_ms = timer_get_ms();
        memcpy(frame->data, src, len);
        lbuf_push(frame, 1u);
        return;
    }

    /* PCM: accumulate to frame_size chunks */
    while (remain > 0) {
        u32 copy_len;

        if (rec->pending_frame == NULL) {
            rec->pending_frame = app_video_audio_rec_alloc_frame(rec);
            if (rec->pending_frame == NULL) {
                break;
            }
            rec->pending_len = 0;
        }

        copy_len = rec->frame_size - rec->pending_len;
        if (copy_len > remain) {
            copy_len = remain;
        }

        memcpy(rec->pending_frame->data + rec->pending_len, src, copy_len);
        rec->pending_len += copy_len;
        src += copy_len;
        remain -= copy_len;

        if (rec->pending_len == rec->frame_size) {
            rec->pending_frame->len = rec->frame_size;
            lbuf_push(rec->pending_frame, 1u);
            rec->pending_frame = NULL;
            rec->pending_len = 0;
        }
    }
}

static int app_video_audio_rec_tx_cb(u8 *buf, u32 len)
{
    int i;

    for (i = 0; i < g_handle_count; i++) {
        if (g_handles[i] && g_handles[i]->started) {
            feed_one_handle(g_handles[i], buf, len);
        }
    }
    return 0;
}

int app_video_audio_rec_open(app_video_audio_rec_t *handle,
                             const app_video_audio_rec_attr_t *attr)
{
    app_video_audio_rec_t rec;
    uint32_t buffer_size = APP_VIDEO_AUDIO_REC_BUFFER_SIZE_DEFAULT;
    uint32_t frame_interval_ms = APP_VIDEO_AUDIO_REC_FRAME_INTERVAL_DEFAULT_MS;
    uint32_t sample_rate = APP_VIDEO_AUDIO_REC_SAMPLE_RATE;
    uint32_t bytes_per_sec;
    uint32_t frame_size;
    uint64_t frame_bytes;

    if (handle == NULL) {
        return -EINVAL;
    }
    if (*handle != NULL) {
        return -EEXIST;
    }

    /* 让 attr 的 buffer_size/frame_interval_ms/sample_rate 生效(之前恒用默认值,
     * attr 的 buffer_size/frame_interval_ms 被静默丢弃)。0 = 用默认。 */
    if (attr != NULL) {
        if (attr->buffer_size != 0) {
            buffer_size = attr->buffer_size;
        }
        if (attr->frame_interval_ms != 0) {
            frame_interval_ms = attr->frame_interval_ms;
        }
        if (attr->sample_rate != 0) {
            sample_rate = attr->sample_rate;
        }
    }

    /* frame_size 按【实际采样率】算, 而非固定 BYTES_PER_SEC(8000Hz):
     * 否则 16kHz 录音下打包块大小/推送速率与真实数据流错配。 */
    bytes_per_sec = sample_rate * APP_VIDEO_AUDIO_REC_BYTES_PER_SAMPLE;
    frame_bytes = (uint64_t)bytes_per_sec * frame_interval_ms / 1000u;
    if (frame_bytes > 0xffffffffu) {
        return -EINVAL;
    }
    frame_size = (uint32_t)frame_bytes;
    frame_size = frame_size / APP_VIDEO_AUDIO_REC_BYTES_PER_SAMPLE * APP_VIDEO_AUDIO_REC_BYTES_PER_SAMPLE;
    if (frame_size == 0 || frame_size > buffer_size) {
        return -EINVAL;
    }

    rec = zalloc(sizeof(*rec));
    if (rec == NULL) {
        return -ENOMEM;
    }

    rec->sample_rate = APP_VIDEO_AUDIO_REC_SAMPLE_RATE;
    rec->codec_type = AUDIO_CODING_PCM;
    rec->bit_rate = 0;
    if (attr != NULL) {
        if (attr->sample_rate != 0) {
            rec->sample_rate = attr->sample_rate;
        }
        if (attr->codec_type != 0) {
            rec->codec_type = attr->codec_type;
        }
        if (attr->bit_rate != 0) {
            rec->bit_rate = attr->bit_rate;
        }
    }

    rec->buf = malloc(buffer_size);
    if (rec->buf == NULL) {
        free(rec);
        return -ENOMEM;
    }

    rec->buffer_size = buffer_size;
    rec->frame_size = frame_size;
    rec->lbuf = lbuf_init(rec->buf,
                          rec->buffer_size,
                          APP_VIDEO_AUDIO_REC_LBUF_ALIGN,
                          sizeof(app_video_audio_rec_frame_t));
    if (rec->lbuf == NULL || lbuf_avaliable(rec->lbuf, rec->frame_size) <= 0) {
        free(rec->buf);
        free(rec);
        return -ENOMEM;
    }

    *handle = rec;
    return 0;
}

static void register_handle(app_video_audio_rec_t rec)
{
    int i;
    for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
        if (g_handles[i] == NULL) {
            g_handles[i] = rec;
            if (i >= g_handle_count) {
                g_handle_count = i + 1;
            }
            return;
        }
    }
}

static void unregister_handle(app_video_audio_rec_t rec)
{
    int i;
    for (i = 0; i < g_handle_count; i++) {
        if (g_handles[i] == rec) {
            g_handles[i] = NULL;
            /* compact */
            while (g_handle_count > 0 && g_handles[g_handle_count - 1] == NULL) {
                g_handle_count--;
            }
            return;
        }
    }
}

int app_video_audio_rec_start(app_video_audio_rec_t handle)
{
    int ret;

    if (handle == NULL) {
        return -EINVAL;
    }
    if (handle->started) {
        return -EBUSY;
    }

    if (g_hw_refcount == 0) {
        ret = video_voice_recoder_open(handle->codec_type, 0, handle->sample_rate, handle->bit_rate);
        if (ret != 0) {
            return ret;
        }
        video_voice_recoder_set_video_tx_node_func(app_video_audio_rec_tx_cb);
    }

    app_video_audio_rec_free_pending(handle);
    lbuf_clear(handle->lbuf);

    register_handle(handle);
    g_hw_refcount++;
    handle->started = 1;

    return 0;
}

void app_video_audio_rec_stop(app_video_audio_rec_t handle)
{
    if (handle == NULL || !handle->started) {
        return;
    }

    handle->started = 0;
    unregister_handle(handle);
    app_video_audio_rec_free_pending(handle);

    g_hw_refcount--;
    if (g_hw_refcount == 0) {
        video_voice_recoder_set_video_tx_node_func(NULL);
        video_voice_recoder_close();
    }
}

void app_video_audio_rec_close(app_video_audio_rec_t *handle)
{
    app_video_audio_rec_t rec;

    if (handle == NULL || *handle == NULL) {
        return;
    }

    rec = *handle;
    app_video_audio_rec_stop(rec);
    lbuf_clear(rec->lbuf);
    free(rec->buf);
    free(rec);
    *handle = NULL;
}

int app_video_audio_rec_get_frame(app_video_audio_rec_t handle,
                                  app_video_audio_rec_frame_t **frame)
{
    if (handle == NULL || frame == NULL) {
        return -EINVAL;
    }

    *frame = (app_video_audio_rec_frame_t *)lbuf_pop(handle->lbuf, 1u);
    return *frame == NULL ? -EAGAIN : 0;
}

void app_video_audio_rec_release_frame(app_video_audio_rec_frame_t *frame)
{
    if (frame != NULL) {
        lbuf_free(frame);
    }
}

int app_video_audio_rec_get_format(app_video_audio_rec_t handle,
                                   app_video_audio_rec_format_t *format)
{
    if (handle == NULL || format == NULL) {
        return -EINVAL;
    }

    format->sample_rate = handle->sample_rate;
    format->channels = APP_VIDEO_AUDIO_REC_CHANNELS;
    format->bits_per_sample = APP_VIDEO_AUDIO_REC_BITS_PER_SAMPLE;

    return 0;
}
