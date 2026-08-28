#ifndef AI_AUDIO_SOURCE_H
#define AI_AUDIO_SOURCE_H

/* 录音源配置；未填写的字段由默认实现按平台默认值补齐。 */
struct ai_audio_source_config {
    const char *format;
    int sample_rate;
    int channels;
    int bits_per_sample;
    int volume;
    int channel_bit_map;
    int frame_ms;
    int buffer_size;
    int use_vad;
    int vad_start_threshold;
    int vad_stop_threshold;
};

/* 平台可替换的录音源实现；register 后 framework 会统一走这组回调。 */
struct ai_audio_source_ops {
    int (*start)(const struct ai_audio_source_config *cfg, void **ctx);
    int (*stop)(void *ctx);
    int (*read)(void *ctx, unsigned char *buf, unsigned int len);
    int (*data_len)(void *ctx);
    int (*is_end)(void *ctx);
    int (*is_closed)(void *ctx);
    int (*is_error)(void *ctx);
    int (*is_open)(void *ctx);
    int (*is_speaking)(void *ctx);
    int (*is_speak_stopped)(void *ctx);
};

struct ai_audio_source_drop_stats {
    unsigned int bytes;
    unsigned int frames;
    unsigned int events;
};

/* 注册自定义录音源；传 NULL 后会恢复使用默认实现。 */
int ai_audio_source_register_ops(const struct ai_audio_source_ops *ops);
/* 打开并启动录音源；成功后数据会被持续写入内部缓冲。 */
int ai_audio_source_start(const struct ai_audio_source_config *cfg);
/* 停止录音源并释放内部上下文。 */
int ai_audio_source_stop(void);
/* 从内部缓冲读取编码后的音频数据。 */
int ai_audio_source_read(unsigned char *buf, unsigned int len);
/* 查询当前缓冲区里可读的音频字节数。 */
int ai_audio_source_data_len(void);
/* 查询录音流程是否自然结束。 */
int ai_audio_source_is_end(void);
/* 查询底层录音源是否已经关闭。 */
int ai_audio_source_is_closed(void);
/* 查询录音流程是否遇到错误。 */
int ai_audio_source_is_error(void);
/* 查询录音源是否已经成功打开。 */
int ai_audio_source_is_open(void);
/* 查询当前是否处于检测到说话中的状态。 */
int ai_audio_source_is_speaking(void);
/* 查询 VAD 是否已经判定说话结束。 */
int ai_audio_source_is_speak_stopped(void);
int ai_audio_source_get_drop_stats(struct ai_audio_source_drop_stats *out);

#endif
