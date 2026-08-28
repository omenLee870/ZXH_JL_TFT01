#ifndef AI_PROVIDER_H
#define AI_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

enum ai_status {
    AI_OK = 0,
    AI_ERR_PARAM = -1,
    AI_ERR_NOMEM = -2,
    AI_ERR_NET = -3,
    AI_ERR_PARSE = -4,
    AI_ERR_UNSUPPORTED = -5,
    AI_ERR_STATE = -6,
};

#define AI_ERROR_PROVIDER_MAX 32
#define AI_ERROR_MESSAGE_MAX 128
#define AI_ERROR_RAW_MAX 256

enum ai_error_stage {
    AI_ERROR_STAGE_NONE = 0,
    AI_ERROR_STAGE_CONFIG,
    AI_ERROR_STAGE_CONNECT,
    AI_ERROR_STAGE_TLS,
    AI_ERROR_STAGE_AUTH,
    AI_ERROR_STAGE_SEND,
    AI_ERROR_STAGE_RECEIVE,
    AI_ERROR_STAGE_PARSE,
    AI_ERROR_STAGE_PROVIDER,
    AI_ERROR_STAGE_HTTP,
    AI_ERROR_STAGE_TIMEOUT,
    AI_ERROR_STAGE_CLOSED,
};

struct ai_error_detail {
    enum ai_error_stage stage;
    int error_code;
    int transport_code;
    int raw_truncated;
    char provider[AI_ERROR_PROVIDER_MAX];
    char message[AI_ERROR_MESSAGE_MAX];
    char raw_response[AI_ERROR_RAW_MAX];
};

enum ai_image_format {
    AI_IMAGE_JPEG = 0,
    AI_IMAGE_YUV = 1,
};

enum ai_audio_format {
    AI_AUDIO_PCM16 = 0,
    AI_AUDIO_OPUS,
    AI_AUDIO_G711A,
    AI_AUDIO_G711U,
    AI_AUDIO_VENDOR_BINARY,
};

enum ai_turn_detection_type {
    AI_TURN_DETECTION_SERVER_VAD = 0,
    AI_TURN_DETECTION_SEMANTIC_VAD = 1,
};

struct ai_turn_detection_config {
    enum ai_turn_detection_type type;
    int has_threshold;
    float threshold;
    /* Zero means omit the optional duration and let the provider use its default. */
    unsigned int silence_duration_ms;
    unsigned int idle_timeout_ms;
    int create_response;
};

enum ai_http_backend {
    AI_HTTP_BACKEND_CURL = 0,
    AI_HTTP_BACKEND_LWS,
};

/* 统一事件类型，既给WebSocket对话用，也复用到HTTP流式输出。 */
enum ai_dialogue_event_type {
    AI_DIALOGUE_CONNECTED = 0,
    AI_DIALOGUE_TEXT_DELTA,
    AI_DIALOGUE_TEXT_DONE,
    AI_DIALOGUE_TRANSCRIPT,
    AI_DIALOGUE_AUDIO_DELTA,
    AI_DIALOGUE_AUDIO_DONE,
    AI_DIALOGUE_AUDIO_URL,
    AI_DIALOGUE_FUNCTION_CALL,
    AI_DIALOGUE_SPEECH_STARTED,
    AI_DIALOGUE_SPEECH_STOPPED,
    AI_DIALOGUE_DONE,
    AI_DIALOGUE_ERROR,
    AI_DIALOGUE_CLOSED,
    AI_DIALOGUE_VENDOR_EVENT,
};

/* 创建 ai_client 时使用的统一配置；字符串由调用方提供，client 内部会复制保存。
 * auto_reconnect：默认 0，关闭对话掉线后只发出 CLOSED；置 1 后 framework 会按
 *                指数退避（1s → 2s → ... 上限 30s）有限重连，由 ai_dialogue_poll() 推进。
 *                ai_dialogue_close() 视为用户主动断开，会暂停自动重连，
 *                直到下次显式调用 ai_dialogue_connect()。
 *                直接使用 ai_client 时，provider 会话初始化仍由调用方恢复。
 * reconnect_max_attempts：单次掉线最多重试次数；<= 0 时使用默认值 5。 */
struct ai_config {
    const char *provider_name;
    const char *dialogue_ws_url;
    const char *vision_http_url;
    const char *api_key;
    const char *model;
    const char *provider_params;
    const struct ai_turn_detection_config *turn_detection;
    int timeout_ms;
    int ping_interval_ms;
    int auto_reconnect;
    int reconnect_max_attempts;
    enum ai_http_backend http_backend;
};

struct ai_image_input {
    enum ai_image_format format;
    const unsigned char *data;
    unsigned int size;
    unsigned int width;
    unsigned int height;
};

