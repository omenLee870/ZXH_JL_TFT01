/* Copyright(C)
 * not free
 * All right reserved
 *
 * @file ui_animation.h
 * @brief JL_UI 动画，相关接口确保在ui任务中调用
 * @author
 * @version
 * @date 2024-05-21
 */

#ifndef _UI_ANIMATION_H
#define _UI_ANIMATION_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "typedef.h"
#include "list.h"

/*********************
 *      DEFINES
 *********************/

#define UI_ANIM_REPEAT_INFINITE      0xFFFF
#define UI_ANIM_PLAYTIME_INFINITE    0xFFFFFFFF

#define UI_USE_USER_DATA 1

/**********************
 *      TYPEDEFS
 **********************/

/** Can be used to indicate if animations are enabled or disabled in a case*/
typedef enum {
    UI_ANIM_OFF,
    UI_ANIM_ON,
} ui_anim_enable_t;

struct _ui_anim_t;
struct _ui_timer_t;

/** Get the current value during an animation*/
typedef int32_t (*ui_anim_path_cb_t)(const struct _ui_anim_t *);

/** Generic prototype of "animator" functions.
 * First parameter is the variable to JL_UI element id.
 * Second parameter is the value to set.
 * Compatible with `ui_xxx_set_yyy(obj, value)` functions
 * The `x` in `_xcb_t` means it's not a fully generic prototype because
 * it doesn't receive `ui_anim_t *` as its first argument*/
typedef void (*ui_anim_exec_xcb_t)(int, int32_t);

/** Same as `ui_anim_exec_xcb_t` but receives `ui_anim_t *` as the first parameter.
 * It's more consistent but less convenient. Might be used by binding generator functions.*/
typedef void (*ui_anim_custom_exec_cb_t)(struct _ui_anim_t *, int32_t);

/** Callback to call when the animation is ready*/
typedef void (*ui_anim_ready_cb_t)(struct _ui_anim_t *);

/** Callback to call when the animation really stars (considering `delay`)*/
typedef void (*ui_anim_start_cb_t)(struct _ui_anim_t *);

/** Callback used when the animation values are relative to get the current value*/
typedef int32_t (*ui_anim_get_value_cb_t)(struct _ui_anim_t *);

/** Callback used when the animation is deleted*/
typedef void (*ui_anim_deleted_cb_t)(struct _ui_anim_t *);

/** Describes an animation*/
typedef struct _ui_anim_t {
    int var;                           /**<JL_UI element id*/
    ui_anim_exec_xcb_t exec_cb;          /**< Function to execute to animate*/
    ui_anim_start_cb_t start_cb;         /**< Call it when the animation is starts (considering `delay`)*/
    ui_anim_ready_cb_t ready_cb;         /**< Call it when the animation is ready*/
    ui_anim_deleted_cb_t deleted_cb;     /**< Call it when the animation is deleted*/
    ui_anim_get_value_cb_t get_value_cb; /**< Get the current value in relative mode*/
#if UI_USE_USER_DATA
    void *user_data;  /**< Custom user data*/
#endif
    ui_anim_path_cb_t path_cb;         /**< Describe the path (curve) of animations*/
    int32_t start_value;               /**< Start value*/
    int32_t current_value;             /**< Current value*/
    int32_t end_value;                 /**< End value*/
    int32_t time;                /**< Animation time in ms*/
    int32_t act_time;            /**< Current time in animation. Set to negative to make delay.*/
    uint32_t playback_delay;     /**< Wait before play back*/
    uint32_t playback_time;      /**< Duration of playback animation*/
    uint32_t repeat_delay;       /**< Wait before repeat*/
    uint16_t repeat_cnt;         /**< Repeat count for the animation*/
    uint8_t early_apply  : 1;    /**< 1: Apply start value immediately even is there is `delay`*/

    /*Animation system use these - user shouldn't set*/
    uint8_t playback_now : 1; /**< Play back is in progress*/
    uint8_t run_round : 1;    /**< Indicates the animation has run in this round*/
    uint8_t start_cb_called : 1;    /**< Indicates that the `start_cb` was already called*/

    struct list_head entry;
} ui_anim_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Init. the animation module
 */
void ui_anim_core_init(void);

/**
 * Initialize an animation variable.
 * E.g.:
 * ui_anim_t a;
 * ui_anim_init(&a);
 * ui_anim_set_...(&a);
 * ui_anim_start(&a);
 * @param a     pointer to an `ui_anim_t` variable to initialize
 */
void ui_anim_init(ui_anim_t *a);

/**
 * Set a variable to animate
 * @param a     pointer to an initialized `ui_anim_t` variable
 * @param var   JL_UI element id
 */
static inline void ui_anim_set_var(ui_anim_t *a, int var)
{
    a->var = var;
}

/**
 * Set a function to animate `var`
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param exec_cb   a function to execute during animation
 *                  LVGL's built-in functions can be used.
 *                  E.g. ui_obj_set_x
 */
