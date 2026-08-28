/**
 * @file lv_arc_menu.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#if LV_USE_ARCMENU != 0
#include "lv_arcmenu_private.h"
#include "../../../../../src/core/lv_obj_class_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/widgets/label/lv_label_private.h"


/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_arcmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif
#if LV_USE_LABEL == 0
#error "lv_arcmenu: lv_label is required. Enable it in lv_conf.h (LV_USE_LABEL 1)"
#endif


/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_arcmenu_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_arcmenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_arcmenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void scroll_event_cb(lv_event_t *e);
static lv_obj_t *get_img(lv_obj_t *btn);
static lv_obj_t *get_label(lv_obj_t *btn);
static void update_img_size_with_scale(lv_obj_t *img);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_arcmenu_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_arcmenu_constructor,
    .width_def = 200,
    .height_def = 200,
    .instance_size = sizeof(lv_arcmenu_t),
};

const lv_obj_class_t lv_arcmenu_button_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_arcmenu_button_constructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_arcmenu_button_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_arcmenu_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t *lv_arcmenu_add_button(lv_obj_t *menu, const void *icon, const char *txt)
{
    lv_obj_t *btn = lv_obj_class_create_obj(&lv_arcmenu_button_class, menu);
    lv_obj_class_init_obj(btn);
    lv_obj_set_size(btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_arcmenu_button_t *arc_menu_btn = (lv_arcmenu_button_t *)btn;

    if (icon) {
        lv_obj_t *img = lv_image_create(btn);
        lv_image_set_src(img, icon);
        arc_menu_btn->img = img;
    }

    if (txt) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_flex_grow(label, 1);
        arc_menu_btn->label = label;
    }

    return btn;
}

/*=====================
 * Setter functions
 *====================*/

void lv_arcmenu_set_arc(lv_obj_t *obj, bool is_arc)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    arc_menu->is_arc = is_arc;

    if (is_arc) {
        lv_obj_send_event(obj, LV_EVENT_SCROLL, NULL);
    } else {
        int32_t child_cnt = lv_obj_get_child_count(obj);
        for (int32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(obj, i);
            lv_obj_set_style_translate_x(child, 0, 0);
        }
    }
}

void lv_arcmenu_set_left_arc(lv_obj_t *obj, bool is_left_arc)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    arc_menu->is_left_arc = is_left_arc;

    if (arc_menu->is_arc) {
        lv_obj_send_event(obj, LV_EVENT_SCROLL, NULL);
    }
}

void lv_arcmenu_set_radius(lv_obj_t *obj, int32_t radius)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    arc_menu->radius = radius;

    if (arc_menu->is_arc) {
        lv_obj_send_event(obj, LV_EVENT_SCROLL, NULL);
    }
}

void lv_arcmenu_button_set_text(lv_obj_t *btn, const char *txt)
{
    lv_obj_t *label = get_label(btn);
    lv_label_set_text(label, txt);
}

void lv_arcmenu_button_set_image(lv_obj_t *btn, const void *icon)
{
    lv_obj_t *img = get_img(btn);
    lv_image_set_src(img, icon);
    update_img_size_with_scale(img);
}

void lv_arcmenu_button_set_image_zoom(lv_obj_t *btn, uint16_t zoom)
{
    lv_obj_t *img = get_img(btn);

    if (zoom == 0) {
        zoom = LV_SCALE_NONE;
    }

    lv_image_set_scale(img, zoom);

    update_img_size_with_scale(img);
}

/*=====================
 * Getter functions
 *====================*/

bool lv_arcmenu_is_arc(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    return arc_menu->is_arc;
}

bool lv_arcmenu_is_left_arc(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    return arc_menu->is_left_arc;
}

int32_t lv_arcmenu_get_radius(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    return arc_menu->radius;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_arcmenu_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)obj;
    arc_menu->is_arc = true;
    arc_menu->radius = 100;
    arc_menu->is_left_arc = false;

    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

    lv_obj_add_event_cb(obj, scroll_event_cb, LV_EVENT_SCROLL, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_arcmenu_button_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_arcmenu_button_t *arc_menu_btn = (lv_arcmenu_button_t *)obj;
    arc_menu_btn->img = NULL;
    arc_menu_btn->label = NULL;

    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_arcmenu_t *arc_menu = (lv_arcmenu_t *)cont;

    if (!arc_menu->is_arc) {
        return;
    }

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    int32_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_count(cont);
    int32_t r = arc_menu->radius;

    for (i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        int32_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;
        int32_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        int32_t x;
        if (diff_y >= r) {
            x = r;
        } else {
            uint32_t x_sqr = r * r - diff_y * diff_y;
            lv_sqrt_res_t res;
            lv_sqrt(x_sqr, &res, 0x8000);
            x = r - res.i;
        }

        if (arc_menu->is_left_arc) {
            x = -x;
        }

        // lv_obj_clear_flag(child, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_translate_x(child, x, 0);

        // Larger horizontal offset could be mapped to opacity if needed
        // lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        // lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

static lv_obj_t *get_img(lv_obj_t *btn)
{
    lv_arcmenu_button_t *arc_menu_btn = (lv_arcmenu_button_t *)btn;
    if (!arc_menu_btn->img) {
        arc_menu_btn->img = lv_image_create(btn);
        // Ensure the image stays as the first child
        if (lv_obj_get_index(arc_menu_btn->img) != 0) {
            lv_obj_move_to_index(arc_menu_btn->img, 0);
        }
    }

    return arc_menu_btn->img;
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

static lv_obj_t *get_label(lv_obj_t *btn)
{
    lv_arcmenu_button_t *arc_menu_btn = (lv_arcmenu_button_t *)btn;
    if (!arc_menu_btn->label) {
        arc_menu_btn->label = lv_label_create(btn);
        lv_label_set_long_mode(arc_menu_btn->label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_flex_grow(arc_menu_btn->label, 1);
    }

    return arc_menu_btn->label;
}

#endif
