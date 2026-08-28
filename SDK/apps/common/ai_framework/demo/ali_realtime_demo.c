#include "system/includes.h"
#include "app_config.h"
#include "ai_realtime.h"
#include "ali_realtime_demo.h"
#include "ai_audio_sink.h"
#include "fs/fs.h"

#include <stdlib.h>
#include <string.h>

#if TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void free_psram(void *ptr);
#endif

static void *ali_realtime_demo_malloc_large(size_t size)
{
#if TCFG_PSRAM_DEV_ENABLE
    return malloc_psram(size);
#else
    return malloc(size);
#endif
}

static void ali_realtime_demo_free_large(void *ptr)
{
    if (!ptr) {
        return;
    }
#if TCFG_PSRAM_DEV_ENABLE
    free_psram(ptr);
#else
    free(ptr);
#endif
}

/* API document: https://help.aliyun.com/zh/model-studio/realtime?spm=a2c4g.11186623.help-menu-2400256.d_0_3_9_0.30fd35ccDfuImF */
#ifndef ALI_REALTIME_API_KEY
#define ALI_REALTIME_API_KEY ""
#endif

#define ALI_REALTIME_CAPTURE_MS 10000
#define ALI_REALTIME_WAIT_MS    60000
#ifndef ALI_REALTIME_STATIC_PCM_ENABLE
#define ALI_REALTIME_STATIC_PCM_ENABLE 0
#endif
#ifndef ALI_REALTIME_VAD_ENABLE
#define ALI_REALTIME_VAD_ENABLE 1
#endif
#define ALI_REALTIME_VAD_THRESHOLD          0.5f
#define ALI_REALTIME_VAD_SILENCE_MS         800u
#define ALI_REALTIME_VAD_IDLE_TIMEOUT_MS    0u
#define ALI_REALTIME_PCM_TEST_PATH     "storage/sd1/C/1.pcm"
#define ALI_REALTIME_IMAGE_TEST_PATH   "storage/sd1/C/1.jpg"
#define ALI_REALTIME_AUDIO_DUMP_PATH   "storage/sd1/C/ali_rt_out_24k.pcm"
#ifndef ALI_REALTIME_AUDIO_PLAY_ENABLE
#define ALI_REALTIME_AUDIO_PLAY_ENABLE 1
#endif
#ifndef ALI_REALTIME_AUDIO_DUMP_ENABLE
#define ALI_REALTIME_AUDIO_DUMP_ENABLE 0
#endif
#if TCFG_PSRAM_DEV_ENABLE
#define ALI_REALTIME_AUDIO_BUFFER_SIZE (512 * 1024)
#else
#define ALI_REALTIME_AUDIO_BUFFER_SIZE (48 * 1024)
#endif
#define ALI_REALTIME_AUDIO_PREBUFFER_SIZE (24 * 1024)
#define ALI_REALTIME_AUDIO_FRAME_BYTES 960
#define ALI_REALTIME_AUDIO_DRAIN_MS    15000
#define ALI_REALTIME_RECONNECT_DELAY_TICKS 100
#define ALI_REALTIME_IMAGE_TEST_PROMPT "你是儿童相机里的AI助手,如果有收到图片请描述下图片内容, 如果没收到图片请回复没有收到，并说明什么问题 不要胡言乱语，现在是测试阶段正常我每次发送的请求都是有图片数据的，如果你那边没有识别到图片，请告诉我原因"

extern u32 timer_get_ms(void);

static volatile int ali_realtime_demo_running;
static volatile int ali_realtime_demo_started;
static volatile int ali_realtime_demo_stop_requested;

struct ali_realtime_demo_input {
    unsigned char *image_data;
    unsigned int image_len;
    int use_sd_image;
};

