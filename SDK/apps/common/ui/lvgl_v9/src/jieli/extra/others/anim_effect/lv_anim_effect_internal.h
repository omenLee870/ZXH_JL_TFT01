#ifndef LV_ANIM_EFFECT_INTERNAL_H
#define LV_ANIM_EFFECT_INTERNAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "../../../../../lvgl.h"

#if LV_USE_ANIM_EFFECT

typedef struct {
    lv_obj_t *obj;
    lv_anim_exec_xcb_t exec_cb;
} lv_anim_effect_context_t;

void lv_anim_effect_delete(lv_obj_t *obj);
void lv_anim_effect_add_obj_delete_cb(lv_obj_t *obj);

void lv_anim_effect_init(lv_anim_t *anim, lv_obj_t *obj, lv_anim_exec_xcb_t exec_cb);

#endif /*LV_USE_ANIM_EFFECT*/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ANIM_EFFECT_INTERNAL_H*/
