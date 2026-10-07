/*Generate Code, Do NOT Edit!*/
#ifndef UI_STYLE_H
#define UI_STYLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// slider
extern lv_style_t gui_slider_main_default_style;
extern lv_style_t gui_slider_indicator_default_style;
extern lv_style_t gui_slider_knob_default_style;

// img
extern lv_style_t gui_img_main_default_style;

// scr
extern lv_style_t gui_scr_main_default_style;

// menu_item
extern lv_style_t gui_menu_item_menu_text_item_default_style;

// label
extern lv_style_t gui_label_main_default_style;

// cont
extern lv_style_t gui_cont_main_default_style;

// imglist
extern lv_style_t gui_imglist_main_default_style;

// btn
extern lv_style_t gui_btn_main_default_style;

enum {
	GUI_CTRL_SLIDER,
	GUI_CTRL_IMG,
	GUI_CTRL_SCR,
	null,
	GUI_CTRL_LABEL,
	GUI_CTRL_CONT,
	GUI_CTRL_IMGLIST,
	GUI_CTRL_BUTTON,
};
typedef uint8_t gui_ctrl_type_t;

extern void ui_style_init(void);
extern void ui_style_set(lv_obj_t * obj, gui_ctrl_type_t type);

#ifdef __cplusplus
}
#endif
#endif /* UI_STYLE_H */
