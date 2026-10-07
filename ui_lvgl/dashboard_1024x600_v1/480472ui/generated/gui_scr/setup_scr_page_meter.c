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

static const char* page_meter_imglist_1_imgs[22] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00000.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00001.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00002.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00003.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00004.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00005.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00006.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00007.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00010.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00011.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00012.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00013.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00014.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00015.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00016.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00017.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00020.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00021.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00022.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00023.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00024.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00025.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00007b.zip",
	"storage/res_nor_mode/C/lvgl/4b00007c.zip",
	"storage/res_nor_mode/C/lvgl/4b00007d.zip",
	"storage/res_nor_mode/C/lvgl/4b00007e.zip",
	"storage/res_nor_mode/C/lvgl/4b00007f.zip",
	"storage/res_nor_mode/C/lvgl/4b000080.zip",
	"storage/res_nor_mode/C/lvgl/4b000081.zip",
	"storage/res_nor_mode/C/lvgl/4b000082.zip",
	"storage/res_nor_mode/C/lvgl/4b000083.zip",
	"storage/res_nor_mode/C/lvgl/4b000084.zip",
	"storage/res_nor_mode/C/lvgl/4b000085.zip",
	"storage/res_nor_mode/C/lvgl/4b000086.zip",
	"storage/res_nor_mode/C/lvgl/4b000087.zip",
	"storage/res_nor_mode/C/lvgl/4b000088.zip",
	"storage/res_nor_mode/C/lvgl/4b000089.zip",
	"storage/res_nor_mode/C/lvgl/4b00008a.zip",
	"storage/res_nor_mode/C/lvgl/4b00008b.zip",
	"storage/res_nor_mode/C/lvgl/4b00008c.zip",
	"storage/res_nor_mode/C/lvgl/4b00008d.zip",
	"storage/res_nor_mode/C/lvgl/4b00008e.zip",
	"storage/res_nor_mode/C/lvgl/4b00008f.zip",
	"storage/res_nor_mode/C/lvgl/4b000090.zip"
#endif
};
static const char* page_meter_imglist_2_imgs[10] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_0.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_1.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_2.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_3.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_4.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_5.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_6.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_7.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_8.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_9.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000091.zip",
	"storage/res_nor_mode/C/lvgl/4b000092.zip",
	"storage/res_nor_mode/C/lvgl/4b000093.zip",
	"storage/res_nor_mode/C/lvgl/4b000094.zip",
	"storage/res_nor_mode/C/lvgl/4b000095.zip",
	"storage/res_nor_mode/C/lvgl/4b000096.zip",
	"storage/res_nor_mode/C/lvgl/4b000097.zip",
	"storage/res_nor_mode/C/lvgl/4b000098.zip",
	"storage/res_nor_mode/C/lvgl/4b000099.zip",
	"storage/res_nor_mode/C/lvgl/4b00009a.zip"
#endif
};
static const char* page_meter_imglist_3_imgs[10] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_0.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_1.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_2.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_3.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_4.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_5.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_6.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_7.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_8.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_9.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000091.zip",
	"storage/res_nor_mode/C/lvgl/4b000092.zip",
	"storage/res_nor_mode/C/lvgl/4b000093.zip",
	"storage/res_nor_mode/C/lvgl/4b000094.zip",
	"storage/res_nor_mode/C/lvgl/4b000095.zip",
	"storage/res_nor_mode/C/lvgl/4b000096.zip",
	"storage/res_nor_mode/C/lvgl/4b000097.zip",
	"storage/res_nor_mode/C/lvgl/4b000098.zip",
	"storage/res_nor_mode/C/lvgl/4b000099.zip",
	"storage/res_nor_mode/C/lvgl/4b00009a.zip"
#endif
};

