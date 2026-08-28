#include "ai_internal.h"

#include "libwebsockets.h"
#include <mbedtls/certs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AI_WS_RECONNECT_MIN_MS  1000u
#define AI_WS_RECONNECT_MAX_MS  30000u
#define AI_WS_RECONNECT_STABLE_MS 60000u

/* 本文件只处理 WebSocket 传输：URL 解析、lws 生命周期、发送队列、分片收包和重连调度。 */
extern u32 timer_get_ms(void);

static unsigned int ai_ws_binary_write_count;
static unsigned int ai_ws_binary_write_bytes;
static unsigned int ai_ws_binary_write_start_ms;

/* Provider sessions own their keepalive policy; do not apply lws' fixed 40/50s validity timeout. */
static const lws_retry_bo_t ai_ws_no_idle_timeout = {0};

static int ai_lws_callback(struct lws *wsi,
                           enum lws_callback_reasons reason,
                           void *user,
                           void *in,
                           size_t len);

static int ai_dialogue_connect_locked(struct ai_client *client);

/* 指数退避且次数封顶；连接稳定 60s 后才视为一次有效恢复并清零计数。 */
static int ai_dialogue_schedule_reconnect(struct ai_client *client)
{
    unsigned int delay;
    unsigned int now;

    if (!client || !client->auto_reconnect || client->disconnect_requested) {
        return 0;
    }
    if (!client->dialogue_ws_url) {
        return 0;
    }
    if (client->reconnect_need) {
        return 1;
    }

    now = (unsigned int)timer_get_ms();
    if (client->reconnect_connected_ms &&
        (now - client->reconnect_connected_ms) >= AI_WS_RECONNECT_STABLE_MS) {
        client->reconnect_attempts = 0;
        client->reconnect_backoff_ms = 0;
    }
    client->reconnect_connected_ms = 0;
    if (client->reconnect_attempts >= (unsigned int)client->reconnect_max_attempts) {
        printf("[ai_ws] reconnect exhausted: provider=%s attempts=%u\n",
               client->provider_name ? client->provider_name : "",
               client->reconnect_attempts);
        return 0;
    }

    delay = client->reconnect_backoff_ms ? client->reconnect_backoff_ms * 2 : AI_WS_RECONNECT_MIN_MS;
    if (delay > AI_WS_RECONNECT_MAX_MS) {
        delay = AI_WS_RECONNECT_MAX_MS;
    }
    client->reconnect_backoff_ms = delay;
    client->reconnect_after_ms = now + delay;
    client->reconnect_need = 1;
    return 1;
}

