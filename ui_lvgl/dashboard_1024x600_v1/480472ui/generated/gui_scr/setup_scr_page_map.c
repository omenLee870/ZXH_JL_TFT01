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

static const char* page_map_imgList_gear_mode_imgs[3] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\SPORT.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\TTFAR.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\FREE.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000004.zip",
	"storage/res_nor_mode/C/lvgl/4b000005.zip",
	"storage/res_nor_mode/C/lvgl/4b000006.zip"
#endif
};
static const char* page_map_imgList_bluetooth_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_25.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_26.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000008.zip",
	"storage/res_nor_mode/C/lvgl/4b000009.zip"
#endif
};
static const char* page_map_imgList_phone_bl_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_23.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_24.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00000a.zip",
	"storage/res_nor_mode/C/lvgl/4b00000b.zip"
#endif
};
static const char* page_map_imgList_gps_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_29.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_30.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00000c.zip",
	"storage/res_nor_mode/C/lvgl/4b00000d.zip"
#endif
};
static const char* page_map_imgList_networksignal_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_27.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_28.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00000e.zip",
	"storage/res_nor_mode/C/lvgl/4b00000f.zip"
#endif
};
static const char* page_map_imgList_soc_beam_imgs[3] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_20.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_21.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_22.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000012.zip",
	"storage/res_nor_mode/C/lvgl/4b000013.zip",
	"storage/res_nor_mode/C/lvgl/4b000014.zip"
