/**
 * @file ui_draw_rect.h
 *
 */

#ifndef UI_DRAW_RECT_H
#define UI_DRAW_RECT_H

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
    ui_style_int_t radius;

    /*Background*/
    ui_color_t bg_color;
    ui_color_t bg_grad_color;
    ui_grad_dir_t bg_grad_dir;
    ui_style_int_t bg_main_color_stop;
    ui_style_int_t bg_grad_color_stop;
    ui_opa_t bg_opa;
    // ui_blend_mode_t bg_blend_mode;
    struct rect *bg_limit_rect;

    /*Border*/
    ui_color_t border_color;
    ui_style_int_t border_width;
    ui_style_int_t border_side;
    ui_opa_t border_opa;
    // ui_blend_mode_t border_blend_mode;
    uint8_t border_post : 1;        /*There is a border it will be drawn later. */

} ui_draw_rect_dsc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

ATTRIBUTE_FAST_MEM void ui_draw_rect_dsc_init(ui_draw_rect_dsc_t *dsc);

//! @endcond

/**
 * Draw a rectangle
 * @param coords the coordinates of the rectangle
 * @param clip the rectangle will be drawn only in this area
 * @param dsc pointer to an initialized `ui_draw_rect_dsc_t` variable
 */
void ui_draw_rect_inner(const ui_area_t *coords, const ui_area_t *clip, const ui_draw_rect_dsc_t *dsc);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*UI_DRAW_RECT_H*/

