/**
 * @file lv_gridmenu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_gridmenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/display/lv_display_private.h"
#include "../../../../../src/indev/lv_indev_private.h"
#include "math.h"

#if LV_USE_GRIDMENU != 0

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
    #error "lv_gridmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_gridmenu_class

#define SCROLL_IN_RANGE(val, bound_a, bound_b) \
    ((val) >= LV_MIN((bound_a), (bound_b)) && (val) <= LV_MAX((bound_a), (bound_b)))
#define CLAMP(x, min, max)             (x < min ? min : (x > max ? max : x))
#define SQUARE(x)                       ((x) * (x))
#define PYTHAG(x, y)                    sqrtf(SQUARE(x) + SQUARE(y))
#define PYTHAG3D(x, y, z)               sqrtf(SQUARE(x) + SQUARE(y) + SQUARE(z))
#define CALC_AND_UPDATE_MENU_ITEMS(cont) \
    calc_render_param(cont); \
    update_menu_items(cont);

#define MENU_RENDER_MARGIN_HOR    50
#define MENU_RENDER_MARGIN_VER    50

#define DEFAULT_PADDING_HOR lv_display_get_physical_hor_res(NULL) / 2 - 50
#define DEFAULT_PADDING_VER lv_display_get_physical_ver_res(NULL) / 2 - 50

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_gridmenu_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_gridmenu_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_gridmenu_button_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_gridmenu_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void update_menu_items(lv_obj_t * cont);
static void scroll_timer_cb(lv_timer_t * p_timer);
static void scroll_hor_anim_exec_cb(void * var, int32_t val);
static void scroll_ver_anim_exec_cb(void * var, int32_t val);
static void scroll_anim_ready_cb(lv_anim_t * anim);
static uint32_t get_disp_child_cnt(const lv_obj_t * obj);
static lv_obj_t * get_disp_item(lv_obj_t * cont, int32_t * idx);
static void calc_render_param(lv_obj_t * cont);
static void get_item_size(lv_obj_t * cont, int32_t * width, int32_t * height);
static int32_t calculate_total_height(lv_gridmenu_t * menu);

static int16_t grid_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn);
static int16_t curve_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn);
static int16_t star_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_gridmenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_gridmenu_constructor,
    .destructor_cb = lv_gridmenu_destructor,
    .event_cb = lv_gridmenu_event,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_gridmenu_t),
};
const lv_obj_class_t lv_gridmenu_button_class = {
    .base_class = &lv_image_class,
    .constructor_cb = lv_gridmenu_button_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_gridmenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_gridmenu_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t * lv_gridmenu_add_button(lv_obj_t * parent, const void * icon)
{
    lv_obj_t * btn = lv_obj_class_create_obj(&lv_gridmenu_button_class, parent);
    lv_obj_class_init_obj(btn);
    lv_image_set_src(btn, icon);

    CALC_AND_UPDATE_MENU_ITEMS(parent);
    return btn;
}

/*=====================
 * Setter functions
 *====================*/

void lv_gridmenu_set_item_per_row(lv_obj_t * obj, uint8_t item_per_row)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->item_per_row = item_per_row;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_style(lv_obj_t * obj, lv_gridmenu_style_t style, lv_gridmenu_style_param_t * param)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->style = style;
    if(style == LV_GRIDMENU_STYLE_STAR) {
        menu->item_transform_cb = star_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
        else {
            menu->style_param.star.radius = 500.f;
            menu->style_param.star.distortion_coeff = 1.3f;
        }
    }
    else if(style == LV_GRIDMENU_STYLE_CURVE) {
        menu->item_transform_cb = curve_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
        else {
            menu->style_param.curve.radius = 200;
        }
    }
    else {
        menu->item_transform_cb = grid_item_transform_cb;
        if(param) {
            menu->style_param = *param;
        }
    }
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_item_transform_cb(lv_obj_t * obj, lv_gridmenu_item_transform_cb_t item_transform_cb,
                                       lv_gridmenu_style_param_t * param)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->item_transform_cb = item_transform_cb;
    if(param) {
        menu->style_param = *param;
    }
    menu->style = LV_GRIDMENU_STYLE_CUSTOM;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_hex_layout(lv_obj_t * obj, bool hex_layout)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->hex_layout = hex_layout;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->scroll.scroll_dir = dir;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_scroll_offset(lv_obj_t * obj, lv_point_t offset, bool anim, uint32_t speed)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;

    int32_t hor_diff = LV_ABS(offset.x - menu->scroll.hor_offset);
    int32_t ver_diff = LV_ABS(offset.y - menu->scroll.ver_offset);
    if(speed == 0) speed = 750;

    if(anim) {
        if(menu->scroll.scroll_tmr) {
            lv_timer_pause(menu->scroll.scroll_tmr);
        }
        menu->scroll.hor_momentum = 0;
        menu->scroll.ver_momentum = 0;
        menu->scroll.animating_axis_cnt = 0;

        if(!menu->is_loop) {
            int32_t limit = 0;
            offset.x = CLAMP(offset.x, menu->scroll.hor_offset_max - limit, menu->scroll.hor_offset_min + limit);
            offset.y = CLAMP(offset.y, menu->scroll.ver_offset_max - limit, menu->scroll.ver_offset_min + limit);
        }

        lv_anim_t * cur_h = lv_anim_get(obj, scroll_hor_anim_exec_cb);
        lv_anim_t * cur_v = lv_anim_get(obj, scroll_ver_anim_exec_cb);

        if(hor_diff >= 1) {
            if(cur_h) {
                cur_h->path_cb = lv_anim_path_linear;
                cur_h->start_value = menu->scroll.hor_offset;
                cur_h->end_value = offset.x;
                cur_h->duration = lv_anim_speed_to_time(speed, menu->scroll.hor_offset, offset.x);
                cur_h->act_time = 0;
            }
            else {
                lv_anim_t a_hor;
                lv_anim_init(&a_hor);
                a_hor.var = (void *)obj;
                a_hor.path_cb = lv_anim_path_linear;
                a_hor.start_value = menu->scroll.hor_offset;
                a_hor.end_value = offset.x;
                a_hor.duration = lv_anim_speed_to_time(speed, menu->scroll.hor_offset, offset.x);
                a_hor.exec_cb = scroll_hor_anim_exec_cb;
                a_hor.completed_cb = scroll_anim_ready_cb;
                lv_anim_start(&a_hor);
            }
            menu->scroll.animating_axis_cnt++;
        }
        else if(cur_h) {
            lv_anim_delete(obj, scroll_hor_anim_exec_cb);
        }

        if(ver_diff >= 1) {
            if(cur_v) {
                cur_v->path_cb = lv_anim_path_linear;
                cur_v->start_value = menu->scroll.ver_offset;
                cur_v->end_value = offset.y;
                cur_v->duration = lv_anim_speed_to_time(speed, menu->scroll.ver_offset, offset.y);
                cur_v->act_time = 0;
            }
            else {
                lv_anim_t a_ver;
                lv_anim_init(&a_ver);
                a_ver.var = (void *)obj;
                a_ver.path_cb = lv_anim_path_linear;
                a_ver.start_value = menu->scroll.ver_offset;
                a_ver.end_value = offset.y;
                a_ver.duration = lv_anim_speed_to_time(speed, menu->scroll.ver_offset, offset.y);
                a_ver.exec_cb = scroll_ver_anim_exec_cb;
                a_ver.completed_cb = scroll_anim_ready_cb;
                lv_anim_start(&a_ver);
            }
            menu->scroll.animating_axis_cnt++;
        }
        else if(cur_v) {
            lv_anim_delete(obj, scroll_ver_anim_exec_cb);
        }

        if(menu->scroll.animating_axis_cnt == 0 && menu->scroll.scroll_tmr) {
            lv_timer_resume(menu->scroll.scroll_tmr);
        }
    }
    else {
        menu->scroll.hor_offset = CLAMP(offset.x, menu->scroll.hor_offset_max, menu->scroll.hor_offset_min);
        if(menu->is_loop && (menu->scroll.scroll_dir & LV_DIR_VER)) {
            menu->scroll.ver_offset = offset.y;
        }
        else {
            menu->scroll.ver_offset = CLAMP(offset.y, menu->scroll.ver_offset_max, menu->scroll.ver_offset_min);
        }
        update_menu_items(obj);
    }
}

