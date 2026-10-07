/*Generate Code, Do NOT Edit!*/
#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "./common.h"
#include "./gui_fonts/gui_fonts.h"
#include "./gui_images/gui_images.h"
#include "./gui_scr/ui_scr_manager.h"
#include "./gui_scr/ui_style.h"

#ifdef JL_GUI_KERNEL_VERSION_MAJOR
#if GUI_CORE_VERSION_MAJOR > JL_GUI_KERNEL_VERSION_MAJOR
	#error "Current SDK Version is older than the kernel version. Please update the SDK version!"
#endif
#if GUI_CORE_VERSION_MAJOR < JL_GUI_KERNEL_VERSION_MAJOR
	#error "Current SDK Version is newer than the kernel version. Please update the kernel version!"
#endif
#if GUI_CORE_VERSION_MINOR > JL_GUI_KERNEL_VERSION_MINOR
	#warning "Current Kernel Version is too low, some features may not be available, please update the kernel version!"
#endif
#else
	#error "Current Kernel Version is not defined, please replace or upgrade the kernel!"
#endif

#ifndef LV_USE_GUIBUILDER_SIMULATOR
#if defined(COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MIN_MAJOR) && defined(COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MAX_MAJOR)
#if GUI_TEMPLATE_VERSION_MAJOR < COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MIN_MAJOR || GUI_TEMPLATE_VERSION_MAJOR > COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MAX_MAJOR
	#error "Current Project Template Version is not compatible with the SDK version. Please select a compatible SDK version!"
#endif
#if GUI_TEMPLATE_VERSION_MINOR < COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MIN_MINOR || GUI_TEMPLATE_VERSION_MINOR > COMPATIBLE_UI_PRJ_TEMPLATE_VERSION_MAX_MINOR
	#warning "Current Project Template Version has minor incompatibilities with the SDK. Some features may not be available. Consider updating the SDK version."
#endif
#endif
#endif

typedef struct {
	// Screen page_map
	lv_obj_t *page_map;
	bool      page_map_del;
	lv_obj_t *page_map_view_1;
	lv_obj_t *page_map_imgList_gear_mode;
	lv_obj_t *page_map_lbl_time;
	lv_obj_t *page_map_img_weather;
	lv_obj_t *page_map_lbl_temp;
	lv_obj_t *page_map_imgList_bluetooth;
	lv_obj_t *page_map_imgList_phone_bl;
	lv_obj_t *page_map_imgList_gps;
	lv_obj_t *page_map_imgList_networksignal;
	lv_obj_t *page_map_img_ready;
	lv_obj_t *page_map_view_2;
	lv_obj_t *page_map_lbl_endurance;
	lv_obj_t *page_map_img_charge;
	lv_obj_t *page_map_imgList_soc_beam;
	lv_obj_t *page_map_lbl_bat;
	lv_obj_t *page_map_lbl_odo_num;
	lv_obj_t *page_map_imglist_bat;
	lv_timer_t *page_map_timer_1;
	lv_obj_t *page_map_view_3;
	lv_obj_t *page_map_lbl_1;
	lv_obj_t *page_map_lbl_2;
	lv_obj_t *page_map_lbl_3;
	lv_obj_t *page_map_lbl_4;
	lv_obj_t *page_map_img_1;
	lv_timer_t *page_map_timer_2;
	lv_obj_t *page_map_btn_1;
	lv_obj_t *page_map_btn_1_label;
	lv_obj_t *page_map_btn_2;
	lv_obj_t *page_map_btn_2_label;
	lv_obj_t *page_map_view_4;
	lv_obj_t *page_map_lbl_5;
	lv_obj_t *page_map_lbl_6;
	lv_obj_t *page_map_lbl_7;
	lv_obj_t *page_map_lbl_8;
	lv_obj_t *page_map_lbl_9;
	lv_obj_t *page_map_lbl_10;
	lv_obj_t *page_map_img_2;
} lv_ui_page_map;

typedef struct {
	// Screen page_music
	lv_obj_t *page_music;
	bool      page_music_del;
	lv_obj_t *page_music_view_1;
	lv_obj_t *page_music_img_1;
	lv_obj_t *page_music_img_4;
	lv_obj_t *page_music_img_2;
	lv_obj_t *page_music_slider_1;
	lv_obj_t *page_music_img_3;
	lv_obj_t *page_music_lbl_1;
	lv_obj_t *page_music_img_5;
	lv_obj_t *page_music_imgbtn_1;
	lv_obj_t *page_music_imgbtn_1_label;
	lv_obj_t *page_music_lbl_3;
	lv_obj_t *page_music_lbl_4;
} lv_ui_page_music;

