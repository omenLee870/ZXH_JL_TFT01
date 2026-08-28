/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file p.h
 *
 * @brief 杰理UI文本操作
 *
 * @author
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_P_TEXT_H__
#define __UI_P_TEXT_H__

#include "jlui/ui_core.h"


struct ui_str {
    const char *format;
    char *str;
};


struct element_text {
    struct element elm;               //must be first
    char *str;
    // const char *format;

    void *priv;
    u32 x_interval: 8;	// 字符间隔
    u32 format : 3;		// 编码格式，与UI资源同步
    u32 unused : 5;		// 5bit暂未使用
    u32 color: 16;		// 使用RGB565，这里多8byte保留
    const struct element_event_handler *handler;
};


void text_element_set_text(struct element_text *text, char *str,
                           int format, int color);


void text_element_init(struct element_text *text, int id, u8 page, u8 prj, u8 css_num,
                       const struct element_css1 *css
                      );


void text_element_set_event_handler(struct element_text *text, void *priv,
                                    const struct element_event_handler *handler);






#endif

