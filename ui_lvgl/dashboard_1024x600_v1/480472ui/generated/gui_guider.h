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
	lv_obj_t *page_map_imglist_bat;
	lv_obj_t *page_map_lbl_odo_num;
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
	// Screen page_meter
	lv_obj_t *page_meter;
	bool      page_meter_del;
	lv_obj_t *page_meter_imglist_1;
	lv_timer_t *page_meter_timer_1;
	lv_obj_t *page_meter_imglist_2;
	lv_obj_t *page_meter_imglist_3;
	lv_obj_t *page_meter_img_1;
	lv_obj_t *page_meter_img_2;
	lv_obj_t *page_meter_img_3;
	lv_obj_t *page_meter_img_4;
	lv_obj_t *page_meter_img_5;
	lv_obj_t *page_meter_img_6;
	lv_obj_t *page_meter_img_7;
	lv_obj_t *page_meter_img_8;
	lv_obj_t *page_meter_img_9;
	lv_obj_t *page_meter_img_10;
	lv_obj_t *page_meter_img_11;
	lv_obj_t *page_meter_img_12;
	lv_obj_t *page_meter_img_13;
	lv_obj_t *page_meter_img_14;
	lv_obj_t *page_meter_img_15;
	lv_obj_t *page_meter_img_16;
	lv_obj_t *page_meter_img_17;
	lv_obj_t *page_meter_img_18;
	lv_obj_t *page_meter_btn_1;
	lv_obj_t *page_meter_btn_1_label;
	lv_obj_t *page_meter_btn_2;
	lv_obj_t *page_meter_btn_2_label;
} lv_ui_page_meter;

typedef struct {
	// Screen page_music
	lv_obj_t *page_music;
	bool      page_music_del;
	lv_obj_t *page_music_img_1;
	lv_obj_t *page_music_img_2;
	lv_timer_t *page_music_timer_1;
	lv_obj_t *page_music_slider_1;
	lv_obj_t *page_music_img_3;
	lv_obj_t *page_music_btn_1;
	lv_obj_t *page_music_btn_1_label;
} lv_ui_page_music;

// generate lv_ui gui_guider
typedef struct {
	lv_ui_page_map *page_map;
	lv_ui_page_meter *page_meter;
	lv_ui_page_music *page_music;
	
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







// Screen page_meter
lv_obj_t * setup_scr_page_meter(lv_ui *ui);
void unload_scr_page_meter(lv_ui *ui);



// Screen page_music
lv_obj_t * setup_scr_page_music(lv_ui *ui);
void unload_scr_page_music(lv_ui *ui);
#ifdef __cplusplus
}
#endif
#endif
