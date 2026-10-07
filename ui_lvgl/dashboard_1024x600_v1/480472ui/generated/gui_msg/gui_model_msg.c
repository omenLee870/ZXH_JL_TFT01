/*Generate Code, Do NOT Edit!*/
#include "./gui_model_msg.h"
#if LV_USE_OBSERVER

static lv_ll_t subs_ll;

#if LV_USE_GUIBUILDER_SIMULATOR
_gui_msg_entry_t gui_model_msg_entry_table[] = {
	{ GUI_MODEL_MSG_ID_PROP, gui_model_msg_prop_cb, VALUE_INT },
	};
#endif


GUI_WEAK int gui_model_msg_prop_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	static int32_t prop_var = 1;
	_gui_msg_int32_cb(&prop_var, access, data);
	return 0;
}

void gui_model_msg_init(lv_ui *ui)
{
	int32_t ids[1] = {
		GUI_MODEL_MSG_ID_PROP,
	};
	for (int i = 0; i < 1; i++) {
		gui_msg_sub_t * sub = gui_msg_create_sub(ids[i]);
		if (sub != NULL) lv_subject_init_pointer(sub->subject, &guider_msg_data);
	}
}

void gui_model_msg_init_ui()
{
}

void gui_model_msg_init_events()
{
	void * res = NULL;
	_gui_msg_status_t status[1] = {
		{GUI_MODEL_MSG_ID_PROP, 0, 0},
	};

	for (int i = 0; i < 1; i++) {
		lv_subject_t * subject = gui_msg_get_subject(status[i].msg_id);
		if (subject == NULL) continue;
		lv_ll_t subject_ll = subject->subs_ll;
		gui_msg_sub_t * head = lv_ll_get_head(&subject_ll);
		if (head != NULL) status[i].is_unsubscribe = 1;
	}


	for (int i = 0; i < 1; i++) {
		if (status[i].is_subscribe == 0 && status[i].is_unsubscribe == 1) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_UNSUBSCRIBE);
		} else if (status[i].is_subscribe == 1 && status[i].is_unsubscribe == 0) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_SUBSCRIBE);
		}
	}
}

void gui_model_msg_unsubscribe()
{
	_gui_msg_status_t status[1] = {
		{GUI_MODEL_MSG_ID_PROP, 0, 0},
	};
	for (int i = 0; i < 1; i++) {
		lv_subject_t * subject = gui_msg_get_subject(status[i].msg_id);
		if (subject == NULL) continue;
		lv_ll_t subject_ll = subject->subs_ll;
		lv_observer_t * head = lv_ll_get_head(&subject_ll);
		if (head != NULL) status[i].is_unsubscribe = 1;
		while (head != NULL) {
			lv_obj_t * obj = head->target;
			if (obj != NULL && lv_obj_is_valid(obj) == true) {
				lv_obj_remove_from_subject(obj, subject);
			}
			head = lv_ll_get_head(&subject_ll);
		}
	}

	for (int i = 0; i < 1; i++) {
		if (status[i].is_unsubscribe == 1) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_UNSUBSCRIBE);
		}
	}
}

gui_msg_status_t gui_model_msg_send(int32_t msg_id, void * value, int32_t len)
{
	if (msg_id == GUI_MODEL_MSG_ID) {
	} else {
		gui_msg_data_type_t data_type = VALUE_INT;
		switch (msg_id) {
			case GUI_MODEL_MSG_ID_PROP:
				{
					data_type = VALUE_INT;
					guider_msg_data.value_array.ptr = value;
					guider_msg_data.value_array.len = len;
				}
			break;
			default: 
				break;
		}
		gui_msg_action_change(msg_id, GUI_MSG_ACCESS_SET, &guider_msg_data, data_type);
		lv_subject_t * subject = gui_msg_get_subject(msg_id);
		if (subject == NULL) return GUI_MSG_STATUS_NO_SUBSCRIBE;
		lv_subject_set_pointer(subject, &guider_msg_data);
	}
	return GUI_MSG_STATUS_SUCCESS;
}

#endif
