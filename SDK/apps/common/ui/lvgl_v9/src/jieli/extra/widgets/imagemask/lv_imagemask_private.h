/**
 * @file lv_imagemask_private.h
 */

#ifndef LV_IMAGEMASK_PRIVATE_H
#define LV_IMAGEMASK_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "lv_imagemask.h"

#if LV_USE_IMAGEMASK != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_imagemask_t {
    lv_image_t img;                 /* Inherit lv_image_t layout */
    lv_image_dsc_t *mask_dsc;     /* Owned mask created by this widget, free on destroy */
    const void *mask;      /* User provided mask source (file path or dsc) */
    int32_t mask_scale;
    int32_t mask_rotation;
    int32_t mask_offset_x;
    int32_t mask_offset_y;
    lv_point_t mask_pivot;
    uint8_t mask_type;
} lv_imagemask_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_IMAGEMASK != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_IMAGEMASK_PRIVATE_H*/