#endif
};
static const char* page_map_imglist_bat_imgs[100] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_001.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_002.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_003.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_004.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_005.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_006.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_007.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_008.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_009.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_010.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_011.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_012.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_013.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_014.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_015.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_016.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_017.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_018.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_019.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_020.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_021.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_022.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_023.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_024.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_025.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_026.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_027.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_028.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_029.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_030.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_031.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_032.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_033.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_034.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_035.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_036.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_037.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_038.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_039.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_040.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_041.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_042.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_043.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_044.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_045.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_046.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_047.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_048.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_049.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_050.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_051.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_052.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_053.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_054.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_055.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_056.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_057.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_058.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_059.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_060.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_061.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_062.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_063.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_064.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_065.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_066.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_067.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_068.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_069.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_070.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_071.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_072.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_073.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_074.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_075.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_076.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_077.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_078.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_079.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_080.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_081.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_082.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_083.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_084.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_085.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_086.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_087.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_088.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_089.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_090.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_091.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_092.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_093.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_094.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_095.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_096.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_097.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_098.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_099.png",
	"A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_100.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000015.zip",
	"storage/res_nor_mode/C/lvgl/4b000016.zip",
	"storage/res_nor_mode/C/lvgl/4b000017.zip",
	"storage/res_nor_mode/C/lvgl/4b000018.zip",
	"storage/res_nor_mode/C/lvgl/4b000019.zip",
	"storage/res_nor_mode/C/lvgl/4b00001a.zip",
	"storage/res_nor_mode/C/lvgl/4b00001b.zip",
	"storage/res_nor_mode/C/lvgl/4b00001c.zip",
	"storage/res_nor_mode/C/lvgl/4b00001d.zip",
	"storage/res_nor_mode/C/lvgl/4b00001e.zip",
	"storage/res_nor_mode/C/lvgl/4b00001f.zip",
	"storage/res_nor_mode/C/lvgl/4b000020.zip",
	"storage/res_nor_mode/C/lvgl/4b000021.zip",
	"storage/res_nor_mode/C/lvgl/4b000022.zip",
	"storage/res_nor_mode/C/lvgl/4b000023.zip",
	"storage/res_nor_mode/C/lvgl/4b000024.zip",
	"storage/res_nor_mode/C/lvgl/4b000025.zip",
	"storage/res_nor_mode/C/lvgl/4b000026.zip",
	"storage/res_nor_mode/C/lvgl/4b000027.zip",
	"storage/res_nor_mode/C/lvgl/4b000028.zip",
	"storage/res_nor_mode/C/lvgl/4b000029.zip",
	"storage/res_nor_mode/C/lvgl/4b00002a.zip",
	"storage/res_nor_mode/C/lvgl/4b00002b.zip",
	"storage/res_nor_mode/C/lvgl/4b00002c.zip",
	"storage/res_nor_mode/C/lvgl/4b00002d.zip",
	"storage/res_nor_mode/C/lvgl/4b00002e.zip",
	"storage/res_nor_mode/C/lvgl/4b00002f.zip",
	"storage/res_nor_mode/C/lvgl/4b000030.zip",
	"storage/res_nor_mode/C/lvgl/4b000031.zip",
	"storage/res_nor_mode/C/lvgl/4b000032.zip",
	"storage/res_nor_mode/C/lvgl/4b000033.zip",
	"storage/res_nor_mode/C/lvgl/4b000034.zip",
	"storage/res_nor_mode/C/lvgl/4b000035.zip",
	"storage/res_nor_mode/C/lvgl/4b000036.zip",
	"storage/res_nor_mode/C/lvgl/4b000037.zip",
	"storage/res_nor_mode/C/lvgl/4b000038.zip",
	"storage/res_nor_mode/C/lvgl/4b000039.zip",
	"storage/res_nor_mode/C/lvgl/4b00003a.zip",
	"storage/res_nor_mode/C/lvgl/4b00003b.zip",
	"storage/res_nor_mode/C/lvgl/4b00003c.zip",
	"storage/res_nor_mode/C/lvgl/4b00003d.zip",
	"storage/res_nor_mode/C/lvgl/4b00003e.zip",
	"storage/res_nor_mode/C/lvgl/4b00003f.zip",
	"storage/res_nor_mode/C/lvgl/4b000040.zip",
	"storage/res_nor_mode/C/lvgl/4b000041.zip",
	"storage/res_nor_mode/C/lvgl/4b000042.zip",
	"storage/res_nor_mode/C/lvgl/4b000043.zip",
	"storage/res_nor_mode/C/lvgl/4b000044.zip",
	"storage/res_nor_mode/C/lvgl/4b000045.zip",
	"storage/res_nor_mode/C/lvgl/4b000046.zip",
	"storage/res_nor_mode/C/lvgl/4b000047.zip",
	"storage/res_nor_mode/C/lvgl/4b000048.zip",
	"storage/res_nor_mode/C/lvgl/4b000049.zip",
	"storage/res_nor_mode/C/lvgl/4b00004a.zip",
	"storage/res_nor_mode/C/lvgl/4b00004b.zip",
	"storage/res_nor_mode/C/lvgl/4b00004c.zip",
	"storage/res_nor_mode/C/lvgl/4b00004d.zip",
	"storage/res_nor_mode/C/lvgl/4b00004e.zip",
	"storage/res_nor_mode/C/lvgl/4b00004f.zip",
	"storage/res_nor_mode/C/lvgl/4b000050.zip",
	"storage/res_nor_mode/C/lvgl/4b000051.zip",
	"storage/res_nor_mode/C/lvgl/4b000052.zip",
	"storage/res_nor_mode/C/lvgl/4b000053.zip",
	"storage/res_nor_mode/C/lvgl/4b000054.zip",
	"storage/res_nor_mode/C/lvgl/4b000055.zip",
	"storage/res_nor_mode/C/lvgl/4b000056.zip",
	"storage/res_nor_mode/C/lvgl/4b000057.zip",
	"storage/res_nor_mode/C/lvgl/4b000058.zip",
	"storage/res_nor_mode/C/lvgl/4b000059.zip",
	"storage/res_nor_mode/C/lvgl/4b00005a.zip",
	"storage/res_nor_mode/C/lvgl/4b00005b.zip",
	"storage/res_nor_mode/C/lvgl/4b00005c.zip",
	"storage/res_nor_mode/C/lvgl/4b00005d.zip",
	"storage/res_nor_mode/C/lvgl/4b00005e.zip",
	"storage/res_nor_mode/C/lvgl/4b00005f.zip",
	"storage/res_nor_mode/C/lvgl/4b000060.zip",
	"storage/res_nor_mode/C/lvgl/4b000061.zip",
	"storage/res_nor_mode/C/lvgl/4b000062.zip",
	"storage/res_nor_mode/C/lvgl/4b000063.zip",
	"storage/res_nor_mode/C/lvgl/4b000064.zip",
	"storage/res_nor_mode/C/lvgl/4b000065.zip",
	"storage/res_nor_mode/C/lvgl/4b000066.zip",
	"storage/res_nor_mode/C/lvgl/4b000067.zip",
	"storage/res_nor_mode/C/lvgl/4b000068.zip",
	"storage/res_nor_mode/C/lvgl/4b000069.zip",
	"storage/res_nor_mode/C/lvgl/4b00006a.zip",
	"storage/res_nor_mode/C/lvgl/4b00006b.zip",
	"storage/res_nor_mode/C/lvgl/4b00006c.zip",
	"storage/res_nor_mode/C/lvgl/4b00006d.zip",
	"storage/res_nor_mode/C/lvgl/4b00006e.zip",
	"storage/res_nor_mode/C/lvgl/4b00006f.zip",
	"storage/res_nor_mode/C/lvgl/4b000070.zip",
	"storage/res_nor_mode/C/lvgl/4b000071.zip",
	"storage/res_nor_mode/C/lvgl/4b000072.zip",
	"storage/res_nor_mode/C/lvgl/4b000073.zip",
	"storage/res_nor_mode/C/lvgl/4b000074.zip",
	"storage/res_nor_mode/C/lvgl/4b000075.zip",
	"storage/res_nor_mode/C/lvgl/4b000076.zip",
	"storage/res_nor_mode/C/lvgl/4b000077.zip",
	"storage/res_nor_mode/C/lvgl/4b000078.zip"
