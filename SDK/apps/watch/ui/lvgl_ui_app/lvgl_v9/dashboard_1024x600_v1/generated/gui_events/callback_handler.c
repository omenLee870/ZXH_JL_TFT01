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
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
		if (ui_scr != NULL && ui_scr->page_map_timer_1 == src) {
			ui_scr->page_map_timer_1 = NULL;
		}
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
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
		if (ui_scr != NULL && ui_scr->page_map_timer_2 == src) {
			ui_scr->page_map_timer_2 = NULL;
		}
	}

}
// Generate page_app_select_animimg_1 CallBack Handler
void page_app_select_animimg_1_anim_start_cb(lv_anim_t * anim)
{

}
// Generate page_app_select_animimg_1 CallBack Handler
void page_app_select_animimg_1_anim_ready_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_3 CallBack Handler
void page_meter_animimg_3_anim_start_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_3 CallBack Handler
void page_meter_animimg_3_anim_ready_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_1 CallBack Handler
void page_meter_animimg_1_anim_start_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_1 CallBack Handler
void page_meter_animimg_1_anim_ready_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_2 CallBack Handler
void page_meter_animimg_2_anim_start_cb(lv_anim_t * anim)
{

}
// Generate page_meter_animimg_2 CallBack Handler
void page_meter_animimg_2_anim_ready_cb(lv_anim_t * anim)
{

}
// Generate page_meter_timer_1 CallBack Handler
void page_meter_timer_1_timer_cb(lv_timer_t * src)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);

	static uint8_t tens_idx = 0;
	lv_imglist_set_act(ui_scr->page_meter_imglist_speed_tens, tens_idx);
	lv_imglist_set_act(ui_scr->page_meter_imglist_speed_ones, tens_idx);

	if(tens_idx <=  9){
	  tens_idx++;
	}else{
	  tens_idx = 0;
	}

	if (src->repeat_count == 0) {
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
		if (ui_scr != NULL && ui_scr->page_meter_timer_1 == src) {
			ui_scr->page_meter_timer_1 = NULL;
		}
	}

}
// Generate page_meter_timer_2 CallBack Handler
void page_meter_timer_2_timer_cb(lv_timer_t * src)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);

	static uint8_t turn_img_flag = 0;

	if(turn_img_flag){
	  turn_img_flag = 0;
	  lv_obj_add_flag(ui_scr->page_meter_img_left_turn, LV_OBJ_FLAG_HIDDEN);
	  lv_obj_add_flag(ui_scr->page_meter_img_right_turn, LV_OBJ_FLAG_HIDDEN);
	}else{
	  turn_img_flag = 1;
	  lv_obj_clear_flag(ui_scr->page_meter_img_left_turn, LV_OBJ_FLAG_HIDDEN);
	  lv_obj_clear_flag(ui_scr->page_meter_img_right_turn, LV_OBJ_FLAG_HIDDEN);
	}

	if (src->repeat_count == 0) {
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
		if (ui_scr != NULL && ui_scr->page_meter_timer_2 == src) {
			ui_scr->page_meter_timer_2 = NULL;
		}
	}

}
// Generate page_meter_timer_3 CallBack Handler
void page_meter_timer_3_timer_cb(lv_timer_t * src)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);

	static uint8_t turn_img = 0;
	#if 0
	switch (turn_img) {
	    case 0:
	        lv_obj_clear_flag(ui_scr->home_img_brake_system_fault, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_hdc, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 1:
	        lv_obj_clear_flag(ui_scr->home_img_hdc, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_position, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 2:
	        lv_obj_clear_flag(ui_scr->home_img_position, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_imgList_high_low_beam, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 3:
	        lv_obj_clear_flag(ui_scr->home_imgList_high_low_beam, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_auto, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 4:
	        lv_obj_clear_flag(ui_scr->home_img_auto, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_abs, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 5:
	        lv_obj_clear_flag(ui_scr->home_img_abs, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_limping, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 6:
	        lv_obj_clear_flag(ui_scr->home_img_limping, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_P_mode, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 7:
	        lv_obj_clear_flag(ui_scr->home_img_P_mode, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_force_touch, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 8:
	        lv_obj_clear_flag(ui_scr->home_img_force_touch, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_box_open, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 9:
	        lv_obj_clear_flag(ui_scr->home_img_box_open, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_side_support, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 10:
	        lv_obj_clear_flag(ui_scr->home_img_side_support, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_tcs, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 11:
	        lv_obj_clear_flag(ui_scr->home_img_tcs, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_tapLoc, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 12:
	        lv_obj_clear_flag(ui_scr->home_img_tapLoc, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_car_fault, LV_OBJ_FLAG_HIDDEN);
	        turn_img++;
	        break;
	    case 13:
	        lv_obj_clear_flag(ui_scr->home_img_car_fault, LV_OBJ_FLAG_HIDDEN);
	        lv_obj_add_flag(ui_scr->home_img_brake_system_fault, LV_OBJ_FLAG_HIDDEN);
	        turn_img = 0;
	        break;
	    }

	#endif

	if (src->repeat_count == 0) {
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
		if (ui_scr != NULL && ui_scr->page_meter_timer_3 == src) {
			ui_scr->page_meter_timer_3 = NULL;
		}
	}

}
// Generate hardware_test_timer_1 CallBack Handler
void hardware_test_timer_1_timer_cb(lv_timer_t * src)
{
	lv_ui_hardware_test * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
	hardware_test_ui_highlight_timer_cb();

	if (src->repeat_count == 0) {
		ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
		if (ui_scr != NULL && ui_scr->hardware_test_timer_1 == src) {
			ui_scr->hardware_test_timer_1 = NULL;
		}
	}

}
