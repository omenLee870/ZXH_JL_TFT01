/**
 * @file lv_listmenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_listmenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"
#include "math.h"

#if LV_USE_LISTMENU != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
    #error "lv_listmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_listmenu_class

#define SCROLL_IN_RANGE(val, bound_a, bound_b) \
    ((val) >= LV_MIN((bound_a), (bound_b)) && (val) <= LV_MAX((bound_a), (bound_b)))
#define CLAMP(x, min, max)              (x < min ? min : (x > max ? max : x))

#define MENU_RENDER_MARGIN  50  /* 菜单渲染边距 */
#define SCROLL_THRESHOLD 10  /* 滚动阈值 */

#ifndef M_PI
    #define M_PI    3.1415926
#endif // M_PI

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_listmenu_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_listmenu_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_listmenu_button_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_listmenu_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void update_menu_items(lv_obj_t * cont);
static void scroll_timer_cb(lv_timer_t * p_timer);
static void scroll_anim_exec_cb(void * var, int32_t val);
static void scroll_anim_ready_cb(lv_anim_t * anim);
static void calc_render_param(lv_obj_t * cont);
static int32_t calculate_total_height(lv_listmenu_t * menu);
static int32_t calculate_loop_cycle_height(const lv_obj_t * obj);
static bool listmenu_can_scroll(lv_obj_t * obj, lv_listmenu_t * menu);
static void update_img_size_with_scale(lv_obj_t * img);
static void apply_btn_layout_mode(lv_obj_t * btn, lv_listmenu_item_layout_t layout);
static lv_obj_t * get_parent_menu_from_btn(lv_obj_t * btn);
static void update_snap_candidate(int32_t candidate_offset, int32_t current_offset, int8_t direction,
                                  bool * found_nearest, int32_t * nearest_offset, int32_t * nearest_dist,
                                  bool * found_in_direction, int32_t * direction_offset, int32_t * direction_dist);

static int16_t list_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn);
static int16_t arc_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn);
static int16_t gear_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_listmenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_listmenu_constructor,
    .destructor_cb = lv_listmenu_destructor,
    .event_cb = lv_listmenu_event,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_listmenu_t),
};
const lv_obj_class_t lv_listmenu_button_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_listmenu_button_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_listmenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *      MACROS
 **********************/

lv_obj_t * lv_listmenu_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t * lv_listmenu_add_button(lv_obj_t * parent, const void * icon, const char * txt)
{
    LV_ASSERT_OBJ(parent, MY_CLASS);

    lv_obj_t * btn = lv_obj_class_create_obj(&lv_listmenu_button_class, parent);
    lv_obj_class_init_obj(btn);
    lv_obj_set_size(btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_listmenu_button_t * list_menu_btn = (lv_listmenu_button_t *)btn;

    lv_obj_t * img = lv_image_create(btn);
    if(icon) {
        lv_image_set_src(img, icon);
    }
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    list_menu_btn->img = img;

    lv_obj_t * label = lv_label_create(btn);
    if(txt) {
        lv_label_set_text(label, txt);
    }
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    list_menu_btn->label = label;

    apply_btn_layout_mode(btn, ((lv_listmenu_t *)parent)->item_layout);
    calc_render_param(parent);
    update_menu_items(parent);
    return btn;
}

/*=====================
 * Setter functions
 *====================*/

void lv_listmenu_set_style(lv_obj_t * obj, lv_listmenu_style_t style, lv_listmenu_style_param_t * param)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->style = style;

    if(style == LV_LISTMENU_STYLE_LIST) {
        menu->item_transform_cb = list_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
    }
    else if(style == LV_LISTMENU_STYLE_ARC) {
        menu->item_transform_cb = arc_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
        else {
            menu->style_param.arc.radius = lv_display_get_horizontal_resolution(NULL) / 2;
        }
    }
    else if(style == LV_LISTMENU_STYLE_GEAR) {
        menu->item_transform_cb = gear_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
        else {
            menu->style_param.gear.radius = lv_display_get_horizontal_resolution(NULL);
        }
    }

    update_menu_items(obj);
}

void lv_listmenu_set_item_transform_cb(lv_obj_t * obj, lv_listmenu_item_transform_cb_t item_transform_cb,
                                       lv_listmenu_style_param_t * param)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->item_transform_cb = item_transform_cb;
    menu->style = LV_LISTMENU_STYLE_CUSTOM;
    if(param) {
        menu->style_param = *param;
    }

    update_menu_items(obj);
}

