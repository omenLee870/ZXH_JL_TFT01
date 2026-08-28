#include "ai_internal.h"

#include "libwebsockets.h"
#include "curl/curl.h"
#include <mbedtls/certs.h>

//#include <stdlib.h>
//#include <string.h>

#define AI_HTTP_LWS_DEFAULT_TIMEOUT_MS 10000
#define AI_HTTP_LWS_READ_BUF_SIZE      2048

/* Upper bound on any single growable HTTP buffer (full response, SSE reassembly,
 * or the raw stream mirror). Protects PSRAM from an unbounded/malicious response.
 * Vision replies are short text; products needing larger bodies can raise this. */
#ifndef AI_HTTP_MAX_BODY_BYTES
#define AI_HTTP_MAX_BODY_BYTES (512u * 1024u)
#endif

/* 本文件只处理 HTTP 传输，body 构造和响应语义仍由 provider 完成。 */
extern u32 timer_get_ms(void);

struct ai_http_buffer {
    char *data;
    size_t len;
};

struct ai_http_url {
    char *host;
    char *path;
    int port;
    int use_ssl;
};

struct ai_http_lws_ctx {
    struct ai_client *client;
    struct ai_vision_result *out;
    struct lws_context *context;
    struct lws *wsi;
    char *request_body;
    size_t request_body_len;
    int request_sent;
    int completed;
    int failed;
    int response_code;
    int is_sse;
    int stream_done;
    int ret;
    unsigned char *read_buffer;
    struct ai_http_buffer response;
    struct ai_http_buffer sse_pending;
    struct ai_http_buffer raw_stream;
};

/* curl/lws 都复用这套增长缓冲，避免两条 HTTP 路径各自维护拼接逻辑。 */
static int ai_http_buffer_append(struct ai_http_buffer *buf, const void *data, size_t len)
{
    char *next;

    if (!buf || (!data && len)) {
        return AI_ERR_PARAM;
    }
    if (!len) {
        return AI_OK;
    }

    /* Bound total growth so a runaway response cannot exhaust the heap. */
    if (len >= AI_HTTP_MAX_BODY_BYTES ||
        buf->len > AI_HTTP_MAX_BODY_BYTES - len - 1) {
        printf("[ai_http] response buffer over cap: have=%u add=%u cap=%u\n",
               (unsigned int)buf->len, (unsigned int)len, (unsigned int)AI_HTTP_MAX_BODY_BYTES);
        return AI_ERR_NOMEM;
    }

    next = (char *)realloc(buf->data, buf->len + len + 1);
    if (!next) {
        return AI_ERR_NOMEM;
    }

    memcpy(next + buf->len, data, len);
    buf->len += len;
    next[buf->len] = '\0';
    buf->data = next;
    return AI_OK;
}

static void ai_http_buffer_free(struct ai_http_buffer *buf)
{
    if (!buf) {
        return;
    }

    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
}

static size_t ai_http_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct ai_http_buffer *buf = (struct ai_http_buffer *)userdata;
    size_t bytes = size * nmemb;

    if (!buf || !ptr) {
        return 0;
    }
    if (ai_http_buffer_append(buf, ptr, bytes) != AI_OK) {
        return 0;
    }

    return bytes;
}

static int ai_http_add_default_headers(struct ai_client *client,
                                       ai_http_header_add_fn add,
                                       void *ctx)
{
    char *value;
    size_t len;
    int ret;

    if (!client || !add || !client->api_key || !client->api_key[0]) {
        return AI_OK;
    }
    len = strlen("Bearer ") + strlen(client->api_key) + 1;
    value = (char *)malloc(len);
    if (!value) {
        return AI_ERR_NOMEM;
    }
    strcpy(value, "Bearer ");
    strcat(value, client->api_key);
    ret = add(ctx, "Authorization", value);
    free(value);
    return ret;
}

