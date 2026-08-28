/**
 * @file lv_listmenu_private.h
 *
 */

#ifndef LV_LISTMENU_PRIVATE_H
#define LV_LISTMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/widgets/label/lv_label_private.h"
#include "../../../../../src/misc/lv_timer_private.h"
#include "lv_listmenu.h"

#if LV_USE_LISTMENU != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_listmenu_t {
    lv_obj_t obj;

    lv_listmenu_item_transform_cb_t item_transform_cb;
    lv_listmenu_style_t style;
    lv_listmenu_item_layout_t item_layout;
    lv_listmenu_style_param_t style_param;

    struct {
        int32_t ver_offset;
        int32_t offset_min;
        int32_t offset_max;
        int32_t momentum;
        bool is_scroll;
        lv_timer_t *scroll_tmr;
    } scroll;

    int32_t friction_factor;
    int32_t snap_factor;
    int32_t elastic_factor;
    int32_t out_of_bound_factor;

    int32_t top_margin;
    int32_t bottom_margin;

    bool is_snap;
    bool is_loop;
    bool _is_init;

    int32_t _cached_total_height;
    bool _need_recalc_total_height;
};

struct _lv_listmenu_button_t {
    lv_obj_t obj;
    lv_obj_t *img;
    lv_obj_t *label;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_LISTMENU != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_LISTMENU_PRIVATE_H*/