void lv_listmenu_set_item_layout(lv_obj_t * obj, lv_listmenu_item_layout_t layout)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    if(layout != LV_LISTMENU_ITEM_LAYOUT_HORIZONTAL && layout != LV_LISTMENU_ITEM_LAYOUT_VERTICAL) {
        return;
    }

    if(menu->item_layout == layout) return;
    menu->item_layout = layout;

    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(child) {
            apply_btn_layout_mode(child, layout);
        }
    }

    calc_render_param(obj);
    update_menu_items(obj);
}
void lv_listmenu_set_scroll_offset(lv_obj_t * obj, int32_t ver_offset, bool anim, uint32_t speed)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    bool fixed_range = !listmenu_can_scroll(obj, menu);

    if(fixed_range) {
        ver_offset = menu->scroll.offset_min;
        anim = false;
    }

    int32_t ver_diff = LV_ABS(ver_offset - menu->scroll.ver_offset);

    if(anim) {
        if(menu->scroll.scroll_tmr) {
            lv_timer_pause(menu->scroll.scroll_tmr);
        }
        menu->scroll.momentum = 0;

        if(!menu->is_loop) {
            int32_t limit = 0;
            ver_offset = CLAMP(ver_offset, menu->scroll.offset_max - limit, menu->scroll.offset_min + limit);
        }

        lv_anim_t * cur_v = lv_anim_get(obj, scroll_anim_exec_cb);

        if(ver_diff >= 1) {
            if(cur_v) {
                cur_v->path_cb = lv_anim_path_linear;
                cur_v->start_value = menu->scroll.ver_offset;
                cur_v->end_value = ver_offset;
                cur_v->duration = lv_anim_speed_to_time(speed, menu->scroll.ver_offset, ver_offset);
                cur_v->act_time = 0;
            }
            else {
                lv_anim_t a;
                lv_anim_init(&a);
                a.var = (void *)obj;
                a.path_cb = lv_anim_path_linear;
                a.start_value = menu->scroll.ver_offset;
                a.end_value = ver_offset;
                a.duration = lv_anim_speed_to_time(speed, menu->scroll.ver_offset, ver_offset);
                a.exec_cb = scroll_anim_exec_cb;
                a.completed_cb = scroll_anim_ready_cb;
                lv_anim_start(&a);
            }
        }
        else if(cur_v) {
            lv_anim_delete(obj, scroll_anim_exec_cb);
        }
    }
    else {
        if(!menu->is_loop) {
            ver_offset = CLAMP(ver_offset, menu->scroll.offset_max, menu->scroll.offset_min);
        }
        menu->scroll.ver_offset = ver_offset;
        update_menu_items(obj);
    }
}

void lv_listmenu_set_scroll_to_center(lv_obj_t * obj, bool anim, uint32_t speed)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    int32_t offset_min = menu->scroll.offset_min - menu->top_margin;
    int32_t offset_max = menu->scroll.offset_max + menu->bottom_margin;
    int32_t offset = (offset_min + offset_max) / 2;
    lv_listmenu_set_scroll_offset(obj, offset, anim, speed);
}
void lv_listmenu_stop_scroll(lv_obj_t * obj, bool snap_to_nearest, uint32_t speed)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    if(menu->scroll.scroll_tmr) {
        lv_timer_pause(menu->scroll.scroll_tmr);
    }
    menu->scroll.momentum = 0;

    if(!snap_to_nearest || !menu->is_snap) return;

    int32_t center_y = lv_obj_get_height(obj) / 2;
    int32_t current_offset = menu->scroll.ver_offset;
    int32_t nearest_offset = current_offset;
    int32_t direction_offset = current_offset;
    int32_t nearest_dist = 0x7fffffff;
    int32_t direction_dist = 0x7fffffff;
    bool found_nearest = false;
    bool found_in_direction = false;

    int8_t direction = 0;
    lv_anim_t * cur_v = lv_anim_get(obj, scroll_anim_exec_cb);
    if(cur_v) {
        if(cur_v->end_value > cur_v->start_value) direction = 1;
        else if(cur_v->end_value < cur_v->start_value) direction = -1;
    }

    int32_t pad_row = lv_obj_get_style_pad_row(obj, LV_PART_MAIN);
    int32_t total_height = calculate_loop_cycle_height(obj);
    bool use_loop_candidate = menu->is_loop && listmenu_can_scroll(obj, menu) && total_height > 0;

    int32_t current_y = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(child == NULL || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

        int32_t item_h = lv_obj_get_height(child);
        int32_t base_target = center_y - (current_y + item_h / 2);
        current_y += item_h + pad_row;

        if(use_loop_candidate) {
            float ratio = (float)(current_offset - base_target) / (float)total_height;
            int32_t n0 = (int32_t)((ratio >= 0.f) ? (ratio + 0.5f) : (ratio - 0.5f));
            for(int32_t n = n0 - 1; n <= n0 + 1; n++) {
                int32_t candidate = base_target + (int32_t)(n * total_height);
                update_snap_candidate(candidate, current_offset, direction,
                                      &found_nearest, &nearest_offset, &nearest_dist,
                                      &found_in_direction, &direction_offset, &direction_dist);
            }
        }
        else {
            update_snap_candidate(base_target, current_offset, direction,
                                  &found_nearest, &nearest_offset, &nearest_dist,
                                  &found_in_direction, &direction_offset, &direction_dist);
        }
    }

    int32_t best_offset = found_in_direction ? direction_offset : (found_nearest ? nearest_offset : current_offset);
    if(!menu->is_loop) {
        best_offset = CLAMP(best_offset, menu->scroll.offset_max, menu->scroll.offset_min);
    }

    lv_listmenu_set_scroll_offset(obj, best_offset, true, speed);
}

