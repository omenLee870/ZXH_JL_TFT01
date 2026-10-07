/**
 * @file lv_imagemask.h
 */

#ifndef LV_IMAGEMASK_H
#define LV_IMAGEMASK_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_IMAGEMASK != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_imagemask: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Note: The complete struct definitions are in lv_imagemask_private.h*/

extern const lv_obj_class_t lv_imagemask_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a new image mask widget (inherits from lv_image)
 * @param parent parent object
 */
lv_obj_t * lv_imagemask_create(lv_obj_t * parent);

/**********************
 * Setter functions
 **********************/

/**
 * Set the main image source displayed by this widget
 * @param obj image mask object
 * @param src image source (file path, symbol, or lv_image_dsc_t*)
 */
void lv_imagemask_set_src(lv_obj_t * obj, const void * src);

/**
 * Set image mask from an image source (file path or image dsc).
 * The widget takes care of generating an internal A8 mask matching the
 * object's current size and frees it automatically on destruction.
 * @param obj image mask object
 * @param mask image source for generating the alpha mask
 */
void lv_imagemask_set_mask(lv_obj_t * obj, const void * mask);

/**
 * @brief Set the scale of the image mask
 * @param obj image mask object
 * @param scale scale factor
 */
void lv_imagemask_set_mask_scale(lv_obj_t * obj, int32_t scale);

/**
 * @brief Set the rotation of the image mask
 * @param obj image mask object
 * @param rotation rotation angle
 */
void lv_imagemask_set_mask_rotation(lv_obj_t * obj, int32_t rotation);

/**
 * @brief Set the offset of the image mask
 * @param obj image mask object
 * @param offset_x offset x
 */
void lv_imagemask_set_mask_offset_x(lv_obj_t * obj, int32_t offset_x);

/**
 * @brief Set the offset of the image mask
 * @param obj image mask object
 * @param offset_y offset y
 */
void lv_imagemask_set_mask_offset_y(lv_obj_t * obj, int32_t offset_y);

/**
 * @brief Set the pivot of the image mask
 * @param obj image mask object
 * @param x x
 * @param y y
 */
void lv_imagemask_set_mask_pivot(lv_obj_t * obj, int32_t x, int32_t y);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief Get the currently applied mask descriptor (can be NULL)
 * @param obj image mask object
 * @return the currently applied mask descriptor (can be NULL)
 */
const void * lv_imagemask_get_mask(const lv_obj_t * obj);

/**
 * @brief Get the scale of the image mask
 * @param obj image mask object
 * @return the scale of the image mask
 */
int32_t lv_imagemask_get_mask_scale(const lv_obj_t * obj);

/**
 * @brief Get the rotation of the image mask
 * @param obj image mask object
 * @return the rotation of the image mask
 */
int32_t lv_imagemask_get_mask_rotation(const lv_obj_t * obj);

/**
 * @brief Get the offset of the image mask
 * @param obj image mask object
 * @return the offset of the image mask
 */
int32_t lv_imagemask_get_mask_offset_x(const lv_obj_t * obj);

/**
 * @brief Get the offset of the image mask
 * @param obj image mask object
 * @return the offset of the image mask
 */
int32_t lv_imagemask_get_mask_offset_y(const lv_obj_t * obj);

/**
 * @brief Get the pivot of the image mask
 * @param obj image mask object
 * @param pivot the pivot of the image mask
 */
void lv_imagemask_get_mask_pivot(const lv_obj_t * obj, lv_point_t * pivot);

#endif /*LV_USE_IMAGEMASK*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_IMAGEMASK_H*/