static int ali_realtime_demo_jpeg_size(const unsigned char *data, unsigned int len,
                                       unsigned int *width, unsigned int *height, int *is_420)
{
    unsigned int pos = 2;

    if (!data || len < 4 || data[0] != 0xff || data[1] != 0xd8 || !width || !height || !is_420) {
        return -1;
    }
    *is_420 = 0;

    while (pos + 8 < len) {
        unsigned int marker;
        unsigned int seg_len;

        if (data[pos] != 0xff) {
            pos++;
            continue;
        }
        while (pos < len && data[pos] == 0xff) {
            pos++;
        }
        if (pos >= len) {
            break;
        }

        marker = data[pos++];
        if (marker == 0xd9 || marker == 0xda) {
            break;
        }
        if (pos + 2 > len) {
            break;
        }

        seg_len = ((unsigned int)data[pos] << 8) | data[pos + 1];
        if (seg_len < 2 || pos + seg_len > len) {
            break;
        }

        if ((marker >= 0xc0 && marker <= 0xc3) || (marker >= 0xc5 && marker <= 0xc7) ||
            (marker >= 0xc9 && marker <= 0xcb) || (marker >= 0xcd && marker <= 0xcf)) {
            unsigned int components;

            if (seg_len < 8) {
                return -1;
            }
            components = data[pos + 7];
            if (seg_len < 8 + components * 3) {
                return -1;
            }
            *height = ((unsigned int)data[pos + 3] << 8) | data[pos + 4];
            *width = ((unsigned int)data[pos + 5] << 8) | data[pos + 6];
            *is_420 = components == 3 && data[pos + 9] == 0x22 &&
                      data[pos + 12] == 0x11 && data[pos + 15] == 0x11;
            return 0;
        }

        pos += seg_len;
    }

    return -1;
}

static void ali_realtime_demo_log_bytes(const char *tag, const unsigned char *data, unsigned int len)
{
    unsigned int n = len > 32 ? 32 : len;

    printf("[ali_rt] %s: len=%u head=", tag ? tag : "bytes", len);
    put_buf(data, n);
}

static unsigned char *ali_realtime_demo_read_file(const char *path, unsigned int *out_len)
{
    FILE *fd;
    unsigned char *buf;
    int len;
    int rlen;

    if (!path || !out_len) {
        return NULL;
    }

    *out_len = 0;
    fd = fopen(path, "r");
    if (!fd) {
        printf("[ali_rt] image open failed: %s\n", path);
        return NULL;
    }

    len = flen(fd);
    if (len <= 0) {
        printf("[ali_rt] invalid image len: %d\n", len);
        fclose(fd);
        return NULL;
    }

    buf = (unsigned char *)ali_realtime_demo_malloc_large(len);
    if (!buf) {
        printf("[ali_rt] image malloc failed: %d\n", len);
        fclose(fd);
        return NULL;
    }

    memset(buf, 0, len);

    rlen = fread(buf, len, 1, fd);
    fclose(fd);
    if (rlen != len) {
        printf("[ali_rt] image read failed: %d/%d\n", rlen, len);
        ali_realtime_demo_free_large(buf);
        return NULL;
    }

    put_buf(buf, 32);
    *out_len = (unsigned int)len;
    ali_realtime_demo_log_bytes("image file raw head", buf, (unsigned int)len);
    return buf;
}

struct ali_realtime_demo_ctx {
    volatile int response_seen;
    volatile int session_updated;
    volatile int done;
    volatile int committed;
    volatile int closed;
    volatile int error;
    volatile int playback_active;
    volatile int playback_done;
    unsigned int turns;
    int audio_sink_opened;
    unsigned int audio_sink_write_errors;
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
    FILE *audio_dump;
    unsigned int audio_dump_bytes;
#endif
};

static void ali_realtime_mark_response(struct ali_realtime_demo_ctx *ctx)
{
    if (ctx && !ctx->response_seen) {
        ctx->response_seen = 1;
        printf("[ali_rt] first response at %u ms\n", timer_get_ms());
    }
}

#if ALI_REALTIME_AUDIO_DUMP_ENABLE
static void ali_realtime_audio_dump_close(struct ali_realtime_demo_ctx *ctx)
{
    if (!ctx || !ctx->audio_dump) {
        return;
    }

    fclose(ctx->audio_dump);
    ctx->audio_dump = NULL;
    printf("[ali_rt] audio dump saved: %s bytes=%u\n",
           ALI_REALTIME_AUDIO_DUMP_PATH, ctx->audio_dump_bytes);
}

