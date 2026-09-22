#include "app_config.h"

// *INDENT-OFF*



c_SRC_FILES +=       audio/framework/plugs/source/adc_file.c       audio/framework/nodes/volume_node.c 
#if TCFG_PLC_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/plc_node.c
#endif

#if TCFG_AUDIO_DEBUG_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/nodes/audio_debug_node.c

#endif


#if (TCFG_ESCO_RX_NODE_ENABLE || TCFG_ESCO_TX_NODE_ENABLE)
c_SRC_FILES +=       audio/framework/plugs/source/esco_file.c       audio/framework/nodes/esco_tx_node.c
#endif

#if TCFG_BT_SUPPORT_A2DP
c_SRC_FILES +=       audio/framework/plugs/source/a2dp_file.c       audio/framework/plugs/source/a2dp_streamctrl.c
#endif

#if TCFG_VIRTUAL_UDISK_NODE
c_SRC_FILES +=       audio/framework/nodes/virtual_udisk_node.c
#endif

#if TCFG_NS_NODE_ENABLE || TCFG_NS_NODE_LITE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/ns_node.c
#endif

#if TCFG_CHANNEL_SWAP_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/channle_swap_node.c
#endif

#if (TCFG_EFFECT_DEV0_NODE_ENABLE ||           TCFG_EFFECT_DEV1_NODE_ENABLE ||           TCFG_EFFECT_DEV2_NODE_ENABLE ||           TCFG_EFFECT_DEV3_NODE_ENABLE ||           TCFG_EFFECT_DEV4_NODE_ENABLE)
c_SRC_FILES +=       audio/framework/nodes/effect_dev_node_common.c
#endif

#if TCFG_EFFECT_DEV0_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/effect_dev0_node.c
#endif
#if TCFG_EFFECT_DEV1_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/effect_dev1_node.c
#endif
#if TCFG_EFFECT_DEV2_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/effect_dev2_node.c
#endif
#if TCFG_EFFECT_DEV3_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/effect_dev3_node.c
#endif
#if TCFG_EFFECT_DEV4_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/effect_dev4_node.c
#endif
#if TCFG_SINK_DEV0_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/sink_dev0_node.c
#endif
#if TCFG_SINK_DEV1_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/sink_dev1_node.c
#endif
#if TCFG_SINK_DEV2_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/sink_dev2_node.c
#endif
#if TCFG_SINK_DEV3_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/sink_dev3_node.c
#endif
#if TCFG_SINK_DEV4_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/sink_dev4_node.c
#endif

#if TCFG_SOURCE_DEV0_NODE_ENABLE
c_SRC_FILES +=       audio/framework/plugs/source/source_dev0_file.c
#endif
#if TCFG_SOURCE_DEV1_NODE_ENABLE
c_SRC_FILES +=       audio/framework/plugs/source/source_dev1_file.c
#endif
#if TCFG_SOURCE_DEV2_NODE_ENABLE
c_SRC_FILES +=       audio/framework/plugs/source/source_dev2_file.c
#endif
#if TCFG_SOURCE_DEV3_NODE_ENABLE
c_SRC_FILES +=       audio/framework/plugs/source/source_dev3_file.c
#endif
#if TCFG_SOURCE_DEV4_NODE_ENABLE
c_SRC_FILES +=       audio/framework/plugs/source/source_dev4_file.c
#endif

#if TCFG_AGC_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/agc_node.c
#endif

#if TCFG_SURROUND_DEMO_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/surround_demo_node.c
#endif

#if TCFG_LHDC_X_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/lhdc_x_node.c
#endif

#if TCFG_AUDIO_CVP_SMS_ANS_MODE || TCFG_AUDIO_CVP_SMS_DNS_MODE
c_SRC_FILES +=       audio/framework/nodes/cvp_sms_node.c
#endif

c_SRC_FILES +=       audio/framework/nodes/cvp_dms_node.c

#if TCFG_AUDIO_CVP_3MIC_MODE
c_SRC_FILES +=       audio/framework/nodes/cvp_3mic_node.c
#endif

#if TCFG_AUDIO_CVP_DEVELOP_ENABLE
c_SRC_FILES +=       audio/framework/nodes/cvp_develop_node.c
#endif

#if TCFG_UART_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/uart_node.c
#endif

#if TCFG_AUDIO_CVP_V3_MODE
c_SRC_FILES +=       audio/framework/nodes/cvp_v3_node.c
#endif

#if TCFG_AUDIO_CVP_SMS_VF_MODE
c_SRC_FILES +=       audio/framework/nodes/cvp_sms_vf_node.c
#endif

#if TCFG_DNS_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/dns_node.c
#endif

#if TCFG_AI_TX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/ai_tx_node.c       audio/framework/nodes/a2dp_tx_node.c
#endif

#if TCFG_INTERCOM_TX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/intercom_tx_node.c
#endif

#if TCFG_VIDEO_TX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/video_tx_node.c       audio/interface/recoder/video_voice_recoder.c
#endif

#if TCFG_VIDEO_RX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/video_rx_file.c       audio/interface/player/video_rx_player.c
#endif

#if TCFG_AI_RX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/ai_rx_file.c 	  audio/interface/player/ai_rx_player.c
#endif

#if TCFG_INTERCOM_RX_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/intercom_rx_file.c 	  audio/interface/player/intercom_rx_player.c
#endif

#if TCFG_DATA_EXPORT_NODE_ENABLE
c_SRC_FILES +=       audio/framework/nodes/data_export_node.c
#endif

#if TCFG_LOCAL_TWS_ENABLE
c_SRC_FILES +=       audio/framework/nodes/local_tws_source_node.c       audio/framework/plugs/source/local_tws_file.c 	  audio/interface/player/local_tws_player.c
#endif

#if EXPORT_PLATFORM_AUDIO_PDM_ENABLE
#if TCFG_PDM_NODE_ENABLE
c_SRC_FILES +=     audio/framework/plugs/source/pdm_mic_file.c
#endif
#endif

#if EXPORT_PLATFORM_AUDIO_SPDIF_ENABLE
#if TCFG_APP_SPDIF_EN
c_SRC_FILES += 	  audio/framework/plugs/source/spdif_file.c
#endif
#endif


// Audio Test Tool Kit
#if TCFG_AUDIO_MIC_DUT_ENABLE
c_SRC_FILES += 	  audio/test_tools/mic_dut_process.c
#endif

#if AUDIO_ENC_MPT_SELF_ENABLE
c_SRC_FILES += 	  audio/test_tools/audio_enc_mpt_self.c 	  audio/test_tools/audio_enc_mpt_cvp_ctr.c
#endif

#if TCFG_AEC_TOOL_ONLINE_ENABLE
c_SRC_FILES += 	  audio/test_tools/cvp_tool.c
#endif

#if TCFG_AUDIO_DUT_ENABLE
c_SRC_FILES += 	  audio/test_tools/audio_dut_control.c 	  audio/test_tools/audio_dut_control_old.c
#endif

// Audio Common
c_SRC_FILES += 	  audio/common/audio_node_config.c 	  audio/common/audio_dvol.c 	  audio/common/audio_general.c 	  audio/common/audio_build_needed.c 	  audio/common/online_debug/aud_data_export.c 	  audio/common/online_debug/audio_online_debug.c 	  audio/common/online_debug/audio_capture.c 	  audio/common/audio_plc.c 	  audio/common/audio_noise_gate.c 	  audio/common/audio_ns.c 	  audio/common/audio_utils.c 	  audio/common/audio_export_demo.c 	  audio/common/amplitude_statistic.c 	  audio/common/frame_length_adaptive.c 	  audio/common/bt_audio_energy_detection.c 	  audio/common/audio_event_handler.c 	  audio/common/debug/audio_debug.c 	  audio/common/power/mic_power_manager.c 	  audio/common/audio_volume_mixer.c 	  audio/common/pcm_data/sine_pcm.c 
#if AUDIO_IIS_LRCLK_CAPTURE_EN
c_SRC_FILES += 	  audio/common/audio_iis_lrclk_capture.c
#endif

#if TCFG_AUDIO_MIC_DUT_ENABLE
c_SRC_FILES += 	  audio/common/online_debug/aud_mic_dut.c
#endif

#if (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART)
c_SRC_FILES += 	  audio/common/uartPcmSender.c
#endif

#if 0
c_SRC_FILES += 	  audio/common/demo/codec_demo/audio_enc_file_demo.c 	  audio/common/demo/codec_demo/audio_file_dec_demo.c 	  audio/common/demo/codec_demo/audio_frame_codec_demo.c 	  audio/common/demo/audio_eq_demo.c 	  audio/common/demo/codec_demo/audio_msbc_hw_codec_demo.c 	  audio/common/demo/codec_demo/audio_msbc_sw_codec_demo.c 	  audio/common/demo/codec_demo/audio_sbc_codec_demo.c 	  audio/common/demo/audio_alink_demo.c 	  audio/common/demo/codec_demo/audio_speex_codec_demo.c 	  audio/common/demo/codec_demo/audio_opus_codec_demo.c 
#endif

#if EXPORT_PLATFORM_HW_MATH_VERSION == 2
c_SRC_FILES += 	  audio/common/demo/hw_math_v2_demo.c
#endif


#if EXPORT_PLATFORM_AUDIO_WM8978_ENABLE
#if 0
c_SRC_FILES += 	  audio/common/wm8978/wm8978.c 	  audio/common/wm8978/iic.c
#endif
#endif

// Audio Player
#if TCFG_APP_IIS_EN || TCFG_APP_DSP_EN
c_SRC_FILES += 	  audio/interface/player/iis_player.c
#endif

c_SRC_FILES += 	  audio/interface/player/tone_player.c 	  audio/interface/player/ring_player.c 	  audio/interface/player/a2dp_player.c 	  audio/interface/player/esco_player.c 	  audio/interface/player/key_tone_player.c 	  audio/interface/player/dev_flow_player.c 	  audio/interface/player/adda_loop_player.c       audio/interface/player/reference_time.c 
#if TCFG_APP_LINEIN_EN
c_SRC_FILES += 	  audio/interface/player/linein_player.c
#endif

#if TCFG_APP_LOUDSPEAKER_EN
c_SRC_FILES += 	  audio/interface/player/loudspeaker_iis_player.c 	  audio/interface/player/loudspeaker_mic_player.c
#endif

#if TCFG_APP_MIC_EN || TCFG_APP_DSP_EN
c_SRC_FILES += 	  audio/interface/player/mic_player.c
#endif

// Audio Recoder
c_SRC_FILES += 	  audio/interface/recoder/esco_recoder.c 	  audio/interface/recoder/dev_flow_recoder.c 
#if (BT_MIC_EN || TCFG_APP_AI_TOOLS)
c_SRC_FILES += 	  audio/interface/recoder/ai_voice_recoder.c
#endif

#if TCFG_INTERCOM_TX_NODE_ENABLE
c_SRC_FILES += 	  audio/interface/recoder/intercom_voice_recoder.c
#endif

c_SRC_FILES += 	  audio/interface/user_defined/audio_dsp_low_latency_player.c

#if EXPORT_PLATFORM_LE_AUDIO_ENABLE
#if TCFG_LE_AUDIO_APP_CONFIG
c_SRC_FILES += 	audio/interface/player/le_audio_player.c 	audio/framework/nodes/le_audio_source.c 	audio/framework/plugs/source/le_audio_file.c 	audio/le_audio/le_audio_stream.c 	audio/interface/recoder/le_audio_recorder.c 	audio/interface/recoder/le_audio_mix_mic_recorder.c 	audio/le_audio/le_audio_plc_test.c
#endif
#endif

c_SRC_FILES += 	audio/midi/midi_event.c 	audio/midi/midi_lib_api.c 	audio/midi/midi_play.c 	audio/midi/audio_midi.c 	audio/midi/audio_dec_midi_file.c 	audio/midi/audio_dec_midi_ctrl.c

#if TCFG_MIDI_CONTROLLER_NODE_ENABLE
c_SRC_FILES += 	audio/framework/plugs/source/midi_controller_file.c

#endif

