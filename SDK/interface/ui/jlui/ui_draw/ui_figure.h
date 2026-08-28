#ifndef UI_FIGURE_H
#define UI_FIGURE_H

#include "ui_draw/ui_circle.h"
#include "ui_draw/ui_rect.h"
#include "ui_draw/ui_basic.h"
#include "ui_draw/ui_mask.h"
#include "ui_draw/ui_curve_grad.h"
//ui_draw_default_id
#define DRAW_LINE           0x1
#define DRAW_LINE_BY_ANGLE  0x2
#define DRAW_RECT           0x3
#define FILL_RECT           0x4
#define DRAW_CIRCLE         0x5
#define DRAW_RING           0x6
#define DRAW_RING_CPU       0x7
#define DRAW_BAR_CPU        0x8
#define DRAW_LINEMETER      0x9
#define DRAW_AIRECT		    0xa
#define DRAW_NEWLINE		0xb
#define DRAW_POLY			0xc
#define DRAW_SMOOTH_LINE	0xd
#define DRAW_BAR        	0xe
#define DRAW_CURVE_GRAD		0xf


typedef enum {
    DRAW_SLIDEBAR_HORIZONTAL_LEFT,
    DRAW_SLIDEBAR_HORIZONTAL_RIGHT,
    DRAW_SLIDEBAR_VERTICAL_TOP,
    DRAW_SLIDEBAR_VERTICAL_BTM,
} SLIDEBAR_STYLE;

struct _line {
    int x_begin;
    int y_begin;
    int x_end;
    int y_end;
    int length;
    int angle;
    int color;
};

struct _rect {
    struct rect rectangle;
    int color;
};

/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_line 画水平/垂直线段接口
 *
 * @param _dc			dc
 * @param line_info 	线段参数
 */
/* ------------------------------------------------------------------------------------*/
void draw_line(void *_dc, struct _line *line_info);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_line_by_angle 带角度的线段
 *
 * @param _dc			dc
 * @param line_info		线段参数
 */
/* ------------------------------------------------------------------------------------*/
void draw_line_by_angle(void *_dc, struct _line *line_info);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_triangle 绘制三角形
 *
 * @param _dc			dc
 * @param line_info1	边1参数
 * @param line_info2	边2参数
 * @param line_info3	边3参数
 */
/* ------------------------------------------------------------------------------------*/
void draw_triangle(void *_dc, struct _line *line_info1, struct _line *line_info2, struct _line *line_info3);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_rect		绘制矩形(非填充)
 *
 * @param _dc			dc
 * @param rectangle		矩形参数
 * @param color			边框颜色
 */
/* ------------------------------------------------------------------------------------*/
void draw_rect(void *_dc, struct rect *rectangle, u16 color);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_circle_or_arc 绘制圆或弧
 *
 * @param _dc			dc
 * @param info			圆/弧参数
 */
/* ------------------------------------------------------------------------------------*/
void draw_circle_or_arc(void *_dc, struct circle_info *info);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_draw_default 绘制default_id的图形
 *
 * @param id			见ui_draw_default_id
 * @param dst_buf		屏显buffer
 * @param dst_r			屏显区域
 * @param src_r			绘制区域
 * @param bytes_per_pixel 像素字节数
 * @param priv			图像参数
 */
/* ------------------------------------------------------------------------------------*/
void ui_draw_default(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv);
typedef void (*custom_draw_func)(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv);



/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_draw_curve_grad 绘制平滑曲线渐变图
 *
 * @Params _dc dc
 * @Params x 绘制x坐标
 * @Params y 绘制y坐标
 * @Params width 绘制宽度
 * @Params height 绘制高度
 * @Params points 离散点坐标列表
 * @Params points_num 离散点坐标数量
 * @Params line_width 曲线宽度（像素）
 * @Params color 绘制颜色
 */
/* ------------------------------------------------------------------------------------*/
void ui_draw_curve_grad(void *_dc, int x, int y, int width, int height, struct cg_point *points, int points_num, int line_width, u32 color);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_draw_get_color 获取自定义绘图的配置颜色
 *
 * @Params id 自定义绘图功能ID
 * @Params priv 自定义绘图功能配置
 *
 * @return ARGB8565 颜色值
 */
/* ------------------------------------------------------------------------------------*/
u32 ui_draw_get_color(u32 id, void *priv);

#endif

