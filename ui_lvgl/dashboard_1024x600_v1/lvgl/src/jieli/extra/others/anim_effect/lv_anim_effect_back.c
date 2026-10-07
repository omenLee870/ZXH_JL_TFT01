#include "lv_anim_effect_back.h"
#include "lv_anim_effect_internal.h"
#if LV_USE_ANIM_EFFECT

static void tranlate_x(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end)
{
    lv_anim_effect_start_direct(obj, delay, time, start, end, (lv_anim_exec_xcb_t)lv_anim_effect_translate_x,
                                lv_anim_path_linear, false);
}

static void tranlate_y(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end)
{
    lv_anim_effect_translate_y(obj, start);
    lv_anim_effect_start_direct(obj, delay, time, start, end, (lv_anim_exec_xcb_t)lv_anim_effect_translate_y,
                                lv_anim_path_linear, false);
}

static void set_opa(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end)
{
    lv_anim_effect_start_direct(obj, delay, time, start, end, (lv_anim_exec_xcb_t)lv_anim_effect_set_opa,
                                lv_anim_path_linear, false);
}

static void set_scale(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end)
{
    lv_anim_effect_start_direct(obj, delay, time, start, end, (lv_anim_exec_xcb_t)lv_anim_effect_scale,
                                lv_anim_path_linear, false);
}

void lv_anim_effect_back_in_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);

    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 80);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay + t_start, time, 70, 100);
    set_opa(obj, anim_args->delay + t_start, time, LV_OPA_70, LV_OPA_100);
    int32_t translate = lv_obj_get_style_y(obj, 0) + lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args->delay, t_start, -translate, 0);
}

void lv_anim_effect_back_in_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 80);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay + t_start, time, 70, 100);
    set_opa(obj, anim_args->delay + t_start, time, LV_OPA_70, LV_OPA_100);
    int32_t translate = lv_obj_get_style_x(obj, 0) + lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args->delay, t_start, -translate, 0);
}

void lv_anim_effect_back_in_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 80);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay + t_start, time, 70, 100);
    set_opa(obj, anim_args->delay + t_start, time, LV_OPA_70, LV_OPA_100);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_horizontal_resolution(d) - lv_obj_get_style_x(obj, 0);
    tranlate_x(obj, anim_args->delay, t_start, translate, 0);
}

void lv_anim_effect_back_in_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 80);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay + t_start, time, 70, 100);
    set_opa(obj, anim_args->delay + t_start, time, LV_OPA_70, LV_OPA_100);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_vertical_resolution(d) - lv_obj_get_style_y(obj, 0);
    tranlate_y(obj, anim_args->delay, t_start, translate, 0);
}

void lv_anim_effect_back_out_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 20);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay, t_start, 100, 70);
    set_opa(obj, anim_args->delay, t_start, LV_OPA_100, LV_OPA_70);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_vertical_resolution(d) - lv_obj_get_style_y(obj, 0);
    tranlate_y(obj, anim_args->delay + t_start, time, 0, translate);
}

void lv_anim_effect_back_out_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 20);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay, t_start, 100, 70);
    set_opa(obj, anim_args->delay, t_start, LV_OPA_100, LV_OPA_70);
    int32_t translate = lv_obj_get_style_x(obj, 0) + lv_obj_get_style_width(obj, 0);
    tranlate_x(obj, anim_args->delay + t_start, time, 0, -translate);
}

void lv_anim_effect_back_out_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 20);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay, t_start, 100, 70);
    set_opa(obj, anim_args->delay, t_start, LV_OPA_100, LV_OPA_70);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_horizontal_resolution(d) - lv_obj_get_style_x(obj, 0);
    tranlate_x(obj, anim_args->delay + t_start, time, 0, translate);
}

void lv_anim_effect_back_out_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    int32_t t_start = lv_anim_effect_get_time(anim_args->duration, 20);
    int32_t time = anim_args->duration - t_start;
    set_scale(obj, anim_args->delay, t_start, 100, 70);
    set_opa(obj, anim_args->delay, t_start, LV_OPA_100, LV_OPA_70);
    int32_t translate = lv_obj_get_style_y(obj, 0) + lv_obj_get_style_height(obj, 0);
    tranlate_y(obj, anim_args->delay + t_start, time, 0, -translate);
}

#endif /*LV_USE_ANIM_EFFECT*/
