/*Generate Code, Do NOT Edit!*/
#include "./events_init.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include "lvgl.h"
#include "./callback_handler.h"
#include "../gui_timelines/gui_timelines.h"
#include "../gui_group/gui_group.h"

#if !LV_USE_GUIBUILDER_SIMULATOR
extern int map_jpeg_disp_frame_uninit(void);
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR
//#include "fs/fs.h"
extern int map_jpeg_disp_one_frame(u16 width, u16 height, u8 *buf, int len);
extern int map_jpeg_disp_frame_init(void);
extern void lv_example_lyrics_text_input(char *new_text);
static int init_flag = 1;
#endif

static lv_obj_t *page_map_simple_navi_view = NULL;
static int page_map_img_2_init_flag = 1;
int in_ui_navi_flag = 0; //只有ui在导航界面允许更新导航信息

int get_in_ui_navi_flag(void)
{
    return in_ui_navi_flag; 
}

void set_in_ui_navi_flag(int val)
{
    in_ui_navi_flag = val;  }

#if !LV_USE_GUIBUILDER_SIMULATOR
extern void logo_stop(void (*func)());
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR
extern int logo_show(char *logo_path, char *audio_path, int time_out, void (*func)());

char *lyrics_test = "lyrics one lyrics two";
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR
extern void logo_stop(void (*func)());
extern void lyrics_example_clean(void);
#endif
void scr_loaded_handler(lv_event_t *e)
{
	lv_obj_t * src = lv_event_get_target(e);
	gui_msg_init_ui();
	gui_msg_init_events();
}

void events_init(lv_ui *ui)
{
}

static void page_map_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_SCREEN_LOADED:
	{
		//custom code 
		{
			set_in_ui_navi_flag(1);
			if (page_map_img_2_init_flag){
			  lv_ui_page_map * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
			  lv_obj_clear_flag(ui_scr->page_map_img_2 , LV_OBJ_FLAG_HIDDEN);
			  page_map_img_2_init_flag = 0;
			}
			
			#if !LV_USE_GUIBUILDER_SIMULATOR
			
			if (init_flag){
			  lv_example_lyrics_text_input(lyrics_test);
			  init_flag = 0;
			}
			
			page_map_simple_navi_view = get_page_map_view_3_in_map();
			if (page_map_simple_navi_view) {
			    lv_obj_clear_flag(page_map_simple_navi_view, LV_OBJ_FLAG_HIDDEN);
			}
			
			#endif
		}
	}
	break;
	case LV_EVENT_SCREEN_UNLOADED:
	{
		//custom code 
		{
			#if !LV_USE_GUIBUILDER_SIMULATOR
			page_map_simple_navi_view = get_page_map_view_3_in_map();
			if (page_map_simple_navi_view) {
			      lv_obj_add_flag(page_map_simple_navi_view, LV_OBJ_FLAG_HIDDEN);
			}
			#endif
		}
	}
	break;
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
		if(dir & LV_DIR_LEFT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_METER);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
			}
		}
		if(dir & LV_DIR_LEFT)
		{
			//custom code page_meter
			{
				#if !LV_USE_GUIBUILDER_SIMULATOR
				set_in_ui_navi_flag(0);
				//map_jpeg_disp_frame_uninit();
				#endif
			}
		}
	}
	break;
	default:
		break;
	}
}

static void page_map_btn_2_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_METER);
		if(screen != NULL) {
			ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
		}
		//custom code page_meter
		{
			set_in_ui_navi_flag(0);
		}
	}
	break;
	default:
		break;
	}
}
static void page_map_screen_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
		case LV_EVENT_SCREEN_LOAD_START:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_MAP, GUI_SCREEN_ACTION_LOAD);
			break;
		}
		case LV_EVENT_SCREEN_UNLOADED:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_MAP, GUI_SCREEN_ACTION_UNLOAD);
			break;
		}
	}
}

void events_init_page_map(lv_ui *ui)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
	lv_obj_add_event_cb(ui_scr->page_map, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_map, page_map_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_map_btn_2, page_map_btn_2_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_map, page_map_screen_event_handler, LV_EVENT_ALL, ui);
}

static void page_meter_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_SCREEN_LOADED:
	{
		//custom code 
		{
			#if !LV_USE_GUIBUILDER_SIMULATOR
			//logo_show("storage/sd0/C/avi/moto.avi", NULL, 0Xffff, NULL);
			//logo_show(CONFIG_AVI_PATH, NULL, 0Xffff, NULL);
			#endif
			
		}
	}
	break;
	case LV_EVENT_SCREEN_UNLOADED:
	{
		//custom code 
		{
			#if !LV_USE_GUIBUILDER_SIMULATOR
			//logo_stop(NULL);
			//lyrics_example_clean();
			#endif
		}
	}
	break;
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
		if(dir & LV_DIR_RIGHT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_MAP);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true, false);
			}
		}
		if(dir & LV_DIR_RIGHT)
		{
			//custom code page_map
			{
				#if !LV_USE_GUIBUILDER_SIMULATOR
				//logo_stop();
				#endif
			}
		}
		if(dir & LV_DIR_LEFT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_MUSIC);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
			}
		}
	}
	break;
	default:
		break;
	}
}

static void page_meter_btn_1_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_MAP);
		if(screen != NULL) {
			ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
		}
	}
	break;
	default:
		break;
	}
}

static void page_meter_btn_2_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_MUSIC);
		if(screen != NULL) {
			ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
		}
	}
	break;
	default:
		break;
	}
}

void events_init_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	lv_obj_add_event_cb(ui_scr->page_meter, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_meter, page_meter_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_meter_btn_1, page_meter_btn_1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_meter_btn_2, page_meter_btn_2_event_handler, LV_EVENT_ALL, ui);
}

static void page_music_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
		if(dir & LV_DIR_RIGHT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_METER);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
			}
		}
	}
	break;
	default:
		break;
	}
}

static void page_music_btn_1_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_METER);
		if(screen != NULL) {
			ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true, false);
		}
	}
	break;
	default:
		break;
	}
}

void events_init_page_music(lv_ui *ui)
{
	lv_ui_page_music * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
	lv_obj_add_event_cb(ui_scr->page_music, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_music, page_music_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_music_btn_1, page_music_btn_1_event_handler, LV_EVENT_ALL, ui);
}
