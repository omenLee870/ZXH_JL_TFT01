/**
 * @file lv_curvemenu.h
 *
 */

#ifndef LV_CURVEMENU_H
#define LV_CURVEMENU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_CURVEMENU != 0
#include "../../../../../src/core/lv_obj.h"
#include "../../../../../src/widgets/image/lv_image.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_curvemenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t lv_curvemenu_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a curve menu objects
 * @param parent pointer to the parent object
 * @return lv_obj_t* pointer to the created curve menu object
 */
lv_obj_t *lv_curvemenu_create(lv_obj_t *parent);

/**
 * Add a button to the curve menu
 * @param parent pointer to the parent object
 * @param icon pointer to an image source (a C array or path to a file)
 * @return lv_obj_t* pointer to the created button object
 */
lv_obj_t *lv_curvemenu_add_button(lv_obj_t *parent, const void *icon);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the radius of the curve menu
 * @param obj
 * @param radius
 */
void lv_curvemenu_set_radius(lv_obj_t *obj, int32_t radius);

void lv_curvemenu_set_angle_step(lv_obj_t *obj, int32_t angle_step);

void lv_curvemenu_set_start_angle(lv_obj_t *obj, int32_t start);

void lv_curvemenu_set_end_angle(lv_obj_t *obj, int32_t end);

void lv_curvemenu_set_angles(lv_obj_t *obj, int32_t start, int32_t end);

void lv_curvemenu_set_col_num(lv_obj_t *obj, int32_t col_num);

void lv_curvemenu_set_cr(lv_obj_t *obj, int32_t cr);

void lv_curvemenu_button_set_hide(lv_obj_t *btn, bool is_hide);

void lv_curvemenu_button_set_zoom(lv_obj_t *btn, uint16_t zoom);

void lv_curvemenu_button_set_src(lv_obj_t *btn, const void *icon);

/*=====================
 * Getter functions
 *====================*/

int32_t lv_curvemenu_get_radius(lv_obj_t *obj);

int32_t lv_curvemenu_get_angle_step(lv_obj_t *obj);

int32_t lv_curvemenu_get_start_angle(lv_obj_t *obj);

int32_t lv_curvemenu_get_end_angle(lv_obj_t *obj);

int32_t lv_curvemenu_get_col_num(lv_obj_t *obj);

int32_t lv_curvemenu_get_cr(lv_obj_t *obj);

bool lv_curvemenu_button_get_hide(lv_obj_t *btn);

uint16_t lv_curvemenu_button_get_zoom(lv_obj_t *btn);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_CURVEMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_CURVEMENU_H*/