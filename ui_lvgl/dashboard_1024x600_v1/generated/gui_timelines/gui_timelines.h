/*Generate Code, Do NOT Edit!*/
#ifndef GUI_TIMELINES_H_
#define GUI_TIMELINES_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "../gui_guider.h"
#include "lvgl.h"
#include "../../custom/custom.h"

typedef int32_t (*gui_timeline_init_cb)(lv_ui *ui);

typedef union {
	lv_color32_t * value_color_array;
	lv_obj_t * value_obj;
	int32_t value_int;
} gui_anim_data_t;

typedef struct {
	lv_obj_t * obj;
	lv_anim_exec_xcb_t exec_cb;
	gui_anim_data_t data;
} gui_anim_var_t;

typedef struct {
	lv_anim_timeline_t * timeline;
	gui_timeline_init_cb init_cb;
	int32_t _repeat_count;
	int32_t _period;
	gui_anim_var_t ** vars;
	int32_t var_cnt;
} gui_timeline_t;

#define init_gui_timelines(ui) gui_timelines_init(ui)
#define stop_gui_timelines() gui_timelines_stop()
#define delete_gui_timelines() gui_timelines_delete()
#define GUI_ANIM_OBJ(var) (var ? ((gui_anim_var_t *)var)->obj : NULL)
#define GUI_ANIM_DATA_OBJ(var) (var ? ((gui_anim_var_t *)var)->data.value_obj : NULL)
extern void gui_timelines_init(lv_ui *ui);
extern void gui_timelines_stop();
extern void gui_timelines_delete();

extern void gui_anim_set_var(gui_timeline_t *timeline, lv_anim_t *anim, lv_obj_t *obj, lv_anim_exec_xcb_t exec_cb, gui_anim_data_t data);
extern void gui_timeline_start(gui_timeline_t *timeline);
extern void gui_timeline_stop(gui_timeline_t *timeline);
extern void gui_timeline_delete(gui_timeline_t *timeline);
extern void gui_timeline_set_period(gui_timeline_t *timeline, uint32_t period);
extern void gui_timeline_set_repeat_count(gui_timeline_t *timeline, int32_t count);

//One file per timeline

#ifdef __cplusplus
}
#endif
#endif /* GUI_TIMELINES_H_ */
