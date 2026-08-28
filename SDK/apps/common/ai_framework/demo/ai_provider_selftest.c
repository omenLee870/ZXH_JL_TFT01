#include "ai_internal.h"

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const struct ai_provider_ops ai_provider_duer;
extern const struct ai_provider_ops ai_provider_ali_realtime;
extern const struct ai_provider_ops ai_provider_glm;
extern const struct ai_provider_ops ai_provider_volc_ark;

/*
 * Offline provider contract + parser regression check. It opens no sockets and
 * no audio devices, so it is safe to call once at startup (e.g. from a factory
 * self-test hook) to catch a broken provider before the field does.
 *
 * Adding a provider: add its ops to selftest_providers[], then add at least one
 * success WS/HTTP fixture and one vendor-error fixture to the tables below
 * (per AI_PROVIDER_PORTING.md). The tables are data, so no new function is needed.
 */

/* Wire fixture -> parse_ws_message -> expected unified event. */
struct ai_selftest_ws_case {
    const struct ai_provider_ops *provider;
    const char *label;
    const char *data;                     /* wire bytes (text JSON, or binary payload) */
    unsigned int len;                     /* 0 => strlen(data) */
    int binary;
    enum ai_dialogue_event_type expected_type;
    const char *expected_text;            /* NULL => do not check text */
    unsigned int expected_url_count;      /* 0 => do not check audio URLs */
    const char *expected_urls[2];
};

/* Response body -> parse_vision_http_response -> expected result. */
struct ai_selftest_http_case {
    const struct ai_provider_ops *provider;
    const char *label;
    const char *json;
    int expect_ok;                        /* 1 => expect AI_OK, 0 => expect vendor error */
    const char *expected_text;            /* NULL => do not check text */
};

static const struct ai_provider_ops *const selftest_providers[] = {
    &ai_provider_duer,
    &ai_provider_ali_realtime,
    &ai_provider_glm,
    &ai_provider_volc_ark,
};

static const struct ai_selftest_ws_case selftest_ws_cases[] = {
    /* duer: ready gate, transcript-ish text, a vendor error, and a binary TTS frame. */
    {
        &ai_provider_duer, "duer ready",
        "{\"type\":\"ready\",\"status\":\"ok\"}", 0, 0,
        AI_DIALOGUE_TEXT_DELTA, NULL
    },
    {
        &ai_provider_duer, "duer error",
        "{\"status\":\"failed\"}", 0, 0,
        AI_DIALOGUE_ERROR, "failed"
    },
    {
        &ai_provider_duer, "duer binary audio",
        "\x01\x02\x03\x04", 4, 1,
        AI_DIALOGUE_AUDIO_DELTA, NULL
    },
    {
        &ai_provider_duer, "duer multiple audio urls",
        "{\"type\":\"inside_rc\",\"status\":\"ok\",\"data\":{\"code\":0,\"data\":["
        "{\"header\":{\"namespace\":\"ai.dueros.device_interface.voice_output\","
        "\"name\":\"Speak\"},\"payload\":{\"url\":\"http://tts\"}},"
        "{\"header\":{\"namespace\":\"ai.dueros.device_interface.audio_player\","
        "\"name\":\"Play\"},\"payload\":{\"audioItem\":{\"stream\":{\"url\":\"http://music\"}}}},"
        "{\"header\":{\"namespace\":\"ai.dueros.device_interface.screen\","
        "\"name\":\"RenderPlayerInfo\"},\"payload\":{\"url\":\"http://art\"}}"
        "]}}",
        0, 0, AI_DIALOGUE_AUDIO_URL, NULL,
        2, { "http://tts", "http://music" }
    },

    /* ali realtime: VAD edges, final text, binary audio, and a vendor error. */
    {
        &ai_provider_ali_realtime, "ali speech_started",
        "{\"type\":\"input_audio_buffer.speech_started\"}", 0, 0,
        AI_DIALOGUE_SPEECH_STARTED, NULL
    },
    {
        &ai_provider_ali_realtime, "ali speech_stopped",
        "{\"type\":\"input_audio_buffer.speech_stopped\"}", 0, 0,
        AI_DIALOGUE_SPEECH_STOPPED, NULL
    },
    {
        &ai_provider_ali_realtime, "ali text.done",
        "{\"type\":\"response.text.done\",\"text\":\"ok\"}", 0, 0,
        AI_DIALOGUE_TEXT_DONE, "ok"
    },
    {
        &ai_provider_ali_realtime, "ali binary audio",
        "\x11\x22\x33", 3, 1,
        AI_DIALOGUE_AUDIO_DELTA, NULL
    },
    {
        &ai_provider_ali_realtime, "ali error",
        "{\"type\":\"error\",\"error\":{\"message\":\"bad key\"}}", 0, 0,
        AI_DIALOGUE_ERROR, "bad key"
    },
};

