#ifndef AI_AUDIO_SINK_H
#define AI_AUDIO_SINK_H

#ifdef __cplusplus
extern "C" {
#endif

enum ai_audio_sink_format {
    AI_AUDIO_SINK_PCM16 = 0,
    AI_AUDIO_SINK_OPUS,
    AI_AUDIO_SINK_G711A,
    AI_AUDIO_SINK_G711U,
    AI_AUDIO_SINK_MP3,
    AI_AUDIO_SINK_AAC,
};

/* 下行音频播放配置；默认实现会把收到的数据喂给 audio_server 解码。 */
struct ai_audio_sink_config {
    enum ai_audio_sink_format format;
    unsigned int sample_rate;
    unsigned short channels;
    unsigned short bits_per_sample;
    unsigned int frame_bytes;
    unsigned char volume;
    unsigned int buffer_size;
    unsigned int prebuffer_bytes; /* PCM prebuffer before playback; 0 keeps the default. */
};

/* 平台可替换的播放 sink；用于把 framework 收到的音频落到具体平台输出。
 * drain 可选：返回 0 表示已排空或调用方不要求阻塞，> 0 表示已等待但仍有残留，
 * < 0 表示底层错误。null 时框架认为该 sink 没有缓存，直接走 close。 */
struct ai_audio_sink_ops {
    int (*open)(const struct ai_audio_sink_config *cfg, void **ctx);
    int (*write)(void *ctx, const unsigned char *data, unsigned int len);
    int (*drain)(void *ctx, int timeout_ms);
    int (*close)(void *ctx);
};

/* 注册自定义播放 sink；传 NULL 后会恢复使用默认实现。 */
int ai_audio_sink_register_ops(const struct ai_audio_sink_ops *ops);
/* 打开下行音频 sink，后续 write 的数据会按 cfg 指定格式解码播放。 */
int ai_audio_sink_open(const struct ai_audio_sink_config *cfg);
/* 向已打开的 sink 写入一段音频数据。 */
int ai_audio_sink_write(const unsigned char *data, unsigned int len);
/* 等待 sink 缓冲数据被消费完。timeout_ms <= 0 时只查询当前状态不等待。
 * 返回 0 表示已排空，> 0 表示等待超时仍有数据，< 0 表示出错或 sink 未打开。
 * 调用方应在 close 前显式 drain，避免最后一段语音被截断。 */
int ai_audio_sink_drain(int timeout_ms);
/* 关闭当前 sink 并回收内部资源。 */
int ai_audio_sink_close(void);

#ifdef __cplusplus
}
#endif

#endif
