#include "system/includes.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#include "ai_realtime.h"
#include "ai_audio_source.h"
#include "ai_internal.h"

#include <stdlib.h>
#include <string.h>

#define AI_REALTIME_DEFAULT_TIMEOUT_MS 10000

extern u32 timer_get_ms(void);
extern void msleep(unsigned int ms);

#ifndef AI_REALTIME_VOICE_CAPTURE_ENABLE
#define AI_REALTIME_VOICE_CAPTURE_ENABLE 1
#endif

/* realtime层负责语音长连接和单次实时会话，供应用自行驱动多轮交互。 */
struct ai_realtime_session {
    struct ai_client *client;
    struct ai_realtime_config cfg;
    const struct ai_provider_ops *provider;
    const struct ai_provider_voice_profile *voice;
    int connected;
    int ready;
    /* closed/error/upload_end/turn_done/session_end are written by the poll
     * thread inside ai_realtime_cb() and read by the background upload thread's
     * loop; volatile so the upload thread observes updates instead of caching
     * them in a register. */
    volatile int closed;
    volatile int error;
    int started;
    volatile int reconnecting;
    int reconnect_restore_pending;
    int reconnect_restore_waiting;
    int reconnect_resume_upload;
    unsigned int reconnect_restore_start_ms;
    volatile int upload_end;
    volatile int turn_done;
    volatile int session_end;
    /* audio_cfg 非 NULL 时 realtime 仅在 upload 期间接管录音源。 */
    int audio_owned;
    volatile int upload_running;
    volatile int upload_stop;
    unsigned int upload_max_capture_ms;
    struct ai_realtime_upload_stats upload_stats;
};

static int ai_realtime_audio_start(struct ai_realtime_session *session)
{
#if AI_REALTIME_VOICE_CAPTURE_ENABLE
    int ret;

    if (!session || !session->cfg.audio_cfg || session->audio_owned) {
        return AI_OK;
    }

    ret = ai_audio_source_start(session->cfg.audio_cfg);
    if (ret != 0) {
        printf("[ai_realtime] audio source start failed: %d\n", ret);
        return AI_ERR_NET;
    }

    session->audio_owned = 1;
    printf("[ai_realtime] audio source started for upload\n");
#else
    (void)session;
#endif
    return AI_OK;
}

static void ai_realtime_audio_stop(struct ai_realtime_session *session)
{
#if AI_REALTIME_VOICE_CAPTURE_ENABLE
    int ret;

    if (!session || !session->audio_owned) {
        return;
    }

    ret = ai_audio_source_stop();
    session->audio_owned = 0;
    printf("[ai_realtime] audio source stopped after upload: %d\n", ret);
#else
    (void)session;
#endif
}

static int ai_realtime_timeout_ms(const struct ai_realtime_config *cfg)
{
    if (cfg && cfg->timeout_ms > 0) {
        return cfg->timeout_ms;
    }

    return AI_REALTIME_DEFAULT_TIMEOUT_MS;
}

static int ai_realtime_provider_supports(const struct ai_provider_ops *provider, unsigned int cap)
{
    return provider && ((provider->caps & cap) != 0);
}

static unsigned int ai_realtime_chunk_size(const struct ai_provider_voice_profile *voice)
{
    return voice && voice->audio_chunk_size ? voice->audio_chunk_size : 80u;
}

static unsigned int ai_realtime_frame_interval_ms(const struct ai_provider_voice_profile *voice)
{
    return voice && voice->frame_interval_ms ? voice->frame_interval_ms : AI_VOICE_DEFAULT_FRAME_MS;
}

static unsigned int ai_realtime_pending_low_watermark(const struct ai_provider_voice_profile *voice)
{
    return voice && voice->pending_low_watermark ? voice->pending_low_watermark : AI_VOICE_DEFAULT_PENDING_LOW;
}

static unsigned int ai_realtime_upload_log_interval(const struct ai_provider_voice_profile *voice)
{
    return voice && voice->upload_log_interval ? voice->upload_log_interval : AI_VOICE_DEFAULT_UPLOAD_LOG;
}

