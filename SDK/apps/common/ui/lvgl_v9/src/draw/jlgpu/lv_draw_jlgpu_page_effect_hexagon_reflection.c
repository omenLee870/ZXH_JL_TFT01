#include "src/core/lv_refr_private.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "jieli/extra/libs/jl_hexagon_reflection/lv_hexagon_reflection.h"
#include "asm/math_fast_function.h"
#include "lv_draw_jlgpu_page_effect_hexagon_reflection.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if (REF_POLYTOPE_CONFIG == REF_POLYTOPE_SIX)
const u8 ref_polytope = REF_POLYTOPE_SIX;
#elif (REF_POLYTOPE_CONFIG == REF_POLYTOPE_EIGHT)
const u8 ref_polytope = REF_POLYTOPE_EIGHT;
#elif (REF_POLYTOPE_CONFIG == REF_POLYTOPE_ALL)
const u8 ref_polytope = REF_POLYTOPE_ALL;
#endif

struct hexagon_reflection_priv {
    lv_draw_jlgpu_unit_t *unit;
    void *curr_elm;//页面
    u8 hold_flag;
    u8 move_flag;
    u8 touch_flag;
    u8 polytope_type; // 多面体的数量配置，多个多面体共存时使用
    int curr_win;
    s16 pos_x;
    s16 pos_y;
#if (REF_POLYTOPE_CONFIG == REF_POLYTOPE_SIX)
    u32 page_id[REF_POLYTOPE_SIX];
    u8 reset_sep[REF_POLYTOPE_SIX * 2];
    u8 reset_sep_last[REF_POLYTOPE_SIX * 2];
#else //八面体或六面体和八面体共存
    u32 page_id[REF_POLYTOPE_EIGHT];
    u8 reset_sep[REF_POLYTOPE_EIGHT * 2];
    u8 reset_sep_last[REF_POLYTOPE_EIGHT * 2];
#endif
    int init : 1;
    int ignore_first_move : 1;
    int cur_left;
    int dial_page_index;
    unsigned long last_time;
    s16 x_curr_angle;
    s16 y_curr_angle;
    int width_tab[16];
    int height_tab[16];
};

static struct hexagon_reflection_priv hexagon_reflection_priv_t = {0};
#define __this (&hexagon_reflection_priv_t)

// 说明：以下参数是经过调整的，不要修改
#define X_ANGLE_ADJ 0.0f
#define X_ANGLE 14.85f // 14.85f //俯仰视角

#define VIEX_DISTANCE_MIN   4.7f
#define VIEX_DISTANCE_MAX   2.575f
#define VIEX_DISTANCE_DIFF  (VIEX_DISTANCE_MIN - VIEX_DISTANCE_MAX)

//缩放大小
#define W_SCALE     1.285f
#define H_SCALE     1.195f


#if LV_USE_DRAW_JLGPU_LIST


