/**
 * @file lv_gridmenu_private.h
 *
 */

#ifndef LV_GRIDMENU_PRIVATE_H
#define LV_GRIDMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/misc/lv_timer_private.h"
#include "lv_gridmenu.h"

#if LV_USE_GRIDMENU != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_gridmenu_t {
    lv_obj_t obj;

    lv_gridmenu_item_transform_cb_t item_transform_cb;
    lv_gridmenu_style_t style;
    lv_gridmenu_style_param_t style_param;

    struct {
        int32_t hor_offset;
        int32_t hor_offset_min;
        int32_t hor_offset_max;
        int32_t hor_momentum;

        int32_t ver_offset;
        int32_t ver_offset_min;
        int32_t ver_offset_max;
        int32_t ver_momentum;

        lv_dir_t scroll_dir;

        lv_timer_t * scroll_tmr;
        uint8_t animating_axis_cnt;
    } scroll;

    int32_t friction_factor;
    int32_t snap_factor;
    int32_t elastic_factor;
    int32_t out_of_bound_factor;

    int32_t top_margin;
    int32_t bottom_margin;
    int32_t left_margin;
    int32_t right_margin;
    uint8_t item_per_row;

    bool hex_layout;
    bool is_snap;
    bool is_loop;
    bool _is_init;

    int32_t _cached_total_height;
    bool _need_recalc_total_height;
};

struct _lv_gridmenu_button_t {
    lv_image_t img;
    uint16_t zoom;
    bool is_hide;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_GRIDMENU != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GRIDMENU_PRIVATE_H*/