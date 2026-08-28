#include "ai_internal.h"

#include "cJSON.h"
#include "curl/curl.h"

/* #include <stdio.h> */
#include <stdlib.h>
#include <string.h>

/* 阅读顺序：默认参数/voice_profile -> start/text/audio/finish 构造 -> inside_rc 响应解析 -> ai_provider_duer 注册表。 */
struct duer_http_buffer {
    char *data;
    size_t len;
};

#ifndef AI_PROVIDER_DUER_CLIENT_AK
#define AI_PROVIDER_DUER_CLIENT_AK ""
#endif
#ifndef AI_PROVIDER_DUER_CLIENT_SK
#define AI_PROVIDER_DUER_CLIENT_SK ""
#endif
#ifndef AI_PROVIDER_DUER_APP_PID
#define AI_PROVIDER_DUER_APP_PID ""
#endif
#ifndef AI_PROVIDER_DUER_CUID
#define AI_PROVIDER_DUER_CUID "duer01"
#endif

#define DUER_WS_URL "ws://duer-kids.baidu.com/sandbox/sota/realtime_asr?sn=" AI_PROVIDER_DUER_CUID

#ifndef AI_PROVIDER_DUER_LOG_FULL_JSON
#define AI_PROVIDER_DUER_LOG_FULL_JSON 1
#endif
#define DUER_LOG_BODY_PREVIEW 160u
#define DUER_MAX_AUDIO_URLS 8u

static const char duer_provider_params[] =
    "{"
    "\"appid\":\"" AI_PROVIDER_DUER_CLIENT_AK "\","
    "\"appkey\":\"" AI_PROVIDER_DUER_CLIENT_SK "\","
    "\"client_id\":\"" AI_PROVIDER_DUER_CLIENT_AK "\","
    "\"client_secret\":\"" AI_PROVIDER_DUER_CLIENT_SK "\","
    "\"pid\":\"" AI_PROVIDER_DUER_APP_PID "\","
    "\"cuid\":\"" AI_PROVIDER_DUER_CUID "\","
    "\"format\":\"opus\","
    "\"sample\":16000,"
    "\"support_dcs\":1,"
    "\"support_tts\":1,"
    "\"support_text2dcs\":1,"
    "\"access_rc\":1,"
    "\"rc_version\":\"1\""
    "}";

static const struct ai_provider_voice_profile duer_voice_profile = {
    .audio_format = AI_AUDIO_OPUS,
    .audio_chunk_size = 80,
    .frame_interval_ms = 10,
    .pending_low_watermark = 4,
    .upload_log_interval = 20,
    .sample_rate = 16000,
    .channels = 1,
    .bits_per_sample = 16,
    .flow_mode = AI_VOICE_FLOW_SINGLE_TURN,
    .poll_timeout_ms = 0,
    .audio_poll_timeout_ms = 0,
    .connect_polls = 500,
    .ready_polls = 500,
    .response_polls = 3000,
    .send_retry_polls = 20,
    .speak_stop_hold_polls = 200,
};

static unsigned int duer_dialog_counter;

static char *duer_url_encode(const char *text);

/* Duer token HTTP 返回体先完整攒在内存里，后面再统一做 JSON 解析。 */
static size_t duer_http_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct duer_http_buffer *buf = (struct duer_http_buffer *)userdata;
    size_t bytes = size * nmemb;
    char *new_data;

    if (!buf || !ptr) {
        return 0;
    }

    new_data = (char *)realloc(buf->data, buf->len + bytes + 1);
    if (!new_data) {
        return 0;
    }

    memcpy(new_data + buf->len, ptr, bytes);
    buf->len += bytes;
    new_data[buf->len] = '\0';
    buf->data = new_data;

    return bytes;
}

static void duer_log_curl_error(const char *stage, CURLcode code)
{
    printf("[duer] %s failed: %d (%s)\n", stage, (int)code, curl_easy_strerror(code));
}

static void duer_log_body_preview(const char *stage, const char *body, size_t len)
{
    size_t preview_len;

    if (!body || !len) {
        printf("[duer] %s body empty\n", stage);
        return;
    }

    preview_len = len > DUER_LOG_BODY_PREVIEW ? DUER_LOG_BODY_PREVIEW : len;
    printf("[duer] %s body len=%u preview=%.*s%s\n",
           stage,
           (unsigned int)len,
           (int)preview_len,
           body,
           len > preview_len ? "..." : "");
}

