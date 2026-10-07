/*Generate Code, Do NOT Edit!*/
#include "lvgl.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include "./gui_guider.h"
#include "./gui_timelines/gui_timelines.h"
#include "./gui_group/gui_group.h"
#include "./gui_events/events_init.h"

void scr_push_stack_loaded_handler(lv_event_t *e)
{
	int32_t scr_id = (int32_t)lv_event_get_user_data(e);
	lv_obj_t * src = lv_event_get_target(e);
	gui_scr_t * scr = ui_get_scr(scr_id);
	if (scr != NULL) {
		gui_scr_stack_push(scr);
	}
}
void ui_load_scr_anim(lv_ui *ui, gui_scr_t *screen, lv_screen_load_anim_t anim_type,	uint32_t time,
											uint32_t delay, bool is_clean, bool auto_del, bool is_push_satck)
{
	if (screen == NULL) return;

	gui_scr_t * act_screen = gui_scr_get_act();
	if (act_screen == screen) return;

	gui_timelines_delete();
	gui_scr_set_act_anim(screen, ui, anim_type, time, delay, is_clean, auto_del);

	if (is_push_satck) {
		lv_obj_remove_event_cb(screen->scr, scr_push_stack_loaded_handler);
		if (screen == gui_scr_get_act()) {
			gui_scr_stack_push(act_screen);
		} else {
			lv_obj_add_event_cb(screen->scr, scr_push_stack_loaded_handler, LV_EVENT_SCREEN_LOADED, (void *)act_screen->id);
		}
	}
}

void ui_scr_stack_pop_anim(lv_ui *ui, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay,
														bool is_clean, bool auto_del, bool is_push_satck)
{
	gui_scr_t * screen = gui_scr_stack_pop();
	if (screen == NULL) return;

	ui_load_scr_anim(ui, screen, anim_type, time, delay, is_clean, auto_del, is_push_satck);
}

gui_scr_t * ui_get_scr(int32_t scr_id)
{
	gui_scr_t * screen = gui_scr_get(scr_id);
	if (screen != NULL) return screen;

	if (scr_id == GUI_SCREEN_PAGE_MAP) return gui_scr_create(GUI_SCREEN_PAGE_MAP, "page_map", NULL, (gui_scr_setup_cb_t)setup_scr_page_map, (gui_scr_unload_cb_t)unload_scr_page_map);
	if (scr_id == GUI_SCREEN_PAGE_MUSIC) return gui_scr_create(GUI_SCREEN_PAGE_MUSIC, "page_music", NULL, (gui_scr_setup_cb_t)setup_scr_page_music, (gui_scr_unload_cb_t)unload_scr_page_music);
	if (scr_id == GUI_SCREEN_PAGE_APP_SELECT) return gui_scr_create(GUI_SCREEN_PAGE_APP_SELECT, "page_app_select", NULL, (gui_scr_setup_cb_t)setup_scr_page_app_select, (gui_scr_unload_cb_t)unload_scr_page_app_select);
	if (scr_id == GUI_SCREEN_PAGE_METER) return gui_scr_create(GUI_SCREEN_PAGE_METER, "page_meter", NULL, (gui_scr_setup_cb_t)setup_scr_page_meter, (gui_scr_unload_cb_t)unload_scr_page_meter);
	if (scr_id == GUI_SCREEN_PAGE_VIDEO) return gui_scr_create(GUI_SCREEN_PAGE_VIDEO, "page_video", NULL, (gui_scr_setup_cb_t)setup_scr_page_video, (gui_scr_unload_cb_t)unload_scr_page_video);
	if (scr_id == GUI_SCREEN_UPGRADE_PAGE) return gui_scr_create(GUI_SCREEN_UPGRADE_PAGE, "upgrade_page", NULL, (gui_scr_setup_cb_t)setup_scr_upgrade_page, (gui_scr_unload_cb_t)unload_scr_upgrade_page);
	if (scr_id == GUI_SCREEN_HARDWARE_TEST) return gui_scr_create(GUI_SCREEN_HARDWARE_TEST, "hardware_test", NULL, (gui_scr_setup_cb_t)setup_scr_hardware_test, (gui_scr_unload_cb_t)unload_scr_hardware_test);
	return NULL;
}

