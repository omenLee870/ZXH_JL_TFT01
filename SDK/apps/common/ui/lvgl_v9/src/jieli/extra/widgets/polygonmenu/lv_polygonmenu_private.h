/**
 * @file lv_polygonmenu_private.h
 * Private header for polygonmenu widget
 */

#ifndef LV_POLYGONMENU_PRIVATE_H
#define LV_POLYGONMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "lv_polygonmenu.h"

#if LV_USE_POLYGONMENU != 0

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_polygonmenu_t {
    lv_obj_t obj;
    lv_polygonmenu_shape_t shape;

    lv_polygonmenu_item_transform_cb_t item_transform_cb;
    lv_polygonmenu_style_param_t style_param;
    int32_t icon_min_size; // 转换后，隐藏大小小于该值的icon

    lv_polygonmenu_scr_cb scr_cb;

    // 内部属性
    int32_t icon_size;
    int32_t icon_width;
    int32_t icon_height;

    uint8_t status;
    uint8_t prev_status;
    uint16_t max_row_icon_count;
    int32_t offset_x;
    int32_t offset_y;
    lv_timer_t *move_tmr;
    lv_timer_t *temp_status_report_tmr;
    void *move_tmr_data;
};

struct _lv_polygonmenu_button_t {
    lv_image_t obj;
    bool is_hide; // 是否隐藏
    int32_t scr_id; //屏幕id

    // 内部属性
    int32_t offset_x;
    int32_t offset_y;
};

#endif /*LV_USE_POLYGONMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_POLYGONMENU_PRIVATE_H*/