void lv_listmenu_set_margin(lv_obj_t * obj, int32_t top_margin, int32_t bottom_margin)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->top_margin = top_margin;
    menu->bottom_margin = bottom_margin;
    calc_render_param(obj);
    update_menu_items(obj);
}

void lv_listmenu_set_snap_enabled(lv_obj_t * obj, bool enabled)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->is_snap = enabled;
}

void lv_listmenu_set_elastic_factor(lv_obj_t * obj, int32_t elastic_factor)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->elastic_factor = elastic_factor;
}

void lv_listmenu_set_out_of_bound_factor(lv_obj_t * obj, int32_t out_of_bound_factor)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->out_of_bound_factor = out_of_bound_factor;
}

void lv_listmenu_set_friction_factor(lv_obj_t * obj, int32_t friction_factor)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->friction_factor = friction_factor;
}

void lv_listmenu_set_snap_factor(lv_obj_t * obj, int32_t snap_factor)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->snap_factor = snap_factor;
}

void lv_listmenu_set_loop(lv_obj_t * obj, bool enabled)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->is_loop = enabled;
    update_menu_items(obj);
}
void lv_listmenu_button_set_hide(lv_obj_t * btn, bool hide)
{
    lv_obj_t * parent = get_parent_menu_from_btn(btn);

    if(hide) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }

    calc_render_param(parent);
    update_menu_items(parent);
}
void lv_listmenu_button_set_image_zoom(lv_obj_t * btn, uint16_t zoom)
{
    lv_obj_t * parent = get_parent_menu_from_btn(btn);

    lv_listmenu_button_t * btn_data = (lv_listmenu_button_t *)btn;
    if(btn_data->img) {
        lv_image_set_scale(btn_data->img, zoom);
        update_img_size_with_scale(btn_data->img);
    }

    calc_render_param(parent);
    update_menu_items(parent);
}
void lv_listmenu_button_set_image(lv_obj_t * btn, const void * icon)
{
    lv_obj_t * parent = get_parent_menu_from_btn(btn);

    lv_listmenu_button_t * btn_data = (lv_listmenu_button_t *)btn;
    if(btn_data->img) {
        lv_image_set_src(btn_data->img, icon);
        update_img_size_with_scale(btn_data->img);
    }

    calc_render_param(parent);
    update_menu_items(parent);
}
void lv_listmenu_button_set_text(lv_obj_t * btn, const char * text)
{
    lv_obj_t * parent = get_parent_menu_from_btn(btn);

    lv_listmenu_button_t * btn_data = (lv_listmenu_button_t *)btn;
    if(btn_data->label) {
        lv_label_set_text(btn_data->label, text);
    }

    calc_render_param(parent);
    update_menu_items(parent);
}

/*=====================
 * Getter functions
 *====================*/

lv_listmenu_style_t lv_listmenu_get_style(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->style;
}

