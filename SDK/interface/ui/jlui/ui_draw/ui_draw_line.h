/**
 * @file ui_draw_line.h
 *
 */

#ifndef UI_DRAW_LINE_H
#define UI_DRAW_LINE_H
#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "ui_draw/ui_obj.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    ui_color_t color;
    ui_style_int_t width;
    ui_style_int_t dash_width;
    ui_style_int_t dash_gap;
    ui_opa_t opa;
    ui_blend_mode_t blend_mode  : 2;
    uint8_t round_start : 1;
    uint8_t round_end   : 1;
    uint8_t raw_end     : 1;    /*Do not bother with perpendicular line ending is it's not visible for any reason*/
} ui_draw_line_dsc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

//! @cond Doxygen_Suppress
/**
 * Draw a line
 * @param point1 first point of the line
 * @param point2 second point of the line
 * @param clip the line will be drawn only in this area
 * @param dsc pointer to an initialized `lv_draw_line_dsc_t` variable
 */
ATTRIBUTE_FAST_MEM void ui_draw_line_inner(const ui_point_t *point1, const ui_point_t *point2, const ui_area_t *clip, const ui_draw_line_dsc_t *dsc);

ATTRIBUTE_FAST_MEM void ui_draw_line_dsc_init(ui_draw_line_dsc_t *dsc);

//! @endcond

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UI_DRAW_LINE_H*/