static int ai_http_apply_headers(struct ai_client *client,
                                 ai_http_header_add_fn add,
                                 void *ctx)
{
    if (client && client->provider && client->provider->append_http_headers) {
        return client->provider->append_http_headers(client, add, ctx);
    }
    return ai_http_add_default_headers(client, add, ctx);
}

static int ai_http_curl_add_header(void *ctx, const char *name, const char *value)
{
    struct curl_slist **headers = (struct curl_slist **)ctx;
    struct curl_slist *next;
    size_t name_len;
    size_t value_len;
    char *line;

    if (!headers || !name || !name[0] || !value) {
        return AI_ERR_PARAM;
    }
    name_len = strlen(name);
    if (name[name_len - 1] == ':') {
        name_len--;
    }
    value_len = strlen(value);
    line = (char *)malloc(name_len + value_len + 3);
    if (!line) {
        return AI_ERR_NOMEM;
    }
    memcpy(line, name, name_len);
    memcpy(line + name_len, ": ", 2);
    memcpy(line + name_len + 2, value, value_len + 1);
    next = curl_slist_append(*headers, line);
    free(line);
    if (!next) {
        return AI_ERR_NOMEM;
    }
    *headers = next;
    return AI_OK;
}


static void ai_http_url_free(struct ai_http_url *url)
{
    if (!url) {
        return;
    }

    ai_free_string(&url->host);
    ai_free_string(&url->path);
    url->port = 0;
    url->use_ssl = 0;
}

