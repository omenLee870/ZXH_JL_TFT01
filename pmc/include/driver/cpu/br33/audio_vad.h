#ifndef _AUDIO_VAD_H_
#define _AUDIO_VAD_H_

#include "includes.h"
struct vad_mic_platform_data {
    //digital
    u32 sr;
    s16 vco_dc_sys;
    u16 dig_pvth;           //AVAD数字阈值，16bit
    u16 dig_nvth;           //AVAD数字阈值，16bit
    u8 ana_vth;             //AVAD模拟阈值，4bit
    u8 pfm_mode;            // 性能模式， 0：高性能模式     1: 低功耗模式
    u8 mic_dat_sel;		    // MIC数据来源选择
    u8 dcc_en;
    u8 dcc_lvl;             // 15: bypass
    //analog
    u8 mode;                // 模拟通道模式, 单端隔直 / 差分隔直
    u8 clk_mode;			// 时钟来源选择, 数字单端时钟 / 差分晶振时钟 / 数字晶振时钟
    u8 pga_gain;            // VAD PGA增益, -6~39dB, 3dB/step
    u8 micldo_pwr;          // micldo 输出使能
    u8 micldo_vsel;         //
    u8 micbias_inside;		//
    u8 micbias_rsel;        //
    u8 vcm_lvl;             //与主系统vcm_lvl一致
    //clk
    u8 dvad_clk_src;
    //other
    u8 adc_test;
};

void audio_vad_common_analog_open(struct vad_mic_platform_data *data);
void audio_vad_common_analog_close();
void audio_dvad_analog_open(struct vad_mic_platform_data *data);
void audio_dvad_digital_open(struct vad_mic_platform_data *data);
void audio_dvad_close();
void audio_avad_analog_open(struct vad_mic_platform_data *data);
void audio_avad_digital_open(struct vad_mic_platform_data *data);
void audio_avad_close();
void audio_avad_vth_set(u8 vth);
#endif
