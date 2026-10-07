/*Generate Code, Do NOT Edit!*/
#ifndef SCREEN_HANDLER_H_
#define SCREEN_HANDLER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "../gui_guider.h"
//screen action function
#if LV_USE_GUIBUILDER_SIMULATOR && LV_USE_CUSTOM_SCR_ACTION_CB
extern void gui_scr_action_cb(gui_screen_id_t id, gui_screen_action_t action);
#else
extern GUI_WEAKREF void gui_scr_action_cb(gui_screen_id_t id, gui_screen_action_t action);
#endif

#ifdef __cplusplus
}
#endif
#endif /* SCREEN_HANDLER_H_ */
