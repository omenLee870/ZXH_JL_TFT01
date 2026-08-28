#ifndef AI_INTERNAL_H
#define AI_INTERNAL_H

#include "ai_provider.h"
#include "os/os_api.h"

#include <stddef.h>

struct lws;
struct lws_context;

typedef int (*ai_http_header_add_fn)(void *ctx, const char *name, const char *value);

#define AI_WS_QUEUE_DEPTH 16

/* Upper bound on a single reassembled WebSocket message. A frame larger than
 * this is treated as a protocol/DoS error and the connection is dropped, so a
 * runaway or malicious server cannot grow the RX buffer until PSRAM is exhausted.
 * Products with legitimately larger downlink frames can raise this. */
#ifndef AI_WS_RX_MAX_BYTES
#define AI_WS_RX_MAX_BYTES (256u * 1024u)
#endif

#define AI_PROVIDER_CAP_IMAGE_TO_TEXT 0x0001u
#define AI_PROVIDER_CAP_VOICE_DIALOGUE 0x0002u

#define AI_PROVIDER_FLAG_VISION_API_KEY_REQUIRED 0x0001u

#define AI_VOICE_FLOW_LONG_STREAM       0u
#define AI_VOICE_FLOW_SINGLE_TURN       1u
#define AI_VOICE_DEFAULT_FRAME_MS       10u
#define AI_VOICE_DEFAULT_PENDING_LOW    4u
#define AI_VOICE_DEFAULT_UPLOAD_LOG     20u

#ifndef AI_PROVIDER_DEMO_ENABLE
#define AI_PROVIDER_DEMO_ENABLE 0
#endif

/* 单次语音流程的轮询参数都收敛到 voice profile，避免每个 provider 各自硬编码一份。
 * ready_polls 只对声明 is_ready_event 的 provider 生效；没有 ready gate 的厂商连接后可直接发送音频。
 * send_retry_polls：单帧音频发送遇到 STATE 时的回退轮询上限；
 * speak_stop_hold_polls：VAD 判定说话结束后再多观察的 poll 次数，避免误判。*/
struct ai_provider_voice_profile {
    enum ai_audio_format audio_format;
    unsigned int audio_chunk_size;
    unsigned int frame_interval_ms;
    unsigned int pending_low_watermark;
    unsigned int upload_log_interval;
    unsigned int sample_rate;
    unsigned short channels;
    unsigned short bits_per_sample;
    unsigned int flow_mode;
    int poll_timeout_ms;
    int audio_poll_timeout_ms;
    int connect_polls;
    int ready_polls;
    int response_polls;
    int send_retry_polls;
    int speak_stop_hold_polls;
};

struct ai_ws_url {
    char *host;
    char *path;
    int port;
    int use_ssl;
};

struct ai_ws_message {
    unsigned char *data;
    unsigned int len;
    int binary;
};

/* WebSocket 状态机：CONNECTING -> CONNECTED -> CLOSING -> DISCONNECTED；
 * 任何阶段失败都进入 ERROR，需要由调用方显式 close 后才能重新 connect。 */
enum ai_ws_state {
    AI_WS_DISCONNECTED = 0,
    AI_WS_CONNECTING,
    AI_WS_CONNECTED,
    AI_WS_CLOSING,
    AI_WS_ERROR,
};

/*
 * 厂商协议适配表。只有新增 providers/ai_provider_xxx.c 时才需要实现它。
 *
 * provider 层只负责“厂家协议差异”：
 *   1. 这个厂家支持哪些能力，默认 URL / model / voice 参数是什么；
 *   2. framework 要发送 start / text / audio / commit / stop 时，厂家报文怎么拼；
 *   3. 服务端 WebSocket / HTTP 响应怎么解析成统一 ai_dialogue_event。
 *
 * provider 层不要做这些事：
 *   - 不创建线程，不读取麦克风，不播放音频；
 *   - 不直接调用 lws_service，不维护 WebSocket 重连状态机；
 *   - 不把 URL 播放、cbuf 写入、业务按键流程写在这里。
 *
 * 普通 WebSocket / HTTP 对话厂家：新增 provider 文件并在 ai_provider_registry.c 注册。
 * MQTT 设备管理、OTA、影子状态、普通 topic 控制：不要套这个 ops，走 ai_mqtt 或业务层。
 */
struct ai_provider_ops {
    /*
     * provider 名称。必填。
     * 应用层 cfg.provider_name 会用这个字符串在 ai_provider_find() 里查找 provider。
     * 建议只用小写字母/数字/下划线，例如 "duer"、"glm"、"aitoye2e"。
     */
    const char *name;

