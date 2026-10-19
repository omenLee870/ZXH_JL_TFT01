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


lv_obj_t * setup_scr_upgrade_page(lv_ui *ui)
{
	lv_ui_upgrade_page * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_UPGRADE_PAGE);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_upgrade_page));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_upgrade_page));
		ui->upgrade_page = ui_scr;
	}

	//Write codes upgrade_page
	ui_scr->upgrade_page = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->upgrade_page, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->upgrade_page, GUI_CTRL_SCR);

	//Write codes upgrade_page_lbl_upgrade_title
	ui_scr->upgrade_page_lbl_upgrade_title = lv_label_create(ui_scr->upgrade_page);
	lv_label_set_text(ui_scr->upgrade_page_lbl_upgrade_title, "正在升级，请勿断电\n\n升级成功！\n升级失败！\n");
	lv_label_set_long_mode(ui_scr->upgrade_page_lbl_upgrade_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->upgrade_page_lbl_upgrade_title, 0, 140);
	lv_obj_set_size(ui_scr->upgrade_page_lbl_upgrade_title, 1024, 66);
	lv_obj_set_scrollbar_mode(ui_scr->upgrade_page_lbl_upgrade_title, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->upgrade_page_lbl_upgrade_title, GUI_CTRL_LABEL);

	//Set style for upgrade_page_lbl_upgrade_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->upgrade_page_lbl_upgrade_title, &lv_font_simsun_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->upgrade_page_lbl_upgrade_title, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->upgrade_page_lbl_upgrade_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->upgrade_page_lbl_upgrade_title, 8, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes upgrade_page_bar_upgrade
	ui_scr->upgrade_page_bar_upgrade = lv_bar_create(ui_scr->upgrade_page);
	lv_obj_set_pos(ui_scr->upgrade_page_bar_upgrade, 144, 273);
	lv_obj_set_size(ui_scr->upgrade_page_bar_upgrade, 742, 42);
	lv_obj_set_scrollbar_mode(ui_scr->upgrade_page_bar_upgrade, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->upgrade_page_bar_upgrade, GUI_CTRL_BAR);
	lv_obj_set_style_anim_duration(ui_scr->upgrade_page_bar_upgrade, 1000, 0);
	lv_bar_set_mode(ui_scr->upgrade_page_bar_upgrade, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui_scr->upgrade_page_bar_upgrade, 0, 100);
	lv_bar_set_value(ui_scr->upgrade_page_bar_upgrade, 10, LV_ANIM_OFF);

	//Write codes upgrade_page_lbl_upgrade_percent
	ui_scr->upgrade_page_lbl_upgrade_percent = lv_label_create(ui_scr->upgrade_page);
	lv_label_set_text(ui_scr->upgrade_page_lbl_upgrade_percent, "10%");
	lv_label_set_long_mode(ui_scr->upgrade_page_lbl_upgrade_percent, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->upgrade_page_lbl_upgrade_percent, 0, 386);
	lv_obj_set_size(ui_scr->upgrade_page_lbl_upgrade_percent, 1024, 66);
	lv_obj_set_scrollbar_mode(ui_scr->upgrade_page_lbl_upgrade_percent, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->upgrade_page_lbl_upgrade_percent, GUI_CTRL_LABEL);

	//Set style for upgrade_page_lbl_upgrade_percent. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->upgrade_page_lbl_upgrade_percent, &lv_font_simsun_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->upgrade_page_lbl_upgrade_percent, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->upgrade_page_lbl_upgrade_percent, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->upgrade_page_lbl_upgrade_percent, 8, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_update_layout(ui_scr->upgrade_page);
	ui_scr->upgrade_page_del = false;

	//Init events for screen
	events_init_upgrade_page(ui);
	return ui_scr->upgrade_page;
}
void unload_scr_upgrade_page(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_UPGRADE_PAGE);
}
