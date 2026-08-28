#include "system/includes.h"
#include "app_config.h"
#include "ai_realtime.h"
#include "ai_audio_url_player.h"

#include <stdlib.h>
#include <string.h>

#define DUER_WS_URL_FORMAT "ws://duer-kids.baidu.com/sandbox/sota/realtime_asr?sn=%s"
#define DUER_MAX_CAPTURE_MS 10000
#define DUER_WAIT_RESPONSE_MS 60000

#ifndef DUER_TEST_CLIENT_AK
#define DUER_TEST_CLIENT_AK ""
#endif
#ifndef DUER_TEST_CLIENT_SK
#define DUER_TEST_CLIENT_SK ""
#endif
#ifndef DUER_TEST_APP_PID
#define DUER_TEST_APP_PID ""
#endif

extern u32 timer_get_ms(void);

static volatile int duer_ai_test_running;
static volatile int duer_ai_test_started;
static volatile int duer_ai_test_playback_stopped;
static unsigned int duer_ai_test_counter;

struct duer_ai_test_ctx {
    volatile int response_seen;
    volatile int closed;
    volatile int error;
};

static void duer_ai_test_make_cuid(char *buf, unsigned int len)
{
    duer_ai_test_counter++;
    snprintf(buf, len, "d%05u", duer_ai_test_counter & 0xfffff);
}

static void duer_ai_test_mark_response(struct duer_ai_test_ctx *ctx)
{
    if (ctx && !ctx->response_seen) {
        ctx->response_seen = 1;
        printf("[duer] first response at %u ms\n", timer_get_ms());
    }
}

static void duer_ai_test_url_player_cb(const char *url,
                                       enum ai_audio_url_event event,
                                       int error,
                                       void *user)
{
    (void)user;

    switch (event) {
    case AI_AUDIO_URL_EVENT_START:
        printf("[duer] audio url play start: %s\n", url ? url : "");
        break;
    case AI_AUDIO_URL_EVENT_END:
        printf("[duer] audio url play end: %s\n", url ? url : "");
        break;
    case AI_AUDIO_URL_EVENT_ERROR:
        printf("[duer] audio url play error %d: %s\n", error, url ? url : "");
        break;
    default:
        break;
    }
}

static void duer_ai_test_wait_url_player(void)
{
    while (ai_audio_url_player_is_running()) {
        os_time_dly(1);
    }
}

static void duer_ai_test_cb(const struct ai_dialogue_event *event, void *user)
{
    struct duer_ai_test_ctx *ctx = (struct duer_ai_test_ctx *)user;

    if (!event || !ctx) {
        return;
    }

    switch (event->type) {
    case AI_DIALOGUE_CONNECTED:
        printf("[duer] realtime connected\n");
        break;
    case AI_DIALOGUE_TEXT_DELTA:
    case AI_DIALOGUE_TEXT_DONE:
    case AI_DIALOGUE_TRANSCRIPT:
        if (event->text) {
            printf("[duer] text: %s\n", event->text);
            if (event->type != AI_DIALOGUE_TEXT_DELTA || event->text != event->raw_json) {
                duer_ai_test_mark_response(ctx);
            }
        }
        if (event->raw_json && event->text && event->text != event->raw_json) {
            printf("[duer] raw: %s\n", event->raw_json);
        }
        break;
    case AI_DIALOGUE_AUDIO_DELTA:
        printf("[duer] audio delta: %u bytes\n", event->data_len);
        duer_ai_test_mark_response(ctx);
        break;
    case AI_DIALOGUE_AUDIO_URL: {
        unsigned int url_count = event->urls && event->url_count ? event->url_count :
                                 (event->url ? 1u : 0u);
        unsigned int i;

        for (i = 0; i < url_count; i++) {
            const char *url = event->urls && event->url_count ? event->urls[i] : event->url;
            int play_ret;

            printf("[duer] audio url[%u/%u]: %s\n", i + 1, url_count, url);
            if (duer_ai_test_playback_stopped) {
                printf("[duer] audio url ignored: playback stopped\n");
                continue;
            }
            play_ret = ai_audio_url_player_enqueue(url);
            if (play_ret != AI_OK) {
                printf("[duer] audio url enqueue failed: %d\n", play_ret);
            }
            if (duer_ai_test_playback_stopped) {
                ai_audio_url_player_stop();
            }
        }
        duer_ai_test_mark_response(ctx);
        break;
    }
    case AI_DIALOGUE_DONE:
        printf("[duer] done\n");
        duer_ai_test_mark_response(ctx);
        break;
    case AI_DIALOGUE_ERROR:
        printf("[duer] error %d at %u ms: %s raw=%s\n",
               event->error_code,
               timer_get_ms(),
               event->text ? event->text : "",
               event->raw_json ? event->raw_json : "");
        ctx->error = 1;
        break;
    case AI_DIALOGUE_CLOSED:
        printf("[duer] closed at %u ms response=%d error=%d\n",
               timer_get_ms(), ctx->response_seen, ctx->error);
        ctx->closed = 1;
        break;
    default:
        break;
    }
}