    /*
     * provider 能力位。必填。
     * AI_PROVIDER_CAP_VOICE_DIALOGUE 表示支持 WebSocket/实时语音对话。
     * AI_PROVIDER_CAP_IMAGE_TO_TEXT 表示支持 HTTP 图文/文本请求。
     * 一个 provider 可以同时声明多个能力。
     */
    unsigned int caps;

    /*
     * provider 附加行为标志。没有特殊要求时填 0。
     * 例如 AI_PROVIDER_FLAG_VISION_API_KEY_REQUIRED 表示 HTTP vision 请求必须有 api_key。
     */
    unsigned int flags;

    /*
     * 默认 WebSocket 对话 URL。可选。
     * ai_realtime_start() 创建底层 client 时，如果 cfg.dialogue_ws_url 为空，就使用这里。
     * 如果厂家 URL 每轮都要动态生成，例如 aitoye2e 的 e2e_voice_start()，当前应由应用层
     * 先生成 URL 再填 cfg.dialogue_ws_url；后续可扩展 provider 动态 URL hook。
     */
    const char *default_dialogue_ws_url;

    /*
     * 默认 HTTP vision/text URL。可选。
     * ai_vision_recognize() 使用，cfg.vision_http_url 为空时才取这里。
     */
    const char *default_vision_http_url;

    /*
     * 默认模型名。可选。
     * cfg.model 为空时复制到 client->model，provider 构造报文时可读取 client->model。
     */
    const char *default_model;

    /*
     * 默认厂家私有参数。可选。
     * cfg.provider_params 为空时复制到 client->provider_params。通常放 JSON 字符串，
     * 例如 cuid、appid、voice、sn、业务开关等 provider 私有配置。
     */
    const char *default_provider_params;

    /*
     * 语音 profile。语音 provider 必填，HTTP-only provider 填 NULL。
     * realtime 层用它决定音频格式、每包大小、发送间隔、单次/长连接流程、ready 等待时间。
     */
    const struct ai_provider_voice_profile *voice;

    /* 每次连接前动态生成完整 ws/wss URL；返回的字符串由 framework 释放。 */
    int (*build_dialogue_ws_url)(struct ai_client *client, char **out_url);

    /*
     * 判断服务端事件是否表示“可以开始发音频”。可选。
     * 只有 Duer 这类必须等 ready 的单次对话需要实现；普通长连接实时对话填 NULL。
     * ai_realtime_start() 在发送 session_start 后根据这个回调等待 ready gate。
     */
    int (*is_ready_event)(const struct ai_dialogue_event *event);

    /* Provider says request-side audio can stop, but the turn may still need commit/response. */
    int (*is_upload_end_event)(const struct ai_dialogue_event *event);

    /*
     * 判断服务端事件是否表示“整个会话结束”。可选。
     * 单次会话用它让 realtime 上传线程停止；长连接连续对话通常填 NULL。
     */
    int (*is_session_end_event)(const struct ai_dialogue_event *event);

    /*
     * 覆盖 framework 默认的“本轮输出完成”判断。可选。
     * 默认只有 AI_DIALOGUE_DONE 结束当前轮；TEXT_DONE / AUDIO_DONE / AUDIO_URL
     * 只表示对应输出通道完成。Duer 等特殊协议可在这里保留厂家语义。
     */
    int (*is_turn_done_event)(const struct ai_dialogue_event *event);

    /*
     * 构造连接后的启动包。可选。
     * 如果厂家要求 WebSocket 连接成功后先发 session_start / config / start 帧，就实现它。
     * 如果连接成功即可直接发音频，例如某些长连接协议，填 NULL。
     */
    int (*build_session_start)(struct ai_client *client, struct ai_ws_message *out_msg);

    /*
     * 构造文本输入帧。支持文本输入时实现，否则填 NULL。
     * ai_dialogue_send_text() / ai_realtime_send_text() 会调用它。
     */
    int (*build_text)(struct ai_client *client, const char *text, struct ai_ws_message *out_msg);

    /*
     * 构造音频输入帧。语音 provider 必填。
     * 可以输出二进制音频帧，也可以输出 JSON/base64 文本帧；结果写入 out_msg。
     * ai_realtime_upload_audio_source() 读取音频后最终会调用到这里。
     */
    int (*build_audio)(struct ai_client *client, const struct ai_audio_input *audio, struct ai_ws_message *out_msg);

    /* 构造图片输入帧。支持实时多模态图片输入时实现，否则填 NULL。 */
    int (*build_image)(struct ai_client *client, const struct ai_image_input *image, struct ai_ws_message *out_msg);

    /*
     * 构造一轮输入结束/提交帧。可选。
     * 如果厂家需要 finish / commit / end 包才开始返回最终结果，就实现它。
     * 如果长连接协议不需要显式提交，填 NULL。
     */
    int (*build_commit)(struct ai_client *client, struct ai_ws_message *out_msg);