typedef struct {
	// Screen page_app_select
	lv_obj_t *page_app_select;
	bool      page_app_select_del;
	lv_obj_t *page_app_select_animimg_1;
	lv_obj_t *page_app_select_digitclock_1;
	lv_obj_t *page_app_select_imglist_1;
	lv_obj_t *page_app_select_lbl_1;
	lv_obj_t *page_app_select_imgList_bluetooth;
	lv_obj_t *page_app_select_imgList_phone_bl;
	lv_obj_t *page_app_select_imgList_gps;
	lv_obj_t *page_app_select_imgList_networksignal;
	lv_obj_t *page_app_select_lbl_mode;
	lv_obj_t *page_app_select_lbl_2;
	lv_obj_t *page_app_select_lbl_3;
	lv_obj_t *page_app_select_lbl_4;
	lv_obj_t *page_app_select_lbl_5;
	lv_obj_t *page_app_select_lbl_6;
	lv_obj_t *page_app_select_lbl_7;
	lv_obj_t *page_app_select_lbl_8;
	lv_obj_t *page_app_select_lbl_9;
	lv_obj_t *page_app_select_lbl_10;
	lv_obj_t *page_app_select_lbl_11;
	lv_obj_t *page_app_select_lbl_12;
	lv_obj_t *page_app_select_arcmenu_1;
	lv_obj_t *page_app_select_arcmenu_1_item;
	lv_obj_t *page_app_select_arcmenu_1_item1;
	lv_obj_t *page_app_select_arcmenu_1_item2;
	lv_obj_t *page_app_select_view_select_app_map;
	lv_obj_t *page_app_select_lbl_17;
	lv_obj_t *page_app_select_lbl_16;
	lv_obj_t *page_app_select_slider_2;
	lv_obj_t *page_app_select_lbl_15;
	lv_obj_t *page_app_select_lbl_14;
	lv_obj_t *page_app_select_img_1;
	lv_obj_t *page_app_select_view_select_app_music;
	lv_obj_t *page_app_select_slider_1;
	lv_obj_t *page_app_select_img_3;
	lv_obj_t *page_app_select_lbl_13;
	lv_obj_t *page_app_select_btn_back;
	lv_obj_t *page_app_select_btn_back_label;
} lv_ui_page_app_select;

typedef struct {
	// Screen page_meter
	lv_obj_t *page_meter;
	bool      page_meter_del;
	lv_obj_t *page_meter_animimg_3;
	lv_obj_t *page_meter_animimg_1;
	lv_obj_t *page_meter_animimg_2;
	lv_obj_t *page_meter_img_19;
	lv_obj_t *page_meter_img_2;
	lv_obj_t *page_meter_img_4;
	lv_obj_t *page_meter_img_20;
	lv_obj_t *page_meter_img_5;
	lv_obj_t *page_meter_img_7;
	lv_obj_t *page_meter_img_left_turn;
	lv_obj_t *page_meter_img_right_turn;
	lv_obj_t *page_meter_imgList_bluetooth;
	lv_obj_t *page_meter_imgList_phone_bl;
	lv_obj_t *page_meter_imgList_gps;
	lv_obj_t *page_meter_imgList_networksignal;
	lv_obj_t *page_meter_img_ready;
	lv_obj_t *page_meter_imglist_4;
	lv_obj_t *page_meter_lbl_1;
	lv_obj_t *page_meter_imgList_gear_mode;
	lv_obj_t *page_meter_img_1;
	lv_obj_t *page_meter_img_3;
	lv_obj_t *page_meter_img_10;
	lv_obj_t *page_meter_img_11;
	lv_obj_t *page_meter_img_12;
	lv_obj_t *page_meter_img_13;
	lv_obj_t *page_meter_img_14;
	lv_obj_t *page_meter_img_15;
	lv_obj_t *page_meter_img_16;
	lv_obj_t *page_meter_img_17;
	lv_obj_t *page_meter_img_18;
	lv_timer_t *page_meter_timer_1;
	lv_timer_t *page_meter_timer_2;
	lv_timer_t *page_meter_timer_3;
	lv_obj_t *page_meter_imglist_speed_tens;
	lv_obj_t *page_meter_imglist_speed_ones;
	lv_obj_t *page_meter_img_21;
	lv_obj_t *page_meter_imglist_5;
	lv_obj_t *page_meter_lbl_2;
	lv_obj_t *page_meter_lbl_3;
	lv_obj_t *page_meter_lbl_4;
	lv_obj_t *page_meter_lbl_5;
	lv_obj_t *page_meter_digitclock_1;
	lv_obj_t *page_meter_imglist_bat;
} lv_ui_page_meter;

