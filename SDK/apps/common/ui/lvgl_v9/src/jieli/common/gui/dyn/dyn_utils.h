/**
 * @file dyn_utils.h
 *
 */

#ifndef DYN_UTILS_H
#define DYN_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "dyn_reader.h"
#include "gui_parse_dyn.h"
#include <time.h>

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

bool dyn_read_string_alloc(const uint8_t **p, const uint8_t *end, char **out);
void dyn_digitclock_set_text(lv_obj_t *obj, const char *fmt, const struct tm *time);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DYN_UTILS_H*/