static const struct lws_protocols ai_ws_protocols[] = {
    { "ai-dialogue", ai_lws_callback, 0, 4096, 0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

/* URL 解析后的 host/path 都是动态复制的，统一从这里释放。 */
static void ai_ws_url_free(struct ai_ws_url *url)
{
    if (!url) {
        return;
    }

    ai_free_string(&url->host);
    ai_free_string(&url->path);
    url->port = 0;
    url->use_ssl = 0;
}

/* 只支持 ws/wss 完整 URL，解析结果给 libwebsockets 连接参数直接复用。 */
static int ai_ws_parse_url(const char *url, struct ai_ws_url *out)
{
    const char *p;
    const char *path;
    const char *colon;
    size_t host_len;

    if (!url || !out) {
        return AI_ERR_PARAM;
    }

    memset(out, 0, sizeof(*out));

    if (strncmp(url, "wss://", 6) == 0) {
        out->use_ssl = 1;
        out->port = 443;
        p = url + 6;
    } else if (strncmp(url, "ws://", 5) == 0) {
        out->use_ssl = 0;
        out->port = 80;
        p = url + 5;
    } else {
        return AI_ERR_PARAM;
    }

    path = strchr(p, '/');
    if (!path) {
        path = p + strlen(p);
    }

    host_len = (size_t)(path - p);
    if (!host_len) {
        return AI_ERR_PARAM;
    }

    colon = memchr(p, ':', host_len);
    if (colon) {
        size_t port_len = host_len - (size_t)(colon - p) - 1;
        char *port_str;

        if (!port_len) {
            return AI_ERR_PARAM;
        }

        size_t i;

        for (i = 0; i < port_len; i++) {
            if (colon[1 + i] < '0' || colon[1 + i] > '9') {
                return AI_ERR_PARAM;
            }
        }

        port_str = ai_strndup(colon + 1, port_len);
        if (!port_str) {
            return AI_ERR_NOMEM;
        }
        out->port = atoi(port_str);
        free(port_str);
        if (out->port <= 0 || out->port > 65535) {
            return AI_ERR_PARAM;
        }
        host_len = (size_t)(colon - p);
        if (!host_len) {
            return AI_ERR_PARAM;
        }
    }

    out->host = ai_strndup(p, host_len);
    if (!out->host) {
        ai_ws_url_free(out);
        return AI_ERR_NOMEM;
    }

    out->path = *path ? ai_strdup(path) : ai_strdup("/");
    if (!out->path) {
        ai_ws_url_free(out);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static struct ai_client *ai_ws_client_from_callback(struct lws *wsi, void *user)
{
    struct ai_client *client = (struct ai_client *)user;

    if (!client && wsi) {
        client = (struct ai_client *)lws_get_opaque_user_data(wsi);
    }
    if (!client && wsi) {
        client = (struct ai_client *)lws_context_user(lws_get_context(wsi));
    }

    return client;
}

int ai_ws_append_bearer_auth(struct ai_client *client,
                             struct lws *wsi,
                             unsigned char **p,
                             unsigned char *end)
{
    char *value;
    size_t value_len;
    int ret;

    if (!client || !wsi || !p || !end || !client->api_key || !client->api_key[0]) {
        return AI_OK;
    }

    value_len = strlen("Bearer ") + strlen(client->api_key);
    value = (char *)malloc(value_len + 1);
    if (!value) {
        return AI_ERR_NOMEM;
    }

    strcpy(value, "Bearer ");
    strcat(value, client->api_key);
    ret = lws_add_http_header_by_name(wsi,
                                      (const unsigned char *)"authorization:",
                                      (const unsigned char *)value,
                                      (int)value_len,
                                      p,
                                      end);
    free(value);

    return ret ? AI_ERR_NET : AI_OK;
}

static int ai_ws_append_headers(struct ai_client *client, struct lws *wsi, void *in, size_t len)
{
    unsigned char **p = (unsigned char **)in;
    unsigned char *end;

    if (!client || !wsi || !in) {
        return AI_ERR_PARAM;
    }
    if (!client->provider || !client->provider->append_ws_headers) {
        return AI_OK;
    }

    end = *p + len;
    return client->provider->append_ws_headers(client, wsi, p, end);
}

/* 从队列里取一条待发消息并写出；写出后立即释放，发送失败由上层把 ws_state 标 ERROR。 */
static int ai_ws_send_one(struct ai_client *client, struct lws *wsi)
{
    struct ai_ws_message msg;
    unsigned char *buf;
    unsigned int len;
    int write_type;
    int written;
    int is_binary;

    msg = ai_ws_queue_pop(client);
    if (!msg.data) {
        return AI_OK;
    }

    len = msg.len;
    is_binary = msg.binary;
    write_type = msg.binary ? LWS_WRITE_BINARY : LWS_WRITE_TEXT;
    buf = (unsigned char *)ai_malloc_large(LWS_PRE + len);
    if (!buf) {
        ai_ws_message_free(&msg);
        return AI_ERR_NOMEM;
    }

    memcpy(buf + LWS_PRE, msg.data, len);
    written = lws_write(wsi, buf + LWS_PRE, len, write_type);

    ai_free_large(buf);
    ai_ws_message_free(&msg);

    if (written < 0 || (unsigned int)written < len) {
        return AI_ERR_NET;
    }

    if (is_binary) {
        if (!ai_ws_binary_write_count) {
            ai_ws_binary_write_start_ms = timer_get_ms();
            printf("[ai_ws] first binary write at %u ms len=%u\n", ai_ws_binary_write_start_ms, len);
        }
        ai_ws_binary_write_count++;
        ai_ws_binary_write_bytes += len;
        if ((ai_ws_binary_write_count % 50) == 0) {
            printf("[ai_ws] binary writes=%u bytes=%u duration=%u ms queued=%d\n",
                   ai_ws_binary_write_count,
                   ai_ws_binary_write_bytes,
                   timer_get_ms() - ai_ws_binary_write_start_ms,
                   ai_ws_queue_pending(client));
        }
    }

    return AI_OK;
}

/* WebSocket收包后先交给provider解析，再统一回灌到framework事件回调。 */
static void ai_ws_emit_parsed_message(struct ai_client *client,
                                      const unsigned char *data,
                                      unsigned int len,
                                      int binary)
{
    static const unsigned char empty_text[] = "";
    struct ai_dialogue_event event;
    unsigned char *owned_data = NULL;
    const unsigned char *parse_data = data ? data : empty_text;
    int ret;

    if (!client || (!data && len)) {
        return;
    }

    if (!client->provider || !client->provider->parse_ws_message) {
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_PARSE, AI_ERR_PARSE, 0,
                               "websocket parser missing", NULL);
        return;
    }

    ret = client->provider->parse_ws_message(client, parse_data, len, binary, &event, &owned_data);
    if (ret == AI_OK) {
        if (event.type == AI_DIALOGUE_ERROR) {
            if (!event.error_code) {
                event.error_code = AI_ERR_PARSE;
            }
            ai_client_set_error(client, AI_ERROR_STAGE_PROVIDER,
                                event.error_code ? event.error_code : AI_ERR_PARSE,
                                event.error_code, event.text, event.raw_json);
            event.error = &client->last_error;
        }
        if (client->dialogue_cb) {
            client->dialogue_cb(&event, client->dialogue_user);
        }
    } else {
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_PARSE, ret, ret,
                               "websocket parse failed",
                               binary ? NULL : (const char *)parse_data);
    }

    /* owned_data is always allocated from the large heap by the provider
     * (ai_malloc_large / ai_strdup_large / ai_base64_decode), so it is always
     * released the same way regardless of event type. */
    if (owned_data) {
        ai_free_large(owned_data);
    }
}

static void ai_ws_log_peer_close(struct ai_client *client, const void *in, size_t len)
{
    const unsigned char *payload = (const unsigned char *)in;
    unsigned int code = 0;

    if (payload && len >= 2) {
        code = ((unsigned int)payload[0] << 8) | payload[1];
    }

    if (payload && len > 2) {
        printf("[ai_ws] peer close: provider=%s state=%d code=%u reason_len=%u reason=%.*s\n",
               client->provider_name ? client->provider_name : "",
               client->ws_state,
               code,
               (unsigned int)(len - 2),
               (int)(len - 2),
               (const char *)payload + 2);
    } else {
        printf("[ai_ws] peer close: provider=%s state=%d code=%u payload_len=%u\n",
               client->provider_name ? client->provider_name : "",
               client->ws_state,
               code,
               (unsigned int)len);
    }

    if (payload && len > 2) {
        char *reason = ai_strndup((const char *)payload + 2, len - 2);
        ai_client_set_error(client, AI_ERROR_STAGE_CLOSED,
                            code == 1000 ? AI_OK : AI_ERR_NET,
                            (int)code, reason ? reason : "websocket peer closed", reason);
        free(reason);
    } else {
        ai_client_set_error(client, AI_ERROR_STAGE_CLOSED,
                            code == 1000 ? AI_OK : AI_ERR_NET,
                            (int)code, "websocket peer closed", NULL);
    }
}

static int ai_dialogue_can_queue(struct ai_client *client)
{
    if (!client) {
        return AI_ERR_PARAM;
    }
    if (client->ws_state != AI_WS_CONNECTING && client->ws_state != AI_WS_CONNECTED) {
        return AI_ERR_STATE;
    }

    return AI_OK;
}

static void ai_ws_ctx_lock(struct ai_client *client)
{
    if (client && client->ws_ctx_mutex_initialized) {
        os_mutex_pend(&client->ws_ctx_mutex, 0);
    }
}

static void ai_ws_ctx_unlock(struct ai_client *client)
{
    if (client && client->ws_ctx_mutex_initialized) {
        os_mutex_post(&client->ws_ctx_mutex);
    }
}

/* Detach and destroy the lws context safely against the upload thread's
 * lws_cancel_service(). The pointer is cleared under ws_ctx_mutex, but the
 * actual lws_context_destroy() runs AFTER releasing the lock: destroy invokes
 * CLIENT_CLOSED callbacks (which take ws_queue_mutex / call the app callback),
 * so holding ws_ctx_mutex across it could deadlock. Once the pointer is NULL,
 * the upload thread's guarded cancel path simply skips. */
static void ai_ws_ctx_destroy(struct ai_client *client)
{
    struct lws_context *ctx;

    if (!client) {
        return;
    }

    ai_ws_ctx_lock(client);
    ctx = client->ws_context;
    client->ws_context = NULL;
    client->ws_wsi = NULL;
    ai_ws_ctx_unlock(client);

    if (ctx) {
        lws_context_destroy(ctx);
    }
}

static int ai_dialogue_queue_message(struct ai_client *client, struct ai_ws_message *msg)
{
    int ret;

    ret = ai_ws_queue_push_optional(client, msg);
    if (ret != AI_OK) {
        ai_ws_message_free(msg);
        return ret;
    }

    /* Wake the poll thread's lws_service. Guard the context pointer with
     * ws_ctx_mutex so we never call lws_cancel_service on a context the poll
     * thread is concurrently destroying (reconnect/close) from another thread. */
    ai_ws_ctx_lock(client);
    if (client->ws_context) {
        lws_cancel_service(client->ws_context);
    }
    ai_ws_ctx_unlock(client);

    return AI_OK;
}

static int ai_lws_callback(struct lws *wsi,
                           enum lws_callback_reasons reason,
                           void *user,
                           void *in,
                           size_t len)
{
    struct ai_client *client = ai_ws_client_from_callback(wsi, user);
    int ret;

    if (!client) {
        return 0;
    }

    if (reason == LWS_CALLBACK_EVENT_WAIT_CANCELLED) {
        if (client->ws_state == AI_WS_CONNECTED && client->ws_wsi && ai_ws_queue_pending(client) > 0) {
            lws_callback_on_writable(client->ws_wsi);
        }
        return 0;
    }

    switch (reason) {
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        ret = ai_ws_append_headers(client, wsi, in, len);
        if (ret != AI_OK) {
            client->ws_state = AI_WS_ERROR;
            ai_dialogue_emit_error(client, AI_ERROR_STAGE_AUTH, ret, 0,
                                   "websocket auth header failed", NULL);
            return -1;
        }
        break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        client->ws_wsi = wsi;
        client->ws_state = AI_WS_CONNECTED;
        ai_client_clear_error(client);
        client->reconnect_need = 0;
        client->reconnect_connected_ms = (unsigned int)timer_get_ms();
        client->ping_pending = 0;
        ai_ws_binary_write_count = 0;
        ai_ws_binary_write_bytes = 0;
        ai_ws_binary_write_start_ms = 0;
        ai_ws_rx_reset(client);
        ai_dialogue_emit(client, AI_DIALOGUE_CONNECTED, NULL, NULL, 0, NULL, AI_OK);
        if (client->ping_interval_ms > 0) {
            lws_set_timer_usecs(wsi, (lws_usec_t)client->ping_interval_ms * 1000);
        }
        if (ai_ws_queue_pending(client) > 0) {
            lws_callback_on_writable(wsi);
        }
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        /* Emit a due keepalive PING here (the only place lws allows writes).
         * Send it as its own frame and defer any queued data to the next
         * writable, so a control frame never splits a data frame. */
        if (client->ping_pending) {
            unsigned char ping[LWS_PRE + 4];

            client->ping_pending = 0;
            memset(ping, 0, sizeof(ping));
            memcpy(ping + LWS_PRE, "ping", 4);
            ret = lws_write(wsi, ping + LWS_PRE, 4, LWS_WRITE_PING);
            if (ret < 4) {
                client->ws_state = AI_WS_ERROR;
                ai_dialogue_schedule_reconnect(client);
                ai_dialogue_emit_error(client, AI_ERROR_STAGE_SEND, AI_ERR_NET, 0,
                                       "websocket ping failed", NULL);
                return -1;
            }
            if (ai_ws_queue_pending(client) > 0) {
                lws_callback_on_writable(wsi);
            }
            break;
        }
        ret = ai_ws_send_one(client, wsi);
        if (ret != AI_OK) {
            client->ws_state = AI_WS_ERROR;
            if (ret == AI_ERR_NET) {
                ai_dialogue_schedule_reconnect(client);
            }
            ai_dialogue_emit_error(client, AI_ERROR_STAGE_SEND, ret, 0,
                                   "websocket send failed", NULL);
            return -1;
        }
        if (ai_ws_queue_pending(client) > 0) {
            lws_callback_on_writable(wsi);
        }
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        if (in && len) {
            int binary = lws_frame_is_binary(wsi);
            int append_ret = ai_ws_rx_append(client, in, (unsigned int)len, binary);
            if (append_ret != AI_OK) {
                client->ws_state = AI_WS_ERROR;
                ai_ws_rx_reset(client);
                ai_dialogue_emit_error(client, AI_ERROR_STAGE_RECEIVE, append_ret, 0,
                                       "websocket receive alloc failed", NULL);
                return -1;
            }
        }
        /* 等收完最后一片再交给 provider，避免大消息（>rx_buffer_size）被截断后解析失败。 */
        if (lws_is_final_fragment(wsi) && lws_remaining_packet_payload(wsi) == 0) {
            if (client->ws_rx_len > 0 || in) {
                ai_ws_emit_parsed_message(client,
                                          client->ws_rx_buf,
                                          client->ws_rx_len,
                                          client->ws_rx_binary);
            }
            ai_ws_rx_reset(client);
        }
        break;

    case LWS_CALLBACK_TIMER:
        if (client->ws_state == AI_WS_CONNECTED && client->ping_interval_ms > 0) {
            /* Do not write here: defer the PING to the WRITEABLE callback so it
             * cannot corrupt a data frame that is mid-transmission. */
            client->ping_pending = 1;
            if (client->ws_wsi) {
                lws_callback_on_writable(client->ws_wsi);
            }
            lws_set_timer_usecs(wsi, (lws_usec_t)client->ping_interval_ms * 1000);
        }
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        if (client->reconnect_context_destroying) {
            client->ws_wsi = NULL;
            client->ws_state = AI_WS_DISCONNECTED;
            ai_ws_queue_clear(client);
            ai_ws_rx_reset(client);
            break;
        }
        printf("[ai_ws] client closed: provider=%s state=%d queued=%d rx_len=%u reconnect=%d disconnect=%d bin_frames=%u bin_bytes=%u bin_duration=%u\n",
               client->provider_name ? client->provider_name : "",
               client->ws_state,
               ai_ws_queue_pending(client),
               client->ws_rx_len,
               client->auto_reconnect,
               client->disconnect_requested,
               ai_ws_binary_write_count,
               ai_ws_binary_write_bytes,
               ai_ws_binary_write_start_ms ? timer_get_ms() - ai_ws_binary_write_start_ms : 0);
        client->ws_wsi = NULL;
        client->ws_state = AI_WS_DISCONNECTED;
        ai_ws_queue_clear(client);
        ai_ws_rx_reset(client);
        /* 真正的 connect() 留给 poll()，避免在 lws_service 同栈里再次进 lws。 */
        ai_dialogue_schedule_reconnect(client);
        ai_dialogue_emit(client, AI_DIALOGUE_CLOSED, NULL, NULL, 0, NULL, AI_OK);
        break;

    case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        ai_ws_log_peer_close(client, in, len);
        client->ws_state = AI_WS_CLOSING;
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        client->ws_wsi = NULL;
        client->ws_state = AI_WS_ERROR;
        ai_ws_queue_clear(client);
        ai_ws_rx_reset(client);
        ai_dialogue_schedule_reconnect(client);
        if (in && len) {
            char *message = ai_strndup((const char *)in, len);
            printf("[ai_ws] connection error: provider=%s len=%u detail=%.*s\n",
                   client->provider_name ? client->provider_name : "",
                   (unsigned int)len,
                   (int)len,
                   (const char *)in);
            ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                                   message ? message : "websocket connection error", NULL);
            free(message);
        } else {
            printf("[ai_ws] connection error: provider=%s no detail\n",
                   client->provider_name ? client->provider_name : "");
            ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                                   "websocket connection error", NULL);
        }
        break;

    default:
        break;
    }

    return 0;
}

