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

static const char* page_app_select_animimg_1_imgs[21] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_0.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_1.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_2.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_3.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_4.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_5.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_6.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_7.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_8.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_9.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_0.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_1.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_2.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_3.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_4.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_5.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_6.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_7.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_8.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_9.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_10.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000092.zip",
	"storage/res_nor_mode/C/lvgl/4b000093.zip",
	"storage/res_nor_mode/C/lvgl/4b000094.zip",
	"storage/res_nor_mode/C/lvgl/4b000095.zip",
	"storage/res_nor_mode/C/lvgl/4b000096.zip",
	"storage/res_nor_mode/C/lvgl/4b000097.zip",
	"storage/res_nor_mode/C/lvgl/4b000098.zip",
	"storage/res_nor_mode/C/lvgl/4b000099.zip",
	"storage/res_nor_mode/C/lvgl/4b00009a.zip",
	"storage/res_nor_mode/C/lvgl/4b00009b.zip",
	"storage/res_nor_mode/C/lvgl/4b00009c.zip",
	"storage/res_nor_mode/C/lvgl/4b00009d.zip",
	"storage/res_nor_mode/C/lvgl/4b00009e.zip",
	"storage/res_nor_mode/C/lvgl/4b00009f.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a0.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a1.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a2.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a6.zip"
#endif
};
static const char* page_app_select_imglist_1_imgs[9] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_duoyun.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_baoyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_dayu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_leiyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_qing.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_wumai.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_xiaoyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_ye.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_yintian.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0000a7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000aa.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ab.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ac.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ad.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ae.zip",
	"storage/res_nor_mode/C/lvgl/4b0000af.zip"
#endif
};
static const char* page_app_select_imgList_bluetooth_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_25.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_26.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000019.zip",
	"storage/res_nor_mode/C/lvgl/4b00001a.zip"
#endif
};
static const char* page_app_select_imgList_phone_bl_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_23.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_24.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001b.zip",
	"storage/res_nor_mode/C/lvgl/4b00001c.zip"
#endif
};
static const char* page_app_select_imgList_gps_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_29.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_30.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001d.zip",
	"storage/res_nor_mode/C/lvgl/4b00001e.zip"
#endif
};
static const char* page_app_select_imgList_networksignal_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_27.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_28.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001f.zip",
	"storage/res_nor_mode/C/lvgl/4b000020.zip"
#endif
};

