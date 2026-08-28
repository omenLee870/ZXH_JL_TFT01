/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo.h
 *
 * @brief GPU硬件模块DEMO使能配置
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-03-14
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __GPU_DEMO_H__
#define __GPU_DEMO_H__

#include "app_config.h"


#if (defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE)

/* ------------------------------------------------------------------------------------*/
/**
  DEMO 宏开关，使能宏运行对应DEMO。
  注意 GPU_IMAGE_AUTO_TEST 用于纹理自动测试
 */
/* ------------------------------------------------------------------------------------*/
/* demo文件：gpu_demo_fill.c */
#define GPU_DEMO_FILL                        0 /*! 填充任务示例        */
#define GPU_DEMO_FILL_AFFINE                 0 /*! 填充仿射变换示例    */
#define GPU_DEMO_FILL_PERSPECTIVE            0 /*! 填充透视变换示例    */
#define GPU_DEMO_FILL_MASK                   0 /*! 填充 + MASK示例     */
#define GPU_DEMO_FILL_PATH                   0 /*! 填充 + PATH示例     */
#define GPU_DEMO_FILL_PATH_AFFINE            0 /*! 填充 + PATH仿射变换 */
#define GPU_DEMO_FILL_PATH_PERSPECTIVE       0 /*! 填充 + PATH透视变换 */

/* demo文件：gpu_demo_texture.c */
#define GPU_DEMO_TEXTURE                     0 /*! 纹理任务示例        */
#define GPU_DEMO_TEXTURE_AFFINE              0 /*! 纹理仿射变化示例    */
#define GPU_DEMO_TEXTURE_PERSPECTIVE         0 /*! 纹理透视变化示例    */
#define GPU_DEMO_TEXTURE_MASK			     0 /*! 纹理 + MASK示例     */
#define GPU_DEMO_TEXTURE_PATH			     0 /*! 纹理 + PATH示例	   */
#define GPU_DEMO_TEXTURE_PATH_AFFINE	     0 /*! 纹理 + PATH仿射变换 */
#define GPU_DEMO_TEXTURE_PATH_PERSPECTIVE    0 /*! 纹理 + PATH透视变换 */

/* demo文件：gpu_demo_lineargrad.c */
#define GPU_DEMO_LINEGRAD                    0 /*! 线性渐变任务示例      */
#define GPU_DEMO_LINEGRAD_AFFINE             0 /*! 线性渐变仿射变换      */
#define GPU_DEMO_LINEGRAD_PERSPECTIVE        0 /*! 线性渐变透视变换      */
#define GPU_DEMO_LINEGRAD_MASK				 0 /*! 线性渐变+MASK示例     */
#define GPU_DEMO_LINEGRAD_PATH				 0 /*! 线性渐变+PATH示例     */
#define GPU_DEMO_LINEGRAD_PATH_AFFINE		 0 /*! 线性渐变+PATH仿射变换 */
#define GPU_DEMO_LINEGRAD_PATH_PERSPECTIVE	 0 /*! 线性渐变+PATH透视变换 */

/* demo文件：gpu_demo_radialgrad.c */
#define GPU_DEMO_RADIALGRAD	                 0 /*! 径向渐变任务示例      */
#define GPU_DEMO_RADIALGRAD_AFFINE           0 /*! 径向渐变仿射变换      */
#define GPU_DEMO_RADIALGRAD_PERSPECTIVE	     0 /*! 径向渐变透视变换      */
#define GPU_DEMO_RADIALGRAD_MASK			 0 /*! 径向渐变+MASK示例     */
#define GPU_DEMO_RADIALGRAD_PATH			 0 /*! 径向渐变+PATH示例     */
#define GPU_DEMO_RADIALGRAD_PATH_AFFINE		 0 /*! 径向渐变+PATH仿射变换 */
#define GPU_DEMO_RADIALGRAD_PATH_PERSPECTIVE 0 /*! 径向渐变+PATH透视变换 */

/* demo文件：gpu_demo_roundgrad.c */
#define GPU_DEMO_ROUNDGRAD                   0 /*! 圆角渐变任务示例      */
#define GPU_DEMO_ROUNDGRAD_AFFINE            0 /*! 圆角渐变仿射变换      */
#define GPU_DEMO_ROUNDGRAD_PERSPECTIVE       0 /*! 圆角渐变透视变换      */
#define GPU_DEMO_ROUNDGRAD_MASK				 0 /*! 圆角渐变+MASK示例     */
#define GPU_DEMO_ROUNDGRAD_PATH				 0 /*! 圆角渐变+PATH示例     */
#define GPU_DEMO_ROUNDGRAD_PATH_AFFINE		 0 /*! 圆角渐变+PATH仿射变换 */
#define GPU_DEMO_ROUNDGRAD_PATH_PERSPECTIVE	 0 /*! 圆角渐变+PATH透视变换 */