static void ali_realtime_audio_dump_write(struct ali_realtime_demo_ctx *ctx,
        const unsigned char *data,
        unsigned int len)
{
    int wlen;

    if (!ctx || !data || !len) {
        return;
    }

    if (!ctx->audio_dump) {
        ctx->audio_dump = fopen(ALI_REALTIME_AUDIO_DUMP_PATH, "w+");
        ctx->audio_dump_bytes = 0;
        if (!ctx->audio_dump) {
            printf("[ali_rt] audio dump open failed: %s\n", ALI_REALTIME_AUDIO_DUMP_PATH);
            return;
        }
        printf("[ali_rt] audio dump open: %s\n", ALI_REALTIME_AUDIO_DUMP_PATH);
    }

    wlen = fwrite((void *)data, 1, len, ctx->audio_dump);
    if (wlen != (int)len) {
        printf("[ali_rt] audio dump write failed: %d/%u\n", wlen, len);
        ali_realtime_audio_dump_close(ctx);
        return;
    }
    ctx->audio_dump_bytes += len;
}
#endif

#if ALI_REALTIME_STATIC_PCM_ENABLE
static int ali_realtime_send_static_pcm(struct ai_realtime_session *session)
{
    FILE *fd;
    unsigned char frame[640];
    unsigned int frames = 0;
    unsigned int bytes = 0;
    int rlen;
    int ret;

    fd = fopen(ALI_REALTIME_PCM_TEST_PATH, "r");
    if (!fd) {
        printf("[ali_rt] pcm open failed: %s\n", ALI_REALTIME_PCM_TEST_PATH);
        return AI_ERR_PARAM;
    }

    printf("[ali_rt] send pcm file: %s\n", ALI_REALTIME_PCM_TEST_PATH);
    while ((rlen = fread(frame, sizeof(frame), 1, fd)) > 0) {
        ret = ai_realtime_send_audio(session, frame, (unsigned int)rlen);
        if (ret != AI_OK) {
            printf("[ali_rt] pcm send failed: %d at frame=%u bytes=%u\n", ret, frames, bytes);
            fclose(fd);
            return ret;
        }
        frames++;
        bytes += (unsigned int)rlen;
        ai_realtime_poll(session, 0);
        os_time_dly(2);
    }
    fclose(fd);

    printf("[ali_rt] pcm file sent: frames=%u bytes=%u\n", frames, bytes);
    while (ai_realtime_pending(session) > 0) {
        ai_realtime_poll(session, 0);
        os_time_dly(1);
    }
    return bytes ? AI_OK : AI_ERR_PARAM;
}
#endif