// Audio Effects
c_SRC_FILES +=       audio/effect/eq_config.c 	  audio/effect/spk_eq.c 	  audio/effect/audio_voice_changer_api.c 	  audio/effect/esco_ul_voice_changer.c 	  audio/effect/bass_treble.c 	  audio/effect/audio_dc_offset_remove.c 	  audio/effect/effects_adj.c 	  audio/effect/user_effects_adj.c 	  audio/effect/effects_dev.c 	  audio/effect/effects_default_param.c 	  audio/effect/node_param_update.c 	  audio/effect/scene_update.c 
// ICSD
#if EXPORT_PLATFORM_ICSD_ENABLE
#if TCFG_AUDIO_ANC_ENABLE
c_SRC_FILES += 	audio/common/icsd/adt/icsd_adt.c 	audio/common/icsd/adt/icsd_adt_app.c 	audio/common/icsd/adt/icsd_adt_config.c 	audio/common/icsd/adt/icsd_adt_alg.c 	audio/common/icsd/adt/icsd_adt_demo.c 	audio/common/icsd/anc/icsd_anc_app.c 	audio/common/icsd/anc/icsd_anc_board.c 	audio/common/icsd/anc/icsd_anc_data.c 	audio/common/icsd/anc/icsd_anc_interactive.c 	audio/common/icsd/common/icsd_common.c 	audio/common/icsd/dot/icsd_dot_app.c 	audio/common/icsd/dot/icsd_dot.c 	audio/common/icsd/common_v2/icsd_common_v2.c 	audio/common/icsd/common_v2/icsd_common_v2_app.c 	audio/common/icsd/anc_v2/icsd_anc_v2.c 	audio/common/icsd/anc_v2/icsd_anc_v2_app.c 	audio/common/icsd/anc_v2/icsd_anc_v2_interactive.c 	audio/common/icsd/config/icsd_anc_v2_config.c 	audio/common/icsd/rt_anc/rt_anc.c 	audio/common/icsd/rt_anc/rt_anc_app.c 	audio/common/icsd/rt_anc/rt_anc_config.c 	audio/common/icsd/tool/anc_ext_tool.c 	audio/common/icsd/tool/anc_ext_tool_file.c 	audio/common/icsd/aeq/icsd_aeq_app.c 	audio/common/icsd/aeq/icsd_aeq.c 	audio/common/icsd/aeq/icsd_aeq_config.c 	audio/common/icsd/afq/icsd_afq_app.c 	audio/common/icsd/afq/icsd_afq.c 	audio/common/icsd/afq/icsd_afq_config.c 	audio/common/icsd/cmp/icsd_cmp_app.c 	audio/common/icsd/cmp/icsd_cmp_config.c 	audio/common/icsd/demo/icsd_demo.c 	audio/common/icsd/ein/icsd_ein_config.c 	audio/common/icsd/vdt/icsd_vdt_config.c 	audio/common/icsd/wat/icsd_wat_config.c 	audio/common/icsd/wind/icsd_wind_config.c 	audio/common/icsd/avc/icsd_avc_config.c 
#endif
#endif

// Audio ANC
#if EXPORT_PLATFORM_ANC_ENABLE
#if TCFG_AUDIO_ANC_ENABLE
c_SRC_FILES += 	  audio/anc/audio_anc_fade_ctr.c 	  audio/anc/audio_anc_common_plug.c 	  audio/anc/audio_anc_debug_tool.c 	  audio/anc/audio_anc_mult_scene.c 	  audio/anc/audio_anc_common.c 
#endif
#endif

// Smart Voice
#if EXPORT_PLATFORM_AUDIO_SMART_VOICE_ENABLE
#if TCFG_SMART_VOICE_ENABLE
c_SRC_FILES += 	audio/smart_voice/smart_voice_core.c 	audio/smart_voice/smart_voice_config.c 	audio/smart_voice/jl_kws_platform.c 	audio/smart_voice/user_asr.c 	audio/smart_voice/voice_mic_data.c 	audio/smart_voice/voice_mic_aec.c 	audio/smart_voice/kws_event.c 	audio/smart_voice/nn_vad.c
#endif
#endif

// KWS(yes/no)
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
c_SRC_FILES += 	audio/jl_kws/jl_kws_main.c 	audio/jl_kws/jl_kws_audio.c 	audio/jl_kws/jl_kws_algo.c 	audio/jl_kws/jl_kws_event.c 
#endif


#if EXPORT_PLATFORM_AUDIO_SPATIAL_EFFECT_ENABLE
#if TCFG_SPATIAL_AUDIO_ENABLE
c_SRC_FILES += 	audio/framework/nodes/spatial_effects_node.c 	audio/common/online_debug/aud_spatial_effect_dut.c 
c_SRC_FILES += 	audio/effect/spatial_effect/spatial_effect.c 	audio/effect/spatial_effect/spatial_effect_imu.c 	audio/effect/spatial_effect/spatial_effect_tws.c 	audio/effect/spatial_effect/spatial_imu_trim.c 	audio/effect/spatial_effect/spatial_effects_process.c 
#endif
#endif

#if EXPORT_SOMATOSENSORY_ENABLE
c_SRC_FILES += 	audio/effect/somatosensory/audio_somatosensory.c 
#endif

// Clear Voice Process(AEC/NLP/NS/AGC...)
c_SRC_FILES += 	  audio/CVP/audio_aec.c 	  audio/CVP/audio_cvp.c 	  audio/CVP/audio_cvp_dms.c 	  audio/CVP/audio_cvp_3mic.c 	  audio/CVP/audio_cvp_online.c 	  audio/CVP/audio_cvp_demo.c 	  audio/CVP/audio_cvp_develop.c 	  audio/CVP/audio_cvp_sync.c 	  audio/CVP/audio_cvp_ais_3mic.c 	  audio/CVP/audio_cvp_ref_task.c 	  audio/CVP/audio_cvp_config.c 
#if TCFG_AUDIO_CVP_V3_MODE
c_SRC_FILES += 	  audio/CVP/audio_cvp_v3.c
#endif

#if TCFG_AUDIO_CVP_SMS_VF_MODE
c_SRC_FILES += 	  audio/CVP/audio_cvp_sms_vf.c
#endif

#if TCFG_USER_TWS_ENABLE
c_SRC_FILES += 	  audio/interface/player/tws_tone_player.c
#endif

// ALINK BUILD
#if EXPORT_PLATFORM_AUDIO_ALINK_ENABLE
#if TCFG_IIS_NODE_ENABLE || TCFG_TDM_TX_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/nodes/iis_node.c
#endif

#if TCFG_IIS_RX_NODE_ENABLE || TCFG_TDM_RX_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/plugs/source/iis_file.c
#endif

#if TCFG_MULTI_CH_IIS_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/nodes/multi_ch_iis_node.c
#endif
#if TCFG_MULTI_CH_IIS_RX_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/plugs/source/multi_ch_iis_file.c
#endif

#endif

// SPDIF BUILD
#if EXPORT_PLATFORM_AUDIO_SPDIF_ENABLE
#if TCFG_SPDIF_MASTER_NODE_ENABLE
c_SRC_FILES += 	  audio/framework/nodes/spdif_node.c
#endif
#endif

#if TCFG_APP_FM_EN
c_SRC_FILES += 	  audio/interface/player/fm_player.c 	  audio/framework/plugs/source/fm_file.c 
#endif

#if EXPORT_PLATFORM_AUDIO_TDM_ENABLE
c_SRC_FILES += 	  audio/interface/player/tdm_player.c 	  audio/framework/plugs/source/tdm_file.c 	  audio/framework/nodes/tdm_node.c 
#endif

#if TCFG_APP_MUSIC_EN || TCFG_APP_RECORD_EN
c_SRC_FILES +=       audio/interface/player/file_player.c 
#endif

#if TCFG_APP_RECORD_EN || TCFG_MIX_RECORD_ENABLE
c_SRC_FILES +=       audio/interface/recoder/file_recorder.c 
#endif

c_SRC_FILES += 	  audio/framework/plugs/source/linein_file.c 
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
c_SRC_FILES += 	  audio/framework/plugs/source/pc_spk_file.c 	  audio/interface/player/pc_spk_player.c 
#endif

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
c_SRC_FILES += 	  audio/framework/nodes/pc_mic_node.c 	  audio/interface/recoder/pc_mic_recoder.c 
#endif

#if TCFG_PITCH_SPEED_NODE_ENABLE
c_SRC_FILES += 	  audio/effect/audio_pitch_speed_api.c 
#endif



#if TCFG_AUDIO_VAD_NODE_ENABLE
c_SRC_FILES +=         audio/framework/nodes/vad_node.c 
#endif

// Audio Effects params update demo
#if EXPORT_PLATFORM_AUDIO_EFFECT_DEMO_ENABLE
#if 0
c_SRC_FILES +=       audio/effect/demo/autotune_demo.c       audio/effect/demo/bass_treble_demo.c       audio/effect/demo/chorus_demo.c       audio/effect/demo/crossover_demo.c       audio/effect/demo/drc_advance_demo.c       audio/effect/demo/drc_demo.c       audio/effect/demo/dynamic_eq_demo.c       audio/effect/demo/echo_demo.c       audio/effect/demo/energy_detect_demo.c       audio/effect/demo/eq_demo.c       audio/effect/demo/frequency_shift_howling_demo.c       audio/effect/demo/gain_demo.c       audio/effect/demo/harmonic_exciter_demo.c       audio/effect/demo/multiband_drc_demo.c       audio/effect/demo/noisegate_demo.c       audio/effect/demo/notch_howling_demo.c       audio/effect/demo/pitch_speed_demo.c       audio/effect/demo/reverb_advance_demo.c       audio/effect/demo/reverb_demo.c       audio/effect/demo/spectrum_demo.c       audio/effect/demo/stereo_widener_demo.c       audio/effect/demo/surround_demo.c       audio/effect/demo/virtual_bass_demo.c       audio/effect/demo/voice_changer_demo.c       audio/effect/demo/lhdc_x_demo.c 
#endif
#endif

c_SRC_FILES += 	audio/cpu/common.c 



// *INDENT-OFF*

#if EXPORT_LE_AUDIO_SUPPORT

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
c_SRC_FILES += 	apps/common/le_audio/big.c
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN | LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
c_SRC_FILES += 	apps/common/le_audio/cig.c
#endif

#if TCFG_LE_AUDIO_APP_CONFIG
c_SRC_FILES += 	apps/common/le_audio/wireless_trans_manager.c
#endif

#endif

// *INDENT-OFF*

#if CONFIG_VIDEO_ENABLE


#if TCFG_VIDEO0_ENABLE

