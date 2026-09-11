/**
 * @file lv_port_disp_templ.h
 *
 */

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../../lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(void);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

void lv_port_disp_draw_lcd(const lv_area_t *area, uint8_t *px_map);

u8 lv_jl_lcd_get_need_gxdma(void);
u8 lv_jl_lcd_get_need_gpu_link_dbi(void);

void lv_jlgpu_sync_full_area(lv_display_t *disp_refr);

/* 等待 LIST==2 异步渲染线程(_jlgpu_run_disp_cb)排空在飞的渲染任务。
 * 释放被渲染引用过的显示 buffer 前调用: 先删除/停用相关控件, 再调用本函数,
 * 返回后不再有渲染引用, 可安全释放 buffer 或拆显示通路。 */
void lv_jlgpu_run_disp_drain(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
