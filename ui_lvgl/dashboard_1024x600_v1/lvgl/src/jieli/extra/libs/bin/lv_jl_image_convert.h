/**
 * @file lv_jl_image_convert.h
 *
 */

#ifndef LV_JL_IMAGE_CONVERT_H
#define LV_JL_IMAGE_CONVERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../lvgl.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

bool lv_jl_image_needs_convert(lv_color_format_t cf);

lv_draw_buf_t * lv_jl_image_convert(const lv_image_dsc_t * src_dsc);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_JL_IMAGE_CONVERT_H*/
