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

/** 应用一个事件项，payload 以 event_code 开头。 */
bool event_set_item(uint32_t action_type, dyn_view_t payload, lv_obj_t *obj, int32_t page_id);

void event_set_get_cb(gui_get_event_cb_t cb);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EVENT_H*/
