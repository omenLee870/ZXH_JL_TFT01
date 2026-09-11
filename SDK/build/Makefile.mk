# 需要编译的 .c 文件
c_SRC_FILES := \
	apps/common/ai_framework/audio/ai_audio_player.c \
	apps/common/ai_framework/audio/ai_audio_sink.c \
	apps/common/ai_framework/audio/ai_audio_source.c \
	apps/common/ai_framework/audio/ai_audio_url_player.c \
	apps/common/ai_framework/core/ai_dialogue.c \
	apps/common/ai_framework/core/ai_provider_registry.c \
	apps/common/ai_framework/core/ai_realtime.c \
	apps/common/ai_framework/demo/ai_provider_selftest.c \
	apps/common/ai_framework/demo/ali_realtime_demo.c \
	apps/common/ai_framework/demo/duer_realtime_demo.c \
	apps/common/ai_framework/demo/glm_vision_demo.c \
	apps/common/ai_framework/demo/volc_ark_vision_demo.c \
	apps/common/ai_framework/providers/ai_provider_ali_realtime.c \
	apps/common/ai_framework/providers/ai_provider_duer.c \
	apps/common/ai_framework/providers/ai_provider_glm.c \
	apps/common/ai_framework/providers/ai_provider_volc_ark.c \
	apps/common/ai_framework/transport/ai_http.c \
	apps/common/ai_framework/transport/ai_image.c \
	apps/common/ai_framework/transport/ai_ws.c \
	apps/common/app_mode_manager/app_mode_manager.c \
	apps/common/bplus_tree/src/bplus.c \
	apps/common/bplus_tree/src/pages.c \
	apps/common/bplus_tree/src/utils.c \
	apps/common/bplus_tree/src/values.c \
	apps/common/bplus_tree/src/writer.c \
	apps/common/bt_common/bt_test_api.c \
	apps/common/c++/cpp_run_init.c \
	apps/common/charge_box/chargeIc_manage.c \
	apps/common/charge_box/charge_ic/charge_ic_ac982.c \
	apps/common/charge_box/charge_ic/charge_ic_sy7609.c \
	apps/common/charge_box/chgbox_box.c \
	apps/common/charge_box/chgbox_ctrl.c \
	apps/common/charge_box/chgbox_det.c \
	apps/common/charge_box/chgbox_full_timer.c \
	apps/common/charge_box/chgbox_handshake.c \
	apps/common/charge_box/chgbox_ui.c \
	apps/common/charge_box/chgbox_wireless.c \
	apps/common/charge_box/soc.c \
	apps/common/clock_manager/clock_manager.c \
	apps/common/config/app_config.c \
	apps/common/config/bt_name_parse.c \
	apps/common/config/bt_profile_config.c \
	apps/common/config/cfg_tool_cdc.c \
	apps/common/config/cfg_tool_statistics_functions/cfg_tool_statistics.c \
	apps/common/config/ci_transport_uart.c \
	apps/common/config/new_cfg_tool.c \
	apps/common/debug/assert.c \
	apps/common/debug/debug.c \
	apps/common/debug/debug_lite.c \
	apps/common/debug/debug_uart_config.c \
	apps/common/dev_manager/dev_manager.c \
	apps/common/dev_manager/dev_reg.c \
	apps/common/dev_manager/dev_update.c \
	apps/common/device/in_ear_detect/in_ear_detect.c \
	apps/common/device/in_ear_detect/in_ear_manage.c \
	apps/common/device/key/adkey.c \
	apps/common/device/key/adkey_rtcvdd.c \
	apps/common/device/key/ctmu_touch_key.c \
	apps/common/device/key/iokey.c \
	apps/common/device/key/irkey.c \
	apps/common/device/key/key_driver.c \
	apps/common/device/key/lptouch_key.c \
	apps/common/device/key/rdec_key.c \
	apps/common/device/key/touch_key.c \
	apps/common/device/key/uart_key.c \
	apps/common/device/sensor/XS9950A/xs9950a_dvp.c \
	apps/common/device/sensor/gc0308/gc0308.c \
	apps/common/device/sensor/gc0310_spi/gc0310.c \
	apps/common/device/sensor/ir_sensor/ir_manage.c \
	apps/common/device/sensor/ir_sensor/jsa1221.c \
	apps/common/device/sensor/nt99141/nt99141.c \
	apps/common/device/sensor/ntc/ntc_det.c \
	apps/common/device/storage_device/nandflash/ftl_device.c \
	apps/common/device/storage_device/nandflash/nandflash.c \
	apps/common/device/storage_device/nandflash/nandflash_part.c \
	apps/common/device/storage_device/norflash/inside_norflash.c \
	apps/common/device/storage_device/norflash/norflash_espi.c \
	apps/common/device/storage_device/norflash/norflash_sfc.c \
	apps/common/device/touch_panel/cst816d/cst816d.c \
	apps/common/device/touch_panel/ft3x68/ft3x68.c \
	apps/common/device/touch_panel/jd9165t/jd9165t.c \
	apps/common/example/peripheral/can/can_test.c \
	apps/common/fat_nor/cfg_private.c \
	apps/common/fat_nor/virfat_flash.c \
	apps/common/file_operate/file_bs_deal.c \
	apps/common/file_operate/file_manager.c \
	apps/common/flashdb/fal.c \
	apps/common/flashdb/src/fdb.c \
	apps/common/flashdb/src/fdb_kvdb.c \
	apps/common/flashdb/src/fdb_tsdb.c \
	apps/common/flashdb/src/fdb_util.c \
	apps/common/jldtp/jldtp_manager.c \
	apps/common/jldtp/jldtp_test.c \
	apps/common/music/breakpoint.c \
	apps/common/music/music_decrypt.c \
	apps/common/music/music_id3.c \
	apps/common/music/music_player.c \
	apps/common/net/iperf_test.c \
	apps/common/net/lwip_2_2_0/port/LwIP.c \
	apps/common/net/lwip_2_2_0/port/bt_ethernetif.c \
	apps/common/net/lwip_2_2_0/port/ext_wireless_ethernetif.c \
	apps/common/net/lwip_2_2_0/port/fixme.c \
	apps/common/net/lwip_2_2_0/port/lte_ethernetif.c \
	apps/common/net/lwip_2_2_0/port/spi_ethernetif.c \
	apps/common/net/lwip_2_2_0/port/sys_arch.c \
	apps/common/net/lwip_2_2_0/port/uart_ethernetif.c \
	apps/common/netapps/iflytek/SparkDesk/sparkdesk_main.c \
	apps/common/netapps/iflytek/authentication.c \
	apps/common/netapps/iflytek/demo.c \
	apps/common/netapps/iflytek/ifly_common.c \
	apps/common/netapps/iflytek/ifly_socket.c \
	apps/common/netapps/iflytek/tts/ifly_dec_file.c \
	apps/common/netapps/iflytek/tts/tts_main.c \
	apps/common/netapps/iflytek/vad/vad_main.c \
	apps/common/pay/upay_t_head_v2/app/antchain/src/bot_adapter.c \
	apps/common/pay/upay_t_head_v2/hal/csi/hsc32i1/hs_private.c \
	apps/common/pay/upay_t_head_v2/hal/csi/softse/aes.c \
	apps/common/pay/upay_t_head_v2/hal/csi/softse/kdf.c \
	apps/common/pay/upay_t_head_v2/hal/csi/softse/sha256.c \
	apps/common/pay/upay_t_head_v2/hal/csi/softse/utils.c \
	apps/common/pay/upay_t_head_v2/hal/csi/vendor_cache.c \
	apps/common/pay/upay_t_head_v2/hal/csi/vendor_crypto.c \
	apps/common/pay/upay_t_head_v2/hal/csi/vendor_product.c \
	apps/common/pay/upay_t_head_v2/hal/csi/vendor_rot.c \
	apps/common/pay/upay_t_head_v2/hal/csi/vendor_ssp.c \
	apps/common/pay/upay_t_head_v2/hal/network/alipay_net_kal.c \
	apps/common/pay/upay_t_head_v2/jl_transitcode/transit_other_adapation.c \
	apps/common/pay/upay_t_head_v2/jl_transitcode/transit_task.c \
	apps/common/perf_counter/perf_counter.c \
	apps/common/temp_trim/dtemp_pll_trim.c \
	apps/common/test/fs_test.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/mass_data/mass_data.c \
	apps/common/time/time.c \
	apps/common/timestamp/timestamp.c \
	apps/common/ui/fb_component/fb_combine.c \
	apps/common/ui/fb_component/fb_lcd.c \
	apps/common/ui/fb_component/fb_out_dev.c \
	apps/common/ui/jlui_demo/jlui_demo_compass.c \
	apps/common/ui/jlui_demo/jlui_demo_hslider.c \
	apps/common/ui/jlui_demo/jlui_demo_layout_rotate.c \
	apps/common/ui/jlui_demo/jlui_demo_layout_scale.c \
	apps/common/ui/jlui_demo/jlui_demo_main.c \
	apps/common/ui/jlui_demo/jlui_demo_number.c \
	apps/common/ui/jlui_demo/jlui_demo_time.c \
	apps/common/ui/jlui_demo/jlui_demo_vslider.c \
	apps/common/ui/jlui_demo/jlui_demo_watch.c \
	apps/common/ui/jlui_demo/jlui_page_switch.c \
	apps/common/ui/lcd/lcd_ui_api.c \
	apps/common/ui/lcd/lcd_ui_return_page.c \
	apps/common/update/ex_flash_file_download.c \
	apps/common/update/http_db_passive_update_demo.c \
	apps/common/update/lc_flash_ufw_update.c \
	apps/common/update/nandflash_ufw_update.c \
	apps/common/update/norflash_ufw_update.c \
	apps/common/update/reusable_update.c \
	apps/common/update/testbox_uart_update.c \
	apps/common/update/testbox_update.c \
	apps/common/update/uart_update.c \
	apps/common/update/uart_update_master.c \
	apps/common/update/update.c \
	apps/common/update/user_file_download/ext_flash_file_download.c \
	apps/common/update/user_file_download/nand_zone_file_download.c \
	apps/common/update/user_file_download/reserve_file_download.c \
	apps/common/update/user_file_download/user_file_download.c \
	apps/watch/app_main.c \
	apps/watch/audio/jlstream_event_handler.c \
	apps/watch/audio/mix_record_api.c \
	apps/watch/audio/rcsp_mix_record.c \
	apps/watch/audio/scene_switch.c \
	apps/watch/audio/tone_table.c \
	apps/watch/audio/vol_sync.c \
	apps/watch/board/adkey_config.c \
	apps/watch/board/br33/board_jl380n_demo/board_jl380n_demo.c \
	apps/watch/board/iokey_config.c \
	apps/watch/board/rdeckey_config.c \
	apps/watch/call/call_common.c \
	apps/watch/common/common_api.c \
	apps/watch/common/common_music_ctrl.c \
	apps/watch/data_storage/data_call_log_storage.c \
	apps/watch/data_storage/data_message_storage.c \
	apps/watch/data_storage/data_phonebook_storage.c \
	apps/watch/data_storage/data_rcsp_interface.c \
	apps/watch/data_storage/data_storage_fdb.c \
	apps/watch/data_storage/data_storage_null.c \
	apps/watch/data_storage/data_storage_vm.c \
	apps/watch/data_storage/data_weather_storage.c \
	apps/watch/demo/att_over_edr_demo.c \
	apps/watch/demo/pbg_demo.c \
	apps/watch/demo/trans_data_demo.c \
	apps/watch/health_manager/algo/shm_algo_gsensor.c \
	apps/watch/health_manager/common/health_manager.c \
	apps/watch/health_manager/common/shm_common_api.c \
	apps/watch/health_manager/common/shm_file_api.c \
	apps/watch/health_manager/common/shm_info_storage.c \
	apps/watch/health_manager/detect/shm_det_mense.c \
	apps/watch/health_manager/module/shm_mod_blood_oxygen.c \
	apps/watch/health_manager/module/shm_mod_daily_active.c \
	apps/watch/health_manager/module/shm_mod_heart_rate.c \
	apps/watch/health_manager/module/shm_mod_sleep.c \
	apps/watch/health_manager/module/shm_mod_sport.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_bt_disconn.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_continuous_heart_rate.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_exercise_heart_rate.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_fall_detection.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_personal_info.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_pressure_detection.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_raise_wrist.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_rcsp_interface.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_sedentary.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_sensor_opt.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_info_opt/sport_info_sleep_detection.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_opt/sport_data_rcsp_interface.c \
	apps/watch/health_manager/rcsp_adaptor/sport_data_sync_opt/sport_data_sync_rcsp_interface.c \
	apps/watch/jlstreamtran/jlstream_recv_demo.c \
	apps/watch/log_config/app_config.c \
	apps/watch/log_config/lib_btctrler_config.c \
	apps/watch/log_config/lib_btstack_config.c \
	apps/watch/log_config/lib_driver_config.c \
	apps/watch/log_config/lib_jlui_config.c \
	apps/watch/log_config/lib_media_config.c \
	apps/watch/log_config/lib_mpi_config.c \
	apps/watch/log_config/lib_net_config.c \
	apps/watch/log_config/lib_system_config.c \
	apps/watch/log_config/lib_update_config.c \
	apps/watch/log_config/lib_video_config.c \
	apps/watch/media_file/media_file_list.c \
	apps/watch/media_file/simple_avi_unpkg.c \
	apps/watch/message/adapter/app_msg.c \
	apps/watch/message/adapter/audio_player.c \
	apps/watch/message/adapter/battery.c \
	apps/watch/message/adapter/btstack.c \
	apps/watch/message/adapter/driver.c \
	apps/watch/message/adapter/key.c \
	apps/watch/misc/bitstore.c \
	apps/watch/mode/bt/a2dp_play.c \
	apps/watch/mode/bt/bt.c \
	apps/watch/mode/bt/bt_app_msg_handler.c \
	apps/watch/mode/bt/bt_background.c \
	apps/watch/mode/bt/bt_emitter.c \
	apps/watch/mode/bt/bt_event_func.c \
	apps/watch/mode/bt/bt_key_func.c \
	apps/watch/mode/bt/bt_key_msg_table.c \
	apps/watch/mode/bt/bt_slience_detect.c \
	apps/watch/mode/bt/dual_conn.c \
	apps/watch/mode/bt/low_latency.c \
	apps/watch/mode/bt/phone_call.c \
	apps/watch/mode/bt/poweroff.c \
	apps/watch/mode/bt/sniff.c \
	apps/watch/mode/bt/tone.c \
	apps/watch/mode/common/app_default_msg_handler.c \
	apps/watch/mode/common/dev_multiplex_api.c \
	apps/watch/mode/common/dev_status.c \
	apps/watch/mode/idle/idle.c \
	apps/watch/mode/idle/idle_app_msg_handler.c \
	apps/watch/mode/idle/idle_key_msg_table.c \
	apps/watch/mode/key_tone.c \
	apps/watch/mode/music/music.c \
	apps/watch/mode/music/music_app_msg_handler.c \
	apps/watch/mode/music/music_key_msg_table.c \
	apps/watch/mode/power_on/power_on.c \
	apps/watch/mode/update/update.c \
	apps/watch/mode/video/video.c \
	apps/watch/product_test/product_test.c \
	apps/watch/product_test/pt_gpio.c \
	apps/watch/product_test/pt_gsensor.c \
	apps/watch/product_test/pt_hr.c \
	apps/watch/product_test/pt_lcd_tp.c \
	apps/watch/product_test/pt_motor.c \
	apps/watch/product_test/pt_sd.c \
	apps/watch/product_test/pt_speaker_mic.c \
	apps/watch/screen_mirror/screen_mirror.c \
	apps/watch/screen_mirror/screen_mirror_bt_h264.c \
	apps/watch/sensor_hub/algo_motion_config.c \
	apps/watch/sensor_hub/gsensor/sc7a20.c \
	apps/watch/sensor_hub/msensor/driver_mmc5603.c \
	apps/watch/sensor_hub/sensor_driver.c \
	apps/watch/sensor_hub/sensor_driver_p11.c \
	apps/watch/sensor_hub/sensor_hal_acceler.c \
	apps/watch/sensor_hub/sensor_hal_gyro.c \
	apps/watch/sensor_hub/sensor_hal_magnetic.c \
	apps/watch/sensor_hub/sensor_hal_vc_hr_11.c \
	apps/watch/sensor_hub/sensor_hal_wrist_tilt.c \
	apps/watch/sensor_hub/sensor_hub.c \
	apps/watch/sensor_hub/sensor_service_compass.c \
	apps/watch/sensor_hub/sensor_service_motion.c \
	apps/watch/sensor_hub/sensor_service_vc_hr_spo2.c \
	apps/watch/sensor_hub/sensor_service_wrist_tilt.c \
	apps/watch/test/rssi.c \
	apps/watch/third_part/ai_interaction/ai_audio.c \
	apps/watch/third_part/ai_interaction/ai_interaction.c \
	apps/watch/third_part/ai_interaction/ai_text.c \
	apps/watch/third_part/app_protocol_deal.c \
	apps/watch/tools/app_testbox.c \
	apps/watch/ui/jlui_app/lyrics_api.c \
	apps/watch/ui/jlui_app/ui_app_msg_watch_style.c \
	apps/watch/ui/jlui_app/ui_dial_manage.c \
	apps/watch/ui/jlui_app/ui_screen_saver.c \
	apps/watch/ui/jlui_app/ui_sys_param_api.c \
	apps/watch/ui/jlui_app/watch_syscfg_manage.c \
	apps/watch/ui/smart_voice_user.c \
	apps/watch/ui/ui_app_module/mense_manage/mense_manage.c \
	apps/watch/ui/ui_app_module/ui_app_effect/ui_app_effect.c \
	apps/watch/ui/ui_app_module/ui_global_msg/ui_global_msg.c \
	apps/watch/ui/ui_key_remap.c \
	apps/watch/user_cfg.c \
	apps/watch/video/transfer/tjpgd.c \
	apps/watch/video/video_api.c \
	apps/watch/wifi/ctp/ctp_cmd.c \
	apps/watch/wifi/ctp/ctp_cmd_handle.c \
	apps/watch/wifi/ctp/ctp_screen_mirror_cmd.c \
	apps/watch/wifi/ctp/ctp_video_cmd.c \
	apps/watch/wifi/ctp/log_config.c \
	apps/watch/wifi/lte_app_task.c \
	apps/watch/wifi/lte_network_test.c \
	apps/watch/wifi/lws_full_test.c \
	apps/watch/wifi/playback/packet.c \
	apps/watch/wifi/playback/video_playback.c \
	apps/watch/wifi/playback/video_preview.c \
	apps/watch/wifi/spi_net_app_task.c \
	apps/watch/wifi/stream/mpi_video_net.c \
	apps/watch/wifi/uart_net_app_task.c \
	apps/watch/wifi/wifi_app_task.c \
	cpu/br33/bp_table.c \
	cpu/br33/charge/charge.c \
	cpu/br33/charge/charge_config.c \
	cpu/br33/charge/chargebox_hw.c \
	cpu/br33/charge/chargestore.c \
	cpu/br33/charge/chargestore_config.c \
	cpu/br33/overlay_code.c \
	cpu/br33/periph/led/pwm_led.c \
	cpu/br33/periph/pwm_interface.c \
	cpu/br33/periph/touch/lpctmu_hw.c \
	cpu/br33/power/key_wakeup.c \
	cpu/br33/power/p11_cbuf.c \
	cpu/br33/power/power_app.c \
	cpu/br33/power/power_config.c \
	cpu/br33/power/power_port.c \
	cpu/br33/power/ram_share.c \
	cpu/br33/setup.c \
	cpu/br33/ui_driver/gpu_demo/anim/gpu_anim_cube.c \
	cpu/br33/ui_driver/gpu_demo/anim/gpu_anim_flip.c \
	cpu/br33/ui_driver/gpu_demo/draw/gpu_demo_draw_cfg.c \
	cpu/br33/ui_driver/gpu_demo/draw/gpu_task.c \
	cpu/br33/ui_driver/gpu_demo/draw/lcd_task.c \
	cpu/br33/ui_driver/gpu_demo/draw/render_queue.c \
	cpu/br33/ui_driver/gpu_demo/draw/ui_pipeline_demo.c \
	cpu/br33/ui_driver/gpu_demo/draw/ui_task.c \
	cpu/br33/ui_driver/gpu_demo/gpu_buf_manager.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_anglegrad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_bar.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_breakpoint.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_cubic_bezier_curve.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_draw.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_fill.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_img_fill.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_img_grad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_jlui_res.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_line_chart.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_linear_radial_grad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_lineargrad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_radial_angle_grad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_radialgrad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_ring.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_ringgrad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_roundgrad.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_slider.c \
	cpu/br33/ui_driver/gpu_demo/gpu_demo_texture.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_compress.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_raw_line.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_raw_tile.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_rle_line.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_rle_tile.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_type_alpha_end.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_type_bigend.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_type_raw.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_data_type_rb_swap.c \
	cpu/br33/ui_driver/gpu_demo/img_data/img_test.c \
	cpu/br33/ui_driver/interface/gpu_draw.c \
	cpu/br33/ui_driver/interface/gpu_exception.c \
	cpu/br33/ui_driver/interface/gpu_task.c \
	cpu/br33/ui_driver/interface/jpeg_manager.c \
	cpu/br33/ui_driver/interface/jpeg_stream.c \
	cpu/br33/ui_driver/interface/lcd_task.c \
	cpu/br33/ui_driver/interface/ui_buf_manager.c \
	cpu/br33/ui_driver/interface/ui_platform.c \
	cpu/br33/ui_driver/interface/ui_resource.c \
	cpu/br33/ui_driver/lcd_drive/lcd_drive.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mcu/lcd_mcu_D54E4PA7551B_XP2_6_240x400.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mcu/lcd_mcu_JD5858_360x360.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mcu/lcd_mcu_NV3031A_240x320.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mcu/lcd_mcu_st7789v_240x320.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mipi/lcd_mipi_CO5300_466x466.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mipi/lcd_mipi_ILI9881C_800x1280.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mipi/lcd_mipi_JD9165T_1024x600.c \
	cpu/br33/ui_driver/lcd_drive/lcd_mipi/lcd_mipi_ST7701S_480x854.c \
	cpu/br33/ui_driver/lcd_drive/lcd_rgb/lcd_rgb_480x272.c \
	cpu/br33/ui_driver/lcd_drive/lcd_rgb/lcd_rgb_fl58202_800x480.c \
	cpu/br33/ui_driver/lcd_drive/lcd_rgb/lcd_rgb_st7701s_480x800.c \
	cpu/br33/ui_driver/lcd_drive/lcd_rgb/lcd_rgb_st7789v_240x240.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_CO5300_466x466.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_GC9B71_320x386.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_ICNA3310B_466x466.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_JD9161C_480x480.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_JD9853_320x172.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_ST77903_400x400_v2.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_ST7801N_320x386.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_sh8501a_240x296.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_qspi_st77916_320x385.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_gc9307_RQ_BOE_172x320.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_rm69330_454x454.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_sh8601a_454x454.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_st7789_BOE1.54_update_240x240.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_st7789v_240x240.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/lcd_spi_st7789v_240x320.c \
	cpu/br33/ui_driver/lcd_drive/lcd_spi/oled_spi_nb3030_172x320.c \
	cpu/components/iic_api.c \
	cpu/components/iic_soft.c \
	cpu/components/ir_decoder.c \
	cpu/components/ir_encoder.c \
	cpu/components/rdec.c \
	cpu/components/rdec_soft.c \
	cpu/components/sdpg_v4.c \
	cpu/config/gpio_config.c \
	cpu/config/gpio_file_parse.c \
	cpu/config/lib_power_config.c \
	cpu/periph_demo/adv_timer_demo.c \
	cpu/periph_demo/gptimer_demo.c \
	cpu/periph_demo/iic_master_demo.c \
	cpu/periph_demo/iic_slave_demo.c \
	cpu/periph_demo/ir_encoder_decoder_demo.c \
	cpu/periph_demo/ledc_test.c \
	cpu/periph_demo/pwm_led_test.c \
	cpu/periph_demo/rdec_demo.c \
	cpu/periph_demo/sd_test.c \
	cpu/periph_demo/spi_test.c \
	cpu/periph_demo/syscfg_otp_demo.c \
	cpu/periph_demo/uart_test.c \
	cpu/power/msg.c \


