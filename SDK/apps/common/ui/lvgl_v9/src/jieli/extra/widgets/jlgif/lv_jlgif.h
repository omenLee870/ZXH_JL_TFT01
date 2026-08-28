/**
 * @file lv_jlgif.h
 */

#ifndef LV_JLGIF_H
#define LV_JLGIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/lv_conf_internal.h"
#if LV_USE_JLGIF != 0
#include "../../../../../src/core/lv_obj.h"
#include "jlgifdec.h"

/**********************
 *      TYPEDEFS
 **********************/

extern const lv_obj_class_t lv_jlgif_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Create a JLGIF object
 * @param parent Parent object
 * @return Created object
 */
lv_obj_t *lv_jlgif_create(lv_obj_t *parent);

/**
 * @brief Set the GIF source
 * @param obj JLGIF object
 * @param src Source data (file path or memory data)
 */
void lv_jlgif_set_src(lv_obj_t *obj, const void *src);

/**
 * @brief Restart the JLGIF animation
 * @param obj JLGIF object
 */
void lv_jlgif_restart(lv_obj_t *obj);

#endif /* LV_USE_JLGIF */

#ifdef __cplusplus
}
#endif

#endif /* LV_JLGIF_H */

