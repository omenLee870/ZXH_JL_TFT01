#ifndef LIB_MEDIA_CONFIG_H
#define LIB_MEDIA_CONFIG_H


extern const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE;
extern const int CONFIG_MULTI_THREAD_SELF_ADAPTION_ENABLE;
extern const int config_media_24bit_enable;
extern const int CONFIG_SEAMLESS_RECORDER_ENABLE;
extern const int config_ch_adapter_32bit_enable;
extern const int config_mixer_32bit_enable;
extern const int config_jlstream_fade_32bit_enable;
extern const int config_out_dev_limiter_enable;
extern const int config_peak_rms_32bit_enable;
extern const int config_audio_vocal_track_synthesis_32bit_enable;
extern const int config_audio_invert_32bit_enable;
extern const int config_jlstream_node_report_enable;
extern const int CONFIG_MAX_ENC_DATA_CACHE_SIZE;
extern const int CONFIG_LOG_OUTPUT_ENABLE;
extern const int config_audio_cfg_online_enable;
extern const int CONFIG_MEDIA_MEM_DEBUG;
extern const int config_media_tws_en;
extern const int config_audio_cvp_ref_ch_recognize_enable;
extern const int config_sync_msg_latch_en;
extern const int config_sync_msg_latch_type;
extern const float const_out_dev_pns_time_ms;
extern const int AUDIO_ALINK_CODE_EN;
extern const int audio_alink2da_en;
extern const int audio_da2alink_en;
/*
 *******************************************************************
 *						jlstream Configs
 *******************************************************************
 */
extern const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE;
extern const int CONFIG_JLSTREAM_FADE_TIME_MSEC;
extern const int config_jlstream_fade_in_silence_check_enable;
extern const int config_jlstream_fade_32bit_enable;
extern const int config_jlstream_node_report_enable;
extern const int CONFIG_JLSTREAM_ASYNC_NODE_ENABLE;

/*
 *******************************************************************
 *						DAC Configs
 *******************************************************************
 */
extern const int config_audio_dac_filter_type;
extern const int config_audio_dac_src_filter_type;
extern const int config_audio_dac_output_channel;
extern const int config_audio_dac_output_mode;
extern const int config_audio_dac_channel_left_enable;
extern const int config_audio_dac_channel_right_enable;
extern const int config_audio_dac_power_on_mode;
extern const int config_audio_dac_power_off_lite;
extern const int config_audio_dac_mix_enable;
extern const int config_audio_dac_noisefloor_optimize_enable;
extern const char config_audio_dac_trim_enable;
extern const int config_audio_dac_mute_timeout;
extern const int config_audio_dac_pa_mode;
extern const int config_audio_dac_power_mode;
extern const int config_audio_dac_underrun_time_lea;
extern const int config_audio_dac_underrun_detect_enable;
extern const int config_audio_dac_ng_debug;
extern const int config_audio_dac_enable;
extern const int config_audio_dac_dma_buf_realloc_enable;
extern const int CONFIG_DAC_CACHE_MSEC;
extern const int config_audio_dac_delay_off_ms;
extern const int config_audio_dac_buffer_time_ms;
extern const int config_audio_dac_fade_out_time;
/*
 *******************************************************************
 *						ADC Configs
 *******************************************************************
 */
extern const int config_audio_adc_filter_type;
extern const int config_audio_adc0_enable;
extern const int config_audio_adc1_enable;
extern const int config_audio_adc2_enable;
extern const int config_audio_adc3_enable;
extern const int config_audio_adc4_enable;
extern const int config_audio_adc5_enable;
extern const int config_audio_adc6_enable;
extern const int config_audio_adc7_enable;
extern const int config_audio_adc0_input_mode;
extern const int config_audio_adc1_input_mode;
extern const int config_audio_adc2_input_mode;
extern const int config_audio_adc3_input_mode;
extern const int config_audio_adc4_input_mode;
extern const int config_audio_adc5_input_mode;
extern const int config_audio_adc6_input_mode;
extern const int config_audio_adc7_input_mode;
extern const u8 const_mic_capless_open_delay_debug;
extern const u8 const_mic_capless_trim_delay_debug;
extern const u8 const_adc_async_en;						//是否支持多个ADC异步打开

