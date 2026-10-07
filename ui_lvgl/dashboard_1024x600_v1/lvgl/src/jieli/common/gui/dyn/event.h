/**
 * @file event.h
 *
 */

#ifndef EVENT_H
#define EVENT_H

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
 * @brief Set the control event
 * @param buf The control data
 * @param size The size of the control data
 * @param obj The object of the control
 * @param page_id The page id of the control
 */
void event_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id);

/**
 * @brief Set the event get cb
 * @param cb
 */
void event_set_get_cb(gui_get_event_cb_t cb);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EVENT_H*/