static void ali_realtime_demo_cb(const struct ai_dialogue_event *event, void *user)
{
    struct ali_realtime_demo_ctx *ctx = (struct ali_realtime_demo_ctx *)user;

    if (!event || !ctx) {
        return;
    }

    switch (event->type) {
    case AI_DIALOGUE_CONNECTED:
        printf("[ali_rt] connected\n");
        break;
    case AI_DIALOGUE_TEXT_DELTA:
    case AI_DIALOGUE_TEXT_DONE:
    case AI_DIALOGUE_TRANSCRIPT:
        if (event->text) {
            printf("[ali_rt] text: %s\n", event->text);
            if (strstr(event->text, "session.updated")) {
                ctx->session_updated = 1;
                printf("[ali_rt] session updated\n");
            }
            if (strstr(event->text, "input_audio_buffer.committed")) {
                ctx->committed = 1;
                printf("[ali_rt] input committed\n");
            }
            ali_realtime_mark_response(ctx);
        }
        if (event->type == AI_DIALOGUE_TEXT_DONE) {
            ctx->done = 1;
        }
        break;
    case AI_DIALOGUE_AUDIO_DELTA:
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
        if (!ctx->playback_active) {
            ctx->playback_active = 1;
            ctx->playback_done = 0;
            printf("[ali_rt] playback started, capture will pause\n");
        }
#endif
        printf("[ali_rt] audio delta: %u bytes\n", event->data_len);
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
        if (ctx->audio_sink_opened) {
            int wlen = ai_audio_sink_write(event->data, event->data_len);

            if (wlen != (int)event->data_len) {
                ctx->audio_sink_write_errors++;
                printf("[ali_rt] audio sink write failed: %d/%u errors=%u\n",
                       wlen, event->data_len, ctx->audio_sink_write_errors);
            }
        } else {
            printf("[ali_rt] audio sink is not open\n");
        }
#endif
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
        ali_realtime_audio_dump_write(ctx, event->data, event->data_len);
#endif
        ali_realtime_mark_response(ctx);
        break;
    case AI_DIALOGUE_AUDIO_URL:
        printf("[ali_rt] audio url: %s\n", event->url ? event->url : "");
        ali_realtime_mark_response(ctx);
        break;
    case AI_DIALOGUE_SPEECH_STARTED:
        printf("[ali_rt] vad speech started\n");
        break;
    case AI_DIALOGUE_SPEECH_STOPPED:
        printf("[ali_rt] vad speech stopped\n");
        break;
    case AI_DIALOGUE_DONE:
        ctx->turns++;
        printf("[ali_rt] turn done: %u\n", ctx->turns);
        if (ctx->playback_active) {
            ctx->playback_done = 1;
        }
        ctx->done = 1;
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
        ali_realtime_audio_dump_close(ctx);
#endif
        ali_realtime_mark_response(ctx);
        break;
    case AI_DIALOGUE_ERROR:
        printf("[ali_rt] error %d: %s raw=%s\n",
               event->error_code,
               event->text ? event->text : "",
               event->raw_json ? event->raw_json : "");
        ctx->error = 1;
        ctx->playback_done = 1;
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
        ali_realtime_audio_dump_close(ctx);
#endif
        break;
    case AI_DIALOGUE_CLOSED:
        printf("[ali_rt] closed response=%d done=%d error=%d\n", ctx->response_seen, ctx->done, ctx->error);
        ctx->closed = 1;
        ctx->playback_done = 1;
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
        ali_realtime_audio_dump_close(ctx);
#endif
        break;
    default:
        break;
    }
}