/* provider_params 允许 app 覆盖默认账号/设备字段，缺省时退回空对象继续走默认值。 */
static cJSON *duer_parse_params(struct ai_client *client)
{
    if (!client || !client->provider_params || !client->provider_params[0]) {
        return cJSON_CreateObject();
    }

    return cJSON_Parse(client->provider_params);
}

static const char *duer_get_string(cJSON *root, const char *name)
{
    cJSON *item = cJSON_GetObjectItem(root, name);

    if (cJSON_IsString(item)) {
        return item->valuestring;
    }

    return NULL;
}

static const char *duer_get_string2(cJSON *root, const char *name1, const char *name2)
{
    const char *value = duer_get_string(root, name1);

    return value ? value : duer_get_string(root, name2);
}

static int duer_get_int(cJSON *root, const char *name, int default_value)
{
    cJSON *item = cJSON_GetObjectItem(root, name);

    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item) ? 1 : 0;
    }

    return default_value;
}

static int duer_json_print_to_msg(cJSON *root, struct ai_ws_message *out_msg)
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

    ret = ai_ws_message_set_text(out_msg, printed);
    cJSON_free(printed);
    return ret;
}

static int duer_add_string(cJSON *root, const char *name, const char *value)
{
    if (!value || !cJSON_AddStringToObject(root, name, value)) {
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int duer_add_number(cJSON *root, const char *name, int value)
{
    if (!cJSON_AddNumberToObject(root, name, value)) {
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int duer_add_bool(cJSON *root, const char *name, int value)
{
    if (!cJSON_AddItemToObject(root, name, cJSON_CreateBool(value ? 1 : 0))) {
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

/* access_token 单独抽出来解析，方便把 HTTP 获取和 ws 建链解耦。 */
static int duer_extract_access_token(const char *json, char **out)
{
    cJSON *root;
    const char *token;

    if (!json || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    root = cJSON_Parse(json);
    if (!root) {
        printf("[duer] token response json parse failed\n");
        return AI_ERR_PARSE;
    }

    token = duer_get_string(root, "access_token");
    if (token) {
        *out = ai_strdup(token);
    } else {
        printf("[duer] token response missing access_token\n");
    }

    cJSON_Delete(root);
    return *out ? AI_OK : AI_ERR_PARSE;
}

/* 优先复用外部传入 token，没有时才走百度 oauth，避免每次对话都额外取票。 */
static int duer_get_access_token(struct ai_client *client, cJSON *params, char **out)
{
    const char *token;
    const char *client_id;
    const char *client_secret;
    char *encoded_client_id;
    char *encoded_client_secret;
    char url[512];
    CURL *curl;
    CURLcode code;
    char curl_error[CURL_ERROR_SIZE] = {0};
    struct duer_http_buffer response = {0};
    long http_code = 0;
    int written;
    int ret;

    if (!client || !params || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    token = duer_get_string(params, "access_token");
    if (!token) {
        token = client->api_key;
    }
    if (token && token[0]) {
        *out = ai_strdup(token);
        return *out ? AI_OK : AI_ERR_NOMEM;
    }

    client_id = duer_get_string2(params, "client_id", "appid");
    if (!client_id) {
        client_id = duer_get_string(params, "app_id");
    }
    client_secret = duer_get_string2(params, "client_secret", "appkey");
    if (!client_secret) {
        client_secret = duer_get_string(params, "app_key");
    }
    if (!client_id || !client_secret) {
        printf("[duer] token request missing client_id or client_secret\n");
        return AI_ERR_PARAM;
    }

    encoded_client_id = duer_url_encode(client_id);
    encoded_client_secret = duer_url_encode(client_secret);
    if (!encoded_client_id || !encoded_client_secret) {
        printf("[duer] token request url encode failed\n");
        free(encoded_client_id);
        free(encoded_client_secret);
        return AI_ERR_NOMEM;
    }

    written = snprintf(url, sizeof(url),
                       "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s",
                       encoded_client_id,
                       encoded_client_secret);
    free(encoded_client_id);
    free(encoded_client_secret);
    if (written < 0 || written >= (int)sizeof(url)) {
        printf("[duer] token request url overflow\n");
        return AI_ERR_PARAM;
    }

    curl = curl_easy_init();
    if (!curl) {
        printf("[duer] token request curl init failed\n");
        return AI_ERR_NET;
    }

    code = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
    if (code != CURLE_OK) {
        duer_log_curl_error("token set error buffer", code);
        ret = AI_ERR_NET;
        goto out;
    }
    code = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (code != CURLE_OK) {
        duer_log_curl_error("token set url", code);
        ret = AI_ERR_NET;
        goto out;
    }
    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, duer_http_write_cb);
    if (code != CURLE_OK) {
        duer_log_curl_error("token set write function", code);
        ret = AI_ERR_NET;
        goto out;
    }
    code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if (code != CURLE_OK) {
        duer_log_curl_error("token set write data", code);
        ret = AI_ERR_NET;
        goto out;
    }
    if (client->timeout_ms > 0) {
        code = curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)client->timeout_ms);
        if (code != CURLE_OK) {
            duer_log_curl_error("token set timeout", code);
            ret = AI_ERR_NET;
            goto out;
        }
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (code != CURLE_OK) {
        duer_log_curl_error("token request", code);
        if (curl_error[0]) {
            printf("[duer] token curl detail: %s\n", curl_error);
        }
        duer_log_body_preview("token error", response.data, response.len);
        ret = AI_ERR_NET;
    } else if (http_code < 200 || http_code >= 300) {
        printf("[duer] token request http failed: %ld\n", http_code);
        duer_log_body_preview("token http error", response.data, response.len);
        ret = AI_ERR_NET;
    } else if (!response.data) {
        printf("[duer] token request returned empty body\n");
        ret = AI_ERR_NET;
    } else {
        ret = duer_extract_access_token(response.data, out);
        if (ret != AI_OK) {
            printf("[duer] token parse failed: %d\n", ret);
            duer_log_body_preview("token parse error", response.data, response.len);
        }
    }

out:
    curl_easy_cleanup(curl);
    free(response.data);
    return ret;
}

static int duer_is_url_unreserved(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

/* Duer 文本 query 和 oauth 参数都要走 URL 编码，避免把协议细节散落到调用处。 */
static char *duer_url_encode(const char *text)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t len;
    char *out;
    char *p;
    size_t i;

    if (!text) {
        return NULL;
    }

    len = strlen(text);
    out = (char *)malloc(len * 3 + 1);
    if (!out) {
        return NULL;
    }

    p = out;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        if (duer_is_url_unreserved(c)) {
            *p++ = (char)c;
        } else {
            *p++ = '%';
            *p++ = hex[c >> 4];
            *p++ = hex[c & 0x0f];
        }
    }
    *p = '\0';

    return out;
}

/* 单次对话 request id 本地自增即可，保证一次启动内每轮会话可区分。 */
static void duer_make_dialog_id(const char *cuid, char *buf, size_t len)
{
    duer_dialog_counter++;
    snprintf(buf, len, "%s_%08u", cuid ? cuid : "duer", duer_dialog_counter);
}

/* Duer start 包字段较多，统一在这里按默认值和 provider_params 组装。 */
static int duer_build_session_start(struct ai_client *client, struct ai_ws_message *out_msg)
{
    cJSON *params;
    char *json;
    char *access_token = NULL;
    char dialog_id[80];
    int support_dcs;
    int sample;
    int support_tts;
    int support_text2dcs;
    int access_rc;
    int written;
    const char *appid;
    const char *appkey;
    const char *client_id;
    const char *pid;
    const char *cuid;
    const char *format;
    const char *user_id;
    const char *dialog_request_id;
    const char *rc_version;
    int ret;

    if (!client || !out_msg) {
        return AI_ERR_PARAM;
    }

    params = duer_parse_params(client);
    if (!params) {
        return AI_ERR_PARSE;
    }

    appid = duer_get_string2(params, "appid", "app_id");
    client_id = duer_get_string(params, "client_id");
    if (!appid) {
        appid = client_id;
    }
    if (!client_id) {
        client_id = appid;
    }

    appkey = duer_get_string2(params, "appkey", "app_key");
    if (!appkey) {
        appkey = duer_get_string(params, "client_secret");
    }
    pid = duer_get_string(params, "pid");
    cuid = duer_get_string2(params, "cuid", "sn");
    if (!cuid) {
        cuid = "duer_test";
    }
    format = duer_get_string(params, "format");
    if (!format) {
        format = "opus";
    }
    user_id = duer_get_string(params, "user_id");
    if (!user_id) {
        user_id = cuid;
    }
    dialog_request_id = duer_get_string(params, "dialog_request_id");
    if (!dialog_request_id) {
        duer_make_dialog_id(cuid, dialog_id, sizeof(dialog_id));
        dialog_request_id = dialog_id;
    }
    rc_version = duer_get_string(params, "rc_version");
    if (!rc_version) {
        rc_version = "1";
    }

    if (!appid || !appkey || !client_id || !pid) {
        printf("[duer] session_start missing params: appid=%d appkey=%d client_id=%d pid=%d\n",
               appid != NULL,
               appkey != NULL,
               client_id != NULL,
               pid != NULL);
        cJSON_Delete(params);
        return AI_ERR_PARAM;
    }

    support_dcs = duer_get_int(params, "support_dcs", 1);
    sample = duer_get_int(params, "sample", 16000);
    support_tts = duer_get_int(params, "support_tts", 1);
    support_text2dcs = duer_get_int(params, "support_text2dcs", 1);
    access_rc = duer_get_int(params, "access_rc", 1);

    ret = duer_get_access_token(client, params, &access_token);
    if (ret != AI_OK) {
        printf("[duer] session_start access_token failed: %d\n", ret);
        cJSON_Delete(params);
        return ret;
    }

    json = (char *)malloc(1024);
    if (!json) {
        printf("[duer] session_start json alloc failed\n");
        free(access_token);
        cJSON_Delete(params);
        return AI_ERR_NOMEM;
    }

    written = snprintf(json, 1024,
                       "{\"type\":\"start\",\"data\":{"
                       "\"appid\":\"%s\","
                       "\"appkey\":\"%s\","
                       "\"client_id\":\"%s\","
                       "\"pid\":\"%s\","
                       "\"cuid\":\"%s\","
                       "\"format\":\"%s\","
                       "\"support_dcs\":%d,"
                       "\"sample\":%d,"
                       "\"user_id\":\"%s\","
                       "\"access_token\":\"%s\","
                       "\"support_tts\":%s,"
                       "\"support_text2dcs\":%s,"
                       "\"dialog_request_id\":\"%s\","
                       "\"access_rc\":%s,"
                       "\"rc_version\":\"%s\""
                       "}}",
                       appid,
                       appkey,
                       client_id,
                       pid,
                       cuid,
                       format,
                       support_dcs,
                       sample,
                       user_id,
                       access_token,
                       support_tts ? "true" : "false",
                       support_text2dcs ? "true" : "false",
                       dialog_request_id,
                       access_rc ? "true" : "false",
                       rc_version);
    if (written < 0 || written >= 1024) {
        printf("[duer] session_start json overflow: %d\n", written);
        free(json);
        free(access_token);
        cJSON_Delete(params);
        return AI_ERR_PARAM;
    }

    ret = ai_ws_message_set_text(out_msg, json);
    if (ret != AI_OK) {
        printf("[duer] session_start serialize failed: %d\n", ret);
    } else {
        printf("[duer] session_start payload ready: len=%u dialog_id=%s\n",
               out_msg->len,
               dialog_request_id);
    }

    free(json);
    free(access_token);
    cJSON_Delete(params);
    return ret;
}

/* 文本输入走 DCS LinkClicked 兼容路径，这样语音和文本都能复用同一套后端能力。 */
static int duer_build_text(struct ai_client *client, const char *text, struct ai_ws_message *out_msg)
{
    cJSON *params;
    cJSON *root;
    cJSON *event;
    cJSON *header;
    cJSON *payload;
    cJSON *initiator;
    char *url;
    char *encoded_text;
    size_t url_len;
    const char *cuid;
    const char *dialog_request_id;
    int ret;

    if (!client || !text || !out_msg) {
        return AI_ERR_PARAM;
    }

    params = duer_parse_params(client);
    if (!params) {
        return AI_ERR_PARSE;
    }

    cuid = duer_get_string2(params, "cuid", "sn");
    if (!cuid) {
        cuid = "duer_test";
    }
    dialog_request_id = duer_get_string(params, "dialog_request_id");
    if (!dialog_request_id) {
        dialog_request_id = "";
    }

    encoded_text = duer_url_encode(text);
    if (!encoded_text) {
        cJSON_Delete(params);
        return AI_ERR_NOMEM;
    }

    url_len = strlen("dueros://server.dueros.ai/query?q=") + strlen(encoded_text) + 1;
    url = (char *)malloc(url_len);
    if (!url) {
        free(encoded_text);
        cJSON_Delete(params);
        return AI_ERR_NOMEM;
    }
    snprintf(url, url_len, "dueros://server.dueros.ai/query?q=%s", encoded_text);

    root = cJSON_CreateObject();
    event = cJSON_CreateObject();
    header = cJSON_CreateObject();
    payload = cJSON_CreateObject();
    initiator = cJSON_CreateObject();
    if (!root || !event || !header || !payload || !initiator) {
        cJSON_Delete(root);
        cJSON_Delete(event);
        cJSON_Delete(header);
        cJSON_Delete(payload);
        cJSON_Delete(initiator);
        free(url);
        free(encoded_text);
        cJSON_Delete(params);
        return AI_ERR_NOMEM;
    }

    ret = duer_add_string(root, "status", "ok");
    if (ret == AI_OK) {
        ret = duer_add_string(root, "type", "dcs_decide");
    }
    if (ret == AI_OK) {
        ret = duer_add_string(root, "sn", cuid);
    }
    if (ret == AI_OK) {
        ret = duer_add_number(root, "end", 1);
    }
    if (ret == AI_OK && !cJSON_AddItemToObject(root, "event", event)) {
        ret = AI_ERR_NOMEM;
    }
    if (ret == AI_OK) {
        event = NULL;
        if (!cJSON_AddItemToObject(cJSON_GetObjectItem(root, "event"), "header", header)) {
            ret = AI_ERR_NOMEM;
        }
    }
    if (ret == AI_OK) {
        header = NULL;
        if (!cJSON_AddItemToObject(cJSON_GetObjectItem(root, "event"), "payload", payload)) {
            ret = AI_ERR_NOMEM;
        }
    }
    if (ret == AI_OK) {
        payload = NULL;
        ret = duer_add_string(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "header"),
                              "namespace", "ai.dueros.device_interface.screen");
    }
    if (ret == AI_OK) {
        ret = duer_add_string(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "header"),
                              "name", "LinkClicked");
    }
    if (ret == AI_OK) {
        ret = duer_add_string(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "header"),
                              "dialogRequestId", dialog_request_id);
    }
    if (ret == AI_OK) {
        ret = duer_add_string(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "payload"), "url", url);
    }
    if (ret == AI_OK && !cJSON_AddItemToObject(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "payload"),
            "initiator", initiator)) {
        ret = AI_ERR_NOMEM;
    }
    if (ret == AI_OK) {
        initiator = NULL;
        ret = duer_add_string(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "event"), "payload"), "initiator"),
                              "type", "USER_CLICK");
    }
    if (ret == AI_OK) {
        ret = duer_json_print_to_msg(root, out_msg);
    }

    cJSON_Delete(root);
    cJSON_Delete(event);
    cJSON_Delete(header);
    cJSON_Delete(payload);
    cJSON_Delete(initiator);
    free(url);
    free(encoded_text);
    cJSON_Delete(params);
    return ret;
}