/* 动态设置多面体的数量，在配置成多面体共存是启用 */
static u8 custom_polytope_type = REF_POLYTOPE_SIX;
void hexagon_reflection_custom_polytope_type_set(u8 type)
{
    custom_polytope_type = type;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_reflection_effect_set_page_index 配置3d灯笼倒影页面
 *
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_reflection_effect_set_page_index(uint32_t page_id, uint32_t page_index)
{
    if (page_index >= REF_POLYTOPE_CONFIG) {
        __this->page_id[REF_POLYTOPE_CONFIG - 1] = page_id;
    } else {
        __this->page_id[page_index] = page_id;
    }
}

void lv_hexagon_reflection_effect_get_page_id(uint32_t *page_id_arr)
{
    for (int i = 0; i < REF_POLYTOPE_CONFIG ; i++) {
        page_id_arr[i] = __this->page_id[i];
    }
}

int lv_hexagon_reflection_effect_get_face_scr_id(int x, int y)
{
    int face_index = lv_hexagon_reflection_get_face_index(x, y);
    if (face_index == -1) {
        return face_index;
    }
    // face_index: 0..11 for 6-sided, map to 0..5 (reflect=0..5, body=6..11)
    if (face_index >= __this->polytope_type) {
        face_index -= __this->polytope_type;
    }
    return __this->page_id[face_index];
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_reflection_effect_init 3d灯笼倒影初始化
 *
 * @param curr_elm
 * @param list_flag
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_reflection_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag)
{
    if (__this->init) {
        return;
    }
    if (ref_polytope == REF_POLYTOPE_SIX) {
        __this->polytope_type = REF_POLYTOPE_SIX;
    } else if (ref_polytope == REF_POLYTOPE_EIGHT) {
        __this->polytope_type = REF_POLYTOPE_EIGHT;
    } else if (ref_polytope == REF_POLYTOPE_ALL) {
        __this->polytope_type = custom_polytope_type; //用户自己定义，默认6面体
        lv_hexagon_reflection_polytope_type_set(__this->polytope_type);
    } else {
        /*log_error("polytope config err\n");*/
        return;
    }

    lv_display_t *disp =  lv_refr_get_disp_refreshing();
    int width  = lv_display_get_horizontal_resolution(disp);
    int height = lv_display_get_vertical_resolution(disp);

    for (int i = 0; i < __this->polytope_type * 2; i++) {
        __this->width_tab[i] = width;
        __this->height_tab[i] = height;
    }

    __this->unit = unit;

    lv_hexagon_reflection_init(__this->width_tab, __this->height_tab, ((float)height / (float)width) * W_SCALE, H_SCALE, VIEX_DISTANCE_MIN, 0, 0, width, height, (X_ANGLE - X_ANGLE_ADJ));

    if (list_flag == false) {
        for (int i = 0; i < __this->polytope_type * 2; i++) {
            __this->reset_sep[i] = __this->reset_sep_last[i] = i + 1;
        }
    }

    __this->init = 1;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_reflection_effect_uninit 反初始化
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_reflection_effect_uninit(void)
{
    if (__this->init) {
        if (__this->dial_page_index != -1) {
            __this->dial_page_index = -1;
        }

        lv_hexagon_reflection_uninit();

        __this->init = 0;
        __this->cur_left = 0;
        __this->x_curr_angle = 0;
        __this->hold_flag = 0;

    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_ui_hexagon_reflection_move 滑动接口
 *
 * @param unit
 * @param xoffset
 * @param yoffset
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int lv_ui_hexagon_reflection_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset)
{
    lv_display_t *disp =  lv_refr_get_disp_refreshing();

    int page_width  = lv_display_get_horizontal_resolution(disp);
    int page_height = lv_display_get_vertical_resolution(disp);

    __this->cur_left += xoffset;
    __this->cur_left = (__this->cur_left >= page_width) ? (__this->cur_left = page_width) : __this->cur_left;
    __this->cur_left = (__this->cur_left <= -page_width) ? (__this->cur_left = -page_width) : __this->cur_left;

    if ((__this->cur_left != 0) && (__this->init == 0)) {
        lv_hexagon_reflection_effect_init(unit, false);
    }

    if (__this->init) {
        __this->x_curr_angle -= xoffset;
        lv_hexagon_reflection_draw(unit, __this->x_curr_angle, __this->y_curr_angle, X_ANGLE, (X_ANGLE - X_ANGLE_ADJ), __this->reset_sep, 1);
    }

    return 0;
}

static const sticky_effect_ops_t hexagon_reflection_effect_ops = {
    .init            = lv_hexagon_reflection_effect_init,
    .uninit          = lv_hexagon_reflection_effect_uninit,
    .move            = lv_ui_hexagon_reflection_move,
    .get_face_scr_id = lv_hexagon_reflection_effect_get_face_scr_id,
    .get_face_index  = lv_hexagon_reflection_get_face_index,
    .get_page_id     = lv_hexagon_reflection_effect_get_page_id,
};

const sticky_effect_ops_t *lv_hexagon_reflection_effect_get_ops(void)
{
    return &hexagon_reflection_effect_ops;
}
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
