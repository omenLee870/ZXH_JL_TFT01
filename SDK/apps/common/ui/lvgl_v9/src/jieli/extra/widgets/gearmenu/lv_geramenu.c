/**
 * @file lv_gearmenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_gearmenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"

#if LV_USE_GEARMENU != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_gearmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif
#if LV_USE_LABEL == 0
#error "lv_gearmenu: lv_label is required. Enable it in lv_conf.h (LV_USE_LABEL 1)"
#endif


/*********************
 *      DEFINES
 *********************/

#define MY_CLASS &lv_gearmenu_class
#define MAX_SCROLL_ANGLE 40
// #define CHECK_BTN(btn) if (!lv_obj_check_type(btn, &lv_gearmenu_button_class)) {LV_LOG_WARN("lv_gearmenu_btn_set_img: obj is not an lv_gearmenu_button_t object"); return;}

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_gearmenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_gearmenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_gearmenu_event(const lv_obj_class_t *class_p, lv_event_t *e);

static void update_menu_items(lv_obj_t *cont);
static void gearmenu_anim_cb(void *var, int32_t v);
static void gearmenu_anim_del_cb(lv_anim_t *cb);
static uint32_t get_disp_child_cnt(const lv_obj_t *obj);
static lv_obj_t *get_img(lv_obj_t *btn);
static lv_obj_t *get_label(lv_obj_t *btn);
static void get_angle_range(lv_obj_t *cont, int32_t *angle_start, int32_t *angle_end);
static void set_menu_angle(lv_obj_t *obj, int32_t angle);
static void refresh_menu(lv_obj_t *obj);
static void update_img_size_with_scale(lv_obj_t *img);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_gearmenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_gearmenu_constructor,
    .event_cb = lv_gearmenu_event,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_gearmenu_t),
};

const lv_obj_class_t lv_gearmenu_button_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_gearmenu_button_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_gearmenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_gearmenu_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t *lv_gearmenu_add_button(lv_obj_t *gear_menu, const void *icon, const char *txt)
{
    lv_obj_t *btn = lv_obj_class_create_obj(&lv_gearmenu_button_class, gear_menu);
    lv_obj_class_init_obj(btn);
    lv_obj_set_size(btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;

    if (icon) {
        lv_obj_t *img = lv_image_create(btn);
        lv_image_set_src(img, icon);
        lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        gear_menu_btn->img = img;
    }

    if (txt) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_flex_grow(label, 1);
        gear_menu_btn->label = label;
    }

    refresh_menu(gear_menu);
    return btn;
}

static int32_t get_normalized_angle_in_loop(lv_obj_t *obj, int32_t angle)
{
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    int32_t dips_child_cnt = get_disp_child_cnt(obj);
    int32_t angle_step = gear_menu->angle_step;
    int32_t valid_icon_agnles = dips_child_cnt * angle_step;
    int32_t loop_angle_offset = (gear_menu->angle_end - gear_menu->angle_start) - valid_icon_agnles;
    int32_t step;

    if (gear_menu->angle_start >= gear_menu->angle_end || angle_step < 0) {
        return gear_menu->angle_start;
    }

    if (dips_child_cnt <= 1) {
        // 边界情况
        if (angle < gear_menu->angle_start) {
            while (angle < gear_menu->angle_start) {
                step = angle - gear_menu->angle_start;
                angle = gear_menu->angle_end + step;
            }
        } else if (angle >= gear_menu->angle_end) {
            while (angle >= gear_menu->angle_end) {
                step = angle - gear_menu->angle_end;
                angle = gear_menu->angle_start + step;
            }
        }
        return angle;
    }

    int32_t loop_min_angle = gear_menu->angle_start - (dips_child_cnt - 1) * angle_step;
    int32_t loop_max_angle = gear_menu->angle_end;
    if (angle < loop_min_angle) {
        if (loop_angle_offset > 0) {
            while (angle < loop_min_angle) {
                step = angle - loop_min_angle;
                angle = gear_menu->angle_end - (dips_child_cnt - 1) * angle_step + step;
            }
        } else {
            while (angle < loop_min_angle) {
                step = angle - loop_min_angle;
                angle = gear_menu->angle_start + angle_step + step;
            }
        }
        return angle;
    } else {
        if (loop_angle_offset > 0) {
            while (angle >= loop_max_angle) {
                step = angle - loop_max_angle;
                angle = gear_menu->angle_start + step;
            }
        } else {
            while (angle >= loop_max_angle) {
                step = angle - loop_max_angle;
                angle = gear_menu->angle_end - valid_icon_agnles + step;
            }
        }
        return angle;
    }
}


