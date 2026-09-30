#include "app_config.h"




objs += \
      $(ROOT)/audio/framework/plugs/source/adc_file.o \
      $(ROOT)/audio/framework/nodes/volume_node.o \

objs += \
      $(ROOT)/audio/framework/nodes/plc_node.o

objs += \
	  $(ROOT)/audio/framework/nodes/audio_debug_node.o



objs += \
      $(ROOT)/audio/framework/plugs/source/esco_file.o \
      $(ROOT)/audio/framework/nodes/esco_tx_node.o

objs += \
      $(ROOT)/audio/framework/plugs/source/a2dp_file.o \
      $(ROOT)/audio/framework/plugs/source/a2dp_streamctrl.o

objs += \
      $(ROOT)/audio/framework/nodes/virtual_udisk_node.o

objs += \
      $(ROOT)/audio/framework/nodes/ns_node.o

objs += \
      $(ROOT)/audio/framework/nodes/channle_swap_node.o

objs += \
      $(ROOT)/audio/framework/nodes/effect_dev_node_common.o

objs += \
      $(ROOT)/audio/framework/nodes/effect_dev0_node.o
objs += \
      $(ROOT)/audio/framework/nodes/effect_dev1_node.o
objs += \
      $(ROOT)/audio/framework/nodes/effect_dev2_node.o
objs += \
      $(ROOT)/audio/framework/nodes/effect_dev3_node.o
objs += \
      $(ROOT)/audio/framework/nodes/effect_dev4_node.o
objs += \
      $(ROOT)/audio/framework/nodes/sink_dev0_node.o
objs += \
      $(ROOT)/audio/framework/nodes/sink_dev1_node.o
objs += \
      $(ROOT)/audio/framework/nodes/sink_dev2_node.o
objs += \
      $(ROOT)/audio/framework/nodes/sink_dev3_node.o
objs += \
      $(ROOT)/audio/framework/nodes/sink_dev4_node.o

objs += \
      $(ROOT)/audio/framework/plugs/source/source_dev0_file.o
objs += \
      $(ROOT)/audio/framework/plugs/source/source_dev1_file.o
objs += \
      $(ROOT)/audio/framework/plugs/source/source_dev2_file.o
objs += \
      $(ROOT)/audio/framework/plugs/source/source_dev3_file.o
objs += \
      $(ROOT)/audio/framework/plugs/source/source_dev4_file.o

objs += \
      $(ROOT)/audio/framework/nodes/agc_node.o

objs += \
      $(ROOT)/audio/framework/nodes/surround_demo_node.o

objs += \
      $(ROOT)/audio/framework/nodes/lhdc_x_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_sms_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_dms_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_3mic_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_develop_node.o

objs += \
      $(ROOT)/audio/framework/nodes/uart_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_v3_node.o

objs += \
      $(ROOT)/audio/framework/nodes/cvp_sms_vf_node.o

objs += \
      $(ROOT)/audio/framework/nodes/dns_node.o

objs += \
      $(ROOT)/audio/framework/nodes/ai_tx_node.o \
      $(ROOT)/audio/framework/nodes/a2dp_tx_node.o

objs += \
      $(ROOT)/audio/framework/nodes/intercom_tx_node.o

objs += \
      $(ROOT)/audio/framework/nodes/video_tx_node.o \
      $(ROOT)/audio/interface/recoder/video_voice_recoder.o

objs += \
      $(ROOT)/audio/framework/nodes/video_rx_file.o \
      $(ROOT)/audio/interface/player/video_rx_player.o

objs += \
      $(ROOT)/audio/framework/nodes/ai_rx_file.o \
	  $(ROOT)/audio/interface/player/ai_rx_player.o

objs += \
      $(ROOT)/audio/framework/nodes/intercom_rx_file.o \
	  $(ROOT)/audio/interface/player/intercom_rx_player.o

objs += \
      $(ROOT)/audio/framework/nodes/data_export_node.o

objs += \
      $(ROOT)/audio/framework/nodes/local_tws_source_node.o \
      $(ROOT)/audio/framework/plugs/source/local_tws_file.o \
	  $(ROOT)/audio/interface/player/local_tws_player.o

#if EXPORT_PLATFORM_AUDIO_PDM_ENABLE
objs += \
    $(ROOT)/audio/framework/plugs/source/pdm_mic_file.o
#endif

#if EXPORT_PLATFORM_AUDIO_SPDIF_ENABLE
objs += \
	  $(ROOT)/audio/framework/plugs/source/spdif_file.o
#endif


objs += \
	  $(ROOT)/audio/test_tools/mic_dut_process.o

objs += \
	  $(ROOT)/audio/test_tools/audio_enc_mpt_self.o \
	  $(ROOT)/audio/test_tools/audio_enc_mpt_cvp_ctr.o

objs += \
	  $(ROOT)/audio/test_tools/cvp_tool.o

objs += \
	  $(ROOT)/audio/test_tools/audio_dut_control.o \
	  $(ROOT)/audio/test_tools/audio_dut_control_old.o

