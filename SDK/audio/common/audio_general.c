#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_general.data.bss")
#pragma data_seg(".audio_general.data")
#pragma const_seg(".audio_general.text.const")
#pragma code_seg(".audio_general.text")
#endif

#include "system/init.h"
#include "media/audio_general.h"
#include "app_config.h"
#include "media/audio_def.h"
#include "jlstream.h"
#include "uartPcmSender.h"
#include "debug/audio_debug.h"
#include "audio_config_def.h"
#include "scene_update.h"
#include "effects/voiceChanger_api.h"
#include "effects/audio_effects.h"
#include "sync/audio_syncts.h"
#include "codec/audio_midi.h"
#include "audio_general_config.h"
#include "media/audio_src.h"
#include "debug.h"
#include "smart_voice.h"
#include "cvp_v3.h"

//静态检查
_Static_assert(ARRAY_SIZE(mm_module) == (AUD_MODULE_END + 1), "Audio Memory Array Index Error");
/*音频配置在线调试配置*/
const int config_audio_cfg_debug_online = TCFG_CFG_TOOL_ENABLE;
#if TCFG_USER_TWS_ENABLE
const int config_media_tws_en = 1;
#else
const int config_media_tws_en = 0;
#endif

const int config_audio_dac_ng_debug = 0;
const int config_audio_dac_enable = TCFG_DAC_NODE_ENABLE;

/* 16bit数据流中也存在32bit位宽数据的处理 */
const int config_ch_adapter_32bit_enable = 1;
const int config_mixer_32bit_enable = 1;
const int config_jlstream_fade_32bit_enable = 1;
const int CONFIG_JLSTREAM_FADE_TIME_MSEC = 100;
const int config_jlstream_fade_in_silence_check_enable = 1;
const int config_peak_rms_32bit_enable = 1;
const int config_audio_vocal_track_synthesis_32bit_enable = 1;
const int config_audio_invert_32bit_enable = 1;

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L)
const int config_audio_dac_channel_left_enable = 1;
const int config_audio_dac_channel_right_enable = 0;
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_R)
const int config_audio_dac_channel_left_enable = 0;
const int config_audio_dac_channel_right_enable = 1;
#else
const int config_audio_dac_channel_left_enable = 1;
const int config_audio_dac_channel_right_enable = 1;
#endif
#ifdef TCFG_AUDIO_DAC_POWER_ON_MODE
const int config_audio_dac_power_on_mode = TCFG_AUDIO_DAC_POWER_ON_MODE;
#endif
#ifdef TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE
const int config_audio_dac_power_off_lite = TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE;
#else
const int config_audio_dac_power_off_lite = 0;
#endif
#if TCFG_CFG_TOOL_ENABLE
const int config_audio_cfg_online_enable = 1;
#else
const int config_audio_cfg_online_enable = 0;
#endif

const int config_audio_dac_dma_buf_realloc_enable = 0;
#ifdef TCFG_DAC_POWER_MODE
const int config_audio_dac_power_mode = TCFG_DAC_POWER_MODE;
#endif
const int config_audio_dac_underrun_time_lea = 100; //le_audio 音频流欠载检测时间(us); 低延时模式建议不要超过100us
#ifndef TCFG_AUDIO_DAC_BUFFER_TIME_MS
#define TCFG_AUDIO_DAC_BUFFER_TIME_MS 50
#endif
const int config_audio_dac_buffer_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS;
const int config_audio_dac_fade_out_time = 10; //DAC suspend/stop数据淡出时间

const int config_audio_gain_enable = TCFG_GAIN_NODE_ENABLE;
#ifndef TCFG_SPLIT_GAIN_NODE_ENABLE
#define TCFG_SPLIT_GAIN_NODE_ENABLE 0
#endif
const int config_audio_split_gain_enable = TCFG_SPLIT_GAIN_NODE_ENABLE;
const int config_audio_stereomix_enable = TCFG_STEROMIX_NODE_ENABLE;
#ifdef TCFG_SPLIT_GAIN_FADE_ENABLE
const int config_audio_split_gain_fade_enable = TCFG_SPLIT_GAIN_FADE_ENABLE;
#else
const int config_audio_split_gain_fade_enable = 0;
#endif
#ifdef TCFG_FADE_IN_SILENCE_CHECK_ENABLE
const int config_audio_fade_in_slience_check_enable = TCFG_FADE_IN_SILENCE_CHECK_ENABLE;
#else
const int config_audio_fade_in_slience_check_enable = 0;
#endif

//ADC Enable Config
const int config_audio_adc0_enable = TCFG_ADC0_ENABLE;
#ifdef TCFG_ADC1_ENABLE
const int config_audio_adc1_enable = TCFG_ADC1_ENABLE;
#else
const int config_audio_adc1_enable = 0;
#endif
#ifdef TCFG_ADC2_ENABLE
const int config_audio_adc2_enable = TCFG_ADC2_ENABLE;
#else
const int config_audio_adc2_enable = 0;
#endif
#ifdef TCFG_ADC3_ENABLE
const int config_audio_adc3_enable = TCFG_ADC3_ENABLE;
#else
const int config_audio_adc3_enable = 0;
#endif
#ifdef TCFG_ADC4_ENABLE
const int config_audio_adc4_enable = TCFG_ADC4_ENABLE;
#else
const int config_audio_adc4_enable = 0;
#endif
#ifdef TCFG_ADC5_ENABLE
const int config_audio_adc5_enable = TCFG_ADC5_ENABLE;
#else
const int config_audio_adc5_enable = 0;
#endif
#ifdef TCFG_ADC6_ENABLE
const int config_audio_adc6_enable = TCFG_ADC6_ENABLE;
#else
const int config_audio_adc6_enable = 0;
#endif
const int config_audio_adc7_enable = 0;

//ADC input Mode:Single-Ended/Differential/Single-Ended Capless
const int config_audio_adc0_input_mode = TCFG_ADC0_MODE;
#ifdef TCFG_ADC1_MODE
const int config_audio_adc1_input_mode = TCFG_ADC1_MODE;
#else
const int config_audio_adc1_input_mode = 0;
#endif
#ifdef TCFG_ADC2_MODE
const int config_audio_adc2_input_mode = TCFG_ADC2_MODE;
#else
const int config_audio_adc2_input_mode = 0;
#endif
#ifdef TCFG_ADC3_MODE
const int config_audio_adc3_input_mode = TCFG_ADC3_MODE;
#else
const int config_audio_adc3_input_mode = 0;
#endif
#ifdef TCFG_ADC4_MODE
const int config_audio_adc4_input_mode = TCFG_ADC4_MODE;
#else
const int config_audio_adc4_input_mode = 0;
#endif
#ifdef TCFG_ADC5_MODE
const int config_audio_adc5_input_mode = TCFG_ADC5_MODE;
#else
const int config_audio_adc5_input_mode = 0;
#endif
#ifdef TCFG_ADC6_MODE
const int config_audio_adc6_input_mode = TCFG_ADC6_MODE;
#else
const int config_audio_adc6_input_mode = 0;
#endif
const int config_audio_adc7_input_mode = 0;
/*
 *******************************************************************
 *						Audio Debug Config
 *******************************************************************
 */