static int ai_realtime_is_single_turn(const struct ai_provider_voice_profile *voice)
{
    return voice && voice->flow_mode == AI_VOICE_FLOW_SINGLE_TURN;
}

static void ai_realtime_store_upload_stats(struct ai_realtime_upload_stats *stats,
        unsigned int bytes,
        unsigned int chunks,
        unsigned int duration_ms,
        int stopped_by_session_end,
        int stopped_by_timeout,
        int source_end)
{
    struct ai_audio_source_drop_stats drop;

    if (!stats) {
        return;
    }

    stats->bytes = bytes;
    stats->chunks = chunks;
    stats->duration_ms = duration_ms;
    if (stopped_by_session_end) {
        stats->stopped_by_session_end = 1;
    }
    if (stopped_by_timeout) {
        stats->stopped_by_timeout = 1;
    }
    if (source_end) {
        stats->source_end = 1;
    }
    if (ai_audio_source_get_drop_stats(&drop) == 0) {
        stats->drop_bytes = drop.bytes;
        stats->drop_frames = drop.frames;
        stats->drop_events = drop.events;
    }
}

static void ai_realtime_cb(const struct ai_dialogue_event *event, void *user)
{
    struct ai_realtime_session *session = (struct ai_realtime_session *)user;
    int retrying;

    if (!event || !session) {
        return;
    }
    retrying = session->started && session->cfg.auto_reconnect && session->client &&
               session->client->reconnect_need;

    if (session->provider && session->provider->is_ready_event && session->provider->is_ready_event(event)) {
        session->ready = 1;
    }
    if (session->provider && session->provider->is_upload_end_event &&
        session->provider->is_upload_end_event(event)) {
        session->upload_end = 1;
    }
    if (session->provider && session->provider->is_session_end_event &&
        session->provider->is_session_end_event(event)) {
        session->session_end = 1;
    }

    if (session->provider && session->provider->is_turn_done_event) {
        if (session->provider->is_turn_done_event(event)) {
            session->turn_done = 1;
        }
    } else if (event->type == AI_DIALOGUE_DONE) {
        session->turn_done = 1;
    }

    switch (event->type) {
    case AI_DIALOGUE_CONNECTED:
        session->connected = 1;
        if (session->started && session->reconnecting) {
            session->ready = 0;
            session->reconnect_restore_pending = 1;
            session->reconnect_restore_waiting = 0;
            printf("[ai_realtime] transport reconnected, restoring provider session\n");
        }
        break;
    case AI_DIALOGUE_ERROR:
        if (retrying) {
            session->connected = 0;
            session->ready = 0;
            session->reconnecting = 1;
            session->reconnect_restore_pending = 0;
            session->reconnect_restore_waiting = 0;
            if (session->upload_running) {
                session->reconnect_resume_upload = 1;
            }
            printf("[ai_realtime] transient transport error, waiting reconnect\n");
            return;
        }
        session->connected = 0;
        session->reconnecting = 0;
        session->reconnect_restore_pending = 0;
        session->reconnect_restore_waiting = 0;
        session->reconnect_resume_upload = 0;
        session->error = 1;
        break;
    case AI_DIALOGUE_CLOSED:
        if (retrying) {
            session->connected = 0;
            session->ready = 0;
            session->reconnecting = 1;
            session->reconnect_restore_pending = 0;
            session->reconnect_restore_waiting = 0;
            if (session->upload_running) {
                session->reconnect_resume_upload = 1;
            }
            printf("[ai_realtime] transport closed, waiting reconnect\n");
            return;
        }
        session->connected = 0;
        session->reconnecting = 0;
        session->reconnect_restore_pending = 0;
        session->reconnect_restore_waiting = 0;
        session->reconnect_resume_upload = 0;
        session->closed = 1;
        break;
    default:
        break;
    }

    if (session->cfg.dialogue_cb) {
        session->cfg.dialogue_cb(event, session->cfg.dialogue_user);
    }
}