/* Duer 语音上行直接发二进制帧，格式选择在 session_start 和 voice profile 里约定。 */
static int duer_build_audio(struct ai_client *client,
                            const struct ai_audio_input *audio,
                            struct ai_ws_message *out_msg)
{
    (void)client;

    if (!audio || !audio->data || !audio->size || !out_msg) {
        return AI_ERR_PARAM;
    }

    return ai_ws_message_set_binary(out_msg, audio->data, audio->size);
}

/* commit/stop 都走 finish，让 realtime 层不需要感知 Duer 的结束语义。 */
static int duer_build_finish(struct ai_client *client, struct ai_ws_message *out_msg)
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

    ret = duer_add_string(root, "type", "finish");
    if (ret == AI_OK) {
        ret = duer_json_print_to_msg(root, out_msg);
    }

    cJSON_Delete(root);
    return ret;
}

static const char *duer_get_nested_string(cJSON *root, const char *name)
{
    cJSON *item;

    if (!root) {
        return NULL;
    }

    item = cJSON_GetObjectItem(root, name);
    if (cJSON_IsString(item)) {
        return item->valuestring;
    }

    return NULL;
}

static cJSON *duer_find_first_payload(cJSON *data)
{
    cJSON *items;

    items = cJSON_GetObjectItem(data, "data");
    if (!cJSON_IsArray(items) || cJSON_GetArraySize(items) <= 0) {
        return NULL;
    }

    return cJSON_GetObjectItem(cJSON_GetArrayItem(items, 0), "payload");
}