c_SRC_FILES += apps/common/video/avilib/avilib.c apps/common/video/avilib/platform.c
c_SRC_FILES += apps/common/video/libmov/api/libmov_alloc.c apps/common/video/libmov/api/mp4_file_io.c apps/common/video/libmov/api/mp4_muxer.c apps/common/video/libmov/src/codec/mpeg4-aac-asc.c apps/common/video/libmov/src/codec/mpeg4-aac.c apps/common/video/libmov/src/codec/mpeg4-annexbtomp4.c apps/common/video/libmov/src/codec/mpeg4-avc.c apps/common/video/libmov/src/libmov/fmp4-reader.c apps/common/video/libmov/src/libmov/fmp4-writer.c apps/common/video/libmov/src/libmov/mov-avc1.c apps/common/video/libmov/src/libmov/mov-dinf.c apps/common/video/libmov/src/libmov/mov-elst.c apps/common/video/libmov/src/libmov/mov-esds.c apps/common/video/libmov/src/libmov/mov-ftyp.c apps/common/video/libmov/src/libmov/mov-hdlr.c apps/common/video/libmov/src/libmov/mov-hdr.c apps/common/video/libmov/src/libmov/mov-iods.c apps/common/video/libmov/src/libmov/mov-leva.c apps/common/video/libmov/src/libmov/mov-mdhd.c apps/common/video/libmov/src/libmov/mov-mehd.c apps/common/video/libmov/src/libmov/mov-mfhd.c apps/common/video/libmov/src/libmov/mov-minf.c apps/common/video/libmov/src/libmov/mov-mvhd.c apps/common/video/libmov/src/libmov/mov-opus.c apps/common/video/libmov/src/libmov/mov-reader.c apps/common/video/libmov/src/libmov/mov-sidx.c apps/common/video/libmov/src/libmov/mov-stco.c apps/common/video/libmov/src/libmov/mov-stsc.c apps/common/video/libmov/src/libmov/mov-stsd.c apps/common/video/libmov/src/libmov/mov-stss.c apps/common/video/libmov/src/libmov/mov-stsz.c apps/common/video/libmov/src/libmov/mov-stts.c apps/common/video/libmov/src/libmov/mov-tag.c apps/common/video/libmov/src/libmov/mov-tfdt.c apps/common/video/libmov/src/libmov/mov-tfhd.c apps/common/video/libmov/src/libmov/mov-tfra.c apps/common/video/libmov/src/libmov/mov-tkhd.c apps/common/video/libmov/src/libmov/mov-track.c apps/common/video/libmov/src/libmov/mov-trex.c apps/common/video/libmov/src/libmov/mov-trun.c apps/common/video/libmov/src/libmov/mov-tx3g.c apps/common/video/libmov/src/libmov/mov-udta.c apps/common/video/libmov/src/libmov/mov-vpcc.c apps/common/video/libmov/src/libmov/mov-writer.c
c_SRC_FILES += apps/common/video/mpi_app/audio/app_video_audio_rec.c apps/common/video/mpi_app/audio/pcm_write_demo.c apps/common/video/mpi_app/camera/app_camera_display.c apps/common/video/mpi_app/camera/app_camera_event.c apps/common/video/mpi_app/camera/app_camera_mpi.c apps/common/video/mpi_app/camera/app_camera_net_stream.c apps/common/video/mpi_app/camera/app_camera_osd.c apps/common/video/mpi_app/camera/app_camera_photo.c apps/common/video/mpi_app/camera/app_camera_rec.c apps/common/video/mpi_app/camera/app_camera_src.c apps/common/video/mpi_app/lvgl_video_obj/lvgl_video_init.c apps/common/video/mpi_app/mpi_camera_demo.c apps/common/video/mpi_app/mpi_playback_demo.c apps/common/video/mpi_app/playback/app_playback_demux.c apps/common/video/mpi_app/playback/app_playback_event.c apps/common/video/mpi_app/playback/app_playback_image.c apps/common/video/mpi_app/playback/app_playback_mpi.c apps/common/video/mpi_app/playback/app_playback_video.c
c_SRC_FILES += apps/common/video/mpi_demo/decoder/mpi_jpeg_vdec.c apps/common/video/mpi_demo/decoder/mpi_vdec_vpss.c apps/common/video/mpi_demo/encoder/mpi_vi_vpss.c apps/common/video/mpi_demo/encoder/mpi_vi_vpss_venc.c apps/common/video/mpi_demo/h264/mpi_h264_vpss_vo.c apps/common/video/mpi_demo/h264/mpi_vi_vpss_h264.c apps/common/video/mpi_demo/h264/mpi_vi_vpss_h264_avi.c
#endif


#endif



// *INDENT-OFF*

#if CONFIG_LV9_ENABLE


#if CONFIG_LVGL_UI_ENABLE

