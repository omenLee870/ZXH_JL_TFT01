/**
 * @file prop.h
 *
 */

#ifndef PROP_H
#define PROP_H

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

/** 应用一个属性项，payload 不含 item 头和填充。 */
bool prop_set_item(uint32_t prop_id, dyn_view_t payload, lv_obj_t *obj, int32_t page_id);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PROP_H*/
