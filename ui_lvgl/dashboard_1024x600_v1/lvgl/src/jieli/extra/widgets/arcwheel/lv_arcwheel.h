/**
 * @file lv_arcwheel.h
 */

#ifndef LV_ARCWHEEL_H
#define LV_ARCWHEEL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_ARCWHEEL != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_arcwheel: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_ARCWHEEL_STYLE_SLIDE_SCALE,
    LV_ARCWHEEL_STYLE_CUSTOM,
} lv_arcwheel_style_t;

typedef enum {
    LV_ARCWHEEL_SCROLL_DIR_HORIZONTAL,
    LV_ARCWHEEL_SCROLL_DIR_VERTICAL,
} lv_arcwheel_scroll_dir_t;

typedef union {
    struct {
        uint16_t zoom_min;
        uint16_t zoom_max;
    } slide_scale;

    struct {
        int32_t nop1;
        int32_t nop2;
        int32_t nop3;
        int32_t nop4;
    } custom;
} lv_arcwheel_transform_param_t;

typedef uint16_t (*lv_arcwheel_item_transform_cb_t)(const lv_obj_t * arcwheel, const lv_obj_t * item,
                                                    int32_t angle, lv_point_t * item_pos);

/*Note: The complete struct definitions are in lv_arcwheel_private.h*/

extern const lv_obj_class_t lv_arcwheel_class;
extern const lv_obj_class_t lv_arcwheel_item_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_arcwheel_create(lv_obj_t * parent);
lv_obj_t * lv_arcwheel_add_item(lv_obj_t * wheel, const void * icon);

void lv_arcwheel_set_radius(lv_obj_t * obj, int32_t radius_x, int32_t radius_y);
void lv_arcwheel_set_px_per_angle(lv_obj_t * obj, int32_t px_per_angle);
void lv_arcwheel_set_loop(lv_obj_t * obj, bool enabled);
void lv_arcwheel_set_visible_count(lv_obj_t * obj, uint16_t count);
void lv_arcwheel_set_angle_offset(lv_obj_t * obj, int32_t offset);
void lv_arcwheel_rotate_to_angle(lv_obj_t * obj, int32_t angle, bool anim, uint32_t speed);
void lv_arcwheel_stop_rotate(lv_obj_t * obj, bool snap_to_nearest, uint32_t speed);
void lv_arcwheel_item_set_src(lv_obj_t * item, const void * src);
void lv_arcwheel_set_style(lv_obj_t * obj, lv_arcwheel_style_t style, lv_arcwheel_transform_param_t * params);
void lv_arcwheel_set_item_transform_cb(lv_obj_t * obj, lv_arcwheel_item_transform_cb_t transform_cb,
                                       lv_arcwheel_transform_param_t * params);
void lv_arcwheel_set_friction_factor(lv_obj_t * obj, int32_t factor);
void lv_arcwheel_set_snap_factor(lv_obj_t * obj, int32_t factor);
void lv_arcwheel_set_snap(lv_obj_t * obj, bool enabled);
void lv_arcwheel_set_scroll_direction(lv_obj_t * obj, lv_arcwheel_scroll_dir_t dir);

int32_t lv_arcwheel_get_radius_x(lv_obj_t * obj);
int32_t lv_arcwheel_get_radius_y(lv_obj_t * obj);
int32_t lv_arcwheel_get_current_angle(lv_obj_t * obj);
int32_t lv_arcwheel_get_angle_offset(lv_obj_t * obj);
uint16_t lv_arcwheel_get_item_count(lv_obj_t * obj);
uint16_t lv_arcwheel_get_visible_count(lv_obj_t * obj);
int32_t lv_arcwheel_get_px_per_angle(lv_obj_t * obj);
bool lv_arcwheel_is_loop(lv_obj_t * obj);
lv_arcwheel_style_t lv_arcwheel_get_style(lv_obj_t * obj);
lv_arcwheel_transform_param_t * lv_arcwheel_get_transform_params(lv_obj_t * obj);
int32_t lv_arcwheel_get_friction_factor(lv_obj_t * obj);
int32_t lv_arcwheel_get_snap_factor(lv_obj_t * obj);
bool lv_arcwheel_is_snap(lv_obj_t * obj);
lv_arcwheel_scroll_dir_t lv_arcwheel_get_scroll_direction(lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ARCWHEEL*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_ARCWHEEL_H*/
