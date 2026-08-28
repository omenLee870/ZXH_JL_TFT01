#ifndef __JLUI_DEMO_CFG_H__
#define __JLUI_DEMO_CFG_H__


#include "board_config.h"

#if (defined JLUI_DEMO_ENABLE && JLUI_DEMO_ENABLE)

#define JLUI_DEMO_IMAGE_ENABLE	1
#define JLUI_DEMO_NUMBER_ENABLE 1
#define JLUI_DEMO_TEXT_ENABLE   1
#define JLUI_DEMO_TIME_ENABLE   1
#define JLUI_DEMO_WATCH_ENABLE  1
#define JLUI_DEMO_HSLIDER_ENABLE 1
#define JLUI_DEMO_VSLIDER_ENABLE 1
#define JLUI_DEMO_COMPASS_ENABLE 1

#define JLUI_PAGE_SWITCH_ENABLE 1 /*! 页面自动切换 */
#define JLUI_DEMO_LAYOUT_SCALE  1 /*! 布局缩放 */
#define JLUI_DEMO_LAYOUT_ROTATE 1 /*! 布局旋转 */


#define JLUI_DEMO_DEFAULT_TIME	50 /*! 默认刷新时间间隔 */

#define JLUI_DEMO_REFRESH()		ui_core_redraw(ui_core_get_element_by_id(ui_get_current_window_id()))

#define JLUI_DEMO_SWITCH(page)                   \
	do{                                          \
		u32 __win = ui_get_current_window_id();  \
		if (__win && (__win != page)) {          \
			ui_hide(ui_get_current_window_id()); \
			UI_SHOW_WINDOW(page);                \
		}                                        \
	} while (0)


int jlui_page_auto_switch();
int jlui_layout_scale();
int jlui_layout_rotate();
int jlui_demo_hslider();
int jlui_demo_vslider();
int jlui_demo_time();
int jlui_demo_number();
int jlui_demo_compass();
int jlui_demo_watch();

#endif

#endif