static unsigned int duer_collect_payload_audio_urls(cJSON *data,
        const char **urls,
        unsigned int max_urls)
{
    cJSON *items;
    int count;
    unsigned int url_count = 0;
    int i;

    items = cJSON_GetObjectItem(data, "data");
    if (!cJSON_IsArray(items) || !urls || !max_urls) {
        return 0;
    }

    count = cJSON_GetArraySize(items);
    for (i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(items, i);
        cJSON *header = cJSON_GetObjectItem(item, "header");
        cJSON *payload = cJSON_GetObjectItem(item, "payload");
        cJSON *audio_item;
        cJSON *stream;
        const char *name_space = duer_get_nested_string(header, "namespace");
        const char *name = duer_get_nested_string(header, "name");
        const char *url;

        if (name_space && name &&
            !strcmp(name_space, "ai.dueros.device_interface.voice_output") &&
            !strcmp(name, "Speak")) {
            url = duer_get_nested_string(payload, "url");
            if (url && url[0]) {
                urls[url_count++] = url;
                if (url_count >= max_urls) {
                    break;
                }
            }
            continue;
        }

        if (!name_space || !name ||
            strcmp(name_space, "ai.dueros.device_interface.audio_player") ||
            strcmp(name, "Play")) {
            continue;
        }
        audio_item = cJSON_GetObjectItem(payload, "audioItem");
        stream = cJSON_GetObjectItem(audio_item, "stream");
        url = duer_get_nested_string(stream, "url");
        if (url && url[0]) {
            urls[url_count++] = url;
            if (url_count >= max_urls) {
                break;
            }
        }
    }

    return url_count;
}