/* demo文件：gpu_demo_ringgrad.c */
#define GPU_DEMO_RINGGRAD                    0 /*! 圆环渐变任务示例      */
#define GPU_DEMO_RINGGRAD_AFFINE             0 /*! 圆环渐变仿射变换      */
#define GPU_DEMO_RINGGRAD_PERSPECTIVE        0 /*! 圆环渐变透视变换      */
#define GPU_DEMO_RINGGRAD_MASK				 0 /*! 圆环渐变+MASK示例     */
#define GPU_DEMO_RINGGRAD_PATH				 0 /*! 圆环渐变+PATH示例     */
#define GPU_DEMO_RINGGRAD_PATH_AFFINE		 0 /*! 圆环渐变+PATH仿射变换 */
#define GPU_DEMO_RINGGRAD_PATH_PERSPECTIVE	 0 /*! 圆环渐变+PATH透视变换 */

/* demo文件：gpu_demo_anglegrad.c */
#define GPU_DEMO_ANGLEGRAD                   0 /*! 角度渐变任务示例      */
#define GPU_DEMO_ANGLEGRAD_AFFINE            0 /*! 角度渐变仿射变换      */
#define GPU_DEMO_ANGLEGRAD_PERSPECTIVE       0 /*! 角度渐变透视变换      */
#define GPU_DEMO_ANGLEGRAD_MASK				 0 /*! 角度渐变+MASK示例     */
#define GPU_DEMO_ANGLEGRAD_PATH				 0 /*! 角度渐变+PATH示例     */
#define GPU_DEMO_ANGLEGRAD_PATH_AFFINE		 0 /*! 角度渐变+PATH仿射变换 */
#define GPU_DEMO_ANGLEGRAD_PATH_PERSPECTIVE	 0 /*! 角度渐变+PATH透视变换 */

/* demo文件：gpu_demo_linear_radial_grad.c */
#define GPU_DEMO_LINEAR_RADIAL_GRAD          0 /*! 线性+径向渐变示例      */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_AFF      0 /*! 线性+径向仿射变换      */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_PER      0 /*! 线性+径向透视变换      */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_MASK     0 /*! 线性+径向+MASK示例     */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_PATH     0 /*! 线性+径向+PATH示例     */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_AFF 0 /*! 线性+径向+PATH仿射变换 */
#define GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_PER 0 /*! 线性+径向+PATH透视变换 */

/* demo文件：gpu_demo_radial_angle_grad.c */
#define GPU_DEMO_RADIAL_ANGLE_GRAD           0 /*! 径向+角度渐变示例      */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_AFF       0 /*! 径向+角度仿射变换      */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_PER       0 /*! 径向+角度透视变换      */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_MASK      0 /*! 径向+角度+MASK示例     */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_PATH      0 /*! 径向+角度+PATH示例     */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_AFF  0 /*! 径向+角度+PATH仿射变换 */
#define GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_PER  0 /*! 径向+角度+PATH透视变换 */

/* demo文件：gpu_demo_img_fill.c */
#define GPU_DEMO_IMG_FILL					 0 /*! 纹理+填充示例          */
#define GPU_DEMO_IMG_FILL_AFF				 0 /*! 纹理+填充仿射变换      */
#define GPU_DEMO_IMG_FILL_PER				 0 /*! 纹理+填充透视变换      */
#define GPU_DEMO_IMG_FILL_MASK				 0 /*! 纹理+填充+MASK示例     */
#define GPU_DEMO_IMG_FILL_PATH				 0 /*! 纹理+填充+PATH示例     */
#define GPU_DEMO_IMG_FILL_PATH_AFF			 0 /*! 纹理+填充+PATH仿射变换 */
#define GPU_DEMO_IMG_FILL_PATH_PER			 0 /*! 纹理+填充+PATH透视变换 */

/* demo文件：gpu_demo_img_grad.c */
#define GPU_DEMO_IMG_GRAD					 0 /*! 纹理+渐变示例          */
#define GPU_DEMO_IMG_GRAD_AFF				 0 /*! 纹理+渐变仿射变换      */
#define GPU_DEMO_IMG_GRAD_PER				 0 /*! 纹理+渐变透视变换      */
#define GPU_DEMO_IMG_GRAD_MASK				 0 /*! 纹理+渐变+MASK示例     */
#define GPU_DEMO_IMG_GRAD_PATH				 0 /*! 纹理+渐变+PATH示例     */
#define GPU_DEMO_IMG_GRAD_PATH_AFF			 0 /*! 纹理+渐变+PATH仿射变换 */
#define GPU_DEMO_IMG_GRAD_PATH_PER			 0 /*! 纹理+渐变+PATH透视变换 */

