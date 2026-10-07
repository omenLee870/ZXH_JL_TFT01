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
#include "event.h"
#include "prop.h"
#include "style.h"

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

lv_obj_t * control_parse(char * buf, int32_t size, lv_obj_t * parent, int32_t page_id)
{
    if(buf == NULL || size <= 0) {
        return NULL;
    }

    // 解析 JLDynControl
    JLDynControl * control = (JLDynControl *)buf;
    lv_obj_t * obj = control_gen(buf, size, parent); // 创建控件
    if(obj == NULL) {
        LV_LOG_WARN("control_gen failed");
        return NULL;
    }

    prop_set(buf, size, obj, page_id);  // 设置控件属性
    style_set(buf, size, obj, page_id); // 设置控件样式
    event_set(buf, size, obj, page_id); // 设置控件事件
    bind_set(buf, size, obj, page_id);  // 设置控件绑定
    // 递归创建子控件
    int32_t offset = control->children_offset + control->children_cnt * sizeof(int32_t);
    int32_t children_size = control->prop_offset - control->children_offset - control->children_cnt * sizeof(int32_t);
    int32_t children_idx = 0;
    while(offset < children_size) {
        if(children_idx < control->children_cnt - 1) {
            int32_t child_size =
                *(int32_t *)(buf + control->children_offset + (children_idx + 1) * sizeof(int32_t)) - offset;
            lv_obj_t * child = control_parse(buf + offset, child_size, obj, page_id);
            offset += child_size;
            children_idx++;
        }
        else {
            int32_t child_size = size - offset;
            lv_obj_t * child = control_parse(buf + offset, child_size, obj, page_id);
            offset += child_size;
        }
    }

    return obj;
}

lv_obj_t * control_gen(char * buf, int32_t size, lv_obj_t * parent)
{
    if(buf == NULL || size <= 0) {
        LV_LOG_WARN("control_gen failed");
        return NULL;
    }

    JLDynControl * control = (JLDynControl *)buf;

    lv_obj_t * obj = NULL;
    switch(control->type) {
        case TYPE_OBJ_ID:
            obj = lv_obj_create(parent);
            break;
        case TYPE_LABEL_ID:
            obj = lv_label_create(parent);
            break;
        case TYPE_BTN_ID:
            obj = lv_btn_create(parent);
            break;
        case TYPE_IMG_ID:
            obj = lv_image_create(parent);
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
            obj = lv_imglist_create(parent);
            break;
        case TYPE_LINE_ID:
            obj = lv_line_create(parent);
            break;
        default:
            LV_LOG_WARN("control_gen failed %d", control->type);
            break;
    }

    return obj;
}

#endif /*LV_USE_GUI_PARSE_DYN*/