static int duer_payload_is_transcript_type(const char *type)
{
    if (!type || !type[0]) {
        return 0;
    }

    return !strcmp(type, "mid_result") ||
           !strcmp(type, "fin_result") ||
           strstr(type, "asr") != NULL;
}

static int duer_payload_is_control_type(const char *type)
{
    if (!type || !type[0]) {
        return 0;
    }

    return !strcmp(type, "ready") ||
           !strcmp(type, "dcs_decide") ||
           !strcmp(type, "heartbeat") ||
           strstr(type, "control") != NULL;
}

static const char *duer_find_payload_text(cJSON *data, int transcript_only, int *out_is_transcript)
{
    cJSON *items;
    int count;
    int i;

    if (out_is_transcript) {
        *out_is_transcript = 0;
    }

    items = cJSON_GetObjectItem(data, "data");
    if (!cJSON_IsArray(items)) {
        return NULL;
    }

    count = cJSON_GetArraySize(items);
    for (i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(items, i);
        cJSON *payload = cJSON_GetObjectItem(item, "payload");
        const char *type = duer_get_nested_string(payload, "type");
        const char *value;
        int is_transcript = duer_payload_is_transcript_type(type);

        if (duer_payload_is_control_type(type)) {
            continue;
        }
        if (transcript_only && !is_transcript) {
            continue;
        }
        if (!transcript_only && is_transcript) {
            continue;
        }

        value = duer_get_nested_string(payload, "tts");
        if (value && value[0]) {
            if (out_is_transcript) {
                *out_is_transcript = is_transcript;
            }
            return value;
        }
        value = duer_get_nested_string(payload, "part");
        if (value && value[0]) {
            if (out_is_transcript) {
                *out_is_transcript = is_transcript;
            }
            return value;
        }
        value = duer_get_nested_string(payload, "content");
        if (value && value[0]) {
            if (out_is_transcript) {
                *out_is_transcript = is_transcript;
            }
            return value;
        }
        value = duer_get_nested_string(payload, "text");
        if (value && value[0]) {
            if (out_is_transcript) {
                *out_is_transcript = is_transcript;
            }
            return value;
        }
        value = duer_get_nested_string(payload, "answer");
        if (value && value[0]) {
            if (out_is_transcript) {
                *out_is_transcript = is_transcript;
            }
            return value;
        }
    }

    return NULL;
}

