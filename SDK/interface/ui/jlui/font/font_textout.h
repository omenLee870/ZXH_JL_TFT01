#ifndef __FONT_OUT_H__
#define __FONT_OUT_H__

#include "generic/typedef.h"
#include "font/font_all.h"
#include "font/font_unic.h"

/**
 * @brief  打开字库
 *
 * @param info:字库信息
 * @param language:语言id
 *
 * @returns   TRUE:打开成功 FALSE:打开失败
 */
struct font_info *font_open(struct font_info *info, u8 language);


/**
 * @brief  关闭字库,释放相关资源
 *
 * @param info
 */
void font_close(struct font_info *info);


/**
 * @brief 获取内码字符宽度
 *
 * @param info
 * @param str
 * @param strlen
 *
 * @returns 字符串宽度
 */
u16 font_text_width(struct font_info *info, u8 *str, u16 strlen);


/**
 * @brief 获取unicode编码字符宽度
 *
 * @param info
 * @param str
 * @param strlen
 *
 * @returns 字符串宽度
 */
u16 font_textw_width(struct font_info *info, u8 *str, u16 strlen);


/**
 * @brief 获取utf8编码字符宽度
 *
 * @param info
 * @param str
 * @param strlen
 *
 * @returns 字符串宽度
 */
u16 font_textu_width(struct font_info *info, u8 *str, u16 strlen);


/**
 * @brief 设置字库语言id
 *
 * @param language
 *
 * @returns
 */
void font_lang_set(int language);


/**
 * @brief 获取字库语言id *
 *
 * @returns 字库语言id
 */
int font_lang_get();


// 字库换行方式 value == 1:换行时考虑单词整体性, value == 0:强制换行; 默认为1
void font_smart_line_break_set(u8 value);


/**
 * @brief  字库内码显示接口
 *
 * @param info
 * @param str
 * @param strlen
 *
 * @returns 实际显示的编码长度
 */
u16 font_textout(struct font_info *info, u8 *str, u16 strlen, u16 x, u16 y);


/**
 * @brief  字库unicode显示接口
 *
 * @param info
 * @param str
 * @param strlen
 * @param x 显示起始x坐标
 * @param y 显示起始Y坐标
 *
 * @returns 实际显示的编码长度
 */
u16 font_textout_unicode(struct font_info *info, u8 *str, u16 strlen, u16 x, u16 y);


/**
 * @brief  字库utf8显示接口
 *
 * @param info
 * @param str
 * @param strlen
 * @param x 显示起始x坐标
 * @param y 显示起始Y坐标
 *
 * @returns 实际显示的编码长度
 */
u16 font_textout_utf8(struct font_info *info, u8 *str, u16 strlen, u16 x, u16 y);


/**
 * @brief  utf8转内码
 *
 * @param info
 * @param utf8
 * @param utf8len
 * @param ansi
 *
 * @returns 内码长度
 */
u16 font_utf8toansi(struct font_info *info, u8 *utf8, u16 utf8len, u8 *ansi);


/**
 * @brief utf16转内码
 *
 * @param info
 * @param utf
 * @param len
 * @param ansi
 *
 * @returns 内码长度
 */
u16 font_utf16toansi(struct font_info *info, u8 *utf, u16 len, u8 *ansi);


/**
 * @brief utf8转utf16
 *
 * @param info
 * @param utf8
 * @param utf8len
 * @param utf16
 *
 * @returns utf16长度
 */
u16 font_utf8toutf16(struct font_info *info, u8 *utf8, u16 utf8len, u16 *utf16);


/**
 * @brief  字库层malloc接口，弱函数，用户可自定义
 *
 * @param size 申请大小(byte)
 * @returns buf地址
 */
void *font_malloc(size_t size);


/**
 * @brief  字库层zalloc接口，弱函数，用户可自定义
 *
 * @param size 申请大小(byte)
 * @returns buf地址
 */
void *font_zalloc(size_t size);


/**
 * @brief  字库层free接口，弱函数，用户可自定义
 *
 * @param pv 待释放内存地址
 */
void font_free(void *pv);


/**
 * @brief 字库多行显示时用于记录每行宽度信息，多行居中的功能需要
 *
 * @param info 字库结构体
 */
int font_create_each_line_width_info(struct font_info *info);


/**
 * @brief 字库释放多行信息
 *
 * @param info 字库结构体
 */
void font_release_each_line_width_info(struct font_info *info);


#endif
