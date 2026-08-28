#ifndef __FOOTBALL_H__
#define __FOOTBALL_H__

#include "gpu_port.h"

struct football_bg_rgb_t {
    u8 red;
    u8 green;
    u8 blue;
};

struct football_param {
    pJLGPUTaskHead_t head;//gpu链表
    u8 mask_en;  // 六边形背景+图标mask
    int element_id; //控件id(ui框架相关, 可不配置)
    int task_id; //gpu任务id(ui框架相关, 第三方UI框架需指定一个链表里唯一的标识, 用于区分不同的图片)
    int prior; //优先级(ui框架相关, 可不配置)
    int image_bg_num; //背景图片数量，固定用法, 0:无六边形背景图片(六边形图标) 1:共用一张六边形背景图片,可以单独上色(圆形图标)  3:使用3张六边形背景图片叠加,3张图片分别上色混合叠加(圆形图标) 20:使用20张六边形背景图片，跟圆形图标一一对应(圆形图标)
    int image_num; //图片数量
    struct ui_image_attrs *image_bg;  //背景图片属性, 需要跟image_bg_num保持一致
    struct ui_image_attrs *image; //图片属性, 需要跟image_num保持一致
    struct football_bg_rgb_t *football_bg_rgb; // 背景图片颜色
};

/*----------------------------------------------------------------------------*/
/**@brief    足球模型初始化
   @param    width : 足球球面宽度
   @param    height : 足球球面高度
   @param    scale : 足球球面缩放
   @param    view_distance : 视距(值越小, 足球越大)
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @note
*/
/*----------------------------------------------------------------------------*/
int football_init(int width, int height, float scale, float view_distance, int win_x, int win_y, int win_w, int win_h);
/*----------------------------------------------------------------------------*/
/**@brief    足球刷新
   @param    param : 足球显示相关参数
   @param    x_diff : x方向偏移
   @param    y_diff : y方向偏移
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_draw(struct football_param *param, float x_diff, float y_diff);
/*----------------------------------------------------------------------------*/
/**@brief   足球触摸点检测
   @param   x : 触摸点x坐标
   @param   y : 触摸点y坐标
   @return  -1: 触摸点未落在球面上, >= 0: 球面索引(从0开始)
   @note
*/
/*----------------------------------------------------------------------------*/
int football_get_face_index(int x, int y);
/*----------------------------------------------------------------------------*/
/**@brief    足球资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_uinit(void);
/*----------------------------------------------------------------------------*/
/**@brief   设置mask图片缩放倍数（mask_en使能后有效）
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_set_mask_scale(float scale);

/*----------------------------------------------------------------------------*/
/**@brief    足球六边形背景+图标mask
   @param
   @return
   @note     需在football_init后调用
*/
/*----------------------------------------------------------------------------*/
void set_football_bg_mask_en(u8 flag);
/*----------------------------------------------------------------------------*/
/**@brief   设置mask图片缩放倍数（mask_en使能后有效）
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_set_mask_scale(float scale);

/*----------------------------------------------------------------------------*/
/**@brief   获取足球位置
   @param   cur_mat : 位置参数, float[9]数组
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_get_pos(float *cur_mat);
/*----------------------------------------------------------------------------*/
/**@brief   设置足球位置
   @param   cur_mat : 位置参数, float[9]数组
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void football_set_pos(float *cur_mat);

#endif

