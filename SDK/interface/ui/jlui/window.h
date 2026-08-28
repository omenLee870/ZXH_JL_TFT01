/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/**
 * @file window.h
 *
 * @brief 杰理UI窗口控件API
 *
 * @author zhuhaifang@zh-jieli.com
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_WINDOW_WIDGET_H__
#define __UI_WINDOW_WIDGET_H__


#include "jlui/layer.h"
#include "jlui/ui_core.h"
#include "jlui/control.h"
#include "list.h"



/* ------------------------------------------------------------------------------------*/
/**
 * @brief window 控件句柄结构体定义
 */
/* ------------------------------------------------------------------------------------*/
struct window {
    struct element elm; 	//must be first
    u8 busy;	// 繁忙状态
    u8 hide;	// 是否隐藏
    u8 ctrl_num;	// 控件数量
    struct list_head entry;
    struct layer *layer;	// 图层
    const struct window_info *info;
    const struct element_event_handler *handler;

    void *private_data;	// 私有参数
    struct element_event_handler global_handler;	// 全局回调，作为默认回调，若无其它控件接管事件处理，默认跑这里
};	// 108 byte


extern const struct window_info *window_table;


#define REGISTER_WINDOW_EVENT_HANDLER(id) \
	REGISTER_UI_EVENT_HANDLER(id)


#define ui_window_for_id(id) \
	ui_element_for_id(id, struct window)

int window_init(int id);

int window_show(int);

int window_hide(int id);

int window_toggle(int id);

int window_ontouch(struct element_touch_event *e);

int window_onkey(struct element_key_event *e);


#endif