void lv_gearmenu_rotate_to_the_closest_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;

    /* 获取最近的 angle_step 整数倍的角度 */
    int32_t angle = gear_menu->angle;
    int32_t angle_step = gear_menu->angle_step;
    int32_t idx = angle / angle_step;
    int32_t angle_closest = idx * angle_step;
    if (angle - angle_closest > angle_step / 2) {
        angle_closest += angle_step;
    } else if (angle - angle_closest < -angle_step / 2) {
        angle_closest -= angle_step;
    }


    int32_t max_angle, min_angle;
    if (!gear_menu->is_loop_mode) {
        get_angle_range(obj, &max_angle, &min_angle);
    } else {
        max_angle = gear_menu->angle_end;
        min_angle = gear_menu->angle_start;
        int32_t disp_count = get_disp_child_cnt(obj);
        if (disp_count > 1) {
            min_angle = gear_menu->angle_start - (disp_count - 1) * gear_menu->angle_step;
        }
    }

    if (angle_closest > max_angle) {
        angle_closest = max_angle;
    } else if (angle_closest < min_angle) {
        angle_closest = min_angle;
    }

    /* 动画旋转到最近的角度 */
    if (gear_menu->bounce_anim_time <= 0) {
        lv_anim_delete(obj, NULL);
        set_menu_angle(obj, angle_closest);
    } else {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, gearmenu_anim_cb);
        lv_anim_set_deleted_cb(&a, gearmenu_anim_del_cb);
        lv_anim_set_values(&a, gear_menu->angle, angle_closest);
        lv_anim_set_var(&a, obj);
        lv_anim_set_duration(&a, gear_menu->bounce_anim_time);
        lv_anim_start(&a);
    }
}

/*=====================
 * Setter functions
 *====================*/

void lv_gearmenu_set_start_angle(lv_obj_t *obj, int32_t start)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->angle_start = start;
    refresh_menu(obj);
}

void lv_gearmenu_set_end_angle(lv_obj_t *obj, int32_t end)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->angle_end = end;
    refresh_menu(obj);
}

void lv_gearmenu_set_angles(lv_obj_t *obj, int32_t start, int32_t end)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->angle_start = start;
    gear_menu->angle_end = end;
    refresh_menu(obj);
}

void lv_gearmenu_set_radius(lv_obj_t *obj, int32_t radius)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->radius = radius;
    refresh_menu(obj);
}

void lv_gearmenu_set_angle_step(lv_obj_t *obj, int32_t angle_step)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->angle_step = angle_step;
    refresh_menu(obj);
}

void lv_gearmenu_set_px_per_angle(lv_obj_t *obj, int32_t px_per_angle)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->px_per_angle = px_per_angle;
    refresh_menu(obj);
}

void lv_gearmenu_set_center_x(lv_obj_t *obj, int32_t x)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->center.x = x;
    refresh_menu(obj);
}

void lv_gearmenu_set_center_y(lv_obj_t *obj, int32_t y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->center.y = y;
    refresh_menu(obj);
}

void lv_gearmenu_set_center(lv_obj_t *obj, int32_t x, int32_t y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->center.x = x;
    gear_menu->center.y = y;
    refresh_menu(obj);
}

void lv_gearmenu_set_clockwise(lv_obj_t *obj, bool is_clockwise)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->is_clockwise = is_clockwise;
    refresh_menu(obj);
}

