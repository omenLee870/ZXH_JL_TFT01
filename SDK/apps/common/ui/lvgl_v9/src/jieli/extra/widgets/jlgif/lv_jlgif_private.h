/**
 * @file lv_jlgif_private.h
 * Private header for jlgif widget
 */

#ifndef LV_JLGIF_PRIVATE_H
#define LV_JLGIF_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "lv_jlgif.h"

#if LV_USE_JLGIF

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_jlgif_t lv_jlgif_t;

struct _lv_jlgif_t {
    lv_image_t img_ext;
    jl_gif_t *gif;
    lv_timer_t *timer;
    lv_image_dsc_t imgdsc;
    uint32_t last_frame_time;
};

#endif /* LV_USE_JLGIF */

#ifdef __cplusplus
}
#endif

#endif /* LV_JLGIF_PRIVATE_H */