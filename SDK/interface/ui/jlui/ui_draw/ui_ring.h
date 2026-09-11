#ifndef __DRAW_RING_H__
#define __DRAW_RING_H__

#include "generic/rect.h"

struct ring_info {
    s16 center_x;
    s16 center_y;
    s16 radius;
    u16 color;
    u8 width;
    u8 alpha;
    s16 start_angle;
    s16 end_angle;
    s8 percent;
    u8 dont_output;
};

struct gpu_ring_info {
    int center_x;
    int center_y;
    int inner_radius;
    int outer_radius;
    int start_angle;
    int curr_angle;
    u32 fill_color;
};

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
void draw_ring(int center_x, int center_y, int radius,
               u16 color, u8 width, u8 alpha, int start_angle, int end_angle, int percent,
               u8 *buf, u32 buflen, struct rect *disp, struct rect *ctrl, int draw_id, int dont_output);



struct ring_area {
    int x1;
    int x2;
    int y1;
    int y2;
    int cnt;
};

extern struct ring_area g_ring_area;

#endif