/* 建立一条 provider WebSocket 会话，并把 app 回调挂到 client 上供后续 poll 分发。
 * 用户主动调用 connect 视为强意图：清掉退避计时和 disconnect_requested 标记。 */
int ai_dialogue_connect(struct ai_client *client, ai_dialogue_cb cb, void *user)
{
    if (!client || (!client->dialogue_ws_url &&
                    (!client->provider || !client->provider->build_dialogue_ws_url))) {
        return AI_ERR_PARAM;
    }
    if (client->ws_state != AI_WS_DISCONNECTED && client->ws_state != AI_WS_ERROR) {
        return AI_ERR_STATE;
    }

    client->dialogue_cb = cb;
    client->dialogue_user = user;
    client->disconnect_requested = 0;
    client->reconnect_backoff_ms = 0;
    client->reconnect_after_ms = 0;
    client->reconnect_attempts = 0;
    client->reconnect_connected_ms = 0;
    client->reconnect_need = 0;
    ai_client_clear_error(client);

    return ai_dialogue_connect_locked(client);
}

/* 实际发起连接的内部函数；自动重连时由 poll() 直接调用，回调里禁止直接调用。 */
static int ai_dialogue_connect_locked(struct ai_client *client)
{
    struct ai_ws_url url;
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    char *dynamic_url = NULL;
    int ret;

    if (!client) {
        return AI_ERR_PARAM;
    }
    if (client->provider && client->provider->build_dialogue_ws_url) {
        ret = client->provider->build_dialogue_ws_url(client, &dynamic_url);
        if (ret != AI_OK || !dynamic_url || !dynamic_url[0]) {
            free(dynamic_url);
            ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONFIG,
                                   ret != AI_OK ? ret : AI_ERR_PARAM, 0,
                                   "provider failed to build websocket URL", NULL);
            return ret != AI_OK ? ret : AI_ERR_PARAM;
        }
        ai_free_string(&client->dialogue_ws_url);
        client->dialogue_ws_url = dynamic_url;
    }
    if (!client->dialogue_ws_url) {
        return AI_ERR_PARAM;
    }

    ret = ai_ws_parse_url(client->dialogue_ws_url, &url);
    if (ret != AI_OK) {
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONFIG, ret, 0,
                               "invalid websocket URL", client->dialogue_ws_url);
        return ret;
    }

    if (client->ws_context) {
        client->reconnect_context_destroying = 1;
        ai_ws_ctx_destroy(client);
        client->reconnect_context_destroying = 0;
    }

    client->ws_state = AI_WS_CONNECTING;
    ai_ws_queue_clear(client);
    ai_ws_rx_reset(client);

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = ai_ws_protocols;
    info.user = client;
    if (url.use_ssl) {
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.client_ssl_ca_mem = mbedtls_test_cas_pem;
        info.client_ssl_ca_mem_len = mbedtls_test_cas_pem_len;
    }
    // 开启所有级别的日志，包括 DEBUG
