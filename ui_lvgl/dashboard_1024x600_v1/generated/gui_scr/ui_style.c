/*Generate Code, Do NOT Edit!*/
#include "./ui_style.h"
#include "../gui_guider.h"

// digitclock
lv_style_t gui_digitclock_main_default_style;

// slider
lv_style_t gui_slider_main_default_style;
lv_style_t gui_slider_indicator_default_style;
lv_style_t gui_slider_knob_default_style;

// img
lv_style_t gui_img_main_default_style;

// scr
lv_style_t gui_scr_main_default_style;

// imgbtn
lv_style_t gui_imgbtn_main_default_style;
lv_style_t gui_imgbtn_main_pressed_style;
lv_style_t gui_imgbtn_main_checked_style;

// line
lv_style_t gui_line_main_default_style;

// label
lv_style_t gui_label_main_default_style;

// imglist
lv_style_t gui_imglist_main_default_style;

// ta
lv_style_t gui_ta_main_default_style;
lv_style_t gui_ta_scrollbar_default_style;

// bar
lv_style_t gui_bar_main_default_style;
lv_style_t gui_bar_indicator_default_style;

// arc
lv_style_t gui_arc_main_default_style;
lv_style_t gui_arc_indicator_default_style;
lv_style_t gui_arc_knob_default_style;

// arcmenu
lv_style_t gui_arcmenu_main_default_style;
lv_style_t gui_arcmenu_arc_menu_items_default_style;

// cont
lv_style_t gui_cont_main_default_style;

// btn
lv_style_t gui_btn_main_default_style;
lv_style_t gui_btn_main_focus_key_style;

// animimg
lv_style_t gui_animimg_main_default_style;