void lv_gridmenu_set_scroll_to_center(lv_obj_t * obj, bool anim, uint32_t speed)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    int32_t hor_offset_min = menu->scroll.hor_offset_min - menu->left_margin;
    int32_t hor_offset_max = menu->scroll.hor_offset_max + menu->right_margin;
    int32_t ver_offset_min = menu->scroll.ver_offset_min - menu->top_margin;
    int32_t ver_offset_max = menu->scroll.ver_offset_max + menu->bottom_margin;
    lv_point_t offset;
    offset.x = (hor_offset_max + hor_offset_min) / 2;
    offset.y = (ver_offset_max + ver_offset_min) / 2;
    lv_gridmenu_set_scroll_offset(obj, offset, anim, speed);
}

void lv_gridmenu_set_ver_margin(lv_obj_t * obj, int32_t top_margin, int32_t bottom_margin)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->top_margin = top_margin;
    menu->bottom_margin = bottom_margin;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_hor_margin(lv_obj_t * obj, int32_t left_margin, int32_t right_margin)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->left_margin = left_margin;
    menu->right_margin = right_margin;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_set_snap_enable(lv_obj_t * obj, bool enable)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->is_snap = enable;
}

void lv_gridmenu_set_elastic_factor(lv_obj_t * obj, int32_t elastic_factor)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->elastic_factor = elastic_factor;
}

void lv_gridmenu_set_friction_factor(lv_obj_t * obj, int32_t friction_factor)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->friction_factor = friction_factor;
}

void lv_gridmenu_set_out_of_bound_factor(lv_obj_t * obj, int32_t out_of_bound_factor)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->out_of_bound_factor = out_of_bound_factor;
}

void lv_gridmenu_set_snap_factor(lv_obj_t * obj, int32_t snap_factor)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->snap_factor = snap_factor;
}

void lv_gridmenu_set_loop(lv_obj_t * obj, bool enable)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    menu->is_loop = enable;
    CALC_AND_UPDATE_MENU_ITEMS(obj);
}

void lv_gridmenu_button_set_hide(lv_obj_t * btn, bool hide)
{
    lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)btn;
    gridmenu_btn->is_hide = hide;
    if(hide) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }
    CALC_AND_UPDATE_MENU_ITEMS(lv_obj_get_parent(btn));
}

void lv_gridmenu_button_set_zoom(lv_obj_t * btn, uint16_t zoom)
{
    // lv_image_set_scale(btn, zoom);
    lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)btn;
    gridmenu_btn->zoom = zoom;
    CALC_AND_UPDATE_MENU_ITEMS(lv_obj_get_parent(btn));
}

/*=====================
 * Getter functions
 *====================*/

lv_gridmenu_style_t lv_gridmenu_get_style(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->style;
}

lv_gridmenu_style_param_t * lv_gridmenu_get_style_param(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return &menu->style_param;
}

bool lv_gridmenu_is_hex_layout(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->hex_layout;
}

int32_t lv_gridmenu_get_item_per_row(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->item_per_row;
}

int32_t lv_gridmenu_get_top_margin(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->top_margin;
}

int32_t lv_gridmenu_get_bottom_margin(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->bottom_margin;
}

