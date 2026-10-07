/*Generate Code, Do NOT Edit!*/
#include "lvgl.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include <stdlib.h>
#include "../gui_guider.h"
#include "../gui_events/events_init.h"
#include "../gui_events/callback_handler.h"
#include "../gui_timelines/gui_timelines.h"
#include "../../custom/custom.h"
#include "string.h"


lv_obj_t * setup_scr_page_music(lv_ui *ui)
{
	lv_ui_page_music * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_music));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_music));
		ui->page_music = ui_scr;
	}

	//Write codes page_music
	ui_scr->page_music = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_music, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music, GUI_CTRL_SCR);

	//Write codes page_music_img_1
	ui_scr->page_music_img_1 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_1, gui_get_res_path(GUI_RES_DIE1_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_1, 0);
	lv_image_set_scale(ui_scr->page_music_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_1, 22, 63);
	lv_obj_set_size(ui_scr->page_music_img_1, 167, 158);
	lv_obj_add_flag(ui_scr->page_music_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_1, GUI_CTRL_IMG);

	//Write codes page_music_img_2
	ui_scr->page_music_img_2 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_2, gui_get_res_path(GUI_RES_ZHEN_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_2, 0);
	lv_image_set_scale(ui_scr->page_music_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_2, 149, -2);
	lv_obj_set_size(ui_scr->page_music_img_2, 92, 181);
	lv_obj_add_flag(ui_scr->page_music_img_2, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_2, GUI_CTRL_IMG);

	// Write Timer page_music_timer_1
	if(ui_scr->page_music_timer_1 != NULL) lv_timer_delete(ui_scr->page_music_timer_1);
	ui_scr->page_music_timer_1 = lv_timer_create(page_music_timer_1_timer_cb, 30, "");
	lv_timer_set_repeat_count(ui_scr->page_music_timer_1, -1);

	//Write codes page_music_slider_1
	ui_scr->page_music_slider_1 = lv_slider_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_slider_1, 53, 235);
	lv_obj_set_size(ui_scr->page_music_slider_1, 102, 6);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_slider_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_slider_1, GUI_CTRL_SLIDER);
	lv_slider_set_range(ui_scr->page_music_slider_1, 0, 100);
	lv_slider_set_mode(ui_scr->page_music_slider_1, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->page_music_slider_1, 0, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->page_music_slider_1, 0, LV_ANIM_OFF);

	//Write codes page_music_img_3
	ui_scr->page_music_img_3 = lv_image_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_img_3, 281, 28);
	lv_obj_set_size(ui_scr->page_music_img_3, 120, 113);
	ui_style_set(ui_scr->page_music_img_3, GUI_CTRL_IMG);

	//Write codes page_music_btn_1
	ui_scr->page_music_btn_1 = lv_btn_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_btn_1, 73, 8);
	lv_obj_set_size(ui_scr->page_music_btn_1, 35, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_btn_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_btn_1, GUI_CTRL_BUTTON);
	ui_scr->page_music_btn_1_label = lv_label_create(ui_scr->page_music_btn_1);
	lv_label_set_text(ui_scr->page_music_btn_1_label, "back");
	lv_obj_set_style_pad_all(ui_scr->page_music_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_music_btn_1_label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_update_layout(ui_scr->page_music);
	ui_scr->page_music_del = false;

	//Init events for screen
	events_init_page_music(ui);
	return ui_scr->page_music;
}
void unload_scr_page_music(lv_ui *ui)
{
	lv_ui_page_music * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
	if(ui_scr->page_music_timer_1 != NULL) {
		lv_timer_delete(ui_scr->page_music_timer_1);
		ui_scr->page_music_timer_1 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
}
