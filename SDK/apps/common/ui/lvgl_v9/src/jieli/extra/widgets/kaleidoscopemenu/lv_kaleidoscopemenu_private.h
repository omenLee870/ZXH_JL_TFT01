/**
 * @file lv_kaleidoscopemenu_private.h
 * Private header for kaleidoscopemenu widget
 */

#ifndef LV_KALEIDOSCOPEMENU_PRIVATE_H
#define LV_KALEIDOSCOPEMENU_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../src/core/lv_obj_private.h"
#include "../../../../../src/widgets/image/lv_image_private.h"
#include "lv_kaleidoscopemenu.h"

#if LV_USE_KALEIDOSCOPEMENU != 0

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_kaleidoscopemenu_t lv_kaleidoscopemenu_t;
typedef struct _lv_kaleidoscopemenu_button_t lv_kaleidoscopemenu_button_t;

struct _lv_kaleidoscopemenu_t {
    lv_obj_t obj;
    uint16_t icon_size;
    uint16_t ring_interval; // 用于调整相邻的各圈的间隔

    // single_move的大小依赖于icon_size和ring_interval，不能直接修改
    uint16_t icon_single_move;   // 单次移动距离

    int16_t roffset;    //移动距离
    int8_t cnt;         //圈数切换的参数
    int8_t last_dir;    //最后一次移动的方向
    uint16_t last_move_distance;
    int8_t energy_cnt;  //惯性计数
    uint16_t key_elm_index;  //最靠近屏幕右侧边界的图标下标
    lv_timer_t *touch_release_timer;  // 惯性定时器
};

struct _lv_kaleidoscopemenu_button_t {
    lv_image_t obj;
    bool _hide;
};

#endif /*LV_USE_KALEIDOSCOPEMENU*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_KALEIDOSCOPEMENU_PRIVATE_H*/