int32_t lv_gridmenu_get_left_margin(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->left_margin;
}

int32_t lv_gridmenu_get_right_margin(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->right_margin;
}

int32_t lv_gridmenu_get_scroll_hor_offset(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->scroll.hor_offset;
}

int32_t lv_gridmenu_get_scroll_ver_offset(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->scroll.ver_offset;
}

lv_dir_t lv_gridmenu_get_scroll_dir(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->scroll.scroll_dir;
}

bool lv_gridmenu_is_snap_enable(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->is_snap;
}

int32_t lv_gridmenu_get_elastic_factor(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->elastic_factor;
}

int32_t lv_gridmenu_get_friction_factor(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->friction_factor;
}

int32_t lv_gridmenu_get_out_of_bound_factor(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->out_of_bound_factor;
}

int32_t lv_gridmenu_get_snap_factor(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->snap_factor;
}

bool lv_gridmenu_is_loop(lv_obj_t * obj)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    return menu->is_loop;
}

void lv_gridmenu_stop_scroll(lv_obj_t * obj, bool snap_to_nearest, uint32_t speed)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    if(menu->scroll.scroll_tmr) lv_timer_pause(menu->scroll.scroll_tmr);
    menu->scroll.hor_momentum = 0;
    menu->scroll.ver_momentum = 0;

    if(!snap_to_nearest || !menu->is_snap) {
        return;
    }

    lv_point_t offset = {
        .x = menu->scroll.hor_offset,
        .y = menu->scroll.ver_offset,
    };

    int32_t item_w = 0;
    int32_t item_h = 0;
    get_item_size(obj, &item_w, &item_h);

    int32_t pad_row = lv_obj_get_style_pad_row(obj, LV_PART_ITEMS);
    int32_t pad_col = lv_obj_get_style_pad_column(obj, LV_PART_ITEMS);

    if(menu->scroll.scroll_dir & LV_DIR_VER) {
        int32_t center_y = lv_obj_get_height(obj) / 2;
        int32_t row_h = menu->hex_layout ? (int32_t)(item_h - 0.14f * item_w + pad_row)
                          : (int32_t)(item_h + pad_row);
        if(row_h > 0) {
            float k_cur_f = ((float)center_y - (float)offset.y - (float)(item_h / 2)) / (float)row_h;
            int32_t k_tar = (int32_t)((k_cur_f >= 0.f) ? (k_cur_f + 0.5f) : (k_cur_f - 0.5f));
            offset.y = center_y - (k_tar * row_h + item_h / 2);
            if(!menu->is_loop) {
                offset.y = CLAMP(offset.y, menu->scroll.ver_offset_max, menu->scroll.ver_offset_min);
            }
        }
    }

    if(menu->scroll.scroll_dir & LV_DIR_HOR) {
        int32_t center_x = lv_obj_get_width(obj) / 2;
        int32_t col_w = item_w + pad_col;
        if(col_w > 0) {
            float k_cur_f = ((float)center_x - (float)offset.x - (float)(item_w / 2)) / (float)col_w;
            int32_t k_tar = (int32_t)((k_cur_f >= 0.f) ? (k_cur_f + 0.5f) : (k_cur_f - 0.5f));
            offset.x = center_x - (k_tar * col_w + item_w / 2);
            if(!menu->is_loop) {
                offset.x = CLAMP(offset.x, menu->scroll.hor_offset_max, menu->scroll.hor_offset_min);
            }
        }
    }

    lv_gridmenu_set_scroll_offset(obj, offset, true, speed);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_gridmenu_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;

    menu->friction_factor = 80;
    menu->snap_factor = 40;
    menu->elastic_factor = 15;
    menu->out_of_bound_factor = 60;

    menu->top_margin = 0;
    menu->bottom_margin = 0;
    menu->left_margin = 0;
    menu->right_margin = 0;

    menu->item_per_row = 3;
    menu->style = LV_GRIDMENU_STYLE_GRID;
    menu->hex_layout = false;
    menu->is_loop = false;
    menu->_is_init = false;
    menu->is_snap = true;
    menu->_cached_total_height = 0;
    menu->_need_recalc_total_height = true;
    menu->scroll.animating_axis_cnt = 0;
    menu->item_transform_cb = grid_item_transform_cb;
    menu->style_param.curve.radius = 200;
    menu->scroll.scroll_dir = LV_DIR_VER;
    menu->scroll.scroll_tmr = lv_timer_create(scroll_timer_cb, LV_DEF_REFR_PERIOD, obj);
    lv_timer_pause(menu->scroll.scroll_tmr);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_gridmenu_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    if(menu->scroll.scroll_tmr) {
        lv_timer_delete(menu->scroll.scroll_tmr);
    }

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_gridmenu_button_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_gridmenu_button_t * btn = (lv_gridmenu_button_t *)obj;
    btn->is_hide = false;
    btn->zoom = LV_SCALE_NONE;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_gridmenu_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;


    uint32_t code = lv_event_get_code(e);
    lv_obj_t * cont = lv_event_get_current_target(e);
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;

    if(code == LV_EVENT_PRESSED) {
        lv_timer_pause(menu->scroll.scroll_tmr);
        menu->scroll.hor_momentum = 0;
        menu->scroll.ver_momentum = 0;
    }
    else if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_point_t diff = indev->pointer.vect;
        int32_t next_x = menu->scroll.hor_offset + diff.x;
        int32_t next_y = menu->scroll.ver_offset + diff.y;
        if(!SCROLL_IN_RANGE(next_x, menu->scroll.hor_offset_max, menu->scroll.hor_offset_min)) {
            diff.x *= menu->out_of_bound_factor / 100.f;
        }
        if(!SCROLL_IN_RANGE(next_y, menu->scroll.ver_offset_max, menu->scroll.ver_offset_min)) {
            diff.y *= menu->out_of_bound_factor / 100.f;
        }

        if(menu->scroll.scroll_dir & LV_DIR_HOR) {
            menu->scroll.hor_offset += diff.x;
        }
        if(menu->scroll.scroll_dir & LV_DIR_VER) {
            menu->scroll.ver_offset += diff.y;
        }

        update_menu_items(cont);
    }
    else if(code == LV_EVENT_RELEASED) {
        lv_indev_t * indev = lv_event_get_indev(e);
        menu->scroll.hor_momentum = indev->pointer.vect.x;
        menu->scroll.ver_momentum = indev->pointer.vect.y;
        lv_timer_resume(menu->scroll.scroll_tmr);
    }
    else if(code == LV_EVENT_SIZE_CHANGED) {
        if(lv_obj_get_width(cont) != 0 && lv_obj_get_height(cont) != 0) {
            if(!menu->_is_init) {
                menu->_is_init = true;
            }
            CALC_AND_UPDATE_MENU_ITEMS(cont);
        }
    }
    else if(code == LV_EVENT_STYLE_CHANGED) {
        if(menu->_is_init) {
            CALC_AND_UPDATE_MENU_ITEMS(cont);
        }
    }
    else if(code == LV_EVENT_CHILD_DELETED) {
        menu->_need_recalc_total_height = true;
        calculate_total_height(menu);
    }
}