c_SRC_FILES += apps/common/ui/lvgl_v9/demos/amap/amap_canvas.c apps/common/ui/lvgl_v9/demos/benchmark/assets/img_benchmark_avatar.c apps/common/ui/lvgl_v9/demos/benchmark/assets/img_benchmark_lvgl_logo_argb.c apps/common/ui/lvgl_v9/demos/benchmark/assets/img_benchmark_lvgl_logo_rgb.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_12_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_14_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_16_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_18_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_20_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_24_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/assets/lv_font_benchmark_montserrat_26_aligned.c apps/common/ui/lvgl_v9/demos/benchmark/lv_demo_benchmark.c apps/common/ui/lvgl_v9/demos/gltf/lv_demo_gltf.c apps/common/ui/lvgl_v9/demos/keypad_encoder/lv_demo_keypad_encoder.c apps/common/ui/lvgl_v9/demos/lv_demos.c apps/common/ui/lvgl_v9/demos/map/gps_driver.c apps/common/ui/lvgl_v9/demos/map/graph_tile.c apps/common/ui/lvgl_v9/demos/map/map_route.c apps/common/ui/lvgl_v9/demos/map/map_tiles.c apps/common/ui/lvgl_v9/demos/map/nav_guide.c apps/common/ui/lvgl_v9/demos/map/nav_hud.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_corner_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_list_pause.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_list_play.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_list_play_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_loop.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_loop_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_next.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_next_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_pause.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_pause_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_play.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_play_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_prev.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_prev_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_rnd.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_btn_rnd_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_corner_left.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_corner_left_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_corner_right.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_corner_right_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_1.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_1_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_2.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_2_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_3.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_cover_3_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_1.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_1_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_2.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_2_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_3.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_3_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_4.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_icon_4_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_list_border.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_list_border_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_logo.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_slider_knob.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_slider_knob_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_wave_bottom.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_wave_bottom_large.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_wave_top.c apps/common/ui/lvgl_v9/demos/music/assets/img_lv_demo_music_wave_top_large.c apps/common/ui/lvgl_v9/demos/music/lv_demo_music.c apps/common/ui/lvgl_v9/demos/music/lv_demo_music_list.c apps/common/ui/lvgl_v9/demos/music/lv_demo_music_main.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_arc_bg.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_argb8888.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_argb8888_premultiplied.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_i1.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_l8.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_rgb565.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_rgb565_swapped.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_rgb565a8.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_rgb888.c apps/common/ui/lvgl_v9/demos/render/assets/img_render_lvgl_logo_xrgb8888.c apps/common/ui/lvgl_v9/demos/render/lv_demo_render.c apps/common/ui/lvgl_v9/demos/stress/lv_demo_stress.c apps/common/ui/lvgl_v9/demos/vector_graphic/assets/img_demo_vector_avatar.c apps/common/ui/lvgl_v9/demos/vector_graphic/lv_demo_vector_graphic.c apps/common/ui/lvgl_v9/demos/widgets/assets/img_clothes.c apps/common/ui/lvgl_v9/demos/widgets/assets/img_demo_widgets_avatar.c apps/common/ui/lvgl_v9/demos/widgets/assets/img_demo_widgets_needle.c apps/common/ui/lvgl_v9/demos/widgets/assets/img_lvgl_logo.c apps/common/ui/lvgl_v9/demos/widgets/lv_demo_widgets.c apps/common/ui/lvgl_v9/demos/widgets/lv_demo_widgets_analytics.c apps/common/ui/lvgl_v9/demos/widgets/lv_demo_widgets_components.c apps/common/ui/lvgl_v9/demos/widgets/lv_demo_widgets_profile.c apps/common/ui/lvgl_v9/demos/widgets/lv_demo_widgets_shop.c apps/common/ui/lvgl_v9/examples/anim/lv_example_anim_1.c apps/common/ui/lvgl_v9/examples/anim/lv_example_anim_2.c apps/common/ui/lvgl_v9/examples/anim/lv_example_anim_3.c apps/common/ui/lvgl_v9/examples/anim/lv_example_anim_4.c apps/common/ui/lvgl_v9/examples/anim/lv_example_anim_timeline_1.c apps/common/ui/lvgl_v9/examples/assets/animimg001.c apps/common/ui/lvgl_v9/examples/assets/animimg002.c apps/common/ui/lvgl_v9/examples/assets/animimg003.c apps/common/ui/lvgl_v9/examples/assets/emoji/img_emoji_F617.c apps/common/ui/lvgl_v9/examples/assets/img_caret_down.c apps/common/ui/lvgl_v9/examples/assets/img_cogwheel_argb.c apps/common/ui/lvgl_v9/examples/assets/img_cogwheel_indexed16.c apps/common/ui/lvgl_v9/examples/assets/img_cogwheel_rgb.c apps/common/ui/lvgl_v9/examples/assets/img_hand.c apps/common/ui/lvgl_v9/examples/assets/img_multilang_like.c apps/common/ui/lvgl_v9/examples/assets/img_skew_strip.c apps/common/ui/lvgl_v9/examples/assets/img_star.c apps/common/ui/lvgl_v9/examples/assets/img_svg_img.c apps/common/ui/lvgl_v9/examples/assets/img_transform_avatar_15.c apps/common/ui/lvgl_v9/examples/assets/imgbtn_left.c apps/common/ui/lvgl_v9/examples/assets/imgbtn_mid.c apps/common/ui/lvgl_v9/examples/assets/imgbtn_right.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_bubble.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_button.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_click.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_draw.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_streak.c apps/common/ui/lvgl_v9/examples/event/lv_example_event_trickle.c apps/common/ui/lvgl_v9/examples/get_started/lv_example_get_started_1.c apps/common/ui/lvgl_v9/examples/get_started/lv_example_get_started_2.c apps/common/ui/lvgl_v9/examples/get_started/lv_example_get_started_3.c apps/common/ui/lvgl_v9/examples/get_started/lv_example_get_started_4.c apps/common/ui/lvgl_v9/examples/grad/lv_example_grad_1.c apps/common/ui/lvgl_v9/examples/grad/lv_example_grad_2.c apps/common/ui/lvgl_v9/examples/grad/lv_example_grad_3.c apps/common/ui/lvgl_v9/examples/grad/lv_example_grad_4.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_1.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_2.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_3.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_4.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_5.c apps/common/ui/lvgl_v9/examples/layouts/flex/lv_example_flex_6.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_1.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_2.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_3.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_4.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_5.c apps/common/ui/lvgl_v9/examples/layouts/grid/lv_example_grid_6.c apps/common/ui/lvgl_v9/examples/libs/barcode/lv_example_barcode_1.c apps/common/ui/lvgl_v9/examples/libs/bmp/lv_example_bmp_1.c apps/common/ui/lvgl_v9/examples/libs/ffmpeg/lv_example_ffmpeg_1.c apps/common/ui/lvgl_v9/examples/libs/ffmpeg/lv_example_ffmpeg_2.c apps/common/ui/lvgl_v9/examples/libs/freetype/lv_example_freetype_1.c apps/common/ui/lvgl_v9/examples/libs/freetype/lv_example_freetype_2.c apps/common/ui/lvgl_v9/examples/libs/freetype/lv_example_freetype_3.c apps/common/ui/lvgl_v9/examples/libs/gif/img_bulb_gif.c apps/common/ui/lvgl_v9/examples/libs/gif/lv_example_gif_1.c apps/common/ui/lvgl_v9/examples/libs/gltf/lv_example_gltf_1.c apps/common/ui/lvgl_v9/examples/libs/gltf/lv_example_gltf_2.c apps/common/ui/lvgl_v9/examples/libs/gltf/lv_example_gltf_3.c apps/common/ui/lvgl_v9/examples/libs/gstreamer/lv_example_gstreamer_1.c apps/common/ui/lvgl_v9/examples/libs/libjpeg_turbo/lv_example_libjpeg_turbo_1.c apps/common/ui/lvgl_v9/examples/libs/libpng/img_png_demo.c apps/common/ui/lvgl_v9/examples/libs/libpng/lv_example_libpng_1.c apps/common/ui/lvgl_v9/examples/libs/libwebp/lv_example_libwebp_1.c apps/common/ui/lvgl_v9/examples/libs/lodepng/img_wink_png.c apps/common/ui/lvgl_v9/examples/libs/lodepng/lv_example_lodepng_1.c apps/common/ui/lvgl_v9/examples/libs/qrcode/lv_example_qrcode_1.c apps/common/ui/lvgl_v9/examples/libs/rlottie/lv_example_rlottie_1.c apps/common/ui/lvgl_v9/examples/libs/rlottie/lv_example_rlottie_2.c apps/common/ui/lvgl_v9/examples/libs/rlottie/lv_example_rlottie_approve.c apps/common/ui/lvgl_v9/examples/libs/svg/lv_example_svg_1.c apps/common/ui/lvgl_v9/examples/libs/svg/lv_example_svg_2.c apps/common/ui/lvgl_v9/examples/libs/svg/lv_example_svg_3.c apps/common/ui/lvgl_v9/examples/libs/tiny_ttf/lv_example_tiny_ttf_1.c apps/common/ui/lvgl_v9/examples/libs/tiny_ttf/lv_example_tiny_ttf_2.c apps/common/ui/lvgl_v9/examples/libs/tiny_ttf/lv_example_tiny_ttf_3.c apps/common/ui/lvgl_v9/examples/libs/tiny_ttf/ubuntu_font.c apps/common/ui/lvgl_v9/examples/libs/tjpgd/lv_example_tjpgd_1.c apps/common/ui/lvgl_v9/examples/others/file_explorer/lv_example_file_explorer_1.c apps/common/ui/lvgl_v9/examples/others/file_explorer/lv_example_file_explorer_2.c apps/common/ui/lvgl_v9/examples/others/file_explorer/lv_example_file_explorer_3.c apps/common/ui/lvgl_v9/examples/others/font_manager/lv_example_font_manager_1.c apps/common/ui/lvgl_v9/examples/others/font_manager/lv_example_font_manager_2.c apps/common/ui/lvgl_v9/examples/others/font_manager/lv_example_font_manager_3.c apps/common/ui/lvgl_v9/examples/others/fragment/lv_example_fragment_1.c apps/common/ui/lvgl_v9/examples/others/fragment/lv_example_fragment_2.c apps/common/ui/lvgl_v9/examples/others/gestures/lv_example_gestures.c apps/common/ui/lvgl_v9/examples/others/gridnav/lv_example_gridnav_1.c apps/common/ui/lvgl_v9/examples/others/gridnav/lv_example_gridnav_2.c apps/common/ui/lvgl_v9/examples/others/gridnav/lv_example_gridnav_3.c apps/common/ui/lvgl_v9/examples/others/gridnav/lv_example_gridnav_4.c apps/common/ui/lvgl_v9/examples/others/gridnav/lv_example_gridnav_5.c apps/common/ui/lvgl_v9/examples/others/ime/lv_example_ime_pinyin_1.c apps/common/ui/lvgl_v9/examples/others/ime/lv_example_ime_pinyin_2.c apps/common/ui/lvgl_v9/examples/others/imgfilter/lv_example_image_filter.c apps/common/ui/lvgl_v9/examples/others/imgfont/lv_example_imgfont_1.c apps/common/ui/lvgl_v9/examples/others/monkey/lv_example_monkey_1.c apps/common/ui/lvgl_v9/examples/others/monkey/lv_example_monkey_2.c apps/common/ui/lvgl_v9/examples/others/monkey/lv_example_monkey_3.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_1.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_2.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_3.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_4.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_5.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_6.c apps/common/ui/lvgl_v9/examples/others/observer/lv_example_observer_7.c apps/common/ui/lvgl_v9/examples/others/snapshot/lv_example_snapshot_1.c apps/common/ui/lvgl_v9/examples/others/translation/lv_example_translation_1.c apps/common/ui/lvgl_v9/examples/others/translation/lv_example_translation_2.c apps/common/ui/lvgl_v9/examples/porting/jlgpu_filter_test.c apps/common/ui/lvgl_v9/examples/porting/jlgpu_image_filters.c apps/common/ui/lvgl_v9/examples/porting/lv_port_disp.c apps/common/ui/lvgl_v9/examples/porting/lv_port_disp_template.c apps/common/ui/lvgl_v9/examples/porting/lv_port_fs.c apps/common/ui/lvgl_v9/examples/porting/lv_port_fs_template.c apps/common/ui/lvgl_v9/examples/porting/lv_port_indev.c apps/common/ui/lvgl_v9/examples/porting/lv_port_indev_template.c apps/common/ui/lvgl_v9/examples/porting/lv_port_lcd.c apps/common/ui/lvgl_v9/examples/porting/lv_port_lcd_stm32_template.c apps/common/ui/lvgl_v9/examples/porting/lv_port_video_disp.c apps/common/ui/lvgl_v9/examples/porting/osal/lv_example_osal.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_1.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_2.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_3.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_4.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_5.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_6.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_7.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_8.c apps/common/ui/lvgl_v9/examples/scroll/lv_example_scroll_9.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_1.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_10.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_11.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_12.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_13.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_14.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_15.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_16.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_17.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_18.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_19.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_2.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_20.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_21.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_3.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_4.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_5.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_6.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_7.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_8.c apps/common/ui/lvgl_v9/examples/styles/lv_example_style_9.c apps/common/ui/lvgl_v9/examples/widgets/animimg/lv_example_animimg_1.c apps/common/ui/lvgl_v9/examples/widgets/arc/lv_example_arc_1.c apps/common/ui/lvgl_v9/examples/widgets/arc/lv_example_arc_2.c apps/common/ui/lvgl_v9/examples/widgets/arc/lv_example_arc_3.c apps/common/ui/lvgl_v9/examples/widgets/arclabel/lv_example_arclabel_1.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_1.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_2.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_3.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_4.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_5.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_6.c apps/common/ui/lvgl_v9/examples/widgets/bar/lv_example_bar_7.c apps/common/ui/lvgl_v9/examples/widgets/button/lv_example_button_1.c apps/common/ui/lvgl_v9/examples/widgets/button/lv_example_button_2.c apps/common/ui/lvgl_v9/examples/widgets/button/lv_example_button_3.c apps/common/ui/lvgl_v9/examples/widgets/buttonmatrix/lv_example_buttonmatrix_1.c apps/common/ui/lvgl_v9/examples/widgets/buttonmatrix/lv_example_buttonmatrix_2.c apps/common/ui/lvgl_v9/examples/widgets/buttonmatrix/lv_example_buttonmatrix_3.c apps/common/ui/lvgl_v9/examples/widgets/calendar/lv_example_calendar_1.c apps/common/ui/lvgl_v9/examples/widgets/calendar/lv_example_calendar_2.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_1.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_10.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_11.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_12.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_2.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_3.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_4.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_5.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_6.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_7.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_8.c apps/common/ui/lvgl_v9/examples/widgets/canvas/lv_example_canvas_9.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_1.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_2.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_3.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_4.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_5.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_6.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_7.c apps/common/ui/lvgl_v9/examples/widgets/chart/lv_example_chart_8.c apps/common/ui/lvgl_v9/examples/widgets/checkbox/lv_example_checkbox_1.c apps/common/ui/lvgl_v9/examples/widgets/checkbox/lv_example_checkbox_2.c apps/common/ui/lvgl_v9/examples/widgets/dropdown/lv_example_dropdown_1.c apps/common/ui/lvgl_v9/examples/widgets/dropdown/lv_example_dropdown_2.c apps/common/ui/lvgl_v9/examples/widgets/dropdown/lv_example_dropdown_3.c apps/common/ui/lvgl_v9/examples/widgets/image/lv_example_image_1.c apps/common/ui/lvgl_v9/examples/widgets/image/lv_example_image_2.c apps/common/ui/lvgl_v9/examples/widgets/image/lv_example_image_3.c apps/common/ui/lvgl_v9/examples/widgets/image/lv_example_image_4.c apps/common/ui/lvgl_v9/examples/widgets/image/lv_example_image_5.c apps/common/ui/lvgl_v9/examples/widgets/imagebutton/lv_example_imagebutton_1.c apps/common/ui/lvgl_v9/examples/widgets/keyboard/lv_example_keyboard_1.c apps/common/ui/lvgl_v9/examples/widgets/keyboard/lv_example_keyboard_2.c apps/common/ui/lvgl_v9/examples/widgets/keyboard/lv_example_keyboard_3.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_1.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_2.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_3.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_4.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_5.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_6.c apps/common/ui/lvgl_v9/examples/widgets/label/lv_example_label_7.c apps/common/ui/lvgl_v9/examples/widgets/led/lv_example_led_1.c apps/common/ui/lvgl_v9/examples/widgets/line/lv_example_line_1.c apps/common/ui/lvgl_v9/examples/widgets/list/lv_example_list_1.c apps/common/ui/lvgl_v9/examples/widgets/list/lv_example_list_2.c apps/common/ui/lvgl_v9/examples/widgets/lottie/lv_example_lottie_1.c apps/common/ui/lvgl_v9/examples/widgets/lottie/lv_example_lottie_2.c apps/common/ui/lvgl_v9/examples/widgets/lottie/lv_example_lottie_approve.c apps/common/ui/lvgl_v9/examples/widgets/menu/lv_example_menu_1.c apps/common/ui/lvgl_v9/examples/widgets/menu/lv_example_menu_2.c apps/common/ui/lvgl_v9/examples/widgets/menu/lv_example_menu_3.c apps/common/ui/lvgl_v9/examples/widgets/menu/lv_example_menu_4.c apps/common/ui/lvgl_v9/examples/widgets/menu/lv_example_menu_5.c apps/common/ui/lvgl_v9/examples/widgets/msgbox/lv_example_msgbox_1.c apps/common/ui/lvgl_v9/examples/widgets/msgbox/lv_example_msgbox_2.c apps/common/ui/lvgl_v9/examples/widgets/msgbox/lv_example_msgbox_3.c apps/common/ui/lvgl_v9/examples/widgets/roller/lv_example_roller_1.c apps/common/ui/lvgl_v9/examples/widgets/roller/lv_example_roller_2.c apps/common/ui/lvgl_v9/examples/widgets/roller/lv_example_roller_3.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_1.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_10.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_11.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_12.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_2.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_3.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_4.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_5.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_6.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_7.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_8.c apps/common/ui/lvgl_v9/examples/widgets/scale/lv_example_scale_9.c apps/common/ui/lvgl_v9/examples/widgets/slider/lv_example_slider_1.c apps/common/ui/lvgl_v9/examples/widgets/slider/lv_example_slider_2.c apps/common/ui/lvgl_v9/examples/widgets/slider/lv_example_slider_3.c apps/common/ui/lvgl_v9/examples/widgets/slider/lv_example_slider_4.c apps/common/ui/lvgl_v9/examples/widgets/span/lv_example_span_1.c apps/common/ui/lvgl_v9/examples/widgets/spinbox/lv_example_spinbox_1.c apps/common/ui/lvgl_v9/examples/widgets/spinner/lv_example_spinner_1.c apps/common/ui/lvgl_v9/examples/widgets/switch/lv_example_switch_1.c apps/common/ui/lvgl_v9/examples/widgets/switch/lv_example_switch_2.c apps/common/ui/lvgl_v9/examples/widgets/table/lv_example_table_1.c apps/common/ui/lvgl_v9/examples/widgets/table/lv_example_table_2.c apps/common/ui/lvgl_v9/examples/widgets/tabview/lv_example_tabview_1.c apps/common/ui/lvgl_v9/examples/widgets/tabview/lv_example_tabview_2.c apps/common/ui/lvgl_v9/examples/widgets/textarea/lv_example_textarea_1.c apps/common/ui/lvgl_v9/examples/widgets/textarea/lv_example_textarea_2.c apps/common/ui/lvgl_v9/examples/widgets/textarea/lv_example_textarea_3.c apps/common/ui/lvgl_v9/examples/widgets/textarea/lv_example_textarea_4.c apps/common/ui/lvgl_v9/examples/widgets/tileview/lv_example_tileview_1.c apps/common/ui/lvgl_v9/examples/widgets/win/lv_example_win_1.c apps/common/ui/lvgl_v9/src/core/lv_group.c apps/common/ui/lvgl_v9/src/core/lv_obj.c apps/common/ui/lvgl_v9/src/core/lv_obj_class.c apps/common/ui/lvgl_v9/src/core/lv_obj_draw.c apps/common/ui/lvgl_v9/src/core/lv_obj_event.c apps/common/ui/lvgl_v9/src/core/lv_obj_id_builtin.c apps/common/ui/lvgl_v9/src/core/lv_obj_pos.c apps/common/ui/lvgl_v9/src/core/lv_obj_property.c apps/common/ui/lvgl_v9/src/core/lv_obj_scroll.c apps/common/ui/lvgl_v9/src/core/lv_obj_style.c apps/common/ui/lvgl_v9/src/core/lv_obj_style_gen.c apps/common/ui/lvgl_v9/src/core/lv_obj_tree.c apps/common/ui/lvgl_v9/src/core/lv_observer.c apps/common/ui/lvgl_v9/src/core/lv_refr.c apps/common/ui/lvgl_v9/src/debugging/monkey/lv_monkey.c apps/common/ui/lvgl_v9/src/debugging/sysmon/lv_sysmon.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_display.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_fs.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_helpers.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_indev.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_indev_gesture.c apps/common/ui/lvgl_v9/src/debugging/test/lv_test_screenshot_compare.c apps/common/ui/lvgl_v9/src/debugging/vg_lite_tvg/vg_lite_matrix.c apps/common/ui/lvgl_v9/src/display/lv_display.c apps/common/ui/lvgl_v9/src/draw/convert/helium/lv_draw_buf_convert_helium.c apps/common/ui/lvgl_v9/src/draw/convert/lv_draw_buf_convert.c apps/common/ui/lvgl_v9/src/draw/convert/neon/lv_draw_buf_convert_neon.c apps/common/ui/lvgl_v9/src/draw/dma2d/lv_draw_dma2d.c apps/common/ui/lvgl_v9/src/draw/dma2d/lv_draw_dma2d_fill.c apps/common/ui/lvgl_v9/src/draw/dma2d/lv_draw_dma2d_img.c apps/common/ui/lvgl_v9/src/draw/espressif/ppa/lv_draw_ppa.c apps/common/ui/lvgl_v9/src/draw/espressif/ppa/lv_draw_ppa_buf.c apps/common/ui/lvgl_v9/src/draw/espressif/ppa/lv_draw_ppa_fill.c apps/common/ui/lvgl_v9/src/draw/espressif/ppa/lv_draw_ppa_img.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_arc.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_fill.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_image.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_letter.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_line.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_ram_g.c apps/common/ui/lvgl_v9/src/draw/eve/lv_draw_eve_triangle.c apps/common/ui/lvgl_v9/src/draw/eve/lv_eve.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_arc.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_ball.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_border.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_box_shadow.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_common.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_fill.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_img.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_letter.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_line.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_mask_rect.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_page_effect_cube.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_page_effect_hexagon.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_page_effect_hexagon_reflection.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_page_effect_lrb_mirror.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_page_effect_manager.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_triangle.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_vec.c apps/common/ui/lvgl_v9/src/draw/jlgpu/lv_draw_jlgpu_vector.c apps/common/ui/lvgl_v9/src/draw/lv_draw.c apps/common/ui/lvgl_v9/src/draw/lv_draw_3d.c apps/common/ui/lvgl_v9/src/draw/lv_draw_arc.c apps/common/ui/lvgl_v9/src/draw/lv_draw_blur.c apps/common/ui/lvgl_v9/src/draw/lv_draw_buf.c apps/common/ui/lvgl_v9/src/draw/lv_draw_image.c apps/common/ui/lvgl_v9/src/draw/lv_draw_label.c apps/common/ui/lvgl_v9/src/draw/lv_draw_line.c apps/common/ui/lvgl_v9/src/draw/lv_draw_mask.c apps/common/ui/lvgl_v9/src/draw/lv_draw_rect.c apps/common/ui/lvgl_v9/src/draw/lv_draw_triangle.c apps/common/ui/lvgl_v9/src/draw/lv_draw_vector.c apps/common/ui/lvgl_v9/src/draw/lv_image_decoder.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_3d.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_arc.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_border.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_box_shadow.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_fill.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_grad.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_image.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_label.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_layer.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_line.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_mask_rect.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_triangle.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_draw_nanovg_vector.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_nanovg_fbo_cache.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_nanovg_image_cache.c apps/common/ui/lvgl_v9/src/draw/nanovg/lv_nanovg_utils.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_arc.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_border.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_fill.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_img.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_label.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_layer.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_line.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_stm32_hal.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_triangle.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_utils.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_draw_nema_gfx_vector.c apps/common/ui/lvgl_v9/src/draw/nema_gfx/lv_nema_gfx_path.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_draw_buf_g2d.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_draw_g2d.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_draw_g2d_fill.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_draw_g2d_img.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_g2d_buf_map.c apps/common/ui/lvgl_v9/src/draw/nxp/g2d/lv_g2d_utils.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_draw_buf_pxp.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_draw_pxp.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_draw_pxp_fill.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_draw_pxp_img.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_draw_pxp_layer.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_pxp_cfg.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_pxp_osa.c apps/common/ui/lvgl_v9/src/draw/nxp/pxp/lv_pxp_utils.c apps/common/ui/lvgl_v9/src/draw/opengles/lv_draw_opengles.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_arc.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_border.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_fill.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_image.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_label.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_line.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_mask_rectangle.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_triangle.c apps/common/ui/lvgl_v9/src/draw/renesas/dave2d/lv_draw_dave2d_utils.c apps/common/ui/lvgl_v9/src/draw/sdl/lv_draw_sdl.c apps/common/ui/lvgl_v9/src/draw/snapshot/lv_snapshot.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_a8.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_al88.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_argb8888.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_i1.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_l8.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.c apps/common/ui/lvgl_v9/src/draw/sw/blend/lv_draw_sw_blend_to_rgb888.c apps/common/ui/lvgl_v9/src/draw/sw/blend/neon/lv_draw_sw_blend_neon_to_rgb565.c apps/common/ui/lvgl_v9/src/draw/sw/blend/neon/lv_draw_sw_blend_neon_to_rgb888.c apps/common/ui/lvgl_v9/src/draw/sw/blend/riscv_v/lv_draw_sw_blend_riscv_v_to_rgb888.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_arc.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_blur.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_border.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_box_shadow.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_fill.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_grad.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_img.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_letter.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_line.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_mask.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_mask_rect.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_transform.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_triangle.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_utils.c apps/common/ui/lvgl_v9/src/draw/sw/lv_draw_sw_vector.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_buf_vg_lite.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_arc.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_border.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_box_shadow.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_fill.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_img.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_label.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_layer.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_line.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_mask_rect.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_triangle.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_draw_vg_lite_vector.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_bitmap_font_cache.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_decoder.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_grad.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_math.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_path.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_pending.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_stroke.c apps/common/ui/lvgl_v9/src/draw/vg_lite/lv_vg_lite_utils.c apps/common/ui/lvgl_v9/src/drivers/display/drm/lv_linux_drm.c apps/common/ui/lvgl_v9/src/drivers/display/drm/lv_linux_drm_common.c apps/common/ui/lvgl_v9/src/drivers/display/drm/lv_linux_drm_egl.c apps/common/ui/lvgl_v9/src/drivers/display/fb/lv_linux_fbdev.c apps/common/ui/lvgl_v9/src/drivers/display/ft81x/lv_ft81x.c apps/common/ui/lvgl_v9/src/drivers/display/ili9341/lv_ili9341.c apps/common/ui/lvgl_v9/src/drivers/display/lcd/lv_lcd_generic_mipi.c apps/common/ui/lvgl_v9/src/drivers/display/nv3007/lv_nv3007.c apps/common/ui/lvgl_v9/src/drivers/display/nxp_elcdif/lv_nxp_elcdif.c apps/common/ui/lvgl_v9/src/drivers/display/renesas_glcdc/lv_renesas_glcdc.c apps/common/ui/lvgl_v9/src/drivers/display/st7735/lv_st7735.c apps/common/ui/lvgl_v9/src/drivers/display/st7789/lv_st7789.c apps/common/ui/lvgl_v9/src/drivers/display/st7796/lv_st7796.c apps/common/ui/lvgl_v9/src/drivers/display/st_ltdc/lv_st_ltdc.c apps/common/ui/lvgl_v9/src/drivers/draw/eve/lv_draw_eve_display.c apps/common/ui/lvgl_v9/src/drivers/evdev/lv_evdev.c apps/common/ui/lvgl_v9/src/drivers/libinput/lv_libinput.c apps/common/ui/lvgl_v9/src/drivers/libinput/lv_xkb.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_cache.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_entry.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_fbdev.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_image_cache.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_lcd.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_libuv.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_mouse.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_profiler.c apps/common/ui/lvgl_v9/src/drivers/nuttx/lv_nuttx_touchscreen.c apps/common/ui/lvgl_v9/src/drivers/opengles/assets/lv_opengles_shader.c apps/common/ui/lvgl_v9/src/drivers/opengles/glad/src/egl.c apps/common/ui/lvgl_v9/src/drivers/opengles/glad/src/gl.c apps/common/ui/lvgl_v9/src/drivers/opengles/glad/src/gles2.c apps/common/ui/lvgl_v9/src/drivers/opengles/lv_opengles_debug.c apps/common/ui/lvgl_v9/src/drivers/opengles/lv_opengles_driver.c apps/common/ui/lvgl_v9/src/drivers/opengles/lv_opengles_egl.c apps/common/ui/lvgl_v9/src/drivers/opengles/lv_opengles_glfw.c apps/common/ui/lvgl_v9/src/drivers/opengles/lv_opengles_texture.c apps/common/ui/lvgl_v9/src/drivers/opengles/opengl_shader/lv_opengl_shader_manager.c apps/common/ui/lvgl_v9/src/drivers/opengles/opengl_shader/lv_opengl_shader_program.c apps/common/ui/lvgl_v9/src/drivers/qnx/lv_qnx.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_egl.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_keyboard.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_mouse.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_mousewheel.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_sw.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_texture.c apps/common/ui/lvgl_v9/src/drivers/sdl/lv_sdl_window.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_context.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_display.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_indev_keyboard.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_indev_pointer.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_indev_touch.c apps/common/ui/lvgl_v9/src/drivers/uefi/lv_uefi_private.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wayland.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_egl_backend.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_g2d_backend.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_keyboard.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_pointer.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_seat.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_shm_backend.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_touch.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_window.c apps/common/ui/lvgl_v9/src/drivers/wayland/lv_wl_xdg_shell.c apps/common/ui/lvgl_v9/src/drivers/windows/lv_windows_context.c apps/common/ui/lvgl_v9/src/drivers/windows/lv_windows_display.c apps/common/ui/lvgl_v9/src/drivers/windows/lv_windows_input.c apps/common/ui/lvgl_v9/src/drivers/x11/lv_x11_display.c apps/common/ui/lvgl_v9/src/drivers/x11/lv_x11_input.c apps/common/ui/lvgl_v9/src/font/binfont_loader/lv_binfont_loader.c apps/common/ui/lvgl_v9/src/font/fmt_txt/lv_font_fmt_txt.c apps/common/ui/lvgl_v9/src/font/font_manager/lv_font_manager.c apps/common/ui/lvgl_v9/src/font/font_manager/lv_font_manager_recycle.c apps/common/ui/lvgl_v9/src/font/imgfont/lv_imgfont.c apps/common/ui/lvgl_v9/src/font/lv_font.c apps/common/ui/lvgl_v9/src/font/lv_font_dejavu_16_persian_hebrew.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_10.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_12.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_14.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_14_aligned.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_16.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_18.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_20.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_22.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_24.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_26.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_28.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_28_compressed.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_30.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_32.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_34.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_36.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_38.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_40.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_42.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_44.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_46.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_48.c apps/common/ui/lvgl_v9/src/font/lv_font_montserrat_8.c apps/common/ui/lvgl_v9/src/font/lv_font_source_han_sans_sc_14_cjk.c apps/common/ui/lvgl_v9/src/font/lv_font_source_han_sans_sc_16_cjk.c apps/common/ui/lvgl_v9/src/font/lv_font_unscii_16.c apps/common/ui/lvgl_v9/src/font/lv_font_unscii_8.c apps/common/ui/lvgl_v9/src/indev/lv_gridnav.c apps/common/ui/lvgl_v9/src/indev/lv_indev.c apps/common/ui/lvgl_v9/src/indev/lv_indev_gesture.c apps/common/ui/lvgl_v9/src/indev/lv_indev_scroll.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/bind.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/control.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/dyn_reader.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/dyn_utils.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/event.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/gui_parse_dyn.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/prop.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/res.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/res_font.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/res_image.c apps/common/ui/lvgl_v9/src/jieli/common/gui/dyn/style.c apps/common/ui/lvgl_v9/src/jieli/common/gui/scr/gui_scr.c apps/common/ui/lvgl_v9/src/jieli/common/gui/scr/gui_scr_manager.c apps/common/ui/lvgl_v9/src/jieli/common/gui/scr/gui_scr_stack.c apps/common/ui/lvgl_v9/src/jieli/common/gui/scr/gui_scr_switch.c apps/common/ui/lvgl_v9/src/jieli/common/jl_common.c apps/common/ui/lvgl_v9/src/jieli/extra/jl_extra.c apps/common/ui/lvgl_v9/src/jieli/extra/libs/jl_bin/lv_font_loader_jl_bin.c apps/common/ui/lvgl_v9/src/jieli/extra/libs/jl_bin/lv_jl_bin.c apps/common/ui/lvgl_v9/src/jieli/extra/libs/jl_bin/lv_jl_image_convert.c apps/common/ui/lvgl_v9/src/jieli/extra/libs/jl_c3/jl_c3.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_attention.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_back.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_bounce.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_fade.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_global.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_internal.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_rotate.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_sliding.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_specials.c apps/common/ui/lvgl_v9/src/jieli/extra/others/anim_effect/lv_anim_effect_zooming.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/album/lv_album.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/arcmenu/lv_arcmenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/curvemenu/lv_curvemenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/gearmenu/lv_geramenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/gridmenu/lv_gridmenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/imagemask/lv_imagemask.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/imglist/lv_imglist.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/jlgif/jlgifdec.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/jlgif/lv_jlgif.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/kaleidoscopemenu/lv_kaleidoscopemenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/listmenu/lv_listmenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/polygonmenu/lv_polygonmenu.c apps/common/ui/lvgl_v9/src/jieli/extra/widgets/textprogress/lv_textprogress.c apps/common/ui/lvgl_v9/src/jieli/lv_jieli.c apps/common/ui/lvgl_v9/src/layouts/flex/lv_flex.c apps/common/ui/lvgl_v9/src/layouts/grid/lv_grid.c apps/common/ui/lvgl_v9/src/layouts/lv_layout.c apps/common/ui/lvgl_v9/src/libs/FT800-FT813/EVE_commands.c apps/common/ui/lvgl_v9/src/libs/FT800-FT813/EVE_supplemental.c apps/common/ui/lvgl_v9/src/libs/barcode/code128.c apps/common/ui/lvgl_v9/src/libs/barcode/lv_barcode.c apps/common/ui/lvgl_v9/src/libs/bin_decoder/lv_bin_decoder.c apps/common/ui/lvgl_v9/src/libs/bmp/lv_bmp.c apps/common/ui/lvgl_v9/src/libs/ffmpeg/lv_ffmpeg.c apps/common/ui/lvgl_v9/src/libs/freetype/lv_freetype.c apps/common/ui/lvgl_v9/src/libs/freetype/lv_freetype_glyph.c apps/common/ui/lvgl_v9/src/libs/freetype/lv_freetype_image.c apps/common/ui/lvgl_v9/src/libs/freetype/lv_freetype_outline.c apps/common/ui/lvgl_v9/src/libs/freetype/lv_ftsystem.c apps/common/ui/lvgl_v9/src/libs/frogfs/src/decomp_raw.c apps/common/ui/lvgl_v9/src/libs/frogfs/src/frogfs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_cbfs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_fatfs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_frogfs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_littlefs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_memfs.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_posix.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_stdio.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_uefi.c apps/common/ui/lvgl_v9/src/libs/fsdrv/lv_fs_win32.c apps/common/ui/lvgl_v9/src/libs/gif/gif.c apps/common/ui/lvgl_v9/src/libs/gltf/gltf_environment/lv_gltf_ibl_sampler.c apps/common/ui/lvgl_v9/src/libs/gltf/gltf_view/assets/chromatic.c apps/common/ui/lvgl_v9/src/libs/gltf/gltf_view/assets/lv_gltf_view_shader.c apps/common/ui/lvgl_v9/src/libs/gltf/math/lv_3dmath.c apps/common/ui/lvgl_v9/src/libs/gstreamer/lv_gstreamer.c apps/common/ui/lvgl_v9/src/libs/libjpeg_turbo/lv_libjpeg_turbo.c apps/common/ui/lvgl_v9/src/libs/libpng/lv_libpng.c apps/common/ui/lvgl_v9/src/libs/libwebp/lv_libwebp.c apps/common/ui/lvgl_v9/src/libs/lodepng/lodepng.c apps/common/ui/lvgl_v9/src/libs/lodepng/lv_lodepng.c apps/common/ui/lvgl_v9/src/libs/lz4/lz4.c apps/common/ui/lvgl_v9/src/libs/nanovg/nanovg.c apps/common/ui/lvgl_v9/src/libs/qrcode/lv_qrcode.c apps/common/ui/lvgl_v9/src/libs/qrcode/qrcodegen.c apps/common/ui/lvgl_v9/src/libs/rle/lv_rle.c apps/common/ui/lvgl_v9/src/libs/rlottie/lv_rlottie.c apps/common/ui/lvgl_v9/src/libs/svg/lv_svg.c apps/common/ui/lvgl_v9/src/libs/svg/lv_svg_decoder.c apps/common/ui/lvgl_v9/src/libs/svg/lv_svg_parser.c apps/common/ui/lvgl_v9/src/libs/svg/lv_svg_render.c apps/common/ui/lvgl_v9/src/libs/svg/lv_svg_token.c apps/common/ui/lvgl_v9/src/libs/tiny_ttf/lv_tiny_ttf.c apps/common/ui/lvgl_v9/src/libs/tjpgd/lv_tjpgd.c apps/common/ui/lvgl_v9/src/libs/tjpgd/tjpgd.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLite/vg_lite.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLite/vg_lite_image.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLite/vg_lite_matrix.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLite/vg_lite_path.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLite/vg_lite_stroke.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/VGLiteKernel/vg_lite_kernel.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/lv_vg_lite_hal/lv_vg_lite_hal.c apps/common/ui/lvgl_v9/src/libs/vg_lite_driver/lv_vg_lite_hal/vg_lite_os.c apps/common/ui/lvgl_v9/src/lv_init.c apps/common/ui/lvgl_v9/src/misc/cache/class/lv_cache_lru_ll.c apps/common/ui/lvgl_v9/src/misc/cache/class/lv_cache_lru_rb.c apps/common/ui/lvgl_v9/src/misc/cache/class/lv_cache_sc_da.c apps/common/ui/lvgl_v9/src/misc/cache/instance/lv_image_cache.c apps/common/ui/lvgl_v9/src/misc/cache/instance/lv_image_header_cache.c apps/common/ui/lvgl_v9/src/misc/cache/lv_cache.c apps/common/ui/lvgl_v9/src/misc/cache/lv_cache_entry.c apps/common/ui/lvgl_v9/src/misc/lv_anim.c apps/common/ui/lvgl_v9/src/misc/lv_anim_timeline.c apps/common/ui/lvgl_v9/src/misc/lv_area.c apps/common/ui/lvgl_v9/src/misc/lv_array.c apps/common/ui/lvgl_v9/src/misc/lv_async.c apps/common/ui/lvgl_v9/src/misc/lv_bidi.c apps/common/ui/lvgl_v9/src/misc/lv_circle_buf.c apps/common/ui/lvgl_v9/src/misc/lv_color.c apps/common/ui/lvgl_v9/src/misc/lv_color_op.c apps/common/ui/lvgl_v9/src/misc/lv_event.c apps/common/ui/lvgl_v9/src/misc/lv_fs.c apps/common/ui/lvgl_v9/src/misc/lv_grad.c apps/common/ui/lvgl_v9/src/misc/lv_iter.c apps/common/ui/lvgl_v9/src/misc/lv_ll.c apps/common/ui/lvgl_v9/src/misc/lv_log.c apps/common/ui/lvgl_v9/src/misc/lv_lru.c apps/common/ui/lvgl_v9/src/misc/lv_math.c apps/common/ui/lvgl_v9/src/misc/lv_matrix.c apps/common/ui/lvgl_v9/src/misc/lv_palette.c apps/common/ui/lvgl_v9/src/misc/lv_pending.c apps/common/ui/lvgl_v9/src/misc/lv_profiler_builtin.c apps/common/ui/lvgl_v9/src/misc/lv_profiler_builtin_posix.c apps/common/ui/lvgl_v9/src/misc/lv_rb.c apps/common/ui/lvgl_v9/src/misc/lv_style.c apps/common/ui/lvgl_v9/src/misc/lv_style_gen.c apps/common/ui/lvgl_v9/src/misc/lv_templ.c apps/common/ui/lvgl_v9/src/misc/lv_text.c apps/common/ui/lvgl_v9/src/misc/lv_text_ap.c apps/common/ui/lvgl_v9/src/misc/lv_timer.c apps/common/ui/lvgl_v9/src/misc/lv_tree.c apps/common/ui/lvgl_v9/src/misc/lv_utils.c apps/common/ui/lvgl_v9/src/osal/lv_cmsis_rtos2.c apps/common/ui/lvgl_v9/src/osal/lv_freertos.c apps/common/ui/lvgl_v9/src/osal/lv_linux.c apps/common/ui/lvgl_v9/src/osal/lv_mqx.c apps/common/ui/lvgl_v9/src/osal/lv_os.c apps/common/ui/lvgl_v9/src/osal/lv_os_jieli.c apps/common/ui/lvgl_v9/src/osal/lv_os_none.c apps/common/ui/lvgl_v9/src/osal/lv_pthread.c apps/common/ui/lvgl_v9/src/osal/lv_rtthread.c apps/common/ui/lvgl_v9/src/osal/lv_sdl2.c apps/common/ui/lvgl_v9/src/osal/lv_windows.c apps/common/ui/lvgl_v9/src/others/file_explorer/lv_file_explorer.c apps/common/ui/lvgl_v9/src/others/fragment/lv_fragment.c apps/common/ui/lvgl_v9/src/others/fragment/lv_fragment_manager.c apps/common/ui/lvgl_v9/src/others/translation/lv_translation.c apps/common/ui/lvgl_v9/src/stdlib/builtin/lv_mem_core_builtin.c apps/common/ui/lvgl_v9/src/stdlib/builtin/lv_sprintf_builtin.c apps/common/ui/lvgl_v9/src/stdlib/builtin/lv_string_builtin.c apps/common/ui/lvgl_v9/src/stdlib/builtin/lv_tlsf.c apps/common/ui/lvgl_v9/src/stdlib/clib/lv_mem_core_clib.c apps/common/ui/lvgl_v9/src/stdlib/clib/lv_sprintf_clib.c apps/common/ui/lvgl_v9/src/stdlib/clib/lv_string_clib.c apps/common/ui/lvgl_v9/src/stdlib/lv_mem.c apps/common/ui/lvgl_v9/src/stdlib/micropython/lv_mem_core_micropython.c apps/common/ui/lvgl_v9/src/stdlib/rtthread/lv_mem_core_rtthread.c apps/common/ui/lvgl_v9/src/stdlib/rtthread/lv_sprintf_rtthread.c apps/common/ui/lvgl_v9/src/stdlib/rtthread/lv_string_rtthread.c apps/common/ui/lvgl_v9/src/stdlib/uefi/lv_mem_core_uefi.c apps/common/ui/lvgl_v9/src/themes/default/lv_theme_default.c apps/common/ui/lvgl_v9/src/themes/lv_theme.c apps/common/ui/lvgl_v9/src/themes/mono/lv_theme_mono.c apps/common/ui/lvgl_v9/src/themes/simple/lv_theme_simple.c apps/common/ui/lvgl_v9/src/tick/lv_tick.c apps/common/ui/lvgl_v9/src/widgets/3dtexture/lv_3dtexture.c apps/common/ui/lvgl_v9/src/widgets/animimage/lv_animimage.c apps/common/ui/lvgl_v9/src/widgets/arc/lv_arc.c apps/common/ui/lvgl_v9/src/widgets/arclabel/lv_arclabel.c apps/common/ui/lvgl_v9/src/widgets/bar/lv_bar.c apps/common/ui/lvgl_v9/src/widgets/button/lv_button.c apps/common/ui/lvgl_v9/src/widgets/buttonmatrix/lv_buttonmatrix.c apps/common/ui/lvgl_v9/src/widgets/calendar/lv_calendar.c apps/common/ui/lvgl_v9/src/widgets/calendar/lv_calendar_chinese.c apps/common/ui/lvgl_v9/src/widgets/calendar/lv_calendar_header_arrow.c apps/common/ui/lvgl_v9/src/widgets/calendar/lv_calendar_header_dropdown.c apps/common/ui/lvgl_v9/src/widgets/canvas/lv_canvas.c apps/common/ui/lvgl_v9/src/widgets/chart/lv_chart.c apps/common/ui/lvgl_v9/src/widgets/checkbox/lv_checkbox.c apps/common/ui/lvgl_v9/src/widgets/dropdown/lv_dropdown.c apps/common/ui/lvgl_v9/src/widgets/gif/lv_gif.c apps/common/ui/lvgl_v9/src/widgets/image/lv_image.c apps/common/ui/lvgl_v9/src/widgets/imagebutton/lv_imagebutton.c apps/common/ui/lvgl_v9/src/widgets/ime/lv_ime_pinyin.c apps/common/ui/lvgl_v9/src/widgets/keyboard/lv_keyboard.c apps/common/ui/lvgl_v9/src/widgets/label/lv_label.c apps/common/ui/lvgl_v9/src/widgets/led/lv_led.c apps/common/ui/lvgl_v9/src/widgets/line/lv_line.c apps/common/ui/lvgl_v9/src/widgets/list/lv_list.c apps/common/ui/lvgl_v9/src/widgets/lottie/lv_lottie.c apps/common/ui/lvgl_v9/src/widgets/menu/lv_menu.c apps/common/ui/lvgl_v9/src/widgets/msgbox/lv_msgbox.c apps/common/ui/lvgl_v9/src/widgets/objx_templ/lv_objx_templ.c apps/common/ui/lvgl_v9/src/widgets/property/lv_animimage_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_arc_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_bar_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_buttonmatrix_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_chart_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_checkbox_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_dropdown_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_image_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_keyboard_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_label_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_led_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_line_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_menu_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_obj_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_roller_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_scale_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_slider_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_span_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_spinbox_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_spinner_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_style_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_switch_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_table_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_tabview_properties.c apps/common/ui/lvgl_v9/src/widgets/property/lv_textarea_properties.c apps/common/ui/lvgl_v9/src/widgets/roller/lv_roller.c apps/common/ui/lvgl_v9/src/widgets/scale/lv_scale.c apps/common/ui/lvgl_v9/src/widgets/slider/lv_slider.c apps/common/ui/lvgl_v9/src/widgets/span/lv_span.c apps/common/ui/lvgl_v9/src/widgets/spinbox/lv_spinbox.c apps/common/ui/lvgl_v9/src/widgets/spinner/lv_spinner.c apps/common/ui/lvgl_v9/src/widgets/switch/lv_switch.c apps/common/ui/lvgl_v9/src/widgets/table/lv_table.c apps/common/ui/lvgl_v9/src/widgets/tabview/lv_tabview.c apps/common/ui/lvgl_v9/src/widgets/textarea/lv_textarea.c apps/common/ui/lvgl_v9/src/widgets/tileview/lv_tileview.c apps/common/ui/lvgl_v9/src/widgets/win/lv_win.c
c_SRC_FILES += 	apps/common/ui/lvgl_v9/lvgl_v9_main.c 
#endif


