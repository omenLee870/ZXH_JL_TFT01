/**
 * @file control.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "control.h"

#if LV_USE_GUI_PARSE_DYN != 0

#include "bind.h"
#include "dyn_reader.h"
#include "event.h"
#include "prop.h"
#include "style.h"

/*********************
 *      DEFINES
 *********************/
#define CONTROL_SUPPORT_FROM_LV_USE(flag) ((flag) ? CONTROL_SUPPORT_AVAILABLE : CONTROL_SUPPORT_DISABLED)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool parse_block_items(dyn_block_view_t block, lv_obj_t *obj, int32_t page_id);
static bool padding_tail_is_zero(const uint8_t *p, const uint8_t *end);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool control_apply_blocks(dyn_view_t blocks, uint8_t block_count, lv_obj_t *obj, int32_t page_id)
{
    if (obj == NULL) {
        return false;
    }

    const uint8_t *p = blocks.ptr;
    const uint8_t *end = blocks.ptr + blocks.len;
    for (uint8_t i = 0; i < block_count; i++) {
        dyn_block_view_t block;
        if (!dyn_read_block(&p, end, &block)) {
            LV_LOG_WARN("invalid control block");
            return false;
        }

        if (!parse_block_items(block, obj, page_id)) {
            return false;
        }
    }

    if (!padding_tail_is_zero(p, end)) {
        LV_LOG_WARN("invalid control record padding");
        return false;
    }
    return true;
}

control_support_t control_get_support(uint8_t control_type)
{
    switch (control_type) {
    case TYPE_OBJ_ID:
        return CONTROL_SUPPORT_AVAILABLE;
    case TYPE_LABEL_ID:
    case TYPE_DIGITCLOCK_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_LABEL);
    case TYPE_BTN_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_BUTTON);
    case TYPE_IMG_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_IMAGE);
    case TYPE_IMGLIST_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_IMGLIST);
    case TYPE_LINE_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_LINE);
    case TYPE_ANALOGCLOCK_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_SCALE);
    case TYPE_ANIMIMG_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_ANIMIMG);
    case TYPE_BAR_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_BAR);
    case TYPE_SLIDER_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_SLIDER);
    case TYPE_SPINNER_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_SPINNER);
    case TYPE_CHECKBOX_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_CHECKBOX);
    case TYPE_SWITCH_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_SWITCH);
    case TYPE_LED_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_LED);
    case TYPE_DDLIST_ID:
        return CONTROL_SUPPORT_FROM_LV_USE(LV_USE_DROPDOWN);
    default:
        return CONTROL_SUPPORT_UNKNOWN;
    }
}