const int config_esco_media_stream_dump_enable = 0;	//ESCO Media Stream Dump
const int config_a2dp_media_stream_dump_enable = 0;	//A2dp Media Stream Dump
const int config_lea_media_stream_dump_enable = 0;	//LE Audio Media Stream Dump

/*
 *******************************************************************
 *						Audio CVP Config
 *******************************************************************
 */
const int config_audio_cvp_ref_source = 1;/*0:DAC Internal 1:External*/
#if ((defined(TCFG_AUDIO_SIDETONE_ENABLE) && TCFG_AUDIO_SIDETONE_ENABLE) || \
    (defined(TCFG_HOWLING_AHS_NODE_ENABLE) && TCFG_HOWLING_AHS_NODE_ENABLE) || \
        (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC && CONFIG_VAD_PLATFORM_SUPPORT_EN))
const int config_audio_cvp_ref_ch_recognize_enable = 1;
#else
const int config_audio_cvp_ref_ch_recognize_enable = 0;
#endif

/*
 *******************************************************************
 *						Audio LLNS_DNS Config
 *******************************************************************
 */
#define LLNS_DNS_DELAY_2FRAME      1 //使用2帧延时模式，仅48k采样率有效

#ifdef TCFG_AUDIO_ADC_SAMPLE_RATE
//若支持固定ADC采样率，使用ADC采样率，否则使用全局采样率。亦可自行配置采样率，需注意仅支持32k、48k
#define LLNS_DNS_SR         TCFG_AUDIO_ADC_SAMPLE_RATE
#else
#define LLNS_DNS_SR         TCFG_AUDIO_GLOBAL_SAMPLE_RATE
#endif

#if (TCFG_LLNS_DNS_NODE_ENABLE && ((LLNS_DNS_SR != 32000) && (LLNS_DNS_SR != 48000)))
#error "llns_dns only support 32k or 48k"
#endif

/*
 *          |   32k   |        48k
 * ---------|---------|---------|-----------------
 * mode 0   |  BIT(0) | BIT(1)  | BIT(3) [2帧延时]
 * ---------|---------|---------|-----------------
 * mode 1   |  BIT(4) | BIT(5)  | BIT(6) [2帧延时]
 * ---------|---------|---------|-----------------
 * mode 2   |   N/A   |       BIT(7)
 */

#ifndef TCFG_LLNS_DNS_MODE
#define TCFG_LLNS_DNS_MODE         0 //由工具导出
#endif

#if (LLNS_DNS_SR == 32000)

#if (TCFG_LLNS_DNS_MODE == 0)
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR32K_MODE0
#define LLNS_DNS_PARAM_0        (0.0f)
#define LLNS_DNS_PARAM_1        (0.05f)
#elif (TCFG_LLNS_DNS_MODE == 1)
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR32K_MODE1
#define LLNS_DNS_PARAM_0        (0.1f)
#define LLNS_DNS_PARAM_1        (0.5f)
#elif (TCFG_LLNS_DNS_MODE == 2)
#error "llns dns not support sr 32k mode2"
#else
#define LLNS_TABLE_SELECT       0
#endif

#else

#if (TCFG_LLNS_DNS_MODE == 0)
#if LLNS_DNS_DELAY_2FRAME
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR48K_MODE0_2FRAME
#else
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR48K_MODE0
#endif
#define LLNS_DNS_PARAM_0        (0.0f)
#define LLNS_DNS_PARAM_1        (0.05f)
#elif (TCFG_LLNS_DNS_MODE == 1)
#if LLNS_DNS_DELAY_2FRAME
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR48K_MODE1_2FRAME
#else
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR48K_MODE1
#endif
#define LLNS_DNS_PARAM_0        (0.1f)
#define LLNS_DNS_PARAM_1        (0.5f)
#elif (TCFG_LLNS_DNS_MODE == 2)
#define LLNS_TABLE_SELECT       NN_TABLE_LLNS_DNS_SR48K_MODE2
#else
#define LLNS_TABLE_SELECT       0
#endif
#define LLNS_DNS_PARAM_0        (0.0f)
#define LLNS_DNS_PARAM_1        (0.05f)

#endif

const u32 LLNS_DNS_SUPPORT_SAMPLE_RATE = LLNS_DNS_SR;
const u32 const_llns_dns_set_param_mode = 0;
const float const_llns_dns_param_0 = LLNS_DNS_PARAM_0;
const float const_llns_dns_param_1 = LLNS_DNS_PARAM_1;
const u32 const_llns_dns_param_2 = 20;

#define CVP_TABLE_SELECT   	TCFG_CVP_V3_NN_TABLE_TYPE

#if (CVP_TABLE_SELECT == NN_TABLE_CVP_1MIC_LARGE) && !(CVP_V3_1MIC_ALGO_ENABLE)
#error "NN_1MIC_LARGE requires CVP_V3_1MIC_ALGO_ENABLE enabled"
#endif

#if TCFG_AUDIO_CVP_V3_MODE
const u32 NN_TABLE_SELECT = (CVP_TABLE_SELECT | LLNS_TABLE_SELECT);
#else
const u32 NN_TABLE_SELECT = (LLNS_TABLE_SELECT);
#endif
/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
////////////////////ID3信息使能/////////////////
const u8 config_flac_id3_enable = 0;
const u8 config_ape_id3_enable  = 0;
const u8 config_m4a_id3_enable  = 0;
const u8 config_wav_id3_enable = 0;
const u8 config_wma_id3_enable = 0;

/////////////////////wma codec//////////////////
const int const_audio_codec_wma_dec_supoort_POS_play = 1; //是否支持指定位置播放

/////////////////////wav codec/////////////////
const int const_audio_codec_wav_dec_bitDepth_set_en = 0;

/////////////////////mty codec/////////////////
const int config_mty_repeat_enable = 1; //mty 支持循环播放

/////////////////////aac codec/////////////////


//sdk流程使用 0: 堆栈buf 在初始化的时候申请，close 的时候释放，
//           1: 堆栈buf 在每一帧run的时候申请，run 结束后释放。
//解码和能量检测共用配置
const int AAC_DEC_STKBUF_RELEASE_ENABLE = 1;

const int LC3_CODEC_STKBUF_RELEASE_ENABLE = 1;
const int JLA_CODEC_STKBUF_RELEASE_ENABLE = 1;
const int JLA_V2_CODEC_STKBUF_RELEASE_ENABLE = 1;
/////////////////////opus codec/////////////////
/*
 opus 配置编码支持的类型
silk:支持参数：8K/16K 单声道，20/40/60/80/100ms, 码率：16kbps~80kbps
			   复杂度：0~3  封装：百度无头 酷狗eng+range ogg封装 源码兼容模式

cetl:支持参数：8K/16K/24K/48K 单/双声道，10/20/ms
			   复杂度：0~4  封装：百度无头 酷狗eng+range ogg封装 源码兼容模式
如果silk/celt 需要同时支持， 需要配置：
	OPUS_ENC_SUPPORT_TYPE = OPUS_TYPE_SILK | OPUS_TYPE_CELT;并更具实际情况配置相应的编码器
 */
