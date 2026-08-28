/**
 * @file gui_scr.h
 *
 */
#ifndef GUI_SCR_H
#define GUI_SCR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../lv_conf_internal.h"

#if LV_USE_GUI_SCR != 0

#include "../../../../core/lv_obj.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef lv_obj_t *(*gui_scr_setup_cb_t)(void *ui);
typedef void (*gui_scr_unload_cb_t)(void *ui);

typedef struct {
    int32_t id;                    // 页面ID
    const char *name;              // 页面名称
    lv_obj_t *scr;                 // 页面对象
    bool is_del;                   // 是否删除
    bool is_act;                   // 是否是活动页面
    gui_scr_setup_cb_t setup_cb;   // 页面初始化回调
    gui_scr_unload_cb_t unload_cb; // 页面卸载回调
} gui_scr_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief initialize the scr list (only call once)
 */
void gui_scr_init(void);

/**
 * @brief create a new screen
 *
 * @param id screen id
 * @param name screen name
 * @param scr screen object
 * @param setup_cb screen setup callback
 * @param unload_cb screen unload callback
 * @return gui_scr_t* screen object
 */
gui_scr_t *gui_scr_create(int32_t id, const char *name, lv_obj_t *obj, gui_scr_setup_cb_t setup_cb,
                          gui_scr_unload_cb_t unload_cb);

/**
 * @brief delete the screen
 *
 * @param screen screen object
 */
void gui_scr_del(gui_scr_t *screen);

/**
 * @brief delete all screens
 */
void gui_scr_del_all(void);

/**
 * @brief unload the screen
 *
 * @param is_clear whether to clear the screen
 * @param is_del whether to call the unload callback
 * @param ui lv_ui object
 * @return gui_scr_t* unloaded screen object
 */
gui_scr_t *gui_scr_unload_act(bool is_clear, bool is_del, void *ui);

/*=====================
 * Setter functions
 *====================*/

/**
 * @brief set the screen as active
 *
 * @param screen screen object
 * @param ui lv_ui object
 */
void gui_scr_set_act(gui_scr_t *screen, void *ui);

/**
 * @brief set the screen animation
 *
 * @param screen screen object
 * @param ui lv_ui object
 * @param anim_type animation type
 * @param time animation time
 * @param delay animation delay
 * @param is_clear whether to clear the old screen
 * @param auto_del whether to delete the old screen
 */
void gui_scr_set_act_anim(gui_scr_t *screen, void *ui, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay,
                          bool is_clear, bool auto_del);

/*=====================
 * Getter functions
 *====================*/

/**
 * @brief get the active screen
 *
 * @return gui_scr_t* active screen object
 */
gui_scr_t *gui_scr_get_act(void);

/**
 * @brief get the screen by id
 *
 * @param id screen id
 * @return gui_scr_t* screen object
 */
gui_scr_t *gui_scr_get(int32_t id);

// 这个函数可能不太合理，因为有可能有多个相同名字的页面
/**
 * @brief get the screen by name
 *
 * @param name screen name
 * @return gui_scr_t* screen object
 */
gui_scr_t *gui_scr_get_by_name(const char *name);

/**
 * @brief get the screen by object
 *
 * @param obj screen object
 * @return gui_scr_t* screen object
 */
gui_scr_t *gui_scr_get_by_obj(lv_obj_t *obj);

/**
 * @brief get the screen by object
 *
 * @param scr screen object
 * @return gui_scr_t* screen object
 */
int32_t gui_scr_get_id(gui_scr_t *screen);

/**
 * @brief get the screen name
 *
 * @param screen screen object
 * @return const char* screen name
 */
const char *gui_scr_get_name(gui_scr_t *screen);

/**
 * @brief get the screen object
 *
 * @param screen screen object
 * @return lv_obj_t* screen object
 */
bool gui_scr_is_act(gui_scr_t *screen);

/**
 * @brief get the screen object
 *
 * @param screen screen object
 * @return lv_obj_t* screen object
 */
bool gui_scr_is_del(gui_scr_t *screen);

/**
 * @brief get the screen count
 *
 * @return uint32_t screen count
 */
uint32_t gui_scr_get_len(void);

/**
 * @brief check the screen list is empty
 *
 * @return true screen list is empty
 * @return false screen list is not empty
 */
bool gui_scr_is_empty(void);

#endif /*LV_USE_GUI_SCR*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SCR_MANAGER_H*/