lv_listmenu_style_param_t * lv_listmenu_get_style_param(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return &menu->style_param;
}

lv_listmenu_item_layout_t lv_listmenu_get_item_layout(lv_obj_t * obj)
{
    return ((lv_listmenu_t *)obj)->item_layout;
}

lv_obj_t * lv_listmenu_get_center_btn(lv_obj_t * obj)
{
    // 获取中心位置的按钮
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    int32_t center_y = lv_obj_get_height(obj) / 2;
    int32_t closest_dist = LV_COORD_MAX;
    lv_obj_t * selected_btn = NULL;

    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

        int32_t child_y = lv_obj_get_y(child) + lv_obj_get_height(child) / 2;
        int32_t dist = LV_ABS(child_y - center_y);

        if(dist < closest_dist) {
            closest_dist = dist;
            selected_btn = child;
        }
    }

    return selected_btn;
}

int32_t lv_listmenu_get_scroll_offset(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->scroll.ver_offset;
}

int32_t lv_listmenu_get_scroll_to_center(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    int32_t offset_min = menu->scroll.offset_min - menu->top_margin;
    int32_t offset_max = menu->scroll.offset_max + menu->bottom_margin;
    int32_t offset = (offset_min + offset_max) / 2;
    return offset;
}

int32_t lv_listmenu_get_top_margin(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->top_margin;
}

int32_t lv_listmenu_get_bottom_margin(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->bottom_margin;
}

bool lv_listmenu_is_snap_enabled(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->is_snap;
}

int32_t lv_listmenu_get_friction_factor(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->friction_factor;
}

int32_t lv_listmenu_get_snap_factor(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->snap_factor;
}

int32_t lv_listmenu_get_elastic_factor(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->elastic_factor;
}

int32_t lv_listmenu_get_out_of_bound_factor(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->out_of_bound_factor;
}