/* demo文件：gpu_demo_ring.c */
#define GPU_DEMO_RING_PATH                   0 /*! 径向+角度+PATH绘制圆环 */
#define GPU_DEMO_RING_RADIAL_ANGLE_PATH      0 /*! PATH绘制圆环 */

/* demo文件：gpu_demo_cubic_bezier_curve.c */
#define GPU_DEMO_CUBIC_BEZIER                1 /*! 绘制三阶贝塞尔曲线 */

/* demo文件：gpu_demo_line_chart.c */
#define GPU_DEMO_LINE_CHART                  0 /*! 绘制折线图 */

/* demo文件：gpu_demo_bar.c */
#define GPU_DEMO_BAR                         0 /*! 绘制进度条 */

/* demo文件：gpu_demo_slider.c */
#define GPU_DEMO_SLIDER                      0 /*! 绘制滑动条 */

/* demo文件：gpu_demo_jlui_res.c */
#define GPU_DEMO_JLUI_RES                    0 /*! 绘制杰理UI资源图片 */

/* demo文件：gpu_demo_breakpoint.c */
#define GPU_DEMO_BREAKPOINT                  0 /*! 断点任务 */

/* demo文件：anim/gpu_anim_cube.c */
#define GPU_DEMO_ANIM_CUBE					 0 /* 立方体效果 */

/* demo文件：anim/gpu_anim_flip.c */
#define GPU_DEMO_ANIM_FLIP					 0 /* 翻转动画   */

/*
    demo文件：draw 目录下所有，且本demo不允许与其他demo同时运行
    注意：使用时需手动在 lcd_drive.c 文件的 te_signal_sync_int_handler 函数
    手动调用 adpt_te_isr_te_signal() 获取TE信号；
    即：
    static void te_signal_sync_int_handler(enum gpio_port port, u32 pin, enum gpio_irq_edge edge)
    {
        adpt_te_isr_te_signal();
        return;
    }
 */
#define GPU_DEMO_ASYN_DRAW                   0 /* 异步渲染   */

/* img_data/img_test.c */
#define GPU_IMAGE_AUTO_TEST	                 0 /*! 自动纹理测试 */



#define GPU_DEMO_DRAW_X		(INV_VAL) /*! 指定DEMO渲染 X坐标，不等于INV_VAL时有效 */
#define GPU_DEMO_DRAW_Y		(INV_VAL) /*! 指定DEMO渲染 Y坐标，不等于INV_VAL时有效 */
#define GPU_DEMO_DRAW_W		(INV_VAL) /*! 指定DEMO渲染的宽度，不等于INV_VAL时有效 */
#define GPU_DEMO_DRAW_H		(INV_VAL) /*! 指定DEMO渲染的高度，不等于INV_VAL时有效 */

#define GET_X(x) ((GPU_DEMO_DRAW_X == (INV_VAL)) ? (x) : GPU_DEMO_DRAW_X)
#define GET_Y(y) ((GPU_DEMO_DRAW_Y == (INV_VAL)) ? (y) : GPU_DEMO_DRAW_Y)
#define GET_W(w) ((GPU_DEMO_DRAW_W == (INV_VAL)) ? (w) : GPU_DEMO_DRAW_W)
#define GET_H(h) ((GPU_DEMO_DRAW_H == (INV_VAL)) ? (h) : GPU_DEMO_DRAW_H)


#define INV_VAL	 			(JLGPU_WIN_MAX) /*! 无效标志 */
#define GPU_DEMO_SHOW_MS	(500) 			/*! DEMO默认刷新时间，使能多个DEMO时有效 */
#define GPU_WIN_OFFSET		0//((2047 - 466) / 2) /*! GPU窗口偏移，这里偏移到2K中间 */



#include "typedef.h"  /*! 数据类型重定义 */
#include "ui_color.h" /*! UI颜色分量操作 */
#include "gpu.h"	  /*! GPU 驱动头文件 */

struct gpu_demo_list {
    char *name;
    u32   time;
    u32  count;
    void (*init)(void);
    void (*free)(void);
    pJLGPUChainHead_t (*draw)(pJLGPUChainHead_t head, struct rect *draw_rect);
};
extern struct gpu_demo_list demo_list[]; /*! demo列表           */


extern void gpu_demo_run(void);          /*! demo启动           */


#endif /*! GPU_DEMO_ENABLE */
#endif /*! __GPU_DEMO_H__ */

