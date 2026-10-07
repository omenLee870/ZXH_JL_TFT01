/*Generate Code, Do NOT Edit!*/
#include "lvgl.h"
#include "../gui_guider.h"
#include "./gui_timelines.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif

void gui_timeline_var_register(gui_timeline_t *timeline, gui_anim_var_t *var)
{
	if (timeline == NULL || var == NULL) return;
	gui_anim_var_t **nl = NULL;
	if (timeline->var_cnt == 0) {
		nl = lv_malloc(sizeof(gui_anim_var_t *));
	} else {
		nl = lv_realloc(timeline->vars, (timeline->var_cnt + 1) * sizeof(gui_anim_var_t *));
	}
	if (nl == NULL) return;
	nl[timeline->var_cnt] = var;
	timeline->vars = nl;
	timeline->var_cnt++;
}

void gui_timeline_var_unregister(gui_timeline_t * timeline)
{
	if (timeline == NULL || timeline->vars == NULL || timeline->var_cnt == 0) return;

	for (uint32_t i = 0; i < timeline->var_cnt; i++) {
		gui_anim_var_t *var = timeline->vars[i];
		if (var) lv_free(var);
		timeline->vars[i] = NULL;
	}
	lv_memzero(timeline->vars, (size_t)timeline->var_cnt * sizeof(gui_anim_var_t *));
	lv_free(timeline->vars);
	timeline->vars = NULL;
	timeline->var_cnt = 0;
}

//Init Timelines
void gui_timelines_init(lv_ui *ui)
{
}
//Stop Timelines
void gui_timelines_stop()
{
}
//Delete Timelines
void gui_timelines_delete()
{
}

void gui_anim_set_var(gui_timeline_t *timeline, lv_anim_t *anim, lv_obj_t *obj, lv_anim_exec_xcb_t exec_cb, gui_anim_data_t data)
{
	lv_anim_set_exec_cb(anim, exec_cb);
	gui_anim_var_t *var = lv_malloc(sizeof(gui_anim_var_t));
	if (var == NULL) return;
	var->obj = obj;
	var->exec_cb = exec_cb;
	var->data = data;
	gui_timeline_var_register(timeline, var);
	lv_anim_set_var(anim, var);
}

void gui_timeline_start(gui_timeline_t *timeline)
{
	if (timeline == NULL) return;
	if (timeline->timeline != NULL) {
		int32_t temp_repeat_count = timeline->_repeat_count;
		timeline->_repeat_count = 0;
		lv_anim_timeline_delete(timeline->timeline);
		gui_timeline_var_unregister(timeline);
		timeline->_repeat_count = temp_repeat_count;
	}

	int32_t res = timeline->init_cb(&guider_ui);
	if(res != -1) {
		lv_anim_timeline_start(timeline->timeline);
	}
}

void gui_timeline_stop(gui_timeline_t *timeline)
{
	if (timeline == NULL) return;
	timeline->_repeat_count = 0;
	if (timeline->timeline != NULL) {
		lv_anim_timeline_pause(timeline->timeline);
	}
}

void gui_timeline_delete(gui_timeline_t *timeline)
{
	if (timeline == NULL) return;
	timeline->_repeat_count = 0;
	timeline->_period = 0;
	if (timeline->timeline != NULL) {
		lv_anim_timeline_delete(timeline->timeline);
		timeline->timeline = NULL;
	}
	gui_timeline_var_unregister(timeline);
}

void gui_timeline_set_period(gui_timeline_t *timeline, uint32_t period)
{
	if (timeline == NULL) return;
	timeline->_period = period;
}

void gui_timeline_set_repeat_count(gui_timeline_t *timeline, int32_t repeat_count)
{
	if (timeline == NULL) return;
	timeline->_repeat_count = repeat_count;
}