bool lv_listmenu_is_loop(lv_obj_t * obj)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    return menu->is_loop;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_listmenu_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_listmenu_t * menu = (lv_listmenu_t *)obj;

    menu->style = LV_LISTMENU_STYLE_LIST;
    menu->item_layout = LV_LISTMENU_ITEM_LAYOUT_HORIZONTAL;
    menu->_is_init = false;
    menu->is_snap = true;
    menu->item_transform_cb = list_item_transform_cb;
    menu->style_param.arc.radius = 0;
    menu->friction_factor = 80;
    menu->snap_factor = 40;
    menu->elastic_factor = 15;
    menu->out_of_bound_factor = 60;
    menu->is_loop = false;

    menu->_cached_total_height = 0;
    menu->_need_recalc_total_height = true;

    menu->top_margin = 0;
    menu->bottom_margin = 0;
    menu->scroll.ver_offset = 0;
    menu->scroll.offset_min = 0;
    menu->scroll.offset_max = 0;
    menu->scroll.momentum = 0;

    menu->scroll.scroll_tmr = lv_timer_create(scroll_timer_cb, LV_DEF_REFR_PERIOD, obj);
    lv_timer_pause(menu->scroll.scroll_tmr);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_listmenu_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_listmenu_t * menu = (lv_listmenu_t *)obj;

    // 删除定时器
    if(menu->scroll.scroll_tmr) {
        lv_timer_delete(menu->scroll.scroll_tmr);
        menu->scroll.scroll_tmr = NULL;
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_listmenu_button_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_listmenu_button_t * btn = (lv_listmenu_button_t *)obj;
    btn->img = NULL;
    btn->label = NULL;
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}
static void lv_listmenu_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;

    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        lv_anim_delete(obj, scroll_anim_exec_cb);
        lv_timer_pause(menu->scroll.scroll_tmr);
        menu->scroll.momentum = 0;
        menu->scroll.is_scroll = false;
    }
    else if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        int32_t diff = indev->pointer.vect.y;

        // Boundary slowdown
        if(!menu->is_loop && !SCROLL_IN_RANGE(menu->scroll.ver_offset, menu->scroll.offset_min, menu->scroll.offset_max)) {
            diff = diff * menu->out_of_bound_factor / 100.f;
        }

        menu->scroll.ver_offset += diff;

        if(!menu->scroll.is_scroll) {
            if(LV_ABS(indev->pointer.vect.y) > LV_ABS(indev->pointer.vect.x)) {
                menu->scroll.momentum += diff;
                if(LV_ABS(menu->scroll.momentum) > SCROLL_THRESHOLD) {
                    menu->scroll.is_scroll = true;
                }
            }
        }

        update_menu_items(obj);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if(menu->scroll.is_scroll) {
            lv_indev_t * indev = lv_event_get_indev(e);
            menu->scroll.momentum = indev->pointer.vect.y;
            lv_timer_resume(menu->scroll.scroll_tmr);
        }
    }
    else if(code == LV_EVENT_SIZE_CHANGED) {
        if(!menu->_is_init && lv_obj_get_width(obj) != 0 && lv_obj_get_height(obj) != 0) {
            menu->_is_init = true;
            calc_render_param(obj);
            update_menu_items(obj);
        }
    }
    else if(code == LV_EVENT_STYLE_CHANGED) {
        if(menu->_is_init) {
            calc_render_param(obj);
            update_menu_items(obj);
        }
    }
    else if(code == LV_EVENT_CHILD_DELETED) {
        menu->_need_recalc_total_height = true;
        calculate_total_height(menu);
    }
}
static void update_menu_items(lv_obj_t * cont)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)cont;
    if(!lv_obj_is_valid(cont) || !menu->_is_init) return;

    int32_t cont_height = lv_obj_get_height(cont);
    int32_t center_y = cont_height / 2;
    int32_t padding_row = lv_obj_get_style_pad_row(cont, LV_PART_MAIN);
    bool can_scroll = listmenu_can_scroll(cont, menu);
    int32_t total_height = calculate_loop_cycle_height(cont);

    uint32_t child_cnt = lv_obj_get_child_count(cont);
    if(child_cnt == 0) return;

    int32_t y_ofs = menu->scroll.ver_offset;

    if(!can_scroll) {
        y_ofs = menu->scroll.offset_min;
        menu->scroll.ver_offset = y_ofs;
    }

    if(menu->is_loop && can_scroll && total_height > 0) {
        y_ofs = y_ofs % total_height;
        if(y_ofs < 0) y_ofs += total_height;
    }

    int32_t current_y = 0;

    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        if(child == NULL || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

        int32_t item_height = lv_obj_get_height(child);
        lv_point_t item_pos;
        item_pos.x = 0;

        if(menu->is_loop && can_scroll && total_height > 0) {
            int32_t item_center_y_natural = current_y + item_height / 2;
            int32_t dist = item_center_y_natural + y_ofs - center_y;

            while(dist > total_height / 2) dist -= total_height;
            while(dist < -total_height / 2) dist += total_height;

            item_pos.y = center_y + dist;
            current_y += item_height + padding_row;
        }
        else {
            item_pos.y = menu->scroll.ver_offset + current_y + item_height / 2;
            current_y += item_height + padding_row;
        }

        bool is_visible = true;
        int16_t zoom = LV_SCALE_NONE;
        if(menu->item_transform_cb && is_visible) {
            zoom = menu->item_transform_cb(&item_pos, cont, child);
            if(zoom < 10) is_visible = false;
        }

        if(is_visible) {
            lv_obj_set_pos(child, item_pos.x, item_pos.y - item_height / 2);
            lv_obj_set_style_transform_scale(child, zoom, 0);
        }
    }

    lv_obj_invalidate(cont);
}
static void scroll_timer_cb(lv_timer_t * p_timer)
{
    lv_obj_t * obj = (lv_obj_t *)p_timer->user_data;
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;

    if(!listmenu_can_scroll(obj, menu)) {
        menu->scroll.ver_offset = menu->scroll.offset_min;
        menu->scroll.momentum = 0;
        menu->scroll.is_scroll = false;
        lv_timer_pause(p_timer);
        update_menu_items(obj);
        return;
    }

    float diff = menu->scroll.momentum;
    float offset = menu->scroll.ver_offset;
    float min_offset = menu->scroll.offset_min;
    float max_offset = menu->scroll.offset_max;
    bool scroll_done = true;

    diff = diff * menu->friction_factor / 100.f;

    // Check out of bounds
    if(!menu->is_loop && !SCROLL_IN_RANGE(offset, min_offset, max_offset)) {
        // Out of bounds, apply elastic effect
        scroll_done = false;
        diff = diff * menu->out_of_bound_factor / 100.f;
        offset += diff;

        float force = LV_MAX(offset - min_offset, max_offset - offset);
        if(force > 1.f) {
            offset += copysignf(force * menu->elastic_factor / 100.f, min_offset - offset) + copysignf(1.f, min_offset - offset);
        }
        else {
            offset = offset > min_offset ? min_offset : max_offset;
            scroll_done = true;
        }
    }
    else {
        // Normal range or loop mode
        offset += diff;

        if(menu->is_loop) {
            int32_t total_height = calculate_loop_cycle_height(obj);

            if(total_height > 0) {
                int32_t center_y = lv_obj_get_height(obj) / 2;
                int32_t start_range = center_y - total_height;

                while(offset > center_y) offset -= total_height;
                while(offset <= start_range) offset += total_height;
            }
        }

        if(menu->is_snap) {
            int32_t center_y = lv_obj_get_height(obj) / 2;
            int32_t current_offset = (int32_t)offset;
            int32_t nearest_offset = current_offset;
            int32_t nearest_dist = 0x7fffffff;
            bool found_nearest = false;
            bool found_in_direction = false;
            int32_t direction_offset = current_offset;
            int32_t direction_dist = 0x7fffffff;

            uint32_t child_cnt = lv_obj_get_child_count(obj);
            int32_t padding_row = lv_obj_get_style_pad_row(obj, LV_PART_MAIN);
            int32_t loop_cycle_h = calculate_loop_cycle_height(obj);
            bool use_loop_candidate = menu->is_loop && listmenu_can_scroll(obj, menu) && loop_cycle_h > 0;

            int32_t current_y = 0;
            for(uint32_t i = 0; i < child_cnt; i++) {
                lv_obj_t * child = lv_obj_get_child(obj, i);
                if(child == NULL || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;

                int32_t item_h = lv_obj_get_height(child);
                int32_t base_target = center_y - (current_y + item_h / 2);
                current_y += item_h + padding_row;

                if(use_loop_candidate) {
                    float ratio = (float)(current_offset - base_target) / (float)loop_cycle_h;
                    int32_t n0 = (int32_t)((ratio >= 0.f) ? (ratio + 0.5f) : (ratio - 0.5f));
                    for(int32_t n = n0 - 1; n <= n0 + 1; n++) {
                        int32_t candidate = base_target + (int32_t)(n * loop_cycle_h);
                        update_snap_candidate(candidate, current_offset, 0,
                                              &found_nearest, &nearest_offset, &nearest_dist,
                                              &found_in_direction, &direction_offset, &direction_dist);
                    }
                }
                else {
                    update_snap_candidate(base_target, current_offset, 0,
                                          &found_nearest, &nearest_offset, &nearest_dist,
                                          &found_in_direction, &direction_offset, &direction_dist);
                }
            }

            float snap_offset = found_nearest ? (float)nearest_offset : offset;

            if(!menu->is_loop) {
                snap_offset = CLAMP(snap_offset, max_offset, min_offset);
            }

            float force = offset - snap_offset;
            bool overshot = (diff > 0 && force > 0) || (diff < 0 && force < 0);
            if(overshot) {
                offset = snap_offset;
                diff = 0;
                scroll_done = true;
                force = 0;
            }

            if(fabsf(diff) < fabsf(force)) {
                force = force * menu->snap_factor / 100.f;

                if(fabsf(force) > 1.f) {
                    offset -= force;
                    scroll_done = false;
                }
                else {
                    offset = snap_offset;
                    diff = 0;
                    scroll_done = true;
                }
            }
        }
    }

    if(LV_ABS(diff) < (menu->snap_factor / 100.f) && scroll_done) {
        diff = 0;
        lv_timer_pause(p_timer);
    }

    menu->scroll.ver_offset = (int32_t)offset;
    menu->scroll.momentum = (int32_t)diff;

    update_menu_items(obj);
}

static void scroll_anim_exec_cb(void * var, int32_t val)
{
    lv_obj_t * obj = (lv_obj_t *)var;
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    menu->scroll.ver_offset = val;
    update_menu_items(obj);
}

static void scroll_anim_ready_cb(lv_anim_t * anim)
{
    lv_obj_t * obj = anim->var;
    lv_listmenu_t * menu = (lv_listmenu_t *)obj;
    lv_timer_resume(menu->scroll.scroll_tmr);
}
static void calc_render_param(lv_obj_t * cont)
{
    lv_listmenu_t * menu = (lv_listmenu_t *)cont;
    int32_t old_offset_min = menu->scroll.offset_min;
    int32_t old_offset_max = menu->scroll.offset_max;
    int32_t old_ver_offset = menu->scroll.ver_offset;

    lv_obj_update_layout(cont);

    menu->_need_recalc_total_height = true;

    int32_t total_height = calculate_total_height(menu);
    int32_t cont_height = lv_obj_get_height(cont);

    if(total_height <= 0) {
        menu->scroll.offset_min = 0;
        menu->scroll.offset_max = 0;
    }
    else if(total_height <= cont_height) {
        // Content fits inside container
        menu->scroll.offset_min = (cont_height - total_height) / 2;
        menu->scroll.offset_max = menu->scroll.offset_min;
    }
    else {
        // Content exceeds container height
        menu->scroll.offset_min = -1 +
                                  menu->top_margin; // Keep -1 to avoid a scroll_timer_cb endless loop when top offset is 0.
        menu->scroll.offset_max = cont_height - total_height - menu->bottom_margin;
    }

    if(menu->scroll.offset_max > menu->scroll.offset_min) {
        menu->scroll.offset_max = menu->scroll.offset_min;
    }

    bool was_fit_centered = (old_offset_min == old_offset_max) && (old_offset_min > 0) && (old_ver_offset == old_offset_min);
    bool now_scrollable = (total_height > cont_height) && (menu->scroll.offset_max < menu->scroll.offset_min);
    if(!menu->is_loop && was_fit_centered && now_scrollable) {
        menu->scroll.ver_offset = 0;
    }

    // Keep current offset in range
    if(!menu->is_loop) {
        menu->scroll.ver_offset = CLAMP(menu->scroll.ver_offset, menu->scroll.offset_max, menu->scroll.offset_min);
    }
}


static int32_t calculate_total_height(lv_listmenu_t * menu)
{
    if(!menu->_need_recalc_total_height && menu->_cached_total_height > 0) {
        return menu->_cached_total_height;
    }

    lv_obj_t * obj = (lv_obj_t *)menu;
    int32_t total_height = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    uint32_t disp_child_cnt = 0;

    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(!lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
            total_height += lv_obj_get_height(child);
            disp_child_cnt++;
        }
    }

    int32_t padding_row = lv_obj_get_style_pad_row(obj, LV_PART_MAIN);
    if(padding_row > 0 && disp_child_cnt > 1) {
        total_height += padding_row * (disp_child_cnt - 1);
    }

    menu->_cached_total_height = total_height;
    menu->_need_recalc_total_height = false;

    return total_height;
}

