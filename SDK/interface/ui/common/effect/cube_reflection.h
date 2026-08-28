#ifndef __CUBE_REFLECTION__
#define __CUBE_REFLECTION__

#include "gpu_port.h"

struct cube_reflection_param {
    pJLGPUTaskHead_t head; //gpu链表
};

/*----------------------------------------------------------------------------*/
/**@brief    3D 灯笼倒影模型初始化
   @param    width : 立方体每个面的宽度
   @param    heihgt : 立方体每个面的高度
   @param    w_scale : 立方体每个面宽度的缩放系数
   @param    h_scale : 立方体每个面高度的缩放系数
   @param    view_distance : 视距(值越小, 立方体越大)
   @param    view_distance_min : 视距最小值(值最小, 立方体最大)
   @param    view_distance_max : 视距最大值(值最大, 立方体最小)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_reflection_init(int width[12], int height[12], float w_scale, float h_scale, float view_distance, float view_distance_min, float view_distance_max, int win_x, int win_y, int win_w, int win_h);
/*----------------------------------------------------------------------------*/
/**@brief    3D 灯笼倒影刷新
   @param     x_update_val : x方向更新的角度
   @param     y_update_val : y方向更新的角度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_reflection_draw(struct cube_reflection_param *param, float x_update_val, float y_update_val);
/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼倒影触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return -1 : 触摸点未落在立方体上, >= 0: 立方体某个面的索引(从0开始)
   @note
*/
/*----------------------------------------------------------------------------*/
int cube_reflection_get_face_index(int x, int y);
/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼倒影资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void cube_reflection_uninit(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief cube_reflection_get_face_angle 获取对应 ID 面的角度
 *
 * @Params index
 * @Params x_angle
 * @Params y_angle
 */
/* ------------------------------------------------------------------------------------*/
void cube_reflection_get_face_angle(int index, int *x_angle, int *y_angle);

#endif