static inline void ui_anim_set_exec_cb(ui_anim_t *a, ui_anim_exec_xcb_t exec_cb)
{
    a->exec_cb = exec_cb;
}

/**
 * Set the duration of an animation
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param duration  duration of the animation in milliseconds
 */
static inline void ui_anim_set_time(ui_anim_t *a, uint32_t duration)
{
    a->time = duration;
}

/**
 * Set a delay before starting the animation
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param delay     delay before the animation in milliseconds
 */
static inline void ui_anim_set_delay(ui_anim_t *a, uint32_t delay)
{
    a->act_time = -(int32_t)(delay);
}

/**
 * Set the start and end values of an animation
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param start     the start value
 * @param end       the end value
 */
static inline void ui_anim_set_values(ui_anim_t *a, int32_t start, int32_t end)
{
    a->start_value = start;
    a->current_value = start;
    a->end_value = end;
}

/**
 * Set the path (curve) of the animation.
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param path_cb a function to set the current value of the animation.
 */
static inline void ui_anim_set_path_cb(ui_anim_t *a, ui_anim_path_cb_t path_cb)
{
    a->path_cb = path_cb;
}

/**
 * Set a function call when the animation really starts (considering `delay`)
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param start_cb  a function call when the animation starts
 */
static inline void ui_anim_set_start_cb(ui_anim_t *a, ui_anim_start_cb_t start_cb)
{
    a->start_cb = start_cb;
}

/**
 * Set a function to use the current value of the variable and make start and end value
 * relative to the returned current value.
 * @param a             pointer to an initialized `ui_anim_t` variable
 * @param get_value_cb  a function call when the animation starts
 */
static inline void ui_anim_set_get_value_cb(ui_anim_t *a, ui_anim_get_value_cb_t get_value_cb)
{
    a->get_value_cb = get_value_cb;
}

/**
 * Set a function call when the animation is ready
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param ready_cb  a function call when the animation is ready
 */
static inline void ui_anim_set_ready_cb(ui_anim_t *a, ui_anim_ready_cb_t ready_cb)
{
    a->ready_cb = ready_cb;
}

/**
 * Set a function call when the animation is deleted.
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param deleted_cb  a function call when the animation is deleted
 */
static inline void ui_anim_set_deleted_cb(ui_anim_t *a, ui_anim_deleted_cb_t deleted_cb)
{
    a->deleted_cb = deleted_cb;
}

/**
 * Make the animation to play back to when the forward direction is ready
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param time      the duration of the playback animation in milliseconds. 0: disable playback
 */
static inline void ui_anim_set_playback_time(ui_anim_t *a, uint32_t time)
{
    a->playback_time = time;
}

/**
 * Make the animation to play back to when the forward direction is ready
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param delay     delay in milliseconds before starting the playback animation.
 */
static inline void ui_anim_set_playback_delay(ui_anim_t *a, uint32_t delay)
{
    a->playback_delay = delay;
}

/**
 * Make the animation repeat itself.
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param cnt       repeat count or `UI_ANIM_REPEAT_INFINITE` for infinite repetition. 0: to disable repetition.
 */
static inline void ui_anim_set_repeat_count(ui_anim_t *a, uint16_t cnt)
{
    a->repeat_cnt = cnt;
}

/**
 * Set a delay before repeating the animation.
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param delay     delay in milliseconds before repeating the animation.
 */
static inline void ui_anim_set_repeat_delay(ui_anim_t *a, uint32_t delay)
{
    a->repeat_delay = delay;
}

/**
 * Set a whether the animation's should be applied immediately or only when the delay expired.
 * @param a         pointer to an initialized `ui_anim_t` variable
 * @param en        true: apply the start value immediately in `ui_anim_start`;
 *                  false: apply the start value only when `delay` ms is elapsed and the animations really starts
 */
static inline void ui_anim_set_early_apply(ui_anim_t *a, bool en)
{
    a->early_apply = en;
}

/**
 * Set the custom user data field of the animation.
 * @param a           pointer to an initialized `ui_anim_t` variable
 * @param user_data   pointer to the new user_data.
 */
#if UI_USE_USER_DATA
static inline void ui_anim_set_user_data(ui_anim_t *a, void *user_data)
{
    a->user_data = user_data;
}
#endif

/**
 * Create an animation
 * @param a         an initialized 'anim_t' variable. Not required after call.
 * @return          pointer to the created animation (different from the `a` parameter)
 */
ui_anim_t *ui_anim_start(const ui_anim_t *a);

/**
 * Get a delay before starting the animation
 * @param a pointer to an initialized `ui_anim_t` variable
 * @return delay before the animation in milliseconds
 */
static inline uint32_t ui_anim_get_delay(ui_anim_t *a)
{
    return -a->act_time;
}

/**
 * Get the time used to play the animation.
 * @param a pointer to an animation.
 * @return the play time in milliseconds.
 */
uint32_t ui_anim_get_playtime(ui_anim_t *a);

