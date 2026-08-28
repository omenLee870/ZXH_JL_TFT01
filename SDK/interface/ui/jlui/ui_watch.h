/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_watch.h
 *
 * @brief 杰理UI表盘控件API
 *
 * @author zhuhaifang@zh-jieli.com
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_WATCH_WIDGET_H__
#define __UI_WATCH_WIDGET_H__


#include "jlui/control.h"
#include "jlui/ui_core.h"



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 表盘控件子控件数量定义
 */
/* ------------------------------------------------------------------------------------*/
#define WATCH_CHILD_NUM 	(CTRL_WATCH_CHILD_END - CTRL_WATCH_CHILD_BEGIN)


struct watch_pic_info {
    struct ui_ctrl_info_head head;
    s16 cent_x;
    s16 cent_y;
    s16 dst_cent_x;
    s16 dst_cent_y;
    struct ui_image_list *img;
};

struct watch_css_info {
    s16 left;
    s16 top;
    s16 width;
    s16 height;
};



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 表盘控件句柄结构定义
 */
/* ------------------------------------------------------------------------------------*/
struct ui_watch {
    struct element elm;
    struct element child_elm[WATCH_CHILD_NUM];	// 子控件句柄 3个
    struct watch_css_info child_css[WATCH_CHILD_NUM];	// 子控件属性 3个
    char source[9];		// 数据源

    u32 ctrl_num : 5;	// 控件数量，如果与UI资源保持一致，需7bit
    u32 hour : 5;		// 时 max 31
    u32 min : 6;		// 分 max 63
    u32 sec : 6;		// 秒 max 63
    u32 msec : 10;		// 毫秒 max 1000

    const struct layout_info *info;
    const struct watch_pic_info *pic_info[WATCH_CHILD_NUM];
    const struct element_event_handler *handler;
};	// 316byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取表盘控件elm指针
 */
/* ------------------------------------------------------------------------------------*/
#define ui_watch_for_id(id) \
	ui_element_for_id(id, struct ui_watch)



/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_watch_enable 表盘控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_watch_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_watch_set_time_by_id 设置表盘控件实际（待redraw）
 *
 * @param id 表盘控件ID
 * @param hour 小时
 * @param min 分钟
 * @param sec 秒钟
 * @param msec 毫秒
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_watch_set_time_by_id(int id, int hour, int min, int sec, int msec);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_watch_set_time 设置表盘控件时间（不带redraw）
 *
 * @param watch表盘控件句柄
 * @param hour 小时
 * @param min 分钟
 * @param sec 秒钟
 * @param msec 毫秒
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_watch_set_time(struct ui_watch *watch, int hour, int min, int sec, int msec);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_watch_update 表盘控件主动刷新
 *
 * @param watch 表盘控件句柄
 * @param refresh 是否调用redraw刷新
 */
/* ------------------------------------------------------------------------------------*/
void ui_watch_update(struct ui_watch *watch, u8 refresh);



#endif