static int32_t calculate_loop_cycle_height(const lv_obj_t * obj)
{
    if(obj == NULL) {
        return 0;
    }

    int32_t total_height = 0;
    uint32_t disp_child_cnt = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);

    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(child == NULL || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        total_height += lv_obj_get_height(child);
        disp_child_cnt++;
    }

    int32_t pad_row = lv_obj_get_style_pad_row(obj, LV_PART_MAIN);
    if(pad_row > 0 && disp_child_cnt > 1) {
        total_height += pad_row * disp_child_cnt;
    }

    return total_height;
}

static bool listmenu_can_scroll(lv_obj_t * obj, lv_listmenu_t * menu)
{
    if(obj == NULL || menu == NULL) {
        return false;
    }

    int32_t total_height = 0;
    uint32_t disp_child_cnt = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(child == NULL || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        total_height += lv_obj_get_height(child);
        disp_child_cnt++;
    }

    int32_t pad_row = lv_obj_get_style_pad_row(obj, LV_PART_MAIN);
    if(pad_row > 0 && disp_child_cnt > 1) {
        total_height += pad_row * (disp_child_cnt - 1);
    }

    return total_height > lv_obj_get_height(obj);
}

static lv_obj_t * get_parent_menu_from_btn(lv_obj_t * btn)
{
    LV_ASSERT_OBJ(btn, &lv_listmenu_button_class);

    lv_obj_t * parent = lv_obj_get_parent(btn);
    LV_ASSERT_OBJ(parent, MY_CLASS);

    return parent;
}

