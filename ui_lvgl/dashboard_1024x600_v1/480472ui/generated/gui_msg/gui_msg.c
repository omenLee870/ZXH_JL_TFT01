/*Generate Code, Do NOT Edit!*/
#include "./gui_msg.h"
#include "string.h"

gui_msg_data_t guider_msg_data = {0};
gui_msg_send_status_t gui_msg_send_status = GUI_MSG_SEND_DONE;
static lv_ll_t subs_ll;

void gui_msg_init(lv_ui *ui)
{
#if LV_USE_OBSERVER
	lv_ll_init(&subs_ll, sizeof (gui_msg_sub_t));
	gui_model_msg_init(ui);
	gui_sys_model_msg_init(ui);
#endif
}
void gui_msg_init_ui()
{
#if LV_USE_OBSERVER
	gui_model_msg_init_ui();
	gui_sys_model_msg_init_ui();
#endif
}
void gui_msg_init_events()
{
#if LV_USE_OBSERVER
	gui_model_msg_init_events();
	gui_sys_model_msg_init_events();
#endif
}
void gui_msg_unsubscribe()
{
#if LV_USE_OBSERVER
	gui_model_msg_unsubscribe();
	gui_sys_model_msg_unsubscribe();
#endif
}
gui_msg_status_t gui_msg_send(int32_t msg_id, void * value, int32_t len)
{
#if LV_USE_OBSERVER
	gui_msg_send_status = GUI_MSG_SENDING;
	gui_msg_status_t ret;
	switch (msg_id)
	{
		case GUI_MODEL_MSG_ID_PROP:
			ret = gui_model_msg_send(msg_id, value, len);
			gui_msg_send_status = GUI_MSG_SEND_DONE;
			return ret;

		case GUI_SYS_MODEL_MSG_ID_SYS_TIME:
			ret = gui_sys_model_msg_send(msg_id, value, len);
			gui_msg_send_status = GUI_MSG_SEND_DONE;
			return ret;

	default:
		break;
	}
	gui_msg_send_status = GUI_MSG_SEND_DONE;
	return GUI_MSG_STATUS_NO_FOUND_ID;
#else
	return GUI_MSG_STATUS_ERROR;
#endif
}
gui_msg_send_status_t gui_msg_get_send_status()
{
	return gui_msg_send_status;
}
GUI_WEAK gui_msg_data_t * gui_msg_get(int32_t msg_id)
{
	return gui_msg_get_guider(msg_id);
}
gui_msg_data_t * gui_msg_get_guider(int32_t msg_id)
{
#if LV_USE_OBSERVER
#if !LV_USE_GUIBUILDER_SIMULATOR
	if (msg_id >= GUI_MSG_ID_CUSTOM_START && msg_id <= GUI_MSG_ID_CUSTOM_END) {
		int32_t index = msg_id - GUI_MSG_ID_CUSTOM_START;
		gui_msg_entry_table[index].cb(GUI_MSG_ACCESS_GET, &guider_msg_data, gui_msg_entry_table[index].type);
		return &guider_msg_data;
	}
	else {
		for (int i = GUI_MSG_ID_CUSTOM_END - GUI_MSG_ID_CUSTOM_START + 1; i < sizeof(gui_msg_entry_table) / sizeof(gui_msg_entry_table[0]); i++) {
			if (msg_id == gui_msg_entry_table[i].msg_id) {
				gui_msg_entry_table[i].cb(GUI_MSG_ACCESS_GET, &guider_msg_data, gui_msg_entry_table[i].type);
				return &guider_msg_data;
			}
		}
	}
	return NULL;
#else
	if (msg_id >= GUI_MODEL_MSG_ID_PROP && msg_id <= GUI_MODEL_MSG_ID_PROP) {
		int32_t index = msg_id - GUI_MODEL_MSG_ID_PROP;
		gui_model_msg_entry_table[index].cb(GUI_MSG_ACCESS_GET, &guider_msg_data, gui_model_msg_entry_table[index].type);
		return &guider_msg_data;
	} else {
		for (int i = 0; i < 1; i++) {
			if (msg_id == gui_sys_model_msg_entry_table[i].msg_id) {
				gui_sys_model_msg_entry_table[i].cb(GUI_MSG_ACCESS_GET, &guider_msg_data, gui_sys_model_msg_entry_table[i].type);
				return &guider_msg_data;
			}
		}
	}
	return NULL;
#endif
#endif
}
GUI_WEAK void gui_msg_action_change(int32_t msg_id, gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	gui_msg_action_change_guider(msg_id, access, data, type);
}
void gui_msg_action_change_guider(int32_t msg_id, gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
#if LV_USE_OBSERVER
#if !LV_USE_GUIBUILDER_SIMULATOR
	if (msg_id >= GUI_MSG_ID_CUSTOM_START && msg_id <= GUI_MSG_ID_CUSTOM_END) {
		int32_t index = msg_id - GUI_MSG_ID_CUSTOM_START;
		gui_msg_entry_table[index].cb(access, data, type);
	}
	else {
		for (int i = GUI_MSG_ID_CUSTOM_END - GUI_MSG_ID_CUSTOM_START + 1; i < sizeof(gui_msg_entry_table) / sizeof(gui_msg_entry_table[0]); i++) {
			if (msg_id == gui_msg_entry_table[i].msg_id) {
				gui_msg_entry_table[i].cb(access, data, type);
				return;
			}
		}
	}
#else
	if (msg_id >= GUI_MODEL_MSG_ID_PROP && msg_id <= GUI_MODEL_MSG_ID_PROP) {
		int32_t index = msg_id - GUI_MODEL_MSG_ID_PROP;
		gui_model_msg_entry_table[index].cb(access, data, type);
	} else {
		for (int i = 0; i < 1; i++) {
			if (msg_id == gui_sys_model_msg_entry_table[i].msg_id) {
				gui_sys_model_msg_entry_table[i].cb(access, data, type);
				return;
			}
		}
	}
#endif
#endif
}
GUI_WEAK void gui_msg_subscribe_change(int32_t msg_id, gui_msg_subscribe_t sub_type)
{
	gui_msg_subscribe_change_guider(msg_id, sub_type);
}
void gui_msg_subscribe_change_guider(int32_t msg_id, gui_msg_subscribe_t sub_type)
{
}
lv_subject_t * gui_msg_get_subject(int32_t msg_id)
{
	gui_msg_sub_t * sub = gui_msg_get_sub(msg_id);
	if (sub != NULL) return sub->subject;
	return NULL;
}
gui_msg_sub_t * gui_msg_get_sub(int32_t msg_id)
{
	gui_msg_sub_t * head = lv_ll_get_head(&subs_ll);
	while (head != NULL)
	{
		if (head->msg_id == msg_id) return head;
		head = lv_ll_get_next(&subs_ll, head);
	}
	return NULL;
}
gui_msg_sub_t * gui_msg_create_sub(int32_t msg_id)
{
	gui_msg_sub_t * sub = gui_msg_get_sub(msg_id);
	if (sub == NULL)
	{
		sub = lv_ll_ins_tail(&subs_ll);
		sub->msg_id = msg_id;
		sub->subject = lv_malloc(sizeof(lv_subject_t));
		if (sub->subject == NULL) return NULL;
	}
	return sub;
}
gui_msg_data_t * gui_msg_get_data()
{
	return &guider_msg_data;
}
char * gui_msg_get_name(int32_t msg_id)
{
	switch (msg_id)
	{
		case GUI_MODEL_MSG_ID: return "MODEL";
		case GUI_MODEL_MSG_ID_PROP: return "MODEL_PROP";

		case GUI_SYS_MODEL_MSG_ID: return "SYS_MODEL";
		case GUI_SYS_MODEL_MSG_ID_SYS_TIME: return "SYS_MODEL_SYS_TIME";

	default:
		break;
	}
	return NULL;
}
gui_msg_data_type_t gui_msg_get_data_type(int32_t msg_id)
{
#if !LV_USE_GUIBUILDER_SIMULATOR
	if (msg_id >= GUI_MSG_ID_CUSTOM_START && msg_id <= GUI_MSG_ID_CUSTOM_END) {
		return gui_msg_entry_table[msg_id - GUI_MSG_ID_CUSTOM_START].type;
	} else {
		for (int i = GUI_MSG_ID_CUSTOM_END - GUI_MSG_ID_CUSTOM_START + 1; i < sizeof(gui_msg_entry_table) / sizeof(gui_msg_entry_table[0]); i++) {
			if (msg_id == gui_msg_entry_table[i].msg_id) {
				return gui_msg_entry_table[i].type;
			}
		}
	}
	return VALUE_UNKNOWN;
#else
	if (msg_id >= GUI_MODEL_MSG_ID_PROP && msg_id <= GUI_MODEL_MSG_ID_PROP) {
		return gui_model_msg_entry_table[msg_id - GUI_MODEL_MSG_ID_PROP].type;
	} else {
		for (int i = 0; i < 1; i++) {
			if (msg_id == gui_sys_model_msg_entry_table[i].msg_id) {
				return gui_sys_model_msg_entry_table[i].type;
			}
		}
	}
	return VALUE_UNKNOWN;
#endif
}
gui_msg_array_type_t gui_msg_get_data_array_type(int32_t msg_id)
{
	switch (msg_id)
	{
		default: return GUI_MSG_ARRAY_TYPE_INVALID;
	}
}
bool gui_msg_has_observer(lv_subject_t * subject, lv_observer_cb_t cb, lv_obj_t * obj, void * user_data)
{
	if(subject == NULL || subject->type == LV_SUBJECT_TYPE_INVALID) {
		return false;
	}
	lv_observer_t * observer = lv_ll_get_head(&(subject->subs_ll));
	while (observer != NULL) {
		if (observer->cb == cb && observer->target == obj && observer->user_data == user_data) {
			return true;
		}
		observer = lv_ll_get_next(&(subject->subs_ll), observer);
	}
	return false;
}
void gui_msg_setup_component(bool subscribe_enabled, bool event_enabled, lv_subject_t *subject, lv_obj_t *target_obj, gui_msg_data_t *msg_data, lv_observer_cb_t observer_cb, int32_t msg_id, gui_msg_action_t msg_action, gui_msg_data_type_t data_type, lv_event_cb_t event_cb)
{
	if(subject == NULL || subject->type == LV_SUBJECT_TYPE_INVALID) {
		return;
	}

	if(subscribe_enabled) {
		if(!gui_msg_has_observer(subject, observer_cb, target_obj, msg_data)) {
			gui_msg_action_change(msg_id, msg_action, msg_data, data_type);
			lv_subject_add_observer_obj(subject, observer_cb, target_obj, msg_data);
		}
	}

	if(event_enabled) {
		lv_obj_remove_event_cb(target_obj, event_cb);
		lv_obj_add_event_cb(target_obj, event_cb, LV_EVENT_VALUE_CHANGED, (void *)msg_id);
	}
}

void _gui_msg_int32_cb(int32_t *var_ptr, gui_msg_action_t access, gui_msg_data_t * data)
{
	if (access == GUI_MSG_ACCESS_SET) {
		*var_ptr = data->value_int;
	}
	data->value_int = *var_ptr;
	return;
}
void _gui_msg_tm_cb(struct tm *var_ptr, bool is_systime, gui_msg_action_t access, gui_msg_data_t * data)
{
	if (access == GUI_MSG_ACCESS_SET) {
		*var_ptr = data->value_time;
	} else {
		if (is_systime) {
			time_t now = time(NULL);
			struct tm *tm = localtime(&now);
			if (tm != NULL) {
				*var_ptr = *tm;
			}
		}
	}
	data->value_time = *var_ptr;
	return;
}
