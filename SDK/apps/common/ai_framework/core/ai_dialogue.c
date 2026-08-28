#include "ai_internal.h"
#include "sdk_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *malloc_psram(size_t size);
void *realloc_psram(void *pv, size_t size);
void free_psram(void *pv);

/* 大块 AI 数据优先放 PSRAM：图片/base64/WS 大帧。小字符串仍走普通堆。 */
void *ai_malloc_large(size_t size)
{
#if TCFG_PSRAM_DEV_ENABLE
    return malloc_psram(size);
#else
    return malloc(size);
#endif
}

void *ai_realloc_large(void *ptr, size_t size)
{
#if TCFG_PSRAM_DEV_ENABLE
    return realloc_psram(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

void ai_free_large(void *ptr)
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

/* 底层上下文管理、WebSocket 发送队列和接收拼包；厂家协议字段放在 providers/。 */
char *ai_strdup(const char *s)
{
    size_t len;
    char *copy;

    if (!s) {
        return NULL;
    }

    len = strlen(s);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

char *ai_strndup(const char *s, size_t len)
{
    char *copy;

    if (!s) {
        return NULL;
    }

    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

/* Large/PSRAM-heap strdup. Provider parsers must use this (not ai_strdup) for
 * any string handed back through owned_data, since the framework frees
 * owned_data with ai_free_large(). See parse_ws_message contract in ai_internal.h. */
char *ai_strdup_large(const char *s)
{
    size_t len;
    char *copy;

    if (!s) {
        return NULL;
    }

    len = strlen(s);
    copy = (char *)ai_malloc_large(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

void ai_free_string(char **s)
{
    if (s && *s) {
        free(*s);
        *s = NULL;
    }
}

static int ai_copy_error_field(char *dst, size_t dst_len, const char *src)
{
    size_t len;

    if (!dst || !dst_len) {
        return 0;
    }
    if (!src) {
        dst[0] = '\0';
        return 0;
    }
    len = strlen(src);
    if (len >= dst_len) {
        len = dst_len - 1;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return src[len] != '\0';
}

static int ai_copy_config_string(char **dst, const char *src)
{
    *dst = NULL;
    if (!src) {
        return AI_OK;
    }

    *dst = ai_strdup(src);
    if (!*dst) {
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

void ai_ws_message_free(struct ai_ws_message *msg)
{
    if (!msg) {
        return;
    }

    ai_free_large(msg->data);
    msg->data = NULL;
    msg->len = 0;
    msg->binary = 0;
}

int ai_ws_message_set_text(struct ai_ws_message *msg, const char *text)
{
    size_t len;

    if (!msg || !text) {
        return AI_ERR_PARAM;
    }

    memset(msg, 0, sizeof(*msg));
    len = strlen(text);
    msg->data = (unsigned char *)ai_malloc_large(len + 1);
    if (!msg->data) {
        return AI_ERR_NOMEM;
    }

    memcpy(msg->data, text, len + 1);
    msg->len = (unsigned int)len;
    msg->binary = 0;
    return AI_OK;
}

int ai_ws_message_set_binary(struct ai_ws_message *msg, const unsigned char *data, unsigned int len)
{
    if (!msg || (!data && len)) {
        return AI_ERR_PARAM;
    }

    memset(msg, 0, sizeof(*msg));
    if (!len) {
        return AI_OK;
    }

    msg->data = (unsigned char *)ai_malloc_large(len);
    if (!msg->data) {
        return AI_ERR_NOMEM;
    }

    memcpy(msg->data, data, len);
    msg->len = len;
    msg->binary = 1;
    return AI_OK;
}

static void ai_ws_queue_lock(struct ai_client *client)
{
    if (client && client->ws_queue_mutex_initialized) {
        os_mutex_pend(&client->ws_queue_mutex, 0);
    }
}

static void ai_ws_queue_unlock(struct ai_client *client)
{
    if (client && client->ws_queue_mutex_initialized) {
        os_mutex_post(&client->ws_queue_mutex);
    }
}

/* 把待发消息所有权转移进 client 队列；成功后调用方不再负责释放 msg->data。 */
int ai_ws_queue_push(struct ai_client *client, struct ai_ws_message *msg)
{
    if (!client || !msg || !msg->data) {
        return AI_ERR_PARAM;
    }

    ai_ws_queue_lock(client);
    if (client->ws_queue_count >= AI_WS_QUEUE_DEPTH) {
        ai_ws_queue_unlock(client);
        return AI_ERR_STATE;
    }

    client->ws_queue[client->ws_queue_tail] = *msg;
    memset(msg, 0, sizeof(*msg));
    client->ws_queue_tail = (client->ws_queue_tail + 1) % AI_WS_QUEUE_DEPTH;
    client->ws_queue_count++;
    ai_ws_queue_unlock(client);

    return AI_OK;
}

int ai_ws_queue_push_optional(struct ai_client *client, struct ai_ws_message *msg)
{
    if (!client || !msg) {
        return AI_ERR_PARAM;
    }
    if (!msg->data) {
        return AI_OK;
    }

    return ai_ws_queue_push(client, msg);
}

struct ai_ws_message ai_ws_queue_pop(struct ai_client *client)
{
    struct ai_ws_message msg;

    memset(&msg, 0, sizeof(msg));
    if (!client) {
        return msg;
    }

    ai_ws_queue_lock(client);
    if (client->ws_queue_count <= 0) {
        ai_ws_queue_unlock(client);
        return msg;
    }

    msg = client->ws_queue[client->ws_queue_head];
    memset(&client->ws_queue[client->ws_queue_head], 0, sizeof(client->ws_queue[client->ws_queue_head]));
    client->ws_queue_head = (client->ws_queue_head + 1) % AI_WS_QUEUE_DEPTH;
    client->ws_queue_count--;
    ai_ws_queue_unlock(client);

    return msg;
}

void ai_ws_queue_clear(struct ai_client *client)
{
    if (!client) {
        return;
    }

    ai_ws_queue_lock(client);
    while (client->ws_queue_count > 0) {
        struct ai_ws_message *msg = &client->ws_queue[client->ws_queue_head];

        ai_ws_message_free(msg);
        client->ws_queue_head = (client->ws_queue_head + 1) % AI_WS_QUEUE_DEPTH;
        client->ws_queue_count--;
    }

    client->ws_queue_head = 0;
    client->ws_queue_tail = 0;
    client->ws_queue_count = 0;
    ai_ws_queue_unlock(client);
}

int ai_ws_queue_pending(struct ai_client *client)
{
    int pending;

    if (!client) {
        return 0;
    }

    ai_ws_queue_lock(client);
    pending = client->ws_queue_count;
    ai_ws_queue_unlock(client);

    return pending;
}

/* 把单次 LWS_CALLBACK_CLIENT_RECEIVE 的片段追加到装配 buffer，等收到 final fragment 再统一分发。 */
int ai_ws_rx_append(struct ai_client *client, const void *data, unsigned int len, int binary)
{
    unsigned int need;
    unsigned int new_cap;
    unsigned char *next;

    if (!client) {
        return AI_ERR_PARAM;
    }
    if (!len) {
        client->ws_rx_binary = binary;
        return AI_OK;
    }
    if (!data) {
        return AI_ERR_PARAM;
    }

    /* 第一片决定整帧是 text 还是 binary。 */
    if (client->ws_rx_len == 0) {
        client->ws_rx_binary = binary;
    }

    /* Cap the reassembly buffer so an oversized/never-final frame stream cannot
     * grow it without bound. Over the limit we fail the append; the caller
     * (CLIENT_RECEIVE) turns this into an ERROR + disconnect. */
    if (len >= AI_WS_RX_MAX_BYTES ||
        client->ws_rx_len > AI_WS_RX_MAX_BYTES - len - 1) {
        printf("[ai_ws] rx frame too large: have=%u add=%u cap=%u\n",
               client->ws_rx_len, len, (unsigned int)AI_WS_RX_MAX_BYTES);
        return AI_ERR_NOMEM;
    }
    need = client->ws_rx_len + len + 1;
    if (need > client->ws_rx_cap) {
        new_cap = client->ws_rx_cap ? client->ws_rx_cap : 1024;
        while (new_cap < need) {
            new_cap *= 2;
        }
        if (new_cap > AI_WS_RX_MAX_BYTES) {
            new_cap = AI_WS_RX_MAX_BYTES;
        }
        next = (unsigned char *)ai_realloc_large(client->ws_rx_buf, new_cap);
        if (!next) {
            return AI_ERR_NOMEM;
        }
        client->ws_rx_buf = next;
        client->ws_rx_cap = new_cap;
    }

    memcpy(client->ws_rx_buf + client->ws_rx_len, data, len);
    client->ws_rx_len += len;
    client->ws_rx_buf[client->ws_rx_len] = '\0';
    return AI_OK;
}

void ai_ws_rx_reset(struct ai_client *client)
{
    if (!client) {
        return;
    }
    client->ws_rx_len = 0;
    client->ws_rx_binary = 0;
    if (client->ws_rx_buf && client->ws_rx_cap > 0) {
        client->ws_rx_buf[0] = '\0';
    }
}

void ai_ws_rx_release(struct ai_client *client)
{
    if (!client) {
        return;
    }
    ai_free_large(client->ws_rx_buf);
    client->ws_rx_buf = NULL;
    client->ws_rx_len = 0;
    client->ws_rx_cap = 0;
    client->ws_rx_binary = 0;
}

void ai_client_clear_error(struct ai_client *client)
{
    if (client) {
        memset(&client->last_error, 0, sizeof(client->last_error));
    }
}

void ai_client_set_error(struct ai_client *client,
                         enum ai_error_stage stage,
                         int error_code,
                         int transport_code,
                         const char *message,
                         const char *raw_response)
{
    if (!client) {
        return;
    }

    ai_client_clear_error(client);
    client->last_error.stage = stage;
    client->last_error.error_code = error_code;
    client->last_error.transport_code = transport_code;
    ai_copy_error_field(client->last_error.provider, sizeof(client->last_error.provider), client->provider_name);
    ai_copy_error_field(client->last_error.message, sizeof(client->last_error.message), message);
    client->last_error.raw_truncated = ai_copy_error_field(client->last_error.raw_response,
                                       sizeof(client->last_error.raw_response),
                                       raw_response);
}

int ai_client_get_last_error(const struct ai_client *client, struct ai_error_detail *out)
{
    if (!client || !out) {
        return AI_ERR_PARAM;
    }
    *out = client->last_error;
    return AI_OK;
}

void ai_dialogue_emit_error(struct ai_client *client,
                            enum ai_error_stage stage,
                            int error_code,
                            int transport_code,
                            const char *message,
                            const char *raw_response)
{
    ai_client_set_error(client, stage, error_code, transport_code, message, raw_response);
    ai_dialogue_emit(client, AI_DIALOGUE_ERROR, message, NULL, 0, raw_response, error_code);
}

int ai_client_set_dialogue_callback(struct ai_client *client, ai_dialogue_cb cb, void *user)
{
    if (!client) {
        return AI_ERR_PARAM;
    }

    client->dialogue_cb = cb;
    client->dialogue_user = user;
    return AI_OK;
}

/* 统一把 framework 内部事件包装成 ai_dialogue_event，再转发给应用层回调。 */
void ai_dialogue_emit(struct ai_client *client,
                      enum ai_dialogue_event_type type,
                      const char *text,
                      const unsigned char *data,
                      unsigned int data_len,
                      const char *raw_json,
                      int error_code)
{
    struct ai_dialogue_event event;

    if (!client || !client->dialogue_cb) {
        return;
    }

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.text = text;
    event.url = NULL;
    event.data = data;
    event.data_len = data_len;
    event.raw_json = raw_json;
    event.error_code = error_code;
    if ((type == AI_DIALOGUE_ERROR || type == AI_DIALOGUE_CLOSED) &&
        client->last_error.stage != AI_ERROR_STAGE_NONE) {
        event.error = &client->last_error;
    }
    client->dialogue_cb(&event, client->dialogue_user);
}

static void ai_client_free(struct ai_client *client)
{
    if (!client) {
        return;
    }

    ai_ws_queue_clear(client);
    if (client->ws_queue_mutex_initialized) {
        os_mutex_del(&client->ws_queue_mutex, OS_DEL_ALWAYS);
        client->ws_queue_mutex_initialized = 0;
    }
    if (client->ws_ctx_mutex_initialized) {
        os_mutex_del(&client->ws_ctx_mutex, OS_DEL_ALWAYS);
        client->ws_ctx_mutex_initialized = 0;
    }
    ai_ws_rx_release(client);
    ai_free_string(&client->provider_name);
    ai_free_string(&client->dialogue_ws_url);
    ai_free_string(&client->vision_http_url);
    ai_free_string(&client->api_key);
    ai_free_string(&client->model);
    ai_free_string(&client->provider_params);
    free(client);
}

/* 创建 client 时会把配置里的字符串全部复制一份，避免后续依赖外部栈变量。 */
int ai_client_create(const struct ai_config *cfg, struct ai_client **out)
{
    struct ai_client *client;
    const char *provider_name;
    const char *validation_error = NULL;
    int ret;

    if (!cfg || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    provider_name = (cfg->provider_name && cfg->provider_name[0]) ? cfg->provider_name : NULL;

    client = (struct ai_client *)calloc(1, sizeof(*client));
    if (!client) {
        return AI_ERR_NOMEM;
    }

    if (os_mutex_create(&client->ws_queue_mutex)) {
        free(client);
        return AI_ERR_NOMEM;
    }
    client->ws_queue_mutex_initialized = 1;

    if (os_mutex_create(&client->ws_ctx_mutex)) {
        os_mutex_del(&client->ws_queue_mutex, OS_DEL_ALWAYS);
        client->ws_queue_mutex_initialized = 0;
        free(client);
        return AI_ERR_NOMEM;
    }
    client->ws_ctx_mutex_initialized = 1;

    client->provider = ai_provider_find(provider_name);
    if (!client->provider) {
        ai_client_free(client);
        return AI_ERR_PARAM;
    }
    ret = ai_provider_validate(client->provider, &validation_error);
    if (ret != AI_OK) {
        printf("[ai_provider] invalid %s: %s\n",
               client->provider->name ? client->provider->name : "",
               validation_error ? validation_error : "unknown");
        ai_client_free(client);
        return ret;
    }

    provider_name = client->provider->name;
    ret = ai_copy_config_string(&client->provider_name, provider_name);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    ret = ai_copy_config_string(&client->dialogue_ws_url,
                                cfg->dialogue_ws_url ? cfg->dialogue_ws_url : client->provider->default_dialogue_ws_url);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    ret = ai_copy_config_string(&client->vision_http_url,
                                cfg->vision_http_url ? cfg->vision_http_url : client->provider->default_vision_http_url);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    ret = ai_copy_config_string(&client->api_key, cfg->api_key);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    ret = ai_copy_config_string(&client->model,
                                cfg->model ? cfg->model : client->provider->default_model);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    ret = ai_copy_config_string(&client->provider_params,
                                cfg->provider_params ? cfg->provider_params : client->provider->default_provider_params);
    if (ret != AI_OK) {
        ai_client_free(client);
        return ret;
    }

    if (cfg->turn_detection) {
        client->turn_detection = *cfg->turn_detection;
        client->has_turn_detection = 1;
    }

    client->timeout_ms = cfg->timeout_ms;
    client->ping_interval_ms = cfg->ping_interval_ms;
    client->auto_reconnect = cfg->auto_reconnect;
    client->reconnect_max_attempts = cfg->reconnect_max_attempts > 0 ?
                                     cfg->reconnect_max_attempts : 5;
    client->http_backend = cfg->http_backend;
    client->ws_state = AI_WS_DISCONNECTED;
    *out = client;

    return AI_OK;
}

void ai_client_destroy(struct ai_client *client)
{
    if (!client) {
        return;
    }

    ai_dialogue_close(client);
    ai_client_free(client);
}

void ai_vision_result_free(struct ai_vision_result *result)
{
    if (!result) {
        return;
    }

    ai_free_string(&result->text);
    ai_free_string(&result->raw_json);
}
