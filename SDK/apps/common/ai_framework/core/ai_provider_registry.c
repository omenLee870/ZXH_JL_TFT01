#include "ai_internal.h"

#include <string.h>

#if AI_PROVIDER_DEMO_ENABLE
extern const struct ai_provider_ops ai_provider_demo;
#endif
extern const struct ai_provider_ops ai_provider_duer;
extern const struct ai_provider_ops ai_provider_glm;
extern const struct ai_provider_ops ai_provider_volc_ark;
extern const struct ai_provider_ops ai_provider_ali_realtime;

struct ai_provider_alias {
    const char *name;
    const struct ai_provider_ops *provider;
};

static const struct ai_provider_alias ai_provider_table[] = {
#if AI_PROVIDER_DEMO_ENABLE
    {"demo", &ai_provider_demo},
    {"generic_json", &ai_provider_demo},
#endif
    {"duer", &ai_provider_duer},
    {"baidu_duer", &ai_provider_duer},
    {"glm", &ai_provider_glm},
    {"zhipu", &ai_provider_glm},
    {"bigmodel", &ai_provider_glm},
    {"volc", &ai_provider_volc_ark},
    {"volc_ark", &ai_provider_volc_ark},
    {"ark", &ai_provider_volc_ark},
    {"doubao", &ai_provider_volc_ark},
    {"ali", &ai_provider_ali_realtime},
    {"aliyun", &ai_provider_ali_realtime},
    {"ali_realtime", &ai_provider_ali_realtime},
    {"qwen_realtime", &ai_provider_ali_realtime},
};

const struct ai_provider_ops *ai_provider_find(const char *name)
{
    int i;

#if AI_PROVIDER_DEMO_ENABLE
    if (!name) {
        name = "demo";
    }
#else
    if (!name) {
        name = "glm";
    }
#endif

    for (i = 0; i < (int)(sizeof(ai_provider_table) / sizeof(ai_provider_table[0])); i++) {
        if (strcmp(name, ai_provider_table[i].name) == 0) {
            return ai_provider_table[i].provider;
        }
    }

    return NULL;
}

static int ai_provider_invalid(const char **reason, const char *message)
{
    if (reason) {
        *reason = message;
    }
    return AI_ERR_UNSUPPORTED;
}

int ai_provider_validate(const struct ai_provider_ops *provider, const char **reason)
{
    if (reason) {
        *reason = NULL;
    }
    if (!provider || !provider->name || !provider->name[0]) {
        return ai_provider_invalid(reason, "missing provider name");
    }
    if (!(provider->caps & (AI_PROVIDER_CAP_IMAGE_TO_TEXT | AI_PROVIDER_CAP_VOICE_DIALOGUE))) {
        return ai_provider_invalid(reason, "no supported capability");
    }
    if (provider->caps & AI_PROVIDER_CAP_VOICE_DIALOGUE) {
        if (!provider->voice) {
            return ai_provider_invalid(reason, "voice capability missing profile");
        }
        if (!provider->build_audio || !provider->parse_ws_message) {
            return ai_provider_invalid(reason, "voice capability missing build_audio/parse_ws_message");
        }
        if (!provider->voice->audio_chunk_size || !provider->voice->sample_rate ||
            !provider->voice->channels || !provider->voice->bits_per_sample) {
            return ai_provider_invalid(reason, "invalid voice format/profile");
        }
        if (provider->voice->flow_mode != AI_VOICE_FLOW_LONG_STREAM &&
            provider->voice->flow_mode != AI_VOICE_FLOW_SINGLE_TURN) {
            return ai_provider_invalid(reason, "invalid voice flow_mode");
        }
        if (provider->voice->connect_polls <= 0) {
            return ai_provider_invalid(reason, "voice connect_polls must be > 0");
        }
        if (provider->voice->response_polls <= 0) {
            return ai_provider_invalid(reason, "voice response_polls must be > 0");
        }
        if (provider->is_ready_event && provider->voice->ready_polls <= 0) {
            return ai_provider_invalid(reason, "is_ready_event requires ready_polls > 0");
        }
    }
    if (provider->caps & AI_PROVIDER_CAP_IMAGE_TO_TEXT) {
        if (!provider->build_vision_http_body) {
            return ai_provider_invalid(reason, "HTTP capability missing request builder");
        }
        if (!provider->parse_vision_http_response && !provider->parse_vision_http_stream_event) {
            return ai_provider_invalid(reason, "HTTP capability missing response parser");
        }
    }
    return AI_OK;
}
