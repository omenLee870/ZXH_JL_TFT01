/*Generate Code, Do NOT Edit!*/
#ifndef GUI_SYS_MODEL_MSG_H_
#define GUI_SYS_MODEL_MSG_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "./gui_msg.h"

#if LV_USE_OBSERVER

extern void gui_sys_model_msg_init(lv_ui *ui);
extern void gui_sys_model_msg_init_ui();
extern void gui_sys_model_msg_init_events();
extern void gui_sys_model_msg_unsubscribe();
extern gui_msg_status_t gui_sys_model_msg_send(int32_t msg_id, void * value, int32_t len);

#if LV_USE_GUIBUILDER_SIMULATOR
extern _gui_msg_entry_t gui_sys_model_msg_entry_table[];
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif /* GUI_SYS_MODEL_MSG_H_ */