struct ai_audio_input {
    enum ai_audio_format format;
    const unsigned char *data;
    unsigned int size;
    unsigned int sample_rate;
    unsigned short channels;
    unsigned short bits_per_sample;
};

/* 对话/流式回调统一事件；指针字段仅在回调期间有效，如需保留请调用方自行复制。 */
struct ai_dialogue_event {
    enum ai_dialogue_event_type type;
    const char *text;
    const char *url;
    const unsigned char *data;
    unsigned int data_len;
    const char *raw_json;
    int error_code;
    const struct ai_error_detail *error;
    const char *const *urls;
    unsigned int url_count;
    const char *vendor_event;
};

typedef void (*ai_dialogue_cb)(const struct ai_dialogue_event *event, void *user);

/* text给上层直接消费，raw_json保留原始响应或原始流事件，便于调试与错误透传。 */
struct ai_vision_result {
    char *text;
    char *raw_json;
};

struct ai_client;

/*
 * ai_client 是底层连接 / 请求句柄，不是产品层实时语音会话。
 *
 * 直接使用 ai_client 的场景：
 *   - HTTP 识图 / 文本：ai_client_create -> ai_vision_recognize；
 *   - 自定义 WebSocket：ai_dialogue_connect / send / poll；
 *   - provider 单元测试或临时验证。
 *
 * 常规语音对话不要直接创建线程读音频再调 ai_dialogue_send_audio，优先用 ai_realtime_*。
 * ai_realtime 会在内部创建 ai_client，并统一管理连接、ready gate、音频上传线程和 commit / close。
 */

/* 创建底层 client 并复制 cfg 里的字符串配置；成功后由 ai_client_destroy() 释放。 */
int ai_client_create(const struct ai_config *cfg, struct ai_client **out);
/* 销毁 client，并一并关闭仍然存在的网络上下文和待发队列。 */
void ai_client_destroy(struct ai_client *client);
/* 给 HTTP 流式输出或后续对话事件注册回调；非流式请求可以不设置。 */
int ai_client_set_dialogue_callback(struct ai_client *client, ai_dialogue_cb cb, void *user);
/* Copies the last structured transport/provider error. stage NONE means no error has been recorded. */
int ai_client_get_last_error(const struct ai_client *client, struct ai_error_detail *out);

/* 建立对话连接并注册事件回调；后续必须持续调用 ai_dialogue_poll() 驱动收发。 */
int ai_dialogue_connect(struct ai_client *client, ai_dialogue_cb cb, void *user);
/* 发送会话开始请求，具体报文格式由 provider 实现。 */
int ai_dialogue_session_start(struct ai_client *client);
/* 发送一段文本输入；调用方传入的 text 不会被长期借用。 */
int ai_dialogue_send_text(struct ai_client *client, const char *text);
/* 发送一段音频输入；音频内容会在内部复制入待发队列。 */
int ai_dialogue_send_audio(struct ai_client *client, const struct ai_audio_input *audio);
/* 发送一张图片输入；图片内容会在内部复制入待发队列。 */
int ai_dialogue_send_image(struct ai_client *client, const struct ai_image_input *image);
/* 提交当前轮输入，通知服务端可以开始出最终回复。 */
int ai_dialogue_commit(struct ai_client *client);
/* 中断当前轮生成或上传流程，是否支持由 provider 决定。 */
int ai_dialogue_interrupt(struct ai_client *client);
/* 请求服务端根据已提交输入生成回复，Manual realtime 模式常用。 */
int ai_dialogue_response_create(struct ai_client *client);
/* 请求结束会话，通常用于长连接场景显式收尾。 */
int ai_dialogue_session_stop(struct ai_client *client);
/* 驱动底层网络事件循环；未持续 poll 时，对话不会继续推进。 */
int ai_dialogue_poll(struct ai_client *client, int timeout_ms);
/* 非阻塞驱动一次底层网络事件循环，用于音频上传循环里轻量泵网络。 */
int ai_dialogue_poll_now(struct ai_client *client);
/* 查询对话发送队列是否还有待写出的消息。 */
int ai_dialogue_pending(struct ai_client *client);
/* 本地关闭对话连接，并触发 CLOSED 事件收尾。 */
int ai_dialogue_close(struct ai_client *client);

/* 执行一次视觉/文本 HTTP 请求；image 可为空，out 里的字符串需由 ai_vision_result_free() 释放。 */
int ai_vision_recognize(struct ai_client *client,
                        const struct ai_image_input *image,
                        const char *prompt,
                        struct ai_vision_result *out);
/* 释放 ai_vision_recognize() 返回的文本和原始响应缓存。 */
void ai_vision_result_free(struct ai_vision_result *result);

/* Offline provider contract and parser fixture check; does not open network or audio devices. */
int ai_provider_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