#if defined(TCFG_ENCODER_CHANNEL_NUM) && (TCFG_ENCODER_CHANNEL_NUM & BIT(1)) //双声道opus编码
const u8 OPUS_ENC_SUPPORT_TYPE = OPUS_TYPE_CELT | OPUS_TYPE_SILK;
#else
const u8 OPUS_ENC_SUPPORT_TYPE = OPUS_TYPE_SILK;
#endif

#ifndef TCFG_OPUS_DEC_PLC_EN
#define TCFG_OPUS_DEC_PLC_EN  0
#endif

#ifndef TCFG_OPUS_DEC_SUPPORT_TYPE
#define TCFG_OPUS_DEC_SUPPORT_TYPE  OPUS_TYPE_SILK
#endif

//优化opus解码内存
//0:解码输出48k双声道，1解码输出16k单声道，仅silk解码支持配置
const int CONFIG_OGG_OPUS_DEC_16K_1CH =  0;
//是否支持opus解码plc
const int CONFIG_OGG_OPUS_DEC_PLC_EN =  TCFG_OPUS_DEC_PLC_EN;
// 设置解码模式   OPUS_TYPE_SILK:silk OPUS_TYPE_CELT:celt；OPUS_TYPE_HYBRID：silk+celt
const int CONFIG_OGG_OPUS_DEC_SUPPORT_TYPE = TCFG_OPUS_DEC_SUPPORT_TYPE;
// 优化opus解码库代码量
/* const int TEST_ONLY_SILK_161CH = 0; // 0：测试48k2ch输出 1：测试16k1ch输出 */
const int SET_OPUS_TYPE = (CONFIG_OGG_OPUS_DEC_SUPPORT_TYPE == OPUS_TYPE_HYBRID) \
                          ? (OPUS_TYPE_SILK | OPUS_TYPE_CELT) \
                          : CONFIG_OGG_OPUS_DEC_SUPPORT_TYPE;   //  根据应用层解码类型配置推导：HYBRID内部等价于SILK|CELT
const int SET_OPUS_FEATURE = OPUS_FEAT_OGG | (CONFIG_OGG_OPUS_DEC_PLC_EN ? OPUS_FEAT_PLC : 0); // OGG始终编译，PLC根据应用层CONFIG_OGG_OPUS_DEC_PLC_EN决定
const int SET_OPUS_OUTPUT_MODE = CONFIG_OGG_OPUS_DEC_16K_1CH ? OPUS_OUTPUT_16K_MONO : OPUS_OUTPUT_48K_STEREO; // 根据应用层输出模式配置决定编译哪种输出处理代码
/////////////////////flac codec/////////////////
const int FLAC_TGF_SEARCH_ACCURACY_POINTS = 8192; //定位目标点数的精度,在目标点附近多少点内停止搜索.加大该值可加快定位，但定位精度会降低

/*
 *******************************************************************
 *						record Config
 *******************************************************************
 */

/* 最大录音数据缓存大小,
 * 如果打印"too_much_enc_data"表示设备写入速度不够,需要提高设备写入速度或增大缓存
 * CONFIG_MAX_ENC_DATA_CACHE_SIZE 是编码器数据缓冲区的大小
 */
const int CONFIG_MAX_ENC_DATA_CACHE_SIZE = 16 * 1024;

/* WRITE_FILE_BLOCK_SIZE决定写文件的块大小,要满足条件WRITE_FILE_BUF_SIZE >= WRITE_FILE_BLOCK_SIZE 且是倍数关系 */
/* WRITE_FILE_BLOCK_SIZE越大，写文件的周期越长，对写入速度较慢的存储设备会兼容更好一些 */
const u32 CONFIG_WRITE_FILE_BUF_SIZE = 8 * 1024;
const u32 CONFIG_WRITE_FILE_BLOCK_SIZE = 4 * 1024;

const int config_write_file_log_enable = 0;

/*
 *******************************************************************
 *						Audio Effects Config
 *******************************************************************
 */
const int config_out_dev_data_convert_enable = 1;     	//输出设备支持位宽转换处理
const int config_out_dev_channel_convert_enable = 0;     	//输出设备支持声道转换处理
const int config_out_dev_limiter_enable = 0;			//输出级限幅使能
const float config_bandmerge_node_fade_step = 0.0f;		//淡入步进 0:默认不淡入 非0：淡入步进，范围：0.01f~10.0f，建议值0.1f,步进越大，更新越快
const int config_bandmerge_node_processing_method = 1;	//0：bandmerge 拿到所有iport的数据后，一次性叠加完成。 1：逐个叠加到目标地址，不做等待


/*控制 eq_design.c中的butterworth 函数 设计的系数是定点还是浮点 */
#if defined(EQ_CORE_V1)
const int butterworth_iir_filter_coeff_type_select = 0;//虚拟低音根据此变量使用相应的滤波器设计函数 0:float  1:int
#else
const int butterworth_iir_filter_coeff_type_select = 1;//虚拟低音根据此变量使用相应的滤波器设计函数 0:float  1:int
#endif

const int virtual_bass_pro_soft_crossover = 0;//控制虚拟低音pro 中的分频器是用软件运行或者硬件运行  1 软件EQ  0 硬件EQ 默认硬件EQ
const int virtual_bass_pro_soft_eq = 1;       //控制虚拟低音pro 中的EQ是用软件运行或者硬件运行 1软件 0硬件 默认1
const int virtual_bass_eq_hard_select = 0; //虚拟低音使用的eq类型 0:使用软件eq  1:使用硬件eq

//***********************
//* 	 EQ             *
//***********************

#if TCFG_SPEAKER_EQ_NODE_ENABLE
#if TCFG_DAC_NODE_ENABLE && ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR) || (TCFG_AUDIO_DAC_CONNECT_MODE ==DAC_OUTPUT_DUAL_LR_DIFF))
#define AUDIO_EQ_RIGHT_CH_INDEPENDENT_COEFF_EN 1  //EQ右声道独立系数配置，通常用于立体声SPK-EQ
#else
#define AUDIO_EQ_RIGHT_CH_INDEPENDENT_COEFF_EN 0
#endif
#else
#define AUDIO_EQ_RIGHT_CH_INDEPENDENT_COEFF_EN 0
#endif



#define AUDIO_EQ_FADE_ENABLE		1	//EQ系数更新淡入淡出
//EQ配置relase使能：使能后根据工具EQ节点用到的滤波器类型仅使能对应的滤波器，优化代码体积，无法在线修改滤波器类型
#define AUDIO_EQ_CONFIG_RELEASE		0

