#include "lv_anim_effect_sliding.h"
#include "lv_anim_effect_internal.h"
#if LV_USE_ANIM_EFFECT

static void tranlate_x(lv_obj_t * obj, lv_anim_effect_args_t * anim_args, int32_t start, int32_t end)
{
    lv_anim_effect_translate_x(obj, start);
    lv_anim_effect_start_direct(obj, anim_args->delay, anim_args->duration, start, end,
                                (lv_anim_exec_xcb_t)lv_anim_effect_translate_x, lv_anim_path_linear, false);
}

static void tranlate_y(lv_obj_t * obj, lv_anim_effect_args_t * anim_args, int32_t start, int32_t end)
{
    lv_anim_effect_translate_y(obj, start);
    lv_anim_effect_start_direct(obj, anim_args->delay, anim_args->duration, start, end,
                                (lv_anim_exec_xcb_t)lv_anim_effect_translate_y, lv_anim_path_linear, false);
}

static void set_end_opa(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_start_direct(obj, anim_args->delay + anim_args->duration, LV_DEF_REFR_PERIOD * 2,
                                LV_OPA_10, LV_OPA_0, (lv_anim_exec_xcb_t)lv_anim_effect_set_opa,
                                lv_anim_path_linear, false);
}

void lv_anim_effect_sliding_in_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args, -translate, 0);
    lv_anim_effect_set_opa(obj, LV_OPA_100);
}

void lv_anim_effect_sliding_in_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args, -translate, 0);
    lv_anim_effect_set_opa(obj, LV_OPA_100);
}

void lv_anim_effect_sliding_in_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args, translate, 0);
    lv_anim_effect_set_opa(obj, LV_OPA_100);
}

void lv_anim_effect_sliding_in_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args, translate, 0);
    lv_anim_effect_set_opa(obj, LV_OPA_100);
}

void lv_anim_effect_sliding_out_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args, 0, translate);
    set_end_opa(obj, anim_args);
}

void lv_anim_effect_sliding_out_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args, 0, -translate);
    set_end_opa(obj, anim_args);
}

void lv_anim_effect_sliding_out_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args, 0, translate);
    set_end_opa(obj, anim_args);
}

void lv_anim_effect_sliding_out_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t translate = lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args, 0, -translate);
    set_end_opa(obj, anim_args);
}

#endif /*LV_USE_ANIM_EFFECT*/
