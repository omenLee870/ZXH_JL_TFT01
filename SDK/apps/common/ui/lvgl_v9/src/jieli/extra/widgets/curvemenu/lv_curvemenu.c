/**
 * @file lv_curvemenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_curvemenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"

#if LV_USE_CURVEMENU != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_curvemenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/


#define MY_CLASS &lv_curvemenu_class
#define SCROLL_IN_RANGE(val, max, min) (val > max && val < min)
#define MAX_SCROLL_ANGLE 40
#define OUT_OF_BOUND_FACTOR 0.6f
#define ELASIC_FACTOR       0.25f



/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_curvemenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_curvemenu_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_curvemenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_curvemenu_event(const lv_obj_class_t *class_p, lv_event_t *e);

static void update_menu_items(lv_obj_t *cont);
static void position_and_zoom_item(lv_obj_t *child, int32_t x, int32_t y, uint16_t zoom);
static uint32_t get_disp_child_cnt(const lv_obj_t *obj);
static void scroll_timer_cb(lv_timer_t *p_timer);
static void get_angle_range(lv_obj_t *cont, int32_t *angle_start, int32_t *angle_end);
static void update_img_size_with_scale(lv_obj_t *img);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_curvemenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_curvemenu_constructor,
    .destructor_cb = lv_curvemenu_destructor,
    .event_cb = lv_curvemenu_event,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_curvemenu_t),
};
const lv_obj_class_t lv_curvemenu_button_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_curvemenu_button_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_curvemenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_curvemenu_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t *lv_curvemenu_add_button(lv_obj_t *curve_menu, const void *icon)
{
    lv_obj_t *btn = lv_obj_class_create_obj(&lv_curvemenu_button_class, curve_menu);
    lv_obj_class_init_obj(btn);
    lv_image_set_src(btn, icon);
    update_menu_items(curve_menu);
    return btn;
}

/*=====================
 * Setter functions
 *====================*/

void lv_curvemenu_set_radius(lv_obj_t *obj, int32_t radius)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->radius = radius;

    /*更新px_per_angle*/
    int32_t per_height = radius * lv_trigo_sin(curve_menu->angle_step) / LV_TRIGO_SIN_MAX;
    curve_menu->px_per_angle = per_height * 10 / curve_menu->angle_step;

    update_menu_items(obj);
}

void lv_curvemenu_set_angle_step(lv_obj_t *obj, int32_t angle_step)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->angle_step = angle_step;

    /*更新px_per_angle*/
    int32_t per_height = curve_menu->radius * lv_trigo_sin(angle_step) / LV_TRIGO_SIN_MAX;
    curve_menu->px_per_angle = per_height * 10 / angle_step;

    update_menu_items(obj);
}

void lv_curvemenu_set_start_angle(lv_obj_t *obj, int32_t start)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->angle_start = start;

    update_menu_items(obj);
}

void lv_curvemenu_set_end_angle(lv_obj_t *obj, int32_t end)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->angle_end = end;

    update_menu_items(obj);
}

void lv_curvemenu_set_angles(lv_obj_t *obj, int32_t start, int32_t end)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->angle_start = start;
    curve_menu->angle_end = end;

    update_menu_items(obj);
}

void lv_curvemenu_set_col_num(lv_obj_t *obj, int32_t col_num)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->col_num = col_num;

    update_menu_items(obj);
}

void lv_curvemenu_set_cr(lv_obj_t *obj, int32_t cr)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->cr = cr;

    update_menu_items(obj);
}

void lv_curvemenu_button_set_hide(lv_obj_t *btn, bool is_hide)
{
    LV_ASSERT_OBJ(btn, &lv_curvemenu_button_class);

    lv_curvemenu_button_t *curve_menu_btn = (lv_curvemenu_button_t *)btn;
    curve_menu_btn->is_hide = is_hide;
    if (is_hide) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }

    update_menu_items(lv_obj_get_parent(btn));
}

void lv_curvemenu_button_set_zoom(lv_obj_t *btn, uint16_t zoom)
{
    LV_ASSERT_OBJ(btn, &lv_curvemenu_button_class);

    lv_curvemenu_button_t *curve_menu_btn = (lv_curvemenu_button_t *)btn;
    curve_menu_btn->zoom = zoom;
    lv_image_set_scale(btn, zoom);
    update_img_size_with_scale(btn);

    update_menu_items(lv_obj_get_parent(btn));
}

