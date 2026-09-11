#include "ai_internal.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE

#include "cJSON.h"
#include "libwebsockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Replace {WorkspaceId} with the business workspace ID before use. */
#define ALI_REALTIME_WS_URL "wss://{WorkspaceId}.cn-beijing.maas.aliyuncs.com/api-ws/v1/realtime?model=" ALI_REALTIME_MODEL
#define ALI_REALTIME_MODEL "qwen3-omni-flash-realtime"
/* #define ALI_REALTIME_MODEL "qwen3.5-omni-plus-realtime" */
#define ALI_REALTIME_TX_LOG 0
#define ALI_REALTIME_TX_PREFIX_LEN 24

static const char *ali_realtime_json_string(cJSON *root, const char *name);

static const struct ai_provider_voice_profile ali_realtime_voice_profile = {
    .audio_format = AI_AUDIO_PCM16,
    .audio_chunk_size = 3200,
    .frame_interval_ms = 100,
    .pending_low_watermark = 4,
    .upload_log_interval = 20,
    .sample_rate = 16000,
    .channels = 1,
    .bits_per_sample = 16,
    .flow_mode = AI_VOICE_FLOW_LONG_STREAM,
    .poll_timeout_ms = 20,
    .audio_poll_timeout_ms = 10,
    .connect_polls = 500,
    .ready_polls = 500,
    .response_polls = 3000,
    .send_retry_polls = 20,
    .speak_stop_hold_polls = 200,
};

static int ali_realtime_add_event_id(cJSON *root, const char *suffix)
{
    static unsigned int counter;
    char event_id[48];
    int written;

    if (!root || !suffix) {
        return AI_ERR_PARAM;
    }

    counter++;
    written = snprintf(event_id, sizeof(event_id), "event_%s_%u", suffix, counter);
    if (written < 0 || written >= (int)sizeof(event_id)) {
        return AI_ERR_NOMEM;
    }

    return cJSON_AddStringToObject(root, "event_id", event_id) ? AI_OK : AI_ERR_NOMEM;
}

#if ALI_REALTIME_TX_LOG
static void ali_realtime_mask_b64_field(cJSON *root, const char *name)
{
    cJSON *item;
    char masked[96];
    char prefix[ALI_REALTIME_TX_PREFIX_LEN + 1];
    const char *value;
    unsigned int prefix_len;
    unsigned int value_len;
    int written;

    item = cJSON_GetObjectItem(root, name);
    if (!cJSON_IsString(item) || !item->valuestring) {
        return;
    }

    value = item->valuestring;
    value_len = (unsigned int)strlen(value);
    prefix_len = value_len > ALI_REALTIME_TX_PREFIX_LEN ? ALI_REALTIME_TX_PREFIX_LEN : value_len;
    memcpy(prefix, value, prefix_len);
    prefix[prefix_len] = '\0';
    written = snprintf(masked, sizeof(masked), "<base64 len=%u prefix=%s>", value_len, prefix);
    if (written < 0 || written >= (int)sizeof(masked)) {
        return;
    }

    cJSON_SetValuestring(item, masked);
}

static void ali_realtime_log_tx_json(cJSON *root)
{
    cJSON *dup;
    char *printed;
    const char *type;

    if (!root) {
        return;
    }

    dup = cJSON_Duplicate(root, 1);
    if (!dup) {
        return;
    }

    ali_realtime_mask_b64_field(dup, "audio");
    ali_realtime_mask_b64_field(dup, "image");

    printed = cJSON_PrintUnformatted(dup);
    type = ali_realtime_json_string(dup, "type");
    if (printed) {
        printf("[ali_rt] tx %s: %s\n", type ? type : "json", printed);
        cJSON_free(printed);
    }
    cJSON_Delete(dup);
}
#endif

static int ali_realtime_json_msg(cJSON *root, struct ai_ws_message *out_msg)
{
    char *printed;
    int ret;

    if (!root || !out_msg) {
        return AI_ERR_PARAM;
    }

    memset(out_msg, 0, sizeof(*out_msg));
    printed = cJSON_PrintUnformatted(root);
    if (!printed) {
        return AI_ERR_NOMEM;
    }

#if ALI_REALTIME_TX_LOG
    ali_realtime_log_tx_json(root);
#endif

    ret = ai_ws_message_set_text(out_msg, printed);
    cJSON_free(printed);
    return ret;
}

static int ali_realtime_b64_append_msg(const char *event_suffix,
                                       const char *type,
                                       const char *field,
                                       const char *base64,
                                       struct ai_ws_message *out_msg)
{
    static unsigned int counter;
    char event_id[48];
    char *json;
    char prefix[ALI_REALTIME_TX_PREFIX_LEN + 1];
    size_t json_len;
    unsigned int prefix_len;
    int written;