int ai_realtime_open(const struct ai_realtime_config *cfg,
                     struct ai_realtime_session **out)
{
    struct ai_realtime_session *session;
    const struct ai_provider_ops *provider;
    const char *provider_name;

    if (!cfg || !cfg->provider_name || !cfg->provider_name[0] || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    provider_name = cfg->provider_name;
    provider = ai_provider_find(provider_name);
    if (!ai_realtime_provider_supports(provider, AI_PROVIDER_CAP_VOICE_DIALOGUE) || !provider->voice) {
        return AI_ERR_UNSUPPORTED;
    }

    session = (struct ai_realtime_session *)calloc(1, sizeof(*session));
    if (!session) {
        return AI_ERR_NOMEM;
    }

    session->cfg = *cfg;
    session->provider = provider;
    session->voice = provider->voice;
    *out = session;
    return AI_OK;
}

/* 启动长连接 realtime 会话；只有 provider 声明 ready 事件时才在返回前等待 ready gate。 */
int ai_realtime_start(struct ai_realtime_session *session)
{
    struct ai_config ai_cfg;
    int ret;
    int i;

    if (!session) {
        return AI_ERR_PARAM;
    }
    if (session->client) {
        return AI_ERR_STATE;
    }

    session->connected = 0;
    session->ready = 0;
    session->closed = 0;
    session->error = 0;
    session->started = 0;
    session->upload_end = 0;
    session->turn_done = 0;
    session->session_end = 0;
    session->reconnecting = 0;
    session->reconnect_restore_pending = 0;
    session->reconnect_restore_waiting = 0;
    session->reconnect_resume_upload = 0;
    session->reconnect_restore_start_ms = 0;
    session->audio_owned = 0;

    memset(&ai_cfg, 0, sizeof(ai_cfg));
    ai_cfg.provider_name = session->cfg.provider_name;
    ai_cfg.dialogue_ws_url = session->cfg.dialogue_ws_url ? session->cfg.dialogue_ws_url : session->provider->default_dialogue_ws_url;
    ai_cfg.api_key = session->cfg.api_key;
    ai_cfg.model = session->cfg.model ? session->cfg.model : session->provider->default_model;
    ai_cfg.provider_params = session->cfg.provider_params ? session->cfg.provider_params : session->provider->default_provider_params;
    ai_cfg.turn_detection = session->cfg.turn_detection;
    ai_cfg.timeout_ms = ai_realtime_timeout_ms(&session->cfg);
    ai_cfg.ping_interval_ms = session->cfg.ping_interval_ms;
    ai_cfg.auto_reconnect = session->cfg.auto_reconnect;
    ai_cfg.reconnect_max_attempts = session->cfg.reconnect_max_attempts;
    ret = ai_client_create(&ai_cfg, &session->client);
    if (ret != AI_OK) {
        return ret;
    }

    ret = ai_dialogue_connect(session->client, ai_realtime_cb, session);
    if (ret != AI_OK) {
        ai_client_destroy(session->client);
        session->client = NULL;
        return ret;
    }

    for (i = 0; i < session->voice->connect_polls && !session->connected && !session->closed && !session->error; i++) {
        ai_dialogue_poll(session->client, session->voice->poll_timeout_ms);
        os_time_dly(1);
    }
    if (!session->connected) {
        if (!session->error) {
            ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_NET, 0,
                                   "realtime connect timeout", NULL);
        }
        ai_client_destroy(session->client);
        session->client = NULL;
        return AI_ERR_NET;
    }

