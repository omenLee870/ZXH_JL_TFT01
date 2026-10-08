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
#include "../../custom/chongwuyibiao_donghua.h"
#include "../../custom/chongwuyibiao_yinyue.h"
#include "../../custom/neicun_zhuizong.h"


lv_obj_t * setup_scr_chongwuyibiao(lv_ui *ui)
{
	lv_ui_chongwuyibiao * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_CHONGWUYIBIAO);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_chongwuyibiao));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_chongwuyibiao));
		ui->chongwuyibiao = ui_scr;
	}

	//Write codes chongwuyibiao
	ui_scr->chongwuyibiao = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->chongwuyibiao, GUI_CTRL_SCR);

	//Write codes chongwuyibiao_caodirongqi
	ui_scr->chongwuyibiao_caodirongqi = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_caodirongqi, 152, 300);
	lv_obj_set_size(ui_scr->chongwuyibiao_caodirongqi, 720, 140);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_caodirongqi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_caodirongqi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_caodirongqi
	lv_obj_add_flag(ui_scr->chongwuyibiao_caodirongqi, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_caodirongqi, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_caodirongqi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_caodirongqi, 70, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_caodirongqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_caodi
	ui_scr->chongwuyibiao_caodi = lv_image_create(ui_scr->chongwuyibiao_caodirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_caodi, gui_get_res_path(GUI_RES_STAGE_DAY_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_caodi, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_caodi, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_caodi, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_caodi, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_caodi, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_caodi, 720, 140);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_caodi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_caodi
	lv_obj_add_flag(ui_scr->chongwuyibiao_caodi, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_caodi, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_yun4
	ui_scr->chongwuyibiao_yun4 = lv_image_create(ui_scr->chongwuyibiao_caodirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yun4, gui_get_res_path(GUI_RES_CLOUD_4_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yun4, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yun4, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_yun4, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yun4, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yun4, 486, 23);
	lv_obj_set_size(ui_scr->chongwuyibiao_yun4, 79, 42);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yun4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_yun4
	lv_obj_add_flag(ui_scr->chongwuyibiao_yun4, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_yun4, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_yun2
	ui_scr->chongwuyibiao_yun2 = lv_image_create(ui_scr->chongwuyibiao_caodirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yun2, gui_get_res_path(GUI_RES_CLOUD_2_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yun2, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yun2, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_yun2, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yun2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yun2, 235, 42);
	lv_obj_set_size(ui_scr->chongwuyibiao_yun2, 40, 24);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yun2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_yun2
	lv_obj_add_flag(ui_scr->chongwuyibiao_yun2, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_yun2, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_yun1
	ui_scr->chongwuyibiao_yun1 = lv_image_create(ui_scr->chongwuyibiao_caodirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yun1, gui_get_res_path(GUI_RES_CLOUD_1_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yun1, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yun1, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_yun1, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yun1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yun1, 77, 14);
	lv_obj_set_size(ui_scr->chongwuyibiao_yun1, 86, 43);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yun1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_yun1
	lv_obj_add_flag(ui_scr->chongwuyibiao_yun1, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_yun1, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_yun3
	ui_scr->chongwuyibiao_yun3 = lv_image_create(ui_scr->chongwuyibiao_caodirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yun3, gui_get_res_path(GUI_RES_CLOUD_3_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yun3, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yun3, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_yun3, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yun3, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yun3, 370, 42);
	lv_obj_set_size(ui_scr->chongwuyibiao_yun3, 41, 24);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yun3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_yun3
	lv_obj_add_flag(ui_scr->chongwuyibiao_yun3, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_yun3, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_xiaogou
	ui_scr->chongwuyibiao_xiaogou = lv_image_create(ui_scr->chongwuyibiao);
	lv_image_set_src(ui_scr->chongwuyibiao_xiaogou, gui_get_res_path(GUI_RES_DOG_OPEN_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_xiaogou, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_xiaogou, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_xiaogou, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_xiaogou, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xiaogou, 397, 200);
	lv_obj_set_size(ui_scr->chongwuyibiao_xiaogou, 230, 230);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xiaogou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_xiaogou
	lv_obj_add_flag(ui_scr->chongwuyibiao_xiaogou, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_xiaogou, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_dianliangtiao
	ui_scr->chongwuyibiao_dianliangtiao = lv_bar_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_dianliangtiao, 160, 120);
	lv_obj_set_size(ui_scr->chongwuyibiao_dianliangtiao, 180, 16);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_dianliangtiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_dianliangtiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_dianliangtiao, GUI_CTRL_BAR);

	//Set style for chongwuyibiao_dianliangtiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_dianliangtiao, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_dianliangtiao, lv_color_make(0x17, 0x33, 0x29), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_dianliangtiao, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->chongwuyibiao_dianliangtiao, lv_color_make(0x17, 0x33, 0x29), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->chongwuyibiao_dianliangtiao, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for chongwuyibiao_dianliangtiao. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_dianliangtiao, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_dianliangtiao, lv_color_make(0x54, 0xf0, 0x5a), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_anim_duration(ui_scr->chongwuyibiao_dianliangtiao, 1000, 0);
	lv_bar_set_mode(ui_scr->chongwuyibiao_dianliangtiao, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui_scr->chongwuyibiao_dianliangtiao, 0, 100);
	lv_bar_set_value(ui_scr->chongwuyibiao_dianliangtiao, 68, LV_ANIM_OFF);

	//Write codes chongwuyibiao_zuocedengtiao
	ui_scr->chongwuyibiao_zuocedengtiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuocedengtiao, 100, 190);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuocedengtiao, 12, 220);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuocedengtiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuocedengtiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuocedengtiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_zuocedengtiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_zuocedengtiao, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_zuocedengtiao, lv_color_make(0x36, 0xa9, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui_scr->chongwuyibiao_zuocedengtiao, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->chongwuyibiao_zuocedengtiao, lv_color_make(0x36, 0xa9, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->chongwuyibiao_zuocedengtiao, 102, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui_scr->chongwuyibiao_zuocedengtiao, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_youcedengtiao
	ui_scr->chongwuyibiao_youcedengtiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youcedengtiao, 912, 190);
	lv_obj_set_size(ui_scr->chongwuyibiao_youcedengtiao, 12, 220);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youcedengtiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youcedengtiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youcedengtiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_youcedengtiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_youcedengtiao, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_youcedengtiao, lv_color_make(0x36, 0xa9, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui_scr->chongwuyibiao_youcedengtiao, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->chongwuyibiao_youcedengtiao, lv_color_make(0x36, 0xa9, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->chongwuyibiao_youcedengtiao, 102, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(ui_scr->chongwuyibiao_youcedengtiao, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_dianchirongqi
	ui_scr->chongwuyibiao_dianchirongqi = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_dianchirongqi, 350, 116);
	lv_obj_set_size(ui_scr->chongwuyibiao_dianchirongqi, 50, 24);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_dianchirongqi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_dianchirongqi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_dianchirongqi, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_dianchirongqi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_dianchirongqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_dianchizhuti
	ui_scr->chongwuyibiao_dianchizhuti = lv_obj_create(ui_scr->chongwuyibiao_dianchirongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_dianchizhuti, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_dianchizhuti, 44, 24);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_dianchizhuti, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_dianchizhuti, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_dianchizhuti, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_dianchizhuti. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_dianchizhuti, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_dianchizhuti, lv_color_make(0x54, 0xf0, 0x5a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_dianliangshuzi
	ui_scr->chongwuyibiao_dianliangshuzi = lv_label_create(ui_scr->chongwuyibiao_dianchizhuti);
	lv_label_set_text(ui_scr->chongwuyibiao_dianliangshuzi, "68");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_dianliangshuzi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_dianliangshuzi, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_dianliangshuzi, 44, 24);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_dianliangshuzi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_dianliangshuzi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_dianliangshuzi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_dianliangshuzi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_dianliangshuzi, lv_color_make(0x10, 0x18, 0x20), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_dianliangshuzi, &lv_font_Lato_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_dianliangshuzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->chongwuyibiao_dianliangshuzi, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_dianliangshuzi, 2, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_dianchizhengji
	ui_scr->chongwuyibiao_dianchizhengji = lv_obj_create(ui_scr->chongwuyibiao_dianchirongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_dianchizhengji, 44, 7);
	lv_obj_set_size(ui_scr->chongwuyibiao_dianchizhengji, 5, 10);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_dianchizhengji, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_dianchizhengji, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_dianchizhengji, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_dianchizhengji. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_dianchizhengji, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_dianchizhengji, lv_color_make(0x54, 0xf0, 0x5a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_chesushuzi
	ui_scr->chongwuyibiao_chesushuzi = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_chesushuzi, "46");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_chesushuzi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_chesushuzi, 150, 206);
	lv_obj_set_size(ui_scr->chongwuyibiao_chesushuzi, 150, 90);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_chesushuzi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_chesushuzi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_chesushuzi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_chesushuzi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_chesushuzi, &lv_font_Lato_Regular_88, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_chesushuzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->chongwuyibiao_chesushuzi, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_chesushuzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_chesudanwei
	ui_scr->chongwuyibiao_chesudanwei = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_chesudanwei, "KM/H");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_chesudanwei, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_chesudanwei, 308, 253);
	lv_obj_set_size(ui_scr->chongwuyibiao_chesudanwei, 72, 26);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_chesudanwei, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_chesudanwei, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_chesudanwei, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_chesudanwei. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_chesudanwei, lv_color_make(0x9f, 0xb0, 0xb8), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_chesudanwei, &lv_font_Lato_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_chesudanwei, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_chesudanwei, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_shijian
	ui_scr->chongwuyibiao_shijian = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_shijian, "01:19");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_shijian, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shijian, 715, 213);
	lv_obj_set_size(ui_scr->chongwuyibiao_shijian, 150, 50);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shijian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shijian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shijian, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_shijian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_shijian, &lv_font_Lato_Regular_44, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_shijian, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_shijian, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_wendu
	ui_scr->chongwuyibiao_wendu = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_wendu, "25°C");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_wendu, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_wendu, 650, 253);
	lv_obj_set_size(ui_scr->chongwuyibiao_wendu, 50, 26);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_wendu, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_wendu, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_wendu, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_wendu. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_wendu, lv_color_make(0xaa, 0xb8, 0xbf), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_wendu, &lv_font_Lato_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_wendu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->chongwuyibiao_wendu, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_wendu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_riqixingqi
	ui_scr->chongwuyibiao_riqixingqi = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_riqixingqi, "05-08 周六");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_riqixingqi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_riqixingqi, 715, 253);
	lv_obj_set_size(ui_scr->chongwuyibiao_riqixingqi, 150, 28);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_riqixingqi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_riqixingqi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_riqixingqi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_riqixingqi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_riqixingqi, lv_color_make(0xaa, 0xb8, 0xbf), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_riqixingqi, &lv_font_NotoSansSC_Medium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_riqixingqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_riqixingqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_lichengbiaoti
	ui_scr->chongwuyibiao_lichengbiaoti = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_lichengbiaoti, "ODO");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_lichengbiaoti, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_lichengbiaoti, 155, 466);
	lv_obj_set_size(ui_scr->chongwuyibiao_lichengbiaoti, 60, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_lichengbiaoti, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_lichengbiaoti, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_lichengbiaoti, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_lichengbiaoti. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_lichengbiaoti, lv_color_make(0xaa, 0xb8, 0xbf), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_lichengbiaoti, &lv_font_Lato_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_lichengbiaoti, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_lichengbiaoti, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_lichengshuzi
	ui_scr->chongwuyibiao_lichengshuzi = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_lichengshuzi, "77.0");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_lichengshuzi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_lichengshuzi, 223, 466);
	lv_obj_set_size(ui_scr->chongwuyibiao_lichengshuzi, 95, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_lichengshuzi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_lichengshuzi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_lichengshuzi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_lichengshuzi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_lichengshuzi, &lv_font_Lato_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_lichengshuzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->chongwuyibiao_lichengshuzi, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_lichengshuzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_lichengdanwei
	ui_scr->chongwuyibiao_lichengdanwei = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_lichengdanwei, "KM");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_lichengdanwei, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_lichengdanwei, 326, 466);
	lv_obj_set_size(ui_scr->chongwuyibiao_lichengdanwei, 40, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_lichengdanwei, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_lichengdanwei, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_lichengdanwei, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_lichengdanwei. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_lichengdanwei, lv_color_make(0xaa, 0xb8, 0xbf), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_lichengdanwei, &lv_font_Lato_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_lichengdanwei, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_lichengdanwei, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_jiashimoshi
	ui_scr->chongwuyibiao_jiashimoshi = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_jiashimoshi, "NORMAL");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_jiashimoshi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jiashimoshi, 715, 466);
	lv_obj_set_size(ui_scr->chongwuyibiao_jiashimoshi, 110, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jiashimoshi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jiashimoshi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_jiashimoshi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_jiashimoshi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_jiashimoshi, lv_color_make(0x36, 0xa9, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_jiashimoshi, &lv_font_Lato_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_jiashimoshi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_jiashimoshi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_guzhangxinxi
	ui_scr->chongwuyibiao_guzhangxinxi = lv_label_create(ui_scr->chongwuyibiao);
	lv_label_set_text(ui_scr->chongwuyibiao_guzhangxinxi, "ERROR 00");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_guzhangxinxi, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_guzhangxinxi, 390, 466);
	lv_obj_set_size(ui_scr->chongwuyibiao_guzhangxinxi, 170, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_guzhangxinxi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_guzhangxinxi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_guzhangxinxi
	lv_obj_add_flag(ui_scr->chongwuyibiao_guzhangxinxi, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_guzhangxinxi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_guzhangxinxi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_guzhangxinxi, lv_color_make(0xff, 0x3b, 0x30), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_guzhangxinxi, &lv_font_Lato_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_guzhangxinxi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_guzhangxinxi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_tianqitubiao
	ui_scr->chongwuyibiao_tianqitubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_tianqitubiao, 657, 207);
	lv_obj_set_size(ui_scr->chongwuyibiao_tianqitubiao, 36, 36);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_tianqitubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_tianqitubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_tianqitubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_tianqitubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_tianqitubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangzhongxin
	ui_scr->chongwuyibiao_taiyangzhongxin = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangzhongxin, 11, 11);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangzhongxin, 14, 14);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangzhongxin, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangzhongxin, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangzhongxin, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangzhongxin. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangzhongxin, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangzhongxin, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_1
	ui_scr->chongwuyibiao_taiyangguangmang_1 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_1, 17, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_1, 2, 7);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_1, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_1, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_2
	ui_scr->chongwuyibiao_taiyangguangmang_2 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_2, 17, 29);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_2, 2, 7);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_2, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_2, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_3
	ui_scr->chongwuyibiao_taiyangguangmang_3 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_3, 0, 17);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_3, 7, 2);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_3, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_3, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_4
	ui_scr->chongwuyibiao_taiyangguangmang_4 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_4, 29, 17);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_4, 7, 2);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_4, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_4, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_4, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_5
	ui_scr->chongwuyibiao_taiyangguangmang_5 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_5, 5, 5);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_5, 4, 4);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_5, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_5, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_5, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_6
	ui_scr->chongwuyibiao_taiyangguangmang_6 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_6, 27, 5);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_6, 4, 4);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_6, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_6, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_6, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_6. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_6, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_7
	ui_scr->chongwuyibiao_taiyangguangmang_7 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_7, 5, 27);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_7, 4, 4);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_7, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_7, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_7, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_7. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_7, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_7, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_taiyangguangmang_8
	ui_scr->chongwuyibiao_taiyangguangmang_8 = lv_obj_create(ui_scr->chongwuyibiao_tianqitubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_taiyangguangmang_8, 27, 27);
	lv_obj_set_size(ui_scr->chongwuyibiao_taiyangguangmang_8, 4, 4);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_taiyangguangmang_8, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_taiyangguangmang_8, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_taiyangguangmang_8, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_taiyangguangmang_8. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_taiyangguangmang_8, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_taiyangguangmang_8, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_zuozhuanxiang
	ui_scr->chongwuyibiao_zuozhuanxiang = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuozhuanxiang, 152, 66);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuozhuanxiang, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuozhuanxiang, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuozhuanxiang, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuozhuanxiang, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_zuozhuanxiang. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_zuozhuanxiang, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_zuozhuanxiangguangyunhengxian
	ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian = lv_line_create(ui_scr->chongwuyibiao_zuozhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_zuozhuanxiangguangyunhengxian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, lv_color_make(0x12, 0x33, 0x14), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_zuozhuanxiangguangyunhengxian[] ={{44, 15},{4, 15},};
	lv_line_set_points(ui_scr->chongwuyibiao_zuozhuanxiangguangyunhengxian,chongwuyibiao_zuozhuanxiangguangyunhengxian,2);

	//Write codes chongwuyibiao_zuozhuanxiangguangyunjiantou
	ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou = lv_line_create(ui_scr->chongwuyibiao_zuozhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_zuozhuanxiangguangyunjiantou. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, lv_color_make(0x12, 0x33, 0x14), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_zuozhuanxiangguangyunjiantou[] ={{17, 3},{4, 15},{17, 27},};
	lv_line_set_points(ui_scr->chongwuyibiao_zuozhuanxiangguangyunjiantou,chongwuyibiao_zuozhuanxiangguangyunjiantou,3);

	//Write codes chongwuyibiao_zuozhuanxianghengxian
	ui_scr->chongwuyibiao_zuozhuanxianghengxian = lv_line_create(ui_scr->chongwuyibiao_zuozhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuozhuanxianghengxian, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuozhuanxianghengxian, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuozhuanxianghengxian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuozhuanxianghengxian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuozhuanxianghengxian, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_zuozhuanxianghengxian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_zuozhuanxianghengxian, lv_color_make(0x58, 0xff, 0x63), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_zuozhuanxianghengxian, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_zuozhuanxianghengxian[] ={{44, 15},{4, 15},};
	lv_line_set_points(ui_scr->chongwuyibiao_zuozhuanxianghengxian,chongwuyibiao_zuozhuanxianghengxian,2);

	//Write codes chongwuyibiao_zuozhuanxiangjiantou
	ui_scr->chongwuyibiao_zuozhuanxiangjiantou = lv_line_create(ui_scr->chongwuyibiao_zuozhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_zuozhuanxiangjiantou. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, lv_color_make(0x58, 0xff, 0x63), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_zuozhuanxiangjiantou, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_zuozhuanxiangjiantou[] ={{17, 3},{4, 15},{17, 27},};
	lv_line_set_points(ui_scr->chongwuyibiao_zuozhuanxiangjiantou,chongwuyibiao_zuozhuanxiangjiantou,3);

	//Write codes chongwuyibiao_youzhuanxiang
	ui_scr->chongwuyibiao_youzhuanxiang = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youzhuanxiang, 824, 66);
	lv_obj_set_size(ui_scr->chongwuyibiao_youzhuanxiang, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youzhuanxiang, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youzhuanxiang, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youzhuanxiang, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_youzhuanxiang. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_youzhuanxiang, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_youzhuanxiangguangyunhengxian
	ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian = lv_line_create(ui_scr->chongwuyibiao_youzhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_youzhuanxiangguangyunhengxian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, lv_color_make(0x12, 0x33, 0x14), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_youzhuanxiangguangyunhengxian[] ={{4, 15},{44, 15},};
	lv_line_set_points(ui_scr->chongwuyibiao_youzhuanxiangguangyunhengxian,chongwuyibiao_youzhuanxiangguangyunhengxian,2);

	//Write codes chongwuyibiao_youzhuanxiangguangyunjiantou
	ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou = lv_line_create(ui_scr->chongwuyibiao_youzhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_youzhuanxiangguangyunjiantou. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, lv_color_make(0x12, 0x33, 0x14), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_youzhuanxiangguangyunjiantou[] ={{31, 3},{44, 15},{31, 27},};
	lv_line_set_points(ui_scr->chongwuyibiao_youzhuanxiangguangyunjiantou,chongwuyibiao_youzhuanxiangguangyunjiantou,3);

	//Write codes chongwuyibiao_youzhuanxianghengxian
	ui_scr->chongwuyibiao_youzhuanxianghengxian = lv_line_create(ui_scr->chongwuyibiao_youzhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youzhuanxianghengxian, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_youzhuanxianghengxian, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youzhuanxianghengxian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youzhuanxianghengxian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youzhuanxianghengxian, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_youzhuanxianghengxian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_youzhuanxianghengxian, lv_color_make(0x58, 0xff, 0x63), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_youzhuanxianghengxian, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_youzhuanxianghengxian[] ={{4, 15},{44, 15},};
	lv_line_set_points(ui_scr->chongwuyibiao_youzhuanxianghengxian,chongwuyibiao_youzhuanxianghengxian,2);

	//Write codes chongwuyibiao_youzhuanxiangjiantou
	ui_scr->chongwuyibiao_youzhuanxiangjiantou = lv_line_create(ui_scr->chongwuyibiao_youzhuanxiang);
	lv_obj_set_pos(ui_scr->chongwuyibiao_youzhuanxiangjiantou, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_youzhuanxiangjiantou, 48, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_youzhuanxiangjiantou, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_youzhuanxiangjiantou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_youzhuanxiangjiantou, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_youzhuanxiangjiantou. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_youzhuanxiangjiantou, lv_color_make(0x58, 0xff, 0x63), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_line_width(ui_scr->chongwuyibiao_youzhuanxiangjiantou, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_youzhuanxiangjiantou[] ={{31, 3},{44, 15},{31, 27},};
	lv_line_set_points(ui_scr->chongwuyibiao_youzhuanxiangjiantou,chongwuyibiao_youzhuanxiangjiantou,3);

	//Write codes chongwuyibiao_xinhaozhu_1
	ui_scr->chongwuyibiao_xinhaozhu_1 = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xinhaozhu_1, 820, 130);
	lv_obj_set_size(ui_scr->chongwuyibiao_xinhaozhu_1, 5, 8);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xinhaozhu_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xinhaozhu_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xinhaozhu_1, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_xinhaozhu_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_xinhaozhu_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_xinhaozhu_1, lv_color_make(0x30, 0x40, 0x4a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_xinhaozhu_2
	ui_scr->chongwuyibiao_xinhaozhu_2 = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xinhaozhu_2, 828, 126);
	lv_obj_set_size(ui_scr->chongwuyibiao_xinhaozhu_2, 5, 12);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xinhaozhu_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xinhaozhu_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xinhaozhu_2, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_xinhaozhu_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_xinhaozhu_2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_xinhaozhu_2, lv_color_make(0x30, 0x40, 0x4a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_xinhaozhu_3
	ui_scr->chongwuyibiao_xinhaozhu_3 = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xinhaozhu_3, 836, 122);
	lv_obj_set_size(ui_scr->chongwuyibiao_xinhaozhu_3, 5, 16);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xinhaozhu_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xinhaozhu_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xinhaozhu_3, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_xinhaozhu_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_xinhaozhu_3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_xinhaozhu_3, lv_color_make(0x30, 0x40, 0x4a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_xinhaozhu_4
	ui_scr->chongwuyibiao_xinhaozhu_4 = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xinhaozhu_4, 844, 118);
	lv_obj_set_size(ui_scr->chongwuyibiao_xinhaozhu_4, 5, 20);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xinhaozhu_4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xinhaozhu_4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xinhaozhu_4, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_xinhaozhu_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_xinhaozhu_4, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_xinhaozhu_4, lv_color_make(0x30, 0x40, 0x4a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_shachetubiao
	ui_scr->chongwuyibiao_shachetubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shachetubiao, 424, 112);
	lv_obj_set_size(ui_scr->chongwuyibiao_shachetubiao, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shachetubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shachetubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_shachetubiao
	lv_obj_add_flag(ui_scr->chongwuyibiao_shachetubiao, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_shachetubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_shachetubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_shachetubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_shacheneiquan
	ui_scr->chongwuyibiao_shacheneiquan = lv_arc_create(ui_scr->chongwuyibiao_shachetubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shacheneiquan, 8, 5);
	lv_obj_set_size(ui_scr->chongwuyibiao_shacheneiquan, 22, 22);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shacheneiquan, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shacheneiquan, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shacheneiquan, GUI_CTRL_ARC);
	lv_arc_set_mode(ui_scr->chongwuyibiao_shacheneiquan, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui_scr->chongwuyibiao_shacheneiquan, 0, 100);
	lv_arc_set_bg_angles(ui_scr->chongwuyibiao_shacheneiquan, 0, 360);
	lv_arc_set_value(ui_scr->chongwuyibiao_shacheneiquan, 0);
	lv_arc_set_rotation(ui_scr->chongwuyibiao_shacheneiquan, 0);

	//Write codes chongwuyibiao_shachezuohuan
	ui_scr->chongwuyibiao_shachezuohuan = lv_arc_create(ui_scr->chongwuyibiao_shachetubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shachezuohuan, 2, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shachezuohuan, 32, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shachezuohuan, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shachezuohuan, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shachezuohuan, GUI_CTRL_ARC);
	lv_arc_set_mode(ui_scr->chongwuyibiao_shachezuohuan, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui_scr->chongwuyibiao_shachezuohuan, 0, 100);
	lv_arc_set_bg_angles(ui_scr->chongwuyibiao_shachezuohuan, 130, 230);
	lv_arc_set_value(ui_scr->chongwuyibiao_shachezuohuan, 0);
	lv_arc_set_rotation(ui_scr->chongwuyibiao_shachezuohuan, 0);

	//Write codes chongwuyibiao_shacheyouhuan
	ui_scr->chongwuyibiao_shacheyouhuan = lv_arc_create(ui_scr->chongwuyibiao_shachetubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shacheyouhuan, 4, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shacheyouhuan, 32, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shacheyouhuan, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shacheyouhuan, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shacheyouhuan, GUI_CTRL_ARC);
	lv_arc_set_mode(ui_scr->chongwuyibiao_shacheyouhuan, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui_scr->chongwuyibiao_shacheyouhuan, 0, 100);
	lv_arc_set_bg_angles(ui_scr->chongwuyibiao_shacheyouhuan, 310, 50);
	lv_arc_set_value(ui_scr->chongwuyibiao_shacheyouhuan, 0);
	lv_arc_set_rotation(ui_scr->chongwuyibiao_shacheyouhuan, 0);

	//Write codes chongwuyibiao_shachegantanhao
	ui_scr->chongwuyibiao_shachegantanhao = lv_label_create(ui_scr->chongwuyibiao_shachetubiao);
	lv_label_set_text(ui_scr->chongwuyibiao_shachegantanhao, "!");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_shachegantanhao, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shachegantanhao, 0, 6);
	lv_obj_set_size(ui_scr->chongwuyibiao_shachegantanhao, 38, 22);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shachegantanhao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shachegantanhao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shachegantanhao, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_shachegantanhao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->chongwuyibiao_shachegantanhao, lv_color_make(0xff, 0xc9, 0x28), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_shachegantanhao, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_shachegantanhao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->chongwuyibiao_shachegantanhao, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_shachegantanhao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_jinguangdengtubiao
	ui_scr->chongwuyibiao_jinguangdengtubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jinguangdengtubiao, 480, 112);
	lv_obj_set_size(ui_scr->chongwuyibiao_jinguangdengtubiao, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jinguangdengtubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jinguangdengtubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_jinguangdengtubiao
	lv_obj_add_flag(ui_scr->chongwuyibiao_jinguangdengtubiao, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_jinguangdengtubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_jinguangdengtubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_jinguangdengtubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_jinguangdenglunkuo
	ui_scr->chongwuyibiao_jinguangdenglunkuo = lv_line_create(ui_scr->chongwuyibiao_jinguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jinguangdenglunkuo, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_jinguangdenglunkuo, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jinguangdenglunkuo, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jinguangdenglunkuo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_jinguangdenglunkuo, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_jinguangdenglunkuo[] ={{19, 7},{26, 7},{31, 11},{33, 16},{31, 21},{26, 25},{19, 25},{19, 7},};
	lv_line_set_points(ui_scr->chongwuyibiao_jinguangdenglunkuo,chongwuyibiao_jinguangdenglunkuo,8);

	//Write codes chongwuyibiao_jinguangdengguangxian_1
	ui_scr->chongwuyibiao_jinguangdengguangxian_1 = lv_line_create(ui_scr->chongwuyibiao_jinguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jinguangdengguangxian_1, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_jinguangdengguangxian_1, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jinguangdengguangxian_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jinguangdengguangxian_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_jinguangdengguangxian_1, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_jinguangdengguangxian_1[] ={{3, 14},{13, 9},};
	lv_line_set_points(ui_scr->chongwuyibiao_jinguangdengguangxian_1,chongwuyibiao_jinguangdengguangxian_1,2);

	//Write codes chongwuyibiao_jinguangdengguangxian_2
	ui_scr->chongwuyibiao_jinguangdengguangxian_2 = lv_line_create(ui_scr->chongwuyibiao_jinguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jinguangdengguangxian_2, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_jinguangdengguangxian_2, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jinguangdengguangxian_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jinguangdengguangxian_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_jinguangdengguangxian_2, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_jinguangdengguangxian_2[] ={{3, 21},{13, 16},};
	lv_line_set_points(ui_scr->chongwuyibiao_jinguangdengguangxian_2,chongwuyibiao_jinguangdengguangxian_2,2);

	//Write codes chongwuyibiao_jinguangdengguangxian_3
	ui_scr->chongwuyibiao_jinguangdengguangxian_3 = lv_line_create(ui_scr->chongwuyibiao_jinguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_jinguangdengguangxian_3, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_jinguangdengguangxian_3, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_jinguangdengguangxian_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_jinguangdengguangxian_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_jinguangdengguangxian_3, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_jinguangdengguangxian_3[] ={{3, 28},{13, 23},};
	lv_line_set_points(ui_scr->chongwuyibiao_jinguangdengguangxian_3,chongwuyibiao_jinguangdengguangxian_3,2);

	//Write codes chongwuyibiao_yuanguangdengtubiao
	ui_scr->chongwuyibiao_yuanguangdengtubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yuanguangdengtubiao, 592, 112);
	lv_obj_set_size(ui_scr->chongwuyibiao_yuanguangdengtubiao, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yuanguangdengtubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yuanguangdengtubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_yuanguangdengtubiao
	lv_obj_add_flag(ui_scr->chongwuyibiao_yuanguangdengtubiao, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_yuanguangdengtubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_yuanguangdengtubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_yuanguangdengtubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_yuanguangdenglunkuo
	ui_scr->chongwuyibiao_yuanguangdenglunkuo = lv_line_create(ui_scr->chongwuyibiao_yuanguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yuanguangdenglunkuo, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_yuanguangdenglunkuo, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yuanguangdenglunkuo, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yuanguangdenglunkuo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yuanguangdenglunkuo, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_yuanguangdenglunkuo. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_yuanguangdenglunkuo, lv_color_make(0x4d, 0xa3, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_yuanguangdenglunkuo[] ={{19, 7},{26, 7},{31, 11},{33, 16},{31, 21},{26, 25},{19, 25},{19, 7},};
	lv_line_set_points(ui_scr->chongwuyibiao_yuanguangdenglunkuo,chongwuyibiao_yuanguangdenglunkuo,8);

	//Write codes chongwuyibiao_yuanguangdengguangxian_1
	ui_scr->chongwuyibiao_yuanguangdengguangxian_1 = lv_line_create(ui_scr->chongwuyibiao_yuanguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_yuanguangdengguangxian_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_yuanguangdengguangxian_1, lv_color_make(0x4d, 0xa3, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_yuanguangdengguangxian_1[] ={{3, 9},{13, 9},};
	lv_line_set_points(ui_scr->chongwuyibiao_yuanguangdengguangxian_1,chongwuyibiao_yuanguangdengguangxian_1,2);

	//Write codes chongwuyibiao_yuanguangdengguangxian_2
	ui_scr->chongwuyibiao_yuanguangdengguangxian_2 = lv_line_create(ui_scr->chongwuyibiao_yuanguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_yuanguangdengguangxian_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_yuanguangdengguangxian_2, lv_color_make(0x4d, 0xa3, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_yuanguangdengguangxian_2[] ={{3, 16},{13, 16},};
	lv_line_set_points(ui_scr->chongwuyibiao_yuanguangdengguangxian_2,chongwuyibiao_yuanguangdengguangxian_2,2);

	//Write codes chongwuyibiao_yuanguangdengguangxian_3
	ui_scr->chongwuyibiao_yuanguangdengguangxian_3 = lv_line_create(ui_scr->chongwuyibiao_yuanguangdengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, GUI_CTRL_LINE);

	//Set style for chongwuyibiao_yuanguangdengguangxian_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_line_color(ui_scr->chongwuyibiao_yuanguangdengguangxian_3, lv_color_make(0x4d, 0xa3, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	static const lv_point_precise_t chongwuyibiao_yuanguangdengguangxian_3[] ={{3, 23},{13, 23},};
	lv_line_set_points(ui_scr->chongwuyibiao_yuanguangdengguangxian_3,chongwuyibiao_yuanguangdengguangxian_3,2);

	//Write codes chongwuyibiao_shikuodengtubiao
	ui_scr->chongwuyibiao_shikuodengtubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengtubiao, 536, 112);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengtubiao, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengtubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengtubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_shikuodengtubiao
	lv_obj_add_flag(ui_scr->chongwuyibiao_shikuodengtubiao, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengtubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_shikuodengtubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_shikuodengtubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_shikuodengxian_1
	ui_scr->chongwuyibiao_shikuodengxian_1 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_1, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_1, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_1, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_1[] ={{12, 8},{9, 11},{9, 21},{12, 24},{12, 8},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_1,chongwuyibiao_shikuodengxian_1,5);

	//Write codes chongwuyibiao_shikuodengxian_2
	ui_scr->chongwuyibiao_shikuodengxian_2 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_2, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_2, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_2, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_2[] ={{26, 8},{29, 11},{29, 21},{26, 24},{26, 8},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_2,chongwuyibiao_shikuodengxian_2,5);

	//Write codes chongwuyibiao_shikuodengxian_3
	ui_scr->chongwuyibiao_shikuodengxian_3 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_3, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_3, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_3, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_3[] ={{2, 8},{5, 10},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_3,chongwuyibiao_shikuodengxian_3,2);

	//Write codes chongwuyibiao_shikuodengxian_4
	ui_scr->chongwuyibiao_shikuodengxian_4 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_4, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_4, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_4, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_4[] ={{2, 16},{5, 16},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_4,chongwuyibiao_shikuodengxian_4,2);

	//Write codes chongwuyibiao_shikuodengxian_5
	ui_scr->chongwuyibiao_shikuodengxian_5 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_5, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_5, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_5, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_5, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_5[] ={{2, 24},{5, 22},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_5,chongwuyibiao_shikuodengxian_5,2);

	//Write codes chongwuyibiao_shikuodengxian_6
	ui_scr->chongwuyibiao_shikuodengxian_6 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_6, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_6, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_6, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_6, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_6, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_6[] ={{33, 10},{36, 8},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_6,chongwuyibiao_shikuodengxian_6,2);

	//Write codes chongwuyibiao_shikuodengxian_7
	ui_scr->chongwuyibiao_shikuodengxian_7 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_7, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_7, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_7, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_7, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_7, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_7[] ={{33, 16},{36, 16},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_7,chongwuyibiao_shikuodengxian_7,2);

	//Write codes chongwuyibiao_shikuodengxian_8
	ui_scr->chongwuyibiao_shikuodengxian_8 = lv_line_create(ui_scr->chongwuyibiao_shikuodengtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shikuodengxian_8, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_shikuodengxian_8, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shikuodengxian_8, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shikuodengxian_8, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shikuodengxian_8, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_shikuodengxian_8[] ={{33, 22},{36, 24},};
	lv_line_set_points(ui_scr->chongwuyibiao_shikuodengxian_8,chongwuyibiao_shikuodengxian_8,2);

	//Write codes chongwuyibiao_xunhangtubiao
	ui_scr->chongwuyibiao_xunhangtubiao = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangtubiao, 648, 112);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangtubiao, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangtubiao, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangtubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_xunhangtubiao
	lv_obj_add_flag(ui_scr->chongwuyibiao_xunhangtubiao, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_xunhangtubiao, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_xunhangtubiao. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_xunhangtubiao, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_xunhangyuanhu
	ui_scr->chongwuyibiao_xunhangyuanhu = lv_arc_create(ui_scr->chongwuyibiao_xunhangtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangyuanhu, 4, 1);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangyuanhu, 30, 30);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangyuanhu, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangyuanhu, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xunhangyuanhu, GUI_CTRL_ARC);

	//Set style for chongwuyibiao_xunhangyuanhu. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_arc_color(ui_scr->chongwuyibiao_xunhangyuanhu, lv_color_make(0x54, 0xf0, 0x5a), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_arc_set_mode(ui_scr->chongwuyibiao_xunhangyuanhu, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui_scr->chongwuyibiao_xunhangyuanhu, 0, 100);
	lv_arc_set_bg_angles(ui_scr->chongwuyibiao_xunhangyuanhu, 155, 25);
	lv_arc_set_value(ui_scr->chongwuyibiao_xunhangyuanhu, 0);
	lv_arc_set_rotation(ui_scr->chongwuyibiao_xunhangyuanhu, 0);

	//Write codes chongwuyibiao_xunhangzhizhen
	ui_scr->chongwuyibiao_xunhangzhizhen = lv_line_create(ui_scr->chongwuyibiao_xunhangtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangzhizhen, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangzhizhen, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangzhizhen, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangzhizhen, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xunhangzhizhen, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_xunhangzhizhen[] ={{19, 24},{27, 12},};
	lv_line_set_points(ui_scr->chongwuyibiao_xunhangzhizhen,chongwuyibiao_xunhangzhizhen,2);

	//Write codes chongwuyibiao_xunhangkedu_1
	ui_scr->chongwuyibiao_xunhangkedu_1 = lv_line_create(ui_scr->chongwuyibiao_xunhangtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangkedu_1, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangkedu_1, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangkedu_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangkedu_1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xunhangkedu_1, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_xunhangkedu_1[] ={{8, 19},{11, 21},};
	lv_line_set_points(ui_scr->chongwuyibiao_xunhangkedu_1,chongwuyibiao_xunhangkedu_1,2);

	//Write codes chongwuyibiao_xunhangkedu_2
	ui_scr->chongwuyibiao_xunhangkedu_2 = lv_line_create(ui_scr->chongwuyibiao_xunhangtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangkedu_2, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangkedu_2, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangkedu_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangkedu_2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xunhangkedu_2, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_xunhangkedu_2[] ={{19, 8},{19, 12},};
	lv_line_set_points(ui_scr->chongwuyibiao_xunhangkedu_2,chongwuyibiao_xunhangkedu_2,2);

	//Write codes chongwuyibiao_xunhangkedu_3
	ui_scr->chongwuyibiao_xunhangkedu_3 = lv_line_create(ui_scr->chongwuyibiao_xunhangtubiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xunhangkedu_3, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_xunhangkedu_3, 38, 32);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xunhangkedu_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xunhangkedu_3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xunhangkedu_3, GUI_CTRL_LINE);
	static const lv_point_precise_t chongwuyibiao_xunhangkedu_3[] ={{30, 19},{27, 21},};
	lv_line_set_points(ui_scr->chongwuyibiao_xunhangkedu_3,chongwuyibiao_xunhangkedu_3,2);

	//Write codes chongwuyibiao_yinyuerongqi
	ui_scr->chongwuyibiao_yinyuerongqi = lv_obj_create(ui_scr->chongwuyibiao);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yinyuerongqi, 152, 210);
	lv_obj_set_size(ui_scr->chongwuyibiao_yinyuerongqi, 720, 236);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_yinyuerongqi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yinyuerongqi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yinyuerongqi, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_yinyuerongqi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_yinyuerongqi, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_yinyuerongqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_yinyuebeijing
	ui_scr->chongwuyibiao_yinyuebeijing = lv_image_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yinyuebeijing, gui_get_res_path(GUI_RES__PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yinyuebeijing, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yinyuebeijing, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_yinyuebeijing, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yinyuebeijing, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yinyuebeijing, 0, 120);
	lv_obj_set_size(ui_scr->chongwuyibiao_yinyuebeijing, 720, 116);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yinyuebeijing, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yinyuebeijing, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_kongzhibeijing
	ui_scr->chongwuyibiao_kongzhibeijing = lv_image_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_kongzhibeijing, gui_get_res_path(GUI_RES__1_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_kongzhibeijing, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_kongzhibeijing, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_kongzhibeijing, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_kongzhibeijing, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_kongzhibeijing, 436, 136);
	lv_obj_set_size(ui_scr->chongwuyibiao_kongzhibeijing, 268, 84);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_kongzhibeijing, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_kongzhibeijing, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_shangyiqu
	ui_scr->chongwuyibiao_shangyiqu = lv_imgbtn_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_shangyiqu, 463, 158);
	lv_obj_set_size(ui_scr->chongwuyibiao_shangyiqu, 40, 40);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_shangyiqu, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_shangyiqu, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_shangyiqu, GUI_CTRL_IMGBTN);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_shangyiqu, LV_IMGBTN_STATE_RELEASED, NULL, gui_get_res_path(GUI_RES__2_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_shangyiqu, LV_IMGBTN_STATE_PRESSED, NULL, gui_get_res_path(GUI_RES__2_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_shangyiqu, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, gui_get_res_path(GUI_RES__2_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_shangyiqu, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, gui_get_res_path(GUI_RES__2_PNG), NULL);
	lv_obj_add_flag(ui_scr->chongwuyibiao_shangyiqu, LV_OBJ_FLAG_CHECKABLE);
	ui_scr->chongwuyibiao_shangyiqu_label = lv_label_create(ui_scr->chongwuyibiao_shangyiqu);
	lv_label_set_text(ui_scr->chongwuyibiao_shangyiqu_label, "");
	lv_obj_set_style_pad_all(ui_scr->chongwuyibiao_shangyiqu, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->chongwuyibiao_shangyiqu_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes chongwuyibiao_zantingjian
	ui_scr->chongwuyibiao_zantingjian = lv_imgbtn_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zantingjian, 550, 158);
	lv_obj_set_size(ui_scr->chongwuyibiao_zantingjian, 40, 40);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zantingjian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zantingjian, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zantingjian, GUI_CTRL_IMGBTN);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_zantingjian, LV_IMGBTN_STATE_RELEASED, NULL, gui_get_res_path(GUI_RES__3_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_zantingjian, LV_IMGBTN_STATE_PRESSED, NULL, gui_get_res_path(GUI_RES__3_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_zantingjian, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, gui_get_res_path(GUI_RES__3_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_zantingjian, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, gui_get_res_path(GUI_RES__3_PNG), NULL);
	lv_obj_add_flag(ui_scr->chongwuyibiao_zantingjian, LV_OBJ_FLAG_CHECKABLE);
	ui_scr->chongwuyibiao_zantingjian_label = lv_label_create(ui_scr->chongwuyibiao_zantingjian);
	lv_label_set_text(ui_scr->chongwuyibiao_zantingjian_label, "");
	lv_obj_set_style_pad_all(ui_scr->chongwuyibiao_zantingjian, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->chongwuyibiao_zantingjian_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes chongwuyibiao_bofangjian
	ui_scr->chongwuyibiao_bofangjian = lv_imgbtn_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_bofangjian, 550, 158);
	lv_obj_set_size(ui_scr->chongwuyibiao_bofangjian, 40, 40);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_bofangjian, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_bofangjian, LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget chongwuyibiao_bofangjian
	lv_obj_add_flag(ui_scr->chongwuyibiao_bofangjian, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->chongwuyibiao_bofangjian, GUI_CTRL_IMGBTN);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_bofangjian, LV_IMGBTN_STATE_RELEASED, NULL, gui_get_res_path(GUI_RES__4_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_bofangjian, LV_IMGBTN_STATE_PRESSED, NULL, gui_get_res_path(GUI_RES__4_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_bofangjian, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, gui_get_res_path(GUI_RES__4_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_bofangjian, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, gui_get_res_path(GUI_RES__4_PNG), NULL);
	lv_obj_add_flag(ui_scr->chongwuyibiao_bofangjian, LV_OBJ_FLAG_CHECKABLE);
	ui_scr->chongwuyibiao_bofangjian_label = lv_label_create(ui_scr->chongwuyibiao_bofangjian);
	lv_label_set_text(ui_scr->chongwuyibiao_bofangjian_label, "");
	lv_obj_set_style_pad_all(ui_scr->chongwuyibiao_bofangjian, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->chongwuyibiao_bofangjian_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes chongwuyibiao_xiayiqu
	ui_scr->chongwuyibiao_xiayiqu = lv_imgbtn_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_xiayiqu, 637, 158);
	lv_obj_set_size(ui_scr->chongwuyibiao_xiayiqu, 40, 40);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_xiayiqu, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_xiayiqu, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_xiayiqu, GUI_CTRL_IMGBTN);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_xiayiqu, LV_IMGBTN_STATE_RELEASED, NULL, gui_get_res_path(GUI_RES__5_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_xiayiqu, LV_IMGBTN_STATE_PRESSED, NULL, gui_get_res_path(GUI_RES__5_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_xiayiqu, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, gui_get_res_path(GUI_RES__5_PNG), NULL);
	lv_imagebutton_set_src(ui_scr->chongwuyibiao_xiayiqu, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, gui_get_res_path(GUI_RES__5_PNG), NULL);
	lv_obj_add_flag(ui_scr->chongwuyibiao_xiayiqu, LV_OBJ_FLAG_CHECKABLE);
	ui_scr->chongwuyibiao_xiayiqu_label = lv_label_create(ui_scr->chongwuyibiao_xiayiqu);
	lv_label_set_text(ui_scr->chongwuyibiao_xiayiqu_label, "");
	lv_obj_set_style_pad_all(ui_scr->chongwuyibiao_xiayiqu, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->chongwuyibiao_xiayiqu_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes chongwuyibiao_zhuanjirongqi
	ui_scr->chongwuyibiao_zhuanjirongqi = lv_obj_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zhuanjirongqi, 16, 130);
	lv_obj_set_size(ui_scr->chongwuyibiao_zhuanjirongqi, 96, 96);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_zhuanjirongqi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zhuanjirongqi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zhuanjirongqi, GUI_CTRL_CONT);

	//Set style for chongwuyibiao_zhuanjirongqi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->chongwuyibiao_zhuanjirongqi, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->chongwuyibiao_zhuanjirongqi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_zhuanjifengmian
	ui_scr->chongwuyibiao_zhuanjifengmian = lv_image_create(ui_scr->chongwuyibiao_zhuanjirongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_zhuanjifengmian, gui_get_res_path(GUI_RES__6_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_zhuanjifengmian, 48, 48);
	lv_image_set_rotation(ui_scr->chongwuyibiao_zhuanjifengmian, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_zhuanjifengmian, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_zhuanjifengmian, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_zhuanjifengmian, 0, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_zhuanjifengmian, 96, 96);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_zhuanjifengmian, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_zhuanjifengmian, GUI_CTRL_IMG);

	//Set style for chongwuyibiao_zhuanjifengmian. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->chongwuyibiao_zhuanjifengmian, 48, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_clip_corner(ui_scr->chongwuyibiao_zhuanjifengmian, true, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_yinfuyou
	ui_scr->chongwuyibiao_yinfuyou = lv_image_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yinfuyou, gui_get_res_path(GUI_RES__7_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yinfuyou, 12, 16);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yinfuyou, 120);
	lv_image_set_scale(ui_scr->chongwuyibiao_yinfuyou, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yinfuyou, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yinfuyou, 454, 76);
	lv_obj_set_size(ui_scr->chongwuyibiao_yinfuyou, 24, 32);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yinfuyou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yinfuyou, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_yinfuzuo
	ui_scr->chongwuyibiao_yinfuzuo = lv_image_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_yinfuzuo, gui_get_res_path(GUI_RES__8_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_yinfuzuo, 12, 16);
	lv_image_set_rotation(ui_scr->chongwuyibiao_yinfuzuo, -120);
	lv_image_set_scale(ui_scr->chongwuyibiao_yinfuzuo, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_yinfuzuo, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_yinfuzuo, 478, 61);
	lv_obj_set_size(ui_scr->chongwuyibiao_yinfuzuo, 24, 32);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_yinfuzuo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_yinfuzuo, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_tinggexiaogou
	ui_scr->chongwuyibiao_tinggexiaogou = lv_image_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_image_set_src(ui_scr->chongwuyibiao_tinggexiaogou, gui_get_res_path(GUI_RES__9_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_tinggexiaogou, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_tinggexiaogou, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_tinggexiaogou, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_tinggexiaogou, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_tinggexiaogou, 266, 0);
	lv_obj_set_size(ui_scr->chongwuyibiao_tinggexiaogou, 216, 142);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_tinggexiaogou, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_tinggexiaogou, GUI_CTRL_IMG);

	//Write codes chongwuyibiao_gequmingcheng
	ui_scr->chongwuyibiao_gequmingcheng = lv_label_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_label_set_text(ui_scr->chongwuyibiao_gequmingcheng, "等待歌曲信息");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_gequmingcheng, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_pos(ui_scr->chongwuyibiao_gequmingcheng, 132, 141);
	lv_obj_set_size(ui_scr->chongwuyibiao_gequmingcheng, 288, 38);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_gequmingcheng, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_gequmingcheng, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_gequmingcheng, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_gequmingcheng. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_gequmingcheng, &lv_font_lyrics_26, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_gequmingcheng, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_gequmingcheng, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_geciwenzi
	ui_scr->chongwuyibiao_geciwenzi = lv_label_create(ui_scr->chongwuyibiao_yinyuerongqi);
	lv_label_set_text(ui_scr->chongwuyibiao_geciwenzi, "暂无歌词");
	lv_label_set_long_mode(ui_scr->chongwuyibiao_geciwenzi, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_pos(ui_scr->chongwuyibiao_geciwenzi, 132, 180);
	lv_obj_set_size(ui_scr->chongwuyibiao_geciwenzi, 288, 28);
	lv_obj_set_scrollbar_mode(ui_scr->chongwuyibiao_geciwenzi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_geciwenzi, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_geciwenzi, GUI_CTRL_LABEL);

	//Set style for chongwuyibiao_geciwenzi. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->chongwuyibiao_geciwenzi, &lv_font_lyrics_20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->chongwuyibiao_geciwenzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->chongwuyibiao_geciwenzi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes chongwuyibiao_lanyalianjietubiao
	ui_scr->chongwuyibiao_lanyalianjietubiao = lv_image_create(ui_scr->chongwuyibiao);
	lv_image_set_src(ui_scr->chongwuyibiao_lanyalianjietubiao, gui_get_res_path(GUI_RES__10_PNG));
	lv_image_set_pivot(ui_scr->chongwuyibiao_lanyalianjietubiao, 0, 0);
	lv_image_set_rotation(ui_scr->chongwuyibiao_lanyalianjietubiao, 0);
	lv_image_set_scale(ui_scr->chongwuyibiao_lanyalianjietubiao, 256);
	lv_image_set_inner_align(ui_scr->chongwuyibiao_lanyalianjietubiao, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->chongwuyibiao_lanyalianjietubiao, 768, 110);
	lv_obj_set_size(ui_scr->chongwuyibiao_lanyalianjietubiao, 28, 36);
	lv_obj_remove_flag(ui_scr->chongwuyibiao_lanyalianjietubiao, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->chongwuyibiao_lanyalianjietubiao, GUI_CTRL_IMG);

	lv_obj_update_layout(ui_scr->chongwuyibiao);
	ui_scr->chongwuyibiao_del = false;

	//Init events for screen
	events_init_chongwuyibiao(ui);

	neicun_zhuizong_jieduan("chongwu_jingtai");
	chongwuyibiao_donghua_bangding(ui_scr);
	chongwuyibiao_yinyue_bangding(ui_scr);
	return ui_scr->chongwuyibiao;
}
void unload_scr_chongwuyibiao(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_CHONGWUYIBIAO);
}
