/**
 * @file lv_textprogress_private.h
 *
 */

#ifndef LV_TEXTPROGRESS_PRIVATE_H
#define LV_TEXTPROGRESS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/label/lv_label_private.h"
#include "lv_textprogress.h"

#if LV_USE_TEXTPROGRESS

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_textprogress_t {
    lv_label_t text;
    uint32_t range_max;
    uint32_t range_min;
    uint32_t value;
    uint32_t range_steps;
    uint32_t range_steps_min;
    uint16_t decimals;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_TEXTPROGRESS */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEXTPROGRESS_PRIVATE_H*/