static void update_menu_items(lv_obj_t * cont)
{
    if(!lv_obj_is_valid(cont) || lv_obj_get_width(cont) == 0 || lv_obj_get_height(cont) == 0) {
        return;
    }

    int32_t child_cnt = get_disp_child_cnt(cont);
    if(child_cnt == 0) {
        return;
    }

    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;
    int32_t item_per_row = menu->item_per_row;
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);

    int32_t x_ofs = menu->scroll.hor_offset;
    int32_t y_ofs = menu->scroll.ver_offset;
    uint32_t draw_count = 0;
    int32_t item_width, item_height;
    get_item_size(cont, &item_width, &item_height);

    int32_t pad_row = lv_obj_get_style_pad_row(cont, LV_PART_ITEMS);
    int32_t pad_column = lv_obj_get_style_pad_column(cont, LV_PART_ITEMS);

    int32_t cont_w = lv_obj_get_width(cont);
    int32_t cont_h = lv_obj_get_height(cont);
    int32_t center_x = cont_w / 2;
    int32_t center_y = cont_h / 2;
    int32_t total_width = 0;
    int32_t total_height = 0;
    bool use_loop_hor = false;
    bool use_loop_ver = false;
    int32_t x_ofs_local = x_ofs;
    int32_t y_ofs_local = y_ofs;

    if((menu->scroll.scroll_dir & LV_DIR_HOR) &&
       !(menu->scroll.scroll_dir & LV_DIR_VER) &&
       menu->is_loop && !menu->hex_layout) {
        uint16_t total_row = (child_cnt + item_per_row - 1) / item_per_row;
        if(total_row == 1) {
            uint16_t total_col = child_cnt;
            int32_t col_width = item_width + pad_column;
            total_width = total_col * col_width;

            if(total_width > cont_w) {
                use_loop_hor = true;
                if(total_width != 0) {
                    x_ofs_local = x_ofs_local % total_width;
                    if(x_ofs_local < 0) x_ofs_local += total_width;
                }
            }
        }
    }

    if((menu->scroll.scroll_dir & LV_DIR_VER) && menu->is_loop) {
        total_height = calculate_total_height(menu);
        if(total_height > cont_h) {
            use_loop_ver = true;
            if(total_height != 0) {
                y_ofs_local = y_ofs_local % total_height;
                if(y_ofs_local < 0) y_ofs_local += total_height;
            }
        }
    }

    int32_t next_real_idx = 0;
    uint32_t visible_idx = 0;
    int32_t invisible_real_idx = -1;

    while(visible_idx < (uint32_t)child_cnt) {
        lv_obj_t * child = get_disp_item(cont, &next_real_idx);
        if(child == NULL) break;

        lv_gridmenu_button_t * btn = (lv_gridmenu_button_t *)child;
        int32_t real_idx = next_real_idx;
        next_real_idx = real_idx + 1;

        lv_point_t item_pos;
        bool isHide = false;
        int32_t item_x_natural = 0;
        int32_t item_y_natural = 0;
        if(menu->hex_layout) {
            uint16_t group_size = item_per_row * 2 - 1;
            uint16_t group_idx = visible_idx / group_size;
            uint16_t group_remain = visible_idx % group_size;

            uint16_t yy = group_idx * 2;
            uint16_t xx = group_remain;

            if(group_remain >= item_per_row) {
                yy++;
                xx -= item_per_row;
                item_x_natural = xx * (item_width + pad_column) + (item_width + pad_column) / 2;
            }
            else {
                item_x_natural = xx * (item_width + pad_column);
            }

            item_y_natural = yy * (item_height - 0.14f * item_width) + yy * pad_row;
        }
        else {
            int32_t row = visible_idx / item_per_row;
            int32_t col = visible_idx % item_per_row;
            item_x_natural = col * item_width + col * pad_column;
            item_y_natural = row * item_height + row * pad_row;
        }

        if(use_loop_ver) {
            int32_t item_center_y_natural = item_y_natural + item_height / 2;
            int32_t dist_y = item_center_y_natural + y_ofs_local - center_y;
            while(dist_y > total_height / 2) dist_y -= total_height;
            while(dist_y < -total_height / 2) dist_y += total_height;
            int32_t item_center_y = center_y + dist_y;
            item_pos.y = item_center_y - item_height / 2;
        }
        else {
            item_pos.y = item_y_natural + y_ofs;
        }

        if(use_loop_hor) {
            int32_t item_center_x_natural = item_x_natural + item_width / 2;
            int32_t dist_x = item_center_x_natural + x_ofs_local - center_x;
            while(dist_x > total_width / 2) dist_x -= total_width;
            while(dist_x < -total_width / 2) dist_x += total_width;
            int32_t item_center_x = center_x + dist_x;
            item_pos.x = item_center_x - item_width / 2;
        }
        else {
            item_pos.x = item_x_natural + x_ofs;
        }

        int32_t item_abs_x = cont_a.x1 + item_pos.x;
        int32_t item_abs_y = cont_a.y1 + item_pos.y;
        if((item_abs_x + item_width < (cont_a.x1 - MENU_RENDER_MARGIN_HOR)) ||
           (item_abs_x - item_width > (cont_a.x2 + MENU_RENDER_MARGIN_HOR)) ||
           (item_abs_y + item_height < (cont_a.y1 - MENU_RENDER_MARGIN_VER)) ||
           (item_abs_y - item_height > (cont_a.y2 + MENU_RENDER_MARGIN_VER))) {
            isHide = true;
        }

        item_pos.x += item_width / 2;
        item_pos.y += item_height / 2;

        int16_t zoom = LV_SCALE_NONE;
        if(menu->item_transform_cb) {
            zoom = menu->item_transform_cb(&item_pos, cont, child);
        }

        if(isHide || zoom < 2) {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            if(!use_loop_ver) {
                if((item_abs_y - item_height) > (cont_a.y2 + MENU_RENDER_MARGIN_VER)) {
                    invisible_real_idx = real_idx;
                    break;
                }
            }
        }
        else {
            lv_point_t item_size = {
                .x = lv_obj_get_width(child) * zoom / 256,
                .y = lv_obj_get_height(child) * zoom / 256,
            };
            lv_obj_set_pos(child, item_pos.x - item_size.x / 2, item_pos.y - item_size.y / 2);
            lv_obj_set_style_transform_scale(child, zoom * btn->zoom / 256, 0);
            lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
            draw_count++;
        }

        visible_idx++;
    }

    if(!use_loop_ver && invisible_real_idx >= 0) {
        int32_t idx = invisible_real_idx + 1;
        while(1) {
            lv_obj_t * child = get_disp_item(cont, &idx);
            if(child == NULL) break;
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            idx++;
        }
    }

    lv_obj_invalidate(cont);
}

