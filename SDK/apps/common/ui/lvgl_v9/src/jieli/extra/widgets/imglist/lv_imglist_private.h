/**
 * @file lv_imglist_private.h
 *
 */

#ifndef LV_IMGLIST_PRIVATE_H
#define LV_IMGLIST_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "lv_imglist.h"

#if LV_USE_IMGLIST != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Data of image*/
struct _lv_imglist_t {
    lv_image_t img;
    /*picture sequence */
    const void **dsc;
    int8_t  pic_count;
    uint32_t act_idx;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_IMGLIST != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_IMGLIST_PRIVATE_H*/