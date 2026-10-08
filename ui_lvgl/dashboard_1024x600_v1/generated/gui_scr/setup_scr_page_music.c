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
	lv_obj_remove_flag(ui_scr->page_music, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->page_music, GUI_CTRL_SCR);

	//Write codes page_music_view_1
	ui_scr->page_music_view_1 = lv_obj_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_view_1, 7, 170);
	lv_obj_set_size(ui_scr->page_music_view_1, 1008, 271);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_view_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_view_1, GUI_CTRL_CONT);

	//Set style for page_music_view_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_music_view_1, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_music_view_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_music_img_1
	ui_scr->page_music_img_1 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_1, gui_get_res_path(GUI_RES_DIE_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_1, 0);
	lv_image_set_scale(ui_scr->page_music_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_1, 323, 22);
	lv_obj_set_size(ui_scr->page_music_img_1, 358, 358);
	lv_obj_add_flag(ui_scr->page_music_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_1, GUI_CTRL_IMG);

	//Write codes page_music_img_4
	ui_scr->page_music_img_4 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_4, gui_get_res_path(GUI_RES_IMAGE_4225207_3_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_4, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_4, 0);
	lv_image_set_scale(ui_scr->page_music_img_4, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_4, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_4, 269, 489);
	lv_obj_set_size(ui_scr->page_music_img_4, 77, 80);
	lv_obj_add_flag(ui_scr->page_music_img_4, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_4, GUI_CTRL_IMG);

	//Write codes page_music_img_2
	ui_scr->page_music_img_2 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_2, gui_get_res_path(GUI_RES_ZHEN_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_2, 0);
	lv_image_set_scale(ui_scr->page_music_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_2, 536, -26);
	lv_obj_set_size(ui_scr->page_music_img_2, 185, 356);
	lv_obj_add_flag(ui_scr->page_music_img_2, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_2, GUI_CTRL_IMG);

	//Write codes page_music_slider_1
	ui_scr->page_music_slider_1 = lv_slider_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_slider_1, 47, 463);
	lv_obj_set_size(ui_scr->page_music_slider_1, 933, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_slider_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_slider_1, GUI_CTRL_SLIDER);

	//Set style for page_music_slider_1. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_music_slider_1, 50, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for page_music_slider_1. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_music_slider_1, 50, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->page_music_slider_1, 0, 100);
	lv_slider_set_mode(ui_scr->page_music_slider_1, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->page_music_slider_1, 10, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->page_music_slider_1, 0, LV_ANIM_OFF);

	//Write codes page_music_img_3
	ui_scr->page_music_img_3 = lv_image_create(ui_scr->page_music);
	lv_image_set_pivot(ui_scr->page_music_img_3, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_3, 0);
	lv_image_set_scale(ui_scr->page_music_img_3, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_3, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_3, 400, 100);
	lv_obj_set_size(ui_scr->page_music_img_3, 200, 200);
	lv_obj_add_flag(ui_scr->page_music_img_3, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_scr->page_music_img_3, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->page_music_img_3, GUI_CTRL_IMG);

	//Set style for page_music_img_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_music_img_3, 200, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_clip_corner(ui_scr->page_music_img_3, true, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_music_lbl_1
	ui_scr->page_music_lbl_1 = lv_label_create(ui_scr->page_music);
	lv_label_set_text(ui_scr->page_music_lbl_1, "周杰伦-青花瓷");
	lv_label_set_long_mode(ui_scr->page_music_lbl_1, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_pos(ui_scr->page_music_lbl_1, 309, 381);
	lv_obj_set_size(ui_scr->page_music_lbl_1, 390, 60);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_lbl_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_lbl_1, GUI_CTRL_LABEL);

	//Set style for page_music_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_music_lbl_1, &lv_font_lyrics_38, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_music_lbl_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_music_img_5
	ui_scr->page_music_img_5 = lv_image_create(ui_scr->page_music);
	lv_image_set_src(ui_scr->page_music_img_5, gui_get_res_path(GUI_RES_IMAGE_4225207_2_PNG));
	lv_image_set_pivot(ui_scr->page_music_img_5, 0, 0);
	lv_image_set_rotation(ui_scr->page_music_img_5, 0);
	lv_image_set_scale(ui_scr->page_music_img_5, 256);
	lv_image_set_inner_align(ui_scr->page_music_img_5, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_music_img_5, 698, 489);
	lv_obj_set_size(ui_scr->page_music_img_5, 77, 80);
	lv_obj_add_flag(ui_scr->page_music_img_5, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_music_img_5, GUI_CTRL_IMG);

	//Write codes page_music_imgbtn_1
	ui_scr->page_music_imgbtn_1 = lv_imgbtn_create(ui_scr->page_music);
	lv_obj_set_pos(ui_scr->page_music_imgbtn_1, 477, 489);
	lv_obj_set_size(ui_scr->page_music_imgbtn_1, 77, 80);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_imgbtn_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_imgbtn_1, GUI_CTRL_IMGBTN);

	//Set style for page_music_imgbtn_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_music_imgbtn_1, &lv_font_MiSans_Medium_14, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for page_music_imgbtn_1. Part: LV_PART_MAIN, State: LV_STATE_PRESSED
	lv_obj_set_style_bg_opa(ui_scr->page_music_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_PRESSED);
	lv_obj_set_style_text_font(ui_scr->page_music_imgbtn_1, &lv_font_MiSans_Medium_14, LV_PART_MAIN|LV_STATE_PRESSED);

	//Set style for page_music_imgbtn_1. Part: LV_PART_MAIN, State: LV_STATE_CHECKED
	lv_obj_set_style_bg_opa(ui_scr->page_music_imgbtn_1, 0, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_text_font(ui_scr->page_music_imgbtn_1, &lv_font_MiSans_Medium_14, LV_PART_MAIN|LV_STATE_CHECKED);
	lv_obj_set_style_image_recolor(ui_scr->page_music_imgbtn_1, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_CHECKED);
	lv_imagebutton_set_src(ui_scr->page_music_imgbtn_1, LV_IMGBTN_STATE_RELEASED, NULL, gui_get_res_path(GUI_RES_IMAGE_4225207_1_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->page_music_imgbtn_1, LV_IMGBTN_STATE_PRESSED, NULL, gui_get_res_path(GUI_RES_IMAGE_4225207_1_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->page_music_imgbtn_1, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, gui_get_res_path(GUI_RES_IMAGE_4225207_0_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->page_music_imgbtn_1, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, gui_get_res_path(GUI_RES_IMAGE_4225207_0_PNG), NULL);
	lv_obj_add_flag(ui_scr->page_music_imgbtn_1, LV_OBJ_FLAG_CHECKABLE);
	ui_scr->page_music_imgbtn_1_label = lv_label_create(ui_scr->page_music_imgbtn_1);
	lv_label_set_text(ui_scr->page_music_imgbtn_1_label, "");
	lv_obj_set_style_pad_all(ui_scr->page_music_imgbtn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_music_imgbtn_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_music_lbl_3
	ui_scr->page_music_lbl_3 = lv_label_create(ui_scr->page_music);
	lv_label_set_text(ui_scr->page_music_lbl_3, "00:00");
	lv_label_set_long_mode(ui_scr->page_music_lbl_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_music_lbl_3, 52, 499);
	lv_obj_set_size(ui_scr->page_music_lbl_3, 165, 70);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_lbl_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_lbl_3, GUI_CTRL_LABEL);

	//Set style for page_music_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_music_lbl_3, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_music_lbl_4
	ui_scr->page_music_lbl_4 = lv_label_create(ui_scr->page_music);
	lv_label_set_text(ui_scr->page_music_lbl_4, "03:54");
	lv_label_set_long_mode(ui_scr->page_music_lbl_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_music_lbl_4, 859, 499);
	lv_obj_set_size(ui_scr->page_music_lbl_4, 131, 70);
	lv_obj_set_scrollbar_mode(ui_scr->page_music_lbl_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_music_lbl_4, GUI_CTRL_LABEL);

	//Set style for page_music_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_music_lbl_4, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_update_layout(ui_scr->page_music);
	ui_scr->page_music_del = false;

	//Init events for screen
	events_init_page_music(ui);
	return ui_scr->page_music;
}
void unload_scr_page_music(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
}