#endif



// *INDENT-OFF*

#if CONFIG_LV9_ENABLE


#if CONFIG_LVGL_UI_ENABLE

c_SRC_FILES += 	apps/common/ui/lvgl_v9/freetype/src/autofit/autofit.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftbase.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftsystem.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftbitmap.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftdebug.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftglyph.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftinit.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftmm.c 	apps/common/ui/lvgl_v9/freetype/src/base/ftstroke.c 	apps/common/ui/lvgl_v9/freetype/src/bdf/bdf.c 	apps/common/ui/lvgl_v9/freetype/src/cache/ftcache.c 	apps/common/ui/lvgl_v9/freetype/src/cff/cff.c 	apps/common/ui/lvgl_v9/freetype/src/cid/type1cid.c 	apps/common/ui/lvgl_v9/freetype/src/gzip/ftgzip.c 	apps/common/ui/lvgl_v9/freetype/src/lzw/ftlzw.c 	apps/common/ui/lvgl_v9/freetype/src/pcf/pcf.c 	apps/common/ui/lvgl_v9/freetype/src/pfr/pfr.c 	apps/common/ui/lvgl_v9/freetype/src/psaux/psaux.c 	apps/common/ui/lvgl_v9/freetype/src/pshinter/pshinter.c 	apps/common/ui/lvgl_v9/freetype/src/psnames/psnames.c 	apps/common/ui/lvgl_v9/freetype/src/raster/raster.c 	apps/common/ui/lvgl_v9/freetype/src/sdf/sdf.c 	apps/common/ui/lvgl_v9/freetype/src/sfnt/sfnt.c 	apps/common/ui/lvgl_v9/freetype/src/smooth/smooth.c 	apps/common/ui/lvgl_v9/freetype/src/svg/svg.c 	apps/common/ui/lvgl_v9/freetype/src/truetype/truetype.c 	apps/common/ui/lvgl_v9/freetype/src/type1/type1.c 	apps/common/ui/lvgl_v9/freetype/src/type42/type42.c 	apps/common/ui/lvgl_v9/freetype/src/winfonts/winfnt.c 	apps/common/ui/lvgl_v9/freetype/src/bzip2/ftbzip2.c 	apps/common/ui/lvgl_v9/freetype/src/dlg/dlgwrap.c 	apps/common/ui/lvgl_v9/freetype/src/gxvalid/gxvalid.c 	apps/common/ui/lvgl_v9/freetype/src/otvalid/otvalid.c 