#if AUDIO_EQ_CONFIG_RELEASE
const int config_audio_eq_hp_enable = EQ_CFG_TYPE_HIGH_PASS;		//High Pass
const int config_audio_eq_lp_enable = EQ_CFG_TYPE_LOW_PASS;			//Low Pass
const int config_audio_eq_bp_enable = EQ_CFG_TYPE_PEAKING;			//Band Pass(Peaking)
const int config_audio_eq_hs_enable = EQ_CFG_TYPE_HIGH_SHELF;		//High Shelf
const int config_audio_eq_ls_enable = EQ_CFG_TYPE_LOW_SHELF;		//Low Shelf
const int config_audio_eq_hs_q_enable = EQ_CFG_TYPE_HIGH_SHELF_Q;	//High Shelf Q
const int config_audio_eq_ls_q_enable = EQ_CFG_TYPE_LOW_SHELF_Q;	//Low Shelf Q
const int config_audio_eq_hp_adv_enable = EQ_CFG_TYPE_HP;			//High Pass Advance：对应工具上阶数可选的Hp
const int config_audio_eq_lp_adv_enable = EQ_CFG_TYPE_LP;			//Low Pass Advance：对应工具上阶数可选的Lp
#else //Debug

const int config_audio_eq_hp_enable = 1;		//High Pass
const int config_audio_eq_lp_enable = 1;		//Low Pass
const int config_audio_eq_bp_enable = 1;		//Band Pass(Peaking)
const int config_audio_eq_hs_enable = 1;		//High Shelf
const int config_audio_eq_ls_enable = 1;		//Low Shelf
#ifdef CONFIG_256K_FLASH
const int config_audio_eq_hs_q_enable = 0;		//High Shelf Q
const int config_audio_eq_ls_q_enable = 0;		//Low Shelf Q
const int config_audio_eq_hp_adv_enable = 0;	//High Pass Advance：对应工具上阶数可选的Hp
const int config_audio_eq_lp_adv_enable = 0;	//Low Pass Advance：对应工具上阶数可选的Lp
#else
const int config_audio_eq_hs_q_enable = 1;		//High Shelf Q
const int config_audio_eq_ls_q_enable = 1;		//Low Shelf Q
const int config_audio_eq_hp_adv_enable = 1;	//High Pass Advance：对应工具上阶数可选的Hp
const int config_audio_eq_lp_adv_enable = 1;	//Low Pass Advance：对应工具上阶数可选的Lp
#endif

#endif

#if TCFG_SPEAKER_EQ_NODE_ENABLE
#if EQ_SECTION_MAX < 10
#undef EQ_SECTION_MAX
#define EQ_SECTION_MAX 10
#endif
#endif/*TCFG_SPEAKER_EQ_NODE_ENABLE*/

const int AUDIO_EQ_MAX_SECTION = EQ_SECTION_MAX;

#if TCFG_EQ_ENABLE || TCFG_VOCAL_REMOVER_NODE_ENABLE
const int config_audio_eq_en = EQ_EN

#if TCFG_CROSSOVER_NODE_ENABLE
                               | EQ_HW_CROSSOVER_TYPE0_EN
#endif/*TCFG_CROSSOVER_NODE_ENABLE*/
#if (AUDIO_EQ_FADE_ENABLE == 0)
                               | EQ_FADE_DISABLE //关闭 eq fade
#endif
#if AUDIO_EQ_RIGHT_CH_INDEPENDENT_COEFF_EN
                               | EQ_RIGHT_CH_INDEPENDENT_COEFF_EN //EQ右声道独立系数配置，通常用于立体声SPK-EQ
#endif
                               ;
#else
const int config_audio_eq_en = 0;
#endif/* TCFG_EQ_ENABLE */

const int const_eq_debug = 0;
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_TIME_MS = 0;//300 //连续多长时间静音就清除EQ MEM
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_LIMIT = 0; //静音判断阀值



#if (defined (TCFG_AUDIO_FLOW_PRESET_AI_TUNE) && TCFG_AUDIO_FLOW_PRESET_AI_TUNE)
const int config_audio_eq_xfade_enable = 1;
const float config_audio_eq_xfade_time = 0.4f;
#else
const int config_audio_eq_xfade_enable = 1;
const float config_audio_eq_xfade_time = 0;//0.4f;//0：一帧fade完成 非0：连续多帧fade，过度更加平滑，fade过程算力会相应增加(fade时间 范围(0~1)单位:秒)
#endif


const int limiter_run_mode = EFx_PRECISION_PRO
#if defined(TCFG_AUDIO_EFX_E9D0_RUN_MODE)
                             | TCFG_AUDIO_EFX_E9D0_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_4E5B_RUN_MODE)
                             | TCFG_AUDIO_EFX_4E5B_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_F58A_RUN_MODE)
                             | ((TCFG_AUDIO_EFX_F58A_RUN_MODE & (EFx_BW_32t16 | EFx_BW_32t32)) ? EFx_BW_32t32 : 0)
                             | ((TCFG_AUDIO_EFX_F58A_RUN_MODE &EFx_BW_16t16) ? EFx_BW_16t16 : 0)
#endif
#if !defined(TCFG_AUDIO_EFX_4E5B_RUN_MODE) && !defined(TCFG_AUDIO_EFX_F58A_RUN_MODE)
                             | 0xFFFF
#endif
#if defined(TCFG_AUDIO_EFX_3A0C_RUN_MODE) && TCFG_AUDIO_EFX_3A0C_RUN_MODE
                             | EFx_BW_32t32
#endif
                             ;

#ifdef TCFG_AUDIO_EFX_6195_RUN_MODE
const  int frequency_shift_run_mode      = TCFG_AUDIO_EFX_6195_RUN_MODE;
#else
const  int frequency_shift_run_mode      = EFx_BW_16t16 | EFx_BW_32t32;//只有 16进16出， 或者 32进32出
#endif

const  int plate_reverb_lite_run_mode    = 0
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_16t16 | EFx_BW_16t32))
    | EFx_BW_16t16
#endif
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_32t32))
    | EFx_BW_32t32
#endif
#ifdef TCFG_AUDIO_EFX_A48F_RUN_MODE
    | TCFG_AUDIO_EFX_A48F_RUN_MODE
#endif
    ;

const  int echo_run_mode                 = 0
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_16t16 | EFx_BW_16t32))
    | EFx_BW_16t16
#endif
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_32t32))
    | EFx_BW_32t32
#endif
#ifdef TCFG_AUDIO_EFX_98A4_RUN_MODE
    | TCFG_AUDIO_EFX_98A4_RUN_MODE
#endif
    ;


const  int voicechanger_run_mode         = 0
#if defined(TCFG_AUDIO_EFX_7293_RUN_MODE)
    | TCFG_AUDIO_EFX_7293_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_AE43_RUN_MODE)//harmony
    | TCFG_AUDIO_EFX_AE43_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_7293_RUN_MODE) && !defined(TCFG_AUDIO_EFX_AE43_RUN_MODE)
    | EFx_BW_16t16 | EFx_BW_32t32//变声位宽控制
#endif
    ;
#if TCFG_HARMONY_NODE_ENABLE
const int pitchshift_have_modeHarmony = 1;//1:harmony节点spectrum的模式支持度的配置，不过会增加它做为变声模式的一些运算, 0:反之
#else
const int pitchshift_have_modeHarmony = 0;
#endif