objs += \
	  $(ROOT)/audio/common/audio_node_config.o \
	  $(ROOT)/audio/common/audio_dvol.o \
	  $(ROOT)/audio/common/audio_general.o \
	  $(ROOT)/audio/common/audio_build_needed.o \
	  $(ROOT)/audio/common/online_debug/aud_data_export.o \
	  $(ROOT)/audio/common/online_debug/audio_online_debug.o \
	  $(ROOT)/audio/common/online_debug/audio_capture.o \
	  $(ROOT)/audio/common/audio_plc.o \
	  $(ROOT)/audio/common/audio_noise_gate.o \
	  $(ROOT)/audio/common/audio_ns.o \
	  $(ROOT)/audio/common/audio_utils.o \
	  $(ROOT)/audio/common/audio_export_demo.o \
	  $(ROOT)/audio/common/amplitude_statistic.o \
	  $(ROOT)/audio/common/frame_length_adaptive.o \
	  $(ROOT)/audio/common/bt_audio_energy_detection.o \
	  $(ROOT)/audio/common/audio_event_handler.o \
	  $(ROOT)/audio/common/debug/audio_debug.o \
	  $(ROOT)/audio/common/power/mic_power_manager.o \
	  $(ROOT)/audio/common/audio_volume_mixer.o \
	  $(ROOT)/audio/common/pcm_data/sine_pcm.o \

objs += \
	  $(ROOT)/audio/common/audio_iis_lrclk_capture.o

objs += \
	  $(ROOT)/audio/common/online_debug/aud_mic_dut.o

objs += \
	  $(ROOT)/audio/common/uartPcmSender.o

objs += \
	  $(ROOT)/audio/common/demo/codec_demo/audio_enc_file_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_file_dec_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_frame_codec_demo.o \
	  $(ROOT)/audio/common/demo/audio_eq_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_msbc_hw_codec_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_msbc_sw_codec_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_sbc_codec_demo.o \
	  $(ROOT)/audio/common/demo/audio_alink_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_speex_codec_demo.o \
	  $(ROOT)/audio/common/demo/codec_demo/audio_opus_codec_demo.o \


#if EXPORT_PLATFORM_HW_MATH_VERSION == 2
objs += \
	  $(ROOT)/audio/common/demo/hw_math_v2_demo.o
#endif


#if EXPORT_PLATFORM_AUDIO_WM8978_ENABLE
objs += \
	  $(ROOT)/audio/common/wm8978/wm8978.o \
	  $(ROOT)/audio/common/wm8978/iic.o
#endif

objs += \
	  $(ROOT)/audio/interface/player/iis_player.o

objs += \
	  $(ROOT)/audio/interface/player/tone_player.o \
	  $(ROOT)/audio/interface/player/ring_player.o \
	  $(ROOT)/audio/interface/player/a2dp_player.o \
	  $(ROOT)/audio/interface/player/esco_player.o \
	  $(ROOT)/audio/interface/player/key_tone_player.o \
	  $(ROOT)/audio/interface/player/dev_flow_player.o \
	  $(ROOT)/audio/interface/player/adda_loop_player.o \
      $(ROOT)/audio/interface/player/reference_time.o \

objs += \
	  $(ROOT)/audio/interface/player/linein_player.o

objs += \
	  $(ROOT)/audio/interface/player/loudspeaker_iis_player.o \
	  $(ROOT)/audio/interface/player/loudspeaker_mic_player.o

objs += \
	  $(ROOT)/audio/interface/player/mic_player.o

objs += \
	  $(ROOT)/audio/interface/recoder/esco_recoder.o \
	  $(ROOT)/audio/interface/recoder/dev_flow_recoder.o \

objs += \
	  $(ROOT)/audio/interface/recoder/ai_voice_recoder.o

objs += \
	  $(ROOT)/audio/interface/recoder/intercom_voice_recoder.o

objs += \
	  $(ROOT)/audio/interface/user_defined/audio_dsp_low_latency_player.o

#if EXPORT_PLATFORM_LE_AUDIO_ENABLE
objs += \
	$(ROOT)/audio/interface/player/le_audio_player.o \
	$(ROOT)/audio/framework/nodes/le_audio_source.o \
	$(ROOT)/audio/framework/plugs/source/le_audio_file.o \
	$(ROOT)/audio/le_audio/le_audio_stream.o \
	$(ROOT)/audio/interface/recoder/le_audio_recorder.o \
	$(ROOT)/audio/interface/recoder/le_audio_mix_mic_recorder.o \
	$(ROOT)/audio/le_audio/le_audio_plc_test.o
#endif

objs += \
	$(ROOT)/audio/midi/midi_event.o \
	$(ROOT)/audio/midi/midi_lib_api.o \
	$(ROOT)/audio/midi/midi_play.o \
	$(ROOT)/audio/midi/audio_midi.o \
	$(ROOT)/audio/midi/audio_dec_midi_file.o \
	$(ROOT)/audio/midi/audio_dec_midi_ctrl.o

objs += \
	$(ROOT)/audio/framework/plugs/source/midi_controller_file.o


