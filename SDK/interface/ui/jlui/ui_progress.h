/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/*
 * @file ui_progress.h
 *
 * @brief 圆弧进度条控件API接口头文件
 *
 * @author
 *
 * @version V201
 *
 * @date 2022-12-15
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_PROGRESS_WIDGET_H__
#define __UI_PROGRESS_WIDGET_H__


#include "jlui/control.h"
#include "jlui/ui_core.h"



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 圆弧进度条控件子控件数量定义
 */
/* ------------------------------------------------------------------------------------*/
#define PROGRESS_CHILD_NUM     (CTRL_PROGRESS_CHILD_END - CTRL_PROGRESS_CHILD_BEGIN)


struct progress_highlight_info {
    struct ui_ctrl_info_head head;
    s16 center_x;		// 旋转中心
    s16 center_y;
    u16 radius_big;		// 圆环大半径
    u16 radius_small;	// 圆环小半径
    u16 angle_begin;	// 开始角度
    u16 angle_end;		// 结束角度
    struct ui_image_list *img;
    u16 color;			// 颜色
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 圆弧进度条控件句柄结构定义
 */
/* ------------------------------------------------------------------------------------*/
struct ui_progress {
    struct element elm;
    struct element child_elm[PROGRESS_CHILD_NUM];	// 子控件个数 1
    char source[9];	// 数据源

    s16 center_x;	// 旋转中心x坐标
    s16 center_y;	// 旋转中心y坐标

    u16 radius;		// 半径
    u16 angle_begin;// 起始角度
    u16 angle_end;	// 结束角度

    u8 ctrl_num;	// 控件数量
    char percent;	// 百分比
    const struct layout_info *info;
    const struct progress_highlight_info *pic_info[PROGRESS_CHILD_NUM];
    const struct element_event_handler *handler;
};	// 160byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取圆弧进度条控件句柄
 */
/* ------------------------------------------------------------------------------------*/
#define ui_progress_for_id(id) \
	ui_element_for_id(id, struct ui_progress)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_progress_enable 圆弧进度条控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_progress_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_progress_set_persent_by_id 设置圆弧进度条百分比（带redraw）
 *
 * @param id 圆弧进度条控件ID
 * @param persent 圆弧进度条显示的百分比
 *
 * @return 0 正常，-22控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_progress_set_persent_by_id(int id, int persent);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_progress_set_persent 设置圆弧进度条百分比（不带redraw）
 *
 * @param progress 圆弧进度条句柄
 * @param percent 圆弧进度条显示的百分比
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_progress_set_persent(struct ui_progress *progress, int percent);



#endif


