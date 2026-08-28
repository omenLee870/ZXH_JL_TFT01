/**
 * @file lv_arcmenu.h
 *
 */
#ifndef LV_ARCMENU_H
#define LV_ARCMENU_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_ARCMENU != 0
#include "../../../../../src/core/lv_obj.h"

/*Testing of dependencies*/
#if LV_USE_IMAGE == 0
#error "lv_arcmenu: lv_image is required. Enable it in lv_conf.h (LV_USE_IMAGE 1)"
#endif
#if LV_USE_LABEL == 0
#error "lv_arcmenu: lv_label is required. Enable it in lv_conf.h (LV_USE_LABEL 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t lv_arcmenu_class;
extern const lv_obj_class_t lv_arcmenu_button_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Create an arc menu object
 * @param parent pointer to the parent object
 * @return lv_obj_t* pointer to the created arc menu object
 */
lv_obj_t *lv_arcmenu_create(lv_obj_t *parent);

/**
 * @brief Add an image button to the arc menu
 * @param menu pointer to the arc menu object
 * @param icon path to the image source
 * @param txt text to display on the button
 * @return lv_obj_t* pointer to the created button object
 */
lv_obj_t *lv_arcmenu_add_button(lv_obj_t *menu, const void *icon, const char *txt);

/*=====================
 * Setter functions
 *====================*/

/**
 * @brief Set the arc menu to be an arc
 * @param obj pointer to the arc menu object
 * @param is_arc true to set the arc menu to be an arc
 */
void lv_arcmenu_set_arc(lv_obj_t *obj, bool is_arc);

/**
 * @brief Set the arc menu to be a left arc
 * @param obj pointer to the arc menu object
 * @param is_left_arc true to set the arc menu to be a left arc
 */
void lv_arcmenu_set_left_arc(lv_obj_t *obj, bool is_left_arc);

/**
 * @brief Set the radius of the arc menu
 * @param obj pointer to the arc menu object
 * @param radius radius of the arc menu
 */
void lv_arcmenu_set_radius(lv_obj_t *obj, int32_t radius);

/**
 * @brief Set the text of the button
 * @param btn pointer to the button object
 * @param txt text to display on the button
 */
void lv_arcmenu_button_set_text(lv_obj_t *btn, const char *txt);

/**
 * @brief Set the image of the button
 * @param btn pointer to the button object
 * @param icon path to the image source
 */
void lv_arcmenu_button_set_image(lv_obj_t *btn, const void *icon);

void lv_arcmenu_button_set_image_zoom(lv_obj_t *btn, uint16_t zoom);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief Get whether the arc menu is an arc
 * @param obj pointer to the arc menu object
 * @return bool true if the arc menu is an arc
 */
bool lv_arcmenu_is_arc(lv_obj_t *obj);

/**
 * @brief Get whether the arc menu is a left arc
 * @param obj pointer to the arc menu object
 * @return bool true if the arc menu is a left arc
 */
bool lv_arcmenu_is_left_arc(lv_obj_t *obj);

/**
 * @brief Get the radius of the arc menu
 * @param obj pointer to the arc menu object
 * @return int32_t radius of the arc menu
 */
int32_t lv_arcmenu_get_radius(lv_obj_t *obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_ARCMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_ARCMENU_H*/