    if (session->provider->build_session_start) {
        ret = ai_dialogue_session_start(session->client);
        if (ret != AI_OK) {
            ai_client_destroy(session->client);
            session->client = NULL;
            return ret;
        }
        for (i = 0; i < session->voice->send_retry_polls && ai_dialogue_pending(session->client) > 0 && !session->closed && !session->error; i++) {
            ai_dialogue_poll(session->client, 0);
            os_time_dly(1);
        }
        if (ai_dialogue_pending(session->client) > 0) {
            ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_STATE, 0,
                                   "realtime session-start send timeout", NULL);
            ai_client_destroy(session->client);
            session->client = NULL;
            return AI_ERR_STATE;
        }
    }

    if (session->provider->is_ready_event && session->voice->ready_polls > 0) {
        for (i = 0; i < session->voice->ready_polls && !session->ready && !session->closed && !session->error; i++) {
            ai_dialogue_poll(session->client, session->voice->poll_timeout_ms);
            os_time_dly(1);
        }
        if (!session->ready) {
#if AI_REALTIME_VOICE_CAPTURE_ENABLE
            if (!session->error) {
                ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_NET, 0,
                                       "realtime ready timeout", NULL);
            }
            ai_client_destroy(session->client);
            session->client = NULL;
            return AI_ERR_NET;
#else
            printf("[ai_realtime] ready timeout, continue network-only flow\n");
#endif
        }
    }

    session->started = 1;
    return AI_OK;
}

int ai_realtime_send_text(struct ai_realtime_session *session, const char *text)
{
    if (!session || !session->client || !text) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    return ai_dialogue_send_text(session->client, text);
}

int ai_realtime_send_audio(struct ai_realtime_session *session,
                           const unsigned char *data,
                           unsigned int len)
{
    struct ai_audio_input audio;

    if (!session || !session->client || !data || !len) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    memset(&audio, 0, sizeof(audio));
    audio.format = session->voice->audio_format;
    audio.data = data;
    audio.size = len;
    audio.sample_rate = session->voice->sample_rate;
    audio.channels = session->voice->channels;
    audio.bits_per_sample = session->voice->bits_per_sample;

    return ai_dialogue_send_audio(session->client, &audio);
}

int ai_realtime_send_image(struct ai_realtime_session *session,
                           const unsigned char *data,
                           unsigned int len)
{
    struct ai_image_input image;

    if (!session || !session->client || !data || !len) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    memset(&image, 0, sizeof(image));
    image.format = AI_IMAGE_JPEG;
    image.data = data;
    image.size = len;

    return ai_dialogue_send_image(session->client, &image);
}

