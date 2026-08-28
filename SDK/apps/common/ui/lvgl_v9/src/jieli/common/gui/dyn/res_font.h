/**
 * @file res_font.h
 *
 */

#ifndef RES_FONT_H
#define RES_FONT_H

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
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void *res_font_parse(char *buf);

/** 按资源项边界解析动态字体资源，失败返回 NULL。 */
void *res_font_parse_view(const uint8_t *data, uint32_t len);

void res_font_free(void *buf);

bool res_font_is_fnt(char *buf);

#endif /*LV_USE_GUI_PARSE_DYN*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*RES_FONT_H*/
