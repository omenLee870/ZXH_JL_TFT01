/**
 * @file style.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "style.h"
#include "res.h"

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void style_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id)
{
    if(buf == NULL || size <= 0) {
        return;
    }

    JLDynControl * control = (JLDynControl *)buf;
    char * style_buf = buf + control->style_offset;
    int32_t offset = 0;
    int32_t style_size = control->event_offset - control->style_offset;
    while(offset < style_size) {
        int32_t style_id = *(int32_t *)(style_buf + offset);
        offset += sizeof(int32_t);
        int32_t temp_offset = offset;
        // LV_LOG_WARN("style_id = %x %x %x", style_id, *(int32_t *)(style_buf +
        // offset), *(int32_t *)(style_buf + offset + sizeof(int32_t)));
        switch(style_id) {
            case LV_OBJ_SET_STYLE_BG_COLOR_ID:
                lv_obj_set_style_bg_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                          *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_GRAD_COLOR_ID:
                lv_obj_set_style_bg_grad_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                               *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_OPA_ID:
                lv_obj_set_style_bg_opa(obj, *(int32_t *)(style_buf + offset),
                                        *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_GRAD_DIR_ID:
                lv_obj_set_style_bg_grad_dir(obj, *(int32_t *)(style_buf + offset),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BORDER_COLOR_ID:
                lv_obj_set_style_border_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BORDER_OPA_ID:
                lv_obj_set_style_border_opa(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BORDER_SIDE_ID:
                lv_obj_set_style_border_side(obj, *(int32_t *)(style_buf + offset),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BORDER_WIDTH_ID:
                lv_obj_set_style_border_width(obj, *(int32_t *)(style_buf + offset),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_RADIUS_ID:
                lv_obj_set_style_radius(obj, *(int32_t *)(style_buf + offset),
                                        *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_COLOR_ID:
                lv_obj_set_style_shadow_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_OFS_X_ID:
                lv_obj_set_style_shadow_offset_x(obj, *(int32_t *)(style_buf + offset),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_OFS_Y_ID:
                lv_obj_set_style_shadow_offset_y(obj, *(int32_t *)(style_buf + offset),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_SPREAD_ID:
                lv_obj_set_style_shadow_spread(obj, *(int32_t *)(style_buf + offset),
                                               *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_OPA_ID:
                lv_obj_set_style_shadow_opa(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_SHADOW_WIDTH_ID:
                lv_obj_set_style_shadow_width(obj, *(int32_t *)(style_buf + offset),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_OUTLINE_COLOR_ID:
                lv_obj_set_style_outline_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                               *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_OUTLINE_OPA_ID:
                lv_obj_set_style_outline_opa(obj, *(int32_t *)(style_buf + offset),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_OUTLINE_WIDTH_ID:
                lv_obj_set_style_outline_width(obj, *(int32_t *)(style_buf + offset),
                                               *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_OUTLINE_PAD_ID:
                lv_obj_set_style_outline_pad(obj, *(int32_t *)(style_buf + offset),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_LEFT_ID:
                lv_obj_set_style_pad_left(obj, *(int32_t *)(style_buf + offset),
                                          *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_RIGHT_ID:
                lv_obj_set_style_pad_right(obj, *(int32_t *)(style_buf + offset),
                                           *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_TOP_ID:
                lv_obj_set_style_pad_top(obj, *(int32_t *)(style_buf + offset),
                                         *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_BOTTOM_ID:
                lv_obj_set_style_pad_bottom(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_ROW_ID:
                lv_obj_set_style_pad_row(obj, *(int32_t *)(style_buf + offset),
                                         *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_COLUMN_ID:
                lv_obj_set_style_pad_column(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_PADDING_ALL_ID:
                lv_obj_set_style_pad_all(obj, *(int32_t *)(style_buf + offset),
                                         *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_TEXT_ID: {
                    int32_t font_id = *(int32_t *)(style_buf + offset);
                    lv_font_t * font = res_get(page_id, font_id);
                    if(font != NULL) {
                        lv_obj_set_style_text_font(obj, font, *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                    }
                    offset += 2 * sizeof(int32_t);
                }
                break;
            case LV_OBJ_SET_STYLE_TEXT_COLOR_ID:
                lv_obj_set_style_text_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LETTER_SPACE_ID:
                lv_obj_set_style_text_letter_space(obj, *(int32_t *)(style_buf + offset),
                                                   *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LINE_SPACE_ID:
                lv_obj_set_style_text_line_space(obj, *(int32_t *)(style_buf + offset),
                                                 *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_TEXT_OPA_ID:
                lv_obj_set_style_text_opa(obj, *(int32_t *)(style_buf + offset),
                                          *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_TEXT_DECOR_ID:
                lv_obj_set_style_text_decor(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_TEXT_ALIGN_ID:
                lv_obj_set_style_text_align(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LINE_COLOR_ID:
                lv_obj_set_style_line_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LINE_WIDTH_ID:
                lv_obj_set_style_line_width(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LINE_OPA_ID:
                lv_obj_set_style_line_opa(obj, *(int32_t *)(style_buf + offset),
                                          *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LINE_ROUNDED_ID:
                lv_obj_set_style_line_rounded(obj, *(int32_t *)(style_buf + offset),
                                              *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_LENGTH_ID:
                lv_obj_set_style_length(obj, *(int32_t *)(style_buf + offset),
                                        *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_ARC_COLOR_ID:
                lv_obj_set_style_arc_color(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                           *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_ARC_WIDTH_ID:
                lv_obj_set_style_arc_width(obj, *(int32_t *)(style_buf + offset),
                                           *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_ARC_ROUNDED_ID:
                lv_obj_set_style_arc_rounded(obj, *(int32_t *)(style_buf + offset),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_IMAGE_RECOLOR_ID:
                lv_obj_set_style_image_recolor(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                             *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_IMAGE_RECOLOR_OPA_ID:
                lv_obj_set_style_image_recolor_opa(obj, *(int32_t *)(style_buf + offset),
                                                 *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_IMAGE_OPA_ID:
                lv_obj_set_style_image_opa(obj, *(int32_t *)(style_buf + offset),
                                         *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_MAX_HEIGHT_ID:
                lv_obj_set_style_max_height(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_IMG_SRC_ID: {
                    int32_t img_id = *(int32_t *)(style_buf + offset);
                    lv_obj_set_style_bg_image_src(obj, res_get(page_id, img_id),
                                                *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                    offset += 2 * sizeof(int32_t);
                }
                break;
            case LV_OBJ_SET_STYLE_BG_IMG_OPA_ID:
                lv_obj_set_style_bg_image_opa(obj, *(int32_t *)(style_buf + offset),
                                            *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_IMG_RECOLOR_ID:
                lv_obj_set_style_bg_image_recolor(obj, lv_color_hex(*(int32_t *)(style_buf + offset)),
                                               *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_BG_IMG_RECOLOR_OPA_ID:
                lv_obj_set_style_bg_image_recolor_opa(obj, *(int32_t *)(style_buf + offset),
                                                    *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_STYLE_ARC_IMG_SRC_ID: {
                    int32_t img_id = *(int32_t *)(style_buf + offset);
                    lv_obj_set_style_arc_image_src(obj, res_get(page_id, img_id),
                                                 *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                    offset += 2 * sizeof(int32_t);
                }
                break;
            case LV_OBJ_SET_STYLE_ARC_OPA_ID:
                lv_obj_set_style_arc_opa(obj, *(int32_t *)(style_buf + offset),
                                         *(int32_t *)(style_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            default:
                LV_LOG_WARN("style_set failed %d", style_id);
                break;
        }
        if(temp_offset == offset) {
            break;
        }
    }
}

#endif /*LV_USE_GUI_PARSE_DYN*/