//    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG, NULL);

    client->ws_context = lws_create_context(&info);
    if (!client->ws_context) {
        client->ws_state = AI_WS_ERROR;
        ai_ws_url_free(&url);
        ai_dialogue_schedule_reconnect(client);
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                               "websocket context create failed", NULL);
        return AI_ERR_NET;
    }

//    printf("%s ---------- %s", url.path, url.host);

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = client->ws_context;
    ccinfo.address = url.host;
    ccinfo.port = url.port;
    ccinfo.path = url.path;
    ccinfo.host = url.host;
    ccinfo.origin = url.host;
    ccinfo.protocol = NULL;
    ccinfo.local_protocol_name = ai_ws_protocols[0].name;
    ccinfo.ietf_version_or_minus_one = -1;
    ccinfo.userdata = client;
    ccinfo.opaque_user_data = client;
    ccinfo.pwsi = &client->ws_wsi;
    ccinfo.retry_and_idle_policy = &ai_ws_no_idle_timeout;
    ccinfo.ssl_connection = LCCSCF_IP_LOW_LATENCY;
    if (url.use_ssl) {
        ccinfo.ssl_connection |= LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;
    }


    client->ws_wsi = lws_client_connect_via_info(&ccinfo);
    ai_ws_url_free(&url);

    if (!client->ws_wsi) {
        printf("Failed to connect\n");
        client->reconnect_context_destroying = 1;
        ai_ws_ctx_destroy(client);
        client->reconnect_context_destroying = 0;
        client->ws_state = AI_WS_ERROR;
        ai_dialogue_schedule_reconnect(client);
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                               "websocket connect start failed", NULL);
        return AI_ERR_NET;
    }

    return AI_OK;
}

