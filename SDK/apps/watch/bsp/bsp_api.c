
#include "app_config.h"

#if TCFG_FW_APP_ENABLE

// libc
ceil
/* nanf */
/* scalbnf */
atoi
strdup
strtol
/* fabsf */
/* fmod */
/* powf */
/* sqrtf */
sqrt
/* __adddf3 */
/* __ledf2 */
/* __gedf2 */
/* __unorddf2 */
/* __eqdf2 */
/* __ltdf2 */
/* __nedf2 */
/* __gtdf2 */
/* __divdf3 */
/* __muldf3 */
/* __subdf3 */
/* __extendsfdf2 */
/* __fixdfsi */
/* __floatsidf */
/* __floatunsidf */
/* __truncdfsf2 */

// base
ASCII_ToLower
CRC16
p33_soft_reset
cpu_assert_debug


// MEMORY
/* memory_init_rang */
/* malloc */
/* zalloc */
/* free */
/* mem_stats */
/* realloc */
/* _malloc_r */

// CBUF
cbuf_init
cbuf_read
cbuf_read_alloc
cbuf_read_updata
cbuf_clear
cbuf_get_data_size
cbuf_get_readptr
cbuf_is_write_able
cbuf_write
cbuf_write_alloc
cbuf_write_updata

// SPIN
spin_lock
spin_unlock

// LIST
/* list_del */
/* list_add_tail */

// MATH
root_fix
angle_fix
sin_fix
angle_float
complex_abs_float
// OS
task_create
os_current_task
os_sem_pend
os_time_dly

// print
sprintf
snprintf
vprintf

printf
puts
putchar
log_print
put_buf


#if 0

// battery
update_bat_info_vm

// 存储
dev_open
dev_bulk_read
syscfg_read
syscfg_write
resfile_open
resfile_read
resfile_write
resfile_seek
resfile_close
resfile_get_pos
resfile_get_len
resfile_get_attrs
resfile_get_name
sdfile_get_disk_capacity
sdfile_cpu_addr2flash_addr
sdfile_flash_addr2cpu_addr

// time
sys_timer_add
sys_timer_del
sys_timer_re_run
sys_timer_modify
sys_timeout_add
sys_timeout_del
sys_timeout_add_2_task
usr_timer_add
usr_timer_modify
usr_timer_del
usr_timeout_add
usr_timeout_modify
usr_timeout_del

get_system_us

timer_get_ms
time_lapse

time
localtime

jiffies_msec

rtc_read_time
rtc_write_time
rtc_calculate_week_val
caculate_weekday_by_time

get_alarm_number2table
alarm_get_info
alarm_add
alarm_get_total
alarm_delete
alarm_update_info_after_isr

// clock
clk_get_max_frequency
clock_lock

// key
key_callback_register

// msg
app_send_message
app_send_message_from

// mode
app_get_current_mode
app_get_current_mode_name
sys_enter_soft_poweroff

// bt
bt_get_curr_channel_state
bt_get_connect_status
bt_get_call_status
bt_a2dp_get_status
bt_phone_get_siri_status
bt_cmd_prepare
bt_get_mac_addr
bt_get_local_name
bt_key_music_prev
bt_key_music_next

bt_close_bredr
is_bredr_close

ble_list_clear_all
le_controller_get_mac

bredr_conn_last_dev

call_dial_number
call_ctrl_answer
call_ctrl_hangup

a2dp_player_runing

// sport
sport_health_ctrl_sport_start
sport_health_ctrl_sport_pause
sport_health_ctrl_sport_continue
sport_health_ctrl_sport_stop
sport_health_get_sport_calories
sport_health_get_sport_distance
sport_info_sync_start_exercise_rcsp
sport_info_sync_pause_exercise_rcsp
sport_info_sync_end_exercise_by_fw
sport_info_sync_keep_exercise_rcsp

// watch数据存储
ui_small_file_weather_get_count
ui_small_file_weather_get_size_by_index
ui_small_file_weather_read_by_index
ui_small_file_message_get_count
ui_small_file_phonebook_get_count
ui_small_file_phonebook_read_by_index

small_file_message_read_by_index
small_file_del_all

// touch
get_touch_x_y_status

// ui - dbi
lcd_clear
lcd_wait_busy
lcd_get_align
lcd_set_draw_area
lcd_draw_kistart
lcd_draw_continue
lcd_draw_set_callback
lcd_te_trig_kistart_set_busy

// ui - gpu
gpu_init
gpu_run
gpu_wait_done
gpu_free_all_tasks
gpu_get_format_bpp
gpu_reset_all_regs
gpu_set_out_layer
gpu_fill
gpu_fill_mask
gpu_texture
gpu_texture_mask
gpu_texture_affine
gpu_matrix_translate
gpu_matrix_scale
gpu_matrix_rotate

jlgpu_task_list_run
jlgpu_create_task_list_head
jlgpu_delete_task_list_head

jlgpu_set_task_list_out_format
jlgpu_set_task_list_out_buf

jlgpu_module_init
jlgpu_get_win_rect
jlgpu_set_win_rect

get_rect_cover

jlgpu_create_task
jlgpu_get_task_list_out_format

ui_disp_out_stride_set


// ui - jpeg
jljpeg_os_init
jljpeg_hw_config_stage1
jljpeg_hw_config_stage2
jljpeg_hw_config_stage3
jljpeg_hw_release
jljpeg_hw_resource_release
jljpeg_img_info_get


// ui driver
lcd_drv_backlight_ctrl
lcd_backlight_status
lcd_sleep_status
lcd_sleep_ctrl
lcd_get_screen_width
lcd_get_screen_height
lcd_reinit_wait_finish
lcd_bl_open
lcd_get_hdl
lcd_get_te_line_period_us
lcd_get_te_frame_period_us
lcd_get_spi_frame_period_us
lv_lcd_wait_te_sync
jiffies_usec
lv_blend_by_gpu
jlui_blend_by_gpu

#endif

#endif /* #if TCFG_FW_APP_ENABLE */

