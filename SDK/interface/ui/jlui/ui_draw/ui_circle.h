#ifndef __CIRCLE_H__
#define __CIRCLE_H__

#include "typedef.h"
#include "generic/rect.h"

#define RADIUS_CIRCLE (0x7FFF)

struct circle_info {
    s16 center_x;
    s16 center_y;
    s16 radius;
    u16 color;
    u8 alpha;
    u8 dont_output;
};
void draw_circle(int center_x, int center_y, int radius,
                 u16 color,  u8 alpha,
                 u8 *buf, u32 buflen, struct rect *disp, struct rect *ctrl, int draw_id, int dont_output);

#endif


