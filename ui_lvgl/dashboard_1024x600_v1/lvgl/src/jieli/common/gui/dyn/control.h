/**
 * @file control.h
 *
 */

#ifndef CONTROL_H
#define CONTROL_H

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
 *    TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Parse the control data, call the lvgl related functions to create the control
 * @param buf The control data
 * @param size The size of the control data
 * @param parent The parent of the control
 * @param page_id The page id of the control
 * @return The control object
 */
lv_obj_t * control_parse(char * buf, int32_t size, lv_obj_t * parent, int32_t page_id);

/**
 * @brief Generate the control object
 * @param buf The control data
 * @param size The size of the control data
 * @param parent The parent of the control
 * @return The control object
 */
lv_obj_t * control_gen(char * buf, int32_t size, lv_obj_t * parent);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*CONTROL_H*/