static void update_snap_candidate(int32_t candidate_offset, int32_t current_offset, int8_t direction,
                                  bool * found_nearest, int32_t * nearest_offset, int32_t * nearest_dist,
                                  bool * found_in_direction, int32_t * direction_offset, int32_t * direction_dist)
{
    int32_t dist = LV_ABS((int32_t)candidate_offset - (int32_t)current_offset);
    if(!(*found_nearest) || dist < *nearest_dist) {
        *found_nearest = true;
        *nearest_dist = dist;
        *nearest_offset = candidate_offset;
    }

    if(direction == 0) {
        return;
    }

    int32_t delta = (int32_t)candidate_offset - (int32_t)current_offset;
    bool in_direction = (direction > 0) ? (delta >= 0) : (delta <= 0);
    if(in_direction) {
        if(!(*found_in_direction) || dist < *direction_dist) {
            *found_in_direction = true;
            *direction_dist = dist;
            *direction_offset = candidate_offset;
        }
    }
}

static void update_img_size_with_scale(lv_obj_t * img)
{
    int32_t src_w = lv_image_get_src_width(img);
    int32_t src_h = lv_image_get_src_height(img);
    int32_t applied_zoom = lv_image_get_scale(img);

    if(src_w <= 0 || src_h <= 0) {
        return;
    }

    if(applied_zoom <= 0) {
        applied_zoom = LV_SCALE_NONE;
    }

    int32_t scaled_w = (int32_t)(((int64_t)src_w * applied_zoom + LV_SCALE_NONE / 2) / LV_SCALE_NONE);
    int32_t scaled_h = (int32_t)(((int64_t)src_h * applied_zoom + LV_SCALE_NONE / 2) / LV_SCALE_NONE);

    if(scaled_w <= 0) scaled_w = 1;
    if(scaled_h <= 0) scaled_h = 1;

    lv_obj_set_size(img, scaled_w, scaled_h);
}
static void apply_btn_layout_mode(lv_obj_t * btn, lv_listmenu_item_layout_t layout)
{
    if(!lv_obj_check_type(btn, &lv_listmenu_button_class)) {
        return;
    }

    lv_listmenu_button_t * btn_data = (lv_listmenu_button_t *)btn;

    lv_flex_flow_t flow = (layout == LV_LISTMENU_ITEM_LAYOUT_VERTICAL) ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW;
    lv_obj_set_flex_flow(btn, flow);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if(btn_data->label) {
        lv_obj_set_flex_grow(btn_data->label, (layout == LV_LISTMENU_ITEM_LAYOUT_HORIZONTAL) ? 1 : 0);
    }
}


