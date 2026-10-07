/**
 * @file lv_arcmenu_private.h
 *
 */

#ifndef LV_ARCMENU_PRIVATE_H
#define LV_ARCMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "lv_arcmenu.h"

#if LV_USE_ARCMENU != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_arcmenu_t {
    lv_obj_t obj;
    bool is_arc;
    bool is_left_arc;
    int32_t radius;
};

struct _lv_arcmenu_button_t {
    lv_obj_t obj;
    lv_obj_t * img;
    lv_obj_t * label;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_ARCMENU != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ARCMENU_PRIVATE_H*/