void lv_curvemenu_button_set_src(lv_obj_t *btn, const void *icon)
{
    LV_ASSERT_OBJ(btn, &lv_curvemenu_button_class);

    lv_image_set_src(btn, icon);
    update_img_size_with_scale(btn);

    update_menu_items(lv_obj_get_parent(btn));
}

/*=====================
 * Getter functions
 *====================*/

int32_t lv_curvemenu_get_radius(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->radius;
}

int32_t lv_curvemenu_get_angle_step(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->angle_step;
}

int32_t lv_curvemenu_get_start_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->angle_start;
}

int32_t lv_curvemenu_get_end_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->angle_end;
}

int32_t lv_curvemenu_get_col_num(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->col_num;
}

int32_t lv_curvemenu_get_cr(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    return curve_menu->cr;
}

bool lv_curvemenu_button_get_hide(lv_obj_t *btn)
{
    LV_ASSERT_OBJ(btn, &lv_curvemenu_button_class);

    lv_curvemenu_button_t *curve_menu_btn = (lv_curvemenu_button_t *)btn;
    return curve_menu_btn->is_hide;
}

uint16_t lv_curvemenu_button_get_zoom(lv_obj_t *btn)
{
    LV_ASSERT_OBJ(btn, &lv_curvemenu_button_class);

    return lv_image_get_scale(btn);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_curvemenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    curve_menu->angle_start = -90;
    curve_menu->angle_end = 90;
    curve_menu->angle = -60;
    curve_menu->angle_step = 30;

    curve_menu->radius = 200;
    curve_menu->px_per_angle = 33;
    curve_menu->col_num = 3;
    curve_menu->cr = 200;
    curve_menu->_is_init = false;

    curve_menu->scroll.scroll_tmr = lv_timer_create(scroll_timer_cb, LV_DEF_REFR_PERIOD, obj);
    lv_timer_pause(curve_menu->scroll.scroll_tmr);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_curvemenu_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)obj;
    if (curve_menu->scroll.scroll_tmr) {
        lv_timer_delete(curve_menu->scroll.scroll_tmr);
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_curvemenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_curvemenu_button_t *curve_menu_btn = (lv_curvemenu_button_t *)obj;
    curve_menu_btn->is_hide = false;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_curvemenu_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    lv_result_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RESULT_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cont = lv_event_get_current_target(e);
    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)cont;

    /*中心坐标 */
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    int32_t x_center = (cont_a.x2 - cont_a.x1 + 1) / 2;
    int32_t y_center = (cont_a.y2 - cont_a.y1 + 1) / 2;

    if (code == LV_EVENT_PRESSED) {
        lv_timer_pause(curve_menu->scroll.scroll_tmr);
        curve_menu->scroll.ver_momentum = 0;
    } else if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_event_get_indev(e);
        lv_point_t diff = indev->pointer.vect;

        diff.y = (float)diff.y * 10 / curve_menu->px_per_angle;
        curve_menu->angle += diff.y;

        /*角度范围限制*/
        int32_t max_angle, min_angle;
        get_angle_range(cont, &max_angle, &min_angle);
        max_angle += MAX_SCROLL_ANGLE;
        min_angle -= MAX_SCROLL_ANGLE;
        if (curve_menu->angle > max_angle) {
            curve_menu->angle = max_angle;
        } else if (curve_menu->angle < min_angle) {
            curve_menu->angle = min_angle;
        }

        update_menu_items(cont);
    } else if (code == LV_EVENT_RELEASED) {
        lv_indev_t *indev = lv_event_get_indev(e);
        curve_menu->scroll.ver_momentum = indev->pointer.vect.y;
        lv_timer_resume(curve_menu->scroll.scroll_tmr);
    } else if (code == LV_EVENT_SIZE_CHANGED) {
        if (!curve_menu->_is_init && lv_obj_get_width(cont) != 0 && lv_obj_get_height(cont) != 0) {
            curve_menu->_is_init = true;
            update_menu_items(cont);
        }
    } else if (code == LV_EVENT_STYLE_CHANGED) {
        if (curve_menu->_is_init) {
            update_menu_items(cont);
        }
    }
}