static int16_t list_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn)
{
    return LV_SCALE_NONE;
}

static int16_t arc_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn)
{
    lv_listmenu_t * listmenu = (lv_listmenu_t *)menu;
    int32_t width = lv_obj_get_width(menu);
    int32_t height = lv_obj_get_height(menu);

    int32_t y_center = icon_pos->y;
    int32_t y_diff = y_center - height / 2;
    int32_t radius = listmenu->style_param.arc.radius;
    y_diff = LV_ABS(y_diff);

    if(y_diff >= radius) {
        icon_pos->x = radius;
    }
    else {
        uint32_t x_sqr = radius * radius - y_diff * y_diff;
        lv_sqrt_res_t res;
        lv_sqrt(x_sqr, &res, 0x8000);
        icon_pos->x = radius - res.i;
    }

    return LV_SCALE_NONE;
}
static int16_t gear_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * menu, const lv_obj_t * btn)
{
    lv_listmenu_t * listmenu = (lv_listmenu_t *)menu;
    int32_t height = lv_obj_get_height(menu);

    int32_t y_center = icon_pos->y;
    int32_t y_diff = y_center - height / 2;
    float radius = listmenu->style_param.gear.radius;

    if(LV_ABS(y_diff) > height || radius <= 0) {
        return LV_SCALE_NONE;
    }

    float sin_value = height / radius / 2.0f;
    float angle_rad = asinf(sin_value);
    float angle = angle_rad * 2.0f * 180.0f / M_PI;

    float angle_diff = (y_diff / (float)height) * angle;

    float angle_sin = sinf(angle_diff * M_PI / 180.0f);
    float angle_cos = cosf(angle_diff * M_PI / 180.0f);

    icon_pos->x = (int32_t)(radius - radius * angle_cos);
    icon_pos->y = (int32_t)(height / 2 + radius * angle_sin);

    return LV_SCALE_NONE;
}

#endif /*LV_USE_LISTMENU*/