/**
 * Get the user_data field of the animation
 * @param   a pointer to an initialized `ui_anim_t` variable
 * @return  the pointer to the custom user_data of the animation
 */
#if UI_USE_USER_DATA
static inline void *ui_anim_get_user_data(ui_anim_t *a)
{
    return a->user_data;
}
#endif

/**
 * Delete an animation of a variable with a given animator function
 * @param var       JL_UI element id
 * @param exec_cb   a function pointer which is animating 'var',
 *                  or NULL to ignore it and delete all the animations of 'var
 * @return          true: at least 1 animation is deleted, false: no animation is deleted
 */
bool ui_anim_del(int var, ui_anim_exec_xcb_t exec_cb);

/**
 * Delete all the animations
 */
void ui_anim_del_all(void);

/**
 * Get the animation of a variable and its `exec_cb`.
 * @param var       JL_UI element id
 * @param exec_cb   a function pointer which is animating 'var', or NULL to return first matching 'var'
 * @return          pointer to the animation.
 */
ui_anim_t *ui_anim_get(int var, ui_anim_exec_xcb_t exec_cb);

/**
 * Get global animation refresher timer.
 * @return pointer to the animation refresher timer.
 */
struct ui_anim_timer_t *ui_anim_get_timer(void);

/**
 * Delete an animation by getting the animated variable from `a`.
 * Only animations with `exec_cb` will be deleted.
 * This function exists because it's logical that all anim. functions receives an
 * `ui_anim_t` as their first parameter. It's not practical in C but might make
 * the API more consequent and makes easier to generate bindings.
 * @param a         pointer to an animation.
 * @param exec_cb   a function pointer which is animating 'var',
 *                  or NULL to ignore it and delete all the animations of 'var
 * @return          true: at least 1 animation is deleted, false: no animation is deleted
 */
static inline bool ui_anim_custom_del(ui_anim_t *a, ui_anim_custom_exec_cb_t exec_cb)
{
    return ui_anim_del(a ? a->var : 0, (ui_anim_exec_xcb_t)exec_cb);
}

/**
 * Get the animation of a variable and its `exec_cb`.
 * This function exists because it's logical that all anim. functions receives an
 * `ui_anim_t` as their first parameter. It's not practical in C but might make
 * the API more consequent and makes easier to generate bindings.
 * @param a         pointer to an animation.
 * @param exec_cb   a function pointer which is animating 'var', or NULL to return first matching 'var'
 * @return          pointer to the animation.
 */
static inline ui_anim_t *ui_anim_custom_get(ui_anim_t *a, ui_anim_custom_exec_cb_t exec_cb)
{
    return ui_anim_get(a ? a->var : 0, (ui_anim_exec_xcb_t)exec_cb);
}

/**
 * Get the number of currently running animations
 * @return      the number of running animations
 */
uint16_t ui_anim_count_running(void);

/**
 * Calculate the time of an animation with a given speed and the start and end values
 * @param speed speed of animation in unit/sec
 * @param start     start value of the animation
 * @param end       end value of the animation
 * @return          the required time [ms] for the animation with the given parameters
 */
uint32_t ui_anim_speed_to_time(uint32_t speed, int32_t start, int32_t end);

/**
 * Manually refresh the state of the animations.
 * Useful to make the animations running in a blocking process where
 * `ui_timer_handler` can't run for a while.
 * Shouldn't be used directly because it is called in `ui_refr_now()`.
 */
void ui_anim_refr_now(void);

/**
 * Calculate the current value of an animation applying linear characteristic
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_linear(const ui_anim_t *a);

/**
 * Calculate the current value of an animation slowing down the start phase
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_ease_in(const ui_anim_t *a);

/**
 * Calculate the current value of an animation slowing down the end phase
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_ease_out(const ui_anim_t *a);

/**
 * Calculate the current value of an animation applying an "S" characteristic (cosine)
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_ease_in_out(const ui_anim_t *a);

/**
 * Calculate the current value of an animation with overshoot at the end
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_overshoot(const ui_anim_t *a);

/**
 * Calculate the current value of an animation with 3 bounces
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_bounce(const ui_anim_t *a);

/**
 * Calculate the current value of an animation applying step characteristic.
 * (Set end value on the end of the animation)
 * @param a     pointer to an animation
 * @return      the current value to set
 */
int32_t ui_anim_path_step(const ui_anim_t *a);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief JL_UI 动画绘制函数
 *
 * @Params param 目前不使用，保留该参数
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_handler(void *param);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 设置动画在哪个任务下跑
 *
 */
/* ------------------------------------------------------------------------------------*/
void ui_anim_set_run_task(const char *run_task_name);

/**********************
 *   GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/*Default display refresh period. JL_UI will redraw changed areas with this period time*/
#define UI_ANIM_DISP_DEF_REFR_PERIOD 2      /*单位:tick(10ms)*/



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_UI_ANIMATION_H*/

