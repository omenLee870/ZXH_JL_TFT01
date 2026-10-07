#include "lv_anim_effect_rotate.h"
#include "lv_anim_effect_internal.h"
#if LV_USE_ANIM_EFFECT

static void set_rotate(lv_obj_t * obj, lv_anim_effect_args_t * anim_args, lv_anim_exec_xcb_t cb, int32_t start,
                       int32_t end)
{
    lv_anim_effect_start_direct(obj, anim_args->delay, anim_args->duration, start, end, cb, lv_anim_path_linear,
                                false);
}

void lv_anim_effect_rotate_in_down_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_left, -45, 0);
}

void lv_anim_effect_rotate_in_down_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_right, 45, 0);
}

void lv_anim_effect_rotate_in_up_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_left, 45, 0);
}

void lv_anim_effect_rotate_in_up_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_right, -90, 0);
}

void lv_anim_effect_rotate_in(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_center, -200, 0);
}

void lv_anim_effect_rotate_out_down_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_left, 0, 45);
}

void lv_anim_effect_rotate_out_down_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_right, 0, -45);
}

void lv_anim_effect_rotate_out_up_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_left, 0, -45);
}

void lv_anim_effect_rotate_out_up_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_bottom_right, 0, 90);
}

void lv_anim_effect_rotate_out(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    set_rotate(obj, anim_args, (lv_anim_exec_xcb_t)lv_anim_effect_rotate_center, 0, 200);
}

#endif /*LV_USE_ANIM_EFFECT*/
