/**
 * @file gui_scr_manager.h
 *
 */

#ifndef GUI_SCR_MANAGER_H
#define GUI_SCR_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../lv_conf_internal.h"

#if LV_USE_GUI_SCR_MANAGER != 0

#include "../../../../core/lv_obj.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_ll_t scr_ll;
    int32_t id;
    uint16_t cnt; /* 页面数量 */
    int32_t effect; /* 预留:页面切换效果 */
} gui_scr_manager_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the screen manager
 * @param scr_manager pointer to a screen manager object
 */
void gui_scr_manager_init(gui_scr_manager_t * scr_manager);

/**
 * Add a screen Id to the screen manager
 * @param scr_manager pointer to a screen manager object
 * @param id screen Id to add
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_add(gui_scr_manager_t * scr_manager, int32_t id);

/**
 * Insert a screen Id to the screen manager
 * @param scr_manager pointer to a screen manager object
 * @param id screen Id to insert
 * @param idx index to insert
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_insert(gui_scr_manager_t * scr_manager, int32_t id, uint32_t idx);

/**
 * Get the head screen Id
 * @param scr_manager pointer to a screen manager object
 * @return int32_t screen Id, -1: empty
 */
int32_t gui_scr_manager_head(gui_scr_manager_t * scr_manager);

/**
 * Get the tail screen Id
 * @param scr_manager pointer to a screen manager object
 * @return int32_t screen Id, -1: empty
 */
int32_t gui_scr_manager_tail(gui_scr_manager_t * scr_manager);

/**
 * Get the next screen Id
 * @param scr_manager pointer to a screen manager object
 * @param id current screen Id
 * @return int32_t screen Id, -1: fail
 */
int32_t gui_scr_manager_next(gui_scr_manager_t * scr_manager, int32_t id);

/**
 * Get the previous screen Id
 * @param scr_manager pointer to a screen manager object
 * @param id current screen Id
 * @return int32_t screen Id, -1: fail
 */
int32_t gui_scr_manager_prev(gui_scr_manager_t * scr_manager, int32_t id);

/**
 * Get the index of the screen Id
 * @param scr_manager pointer to a screen manager object
 * @param id screen Id
 * @return int32_t index, -1: fail
 */
int32_t gui_scr_manager_get_idx(gui_scr_manager_t * scr_manager, int32_t id);

/**
 * Swap two screen Ids
 * @param scr_manager pointer to a screen manager object
 * @param id1 screen Id 1
 * @param id2 screen Id 2
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_swap(gui_scr_manager_t * scr_manager, int32_t id1, int32_t id2);

/**
 * Delete a screen Id
 * @param scr_manager pointer to a screen manager object
 * @param id screen Id to delete
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_del(gui_scr_manager_t * scr_manager, int32_t id);

/**
 * Delete a screen Id by index
 * @param scr_manager pointer to a screen manager object
 * @param idx index to delete
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_del_by_idx(gui_scr_manager_t * scr_manager, uint32_t idx);

/**
 * Delete all screen Ids
 * @param scr_manager pointer to a screen manager object
 * @return int32_t 0: success, -1: fail
 */
int32_t gui_scr_manager_del_all(gui_scr_manager_t * scr_manager);

/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_GUI_SCR_MANAGER*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*GUI_SCR_MANAGER_H*/
