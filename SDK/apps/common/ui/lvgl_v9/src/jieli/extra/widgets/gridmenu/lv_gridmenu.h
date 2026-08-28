/**
 * @file lv_gridmenu.h
 *
 */

#ifndef LV_GRIDMENU_H
#define LV_GRIDMENU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_GRIDMENU != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_gridmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Callback to change the position、zoom of the icon
 *
 * @param icon_pos the position of the icon
 * @param menu the menu object
 * @return int16_t the zoom of the icon
 */
typedef int16_t (*lv_gridmenu_item_transform_cb_t)(lv_point_t *icon_pos, const lv_obj_t *menu, const lv_obj_t *btn);

/* 菜单风格 */
typedef enum {
    LV_GRIDMENU_STYLE_GRID, /* 宫格 */
    LV_GRIDMENU_STYLE_CURVE, /* 曲线 */
    LV_GRIDMENU_STYLE_STAR, /* 满天星 */

    LV_GRIDMENU_STYLE_CUSTOM = 0x10, /* 自定义风格，需要自己实现rect_change_cb函数 */
} lv_gridmenu_style_t;

typedef union {
    struct {
        int32_t radius;
    } curve;

    struct {
        int32_t radius; // 球体半径
        float distortion_coeff; // 畸变系数
    } star;

    struct {
        int32_t nop1;
        int32_t nop2;
        int32_t nop3;
        int32_t nop4;
    } custom;
} lv_gridmenu_style_param_t;

/*Note: The complete struct definitions are in lv_gridmenu_private.h*/

extern const lv_obj_class_t lv_gridmenu_class;
extern const lv_obj_class_t lv_gridmenu_button_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Create a grid menu object
 * @param parent parent object
 * @return lv_obj_t* pointer to the created grid menu object
 */
lv_obj_t *lv_gridmenu_create(lv_obj_t *parent);

/**
 * @brief Create a button in the grid menu
 * @param parent parent object
 * @param icon icon to be displayed on the button
 * @return lv_obj_t* pointer to the created button object
 */
lv_obj_t *lv_gridmenu_add_button(lv_obj_t *parent, const void *icon);

/*=====================
 * Setter functions
 *====================*/

/**
 * @brief Set the number of items per row
 * @param obj grid menu object
 * @param item_per_row number of items per row
 */
void lv_gridmenu_set_item_per_row(lv_obj_t *obj, uint8_t item_per_row);

/**
 * @brief Set the style of the grid menu
 * @param obj grid menu object
 * @param style style of the grid menu
 * @param param style parameters
 */
void lv_gridmenu_set_style(lv_obj_t *obj, lv_gridmenu_style_t style, lv_gridmenu_style_param_t *param);

/**
 * @brief Set the item transform callback function
 * @param obj grid menu object
 * @param item_transform_cb item transform callback function
 * @param param style parameters
 */
void lv_gridmenu_set_item_transform_cb(lv_obj_t *obj, lv_gridmenu_item_transform_cb_t item_transform_cb,
                                       lv_gridmenu_style_param_t *param);

/**
 * @brief Set the hex layout of the grid menu
 * @param obj grid menu object
 * @param hex_layout hex layout of the grid menu
 */
void lv_gridmenu_set_hex_layout(lv_obj_t *obj, bool hex_layout);

/**
 * @brief Set the scroll direction of the grid menu
 * @param obj grid menu object
 * @param dir scroll direction of the grid menu
 */
void lv_gridmenu_set_scroll_dir(lv_obj_t *obj, lv_dir_t dir);

/**
 * @brief Set the scroll offset of the grid menu
 * @param obj grid menu object
 * @param offset scroll offset of the grid menu
 * @param anim animate the scroll offset
 * @param speed scroll speed (px/s)
 */
void lv_gridmenu_set_scroll_offset(lv_obj_t *obj, lv_point_t offset, bool anim, uint32_t speed);

/**
 * @brief Set the scroll to center of the grid menu
 * @param obj grid menu object
 * @param anim animate the scroll to center
 * @param speed scroll speed (px/s)
 */
void lv_gridmenu_set_scroll_to_center(lv_obj_t *obj, bool anim, uint32_t speed);

/**
 * @brief Set the vertical margin of the grid menu
 * @param obj grid menu object
 * @param top_margin top margin of the grid menu
 * @param bottom_margin bottom margin of the grid menu
 */
void lv_gridmenu_set_ver_margin(lv_obj_t *obj, int32_t top_margin, int32_t bottom_margin);

/**
 * @brief Set the horizontal margin of the grid menu
 * @param obj grid menu object
 * @param left_margin left margin of the grid menu
 * @param right_margin right margin of the grid menu
 */
void lv_gridmenu_set_hor_margin(lv_obj_t *obj, int32_t left_margin, int32_t right_margin);

/**
 * @brief Set the snap enable of the grid menu
 * @param obj grid menu object
 * @param enable snap enable of the grid menu
 */
void lv_gridmenu_set_snap_enable(lv_obj_t *obj, bool enable);

/**
 * @brief Set the elastic factor of the grid menu
 * @param obj grid menu object
 * @param elastic_factor elastic factor of the grid menu (0-100, default value: 15)
 */
void lv_gridmenu_set_elastic_factor(lv_obj_t *obj, int32_t elastic_factor);

/**
 * @brief Set the friction factor of the grid menu
 * @param obj grid menu object
 * @param friction_factor friction factor of the grid menu (0-100, default value: 80)
 */
void lv_gridmenu_set_friction_factor(lv_obj_t *obj, int32_t friction_factor);

