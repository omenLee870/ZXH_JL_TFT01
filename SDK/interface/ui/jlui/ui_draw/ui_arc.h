#ifndef __DRAW_ARC_H__
#define __DRAW_ARC_H__

#include "generic/typedef.h"
#include "ui_draw/ui_basic.h"
#include "ui_draw/ui_mask.h"
#include "ui_draw/ui_rect.h"


#define RADIUS_CIRCLE (0x7FFF)


#define SPLIT_RADIUS_LIMIT 10
#define SPLIT_ANGLE_GAP_LIMIT 60

typedef struct {
    int center_x;
    int center_y;
    int radius;
    u16 start_angle;
    u16 end_angle;
    u16 start_quarter;
    u16 end_quarter;
    int width;
    u8 alpha;
    draw_rect_dsc_t *draw_dsc;
    const struct draw_area *draw_area;
    const struct draw_area *clip_area;
} quarter_draw_dsc_t;
/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_ring 圆环接口
 *
 * @param center_x  圆心x坐标
 * @param center_y	圆心y坐标
 * @param radius	外半径
 * @param color		圆环颜色
 * @param width		圆环宽度
 * @param start_angle	起始角度
 * @param end_angle		结束角度
 * @param percent		百分比
 * @param buf			屏显buffer
 * @param buflen		屏显buffer长度
 * @param disp			屏显区域
 * @param ctrl			绘制区域
 * @param draw_id		DRAW_RING imb绘制；DRAW_RING_CPU 软件打点
 */
/* ------------------------------------------------------------------------------------*/
void draw_ring(int center_x, int center_y, int radius, u16 color, u8 width, u8 alpha, int start_angle, int end_angle, int percent, u8 *buf, u32 buflen, struct rect *disp, struct rect *ctrl, int draw_id, int dont_output);


void draw_circular_arc(int center_x, int center_y, u16 radius,  u16 start_angle, u16 end_angle, const struct draw_area *clip_area, const draw_line_dsc_t *dsc);

#endif