static int ai_realtime_upload_audio_source_internal(struct ai_realtime_session *session,
        unsigned int max_capture_ms,
        struct ai_realtime_upload_stats *stats,
        int drive_network)
{
    unsigned char *buf;
    unsigned int chunk_size;
    unsigned int low_watermark;
    unsigned int frame_interval_ms;
    unsigned int catch_up_threshold;
    unsigned int log_interval;
    unsigned int start_ms;
    unsigned int total = 0;
    unsigned int chunks = 0;
    int stopped_by_timeout = 0;
    int stopped_by_source_end = 0;
    int stopped_by_upload_end = 0;
    int catching_up = 0;
    int ret = AI_OK;

    if (!session || !session->client || !session->voice) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    if (stats) {
        memset(stats, 0, sizeof(*stats));
    }

    chunk_size = ai_realtime_chunk_size(session->voice);
    buf = (unsigned char *)ai_malloc_large(chunk_size);
    if (!buf) {
        return AI_ERR_NOMEM;
    }

    low_watermark = ai_realtime_pending_low_watermark(session->voice);
    frame_interval_ms = ai_realtime_frame_interval_ms(session->voice);
    catch_up_threshold = chunk_size * 2;
    log_interval = ai_realtime_upload_log_interval(session->voice);
    session->upload_end = 0;
    start_ms = timer_get_ms();

    printf("[ai_realtime] upload start: chunk=%u interval=%u low=%u single=%d max=%u buffered=%d\n",
           chunk_size,
           frame_interval_ms,
           low_watermark,
           ai_realtime_is_single_turn(session->voice),
           max_capture_ms,
           ai_audio_source_data_len());

    while (!session->closed && !session->error && !session->upload_stop) {
        int data_len;
        unsigned int pace_ms;

        if (session->reconnecting) {
            ret = AI_ERR_STATE;
            break;
        }

        if (ai_realtime_is_single_turn(session->voice) && session->upload_end) {
            if (stats) {
                stats->stopped_by_upload_end = 1;
            }
            stopped_by_upload_end = 1;
            ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                           0, 0, ai_audio_source_is_end());
            break;
        }

        if (ai_realtime_is_single_turn(session->voice) && session->session_end) {
            if (stats) {
                stats->stopped_by_session_end = 1;
            }
            ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                           1, 0, ai_audio_source_is_end());
            break;
        }

        /* Hard wall-clock cap on capture. This must NOT be gated on chunks > 0:
         * if the mic stalls and never produces a frame (driver wedged, no
         * source_end/error), a chunks-gated timeout never fires and run_turn's
         * caller spins forever. Bounding from start_ms guarantees the turn ends. */
        if (max_capture_ms > 0 && (timer_get_ms() - start_ms) >= max_capture_ms) {
            if (stats) {
                stats->stopped_by_timeout = 1;
            }
            stopped_by_timeout = 1;
            ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                           0, 1, ai_audio_source_is_end());
            break;
        }

        data_len = ai_audio_source_data_len();
        if (!catching_up && data_len > (int)catch_up_threshold) {
            catching_up = 1;
            printf("[ai_realtime] upload catch-up start: buffered=%d threshold=%u\n",
                   data_len, catch_up_threshold);
        } else if (catching_up && data_len <= (int)chunk_size) {
            catching_up = 0;
            printf("[ai_realtime] upload catch-up done: buffered=%d\n", data_len);
        }
        pace_ms = frame_interval_ms;
        if (catching_up && pace_ms > 1) {
            pace_ms /= 2;
        }
        if (data_len < (int)chunk_size || ai_dialogue_pending(session->client) >= (int)low_watermark) {
            if (ai_audio_source_is_error()) {
                session->error = 1;
                ret = AI_ERR_NET;
                break;
            }
            if (ai_audio_source_is_end() && data_len < (int)chunk_size) {
                if (stats) {
                    stats->source_end = 1;
                }
                stopped_by_source_end = 1;
                ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                               0, 0, 1);
                break;
            }
            if (drive_network) {
                ai_dialogue_poll_now(session->client);
            }
            if (session->closed || session->error) {
                break;
            }
            if (pace_ms) {
                msleep(pace_ms);
            } else {
                os_time_dly(1);
            }
            continue;
        }

        if (ai_audio_source_read(buf, chunk_size) != (int)chunk_size) {
            continue;
        }

        ret = ai_realtime_send_audio(session, buf, chunk_size);
        if (ret != AI_OK) {
            break;
        }
        if (drive_network) {
            ai_dialogue_poll_now(session->client);
        }
        if (session->closed || session->error) {
            break;
        }

        total += chunk_size;
        chunks++;
        ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                       0, 0, ai_audio_source_is_end());
        if (log_interval && (chunks % log_interval) == 0) {
            printf("[ai_realtime] upload progress: chunks=%u bytes=%u elapsed=%u ms data_len=%d pending=%d\n",
                   chunks,
                   total,
                   timer_get_ms() - start_ms,
                   ai_audio_source_data_len(),
                   ai_dialogue_pending(session->client));
        }

        if (pace_ms) {
            msleep(pace_ms);
        }
    }

    if (ret == AI_OK && chunks > 0 && ai_realtime_is_single_turn(session->voice) &&
        !session->session_end && !session->upload_stop && !session->closed && !session->error &&
        (stopped_by_upload_end || stopped_by_timeout || stopped_by_source_end)) {
        int i;
        int commit_ret;

        commit_ret = ai_realtime_commit(session);
        if (commit_ret == AI_OK) {
            printf("[ai_realtime] single-turn upload finished, commit queued\n");
            if (drive_network) {
                for (i = 0; i < session->voice->send_retry_polls &&
                     ai_dialogue_pending(session->client) > 0 &&
                     !session->closed && !session->error; i++) {
                    ai_dialogue_poll_now(session->client);
                    msleep(frame_interval_ms ? frame_interval_ms : AI_VOICE_DEFAULT_FRAME_MS);
                }
            }
            if (drive_network && ai_dialogue_pending(session->client) > 0) {
                ret = AI_ERR_STATE;
            }
        } else {
            ret = commit_ret;
        }
    }

    ai_realtime_store_upload_stats(stats, total, chunks, timer_get_ms() - start_ms,
                                   0, stopped_by_timeout, stopped_by_source_end || ai_audio_source_is_end());
    if (stats && stopped_by_upload_end) {
        stats->stopped_by_upload_end = 1;
    }

    printf("[ai_realtime] upload stopped: bytes=%u chunks=%u duration=%u closed=%d error=%d upload_end=%d session_end=%d source_end=%d drop=%u/%u pending=%d ret=%d\n",
           total,
           chunks,
           timer_get_ms() - start_ms,
           session->closed,
           session->error,
           session->upload_end,
           session->session_end,
           ai_audio_source_is_end(),
           stats ? stats->drop_bytes : 0,
           stats ? stats->drop_events : 0,
           ai_dialogue_pending(session->client),
           ret);

    if (!chunks && ret == AI_OK) {
        ret = AI_ERR_NET;
    }
    if (session->error) {
        ret = AI_ERR_NET;
    }

    ai_free_large(buf);
    return ret;
}

