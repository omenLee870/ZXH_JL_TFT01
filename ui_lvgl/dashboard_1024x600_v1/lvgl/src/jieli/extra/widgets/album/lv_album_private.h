/**
 * @file lv_album_private.h
 *
 */

#ifndef LV_ALBUM_PRIVATE_H
#define LV_ALBUM_PRIVATE_H

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
#include "lv_album.h"

#if LV_USE_ALBUM != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/



struct _lv_album_t {
    lv_obj_t obj;

    const void ** srcs;           /* 图片源数组 */
    uint16_t count;               /* 图片数量 */

    lv_obj_t * main_cont;          /* 主图片容器 */
    lv_obj_t * main_img;           /* 主图片 */
    lv_obj_t * tran_cont;          /* 过渡图片容器 */
    lv_obj_t * tran_img;           /* 过渡图片 */

    int16_t act_idx;               /* 当前图片索引 */

    bool auto_load_more;          /* 是否自动加载更多 */
    bool syncing;                 /* 内部同步中，避免重复响应 */

    struct {
        lv_dir_t dir;               /* 滑动方向 */
        int32_t offset;             /* 当前滑动偏移（x） */
        int32_t momentum;           /* 兼容字段：内部用于估算水平速度（px/s） */
        bool is_scroll;             /* 是否处于滑动中 */
        bool animating;             /* 是否在回弹/切换动画中 */
        int32_t target;             /* 目标偏移（0 或 ±width） */
        int16_t tran_index;         /* 过渡图对应目标索引，-1 表示无 */
        int8_t  drag_dir;           /* 拖拽方向：-1 左滑，1 右滑，0 无 */
        int32_t offset_y;           /* 当前滑动偏移（y），用于轴向锁定判定 */
        uint32_t  last_tick;        /* 上一次 PRESSING tick 时间（ms） */
        uint8_t   axis_lock;        /* 0 未锁定，1 锁定水平，2 锁定垂直 */
    } scroll;

    lv_album_effect_cfg_t effect; /* 效果配置 */
    lv_album_load_more_cb_t load_more_cb; /* 加载更多图片回调 */

    lv_album_transform_cb_t transform_cb; /* 用户自定义的变换回调（若设置则优先生效） */
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/


#endif /* LV_USE_ALBUM != 0 */

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif /*LV_ALBUM_PRIVATE_H*/