static const struct ai_selftest_http_case selftest_http_cases[] = {
    {
        &ai_provider_glm, "glm ok",
        "{\"choices\":[{\"message\":{\"content\":\"ok\"}}]}", 1, "ok"
    },
    {
        &ai_provider_glm, "glm error",
        "{\"error\":{\"message\":\"invalid api key\"}}", 0, "invalid api key"
    },
    {
        &ai_provider_volc_ark, "volc ok",
        "{\"choices\":[{\"message\":{\"content\":\"ok\"}}]}", 1, "ok"
    },
    {
        &ai_provider_volc_ark, "volc error",
        "{\"error\":{\"message\":\"invalid api key\"}}", 0, "invalid api key"
    },
};

static int ai_selftest_ws_run(const struct ai_selftest_ws_case *c)
{
    struct ai_client client;
    struct ai_dialogue_event event;
    unsigned char *owned = NULL;
    unsigned int len = c->len ? c->len : (unsigned int)strlen(c->data);
    int ok;
    int ret;
    unsigned int i;

    memset(&client, 0, sizeof(client));
    memset(&event, 0, sizeof(event));
    client.provider = c->provider;
    client.provider_name = (char *)c->provider->name;

    ret = c->provider->parse_ws_message(&client, (const unsigned char *)c->data,
                                        len, c->binary, &event, &owned);
    ok = (ret == AI_OK) && (event.type == c->expected_type) &&
         (!c->expected_text || (event.text && strcmp(event.text, c->expected_text) == 0));
    if (ok && c->expected_url_count) {
        ok = event.urls && event.url_count == c->expected_url_count &&
             event.url && !strcmp(event.url, c->expected_urls[0]);
        for (i = 0; ok && i < c->expected_url_count; i++) {
            ok = c->expected_urls[i] &&
                 !strcmp(event.urls[i], c->expected_urls[i]);
        }
    }
    if (!ok) {
        printf("[ai_selftest] FAIL %s ws(%s) ret=%d event=%d text=%s urls=%u/%u\n",
               c->provider->name, c->label ? c->label : "",
               ret, event.type, event.text ? event.text : "",
               event.url_count, c->expected_url_count);
    }
    /* owned_data must be released with ai_free_large(): providers allocate it
     * from the large heap (ai_strdup_large / ai_base64_decode). */
    ai_free_large(owned);
    return ok ? AI_OK : AI_ERR_PARSE;
}

static int ai_selftest_http_run(const struct ai_selftest_http_case *c)
{
    struct ai_client client;
    struct ai_vision_result result;
    int ok;
    int ret;

    memset(&client, 0, sizeof(client));
    memset(&result, 0, sizeof(result));
    client.provider = c->provider;
    client.provider_name = (char *)c->provider->name;

    ret = c->provider->parse_vision_http_response(&client, c->json, &result);
    ok = c->expect_ok ? (ret == AI_OK) : (ret != AI_OK);
    if (ok && c->expected_text) {
        ok = result.text && strcmp(result.text, c->expected_text) == 0;
    }
    if (!ok) {
        printf("[ai_selftest] FAIL %s http(%s) ret=%d text=%s\n",
               c->provider->name, c->label ? c->label : "",
               ret, result.text ? result.text : "");
    }
    ai_vision_result_free(&result);
    return ok ? AI_OK : AI_ERR_PARSE;
}