objs += \
      $(ROOT)/audio/effect/eq_config.o \
	  $(ROOT)/audio/effect/spk_eq.o \
	  $(ROOT)/audio/effect/audio_voice_changer_api.o \
	  $(ROOT)/audio/effect/esco_ul_voice_changer.o \
	  $(ROOT)/audio/effect/bass_treble.o \
	  $(ROOT)/audio/effect/audio_dc_offset_remove.o \
	  $(ROOT)/audio/effect/effects_adj.o \
	  $(ROOT)/audio/effect/user_effects_adj.o \
	  $(ROOT)/audio/effect/effects_dev.o \
	  $(ROOT)/audio/effect/effects_default_param.o \
	  $(ROOT)/audio/effect/node_param_update.o \
	  $(ROOT)/audio/effect/scene_update.o \

#if EXPORT_PLATFORM_ICSD_ENABLE
objs += \
	$(ROOT)/audio/common/icsd/adt/icsd_adt.o \
	$(ROOT)/audio/common/icsd/adt/icsd_adt_app.o \
	$(ROOT)/audio/common/icsd/adt/icsd_adt_config.o \
	$(ROOT)/audio/common/icsd/adt/icsd_adt_alg.o \
	$(ROOT)/audio/common/icsd/adt/icsd_adt_demo.o \
	$(ROOT)/audio/common/icsd/anc/icsd_anc_app.o \
	$(ROOT)/audio/common/icsd/anc/icsd_anc_board.o \
	$(ROOT)/audio/common/icsd/anc/icsd_anc_data.o \
	$(ROOT)/audio/common/icsd/anc/icsd_anc_interactive.o \
	$(ROOT)/audio/common/icsd/common/icsd_common.o \
	$(ROOT)/audio/common/icsd/dot/icsd_dot_app.o \
	$(ROOT)/audio/common/icsd/dot/icsd_dot.o \
	$(ROOT)/audio/common/icsd/common_v2/icsd_common_v2.o \
	$(ROOT)/audio/common/icsd/common_v2/icsd_common_v2_app.o \
	$(ROOT)/audio/common/icsd/anc_v2/icsd_anc_v2.o \
	$(ROOT)/audio/common/icsd/anc_v2/icsd_anc_v2_app.o \
	$(ROOT)/audio/common/icsd/anc_v2/icsd_anc_v2_interactive.o \
	$(ROOT)/audio/common/icsd/config/icsd_anc_v2_config.o \
	$(ROOT)/audio/common/icsd/rt_anc/rt_anc.o \
	$(ROOT)/audio/common/icsd/rt_anc/rt_anc_app.o \
	$(ROOT)/audio/common/icsd/rt_anc/rt_anc_config.o \
	$(ROOT)/audio/common/icsd/tool/anc_ext_tool.o \
	$(ROOT)/audio/common/icsd/tool/anc_ext_tool_file.o \
	$(ROOT)/audio/common/icsd/aeq/icsd_aeq_app.o \
	$(ROOT)/audio/common/icsd/aeq/icsd_aeq.o \
	$(ROOT)/audio/common/icsd/aeq/icsd_aeq_config.o \
	$(ROOT)/audio/common/icsd/afq/icsd_afq_app.o \
	$(ROOT)/audio/common/icsd/afq/icsd_afq.o \
	$(ROOT)/audio/common/icsd/afq/icsd_afq_config.o \
	$(ROOT)/audio/common/icsd/cmp/icsd_cmp_app.o \
	$(ROOT)/audio/common/icsd/cmp/icsd_cmp_config.o \
	$(ROOT)/audio/common/icsd/demo/icsd_demo.o \
	$(ROOT)/audio/common/icsd/ein/icsd_ein_config.o \
	$(ROOT)/audio/common/icsd/vdt/icsd_vdt_config.o \
	$(ROOT)/audio/common/icsd/wat/icsd_wat_config.o \
	$(ROOT)/audio/common/icsd/wind/icsd_wind_config.o \
	$(ROOT)/audio/common/icsd/avc/icsd_avc_config.o \

#endif

#if EXPORT_PLATFORM_ANC_ENABLE
objs += \
	  $(ROOT)/audio/anc/audio_anc_fade_ctr.o \
	  $(ROOT)/audio/anc/audio_anc_common_plug.o \
	  $(ROOT)/audio/anc/audio_anc_debug_tool.o \
	  $(ROOT)/audio/anc/audio_anc_mult_scene.o \
	  $(ROOT)/audio/anc/audio_anc_common.o \

#endif

#if EXPORT_PLATFORM_AUDIO_SMART_VOICE_ENABLE
objs += \
	$(ROOT)/audio/smart_voice/smart_voice_core.o \
	$(ROOT)/audio/smart_voice/smart_voice_config.o \
	$(ROOT)/audio/smart_voice/jl_kws_platform.o \
	$(ROOT)/audio/smart_voice/user_asr.o \
	$(ROOT)/audio/smart_voice/voice_mic_data.o \
	$(ROOT)/audio/smart_voice/voice_mic_aec.o \
	$(ROOT)/audio/smart_voice/kws_event.o \
	$(ROOT)/audio/smart_voice/nn_vad.o
#endif

objs += \
	$(ROOT)/audio/jl_kws/jl_kws_main.o \
	$(ROOT)/audio/jl_kws/jl_kws_audio.o \
	$(ROOT)/audio/jl_kws/jl_kws_algo.o \
	$(ROOT)/audio/jl_kws/jl_kws_event.o \