#ifdef TCFG_AUDIO_EFX_40DC_RUN_MODE
const int  vibrato_run_mode = TCFG_AUDIO_EFX_40DC_RUN_MODE;
#else
const int  vibrato_run_mode = EFx_BW_32t32 | EFx_BW_16t16;
#endif

const  int autotune_run_mode             = 0
#if defined(TCFG_AUDIO_EFX_C07A_RUN_MODE)
    | TCFG_AUDIO_EFX_C07A_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_AE43_RUN_MODE)//harmony
    | TCFG_AUDIO_EFX_AE43_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_7293_RUN_MODE) && !defined(TCFG_AUDIO_EFX_AE43_RUN_MODE)
    | EFx_BW_16t16 | EFx_BW_32t32//autoTune位宽控制
#endif
    ;

#if TCFG_AUDIO_EFX_FB27_RUN_MODE
const int treomolo_run_mode = TCFG_AUDIO_EFX_FB27_RUN_MODE;
#else
const int treomolo_run_mode =  EFx_BW_16t16 | EFx_BW_32t32;
#endif



const  int reverb_run_mode               = 0
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_16t16 | EFx_BW_16t32))
    | EFx_BW_16t16
#endif
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_32t32))
    | EFx_BW_32t32
#endif

#ifdef TCFG_AUDIO_EFX_24AB_RUN_MODE
    | TCFG_AUDIO_EFX_24AB_RUN_MODE
#endif
    ;

const  int plate_reverb_run_mode         = 0
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_16t16 | EFx_BW_16t32))
    | EFx_BW_16t16
#endif
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_32t32))
    | EFx_BW_32t32
#endif
#ifdef TCFG_AUDIO_EFX_5101_RUN_MODE
    | TCFG_AUDIO_EFX_5101_RUN_MODE;
#endif
;


const int plate_reverb_combo_en = 0
#ifdef TCFG_REVERB_TYPE_SEL
                                  | TCFG_REVERB_TYPE_SEL
#endif
                                  ;




#ifdef TCFG_ECHO_COMBO_ENABLE
const int echo_combo_en = TCFG_ECHO_COMBO_ENABLE;
#else
const int echo_combo_en = 0;
#endif


const  int plate_reverb_adv_run_mode     = 0
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_16t16 | EFx_BW_16t32))
    | EFx_BW_16t16
#endif
#if defined(TCFG_AUDIO_EFX_C8E6_RUN_MODE) && (TCFG_AUDIO_EFX_C8E6_RUN_MODE & (EFx_BW_32t32))
    | EFx_BW_32t32
#endif
#ifdef TCFG_AUDIO_EFX_0753_RUN_MODE
    | TCFG_AUDIO_EFX_0753_RUN_MODE;
#endif
;

#ifdef TCFG_AUDIO_EFX_E955_RUN_MODE
const  int noisegate_pro_run_mode        = TCFG_AUDIO_EFX_E955_RUN_MODE;
#else
const  int noisegate_pro_run_mode        = EFx_BW_16t16 | EFx_BW_32t32;
#endif

const  int noisegate_run_mode            = 0
#if defined(TCFG_AUDIO_EFX_B7C4_RUN_MODE)
    | TCFG_AUDIO_EFX_B7C4_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_B0D5_RUN_MODE)//virtual bass
    | ((TCFG_AUDIO_EFX_B0D5_RUN_MODE & (EFx_BW_16t32 |  EFx_BW_32t32)) ? EFx_BW_32t32 : 0)
    | ((TCFG_AUDIO_EFX_B0D5_RUN_MODE &EFx_BW_16t16) ? EFx_BW_16t16 : 0)
#endif
#if !defined(TCFG_AUDIO_EFX_B7C4_RUN_MODE) && !defined(TCFG_AUDIO_EFX_B0D5_RUN_MODE)
    | EFx_BW_16t16 | EFx_BW_32t32
#endif
    ;

#ifdef TCFG_AUDIO_EFX_B0D5_RUN_MODE
const  int virtual_bass_run_mode         = TCFG_AUDIO_EFX_B0D5_RUN_MODE;
#else
const  int virtual_bass_run_mode         = EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t32;
#endif

#if (defined TCFG_AUDIO_EFX_02E6_RUN_MODE && (TCFG_AUDIO_EFX_02E6_RUN_MODE == EFx_BW_16t32))
//Virtual Bass Pro 实际算法为32in32out，只是节点内集成了convert 16->32
#undef TCFG_AUDIO_EFX_02E6_RUN_MODE
#define TCFG_AUDIO_EFX_02E6_RUN_MODE EFx_BW_32t32
#endif

const  int virtual_bass_classic_run_mode = 0
#if defined(TCFG_AUDIO_EFX_E9D0_RUN_MODE)
    | TCFG_AUDIO_EFX_E9D0_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_55C9_RUN_MODE)
    | TCFG_AUDIO_EFX_55C9_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
    | TCFG_AUDIO_EFX_02E6_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_55C9_RUN_MODE) || defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
    | EFx_BW_16t16 | EFx_BW_32t32
#endif
    ;

const  int drc_advance_run_mode          = EFx_PRECISION_NOR
#if defined(TCFG_AUDIO_EFX_E9D0_RUN_MODE)
    | TCFG_AUDIO_EFX_E9D0_RUN_MODE
#endif

#if defined(TCFG_AUDIO_EFX_4250_RUN_MODE)
    | TCFG_AUDIO_EFX_4250_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_74CB_RUN_MODE)
    | ((TCFG_AUDIO_EFX_74CB_RUN_MODE & (EFx_BW_32t16 | EFx_BW_32t32)) ? EFx_BW_32t32 : 0)
    | ((TCFG_AUDIO_EFX_74CB_RUN_MODE &EFx_BW_16t16) ? EFx_BW_16t16 : 0)
#endif
#if defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
    | TCFG_AUDIO_EFX_02E6_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_4250_RUN_MODE) && !defined(TCFG_AUDIO_EFX_74CB_RUN_MODE) && !defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
    | EFx_BW_16t16 | EFx_BW_32t16 | EFx_BW_32t32
#endif
    ;


#ifdef TCFG_AUDIO_EFX_9A58_RUN_MODE
const  int drc_detect_run_mode           = TCFG_AUDIO_EFX_9A58_RUN_MODE | EFx_PRECISION_NOR;
#else
const  int drc_detect_run_mode           = EFx_BW_16t16 | EFx_BW_32t16 | EFx_PRECISION_NOR | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_DEFE_RUN_MODE
const  int drc_run_mode                  = TCFG_AUDIO_EFX_DEFE_RUN_MODE | EFx_PRECISION_NOR;
#else
const  int drc_run_mode                  = EFx_BW_16t16 | EFx_BW_32t16 | EFx_PRECISION_NOR | EFx_BW_32t32;
#endif