static void scroll_timer_cb(lv_timer_t * p_timer)
{
    lv_obj_t * cont = (lv_obj_t *)p_timer->user_data;
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;

    float diff_x = menu->scroll.hor_momentum;
    float diff_y = menu->scroll.ver_momentum;
    float offset_x = menu->scroll.hor_offset;
    float offset_y = menu->scroll.ver_offset;
    float min_offset_x = menu->scroll.hor_offset_min;
    float max_offset_x = menu->scroll.hor_offset_max;
    float min_offset_y = menu->scroll.ver_offset_min;
    float max_offset_y = menu->scroll.ver_offset_max;

    bool scroll_done_x = true;
    bool scroll_done_y = true;

    diff_x = diff_x * menu->friction_factor / 100.f;
    diff_y = diff_y * menu->friction_factor / 100.f;

    if(menu->scroll.scroll_dir & LV_DIR_HOR) {
        bool in_range_x = SCROLL_IN_RANGE(offset_x, max_offset_x, min_offset_x);
        bool use_loop_hor = false;
        int32_t total_width = 0;
        int32_t cont_w = 0;

        if(menu->is_loop && (menu->scroll.scroll_dir & LV_DIR_HOR) &&
           !(menu->scroll.scroll_dir & LV_DIR_VER) && !menu->hex_layout) {
            cont_w = lv_obj_get_width(cont);
            int32_t item_width, item_height;
            get_item_size(cont, &item_width, &item_height);
            int32_t pad_column = lv_obj_get_style_pad_column(cont, LV_PART_ITEMS);
            int32_t child_cnt = get_disp_child_cnt(cont);
            uint16_t item_per_row = menu->item_per_row;
            uint16_t total_row = (child_cnt + item_per_row - 1) / item_per_row;

            if(total_row == 1) {
                uint16_t total_col = child_cnt;
                int32_t col_width = item_width + pad_column;
                total_width = total_col * col_width;
                if(total_width > cont_w) use_loop_hor = true;
            }
        }

        if(in_range_x || use_loop_hor) {
            offset_x += diff_x;

            if(use_loop_hor && total_width > 0) {
                int32_t center_x = cont_w / 2;
                int32_t start_range = center_x - total_width;
                while(offset_x > center_x) offset_x -= total_width;
                while(offset_x <= start_range) offset_x += total_width;
            }

            if(menu->is_snap) {
                int32_t center_x = lv_obj_get_width(cont) / 2;
                int32_t item_width, item_height;
                get_item_size(cont, &item_width, &item_height);
                int32_t pad_column = lv_obj_get_style_pad_column(cont, LV_PART_ITEMS);
                int32_t child_cnt = get_disp_child_cnt(cont);
                uint16_t total_col = use_loop_hor ? child_cnt : LV_MIN(child_cnt, menu->item_per_row);
                int32_t col_width = item_width + pad_column;

                if(col_width > 0 && total_col > 0) {
                    float center_offset = (float)center_x - offset_x;
                    int32_t current_col = (int32_t)((center_offset >= 0.f) ?
                                                    ((center_offset + col_width / 2.f) / col_width) :
                                                    ((center_offset - col_width / 2.f) / col_width));
                    current_col = CLAMP(current_col, 0, total_col - 1);

                    float snap_offset_x = center_x - (current_col * col_width + item_width / 2);
                    if(!use_loop_hor) {
                        snap_offset_x = CLAMP(snap_offset_x, max_offset_x, min_offset_x);
                    }

                    float force_x = offset_x - snap_offset_x;
                    bool overshot = (diff_x > 0 && force_x > 0) || (diff_x < 0 && force_x < 0);
                    if(overshot) {
                        offset_x = snap_offset_x;
                        diff_x = 0;
                        scroll_done_x = true;
                        force_x = 0;
                    }

                    if(fabsf(diff_x) < fabsf(force_x)) {
                        force_x = force_x * menu->snap_factor / 100.f;
                        if(fabsf(force_x) > 0.1f) {
                            offset_x -= force_x;
                            scroll_done_x = false;
                        }
                        else {
                            offset_x = snap_offset_x;
                            diff_x = 0;
                            scroll_done_x = true;
                        }
                    }
                }
            }

            if(LV_ABS(diff_x) < 1 && scroll_done_x) {
                diff_x = 0;
            }
        }
        else if(!use_loop_hor) {
            scroll_done_x = false;
            diff_x *= menu->out_of_bound_factor / 100.f;
            offset_x += diff_x;

            float force = LV_MAX(offset_x - min_offset_x, max_offset_x - offset_x);
            if(force > 1.f) {
                offset_x += copysignf(force * menu->elastic_factor / 100.f, min_offset_x - offset_x) +
                            copysignf(1.f, min_offset_x - offset_x);
            }
            else {
                if(fabsf(offset_x - min_offset_x) < fabsf(offset_x - max_offset_x)) {
                    offset_x = min_offset_x;
                }
                else {
                    offset_x = max_offset_x;
                }
                diff_x = 0;
                scroll_done_x = true;
            }
        }
    }
    else {
        diff_x = 0;
    }

    if(menu->scroll.scroll_dir & LV_DIR_VER) {
        bool in_range_y = SCROLL_IN_RANGE(offset_y, max_offset_y, min_offset_y);
        if(in_range_y || menu->is_loop) {
            offset_y += diff_y;

            if(menu->is_loop) {
                int32_t cont_h = lv_obj_get_height(cont);
                int32_t total_height = calculate_total_height(menu);
                if(total_height > cont_h) {
                    int32_t center_y = cont_h / 2;
                    int32_t start_range = center_y - total_height;
                    while(offset_y > center_y) offset_y -= total_height;
                    while(offset_y <= start_range) offset_y += total_height;
                }
            }

            if(menu->is_snap) {
                int32_t center_y = lv_obj_get_height(cont) / 2;
                int32_t item_width, item_height;
                get_item_size(cont, &item_width, &item_height);
                int32_t pad_row = lv_obj_get_style_pad_row(cont, LV_PART_ITEMS);
                float snap_offset_y = 0;

                if(menu->hex_layout) {
                    int32_t child_cnt = get_disp_child_cnt(cont);
                    float row_height = item_height - 0.14f * item_width + pad_row;
                    uint16_t group_size = menu->item_per_row * 2 - 1;
                    uint16_t groups = (child_cnt + group_size - 1) / group_size;
                    uint16_t total_row = groups * 2;
                    if(child_cnt % group_size <= menu->item_per_row && child_cnt % group_size != 0) {
                        total_row--;
                    }

                    if(row_height > 0 && total_row > 0) {
                        float k_cur_f = ((float)center_y - offset_y - (float)(item_height / 2)) / row_height;
                        int32_t current_row = (int32_t)((k_cur_f >= 0.f) ? (k_cur_f + 0.5f) : (k_cur_f - 0.5f));
                        current_row = CLAMP(current_row, 0, total_row - 1);
                        snap_offset_y = center_y - (current_row * row_height + item_height / 2);
                    }
                }
                else {
                    int32_t child_cnt = get_disp_child_cnt(cont);
                    int32_t item_per_row = menu->item_per_row;
                    int32_t row_height = item_height + pad_row;
                    int32_t total_rows = (child_cnt + item_per_row - 1) / item_per_row;
                    if(row_height > 0 && total_rows > 0) {
                        float k_cur_f = ((float)center_y - offset_y - (float)(item_height / 2)) / row_height;
                        int32_t current_row = (int32_t)((k_cur_f >= 0.f) ? (k_cur_f + 0.5f) : (k_cur_f - 0.5f));
                        current_row = CLAMP(current_row, 0, total_rows - 1);
                        snap_offset_y = center_y - (current_row * row_height + item_height / 2);
                    }
                }

                if(!menu->is_loop) {
                    snap_offset_y = CLAMP(snap_offset_y, max_offset_y, min_offset_y);
                }

                float force_y = offset_y - snap_offset_y;
                bool overshot = (diff_y > 0 && force_y > 0) || (diff_y < 0 && force_y < 0);
                if(overshot) {
                    offset_y = snap_offset_y;
                    diff_y = 0;
                    scroll_done_y = true;
                    force_y = 0;
                }

                if(fabsf(diff_y) < fabsf(force_y)) {
                    force_y = force_y * menu->snap_factor / 100.f;
                    if(fabsf(force_y) > 1.f) {
                        offset_y -= force_y;
                        scroll_done_y = false;
                    }
                    else {
                        offset_y = snap_offset_y;
                        diff_y = 0;
                        scroll_done_y = true;
                    }
                }
            }

            if(LV_ABS(diff_y) < 1 && scroll_done_y) {
                diff_y = 0;
            }
        }
        else if(!menu->is_loop) {
            scroll_done_y = false;
            diff_y *= menu->out_of_bound_factor / 100.f;
            offset_y += diff_y;

            float force = LV_MAX(offset_y - min_offset_y, max_offset_y - offset_y);
            if(force > 1.f) {
                offset_y += copysignf(force * menu->elastic_factor / 100.f, min_offset_y - offset_y) +
                            copysignf(1.f, min_offset_y - offset_y);
            }
            else {
                if(fabsf(offset_y - min_offset_y) < fabsf(offset_y - max_offset_y)) {
                    offset_y = min_offset_y;
                }
                else {
                    offset_y = max_offset_y;
                }
                diff_y = 0;
                scroll_done_y = true;
            }
        }
    }
    else {
        diff_y = 0;
    }

    if(scroll_done_x && (LV_ABS(diff_x) < 0.4f) &&
       scroll_done_y && (LV_ABS(diff_y) < 0.4f)) {
        lv_timer_pause(p_timer);
    }

    menu->scroll.hor_offset = offset_x;
    menu->scroll.ver_offset = offset_y;
    menu->scroll.hor_momentum = diff_x;
    menu->scroll.ver_momentum = diff_y;

    update_menu_items(cont);
}

