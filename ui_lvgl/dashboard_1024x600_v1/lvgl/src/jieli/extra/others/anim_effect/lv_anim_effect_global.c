#include "lv_anim_effect_global.h"
#if LV_USE_ANIM_EFFECT
void lv_anim_effect_set_opa(lv_obj_t * obj, lv_opa_t opa)
{
    lv_obj_set_style_opa(obj, opa, LV_PART_MAIN);
}

uint32_t lv_anim_effect_get_time(int32_t duration, int16_t percent)
{
    if(percent <= 0) {
        return 0;
    }

    if(duration <= 0) {
        return (uint32_t)percent * 10U;
    }

    uint32_t time = ((uint32_t)duration * (uint32_t)percent) / 100U;
    if(time == 0U) {
        time = 1U;
    }
    return time;
}

void lv_anim_effect_set_height(lv_obj_t * obj, uint32_t height)
{
    lv_obj_set_style_height(obj, height, LV_PART_MAIN);
}

// y轴缩放
void lv_anim_effect_scale_y(lv_obj_t * obj, int32_t scale)
{
    int32_t scale_y = (scale * LV_SCALE_NONE) / 100;
    lv_obj_set_style_transform_scale_y(obj, scale_y, LV_PART_MAIN);
}

// x轴缩放
void lv_anim_effect_scale_x(lv_obj_t * obj, int32_t scale)
{
    int32_t scale_x = (scale * LV_SCALE_NONE) / 100;
    lv_obj_set_style_transform_scale_x(obj, scale_x, LV_PART_MAIN);
}

// 缩放
void lv_anim_effect_scale(lv_obj_t * obj, int32_t scale)
{

    int32_t h = lv_obj_get_style_height(obj, 0);
    int32_t retH = h * scale / 100;

    int32_t w = lv_obj_get_style_width(obj, 0);
    int32_t retW = w * scale / 100;
    int32_t tranX = (retW - w) / 2;
    int32_t tranY = (retH - h) / 2;

    lv_obj_set_style_translate_x(obj, -tranX, LV_PART_MAIN);
    lv_obj_set_style_translate_y(obj, -tranY, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(obj, (uint16_t)scale * 2.56, 0); // 如果图片控件缩放会导致崩溃
}

void lv_anim_effect_translate_y(lv_obj_t * obj, int32_t y)
{
    lv_obj_set_style_translate_y(obj, y, LV_PART_MAIN);
}

void lv_anim_effect_translate_x(lv_obj_t * obj, int32_t x)
{
    lv_obj_set_style_translate_x(obj, x, LV_PART_MAIN);
}

// transform - origin : center center;
void lv_anim_effect_rotate_center(lv_obj_t * obj, int32_t deg)
{

    int32_t x = lv_obj_get_style_width(obj, 0) / 2;
    int32_t y = lv_obj_get_style_height(obj, 0) / 2;
    int16_t sin = lv_trigo_sin(deg);
    int16_t cos = lv_trigo_cos(deg);
    int32_t x1 = x * cos / 32767 - y * sin / 32767;
    int32_t y1 = x * sin / 32767 + y * cos / 32767;
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
    lv_obj_set_style_translate_x(obj, x - x1, 0);
    lv_obj_set_style_translate_y(obj, y - y1, 0);
}

// transform - origin : top center;
void lv_anim_effect_rotate_top_center(lv_obj_t * obj, int32_t deg)
{

    int32_t x = lv_obj_get_style_width(obj, 0) / 2;
    int32_t y = 0;
    int16_t sin = lv_trigo_sin(deg);
    int16_t cos = lv_trigo_cos(deg);
    int32_t x1 = x * cos / 32767 - y * sin / 32767;
    int32_t y1 = x * sin / 32767 + y * cos / 32767;
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
    lv_obj_set_style_translate_x(obj, x - x1, 0);
    lv_obj_set_style_translate_y(obj, y - y1, 0);
}

// transform - origin : top center;
void lv_anim_effect_rotate_bottom_center(lv_obj_t * obj, int32_t deg)
{

    int32_t x = lv_obj_get_style_width(obj, 0) / 2;
    int32_t y = lv_obj_get_style_height(obj, 0);
    int16_t sin = lv_trigo_sin(deg);
    int16_t cos = lv_trigo_cos(deg);
    int32_t x1 = x * cos / 32767 - y * sin / 32767;
    int32_t y1 = x * sin / 32767 + y * cos / 32767;
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
    lv_obj_set_style_translate_x(obj, x - x1, 0);
    lv_obj_set_style_translate_y(obj, y - y1, 0);
}

// transform - origin : top left;
void lv_anim_effect_rotate_top_left(lv_obj_t * obj, int32_t deg)
{
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
}
// transform - origin : bottom right;
void lv_anim_effect_rotate_bottom_right(lv_obj_t * obj, int32_t deg)
{

    int32_t x = lv_obj_get_style_width(obj, 0);
    int32_t y = lv_obj_get_style_height(obj, 0);
    int16_t sin = lv_trigo_sin(deg);
    int16_t cos = lv_trigo_cos(deg);
    int32_t x1 = x * cos / 32767 - y * sin / 32767;
    int32_t y1 = x * sin / 32767 + y * cos / 32767;
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
    lv_obj_set_style_translate_x(obj, x - x1, 0);
    lv_obj_set_style_translate_y(obj, y - y1, 0);
}

// transform - origin : bottom left;
void lv_anim_effect_rotate_bottom_left(lv_obj_t * obj, int32_t deg)
{

    int32_t x = 0;
    int32_t y = lv_obj_get_style_height(obj, 0);
    int16_t sin = lv_trigo_sin(deg);
    int16_t cos = lv_trigo_cos(deg);
    int32_t x1 = x * cos / 32767 - y * sin / 32767;
    int32_t y1 = x * sin / 32767 + y * cos / 32767;
    lv_obj_set_style_transform_angle(obj, deg * 10, 0);
    lv_obj_set_style_translate_x(obj, x - x1, 0);
    lv_obj_set_style_translate_y(obj, y - y1, 0);
}

#endif /*LV_USE_ANIM_EFFECT*/
