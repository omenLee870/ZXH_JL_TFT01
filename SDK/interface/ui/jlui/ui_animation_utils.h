/* Copyright(C)
 * not free
 * All right reserved
 *
 * @file ui_animation.h
 * @brief JL_UI 动画相关工具函数
 * @author
 * @version
 * @date 2024-05-21
 */

#ifndef _UI_ANIMATION_UTILS_H
#define _UI_ANIMATION_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "ui_animation.h"

#include "typedef.h"
#include "list.h"


/**********************
 * GLOBAL PROTOTYPES
 **********************/


/* ------------------------------------------------------------------------------------*/
/**
 * @brief animation play interval
 *
 * @Params ms: interval time
 *
 * @note call befor ui_anim_start()
 *
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_set_interval(u16 ms);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief suspend animation
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_timer_suspend(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief resume animation
 *
 * @Params ms: interval time
 *
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_timer_resume(u16 interval_ms);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief Get the elapsed milliseconds since start up
 *
 * @Return the elapsed milliseconds
 */
/* ------------------------------------------------------------------------------------*/
u32 ui_anim_tick_get(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief Get the elapsed milliseconds since a previous time stamp
 *
 * @Params prev_tick a previous time stamp (return value of ui_anim_tick_get() )
 *
 * @Return the elapsed milliseconds since 'prev_tick'
 */
/* ------------------------------------------------------------------------------------*/
u32 ui_anim_tick_elaps(uint32_t prev_tick);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief add a new node to the animaton list
 *
 * @Params root: the animation list head
 * @Params node: new node to be added
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_list_add(struct list_head *root, struct list_head *node);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief del the node from the animaton list
 *
 * @Params node: node to be deleted
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_list_del(struct list_head *node);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief del and free all node from the animation list. The list remain valid but become empty.
 *
 * @Params root: the animation list head
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_list_clear(struct list_head *root);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief Return with first node of the animation list
 *
 * @Params root: the animation list head
 *
 * @Return the first node from a list
 */
/* ------------------------------------------------------------------------------------*/
ui_anim_t *ui_anim_list_get_head(const struct list_head *root);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief Return with the pointer of the next node after 'cur_node'
 *
 * @Params root: the animation list head
 * @Params node: current node
 *
 * @Return pointer to the next node
 */
/* ------------------------------------------------------------------------------------*/
ui_anim_t *ui_anim_list_get_next(const struct list_head *root, const struct list_head *cur_node);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_UI_ANIMATION_UTILS_H*/

