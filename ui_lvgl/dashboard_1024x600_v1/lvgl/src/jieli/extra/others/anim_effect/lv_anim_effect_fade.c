#include "lv_anim_effect_fade.h"
#include "lv_anim_effect_internal.h"
#if LV_USE_ANIM_EFFECT

static void translate_x(lv_obj_t * obj, lv_anim_effect_args_t * anim_args, int32_t start, int32_t end)
{
    lv_anim_effect_translate_x(obj, start);
    lv_anim_effect_start_direct(obj, anim_args->delay, anim_args->duration, start, end,
                                (lv_anim_exec_xcb_t)lv_anim_effect_translate_x, lv_anim_path_linear, true);
}

static void translate_y(lv_obj_t * obj, lv_anim_effect_args_t * anim_args, int32_t start, int32_t end)
{
    lv_anim_effect_translate_y(obj, start);
    lv_anim_effect_start_direct(obj, anim_args->delay, anim_args->duration, start, end,
                                (lv_anim_exec_xcb_t)lv_anim_effect_translate_y, lv_anim_path_linear, true);
}

void lv_anim_effect_fade_in(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
}

void lv_anim_effect_fade_in_bottom_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, lv_obj_get_style_height(obj, 0), 0);
    translate_x(obj, anim_args, -lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_bottom_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, lv_obj_get_style_height(obj, 0), 0);
    translate_x(obj, anim_args, lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, -lv_obj_get_style_height(obj, 0), 0);
}

void lv_anim_effect_fade_in_down_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, -(lv_obj_get_style_y(obj, 0) << 1), 0);
}

void lv_anim_effect_fade_in_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_x(obj, anim_args, -lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_left_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_x(obj, anim_args, -(lv_obj_get_style_x(obj, 0) << 1), 0);
}

void lv_anim_effect_fade_in_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_x(obj, anim_args, lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_right_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_horizontal_resolution(d) - lv_obj_get_style_x(obj, 0);
    translate_x(obj, anim_args, translate << 1, 0);
}

void lv_anim_effect_fade_in_top_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, -lv_obj_get_style_height(obj, 0), 0);
    translate_x(obj, anim_args, -lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_top_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, -lv_obj_get_style_height(obj, 0), 0);
    translate_x(obj, anim_args, lv_obj_get_style_width(obj, 0), 0);
}

void lv_anim_effect_fade_in_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    translate_y(obj, anim_args, lv_obj_get_style_height(obj, 0), 0);
}

void lv_anim_effect_fade_in_up_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, true, lv_anim_path_linear, true);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_vertical_resolution(d) - lv_obj_get_style_y(obj, 0);
    translate_y(obj, anim_args, translate << 1, 0);
}

void lv_anim_effect_fade_out(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
}

void lv_anim_effect_fade_out_bottom_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, lv_obj_get_style_height(obj, 0));
    translate_x(obj, anim_args, 0, -lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_bottom_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, lv_obj_get_style_height(obj, 0));
    translate_x(obj, anim_args, 0, lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_down(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, lv_obj_get_style_height(obj, 0));
}

void lv_anim_effect_fade_out_down_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_vertical_resolution(d) - lv_obj_get_style_y(obj, 0);
    translate_y(obj, anim_args, 0, translate << 1);
}

void lv_anim_effect_fade_out_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_x(obj, anim_args, 0, -lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_left_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_x(obj, anim_args, 0, -(lv_obj_get_style_x(obj, 0) << 1));
}

void lv_anim_effect_fade_out_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_x(obj, anim_args, 0, lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_right_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_horizontal_resolution(d) - lv_obj_get_style_x(obj, 0);
    translate_x(obj, anim_args, 0, translate << 1);
}

void lv_anim_effect_fade_out_top_left(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, -lv_obj_get_style_height(obj, 0));
    translate_x(obj, anim_args, 0, -lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_top_right(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, -lv_obj_get_style_height(obj, 0));
    translate_x(obj, anim_args, 0, lv_obj_get_style_width(obj, 0));
}

void lv_anim_effect_fade_out_up(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    translate_y(obj, anim_args, 0, -lv_obj_get_style_height(obj, 0));
}

void lv_anim_effect_fade_out_up_big(lv_obj_t * obj, lv_anim_effect_args_t * anim_args)
{
    lv_anim_effect_begin(obj);
    lv_anim_effect_start_opa_inout(obj, anim_args->delay, anim_args->duration, false, lv_anim_path_linear, true);
    lv_display_t * d = lv_obj_get_disp(obj);
    int32_t translate = lv_display_get_vertical_resolution(d) - lv_obj_get_style_y(obj, 0);
    translate_y(obj, anim_args, 0, -(translate << 1));
}

#endif /*LV_USE_ANIM_EFFECT*/