    /*
     * 构造打断当前回复的控制帧。可选。
     * ai_realtime_interrupt() 调用；厂家不支持打断时填 NULL。
     */
    int (*build_interrupt)(struct ai_client *client, struct ai_ws_message *out_msg);

    /* 构造请求服务端生成回复的控制帧。可选，Manual realtime 模式常用。 */
    int (*build_response_create)(struct ai_client *client, struct ai_ws_message *out_msg);

    /*
     * 构造结束会话的控制帧。可选。
     * ai_dialogue_session_stop() 调用；如果关闭连接即可结束会话，填 NULL。
     */
    int (*build_session_stop)(struct ai_client *client, struct ai_ws_message *out_msg);

    /*
     * 解析 WebSocket 下行消息。WebSocket provider 必填。
     * transport/ai_ws.c 收到完整帧后调用它，把厂家 JSON/二进制转换为统一 ai_dialogue_event。
     * data/len/binary 是原始下行帧；out_event 是解析后的统一事件。
     * 如果解析出的 text/data 需要新分配内存保存，把指针放到 *owned_data，框架回调后释放。
     *
     * 分配器约定：*owned_data 必须用大堆分配器分配（ai_malloc_large / ai_strdup_large /
     * ai_base64_decode），因为框架统一用 ai_free_large() 释放它。用普通 malloc / ai_strdup
     * 分配再交回 owned_data，在 TCFG_PSRAM_DEV_ENABLE 打开时是跨堆 free（堆损坏）。
     * out_event->text/url 若指向 owned_data 内部，无需单独释放。
     */
    int (*parse_ws_message)(struct ai_client *client,
                            const unsigned char *data,
                            unsigned int len,
                            int binary,
                            struct ai_dialogue_event *out_event,
                            unsigned char **owned_data);

    /*
     * 追加 WebSocket 握手 header。可选。
     * 鉴权放 HTTP header 的厂家实现它，例如 Authorization Bearer。
     * 鉴权放 URL query、start 包或不需要额外 header 时填 NULL。
     */
    int (*append_ws_headers)(struct ai_client *client,
                             struct lws *wsi,
                             unsigned char **p,
                             unsigned char *end);

    /* 追加厂家 HTTP header；实现后替代默认 Authorization: Bearer 行为。 */
    int (*append_http_headers)(struct ai_client *client,
                               ai_http_header_add_fn add,
                               void *ctx);

    /*
     * 构造 HTTP vision/text 请求 body。HTTP provider 按需实现。
     * image 可为空，prompt 是文本提示词；out_body 返回 malloc/cJSON 分配出的请求体字符串，
     * HTTP 层发送后负责释放。
     */
    int (*build_vision_http_body)(struct ai_client *client,
                                  const struct ai_image_input *image,
                                  const char *prompt,
                                  char **out_body);

    /*
     * 解析非流式 HTTP 响应。HTTP provider 按需实现。
     * json 是完整响应体；out->text/out->raw_json 由 provider 分配，调用方用 ai_vision_result_free() 释放。
     */
    int (*parse_vision_http_response)(struct ai_client *client,
                                      const char *json,
                                      struct ai_vision_result *out);

    /*
     * 解析 HTTP streaming 的单条事件。HTTP 流式 provider 按需实现。
     * json 是一条流式事件；out_text 返回本次增量文本，out_done 表示流是否结束。
     */
    int (*parse_vision_http_stream_event)(struct ai_client *client,
                                          const char *json,
                                          char **out_text,
                                          int *out_done);
};

/*
 * 底层 AI 请求 / 对话上下文。
 *
 * 公开类型名暂时保留为 ai_client，是为了兼容已有调用。它实际承担的是
 * ai_dialogue_session 的角色：保存 provider 配置、HTTP / WS 状态、发送队列、接收拼包缓存
 * 和事件回调。它不采集音频，也不决定一轮语音什么时候开始 / 结束。
 *
 * 产品语音流程优先用 ai_realtime_*；只有 HTTP 识图 / 文本、自定义 WebSocket 流程、
 * provider 测试才建议直接操作 ai_client。
 */
struct ai_client {
    char *provider_name;
    char *dialogue_ws_url;
    char *vision_http_url;
    char *api_key;
    char *model;
    char *provider_params;
    struct ai_turn_detection_config turn_detection;
    int has_turn_detection;
    int timeout_ms;
    int ping_interval_ms;
    int auto_reconnect;
    int reconnect_max_attempts;
    enum ai_http_backend http_backend;

    const struct ai_provider_ops *provider;

