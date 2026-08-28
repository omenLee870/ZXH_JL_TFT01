#ifndef AI_AUDIO_URL_PLAYER_H
#define AI_AUDIO_URL_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

enum ai_audio_url_event {
    AI_AUDIO_URL_EVENT_START = 0,
    AI_AUDIO_URL_EVENT_END,
    AI_AUDIO_URL_EVENT_ERROR,
};

typedef void (*ai_audio_url_player_cb)(const char *url,
                                       enum ai_audio_url_event event,
                                       int error,
                                       void *user);

struct ai_audio_url_player_config {
    unsigned char volume;
    unsigned int cbuf_size;
    unsigned int start_play_threshold;
    unsigned int seek_threshold;
    unsigned int timeout_ms;
    ai_audio_url_player_cb cb;
    void *user;
};

int ai_audio_url_player_init(const struct ai_audio_url_player_config *cfg);
int ai_audio_url_player_enqueue(const char *url);
int ai_audio_url_player_stop(void);
int ai_audio_url_player_is_running(void);

#ifdef __cplusplus
}
#endif

#endif