#if EXPORT_PLATFORM_AUDIO_SPATIAL_EFFECT_ENABLE
objs += \
	$(ROOT)/audio/framework/nodes/spatial_effects_node.o \
	$(ROOT)/audio/common/online_debug/aud_spatial_effect_dut.o \

objs += \
	$(ROOT)/audio/effect/spatial_effect/spatial_effect.o \
	$(ROOT)/audio/effect/spatial_effect/spatial_effect_imu.o \
	$(ROOT)/audio/effect/spatial_effect/spatial_effect_tws.o \
	$(ROOT)/audio/effect/spatial_effect/spatial_imu_trim.o \
	$(ROOT)/audio/effect/spatial_effect/spatial_effects_process.o \

#endif

#if EXPORT_SOMATOSENSORY_ENABLE
objs += \
	$(ROOT)/audio/effect/somatosensory/audio_somatosensory.o \

#endif

objs += \
	  $(ROOT)/audio/CVP/audio_aec.o \
	  $(ROOT)/audio/CVP/audio_cvp.o \
	  $(ROOT)/audio/CVP/audio_cvp_dms.o \
	  $(ROOT)/audio/CVP/audio_cvp_3mic.o \
	  $(ROOT)/audio/CVP/audio_cvp_online.o \
	  $(ROOT)/audio/CVP/audio_cvp_demo.o \
	  $(ROOT)/audio/CVP/audio_cvp_develop.o \
	  $(ROOT)/audio/CVP/audio_cvp_sync.o \
	  $(ROOT)/audio/CVP/audio_cvp_ais_3mic.o \
	  $(ROOT)/audio/CVP/audio_cvp_ref_task.o \
	  $(ROOT)/audio/CVP/audio_cvp_config.o \

objs += \
	  $(ROOT)/audio/CVP/audio_cvp_v3.o

objs += \
	  $(ROOT)/audio/CVP/audio_cvp_sms_vf.o

objs += \
	  $(ROOT)/audio/interface/player/tws_tone_player.o

#if EXPORT_PLATFORM_AUDIO_ALINK_ENABLE
objs += \
	  $(ROOT)/audio/framework/nodes/iis_node.o

objs += \
	  $(ROOT)/audio/framework/plugs/source/iis_file.o

objs += \
	  $(ROOT)/audio/framework/nodes/multi_ch_iis_node.o
objs += \
	  $(ROOT)/audio/framework/plugs/source/multi_ch_iis_file.o

#endif

#if EXPORT_PLATFORM_AUDIO_SPDIF_ENABLE
objs += \
	  $(ROOT)/audio/framework/nodes/spdif_node.o
#endif

objs += \
	  $(ROOT)/audio/interface/player/fm_player.o \
	  $(ROOT)/audio/framework/plugs/source/fm_file.o \


#if EXPORT_PLATFORM_AUDIO_TDM_ENABLE
objs += \
	  $(ROOT)/audio/interface/player/tdm_player.o \
	  $(ROOT)/audio/framework/plugs/source/tdm_file.o \
	  $(ROOT)/audio/framework/nodes/tdm_node.o \

#endif

objs += \
      $(ROOT)/audio/interface/player/file_player.o \


objs += \
      $(ROOT)/audio/interface/recoder/file_recorder.o \


objs += \
	  $(ROOT)/audio/framework/plugs/source/linein_file.o \

objs += \
	  $(ROOT)/audio/framework/plugs/source/pc_spk_file.o \
	  $(ROOT)/audio/interface/player/pc_spk_player.o \


objs += \
	  $(ROOT)/audio/framework/nodes/pc_mic_node.o \
	  $(ROOT)/audio/interface/recoder/pc_mic_recoder.o \


objs += \
	  $(ROOT)/audio/effect/audio_pitch_speed_api.o \




objs += \
        $(ROOT)/audio/framework/nodes/vad_node.o \


#if EXPORT_PLATFORM_AUDIO_EFFECT_DEMO_ENABLE
objs += \
      $(ROOT)/audio/effect/demo/autotune_demo.o \
      $(ROOT)/audio/effect/demo/bass_treble_demo.o \
      $(ROOT)/audio/effect/demo/chorus_demo.o \
      $(ROOT)/audio/effect/demo/crossover_demo.o \
      $(ROOT)/audio/effect/demo/drc_advance_demo.o \
      $(ROOT)/audio/effect/demo/drc_demo.o \
      $(ROOT)/audio/effect/demo/dynamic_eq_demo.o \
      $(ROOT)/audio/effect/demo/echo_demo.o \
      $(ROOT)/audio/effect/demo/energy_detect_demo.o \
      $(ROOT)/audio/effect/demo/eq_demo.o \
      $(ROOT)/audio/effect/demo/frequency_shift_howling_demo.o \
      $(ROOT)/audio/effect/demo/gain_demo.o \
      $(ROOT)/audio/effect/demo/harmonic_exciter_demo.o \
      $(ROOT)/audio/effect/demo/multiband_drc_demo.o \
      $(ROOT)/audio/effect/demo/noisegate_demo.o \
      $(ROOT)/audio/effect/demo/notch_howling_demo.o \
      $(ROOT)/audio/effect/demo/pitch_speed_demo.o \
      $(ROOT)/audio/effect/demo/reverb_advance_demo.o \
      $(ROOT)/audio/effect/demo/reverb_demo.o \
      $(ROOT)/audio/effect/demo/spectrum_demo.o \
      $(ROOT)/audio/effect/demo/stereo_widener_demo.o \
      $(ROOT)/audio/effect/demo/surround_demo.o \
      $(ROOT)/audio/effect/demo/virtual_bass_demo.o \
      $(ROOT)/audio/effect/demo/voice_changer_demo.o \
      $(ROOT)/audio/effect/demo/lhdc_x_demo.o \

