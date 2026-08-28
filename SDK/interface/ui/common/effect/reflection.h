#ifndef __REFLECTION__
#define __REFLECTION__

#include "gpu_port.h"
#include "hexagon.h"

#define REF_POLYTOPE_SIX    6   //六面体
#define REF_POLYTOPE_EIGHT  8   //八面体
#define REF_POLYTOPE_ALL    255 //共存
#define REF_POLYTOPE_CONFIG    REF_POLYTOPE_SIX
extern const u8 ref_polytope;
void hexagon_reflection_polytope_type_set(u8 type);

/*----------------------------------------------------------------------------*/
/**@brief    3D 灯笼倒影模型初始化
   @param    width : 立方体每个面的宽度
   @param    heihgt : 立方体每个面的高度
   @param    w_scale : 立方体每个面宽度的缩放系数
   @param    h_scale : 立方体每个面高度的缩放系数
   @param    view_distance : 视距(值越小, 立方体越大)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @param    h_ofs_range : 实际是 reflection_draw 的 x_angle 范围, 用于计算投影中心的偏移
   @note
*/
/*----------------------------------------------------------------------------*/
int reflection_init(int width[16], int height[16], float w_scale, float h_scale, float view_distance, int win_x, int win_y, int win_w, int win_h, float h_ofs_range);


/*----------------------------------------------------------------------------*/
/**@brief    3D灯笼倒影本体与倒影之间的间距
   @param    interval : 间距(默认值为30.0f, 值越大，间距越大)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void reflection_set_interval(float interval);

/*----------------------------------------------------------------------------*/
/**@brief    3D 灯笼倒影刷新
   @param     x_diff : x方向偏移
   @param     y_diff : y方向偏移
   @param     x_angle : 模型绕 x 轴旋转的角度
   @param     h_ofs_val : 实际是 reflection_draw 的 x_angle 的有效偏移, 用于计算投影中心的偏移
   @param     reset_sep : 重新调整的 GPU 任务链表序号
   @param     is_draw : 当前是否需要绘制模型
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int reflection_draw(struct hexagon_param *param, float x_diff, float y_diff, float x_angle, float h_ofs_val, u8 *reset_sep, u8 is_draw);

/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼倒影触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return -1 : 触摸点未落在立方体上, >= 0: 立方体某个面的索引(从0开始)
   @note
*/
/*----------------------------------------------------------------------------*/
int reflection_get_face_index(int x, int y);

/*----------------------------------------------------------------------------*/
/**@brief   3D 灯笼倒影资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void reflection_uninit(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief reflection_get_face_angle 获取对应 ID 面的角度
 *
 * @Params index
 * @Params x_angle
 * @Params y_angle
 */
/* ------------------------------------------------------------------------------------*/
void reflection_get_face_angle(int index, int *x_angle, int *y_angle);

#endif
