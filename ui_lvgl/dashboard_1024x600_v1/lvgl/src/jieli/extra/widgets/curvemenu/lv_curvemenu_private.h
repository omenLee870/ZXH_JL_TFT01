/**
 * @file lv_curvemenu_private.h
 *
 */

#ifndef LV_CURVEMENU_PRIVATE_H
#define LV_CURVEMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "../../../../../src/widgets/label/lv_label_private.h"
#include "../../../../../src/misc/lv_timer_private.h"
#include "lv_curvemenu.h"

#if LV_USE_CURVEMENU != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_curvemenu_t {
    lv_obj_t obj;
    int32_t angle_start;     /*可视区域起始角度*/
    int32_t angle_end;       /*可视区域结束角度*/
    int32_t angle;           /*当前角度*/
    int32_t angle_step;      /*菜单项之间的角度差*/
    int32_t radius;          /*菜单半径*/
    int32_t px_per_angle;    /*每度对应的像素*/
    int32_t col_num;         /*列数*/
    int32_t cr;              /*圆心半径(圆心到菜单控件的距离，实际半径为菜单半径加上圆心半径)*/
    bool _is_init;           /*是否初始化*/

    struct {
        int32_t ver_momentum;
        lv_timer_t * scroll_tmr;
    } scroll;

    lv_obj_t ** layout_buf;
    uint32_t layout_buf_cap;
};

struct _lv_curvemenu_button_t {
    lv_obj_t obj;
    bool is_hide;
    lv_obj_t * img;
    lv_obj_t * label;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_CURVEMENU != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_CURVEMENU_PRIVATE_H*/
