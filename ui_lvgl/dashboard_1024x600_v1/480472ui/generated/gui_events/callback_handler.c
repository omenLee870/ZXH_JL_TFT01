/*Generate Code, Do NOT Edit!*/
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include "../gui_guider.h"
#include "lvgl.h"
#include "./callback_handler.h"

// Generate page_map_timer_1 CallBack Handler
void page_map_timer_1_timer_cb(lv_timer_t * src)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
	
	#if 1
	static uint8_t bat_idx = 0;
	lv_imglist_set_act(ui_scr->page_map_imglist_bat, bat_idx);
	if(bat_idx <  99){
	  bat_idx++;
	}else{
	  bat_idx = 0;
	}
	#endif
	
	if (src->repeat_count == 0) {
		ui_scr->page_map_timer_1 = NULL;
	}
	
}
// Generate page_map_timer_2 CallBack Handler
void page_map_timer_2_timer_cb(lv_timer_t * src)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
	if(!img_2_init_flag){
	  lv_obj_add_flag(ui_scr->page_map_img_2 , LV_OBJ_FLAG_HIDDEN);
	  img_2_init_flag = 1;
	}
	
	
	if (src->repeat_count == 0) {
		ui_scr->page_map_timer_2 = NULL;
	}
	
}
// Generate page_meter_timer_1 CallBack Handler
void page_meter_timer_1_timer_cb(lv_timer_t * src)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
	#if 1
	static uint8_t meter_idx = 0;
	lv_imglist_set_act(ui_scr->page_meter_imglist_1, meter_idx);
	if(meter_idx <  21){
	  meter_idx++;
	}else{
	  meter_idx = 0;
	}
	
	static uint8_t imglist_2_idx = 0;
	lv_imglist_set_act(ui_scr->page_meter_imglist_2, imglist_2_idx);
	lv_imglist_set_act(ui_scr->page_meter_imglist_3, imglist_2_idx);
	if(meter_idx <  21 && imglist_2_idx < 9){
	  imglist_2_idx += 2;
	}else{
	  imglist_2_idx = 0;
	}
	#endif
	
	if (src->repeat_count == 0) {
		ui_scr->page_meter_timer_1 = NULL;
	}
	
}
// Generate page_music_timer_1 CallBack Handler
void page_music_timer_1_timer_cb(lv_timer_t * src)
{
	lv_ui_page_music * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
	#if 0
	static uint32_t angle_idx = 0;
	lv_img_set_angle(ui_scr->page_music_img_1 , angle_idx);
	lv_img_set_pivot(ui_scr->page_music_img_1, 139, 139);
	if(angle_idx < 3600){
	  angle_idx += 10;
	}else{
	  angle_idx = 0;
	}
	#endif
	
	#if 1
	static uint8_t slider_1_idx = 0;
	lv_slider_set_value(ui_scr->page_music_slider_1, slider_1_idx, LV_ANIM_OFF);
	if(slider_1_idx <=  99){
	  slider_1_idx++;
	}else{
	  slider_1_idx = 0;
	}
	#endif
	
	if (src->repeat_count == 0) {
		ui_scr->page_music_timer_1 = NULL;
	}
	
}
