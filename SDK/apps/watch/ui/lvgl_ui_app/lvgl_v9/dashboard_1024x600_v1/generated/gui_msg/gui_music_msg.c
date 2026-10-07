/*Generate Code, Do NOT Edit!*/
#include "./gui_music_msg.h"

#if LV_USE_OBSERVER

static lv_ll_t subs_ll;

#if LV_USE_GUIBUILDER_SIMULATOR
_gui_msg_entry_t gui_music_msg_entry_table[] = {
	{ GUI_MUSIC_MSG_ID_PLAYER_STATE, gui_music_msg_player_state_cb, VALUE_INT },
	{ GUI_MUSIC_MSG_ID_TOL_TIME, gui_music_msg_tol_time_cb, VALUE_STRING },
	{ GUI_MUSIC_MSG_ID_CUR_TIME, gui_music_msg_cur_time_cb, VALUE_STRING },
	{ GUI_MUSIC_MSG_ID_MUSIC_STATE, gui_music_msg_music_state_cb, VALUE_INT },
	{ GUI_MUSIC_MSG_ID_MUSIC_PROCESS, gui_music_msg_music_process_cb, VALUE_INT },
	{ GUI_MUSIC_MSG_ID_MUSIC_LYRICS, gui_music_msg_music_lyrics_cb, VALUE_STRING },
	};
#endif


GUI_WEAK int gui_music_msg_player_state_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	static int32_t player_state_var = 0;
	_gui_msg_int32_cb(&player_state_var, access, data);
	return 0;
}
GUI_WEAK int gui_music_msg_tol_time_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	char tol_time_init_var[] = "03:54";
	static bool tol_time_is_init = false;
	static char * tol_time_var = NULL;
	_gui_msg_char_array_cb(&tol_time_var, tol_time_init_var, &tol_time_is_init, access, data);
	return 0;
}
GUI_WEAK int gui_music_msg_cur_time_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	char cur_time_init_var[] = "00:00";
	static bool cur_time_is_init = false;
	static char * cur_time_var = NULL;
	_gui_msg_char_array_cb(&cur_time_var, cur_time_init_var, &cur_time_is_init, access, data);
	return 0;
}
GUI_WEAK int gui_music_msg_music_state_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	static lv_state_t music_state_var = LV_STATE_DEFAULT;
	_gui_msg_state_cb(&music_state_var, access, data);
	return 0;
}
GUI_WEAK int gui_music_msg_music_process_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	static int32_t music_process_var = 1;
	_gui_msg_int32_cb(&music_process_var, access, data);
	return 0;
}
GUI_WEAK int gui_music_msg_music_lyrics_cb(gui_msg_action_t access, gui_msg_data_t * data, gui_msg_data_type_t type)
{
	char music_lyrics_init_var[] = "周杰伦-青花瓷";
	static bool music_lyrics_is_init = false;
	static char * music_lyrics_var = NULL;
	_gui_msg_char_array_cb(&music_lyrics_var, music_lyrics_init_var, &music_lyrics_is_init, access, data);
	return 0;
}

void gui_music_msg_init(lv_ui *ui)
{
	int32_t ids[6] = {
		GUI_MUSIC_MSG_ID_PLAYER_STATE,
		GUI_MUSIC_MSG_ID_TOL_TIME,
		GUI_MUSIC_MSG_ID_CUR_TIME,
		GUI_MUSIC_MSG_ID_MUSIC_STATE,
		GUI_MUSIC_MSG_ID_MUSIC_PROCESS,
		GUI_MUSIC_MSG_ID_MUSIC_LYRICS,
	};
	for (int i = 0; i < 6; i++) {
		gui_msg_sub_t * sub = gui_msg_create_sub(ids[i]);
		if (sub != NULL) lv_subject_init_pointer(sub->subject, &guider_msg_data);
	}
}

void gui_music_msg_init_ui()
{
}