static void ali_realtime_demo_task(void *priv)
{
    struct ali_realtime_demo_input *input = (struct ali_realtime_demo_input *)priv;
#if !ALI_REALTIME_STATIC_PCM_ENABLE
    struct ai_audio_source_config audio_cfg = {
        .format = "pcm",
        .sample_rate = 16000,
        .channels = 1,
        .volume = 80,
        .channel_bit_map = BIT(1),
        .buffer_size = 64 * 1024,
        .use_vad = 0,
        .vad_start_threshold = 0,
        .vad_stop_threshold = 0,
    };
#if ALI_REALTIME_VAD_ENABLE
    struct ai_turn_detection_config turn_detection = {
        .type = AI_TURN_DETECTION_SERVER_VAD,
        .has_threshold = 1,
        .threshold = ALI_REALTIME_VAD_THRESHOLD,
        .silence_duration_ms = ALI_REALTIME_VAD_SILENCE_MS,
        .idle_timeout_ms = ALI_REALTIME_VAD_IDLE_TIMEOUT_MS,
        .create_response = 1,
    };
#endif
#endif
    struct ai_realtime_config cfg;
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
    struct ai_audio_sink_config sink_cfg = {
        .format = AI_AUDIO_SINK_PCM16,
        .sample_rate = 24000,
        .channels = 1,
        .bits_per_sample = 16,
        .frame_bytes = ALI_REALTIME_AUDIO_FRAME_BYTES,
        .volume = 100,
        .buffer_size = ALI_REALTIME_AUDIO_BUFFER_SIZE,
        .prebuffer_bytes = ALI_REALTIME_AUDIO_PREBUFFER_SIZE,
    };
#endif
    struct ai_realtime_session *session = NULL;
    struct ali_realtime_demo_ctx ctx = {0};
    unsigned char *image_buf = NULL;
    const unsigned char *image_data = NULL;
    unsigned int image_len = 0;
    unsigned int image_width = 0;
    unsigned int image_height = 0;
    int image_is_420 = 0;
    int use_image = 0;
    int use_vad = 0;
    const char *image_source = NULL;
    unsigned int start_ms;
    int ret;

    if (input) {
        if (input->image_data) {
            image_buf = input->image_data;
            image_len = input->image_len;
            image_source = "camera-jpeg-buffer";
            use_image = 1;
        } else if (input->use_sd_image) {
            image_source = ALI_REALTIME_IMAGE_TEST_PATH;
            use_image = 1;
        }
        free(input);
        input = NULL;
    }
#if ALI_REALTIME_VAD_ENABLE && !ALI_REALTIME_STATIC_PCM_ENABLE
    use_vad = !use_image;
#endif

    if (!ALI_REALTIME_API_KEY[0]) {
        printf("[ali_rt] missing ALI_REALTIME_API_KEY\n");
        goto out;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.provider_name = "ali_realtime";
    cfg.api_key = ALI_REALTIME_API_KEY;
    cfg.provider_params = use_image ? ALI_REALTIME_IMAGE_TEST_PROMPT : NULL;
#if ALI_REALTIME_VAD_ENABLE && !ALI_REALTIME_STATIC_PCM_ENABLE
    cfg.turn_detection = use_vad ? &turn_detection : NULL;
#endif
#if !ALI_REALTIME_STATIC_PCM_ENABLE
    cfg.audio_cfg = &audio_cfg;
#endif
    cfg.dialogue_cb = ali_realtime_demo_cb;
    cfg.dialogue_user = &ctx;
    cfg.timeout_ms = 10000;

    printf("[ali_rt] realtime demo start mode=%s capture=%u wait=%u\n",
           use_vad ? "server_vad" : "manual",
           ALI_REALTIME_CAPTURE_MS,
           ALI_REALTIME_WAIT_MS);

    if (use_image) {
        if (!image_buf) {
            image_buf = ali_realtime_demo_read_file(ALI_REALTIME_IMAGE_TEST_PATH, &image_len);
        }
        if (image_buf && ali_realtime_demo_jpeg_size(image_buf, image_len, &image_width, &image_height,
                &image_is_420) == 0 && image_is_420) {
            image_data = image_buf;
            printf("[ali_rt] use image len=%u size=%ux%u source=%s\n", image_len, image_width, image_height,
                   image_source);
        } else if (!image_buf) {
            printf("[ali_rt] image read failed: %s\n", image_source);
            goto out;
        } else {
            printf("[ali_rt] image rejected: path=%s len=%u size=%ux%u jpeg420=%d\n",
                   image_source,
                   image_len,
                   image_width,
                   image_height,
                   image_is_420);
            goto out;
        }
    } else {
        printf("[ali_rt] audio-only conversation\n");
    }

reconnect_session:
    ret = ai_realtime_open(&cfg, &session);
    if (ret != AI_OK) {
        printf("[ali_rt] open failed: %d\n", ret);
        if (use_vad) {
            goto reconnect_vad;
        }
        goto out;
    }

    ret = ai_realtime_start(session);
    if (ret != AI_OK) {
        printf("[ali_rt] start failed: %d\n", ret);
        if (use_vad) {
            goto reconnect_vad;
        }
        goto out;
    }

    start_ms = timer_get_ms();
    while (!ctx.session_updated && !ctx.closed && !ctx.error && (timer_get_ms() - start_ms) < 5000) {
        ai_realtime_poll(session, 20);
    }
    if (!ctx.session_updated) {
        printf("[ali_rt] wait session.updated timeout/error updated=%d closed=%d error=%d\n",
               ctx.session_updated, ctx.closed, ctx.error);
        if (use_vad) {
            goto reconnect_vad;
        }
        goto out;
    }

#if ALI_REALTIME_AUDIO_PLAY_ENABLE
    if (!ctx.audio_sink_opened) {
        ret = ai_audio_sink_open(&sink_cfg);
        if (ret) {
            printf("[ali_rt] audio sink open failed: %d\n", ret);
            goto out;
        }
        ctx.audio_sink_opened = 1;
        printf("[ali_rt] audio sink open: pcm 24000 Hz mono buffer=%u prebuffer=%u frame=%u\n",
               sink_cfg.buffer_size, sink_cfg.prebuffer_bytes, sink_cfg.frame_bytes);
    }
#endif

#if ALI_REALTIME_VAD_ENABLE && !ALI_REALTIME_STATIC_PCM_ENABLE
    if (use_vad) {
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
        if (ctx.playback_active) {
            printf("[ali_rt] wait buffered playback before capture\n");
            while (!ali_realtime_demo_stop_requested && ai_audio_sink_drain(0) > 0) {
                ai_realtime_poll(session, 20);
                os_time_dly(1);
            }
            if (ali_realtime_demo_stop_requested) {
                goto out;
            }
            ctx.playback_active = 0;
            ctx.playback_done = 0;
        }
#endif
        ret = ai_realtime_start_upload(session, 0);
        if (ret != AI_OK) {
            printf("[ali_rt] start VAD upload failed: %d\n", ret);
            goto reconnect_vad;
        }
        printf("[ali_rt] VAD upload running: append only, server auto commit/response\n");
        while (!ali_realtime_demo_stop_requested && !ctx.closed && !ctx.error) {
            ai_realtime_poll(session, 20);
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
            if (ctx.playback_active && ai_realtime_upload_running(session)) {
                ret = ai_realtime_stop_upload(session);
                if (ret != AI_OK) {
                    printf("[ali_rt] pause capture failed: %d\n", ret);
                    ctx.error = 1;
                    break;
                }
                printf("[ali_rt] capture paused for playback\n");
            }
            if (ctx.playback_active && ctx.playback_done && ai_audio_sink_drain(0) == 0) {
                ctx.playback_active = 0;
                ctx.playback_done = 0;
                ret = ai_realtime_start_upload(session, 0);
                if (ret != AI_OK) {
                    printf("[ali_rt] resume capture failed: %d\n", ret);
                    ctx.error = 1;
                    break;
                }
                printf("[ali_rt] playback drained, capture resumed\n");
            }
#endif
        }
        if (ali_realtime_demo_stop_requested) {
            goto out;
        }
        goto reconnect_vad;
    }
#endif

#if ALI_REALTIME_STATIC_PCM_ENABLE
    ret = ali_realtime_send_static_pcm(session);
    if (ret != AI_OK) {
        goto out;
    }
    printf("[ali_rt] static pcm sent\n");
#else
    ret = ai_realtime_start_upload(session, 0);
    if (ret != AI_OK) {
        printf("[ali_rt] start upload failed: %d\n", ret);
        goto out;
    }

    start_ms = timer_get_ms();
    while ((timer_get_ms() - start_ms) < ALI_REALTIME_CAPTURE_MS) {
        ai_realtime_poll(session, 20);
    }

    printf("[ali_rt] stop upload, commit and response.create\n");
    ret = ai_realtime_stop_upload(session);
    if (ret != AI_OK) {
        printf("[ali_rt] stop upload failed: %d\n", ret);
        goto out;
    }
#endif

    if (image_data) {
        printf("[ali_rt] send image: %u bytes source=%s\n", image_len, image_source);
        ret = ai_realtime_send_image(session, image_data, image_len);
        if (ret != AI_OK) {
            printf("[ali_rt] jpeg send failed: %d\n", ret);
            goto out;
        }
        while (ai_realtime_pending(session) > 0) {
            ai_realtime_poll(session, 0);
            os_time_dly(1);
        }
    }

    ret = ai_realtime_commit(session);
    if (ret != AI_OK) {
        printf("[ali_rt] commit failed: %d\n", ret);
        goto out;
    }

    start_ms = timer_get_ms();
    while (!ctx.committed && !ctx.closed && !ctx.error && (timer_get_ms() - start_ms) < 5000) {
        ai_realtime_poll(session, 20);
    }
    if (!ctx.committed) {
        printf("[ali_rt] wait committed timeout/error committed=%d closed=%d error=%d\n",
               ctx.committed, ctx.closed, ctx.error);
        goto out;
    }

    ret = ai_realtime_response_create(session);
    if (ret != AI_OK) {
        printf("[ali_rt] response.create failed: %d\n", ret);
        goto out;
    }

    ret = ai_realtime_wait_response(session, ALI_REALTIME_WAIT_MS);
    if (ret != AI_OK || !ctx.done) {
        printf("[ali_rt] wait response failed: ret=%d done=%d closed=%d error=%d\n",
               ret, ctx.done, ctx.closed, ctx.error);
        goto out;
    }

reconnect_vad:
    if (!use_vad || ali_realtime_demo_stop_requested) {
        goto out;
    }

    printf("[ali_rt] VAD session restart after startup failure or disconnect: closed=%d error=%d\n", ctx.closed, ctx.error);
    ai_realtime_destroy(session);
    session = NULL;
    ctx.session_updated = 0;
    ctx.closed = 0;
    ctx.error = 0;
    ctx.done = 0;
    ctx.response_seen = 0;

    os_time_dly(ALI_REALTIME_RECONNECT_DELAY_TICKS);
    if (!ali_realtime_demo_stop_requested) {
        goto reconnect_session;
    }

out:
#if ALI_REALTIME_AUDIO_PLAY_ENABLE
    if (ctx.audio_sink_opened) {
        int remain = ai_audio_sink_drain(ALI_REALTIME_AUDIO_DRAIN_MS);

        printf("[ali_rt] audio sink drain: remain=%d write_errors=%u\n",
               remain, ctx.audio_sink_write_errors);
        ai_audio_sink_close();
        ctx.audio_sink_opened = 0;
    }
#endif
#if ALI_REALTIME_AUDIO_DUMP_ENABLE
    ali_realtime_audio_dump_close(&ctx);
#endif
    ali_realtime_demo_free_large(image_buf);
    ai_realtime_destroy(session);
    ali_realtime_demo_running = 0;
    ali_realtime_demo_started = 0;
}

static int ali_realtime_demo_start_internal(const unsigned char *jpeg_data,
        unsigned int jpeg_len,
        int use_sd_image)
{
    struct ali_realtime_demo_input *input = NULL;
    int ret;

    if (ali_realtime_demo_started) {
        printf("[ali_rt] demo already started\n");
        return AI_ERR_STATE;
    }
    if (ali_realtime_demo_running) {
        printf("[ali_rt] demo already running\n");
        return AI_ERR_STATE;
    }
    if ((!jpeg_data && jpeg_len) || (jpeg_data && !jpeg_len) || (jpeg_data && use_sd_image)) {
        return AI_ERR_PARAM;
    }

    if (jpeg_data || use_sd_image) {
        input = (struct ali_realtime_demo_input *)calloc(1, sizeof(*input));
        if (!input) {
            return AI_ERR_NOMEM;
        }
        input->use_sd_image = use_sd_image;
    }
    if (jpeg_data) {
        input->image_data = (unsigned char *)ali_realtime_demo_malloc_large(jpeg_len);
        if (!input->image_data) {
            free(input);
            return AI_ERR_NOMEM;
        }
        memcpy(input->image_data, jpeg_data, jpeg_len);
        input->image_len = jpeg_len;
    }

    ali_realtime_demo_started = 1;
    ali_realtime_demo_running = 1;
    ali_realtime_demo_stop_requested = 0;
    ret = thread_fork("ali_realtime_demo", 20, 6 * 1024, 0, 0, ali_realtime_demo_task, input);
    if (ret != OS_NO_ERR) {
        ali_realtime_demo_started = 0;
        ali_realtime_demo_running = 0;
        if (input) {
            ali_realtime_demo_free_large(input->image_data);
            free(input);
        }
        printf("[ali_rt] thread_fork failed: %d\n", ret);
        return AI_ERR_NET;
    }

    return AI_OK;
}

void ali_realtime_demo_start(void)
{
    (void)ali_realtime_demo_start_internal(NULL, 0, 0);
}

void ali_realtime_demo_start_sd_image(void)
{
    (void)ali_realtime_demo_start_internal(NULL, 0, 1);
}

int ali_realtime_demo_start_with_image(const unsigned char *jpeg_data,
                                       unsigned int jpeg_len)
{
    if (!jpeg_data || !jpeg_len) {
        return AI_ERR_PARAM;
    }

    return ali_realtime_demo_start_internal(jpeg_data, jpeg_len, 0);
}

void ali_realtime_demo_stop(void)
{
    ali_realtime_demo_stop_requested = 1;
}
