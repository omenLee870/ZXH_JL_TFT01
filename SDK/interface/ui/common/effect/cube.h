#ifndef __FOOTBALL_H__
#define __FOOTBALL_H__

#include "gpu_port.h"

struct cube_param {
    pJLGPUTaskHead_t head; //gpu链表
};

/*----------------------------------------------------------------------------*/
/**@brief    立方体模型初始化
   @param    width[6] : 立方体6个面的宽度
   @param    heihgt[6] : 立方体6个面的高度
   @param    w_scale : 立方体每个面宽度的缩放系数
   @param    h_scale : 立方体每个面高度的缩放系数
   @param    view_distance : 视距(值越小, 立方体越大)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_init(int width[6], int height[6], float w_scale, float h_scale, float view_distance, int win_x, int win_y, int win_w, int win_h);
/*----------------------------------------------------------------------------*/
/**@brief    立方体刷新
   @param     x_diff : x方向偏移
   @param     y_diff : y方向偏移
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_draw(struct cube_param *param, float x_diff, float y_diff);
/*----------------------------------------------------------------------------*/
/**@brief   立方体触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return -1 : 触摸点未落在立方体上, >= 0: 立方体某个面的索引(从0开始)
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_get_face_index(int x, int y);
/*----------------------------------------------------------------------------*/
/**@brief   立方体资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void cube_uninit(void);

#endif
