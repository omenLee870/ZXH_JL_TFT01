/**
 * @file gui_scr_switch.h
 *
 */

#ifndef GUI_SCR_SWITCH_H
#define GUI_SCR_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "./gui_scr.h"
#include "./gui_scr_manager.h"
#if LV_USE_GUI_SCR_SWITCH != 0

/*********************
 *      DEFINES
 *********************/

typedef enum {
    GUI_SCR_MOVE_MODE_SLIDE = 0,    /* 滑动平移效果 */
    GUI_SCR_MOVE_MODE_ROTATE_EDGE,  /* 边缘旋转效果 */
    GUI_SCR_MOVE_MODE_SLIDE_ZOOM,   /* 滑动缩放组合效果 */
    GUI_SCR_MOVE_MODE_SCALE_FADE,   /* 缩放淡入淡出效果 */
    GUI_SCR_MOVE_MODE_CUSTOM = 0xFF /* 自定义模式 */
} gui_scr_move_mode_t;

#define DIR_THRESHOLD 30 /* 滑动方向距离阈值 */

#define FLIP_SWITCH_THRESHOLD 0.3 //Flip模式下，滑动超过屏幕的30%，则切换页面
#define EDGE_SWITCH_THRESHOLD 0.3 //Edge模式下，滑动超过屏幕的30%，则切换页面

#define EDGE_SWITCH_MAX_ANGLE 60 //Edge模式下，一个屏幕最多可以切换的角度

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 获取页面ID对应的页面
 * @param scr_id 页面ID
 * @return 页面指针
 */
typedef gui_scr_t *(*gui_scr_switch_get_scr_cb_t)(int32_t scr_id);

typedef struct {
    uint16_t move_en: 1;
    uint16_t slide_en: 1;
    uint16_t loop_head: 1; /* 允许在第一页时继续向前切换到最后一页 */
    uint16_t loop_tail: 1; /* 允许在最后一页时继续向后切换到第一页 */
    uint16_t first_result: 6;
    uint16_t result: 6;

    lv_point_t start_pos; /* 首次按下时的坐标 */
    lv_point_t end_pos;   /* 最终按下时的坐标 */
    lv_point_t cur_pos;   /* 当前按下时的坐标 */

    lv_point_t
    scr_offset_pos; /* 当按下时，页面已有偏移量，在页面切换动画中断时，需要恢复偏移量 */

    int32_t start_left;
    int32_t start_pos_x;

    uint32_t src_in_touchdown; /* 当前处于触摸按下的页面ID */
} gui_scr_switch_t;

typedef struct {
    int32_t mode: 8;                        /* 动画模式 */
    int32_t dir: 8;                         /* 滑动方向 */
    uint32_t loop_head: 1;                  /* 允许在第一页时继续向前切换到最后一页 */
    uint32_t loop_tail: 1;                  /* 允许在最后一页时继续向后切换到第一页 */
    int32_t src_id;                         /* 按下的页面ID */
    void *e;                                /* 事件对象 */
    void (*load_scr)(int32_t cur_scr);      /* 加载页面回调 */
    void (*anim_start_cb)(int32_t cur_scr); /* 动画开始回调 */
    void (*anim_end_cb)(int32_t cur_scr);   /* 动画结束回调 */
} gui_scr_switch_anim_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 执行页面移动切换
 * @param scr_switch
 * @param mode
 * @return int32_t
 */
int32_t gui_scr_switch_move(gui_scr_switch_t *scr_switch, int32_t mode);

/**
 * 执行页面移动切换动画
 * @param anim
 * @return int32_t
 */
int32_t gui_scr_switch_move_anim(gui_scr_switch_anim_t *anim);

/**
 * 设置页面切换使用的特效
 * @param anim_type 取 LV_SCR_LOAD_ANIM_OTHER_PAGE ~ LV_SCR_LOAD_ANIM_USER 之间的值走 JLGPU 特效；
 *                  其余值（含 LV_SCREEN_LOAD_ANIM_NONE）保持原有的手写补间动画
 */
void gui_scr_switch_set_anim_type(lv_screen_load_anim_t anim_type);

/**
 * 获取当前页面切换使用的特效
 * @return 当前的特效类型
 */
lv_screen_load_anim_t gui_scr_switch_get_anim_type(void);

/**
 * 检查当前页面是否可以进行切换
 * @return true 可以切换; false 不可以切换
 */
bool gui_scr_switch_is_enable(void);

/**
 * 设置获取页面回调
 * @param get_scr_cb 获取页面回调，在执行回调后，需要能够返回有效的页面对象
 */
void gui_scr_switch_set_get_scr_cb(gui_scr_switch_get_scr_cb_t get_scr_cb);

/**
 * 设置需要进行页面切换的页面管理器
 * @param manager 页面管理器
 */
void gui_scr_switch_set_manager(gui_scr_manager_t *manager);

/**
 * 获取页面切换方向
 * @param start_pos
 * @param end_pos
 * @return lv_dir_t
 */
lv_dir_t gui_scr_switch_get_dir(lv_point_t *start_pos, lv_point_t *end_pos);

/**
 * 获取背景页面
 * @return 背景页面
 */
lv_obj_t *gui_scr_switch_get_bg_scr(void);

#endif /*LV_USE_GUI_SCR_SWITCH*/

#ifdef __cplusplus
}
#endif

#endif // GUI_SCR_SWITCH_H

