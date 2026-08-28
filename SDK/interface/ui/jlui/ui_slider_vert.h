/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_slider_vert.h
 *
 * @brief 杰理UI垂直进度条控件API
 *
 * @author zhuhaifang@zh-jieli.com
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_SLIDER_VERT_WIDGET_H__
#define __UI_SLIDER_VERT_WIDGET_H__


#include "jlui/ui_core.h"
#include "jlui/control.h"



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 定义垂直进度条控件子控件数量
 */
/* ------------------------------------------------------------------------------------*/
#define VSLIDER_CHILD_NUM     (VSLIDER_CHILD_END - VSLIDER_CHILD_BEGIN)


struct vslider_text_info {
    u8 move;
    int min_value;
    int max_value;
    int text_color;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 垂直进度条控件句柄结构定义
 */
/* ------------------------------------------------------------------------------------*/
struct ui_vslider {
    struct element elm;
    struct element child_elm[VSLIDER_CHILD_NUM];

    u8 move: 1;	// 滑动使能标志
    u8 step: 7;	// 步进

    u8 follow_pic: 1;
    u8 persent: 7;	// 百分比

    s16 top;	// selected的top
    s16 height;	// selected的height
    s16 min_value;	// 最小值
    s16 max_value;	// 最大值
    const struct ui_slider_info *info;
    const struct element_event_handler *handler;
};	// 340byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取垂直进度条控件句柄
 */
/* ------------------------------------------------------------------------------------*/
#define ui_vslider_for_id(id) \
	ui_element_for_id(id, struct ui_vslider)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_vslider_enable 垂直进度条控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_vslider_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_vslider_set_persent_by_id 设置垂直进度条百分比（待redraw）
 *
 * @param id 垂直进度条控件ID
 * @param persent 垂直进度条控件显示百分比
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_vslider_set_persent_by_id(int id, int persent);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_vslider_set_persent 设置垂直进度条控件百分比（不带redraw）
 *
 * @param vslider 垂直进度条控件
 * @param persent 垂直进度条控件显示百分比
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_vslider_set_persent(struct ui_vslider *vslider, int persent);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief vslider_touch_slider_move 垂直进度条控件划动回调（如果应用层ON_TOUCH_MOVE没有return true，并且把vslider->move设置为true，底层会自动调用该接口）
 *
 * @param vslider 垂直进度条控件句柄
 * @param e 触摸事件
 *
 * @return false百分比无变化，true百分比改变并已经redraw
 */
/* ------------------------------------------------------------------------------------*/
int vslider_touch_slider_move(struct ui_vslider *vslider, struct element_touch_event *e);//触摸滑动功能


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_vslider_set_follow_pic 设置高亮和非高亮部分是否跟随滑块中心
 *
 * @Params vslider 进度条控件
 * @Params follow_pic 是否跟随滑块中心 true 跟随，false 不跟随
 *
 * @return true 跟随，false 不跟随
 */
/* ------------------------------------------------------------------------------------*/
int  ui_vslider_set_follow_pic(struct ui_vslider *vslider, u8 follow_pic);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief vslider_get_percent 获取垂直进度条百分比
 *
 * @param vslider 垂直进度条控件句柄
 *
 * @return 垂直进度条当前百分比
 */
/* ------------------------------------------------------------------------------------*/
int vslider_get_percent(struct ui_vslider *vslider);




#endif