extern const int config_classd_output_mode;
extern const int config_audio_classd_power_off_enable;
extern const int config_audio_classd_light_power_off_enable;//轻量级关闭
extern const u8 const_adc_async_en;

/*
 *******************************************************************
 *						EQ Configs
 *******************************************************************
 */
extern const int config_audio_eq_hp_enable;		//High Pass
extern const int config_audio_eq_lp_enable;		//Low Pass
extern const int config_audio_eq_bp_enable;		//Band Pass(Peaking)
extern const int config_audio_eq_hs_enable;		//High Shelf
extern const int config_audio_eq_ls_enable;		//Low Shelf
extern const int config_audio_eq_hs_q_enable;	//High Shelf Q
extern const int config_audio_eq_ls_q_enable;	//Low Shelf Q
extern const int config_audio_eq_hp_adv_enable;	//High Pass Advance
extern const int config_audio_eq_lp_adv_enable;	//Low Pass Advance
extern const int config_audio_eq_xfade_enable;
extern const float config_audio_eq_xfade_time;//0：一帧fade完成 非0：连续多帧fade，过度更加平滑，fade过程算力会相应增加(fade时间 范围(0~1)单位:秒)

/*
 *******************************************************************
 *						SRC Configs
 *******************************************************************
 */
extern const int config_audio_src_filter_type;
extern const int config_audio_src_pp_filter_type;

extern const int CONST_SRC_COEF_46875_TO_16000_ENABLE;
extern const int CONST_SRC_COEF_16000_TO_8000_ENABLE;
/*
 *******************************************************************
 *						Audio sync Configs
 *******************************************************************
 */
extern const int config_audio_sync_features;    //音频同步功能配置
extern const u32 config_audio_device_sample_rate_table[][6];    //固定设备采样率设置
extern const int config_audio_sync_fixed_table_num;             //固定设备采样率预设的表内数量
extern const int config_audio_sync_latch_out_sample_max_error;//兼容 iis/tdm 较大输出采样率误差的情况, 特殊场景可考虑配置，0: 2/1000, n:n/1000 最大误差
/*
 *******************************************************************
 *						Effect Configs
 *******************************************************************
 */
extern const int config_audio_gain_enable;
extern const int config_audio_split_gain_enable;
extern const int config_audio_stereomix_enable;
extern const int config_audio_split_gain_fade_enable;
extern const int config_audio_fade_in_slience_check_enable;
extern const int voicechanger_effect_v_config;
extern const int audio_crossover_3band_enable;
extern const int limiter_run_mode;
extern const int drc_advance_run_mode;
extern const int drc_run_mode;
extern const int  stereo_phaser_run_mode;
extern const int  stereo_flanger_run_mode;
extern const int  stereo_chorus_run_mode;
extern const int dynamic_eq_run_mode;
extern const int drc_detect_run_mode;
extern const int virtual_bass_pro_soft_crossover;
extern const int audio_effect_realloc_reserve_mem;
extern const int const_audio_howling_ahs_ref_enable;
extern const int const_audio_howling_ahs_process_ref_enable;
extern const int const_audio_howling_ahs_ref_src_type;
extern const int const_audio_howling_ahs_data_export;
extern const int const_audio_howling_ahs_iis_in_dac_out;
extern const int const_audio_howling_ahs_dual_core;
extern const int const_audio_howling_ahs_frame_byte;
extern const int const_audio_howling_ahs_adc_hw_ref;
extern const int const_audio_howling_ahs_adc_hw_ref_ch;
extern const int const_audio_howling_ahs_adc_hw_ref_mic_gain;
extern const int const_audio_howling_ahs_adc_hw_ref_mic_pre_gain;
extern const int const_audio_howling_ahs_dac_read_points_offset;
extern const int const_audio_howling_ahs_algo_runtime_debug;
extern const int const_audio_ai_vocal_remover_algo_runtime_debug;
extern const int const_audio_autoduck_trigger_status_get;
extern const int virtual_bass_eq_hard_select;

extern const int virtualbass_noisegate_attack_time;
extern const int virtualbass_noisegate_release_time;
extern const int virtualbass_noisegate_hold_time;
extern const float virtualbass_noisegate_threshold;
extern const int config_audio_limiter_xfade_enable;
extern const int config_audio_mblimiter_xfade_enable;
extern const int limiter_lib_in_maskrom;
extern const int const_jlstream_nego_debug_en;
extern const int config_out_dev_data_convert_enable;
extern const int config_out_dev_channel_convert_enable;