#ifdef TCFG_AUDIO_EFX_540E_RUN_MODE
const int pitch_speed_run_mode       = TCFG_AUDIO_EFX_540E_RUN_MODE;
#else
const int pitch_speed_run_mode       = EFx_BW_32t32 | EFx_BW_16t16;
#endif
const int resample_fast_cal_run_mode = EFx_BW_16t16 | EFx_BW_32t32;

#ifdef TCFG_AUDIO_EFX_A8F4_RUN_MODE
const int pcm_delay_run_mode         = TCFG_AUDIO_EFX_A8F4_RUN_MODE;
#else
const int pcm_delay_run_mode         = EFx_BW_16t16 | EFx_BW_32t32;
#endif
#ifdef TCFG_AUDIO_EFX_1B2A_RUN_MODE
const int harmonic_exciter_run_mode  = TCFG_AUDIO_EFX_1B2A_RUN_MODE;
#else
const int harmonic_exciter_run_mode  = EFx_BW_16t16 | EFx_BW_32t32;
#endif
#ifdef TCFG_AUDIO_EFX_ED7F_RUN_MODE
const int lfaudio_plc_run_mode       = TCFG_AUDIO_EFX_ED7F_RUN_MODE;
#else
const int lfaudio_plc_run_mode       = EFx_BW_16t16 | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_BC44_RUN_MODE
const int  stereo_flanger_run_mode = TCFG_AUDIO_EFX_BC44_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_flanger_run_mode = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_7C2B_RUN_MODE
const int  stereo_chorus_run_mode  = TCFG_AUDIO_EFX_7C2B_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_chorus_run_mode  = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_D2E8_RUN_MODE
const int  stereo_phaser_run_mode  = TCFG_AUDIO_EFX_D2E8_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_phaser_run_mode  = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_AB66_RUN_MODE
const int  pingpong_echo_run_mode  = TCFG_AUDIO_EFX_AB66_RUN_MODE;
#else
const int  pingpong_echo_run_mode  = EFx_BW_32t32 | EFx_BW_16t16;
#endif

#ifdef TCFG_AUDIO_EFX_97AA_RUN_MODE
const int distortion_run_mode = TCFG_AUDIO_EFX_97AA_RUN_MODE;
#else
const int distortion_run_mode = EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t16 | EFx_BW_32t32;
#endif

const int dynamic_eq_run_mode = EFx_BW_32t32 | EFx_PRECISION_NOR; //只支持32进32出 不会优化代码预留

const int dynamic_eq_pro_run_mode = EFx_BW_32t32;//只支持32进32出 不会优化代码预留

const int iir_filter_run_mode = 0  //不支持32进16出
#if defined(TCFG_AUDIO_EFX_3845_RUN_MODE)
                                | TCFG_AUDIO_EFX_3845_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_6700_RUN_MODE)
                                | TCFG_AUDIO_EFX_6700_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
                                | TCFG_AUDIO_EFX_02E6_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_3845_RUN_MODE) && !defined(TCFG_AUDIO_EFX_6700_RUN_MODE) && !defined(TCFG_AUDIO_EFX_02E6_RUN_MODE)
                                | EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t32  //不支持32进16出
#endif
                                ;

#ifdef TCFG_AUDIO_EFX_BFE4_RUN_MODE
const int frequency_compressor_run_mode  = TCFG_AUDIO_EFX_BFE4_RUN_MODE; //只支持16进16出与32进32出
#else
const int frequency_compressor_run_mode  = EFx_BW_16t32 | EFx_BW_32t32;
#endif

//pingpong echo
const int ECHO_PINGPONG_LR_SAMEVOL = 1; //0或1可配
/*变声模式使能*/
#ifndef EFFECT_VOICECHANGE_TYPE_CFG
#define EFFECT_VOICECHANGE_TYPE_CFG  BIT(EFFECT_VOICECHANGE_PITCHSHIFT)
#endif
const int voicechanger_effect_v_config = EFFECT_VOICECHANGE_TYPE_CFG;


/*mb drc/limiter 3带使能(1.2k) */
const int audio_crossover_3band_enable       = 1;
const int config_audio_limiter_xfade_enable = 0;
const int config_audio_mblimiter_xfade_enable = 0;

/*Vocal Remover Configs*/
#ifdef TCFG_VOCAL_REMOVER_LOW_PASS_ENABLE
const int audio_vocal_remover_low_cut_enable = TCFG_VOCAL_REMOVER_LOW_PASS_ENABLE;
#else
const int audio_vocal_remover_low_cut_enable = 1;
#endif
#ifdef TCFG_VOCAL_REMOVER_HIGH_PASS_ENABLE
const int audio_vocal_remover_high_cut_enable = TCFG_VOCAL_REMOVER_HIGH_PASS_ENABLE;
#else
const int audio_vocal_remover_high_cut_enable = 1;
#endif
#ifdef TCFG_VOCAL_REMOVER_PRESET_MODE
//预设参数模式[0/1]，0：预设关，使用工具节点配置 1：使用预设模式1
const int audio_vocal_remover_preset_mode = TCFG_VOCAL_REMOVER_PRESET_MODE;
#else
const int audio_vocal_remover_preset_mode = 0;
#endif

//howling_ahs配置
const int const_audio_howling_ahs_ref_enable = 0;   //算法是否需要获取dac回采数据,算法不需要回采数据的情况下可关闭，节省资源消耗

/*
 * 0:不使能
 * BIT(0):process0需要process1的输出作为参考
 * BIT(1):process0需要播放同步输入数据作为参考
 */
const int const_audio_howling_ahs_process_ref_enable = BIT(1);
const int const_audio_howling_ahs_ref_src_type = 1; //0:不使能参考数据变采样，1:软件src，2:硬件src
const int const_audio_howling_ahs_data_export = 0;  //数据写卡导出，需要使能AUDIO_DATA_EXPORT_VIA_UART，串口写卡工具配置3通道，每通道256byte(@16bit) 512byte(@32bit)
const int const_audio_howling_ahs_algo_runtime_debug = 0; //算法运行时间debug，打印较多，仅调试时打开

/*
 * 需要dac回采数据时，ahs算法所处的数据流输入为iis，输出为dac时需要使能。
 */
const int const_audio_howling_ahs_iis_in_dac_out = 0;

/*
 * ADC硬件回采使能，硬件上需要预留ADC回采功放后级输出通路
 * 使能后，需要在"音频配置->ADC配置"中正确配置对应的ADC
 */
const int const_audio_howling_ahs_adc_hw_ref = 0;
const int const_audio_howling_ahs_adc_hw_ref_ch = BIT(0); //硬回采使用的ADC通道
const int const_audio_howling_ahs_adc_hw_ref_mic_gain = 10;//单位：档位，可参考adc节点配置
const int const_audio_howling_ahs_adc_hw_ref_mic_pre_gain = 1;

/* 软件回采dac_read_offset点数设置 */
const int const_audio_howling_ahs_dac_read_points_offset = 0;


/*
 * ahs-nn 算法是否使用双核
 * 1:双核-啸叫抑制效果更优，延时略大，资源消耗多。流程需要串AHS-NN + AHS-NN-POST。
 * 0:单核-延时低，资源消耗少。流程仅需要串AHS-NN。
 */