#endif


#endif



// *INDENT-OFF*



c_SRC_FILES += 	apps/common/third_party_profile/jieli/online_db/spp_online_db.c 	apps/common/third_party_profile/jieli/online_db/online_db_deal.c 	apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.c 	apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.c 	apps/common/third_party_profile/wechat_sport/wechat_sport_demo.c    apps/common/third_party_profile/multi_protocol_common.c     apps/common/third_party_profile/multi_protocol_event.c     apps/common/third_party_profile/multi_protocol_main.c 

c_SRC_FILES += 	apps/common/third_party_profile/media_spp/media_spp.c 

c_SRC_FILES += 	apps/common/third_party_profile/common/3th_profile_api.c 	apps/common/third_party_profile/common/custom_cfg.c 	apps/common/third_party_profile/common/mic_rec.c 

c_SRC_FILES += 	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_multi_common.c 	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_server.c 	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_multi_client.c 	apps/common/third_party_profile/jieli/rcsp/gatt_over_edr_app.c 	apps/common/third_party_profile/jieli/rcsp/adv/ble_rcsp_adv.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_config.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_task.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_event.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_manage.c 	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_bt_manage.c 


c_SRC_FILES += 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve_no_respone.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_respone.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve_no_respone.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_respone.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_user.c 	apps/common/third_party_profile/jieli/rcsp/server/rcsp_command.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_switch_device.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_opt.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_sync.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_adv_bluetooth.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_screen_box_earphone_info.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_eq_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_high_low_vol_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_music_info_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_vol_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_color_led_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_eq_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice_key.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_hearing_aid_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_bt_name_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_key_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_led_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_mic_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_time_stamp_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_work_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_adaptive_noise_reduction.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_ai_no_pick.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_scene_noise_reduction.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_wind_noise_detection.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_voice_enhancement_mode.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_1t2_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_reverbration_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_drc_setting.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_feature.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_status.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_fm_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_rtc_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_music_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_linein_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_bt_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_pc_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update_tws.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_ch_loader_download.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_cmd.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_opt.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_transfer.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_delete.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/dev_format.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_trans_back.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_bluk_trans_prepare.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_simple_transfer.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensors_data_opt.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_func.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensor_log_notify.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/nfc_data_opt.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_opt.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_sync.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/browser/rcsp_browser.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_watch_info.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_watch_device_config.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_common_info_res_file_handler.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/stream_media.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/audio_video_control.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/ai_dialogue_control.c