void lv_gearmenu_set_button_to_center(lv_obj_t *obj, lv_obj_t *btn)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;
    if (gear_menu_btn->is_hide || lv_obj_get_parent(btn) != obj) {
        return;
    }

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    int32_t cur_idx = 0;
    int32_t child_cnt = lv_obj_get_child_count(obj);
    for (int32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)child;
        if (gear_menu_btn->is_hide) {
            continue;
        }
        if (child == btn) {
            break;
        }
        cur_idx++;
    }

    int32_t mid_angle = (gear_menu->angle_start + gear_menu->angle_end) / 2;
    int32_t remainder = mid_angle % gear_menu->angle_step;
    if (remainder != 0) {
        if (remainder >= gear_menu->angle_step / 2) {
            mid_angle = ((mid_angle / gear_menu->angle_step) + 1) * gear_menu->angle_step;
        } else {
            mid_angle = (mid_angle / gear_menu->angle_step) * gear_menu->angle_step;
        }
    }

    int32_t angle = gear_menu->is_clockwise ?
                    mid_angle - cur_idx * gear_menu->angle_step :
                    mid_angle - (get_disp_child_cnt(obj) - cur_idx - 1) * gear_menu->angle_step;
    set_menu_angle(obj, angle);
}

void lv_gearmenu_button_set_image(lv_obj_t *btn, const void *icon)
{
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_obj_t *img = get_img(btn);
    lv_image_set_src(img, icon);
    update_img_size_with_scale(img);
    refresh_menu(lv_obj_get_parent(btn));
}

void lv_gearmenu_button_set_text(lv_obj_t *btn, const char *txt)
{
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_obj_t *label = get_label(btn);
    lv_label_set_text(label, txt);
}

void lv_gearmenu_button_set_image_zoom(lv_obj_t *btn, uint16_t zoom)
{
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_obj_t *img = get_img(btn);
    lv_image_set_scale(img, zoom);
    update_img_size_with_scale(img);
    refresh_menu(lv_obj_get_parent(btn));
}

void lv_gearmenu_button_set_hide(lv_obj_t *btn, bool is_hide)
{
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;
    gear_menu_btn->is_hide = is_hide;
    if (is_hide) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }
    refresh_menu(lv_obj_get_parent(btn));
}

void lv_gearmenu_set_angle(lv_obj_t *obj, int32_t angle)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    set_menu_angle(obj, angle);
}

void lv_gearmenu_set_bounce_anim_time(lv_obj_t *obj, int32_t bounce_anim_time)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->bounce_anim_time = bounce_anim_time;
}

void lv_gearmenu_set_loop_mode(lv_obj_t *obj, bool is_loop_mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;

    gear_menu->is_loop_mode = is_loop_mode;
    refresh_menu(obj);
}

/*=====================
 * Getter functions
 *====================*/

int32_t lv_gearmenu_get_start_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->angle_start;
}

int32_t lv_gearmenu_get_end_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->angle_end;
}

int32_t lv_gearmenu_get_radius(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->radius;
}

int32_t lv_gearmenu_get_angle_step(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->angle_step;
}

int32_t lv_gearmenu_get_px_per_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->px_per_angle;
}

bool lv_gearmenu_button_get_hide(lv_obj_t *btn)
{
    LV_ASSERT_OBJ(btn, &lv_gearmenu_button_class);

    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;
    return gear_menu_btn->is_hide;
}

int32_t lv_gearmenu_get_angle(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->angle;
}

int32_t lv_gearmenu_get_bounce_anim_time(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->bounce_anim_time;
}

