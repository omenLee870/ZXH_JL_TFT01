
/**
 * @file lv_port_indev_templ.h
 *
 */

/*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_INDEV_TEMPL_H
#define LV_PORT_INDEV_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../../lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_indev_init(void);

void lv_indev_timer_read_touch(void *user_data);
void lv_indev_set_touch_timer_en(int en);
void lv_indev_timer_read_key(void *user_data);
bool lv_indev_set_touch_timer_check(void);
void lv_indev_timer_read_encoder(void *user_data);
int32_t lv_port_get_first_touch_x(void);

#if LV_USE_SIM_INERTIAL_SLIDE
//动态使能模拟滑动坐标
//status = 1 enable;status = 0 disable.
void lv_set_sim_inertial_slide_status(uint8_t status);
#endif
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H*/

#endif /*Disable/Enable content*/
