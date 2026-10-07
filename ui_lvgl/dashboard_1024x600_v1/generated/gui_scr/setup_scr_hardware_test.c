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


lv_obj_t * setup_scr_hardware_test(lv_ui *ui)
{
	lv_ui_hardware_test * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_hardware_test));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_hardware_test));
		ui->hardware_test = ui_scr;
	}

	//Write codes hardware_test
	ui_scr->hardware_test = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test, GUI_CTRL_SCR);

	//Set style for hardware_test. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->hardware_test, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes hardware_test_lte_4g
	ui_scr->hardware_test_lte_4g = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_lte_4g, 68, 41);
	lv_obj_set_size(ui_scr->hardware_test_lte_4g, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_lte_4g, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_lte_4g, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_lte_4g_label = lv_label_create(ui_scr->hardware_test_lte_4g);
	lv_label_set_text(ui_scr->hardware_test_lte_4g_label, "4G");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_lte_4g, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_lte_4g_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_edit_1
	ui_scr->hardware_test_edit_1 = lv_textarea_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_edit_1, 650, 185);
	lv_obj_set_size(ui_scr->hardware_test_edit_1, 314, 236);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_edit_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->hardware_test_edit_1, LV_OBJ_FLAG_CLICKABLE);

	//Hidden for widget hardware_test_edit_1
	lv_obj_add_flag(ui_scr->hardware_test_edit_1, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->hardware_test_edit_1, GUI_CTRL_TEXTAREA);
	lv_textarea_set_text(ui_scr->hardware_test_edit_1, "SUCCESS");
	lv_textarea_set_one_line(ui_scr->hardware_test_edit_1, false);

	//Write codes hardware_test_btn_5
	ui_scr->hardware_test_btn_5 = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_5, 349, 41);
	lv_obj_set_size(ui_scr->hardware_test_btn_5, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_5, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_5_label = lv_label_create(ui_scr->hardware_test_btn_5);
	lv_label_set_text(ui_scr->hardware_test_btn_5_label, "WIFI");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_5_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_bt
	ui_scr->hardware_test_btn_bt = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_bt, 707, 41);
	lv_obj_set_size(ui_scr->hardware_test_btn_bt, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_bt, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_bt, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_bt_label = lv_label_create(ui_scr->hardware_test_btn_bt);
	lv_label_set_text(ui_scr->hardware_test_btn_bt_label, "BT");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_bt, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_bt_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_gsensor
	ui_scr->hardware_test_btn_gsensor = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_gsensor, 346, 170);
	lv_obj_set_size(ui_scr->hardware_test_btn_gsensor, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_gsensor, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_gsensor, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_gsensor_label = lv_label_create(ui_scr->hardware_test_btn_gsensor);
	lv_label_set_text(ui_scr->hardware_test_btn_gsensor_label, "Gsensor");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_gsensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_gsensor_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_7
	ui_scr->hardware_test_btn_7 = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_7, 349, 299);
	lv_obj_set_size(ui_scr->hardware_test_btn_7, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_7, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_7, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_7_label = lv_label_create(ui_scr->hardware_test_btn_7);
	lv_label_set_text(ui_scr->hardware_test_btn_7_label, "ADKEY");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_7, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_7_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_ui_effect
	ui_scr->hardware_test_btn_ui_effect = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_ui_effect, 353, 428);
	lv_obj_set_size(ui_scr->hardware_test_btn_ui_effect, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_ui_effect, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_ui_effect, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_ui_effect_label = lv_label_create(ui_scr->hardware_test_btn_ui_effect);
	lv_label_set_text(ui_scr->hardware_test_btn_ui_effect_label, "UI Effect");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_ui_effect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_ui_effect_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_can
	ui_scr->hardware_test_can = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_can, 65, 299);
	lv_obj_set_size(ui_scr->hardware_test_can, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_can, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_can, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_can_label = lv_label_create(ui_scr->hardware_test_can);
	lv_label_set_text(ui_scr->hardware_test_can_label, "CAN");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_can, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_can_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_4
	ui_scr->hardware_test_btn_4 = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_4, 68, 428);
	lv_obj_set_size(ui_scr->hardware_test_btn_4, 239, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_4, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_4_label = lv_label_create(ui_scr->hardware_test_btn_4);
	lv_label_set_text(ui_scr->hardware_test_btn_4_label, "GPS");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_4_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_mic0
	ui_scr->hardware_test_mic0 = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_mic0, 68, 170);
	lv_obj_set_size(ui_scr->hardware_test_mic0, 102, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_mic0, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_mic0, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_mic0_label = lv_label_create(ui_scr->hardware_test_mic0);
	lv_label_set_text(ui_scr->hardware_test_mic0_label, "MIC1");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_mic0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_mic0_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_btn_6
	ui_scr->hardware_test_btn_6 = lv_btn_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_btn_6, 202, 170);
	lv_obj_set_size(ui_scr->hardware_test_btn_6, 102, 84);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_btn_6, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->hardware_test_btn_6, GUI_CTRL_BUTTON);
	ui_scr->hardware_test_btn_6_label = lv_label_create(ui_scr->hardware_test_btn_6);
	lv_label_set_text(ui_scr->hardware_test_btn_6_label, "MIC2");
	lv_obj_set_style_pad_all(ui_scr->hardware_test_btn_6, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->hardware_test_btn_6_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes hardware_test_edit_2
	ui_scr->hardware_test_edit_2 = lv_textarea_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_edit_2, 681, 272);
	lv_obj_set_size(ui_scr->hardware_test_edit_2, 258, 76);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_edit_2, LV_SCROLLBAR_MODE_OFF);

	//Hidden for widget hardware_test_edit_2
	lv_obj_add_flag(ui_scr->hardware_test_edit_2, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->hardware_test_edit_2, GUI_CTRL_TEXTAREA);
	lv_textarea_set_text(ui_scr->hardware_test_edit_2, "RESERVED1");
	lv_textarea_set_one_line(ui_scr->hardware_test_edit_2, false);

	//Write codes hardware_test_edit_3
	ui_scr->hardware_test_edit_3 = lv_textarea_create(ui_scr->hardware_test);
	lv_obj_set_pos(ui_scr->hardware_test_edit_3, 681, 410);
	lv_obj_set_size(ui_scr->hardware_test_edit_3, 258, 76);
	lv_obj_set_scrollbar_mode(ui_scr->hardware_test_edit_3, LV_SCROLLBAR_MODE_OFF);

	//Hidden for widget hardware_test_edit_3
	lv_obj_add_flag(ui_scr->hardware_test_edit_3, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->hardware_test_edit_3, GUI_CTRL_TEXTAREA);
	lv_textarea_set_text(ui_scr->hardware_test_edit_3, "RESERVED2");
	lv_textarea_set_one_line(ui_scr->hardware_test_edit_3, false);

	// Write Timer hardware_test_timer_1
	if(ui_scr->hardware_test_timer_1 != NULL) lv_timer_delete(ui_scr->hardware_test_timer_1);
	ui_scr->hardware_test_timer_1 = lv_timer_create(hardware_test_timer_1_timer_cb, 200, "");
	lv_timer_set_repeat_count(ui_scr->hardware_test_timer_1, -1);

	lv_obj_update_layout(ui_scr->hardware_test);
	ui_scr->hardware_test_del = false;

	//Init events for screen
	events_init_hardware_test(ui);
	return ui_scr->hardware_test;
}
void unload_scr_hardware_test(lv_ui *ui)
{
	lv_ui_hardware_test * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
	if(ui_scr->hardware_test_timer_1 != NULL) {
		lv_timer_delete(ui_scr->hardware_test_timer_1);
		ui_scr->hardware_test_timer_1 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
}