const int const_audio_howling_ahs_dual_core = 1;
const int const_audio_howling_ahs_frame_byte = 257 * 2 * 4 + 5160; //双核处理中间数据长度

const int audio_bass_boost_combo_fade_time_ms = 400;// 0:xfade, 非0：参数更新fade_out->udpate_param->fade_in, 范围 50~1000, 单位:ms
const int audio_bass_boost_combo_multi_frame_xfade_en = 0;  //1:使能多帧xfade效效果
#ifdef TCFG_EQ_LIMITER_C_LIMITER_BAND_NUM
const int const_eq_limiter_combomied_multiband_en = (TCFG_EQ_LIMITER_C_LIMITER_BAND_NUM == 1) ? 0 : 1;
#else
const int const_eq_limiter_combomied_multiband_en = 0;
#endif
const int audio_eq_limiter_combo_fade_time_ms = 0;  // 0:xfade, 非0：参数更新fade_out->udpate_param->fade_in, 范围 50~1000, 单位:ms
/*
 * 某些算法参数更新需要重新申请buffer，
 * 为防止重新申请时内存不足导致异常，
 * 此处设置需要保留的内存大小，
 * 当判断若重新申请buf后，系统内存小于该值则不重新申请，
 * 打印"xxx buf realloc fail"(需要打开log_tag_const_d_EFFECTS)
 * 可视化工具界面显示"流程打开buf失败"
 */
const int audio_effect_realloc_reserve_mem = (13 * 1024);

#if TCFG_AUDIO_FLOW_PRESET_WLM_EFFECT
const int howling_freqshift_lowdelay = 1;//0:fir分支，跟以前效果一致, 1:使用iir分支来节省延时
#else
const int howling_freqshift_lowdelay = 0;//0:fir分支，跟以前效果一致, 1:使用iir分支来节省延时
#endif

#ifdef LIMITER_LIB_IN_MASK
const int limiter_lib_in_maskrom = 1;//1:limiter 算法在maskrom, 0:在flash
#else
const int limiter_lib_in_maskrom = 0;
#endif
#ifdef  TCFG_PLATE_REVERB_LR_DISABLE
const int plate_reverb_LR_sameflag = TCFG_PLATE_REVERB_LR_DISABLE;// 1:plateReverb左右是输出一样的信号， 0:  是会去调rom的引出来的函数，输出真的双声道
#else
const int plate_reverb_LR_sameflag = 0;
#endif

/*
 *******************************************************************
 *						Audio Mic Capless Config
 *******************************************************************
 */
/*
 *          用于debug需要配置多长的mic_capless_delay
 * 部分mic 需要更多延时才能达到稳定的电压值
 * debug步骤：
 * 1、打开 const_mic_capless_open_delay_debug，可打印打开adc后，mic_bias需要多长时间才能达到/接近稳定的电压值
 * 2、将该延时配置到 open_delay_ms 中
 * 3、关闭 const_mic_capless_open_delay_debug

 * 4、打开 const_mic_capless_trim_delay_debug，可打印第一次trim后，mic_bias需要多长时间才能达到/接近稳定的电压值
 * 5、将该延时配置到 trim_delay_ms 中
 * 6、关闭 const_mic_capless_trim_delay_debug
 */
const u8 const_mic_capless_open_delay_debug = 0;
const u8 const_mic_capless_trim_delay_debug = 0;

REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, ALINK)

const int AUDIO_ALINK_CODE_EN = 0
#if ((defined(TCFG_IIS_NODE_ENABLE) && TCFG_IIS_NODE_ENABLE) || (defined(TCFG_TDM_TX_NODE_ENABLE) && TCFG_TDM_TX_NODE_ENABLE))
                                | ALINK_TX_EN
#endif
#if ((defined(TCFG_IIS_RX_NODE_ENABLE) && TCFG_IIS_RX_NODE_ENABLE) || (defined(TCFG_TDM_RX_NODE_ENABLE) && TCFG_TDM_RX_NODE_ENABLE))
                                | ALINK_RX_EN
#endif
#if defined(TCFG_MULTI_CH_IIS_NODE_ENABLE) &&TCFG_MULTI_CH_IIS_NODE_ENABLE
                                | ALINK_MTX_EN
#endif
#if defined(TCFG_MULTI_CH_IIS_RX_NODE_ENABLE) &&TCFG_MULTI_CH_IIS_RX_NODE_ENABLE
                                | ALINK_MRX_EN
#endif
                                ;

const int audio_channel_adapter_code_en = 0
#if ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_L) || (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_R) || (ALINK0_CH_NUM == 3)|| (ALINK1_CH_NUM == 3))
    | CHANNEL_XT3_EN
#endif
#if ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR) || (ALINK0_CH_NUM == 4)|| (ALINK1_CH_NUM == 4))
    | CHANNEL_XT4_EN
    | CHANNEL_4TX_EN
#endif
#if ((ALINK0_CH_NUM == 6)|| (ALINK1_CH_NUM == 6))
    | CHANNEL_XT6_EN
#endif
#if ((ALINK0_CH_NUM == 8)|| (ALINK1_CH_NUM == 8))
    | CHANNEL_XT8_EN
#endif
    ;


const int audio_alink2da_en = 0;
const int audio_da2alink_en = 0;




/*vbass noisegate 参数配置*/
const int virtualbass_noisegate_attack_time = 50;
const int virtualbass_noisegate_release_time = 30;
const int virtualbass_noisegate_hold_time = 15;
const float virtualbass_noisegate_threshold = -85.0f;

/*
 *******************************************************************
 *			Audio hardware DMA malloc config
 *******************************************************************
 */
const int config_src_hw_dma_malloc_enable = 1;


const int const_audio_autoduck_trigger_status_get = 0; //autoduck触发时是否输出回调


/*
 *******************************************************************
 *			Audio sync  config
 *******************************************************************
 */
/*
 * 固定采样率且bypass同步的配置使能 - CONFIG_AUDIO_SYNC_FIXED_SAMPLE_RATE_ENABLE
 *
 * 缓慢对齐控制使能 - CONFIG_AUDIO_SYNC_SLOW_CONTROL_ENABLE
 *
 * 多配置选项使用或运算进行配置
 * */
#if TCFG_VIR_UDISK_ENABLE
const int config_audio_sync_features = CONFIG_AUDIO_SYNC_SLOW_CONTROL_ENABLE;
#else
const int config_audio_sync_features = CONFIG_AUDIO_SYNC_SLOW_CONTROL_BY_MODE;//CONFIG_AUDIO_SYNC_SLOW_CONTROL_ENABLE;
#endif

/*
 *格式：{输入设备UUID, 输出设备UUID，输入设备采样率，输出设备采样率，输入预设采样率，输出预设采样率}
 */
const u32 config_audio_device_sample_rate_table[][6] = {
    {NODE_UUID_LINEIN, NODE_UUID_IIS0_TX, 48000, 48000, 625, 624},
    {NODE_UUID_LINEIN, NODE_UUID_IIS1_TX, 48000, 48000, 625, 624},
};