static void scroll_hor_anim_exec_cb(void * var, int32_t val)
{
    lv_obj_t * cont = (lv_obj_t *)var;
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;
    menu->scroll.hor_offset = val;
    update_menu_items(cont);
}

static void scroll_ver_anim_exec_cb(void * var, int32_t val)
{
    lv_obj_t * cont = (lv_obj_t *)var;
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;
    menu->scroll.ver_offset = val;
    update_menu_items(cont);
}

static void scroll_anim_ready_cb(lv_anim_t * anim)
{
    lv_obj_t * cont = (lv_obj_t *)anim->var;
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;
    if(menu->scroll.animating_axis_cnt > 0) {
        menu->scroll.animating_axis_cnt--;
    }
    if(menu->scroll.animating_axis_cnt == 0 && menu->scroll.scroll_tmr) {
        lv_timer_resume(menu->scroll.scroll_tmr);
    }
}

static uint32_t get_disp_child_cnt(const lv_obj_t * obj)
{
    uint32_t cnt = 0;
    uint32_t child_cnt = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)child;
        if(!gridmenu_btn->is_hide) {
            cnt++;
        }
    }
    return cnt;
}

static lv_obj_t * get_disp_item(lv_obj_t * cont, int32_t * idx)
{
    lv_obj_t * child = NULL;
    int32_t child_cnt = lv_obj_get_child_count(cont);
    if(child_cnt < *idx) {
        return NULL;
    }
    for(int32_t i = *idx; i < child_cnt; i++) {
        child = lv_obj_get_child(cont, i);
        lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)child;
        if(!gridmenu_btn->is_hide) {
            *idx = i;
            return child;
        }
    }
    return NULL;
}