#endif

objs += \
	$(ROOT)/audio/cpu/common.o \





#if EXPORT_LE_AUDIO_SUPPORT

objs += \
	$(ROOT)/apps/common/le_audio/big.o

objs += \
	$(ROOT)/apps/common/le_audio/cig.o

objs += \
	$(ROOT)/apps/common/le_audio/wireless_trans_manager.o

#endif


#if CONFIG_VIDEO_ENABLE



objs += \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/avilib/ -name '*.c' -print)) \

objs += \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/libmov/src/libmov/ -name '*.c' -print)) \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/libmov/src/codec/ -name '*.c' -print)) \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/libmov/api/ -name '*.c' -print)) \

objs += \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/mpi_app/ -name '*.c' -print)) \

objs += \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/video/mpi_demo/ -name '*.c' -print)) \



#endif




#if CONFIG_LV9_ENABLE



objs += \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/ui/lvgl_v9/src -name '*.c' -print)) \
	$(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/ui/lvgl_v9/examples -name '*.c' -print)) \
    $(patsubst %.c, %.o, $(shell find $(ROOT)/apps/common/ui/lvgl_v9/demos -name '*.c' -print)) \

objs += \
	$(ROOT)/apps/common/ui/lvgl_v9/lvgl_v9_main.o \



#endif




#if CONFIG_LV9_ENABLE



objs += \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/autofit/autofit.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftbase.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftsystem.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftbitmap.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftdebug.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftglyph.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftinit.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftmm.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/base/ftstroke.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/bdf/bdf.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/cache/ftcache.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/cff/cff.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/cid/type1cid.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/gzip/ftgzip.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/lzw/ftlzw.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/pcf/pcf.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/pfr/pfr.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/psaux/psaux.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/pshinter/pshinter.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/psnames/psnames.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/raster/raster.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/sdf/sdf.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/sfnt/sfnt.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/smooth/smooth.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/svg/svg.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/truetype/truetype.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/type1/type1.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/type42/type42.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/winfonts/winfnt.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/bzip2/ftbzip2.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/dlg/dlgwrap.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/gxvalid/gxvalid.o \
	$(ROOT)/apps/common/ui/lvgl_v9/freetype/src/otvalid/otvalid.o \




#endif






objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/online_db/spp_online_db.o \
	$(ROOT)/apps/common/third_party_profile/jieli/online_db/online_db_deal.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/wechat_sport/wechat_sport_demo.o\
    $(ROOT)/apps/common/third_party_profile/multi_protocol_common.o \
    $(ROOT)/apps/common/third_party_profile/multi_protocol_event.o \
    $(ROOT)/apps/common/third_party_profile/multi_protocol_main.o \


objs += \
	$(ROOT)/apps/common/third_party_profile/media_spp/media_spp.o \


objs += \
	$(ROOT)/apps/common/third_party_profile/common/3th_profile_api.o \
	$(ROOT)/apps/common/third_party_profile/common/custom_cfg.o \
	$(ROOT)/apps/common/third_party_profile/common/mic_rec.o \


objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/ble_rcsp_multi_common.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/ble_rcsp_server.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/ble_rcsp_multi_client.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/gatt_over_edr_app.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/adv/ble_rcsp_adv.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_config.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_task.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_event.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_manage.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_bt_manage.o \



objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve_no_respone.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_respone.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve_no_respone.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_respone.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_user.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/rcsp_command.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_switch_device.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_opt.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_sync.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_adv_bluetooth.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_screen_box_earphone_info.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_eq_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_high_low_vol_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_music_info_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_vol_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_color_led_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_eq_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice_key.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_hearing_aid_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_bt_name_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_key_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_led_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_mic_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_time_stamp_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_work_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_adaptive_noise_reduction.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_ai_no_pick.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_scene_noise_reduction.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_wind_noise_detection.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_voice_enhancement_mode.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_1t2_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_reverbration_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_drc_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_feature.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_status.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_fm_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_rtc_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_music_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_linein_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_bt_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_pc_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update_tws.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_ch_loader_download.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_cmd.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_opt.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_transfer.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_delete.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/dev_format.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_trans_back.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_bluk_trans_prepare.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_simple_transfer.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensors_data_opt.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_func.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensor_log_notify.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/nfc_data_opt.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_opt.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_sync.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/browser/rcsp_browser.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_watch_info.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_watch_device_config.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/watch_expand/rcsp_common_info_res_file_handler.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/stream_media.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/audio_video_control.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/stream_media/ai_dialogue_control.o

objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/translator/rcsp_translator.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/server/functions/translator/rcsp_translator_audio.o


objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve_no_response.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_response.o \
	$(ROOT)/apps/common/third_party_profile/jieli/rcsp/client/rcsp_m_update/rcsp_update_master.o \

objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/screen_trans/screen_ear_protocol_received.o \
	$(ROOT)/apps/common/third_party_profile/jieli/screen_trans/smartbox_user_app.o \
	$(ROOT)/apps/common/third_party_profile/jieli/stream_media/stream_media_core.o \
	$(ROOT)/apps/common/third_party_profile/jieli/stream_media/stream_media_device_config.o \
	$(ROOT)/apps/common/third_party_profile/jieli/stream_media/stream_media_protocol.o \



objs += \
    $(ROOT)/apps/common/third_party_profile/swift_pair/swift_pair_protocol.o \


objs += \
    $(ROOT)/apps/common/third_party_profile/custom_protocol_demo/custom_protocol.o \


#if EXPORT_FMNA_ENABLE
objs += \
    $(ROOT)/apps/common/third_party_profile/bt_fmy/ble_fmy.o \
    $(ROOT)/apps/common/third_party_profile/bt_fmy/ble_fmy_fmna.o \
    $(ROOT)/apps/common/third_party_profile/bt_fmy/ble_fmy_ota.o \
    $(ROOT)/apps/common/third_party_profile/bt_fmy/ble_fmy_modet.o \

#endif

objs += \
    $(ROOT)/apps/common/third_party_profile/interface/app_protocol_api.o \
    $(ROOT)/apps/common/third_party_profile/interface/app_protocol_ota.o \
    $(ROOT)/apps/common/third_party_profile/interface/app_protocol_common.o \
	$(ROOT)/apps/common/third_party_profile/interface/app_protocol_gma.o \
	$(ROOT)/apps/common/third_party_profile/interface/app_protocol_dma.o \
	$(ROOT)/apps/common/third_party_profile/interface/app_protocol_mma.o \
	$(ROOT)/apps/common/third_party_profile/interface/app_protocol_tme.o \

objs += \
	$(ROOT)/apps/common/third_party_profile/alipay/le_alipay.o \






#if EXPORT_CONFIG_USB_ENABLE
objs += \
	$(ROOT)/apps/common/device/usb/usb_config.o \
	$(ROOT)/apps/common/device/usb/device/descriptor.o \
	$(ROOT)/apps/common/device/usb/device/usb_device.o \
	$(ROOT)/apps/common/device/usb/device/user_setup.o \
	$(ROOT)/apps/common/device/usb/device/task_pc.o \
	$(ROOT)/apps/common/device/usb/device/usb_suspend_resume.o \
	$(ROOT)/apps/common/device/usb/device/detect_host_type.o \



objs += \
	$(ROOT)/apps/common/device/usb/device/msd.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/msd_upgrade.o \

objs += \
	$(ROOT)/apps/common/device/usb/device/iap.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/mtp.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/hid.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/custom_hid.o \
	$(ROOT)/apps/common/device/usb/device/rcsp_hid_inter.o \



objs += \
	$(ROOT)/apps/common/device/usb/device/uac_stream.o

objs += \
	$(ROOT)/apps/common/device/usb/device/uac1.o

objs += \
	$(ROOT)/apps/common/device/usb/device/uac2.o




objs += \
    $(ROOT)/apps/common/device/usb/device/cdc.o \


objs += \
    $(ROOT)/apps/common/device/usb/device/midi2.o
objs += \
    $(ROOT)/apps/common/device/usb/device/midi.o



objs += \
    $(ROOT)/apps/common/device/usb/device/printer.o \


objs += \
	$(ROOT)/apps/common/device/usb/usb_host_config.o \
	$(ROOT)/apps/common/device/usb/host/usb_bulk_transfer.o \
	$(ROOT)/apps/common/device/usb/host/usb_ctrl_transfer.o \
	$(ROOT)/apps/common/device/usb/host/usb_host.o \




objs += \
	$(ROOT)/apps/common/device/usb/host/usb_storage.o


objs += \
	$(ROOT)/apps/common/device/usb/host/adb.o \


#if TCFG_HOST_UVC_ENABLE || CONFIG_WATCH_CASE
 objs += \
	 $(ROOT)/apps/common/device/usb/host/usb_video.o \
	 $(ROOT)/apps/common/device/usb/host/uvc_host.o \
	 $(ROOT)/apps/common/device/usb/host/uvc_jpeg_demo.o \
	 $(ROOT)/apps/common/device/usb/host/videobuf_stub.o \

#endif

objs += \
	$(ROOT)/apps/common/device/usb/host/usbnet.o


objs += \
    $(ROOT)/apps/common/device/usb/host/aoa.o \



objs += \
    $(ROOT)/apps/common/device/usb/host/hid.o \


objs += \
    $(ROOT)/apps/common/device/usb/host/uac_host.o \




objs += \
    $(ROOT)/apps/common/device/usb/usb_epbuf_manager.o \
    $(ROOT)/apps/common/device/usb/usb_task.o \
    $(ROOT)/apps/common/device/usb/usb_multiplex.o \



objs += \
    $(ROOT)/apps/common/device/usb/host/hub.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/webusb.o \


#if TCFG_USB_SLAVE_HCI_ENABLE
objs += \
	$(ROOT)/apps/common/device/usb/device/hci.o \

