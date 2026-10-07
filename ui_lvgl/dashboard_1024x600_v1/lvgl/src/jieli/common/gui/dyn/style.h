/**
 * @file style.h
 *
 */

#ifndef STYLE_H
#define STYLE_H

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
 *     TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Set the control style
 * @param buf The buffer of the control data
 * @param size The size of the control data
 * @param obj The object of the control
 * @param page_id The page id of the control
 */
void style_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*STYLE_H*/
