/*Generate Code, Do NOT Edit!*/
#ifndef CALLBACK_HANDLER_H_
#define CALLBACK_HANDLER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "../gui_guider.h"
#include "lvgl.h"
#include "../../custom/custom.h"

// Generate page_map_timer_1 CallBack Handler
extern void page_map_timer_1_timer_cb(lv_timer_t * src);

// Generate page_map_timer_2 CallBack Handler
static int img_2_init_flag  = 0;
extern void page_map_timer_2_timer_cb(lv_timer_t * src);

// Generate page_meter_timer_1 CallBack Handler
extern void page_meter_timer_1_timer_cb(lv_timer_t * src);

// Generate page_music_timer_1 CallBack Handler
extern void page_music_timer_1_timer_cb(lv_timer_t * src);

#ifdef __cplusplus
}
#endif
#endif /* CALLBACK_HANDLER_H_ */