#if RCSP_MODE && RCSP_TRANSLATOR_ENABLE
c_SRC_FILES += 	apps/common/third_party_profile/jieli/rcsp/server/functions/translator/rcsp_translator.c 	apps/common/third_party_profile/jieli/rcsp/server/functions/translator/rcsp_translator_audio.c
#endif


c_SRC_FILES += 	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve.c 	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve_no_response.c 	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_response.c 	apps/common/third_party_profile/jieli/rcsp/client/rcsp_m_update/rcsp_update_master.c 
c_SRC_FILES += 	apps/common/third_party_profile/jieli/screen_trans/screen_ear_protocol_received.c 	apps/common/third_party_profile/jieli/screen_trans/smartbox_user_app.c 	apps/common/third_party_profile/jieli/stream_media/stream_media_core.c 	apps/common/third_party_profile/jieli/stream_media/stream_media_device_config.c 	apps/common/third_party_profile/jieli/stream_media/stream_media_protocol.c 


c_SRC_FILES +=     apps/common/third_party_profile/swift_pair/swift_pair_protocol.c 

c_SRC_FILES +=     apps/common/third_party_profile/custom_protocol_demo/custom_protocol.c 

#if EXPORT_FMNA_ENABLE
c_SRC_FILES +=     apps/common/third_party_profile/bt_fmy/ble_fmy.c     apps/common/third_party_profile/bt_fmy/ble_fmy_fmna.c     apps/common/third_party_profile/bt_fmy/ble_fmy_ota.c     apps/common/third_party_profile/bt_fmy/ble_fmy_modet.c 
#endif

c_SRC_FILES +=     apps/common/third_party_profile/interface/app_protocol_api.c     apps/common/third_party_profile/interface/app_protocol_ota.c     apps/common/third_party_profile/interface/app_protocol_common.c 	apps/common/third_party_profile/interface/app_protocol_gma.c 	apps/common/third_party_profile/interface/app_protocol_dma.c 	apps/common/third_party_profile/interface/app_protocol_mma.c 	apps/common/third_party_profile/interface/app_protocol_tme.c 
#ifdef CONFIG_NET_ENABLE
c_SRC_FILES += 	apps/common/third_party_profile/alipay/le_alipay.c 
#endif




// *INDENT-OFF*

#if EXPORT_CONFIG_USB_ENABLE
//usb slave
#if TCFG_USB_SLAVE_ENABLE
c_SRC_FILES += 	apps/common/device/usb/usb_config.c 	apps/common/device/usb/device/descriptor.c 	apps/common/device/usb/device/usb_device.c 	apps/common/device/usb/device/user_setup.c 	apps/common/device/usb/device/task_pc.c 	apps/common/device/usb/device/usb_suspend_resume.c 	apps/common/device/usb/device/detect_host_type.c 
#endif


// mass storage
#if TCFG_USB_SLAVE_MSD_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/msd.c 
#endif

c_SRC_FILES += 	apps/common/device/usb/device/msd_upgrade.c 
// iap
#if TCFG_USB_APPLE_DOCK_EN
c_SRC_FILES += 	apps/common/device/usb/device/iap.c 
#endif

// mtp
#if TCFG_USB_SLAVE_MTP_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/mtp.c 
#endif

// hid
#if TCFG_USB_SLAVE_HID_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/hid.c 
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/custom_hid.c 	apps/common/device/usb/device/rcsp_hid_inter.c 
#endif


// audio
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/uac_stream.c

#if (USB_AUDIO_VERSION == USB_AUDIO_VERSION_1_0)
c_SRC_FILES += 	apps/common/device/usb/device/uac1.c
#endif

#if (USB_AUDIO_VERSION == USB_AUDIO_VERSION_2_0)
c_SRC_FILES += 	apps/common/device/usb/device/uac2.c
#endif

#endif



#if TCFG_USB_SLAVE_CDC_ENABLE
c_SRC_FILES +=     apps/common/device/usb/device/cdc.c 
#endif

// midi
#if TCFG_USB_SLAVE_MIDI_ENABLE
#if (USB_MIDI_VERSION == USB_MIDI_VERSION_2_0)
c_SRC_FILES +=     apps/common/device/usb/device/midi2.c
#else
c_SRC_FILES +=     apps/common/device/usb/device/midi.c
#endif

#endif


// printer
#if TCFG_USB_SLAVE_PRINTER_ENABLE
c_SRC_FILES +=     apps/common/device/usb/device/printer.c 
#endif

// usb host
#if TCFG_USB_HOST_ENABLE
c_SRC_FILES += 	apps/common/device/usb/usb_host_config.c 	apps/common/device/usb/host/usb_bulk_transfer.c 	apps/common/device/usb/host/usb_ctrl_transfer.c 	apps/common/device/usb/host/usb_host.c 

#endif


// udisk
#if TCFG_UDISK_ENABLE
c_SRC_FILES += 	apps/common/device/usb/host/usb_storage.c

#endif

// gamepad
#if TCFG_ADB_ENABLE
c_SRC_FILES += 	apps/common/device/usb/host/adb.c 
#endif