#endif
};

lv_obj_t * setup_scr_page_map(lv_ui *ui)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_map));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_map));
		ui->page_map = ui_scr;
	}

	//Write codes page_map
	ui_scr->page_map = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_map, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map, GUI_CTRL_SCR);

	//Set style for page_map. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->page_map, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui_scr->page_map, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui_scr->page_map, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_view_1
	ui_scr->page_map_view_1 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_1, 0, 0);
	lv_obj_set_size(ui_scr->page_map_view_1, 480, 40);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_1, GUI_CTRL_CONT);

	//Set style for page_map_view_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->page_map_view_1, 51, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_gear_mode
	ui_scr->page_map_imgList_gear_mode = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_gear_mode, 95, 11);
	lv_obj_set_size(ui_scr->page_map_imgList_gear_mode, 50, 18);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_gear_mode, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_gear_mode, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_gear_mode. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_gear_mode, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_gear_mode, (const void **) page_map_imgList_gear_mode_imgs, 3);
	lv_imglist_set_act(ui_scr->page_map_imgList_gear_mode, 0);

	//Write codes page_map_lbl_time
	ui_scr->page_map_lbl_time = lv_label_create(ui_scr->page_map_view_1);
	lv_label_set_text(ui_scr->page_map_lbl_time, "10:30");
	lv_label_set_long_mode(ui_scr->page_map_lbl_time, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_time, 188, 6);
	lv_obj_set_size(ui_scr->page_map_lbl_time, 58, 28);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_time, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_time, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_time. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_time, &lv_font_montserratMedium_17, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_weather
	ui_scr->page_map_img_weather = lv_image_create(ui_scr->page_map_view_1);
	lv_image_set_src(ui_scr->page_map_img_weather, gui_get_res_path(GUI_RES_TEM_01_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_weather, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_weather, 0);
	lv_image_set_scale(ui_scr->page_map_img_weather, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_weather, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_weather, 269, 7);
	lv_obj_set_size(ui_scr->page_map_img_weather, 25, 23);
	lv_obj_add_flag(ui_scr->page_map_img_weather, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_weather, GUI_CTRL_IMG);

	//Set style for page_map_img_weather. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_weather, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_temp
	ui_scr->page_map_lbl_temp = lv_label_create(ui_scr->page_map_view_1);
	lv_label_set_text(ui_scr->page_map_lbl_temp, "25°C");
	lv_label_set_long_mode(ui_scr->page_map_lbl_temp, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_temp, 299, 6);
	lv_obj_set_size(ui_scr->page_map_lbl_temp, 52, 28);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_temp, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_temp, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_temp. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_temp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_temp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_temp, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_bluetooth
	ui_scr->page_map_imgList_bluetooth = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_bluetooth, 371, 9);
	lv_obj_set_size(ui_scr->page_map_imgList_bluetooth, 24, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_bluetooth, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_bluetooth, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_bluetooth, (const void **) page_map_imgList_bluetooth_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_bluetooth, 0);

	//Write codes page_map_imgList_phone_bl
	ui_scr->page_map_imgList_phone_bl = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_phone_bl, 398, 9);
	lv_obj_set_size(ui_scr->page_map_imgList_phone_bl, 24, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_phone_bl, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_phone_bl, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_phone_bl. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_phone_bl, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_phone_bl, (const void **) page_map_imgList_phone_bl_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_phone_bl, 0);

	//Write codes page_map_imgList_gps
	ui_scr->page_map_imgList_gps = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_gps, 422, 9);
	lv_obj_set_size(ui_scr->page_map_imgList_gps, 24, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_gps, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_gps, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_gps, (const void **) page_map_imgList_gps_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_gps, 0);

	//Write codes page_map_imgList_networksignal
	ui_scr->page_map_imgList_networksignal = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_networksignal, 448, 9);
	lv_obj_set_size(ui_scr->page_map_imgList_networksignal, 24, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_networksignal, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_networksignal, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_networksignal, (const void **) page_map_imgList_networksignal_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_networksignal, 0);

	//Write codes page_map_img_ready
	ui_scr->page_map_img_ready = lv_image_create(ui_scr->page_map_view_1);
	lv_image_set_src(ui_scr->page_map_img_ready, gui_get_res_path(GUI_RES_READY_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_ready, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_ready, 0);
	lv_image_set_scale(ui_scr->page_map_img_ready, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_ready, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_ready, 16, 14);
	lv_obj_set_size(ui_scr->page_map_img_ready, 65, 14);
	lv_obj_add_flag(ui_scr->page_map_img_ready, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_ready, GUI_CTRL_IMG);

	//Set style for page_map_img_ready. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_ready, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_view_2
	ui_scr->page_map_view_2 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_2, 0, 237);
	lv_obj_set_size(ui_scr->page_map_view_2, 480, 34);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_2, GUI_CTRL_CONT);

	//Write codes page_map_lbl_endurance
	ui_scr->page_map_lbl_endurance = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_endurance, "100km");
	lv_label_set_long_mode(ui_scr->page_map_lbl_endurance, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_endurance, 44, 0);
	lv_obj_set_size(ui_scr->page_map_lbl_endurance, 76, 24);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_endurance, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_endurance, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_endurance. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_endurance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_endurance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_endurance, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_charge
	ui_scr->page_map_img_charge = lv_image_create(ui_scr->page_map_view_2);
	lv_image_set_src(ui_scr->page_map_img_charge, gui_get_res_path(GUI_RES_LAMP_19_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_charge, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_charge, 0);
	lv_image_set_scale(ui_scr->page_map_img_charge, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_charge, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_charge, 12, 6);
	lv_obj_set_size(ui_scr->page_map_img_charge, 24, 23);
	lv_obj_add_flag(ui_scr->page_map_img_charge, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_charge, GUI_CTRL_IMG);

	//Set style for page_map_img_charge. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_charge, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_soc_beam
	ui_scr->page_map_imgList_soc_beam = lv_imglist_create(ui_scr->page_map_view_2);
	lv_obj_set_pos(ui_scr->page_map_imgList_soc_beam, 185, 6);
	lv_obj_set_size(ui_scr->page_map_imgList_soc_beam, 24, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_soc_beam, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_soc_beam, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_soc_beam. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_soc_beam, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_soc_beam, (const void **) page_map_imgList_soc_beam_imgs, 3);
	lv_imglist_set_act(ui_scr->page_map_imgList_soc_beam, 1);

	//Write codes page_map_lbl_bat
	ui_scr->page_map_lbl_bat = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_bat, "100%");
	lv_label_set_long_mode(ui_scr->page_map_lbl_bat, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_bat, 219, 0);
	lv_obj_set_size(ui_scr->page_map_lbl_bat, 51, 24);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_bat, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_bat, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_bat. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_bat, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_bat, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_bat, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imglist_bat
	ui_scr->page_map_imglist_bat = lv_imglist_create(ui_scr->page_map_view_2);
	lv_obj_set_pos(ui_scr->page_map_imglist_bat, 277, 14);
	lv_obj_set_size(ui_scr->page_map_imglist_bat, 96, 7);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imglist_bat, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imglist_bat, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imglist_bat, (const void **) page_map_imglist_bat_imgs, 100);
	lv_imglist_set_act(ui_scr->page_map_imglist_bat, 99);

	//Write codes page_map_lbl_odo_num
	ui_scr->page_map_lbl_odo_num = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_odo_num, "10260km");
	lv_label_set_long_mode(ui_scr->page_map_lbl_odo_num, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_odo_num, 380, 3);
	lv_obj_set_size(ui_scr->page_map_lbl_odo_num, 95, 28);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_odo_num, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_odo_num, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_odo_num. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_odo_num, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_odo_num, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_odo_num, &lv_font_montserratMedium_17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_odo_num, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);

	// Write Timer page_map_timer_1
	if(ui_scr->page_map_timer_1 != NULL) lv_timer_delete(ui_scr->page_map_timer_1);
	ui_scr->page_map_timer_1 = lv_timer_create(page_map_timer_1_timer_cb, 50, "");
	lv_timer_set_repeat_count(ui_scr->page_map_timer_1, -1);

	//Write codes page_map_view_3
	ui_scr->page_map_view_3 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_3, 16, 71);
	lv_obj_set_size(ui_scr->page_map_view_3, 124, 66);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_3, GUI_CTRL_CONT);

	//Set style for page_map_view_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_view_3, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_view_3, lv_color_make(0x08, 0x17, 0x27), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_1
	ui_scr->page_map_lbl_1 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_1, "328");
	lv_label_set_long_mode(ui_scr->page_map_lbl_1, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_1, 49, 10);
	lv_obj_set_size(ui_scr->page_map_lbl_1, 43, 22);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_1, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_2
	ui_scr->page_map_lbl_2 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_2, "22");
	lv_label_set_long_mode(ui_scr->page_map_lbl_2, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_2, 2, 37);
	lv_obj_set_size(ui_scr->page_map_lbl_2, 43, 22);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_2, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_3
	ui_scr->page_map_lbl_3 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_3, "45");
	lv_label_set_long_mode(ui_scr->page_map_lbl_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_3, 35, 37);
	lv_obj_set_size(ui_scr->page_map_lbl_3, 43, 22);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_3, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_4
	ui_scr->page_map_lbl_4 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_4, "12:20");
	lv_label_set_long_mode(ui_scr->page_map_lbl_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_4, 74, 37);
	lv_obj_set_size(ui_scr->page_map_lbl_4, 43, 22);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_4, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_1
	ui_scr->page_map_img_1 = lv_image_create(ui_scr->page_map_view_3);
	lv_image_set_src(ui_scr->page_map_img_1, gui_get_res_path(GUI_RES_LAMP_01_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_1, 0);
	lv_image_set_scale(ui_scr->page_map_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_1, 11, 9);
	lv_obj_set_size(ui_scr->page_map_img_1, 24, 23);
	lv_obj_add_flag(ui_scr->page_map_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_1, GUI_CTRL_IMG);

	// Write Timer page_map_timer_2
	if(ui_scr->page_map_timer_2 != NULL) lv_timer_delete(ui_scr->page_map_timer_2);
	ui_scr->page_map_timer_2 = lv_timer_create(page_map_timer_2_timer_cb, 5000, "");
	lv_timer_set_repeat_count(ui_scr->page_map_timer_2, -1);

	//Write codes page_map_btn_1
	ui_scr->page_map_btn_1 = lv_btn_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_btn_1, 204, 45);
	lv_obj_set_size(ui_scr->page_map_btn_1, 35, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_btn_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_btn_1, GUI_CTRL_BUTTON);
	ui_scr->page_map_btn_1_label = lv_label_create(ui_scr->page_map_btn_1);
	lv_label_set_text(ui_scr->page_map_btn_1_label, "back");
	lv_obj_set_style_pad_all(ui_scr->page_map_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_map_btn_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_map_btn_2
	ui_scr->page_map_btn_2 = lv_btn_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_btn_2, 242, 45);
	lv_obj_set_size(ui_scr->page_map_btn_2, 35, 20);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_btn_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_btn_2, GUI_CTRL_BUTTON);
	ui_scr->page_map_btn_2_label = lv_label_create(ui_scr->page_map_btn_2);
	lv_label_set_text(ui_scr->page_map_btn_2_label, "next");
	lv_obj_set_style_pad_all(ui_scr->page_map_btn_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_map_btn_2_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_map_view_4
	ui_scr->page_map_view_4 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_4, 324, 42);
	lv_obj_set_size(ui_scr->page_map_view_4, 147, 95);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_4, GUI_CTRL_CONT);

	//Set style for page_map_view_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_view_4, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_view_4, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_map_view_4, 128, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_5
	ui_scr->page_map_lbl_5 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_5, "resolution:");
	lv_label_set_long_mode(ui_scr->page_map_lbl_5, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_5, 2, 1);
	lv_obj_set_size(ui_scr->page_map_lbl_5, 69, 16);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_5, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_5, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_6
	ui_scr->page_map_lbl_6 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_6, "fps:max,min,avg");
	lv_label_set_long_mode(ui_scr->page_map_lbl_6, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_6, 2, 22);
	lv_obj_set_size(ui_scr->page_map_lbl_6, 131, 19);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_6, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_6, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_6. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_6, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_7
	ui_scr->page_map_lbl_7 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_7, "0,0,0");
	lv_label_set_long_mode(ui_scr->page_map_lbl_7, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_7, 2, 39);
	lv_obj_set_size(ui_scr->page_map_lbl_7, 138, 16);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_7, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_7, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_7. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_7, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_8
	ui_scr->page_map_lbl_8 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_8, "jpgsize:max,min,avg");
	lv_label_set_long_mode(ui_scr->page_map_lbl_8, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_8, 2, 54);
	lv_obj_set_size(ui_scr->page_map_lbl_8, 143, 19);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_8, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_8, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_8. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_8, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_9
	ui_scr->page_map_lbl_9 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_9, "0,0,0");
	lv_label_set_long_mode(ui_scr->page_map_lbl_9, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_9, 2, 72);
	lv_obj_set_size(ui_scr->page_map_lbl_9, 138, 16);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_9, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_9, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_9. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_9, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_10
	ui_scr->page_map_lbl_10 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_10, "800x400");
	lv_label_set_long_mode(ui_scr->page_map_lbl_10, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_10, 68, 2);
	lv_obj_set_size(ui_scr->page_map_lbl_10, 70, 16);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_10, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_10, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_10. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_10, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_2
	ui_scr->page_map_img_2 = lv_image_create(ui_scr->page_map);
	lv_image_set_src(ui_scr->page_map_img_2, gui_get_res_path(GUI_RES_TIPS_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_2, 0);
	lv_image_set_scale(ui_scr->page_map_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_2, 216, 83);
	lv_obj_set_size(ui_scr->page_map_img_2, 216, 74);
	lv_obj_add_flag(ui_scr->page_map_img_2, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ui_scr->page_map_img_2, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_2, GUI_CTRL_IMG);

	//Set style for page_map_img_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->page_map_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_update_layout(ui_scr->page_map);
	ui_scr->page_map_del = false;

	//Init events for screen
	events_init_page_map(ui);
	return ui_scr->page_map;
}
void unload_scr_page_map(lv_ui *ui)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
	if(ui_scr->page_map_timer_1 != NULL) {
		lv_timer_delete(ui_scr->page_map_timer_1);
		ui_scr->page_map_timer_1 = NULL;
	}
	if(ui_scr->page_map_timer_2 != NULL) {
		lv_timer_delete(ui_scr->page_map_timer_2);
		ui_scr->page_map_timer_2 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
}
