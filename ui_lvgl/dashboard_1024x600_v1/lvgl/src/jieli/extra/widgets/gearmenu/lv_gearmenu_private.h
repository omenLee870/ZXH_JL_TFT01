/**
 * @file lv_gearmenu_private.h
 *
 */

#ifndef LV_GEARMENU_PRIVATE_H
#define LV_GEARMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/widgets/label/lv_label_private.h"
#include "lv_gearmenu.h"

#if LV_USE_GEARMENU != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_gearmenu_t {
    lv_obj_t obj;
    int32_t angle_start; // 可视区域起始角度
    int32_t angle_end; // 可视区域结束角度
    int32_t angle; //当前角度
    int32_t angle_step; // 菜单项之间的角度差
    int32_t radius; // 菜单半径
    int32_t px_per_angle; // 每度对应的像素
    int32_t bounce_anim_time; // 回弹动画时间，单位ms
    lv_point_t last_point; // 上次滚动位置
    lv_point_t center; // 菜单中心坐标
    bool is_clockwise; // 顺时针
    bool is_loop_mode; // 循环模式
    bool _is_init; // 是否初始化
};

struct _lv_gearmenu_button_t {
    lv_obj_t obj;
    lv_obj_t * img;
    lv_obj_t * label;
    bool is_hide;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_GEARMENU != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GEARMENU_PRIVATE_H*/