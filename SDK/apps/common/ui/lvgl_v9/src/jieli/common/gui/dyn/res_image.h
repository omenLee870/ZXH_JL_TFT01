/**
 * @file res_image.h
 *
 */

#ifndef RES_IMAGE_H
#define RES_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "gui_parse_dyn.h"

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void *res_image_parse(char *buf);

void res_image_free(void *buf);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*RES_IMAGE_H*/