int ai_realtime_upload_audio_source(struct ai_realtime_session *session,
                                    unsigned int max_capture_ms,
                                    struct ai_realtime_upload_stats *stats)
{
    int ret;

    if (!session) {
        return AI_ERR_PARAM;
    }

    ret = ai_realtime_audio_start(session);
    if (ret != AI_OK) {
        return ret;
    }

    ret = ai_realtime_upload_audio_source_internal(session, max_capture_ms, stats, 1);
    ai_realtime_audio_stop(session);
    return ret;
}

static void ai_realtime_upload_task(void *priv)
{
    struct ai_realtime_session *session = (struct ai_realtime_session *)priv;

    if (!session) {
        return;
    }

    memset(&session->upload_stats, 0, sizeof(session->upload_stats));
    ai_realtime_upload_audio_source_internal(session,
            session->upload_max_capture_ms,
            &session->upload_stats,
            0);
    ai_realtime_audio_stop(session);
    session->upload_running = 0;
}

int ai_realtime_start_upload(struct ai_realtime_session *session,
                             unsigned int max_capture_ms)
{
    int ret;

    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }
    if (session->upload_running) {
        return AI_ERR_STATE;
    }

    ret = ai_realtime_audio_start(session);
    if (ret != AI_OK) {
        return ret;
    }

    session->reconnect_resume_upload = 0;
    session->upload_stop = 0;
    session->upload_end = 0;
    session->turn_done = 0;
    session->upload_running = 1;
    session->upload_max_capture_ms = max_capture_ms;
    ret = thread_fork("ai_rt_upload", 20, 4 * 1024, 0, 0, ai_realtime_upload_task, session);
    if (ret != OS_NO_ERR) {
        session->upload_running = 0;
        ai_realtime_audio_stop(session);
        return AI_ERR_NET;
    }

    return AI_OK;
}

int ai_realtime_stop_upload(struct ai_realtime_session *session)
{
    unsigned int start_ms;

    if (!session) {
        return AI_ERR_PARAM;
    }

    session->reconnect_resume_upload = 0;
    session->upload_stop = 1;
    start_ms = timer_get_ms();
    while (session->upload_running) {
        if ((timer_get_ms() - start_ms) > 3000) {
            return AI_ERR_STATE;
        }
        os_time_dly(1);
    }

    return AI_OK;
}

int ai_realtime_upload_running(struct ai_realtime_session *session)
{
    if (!session) {
        return 0;
    }

    return session->upload_running ? 1 : 0;
}

int ai_realtime_get_upload_stats(struct ai_realtime_session *session,
                                 struct ai_realtime_upload_stats *out)
{
    if (!session || !out) {
        return AI_ERR_PARAM;
    }