/**
 * @brief Set the out of bound factor of the grid menu
 * @param obj grid menu object
 * @param out_of_bound_factor out of bound factor of the grid menu (0-100, default value: 60)
 */
void lv_gridmenu_set_out_of_bound_factor(lv_obj_t *obj, int32_t out_of_bound_factor);

/**
 * @brief Set the snap factor of the grid menu
 * @param obj grid menu object
 * @param snap_factor snap factor of the grid menu (0-100, default value: 40)
 */
void lv_gridmenu_set_snap_factor(lv_obj_t *obj, int32_t snap_factor);

/**
 * @brief Enable or disable loop (circular) scrolling of the grid menu
 * @param obj grid menu object
 * @param enable whether to enable loop mode
 */
void lv_gridmenu_set_loop(lv_obj_t *obj, bool enable);

/**
 * @brief Set the hide of the grid menu button
 * @param btn grid menu button object
 * @param hide hide of the grid menu button
 */
void lv_gridmenu_button_set_hide(lv_obj_t *btn, bool hide);

/**
 * @brief Stop current scrolling immediately
 * @param obj grid menu object
 * @param snap_to_nearest whether to snap to the nearest item center immediately
 * @param speed scroll speed (px/s)
 */
void lv_gridmenu_stop_scroll(lv_obj_t *obj, bool snap_to_nearest, uint32_t speed);

/**
 * @brief Set the zoom of the grid menu button
 * @param btn grid menu button object
 * @param zoom zoom of the grid menu button
 */
void lv_gridmenu_button_set_zoom(lv_obj_t *btn, uint16_t zoom);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief Get the style of the grid menu
 * @param obj grid menu object
 * @return lv_gridmenu_style_t style of the grid menu
 */
lv_gridmenu_style_t lv_gridmenu_get_style(lv_obj_t *obj);

/**
 * @brief Get the style parameter of the grid menu
 * @param obj grid menu object
 * @return lv_gridmenu_style_param_t* style parameter of the grid menu
 */
lv_gridmenu_style_param_t *lv_gridmenu_get_style_param(lv_obj_t *obj);

/**
 * @brief Get the hex layout of the grid menu
 * @param obj grid menu object
 * @return bool hex layout of the grid menu
 */
bool lv_gridmenu_is_hex_layout(lv_obj_t *obj);

/**
 * @brief Get the item per row of the grid menu
 * @param obj grid menu object
 * @return int32_t item per row of the grid menu
 */
int32_t lv_gridmenu_get_item_per_row(lv_obj_t *obj);

/**
 * @brief Get the top margin of the grid menu
 * @param obj grid menu object
 * @return int32_t top margin of the grid menu
 */
int32_t lv_gridmenu_get_top_margin(lv_obj_t *obj);

/**
 * @brief Get the bottom margin of the grid menu
 * @param obj grid menu object
 * @return int32_t bottom margin of the grid menu
 */
int32_t lv_gridmenu_get_bottom_margin(lv_obj_t *obj);

/**
 * @brief Get the left margin of the grid menu
 * @param obj grid menu object
 * @return int32_t left margin of the grid menu
 */
int32_t lv_gridmenu_get_left_margin(lv_obj_t *obj);

/**
 * @brief Get the right margin of the grid menu
 * @param obj grid menu object
 * @return int32_t right margin of the grid menu
 */
int32_t lv_gridmenu_get_right_margin(lv_obj_t *obj);

/**
 * @brief Get the scroll vertical offset of the grid menu
 * @param obj grid menu object
 * @return int32_t scroll vertical offset of the grid menu
 */
int32_t lv_gridmenu_get_scroll_ver_offset(lv_obj_t *obj);

/**
 * @brief Get the scroll horizontal offset of the grid menu
 * @param obj grid menu object
 * @return int32_t scroll horizontal offset of the grid menu
 */
int32_t lv_gridmenu_get_scroll_hor_offset(lv_obj_t *obj);

/**
 * @brief Get the scroll direction of the grid menu
 * @param obj grid menu object
 * @return lv_dir_t scroll direction of the grid menu
 */
lv_dir_t lv_gridmenu_get_scroll_dir(lv_obj_t *obj);

/**
 * @brief Get the snap enable of the grid menu
 * @param obj grid menu object
 * @return bool snap enable of the grid menu
 */
bool lv_gridmenu_is_snap_enable(lv_obj_t *obj);

/**
 * @brief Get the elastic factor of the grid menu
 * @param obj grid menu object
 * @return int32_t elastic factor of the grid menu
 */
int32_t lv_gridmenu_get_elastic_factor(lv_obj_t *obj);

/**
 * @brief Get the friction factor of the grid menu
 * @param obj grid menu object
 * @return int32_t friction factor of the grid menu
 */
int32_t lv_gridmenu_get_friction_factor(lv_obj_t *obj);

/**
 * @brief Get the out of bound factor of the grid menu
 * @param obj grid menu object
 * @return int32_t out of bound factor of the grid menu
 */
int32_t lv_gridmenu_get_out_of_bound_factor(lv_obj_t *obj);

/**
 * @brief Get the snap factor of the grid menu
 * @param obj grid menu object
 * @return int32_t snap factor of the grid menu
 */
int32_t lv_gridmenu_get_snap_factor(lv_obj_t *obj);

/**
 * @brief Get whether loop (circular) scrolling is enabled
 * @param obj grid menu object
 * @return true if loop mode is enabled
 */
bool lv_gridmenu_is_loop(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_GRIDMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GRIDMENU_H*/