static int duer_copy_assistant_answer(cJSON *data, char **out)
{
    cJSON *assistant_answer;
    const char *value;
    cJSON *answer_root;
    cJSON *content;

    if (!data || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    assistant_answer = cJSON_GetObjectItem(data, "assistant_answer");
    if (!assistant_answer) {
        return AI_OK;
    }

    if (cJSON_IsString(assistant_answer)) {
        value = assistant_answer->valuestring;
        answer_root = cJSON_Parse(value);
        if (answer_root) {
            content = cJSON_GetObjectItem(answer_root, "content");
            if (cJSON_IsString(content)) {
                *out = ai_strdup(content->valuestring);
            }
            cJSON_Delete(answer_root);
            return *out ? AI_OK : AI_ERR_NOMEM;
        }

        *out = ai_strdup(value);
        return *out ? AI_OK : AI_ERR_NOMEM;
    }

    if (cJSON_IsObject(assistant_answer)) {
        content = cJSON_GetObjectItem(assistant_answer, "content");
        if (cJSON_IsString(content)) {
            *out = ai_strdup(content->valuestring);
            return *out ? AI_OK : AI_ERR_NOMEM;
        }
    }

    return AI_OK;
}

static int duer_copy_event_text(struct ai_dialogue_event *out_event,
                                unsigned char **owned_data,
                                const char *text)
{
    if (!text) {
        return AI_OK;
    }

    *owned_data = (unsigned char *)ai_strdup_large(text);
    if (!*owned_data) {
        return AI_ERR_NOMEM;
    }

    out_event->text = (const char *)*owned_data;
    return AI_OK;
}

static int duer_copy_event_text_and_urls(struct ai_dialogue_event *out_event,
        unsigned char **owned_data,
        const char *text,
        const char *const *urls,
        unsigned int url_count)
{
    unsigned int text_len = text ? strlen(text) : 0;
    size_t total_size = url_count * sizeof(char *);
    unsigned char *buf;
    const char **event_urls;
    size_t offset;
    unsigned int i;

    if (!text_len && !url_count) {
        return AI_OK;
    }

    if (text_len) {
        total_size += text_len + 1;
    }
    for (i = 0; i < url_count; i++) {
        total_size += strlen(urls[i]) + 1;
    }

    buf = (unsigned char *)ai_malloc_large(total_size);
    if (!buf) {
        return AI_ERR_NOMEM;
    }

    event_urls = (const char **)buf;
    offset = url_count * sizeof(*event_urls);
    if (text_len) {
        memcpy(buf + offset, text, text_len + 1);
        out_event->text = (const char *)(buf + offset);
        offset += text_len + 1;
    }
    for (i = 0; i < url_count; i++) {
        size_t url_len = strlen(urls[i]) + 1;

        memcpy(buf + offset, urls[i], url_len);
        event_urls[i] = (const char *)(buf + offset);
        offset += url_len;
    }

    if (url_count) {
        out_event->url = event_urls[0];
        out_event->urls = event_urls;
        out_event->url_count = url_count;
    }
    *owned_data = buf;
    return AI_OK;
}

/* ready 只表示服务端允许继续发音频，不能当成真实回复完成。 */
static int duer_is_ready_event(const struct ai_dialogue_event *event)
{
    return event && event->raw_json && strstr(event->raw_json, "\"type\":\"ready\"");
}

static int duer_is_upload_end_event(const struct ai_dialogue_event *event)
{
    return event && event->raw_json && strstr(event->raw_json, "\"type\":\"fin_result\"");
}

static int duer_is_session_end_event(const struct ai_dialogue_event *event)
{
    return event && event->raw_json && strstr(event->raw_json, "\"end\":1");
}

static int duer_is_turn_done_event(const struct ai_dialogue_event *event)
{
    if (!event) {
        return 0;
    }

    return event->type == AI_DIALOGUE_AUDIO_URL ||
           event->type == AI_DIALOGUE_TEXT_DONE ||
           event->type == AI_DIALOGUE_DONE;
}

/* 把 Duer 各种 inside_rc / transcript / audio_url 结构折叠成统一事件，供 framework 层消费。 */
static int duer_parse_ws_message(struct ai_client *client,
                                 const unsigned char *data,
                                 unsigned int len,
                                 int binary,
                                 struct ai_dialogue_event *out_event,
                                 unsigned char **owned_data)
{
    cJSON *root;
    cJSON *data_obj;
    cJSON *status;
    cJSON *code;
    cJSON *is_end;
    const char *json;
    const char *type;
    const char *text;
    const char *audio_urls[DUER_MAX_AUDIO_URLS];
    unsigned int audio_url_count;
    char *assistant_text = NULL;
    int is_transcript = 0;
    int ret;

    (void)client;

    if ((!data && len) || !out_event || !owned_data) {
        return AI_ERR_PARAM;
    }

    *owned_data = NULL;
    memset(out_event, 0, sizeof(*out_event));

    if (binary) {
        out_event->type = AI_DIALOGUE_AUDIO_DELTA;
        out_event->data = data;
        out_event->data_len = len;
        return AI_OK;
    }

#if AI_PROVIDER_DUER_LOG_FULL_JSON
    printf("[duer] ws json len=%u\n%.*s\n",
           len, (int)len, data ? (const char *)data : "");
#endif

    json = (const char *)data;
    out_event->raw_json = json;
    root = cJSON_Parse(json);
    if (!root) {
        out_event->type = AI_DIALOGUE_TEXT_DELTA;
        out_event->text = json;
        return AI_OK;
    }

    type = duer_get_string(root, "type");
    status = cJSON_GetObjectItem(root, "status");
    if (cJSON_IsString(status) && strcmp(status->valuestring, "ok") != 0) {
        out_event->type = AI_DIALOGUE_ERROR;
        out_event->error_code = AI_ERR_PARSE;
        ret = duer_copy_event_text(out_event, owned_data, status->valuestring);
        cJSON_Delete(root);
        return ret;
    }

    if (type && !strcmp(type, "ready")) {
        out_event->type = AI_DIALOGUE_TEXT_DELTA;
        ret = duer_copy_event_text(out_event, owned_data, json);
        cJSON_Delete(root);
        return ret;
    }

    if (type && duer_payload_is_transcript_type(type)) {
        text = duer_get_string(root, "result");
        if (!text || !text[0]) {
            text = duer_get_string(root, "rawResult");
        }
        out_event->type = AI_DIALOGUE_TRANSCRIPT;
        ret = duer_copy_event_text(out_event, owned_data, text ? text : json);
        cJSON_Delete(root);
        return ret;
    }

    data_obj = cJSON_GetObjectItem(root, "data");
    code = cJSON_GetObjectItem(data_obj, "code");
    if (cJSON_IsNumber(code) && code->valueint != 0) {
        const char *msg = duer_get_nested_string(data_obj, "msg");
        out_event->type = AI_DIALOGUE_ERROR;
        out_event->error_code = code->valueint;
        ret = duer_copy_event_text(out_event, owned_data, msg ? msg : "duer error");
        cJSON_Delete(root);
        return ret;
    }

    if (!cJSON_IsObject(data_obj)) {
        if (type && duer_payload_is_control_type(type)) {
            out_event->type = AI_DIALOGUE_TEXT_DELTA;
            ret = duer_copy_event_text(out_event, owned_data, json);
            cJSON_Delete(root);
            return ret;
        }
        out_event->type = AI_DIALOGUE_TEXT_DELTA;
        ret = duer_copy_event_text(out_event, owned_data, json);
        cJSON_Delete(root);
        return ret;
    }

    text = duer_find_payload_text(data_obj, 0, &is_transcript);
    audio_url_count = duer_collect_payload_audio_urls(data_obj, audio_urls, DUER_MAX_AUDIO_URLS);
    if (audio_url_count) {
        out_event->type = AI_DIALOGUE_AUDIO_URL;
        ret = duer_copy_event_text_and_urls(out_event, owned_data, text,
                                            audio_urls, audio_url_count);
        cJSON_Delete(root);
        return ret;
    }

    is_end = cJSON_GetObjectItem(data_obj, "is_end");
    if (cJSON_IsNumber(is_end) && is_end->valueint == 1) {
        ret = duer_copy_assistant_answer(data_obj, &assistant_text);
        if (ret != AI_OK) {
            cJSON_Delete(root);
            return ret;
        }
        out_event->type = assistant_text ? AI_DIALOGUE_TEXT_DONE : AI_DIALOGUE_DONE;
        ret = duer_copy_event_text(out_event, owned_data, assistant_text);
        free(assistant_text);
        cJSON_Delete(root);
        return ret;
    }

    text = duer_find_payload_text(data_obj, 1, &is_transcript);
    if (text) {
        out_event->type = AI_DIALOGUE_TRANSCRIPT;
        ret = duer_copy_event_text(out_event, owned_data, text);
        cJSON_Delete(root);
        return ret;
    }

    text = duer_find_payload_text(data_obj, 0, &is_transcript);
    if (!text) {
        ret = duer_copy_assistant_answer(data_obj, &assistant_text);
        if (ret != AI_OK) {
            cJSON_Delete(root);
            return ret;
        }
        text = assistant_text;
    }

    out_event->type = text ? AI_DIALOGUE_TEXT_DELTA : AI_DIALOGUE_DONE;
    ret = duer_copy_event_text(out_event, owned_data, text);
    free(assistant_text);
    cJSON_Delete(root);
    return ret;
}

const struct ai_provider_ops ai_provider_duer = {
    .name = "duer",
    .caps = AI_PROVIDER_CAP_VOICE_DIALOGUE,
    .default_dialogue_ws_url = DUER_WS_URL,
    .default_provider_params = duer_provider_params,
    .voice = &duer_voice_profile,
    .is_ready_event = duer_is_ready_event,
    .is_upload_end_event = duer_is_upload_end_event,
    .is_session_end_event = duer_is_session_end_event,
    .is_turn_done_event = duer_is_turn_done_event,
    .build_session_start = duer_build_session_start,
    .build_text = duer_build_text,
    .build_audio = duer_build_audio,
    .build_commit = duer_build_finish,
    .build_session_stop = duer_build_finish,
    .parse_ws_message = duer_parse_ws_message,
};