void gui_music_msg_init_events()
{
	void * res = NULL;
	_gui_msg_status_t status[6] = {
		{GUI_MUSIC_MSG_ID_PLAYER_STATE, 0, 0},
		{GUI_MUSIC_MSG_ID_TOL_TIME, 0, 0},
		{GUI_MUSIC_MSG_ID_CUR_TIME, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_STATE, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_PROCESS, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_LYRICS, 0, 0},
	};

	for (int i = 0; i < 6; i++) {
		lv_subject_t * subject = gui_msg_get_subject(status[i].msg_id);
		if (subject == NULL) continue;
		lv_ll_t subject_ll = subject->subs_ll;
		gui_msg_sub_t * head = lv_ll_get_head(&subject_ll);
		if (head != NULL) status[i].is_unsubscribe = 1;
	}

	lv_subject_t * subject_music_process = gui_msg_get_subject(GUI_MUSIC_MSG_ID_MUSIC_PROCESS);
	lv_subject_t * subject_music_lyrics = gui_msg_get_subject(GUI_MUSIC_MSG_ID_MUSIC_LYRICS);
	lv_subject_t * subject_music_state = gui_msg_get_subject(GUI_MUSIC_MSG_ID_MUSIC_STATE);
	lv_subject_t * subject_cur_time = gui_msg_get_subject(GUI_MUSIC_MSG_ID_CUR_TIME);
	lv_subject_t * subject_tol_time = gui_msg_get_subject(GUI_MUSIC_MSG_ID_TOL_TIME);
	if (guider_ui.page_music) {
		lv_ui_page_music * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
		gui_msg_setup_component(true, true, subject_music_process, ui_scr->page_music_slider_1, &guider_msg_data, gui_msg_set_slider_starting_value_by_int32_cb, GUI_MUSIC_MSG_ID_MUSIC_PROCESS, GUI_MSG_ACCESS_GET, VALUE_INT, gui_msg_change_slider_starting_value_cb);

		gui_msg_setup_component(true, false, subject_music_lyrics, ui_scr->page_music_lbl_1, &guider_msg_data, gui_msg_set_label_text_by_string_cb, GUI_MUSIC_MSG_ID_MUSIC_LYRICS, GUI_MSG_ACCESS_GET, VALUE_STRING, NULL);

		gui_msg_setup_component(true, false, subject_music_state, ui_scr->page_music_imgbtn_1, &guider_msg_data, gui_msg_set_control_state_by_int32_cb, GUI_MUSIC_MSG_ID_MUSIC_STATE, GUI_MSG_ACCESS_GET, VALUE_INT, NULL);

		gui_msg_setup_component(true, false, subject_cur_time, ui_scr->page_music_lbl_3, &guider_msg_data, gui_msg_set_label_text_by_string_cb, GUI_MUSIC_MSG_ID_CUR_TIME, GUI_MSG_ACCESS_GET, VALUE_STRING, NULL);

		gui_msg_setup_component(true, false, subject_tol_time, ui_scr->page_music_lbl_4, &guider_msg_data, gui_msg_set_label_text_by_string_cb, GUI_MUSIC_MSG_ID_TOL_TIME, GUI_MSG_ACCESS_GET, VALUE_STRING, NULL);


		for (int i = 0; i < 6; i++) {
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_MUSIC_PROCESS) {
				status[i].is_subscribe = 1;
			}
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_MUSIC_LYRICS) {
				status[i].is_subscribe = 1;
			}
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_MUSIC_STATE) {
				status[i].is_subscribe = 1;
			}
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_CUR_TIME) {
				status[i].is_subscribe = 1;
			}
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_TOL_TIME) {
				status[i].is_subscribe = 1;
			}
		}
	}
	if (guider_ui.page_app_select) {
		lv_ui_page_app_select * ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
		gui_msg_setup_component(true, true, subject_music_process, ui_scr->page_app_select_slider_1, &guider_msg_data, gui_msg_set_slider_starting_value_by_int32_cb, GUI_MUSIC_MSG_ID_MUSIC_PROCESS, GUI_MSG_ACCESS_GET, VALUE_INT, gui_msg_change_slider_starting_value_cb);

		gui_msg_setup_component(true, false, subject_music_lyrics, ui_scr->page_app_select_lbl_13, &guider_msg_data, gui_msg_set_label_text_by_string_cb, GUI_MUSIC_MSG_ID_MUSIC_LYRICS, GUI_MSG_ACCESS_GET, VALUE_STRING, NULL);


		for (int i = 0; i < 6; i++) {
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_MUSIC_PROCESS) {
				status[i].is_subscribe = 1;
			}
			if (status[i].msg_id == GUI_MUSIC_MSG_ID_MUSIC_LYRICS) {
				status[i].is_subscribe = 1;
			}
		}
	}

	for (int i = 0; i < 6; i++) {
		if (status[i].is_subscribe == 0 && status[i].is_unsubscribe == 1) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_UNSUBSCRIBE);
		} else if (status[i].is_subscribe == 1 && status[i].is_unsubscribe == 0) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_SUBSCRIBE);
		}
	}
}

void gui_music_msg_unsubscribe()
{
	_gui_msg_status_t status[6] = {
		{GUI_MUSIC_MSG_ID_PLAYER_STATE, 0, 0},
		{GUI_MUSIC_MSG_ID_TOL_TIME, 0, 0},
		{GUI_MUSIC_MSG_ID_CUR_TIME, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_STATE, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_PROCESS, 0, 0},
		{GUI_MUSIC_MSG_ID_MUSIC_LYRICS, 0, 0},
	};
	for (int i = 0; i < 6; i++) {
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

	for (int i = 0; i < 6; i++) {
		if (status[i].is_unsubscribe == 1) {
			gui_msg_subscribe_change(status[i].msg_id, GUI_MSG_UNSUBSCRIBE);
		}
	}
}

gui_msg_status_t gui_music_msg_send(int32_t msg_id, void * value, int32_t len)
{
	if (msg_id == GUI_MUSIC_MSG_ID) {
	} else {
		gui_msg_data_type_t data_type = VALUE_INT;
		switch (msg_id) {
			case GUI_MUSIC_MSG_ID_MUSIC_STATE:
				{
					data_type = VALUE_INT;
					guider_msg_data.value_array.ptr = value;
					guider_msg_data.value_array.len = len;
				}
			break;
			case GUI_MUSIC_MSG_ID_TOL_TIME:
			case GUI_MUSIC_MSG_ID_CUR_TIME:
			case GUI_MUSIC_MSG_ID_MUSIC_LYRICS:
				{
					data_type = VALUE_STRING;
					guider_msg_data.value_array.ptr = value;
					guider_msg_data.value_array.len = len;
				}
			break;
			case GUI_MUSIC_MSG_ID_PLAYER_STATE:
			case GUI_MUSIC_MSG_ID_MUSIC_PROCESS:
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
