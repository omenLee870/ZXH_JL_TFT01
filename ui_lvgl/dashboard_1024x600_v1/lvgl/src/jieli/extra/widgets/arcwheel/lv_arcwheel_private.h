/**
 * @file lv_arcwheel_private.h
 */

#ifndef LV_ARCWHEEL_PRIVATE_H
#define LV_ARCWHEEL_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/misc/lv_timer_private.h"
#include "lv_arcwheel.h"

#if LV_USE_ARCWHEEL != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_arcwheel_t {
    lv_obj_t obj;

    lv_arcwheel_item_transform_cb_t item_transform_cb;
    lv_arcwheel_style_t style;
    lv_arcwheel_scroll_dir_t scroll_dir;

    int32_t current_angle;
    int32_t friction_factor;
    uint16_t visible_count;
    int32_t snap_factor;
    int32_t radius_x;
    int32_t radius_y;
    int32_t angle_offset;
    int32_t angle_step;
    int32_t px_per_angle;

    lv_arcwheel_transform_param_t transform_param;

    struct {
        lv_timer_t * scroll_tmr;
        int32_t momentum;
    } scroll;

    bool is_loop;
    bool is_snap;
    bool _is_init;

    struct {
        lv_anim_t anim;
        bool is_animating;
    } snap;
};

struct _lv_arcwheel_item_t {
    lv_image_t img;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_ARCWHEEL != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ARCWHEEL_PRIVATE_H*/