const int config_audio_sync_fixed_table_num = ARRAY_SIZE(config_audio_device_sample_rate_table);

const int config_audio_sync_latch_out_sample_max_error = 0;//兼容 iis/tdm 较大输出采样率误差的情况, 特殊场景可考虑配置，0: 2/1000, n:n/1000 最大误差

const float FRAME_DURATION_THREAD = 1.5f;	//范围1.5f~2,采样率和时间戳抖动阈值倍数(丢帧检测阈值,时间戳间隔超过1.5帧，判定丢帧)

/*SRC特定输入输出采样率对应的滤波器系数*/
const int CONST_SRC_COEF_46875_TO_16000_ENABLE = 0;
const int CONST_SRC_COEF_16000_TO_8000_ENABLE = 0;

/*
 *接到同个分流器下的多设备同步,部分场景需单独开启
 * */
#if (((defined(TCFG_IIS_NODE_ENABLE) && TCFG_IIS_NODE_ENABLE) || (defined(TCFG_TDM_TX_NODE_ENABLE) && TCFG_TDM_TX_NODE_ENABLE)) && TCFG_DAC_NODE_ENABLE)
const int config_dev_sync_enable =  1;
#else
const int config_dev_sync_enable =  0;
#endif

/*
 *******************************************************************
 *						AudioOnChip Config
 *******************************************************************
 */
#ifdef AUD_DAC_FILTER_TYPE
const int config_audio_dac_filter_type = AUD_DAC_FILTER_TYPE;
#else
const int config_audio_dac_filter_type = 0;
#endif
#ifdef AUD_DAC_SRC_FILTER_TYPE
const int config_audio_dac_src_filter_type = AUD_DAC_SRC_FILTER_TYPE;
#else
const int config_audio_dac_src_filter_type = 0;
#endif
#ifdef AUD_ADC_FILTER_TYPE
const int config_audio_adc_filter_type = AUD_ADC_FILTER_TYPE;
#else
const int config_audio_adc_filter_type = 0;
#endif
#ifdef AUD_SRC_FILTER_TYPE
const int config_audio_src_filter_type = AUD_SRC_FILTER_TYPE;
#else
const int config_audio_src_filter_type = 0;
#endif
#ifdef AUD_SRC_PP_FILTER_TYPE
const int config_audio_src_pp_filter_type = AUD_SRC_PP_FILTER_TYPE;
#else
const int config_audio_src_pp_filter_type = SRC_PP_FILTER_01;
#endif
/*
 *******************************************************************
 *			Audio MIDI
 *******************************************************************
 */
const int MAINTRACK_USE_CHN = 0;	//配置主通道是轨道还是通道配置  0是轨道 1是通道  默认0
const int NOTE_OFF_TRIGGER = 0;     //配置为1 时 midi 琴 note_off time为0时 音符停止不回调
const int MIDI_SAVE_DIV_ENBALE = 0;     //配置小节回退功能(最大回退8个小节)，若使用小节回退功能需配置成1 默认为0
const int midi_ctrl_support_multi_ch = (MIDI_CTRL_OPORT_NUM > 1) ? 1 : 0;

__attribute__((weak))
int get_system_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_media_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_esco_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_mic_eff_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_usb_audio_stream_bit_width(void *par)
{
    return 0;
}


static struct audio_general_params audio_general_param = {0};

struct audio_general_params *audio_general_get_param(void)
{
    return &audio_general_param;
}
/*
 *输出设备硬件位宽
 * */
int audio_general_out_dev_bit_width()
{
    if (audio_general_param.system_bit_width || audio_general_param.media_bit_width ||
        audio_general_param.esco_bit_width || audio_general_param.mic_bit_width
        || audio_general_param.usb_audio_bit_width) {
        return DATA_BIT_WIDE_24BIT;
    }
    return DATA_BIT_WIDE_16BIT;
}
/*
 *输入设备硬件位宽
 * */
int audio_general_in_dev_bit_width()
{
    if (audio_general_param.media_bit_width || audio_general_param.mic_bit_width) {
        return DATA_BIT_WIDE_24BIT;
    }
    return DATA_BIT_WIDE_16BIT;
}

int audio_general_init()
{
#if defined(TCFG_SCENE_UPDATE_ENABLE) && TCFG_SCENE_UPDATE_ENABLE
    //若流程中有较多音效模块（或渲染封装节点），会导致此处遍历模块耗时较长
    get_music_pipeline_node_uuid();
#if (TCFG_MIC_EFFECT_ENABLE || (defined CONFIG_WIRELESS_MIC_CASE_ENABLE))
    get_mic_pipeline_node_uuid();
#endif
#endif

#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
    uartSendInit();
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

#if TCFG_AUDIO_CONFIG_TRACE
    audio_config_trace_setup(TCFG_AUDIO_CONFIG_TRACE_INTERVAL);
#endif/*TCFG_AUDIO_CONFIG_TRACE*/

#if MEDIA_24BIT_ENABLE
    struct stream_bit_width stream_par = {0};
    if (get_system_stream_bit_width(&stream_par)) {
        audio_general_param.system_bit_width = stream_par.bit_width;
    }

    if (get_media_stream_bit_width(&stream_par)) {
        audio_general_param.media_bit_width = stream_par.bit_width;
    }

    if (get_esco_stream_bit_width(&stream_par)) {
        audio_general_param.esco_bit_width = stream_par.bit_width;
    }

    if (get_mic_eff_stream_bit_width(&stream_par)) {
        audio_general_param.mic_bit_width = stream_par.bit_width;
    }

    if (get_usb_audio_stream_bit_width(&stream_par)) {
        audio_general_param.usb_audio_bit_width = stream_par.bit_width;
    }
#endif

#if defined(TCFG_AUDIO_GLOBAL_SAMPLE_RATE) &&TCFG_AUDIO_GLOBAL_SAMPLE_RATE
    audio_general_param.sample_rate = TCFG_AUDIO_GLOBAL_SAMPLE_RATE;
#endif
    return 0;
}

#if (CONFIG_CPU_BR27 || CONFIG_CPU_BR28 || CONFIG_CPU_BR29 || CONFIG_CPU_BR36)
static const int general_sample_rate_table[] = {8000, 16000, 32000, 44100, 48000, 96000};
#else
static const int general_sample_rate_table[] = {8000, 16000, 32000, 44100, 48000, 96000, 192000};
#endif

int audio_general_set_global_sample_rate(int sample_rate)
{
    local_irq_disable();
    for (u8 i = 0; i < ARRAY_SIZE(general_sample_rate_table); i++) {
        if (general_sample_rate_table[i] == sample_rate) {
            audio_general_param.sample_rate = sample_rate;
            local_irq_enable();
            printf("cur_global_samplerate %d", sample_rate);
            return 0;
        }
    }
    local_irq_enable();
    return -1;
}

int audio_general_get_global_sample_rate()
{
    return audio_general_param.sample_rate;
}