int ai_dialogue_session_start(struct ai_client *client)
{
    struct ai_ws_message msg;
    int ret;

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        printf("[ai] %s session_start rejected by state=%d ret=%d\n",
               client && client->provider_name ? client->provider_name : "unknown",
               client ? client->ws_state : -1,
               ret);
        return ret;
    }
    if (!client->provider || !client->provider->build_session_start) {
        printf("[ai] %s session_start unsupported\n",
               client && client->provider_name ? client->provider_name : "unknown");
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_session_start(client, &msg);
    if (ret != AI_OK) {
        printf("[ai] %s session_start build failed: %d\n",
               client->provider_name ? client->provider_name : "unknown",
               ret);
        ai_ws_message_free(&msg);
        return ret;
    }

    ret = ai_dialogue_queue_message(client, &msg);
    if (ret != AI_OK) {
        printf("[ai] %s session_start queue failed: %d\n",
               client->provider_name ? client->provider_name : "unknown",
               ret);
    }

    return ret;
}

int ai_dialogue_send_text(struct ai_client *client, const char *text)
{
    struct ai_ws_message msg;
    int ret;

    if (!text) {
        return AI_ERR_PARAM;
    }

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_text) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_text(client, text, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_send_audio(struct ai_client *client, const struct ai_audio_input *audio)
{
    struct ai_ws_message msg;
    int ret;

    if (!audio) {
        return AI_ERR_PARAM;
    }

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_audio) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_audio(client, audio, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_send_image(struct ai_client *client, const struct ai_image_input *image)
{
    struct ai_ws_message msg;
    int ret;

    if (!image) {
        return AI_ERR_PARAM;
    }

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_image) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_image(client, image, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_commit(struct ai_client *client)
{
    struct ai_ws_message msg;
    int ret;

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_commit) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_commit(client, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_interrupt(struct ai_client *client)
{
    struct ai_ws_message msg;
    int ret;

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_interrupt) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_interrupt(client, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_response_create(struct ai_client *client)
{
    struct ai_ws_message msg;
    int ret;

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_response_create) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_response_create(client, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_session_stop(struct ai_client *client)
{
    struct ai_ws_message msg;
    int ret;

    ret = ai_dialogue_can_queue(client);
    if (ret != AI_OK) {
        return ret;
    }
    if (!client->provider || !client->provider->build_session_stop) {
        return AI_ERR_UNSUPPORTED;
    }

    memset(&msg, 0, sizeof(msg));
    ret = client->provider->build_session_stop(client, &msg);
    if (ret != AI_OK) {
        ai_ws_message_free(&msg);
        return ret;
    }

    return ai_dialogue_queue_message(client, &msg);
}

int ai_dialogue_pending(struct ai_client *client)
{
    if (!client) {
        return 0;
    }

    return ai_ws_queue_pending(client);
}

int ai_dialogue_poll(struct ai_client *client, int timeout_ms)
{
    int ret;
    int pending;

    if (!client) {
        return AI_ERR_PARAM;
    }

    if (client->reconnect_need && client->auto_reconnect && !client->disconnect_requested) {
        if (client->ws_context) {
            client->reconnect_context_destroying = 1;
            ai_ws_ctx_destroy(client);
            client->reconnect_context_destroying = 0;
        }
        if ((int)((unsigned int)timer_get_ms() - client->reconnect_after_ms) >= 0) {
            client->reconnect_need = 0;
            client->reconnect_attempts++;
            printf("[ai_ws] reconnect attempt: provider=%s attempt=%u/%d\n",
                   client->provider_name ? client->provider_name : "",
                   client->reconnect_attempts,
                   client->reconnect_max_attempts);
            ai_dialogue_connect_locked(client);
        } else {
            if (timeout_ms > 0) {
                os_time_dly((timeout_ms + 9) / 10);
            }
            return AI_OK;
        }
    }

    if (!client->ws_context) {
        return AI_OK;
    }

    /* This SDK's FreeRTOS lws port turns every non-negative timeout into
     * 2000000000 ms. Drive lws nonblocking and preserve poll pacing here. */
    pending = ai_ws_queue_pending(client);
    ret = lws_service(client->ws_context, -1);
    if (ret < 0) {
        client->ws_state = AI_WS_ERROR;
        ai_dialogue_schedule_reconnect(client);
        ai_dialogue_emit_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                               "websocket service failed", NULL);
        return AI_ERR_NET;
    }
    if (timeout_ms > 0 && pending <= 0) {
        os_time_dly((timeout_ms + 9) / 10);
    }

    return AI_OK;
}

int ai_dialogue_poll_now(struct ai_client *client)
{
    return ai_dialogue_poll(client, 0);
}

/* 主动关闭对话连接：设 disconnect_requested 防止 framework 自动重连，
 * 销毁 lws_context 后发出 CLOSED 事件。再次连接需调用 ai_dialogue_connect()。 */
int ai_dialogue_close(struct ai_client *client)
{
    enum ai_ws_state old_state;

    if (!client) {
        return AI_ERR_PARAM;
    }

    client->disconnect_requested = 1;
    client->reconnect_need = 0;
    client->reconnect_backoff_ms = 0;
    client->reconnect_after_ms = 0;
    client->reconnect_attempts = 0;
    client->reconnect_connected_ms = 0;

    old_state = client->ws_state;
    client->ws_state = AI_WS_CLOSING;

    ai_ws_ctx_destroy(client);
    ai_ws_queue_clear(client);
    ai_ws_rx_reset(client);

    if ((old_state == AI_WS_CONNECTING || old_state == AI_WS_CONNECTED || old_state == AI_WS_CLOSING) &&
        client->ws_state != AI_WS_DISCONNECTED) {
        ai_dialogue_emit(client, AI_DIALOGUE_CLOSED, NULL, NULL, 0, NULL, AI_OK);
    }

    client->ws_state = AI_WS_DISCONNECTED;

    return AI_OK;
}