typedef struct {
	// Screen page_video
	lv_obj_t *page_video;
	bool      page_video_del;
	lv_obj_t *page_video_img_1;
} lv_ui_page_video;

typedef struct {
	// Screen upgrade_page
	lv_obj_t *upgrade_page;
	bool      upgrade_page_del;
	lv_obj_t *upgrade_page_lbl_upgrade_title;
	lv_obj_t *upgrade_page_bar_upgrade;
	lv_obj_t *upgrade_page_lbl_upgrade_percent;
} lv_ui_upgrade_page;

typedef struct {
	// Screen hardware_test
	lv_obj_t *hardware_test;
	bool      hardware_test_del;
	lv_obj_t *hardware_test_lte_4g;
	lv_obj_t *hardware_test_lte_4g_label;
	lv_obj_t *hardware_test_edit_1;
	lv_obj_t *hardware_test_btn_5;
	lv_obj_t *hardware_test_btn_5_label;
	lv_obj_t *hardware_test_btn_bt;
	lv_obj_t *hardware_test_btn_bt_label;
	lv_obj_t *hardware_test_btn_gsensor;
	lv_obj_t *hardware_test_btn_gsensor_label;
	lv_obj_t *hardware_test_btn_7;
	lv_obj_t *hardware_test_btn_7_label;
	lv_obj_t *hardware_test_btn_ui_effect;
	lv_obj_t *hardware_test_btn_ui_effect_label;
	lv_obj_t *hardware_test_can;
	lv_obj_t *hardware_test_can_label;
	lv_obj_t *hardware_test_btn_4;
	lv_obj_t *hardware_test_btn_4_label;
	lv_obj_t *hardware_test_mic0;
	lv_obj_t *hardware_test_mic0_label;
	lv_obj_t *hardware_test_btn_6;
	lv_obj_t *hardware_test_btn_6_label;
	lv_obj_t *hardware_test_edit_2;
	lv_obj_t *hardware_test_edit_3;
	lv_timer_t *hardware_test_timer_1;
} lv_ui_hardware_test;

// generate lv_ui gui_guider
typedef struct {
	lv_ui_page_map *page_map;
	lv_ui_page_music *page_music;
	lv_ui_page_app_select *page_app_select;
	lv_ui_page_meter *page_meter;
	lv_ui_page_video *page_video;
	lv_ui_upgrade_page *upgrade_page;
	lv_ui_hardware_test *hardware_test;
	
	lv_group_t *default_group;
} lv_ui;

void ui_load_scr_anim(lv_ui *ui, gui_scr_t *screen, lv_screen_load_anim_t anim_type,	uint32_t time,
											uint32_t delay, bool is_clean, bool auto_del, bool is_push_satck);

void ui_scr_stack_pop_anim(lv_ui *ui, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay,
														bool is_clean, bool auto_del, bool is_push_satck);

gui_scr_t * ui_get_scr(int32_t scr_id);
gui_scr_t * ui_get_setup_scr(int32_t scr_id);
lv_obj_t * ui_get_setup_scr_obj(int32_t scr_id);
void * ui_get_scr_ptr(lv_ui *ui, int32_t scr_id);
void ui_free_scr_ptr(lv_ui *ui, int32_t scr_id);
bool ui_is_act_scr(int32_t scr_id);
void ui_init_style(lv_style_t * style);
void setup_ui(lv_ui *ui);
#include "./gui_msg/gui_msg.h"
extern lv_ui guider_ui;// Screen page_map
lv_obj_t * setup_scr_page_map(lv_ui *ui);
void unload_scr_page_map(lv_ui *ui);







// Screen page_music
lv_obj_t * setup_scr_page_music(lv_ui *ui);
void unload_scr_page_music(lv_ui *ui);
// Screen page_app_select
lv_obj_t * setup_scr_page_app_select(lv_ui *ui);
void unload_scr_page_app_select(lv_ui *ui);

/*DigitalClock*/
void clock_count_with_year(int *year, int *month, int *day, int *hour, int *min, int *sec);





// Screen page_meter
lv_obj_t * setup_scr_page_meter(lv_ui *ui);
void unload_scr_page_meter(lv_ui *ui);













// Screen page_video
lv_obj_t * setup_scr_page_video(lv_ui *ui);
void unload_scr_page_video(lv_ui *ui);
// Screen upgrade_page
lv_obj_t * setup_scr_upgrade_page(lv_ui *ui);
void unload_scr_upgrade_page(lv_ui *ui);
// Screen hardware_test
lv_obj_t * setup_scr_hardware_test(lv_ui *ui);
void unload_scr_hardware_test(lv_ui *ui);
#ifdef __cplusplus
}
#endif
#endif