extern const int auido_echo_delay_ms;
extern const int plate_reverb_combo_en;
extern const int echo_combo_en;
extern const int audio_bass_boost_combo_fade_time_ms;
extern const int audio_bass_boost_combo_multi_frame_xfade_en;
extern const int const_eq_limiter_combomied_multiband_en;
extern const int audio_eq_limiter_combo_fade_time_ms;
extern const int audio_channel_adapter_code_en;



/*
 *******************************************************************
 *						Audio Codec Configs
 *******************************************************************
 */
//通用配置
extern const int CONFIG_DEC_SUPPORT_CHANNELS;
extern const int CONFIG_DEC_SUPPORT_SAMPLERATE;

//id3 配置
extern const u8 config_flac_id3_enable;
extern const u8 config_ape_id3_enable;
extern const u8 config_m4a_id3_enable;
extern const u8 config_wav_id3_enable;
extern const u8 config_wma_id3_enable;

//定点播放 配置
extern const u8 file_dec_dest_play;
extern const int M4A_TGF_POSPLAY_EN;
extern const int FLAC_TGF_POSPLAY_EN;
extern const int APE_TGF_POSPLAY_EN;
extern const int const_audio_codec_wav_dec_supoort_POS_play;

//ab点复读配置
extern const int FLAC_TGF_AB_EN;
extern const int M4A_TGF_AB_EN;
extern const int APE_TGF_AB_EN;
//jla_v2 编解码配置
extern const unsigned short JLA_V2_FRAMELEN_MASK;
extern const int JLA_V2_PLC_EN;
extern const int JLA_V2_PLC_FADE_OUT_START_POINT;
extern const int JLA_V2_PLC_FADE_OUT_POINTS;
extern const int JLA_V2_PLC_FADE_IN_POINTS;
extern const int JLA_V2_CODEC_WITH_FRAME_HEADER;
extern const int JLA_V2_CODEC_STKBUF_RELEASE_ENABLE;
extern const int JLA_V2_NPLC_TYPE_CONFIG;

//jla 编解码配置
extern const int JLA_CODEC_STKBUF_RELEASE_ENABLE;

//lc3 编解码配置
extern const int LC3_CODEC_STKBUF_RELEASE_ENABLE;

//bt_aac 解码配置
extern const int AAC_DEC_MP4A_LATM_ANALYSIS;
extern const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT;
extern const int AAC_DEC_STKBUF_RELEASE_ENABLE;
extern const char config_bt_aac_dec_pcm24_enable;
extern const char config_bt_aac_dec_fifo_precision;
extern const int config_msbc_encode_used_software_enable;
extern const int midi_ctrl_support_multi_ch;
//opus 解码配置
extern const int CONFIG_OGG_OPUS_DEC_SUPPORT;
extern const int CONFIG_OGG_OPUS_DEC_FORMAT_MODE;
extern const int CONFIG_OGG_OPUS_DEC_SET_CBR_PACKET_LEN;
extern const int OPUS_SRINDEX;
/*
 *******************************************************************
 *						record Config
 *******************************************************************
 */
extern const int CONFIG_MAX_ENC_DATA_CACHE_SIZE;
extern const u32 CONFIG_WRITE_FILE_BUF_SIZE;
extern const u32 CONFIG_WRITE_FILE_BLOCK_SIZE;

extern const int config_write_file_log_enable;

/*
 *******************************************************************
 *						Audio Mic Capless Config
 *******************************************************************
 */
extern const u8 const_mic_capless_open_delay_debug;
extern const u8 const_mic_capless_trim_delay_debug;





/*
 *******************************************************************
 *						Audio LLNS_DNS Config
 *******************************************************************
 */
extern const u32 LLNS_DNS_SUPPORT_SAMPLE_RATE;
extern const u32 NN_TABLE_SELECT;
extern const u32 const_llns_dns_set_param_mode;
extern const float const_llns_dns_param_0;
extern const float const_llns_dns_param_1;
extern const u32 const_llns_dns_param_2;
#endif
