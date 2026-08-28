/**
 * @file control.h
 *
 */

#ifndef CONTROL_H
#define CONTROL_H

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
 *    TYPEDEFS
 **********************/
typedef enum {
    CONTROL_SUPPORT_UNKNOWN,
    CONTROL_SUPPORT_AVAILABLE,
    CONTROL_SUPPORT_DISABLED,
} control_support_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/** 将属性、样式、事件和绑定 block 应用到对象。 */
bool control_apply_blocks(dyn_view_t blocks, uint8_t block_count, lv_obj_t *obj, int32_t page_id);

/** 查询 control_type 在当前固件中的支持状态。 */
control_support_t control_get_support(uint8_t control_type);

/** 根据 TYPE_*_ID 创建控件；部分控件会从 blocks 读取创建参数。 */
lv_obj_t *control_gen(uint8_t control_type, dyn_view_t blocks, lv_obj_t *parent);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*CONTROL_H*/
