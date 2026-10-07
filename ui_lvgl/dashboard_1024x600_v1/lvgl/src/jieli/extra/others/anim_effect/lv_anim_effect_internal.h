#ifndef LV_ANIM_EFFECT_INTERNAL_H
#define LV_ANIM_EFFECT_INTERNAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "../../../../../lvgl.h"

#if LV_USE_ANIM_EFFECT

typedef struct {
    lv_obj_t * obj;
    lv_anim_exec_xcb_t exec_cb;
} lv_anim_effect_context_t;

void lv_anim_effect_delete(lv_obj_t * obj);
void lv_anim_effect_add_obj_delete_cb(lv_obj_t * obj);
void lv_anim_effect_begin(lv_obj_t * obj);

void lv_anim_effect_init(lv_anim_t * anim, lv_obj_t * obj, lv_anim_exec_xcb_t exec_cb);
void lv_anim_effect_init_direct(lv_anim_t * anim, lv_obj_t * obj, lv_anim_exec_xcb_t exec_cb);
void lv_anim_effect_start_direct(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end,
                                 lv_anim_exec_xcb_t exec_cb, lv_anim_path_cb_t path_cb, bool early_apply);
void lv_anim_effect_start_ctx(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end,
                              lv_anim_exec_xcb_t exec_cb, lv_anim_path_cb_t path_cb, bool early_apply);
void lv_anim_effect_start_opa_inout(lv_obj_t * obj, int32_t delay, int32_t time, bool in,
                                    lv_anim_path_cb_t path_cb, bool early_apply);

#endif /*LV_USE_ANIM_EFFECT*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ANIM_EFFECT_INTERNAL_H*/