static void calc_render_param(lv_obj_t * cont)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)cont;
    menu->_need_recalc_total_height = true;

    int32_t child_cnt = get_disp_child_cnt(cont);
    int32_t cont_width = lv_obj_get_content_width(cont);
    int32_t cont_height = lv_obj_get_content_height(cont);
    if(child_cnt == 0 || cont_width == 0 || cont_height == 0) {
        return;
    }

    int32_t item_width, item_height;
    get_item_size(cont, &item_width, &item_height);

    int32_t pad_column = lv_obj_get_style_pad_column(cont, LV_PART_ITEMS);

    int32_t total_width = menu->item_per_row * item_width + (menu->item_per_row - 1) * pad_column;
    if(total_width > cont_width) {
        menu->scroll.hor_offset_min = -1 + menu->left_margin;
        menu->scroll.hor_offset_max = cont_width - total_width - menu->right_margin;
    }
    else {
        menu->scroll.hor_offset_min = (cont_width - total_width) / 2;
        menu->scroll.hor_offset_max = menu->scroll.hor_offset_min;
    }

    int32_t total_height = calculate_total_height(menu);
    if(total_height > cont_height) {
        menu->scroll.ver_offset_min = -1 + menu->top_margin;
        menu->scroll.ver_offset_max = cont_height - total_height - menu->bottom_margin;
    }
    else {
        menu->scroll.ver_offset_min = (cont_height - total_height) / 2;
        menu->scroll.ver_offset_max = menu->scroll.ver_offset_min;
    }

    if(!(menu->is_loop && (menu->scroll.scroll_dir & LV_DIR_VER) && total_height > cont_height)) {
        menu->scroll.ver_offset = CLAMP(menu->scroll.ver_offset, menu->scroll.ver_offset_max, menu->scroll.ver_offset_min);
    }
    menu->scroll.hor_offset = CLAMP(menu->scroll.hor_offset, menu->scroll.hor_offset_max, menu->scroll.hor_offset_min);
}

static void get_item_size(lv_obj_t * cont, int32_t * width, int32_t * height)
{
    int32_t idx = 0;
    lv_obj_t * child = get_disp_item(cont, &idx);
    if(child == NULL) {
        return;
    }

    *width = lv_obj_get_width(child);
    *height = lv_obj_get_height(child);
    if(*width == 0 || *height == 0) {
        lv_image_header_t info;
        if(lv_image_decoder_get_info(lv_image_get_src(child), &info)) {
            *width = info.w;
            *height = info.h;
        }
    }
    lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)child;
    *width = *width * gridmenu_btn->zoom / 256;
    *height = *height * gridmenu_btn->zoom / 256;
}