#endif

objs += \
	$(ROOT)/apps/common/device/usb/device/xbox360.o \


objs += \
	$(ROOT)/apps/common/device/usb/device/hid_kbmouse.o \


objs += \
		$(ROOT)/apps/common/device/usb/device/usb_test_device.o \
		$(ROOT)/apps/common/device/usb/host/usb_test_host.o \



objs += \
    $(ROOT)/apps/common/device/usb/demo/host/udisk_block_test.o


#endif





objs += \
	  $(ROOT)/audio/cpu/br33/audio_setup.o \
	  $(ROOT)/audio/cpu/br33/audio_config.o \
	  $(ROOT)/audio/cpu/br33/audio_dai/audio_pdm.o \
	  $(ROOT)/audio/cpu/br33/audio_dai/audio_spdif_slave.o \
	  $(ROOT)/audio/cpu/br33/audio_dai/audio_spdif_master.o \

/* #if TCFG_AUDIO_ANC_ENABLE */
/* objs += \ */
	  /* $(ROOT)/audio/cpu/br33/audio_anc.o \ */
	  /* $(ROOT)/audio/cpu/br33/icsd_anc_user.o */
/* #endif */


objs += \
      $(ROOT)/audio/cpu/br33/audio_accelerator/hw_fft.o \

objs += \
    $(ROOT)/audio/cpu/br33/audio_vad/vad_mic.o \

#if EXPORT_PLATFORM_AUDIO_DEMO
objs += \
	  $(ROOT)/audio/cpu/br33/audio_demo/audio_dac_demo.o \
	  $(ROOT)/audio/cpu/br33/anpu_demo/anpu_example.o \
	  $(ROOT)/audio/cpu/br33/anpu_demo/anpu_performance_test.o \

objs += \
	  $(ROOT)/audio/cpu/br33/audio_demo/audio_adc_demo.o \
	  $(ROOT)/audio/cpu/br33/audio_demo/audio_adc_pt.o \


#endif




objs += \
    $(ROOT)/apps/watch/ble/bt_ble.o \
    $(ROOT)/apps/watch/ble/ble_adv.o


objs += \
    $(ROOT)/apps/watch/battery/charge.o \
    $(ROOT)/apps/watch/battery/charge_device_handle.o

objs += \
    $(ROOT)/apps/watch/battery/battery_level.o \
    $(ROOT)/apps/watch/battery/battery_offset.o


objs += \
    $(ROOT)/apps/watch/battery/charge_store.o



objs += \
	$(ROOT)/apps/watch/mode/bt/bt_call_kws_handler.o \
    $(ROOT)/apps/watch/mode/bt/kws_voice_event_deal.o


objs += \
    $(ROOT)/apps/watch/mode/mic_effect/mic_effect.o \


objs += \
    $(ROOT)/apps/watch/mode/pc/pc.o \
    $(ROOT)/apps/watch/mode/pc/pc_app_msg_handler.o \
    $(ROOT)/apps/watch/mode/pc/pc_key_msg_table.o \



objs += \
    $(ROOT)/apps/watch/mode/video_rec/video_rec.o \
    $(ROOT)/apps/watch/mode/video_rec/video_rec_worker.o \
    $(ROOT)/apps/watch/mode/video_rec/video_rec_ui.o \
    $(ROOT)/apps/watch/mode/video_rec/video_rec_key_msg_table.o \
    $(ROOT)/apps/watch/mode/video_rec/video_rec_app_msg_handler.o


objs += \
    $(ROOT)/apps/watch/mode/video_dec/video_dec.o \
    $(ROOT)/apps/watch/mode/video_dec/video_dec_ui.o \
    $(ROOT)/apps/watch/mode/video_dec/video_dec_key_msg_table.o \
    $(ROOT)/apps/watch/mode/video_dec/video_dec_app_msg_handler.o


objs += \
    $(ROOT)/apps/watch/video/transfer/kid_wt_app.o \
    $(ROOT)/apps/watch/video/transfer/spp_transfer.o \
    $(ROOT)/apps/watch/video/transfer/transfer_stubs.o \
    $(ROOT)/apps/watch/video/transfer/vstream_chat.o \
    $(ROOT)/apps/watch/video/transfer/vstream_cmd.o \
    $(ROOT)/apps/watch/video/transfer/vstream_receiver.o \
    $(ROOT)/apps/watch/video/transfer/vstream_sender.o \
    $(ROOT)/apps/watch/video/transfer/fps_counts.o \
    $(ROOT)/apps/watch/video/transfer/audio_app.o \
    $(ROOT)/apps/watch/video/transfer/kid_page_uart.o

objs += \
    $(ROOT)/apps/watch/video/transfer/kid_wt_video.o


objs += \
    $(ROOT)/apps/watch/mode/video_photo/video_photo.o \
    $(ROOT)/apps/watch/mode/video_photo/video_photo_ui.o \
    $(ROOT)/apps/watch/mode/video_photo/video_photo_key_msg_table.o \
    $(ROOT)/apps/watch/mode/video_photo/video_photo_app_msg_handler.o


objs += \
    $(ROOT)/apps/watch/mode/video/video_player.o \
    $(ROOT)/apps/watch/mode/video/video_key_msg_table.o