# 需要编译的 .S 文件
S_SRC_FILES := \
	apps/watch/sdk_version.z.S \


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES := \
	apps/common/c++/cxx_runtime.cpp \


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


include build/fileList.mk

# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3v2 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--sort-common \
	--plugin-opt=-used-symbol-file=apps/watch/sdk_used_list.used \
	--plugin-opt=-enable-movable-region=true \
	--plugin-opt=-movable-region-section-prefix=.movable.slot. \
	--plugin-opt=-movable-region-stub-section-prefix=.movable.stub. \
	--plugin-opt=-movable-region-prefix=.movable.region. \
	--plugin-opt=-movable-region-stub-prefix=__movable_stub_ \
	--plugin-opt=-movable-region-stub-swi-number=-2 \
	--plugin-opt=-movable-region-map-file-list=apps/watch/movable/funcname.txt \
	--plugin-opt=-movable-region-section-map-file-list=apps/watch/movable/section.txt \
	--plugin-opt=-movable-region-exclude-func-file-list=apps/watch/movable/exclude.txt \
	--plugin-opt=-pi32v2-large-program=true \
	--plugin-opt=-floating-point-may-trap=true \
	--gc-sections \
	--plugin-opt=-pi32v2-csync-before-rti=true \
	--start-group \
	cpu/br33/liba/cpu.a \
	cpu/br33/liba/system.a \
	cpu/br33/liba/btstack.a \
	cpu/br33/liba/rcsp_stack.a \
	cpu/br33/liba/btctrler.a \
	cpu/br33/liba/aec.a \
	cpu/br33/liba/media.a \
	cpu/br33/liba/event.a \
	cpu/br33/liba/jldtp.a \
	cpu/br33/liba/libepmotion.a \
	cpu/br33/liba/libAptFilt_pi32v2_OnChip.a \
	cpu/br33/liba/libEchoSuppress_pi32v2_OnChip.a \
	cpu/br33/liba/libNoiseSuppress_pi32v2_OnChip.a \
	cpu/br33/liba/libSplittingFilter_pi32v2_OnChip.a \
	cpu/br33/liba/libSMS_TDE_pi32v2_OnChip.a \
	cpu/br33/liba/libSMS_TDE_Stereo_pi32v2_OnChip.a \
	cpu/br33/liba/libDualMicSystem_pi32v2_OnChip.a \
	cpu/br33/liba/libAdaptiveEchoSuppress_pi32v2_OnChip.a \
	cpu/br33/liba/lib_esco_audio_plc.a \
	cpu/br33/liba/noisegate.a \
	cpu/br33/liba/lib_mp3_dec.a \
	cpu/br33/liba/lib_mty_dec.a \
	cpu/br33/liba/lib_wav_dec.a \
	cpu/br33/liba/lib_adpcm_code.a \
	cpu/br33/liba/lib_adpcm_ima_code.a \
	cpu/br33/liba/agreement.a \
	cpu/br33/liba/opus_enc_lib.a \
	cpu/br33/liba/libLHDCv4DecodeLib.a \
	cpu/br33/liba/lib_ldac_dec.a \
	cpu/br33/liba/lib_ogg_opus_dec.a \
	cpu/br33/liba/lib_jla_v2_codec.a \
	cpu/br33/liba/lib_jl_codec_common.a \
	cpu/br33/liba/lib_lc3_codec.a \
	cpu/br33/liba/lib_jla_codec.a \
	cpu/br33/liba/libjlsp.a \
	cpu/br33/liba/libjlsp_far_keyword.a \
	cpu/br33/liba/libjlsp_kws_api.a \
	cpu/br33/liba/libjlsp_phone_call_command_keyword.a \
	cpu/br33/liba/libjlsp_smart_watch.a \
	cpu/br33/liba/libjlsp_yesno.a \
	cpu/br33/liba/lib_advaudio_plc.a \
	cpu/br33/liba/drc.a \
	cpu/br33/liba/lib_drc_advance.a \
	cpu/br33/liba/DynamicEQ.a \
	cpu/br33/liba/lib_ape_dec.a \
	cpu/br33/liba/VirtualBass.a \
	cpu/br33/liba/lib_plateReverb_adv.a \
	cpu/br33/liba/lib_reverb_cal.a \
	cpu/br33/liba/lib_wtgv2_dec.a \
	cpu/br33/liba/lib_voiceChanger.a \
	cpu/br33/liba/lib_flac_dec.a \
	cpu/br33/liba/lib_pitch_speed.a \
	cpu/br33/liba/lib_Chorus.a \
	cpu/br33/liba/lib_PcmDelay.a \
	cpu/br33/liba/lib_wma_dec.a \
	cpu/br33/liba/lib_FrequencyShift.a \
	cpu/br33/liba/howling.a \
	cpu/br33/liba/SpectrumShow.a \
	cpu/br33/liba/lib_StereoWidener.a \
	cpu/br33/liba/lib_HarmonicExciter.a \
	cpu/br33/liba/lib_drc_detect.a \
	cpu/br33/liba/lib_mp2_code.a \
	cpu/br33/liba/lib_SurroundEffect.a \
	cpu/br33/liba/lib_resample_fast_cal.a \
	cpu/br33/liba/lib_AutoWah.a \
	cpu/br33/liba/lib_PingPongDelay.a \
	cpu/br33/liba/lib_ThreeD_effect.a \
	cpu/br33/liba/libkwscommon.a \
	cpu/br33/liba/lib_msbc_codec.a \
	cpu/br33/liba/lib_f2a_dec.a \
	cpu/br33/liba/lib_limiter.a \
	cpu/br33/liba/lib_iir_filter.a \
	cpu/br33/liba/lib_speex_codec.a \
	cpu/br33/liba/noisegate.a  \
	cpu/br33/liba/lib_gain_mix.a \
	cpu/br33/liba/lib_eq_design_coeff.a \
	cpu/br33/liba/lib_sbc_eng.a \
	cpu/br33/liba/lib_mp3_enc.a \
	cpu/br33/liba/qr_code.a \
	apps/watch/sensor_hub/driver_vc_hr_11/vc_lib_hr_v637_jieli_NA_01.a \
	apps/watch/sensor_hub/driver_vc_hr_11/vc_lib_spo2_lc11s_int_JieLi_NA_01.a \
	apps/watch/sensor_hub/driver_vc_hr_11/vc_lib_sportMotion_std_int_jieli_NA_01.a \
	cpu/br33/liba/lib_dns_v3.a \
	cpu/br33/liba/lib_nn_v3.a \
	cpu/br33/liba/lib_nn_v3_params.a \
	cpu/br33/liba/libSMSVF_TDE_pi32v2_OnChip.a \
	cpu/br33/liba/libFFT_pi32v2_OnChip.a \
	cpu/br33/liba/lib_wtg_dec.a \
	cpu/br33/liba/lib_wtgv2_dec.a  \
	cpu/br33/liba/lib_bt_aac_dec.a \
	cpu/br33/liba/lib_sbc_dec.a \
	cpu/br33/liba/crypto_toolbox_Osize.a \
	cpu/br33/liba/lib_dns.a \
	cpu/br33/liba/update.a \
	cpu/br33/liba/jl_sm_br33.a \
	cpu/br33/liba/quickjs.a \
	cpu/br33/liba/lwip_2_2_0.a \
	cpu/br33/liba/mbedtls_3_4_0.a \
	cpu/br33/liba/websocket.a \
	cpu/br33/liba/libwebsockets.a \
	cpu/br33/liba/http_cli.a \
	cpu/br33/liba/http_server.a \
	cpu/br33/liba/net_server.a \
	cpu/br33/liba/json.a \
	cpu/br33/liba/cJSON.a \
	cpu/br33/liba/libmpeg.a \
	cpu/br33/liba/network_download.a \
	cpu/br33/liba/libcurl.a \
	cpu/br33/liba/lte_module.a \
	--whole-archive \
	apps/common/pay/upay_t_head_v2/3rdparty/protobuf-c/lib/lib3rdparty_protobuf-c_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/3rdparty/mbedtls/lib/lib3rdparty_mbedtls_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/3rdparty/regex/lib/lib3rdparty_regex_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/3rdparty/sm3/lib/lib3rdparty_sm3_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/3rdparty/uEcc/lib/lib3rdparty_uEcc_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/3rdparty/cJSON/lib/lib3rdparty_cJSON_pi32_jl.a \
	--no-whole-archive \
	--whole-archive  \
	apps/common/pay/upay_t_head_v2/app/aid_code/lib/libapp_aid_code_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/bind_ble/lib/libapp_bind_ble_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/pay_code/lib/libapp_pay_code_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/pay_result/lib/libapp_pay_result_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/provision/lib/libapp_provision_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/scan_pay/lib/libapp_scan_pay_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/setting/lib/libapp_setting_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/app/trans_code/lib/libapp_trans_code_pi32_jl.a \
	--no-whole-archive  \
	--whole-archive   \
	apps/common/pay/upay_t_head_v2/fwk/bind_status/lib/libfwk_bind_status_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/ble_communicate/lib/libfwk_ble_communicate_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/common/lib/libfwk_common_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/crypt/lib/libfwk_crypt_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/crypt_ext/lib/libfwk_crypt_ext_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/device_certificate/lib/libfwk_device_certificate_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/js_api/lib/libfwk_js_api_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/network/lib/libfwk_network_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/quickjs_adapt/lib/libfwk_quickjs_adapt_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/secure_mode_se/lib/libfwk_secure_mode_se_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/sm2/lib/libfwk_sm2_pi32_jl.a \
	apps/common/pay/upay_t_head_v2/fwk/storage/lib/libfwk_storage_pi32_jl.a \
	--no-whole-archive   \
	--whole-archive    \
	apps/common/pay/upay_t_head_v2/hal/csi/lib/libhal_csi_pi32_jl.a \
	--no-whole-archive    \
	--whole-archive     \
	apps/common/pay/upay_t_head_v2/share/common/lib/libshare_common_pi32_jl.a \
	--no-whole-archive     \
	--whole-archive      \
	apps/common/pay/upay_t_head_v2/app/antchain/lib/libapp_antchain_JL_gcc.a \
	--no-whole-archive      \
	apps/common/c++/libstdc++/ucos/libemutls.a \
	apps/common/c++/libstdc++/ucos/libc++.a \
	apps/common/c++/libstdc++/ucos/libc++abi.a \
	cpu/br33/liba/common_lib.a \
	cpu/br33/liba/cbuf.a \
	cpu/br33/liba/lbuf.a \
	cpu/br33/liba/printf.a \
	cpu/br33/liba/device.a \
	cpu/br33/liba/fs.a \
	cpu/br33/liba/ascii.a \
	cpu/br33/liba/cfg_tool.a \
	cpu/br33/liba/vm.a \
	cpu/br33/liba/chargestore.a \
	cpu/br33/liba/kfifo.a \
	cpu/br33/liba/ftl.a \
	cpu/br33/liba/debug_record.a \
	cpu/br33/liba/sscanf.a \
	cpu/br33/liba/screen_trans.a \
	cpu/br33/liba/ui_hw_dbi.a \
	cpu/br33/liba/ui_hw_gpu.a \
	cpu/br33/liba/ui_hw_dmm.a \
	cpu/br33/liba/ui_hw_gxdma.a \
	cpu/br33/liba/ui_hw_jpg.a \
	cpu/br33/liba/ui_common.a \
	cpu/br33/liba/jlgpu_framework.a \
	cpu/br33/liba/ui_jlui.a \
	cpu/br33/liba/ui_res_new.a \
	cpu/br33/liba/ui_expand.a \
	cpu/br33/liba/ui_font_new.a \
	cpu/br33/liba/ui_draw_new.a \
	cpu/br33/liba/ui_gif.a \
	cpu/br33/liba/ui_lv9_fitter.a \
	cpu/br33/liba/video.a \
	--start-group  \
	cpu/br33/liba/opt.a \
	cpu/br33/liba/iperf.a \
	cpu/br33/liba/libaicwifi.a \
	cpu/br33/liba/sdio_card.a \
	cpu/br33/liba/hsm_ext.a \
	cpu/br33/liba/ext_wifi_dev.a \
	cpu/br33/liba/lwip_2_2_0.a  \
	--end-group \
	--end-group  \
	cpu/br33/liba/libc.a \
	-Tcpu/br33/sdk.ld \
	-M=cpu/br33/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES))


# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/watch/sdk_used_list.c -o apps/watch/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/watch/movable/section.c -o apps/watch/movable/section.txt
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br33/sdk_ld.c -o cpu/br33/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br33/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br33/tools/isd_config_rule.c -o cpu/br33/tools/isd_config.ini

ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : %.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.S.o : %.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.s.o : %.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : %.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : %.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cc.o : %.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

-include $(DEP_FILES)