bool lv_gearmenu_get_loop_mode(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    return gear_menu->is_loop_mode;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_gearmenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    gear_menu->angle_start = -90;
    gear_menu->angle_end = 90;
    gear_menu->angle = 30;
    gear_menu->angle_step = 30;
    gear_menu->bounce_anim_time = 200;

    gear_menu->radius = 140;
    gear_menu->px_per_angle = 2;
    gear_menu->last_point.x = 0;
    gear_menu->last_point.y = 0;
    gear_menu->center.x = 160;
    gear_menu->center.y = 200;
    gear_menu->is_clockwise = true;
    gear_menu->is_loop_mode = true;
    gear_menu->_is_init = false;

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_gearmenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)obj;
    gear_menu_btn->img = NULL;
    gear_menu_btn->label = NULL;
    gear_menu_btn->is_hide = false;
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_gearmenu_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    /* Call the ancestor's event handler */
    lv_result_t res;

    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RESULT_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cont = lv_event_get_current_target(e);
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)cont;

    /* 中心坐标 */
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    int32_t x_center = cont_a.x1 + gear_menu->center.x;
    int32_t y_center = cont_a.y1 + gear_menu->center.y;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_active(), &gear_menu->last_point);
    } else if (code == LV_EVENT_PRESSING) {
        lv_point_t p;
        lv_point_t last_point = gear_menu->last_point;
        lv_indev_get_point(lv_indev_active(), &p);
        lv_anim_delete(cont, NULL);

        /* 判断触摸区域 */
        int touch_area = 0;
        if (p.y < y_center && (y_center - p.y) > LV_ABS(p.x - x_center)) {
            touch_area = 1; // 上
        } else if (p.x > x_center && (p.x - x_center) > LV_ABS(p.y - y_center)) {
            touch_area = 2; // 右
        } else if (p.y > y_center && (p.y - y_center) > LV_ABS(p.x - x_center)) {
            touch_area = 3; // 下
        } else if (p.x < x_center && (x_center - p.x) > LV_ABS(p.y - y_center)) {
            touch_area = 4; // 左
        }

        /* 根据区域调整角度 */
        switch (touch_area) {
        case 1: // 上
            gear_menu->angle += (p.x - gear_menu->last_point.x) / gear_menu->px_per_angle;
            break;
        case 2: // 右
            gear_menu->angle += (p.y - gear_menu->last_point.y) / gear_menu->px_per_angle;
            break;
        case 3: // 下
            gear_menu->angle -= (p.x - gear_menu->last_point.x) / gear_menu->px_per_angle;
            break;
        case 4: // 左
            gear_menu->angle -= (p.y - gear_menu->last_point.y) / gear_menu->px_per_angle;
            break;
        }

        gear_menu->last_point = p;
        refresh_menu(cont);
    } else if (code == LV_EVENT_RELEASED) {
        lv_gearmenu_rotate_to_the_closest_angle(cont);
    } else if (code == LV_EVENT_CHILD_CREATED || code == LV_EVENT_CHILD_DELETED || code == LV_EVENT_STYLE_CHANGED ||
               code == LV_EVENT_SIZE_CHANGED) {
        if (lv_obj_get_width(cont) != 0 && lv_obj_get_height(cont) != 0) {
            gear_menu->_is_init = true;
            refresh_menu(cont);
        }
    }
}



