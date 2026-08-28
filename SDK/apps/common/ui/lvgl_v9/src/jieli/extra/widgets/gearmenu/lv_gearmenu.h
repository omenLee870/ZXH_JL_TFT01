/**
 * @file lv_gearmenu.h
 *
 */

#ifndef LV_GEARMENU_H
#define LV_GEARMENU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_GEARMENU != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_gearmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif
#if LV_USE_LABEL == 0
#error "lv_gearmenu: lv_label is required. Enable it in lv_conf.h (LV_USE_LABEL 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Note: The complete struct definitions are in lv_gearmenu_private.h*/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a gear menu object
 * @param parent pointer to the parent object
 * @return lv_obj_t* pointer to the created gear menu object
 */
lv_obj_t *lv_gearmenu_create(lv_obj_t *parent);

/**
 * Add a button to the gear menu
 * @param gear_menu pointer to the gear menu object
 * @param icon pointer to the image source
 * @param txt text to display on the button
 * @return lv_obj_t* pointer to the created button object
 */
lv_obj_t *lv_gearmenu_add_button(lv_obj_t *gear_menu, const void *icon, const char *txt);

/**
 * change the current angle to the closest angle in integral multiple of the closet angle step
 * @param gear_menu pointer to the gear menu object
 */
void lv_gearmenu_rotate_to_the_closest_angle(lv_obj_t *obj);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the start angle of the gear menu
 * @param obj pointer to the gear menu object
 * @param start start angle of the gear menu
 */
void lv_gearmenu_set_start_angle(lv_obj_t *obj, int32_t start);

/**
 * Set the end angle of the gear menu
 * @param obj pointer to the gear menu object
 * @param end end angle of the gear menu
 */
void lv_gearmenu_set_end_angle(lv_obj_t *obj, int32_t end);

/**
 * Set the start and end angles of the gear menu
 * @param obj pointer to the gear menu object
 * @param start start angle of the gear menu
 * @param end end angle of the gear menu
 */
void lv_gearmenu_set_angles(lv_obj_t *obj, int32_t start, int32_t end);

/**
 * Set the radius of the gear menu
 * @param obj pointer to the gear menu object
 * @param radius radius of the gear menu
 */
void lv_gearmenu_set_radius(lv_obj_t *obj, int32_t radius);

/**
 * Set the angle step of the gear menu
 * @param obj pointer to the gear menu object
 * @param angle_step angle step of the gear menu
 */
void lv_gearmenu_set_angle_step(lv_obj_t *obj, int32_t angle_step);

/**
 * Set the pixels per angle of the gear menu
 * @param obj pointer to the gear menu object
 * @param px_per_angle pixels per angle of the gear menu
 */
void lv_gearmenu_set_px_per_angle(lv_obj_t *obj, int32_t px_per_angle);

/**
 * Set the center x of the gear menu
 * @param obj pointer to the gear menu object
 * @param x center x of the gear menu
 */
void lv_gearmenu_set_center_x(lv_obj_t *obj, int32_t x);

/**
 * Set the center y of the gear menu
 * @param obj pointer to the gear menu object
 * @param y center y of the gear menu
 */
void lv_gearmenu_set_center_y(lv_obj_t *obj, int32_t y);

/**
 * Set the center of the gear menu
 * @param obj pointer to the gear menu object
 * @param x center x of the gear menu
 * @param y center y of the gear menu
 */
void lv_gearmenu_set_center(lv_obj_t *obj, int32_t x, int32_t y);

/**
 * Set the gear menu to be clockwise or counterclockwise
 * @param obj pointer to the gear menu object
 * @param is_clockwise true to set the gear menu to be clockwise
 */
void lv_gearmenu_set_clockwise(lv_obj_t *obj, bool is_clockwise);

/**
 * Set the button to the center of the gear menu
 * @param obj pointer to the gear menu object
 * @param btn pointer to the button object
 */
void lv_gearmenu_set_button_to_center(lv_obj_t *obj, lv_obj_t *btn);

/**
 * Set the image of the gear menu button
 * @param btn pointer to the button object
 * @param icon pointer to the image source
 */
void lv_gearmenu_button_set_image(lv_obj_t *btn, const void *icon);

/**
 * Set the text of the gear menu button
 * @param btn pointer to the button object
 * @param txt text to display on the button
 */
void lv_gearmenu_button_set_text(lv_obj_t *btn, const char *txt);

/**
 * Set the zoom of the gear menu button image
 * @param btn pointer to the button object
 * @param zoom zoom of the button image
 */
void lv_gearmenu_button_set_image_zoom(lv_obj_t *btn, uint16_t zoom);

/**
 * Set the gear menu button to be hidden or not
 * @param btn pointer to the button object
 * @param is_hide true to set the button to be hidden
 */
void lv_gearmenu_button_set_hide(lv_obj_t *btn, bool is_hide);

/**
 * Set the current angle of the gear menu
 * @param obj pointer to the gear menu object
 * @param angle angle of the gear menu
 */
void lv_gearmenu_set_angle(lv_obj_t *obj, int32_t angle);

/**
 * Set the bounce animation time of the gear menu
 * @param obj pointer to the gear menu object
 * @param bounce_anim_time the bounce animation time of the gear menu
 */
void lv_gearmenu_set_bounce_anim_time(lv_obj_t *obj, int32_t bounce_anim_time);

/**
 * Set the loop mode of the gear menu
 * @param obj pointer to the gear menu object
 * @param is_loop_mode true to enable the loop mode
 */
void lv_gearmenu_set_loop_mode(lv_obj_t *obj, bool is_loop_mode);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the start angle of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t start angle of the gear menu
 */
int32_t lv_gearmenu_get_start_angle(lv_obj_t *obj);

/**
 * Get the end angle of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t end angle of the gear menu
 */
int32_t lv_gearmenu_get_end_angle(lv_obj_t *obj);

/**
 * Get the radius of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t radius of the gear menu
 */
int32_t lv_gearmenu_get_radius(lv_obj_t *obj);

/**
 * Get the angle step of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t angle step of the gear menu
 */
int32_t lv_gearmenu_get_angle_step(lv_obj_t *obj);

/**
 * Get the pixels per angle of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t pixels per angle of the gear menu
 */
int32_t lv_gearmenu_get_px_per_angle(lv_obj_t *obj);

/**
 * Get the hide status of the gear menu button
 * @param btn pointer to the gear menu button object
 * @return true if the button is hidden
 * @return false if the button is not hidden
 */
bool lv_gearmenu_btn_get_hide(lv_obj_t *btn);

/**
 * Get the current angle of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t angle of the gear menu
 */
int32_t lv_gearmenu_get_angle(lv_obj_t *obj);

/**
 * Get the bounce animation time of the gear menu
 * @param obj pointer to the gear menu object
 * @return int32_t bounce animation time of the gear menu
 */
int32_t lv_gearmenu_get_bounce_anim_time(lv_obj_t *obj);

/**
 * Get the loop mode of the gear menu
 * @param obj pointer to the gear menu object
 * @return whether the loop mode is enabled
 */
bool lv_gearmenu_get_loop_mode(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_GEARMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GEARMENU_H*/