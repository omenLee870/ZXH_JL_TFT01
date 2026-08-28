/**
 * @file lv_kaleidoscopemenu.h
 *
 */
#ifndef LV_KALEIDOSCOPEMENU_H
#define LV_KALEIDOSCOPEMENU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_KALEIDOSCOPEMENU != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_kaleidoscopemenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t lv_kaleidoscopemenu_class;
extern const lv_obj_class_t lv_kaleidoscopemenu_button_class;


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Create an kaleidoscope menu object
 * @param parent pointer to the parent object
 * @return lv_obj_t* pointer to the created kaleidoscope menu object
 */
lv_obj_t *lv_kaleidoscopemenu_create(lv_obj_t *parent);

/**
 * @brief Add an image button to the kaleidoscopemenu menu
 * @param menu pointer to the kaleidoscopemenu menu object
 * @param icon path to the image source
 * @return lv_obj_t* pointer to the created button object
 */
lv_obj_t *lv_kaleidoscopemenu_add_button(lv_obj_t *menu, const void *icon);

/*=====================
 * Setter functions
 *====================*/

/**
 * @brief Set the size of the kaleidoscopemenu btn.
 * @param obj pointer to the kaleidoscope menu object
 * @param icon_size size of the kaleidoscopemenu btn
 */
void lv_kaleidoscopemenu_set_icon_size(lv_obj_t *obj, uint16_t icon_size);

/**
 * @brief Set the icon interval of the kaleidoscopemenu btn ring.
 * @param obj pointer to the kaleidoscope menu object
 * @param ring_interval the interval of the kaleidoscopemenu btn ring
 */
void lv_kaleidoscopemenu_set_ring_interval(lv_obj_t *obj, uint16_t ring_interval);

/**
 * @brief Set the is_hide status of the kaleidoscopemenu btn icon
 * @param obj pointer to the kaleidoscope menu object
 * @param hide is_hide status of the kaleidoscopemenu btn icon
 */
void lv_kaleidoscopemenu_button_set_hide(lv_obj_t *obj, bool hide);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief Get the size of the kaleidoscopemenu btn
 * @param obj pointer to the kaleidoscope menu object
 * @return the size of the kaleidoscopemenu btn
 */
uint16_t lv_kaleidoscopemenu_get_icon_size(lv_obj_t *obj);

/**
 * @brief Get the interval of the kaleidoscopemenu btn ring
 * @param obj pointer to the kaleidoscope menu object
 * @return the interval of the kaleidoscopemenu btn ring
 */
uint16_t lv_kaleidoscopemenu_get_ring_interval(lv_obj_t *obj);

/**
 * @brief Get the is_hide status of the kaleidoscopemenu btn icon
 * @param obj pointer to the kaleidoscope menu object
 * @return is_hide status of the kaleidoscopemenu btn icon
 */
bool lv_kaleidoscopemenu_button_get_hide(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_KALEIDOSCOPEMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_KALEIDOSCOPEMENU_H*/
