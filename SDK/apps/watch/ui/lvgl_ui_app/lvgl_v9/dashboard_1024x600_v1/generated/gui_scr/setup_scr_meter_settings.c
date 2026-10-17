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
#include "ui_meter_settings.h"


lv_obj_t * setup_scr_meter_settings(lv_ui *ui)
{
	lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_meter_settings));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_meter_settings));
		ui->meter_settings = ui_scr;
	}

	//Write codes meter_settings
	ui_scr->meter_settings = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->meter_settings, GUI_CTRL_SCR);

	//Set style for meter_settings. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_back
	ui_scr->meter_settings_back = lv_btn_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_back, 772, 540);
	lv_obj_set_size(ui_scr->meter_settings_back, 220, 40);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_back, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_back, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_back, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_back_label = lv_label_create(ui_scr->meter_settings_back);
	lv_label_set_text(ui_scr->meter_settings_back_label, "返回");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_back_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_panel_0
	ui_scr->meter_settings_panel_0 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_0, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_0, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_0, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_0, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_0, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);
	ui_style_set(ui_scr->meter_settings_panel_0, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_0. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_0, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_0, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_0
	ui_scr->meter_settings_title_0 = lv_label_create(ui_scr->meter_settings_panel_0);
	lv_label_set_text(ui_scr->meter_settings_title_0, "屏幕亮度");
	lv_label_set_long_mode(ui_scr->meter_settings_title_0, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_0, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_0, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_0, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_0, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_0, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_0. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_0, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_0, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_0, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_0, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_brightness_title
	ui_scr->meter_settings_brightness_title = lv_label_create(ui_scr->meter_settings_panel_0);
	lv_label_set_text(ui_scr->meter_settings_brightness_title, "亮度");
	lv_label_set_long_mode(ui_scr->meter_settings_brightness_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_brightness_title, 32, 168);
	lv_obj_set_size(ui_scr->meter_settings_brightness_title, 420, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_brightness_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_brightness_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_brightness_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_brightness_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_brightness_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_brightness_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_brightness_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_brightness_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_brightness_value
	ui_scr->meter_settings_brightness_value = lv_label_create(ui_scr->meter_settings_panel_0);
	lv_label_set_text(ui_scr->meter_settings_brightness_value, "70%");
	lv_label_set_long_mode(ui_scr->meter_settings_brightness_value, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_brightness_value, 552, 168);
	lv_obj_set_size(ui_scr->meter_settings_brightness_value, 120, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_brightness_value, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_brightness_value, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_brightness_value, GUI_CTRL_LABEL);

	//Set style for meter_settings_brightness_value. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_brightness_value, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_brightness_value, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_brightness_value, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_brightness_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_brightness_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_brightness
	ui_scr->meter_settings_brightness = lv_slider_create(ui_scr->meter_settings_panel_0);
	lv_obj_set_pos(ui_scr->meter_settings_brightness, 48, 244);
	lv_obj_set_size(ui_scr->meter_settings_brightness, 604, 24);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_brightness, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui_scr->meter_settings_brightness, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
	lv_obj_remove_flag(ui_scr->meter_settings_brightness, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_brightness, GUI_CTRL_SLIDER);

	//Set style for meter_settings_brightness. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_shadow_opa(ui_scr->meter_settings_brightness, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for meter_settings_brightness. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_brightness, lv_color_make(0x26, 0x8b, 0xff), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for meter_settings_brightness. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_brightness, lv_color_make(0xed, 0xf4, 0xff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->meter_settings_brightness, 10, 100);
	lv_slider_set_mode(ui_scr->meter_settings_brightness, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->meter_settings_brightness, 70, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->meter_settings_brightness, 0, LV_ANIM_OFF);

	//Write codes meter_settings_panel_1
	ui_scr->meter_settings_panel_1 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_1, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_1, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_1, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_1, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_1, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	//Hidden for widget meter_settings_panel_1
	lv_obj_add_flag(ui_scr->meter_settings_panel_1, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_panel_1, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_1, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_1, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_1
	ui_scr->meter_settings_title_1 = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_title_1, "声音");
	lv_label_set_long_mode(ui_scr->meter_settings_title_1, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_1, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_1, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_1, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_1, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_1, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_1, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_1, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_music_title
	ui_scr->meter_settings_music_title = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_music_title, "音乐音量");
	lv_label_set_long_mode(ui_scr->meter_settings_music_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_music_title, 32, 104);
	lv_obj_set_size(ui_scr->meter_settings_music_title, 420, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_music_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_music_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_music_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_music_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_music_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_music_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_music_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_music_value
	ui_scr->meter_settings_music_value = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_music_value, "50%");
	lv_label_set_long_mode(ui_scr->meter_settings_music_value, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_music_value, 552, 104);
	lv_obj_set_size(ui_scr->meter_settings_music_value, 120, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_music_value, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_music_value, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_music_value, GUI_CTRL_LABEL);

	//Set style for meter_settings_music_value. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_music_value, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_music_value, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_music_value, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_music_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_music_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_music
	ui_scr->meter_settings_music = lv_slider_create(ui_scr->meter_settings_panel_1);
	lv_obj_set_pos(ui_scr->meter_settings_music, 48, 180);
	lv_obj_set_size(ui_scr->meter_settings_music, 604, 24);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_music, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui_scr->meter_settings_music, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
	lv_obj_remove_flag(ui_scr->meter_settings_music, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_music, GUI_CTRL_SLIDER);

	//Set style for meter_settings_music. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_shadow_opa(ui_scr->meter_settings_music, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for meter_settings_music. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_music, lv_color_make(0x26, 0x8b, 0xff), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for meter_settings_music. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_music, lv_color_make(0xed, 0xf4, 0xff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->meter_settings_music, 0, 100);
	lv_slider_set_mode(ui_scr->meter_settings_music, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->meter_settings_music, 50, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->meter_settings_music, 0, LV_ANIM_OFF);

	//Write codes meter_settings_tone_title
	ui_scr->meter_settings_tone_title = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_tone_title, "提示音量");
	lv_label_set_long_mode(ui_scr->meter_settings_tone_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_tone_title, 32, 248);
	lv_obj_set_size(ui_scr->meter_settings_tone_title, 420, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_tone_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_tone_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_tone_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_tone_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_tone_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_tone_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_tone_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_tone_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_tone_value
	ui_scr->meter_settings_tone_value = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_tone_value, "30%");
	lv_label_set_long_mode(ui_scr->meter_settings_tone_value, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_tone_value, 552, 248);
	lv_obj_set_size(ui_scr->meter_settings_tone_value, 120, 42);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_tone_value, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_tone_value, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_tone_value, GUI_CTRL_LABEL);

	//Set style for meter_settings_tone_value. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_tone_value, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_tone_value, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_tone_value, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_tone_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_tone_value, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_tone
	ui_scr->meter_settings_tone = lv_slider_create(ui_scr->meter_settings_panel_1);
	lv_obj_set_pos(ui_scr->meter_settings_tone, 48, 324);
	lv_obj_set_size(ui_scr->meter_settings_tone, 604, 24);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_tone, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui_scr->meter_settings_tone, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
	lv_obj_remove_flag(ui_scr->meter_settings_tone, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_tone, GUI_CTRL_SLIDER);

	//Set style for meter_settings_tone. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_shadow_opa(ui_scr->meter_settings_tone, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for meter_settings_tone. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_tone, lv_color_make(0x26, 0x8b, 0xff), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for meter_settings_tone. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_tone, lv_color_make(0xed, 0xf4, 0xff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->meter_settings_tone, 0, 100);
	lv_slider_set_mode(ui_scr->meter_settings_tone, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->meter_settings_tone, 30, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->meter_settings_tone, 0, LV_ANIM_OFF);

	//Write codes meter_settings_key_label
	ui_scr->meter_settings_key_label = lv_label_create(ui_scr->meter_settings_panel_1);
	lv_label_set_text(ui_scr->meter_settings_key_label, "按键提示音");
	lv_label_set_long_mode(ui_scr->meter_settings_key_label, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_key_label, 32, 410);
	lv_obj_set_size(ui_scr->meter_settings_key_label, 420, 48);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_key_label, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_key_label, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_key_label, GUI_CTRL_LABEL);

	//Set style for meter_settings_key_label. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_key_label, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_key_label, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_key_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_key_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_key_tone
	ui_scr->meter_settings_key_tone = lv_switch_create(ui_scr->meter_settings_panel_1);
	lv_obj_set_pos(ui_scr->meter_settings_key_tone, 574, 410);
	lv_obj_set_size(ui_scr->meter_settings_key_tone, 90, 44);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_key_tone, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_key_tone, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_state(ui_scr->meter_settings_key_tone, LV_STATE_CHECKED);
	ui_style_set(ui_scr->meter_settings_key_tone, GUI_CTRL_SWITCH);

	//Write codes meter_settings_panel_2
	ui_scr->meter_settings_panel_2 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_2, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_2, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_2, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_2, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_2, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	//Hidden for widget meter_settings_panel_2
	lv_obj_add_flag(ui_scr->meter_settings_panel_2, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_panel_2, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_2, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_2, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_2
	ui_scr->meter_settings_title_2 = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_title_2, "里程管理");
	lv_label_set_long_mode(ui_scr->meter_settings_title_2, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_2, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_2, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_2, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_2, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_2, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_2, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_2, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_unit_title
	ui_scr->meter_settings_unit_title = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_unit_title, "显示单位");
	lv_label_set_long_mode(ui_scr->meter_settings_unit_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_unit_title, 32, 110);
	lv_obj_set_size(ui_scr->meter_settings_unit_title, 240, 46);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_unit_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_unit_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_unit_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_unit_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_unit_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_unit_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_unit_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_unit_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_unit_km
	ui_scr->meter_settings_unit_km = lv_btn_create(ui_scr->meter_settings_panel_2);
	lv_obj_set_pos(ui_scr->meter_settings_unit_km, 300, 108);
	lv_obj_set_size(ui_scr->meter_settings_unit_km, 174, 54);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_unit_km, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_unit_km, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_unit_km, GUI_CTRL_BUTTON);

	//Set style for meter_settings_unit_km. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_unit_km, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->meter_settings_unit_km_label = lv_label_create(ui_scr->meter_settings_unit_km);
	lv_label_set_text(ui_scr->meter_settings_unit_km_label, "km / km/h");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_unit_km, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_unit_km_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_unit_mi
	ui_scr->meter_settings_unit_mi = lv_btn_create(ui_scr->meter_settings_panel_2);
	lv_obj_set_pos(ui_scr->meter_settings_unit_mi, 490, 108);
	lv_obj_set_size(ui_scr->meter_settings_unit_mi, 174, 54);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_unit_mi, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_unit_mi, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_unit_mi, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_unit_mi_label = lv_label_create(ui_scr->meter_settings_unit_mi);
	lv_label_set_text(ui_scr->meter_settings_unit_mi_label, "mi / mph");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_unit_mi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_unit_mi_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_trip_title
	ui_scr->meter_settings_trip_title = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_trip_title, "小计里程");
	lv_label_set_long_mode(ui_scr->meter_settings_trip_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_trip_title, 32, 224);
	lv_obj_set_size(ui_scr->meter_settings_trip_title, 200, 48);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_trip_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_trip_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_trip_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_trip_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_trip_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_trip_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_trip_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_trip_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_trip
	ui_scr->meter_settings_trip = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_trip, "0.0 km");
	lv_label_set_long_mode(ui_scr->meter_settings_trip, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_trip, 268, 222);
	lv_obj_set_size(ui_scr->meter_settings_trip, 230, 48);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_trip, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_trip, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_trip, GUI_CTRL_LABEL);

	//Set style for meter_settings_trip. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_trip, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_trip, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_trip, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_trip, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_trip, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_trip_clear
	ui_scr->meter_settings_trip_clear = lv_btn_create(ui_scr->meter_settings_panel_2);
	lv_obj_set_pos(ui_scr->meter_settings_trip_clear, 518, 218);
	lv_obj_set_size(ui_scr->meter_settings_trip_clear, 146, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_trip_clear, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_trip_clear, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_trip_clear, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_trip_clear_label = lv_label_create(ui_scr->meter_settings_trip_clear);
	lv_label_set_text(ui_scr->meter_settings_trip_clear_label, "清零");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_trip_clear, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_trip_clear_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_odo_title
	ui_scr->meter_settings_odo_title = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_odo_title, "累计里程");
	lv_label_set_long_mode(ui_scr->meter_settings_odo_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_odo_title, 32, 352);
	lv_obj_set_size(ui_scr->meter_settings_odo_title, 200, 48);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_odo_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_odo_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_odo_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_odo_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_odo_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_odo_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_odo_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_odo_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_odo
	ui_scr->meter_settings_odo = lv_label_create(ui_scr->meter_settings_panel_2);
	lv_label_set_text(ui_scr->meter_settings_odo, "0.0 km");
	lv_label_set_long_mode(ui_scr->meter_settings_odo, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_odo, 268, 350);
	lv_obj_set_size(ui_scr->meter_settings_odo, 390, 48);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_odo, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_odo, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_odo, GUI_CTRL_LABEL);

	//Set style for meter_settings_odo. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_odo, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_odo, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_odo, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_odo, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_odo, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_panel_3
	ui_scr->meter_settings_panel_3 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_3, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_3, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_3, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_3, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_3, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	//Hidden for widget meter_settings_panel_3
	lv_obj_add_flag(ui_scr->meter_settings_panel_3, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_panel_3, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_3, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_3, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_3
	ui_scr->meter_settings_title_3 = lv_label_create(ui_scr->meter_settings_panel_3);
	lv_label_set_text(ui_scr->meter_settings_title_3, "自动熄屏");
	lv_label_set_long_mode(ui_scr->meter_settings_title_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_3, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_3, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_3, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_3, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_3, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_3, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_3, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_sleep_0
	ui_scr->meter_settings_sleep_0 = lv_btn_create(ui_scr->meter_settings_panel_3);
	lv_obj_set_pos(ui_scr->meter_settings_sleep_0, 32, 142);
	lv_obj_set_size(ui_scr->meter_settings_sleep_0, 300, 94);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_sleep_0, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_sleep_0, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_sleep_0, GUI_CTRL_BUTTON);

	//Set style for meter_settings_sleep_0. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_sleep_0, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->meter_settings_sleep_0_label = lv_label_create(ui_scr->meter_settings_sleep_0);
	lv_label_set_text(ui_scr->meter_settings_sleep_0_label, "关闭");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_sleep_0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_sleep_0_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_sleep_1
	ui_scr->meter_settings_sleep_1 = lv_btn_create(ui_scr->meter_settings_panel_3);
	lv_obj_set_pos(ui_scr->meter_settings_sleep_1, 360, 142);
	lv_obj_set_size(ui_scr->meter_settings_sleep_1, 300, 94);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_sleep_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_sleep_1, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_sleep_1, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_sleep_1_label = lv_label_create(ui_scr->meter_settings_sleep_1);
	lv_label_set_text(ui_scr->meter_settings_sleep_1_label, "1 分钟");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_sleep_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_sleep_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_sleep_3
	ui_scr->meter_settings_sleep_3 = lv_btn_create(ui_scr->meter_settings_panel_3);
	lv_obj_set_pos(ui_scr->meter_settings_sleep_3, 32, 268);
	lv_obj_set_size(ui_scr->meter_settings_sleep_3, 300, 94);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_sleep_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_sleep_3, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_sleep_3, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_sleep_3_label = lv_label_create(ui_scr->meter_settings_sleep_3);
	lv_label_set_text(ui_scr->meter_settings_sleep_3_label, "3 分钟");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_sleep_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_sleep_3_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_sleep_5
	ui_scr->meter_settings_sleep_5 = lv_btn_create(ui_scr->meter_settings_panel_3);
	lv_obj_set_pos(ui_scr->meter_settings_sleep_5, 360, 268);
	lv_obj_set_size(ui_scr->meter_settings_sleep_5, 300, 94);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_sleep_5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_sleep_5, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_sleep_5, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_sleep_5_label = lv_label_create(ui_scr->meter_settings_sleep_5);
	lv_label_set_text(ui_scr->meter_settings_sleep_5_label, "5 分钟");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_sleep_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_sleep_5_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_panel_4
	ui_scr->meter_settings_panel_4 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_4, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_4, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_4, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_4, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_4, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	//Hidden for widget meter_settings_panel_4
	lv_obj_add_flag(ui_scr->meter_settings_panel_4, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_panel_4, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_4, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_4, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_4
	ui_scr->meter_settings_title_4 = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_title_4, "设备信息");
	lv_label_set_long_mode(ui_scr->meter_settings_title_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_4, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_4, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_4, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_4, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_4, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_4, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_4, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_device_title
	ui_scr->meter_settings_device_title = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_device_title, "设备名称");
	lv_label_set_long_mode(ui_scr->meter_settings_device_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_device_title, 32, 132);
	lv_obj_set_size(ui_scr->meter_settings_device_title, 240, 44);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_device_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_device_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_device_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_device_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_device_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_device_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_device_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_device_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_device
	ui_scr->meter_settings_device = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_device, "JL380N");
	lv_label_set_long_mode(ui_scr->meter_settings_device, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_device, 286, 132);
	lv_obj_set_size(ui_scr->meter_settings_device, 378, 70);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_device, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_device, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_device, GUI_CTRL_LABEL);

	//Set style for meter_settings_device. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_device, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_device, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_device, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_device, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_model_title
	ui_scr->meter_settings_model_title = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_model_title, "设备型号");
	lv_label_set_long_mode(ui_scr->meter_settings_model_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_model_title, 32, 240);
	lv_obj_set_size(ui_scr->meter_settings_model_title, 240, 44);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_model_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_model_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_model_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_model_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_model_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_model_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_model_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_model_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_model
	ui_scr->meter_settings_model = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_model, "ZXH-E02");
	lv_label_set_long_mode(ui_scr->meter_settings_model, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_model, 286, 240);
	lv_obj_set_size(ui_scr->meter_settings_model, 378, 70);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_model, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_model, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_model, GUI_CTRL_LABEL);

	//Set style for meter_settings_model. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_model, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_model, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_model, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_model, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_firmware_title
	ui_scr->meter_settings_firmware_title = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_firmware_title, "固件版本");
	lv_label_set_long_mode(ui_scr->meter_settings_firmware_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_firmware_title, 32, 348);
	lv_obj_set_size(ui_scr->meter_settings_firmware_title, 240, 44);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_firmware_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_firmware_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_firmware_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_firmware_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_firmware_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_firmware_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_firmware_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_firmware_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_firmware
	ui_scr->meter_settings_firmware = lv_label_create(ui_scr->meter_settings_panel_4);
	lv_label_set_text(ui_scr->meter_settings_firmware, "--");
	lv_label_set_long_mode(ui_scr->meter_settings_firmware, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_firmware, 286, 348);
	lv_obj_set_size(ui_scr->meter_settings_firmware, 378, 70);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_firmware, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_firmware, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_firmware, GUI_CTRL_LABEL);

	//Set style for meter_settings_firmware. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_firmware, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_firmware, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_firmware, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_firmware, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_panel_5
	ui_scr->meter_settings_panel_5 = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_panel_5, 28, 28);
	lv_obj_set_size(ui_scr->meter_settings_panel_5, 716, 544);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_panel_5, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_panel_5, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_panel_5, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);

	//Hidden for widget meter_settings_panel_5
	lv_obj_add_flag(ui_scr->meter_settings_panel_5, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_panel_5, GUI_CTRL_CONT);

	//Set style for meter_settings_panel_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_panel_5, 22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_panel_5, lv_color_make(0x0d, 0x15, 0x22), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_title_5
	ui_scr->meter_settings_title_5 = lv_label_create(ui_scr->meter_settings_panel_5);
	lv_label_set_text(ui_scr->meter_settings_title_5, "恢复默认");
	lv_label_set_long_mode(ui_scr->meter_settings_title_5, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_title_5, 32, 24);
	lv_obj_set_size(ui_scr->meter_settings_title_5, 630, 56);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_title_5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_title_5, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_title_5, GUI_CTRL_LABEL);

	//Set style for meter_settings_title_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_title_5, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_title_5, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_title_5, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_title_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_title_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_restore
	ui_scr->meter_settings_restore = lv_btn_create(ui_scr->meter_settings_panel_5);
	lv_obj_set_pos(ui_scr->meter_settings_restore, 32, 208);
	lv_obj_set_size(ui_scr->meter_settings_restore, 632, 94);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_restore, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_restore, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_restore, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_restore_label = lv_label_create(ui_scr->meter_settings_restore);
	lv_label_set_text(ui_scr->meter_settings_restore_label, "恢复默认");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_restore, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_restore_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_confirm
	ui_scr->meter_settings_confirm = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_confirm, 176, 172);
	lv_obj_set_size(ui_scr->meter_settings_confirm, 672, 256);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_confirm, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(ui_scr->meter_settings_confirm, LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(ui_scr->meter_settings_confirm, LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget meter_settings_confirm
	lv_obj_add_flag(ui_scr->meter_settings_confirm, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->meter_settings_confirm, GUI_CTRL_CONT);

	//Set style for meter_settings_confirm. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->meter_settings_confirm, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->meter_settings_confirm, lv_color_make(0x14, 0x24, 0x3a), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_confirm_title
	ui_scr->meter_settings_confirm_title = lv_label_create(ui_scr->meter_settings_confirm);
	lv_label_set_text(ui_scr->meter_settings_confirm_title, "清零小计里程？");
	lv_label_set_long_mode(ui_scr->meter_settings_confirm_title, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->meter_settings_confirm_title, 32, 36);
	lv_obj_set_size(ui_scr->meter_settings_confirm_title, 608, 66);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_confirm_title, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_confirm_title, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_confirm_title, GUI_CTRL_LABEL);

	//Set style for meter_settings_confirm_title. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_confirm_title, lv_color_make(0x06, 0x09, 0x0e), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->meter_settings_confirm_title, lv_color_make(0xed, 0xf4, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->meter_settings_confirm_title, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui_scr->meter_settings_confirm_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_scr->meter_settings_confirm_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_confirm_cancel
	ui_scr->meter_settings_confirm_cancel = lv_btn_create(ui_scr->meter_settings_confirm);
	lv_obj_set_pos(ui_scr->meter_settings_confirm_cancel, 32, 150);
	lv_obj_set_size(ui_scr->meter_settings_confirm_cancel, 280, 66);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_confirm_cancel, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_confirm_cancel, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_confirm_cancel, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_confirm_cancel_label = lv_label_create(ui_scr->meter_settings_confirm_cancel);
	lv_label_set_text(ui_scr->meter_settings_confirm_cancel_label, "取消");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_confirm_cancel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_confirm_cancel_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_confirm_ok
	ui_scr->meter_settings_confirm_ok = lv_btn_create(ui_scr->meter_settings_confirm);
	lv_obj_set_pos(ui_scr->meter_settings_confirm_ok, 360, 150);
	lv_obj_set_size(ui_scr->meter_settings_confirm_ok, 280, 66);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_confirm_ok, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_confirm_ok, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_confirm_ok, GUI_CTRL_BUTTON);

	//Set style for meter_settings_confirm_ok. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_confirm_ok, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->meter_settings_confirm_ok_label = lv_label_create(ui_scr->meter_settings_confirm_ok);
	lv_label_set_text(ui_scr->meter_settings_confirm_ok_label, "确认");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_confirm_ok, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_confirm_ok_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_settings_menu
	ui_scr->meter_settings_settings_menu = lv_obj_create(ui_scr->meter_settings);
	lv_obj_set_pos(ui_scr->meter_settings_settings_menu, 764, 28);
	lv_obj_set_size(ui_scr->meter_settings_settings_menu, 236, 492);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_settings_menu, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_add_flag(ui_scr->meter_settings_settings_menu, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(ui_scr->meter_settings_settings_menu, LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER);
	ui_style_set(ui_scr->meter_settings_settings_menu, GUI_CTRL_CONT);

	//Set style for meter_settings_settings_menu. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_settings_menu, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->meter_settings_settings_menu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes meter_settings_menu_0
	ui_scr->meter_settings_menu_0 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_0, 8, 0);
	lv_obj_set_size(ui_scr->meter_settings_menu_0, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_0, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_0, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_0, GUI_CTRL_BUTTON);

	//Set style for meter_settings_menu_0. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->meter_settings_menu_0, lv_color_make(0x26, 0x8b, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->meter_settings_menu_0_label = lv_label_create(ui_scr->meter_settings_menu_0);
	lv_label_set_text(ui_scr->meter_settings_menu_0_label, "屏幕亮度");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_0_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_menu_1
	ui_scr->meter_settings_menu_1 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_1, 8, 100);
	lv_obj_set_size(ui_scr->meter_settings_menu_1, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_1, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_1, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_menu_1_label = lv_label_create(ui_scr->meter_settings_menu_1);
	lv_label_set_text(ui_scr->meter_settings_menu_1_label, "声音");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_menu_2
	ui_scr->meter_settings_menu_2 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_2, 8, 200);
	lv_obj_set_size(ui_scr->meter_settings_menu_2, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_2, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_2, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_2, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_menu_2_label = lv_label_create(ui_scr->meter_settings_menu_2);
	lv_label_set_text(ui_scr->meter_settings_menu_2_label, "里程管理");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_2_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_menu_3
	ui_scr->meter_settings_menu_3 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_3, 8, 300);
	lv_obj_set_size(ui_scr->meter_settings_menu_3, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_3, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_3, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_3, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_menu_3_label = lv_label_create(ui_scr->meter_settings_menu_3);
	lv_label_set_text(ui_scr->meter_settings_menu_3_label, "自动熄屏");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_3_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_menu_4
	ui_scr->meter_settings_menu_4 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_4, 8, 400);
	lv_obj_set_size(ui_scr->meter_settings_menu_4, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_4, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_4, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_4, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_menu_4_label = lv_label_create(ui_scr->meter_settings_menu_4);
	lv_label_set_text(ui_scr->meter_settings_menu_4_label, "设备信息");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_4_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes meter_settings_menu_5
	ui_scr->meter_settings_menu_5 = lv_btn_create(ui_scr->meter_settings_settings_menu);
	lv_obj_set_pos(ui_scr->meter_settings_menu_5, 8, 500);
	lv_obj_set_size(ui_scr->meter_settings_menu_5, 220, 88);
	lv_obj_set_scrollbar_mode(ui_scr->meter_settings_menu_5, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->meter_settings_menu_5, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->meter_settings_menu_5, GUI_CTRL_BUTTON);
	ui_scr->meter_settings_menu_5_label = lv_label_create(ui_scr->meter_settings_menu_5);
	lv_label_set_text(ui_scr->meter_settings_menu_5_label, "恢复默认");
	lv_obj_set_style_pad_all(ui_scr->meter_settings_menu_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->meter_settings_menu_5_label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_update_layout(ui_scr->meter_settings);
	ui_scr->meter_settings_del = false;

	//Init events for screen
	events_init_meter_settings(ui);

	ui_meter_settings_bind();
	return ui_scr->meter_settings;
}
void unload_scr_meter_settings(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
}