gui_scr_t * ui_get_setup_scr(int32_t scr_id)
{
	gui_scr_t * screen = ui_get_scr(scr_id);
	if (screen == NULL) return NULL;

	if (screen->scr == NULL || lv_obj_is_valid(screen->scr) == false) {
		if (screen->setup_cb != NULL) {
			screen->scr = screen->setup_cb(&guider_ui);
			screen->is_del = false;
			if (screen->scr == NULL || lv_obj_is_valid(screen->scr) == false) {
				screen->is_del = true;
				return NULL;
			}
		}
	}
	return screen;
}

lv_obj_t * ui_get_setup_scr_obj(int32_t scr_id)
{
	gui_scr_t * screen = ui_get_setup_scr(scr_id);
	if (screen == NULL) return NULL;

	return screen->scr;
}


void * ui_get_scr_ptr(lv_ui *ui, int32_t scr_id)
{
	if (ui == NULL) return NULL;

	if (scr_id == GUI_SCREEN_PAGE_MAP) return ui->page_map;
	if (scr_id == GUI_SCREEN_PAGE_MUSIC) return ui->page_music;
	if (scr_id == GUI_SCREEN_PAGE_APP_SELECT) return ui->page_app_select;
	if (scr_id == GUI_SCREEN_PAGE_METER) return ui->page_meter;
	if (scr_id == GUI_SCREEN_PAGE_VIDEO) return ui->page_video;
	if (scr_id == GUI_SCREEN_UPGRADE_PAGE) return ui->upgrade_page;
	if (scr_id == GUI_SCREEN_HARDWARE_TEST) return ui->hardware_test;
	return NULL;
}

void ui_free_scr_ptr(lv_ui *ui, int32_t scr_id)
{
	if (ui == NULL) return;

	if (scr_id == GUI_SCREEN_PAGE_MAP) {
		if (ui->page_map != NULL) lv_free(ui->page_map);
		ui->page_map = NULL;
	} else if (scr_id == GUI_SCREEN_PAGE_MUSIC) {
		if (ui->page_music != NULL) lv_free(ui->page_music);
		ui->page_music = NULL;
	} else if (scr_id == GUI_SCREEN_PAGE_APP_SELECT) {
		if (ui->page_app_select != NULL) lv_free(ui->page_app_select);
		ui->page_app_select = NULL;
	} else if (scr_id == GUI_SCREEN_PAGE_METER) {
		if (ui->page_meter != NULL) lv_free(ui->page_meter);
		ui->page_meter = NULL;
	} else if (scr_id == GUI_SCREEN_PAGE_VIDEO) {
		if (ui->page_video != NULL) lv_free(ui->page_video);
		ui->page_video = NULL;
	} else if (scr_id == GUI_SCREEN_UPGRADE_PAGE) {
		if (ui->upgrade_page != NULL) lv_free(ui->upgrade_page);
		ui->upgrade_page = NULL;
	} else if (scr_id == GUI_SCREEN_HARDWARE_TEST) {
		if (ui->hardware_test != NULL) lv_free(ui->hardware_test);
		ui->hardware_test = NULL;
	}
}

bool ui_is_act_scr(int32_t scr_id)
{
	return gui_scr_is_act(ui_get_scr(scr_id));
}

void ui_init_style(lv_style_t * style)
{
	if(style->prop_cnt > 0)
		lv_style_reset(style);
	else
		lv_style_init(style);
}


void setup_ui(lv_ui *ui)
{
	init_gui_fonts();
	ui_style_init();
#if LV_USE_OBSERVER
	gui_msg_init(ui);
#endif
	gui_group_init();
	ui_scr_manager_init();
	gui_scr_t *scr = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
	ui_load_scr_anim(ui, scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
	events_init(ui);
	gui_timelines_init(ui);
}
void clock_count_with_year(int *year, int *month, int *day, int *hour, int *min, int *sec)
{
	int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0)) {
		days_in_month[1] = 29;
	}
	(*sec)++;
	if (*sec >= 60) {
		*sec = 0;
		(*min)++;
	}
	if (*min >= 60) {
		*min = 0;
		(*hour)++;
	}
	if (*hour >= 24) {
		*hour = 0;
		(*day)++;
	}
	if (*day > days_in_month[*month - 1]) {
		*day = 1;
		(*month)++;
		if (*month > 12) {
			*month = 1;
			(*year)++;
		}
	}
}