static int32_t calculate_total_height(lv_gridmenu_t * menu)
{
    if(!menu->_need_recalc_total_height && menu->_cached_total_height > 0) {
        return menu->_cached_total_height;
    }

    lv_obj_t * cont = (lv_obj_t *)menu;
    int32_t item_width = 0;
    int32_t item_height = 0;
    get_item_size(cont, &item_width, &item_height);
    int32_t pad_row = lv_obj_get_style_pad_row(cont, LV_PART_ITEMS);
    int32_t child_cnt = get_disp_child_cnt(cont);
    int32_t total_height = 0;

    if(child_cnt <= 0 || menu->item_per_row == 0) {
        menu->_cached_total_height = 0;
        menu->_need_recalc_total_height = false;
        return 0;
    }

    if(menu->hex_layout) {
        uint16_t group_size = menu->item_per_row * 2 - 1;
        uint16_t groups = (child_cnt + group_size - 1) / group_size;
        uint16_t total_row = groups * 2;
        if(child_cnt % group_size <= menu->item_per_row && child_cnt % group_size != 0) {
            total_row--;
        }
        float row_height = item_height - 0.14f * item_width + pad_row;
        total_height = (int32_t)(total_row * row_height);
    }
    else {
        uint16_t total_row = (child_cnt + menu->item_per_row - 1) / menu->item_per_row;
        int32_t row_height = item_height + pad_row;
        total_height = total_row * row_height;
    }

    menu->_cached_total_height = total_height;
    menu->_need_recalc_total_height = false;

    return total_height;
}

static int16_t grid_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn)
{
    return LV_SCALE_NONE;
}

static int16_t curve_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    int32_t width = lv_obj_get_content_width(obj);
    int32_t height = lv_obj_get_content_height(obj);
    float screen_radius = menu->style_param.curve.radius;
    int32_t x_center = width / 2;
    int32_t y_center = height / 2;
    int32_t item_width, item_height;
    get_item_size((lv_obj_t *)obj, &item_width, &item_height);

    // 计算到中心的距离
    float dist = fabsf(icon_pos->y - y_center);
    float scale = 0;

    // 缩放计算
    if(dist < screen_radius) {
        scale = 1.5f * sqrtf(SQUARE(screen_radius) - SQUARE(dist)) / screen_radius;
    }

    // 计算偏移量
    float xofs = (scale - 1.f) * item_width;

    if(icon_pos->x != x_center) {
        icon_pos->x += xofs * copysignf(1.f, icon_pos->x - x_center);
    }

    return (int16_t)(scale * LV_SCALE_NONE);
}

static int16_t star_item_transform_cb(lv_point_t * icon_pos, const lv_obj_t * obj, const lv_obj_t * btn)
{
    lv_gridmenu_t * menu = (lv_gridmenu_t *)obj;
    lv_gridmenu_button_t * gridmenu_btn = (lv_gridmenu_button_t *)btn;
    int32_t width = lv_obj_get_content_width(obj);
    int32_t height = lv_obj_get_content_height(obj);
    int32_t x_center = width / 2;
    int32_t y_center = height / 2;
    int32_t icon_size = lv_obj_get_width(btn) * gridmenu_btn->zoom / 256;
    int32_t icon_radius = icon_size / 2; /* 图标的半径 */
    int32_t radius = lv_obj_get_style_radius(obj, LV_PART_MAIN); /* 菜单的圆角半径 */
    int32_t r_corner = LV_MIN3(radius, x_center, y_center); /* 菜单的圆角半径 */

    /* 边缘检测阈值 */
    int32_t edge_det_threshold = LV_MAX(icon_radius, r_corner); /* 边缘检测阈值 */
    float inner_width_half = (width / 2.0f) - icon_radius; /* 内部宽度的一半 */
    float inner_height_half = (height / 2.0f) - icon_radius; /* 内部高度的一半 */

    float sphere_radius = menu->style_param.star.radius; /* 球体半径 */
    float distortion_coeff = menu->style_param.star.distortion_coeff / 100.f; /* 畸变系数 */

    float scale = 0;

    float cx = icon_pos->x - x_center;
    float cy = icon_pos->y - y_center;
    if(LV_ABS(cy) > sphere_radius || LV_ABS(cx) > sphere_radius) {
        // 超出合理范围，直接返回默认值，不进行球面变换
        return LV_SCALE_NONE;
    }

    float r2d = PYTHAG(cx, cy);
    /* 球面变换 */
    if(r2d > 1.f) {
        float r3d = PYTHAG3D(cx, cy, sphere_radius);
        float force = (r3d - sphere_radius) * distortion_coeff;
        icon_pos->x -= force * cx / r3d;
        icon_pos->y -= force * cy / r3d;
    }

    bool on_edge = false;
    float ex = 0;
    float ey = 0;

    /* 边缘检测 */
    if(edge_det_threshold == x_center && edge_det_threshold == y_center) {
        on_edge = true;
        ex = edge_det_threshold - icon_pos->x;
        ey = edge_det_threshold - icon_pos->y;
    }
    else {
        if(icon_pos->x < edge_det_threshold) {
            ex = edge_det_threshold - icon_pos->x;
            on_edge = true;
        }
        else if(icon_pos->x > (width - edge_det_threshold)) {
            ex = width - icon_pos->x - edge_det_threshold;
            on_edge = true;
        }

        if(icon_pos->y < edge_det_threshold) {
            ey = edge_det_threshold - icon_pos->y;
            on_edge = true;
        }
        else if(icon_pos->y > (height - edge_det_threshold)) {
            ey = height - icon_pos->y - edge_det_threshold;
            on_edge = true;
        }
    }

    if(on_edge) {
        float rr = PYTHAG(ex, ey);
        float dist2edge = edge_det_threshold - rr;

        if(dist2edge < icon_radius) {
            scale = dist2edge / icon_size + 0.5f;
            icon_pos->x += (1.f - scale) * icon_radius * (ex / rr);
            icon_pos->y += (1.f - scale) * icon_radius * (ey / rr);
        }
        else {
            on_edge = false;
        }
    }

    if(!on_edge) {
        float scale_x = LV_MAX((inner_width_half - fabsf(cx)) / inner_width_half, 0);
        float scale_y = LV_MAX((inner_height_half - fabsf(cy)) / inner_height_half, 0);
        scale = 1.f + 0.6f * scale_x * scale_y;
        if(scale < 1.f) {
            scale = 1.f;
        }
    }

    return (int16_t)(scale * 256);
}

#endif
