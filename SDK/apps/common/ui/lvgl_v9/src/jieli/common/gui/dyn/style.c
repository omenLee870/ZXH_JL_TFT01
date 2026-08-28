/**
 * @file style.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "style.h"
#include "dyn_reader.h"
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
static bool read_i32(const uint8_t **p, const uint8_t *end, int32_t *out);
static bool read_value_selector_target(dyn_view_t payload, int32_t *value, int32_t *selector, int32_t *target);
static lv_obj_t *resolve_style_target(lv_obj_t *obj, int32_t target);
static bool payload_consumed(const uint8_t *p, const uint8_t *end, uint32_t style_id);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool style_set_item(uint32_t style_id, dyn_view_t payload, lv_obj_t *obj, int32_t page_id)
{
    int32_t value;
    int32_t selector;
    int32_t target_id;
    if (!read_value_selector_target(payload, &value, &selector, &target_id)) {
        LV_LOG_WARN("invalid style payload %d", (int)style_id);
        return false;
    }
    lv_obj_t *target = resolve_style_target(obj, target_id);
    if (target == NULL) {
        LV_LOG_WARN("invalid style target %d", (int)target_id);
        return true;
    }

    switch (style_id) {
    case LV_OBJ_SET_STYLE_BG_COLOR_ID:
        lv_obj_set_style_bg_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_BG_GRAD_COLOR_ID:
        lv_obj_set_style_bg_grad_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_BG_OPA_ID:
        lv_obj_set_style_bg_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BG_GRAD_DIR_ID:
        lv_obj_set_style_bg_grad_dir(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BORDER_COLOR_ID:
        lv_obj_set_style_border_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_BORDER_OPA_ID:
        lv_obj_set_style_border_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BORDER_SIDE_ID:
        lv_obj_set_style_border_side(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BORDER_WIDTH_ID:
        lv_obj_set_style_border_width(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_RADIUS_ID:
        lv_obj_set_style_radius(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_COLOR_ID:
        lv_obj_set_style_shadow_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_OFS_X_ID:
        lv_obj_set_style_shadow_offset_x(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_OFS_Y_ID:
        lv_obj_set_style_shadow_offset_y(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_SPREAD_ID:
        lv_obj_set_style_shadow_spread(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_OPA_ID:
        lv_obj_set_style_shadow_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_SHADOW_WIDTH_ID:
        lv_obj_set_style_shadow_width(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_OUTLINE_COLOR_ID:
        lv_obj_set_style_outline_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_OUTLINE_OPA_ID:
        lv_obj_set_style_outline_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_OUTLINE_WIDTH_ID:
        lv_obj_set_style_outline_width(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_OUTLINE_PAD_ID:
        lv_obj_set_style_outline_pad(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_LEFT_ID:
        lv_obj_set_style_pad_left(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_RIGHT_ID:
        lv_obj_set_style_pad_right(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_TOP_ID:
        lv_obj_set_style_pad_top(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_BOTTOM_ID:
        lv_obj_set_style_pad_bottom(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_ROW_ID:
        lv_obj_set_style_pad_row(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_COLUMN_ID:
        lv_obj_set_style_pad_column(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_PADDING_ALL_ID:
        lv_obj_set_style_pad_all(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_TEXT_ID: {
        lv_font_t *font = res_get(page_id, value);
        if (font != NULL) {
            lv_obj_set_style_text_font(target, font, selector);
        }
    }
    break;
    case LV_OBJ_SET_STYLE_TEXT_COLOR_ID:
        lv_obj_set_style_text_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_LETTER_SPACE_ID:
        lv_obj_set_style_text_letter_space(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_LINE_SPACE_ID:
        lv_obj_set_style_text_line_space(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_TEXT_OPA_ID:
        lv_obj_set_style_text_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_TEXT_DECOR_ID:
        lv_obj_set_style_text_decor(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_TEXT_ALIGN_ID:
        lv_obj_set_style_text_align(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_LINE_COLOR_ID:
        lv_obj_set_style_line_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_LINE_WIDTH_ID:
        lv_obj_set_style_line_width(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_LINE_OPA_ID:
        lv_obj_set_style_line_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_LINE_ROUNDED_ID:
        lv_obj_set_style_line_rounded(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_LENGTH_ID:
        lv_obj_set_style_length(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_ARC_COLOR_ID:
        lv_obj_set_style_arc_color(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_ARC_WIDTH_ID:
        lv_obj_set_style_arc_width(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_ARC_ROUNDED_ID:
        lv_obj_set_style_arc_rounded(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_IMAGE_RECOLOR_ID:
        lv_obj_set_style_image_recolor(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_IMAGE_RECOLOR_OPA_ID:
        lv_obj_set_style_image_recolor_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_IMAGE_OPA_ID:
        lv_obj_set_style_image_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_MAX_HEIGHT_ID:
        lv_obj_set_style_max_height(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BG_IMG_SRC_ID:
        lv_obj_set_style_bg_image_src(target, res_get(page_id, value), selector);
        break;
    case LV_OBJ_SET_STYLE_BG_IMG_OPA_ID:
        lv_obj_set_style_bg_image_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_BG_IMG_RECOLOR_ID:
        lv_obj_set_style_bg_image_recolor(target, lv_color_hex(value), selector);
        break;
    case LV_OBJ_SET_STYLE_BG_IMG_RECOLOR_OPA_ID:
        lv_obj_set_style_bg_image_recolor_opa(target, value, selector);
        break;
    case LV_OBJ_SET_STYLE_ARC_IMG_SRC_ID:
        lv_obj_set_style_arc_image_src(target, res_get(page_id, value), selector);
        break;
    case LV_OBJ_SET_STYLE_ARC_OPA_ID:
        lv_obj_set_style_arc_opa(target, value, selector);
        break;
    default:
        LV_LOG_WARN("unknown style item %d", (int)style_id);
        break;
    }
    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool read_i32(const uint8_t **p, const uint8_t *end, int32_t *out)
{
    uint32_t value;
    if (!dyn_read_le32(p, end, &value)) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static bool read_value_selector_target(dyn_view_t payload, int32_t *value, int32_t *selector, int32_t *target)
{
    const uint8_t *p = payload.ptr;
    const uint8_t *end = payload.ptr + payload.len;
    if (!read_i32(&p, end, value) || !read_i32(&p, end, selector)) {
        return false;
    }
    *target = STYLE_TARGET_MAIN;
    if (p < end && !read_i32(&p, end, target)) {
        return false;
    }
    return payload_consumed(p, end, 0);
}

static lv_obj_t *resolve_style_target(lv_obj_t *obj, int32_t target)
{
    if (target == STYLE_TARGET_MAIN) {
        return obj;
    }

#if LV_USE_DROPDOWN
    if (target == STYLE_TARGET_DROPDOWN_LIST) {
        if (!lv_obj_check_type(obj, &lv_dropdown_class)) {
            return NULL;
        }
        return lv_dropdown_get_list(obj);
    }
#endif

    return NULL;
}

static bool payload_consumed(const uint8_t *p, const uint8_t *end, uint32_t style_id)
{
    (void)style_id;
    return p == end;
}

#endif /*LV_USE_GUI_PARSE_DYN*/