    struct lws_context *ws_context;
    struct lws *ws_wsi;
    /* Written by the poll thread inside lws callbacks, read by the upload thread
     * (ai_dialogue_can_queue). volatile so the upload thread always re-reads it. */
    volatile enum ai_ws_state ws_state;
    ai_dialogue_cb dialogue_cb;
    void *dialogue_user;
    struct ai_error_detail last_error;

    /* 自动重连状态：need_reconnect 由 ws 回调在掉线时置位，由 poll() 在
     * lws_service 之外发起真正的连接动作，避免回调嵌套。
     * disconnect_requested 表示用户调用 ai_dialogue_close() 主动关闭，
     * 此时 framework 不会再发起自动重连，直到下一次显式 connect()。 */
    int reconnect_need;
    int disconnect_requested;
    int reconnect_context_destroying;
    unsigned int reconnect_attempts;
    unsigned int reconnect_after_ms;
    unsigned int reconnect_backoff_ms;
    unsigned int reconnect_connected_ms;

    /* Keepalive PING must be emitted from the WRITEABLE callback, never straight
     * from the TIMER callback: writing mid-way through a fragmented data frame
     * corrupts the stream. TIMER only sets this and requests a writable. */
    int ping_pending;

    struct ai_ws_message ws_queue[AI_WS_QUEUE_DEPTH];
    OS_MUTEX ws_queue_mutex;
    int ws_queue_mutex_initialized;

    /* Serializes the ws_context pointer between the upload thread (which calls
     * lws_cancel_service on it) and the poll thread (which destroys/recreates it
     * on reconnect/close). Held only around the pointer read/swap, never across
     * lws_context_destroy() itself. */
    OS_MUTEX ws_ctx_mutex;
    int ws_ctx_mutex_initialized;
    int ws_queue_head;
    int ws_queue_tail;
    int ws_queue_count;

    /* WebSocket 收包装配缓冲：用于把 lws 拆开的分片重新拼成完整一帧再交给 provider 解析。 */
    unsigned char *ws_rx_buf;
    unsigned int ws_rx_len;
    unsigned int ws_rx_cap;
    int ws_rx_binary;
};

const struct ai_provider_ops *ai_provider_find(const char *name);
int ai_provider_validate(const struct ai_provider_ops *provider, const char **reason);

void *ai_malloc_large(size_t size);
void *ai_realloc_large(void *ptr, size_t size);
void ai_free_large(void *ptr);
char *ai_strdup(const char *s);
char *ai_strndup(const char *s, size_t len);
/* strdup variant that allocates from the large/PSRAM heap. Use this (or
 * ai_malloc_large / ai_base64_decode) for any buffer handed back through
 * parse_ws_message()'s owned_data, because the framework releases owned_data
 * with ai_free_large(). Mixing plain malloc/ai_strdup with ai_free_large is a
 * cross-heap free (heap corruption) once TCFG_PSRAM_DEV_ENABLE is on. */
char *ai_strdup_large(const char *s);
void ai_free_string(char **s);

void ai_ws_message_free(struct ai_ws_message *msg);
int ai_ws_message_set_text(struct ai_ws_message *msg, const char *text);
int ai_ws_message_set_binary(struct ai_ws_message *msg, const unsigned char *data, unsigned int len);
int ai_ws_queue_push(struct ai_client *client, struct ai_ws_message *msg);
int ai_ws_queue_push_optional(struct ai_client *client, struct ai_ws_message *msg);
struct ai_ws_message ai_ws_queue_pop(struct ai_client *client);
void ai_ws_queue_clear(struct ai_client *client);
int ai_ws_queue_pending(struct ai_client *client);
int ai_ws_rx_append(struct ai_client *client, const void *data, unsigned int len, int binary);
void ai_ws_rx_reset(struct ai_client *client);
void ai_ws_rx_release(struct ai_client *client);
void ai_client_clear_error(struct ai_client *client);
void ai_client_set_error(struct ai_client *client,
                         enum ai_error_stage stage,
                         int error_code,
                         int transport_code,
                         const char *message,
                         const char *raw_response);
void ai_dialogue_emit_error(struct ai_client *client,
                            enum ai_error_stage stage,
                            int error_code,
                            int transport_code,
                            const char *message,
                            const char *raw_response);
void ai_dialogue_emit(struct ai_client *client,
                      enum ai_dialogue_event_type type,
                      const char *text,
                      const unsigned char *data,
                      unsigned int data_len,
                      const char *raw_json,
                      int error_code);
int ai_ws_append_bearer_auth(struct ai_client *client,
                             struct lws *wsi,
                             unsigned char **p,
                             unsigned char *end);

int ai_base64_encode(const unsigned char *data, unsigned int len, char **out);
int ai_base64_decode(const char *text, unsigned char **out, unsigned int *out_len);

#endif
