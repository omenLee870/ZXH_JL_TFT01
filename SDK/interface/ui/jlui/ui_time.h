/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/*
 * @file ui_time.h
 *
 * @brief 时间控件API接口头文件
 *
 * @author
 *
 * @version V201
 *
 * @date 2022-12-15
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_TIME_WIDGET_H__
#define __UI_TIME_WIDGET_H__


#include "jlui/control.h"
#include "jlui/ui_core.h"
#include "jlui/p.h"


struct utime {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 时间控件句柄结构定义
 */
/* ------------------------------------------------------------------------------------*/
struct ui_time {
    struct element_text text;
    char source[9];	// 数据源
    // 8byte
    u8 css_num;		// css属性数量，可用2bit，最大2，与UI资源保持一致

    u16 year: 12;	// 年，最大 4095
    u16 month: 4;	// 月，最大 15
    u8 day;		// 日，可用 5bit，最大到 31
    u8 hour;	// 时，可用 5bit，最大到 31
    // 4byte

    u8 min;		// 分，可用 6bit，最大到 63
    u8 sec;		// 秒，可用 6bit，最大到 63

    // 4byte
    u16 timer;		// 定时器ID
    u8 auto_cnt;	// 自动计时标志，可用1bit，是/否
    // 空2byte

    u32 css[2];		// CSS属性指针
    // 8byte

    u16 color;		// 颜色，使用RGB565，与UI资源保持一致
    u16 hi_color;	// 高亮颜色，使用RGB565，与UI资源保持一致
    // 4byte

    u16 buf[20];	// 显示的图片索引	2024/06/07 11:25:32 --> 最多需19个图片，还需 0xff结尾
    // 40byte

    const struct ui_time_info *info;
    const struct element_event_handler *handler;
};	// 164byte



/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过ID获取时间控件句柄
 */
/* ------------------------------------------------------------------------------------*/
#define ui_time_for_id(id) \
	(struct ui_time *)ui_core_get_element_by_id(id)
// ui_element_for_id(id, struct ui_time)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_time_enable 时间控件使能（内部调用）
 */
/* ------------------------------------------------------------------------------------*/
void ui_time_enable();


/* ------------------------------------------------------------------------------------*/
/**
 * @brief new_ui_time 创建时间控件（内部调用）
 *
 * @param _info 时间控件信息
 * @param parent 父控件句柄
 *
 * @return 时间控件句柄
 */
/* ------------------------------------------------------------------------------------*/
void *new_ui_time(const void *_info, struct element *parent);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_time_update 刷新时间控件的时间（不带redraw）
 *
 * @param time 时间控件句柄
 * @param t 待显示的时间
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_time_update(struct ui_time *time, struct utime *t);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_time_update_by_id 设置时间控件显示的时间（带redraw）
 *
 * @param id 时间控件ID
 * @param time 待显示的时间
 *
 * @return 0 正常，-22 控件不存在
 */
/* ------------------------------------------------------------------------------------*/
int ui_time_update_by_id(int id, struct utime *time);



#endif
