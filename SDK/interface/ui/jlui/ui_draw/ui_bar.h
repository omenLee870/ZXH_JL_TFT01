#ifndef __DRAW_BAR_H__
#define __DRAW_BAR_H__

#include "generic/rect.h"

struct ui_bar_info {
    int x;
    int y;
    int width;
    int height;
    u16 color;
    int percent;
    u8 alpha;
    u8 wait_sync;
};

/* ------------------------------------------------------------------------------------*/
/**
 * @brief draw_bar 绘制圆心端点进度条
 *
 * @param x  		left坐标
 * @param y			top坐标
 * @param width		宽度
 * @param height	高度
 * @param color		颜色值
 * @param percent	百分比
 * @param alpha		透明度(0-255)
 * @param buf		屏显buffer
 * @param buflen	屏显len
 * @param disp		屏显区域
 * @param id		DRAW_BAR_CPU
 */
/* ------------------------------------------------------------------------------------*/
void draw_bar(int x, int y, int width, int height, u16 color, int percent, int alpha, u8 *buf, u32 buflen, struct rect *disp, int id, int wait_sync);

#endif