void ui_style_init(void)
{
	// digitclock
	lv_style_init(&gui_digitclock_main_default_style);
	lv_style_set_radius(&gui_digitclock_main_default_style, 0);
	lv_style_set_bg_color(&gui_digitclock_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_digitclock_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_digitclock_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_digitclock_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_digitclock_main_default_style, 0);
	lv_style_set_bg_opa(&gui_digitclock_main_default_style, 0);
	lv_style_set_shadow_width(&gui_digitclock_main_default_style, 0);
	lv_style_set_shadow_color(&gui_digitclock_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_digitclock_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_digitclock_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_digitclock_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_digitclock_main_default_style, 0);
	lv_style_set_border_color(&gui_digitclock_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&gui_digitclock_main_default_style, 0);
	lv_style_set_border_opa(&gui_digitclock_main_default_style, 255);
	lv_style_set_border_side(&gui_digitclock_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_digitclock_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_digitclock_main_default_style, 0);
	lv_style_set_outline_opa(&gui_digitclock_main_default_style, 128);
	lv_style_set_outline_pad(&gui_digitclock_main_default_style, 0);
	lv_style_set_text_color(&gui_digitclock_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_text_font(&gui_digitclock_main_default_style, &lv_font_MiSans_Medium_36);
	lv_style_set_text_letter_space(&gui_digitclock_main_default_style, 2);
	lv_style_set_text_align(&gui_digitclock_main_default_style, LV_TEXT_ALIGN_CENTER);
	lv_style_set_pad_left(&gui_digitclock_main_default_style, 0);
	lv_style_set_pad_right(&gui_digitclock_main_default_style, 0);
	lv_style_set_pad_top(&gui_digitclock_main_default_style, 7);

	// slider
	lv_style_init(&gui_slider_main_default_style);
	lv_style_set_radius(&gui_slider_main_default_style, 0);
	lv_style_set_bg_color(&gui_slider_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_slider_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_slider_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_slider_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_slider_main_default_style, 0);
	lv_style_set_bg_opa(&gui_slider_main_default_style, 0);
	lv_style_set_shadow_width(&gui_slider_main_default_style, 0);
	lv_style_set_shadow_color(&gui_slider_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_slider_main_default_style, 0);
	lv_style_set_shadow_spread(&gui_slider_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_slider_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_slider_main_default_style, 0);
	lv_style_set_outline_color(&gui_slider_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_slider_main_default_style, 0);
	lv_style_set_outline_opa(&gui_slider_main_default_style, 0);
	lv_style_set_outline_pad(&gui_slider_main_default_style, 0);
	lv_style_init(&gui_slider_indicator_default_style);
	lv_style_set_radius(&gui_slider_indicator_default_style, 20);
	lv_style_set_bg_color(&gui_slider_indicator_default_style, lv_color_make(0xFA, 0xFA, 0xFA));
	lv_style_set_bg_grad_color(&gui_slider_indicator_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_slider_indicator_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_slider_indicator_default_style, 255);
	lv_style_set_bg_main_stop(&gui_slider_indicator_default_style, 0);
	lv_style_set_bg_opa(&gui_slider_indicator_default_style, 255);
	lv_style_init(&gui_slider_knob_default_style);
	lv_style_set_radius(&gui_slider_knob_default_style, 20);
	lv_style_set_bg_color(&gui_slider_knob_default_style, lv_color_make(0xFA, 0xFA, 0xFA));
	lv_style_set_bg_grad_color(&gui_slider_knob_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_slider_knob_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_slider_knob_default_style, 255);
	lv_style_set_bg_main_stop(&gui_slider_knob_default_style, 0);
	lv_style_set_bg_opa(&gui_slider_knob_default_style, 255);

	// img
	lv_style_init(&gui_img_main_default_style);
	lv_style_set_radius(&gui_img_main_default_style, 0);
	lv_style_set_clip_corner(&gui_img_main_default_style, true);
	lv_style_set_bg_color(&gui_img_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_img_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_img_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_img_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_img_main_default_style, 0);
	lv_style_set_bg_opa(&gui_img_main_default_style, 0);
	lv_style_set_outline_color(&gui_img_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_img_main_default_style, 0);
	lv_style_set_outline_opa(&gui_img_main_default_style, 128);
	lv_style_set_outline_pad(&gui_img_main_default_style, 0);
	lv_style_set_image_recolor(&gui_img_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_image_recolor_opa(&gui_img_main_default_style, 0);
	lv_style_set_image_opa(&gui_img_main_default_style, 255);

	// scr
	lv_style_init(&gui_scr_main_default_style);
	lv_style_set_bg_color(&gui_scr_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_color(&gui_scr_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_scr_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&gui_scr_main_default_style, 255);
	lv_style_set_border_color(&gui_scr_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_scr_main_default_style, 0);
	lv_style_set_border_opa(&gui_scr_main_default_style, 255);

	// imgbtn
	lv_style_init(&gui_imgbtn_main_default_style);
	lv_style_set_radius(&gui_imgbtn_main_default_style, 0);
	lv_style_set_clip_corner(&gui_imgbtn_main_default_style, true);
	lv_style_set_bg_color(&gui_imgbtn_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_imgbtn_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_imgbtn_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_imgbtn_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_imgbtn_main_default_style, 0);
	lv_style_set_bg_opa(&gui_imgbtn_main_default_style, 0);
	lv_style_set_shadow_width(&gui_imgbtn_main_default_style, 0);
	lv_style_set_shadow_color(&gui_imgbtn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_imgbtn_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_imgbtn_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_imgbtn_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_imgbtn_main_default_style, 0);
	lv_style_set_outline_color(&gui_imgbtn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_imgbtn_main_default_style, 0);
	lv_style_set_outline_opa(&gui_imgbtn_main_default_style, 128);
	lv_style_set_outline_pad(&gui_imgbtn_main_default_style, 0);
	lv_style_set_text_color(&gui_imgbtn_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&gui_imgbtn_main_default_style, &lv_font_MiSans_Medium_14);
	lv_style_set_text_letter_space(&gui_imgbtn_main_default_style, 0);
	lv_style_set_text_align(&gui_imgbtn_main_default_style, LV_TEXT_ALIGN_CENTER);
	lv_style_set_image_recolor(&gui_imgbtn_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_image_recolor_opa(&gui_imgbtn_main_default_style, 0);
	lv_style_set_image_opa(&gui_imgbtn_main_default_style, 255);
	lv_style_init(&gui_imgbtn_main_pressed_style);
	lv_style_set_radius(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_clip_corner(&gui_imgbtn_main_pressed_style, true);
	lv_style_set_bg_color(&gui_imgbtn_main_pressed_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_imgbtn_main_pressed_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_imgbtn_main_pressed_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_imgbtn_main_pressed_style, 255);
	lv_style_set_bg_main_stop(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_bg_opa(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_shadow_width(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_shadow_color(&gui_imgbtn_main_pressed_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_imgbtn_main_pressed_style, 255);
	lv_style_set_shadow_spread(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_shadow_offset_x(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_shadow_offset_y(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_text_color(&gui_imgbtn_main_pressed_style, lv_color_make(0xFF, 0x33, 0xFF));
	lv_style_set_text_font(&gui_imgbtn_main_pressed_style, &lv_font_MiSans_Medium_14);
	lv_style_set_image_recolor(&gui_imgbtn_main_pressed_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_image_recolor_opa(&gui_imgbtn_main_pressed_style, 0);
	lv_style_set_image_opa(&gui_imgbtn_main_pressed_style, 255);
	lv_style_init(&gui_imgbtn_main_checked_style);
	lv_style_set_radius(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_clip_corner(&gui_imgbtn_main_checked_style, true);
	lv_style_set_bg_color(&gui_imgbtn_main_checked_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_imgbtn_main_checked_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_imgbtn_main_checked_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_imgbtn_main_checked_style, 255);
	lv_style_set_bg_main_stop(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_bg_opa(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_shadow_width(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_shadow_color(&gui_imgbtn_main_checked_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_imgbtn_main_checked_style, 255);
	lv_style_set_shadow_spread(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_shadow_offset_x(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_shadow_offset_y(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_text_color(&gui_imgbtn_main_checked_style, lv_color_make(0xFF, 0x33, 0xFF));
	lv_style_set_text_font(&gui_imgbtn_main_checked_style, &lv_font_MiSans_Medium_14);
	lv_style_set_image_recolor(&gui_imgbtn_main_checked_style, lv_color_make(0xFF, 0xFF, 0xFF));
	lv_style_set_image_recolor_opa(&gui_imgbtn_main_checked_style, 0);
	lv_style_set_image_opa(&gui_imgbtn_main_checked_style, 255);

	// line
	lv_style_init(&gui_line_main_default_style);
	lv_style_set_outline_color(&gui_line_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_line_main_default_style, 0);
	lv_style_set_outline_opa(&gui_line_main_default_style, 128);
	lv_style_set_outline_pad(&gui_line_main_default_style, 0);
	lv_style_set_line_color(&gui_line_main_default_style, lv_color_make(0x54, 0xf0, 0x5a));
	lv_style_set_line_width(&gui_line_main_default_style, 3);
	lv_style_set_line_rounded(&gui_line_main_default_style, true);

	// label
	lv_style_init(&gui_label_main_default_style);
	lv_style_set_radius(&gui_label_main_default_style, 0);
	lv_style_set_bg_color(&gui_label_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_label_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_label_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_label_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_label_main_default_style, 0);
	lv_style_set_bg_opa(&gui_label_main_default_style, 0);
	lv_style_set_shadow_width(&gui_label_main_default_style, 0);
	lv_style_set_shadow_color(&gui_label_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_label_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_label_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_label_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_label_main_default_style, 0);
	lv_style_set_border_color(&gui_label_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_label_main_default_style, 0);
	lv_style_set_border_opa(&gui_label_main_default_style, 255);
	lv_style_set_border_side(&gui_label_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_label_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_label_main_default_style, 0);
	lv_style_set_outline_opa(&gui_label_main_default_style, 128);
	lv_style_set_outline_pad(&gui_label_main_default_style, 0);
	lv_style_set_text_color(&gui_label_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_text_font(&gui_label_main_default_style, &lv_font_MiSans_Medium_18);
	lv_style_set_text_letter_space(&gui_label_main_default_style, 2);
	lv_style_set_text_line_space(&gui_label_main_default_style, 0);
	lv_style_set_text_align(&gui_label_main_default_style, LV_TEXT_ALIGN_LEFT);
	lv_style_set_pad_left(&gui_label_main_default_style, 0);
	lv_style_set_pad_right(&gui_label_main_default_style, 0);
	lv_style_set_pad_top(&gui_label_main_default_style, 8);
	lv_style_set_pad_bottom(&gui_label_main_default_style, 0);

	// imglist
	lv_style_init(&gui_imglist_main_default_style);
	lv_style_set_radius(&gui_imglist_main_default_style, 0);
	lv_style_set_clip_corner(&gui_imglist_main_default_style, true);
	lv_style_set_bg_color(&gui_imglist_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_imglist_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_imglist_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_imglist_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_imglist_main_default_style, 0);
	lv_style_set_bg_opa(&gui_imglist_main_default_style, 0);
	lv_style_set_outline_color(&gui_imglist_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_imglist_main_default_style, 0);
	lv_style_set_outline_opa(&gui_imglist_main_default_style, 128);
	lv_style_set_outline_pad(&gui_imglist_main_default_style, 0);
	lv_style_set_image_recolor(&gui_imglist_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_image_recolor_opa(&gui_imglist_main_default_style, 0);
	lv_style_set_image_opa(&gui_imglist_main_default_style, 255);

	// ta
	lv_style_init(&gui_ta_main_default_style);
	lv_style_set_radius(&gui_ta_main_default_style, 4);
	lv_style_set_bg_color(&gui_ta_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&gui_ta_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_ta_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_ta_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_ta_main_default_style, 0);
	lv_style_set_bg_opa(&gui_ta_main_default_style, 255);
	lv_style_set_shadow_width(&gui_ta_main_default_style, 0);
	lv_style_set_shadow_color(&gui_ta_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_ta_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_ta_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_ta_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_ta_main_default_style, 0);
	lv_style_set_border_color(&gui_ta_main_default_style, lv_color_make(0xe6, 0xe6, 0xe6));
	lv_style_set_border_width(&gui_ta_main_default_style, 2);
	lv_style_set_border_opa(&gui_ta_main_default_style, 255);
	lv_style_set_border_side(&gui_ta_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_ta_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_ta_main_default_style, 0);
	lv_style_set_outline_opa(&gui_ta_main_default_style, 128);
	lv_style_set_outline_pad(&gui_ta_main_default_style, 0);
	lv_style_set_text_color(&gui_ta_main_default_style, lv_color_make(0x86, 0xC0, 0x16));
	lv_style_set_text_font(&gui_ta_main_default_style, &lv_font_MiSans_Medium_40);
	lv_style_set_text_letter_space(&gui_ta_main_default_style, 2);
	lv_style_set_text_align(&gui_ta_main_default_style, LV_TEXT_ALIGN_CENTER);
	lv_style_set_pad_left(&gui_ta_main_default_style, 0);
	lv_style_set_pad_right(&gui_ta_main_default_style, 0);
	lv_style_set_pad_top(&gui_ta_main_default_style, 0);
	lv_style_set_pad_bottom(&gui_ta_main_default_style, 0);
	lv_style_init(&gui_ta_scrollbar_default_style);
	lv_style_set_radius(&gui_ta_scrollbar_default_style, 0);
	lv_style_set_bg_color(&gui_ta_scrollbar_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_opa(&gui_ta_scrollbar_default_style, 255);

	// bar
	lv_style_init(&gui_bar_main_default_style);
	lv_style_set_radius(&gui_bar_main_default_style, 50);
	lv_style_set_bg_color(&gui_bar_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_bar_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_bar_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_bar_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_bar_main_default_style, 0);
	lv_style_set_bg_opa(&gui_bar_main_default_style, 61);
	lv_style_set_shadow_width(&gui_bar_main_default_style, 0);
	lv_style_set_shadow_color(&gui_bar_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_bar_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_bar_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_bar_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_bar_main_default_style, 0);
	lv_style_set_outline_color(&gui_bar_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_bar_main_default_style, 0);
	lv_style_set_outline_opa(&gui_bar_main_default_style, 128);
	lv_style_set_outline_pad(&gui_bar_main_default_style, 0);
	lv_style_init(&gui_bar_indicator_default_style);
	lv_style_set_radius(&gui_bar_indicator_default_style, 50);
	lv_style_set_bg_color(&gui_bar_indicator_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_bar_indicator_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_bar_indicator_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_bar_indicator_default_style, 255);
	lv_style_set_bg_main_stop(&gui_bar_indicator_default_style, 0);
	lv_style_set_bg_opa(&gui_bar_indicator_default_style, 255);

	// arc
	lv_style_init(&gui_arc_main_default_style);
	lv_style_set_radius(&gui_arc_main_default_style, 6);
	lv_style_set_bg_color(&gui_arc_main_default_style, lv_color_make(0xf6, 0xf6, 0xf6));
	lv_style_set_bg_grad_color(&gui_arc_main_default_style, lv_color_make(0xf6, 0xf6, 0xf6));
	lv_style_set_bg_grad_dir(&gui_arc_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_arc_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_arc_main_default_style, 0);
	lv_style_set_bg_opa(&gui_arc_main_default_style, 0);
	lv_style_set_shadow_width(&gui_arc_main_default_style, 0);
	lv_style_set_shadow_color(&gui_arc_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_arc_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_arc_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_arc_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_arc_main_default_style, 0);
	lv_style_set_border_color(&gui_arc_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&gui_arc_main_default_style, 0);
	lv_style_set_border_opa(&gui_arc_main_default_style, 255);
	lv_style_set_border_side(&gui_arc_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_arc_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_arc_main_default_style, 0);
	lv_style_set_outline_opa(&gui_arc_main_default_style, 128);
	lv_style_set_outline_pad(&gui_arc_main_default_style, 0);
	lv_style_set_pad_left(&gui_arc_main_default_style, 0);
	lv_style_set_pad_right(&gui_arc_main_default_style, 0);
	lv_style_set_pad_top(&gui_arc_main_default_style, 0);
	lv_style_set_pad_bottom(&gui_arc_main_default_style, 0);
	lv_style_set_pad_all(&gui_arc_main_default_style, 0);
	lv_style_set_arc_color(&gui_arc_main_default_style, lv_color_make(0xff, 0xc9, 0x28));
	lv_style_set_arc_opa(&gui_arc_main_default_style, 255);
	lv_style_set_arc_width(&gui_arc_main_default_style, 3);
	lv_style_set_arc_rounded(&gui_arc_main_default_style, true);
	lv_style_init(&gui_arc_indicator_default_style);
	lv_style_set_bg_opa(&gui_arc_indicator_default_style, 0);
	lv_style_set_pad_all(&gui_arc_indicator_default_style, 0);
	lv_style_set_arc_color(&gui_arc_indicator_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_arc_opa(&gui_arc_indicator_default_style, 0);
	lv_style_set_arc_width(&gui_arc_indicator_default_style, 0);
	lv_style_set_arc_rounded(&gui_arc_indicator_default_style, true);
	lv_style_init(&gui_arc_knob_default_style);
	lv_style_set_bg_color(&gui_arc_knob_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_arc_knob_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_arc_knob_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_arc_knob_default_style, 255);
	lv_style_set_bg_main_stop(&gui_arc_knob_default_style, 0);
	lv_style_set_bg_opa(&gui_arc_knob_default_style, 0);
	lv_style_set_shadow_width(&gui_arc_knob_default_style, 0);
	lv_style_set_border_width(&gui_arc_knob_default_style, 0);
	lv_style_set_pad_all(&gui_arc_knob_default_style, 0);

	// arcmenu
	lv_style_init(&gui_arcmenu_main_default_style);
	lv_style_set_radius(&gui_arcmenu_main_default_style, 0);
	lv_style_set_clip_corner(&gui_arcmenu_main_default_style, true);
	lv_style_set_bg_color(&gui_arcmenu_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_color(&gui_arcmenu_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_dir(&gui_arcmenu_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_arcmenu_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_arcmenu_main_default_style, 0);
	lv_style_set_bg_opa(&gui_arcmenu_main_default_style, 0);
	lv_style_set_shadow_width(&gui_arcmenu_main_default_style, 0);
	lv_style_set_shadow_color(&gui_arcmenu_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_arcmenu_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_arcmenu_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_arcmenu_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_arcmenu_main_default_style, 0);
	lv_style_set_border_color(&gui_arcmenu_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_arcmenu_main_default_style, 0);
	lv_style_set_border_opa(&gui_arcmenu_main_default_style, 255);
	lv_style_set_border_side(&gui_arcmenu_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_arcmenu_main_default_style, lv_color_make(0x08, 0x1A, 0x0F));
	lv_style_set_outline_width(&gui_arcmenu_main_default_style, 0);
	lv_style_set_outline_opa(&gui_arcmenu_main_default_style, 255);
	lv_style_set_outline_pad(&gui_arcmenu_main_default_style, 0);
	lv_style_set_pad_left(&gui_arcmenu_main_default_style, 12);
	lv_style_set_pad_right(&gui_arcmenu_main_default_style, 12);
	lv_style_set_pad_top(&gui_arcmenu_main_default_style, 0);
	lv_style_set_pad_bottom(&gui_arcmenu_main_default_style, 0);
	lv_style_set_pad_row(&gui_arcmenu_main_default_style, 60);
	lv_style_init(&gui_arcmenu_arc_menu_items_default_style);
	lv_style_set_radius(&gui_arcmenu_arc_menu_items_default_style, 0);
	lv_style_set_bg_color(&gui_arcmenu_arc_menu_items_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_color(&gui_arcmenu_arc_menu_items_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_dir(&gui_arcmenu_arc_menu_items_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_arcmenu_arc_menu_items_default_style, 255);
	lv_style_set_bg_main_stop(&gui_arcmenu_arc_menu_items_default_style, 0);
	lv_style_set_bg_opa(&gui_arcmenu_arc_menu_items_default_style, 0);
	lv_style_set_border_color(&gui_arcmenu_arc_menu_items_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_arcmenu_arc_menu_items_default_style, 0);
	lv_style_set_border_opa(&gui_arcmenu_arc_menu_items_default_style, 255);
	lv_style_set_border_side(&gui_arcmenu_arc_menu_items_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_text_color(&gui_arcmenu_arc_menu_items_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_text_font(&gui_arcmenu_arc_menu_items_default_style, &lv_font_MiSans_Medium_24);
	lv_style_set_text_letter_space(&gui_arcmenu_arc_menu_items_default_style, 0);
	lv_style_set_pad_column(&gui_arcmenu_arc_menu_items_default_style, 10);

	// cont
	lv_style_init(&gui_cont_main_default_style);
	lv_style_set_radius(&gui_cont_main_default_style, 0);
	lv_style_set_bg_color(&gui_cont_main_default_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_bg_grad_color(&gui_cont_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&gui_cont_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_cont_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_cont_main_default_style, 0);
	lv_style_set_bg_opa(&gui_cont_main_default_style, 255);
	lv_style_set_shadow_width(&gui_cont_main_default_style, 0);
	lv_style_set_shadow_color(&gui_cont_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_cont_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_cont_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_cont_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_cont_main_default_style, 0);
	lv_style_set_border_color(&gui_cont_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_cont_main_default_style, 0);
	lv_style_set_border_opa(&gui_cont_main_default_style, 255);
	lv_style_set_border_side(&gui_cont_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_cont_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_cont_main_default_style, 0);
	lv_style_set_outline_opa(&gui_cont_main_default_style, 128);
	lv_style_set_outline_pad(&gui_cont_main_default_style, 0);
	lv_style_set_pad_left(&gui_cont_main_default_style, 0);
	lv_style_set_pad_right(&gui_cont_main_default_style, 0);
	lv_style_set_pad_top(&gui_cont_main_default_style, 0);
	lv_style_set_pad_bottom(&gui_cont_main_default_style, 0);

	// btn
	lv_style_init(&gui_btn_main_default_style);
	lv_style_set_radius(&gui_btn_main_default_style, 10);
	lv_style_set_bg_color(&gui_btn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_btn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_btn_main_default_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_btn_main_default_style, 255);
	lv_style_set_bg_main_stop(&gui_btn_main_default_style, 0);
	lv_style_set_bg_opa(&gui_btn_main_default_style, 255);
	lv_style_set_shadow_width(&gui_btn_main_default_style, 0);
	lv_style_set_shadow_color(&gui_btn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_btn_main_default_style, 255);
	lv_style_set_shadow_spread(&gui_btn_main_default_style, 0);
	lv_style_set_shadow_offset_x(&gui_btn_main_default_style, 0);
	lv_style_set_shadow_offset_y(&gui_btn_main_default_style, 0);
	lv_style_set_border_color(&gui_btn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_border_width(&gui_btn_main_default_style, 0);
	lv_style_set_border_opa(&gui_btn_main_default_style, 255);
	lv_style_set_border_side(&gui_btn_main_default_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_btn_main_default_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_btn_main_default_style, 0);
	lv_style_set_outline_opa(&gui_btn_main_default_style, 128);
	lv_style_set_outline_pad(&gui_btn_main_default_style, 0);
	lv_style_set_text_color(&gui_btn_main_default_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_text_font(&gui_btn_main_default_style, &lv_font_MiSans_Medium_30);
	lv_style_set_text_letter_space(&gui_btn_main_default_style, 0);
	lv_style_set_text_align(&gui_btn_main_default_style, LV_TEXT_ALIGN_CENTER);
	lv_style_init(&gui_btn_main_focus_key_style);
	lv_style_set_radius(&gui_btn_main_focus_key_style, 10);
	lv_style_set_bg_color(&gui_btn_main_focus_key_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_color(&gui_btn_main_focus_key_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_bg_grad_dir(&gui_btn_main_focus_key_style, LV_GRAD_DIR_NONE);
	lv_style_set_bg_grad_stop(&gui_btn_main_focus_key_style, 255);
	lv_style_set_bg_main_stop(&gui_btn_main_focus_key_style, 0);
	lv_style_set_bg_opa(&gui_btn_main_focus_key_style, 255);
	lv_style_set_shadow_width(&gui_btn_main_focus_key_style, 0);
	lv_style_set_shadow_color(&gui_btn_main_focus_key_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_opa(&gui_btn_main_focus_key_style, 255);
	lv_style_set_shadow_spread(&gui_btn_main_focus_key_style, 0);
	lv_style_set_shadow_offset_x(&gui_btn_main_focus_key_style, 0);
	lv_style_set_shadow_offset_y(&gui_btn_main_focus_key_style, 0);
	lv_style_set_border_color(&gui_btn_main_focus_key_style, lv_color_make(0xC3, 0x39, 0x2E));
	lv_style_set_border_width(&gui_btn_main_focus_key_style, 2);
	lv_style_set_border_opa(&gui_btn_main_focus_key_style, 255);
	lv_style_set_border_side(&gui_btn_main_focus_key_style, LV_BORDER_SIDE_FULL);
	lv_style_set_outline_color(&gui_btn_main_focus_key_style, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_outline_width(&gui_btn_main_focus_key_style, 2);
	lv_style_set_outline_opa(&gui_btn_main_focus_key_style, 128);
	lv_style_set_outline_pad(&gui_btn_main_focus_key_style, 2);
	lv_style_set_text_color(&gui_btn_main_focus_key_style, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_text_font(&gui_btn_main_focus_key_style, &lv_font_MiSans_Medium_30);

	// animimg
	lv_style_init(&gui_animimg_main_default_style);
	lv_style_set_radius(&gui_animimg_main_default_style, 0);
	lv_style_set_clip_corner(&gui_animimg_main_default_style, true);

}

void ui_style_set(lv_obj_t * obj, gui_ctrl_type_t type)
{
	switch (type) {
	case GUI_CTRL_DIGITALCLOCK:
		lv_obj_add_style(obj, &gui_digitclock_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_SLIDER:
		lv_obj_add_style(obj, &gui_slider_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_slider_indicator_default_style, LV_PART_INDICATOR|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_slider_knob_default_style, LV_PART_KNOB|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_IMG:
		lv_obj_add_style(obj, &gui_img_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_SCR:
		lv_obj_add_style(obj, &gui_scr_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_IMGBTN:
		lv_obj_add_style(obj, &gui_imgbtn_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_imgbtn_main_pressed_style, LV_PART_MAIN|LV_STATE_PRESSED);
		lv_obj_add_style(obj, &gui_imgbtn_main_checked_style, LV_PART_MAIN|LV_STATE_CHECKED);
	break;
	case GUI_CTRL_LINE:
		lv_obj_add_style(obj, &gui_line_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_LABEL:
		lv_obj_add_style(obj, &gui_label_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_IMGLIST:
		lv_obj_add_style(obj, &gui_imglist_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_TEXTAREA:
		lv_obj_add_style(obj, &gui_ta_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_ta_scrollbar_default_style, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_BAR:
		lv_obj_add_style(obj, &gui_bar_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_bar_indicator_default_style, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_ARC:
		lv_obj_add_style(obj, &gui_arc_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_arc_indicator_default_style, LV_PART_INDICATOR|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_arc_knob_default_style, LV_PART_KNOB|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_ARCMENU:
		lv_obj_add_style(obj, &gui_arcmenu_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_arcmenu_arc_menu_items_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_CONT:
		lv_obj_add_style(obj, &gui_cont_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	case GUI_CTRL_BUTTON:
		lv_obj_add_style(obj, &gui_btn_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(obj, &gui_btn_main_focus_key_style, LV_PART_MAIN|LV_STATE_FOCUS_KEY);
	break;
	case GUI_CTRL_ANIMIMG:
		lv_obj_add_style(obj, &gui_animimg_main_default_style, LV_PART_MAIN|LV_STATE_DEFAULT);
	break;
	default:
	break;
	}
}

