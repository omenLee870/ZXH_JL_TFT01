/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块与芯片系列相关配置
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"

/*
 *******************************************************************
 *						Audio DAC Config
 *******************************************************************
 */
const int config_audio_dac_mute_timeout = 70;   //单位：ms
#ifndef TCFG_AUDIO_DAC_PA_MODE
#define TCFG_AUDIO_DAC_PA_MODE 0
#endif
const int config_audio_dac_pa_mode = TCFG_AUDIO_DAC_PA_MODE;

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
const int config_audio_dac_mix_enable = 1;

//***********************
//*		AAC Codec       *
//***********************
const int AAC_DEC_MP4A_LATM_ANALYSIS = 1;
const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
const int WTS_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
const int MP3_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;

//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_platform_cfg *platform_cfg)
{
    mic_param->mic_mode      = platform_cfg->mic_mode;
    mic_param->mic_ain_sel   = platform_cfg->mic_ain_sel;
    mic_param->mic_bias_sel  = platform_cfg->mic_bias_sel;
    mic_param->mic_bias_rsel = platform_cfg->mic_bias_rsel;
    mic_param->mic_dcc       = platform_cfg->mic_dcc;
    mic_param->mic_dcc_en    = platform_cfg->mic_dcc_en;
    mic_param->cclk_en       = platform_cfg->cclk_en;
}
void audio_linein_param_fill(struct linein_open_param *linein_param, const struct adc_platform_cfg *platform_cfg)
{
    linein_param->linein_mode    = platform_cfg->mic_mode;
    linein_param->linein_ain_sel = platform_cfg->mic_ain_sel;
    linein_param->linein_dcc     = platform_cfg->mic_dcc;
    linein_param->linein_dcc_en  = platform_cfg->mic_dcc_en;
    linein_param->cclk_en        = platform_cfg->cclk_en;
}
//***********************
//*		Sync Msg    *
//***********************
const int config_sync_msg_latch_type = 1;    //锁存方式控制 0：同步锁存，1：异步锁存
const int config_sync_msg_latch_en = 0;//播放同步使用硬件锁存 0:关闭 1：使能


u16 dac_dvol_max_query(void)
{
    u16 DAC_0dB = 16384;
    if ((TCFG_AUDIO_DAC_MODE == DAC_MODE_SINGLE) || (TCFG_AUDIO_DAC_MODE == DAC_MODE_DIFF) || (TCFG_AUDIO_DAC_MODE == DAC_MODE_VCMO_PA)) {
        DAC_0dB = 16959;
    } else if (TCFG_AUDIO_DAC_MODE == DAC_MODE_VCMO) {
        DAC_0dB = 14384;
    }
    return DAC_0dB;
}

//***********************
//*		Audio Clock     *
//***********************
int aud_clock_alloc(const char *name, u32 clk)
{
    y_printf("aud_clock_alloc:%s(%d)\n", name, clk);
    return clock_alloc(name, clk);
}

int aud_clock_free(char *name)
{
    y_printf("aud_clock_free:%s\n", name);
    return clock_free(name);

}