lv_obj_t * setup_scr_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_meter));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_meter));
		ui->page_meter = ui_scr;
	}

	//Write codes page_meter
	ui_scr->page_meter = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter, GUI_CTRL_SCR);

	//Write codes page_meter_imglist_1
	ui_scr->page_meter_imglist_1 = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_1, 0, 0);
	lv_obj_set_size(ui_scr->page_meter_imglist_1, 480, 272);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_1, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_1, (const void **) page_meter_imglist_1_imgs, 22);
	lv_imglist_set_act(ui_scr->page_meter_imglist_1, 0);

	// Write Timer page_meter_timer_1
	if(ui_scr->page_meter_timer_1 != NULL) lv_timer_delete(ui_scr->page_meter_timer_1);
	ui_scr->page_meter_timer_1 = lv_timer_create(page_meter_timer_1_timer_cb, 30, "");
	lv_timer_set_repeat_count(ui_scr->page_meter_timer_1, -1);

	//Write codes page_meter_imglist_2
	ui_scr->page_meter_imglist_2 = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_2, 203, 95);
	lv_obj_set_size(ui_scr->page_meter_imglist_2, 34, 34);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_2, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_2, (const void **) page_meter_imglist_2_imgs, 10);
	lv_imglist_set_act(ui_scr->page_meter_imglist_2, 0);

	//Write codes page_meter_imglist_3
	ui_scr->page_meter_imglist_3 = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_3, 245, 95);
	lv_obj_set_size(ui_scr->page_meter_imglist_3, 34, 34);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_3, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_3, (const void **) page_meter_imglist_3_imgs, 10);
	lv_imglist_set_act(ui_scr->page_meter_imglist_3, 0);

	//Write codes page_meter_img_1
	ui_scr->page_meter_img_1 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_1, gui_get_res_path(GUI_RES_READYC_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_1, 0);
	lv_image_set_scale(ui_scr->page_meter_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_1, 221, 151);
	lv_obj_set_size(ui_scr->page_meter_img_1, 46, 9);
	lv_obj_add_flag(ui_scr->page_meter_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_1, GUI_CTRL_IMG);

	//Write codes page_meter_img_2
	ui_scr->page_meter_img_2 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_2, gui_get_res_path(GUI_RES_LAMP_02_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_2, 0);
	lv_image_set_scale(ui_scr->page_meter_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_2, 74, 196);
	lv_obj_set_size(ui_scr->page_meter_img_2, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_2, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_2, GUI_CTRL_IMG);

	//Write codes page_meter_img_3
	ui_scr->page_meter_img_3 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_3, gui_get_res_path(GUI_RES_FREE_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_3, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_3, 0);
	lv_image_set_scale(ui_scr->page_meter_img_3, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_3, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_3, 34, 83);
	lv_obj_set_size(ui_scr->page_meter_img_3, 50, 18);
	lv_obj_add_flag(ui_scr->page_meter_img_3, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_3, GUI_CTRL_IMG);

	//Write codes page_meter_img_4
	ui_scr->page_meter_img_4 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_4, gui_get_res_path(GUI_RES_LAMP_06_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_4, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_4, 0);
	lv_image_set_scale(ui_scr->page_meter_img_4, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_4, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_4, 47, 158);
	lv_obj_set_size(ui_scr->page_meter_img_4, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_4, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_4, GUI_CTRL_IMG);

	//Write codes page_meter_img_5
	ui_scr->page_meter_img_5 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_5, gui_get_res_path(GUI_RES_LAMP_03_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_5, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_5, 0);
	lv_image_set_scale(ui_scr->page_meter_img_5, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_5, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_5, 69, 41);
	lv_obj_set_size(ui_scr->page_meter_img_5, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_5, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_5, GUI_CTRL_IMG);

	//Write codes page_meter_img_6
	ui_scr->page_meter_img_6 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_6, gui_get_res_path(GUI_RES_LAMP_07_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_6, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_6, 0);
	lv_image_set_scale(ui_scr->page_meter_img_6, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_6, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_6, 26, 120);
	lv_obj_set_size(ui_scr->page_meter_img_6, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_6, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_6, GUI_CTRL_IMG);

	//Write codes page_meter_img_7
	ui_scr->page_meter_img_7 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_7, gui_get_res_path(GUI_RES_LAMP_17_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_7, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_7, 0);
	lv_image_set_scale(ui_scr->page_meter_img_7, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_7, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_7, 29, 228);
	lv_obj_set_size(ui_scr->page_meter_img_7, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_7, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_7, GUI_CTRL_IMG);

	//Write codes page_meter_img_8
	ui_scr->page_meter_img_8 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_8, gui_get_res_path(GUI_RES_LAMP_01_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_8, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_8, 0);
	lv_image_set_scale(ui_scr->page_meter_img_8, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_8, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_8, 29, 17);
	lv_obj_set_size(ui_scr->page_meter_img_8, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_8, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_8, GUI_CTRL_IMG);

	//Write codes page_meter_img_9
	ui_scr->page_meter_img_9 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_9, gui_get_res_path(GUI_RES_LAMP_04_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_9, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_9, 0);
	lv_image_set_scale(ui_scr->page_meter_img_9, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_9, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_9, 421, 17);
	lv_obj_set_size(ui_scr->page_meter_img_9, 24, 23);
	lv_obj_add_flag(ui_scr->page_meter_img_9, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_9, GUI_CTRL_IMG);

	//Write codes page_meter_img_10
	ui_scr->page_meter_img_10 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_10, gui_get_res_path(GUI_RES_GEAR_1_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_10, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_10, 0);
	lv_image_set_scale(ui_scr->page_meter_img_10, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_10, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_10, 257, 231);
	lv_obj_set_size(ui_scr->page_meter_img_10, 17, 16);
	lv_obj_add_flag(ui_scr->page_meter_img_10, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_10, GUI_CTRL_IMG);

	//Write codes page_meter_img_11
	ui_scr->page_meter_img_11 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_11, gui_get_res_path(GUI_RES_GEAR_1_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_11, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_11, 0);
	lv_image_set_scale(ui_scr->page_meter_img_11, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_11, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_11, 241, 231);
	lv_obj_set_size(ui_scr->page_meter_img_11, 17, 16);
	lv_obj_add_flag(ui_scr->page_meter_img_11, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_11, GUI_CTRL_IMG);

	//Write codes page_meter_img_12
	ui_scr->page_meter_img_12 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_12, gui_get_res_path(GUI_RES_GEAR_1_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_12, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_12, 0);
	lv_image_set_scale(ui_scr->page_meter_img_12, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_12, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_12, 226, 231);
	lv_obj_set_size(ui_scr->page_meter_img_12, 17, 16);
	lv_obj_add_flag(ui_scr->page_meter_img_12, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_12, GUI_CTRL_IMG);

	//Write codes page_meter_img_13
	ui_scr->page_meter_img_13 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_13, gui_get_res_path(GUI_RES_GEAR_1_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_13, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_13, 0);
	lv_image_set_scale(ui_scr->page_meter_img_13, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_13, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_13, 212, 231);
	lv_obj_set_size(ui_scr->page_meter_img_13, 17, 16);
	lv_obj_add_flag(ui_scr->page_meter_img_13, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_13, GUI_CTRL_IMG);

	//Write codes page_meter_img_14
	ui_scr->page_meter_img_14 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_14, gui_get_res_path(GUI_RES_MILEAGE_KM_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_14, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_14, 0);
	lv_image_set_scale(ui_scr->page_meter_img_14, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_14, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_14, 275, 237);
	lv_obj_set_size(ui_scr->page_meter_img_14, 9, 9);
	lv_obj_add_flag(ui_scr->page_meter_img_14, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_14, GUI_CTRL_IMG);

	//Write codes page_meter_img_15
	ui_scr->page_meter_img_15 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_15, gui_get_res_path(GUI_RES_VOLTAGE_72V_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_15, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_15, 0);
	lv_image_set_scale(ui_scr->page_meter_img_15, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_15, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_15, 118, 238);
	lv_obj_set_size(ui_scr->page_meter_img_15, 36, 15);
	lv_obj_add_flag(ui_scr->page_meter_img_15, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_15, GUI_CTRL_IMG);

	//Write codes page_meter_img_16
	ui_scr->page_meter_img_16 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_16, gui_get_res_path(GUI_RES_LOGO_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_16, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_16, 0);
	lv_image_set_scale(ui_scr->page_meter_img_16, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_16, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_16, 215, 205);
	lv_obj_set_size(ui_scr->page_meter_img_16, 43, 10);
	lv_obj_add_flag(ui_scr->page_meter_img_16, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_16, GUI_CTRL_IMG);

	//Write codes page_meter_img_17
	ui_scr->page_meter_img_17 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_17, gui_get_res_path(GUI_RES_MILEAGE_TRIP_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_17, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_17, 0);
	lv_image_set_scale(ui_scr->page_meter_img_17, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_17, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_17, 197, 231);
	lv_obj_set_size(ui_scr->page_meter_img_17, 13, 9);
	lv_obj_add_flag(ui_scr->page_meter_img_17, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_17, GUI_CTRL_IMG);

	//Write codes page_meter_img_18
	ui_scr->page_meter_img_18 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_18, gui_get_res_path(GUI_RES_SINGER100_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_18, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_18, 0);
	lv_image_set_scale(ui_scr->page_meter_img_18, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_18, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_18, 395, 108);
	lv_obj_set_size(ui_scr->page_meter_img_18, 60, 57);
	lv_obj_add_flag(ui_scr->page_meter_img_18, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_18, GUI_CTRL_IMG);

	//Write codes page_meter_btn_1
	ui_scr->page_meter_btn_1 = lv_btn_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_btn_1, 92, 6);
	lv_obj_set_size(ui_scr->page_meter_btn_1, 35, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_btn_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_btn_1, GUI_CTRL_BUTTON);
	ui_scr->page_meter_btn_1_label = lv_label_create(ui_scr->page_meter_btn_1);
	lv_label_set_text(ui_scr->page_meter_btn_1_label, "back");
	lv_obj_set_style_pad_all(ui_scr->page_meter_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_meter_btn_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_meter_btn_2
	ui_scr->page_meter_btn_2 = lv_btn_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_btn_2, 131, 6);
	lv_obj_set_size(ui_scr->page_meter_btn_2, 35, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_btn_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_btn_2, GUI_CTRL_BUTTON);
	ui_scr->page_meter_btn_2_label = lv_label_create(ui_scr->page_meter_btn_2);
	lv_label_set_text(ui_scr->page_meter_btn_2_label, "next");
	lv_obj_set_style_pad_all(ui_scr->page_meter_btn_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_meter_btn_2_label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_update_layout(ui_scr->page_meter);
	ui_scr->page_meter_del = false;

	//Init events for screen
	events_init_page_meter(ui);
	return ui_scr->page_meter;
}
void unload_scr_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	if(ui_scr->page_meter_timer_1 != NULL) {
		lv_timer_delete(ui_scr->page_meter_timer_1);
		ui_scr->page_meter_timer_1 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
}