    /* Best-effort snapshot: the upload thread updates these counters field by
     * field, so a concurrent read may mix old/new values. They are advisory
     * (progress logging / drop stats), never used for control flow, so we avoid
     * the per-chunk locking cost. */
    *out = session->upload_stats;
    return AI_OK;
}

static int ai_realtime_wait_turn_done(struct ai_realtime_session *session,
                                      unsigned int wait_response_ms)
{
    unsigned int start_ms;
    int poll_timeout;
    int ret;

    if (!session || !session->client || !session->voice) {
        return AI_ERR_PARAM;
    }

    if (!wait_response_ms) {
        poll_timeout = session->voice->poll_timeout_ms > 0 ? session->voice->poll_timeout_ms : (int)AI_VOICE_DEFAULT_FRAME_MS;
        wait_response_ms = session->voice->response_polls * (unsigned int)poll_timeout;
        /* Defense in depth: ai_provider_validate already forces response_polls > 0,
         * but if a provider is driven without going through validate this keeps the
         * wait bounded instead of looping forever (the timeout check below is gated
         * on wait_response_ms being non-zero). */
        if (!wait_response_ms) {
            wait_response_ms = AI_REALTIME_DEFAULT_TIMEOUT_MS;
        }
    }
    poll_timeout = session->voice->poll_timeout_ms > 0 ? session->voice->poll_timeout_ms : 20;
    start_ms = timer_get_ms();

    while (!session->turn_done && !session->session_end && !session->closed && !session->error) {
        if (wait_response_ms && (timer_get_ms() - start_ms) >= wait_response_ms) {
            ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_NET, 0,
                                   "realtime response timeout", NULL);
            return AI_ERR_NET;
        }
        ret = ai_dialogue_poll(session->client, poll_timeout);
        if (ret != AI_OK) {
            return ret;
        }
        if (poll_timeout <= 0) {
            os_time_dly(1);
        }
    }

    return (session->closed || session->error) ? AI_ERR_NET : AI_OK;
}

int ai_realtime_run_turn(struct ai_realtime_session *session,
                         unsigned int max_capture_ms,
                         unsigned int wait_response_ms,
                         struct ai_realtime_upload_stats *stats)
{
    int ret;
    int manual_turn;

    if (!session) {
        return AI_ERR_PARAM;
    }

    session->upload_end = 0;
    session->turn_done = 0;
    manual_turn = !ai_realtime_is_single_turn(session->voice);
    ret = ai_realtime_upload_audio_source(session, max_capture_ms, stats);

    if (ret != AI_OK && !session->turn_done && !session->session_end) {
        return ret;
    }

    if (manual_turn && session->provider->build_commit) {
        ret = ai_realtime_commit(session);
        if (ret != AI_OK) {
            return ret;
        }
    }

    if (manual_turn && session->provider->build_response_create) {
        ret = ai_realtime_response_create(session);
        if (ret != AI_OK) {
            return ret;
        }
    }

    return ai_realtime_wait_turn_done(session, wait_response_ms);
}

int ai_realtime_commit(struct ai_realtime_session *session)
{
    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    return ai_dialogue_commit(session->client);
}

int ai_realtime_interrupt(struct ai_realtime_session *session)
{
    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    return ai_dialogue_interrupt(session->client);
}

int ai_realtime_response_create(struct ai_realtime_session *session)
{
    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    session->turn_done = 0;
    return ai_dialogue_response_create(session->client);
}

int ai_realtime_wait_response(struct ai_realtime_session *session,
                              unsigned int wait_response_ms)
{
    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }
    if (!session->started || session->closed || session->error) {
        return AI_ERR_STATE;
    }

    return ai_realtime_wait_turn_done(session, wait_response_ms);
}

