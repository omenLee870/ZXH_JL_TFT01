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
#include "../../custom/hardware_test.h"
#if 0//!LV_USE_GUIBUILDER_SIMULATOR
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
#if 0//!LV_USE_GUIBUILDER_SIMULATOR
extern int map_jpeg_disp_frame_uninit(void);
#endif

#include "ui_meter_settings.h"
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
	case LV_EVENT_SCREEN_UNLOAD_START:
	{
		//custom code 
		{
			#if !LV_USE_GUIBUILDER_SIMULATOR
			extern void map_video_leave(void);
			map_video_leave();
			#endif
		}
	}
	break;
	case LV_EVENT_SCREEN_LOADED:
	{
		//custom code 
		{
			#if !LV_USE_GUIBUILDER_SIMULATOR
			extern void map_video_enter(void);
			map_video_enter();
			#endif
		}
	}
	break;
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
		if(dir & LV_DIR_RIGHT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
			}
		}
		if(dir & LV_DIR_RIGHT)
		{
			//custom code page_app_select
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
	lv_obj_add_event_cb(ui_scr->page_map, page_map_screen_event_handler, LV_EVENT_ALL, ui);
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
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
			}
		}
		if(dir & LV_DIR_RIGHT)
		{
		}
	}
	break;
	default:
		break;
	}
}

static void page_music_img_4_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_get_event_cb(RES_EVENT_MUSIC_PREV_ID)(e);
	}
	break;
	default:
		break;
	}
}

static void page_music_img_5_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_get_event_cb(RES_EVENT_MUSIC_NEXT_ID)(e);
	}
	break;
	default:
		break;
	}
}

static void page_music_imgbtn_1_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		gui_get_event_cb(RES_EVENT_MUSIC_PAUSE_PLAY_ID)(e);
	}
	break;
	default:
		break;
	}
}
static void page_music_screen_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
		case LV_EVENT_SCREEN_LOAD_START:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_MUSIC, GUI_SCREEN_ACTION_LOAD);
			break;
		}
		case LV_EVENT_SCREEN_UNLOADED:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_MUSIC, GUI_SCREEN_ACTION_UNLOAD);
			break;
		}
	}
}

void events_init_page_music(lv_ui *ui)
{
	lv_ui_page_music * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MUSIC);
	lv_obj_add_event_cb(ui_scr->page_music, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_music, page_music_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_music_img_4, page_music_img_4_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_music_img_5, page_music_img_5_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_music_imgbtn_1, page_music_imgbtn_1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_music, page_music_screen_event_handler, LV_EVENT_ALL, ui);
}

static void page_app_select_btn_back_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code hardware_test
		{
			hw_input_focus_release();   /* 从 select 进入 test 流程: 清残留焦点 */
			gui_scr_t *screen = ui_get_scr(GUI_SCREEN_HARDWARE_TEST);
			if (screen != NULL) {
			    ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
			}
		}
	}
	break;
	default:
		break;
	}
}

static void appicon_item_event_handler(lv_event_t *e)
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

static void appicon_item1_event_handler(lv_event_t *e)
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

static void appicon_item2_event_handler(lv_event_t *e)
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
static void page_app_select_screen_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
		case LV_EVENT_SCREEN_LOAD_START:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_APP_SELECT, GUI_SCREEN_ACTION_LOAD);
			break;
		}
		case LV_EVENT_SCREEN_UNLOADED:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_APP_SELECT, GUI_SCREEN_ACTION_UNLOAD);
			break;
		}
	}
}

void events_init_page_app_select(lv_ui *ui)
{
	lv_ui_page_app_select * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_APP_SELECT);
	lv_obj_add_event_cb(ui_scr->page_app_select, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_app_select_btn_back, page_app_select_btn_back_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_app_select_arcmenu_1_item, appicon_item_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_app_select_arcmenu_1_item1, appicon_item1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_app_select_arcmenu_1_item2, appicon_item2_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_app_select, page_app_select_screen_event_handler, LV_EVENT_ALL, ui);
}

static void page_meter_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_GESTURE:
	{
		lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
		if(dir & LV_DIR_RIGHT)
		{
			gui_scr_t * screen = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
			if(screen != NULL) {
				ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
			}
		}
	}
	break;
	default:
		break;
	}
}
static void page_meter_screen_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
		case LV_EVENT_SCREEN_LOAD_START:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_METER, GUI_SCREEN_ACTION_LOAD);
			break;
		}
		case LV_EVENT_SCREEN_UNLOADED:
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_METER, GUI_SCREEN_ACTION_UNLOAD);
			break;
		}
	}
}

