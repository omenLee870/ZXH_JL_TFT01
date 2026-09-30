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

/* 本帧是否按脏区收窄 GPU 输出窗口(走 TOP->GXDMA 且没有 JSTC/GPU-DBI 联动时为真)。
 * 为真时驱动自己做脏区同步、窗口只覆盖脏区, LVGL 那套同步/整屏渲染都要让开。 */
u8 lv_jl_lcd_get_partial_render(void);

/* 把上一帧的真实脏区并进本帧的 bbox, 并记下本帧的真实脏区供下一帧用。
 * lv_refr.c 合并完本帧脏区后调它, 再调 lv_jl_lcd_align_render_area() 对齐。 */
void lv_jl_lcd_join_prev_dirty(lv_area_t *dirty);

/* 把脏区扩成"映射到物理后落点正好 tile 对齐"的逻辑矩形(按旋转模式做映射, 非旋转时不动)。
 * lv_refr.c 把脏区并成 bbox 后调它, 保证驱动用的渲染窗口和 LVGL 实际渲染的区域一致。 */
void lv_jl_lcd_align_render_area(lv_area_t *area);

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