static void update_menu_items(lv_obj_t *cont)
{
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)cont;
    int32_t child_cnt = lv_obj_get_child_count(cont);

    // start < end 检查 和 step 检查
    if (gear_menu->angle_start >= gear_menu->angle_end || gear_menu->angle_step < 0) {
        for (int32_t idx = 0; idx < child_cnt; idx++) {
            lv_obj_t *child = lv_obj_get_child(cont, idx);
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    int32_t padding_col = lv_obj_get_style_pad_column(cont, 0);
    int32_t center_x = gear_menu->center.x;
    int32_t center_y = gear_menu->center.y;
    int32_t radius = gear_menu->radius;

    int32_t angle_step = gear_menu->angle_step;
    int32_t base_angle = gear_menu->angle;
    int32_t angle_start = gear_menu->angle_start;
    int32_t angle_end = gear_menu->angle_end;
    int32_t start_idx = gear_menu->is_clockwise ? 0 : child_cnt - 1; // 确定循环范围
    int32_t end_idx = gear_menu->is_clockwise ? child_cnt : -1;
    int32_t step = gear_menu->is_clockwise ? 1 : -1;
    int32_t cur_idx = 0;

    int32_t first_btn_idx = -1, last_btn_idx = -1;
    int32_t first_visible_btn_idx = -1, last_visible_btn_idx = -1;

    int32_t valid_icon_agnles = 0; // (总的is_hide为false的icon数量) * angle_step
    int32_t disp_icon_count = 0;

    for (int32_t i = start_idx; i != end_idx; i += step) {
        lv_obj_t *child = lv_obj_get_child(cont, i);
        lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)child;
        if (gear_menu_btn->is_hide) {
            continue;
        }
        disp_icon_count++;
        valid_icon_agnles += angle_step;

        int32_t cur_angle = angle_step * cur_idx + base_angle; // 计算当前按钮的角度
        // 显示或隐藏按钮
        if (cur_angle >= angle_start && cur_angle < angle_end) {
            if (first_btn_idx < 0) {
                first_btn_idx = i;
                first_visible_btn_idx = cur_idx;
            }
            last_btn_idx = i;
            last_visible_btn_idx = cur_idx;

            int32_t x = center_x + radius * lv_trigo_cos(cur_angle) / LV_TRIGO_SIN_MAX;
            int32_t y = center_y + radius * lv_trigo_sin(cur_angle) / LV_TRIGO_SIN_MAX;
            int32_t child_h = gear_menu_btn->img ? lv_obj_get_height(gear_menu_btn->img) : gear_menu_btn->label ?
                              lv_obj_get_height(gear_menu_btn->label) : 0;

            // 修改文本的y坐标
            if (gear_menu_btn->label && gear_menu_btn->img) {
                int32_t label_x = lv_obj_get_width(gear_menu_btn->img) + padding_col;
                int32_t label_h = lv_obj_get_height(gear_menu_btn->label);
                lv_obj_set_pos(gear_menu_btn->label, label_x, (lv_obj_get_height(gear_menu_btn->img) - label_h) / 2);
            }
            lv_obj_set_pos(child, x, y - child_h / 2);
            lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }

        cur_idx++;
    }

    if (gear_menu->is_loop_mode) {
        if (!gear_menu->is_clockwise) {
            // 逆时针时交换，位置反过来
            int32_t temp = first_btn_idx;
            first_btn_idx = last_btn_idx;
            last_btn_idx = temp;

            temp = first_visible_btn_idx;
            first_visible_btn_idx = last_visible_btn_idx;
            last_visible_btn_idx = temp;
        }

        int32_t loop_angle_offset = LV_MAX((angle_end - angle_start) - valid_icon_agnles, 0);
        loop_angle_offset = gear_menu->is_clockwise ? loop_angle_offset : -loop_angle_offset;

        if (first_btn_idx >= 0) {
            // 顺时针时从第一个按钮往前，逆时针时第一个按钮往后
            int32_t i = (first_btn_idx + child_cnt - 1) % child_cnt;
            int32_t offset_angle = angle_step * (gear_menu->is_clockwise ? (first_visible_btn_idx - 1) :
                                                 (first_visible_btn_idx + 1));
            while (i != first_btn_idx) {
                lv_obj_t *child = lv_obj_get_child(cont, i);
                lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)child;
                if (gear_menu_btn->is_hide) {
                    i = (i + child_cnt - 1) % child_cnt;
                    continue;
                }
                if (!lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
                    break;
                }
                int32_t cur_angle = offset_angle + base_angle - loop_angle_offset;
                if (!(cur_angle >= angle_start && cur_angle < angle_end)) {
                    break;
                }
                int32_t x = center_x + radius * lv_trigo_cos(cur_angle) / LV_TRIGO_SIN_MAX;
                int32_t y = center_y + radius * lv_trigo_sin(cur_angle) / LV_TRIGO_SIN_MAX;
                int32_t child_h = gear_menu_btn->img ? lv_obj_get_height(gear_menu_btn->img) : gear_menu_btn->label ?
                                  lv_obj_get_height(gear_menu_btn->label)
                                  : 0;
                if (gear_menu_btn->label && gear_menu_btn->img) {
                    int32_t label_x = lv_obj_get_width(gear_menu_btn->img) + padding_col;
                    int32_t label_h = lv_obj_get_height(gear_menu_btn->label);
                    lv_obj_set_pos(gear_menu_btn->label, label_x, (lv_obj_get_height(gear_menu_btn->img) - label_h) / 2);
                }
                lv_obj_set_pos(child, x, y - child_h / 2);
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);

                // 更新循环条件
                i = (i + child_cnt - 1) % child_cnt;
                offset_angle += gear_menu->is_clockwise ? -angle_step : angle_step;
            }
        }
        if (last_btn_idx >= 0) {
            // 顺时针时从最后一个按钮往后, 逆时针时第一个按钮往前
            int32_t i = (last_btn_idx + 1) % child_cnt;
            int32_t offset_angle = angle_step * (gear_menu->is_clockwise ? (last_visible_btn_idx + 1) : (last_visible_btn_idx - 1));
            while (i != last_btn_idx) {
                lv_obj_t *child = lv_obj_get_child(cont, i);
                lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)child;
                if (gear_menu_btn->is_hide) {
                    i = (i + 1) % child_cnt;
                    continue;
                }
                if (!lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
                    break;
                }
                int32_t cur_angle = offset_angle + base_angle + loop_angle_offset;
                if (!(cur_angle >= angle_start && cur_angle < angle_end)) {
                    break;
                }
                int32_t x = center_x + radius * lv_trigo_cos(cur_angle) / LV_TRIGO_SIN_MAX;
                int32_t y = center_y + radius * lv_trigo_sin(cur_angle) / LV_TRIGO_SIN_MAX;
                int32_t child_h = gear_menu_btn->img ? lv_obj_get_height(gear_menu_btn->img) : gear_menu_btn->label ?
                                  lv_obj_get_height(gear_menu_btn->label)
                                  : 0;
                if (gear_menu_btn->label && gear_menu_btn->img) {
                    int32_t label_x = lv_obj_get_width(gear_menu_btn->img) + padding_col;
                    int32_t label_h = lv_obj_get_height(gear_menu_btn->label);
                    lv_obj_set_pos(gear_menu_btn->label, label_x, (lv_obj_get_height(gear_menu_btn->img) - label_h) / 2);
                }
                lv_obj_set_pos(child, x, y - child_h / 2);
                lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);

                // 更新循环条件
                i = (i + 1) % child_cnt;
                offset_angle += gear_menu->is_clockwise ? angle_step : -angle_step;
            }
        }
    }

    lv_obj_invalidate(cont);
}