static int ai_realtime_restore_after_reconnect(struct ai_realtime_session *session)
{
    int ret;
    unsigned int timeout_ms;

    if (!session->reconnecting) {
        return AI_OK;
    }

    if (session->reconnect_restore_pending) {
        session->reconnect_restore_pending = 0;
        session->upload_end = 0;
        session->turn_done = 0;
        session->session_end = 0;
        session->reconnect_restore_start_ms = (unsigned int)timer_get_ms();
        if (session->provider->build_session_start) {
            ret = ai_dialogue_session_start(session->client);
            if (ret != AI_OK) {
                ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_SEND, ret, 0,
                                       "reconnect session-start failed", NULL);
                return ret;
            }
        }
        session->reconnect_restore_waiting = 1;
    }

    if (!session->reconnect_restore_waiting) {
        return AI_OK;
    }
    timeout_ms = (unsigned int)ai_realtime_timeout_ms(&session->cfg);
    if (session->provider->is_ready_event) {
        if (!session->ready) {
            if ((unsigned int)timer_get_ms() - session->reconnect_restore_start_ms < timeout_ms) {
                return AI_OK;
            }
            ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_NET, 0,
                                   "reconnect provider ready timeout", NULL);
            return AI_ERR_NET;
        }
    } else if (ai_dialogue_pending(session->client) > 0) {
        if ((unsigned int)timer_get_ms() - session->reconnect_restore_start_ms < timeout_ms) {
            return AI_OK;
        }
        ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_TIMEOUT, AI_ERR_NET, 0,
                               "reconnect session-start send timeout", NULL);
        return AI_ERR_NET;
    }

    session->reconnect_restore_waiting = 0;
    session->reconnecting = 0;
    session->closed = 0;
    session->error = 0;
    printf("[ai_realtime] provider session restored\n");
    return AI_OK;
}

static int ai_realtime_resume_upload_after_reconnect(struct ai_realtime_session *session)
{
    int ret;

    if (session->reconnecting || !session->reconnect_resume_upload || session->upload_running) {
        return AI_OK;
    }

    session->reconnect_resume_upload = 0;
    ret = ai_realtime_start_upload(session, session->upload_max_capture_ms);
    if (ret != AI_OK) {
        ai_dialogue_emit_error(session->client, AI_ERROR_STAGE_SEND, ret, 0,
                               "reconnect audio upload restart failed", NULL);
    }
    return ret;
}

int ai_realtime_poll(struct ai_realtime_session *session, int timeout_ms)
{
    int ret;

    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }

    ret = ai_dialogue_poll(session->client, timeout_ms);
    if (ret != AI_OK && !session->reconnecting) {
        return ret;
    }
    ret = ai_realtime_restore_after_reconnect(session);
    if (ret != AI_OK) {
        return ret;
    }
    return ai_realtime_resume_upload_after_reconnect(session);
}

int ai_realtime_poll_now(struct ai_realtime_session *session)
{
    return ai_realtime_poll(session, 0);
}

int ai_realtime_pending(struct ai_realtime_session *session)
{
    if (!session || !session->client) {
        return 0;
    }

    return ai_dialogue_pending(session->client);
}

int ai_realtime_close(struct ai_realtime_session *session)
{
    int ret;

    if (!session || !session->client) {
        return AI_ERR_PARAM;
    }

    ret = ai_realtime_stop_upload(session);
    if (ret != AI_OK) {
        printf("[ai_realtime] close failed: upload task still running\n");
        return ret;
    }

    ret = ai_dialogue_close(session->client);
    session->reconnecting = 0;
    session->reconnect_resume_upload = 0;
    ai_client_destroy(session->client);
    session->client = NULL;
    session->connected = 0;
    session->ready = 0;
    session->closed = 1;
    session->error = 0;
    session->started = 0;
    ai_realtime_audio_stop(session);
    return ret;
}

void ai_realtime_destroy(struct ai_realtime_session *session)
{
    if (!session) {
        return;
    }

    if (ai_realtime_stop_upload(session) != AI_OK) {
        printf("[ai_realtime] destroy aborted: upload task still running\n");
        return;
    }

    if (session->client) {
        ai_dialogue_close(session->client);
        ai_client_destroy(session->client);
        session->client = NULL;
    }
    ai_realtime_audio_stop(session);
    free(session);
}

#endif
