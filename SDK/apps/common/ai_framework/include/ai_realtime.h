#ifndef AI_REALTIME_H
#define AI_REALTIME_H

#include "ai_audio_source.h"
#include "ai_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ai_realtime_session;

struct ai_realtime_upload_stats {
    unsigned int bytes;
    unsigned int chunks;
    unsigned int duration_ms;
    int stopped_by_session_end;
    int stopped_by_timeout;
    int source_end;
    int stopped_by_upload_end;
    unsigned int drop_bytes;
    unsigned int drop_frames;
    unsigned int drop_events;
};

/*
 * 长连接会话配置；字符串和 audio_cfg 由调用方持有，session 仅借用。
 *
 * audio_cfg 可选：非 NULL 时由 realtime 在 upload 期间负责录音源生命周期。
 * turn_detection 可选：非 NULL 时由 provider 生成服务端 VAD 配置；NULL 保持手动 turn。
 */
struct ai_realtime_config {
    const char *provider_name;
    const char *api_key;
    const char *dialogue_ws_url;
    const char *model;
    const char *provider_params;
    const struct ai_turn_detection_config *turn_detection;
    const struct ai_audio_source_config *audio_cfg;
    ai_dialogue_cb dialogue_cb;
    void *dialogue_user;
    int timeout_ms;
    int ping_interval_ms;
    /* 默认关闭。开启后恢复完整 provider 会话，不继续已经中断的单次 turn。 */
    int auto_reconnect;
    /* <= 0 使用默认值 5，避免无网络或服务端持续拒绝时无限重试。 */
    int reconnect_max_attempts;
};

/* 创建 realtime session 句柄，但此时还不会真正联网。 */
int ai_realtime_open(const struct ai_realtime_config *cfg,
                     struct ai_realtime_session **out);
/* 启动长连接并等待 provider 进入可交互状态。 */
int ai_realtime_start(struct ai_realtime_session *session);
/* 在已启动会话上发送一段文本输入。 */
int ai_realtime_send_text(struct ai_realtime_session *session, const char *text);
/* 在已启动会话上发送一段音频输入。 */
int ai_realtime_send_audio(struct ai_realtime_session *session,
                           const unsigned char *data,
                           unsigned int len);
/* 在已启动会话上发送一张 JPEG 图片输入。 */
int ai_realtime_send_image(struct ai_realtime_session *session,
                           const unsigned char *data,
                           unsigned int len);
/* 按 provider voice profile 从当前 audio_source 拉流并持续上发。 */
int ai_realtime_upload_audio_source(struct ai_realtime_session *session,
                                    unsigned int max_capture_ms,
                                    struct ai_realtime_upload_stats *stats);
/* 后台启动/停止 audio_source 上传任务；适合产品化实时长连接使用。 */
int ai_realtime_start_upload(struct ai_realtime_session *session,
                             unsigned int max_capture_ms);
int ai_realtime_stop_upload(struct ai_realtime_session *session);
int ai_realtime_upload_running(struct ai_realtime_session *session);
/* 查询最近一次/当前后台上传统计；上传线程运行中时该值会持续刷新。 */
int ai_realtime_get_upload_stats(struct ai_realtime_session *session,
                                 struct ai_realtime_upload_stats *out);
/*
 * 运行一轮 provider 定义的语音交互。LONG_STREAM provider 若声明 commit /
 * response.create hook，框架会在停止采集后按顺序发送并等待本轮完成。
 */
int ai_realtime_run_turn(struct ai_realtime_session *session,
                         unsigned int max_capture_ms,
                         unsigned int wait_response_ms,
                         struct ai_realtime_upload_stats *stats);
/* 提交当前一轮输入，但保留底层长连接不关闭。 */
int ai_realtime_commit(struct ai_realtime_session *session);
/* 请求打断当前生成过程。 */
int ai_realtime_interrupt(struct ai_realtime_session *session);
/* 请求服务端根据已提交输入生成回复。 */
int ai_realtime_response_create(struct ai_realtime_session *session);
/* 驱动网络并等待当前一轮回复完成；连接保持，可继续发起下一轮。 */
int ai_realtime_wait_response(struct ai_realtime_session *session,
                              unsigned int wait_response_ms);
/* 驱动长连接收发循环；应用层需要持续调用。 */
int ai_realtime_poll(struct ai_realtime_session *session, int timeout_ms);
/* 非阻塞驱动一次长连接收发循环，适合音频上传循环中调用。 */
int ai_realtime_poll_now(struct ai_realtime_session *session);
/* 查询 realtime 发送队列是否还有待写出的消息。 */
int ai_realtime_pending(struct ai_realtime_session *session);
/* 关闭当前长连接，但不释放 session 结构本身。 */
int ai_realtime_close(struct ai_realtime_session *session);
/* 释放 ai_realtime_open() 返回的 session 句柄。 */
void ai_realtime_destroy(struct ai_realtime_session *session);

#ifdef __cplusplus
}
#endif

#endif