static void gearmenu_anim_cb(void *var, int32_t v)
{
    lv_obj_t *cont = (lv_obj_t *)var;
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)cont;
    // set_menu_angle(obj, gear_menu->angle);
    if (gear_menu->is_loop_mode) {
        gear_menu->angle = get_normalized_angle_in_loop(cont, v);
    } else {
        gear_menu->angle = v;
    }
    update_menu_items(cont);
}

static void gearmenu_anim_del_cb(lv_anim_t *cb)
{
    lv_obj_t *cont = (lv_obj_t *)cb->var;
    refresh_menu(cont);
}

static uint32_t get_disp_child_cnt(const lv_obj_t *obj)
{
    uint32_t cnt = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)child;
        if (!gear_menu_btn->is_hide) {
            cnt++;
        }
    }
    return cnt;
}

static lv_obj_t *get_img(lv_obj_t *btn)
{
    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;
    if (!gear_menu_btn->img) {
        gear_menu_btn->img = lv_image_create(btn);
        lv_obj_remove_flag(gear_menu_btn->img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

    return gear_menu_btn->img;
}

static lv_obj_t *get_label(lv_obj_t *btn)
{
    lv_gearmenu_button_t *gear_menu_btn = (lv_gearmenu_button_t *)btn;
    if (!gear_menu_btn->label) {
        gear_menu_btn->label = lv_label_create(btn);
        lv_label_set_long_mode(gear_menu_btn->label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_remove_flag(gear_menu_btn->label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

    return gear_menu_btn->label;
}

static void get_angle_range(lv_obj_t *cont, int32_t *angle_start, int32_t *angle_end)
{
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)cont;
    int32_t disp_child_cnt = get_disp_child_cnt(cont);

    /* 角度范围限制 */
    *angle_start = (gear_menu->angle_end + gear_menu->angle_start) / 2;
    *angle_end = (disp_child_cnt - 1) * -gear_menu->angle_step + *angle_start;
}

static void set_menu_angle(lv_obj_t *obj, int32_t angle)
{
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;

    if (gear_menu->is_loop_mode) {
        gear_menu->angle = get_normalized_angle_in_loop(obj, angle);
    } else {
        /* 角度范围限制 */
        int32_t max_angle, min_angle;
        get_angle_range(obj, &max_angle, &min_angle);
        max_angle += MAX_SCROLL_ANGLE;
        min_angle -= MAX_SCROLL_ANGLE;
        gear_menu->angle = angle;
        if (gear_menu->angle > max_angle) {
            gear_menu->angle = max_angle;
        } else if (gear_menu->angle < min_angle) {
            gear_menu->angle = min_angle;
        }
    }

    update_menu_items(obj);
}

static void refresh_menu(lv_obj_t *obj)
{
    lv_gearmenu_t *gear_menu = (lv_gearmenu_t *)obj;
    set_menu_angle(obj, gear_menu->angle);
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
