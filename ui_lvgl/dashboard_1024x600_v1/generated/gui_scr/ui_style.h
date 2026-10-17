/*Generate Code, Do NOT Edit!*/
#ifndef UI_STYLE_H
#define UI_STYLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// digitclock
extern lv_style_t gui_digitclock_main_default_style;

// slider
extern lv_style_t gui_slider_main_default_style;
extern lv_style_t gui_slider_indicator_default_style;
extern lv_style_t gui_slider_knob_default_style;

// img
extern lv_style_t gui_img_main_default_style;

// scr
extern lv_style_t gui_scr_main_default_style;

// imgbtn
extern lv_style_t gui_imgbtn_main_default_style;
extern lv_style_t gui_imgbtn_main_pressed_style;
extern lv_style_t gui_imgbtn_main_checked_style;

// sw
extern lv_style_t gui_sw_main_default_style;
extern lv_style_t gui_sw_indicator_checked_style;
extern lv_style_t gui_sw_knob_default_style;

// line
extern lv_style_t gui_line_main_default_style;

// label
extern lv_style_t gui_label_main_default_style;

// imglist
extern lv_style_t gui_imglist_main_default_style;

// ta
extern lv_style_t gui_ta_main_default_style;
extern lv_style_t gui_ta_scrollbar_default_style;

// bar
extern lv_style_t gui_bar_main_default_style;
extern lv_style_t gui_bar_indicator_default_style;

// arc
extern lv_style_t gui_arc_main_default_style;
extern lv_style_t gui_arc_indicator_default_style;
extern lv_style_t gui_arc_knob_default_style;

// arcmenu
extern lv_style_t gui_arcmenu_main_default_style;
extern lv_style_t gui_arcmenu_arc_menu_items_default_style;

// cont
extern lv_style_t gui_cont_main_default_style;

// btn
extern lv_style_t gui_btn_main_default_style;
extern lv_style_t gui_btn_main_focus_key_style;

// animimg
extern lv_style_t gui_animimg_main_default_style;

enum {
	GUI_CTRL_DIGITALCLOCK,
	GUI_CTRL_SLIDER,
	GUI_CTRL_IMG,
	GUI_CTRL_SCR,
	GUI_CTRL_IMGBTN,
	GUI_CTRL_SWITCH,
	GUI_CTRL_LINE,
	GUI_CTRL_LABEL,
	GUI_CTRL_IMGLIST,
	GUI_CTRL_TEXTAREA,
	GUI_CTRL_BAR,
	GUI_CTRL_ARC,
	GUI_CTRL_ARCMENU,
	GUI_CTRL_CONT,
	GUI_CTRL_BUTTON,
	GUI_CTRL_ANIMIMG,
};
typedef uint8_t gui_ctrl_type_t;

extern void ui_style_init(void);
extern void ui_style_set(lv_obj_t * obj, gui_ctrl_type_t type);

#ifdef __cplusplus
}
#endif
#endif /* UI_STYLE_H */