#if TCFG_HOST_UVC_ENABLE || CONFIG_WATCH_CASE
 c_SRC_FILES += 	 apps/common/device/usb/host/usb_video.c 	 apps/common/device/usb/host/uvc_host.c 	 apps/common/device/usb/host/uvc_jpeg_demo.c 	 apps/common/device/usb/host/videobuf_stub.c 
#endif

#if TCFG_HOST_WIRELESS_ENABLE || TCFG_HOST_RNDIS_ENABLE
c_SRC_FILES += 	apps/common/device/usb/host/usbnet.c

#endif

#if TCFG_AOA_ENABLE
c_SRC_FILES +=     apps/common/device/usb/host/aoa.c 
#endif


#if TCFG_HID_HOST_ENABLE
c_SRC_FILES +=     apps/common/device/usb/host/hid.c 
#endif

// usb host audio
#if TCFG_HOST_AUDIO_ENABLE
c_SRC_FILES +=     apps/common/device/usb/host/uac_host.c 
#endif



#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
c_SRC_FILES +=     apps/common/device/usb/usb_epbuf_manager.c     apps/common/device/usb/usb_task.c     apps/common/device/usb/usb_multiplex.c 
#endif


// usb host hub
#if TCFG_HUB_HOST_ENABLE
c_SRC_FILES +=     apps/common/device/usb/host/hub.c 
#endif

// webusb
#if TCFG_USB_WEBUSB_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/webusb.c 
#endif

// hci
#if TCFG_USB_SLAVE_HCI_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/hci.c 
#endif

// xbox360 gamepad (XInput)
#if TCFG_USB_SLAVE_XBOX360_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/xbox360.c 
#endif

// standard keyboard/mouse HID
#if TCFG_USB_SLAVE_HID_KBMOUSE_ENABLE
c_SRC_FILES += 	apps/common/device/usb/device/hid_kbmouse.c 
#endif

// usb test
#if TCFG_USB_TEST_ENABLE
c_SRC_FILES += 		apps/common/device/usb/device/usb_test_device.c 		apps/common/device/usb/host/usb_test_host.c 
#endif


// demo
#if 0
c_SRC_FILES +=     apps/common/device/usb/demo/host/udisk_block_test.c
#endif


#endif




// *INDENT-OFF*

c_SRC_FILES += 	  audio/cpu/br33/audio_setup.c 	  audio/cpu/br33/audio_config.c 	  audio/cpu/br33/audio_dai/audio_pdm.c 	  audio/cpu/br33/audio_dai/audio_spdif_slave.c 	  audio/cpu/br33/audio_dai/audio_spdif_master.c 
/* #if TCFG_AUDIO_ANC_ENABLE */
/* objs += \ */
	  /* audio/cpu/br33/audio_anc.c \ */
	  /* audio/cpu/br33/icsd_anc_user.c */
/* #endif */


//Audio Accelerator
c_SRC_FILES +=       audio/cpu/br33/audio_accelerator/hw_fft.c 
c_SRC_FILES +=     audio/cpu/br33/audio_vad/vad_mic.c 
//Audio Demo Files
#if EXPORT_PLATFORM_AUDIO_DEMO
c_SRC_FILES += 	  audio/cpu/br33/audio_demo/audio_dac_demo.c 	  audio/cpu/br33/anpu_demo/anpu_example.c 	  audio/cpu/br33/anpu_demo/anpu_performance_test.c 
#if TCFG_AUDIO_ADC_ENABLE
c_SRC_FILES += 	  audio/cpu/br33/audio_demo/audio_adc_demo.c 	  audio/cpu/br33/audio_demo/audio_adc_pt.c 
#endif

#endif



// *INDENT-OFF*

c_SRC_FILES +=     apps/watch/ble/bt_ble.c     apps/watch/ble/ble_adv.c


c_SRC_FILES +=     apps/watch/battery/charge.c     apps/watch/battery/charge_device_handle.c

#if 1//TCFG_SYS_LVD_EN
c_SRC_FILES +=     apps/watch/battery/battery_level.c     apps/watch/battery/battery_offset.c
#endif


#if (TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE)
c_SRC_FILES +=     apps/watch/battery/charge_store.c
#endif



#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
c_SRC_FILES += 	apps/watch/mode/bt/bt_call_kws_handler.c     apps/watch/mode/bt/kws_voice_event_deal.c
#endif


#if TCFG_MIC_EFFECT_ENABLE
c_SRC_FILES +=     apps/watch/mode/mic_effect/mic_effect.c 
#endif

#if TCFG_APP_PC_EN
c_SRC_FILES +=     apps/watch/mode/pc/pc.c     apps/watch/mode/pc/pc_app_msg_handler.c     apps/watch/mode/pc/pc_key_msg_table.c 
#endif


#if TCFG_APP_VIDEO_REC_EN
c_SRC_FILES +=     apps/watch/mode/video_rec/video_rec.c     apps/watch/mode/video_rec/video_rec_worker.c     apps/watch/mode/video_rec/video_rec_ui.c     apps/watch/mode/video_rec/video_rec_key_msg_table.c     apps/watch/mode/video_rec/video_rec_app_msg_handler.c

#endif

#if TCFG_APP_VIDEO_DEC_EN
c_SRC_FILES +=     apps/watch/mode/video_dec/video_dec.c     apps/watch/mode/video_dec/video_dec_ui.c     apps/watch/mode/video_dec/video_dec_key_msg_table.c     apps/watch/mode/video_dec/video_dec_app_msg_handler.c

#endif

#if TCFG_VSTREAM_TRANSFER
c_SRC_FILES +=     apps/watch/video/transfer/kid_wt_app.c     apps/watch/video/transfer/spp_transfer.c     apps/watch/video/transfer/transfer_stubs.c     apps/watch/video/transfer/vstream_chat.c     apps/watch/video/transfer/vstream_cmd.c     apps/watch/video/transfer/vstream_receiver.c     apps/watch/video/transfer/vstream_sender.c     apps/watch/video/transfer/fps_counts.c     apps/watch/video/transfer/audio_app.c     apps/watch/video/transfer/kid_page_uart.c

c_SRC_FILES +=     apps/watch/video/transfer/kid_wt_video.c

#endif

#if TCFG_APP_PHOTO_EN
c_SRC_FILES +=     apps/watch/mode/video_photo/video_photo.c     apps/watch/mode/video_photo/video_photo_ui.c     apps/watch/mode/video_photo/video_photo_key_msg_table.c     apps/watch/mode/video_photo/video_photo_app_msg_handler.c

#endif

#if TCFG_APP_VIDEO_EN
c_SRC_FILES +=     apps/watch/mode/video/video_player.c     apps/watch/mode/video/video_key_msg_table.c

#endif

#if TCFG_APP_RECORD_EN
c_SRC_FILES +=     apps/watch/mode/record/record.c     apps/watch/mode/record/record_app_msg_handler.c     apps/watch/mode/record/record_key_msg_table.c 
#endif

c_SRC_FILES +=     apps/watch/mode/rtc/rtc.c     apps/watch/mode/rtc/rtc_app_msg_handler.c     apps/watch/mode/rtc/rtc_key_msg_table.c     apps/watch/mode/rtc/alarm_api.c     apps/watch/mode/rtc/alarm_user.c 

#if (TCFG_BT_AI_SEL_PROTOCOL&LE_AUDIO_CIS_RX_EN) // LE AUDIO协议选择
c_SRC_FILES += 	apps/common/le_audio/cig.c 	apps/common/le_audio/wireless_trans_manager.c 	apps/watch/mode/bt/le_audio/cig/app_le_connected.c 	apps/watch/mode/bt/le_audio/cig/le_connected.c 	apps/watch/mode/bt/le_audio/cig/le_connected_config.c 	apps/watch/mode/bt/le_audio/cig/le_connected_data_interchange.c 	apps/watch/mode/bt/le_audio/le_audio_common.c 
#endif

#ifdef CONFIG_NET_ENABLE
c_SRC_FILES += 	apps/watch/net/net_event.c 
#endif


// *INDENT-OFF*

#if CONFIG_LV9_ENABLE

#if EXPORT_KID_CAMERA_LVGL_APP_CODE_ENABLE

#if CONFIG_LVGL_UI_ENABLE

c_SRC_FILES +=     $(patsubst %.c, %.o, $(shell find  apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/custom -name '*.c' -print))     $(patsubst %.c, %.c, $(shell find  apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/generated -name '*.c' -print))     $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/ui_action.c 

#endif


#endif

#endif



// *INDENT-OFF*

#if CONFIG_LV9_ENABLE

#if EXPORT_WATCH_LVGL_APP_CODE_ENABLE

#if CONFIG_LVGL_UI_ENABLE

c_SRC_FILES +=     $(patsubst %.c, %.o, $(shell find  apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/custom -name '*.c' -print))     $(patsubst %.c, %.c, $(shell find  apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/generated -name '*.c' -print))     $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/ui_action.c 

#endif


#endif

#endif

// *INDENT-OFF*

#if CONFIG_LV9_ENABLE

#if EXPORT_DASHBOARD_LVGL_APP_CODE_ENABLE

#if CONFIG_LVGL_UI_ENABLE

c_SRC_FILES += apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/camera_display_ui.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/custom.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/hardware_test.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_action_music_bg.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_action_page_map.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_music_page_music.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_music_page_video.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_update_page.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_events/callback_handler.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_events/events_init.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_events/i18n_handler.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_events/screen_handler.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_fonts/gui_fonts.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_fonts/lv_font_simsun_32.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_group/gui_group.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_guider.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_msg/gui_model_msg.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_msg/gui_msg.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_msg/gui_music_msg.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_msg/gui_sys_model_msg.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_res/res_common.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_hardware_test.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_page_app_select.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_page_map.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_page_meter.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_page_music.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_page_video.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/setup_scr_upgrade_page.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/ui_scr_manager.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_scr/ui_style.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/gui_timelines/gui_timelines.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated/i18n/lv_i18n.c apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/ui_action.c

#endif


#endif

#endif


// *INDENT-OFF*

#if CONFIG_JLUI_ENABLE

#if EXPORT_CSC_JLUI_APP_CODE_ENABLE

#if CONFIG_JL_UI_ENABLE

c_SRC_FILES +=         apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/language_text.c         apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_bg_manage.c         apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_charging.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_eq.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_find_earphone.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_language_select.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_lock_select.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_music.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_denoise.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_phone_call_status.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_setting.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_take_picture.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_topbar.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_volue_controls.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_weather.c         apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_cube.c         apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_cube_reflection.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_football_menu.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_hexagon.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_hexagon_reflection.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_dial.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_menu_list.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_set.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_menu_manage.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_alarm_clock.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_time_set.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/smartbox_info_manager.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_upgrade.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_dial_sel.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_sd_music.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_bt_emitter.c 		apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_notice.c 

#endif


#endif

#endif


// *INDENT-OFF*

#if CONFIG_JLUI_ENABLE

#if EXPORT_WATCH_JLUI_APP_CODE_ENABLE

#if CONFIG_JL_UI_ENABLE

c_SRC_FILES +=         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_draw_demo.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_animation_demo.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_alipay.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_compass.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_password.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_set.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_shortcut_menu.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_stopwatch.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_timer.c         apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_voice_assistant.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_menu_manage.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial_sel.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial_mode.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_menu_list.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_menu_waterfalls.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_batchtcharge.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_indoor_sports.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_momentum.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_outdoor_sports.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_result.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_target.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sporting.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_intensity_action.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_target_set.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_upgrate.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_music.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_weather.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_photogragh.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_heart.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_oxygen.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_runlight.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_breathtrain.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sleep.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bloodpress.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_calendar.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_heat.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone_keypad.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone_call_status.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sleepaid.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_alarm_clock.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_calculator.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_mense_manage.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_notice.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_pullup_menu.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sidebar.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_football.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_cube.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_cube_reflection.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_hexagon.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_hexagon_reflection.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bedside_watch.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_flashlight.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_engineering_mode.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_rubiks_cube.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_findmy.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bt_emitter.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sd_music.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_smartwin.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_netapp_iflytek.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_iflytek.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_aidial.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_uvc_show.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_camera.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_music_bg.c 		apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_product_test.c 
#endif

#endif

#endif