objs += \
    $(ROOT)/apps/watch/mode/record/record.o \
    $(ROOT)/apps/watch/mode/record/record_app_msg_handler.o \
    $(ROOT)/apps/watch/mode/record/record_key_msg_table.o \


objs += \
    $(ROOT)/apps/watch/mode/rtc/rtc.o \
    $(ROOT)/apps/watch/mode/rtc/rtc_app_msg_handler.o \
    $(ROOT)/apps/watch/mode/rtc/rtc_key_msg_table.o \
    $(ROOT)/apps/watch/mode/rtc/alarm_api.o \
    $(ROOT)/apps/watch/mode/rtc/alarm_user.o \


objs += \
	$(ROOT)/apps/common/le_audio/cig.o \
	$(ROOT)/apps/common/le_audio/wireless_trans_manager.o \
	$(ROOT)/apps/watch/mode/bt/le_audio/cig/app_le_connected.o \
	$(ROOT)/apps/watch/mode/bt/le_audio/cig/le_connected.o \
	$(ROOT)/apps/watch/mode/bt/le_audio/cig/le_connected_config.o \
	$(ROOT)/apps/watch/mode/bt/le_audio/cig/le_connected_data_interchange.o \
	$(ROOT)/apps/watch/mode/bt/le_audio/le_audio_common.o \


objs += \
	$(ROOT)/apps/watch/net/net_event.o \




#if CONFIG_LV9_ENABLE

#if EXPORT_KID_CAMERA_LVGL_APP_CODE_ENABLE

#if !TCFG_KID_CAMERA_UI_INTERCOM


objs += \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_320x240_v1/custom -name '*.c' -print)) \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_320x240_v1/generated -name '*.c' -print)) \
    $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_320x240_v1/ui_action.o \



#endif

#endif

#endif




#if CONFIG_LV9_ENABLE

#if EXPORT_KID_CAMERA_LVGL_APP_CODE_ENABLE

#if TCFG_KID_CAMERA_UI_INTERCOM


objs += \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/custom -name '*.c' -print)) \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/generated -name '*.c' -print)) \
    $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/kid_camera_240x320_v1/ui_action.o \



#endif

#endif

#endif




#if CONFIG_LV9_ENABLE

#if EXPORT_WATCH_LVGL_APP_CODE_ENABLE


objs += \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/custom -name '*.c' -print)) \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/generated -name '*.c' -print)) \
    $(ROOT)/apps/$(APP_CASE)/ui/lvgl_ui_app/lvgl_v9/watch_320x385_v1/ui_action.o \




#endif

#endif


#if CONFIG_LV9_ENABLE

#if EXPORT_DASHBOARD_LVGL_APP_CODE_ENABLE


objs += \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom -name '*.c' -print)) \
    $(patsubst %.c, %.o, $(shell find  $(ROOT)/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated -name '*.c' -print)) \
    $(ROOT)/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/ui_action.o \




#endif

#endif



#if CONFIG_JLUI_ENABLE

#if EXPORT_CSC_JLUI_APP_CODE_ENABLE


objs += \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/language_text.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_bg_manage.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_charging.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_eq.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_find_earphone.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_language_select.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_lock_select.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_music.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_denoise.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_phone_call_status.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_setting.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_take_picture.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_topbar.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_volue_controls.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_weather.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_cube.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_cube_reflection.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_football_menu.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_hexagon.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_hexagon_reflection.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_dial.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_menu_list.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_set.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_menu_manage.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_alarm_clock.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_time_set.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/smartbox_info_manager.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_upgrade.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_dial_sel.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_sd_music.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_bt_emitter.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_CSC_PUBLIC_MOLDS/ui_notice.o \




#endif

#endif



#if CONFIG_JLUI_ENABLE

#if EXPORT_WATCH_JLUI_APP_CODE_ENABLE


objs += \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_draw_demo.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_animation_demo.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_alipay.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_compass.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_password.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_set.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_shortcut_menu.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_stopwatch.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_timer.o \
        $(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_voice_assistant.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_menu_manage.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial_sel.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_dial_mode.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_menu_list.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_menu_waterfalls.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_batchtcharge.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_indoor_sports.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_momentum.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_outdoor_sports.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_result.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_target.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sporting.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_intensity_action.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sport_target_set.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_upgrate.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_music.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_weather.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_photogragh.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_heart.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_oxygen.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_runlight.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_breathtrain.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sleep.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bloodpress.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_calendar.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_heat.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone_keypad.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_phone_call_status.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sleepaid.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_alarm_clock.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_calculator.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_mense_manage.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_notice.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_pullup_menu.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sidebar.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_football.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_cube.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_cube_reflection.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_hexagon.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_hexagon_reflection.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bedside_watch.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_flashlight.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_engineering_mode.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_rubiks_cube.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_findmy.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_bt_emitter.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_sd_music.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_smartwin.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_netapp_iflytek.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_iflytek.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_aidial.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_uvc_show.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_camera.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_music_bg.o \
		$(ROOT)/apps/watch/ui/jlui_app/STYLE_WATCH_PUBLIC_MOLDS/ui_action_product_test.o \


#endif

#endif