static int ai_selftest_ali_session_update(int vad_mode)
{
    struct ai_client client;
    struct ai_ws_message msg;
    cJSON *root = NULL;
    cJSON *session;
    cJSON *turn_detection;
    int ret = AI_ERR_PARSE;

    memset(&client, 0, sizeof(client));
    memset(&msg, 0, sizeof(msg));
    client.provider = &ai_provider_ali_realtime;
    client.provider_name = (char *)ai_provider_ali_realtime.name;
    if (vad_mode) {
        client.has_turn_detection = 1;
        client.turn_detection.type = AI_TURN_DETECTION_SERVER_VAD;
        client.turn_detection.create_response = 1;
        if (vad_mode == 1) {
            client.turn_detection.has_threshold = 1;
            client.turn_detection.threshold = 0.5f;
            client.turn_detection.silence_duration_ms = 800;
        }
    }

    if (client.provider->build_session_start(&client, &msg) != AI_OK) {
        goto out;
    }
    root = cJSON_Parse((const char *)msg.data);
    session = root ? cJSON_GetObjectItem(root, "session") : NULL;
    turn_detection = session ? cJSON_GetObjectItem(session, "turn_detection") : NULL;
    if ((!vad_mode && cJSON_IsNull(turn_detection)) ||
        (vad_mode && cJSON_IsObject(turn_detection) &&
         cJSON_IsString(cJSON_GetObjectItem(turn_detection, "type")) &&
         !strcmp(cJSON_GetObjectItem(turn_detection, "type")->valuestring, "server_vad") &&
         ((vad_mode == 1 &&
           cJSON_IsNumber(cJSON_GetObjectItem(turn_detection, "threshold")) &&
           cJSON_GetObjectItem(turn_detection, "threshold")->valuedouble == 0.5 &&
           cJSON_IsNumber(cJSON_GetObjectItem(turn_detection, "silence_duration_ms")) &&
           cJSON_GetObjectItem(turn_detection, "silence_duration_ms")->valueint == 800) ||
          (vad_mode == 2 &&
           !cJSON_GetObjectItem(turn_detection, "threshold") &&
           !cJSON_GetObjectItem(turn_detection, "silence_duration_ms"))) &&
         cJSON_IsTrue(cJSON_GetObjectItem(turn_detection, "create_response")))) {
        ret = AI_OK;
    }

out:
    if (ret != AI_OK) {
        printf("[ai_selftest] FAIL ali session.update vad_mode=%d\n", vad_mode);
    }
    cJSON_Delete(root);
    ai_ws_message_free(&msg);
    return ret;
}

int ai_provider_selftest(void)
{
    const char *reason;
    unsigned int i;
    unsigned int ws_count = sizeof(selftest_ws_cases) / sizeof(selftest_ws_cases[0]);
    unsigned int http_count = sizeof(selftest_http_cases) / sizeof(selftest_http_cases[0]);
    unsigned int provider_count = sizeof(selftest_providers) / sizeof(selftest_providers[0]);
    int ret;

    for (i = 0; i < provider_count; i++) {
        ret = ai_provider_validate(selftest_providers[i], &reason);
        if (ret != AI_OK) {
            printf("[ai_selftest] FAIL %s contract: %s\n",
                   selftest_providers[i]->name, reason ? reason : "unknown");
            return ret;
        }
    }

    for (i = 0; i < 3; i++) {
        ret = ai_selftest_ali_session_update((int)i);
        if (ret != AI_OK) {
            return ret;
        }
    }

    for (i = 0; i < ws_count; i++) {
        ret = ai_selftest_ws_run(&selftest_ws_cases[i]);
        if (ret != AI_OK) {
            return ret;
        }
    }

    for (i = 0; i < http_count; i++) {
        ret = ai_selftest_http_run(&selftest_http_cases[i]);
        if (ret != AI_OK) {
            return ret;
        }
    }

    printf("[ai_selftest] PASS providers=%u ws_fixtures=%u http_fixtures=%u session_builds=3\n",
           provider_count, ws_count, http_count);
    return AI_OK;
}
