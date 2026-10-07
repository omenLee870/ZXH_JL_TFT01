/*Generate Code, Do NOT Edit!*/
#ifndef GUI_MSG_H_
#define GUI_MSG_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include "../gui_guider.h"
#include "lvgl.h"
#include "../../custom/custom.h"
#include "time.h"

typedef enum {
	GUI_MSG_ACCESS_GET = 0,
	GUI_MSG_ACCESS_SET,
} gui_msg_action_t;

typedef enum {
	GUI_MSG_SUBSCRIBE = 0,
	GUI_MSG_UNSUBSCRIBE,
} gui_msg_subscribe_t;

typedef enum {
	VALUE_INT = 1,
	VALUE_CHAR,
	VALUE_SHORT,
	VALUE_BOOL,
	VALUE_STRING,
	VALUE_COLOR,
	VALUE_DATE,
	VALUE_POINT,
	VALUE_TIME,
	VALUE_ARRAY,
	VALUE_UNKNOWN = 0,
} gui_msg_data_type_t;

typedef enum {
	GUI_MSG_ARRAY_TYPE_INVALID = 0,
	GUI_MSG_ARRAY_TYPE_STRING,
	GUI_MSG_ARRAY_TYPE_INT32,
	GUI_MSG_ARRAY_TYPE_BOOL,
	GUI_MSG_ARRAY_TYPE_COLOR,
	GUI_MSG_ARRAY_TYPE_DATE,
	GUI_MSG_ARRAY_TYPE_POINT,
	GUI_MSG_ARRAY_TYPE_TIME,
} gui_msg_array_type_t;

typedef enum {
	GUI_MSG_STATUS_SUCCESS = 0,
	GUI_MSG_STATUS_ERROR = -1,
	GUI_MSG_STATUS_NO_SUBSCRIBE = -2,
	GUI_MSG_STATUS_NO_FOUND_ID = -3,
} gui_msg_status_t;

typedef enum {
	GUI_MSG_SEND_DONE = 0,
	GUI_MSG_SENDING,
} gui_msg_send_status_t;

typedef union {
	int32_t value_int;
	bool value_bool;
	char * value_string;
	lv_color_t value_color;
	lv_calendar_date_t value_date;
	lv_point_t value_point;
	struct tm value_time;
	struct {
		void * ptr;
		int32_t len;
	} value_array;
} gui_msg_data_t;

typedef enum {
	GUI_MODEL_MSG_ID = 1,
	GUI_SYS_MODEL_MSG_ID = 0xF000,
} gui_msg_id_t;

enum {
	GUI_MSG_ID_CUSTOM_START = 0x2,

	GUI_MODEL_MSG_ID_PROP = 0x2,

	GUI_SYS_MODEL_MSG_ID_SYS_TIME = 0xF001,

	GUI_MSG_ID_CUSTOM_END = 0x2
};

typedef struct {
	lv_subject_t * subject;
	int32_t msg_id;
} gui_msg_sub_t;

typedef struct {
	int32_t msg_id;
	char is_subscribe;
	char is_unsubscribe;
} _gui_msg_status_t;

typedef int (*gui_msg_action_cb_t)(gui_msg_action_t, gui_msg_data_t *, gui_msg_data_type_t);

typedef struct {
	int32_t msg_id;
	gui_msg_action_cb_t cb;
	int32_t type;
} _gui_msg_entry_t;

extern void gui_msg_init(lv_ui *ui);
extern void gui_msg_init_ui();
extern void gui_msg_init_events();
extern void gui_msg_unsubscribe();
extern gui_msg_status_t gui_msg_send(int32_t msg_id, void * value, int32_t len);
extern gui_msg_send_status_t gui_msg_get_send_status();
#if LV_USE_GUIBUILDER_SIMULATOR && LV_USE_CUSTOM_MSG_GET
extern gui_msg_data_t * gui_msg_get(int32_t msg_id);
#else
extern GUI_WEAKREF gui_msg_data_t * gui_msg_get(int32_t msg_id);
#endif
extern gui_msg_data_t * gui_msg_get_guider(int32_t msg_id);
#if LV_USE_GUIBUILDER_SIMULATOR && LV_USE_CUSTOM_MSG_ACTION_CHANGE
extern void gui_msg_action_change(int32_t msg_id, gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type);
#else
extern GUI_WEAKREF void gui_msg_action_change(int32_t msg_id, gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type);
#endif
extern void gui_msg_action_change_guider(int32_t msg_id, gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type);
#if LV_USE_GUIBUILDER_SIMULATOR && LV_USE_CUSTOM_MSG_SUBSCRIBE_CHANGE
extern void gui_msg_subscribe_change(int32_t msg_id, gui_msg_subscribe_t sub_type);
#else
extern GUI_WEAKREF void gui_msg_subscribe_change(int32_t msg_id, gui_msg_subscribe_t sub_type);
#endif
extern void gui_msg_subscribe_change_guider(int32_t msg_id, gui_msg_subscribe_t sub_type);
extern lv_subject_t * gui_msg_get_subject(int32_t msg_id);
extern gui_msg_sub_t * gui_msg_get_sub(int32_t msg_id);
extern gui_msg_sub_t * gui_msg_create_sub(int32_t msg_id);
extern gui_msg_data_t * gui_msg_get_data();
extern gui_msg_data_type_t gui_msg_get_data_type(int32_t msg_id);
extern gui_msg_array_type_t gui_msg_get_data_array_type(int32_t msg_id);
extern char * gui_msg_get_name(int32_t msg_id);
extern bool gui_msg_has_observer(lv_subject_t * subject, lv_observer_cb_t cb, lv_obj_t * obj, void * user_data);
extern void gui_msg_setup_component(bool subscribe_enabled, bool event_enabled, lv_subject_t *subject, lv_obj_t *target_obj, gui_msg_data_t *msg_data, lv_observer_cb_t observer_cb, int32_t msg_id, gui_msg_action_t msg_action, gui_msg_data_type_t data_type, lv_event_cb_t event_cb);

extern void _gui_msg_int32_cb(int32_t *var_ptr, gui_msg_action_t access, gui_msg_data_t * data);
extern void _gui_msg_tm_cb(struct tm *var_ptr, bool is_systime, gui_msg_action_t access, gui_msg_data_t * data);
#if LV_USE_OBSERVER
#include "./gui_model_msg.h"
#include "./gui_sys_model_msg.h"
#define GUI_MSG_MAX_ID 0xf001

extern int gui_model_msg_prop_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type);

extern int gui_sys_model_msg_sys_time_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type);

#if !LV_USE_GUIBUILDER_SIMULATOR
static const _gui_msg_entry_t gui_msg_entry_table[] = {
	{ GUI_MODEL_MSG_ID_PROP, gui_model_msg_prop_cb, VALUE_INT },

	{ GUI_SYS_MODEL_MSG_ID_SYS_TIME, gui_sys_model_msg_sys_time_cb, VALUE_TIME },
};
#endif

#endif

extern gui_msg_data_t guider_msg_data;

#ifdef __cplusplus
}
#endif
#endif /* GUI_MSG_H_ */
