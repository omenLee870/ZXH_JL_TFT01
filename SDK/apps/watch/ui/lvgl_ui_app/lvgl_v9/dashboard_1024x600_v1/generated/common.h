/*Generate Code, Do NOT Edit!*/
#ifndef __COMMON_H__
#define __COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "./gui_res/res_common.h"

#define GUI_UIT_VERSION_MAJOR 1
#define GUI_UIT_VERSION_MINOR 0
#define GUI_UIT_VERSION_PATCH 0
#define GUI_UIT_VERSION_INFO "20260915"

#define GUI_CORE_VERSION_MAJOR 0
#define GUI_CORE_VERSION_MINOR 0
#define GUI_CORE_VERSION_PATCH 1
#define GUI_CORE_VERSION_INFO "LVGL_v9_CORE"

#define GUI_TEMPLATE_VERSION_MAJOR 0
#define GUI_TEMPLATE_VERSION_MINOR 0
#define GUI_TEMPLATE_VERSION_PATCH 1
#define GUI_TEMPLATE_VERSION_INFO "Empty(V9)"

#if LV_USE_GUIBUILDER_SIMULATOR
#define GUI_TM_MON(mon) ((mon) + 1)
#else
#define GUI_TM_MON(mon) (mon)
#endif

typedef enum {
	GUI_SCREEN_ACTION_LOAD = 0,
	GUI_SCREEN_ACTION_UNLOAD
} gui_screen_action_t;

typedef enum {
	GUI_SCREEN_PAGE_MAP = 0,
	GUI_SCREEN_PAGE_MUSIC,
	GUI_SCREEN_PAGE_APP_SELECT,
	GUI_SCREEN_PAGE_METER,
	GUI_SCREEN_PAGE_VIDEO,
	GUI_SCREEN_UPGRADE_PAGE,
	GUI_SCREEN_HARDWARE_TEST,
} gui_screen_id_t;

static const int32_t gui_screen_ids[] = {
	GUI_SCREEN_PAGE_MAP,
	GUI_SCREEN_PAGE_MUSIC,
	GUI_SCREEN_PAGE_APP_SELECT,
	GUI_SCREEN_PAGE_METER,
	GUI_SCREEN_PAGE_VIDEO,
	GUI_SCREEN_UPGRADE_PAGE,
	GUI_SCREEN_HARDWARE_TEST,
};

#define GUI_KEY_F1 1
#define GUI_KEY_up 200
#define GUI_KEY_left 201
#define GUI_KEY_right 202
#define GUI_KEY_down 203
#define GUI_KEY_return 204
#define GUI_KEY_ok 205

#ifdef __cplusplus
}
#endif
#endif
