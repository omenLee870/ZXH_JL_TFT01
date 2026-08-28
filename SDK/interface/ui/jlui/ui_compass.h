/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_compass.h
 *
 * @brief 杰理UI指南针控件API
 *
 * @author
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_COMPASS_WIDGET_H__
#define __UI_COMPASS_WIDGET_H__


#include "jlui/control.h"
#include "jlui/ui_core.h"


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 指南针控件的子控件数量
 */
/* ------------------------------------------------------------------------------------*/
#define COMPASS_CHILD_NUM     (CTRL_COMPASS_CHILD_END - CTRL_COMPASS_CHILD_BEGIN)


struct compass_pic_info {
    struct ui_ctrl_info_head head;
    s16 cent_x;
    s16 cent_y;
    struct ui_image_list *img;
};


struct compass_css_info {
    s16 left;
    s16 top;
    s16 width;
    s16 height;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 指南针控件句柄结构
 */
/* ------------------------------------------------------------------------------------*/
struct ui_compass {
    struct element elm;
    struct element child_elm[COMPASS_CHILD_NUM];	// 子控件单元 2个
    struct compass_css_info child_css[COMPASS_CHILD_NUM];	// 子控件CSS属性 2个
    char source[9];				// 数据源
    u8 ctrl_num;				// 控件数量，从flash读进来
    u16 bk_angle;				// 背景角度
    u32 indicator_angle: 9;		// 指针角度
    u32 last_bk_angle: 9;		   // 上一次背景角度，用于判断是否要重新旋转图片进行刷新，如果当前角度和上一次角度不同才刷新
    u32 last_indicator_angle: 9;	// 上一次指针角度，同上
    const struct layout_info *info;
    const struct compass_pic_info *pic_info[COMPASS_CHILD_NUM];
    const struct element_event_handler *handler;
};	// 240byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取指南针控件句柄
 */
/* ------------------------------------------------------------------------------------*/
#define ui_compass_for_id(id) \
	ui_element_for_id(id, struct ui_compass)



/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_compass_enable 指南针控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_compass_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_compass_update 刷新指南针控件
 *
 * @param compass 指南针控件句柄
 * @param refresh 是否redraw(true redraw, false not redraw)
 */
/* ------------------------------------------------------------------------------------*/
void ui_compass_update(struct ui_compass *compass, u8 refresh);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_compass_set_angle_by_id 设置指南针角度(带redraw)
 *
 * @param id 指南针控件ID
 * @param bk_angle 背景角度
 * @param indicator_angle 指针角度
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_compass_set_angle_by_id(int id, int bk_angle, int indicator_angle);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_compass_set_angle 设置指南针角度(不带redraw)
 *
 * @param compass 指南针控件句柄
 * @param bk_angle 背景图角度
 * @param indicator_angle 指针角度
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_compass_set_angle(struct ui_compass *compass, int bk_angle, int indicator_angle);


#endif


