/**
 * @file bind.h
 *
 */

#ifndef BIND_H
#define BIND_H

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
 *     TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/** 应用一个绑定项，payload 基础格式为 mode + model_id；部分绑定会追加控件私有参数或注册控件事件回写模型。 */
bool bind_set_item(uint32_t bind_id, dyn_view_t payload, lv_obj_t *obj, int32_t page_id);

void bind_set_send_cb(gui_bind_send_cb_t cb);

void bind_set_get_cb(gui_bind_get_cb_t cb);

void bind_set_get_subject_cb(gui_bind_get_subject_cb_t cb);

void bind_set_get_data_cb(gui_bind_get_data_cb_t cb);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*BIND_H*/