/* HTTP URL 解析保持和 ws 那边同风格，后续 provider 只需要给完整 URL。 */
static int ai_http_parse_url(const char *url, struct ai_http_url *out)
{
    const char *p;
    const char *path;
    const char *colon;
    size_t host_len;

    if (!url || !out) {
        return AI_ERR_PARAM;
    }

    memset(out, 0, sizeof(*out));

    if (strncmp(url, "https://", 8) == 0) {
        out->use_ssl = 1;
        out->port = 443;
        p = url + 8;
    } else if (strncmp(url, "http://", 7) == 0) {
        out->use_ssl = 0;
        out->port = 80;
        p = url + 7;
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
        size_t i;

        if (!port_len) {
            return AI_ERR_PARAM;
        }
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
        ai_http_url_free(out);
        return AI_ERR_NOMEM;
    }

    out->path = *path ? ai_strdup(path) : ai_strdup("/");
    if (!out->path) {
        ai_http_url_free(out);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

/* 流式增量文本最终仍要落回 ai_vision_result，保证 blocking 接口也能拿到完整结果。 */
static int ai_vision_result_append_text(struct ai_vision_result *out, const char *text)
{
    char *next;
    size_t old_len;
    size_t add_len;

    if (!out || !text || !text[0]) {
        return AI_OK;
    }

    if (!out->text) {
        out->text = ai_strdup(text);
        return out->text ? AI_OK : AI_ERR_NOMEM;
    }

    old_len = strlen(out->text);
    add_len = strlen(text);
    next = (char *)realloc(out->text, old_len + add_len + 1);
    if (!next) {
        return AI_ERR_NOMEM;
    }

    memcpy(next + old_len, text, add_len + 1);
    out->text = next;
    return AI_OK;
}

static struct ai_http_lws_ctx *ai_http_lws_from_callback(struct lws *wsi, void *user)
{
    struct ai_http_lws_ctx *ctx = (struct ai_http_lws_ctx *)user;

    if (!ctx && wsi) {
        ctx = (struct ai_http_lws_ctx *)lws_get_opaque_user_data(wsi);
    }

    return ctx;
}

struct ai_http_lws_header_ctx {
    struct lws *wsi;
    unsigned char **p;
    unsigned char *end;
};

static int ai_http_lws_add_header(void *user, const char *name, const char *value)
{
    struct ai_http_lws_header_ctx *ctx = (struct ai_http_lws_header_ctx *)user;
    char header[64];
    size_t len;

    if (!ctx || !ctx->wsi || !ctx->p || !name || !name[0] || !value) {
        return AI_ERR_PARAM;
    }
    len = strlen(name);
    /* ponytail: HTTP field names over 62 bytes are rejected; expand only for a real vendor. */
    if (len + (name[len - 1] == ':' ? 1 : 2) > sizeof(header)) {
        return AI_ERR_PARAM;
    }
    memcpy(header, name, len);
    if (header[len - 1] != ':') {
        header[len++] = ':';
    }
    header[len] = '\0';
    return lws_add_http_header_by_name(ctx->wsi,
                                       (const unsigned char *)header,
                                       (const unsigned char *)value,
                                       (int)strlen(value),
                                       ctx->p,
                                       ctx->end) ? AI_ERR_NET : AI_OK;
}

static int ai_http_lws_add_header_token(struct lws *wsi,
                                        unsigned char **p,
                                        unsigned char *end,
                                        enum lws_token_indexes token,
                                        const char *value)
{
    if (!value || !value[0]) {
        return AI_OK;
    }

    return lws_add_http_header_by_token(wsi, token,
                                        (const unsigned char *)value,
                                        (int)strlen(value),
                                        p,
                                        end) ? AI_ERR_NET : AI_OK;
}

/* LWS HTTP 路径在握手阶段把鉴权、Accept 和 body length 一次性补齐。 */
static int ai_http_lws_append_headers(struct ai_http_lws_ctx *ctx, struct lws *wsi, void *in, size_t len)
{
    unsigned char **p = (unsigned char **)in;
    unsigned char *end;
    struct ai_http_lws_header_ctx header_ctx;
    char content_len[24];
    int ret;

    if (!ctx || !ctx->client || !wsi || !in) {
        return AI_ERR_PARAM;
    }

    end = *p + len;
    header_ctx.wsi = wsi;
    header_ctx.p = p;
    header_ctx.end = end;
    ret = ai_http_apply_headers(ctx->client, ai_http_lws_add_header, &header_ctx);
    if (ret != AI_OK) {
        return ret;
    }

    ret = ai_http_lws_add_header_token(wsi, p, end, WSI_TOKEN_HTTP_CONTENT_TYPE, "application/json");
    if (ret != AI_OK) {
        return ret;
    }
    ret = ai_http_lws_add_header_token(wsi, p, end, WSI_TOKEN_HTTP_ACCEPT, "application/json, text/event-stream");
    if (ret != AI_OK) {
        return ret;
    }

    if (ctx->request_body_len > 0) {
        snprintf(content_len, sizeof(content_len), "%u", (unsigned int)ctx->request_body_len);
        ret = ai_http_lws_add_header_token(wsi, p, end, WSI_TOKEN_HTTP_CONTENT_LENGTH, content_len);
        if (ret != AI_OK) {
            return ret;
        }
        lws_client_http_body_pending(wsi, 1);
        lws_callback_on_writable(wsi);
    }

    return AI_OK;
}

/* 一个 SSE block 里可能有多行 data:，这里先拼成 provider 可直接解析的单条 payload。 */
static int ai_http_lws_extract_sse_data(const char *block, size_t len, char **out)
{
    const char *p = block;
    char *data = NULL;
    size_t data_len = 0;

    if (!block || !out) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    while ((size_t)(p - block) < len) {
        const char *line_end = memchr(p, '\n', len - (size_t)(p - block));
        size_t line_len = line_end ? (size_t)(line_end - p) : (len - (size_t)(p - block));
        const char *value;
        size_t value_len;
        char *next;

        if (line_len > 0 && p[line_len - 1] == '\r') {
            line_len--;
        }
        if (line_len >= 5 && strncmp(p, "data:", 5) == 0) {
            value = p + 5;
            if ((size_t)(value - p) < line_len && *value == ' ') {
                value++;
            }
            value_len = line_len - (size_t)(value - p);
            next = (char *)realloc(data, data_len + value_len + (data_len ? 1 : 0) + 1);
            if (!next) {
                free(data);
                return AI_ERR_NOMEM;
            }
            data = next;
            if (data_len) {
                data[data_len++] = '\n';
            }
            memcpy(data + data_len, value, value_len);
            data_len += value_len;
            data[data_len] = '\0';
        }

        if (!line_end) {
            break;
        }
        p = line_end + 1;
    }

    *out = data;
    return AI_OK;
}

static int ai_http_lws_handle_sse_payload(struct ai_http_lws_ctx *ctx, const char *payload)
{
    char *delta_text = NULL;
    int done = 0;
    int ret;

    if (!ctx || !ctx->client || !ctx->out || !payload) {
        return AI_ERR_PARAM;
    }

    ret = ai_http_buffer_append(&ctx->raw_stream, payload, strlen(payload));
    if (ret != AI_OK) {
        return ret;
    }
    ret = ai_http_buffer_append(&ctx->raw_stream, "\n", 1);
    if (ret != AI_OK) {
        return ret;
    }

    if (!ctx->client->provider || !ctx->client->provider->parse_vision_http_stream_event) {
        return AI_ERR_UNSUPPORTED;
    }

    ret = ctx->client->provider->parse_vision_http_stream_event(ctx->client, payload, &delta_text, &done);
    if (ret == AI_OK && delta_text) {
        ret = ai_vision_result_append_text(ctx->out, delta_text);
        if (ret == AI_OK) {
            /* HTTP streaming和语音对话复用同一套事件分发，测试层不需要再区分第二套回调。 */
            ai_dialogue_emit(ctx->client, AI_DIALOGUE_TEXT_DELTA, delta_text, NULL, 0, payload, AI_OK);
        }
    }

    if (ret == AI_OK && done && !ctx->stream_done) {
        ctx->stream_done = 1;
        ai_dialogue_emit(ctx->client, AI_DIALOGUE_TEXT_DONE, NULL, NULL, 0, payload, AI_OK);
    }

    free(delta_text);
    return ret;
}

/* LWS 分片回调可能把同一条 SSE 事件拆开，多次 read 后要在这里重组。 */
static int ai_http_lws_process_sse(struct ai_http_lws_ctx *ctx, const char *data, size_t len)
{
    size_t start = 0;
    int ret;

    if (!ctx || (!data && len)) {
        return AI_ERR_PARAM;
    }

    ret = ai_http_buffer_append(&ctx->sse_pending, data, len);
    if (ret != AI_OK) {
        return ret;
    }

    while (start + 1 < ctx->sse_pending.len) {
        size_t i;
        size_t sep_len = 0;
        char *payload = NULL;

        for (i = start; i + 1 < ctx->sse_pending.len; i++) {
            if (ctx->sse_pending.data[i] == '\n' && ctx->sse_pending.data[i + 1] == '\n') {
                sep_len = 2;
                break;
            }
            if (i + 3 < ctx->sse_pending.len &&
                ctx->sse_pending.data[i] == '\r' &&
                ctx->sse_pending.data[i + 1] == '\n' &&
                ctx->sse_pending.data[i + 2] == '\r' &&
                ctx->sse_pending.data[i + 3] == '\n') {
                sep_len = 4;
                break;
            }
        }

        if (!sep_len) {
            break;
        }

        ret = ai_http_lws_extract_sse_data(ctx->sse_pending.data + start, i - start, &payload);
        if (ret != AI_OK) {
            return ret;
        }
        if (payload && payload[0]) {
            ret = ai_http_lws_handle_sse_payload(ctx, payload);
            free(payload);
            if (ret != AI_OK) {
                return ret;
            }
        } else {
            free(payload);
        }

        start = i + sep_len;
    }

    if (start > 0) {
        size_t remain = ctx->sse_pending.len - start;
        if (remain > 0) {
            memmove(ctx->sse_pending.data, ctx->sse_pending.data + start, remain);
        }
        ctx->sse_pending.len = remain;
        if (ctx->sse_pending.data) {
            ctx->sse_pending.data[remain] = '\0';
        }
    }

    return AI_OK;
}

static int ai_http_lws_callback(struct lws *wsi,
                                enum lws_callback_reasons reason,
                                void *user,
                                void *in,
                                size_t len)
{
    struct ai_http_lws_ctx *ctx = ai_http_lws_from_callback(wsi, user);
    int ret;

    if (!ctx) {
        return 0;
    }

    switch (reason) {
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        ret = ai_http_lws_append_headers(ctx, wsi, in, len);
        if (ret != AI_OK) {
            ctx->ret = ret;
            ctx->failed = 1;
            ctx->completed = 1;
            ai_dialogue_emit_error(ctx->client, AI_ERROR_STAGE_AUTH, ret, 0,
                                   "HTTP header build failed", NULL);
            return -1;
        }
        break;

    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        if (!ctx->request_sent && ctx->request_body && ctx->request_body_len > 0) {
            unsigned char *buf = (unsigned char *)malloc(LWS_PRE + ctx->request_body_len);
            int written;

            if (!buf) {
                ctx->ret = AI_ERR_NOMEM;
                ctx->failed = 1;
                ctx->completed = 1;
                return -1;
            }
            memcpy(buf + LWS_PRE, ctx->request_body, ctx->request_body_len);
            written = lws_write(wsi, buf + LWS_PRE, (unsigned int)ctx->request_body_len, LWS_WRITE_HTTP_FINAL);
            free(buf);
            if (written < 0 || (size_t)written < ctx->request_body_len) {
                ctx->ret = AI_ERR_NET;
                ctx->failed = 1;
                ctx->completed = 1;
                return -1;
            }
            ctx->request_sent = 1;
        }
        lws_client_http_body_pending(wsi, 0);
        break;

    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
        ctx->wsi = wsi;
        ctx->response_code = (int)lws_http_client_http_response(wsi);
        printf("[ai_http] established http status=%d\n", ctx->response_code);
        break;

    case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: {
        int header_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE);
        if (header_len > 0) {
            char content_type[96];
            if (header_len >= (int)sizeof(content_type)) {
                header_len = sizeof(content_type) - 1;
            }
            lws_hdr_copy(wsi, content_type, header_len + 1, WSI_TOKEN_HTTP_CONTENT_TYPE);
            if (strstr(content_type, "text/event-stream")) {
                ctx->is_sse = 1;
            }
        }
        break;
    }

    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
        char *readp = (char *)ctx->read_buffer + LWS_PRE;
        int readlen = AI_HTTP_LWS_READ_BUF_SIZE;

        if (lws_http_client_read(wsi, &readp, &readlen) < 0) {
            ctx->ret = AI_ERR_NET;
            ctx->failed = 1;
            ctx->completed = 1;
            return -1;
        }
        return 0;
    }

    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
        if (ctx->is_sse) {
            ret = ai_http_lws_process_sse(ctx, (const char *)in, len);
        } else {
            ret = ai_http_buffer_append(&ctx->response, in, len);
        }
        if (ret != AI_OK) {
            ctx->ret = ret;
            ctx->failed = 1;
            ctx->completed = 1;
            return -1;
        }
        break;

    case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        if (ctx->is_sse && ctx->sse_pending.len > 0) {
            ret = ai_http_lws_process_sse(ctx, "\n\n", 2);
            if (ret != AI_OK) {
                ctx->ret = ret;
            }
        }
        ctx->response_code = (int)lws_http_client_http_response(wsi);
        printf("[ai_http] completed http status=%d sse=%d body_len=%u raw_len=%u\n",
               ctx->response_code,
               ctx->is_sse,
               (unsigned int)ctx->response.len,
               (unsigned int)ctx->raw_stream.len);
        ctx->completed = 1;
        if (ctx->ret == AI_OK) {
            ctx->ret = (ctx->response_code >= 200 && ctx->response_code < 300) ? AI_OK : AI_ERR_NET;
        }
        if (ctx->ret != AI_OK) {
            ai_client_set_error(ctx->client, AI_ERROR_STAGE_HTTP, ctx->ret,
                                ctx->response_code, "HTTP status error",
                                ctx->is_sse ? ctx->raw_stream.data : ctx->response.data);
        }
        lws_cancel_service(lws_get_context(wsi));
        break;

    case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
        ctx->response_code = (int)lws_http_client_http_response(wsi);
        printf("[ai_http] closed http status=%d failed=%d ret=%d completed=%d\n",
               ctx->response_code,
               ctx->failed,
               ctx->ret,
               ctx->completed);
        if (!ctx->completed) {
            ctx->completed = 1;
            if (ctx->ret == AI_OK) {
                ctx->ret = (ctx->response_code >= 200 && ctx->response_code < 300) ? AI_OK : AI_ERR_NET;
            }
        }
        lws_cancel_service(lws_get_context(wsi));
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        char *detail = in && len ? ai_strndup((const char *)in, len) : NULL;
        printf("[ai_http] connection error: %s\n", detail ? detail : "");
        ctx->ret = AI_ERR_NET;
        ctx->failed = 1;
        ctx->completed = 1;
        ai_dialogue_emit_error(ctx->client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                               detail ? detail : "HTTP connection error", NULL);
        free(detail);
        lws_cancel_service(lws_get_context(wsi));
        break;
    }

    default:
        break;
    }

    return 0;
}