void events_init_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	lv_obj_add_event_cb(ui_scr->page_meter, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_meter, page_meter_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->page_meter, page_meter_screen_event_handler, LV_EVENT_ALL, ui);
}

static void page_video_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_SCREEN_UNLOAD_START:
	{
		//custom code 
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_VIDEO, GUI_SCREEN_ACTION_UNLOAD);
		}
	}
	break;
	case LV_EVENT_SCREEN_LOADED:
	{
		//custom code 
		{
			gui_scr_action_cb(GUI_SCREEN_PAGE_VIDEO, GUI_SCREEN_ACTION_LOAD);
		}
	}
	break;
	default:
		break;
	}
}

void events_init_page_video(lv_ui *ui)
{
	lv_ui_page_video * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_VIDEO);
	lv_obj_add_event_cb(ui_scr->page_video, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->page_video, page_video_event_handler, LV_EVENT_ALL, ui);
}

static void hardware_test_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_SCREEN_LOADED:
	{
		//custom code hardware_test_edit_1
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_hardware_test * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			lv_obj_t * dest = ui_scr->hardware_test_edit_1;
			lv_obj_set_style_text_align(dest, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_lte_4g_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_LTE) != 0) {
			  break;   /* 重复点击当前运行项: 忽略 */
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "PLEASE INSERT SIM\nTEST RUNNING\nGET baidu.com", 0);
			lv_refr_now(NULL);
			/* 异步: 4G http GET(最长 30s 超时)在后台 worker 执行, 完成后经 LVGL
			  * 周期定时器刷新 SUCCESS/FAILED, 避免阻塞 UI 线程 */
			int ret = hardware_test_lte_4g_start();
			if (ret != 0) {
			    hardware_test_item_done();
			    hardware_test_show_result(ui_scr->hardware_test_edit_1, "4G START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_5_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_WIFI) != 0) {
			    break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "USER ACTION\nConnect to\nJL380N-P2P", 0);
			lv_refr_now(NULL);
			int ret = hardware_test_wifi_start();
			if (ret != 0) {
			    hardware_test_show_result(ui_scr->hardware_test_edit_1, "WIFI START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_bt_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			hardware_test_switch_item(HW_TEST_BT);
			char text[96];
			const char *bt_name = hardware_test_bt_get_name();
			if (bt_name == NULL || bt_name[0] == '\0') {
			    bt_name = "Unknown";
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			lv_snprintf(text, sizeof(text), "USER ACTION\nConnect Bluetooth:\n%s", bt_name);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, text, 0);
			hardware_test_item_done();
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_gsensor_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_GSENSOR) != 0) {
			    break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "TEST RUNNING\nReading G-sensor", 0);
			lv_refr_now(NULL);
			int ret = hardware_test_gsensor_start();
			if (ret != 0) {
			    hardware_test_show_result(ui_scr->hardware_test_edit_1, "GSENSOR START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_7_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_ADKEY) != 0) {
			    break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "USER ACTION\nPress an ADKEY", 0);
			hardware_test_adkey_start();
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_ui_effect_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_switch_item(HW_TEST_NONE);   /* 退出测试屏: 停掉当前活动项 */
			hardware_test_hide_message(ui_scr->hardware_test_edit_1);
			hw_input_focus_release();   /* 离开 test 流程回正常 UI: 清 test 屏积累的残留焦点 */
			gui_scr_t *screen = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
			if (screen != NULL) {
			    ui_load_scr_anim(&guider_ui, screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_can_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_CAN) != 0) {
			    break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "USER ACTION\nConnect CAN analyzer\nBaud: 1000000", 0);
			hardware_test_wifi_stop_wait(2000);
			int ret = hardware_test_can_start();
			if (ret != 0) {
			    hardware_test_show_result(ui_scr->hardware_test_edit_1, "CAN START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_4_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_GPS) != 0) {
			  break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "TEST RUNNING\nWaiting for GPS signal", 0);
			int ret = hardware_test_gps_start();
			if (ret != 0) {
			  hardware_test_show_result(ui_scr->hardware_test_edit_1, "GPS START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_mic0_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_MIC1) != 0) {
			  break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "TEST RUNNING\nSpeak to MIC1", 0);
			if (hardware_test_mic_start(0) != 0) {
			  hardware_test_item_done();
			  hardware_test_show_result(ui_scr->hardware_test_edit_1, "MIC1 START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

static void hardware_test_btn_6_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			if (hardware_test_switch_item(HW_TEST_MIC2) != 0) {
			  break;
			}
			lv_ui *ui = lv_event_get_user_data(e);
			lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
			hardware_test_show_message(ui_scr->hardware_test_edit_1, "TEST RUNNING\nSpeak to MIC2", 0);
			if (hardware_test_mic_start(1) != 0) {
			  hardware_test_item_done();
			  hardware_test_show_result(ui_scr->hardware_test_edit_1, "MIC2 START FAILED");
			}
		}
	}
	break;
	default:
		break;
	}
}

void events_init_hardware_test(lv_ui *ui)
{
	lv_ui_hardware_test * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_HARDWARE_TEST);
	lv_obj_add_event_cb(ui_scr->hardware_test, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test, hardware_test_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_lte_4g, hardware_test_lte_4g_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_5, hardware_test_btn_5_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_bt, hardware_test_btn_bt_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_gsensor, hardware_test_btn_gsensor_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_7, hardware_test_btn_7_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_ui_effect, hardware_test_btn_ui_effect_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_can, hardware_test_can_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_4, hardware_test_btn_4_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_mic0, hardware_test_mic0_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->hardware_test_btn_6, hardware_test_btn_6_event_handler, LV_EVENT_ALL, ui);
}

static void chongwuyibiao_settings_entry_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_chongwuyibiao * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_CHONGWUYIBIAO);
			ui_meter_settings_open();
		}
	}
	break;
	default:
		break;
	}
}

