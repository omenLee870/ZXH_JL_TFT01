#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".smart_voice_config.data.bss")
#pragma data_seg(".smart_voice_config.data")
#pragma const_seg(".smart_voice_config.text.const")
#pragma code_seg(".smart_voice_config.text")
#endif
/*****************************************************************
>file name : smart_voice_config.c
>author : lichao
>create time : Mon 01 Nov 2021 11:18:03 AM CST
*****************************************************************/
#include "app_config.h"
#include "typedef.h"
#include "smart_voice.h"
#include "asr/jl_kws.h"
#include "tech_lib/kws_unified_api.h"

#if ((defined TCFG_AUDIO_ASR_DEVELOP) && (TCFG_AUDIO_ASR_DEVELOP == ASR_CFG_AIS))
const int config_lp_vad_enable = CONFIG_VAD_PLATFORM_SUPPORT_EN;
const int config_jl_audio_kws_enable = 0;
const int config_aispeech_asr_enable = 1;
const int config_user_asr_enable = 0;
const int config_audio_kws_event_enable = 1;
#elif ((defined TCFG_AUDIO_ASR_DEVELOP) && (TCFG_AUDIO_ASR_DEVELOP == ASR_CFG_USER_DEFINED))
const int config_lp_vad_enable = CONFIG_VAD_PLATFORM_SUPPORT_EN;
const int config_jl_audio_kws_enable = 0;
const int config_aispeech_asr_enable = 0;
const int config_user_asr_enable = 1;
const int config_audio_kws_event_enable = 1;
#elif (TCFG_SMART_VOICE_ENABLE)
const int config_lp_vad_enable = CONFIG_VAD_PLATFORM_SUPPORT_EN;
const int config_jl_audio_kws_enable = 1; /*KWS 使能*/
const int config_aispeech_asr_enable = 0;
const int config_user_asr_enable = 0;
const int config_audio_kws_event_enable = 1;
#else
const int config_lp_vad_enable = 0;
const int config_jl_audio_kws_enable = 0;
const int config_aispeech_asr_enable = 0;
const int config_user_asr_enable = 0;
const int config_audio_kws_event_enable = 0;
#endif

const int config_audio_nn_vad_enable = 0;

void get_kws_api(struct kws_multi_keyword_model *api)
{
#if (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_CH)
    GET_KWS_FUNC_API *kws_func = get_kws_func_api(KWS_TYPE_PHONE_CALL); //¶ú»ú
#elif (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_FAR_CH)
    GET_KWS_FUNC_API *kws_func = get_kws_func_api(KWS_TYPE_FAR_KEYWORD);//ÒôÏä
#elif (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_WATCH)
    GET_KWS_FUNC_API *kws_func = get_kws_func_api(KWS_TYPE_SMART_WATCH);//ÊÖ±í
#endif

    if (!api || !kws_func) {
        return;
    }
    api->mem_dump = kws_func->get_heap_size;
    api->init = kws_func->init;
    api->reset = kws_func->reset;
    api->process = kws_func->process;
    api->free = kws_func->free;
}

/*重新封装远场语音识别库使用的系统函数接口*/
u32 sys_cfg_read_otp(u32 id, u8 *buf, u32 len)
{
    extern u32 syscfg_read_otp(u32 id, u8 * buf, u32 len);
    return syscfg_read_otp(id, buf, len);
}