static void duer_ai_test_task(void *priv)
{
    char cuid[16];
    char ws_url[128];
    char provider_params[512];
    struct ai_audio_source_config audio_cfg = {
        .format = "opus",
        .sample_rate = 16000,
        .channels = 1,
        .volume = 10,
        .channel_bit_map = BIT(1),
        .buffer_size = 4 * 1024,
        .use_vad = 0,
        .vad_start_threshold = 0,
        .vad_stop_threshold = 0,
    };

    struct ai_realtime_config cfg;
    struct ai_audio_url_player_config url_player_cfg;
    struct ai_realtime_upload_stats upload_stats;
    struct ai_realtime_session *session = NULL;
    struct duer_ai_test_ctx ctx = {0};
    int ret;
    int written;

    (void)priv;

    duer_ai_test_make_cuid(cuid, sizeof(cuid));
    written = snprintf(ws_url, sizeof(ws_url), DUER_WS_URL_FORMAT, cuid);
    if (written < 0 || written >= (int)sizeof(ws_url)) {
        printf("[duer] ws url overflow\n");
        goto out;
    }

    written = snprintf(provider_params, sizeof(provider_params),
                       "{"
                       "\"appid\":\"%s\","
                       "\"appkey\":\"%s\","
                       "\"client_id\":\"%s\","
                       "\"client_secret\":\"%s\","
                       "\"pid\":\"%s\","
                       "\"cuid\":\"%s\","
                       "\"format\":\"opus\","
                       "\"sample\":16000,"
                       "\"support_dcs\":1,"
                       "\"support_tts\":1,"
                       "\"support_text2dcs\":1,"
                       "\"access_rc\":1,"
                       "\"rc_version\":\"1\""
                       "}",
                       DUER_TEST_CLIENT_AK,
                       DUER_TEST_CLIENT_SK,
                       DUER_TEST_CLIENT_AK,
                       DUER_TEST_CLIENT_SK,
                       DUER_TEST_APP_PID,
                       cuid);
    if (written < 0 || written >= (int)sizeof(provider_params)) {
        printf("[duer] provider params overflow\n");
        goto out;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.provider_name = "duer";
    cfg.dialogue_ws_url = ws_url;
    cfg.provider_params = provider_params;
    cfg.audio_cfg = &audio_cfg;
    cfg.dialogue_cb = duer_ai_test_cb;
    cfg.dialogue_user = &ctx;
    cfg.timeout_ms = 10000;

    memset(&url_player_cfg, 0, sizeof(url_player_cfg));
    url_player_cfg.volume = 80;
    url_player_cfg.cb = duer_ai_test_url_player_cb;
    ai_audio_url_player_init(&url_player_cfg);

    printf("[duer] realtime demo cuid=%s capture=1 max=%u\n", cuid, DUER_MAX_CAPTURE_MS);

    ret = ai_realtime_open(&cfg, &session);
    if (ret != AI_OK) {
        printf("[duer] realtime open failed: %d\n", ret);
        goto out;
    }

    ret = ai_realtime_start(session);
    if (ret != AI_OK) {
        printf("[duer] realtime start failed: %d\n", ret);
        goto out;
    }

    memset(&upload_stats, 0, sizeof(upload_stats));
    ret = ai_realtime_run_turn(session, DUER_MAX_CAPTURE_MS, DUER_WAIT_RESPONSE_MS, &upload_stats);
    printf("[duer] turn stopped: ret=%d bytes=%u chunks=%u duration=%u upload_end=%d timeout=%d source_end=%d drop=%u/%u\n",
           ret,
           upload_stats.bytes,
           upload_stats.chunks,
           upload_stats.duration_ms,
           upload_stats.stopped_by_upload_end,
           upload_stats.stopped_by_timeout,
           upload_stats.source_end,
           upload_stats.drop_bytes,
           upload_stats.drop_events);

out:
    ai_realtime_destroy(session);
    session = NULL;
    duer_ai_test_wait_url_player();
    ai_audio_url_player_stop();
    duer_ai_test_running = 0;
    duer_ai_test_started = 0;
}

void duer_ai_test_stop_playback(void)
{
    duer_ai_test_playback_stopped = 1;
    ai_audio_url_player_stop();
    printf("[duer] playback stopped and queue cleared\n");
}

void duer_ai_test_start(void)
{
    int ret;

    if (duer_ai_test_started) {
        printf("[duer] test already started\n");
        return;
    }
    if (duer_ai_test_running) {
        printf("[duer] test already running\n");
        return;
    }

    duer_ai_test_playback_stopped = 0;
    duer_ai_test_started = 1;
    duer_ai_test_running = 1;
    ret = thread_fork("duer_ai_test", 20, 4 * 1024, 0, 0, duer_ai_test_task, NULL);
    if (ret != OS_NO_ERR) {
        duer_ai_test_started = 0;
        duer_ai_test_running = 0;
        printf("[duer] thread_fork failed: %d\n", ret);
    }
}