void events_init_chongwuyibiao(lv_ui *ui)
{
	lv_ui_chongwuyibiao * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_CHONGWUYIBIAO);
	lv_obj_add_event_cb(ui_scr->chongwuyibiao, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->chongwuyibiao_settings_entry, chongwuyibiao_settings_entry_event_handler, LV_EVENT_ALL, ui);
}

static void meter_settings_back_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_close();
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_brightness_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_RELEASED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(0, lv_slider_get_value(src));
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_music_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_RELEASED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(1, lv_slider_get_value(src));
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_tone_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_RELEASED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(2, lv_slider_get_value(src));
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_key_tone_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_VALUE_CHANGED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(3, lv_obj_has_state(src, LV_STATE_CHECKED));
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_unit_km_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(4,0);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_unit_mi_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(4,1);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_trip_clear_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_confirm(6);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_sleep_0_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(5,0);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_sleep_1_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(5,1);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_sleep_3_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(5,3);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_sleep_5_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_change(5,5);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_restore_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_confirm(7);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_confirm_cancel_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_cancel();
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_confirm_ok_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_commit();
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_0_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(0);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_1_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(1);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_2_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(2);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_3_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(3);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_4_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(4);
		}
	}
	break;
	default:
		break;
	}
}

static void meter_settings_menu_5_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//custom code 
		{
			lv_obj_t * src = lv_event_get_target(e);
			lv_ui * ui = lv_event_get_user_data(e);
			lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
			ui_meter_settings_select(5);
		}
	}
	break;
	default:
		break;
	}
}

void events_init_meter_settings(lv_ui *ui)
{
	lv_ui_meter_settings * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_METER_SETTINGS);
	lv_obj_add_event_cb(ui_scr->meter_settings, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_back, meter_settings_back_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_brightness, meter_settings_brightness_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_music, meter_settings_music_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_tone, meter_settings_tone_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_key_tone, meter_settings_key_tone_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_unit_km, meter_settings_unit_km_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_unit_mi, meter_settings_unit_mi_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_trip_clear, meter_settings_trip_clear_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_sleep_0, meter_settings_sleep_0_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_sleep_1, meter_settings_sleep_1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_sleep_3, meter_settings_sleep_3_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_sleep_5, meter_settings_sleep_5_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_restore, meter_settings_restore_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_confirm_cancel, meter_settings_confirm_cancel_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_confirm_ok, meter_settings_confirm_ok_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_0, meter_settings_menu_0_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_1, meter_settings_menu_1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_2, meter_settings_menu_2_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_3, meter_settings_menu_3_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_4, meter_settings_menu_4_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui_scr->meter_settings_menu_5, meter_settings_menu_5_event_handler, LV_EVENT_ALL, ui);
}

void events_init_upgrade_page(lv_ui *ui)
{
	lv_ui_upgrade_page * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_UPGRADE_PAGE);
	lv_obj_add_event_cb(ui_scr->upgrade_page, scr_loaded_handler, LV_EVENT_SCREEN_LOADED, ui);
}