    if (!event_suffix || !type || !field || !base64 || !out_msg) {
        return AI_ERR_PARAM;
    }

    counter++;
    written = snprintf(event_id, sizeof(event_id), "event_%s_%u", event_suffix, counter);
    if (written < 0 || written >= (int)sizeof(event_id)) {
        return AI_ERR_NOMEM;
    }

    json_len = strlen("{\"event_id\":\"\",\"type\":\"\",\"\":\"\"}") +
               strlen(event_id) + strlen(type) + strlen(field) + strlen(base64) + 1;
    json = (char *)ai_malloc_large(json_len);
    if (!json) {
        return AI_ERR_NOMEM;
    }

    written = snprintf(json, json_len, "{\"event_id\":\"%s\",\"type\":\"%s\",\"%s\":\"%s\"}",
                       event_id, type, field, base64);
    if (written < 0 || written >= (int)json_len) {
        ai_free_large(json);
        return AI_ERR_NOMEM;
    }

#if ALI_REALTIME_TX_LOG
    prefix_len = strlen(base64) > ALI_REALTIME_TX_PREFIX_LEN ? ALI_REALTIME_TX_PREFIX_LEN : strlen(base64);
    memcpy(prefix, base64, prefix_len);
    prefix[prefix_len] = '\0';
    printf("[ali_rt] tx %s: {\"event_id\":\"%s\",\"type\":\"%s\",\"%s\":\"<base64 len=%u prefix=%s>\"}\n",
           type, event_id, type, field, (unsigned int)strlen(base64), prefix);
#endif

    memset(out_msg, 0, sizeof(*out_msg));
    out_msg->data = (unsigned char *)json;
    out_msg->len = (unsigned int)written;
    out_msg->binary = 0;
    return AI_OK;
}