static lv_obj_t *get_disp_item(lv_obj_t *cont, int32_t *idx)
{
    lv_obj_t *child = NULL;
    int32_t child_cnt = lv_obj_get_child_count(cont);
    if (child_cnt < *idx) {
        return NULL;
    }
    for (int32_t i = *idx; i < child_cnt; i++) {
        child = lv_obj_get_child(cont, i);
        lv_curvemenu_button_t *gear_menu_btn = (lv_curvemenu_button_t *)child;
        if (!gear_menu_btn->is_hide) {
            *idx = i;
            return child;
        }
    }
    return NULL;
}

static uint32_t get_disp_child_cnt(const lv_obj_t *obj)
{
    uint32_t cnt = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        lv_curvemenu_button_t *gear_menu_btn = (lv_curvemenu_button_t *)child;
        if (!gear_menu_btn->is_hide) {
            cnt++;
        }
    }
    return cnt;
}

static void update_menu_items(lv_obj_t *cont)
{
    if (!lv_obj_is_valid(cont) || lv_obj_get_width(cont) == 0 || lv_obj_get_height(cont) == 0) {
        return;
    }

    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)cont;
    int32_t child_cnt = lv_obj_get_child_count(cont);
    int32_t padding_col = lv_obj_get_style_pad_column(cont, LV_PART_ITEMS);
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);

    int32_t center_x = (cont_a.x2 - cont_a.x1 + 1) / 2;
    int32_t center_y = (cont_a.y2 - cont_a.y1 + 1) / 2;
    int32_t radius = curve_menu->radius;
    int32_t angle_step = curve_menu->angle_step;
    int32_t base_angle = curve_menu->angle;
    int32_t col_cnt = curve_menu->col_num;
    bool is_odd = col_cnt % 2 == 1; /*列数是否为奇数*/
    int32_t mid_idx = col_cnt / 2;  /*中间列的索引*/

    lv_obj_t **childs = lv_malloc(col_cnt * sizeof(lv_curvemenu_button_t *));
    if (childs == NULL) {
        LV_LOG_ERROR("Memory allocation failed");
        return;
    }

    int32_t group_idx = 0;
    int32_t child_col_width = 0;
    int32_t cx = cont_a.x2 + curve_menu->cr;
    int32_t cy = center_y;
    int32_t mid_cr, left_cr;
    mid_cr = cx - center_x;

    for (int32_t i = 0; i < child_cnt;) {
        for (int32_t j = 0; j < col_cnt; j++) {
            childs[j] = get_disp_item(cont, &i);
            i++;
        }
        if (childs[0] == NULL) {
            break;
        }

        if (group_idx == 0) {
            child_col_width = lv_obj_get_width(childs[0]) + padding_col;
        }

        int32_t cur_angle = angle_step * group_idx + base_angle;  /*计算当前按钮的角度*/
        if (cur_angle > curve_menu->angle_start && cur_angle < curve_menu->angle_end) {
            int32_t cos_angle = lv_trigo_cos(cur_angle);
            int32_t sin_angle = lv_trigo_sin(cur_angle);
            int32_t mid_y = center_y + radius * sin_angle / LV_TRIGO_SIN_MAX;
            int32_t mid_x = center_x;
            uint16_t zoom = cos_angle * 256 / LV_TRIGO_SIN_MAX; /*图标缩放系数*/

            int32_t dt_y, x_abs;
            lv_sqrt_res_t res;
            uint32_t x_sqr;

            left_cr = mid_cr + child_col_width;
            dt_y = LV_ABS(mid_y - center_y);
            x_sqr = left_cr * left_cr - dt_y * dt_y;
            lv_sqrt(x_sqr, &res, 0x8000);
            x_abs = cx - res.i;

            int32_t zoom_col_width = x_abs - mid_x; /*缩放后的列宽*/
            float offset = is_odd ? 0.0f : 0.5f;
            for (int32_t j = 0; j < mid_idx; j++) {
                lv_obj_t *left_child = childs[j];
                if (left_child == NULL) {
                    break;
                }

                x_abs = mid_x + zoom_col_width * (mid_idx - j - offset);
                position_and_zoom_item(left_child, x_abs, mid_y, zoom); /*左侧*/

                lv_obj_t *right_child = childs[col_cnt - j - 1];
                if (right_child != NULL) {
                    x_abs = mid_x - zoom_col_width * (mid_idx - j - offset);
                    position_and_zoom_item(right_child, x_abs, mid_y, zoom); /*右侧*/
                }
            }

            if (is_odd) {
                lv_obj_t *mid_child = childs[mid_idx];
                if (mid_child != NULL) {
                    position_and_zoom_item(mid_child, mid_x, mid_y, zoom); /*中间*/
                }
            }
        } else {
            for (int32_t j = 0; j < col_cnt; j++) {
                lv_obj_t *child = childs[j];
                if (child == NULL) {
                    break;
                }
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
        }
        group_idx++;
    }

    lv_free(childs);
    lv_obj_invalidate(cont);
}