lv_obj_t *control_gen(uint8_t control_type, dyn_view_t blocks, lv_obj_t *parent)
{
    (void)blocks;
    lv_obj_t *obj = NULL;
    switch (control_type) {
    case TYPE_OBJ_ID:
        obj = lv_obj_create(parent);
        break;
    case TYPE_LABEL_ID:
#if LV_USE_LABEL
        obj = lv_label_create(parent);
#else
        LV_LOG_WARN("LV_USE_LABEL is disabled");
#endif
        break;
    case TYPE_BTN_ID:
#if LV_USE_BUTTON
        obj = lv_btn_create(parent);
#else
        LV_LOG_WARN("LV_USE_BUTTON is disabled");
#endif
        break;
    case TYPE_IMG_ID:
#if LV_USE_IMAGE
        obj = lv_image_create(parent);
#else
        LV_LOG_WARN("LV_USE_IMAGE is disabled");
#endif
        break;
    case TYPE_ANALOGCLOCK_ID:
#if LV_USE_SCALE
        obj = lv_scale_create(parent);
        lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
        lv_scale_set_angle_range(obj, 360U);
        lv_scale_set_range(obj, 0, 720);
        lv_scale_set_rotation(obj, 270U);
        lv_scale_set_total_tick_count(obj, 61);
        lv_scale_set_major_tick_every(obj, 5);
#else
        LV_LOG_WARN("LV_USE_SCALE is disabled");
#endif
        break;
    case TYPE_IMGLIST_ID:
#if LV_USE_IMGLIST
        obj = lv_imglist_create(parent);
#else
        LV_LOG_WARN("LV_USE_IMGLIST is disabled");
#endif
        break;
    case TYPE_LINE_ID:
#if LV_USE_LINE
        obj = lv_line_create(parent);
#else
        LV_LOG_WARN("LV_USE_LINE is disabled");
#endif
        break;
    case TYPE_ANIMIMG_ID:
#if LV_USE_ANIMIMG
        obj = lv_animimg_create(parent);
#else
        LV_LOG_WARN("LV_USE_ANIMIMG is disabled");
#endif
        break;
    case TYPE_BAR_ID:
#if LV_USE_BAR
        obj = lv_bar_create(parent);
#else
        LV_LOG_WARN("LV_USE_BAR is disabled");
#endif
        break;
    case TYPE_SLIDER_ID:
#if LV_USE_SLIDER
        obj = lv_slider_create(parent);
#else
        LV_LOG_WARN("LV_USE_SLIDER is disabled");
#endif
        break;
    case TYPE_SPINNER_ID:
#if LV_USE_SPINNER
        obj = lv_spinner_create(parent);
#else
        LV_LOG_WARN("LV_USE_SPINNER is disabled");
#endif
        break;
    case TYPE_DIGITCLOCK_ID:
#if LV_USE_LABEL
        obj = lv_label_create(parent);
#else
        LV_LOG_WARN("LV_USE_LABEL is disabled");
#endif
        break;
    case TYPE_CHECKBOX_ID:
#if LV_USE_CHECKBOX
        obj = lv_checkbox_create(parent);
#else
        LV_LOG_WARN("LV_USE_CHECKBOX is disabled");
#endif
        break;
    case TYPE_SWITCH_ID:
#if LV_USE_SWITCH
        obj = lv_switch_create(parent);
#else
        LV_LOG_WARN("LV_USE_SWITCH is disabled");
#endif
        break;
    case TYPE_LED_ID:
#if LV_USE_LED
        obj = lv_led_create(parent);
#else
        LV_LOG_WARN("LV_USE_LED is disabled");
#endif
        break;
    case TYPE_DDLIST_ID:
#if LV_USE_DROPDOWN
        obj = lv_dropdown_create(parent);
#else
        LV_LOG_WARN("LV_USE_DROPDOWN is disabled");
#endif
        break;

    default:
        LV_LOG_WARN("control_gen failed %d", control_type);
        break;
    }

    return obj;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool parse_block_items(dyn_block_view_t block, lv_obj_t *obj, int32_t page_id)
{
    /* 未知 block 只按 block_len 跳过，不假设 payload 仍是 item TLV 流。 */
    if (!dyn_block_has_item_stream(block.block_type)) {
        return true;
    }

    const uint8_t *p = block.items.ptr;
    const uint8_t *end = block.items.ptr + block.items.len;
    for (uint16_t i = 0; i < block.item_count; i++) {
        dyn_item_view_t item;
        if (!dyn_read_item(&p, end, &item)) {
            LV_LOG_WARN("invalid block item");
            return false;
        }

        /* block_type 决定 item_id 的命名空间：属性、样式、事件和绑定分别交给对应模块处理。 */
        switch (block.block_type) {
        case JLDYN_CONTROL_BLOCK_PROP:
            if (!prop_set_item(item.item_id, item.payload, obj, page_id)) {
                return false;
            }
            break;
        case JLDYN_CONTROL_BLOCK_STYLE:
            if (!style_set_item(item.item_id, item.payload, obj, page_id)) {
                return false;
            }
            break;
        case JLDYN_CONTROL_BLOCK_EVENT:
            if (!event_set_item(item.item_id, item.payload, obj, page_id)) {
                return false;
            }
            break;
        case JLDYN_CONTROL_BLOCK_BIND:
            if (!bind_set_item(item.item_id, item.payload, obj, page_id)) {
                return false;
            }
            break;
        default:
            break;
        }
    }

    if (p != end) {
        LV_LOG_WARN("block item_count mismatch");
        return false;
    }
    return true;
}

static bool padding_tail_is_zero(const uint8_t *p, const uint8_t *end)
{
    if (p > end || (end - p) > 3) {
        return false;
    }

    while (p < end) {
        if (*p++ != 0) {
            return false;
        }
    }
    return true;
}

#endif /*LV_USE_GUI_PARSE_DYN*/
