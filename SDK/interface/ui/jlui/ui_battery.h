/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_battery.h
 *
 * @brief 杰理UI电池电量控件API
 *
 * @author
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_BATTERY_WIDGET_H__
#define __UI_BATTERY_WIDGET_H__


#include "jlui/control.h"
#include "list.h"



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 电池控件句柄结构
 */
/* ------------------------------------------------------------------------------------*/
struct ui_battery {
    struct element elm;
    int src;	// 当前显示的图片ID
    u8 index;	// 当前显示的图片索引
    struct ui_image_list *charge_image;	// 充电状态图片列表
    struct ui_image_list *normal_image;	// 电量状态图片列表
    struct list_head entry;		// 电量控件链表，用于把所有电量控件串联起来，一个控件更新，则所有控件一起更新
    const struct ui_battery_info *info;
    const struct element_event_handler *handler;
};	// 96byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取电池电量控件句柄
 */
/* ------------------------------------------------------------------------------------*/
#define ui_battery_for_id(id) \
	ui_element_for_id(id, struct ui_battery)



/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_battery_enable 电池控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_battery_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_battery_level_change 设置所有电池控件状态
 *
 * @param persent 电量百分比
 * @param incharge 是否正在充电（true 充电状态，false 显示电量百分比）
 */
/* ------------------------------------------------------------------------------------*/
void ui_battery_level_change(int persent, int incharge);//改变所有电池控件


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_battery_set_level_by_id 设置指定电池控件状态（带redraw）
 *
 * @param id 电池控件ID
 * @param persent 电量百分比
 * @param incharge 是否正在充电
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_battery_set_level_by_id(int id, int persent, int incharge);//修改指定id


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_battery_set_level 设置指点电池控件状态（不带redraw）
 *
 * @param battery 电池控件句柄
 * @param persent 电量百分比
 * @param incharge 充电状态
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_battery_set_level(struct ui_battery *battery, int persent, int incharge);//初始化使用


#endif