static int ali_realtime_add_turn_detection(struct ai_client *client, cJSON *session)
{
    const struct ai_turn_detection_config *cfg;
    const char *type;
    cJSON *turn_detection;

    if (!client || !session) {
        return AI_ERR_PARAM;
    }
    if (!client->has_turn_detection) {
        return cJSON_AddNullToObject(session, "turn_detection") ? AI_OK : AI_ERR_NOMEM;
    }

    cfg = &client->turn_detection;
    if ((cfg->type != AI_TURN_DETECTION_SERVER_VAD &&
         cfg->type != AI_TURN_DETECTION_SEMANTIC_VAD) ||
        (cfg->has_threshold && (cfg->threshold < -1.0f || cfg->threshold > 1.0f)) ||
        (cfg->silence_duration_ms &&
         (cfg->silence_duration_ms < 200 || cfg->silence_duration_ms > 6000)) ||
        (cfg->type == AI_TURN_DETECTION_SEMANTIC_VAD && cfg->idle_timeout_ms) ||
        (cfg->idle_timeout_ms &&
         (cfg->idle_timeout_ms < 5000 || cfg->idle_timeout_ms > 30000))) {
        printf("[ali_rt] invalid turn_detection config\n");
        return AI_ERR_PARAM;
    }

    type = cfg->type == AI_TURN_DETECTION_SEMANTIC_VAD ? "semantic_vad" : "server_vad";
    turn_detection = cJSON_CreateObject();
    if (!turn_detection ||
        !cJSON_AddStringToObject(turn_detection, "type", type) ||
        (cfg->has_threshold &&
         !cJSON_AddNumberToObject(turn_detection, "threshold", cfg->threshold)) ||
        (cfg->silence_duration_ms &&
         !cJSON_AddNumberToObject(turn_detection, "silence_duration_ms", cfg->silence_duration_ms)) ||
        !cJSON_AddBoolToObject(turn_detection, "create_response", cfg->create_response ? 1 : 0) ||
        (cfg->idle_timeout_ms &&
         !cJSON_AddNumberToObject(turn_detection, "idle_timeout_ms", cfg->idle_timeout_ms))) {
        cJSON_Delete(turn_detection);
        return AI_ERR_NOMEM;
    }
    if (!cJSON_AddItemToObject(session, "turn_detection", turn_detection)) {
        cJSON_Delete(turn_detection);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int ali_realtime_build_session_start(struct ai_client *client, struct ai_ws_message *out_msg)
{
    cJSON *root;
    cJSON *session;
    cJSON *modalities;
    int ret = AI_ERR_NOMEM;

    if (!client || !out_msg) {
        return AI_ERR_PARAM;
    }

    root = cJSON_CreateObject();
    session = cJSON_CreateObject();
    modalities = cJSON_CreateArray();
    if (!root || !session || !modalities) {
        goto out;
    }

    const char *instructions = (client->provider_params && client->provider_params[0]) ?
                               client->provider_params :
                               "你是一个友好的AI助手，请用简洁自然的中文回答用户。";

    if (ali_realtime_add_event_id(root, "session_update") != AI_OK ||
        !cJSON_AddStringToObject(root, "type", "session.update") ||
        !cJSON_AddItemToArray(modalities, cJSON_CreateString("text")) ||
        !cJSON_AddItemToArray(modalities, cJSON_CreateString("audio"))) {
        goto out;
    }

    /* Transfer modalities into session and immediately drop our handle. Keeping
     * modalities non-NULL after ownership moved would make the out: cleanup free
     * it once directly and again via session (double free on any later OOM). */
    if (!cJSON_AddItemToObject(session, "modalities", modalities)) {
        goto out;
    }
    modalities = NULL;

    if (!cJSON_AddStringToObject(session, "input_audio_format", "pcm") ||
        !cJSON_AddStringToObject(session, "output_audio_format", "pcm") ||
        !cJSON_AddStringToObject(session, "voice", "Ethan") ||
        !cJSON_AddStringToObject(session, "instructions", instructions) ||
        ali_realtime_add_turn_detection(client, session) != AI_OK) {
        goto out;
    }

    if (!cJSON_AddItemToObject(root, "session", session)) {
        goto out;
    }
    session = NULL;

    ret = ali_realtime_json_msg(root, out_msg);

out:
    cJSON_Delete(modalities);
    cJSON_Delete(session);
    cJSON_Delete(root);
    return ret;
}

static int ali_realtime_build_audio(struct ai_client *client,
                                    const struct ai_audio_input *audio,
                                    struct ai_ws_message *out_msg)
{
    char *base64 = NULL;
    int ret;

    (void)client;
    if (!audio || !audio->data || !audio->size || !out_msg) {
        return AI_ERR_PARAM;
    }
    if (audio->format != AI_AUDIO_PCM16) {
        return AI_ERR_UNSUPPORTED;
    }

    ret = ai_base64_encode(audio->data, audio->size, &base64);
    if (ret != AI_OK) {
        return ret;
    }

    ret = ali_realtime_b64_append_msg("audio_append", "input_audio_buffer.append", "audio", base64, out_msg);
    ai_free_large(base64);
    return ret;
}

static void ali_realtime_log_image_head(const unsigned char *data, unsigned int len)
{
    char line[192];
    int off;
    unsigned int i;
    unsigned int n = len > 32 ? 32 : len;

    off = snprintf(line, sizeof(line), "[ali_rt] image input head: len=%u", len);
    for (i = 0; i < n && off > 0 && off < (int)sizeof(line); i++) {
        off += snprintf(line + off, sizeof(line) - off, " %02x", data[i]);
    }
    line[sizeof(line) - 1] = '\0';
    printf("%s\n", line);
}

static int ali_realtime_build_image(struct ai_client *client,
                                    const struct ai_image_input *image,
                                    struct ai_ws_message *out_msg)
{
    char *base64 = NULL;
    int ret;

    (void)client;
    if (!image || !image->data || !image->size || !out_msg) {
        return AI_ERR_PARAM;
    }
    if (image->format != AI_IMAGE_JPEG) {
        return AI_ERR_UNSUPPORTED;
    }

    ali_realtime_log_image_head(image->data, image->size);
    ret = ai_base64_encode(image->data, image->size, &base64);
    if (ret != AI_OK) {
        return ret;
    }

    {
        char prefix[33];
        unsigned int base64_len = (unsigned int)strlen(base64);
        unsigned int prefix_len = base64_len > 32 ? 32 : base64_len;

        memcpy(prefix, base64, prefix_len);
        prefix[prefix_len] = '\0';
        printf("[ali_rt] image base64 head: len=%u prefix=%s\n", base64_len, prefix);
    }
    printf("[ali_rt] build image append: %u bytes\n", image->size);
    ret = ali_realtime_b64_append_msg("image_append", "input_image_buffer.append", "image", base64, out_msg);
    ai_free_large(base64);
    return ret;
}

static int ali_realtime_build_commit(struct ai_client *client, struct ai_ws_message *out_msg)
{
    cJSON *root;
    int ret;

    (void)client;
    if (!out_msg) {
        return AI_ERR_PARAM;
    }

    root = cJSON_CreateObject();
    if (!root) {
        return AI_ERR_NOMEM;
    }
    if (ali_realtime_add_event_id(root, "audio_commit") != AI_OK ||
        !cJSON_AddStringToObject(root, "type", "input_audio_buffer.commit")) {
        cJSON_Delete(root);
        return AI_ERR_NOMEM;
    }
    ret = ali_realtime_json_msg(root, out_msg);
    cJSON_Delete(root);
    return ret;
}

static int ali_realtime_build_response_create(struct ai_client *client, struct ai_ws_message *out_msg)
{
    cJSON *root;
    int ret;

    (void)client;
    if (!out_msg) {
        return AI_ERR_PARAM;
    }

    root = cJSON_CreateObject();
    if (!root) {
        return AI_ERR_NOMEM;
    }
    if (ali_realtime_add_event_id(root, "response_create") != AI_OK ||
        !cJSON_AddStringToObject(root, "type", "response.create")) {
        cJSON_Delete(root);
        return AI_ERR_NOMEM;
    }
    ret = ali_realtime_json_msg(root, out_msg);
    cJSON_Delete(root);
    return ret;
}

static int ali_realtime_build_text(struct ai_client *client, const char *text, struct ai_ws_message *out_msg)
{
    cJSON *root;
    cJSON *item;
    cJSON *content;
    cJSON *content_item;
    int ret = AI_ERR_NOMEM;

    (void)client;
    if (!text || !out_msg) {
        return AI_ERR_PARAM;
    }

    root = cJSON_CreateObject();
    item = cJSON_CreateObject();
    content = cJSON_CreateArray();
    content_item = cJSON_CreateObject();
    if (!root || !item || !content || !content_item) {
        goto out;
    }

    if (ali_realtime_add_event_id(root, "text_input") != AI_OK ||
        !cJSON_AddStringToObject(root, "type", "conversation.item.create") ||
        !cJSON_AddStringToObject(item, "type", "message") ||
        !cJSON_AddStringToObject(item, "role", "user") ||
        !cJSON_AddStringToObject(content_item, "type", "input_text") ||
        !cJSON_AddStringToObject(content_item, "text", text)) {
        goto out;
    }

    /* NULL each handle right after its ownership moves into the parent, so the
     * out: cleanup never deletes the same subtree twice on a later OOM. */
    if (!cJSON_AddItemToArray(content, content_item)) {
        goto out;
    }
    content_item = NULL;

    if (!cJSON_AddItemToObject(item, "content", content)) {
        goto out;
    }
    content = NULL;

    if (!cJSON_AddItemToObject(root, "item", item)) {
        goto out;
    }
    item = NULL;

    ret = ali_realtime_json_msg(root, out_msg);

out:
    cJSON_Delete(content_item);
    cJSON_Delete(content);
    cJSON_Delete(item);
    cJSON_Delete(root);
    return ret;
}

static const char *ali_realtime_json_string(cJSON *root, const char *name)
{
    cJSON *item = cJSON_GetObjectItem(root, name);

    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static int ali_realtime_copy_event_text(struct ai_dialogue_event *event,
                                        unsigned char **owned_data,
                                        enum ai_dialogue_event_type type,
                                        const char *text)
{
    if (!text) {
        return 0;
    }

    *owned_data = (unsigned char *)ai_strdup_large(text);
    if (!*owned_data) {
        return AI_ERR_NOMEM;
    }
    event->type = type;
    event->text = (const char *)*owned_data;
    return 1;
}

static int ali_realtime_parse_ws_message(struct ai_client *client,
        const unsigned char *data,
        unsigned int len,
        int binary,
        struct ai_dialogue_event *out_event,
        unsigned char **owned_data)
{
    cJSON *root;
    const char *type;
    const char *text;
    const char *audio;
    int copied;

    (void)client;
    (void)len;
    if (!out_event || !owned_data) {
        return AI_ERR_PARAM;
    }

    memset(out_event, 0, sizeof(*out_event));
    *owned_data = NULL;

    if (binary) {
        out_event->type = AI_DIALOGUE_AUDIO_DELTA;
        out_event->data = data;
        out_event->data_len = len;
        return AI_OK;
    }

    out_event->raw_json = (const char *)data;
    root = cJSON_Parse((const char *)data);
    if (!root) {
        out_event->type = AI_DIALOGUE_TEXT_DELTA;
        out_event->text = (const char *)data;
        return AI_OK;
    }

    type = ali_realtime_json_string(root, "type");
    if (!type) {
        out_event->type = AI_DIALOGUE_TEXT_DELTA;
        out_event->text = (const char *)data;
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "error") == 0) {
        cJSON *error = cJSON_GetObjectItem(root, "error");
        text = error ? ali_realtime_json_string(error, "message") : NULL;
        copied = ali_realtime_copy_event_text(out_event, owned_data, AI_DIALOGUE_ERROR, text ? text : (const char *)data);
        cJSON_Delete(root);
        return copied < 0 ? copied : AI_OK;
    }

    text = ali_realtime_json_string(root, "text");
    if (!text) {
        text = ali_realtime_json_string(root, "transcript");
    }
    if (!text) {
        text = ali_realtime_json_string(root, "delta");
    }

    if (strcmp(type, "conversation.item.input_audio_transcription.completed") == 0) {
        copied = ali_realtime_copy_event_text(out_event, owned_data, AI_DIALOGUE_TRANSCRIPT, text);
        if (copied < 0) {
            cJSON_Delete(root);
            return copied;
        }
        if (!copied) {
            out_event->type = AI_DIALOGUE_TRANSCRIPT;
        }
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "response.audio.delta") == 0) {
        audio = ali_realtime_json_string(root, "delta");
        if (audio && ai_base64_decode(audio, owned_data, &out_event->data_len) == AI_OK) {
            out_event->type = AI_DIALOGUE_AUDIO_DELTA;
            out_event->data = *owned_data;
            cJSON_Delete(root);
            return AI_OK;
        }
        cJSON_Delete(root);
        return AI_ERR_PARSE;
    }

    if (strcmp(type, "response.audio.done") == 0) {
        out_event->type = AI_DIALOGUE_AUDIO_DONE;
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "input_audio_buffer.speech_started") == 0) {
        out_event->type = AI_DIALOGUE_SPEECH_STARTED;
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "input_audio_buffer.speech_stopped") == 0) {
        out_event->type = AI_DIALOGUE_SPEECH_STOPPED;
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "response.text.delta") == 0 ||
        strcmp(type, "response.audio_transcript.delta") == 0) {
        copied = ali_realtime_copy_event_text(out_event, owned_data, AI_DIALOGUE_TEXT_DELTA, text);
        if (copied < 0) {
            cJSON_Delete(root);
            return copied;
        }
        if (!copied) {
            out_event->type = AI_DIALOGUE_TEXT_DELTA;
        }
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "response.text.done") == 0 ||
        strcmp(type, "response.audio_transcript.done") == 0) {
        copied = ali_realtime_copy_event_text(out_event, owned_data, AI_DIALOGUE_TEXT_DONE, text);
        if (copied < 0) {
            cJSON_Delete(root);
            return copied;
        }
        if (!copied) {
            out_event->type = AI_DIALOGUE_TEXT_DONE;
        }
        cJSON_Delete(root);
        return AI_OK;
    }

    if (strcmp(type, "response.done") == 0) {
        out_event->type = AI_DIALOGUE_DONE;
        cJSON_Delete(root);
        return AI_OK;
    }

    out_event->type = AI_DIALOGUE_TEXT_DELTA;
    out_event->text = (const char *)data;

    cJSON_Delete(root);
    return AI_OK;
}