static void position_and_zoom_item(lv_obj_t *child, int32_t x, int32_t y, uint16_t zoom)
{
    if (child == NULL) {
        return;
    }

    int32_t zoom_x_offset = lv_obj_get_width(child) * zoom / 256 / 2;
    int32_t zoom_y_offset = lv_obj_get_height(child) * zoom / 256 / 2;

    lv_obj_set_style_transform_scale(child, zoom, 0);
    lv_obj_set_pos(child, x - zoom_x_offset, y - zoom_y_offset);
    lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
}

static void scroll_timer_cb(lv_timer_t *p_timer)
{
    lv_obj_t *cont = (lv_obj_t *)p_timer->user_data;
    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)cont;

    bool scroll_done_y = false;
    int32_t angle = curve_menu->angle;
    float diff_y = curve_menu->scroll.ver_momentum;
    diff_y *= 0.6f; /*衰减动量，模拟惯性滚动*/

    /*计算最大和最小的角度限制*/
    int32_t max_angle, min_angle;
    get_angle_range(cont, &max_angle, &min_angle);

    if (angle > max_angle) {
        float force = (float)(angle - max_angle);
        curve_menu->angle = max_angle + force - force * ELASIC_FACTOR;
    } else if (angle < min_angle) {
        float force = (float)(min_angle - angle);
        curve_menu->angle = min_angle  - force + force * ELASIC_FACTOR;
    } else {
        curve_menu->angle += (int32_t)diff_y * 10 / curve_menu->px_per_angle; /*更新角度，增加或减少动量*/
        if (diff_y < 1) {
            scroll_done_y = true;
        }
    }

    curve_menu->scroll.ver_momentum = diff_y;
    if (scroll_done_y) {
        lv_timer_pause(p_timer);
    }

    update_menu_items(cont);
}

static void get_angle_range(lv_obj_t *cont, int32_t *angle_start, int32_t *angle_end)
{
    lv_curvemenu_t *curve_menu = (lv_curvemenu_t *)cont;
    int32_t disp_child_cnt = get_disp_child_cnt(cont);
    int32_t row_cnt = disp_child_cnt % curve_menu->col_num == 0 ? disp_child_cnt / curve_menu->col_num : disp_child_cnt /
                      curve_menu->col_num + 1;
    *angle_start = (curve_menu->angle_end + curve_menu->angle_start) / 2;
    *angle_end = (row_cnt - 1) * -curve_menu->angle_step + *angle_start;
}

static void update_img_size_with_scale(lv_obj_t *img)
{
    int32_t src_w = lv_image_get_src_width(img);
    int32_t src_h = lv_image_get_src_height(img);
    int32_t applied_zoom = lv_image_get_scale(img);

    if (src_w <= 0 || src_h <= 0) {
        return;
    }

    if (applied_zoom <= 0) {
        applied_zoom = LV_SCALE_NONE;
    }

    int32_t scaled_w = (int32_t)(((int64_t)src_w * applied_zoom + LV_SCALE_NONE / 2) / LV_SCALE_NONE);
    int32_t scaled_h = (int32_t)(((int64_t)src_h * applied_zoom + LV_SCALE_NONE / 2) / LV_SCALE_NONE);

    if (scaled_w <= 0) {
        scaled_w = 1;
    }
    if (scaled_h <= 0) {
        scaled_h = 1;
    }

    lv_obj_set_size(img, scaled_w, scaled_h);
}


#endif