static const struct lws_protocols ai_http_protocols[] = {
    { "ai-http", ai_http_lws_callback, 0, AI_HTTP_LWS_READ_BUF_SIZE, 0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
};

/* curl 路径适合阻塞式一次性 HTTP 调用；只有完整响应落地后才交给 provider 解析。 */
static int ai_vision_recognize_curl(struct ai_client *client,
                                    const struct ai_image_input *image,
                                    const char *prompt,
                                    struct ai_vision_result *out)
{
    CURL *curl;
    CURLcode code;
    long http_code = 0;
    struct curl_slist *headers = NULL;
    struct ai_http_buffer response = {0};
    char *body = NULL;
    int ret;

    if (!client || !prompt || !prompt[0] || !out) {
        return AI_ERR_PARAM;
    }
    if (!client->vision_http_url || !client->provider || !client->provider->build_vision_http_body) {
        return AI_ERR_PARAM;
    }

    out->text = NULL;
    out->raw_json = NULL;

    ret = client->provider->build_vision_http_body(client, image, prompt, &body);
    if (ret != AI_OK || !body) {
        free(body);
        return ret != AI_OK ? ret : AI_ERR_PARSE;
    }

    curl = curl_easy_init();
    if (!curl) {
        free(body);
        return AI_ERR_NET;
    }

    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        free(body);
        return AI_ERR_NOMEM;
    }

    ret = ai_http_apply_headers(client, ai_http_curl_add_header, &headers);
    if (ret != AI_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(body);
        return ret;
    }

    curl_easy_setopt(curl, CURLOPT_URL, client->vision_http_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ai_http_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if (client->timeout_ms > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)client->timeout_ms);
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        printf("[ai_http] curl failed: %d (%s)\n", (int)code, curl_easy_strerror(code));
        ret = AI_ERR_NET;
        ai_client_set_error(client, AI_ERROR_STAGE_CONNECT, ret, (int)code,
                            curl_easy_strerror(code), NULL);
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (!response.data) {
            ret = AI_ERR_PARSE;
        } else if (!client->provider->parse_vision_http_response) {
            ret = AI_ERR_PARSE;
        } else {
            int parse_ret = client->provider->parse_vision_http_response(client, response.data, out);

            if (http_code < 200 || http_code >= 300) {
                printf("[ai_http] http failed: %ld %s\n", http_code, response.data ? response.data : "");
                ret = AI_ERR_NET;
                ai_client_set_error(client, AI_ERROR_STAGE_HTTP, ret, (int)http_code,
                                    "HTTP status error", response.data);
                if (!out->text) {
                    out->text = ai_strdup(response.data);
                    if (!out->text) {
                        ret = AI_ERR_NOMEM;
                    }
                }
            } else {
                ret = parse_ret;
                if (ret != AI_OK) {
                    ai_client_set_error(client, AI_ERROR_STAGE_PARSE, ret, 0,
                                        "HTTP response parse failed", response.data);
                }
            }
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    ai_http_buffer_free(&response);
    free(body);

    /* On error keep whatever the provider parsed into out (error message in
     * out->text and/or the raw body in out->raw_json) so the caller can surface
     * it; only clear when there is nothing useful. The structured error is
     * always available via ai_client_get_last_error() regardless. */
    if (ret != AI_OK && !out->text && !out->raw_json) {
        ai_vision_result_free(out);
    }

    return ret;
}

/* LWS 路径除了普通 HTTP，还承接 SSE 流式输出，并把 delta 实时回灌给上层。 */
static int ai_vision_recognize_lws(struct ai_client *client,
                                   const struct ai_image_input *image,
                                   const char *prompt,
                                   struct ai_vision_result *out)
{
    struct ai_http_lws_ctx req;
    struct ai_http_url url;
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    u32 start_ms;
    u32 timeout_ms;
    int ret;

    if (!client || !prompt || !prompt[0] || !out) {
        printf("[ai_http] invalid vision args client=%p prompt=%p out=%p\n", client, prompt, out);
        return AI_ERR_PARAM;
    }
    if (!client->vision_http_url || !client->provider || !client->provider->build_vision_http_body) {
        printf("[ai_http] invalid vision client url=%p provider=%p build=%p\n",
               client->vision_http_url,
               client->provider,
               client->provider ? client->provider->build_vision_http_body : NULL);
        return AI_ERR_PARAM;
    }

    memset(out, 0, sizeof(*out));
    memset(&req, 0, sizeof(req));
    req.client = client;
    req.out = out;
    req.ret = AI_OK;

    ret = client->provider->build_vision_http_body(client, image, prompt, &req.request_body);
    if (ret != AI_OK || !req.request_body) {
        free(req.request_body);
        return ret != AI_OK ? ret : AI_ERR_PARSE;
    }
    req.request_body_len = strlen(req.request_body);
    req.read_buffer = (unsigned char *)ai_malloc_large(AI_HTTP_LWS_READ_BUF_SIZE + LWS_PRE);
    if (!req.read_buffer) {
        free(req.request_body);
        return AI_ERR_NOMEM;
    }

    ret = ai_http_parse_url(client->vision_http_url, &url);
    if (ret != AI_OK) {
        ai_free_large(req.read_buffer);
        free(req.request_body);
        return ret;
    }

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = ai_http_protocols;
    info.user = &req;
    if (url.use_ssl) {
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.client_ssl_ca_mem = mbedtls_test_cas_pem;
        info.client_ssl_ca_mem_len = mbedtls_test_cas_pem_len;
    }

    req.context = lws_create_context(&info);
    if (!req.context) {
        ai_http_url_free(&url);
        ai_free_large(req.read_buffer);
        free(req.request_body);
        ai_client_set_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                            "HTTP context create failed", NULL);
        return AI_ERR_NET;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = req.context;
    ccinfo.address = url.host;
    ccinfo.port = url.port;
    ccinfo.path = url.path;
    ccinfo.host = url.host;
    ccinfo.origin = url.host;
    ccinfo.method = "POST";
    ccinfo.alpn = "http/1.1";
    ccinfo.protocol = NULL;
    ccinfo.local_protocol_name = ai_http_protocols[0].name;
    ccinfo.userdata = &req;
    ccinfo.opaque_user_data = &req;
    ccinfo.pwsi = &req.wsi;
    if (url.use_ssl) {
        ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    }

    req.wsi = lws_client_connect_via_info(&ccinfo);
    ai_http_url_free(&url);
    if (!req.wsi) {
        lws_context_destroy(req.context);
        ai_free_large(req.read_buffer);
        free(req.request_body);
        ai_client_set_error(client, AI_ERROR_STAGE_CONNECT, AI_ERR_NET, 0,
                            "HTTP connect start failed", NULL);
        return AI_ERR_NET;
    }

    timeout_ms = client->timeout_ms > 0 ? (u32)client->timeout_ms : AI_HTTP_LWS_DEFAULT_TIMEOUT_MS;
    start_ms = timer_get_ms();
    while (!req.completed) {
        if ((timer_get_ms() - start_ms) >= timeout_ms) {
            req.ret = AI_ERR_NET;
            req.failed = 1;
            ai_client_set_error(client, AI_ERROR_STAGE_TIMEOUT, req.ret, 0,
                                "HTTP request timeout", NULL);
            break;
        }
        if (lws_service(req.context, -1) < 0) {
            req.ret = AI_ERR_NET;
            req.failed = 1;
            ai_client_set_error(client, AI_ERROR_STAGE_RECEIVE, req.ret, 0,
                                "HTTP service failed", NULL);
            break;
        }
        os_time_dly(1);
    }

    if (req.context) {
        lws_context_destroy(req.context);
        req.context = NULL;
    }
    ai_free_large(req.read_buffer);
    free(req.request_body);

    if (req.is_sse) {
        if (req.raw_stream.data) {
            out->raw_json = req.raw_stream.data;
            req.raw_stream.data = NULL;
            req.raw_stream.len = 0;
        }
        if (req.ret == AI_OK && !req.stream_done && !out->text) {
            req.ret = AI_ERR_PARSE;
        }
    } else if (req.response.data && client->provider->parse_vision_http_response) {
        /* Always run the provider parser: on an HTTP-status error it extracts the
         * vendor error message into out->text and the body into out->raw_json; on
         * success it extracts the answer. Only downgrade req.ret to a parse error
         * when the transport itself was OK — a status error stays a status error.
         * (The previous fallback here read req.response.data after setting it NULL,
         * so out->text was always the bogus "http request failed" placeholder.) */
        int parse_ret = client->provider->parse_vision_http_response(client, req.response.data, out);

        if (req.ret == AI_OK) {
            req.ret = parse_ret;
            if (req.ret != AI_OK) {
                ai_client_set_error(client, AI_ERROR_STAGE_PARSE, req.ret, 0,
                                    "HTTP response parse failed", req.response.data);
            }
        }
    } else if (req.ret == AI_OK) {
        req.ret = AI_ERR_PARSE;
    }

    ai_http_buffer_free(&req.response);
    ai_http_buffer_free(&req.sse_pending);
    ai_http_buffer_free(&req.raw_stream);

    if (req.ret != AI_OK && !out->text && !out->raw_json) {
        ai_vision_result_free(out);
    }

    return req.ret;
}

/* HTTP backend 的选择收口在这里，framework/app 层不需要关心 curl 还是 lws。 */
int ai_vision_recognize(struct ai_client *client,
                        const struct ai_image_input *image,
                        const char *prompt,
                        struct ai_vision_result *out)
{
    if (!client) {
        return AI_ERR_PARAM;
    }

    ai_client_clear_error(client);
    switch (client->http_backend) {
    case AI_HTTP_BACKEND_LWS:
        return ai_vision_recognize_lws(client, image, prompt, out);
    case AI_HTTP_BACKEND_CURL:
    default:
        return ai_vision_recognize_curl(client, image, prompt, out);
    }
}
