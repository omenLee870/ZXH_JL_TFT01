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


lv_obj_t * setup_scr_page_video(lv_ui *ui)
{
	lv_ui_page_video * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_VIDEO);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_video));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_video));
		ui->page_video = ui_scr;
	}

	//Write codes page_video
	ui_scr->page_video = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_video, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_video, GUI_CTRL_SCR);

	//Write codes page_video_img_1
	ui_scr->page_video_img_1 = lv_image_create(ui_scr->page_video);
	lv_image_set_src(ui_scr->page_video_img_1, gui_get_res_path(GUI_RES_ASTERN_PNG));
	lv_image_set_pivot(ui_scr->page_video_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_video_img_1, 0);
	lv_image_set_scale(ui_scr->page_video_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_video_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_video_img_1, 141, 387);
	lv_obj_set_size(ui_scr->page_video_img_1, 742, 208);
	lv_obj_add_flag(ui_scr->page_video_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_video_img_1, GUI_CTRL_IMG);

	lv_obj_update_layout(ui_scr->page_video);
	ui_scr->page_video_del = false;

	//Init events for screen
	events_init_page_video(ui);
	return ui_scr->page_video;
}
void unload_scr_page_video(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_VIDEO);
}
