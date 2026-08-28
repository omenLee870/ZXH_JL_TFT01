#ifndef __HEXAGON_H__
#define __HEXAGON_H__

#include "gpu_port.h"

#define POLYTOPE_SIX    6   //六面体
#define POLYTOPE_EIGHT  8   //八面体
#define POLYTOPE_ALL    255 //共存
#define POLYTOPE_CONFIG    POLYTOPE_SIX
extern const u8 polytope;
void hexagon_polytope_type_set(u8 type);

struct hexagon_param {
    pJLGPUTaskHead_t head; //gpu链表
};

/*----------------------------------------------------------------------------*/
/**@brief    3D 灯笼模型初始化
   @param    width : 立方体每个面的宽度
   @param    heihgt : 立方体每个面的高度
   @param    w_scale : 立方体每个面宽度的缩放系数
   @param    h_scale : 立方体每个面高度的缩放系数
   @param    view_distance : 视距(值越小, 立方体越大)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @param    h_ofs_range : 实际是 hexagon_draw 的 x_angle 范围, 用于计算投影中心的偏移
   @note
*/
/*----------------------------------------------------------------------------*/
int hexagon_init(int width[8], int height[8], float w_scale, float h_scale, float view_distance, int win_x, int win_y, int win_w, int win_h, float h_ofs_range);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_draw
 *
 * @Params param
 * @Params x_update_val : x方向更新的角度
 * @Params y_update_val : y方向更新的角度
 * @param  x_angle : 模型绕 x 轴旋转的角度
 * @param  h_ofs_val : 实际是 hexagon_draw 的 x_angle 的有效偏移, 用于计算投影中心的偏移
 * @param  reset_sep : 重新调整的 GPU 任务链表序号
 * @param  is_draw : 当前是否需要绘制模型
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int hexagon_draw(struct hexagon_param *param, float x_update_val, float y_update_val, float x_angle, float h_ofs_val, u8 *reset_sep, u8 is_draw);

/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return -1 : 触摸点未落在立方体上, >= 0: 立方体某个面的索引(从0开始)
   @note
*/
/*----------------------------------------------------------------------------*/
int hexagon_get_face_index(int x, int y);

/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void hexagon_uninit(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_get_face_angle 获取对应 ID 面的角度
 *
 * @Params index
 * @Params x_angle
 * @Params y_angle
 */
/* ------------------------------------------------------------------------------------*/
void hexagon_get_face_angle(int index, int *x_angle, int *y_angle);

#endif