static int ali_realtime_append_ws_headers(struct ai_client *client,
        struct lws *wsi,
        unsigned char **p,
        unsigned char *end)
{
    int ret;

    ret = ai_ws_append_bearer_auth(client, wsi, p, end);
    if (ret != AI_OK) {
        return ret;
    }

    ret = lws_add_http_header_by_name(wsi,
                                      (const unsigned char *)"x-dashscope-datainspection:",
                                      (const unsigned char *)"enable",
                                      6,
                                      p,
                                      end);
    return ret ? AI_ERR_NET : AI_OK;
}

static int ali_realtime_is_ready_event(const struct ai_dialogue_event *event)
{
    return event && event->raw_json &&
           strstr(event->raw_json, "session.updated");
}

const struct ai_provider_ops ai_provider_ali_realtime = {
    .name = "ali_realtime",
    .caps = AI_PROVIDER_CAP_VOICE_DIALOGUE,
    .default_dialogue_ws_url = ALI_REALTIME_WS_URL,
    .default_model = ALI_REALTIME_MODEL,
    .voice = &ali_realtime_voice_profile,
    .is_ready_event = ali_realtime_is_ready_event,
    .build_session_start = ali_realtime_build_session_start,
    .build_text = ali_realtime_build_text,
    .build_audio = ali_realtime_build_audio,
    .build_image = ali_realtime_build_image,
    .build_commit = ali_realtime_build_commit,
    .build_response_create = ali_realtime_build_response_create,
    .parse_ws_message = ali_realtime_parse_ws_message,
    .append_ws_headers = ali_realtime_append_ws_headers,
};

#endif