lv_obj_t * setup_scr_page_app_select(lv_ui *ui)
{
	lv_ui_page_app_select * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_APP_SELECT);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_app_select));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_app_select));
		ui->page_app_select = ui_scr;
	}

	//Write codes page_app_select
	ui_scr->page_app_select = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select, GUI_CTRL_SCR);

	//Write codes page_app_select_animimg_1
	ui_scr->page_app_select_animimg_1 = lv_animimg_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_animimg_1, 0, 0);
	lv_obj_set_size(ui_scr->page_app_select_animimg_1, 1024, 600);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_animimg_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_animimg_1, GUI_CTRL_ANIMIMG);
	lv_animimg_set_duration(ui_scr->page_app_select_animimg_1, 630);
	lv_animimg_set_repeat_count(ui_scr->page_app_select_animimg_1, -1);
	lv_animimg_set_src(ui_scr->page_app_select_animimg_1, (const void **) page_app_select_animimg_1_imgs, 21);
	lv_animimg_start(ui_scr->page_app_select_animimg_1);

	//Write codes page_app_select_digitclock_1
	ui_scr->page_app_select_digitclock_1 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text_fmt(ui_scr->page_app_select_digitclock_1, "%02d:%02d", 21 % 12, 25);
	lv_obj_set_style_text_align(ui_scr->page_app_select_digitclock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_pos(ui_scr->page_app_select_digitclock_1, 34, 24);
	lv_obj_set_size(ui_scr->page_app_select_digitclock_1, 151, 50);
	ui_style_set(ui_scr->page_app_select_digitclock_1, GUI_CTRL_DIGITALCLOCK);

	//Write codes page_app_select_imglist_1
	ui_scr->page_app_select_imglist_1 = lv_imglist_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_imglist_1, 182, 11);
	lv_obj_set_size(ui_scr->page_app_select_imglist_1, 68, 68);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_imglist_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_imglist_1, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_app_select_imglist_1, (const void **) page_app_select_imglist_1_imgs, 9);
	lv_imglist_set_act(ui_scr->page_app_select_imglist_1, 0);

	//Write codes page_app_select_lbl_1
	ui_scr->page_app_select_lbl_1 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_1, "32℃");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_1, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_1, 263, 24);
	lv_obj_set_size(ui_scr->page_app_select_lbl_1, 131, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_1, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_1, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_imgList_bluetooth
	ui_scr->page_app_select_imgList_bluetooth = lv_imglist_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_imgList_bluetooth, 777, 10);
	lv_obj_set_size(ui_scr->page_app_select_imgList_bluetooth, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_imgList_bluetooth, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_imgList_bluetooth, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_app_select_imgList_bluetooth, (const void **) page_app_select_imgList_bluetooth_imgs, 2);
	lv_imglist_set_act(ui_scr->page_app_select_imgList_bluetooth, 0);

	//Write codes page_app_select_imgList_phone_bl
	ui_scr->page_app_select_imgList_phone_bl = lv_imglist_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_imgList_phone_bl, 835, 10);
	lv_obj_set_size(ui_scr->page_app_select_imgList_phone_bl, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_imgList_phone_bl, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_imgList_phone_bl, GUI_CTRL_IMGLIST);

	//Set style for page_app_select_imgList_phone_bl. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_app_select_imgList_phone_bl, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_app_select_imgList_phone_bl, (const void **) page_app_select_imgList_phone_bl_imgs, 2);
	lv_imglist_set_act(ui_scr->page_app_select_imgList_phone_bl, 0);

	//Write codes page_app_select_imgList_gps
	ui_scr->page_app_select_imgList_gps = lv_imglist_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_imgList_gps, 886, 10);
	lv_obj_set_size(ui_scr->page_app_select_imgList_gps, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_imgList_gps, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_imgList_gps, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_app_select_imgList_gps, (const void **) page_app_select_imgList_gps_imgs, 2);
	lv_imglist_set_act(ui_scr->page_app_select_imgList_gps, 0);

	//Write codes page_app_select_imgList_networksignal
	ui_scr->page_app_select_imgList_networksignal = lv_imglist_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_imgList_networksignal, 942, 10);
	lv_obj_set_size(ui_scr->page_app_select_imgList_networksignal, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_imgList_networksignal, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_imgList_networksignal, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_app_select_imgList_networksignal, (const void **) page_app_select_imgList_networksignal_imgs, 2);
	lv_imglist_set_act(ui_scr->page_app_select_imgList_networksignal, 0);

	//Write codes page_app_select_lbl_mode
	ui_scr->page_app_select_lbl_mode = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_mode, "舒适模式");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_mode, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_mode, 140, 153);
	lv_obj_set_size(ui_scr->page_app_select_lbl_mode, 206, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_mode, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_mode, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_mode. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_mode, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_2
	ui_scr->page_app_select_lbl_2 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_2, "PWM");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_2, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_2, 625, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_2, 131, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_2, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_2, &lv_font_MiSans_Medium_34, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_3
	ui_scr->page_app_select_lbl_3 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_3, "23");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_3, 132, 240);
	lv_obj_set_size(ui_scr->page_app_select_lbl_3, 225, 165);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_3, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_3, &lv_font_MiSans_Medium_120, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_4
	ui_scr->page_app_select_lbl_4 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_4, "km/h");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_4, 164, 403);
	lv_obj_set_size(ui_scr->page_app_select_lbl_4, 121, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_4, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_color(ui_scr->page_app_select_lbl_4, lv_color_make(0xF0, 0xF0, 0xF0), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_4, &lv_font_MiSans_Medium_29, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_5
	ui_scr->page_app_select_lbl_5 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_5, "累计");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_5, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_5, 13, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_5, 82, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_5, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_5, &lv_font_MiSans_Medium_31, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_6
	ui_scr->page_app_select_lbl_6 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_6, "200");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_6, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_6, 90, 535);
	lv_obj_set_size(ui_scr->page_app_select_lbl_6, 94, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_6, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_6, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_6. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_6, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_7
	ui_scr->page_app_select_lbl_7 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_7, "km");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_7, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_7, 182, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_7, 82, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_7, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_7, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_7. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_7, &lv_font_MiSans_Medium_31, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_8
	ui_scr->page_app_select_lbl_8 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_8, "本次");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_8, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_8, 283, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_8, 82, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_8, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_8, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_8. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_8, &lv_font_MiSans_Medium_31, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_9
	ui_scr->page_app_select_lbl_9 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_9, "10.2");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_9, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_9, 360, 535);
	lv_obj_set_size(ui_scr->page_app_select_lbl_9, 120, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_9, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_9, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_9. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_9, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_10
	ui_scr->page_app_select_lbl_10 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_10, "km");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_10, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_10, 460, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_10, 82, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_10, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_10, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_10. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_10, &lv_font_MiSans_Medium_31, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_11
	ui_scr->page_app_select_lbl_11 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_11, "60");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_11, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_11, 895, 539);
	lv_obj_set_size(ui_scr->page_app_select_lbl_11, 119, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_11, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_11, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_11. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_11, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_12
	ui_scr->page_app_select_lbl_12 = lv_label_create(ui_scr->page_app_select);
	lv_label_set_text(ui_scr->page_app_select_lbl_12, "%");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_12, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_12, 959, 546);
	lv_obj_set_size(ui_scr->page_app_select_lbl_12, 49, 45);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_12, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_12, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_12. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_12, &lv_font_MiSans_Medium_31, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_arcmenu_1
	ui_scr->page_app_select_arcmenu_1 = lv_arcmenu_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_arcmenu_1, 530, 115);
	lv_obj_set_size(ui_scr->page_app_select_arcmenu_1, 140, 372);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_arcmenu_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_arcmenu_1, GUI_CTRL_ARCMENU);

	//Set style state: LV_STATE_DEFAULT for style_page_app_select_arcmenu_1_extra_items_main_default
	static lv_style_t style_page_app_select_arcmenu_1_extra_items_main_default;
	ui_init_style(&style_page_app_select_arcmenu_1_extra_items_main_default);

	//Delete extra default style for page_app_select_arcmenu_1
	lv_obj_remove_style(ui_scr->page_app_select_arcmenu_1, &gui_arcmenu_arc_menu_items_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->page_app_select_arcmenu_1_item = lv_arcmenu_add_button(ui_scr->page_app_select_arcmenu_1, gui_get_res_path(GUI_RES_A_138SHEZHIXITONGSHEZHIGONGNENGSHEZHISHUXING_PNG), "");
	ui_scr->page_app_select_arcmenu_1_item1 = lv_arcmenu_add_button(ui_scr->page_app_select_arcmenu_1, gui_get_res_path(GUI_RES_YINLE_1_PNG), "");
	ui_scr->page_app_select_arcmenu_1_item2 = lv_arcmenu_add_button(ui_scr->page_app_select_arcmenu_1, gui_get_res_path(GUI_RES_DAOHANG_TIANCHONG_PNG), "");
	for (int i = 0; i < lv_obj_get_child_count(ui_scr->page_app_select_arcmenu_1); i++) {
		lv_obj_add_style(lv_obj_get_child(ui_scr->page_app_select_arcmenu_1, i), &gui_arcmenu_arc_menu_items_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(lv_obj_get_child(ui_scr->page_app_select_arcmenu_1, i), &style_page_app_select_arcmenu_1_extra_items_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	}
	lv_arcmenu_set_arc(ui_scr->page_app_select_arcmenu_1, true);
	lv_arcmenu_set_left_arc(ui_scr->page_app_select_arcmenu_1, false);
	lv_arcmenu_set_radius(ui_scr->page_app_select_arcmenu_1, 125);
	lv_obj_scroll_to_view(lv_obj_get_child(ui_scr->page_app_select_arcmenu_1, 1), LV_ANIM_OFF);

	//Write codes page_app_select_view_select_app_map
	ui_scr->page_app_select_view_select_app_map = lv_obj_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_view_select_app_map, 649, 148);
	lv_obj_set_size(ui_scr->page_app_select_view_select_app_map, 307, 339);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_view_select_app_map, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->page_app_select_view_select_app_map, LV_OBJ_FLAG_SCROLLABLE);

	//Hidden for widget page_app_select_view_select_app_map
	lv_obj_add_flag(ui_scr->page_app_select_view_select_app_map, LV_OBJ_FLAG_HIDDEN);
	ui_style_set(ui_scr->page_app_select_view_select_app_map, GUI_CTRL_CONT);

	//Set style for page_app_select_view_select_app_map. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_app_select_view_select_app_map, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_app_select_view_select_app_map, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_17
	ui_scr->page_app_select_lbl_17 = lv_label_create(ui_scr->page_app_select_view_select_app_map);
	lv_label_set_text(ui_scr->page_app_select_lbl_17, "9:50到达");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_17, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_17, 149, 248);
	lv_obj_set_size(ui_scr->page_app_select_lbl_17, 173, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_17, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_17, GUI_CTRL_LABEL);

	//Write codes page_app_select_lbl_16
	ui_scr->page_app_select_lbl_16 = lv_label_create(ui_scr->page_app_select_view_select_app_map);
	lv_label_set_text(ui_scr->page_app_select_lbl_16, "5.6km");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_16, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_16, 14, 248);
	lv_obj_set_size(ui_scr->page_app_select_lbl_16, 206, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_16, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_16, GUI_CTRL_LABEL);

	//Write codes page_app_select_slider_2
	ui_scr->page_app_select_slider_2 = lv_slider_create(ui_scr->page_app_select_view_select_app_map);
	lv_obj_set_pos(ui_scr->page_app_select_slider_2, 12, 218);
	lv_obj_set_size(ui_scr->page_app_select_slider_2, 269, 13);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_slider_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_slider_2, GUI_CTRL_SLIDER);

	//Set style for page_app_select_slider_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_app_select_slider_2, lv_color_make(0x50, 0x50, 0x50), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for page_app_select_slider_2. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_slider_2, 20, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for page_app_select_slider_2. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_slider_2, 20, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->page_app_select_slider_2, 0, 100);
	lv_slider_set_mode(ui_scr->page_app_select_slider_2, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->page_app_select_slider_2, 30, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->page_app_select_slider_2, 0, LV_ANIM_OFF);

	//Write codes page_app_select_lbl_15
	ui_scr->page_app_select_lbl_15 = lv_label_create(ui_scr->page_app_select_view_select_app_map);
	lv_label_set_text(ui_scr->page_app_select_lbl_15, "200m");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_15, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_15, 67, 143);
	lv_obj_set_size(ui_scr->page_app_select_lbl_15, 206, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_15, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_15, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_15. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_15, &lv_font_MiSans_Medium_34, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_lbl_14
	ui_scr->page_app_select_lbl_14 = lv_label_create(ui_scr->page_app_select_view_select_app_map);
	lv_label_set_text(ui_scr->page_app_select_lbl_14, "深南大道");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_14, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_14, 43, 84);
	lv_obj_set_size(ui_scr->page_app_select_lbl_14, 206, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_14, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_14, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_14. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_14, &lv_font_MiSans_Medium_34, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_img_1
	ui_scr->page_app_select_img_1 = lv_image_create(ui_scr->page_app_select_view_select_app_map);
	lv_image_set_src(ui_scr->page_app_select_img_1, gui_get_res_path(GUI_RES_IMAGE_9158879_0_PNG));
	lv_image_set_pivot(ui_scr->page_app_select_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_app_select_img_1, 0);
	lv_image_set_scale(ui_scr->page_app_select_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_app_select_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_app_select_img_1, 54, -22);
	lv_obj_set_size(ui_scr->page_app_select_img_1, 128, 125);
	lv_obj_add_flag(ui_scr->page_app_select_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_app_select_img_1, GUI_CTRL_IMG);

	//Set style for page_app_select_img_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_image_recolor_opa(ui_scr->page_app_select_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_view_select_app_music
	ui_scr->page_app_select_view_select_app_music = lv_obj_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_view_select_app_music, 636, 123);
	lv_obj_set_size(ui_scr->page_app_select_view_select_app_music, 320, 339);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_view_select_app_music, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->page_app_select_view_select_app_music, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->page_app_select_view_select_app_music, GUI_CTRL_CONT);

	//Set style for page_app_select_view_select_app_music. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_app_select_view_select_app_music, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_app_select_view_select_app_music, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_slider_1
	ui_scr->page_app_select_slider_1 = lv_slider_create(ui_scr->page_app_select_view_select_app_music);
	lv_obj_set_pos(ui_scr->page_app_select_slider_1, 28, 305);
	lv_obj_set_size(ui_scr->page_app_select_slider_1, 264, 10);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_slider_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_slider_1, GUI_CTRL_SLIDER);

	//Set style for page_app_select_slider_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_app_select_slider_1, lv_color_make(0x21, 0x95, 0xf6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_app_select_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Set style for page_app_select_slider_1. Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_slider_1, 20, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Set style for page_app_select_slider_1. Part: LV_PART_KNOB, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_slider_1, 20, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_slider_set_range(ui_scr->page_app_select_slider_1, 0, 100);
	lv_slider_set_mode(ui_scr->page_app_select_slider_1, LV_SLIDER_MODE_NORMAL);
	lv_slider_set_value(ui_scr->page_app_select_slider_1, 60, LV_ANIM_OFF);
	lv_slider_set_start_value(ui_scr->page_app_select_slider_1, 0, LV_ANIM_OFF);

	//Write codes page_app_select_img_3
	ui_scr->page_app_select_img_3 = lv_image_create(ui_scr->page_app_select_view_select_app_music);
	lv_image_set_pivot(ui_scr->page_app_select_img_3, 0, 0);
	lv_image_set_rotation(ui_scr->page_app_select_img_3, 0);
	lv_image_set_scale(ui_scr->page_app_select_img_3, 200);
	lv_image_set_inner_align(ui_scr->page_app_select_img_3, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_app_select_img_3, 53, 15);
	lv_obj_set_size(ui_scr->page_app_select_img_3, 240, 250);
	lv_obj_add_flag(ui_scr->page_app_select_img_3, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_app_select_img_3, GUI_CTRL_IMG);

	//Write codes page_app_select_lbl_13
	ui_scr->page_app_select_lbl_13 = lv_label_create(ui_scr->page_app_select_view_select_app_music);
	lv_label_set_text(ui_scr->page_app_select_lbl_13, "周杰伦-青花瓷");
	lv_label_set_long_mode(ui_scr->page_app_select_lbl_13, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_pos(ui_scr->page_app_select_lbl_13, 14, 224);
	lv_obj_set_size(ui_scr->page_app_select_lbl_13, 265, 59);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_lbl_13, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_lbl_13, GUI_CTRL_LABEL);

	//Set style for page_app_select_lbl_13. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_app_select_lbl_13, &lv_font_lyrics_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_app_select_btn_back
	ui_scr->page_app_select_btn_back = lv_btn_create(ui_scr->page_app_select);
	lv_obj_set_pos(ui_scr->page_app_select_btn_back, 34, 82);
	lv_obj_set_size(ui_scr->page_app_select_btn_back, 177, 71);
	lv_obj_set_scrollbar_mode(ui_scr->page_app_select_btn_back, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_app_select_btn_back, GUI_CTRL_BUTTON);

	//Set style for page_app_select_btn_back. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_app_select_btn_back, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_app_select_btn_back, lv_color_make(0x21, 0x95, 0xf6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_app_select_btn_back, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_app_select_btn_back, &lv_font_MiSans_Medium_24, LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->page_app_select_btn_back_label = lv_label_create(ui_scr->page_app_select_btn_back);
	lv_label_set_text(ui_scr->page_app_select_btn_back_label, "hardware test");
	lv_obj_set_style_pad_all(ui_scr->page_app_select_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_app_select_btn_back_label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_update_layout(ui_scr->page_app_select);
	ui_scr->page_app_select_del = false;

	//Init events for screen
	events_init_page_app_select(ui);
	return ui_scr->page_app_select;
}
void unload_scr_page_app_select(lv_ui *ui)
{
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_APP_SELECT);
}
