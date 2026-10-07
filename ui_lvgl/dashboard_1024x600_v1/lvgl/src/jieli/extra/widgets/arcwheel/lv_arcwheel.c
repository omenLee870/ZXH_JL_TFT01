/**
 * @file lv_arcwheel.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_arcwheel_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"

#if LV_USE_ARCWHEEL != 0

#if LV_USE_IMAGE == 0
    #error "lv_arcwheel: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_arcwheel_class)
#define ITEM_CLASS (&lv_arcwheel_item_class)

#define SCROLL_MOMENTUM_THRESHOLD 1
#define SNAP_TRANSITION_THRESHOLD 30
#define SNAP_ANIM_MIN_DURATION 150
#define SNAP_ANIM_MAX_DURATION 500
#define SNAP_ANIM_ANGLE_FACTOR 2

#define SLIDE_SCALE_ZOOM_MIN 180
#define SLIDE_SCALE_ZOOM_MAX 256

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_arcwheel_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_arcwheel_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_arcwheel_item_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_arcwheel_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void update_items(lv_obj_t * wheel_obj);
static void scroll_timer_cb(lv_timer_t * timer);
static void apply_item_transform(lv_obj_t * item, int32_t x, int32_t y, uint16_t zoom);
static void update_angle_step(lv_arcwheel_t * wheel);
static int32_t calculate_snap_target_angle(lv_arcwheel_t * wheel);
static int32_t calculate_shortest_rotation(int32_t from_angle, int32_t to_angle, int32_t total_angle_range);
static void start_snap_animation(lv_arcwheel_t * wheel, int32_t target_angle, uint32_t speed);
static void stop_snap_animation(lv_arcwheel_t * wheel);
static void snap_anim_exec_cb(void * var, int32_t value);
static void snap_anim_completed_cb(lv_anim_t * anim);
static int32_t clamp_angle_if_needed(lv_arcwheel_t * wheel, int32_t angle);
static bool lex_greater(float d1, uint32_t i1, float d2, uint32_t i2, float eps);
static bool lex_less(float d1, uint32_t i1, float d2, uint32_t i2, float eps);
static float calculate_display_angle(float natural_angle, float current_angle,
                                     float total_angle_range, float half_range);

static uint16_t slide_scale_item_transform_cb(const lv_obj_t * arcwheel, const lv_obj_t * item, int32_t angle,
                                              lv_point_t * item_pos);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_arcwheel_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_arcwheel_constructor,
    .destructor_cb = lv_arcwheel_destructor,
    .event_cb = lv_arcwheel_event,
    .width_def = 300,
    .height_def = 300,
    .instance_size = sizeof(lv_arcwheel_t),
};

const lv_obj_class_t lv_arcwheel_item_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_arcwheel_item_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_arcwheel_item_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_arcwheel_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t * lv_arcwheel_add_item(lv_obj_t * wheel_obj, const void * icon)
{
    LV_ASSERT_OBJ(wheel_obj, MY_CLASS);

    lv_obj_t * item = lv_obj_class_create_obj(ITEM_CLASS, wheel_obj);
    lv_obj_class_init_obj(item);

    if(icon) {
        lv_image_set_src(item, icon);
    }

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)wheel_obj;
    update_angle_step(wheel);
    update_items(wheel_obj);

    return item;
}

void lv_arcwheel_set_radius(lv_obj_t * obj, int32_t radius_x, int32_t radius_y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    if(radius_x <= 0 || radius_y <= 0) {
        LV_LOG_ERROR("radius_x and radius_y must be greater than 0");
        return;
    }

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->radius_x = radius_x;
    wheel->radius_y = radius_y;
    update_items(obj);
}

void lv_arcwheel_set_px_per_angle(lv_obj_t * obj, int32_t px_per_angle)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    if(px_per_angle <= 0) {
        LV_LOG_ERROR("px_per_angle must be greater than 0");
        return;
    }

    ((lv_arcwheel_t *)obj)->px_per_angle = px_per_angle;
}

void lv_arcwheel_set_loop(lv_obj_t * obj, bool enabled)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->is_loop = enabled;
    wheel->current_angle = clamp_angle_if_needed(wheel, wheel->current_angle);
    update_items(obj);
}

void lv_arcwheel_set_visible_count(lv_obj_t * obj, uint16_t count)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    if(count == 0) {
        LV_LOG_ERROR("count must be greater than 0");
        return;
    }

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->visible_count = count;
    update_angle_step(wheel);
    wheel->current_angle = clamp_angle_if_needed(wheel, wheel->current_angle);
    update_items(obj);
}

void lv_arcwheel_set_angle_offset(lv_obj_t * obj, int32_t offset)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((lv_arcwheel_t *)obj)->angle_offset = offset;
    update_items(obj);
}

void lv_arcwheel_rotate_to_angle(lv_obj_t * obj, int32_t angle, bool anim, uint32_t speed)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    int32_t target_angle = clamp_angle_if_needed(wheel, angle);

    if(wheel->scroll.scroll_tmr) {
        lv_timer_pause(wheel->scroll.scroll_tmr);
    }
    wheel->scroll.momentum = 0;
    stop_snap_animation(wheel);

    if(anim) {
        start_snap_animation(wheel, target_angle, speed);
    }
    else {
        wheel->current_angle = target_angle;
        update_items(obj);
    }
}

void lv_arcwheel_stop_rotate(lv_obj_t * obj, bool snap_to_nearest, uint32_t speed)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    if(wheel->scroll.scroll_tmr) {
        lv_timer_pause(wheel->scroll.scroll_tmr);
    }
    wheel->scroll.momentum = 0;
    stop_snap_animation(wheel);

    if(!snap_to_nearest || !wheel->is_snap) return;

    int32_t target_angle = clamp_angle_if_needed(wheel, calculate_snap_target_angle(wheel));
    start_snap_animation(wheel, target_angle, speed);
}

void lv_arcwheel_item_set_src(lv_obj_t * item, const void * src)
{
    LV_ASSERT_OBJ(item, ITEM_CLASS);
    lv_image_set_src(item, src);

    lv_obj_t * parent = lv_obj_get_parent(item);
    if(parent != NULL && lv_obj_check_type(parent, MY_CLASS)) {
        update_items(parent);
    }
}

void lv_arcwheel_set_style(lv_obj_t * obj, lv_arcwheel_style_t style, lv_arcwheel_transform_param_t * params)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->style = style;

    if(style == LV_ARCWHEEL_STYLE_SLIDE_SCALE) {
        wheel->item_transform_cb = slide_scale_item_transform_cb;
        if(params) {
            wheel->transform_param = *params;
        }
        else {
            wheel->transform_param.slide_scale.zoom_min = SLIDE_SCALE_ZOOM_MIN;
            wheel->transform_param.slide_scale.zoom_max = SLIDE_SCALE_ZOOM_MAX;
        }
    }

    update_items(obj);
}

void lv_arcwheel_set_item_transform_cb(lv_obj_t * obj, lv_arcwheel_item_transform_cb_t transform_cb,
                                       lv_arcwheel_transform_param_t * params)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->item_transform_cb = transform_cb;
    wheel->style = LV_ARCWHEEL_STYLE_CUSTOM;
    if(params) {
        wheel->transform_param = *params;
    }

    update_items(obj);
}

void lv_arcwheel_set_friction_factor(lv_obj_t * obj, int32_t factor)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((lv_arcwheel_t *)obj)->friction_factor = CLAMP(factor, 0, 100);
}

void lv_arcwheel_set_snap_factor(lv_obj_t * obj, int32_t factor)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((lv_arcwheel_t *)obj)->snap_factor = CLAMP(factor, 0, 100);
}

void lv_arcwheel_set_snap(lv_obj_t * obj, bool enabled)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((lv_arcwheel_t *)obj)->is_snap = enabled;
}

void lv_arcwheel_set_scroll_direction(lv_obj_t * obj, lv_arcwheel_scroll_dir_t dir)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((lv_arcwheel_t *)obj)->scroll_dir = dir;
}

int32_t lv_arcwheel_get_radius_x(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->radius_x;
}

int32_t lv_arcwheel_get_radius_y(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->radius_y;
}

int32_t lv_arcwheel_get_current_angle(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->current_angle;
}

int32_t lv_arcwheel_get_angle_offset(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->angle_offset;
}

uint16_t lv_arcwheel_get_item_count(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return (uint16_t)lv_obj_get_child_count(obj);
}

uint16_t lv_arcwheel_get_visible_count(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->visible_count;
}

int32_t lv_arcwheel_get_px_per_angle(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->px_per_angle;
}

bool lv_arcwheel_is_loop(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->is_loop;
}

lv_arcwheel_style_t lv_arcwheel_get_style(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->style;
}

lv_arcwheel_transform_param_t * lv_arcwheel_get_transform_params(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return &((lv_arcwheel_t *)obj)->transform_param;
}

int32_t lv_arcwheel_get_friction_factor(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->friction_factor;
}

int32_t lv_arcwheel_get_snap_factor(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->snap_factor;
}

bool lv_arcwheel_is_snap(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->is_snap;
}

lv_arcwheel_scroll_dir_t lv_arcwheel_get_scroll_direction(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    return ((lv_arcwheel_t *)obj)->scroll_dir;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_arcwheel_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    wheel->radius_x = 150;
    wheel->radius_y = 150;
    wheel->current_angle = 0;
    wheel->angle_offset = 0;
    wheel->visible_count = 0;
    wheel->angle_step = 0;
    wheel->px_per_angle = 1;

    wheel->style = LV_ARCWHEEL_STYLE_SLIDE_SCALE;
    wheel->item_transform_cb = slide_scale_item_transform_cb;
    wheel->transform_param.slide_scale.zoom_min = SLIDE_SCALE_ZOOM_MIN;
    wheel->transform_param.slide_scale.zoom_max = SLIDE_SCALE_ZOOM_MAX;

    wheel->is_loop = true;
    wheel->scroll_dir = LV_ARCWHEEL_SCROLL_DIR_HORIZONTAL;
    wheel->_is_init = false;
    wheel->scroll.momentum = 0;
    wheel->scroll.scroll_tmr = lv_timer_create(scroll_timer_cb, LV_DEF_REFR_PERIOD, obj);
    lv_timer_pause(wheel->scroll.scroll_tmr);

    wheel->snap.is_animating = false;
    lv_anim_init(&wheel->snap.anim);

    wheel->friction_factor = 78;
    wheel->snap_factor = 40;
    wheel->is_snap = true;

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_arcwheel_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    stop_snap_animation(wheel);
    if(wheel->scroll.scroll_tmr) {
        lv_timer_delete(wheel->scroll.scroll_tmr);
        wheel->scroll.scroll_tmr = NULL;
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_arcwheel_item_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_CENTER);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_arcwheel_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;

    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        stop_snap_animation(wheel);
        lv_timer_pause(wheel->scroll.scroll_tmr);
        wheel->scroll.momentum = 0;
    }
    else if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_point_t diff = {0, 0};
        if(indev != NULL) {
            lv_indev_get_vect(indev, &diff);
        }
        int32_t scroll_offset = wheel->scroll_dir == LV_ARCWHEEL_SCROLL_DIR_HORIZONTAL ? diff.x : diff.y;
        int32_t angle_delta = (int32_t)((float)scroll_offset / LV_MAX(wheel->px_per_angle, 1));
        wheel->current_angle = clamp_angle_if_needed(wheel, wheel->current_angle + angle_delta);
        update_items(obj);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_point_t diff = {0, 0};
        if(indev != NULL) {
            lv_indev_get_vect(indev, &diff);
        }
        wheel->scroll.momentum = wheel->scroll_dir == LV_ARCWHEEL_SCROLL_DIR_HORIZONTAL ? diff.x : diff.y;
        lv_timer_resume(wheel->scroll.scroll_tmr);
    }
    else if(code == LV_EVENT_SIZE_CHANGED) {
        if(!wheel->_is_init && lv_obj_get_width(obj) != 0 && lv_obj_get_height(obj) != 0) {
            wheel->_is_init = true;
            update_items(obj);
        }
    }
    else if(code == LV_EVENT_STYLE_CHANGED) {
        if(wheel->_is_init) {
            update_items(obj);
        }
    }
}

static void scroll_timer_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)timer->user_data;
    lv_arcwheel_t * wheel = (lv_arcwheel_t *)obj;

    wheel->scroll.momentum = (int32_t)((float)wheel->scroll.momentum * (wheel->friction_factor / 100.0f));
    int32_t angle_delta = (int32_t)((float)wheel->scroll.momentum / LV_MAX(wheel->px_per_angle, 1));

    if(wheel->is_snap && LV_ABS(wheel->scroll.momentum) < SNAP_TRANSITION_THRESHOLD) {
        lv_timer_pause(timer);
        wheel->scroll.momentum = 0;
        start_snap_animation(wheel, calculate_snap_target_angle(wheel), 0);
        return;
    }

    int32_t new_angle = wheel->current_angle + angle_delta;
    int32_t clamped_angle = clamp_angle_if_needed(wheel, new_angle);

    if(!wheel->is_loop && clamped_angle != new_angle) {
        wheel->current_angle = clamped_angle;
        wheel->scroll.momentum = 0;
        lv_timer_pause(timer);
        update_items(obj);
        return;
    }

    wheel->current_angle = clamped_angle;

    if(!wheel->is_snap && LV_ABS(wheel->scroll.momentum) < SCROLL_MOMENTUM_THRESHOLD) {
        wheel->scroll.momentum = 0;
        lv_timer_pause(timer);
    }

    update_items(obj);
}

static bool lex_less(float d1, uint32_t i1, float d2, uint32_t i2, float eps)
{
    if(d1 < d2 - eps) return true;
    if(d1 > d2 + eps) return false;
    return i1 < i2;
}

static bool lex_greater(float d1, uint32_t i1, float d2, uint32_t i2, float eps)
{
    if(d1 > d2 + eps) return true;
    if(d1 < d2 - eps) return false;
    return i1 > i2;
}

static float calculate_display_angle(float natural_angle, float current_angle,
                                     float total_angle_range, float half_range)
{
    float display_angle = natural_angle - current_angle;

    while(display_angle > half_range) display_angle -= total_angle_range;
    while(display_angle < -half_range) display_angle += total_angle_range;

    return display_angle;
}

static void update_items(lv_obj_t * wheel_obj)
{
    if(!lv_obj_is_valid(wheel_obj)) return;

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)wheel_obj;
    if(lv_obj_get_width(wheel_obj) == 0 || lv_obj_get_height(wheel_obj) == 0) return;

    uint32_t child_cnt = lv_obj_get_child_count(wheel_obj);
    if(child_cnt == 0) return;

    lv_area_t area;
    lv_obj_get_coords(wheel_obj, &area);
    int32_t center_x = (area.x2 - area.x1 + 1) / 2;
    int32_t center_y = (area.y2 - area.y1 + 1) / 2;

    uint16_t total_items = (uint16_t)child_cnt;
    uint16_t effective_count = wheel->visible_count > 0 ? wheel->visible_count : total_items;

    float display_angle_span = 360.0f / (effective_count == 0 ? 1 : effective_count);
    float total_angle_range = display_angle_span * total_items;
    float half_range = total_angle_range / 2.0f;
    bool show_all = wheel->visible_count == 0 || wheel->visible_count >= total_items;

    const float eps = 1e-6f;
    float threshold_distance = 0.0f;
    uint32_t threshold_index = 0;

    if(!show_all) {
        float prev_distance = -1.0f;
        uint32_t prev_index = UINT32_MAX;

        for(uint16_t rank = 0; rank < effective_count; rank++) {
            float best_distance = 1e9f;
            uint32_t best_index = UINT32_MAX;
            bool found = false;

            for(uint32_t i = 0; i < child_cnt; i++) {
                float item_natural_angle = i * display_angle_span;
                float display_angle = calculate_display_angle(item_natural_angle, wheel->current_angle,
                                                              total_angle_range, half_range);
                float abs_distance = display_angle < 0.0f ? -display_angle : display_angle;

                if(lex_greater(abs_distance, i, prev_distance, prev_index, eps) &&
                   lex_less(abs_distance, i, best_distance, best_index, eps)) {
                    best_distance = abs_distance;
                    best_index = i;
                    found = true;
                }
            }

            if(found) {
                prev_distance = best_distance;
                prev_index = best_index;
            }
            else {
                break;
            }
        }

        threshold_distance = prev_distance;
        threshold_index = prev_index;
    }

    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(wheel_obj, i);

        float item_natural_angle = i * display_angle_span;
        float display_angle = calculate_display_angle(item_natural_angle, wheel->current_angle,
                                                      total_angle_range, half_range);
        float abs_distance = display_angle < 0.0f ? -display_angle : display_angle;
        bool is_visible = show_all || !lex_greater(abs_distance, i, threshold_distance, threshold_index, eps);

        if(is_visible) {
            lv_point_t item_pos = {0, 0};
            int32_t position_angle = (int32_t)(display_angle + wheel->angle_offset);
            uint16_t zoom = LV_SCALE_NONE;

            if(wheel->item_transform_cb) {
                zoom = wheel->item_transform_cb(wheel_obj, child, position_angle, &item_pos);
            }

            if(zoom == 0) {
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            apply_item_transform(child, center_x + item_pos.x, center_y + item_pos.y, zoom);
            lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_obj_invalidate(wheel_obj);
}

static uint16_t slide_scale_item_transform_cb(const lv_obj_t * arcwheel, const lv_obj_t * item, int32_t angle,
                                              lv_point_t * item_pos)
{
    LV_UNUSED(item);

    lv_arcwheel_t * wheel = (lv_arcwheel_t *)arcwheel;
    int32_t sin_val = lv_trigo_sin(angle);
    int32_t sin_val_90 = lv_trigo_sin(angle + 90);

    item_pos->x = (int32_t)((int64_t)wheel->radius_x * sin_val_90 / LV_TRIGO_SIN_MAX);
    item_pos->y = (int32_t)((int64_t)wheel->radius_y * sin_val / LV_TRIGO_SIN_MAX);

    int32_t depth_val = -sin_val;
    int32_t depth_normalized = (depth_val + LV_TRIGO_SIN_MAX) / 2;

    return (uint16_t)(wheel->transform_param.slide_scale.zoom_min +
                      ((int32_t)(wheel->transform_param.slide_scale.zoom_max -
                                 wheel->transform_param.slide_scale.zoom_min) * depth_normalized / LV_TRIGO_SIN_MAX));
}

static void apply_item_transform(lv_obj_t * item, int32_t x, int32_t y, uint16_t zoom)
{
    if(item == NULL) return;

    int32_t zoom_x_offset = lv_obj_get_width(item) * zoom / LV_SCALE_NONE / 2;
    int32_t zoom_y_offset = lv_obj_get_height(item) * zoom / LV_SCALE_NONE / 2;

    lv_obj_set_pos(item, x - zoom_x_offset, y - zoom_y_offset);
    lv_obj_set_style_transform_scale(item, zoom, LV_PART_MAIN);
}

static void update_angle_step(lv_arcwheel_t * wheel)
{
    if(wheel->visible_count == 0) {
        uint32_t total_items = lv_obj_get_child_count((lv_obj_t *)wheel);
        if(total_items <= 1) {
            wheel->angle_step = 0;
            return;
        }
        wheel->angle_step = 360 / (int32_t)total_items;
        return;
    }

    if(wheel->visible_count <= 1) {
        wheel->angle_step = 0;
        return;
    }

    wheel->angle_step = 360 / wheel->visible_count;
}

static int32_t clamp_angle_if_needed(lv_arcwheel_t * wheel, int32_t angle)
{
    uint32_t total_items = lv_obj_get_child_count((lv_obj_t *)wheel);
    if(total_items <= 1 || wheel->angle_step == 0) return 0;

    if(wheel->is_loop) {
        int32_t total_angle_range = wheel->angle_step * (int32_t)total_items;
        angle %= total_angle_range;
        if(angle < 0) angle += total_angle_range;
        return angle;
    }

    int32_t max_angle = ((int32_t)total_items - 1) * wheel->angle_step;
    if(angle < 0) return 0;
    if(angle > max_angle) return max_angle;
    return angle;
}

static int32_t calculate_snap_target_angle(lv_arcwheel_t * wheel)
{
    uint32_t total_items = lv_obj_get_child_count((lv_obj_t *)wheel);
    if(total_items == 0 || wheel->angle_step == 0) return wheel->current_angle;

    int32_t total_angle_range = wheel->angle_step * (int32_t)total_items;
    int32_t half_range = total_angle_range / 2;

    int32_t normalized_current = wheel->current_angle % total_angle_range;
    if(normalized_current < 0) normalized_current += total_angle_range;

    int32_t nearest_slot_index = 0;
    int32_t min_distance = total_angle_range;

    for(uint32_t i = 0; i < total_items; i++) {
        int32_t virtual_angle = (int32_t)i * wheel->angle_step;
        int32_t distance = LV_ABS(virtual_angle - normalized_current);
        if(distance > half_range) {
            distance = total_angle_range - distance;
        }

        if(distance < min_distance) {
            min_distance = distance;
            nearest_slot_index = (int32_t)i;
        }
    }

    return nearest_slot_index * wheel->angle_step;
}

static int32_t calculate_shortest_rotation(int32_t from_angle, int32_t to_angle, int32_t total_angle_range)
{
    int32_t diff = to_angle - from_angle;
    if(total_angle_range <= 0) return diff;

    int32_t half_range = total_angle_range / 2;
    while(diff > half_range) diff -= total_angle_range;
    while(diff < -half_range) diff += total_angle_range;
    return diff;
}

static void start_snap_animation(lv_arcwheel_t * wheel, int32_t target_angle, uint32_t speed)
{
    stop_snap_animation(wheel);

    int32_t angle_diff;
    if(wheel->is_loop) {
        uint32_t total_items = lv_obj_get_child_count((lv_obj_t *)wheel);
        int32_t total_angle_range = wheel->angle_step * (int32_t)total_items;
        angle_diff = calculate_shortest_rotation(wheel->current_angle, target_angle, total_angle_range);
    }
    else {
        angle_diff = target_angle - wheel->current_angle;
    }
    int32_t target_angle_final = wheel->current_angle + angle_diff;

    if(angle_diff == 0) {
        wheel->current_angle = target_angle_final;
        update_items((lv_obj_t *)wheel);
        return;
    }

    uint32_t duration;
    if(speed > 0) {
        duration = lv_anim_speed_to_time(speed, wheel->current_angle, target_angle_final);
        if(duration == 0) {
            wheel->current_angle = target_angle_final;
            update_items((lv_obj_t *)wheel);
            return;
        }
    }
    else {
        duration = SNAP_ANIM_MIN_DURATION + (uint32_t)(LV_ABS(angle_diff) * SNAP_ANIM_ANGLE_FACTOR);
        duration = CLAMP(duration, SNAP_ANIM_MIN_DURATION, SNAP_ANIM_MAX_DURATION);
    }

    lv_anim_init(&wheel->snap.anim);
    lv_anim_set_var(&wheel->snap.anim, wheel);
    lv_anim_set_values(&wheel->snap.anim, wheel->current_angle, target_angle_final);
    lv_anim_set_duration(&wheel->snap.anim, duration);
    lv_anim_set_exec_cb(&wheel->snap.anim, snap_anim_exec_cb);
    lv_anim_set_completed_cb(&wheel->snap.anim, snap_anim_completed_cb);
    lv_anim_set_path_cb(&wheel->snap.anim, lv_anim_path_ease_out);

    wheel->snap.is_animating = true;
    lv_anim_start(&wheel->snap.anim);
}

static void stop_snap_animation(lv_arcwheel_t * wheel)
{
    if(!wheel->snap.is_animating) return;
    lv_anim_delete(wheel, snap_anim_exec_cb);
    wheel->snap.is_animating = false;
}

static void snap_anim_exec_cb(void * var, int32_t value)
{
    lv_arcwheel_t * wheel = (lv_arcwheel_t *)var;
    wheel->current_angle = value;
    update_items((lv_obj_t *)wheel);
}

static void snap_anim_completed_cb(lv_anim_t * anim)
{
    lv_arcwheel_t * wheel = (lv_arcwheel_t *)anim->var;
    int32_t clamped_angle = clamp_angle_if_needed(wheel, wheel->current_angle);
    if(clamped_angle != wheel->current_angle) {
        wheel->current_angle = clamped_angle;
        update_items((lv_obj_t *)wheel);
    }
    wheel->snap.is_animating = false;
}

#endif /*LV_USE_ARCWHEEL*/
