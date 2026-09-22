#ifndef _UI_API_H_
#define _UI_API_H_

#include "app_config.h"

#if CONFIG_JL_UI_ENABLE

#include "ui/lcd/lcd_drive.h"
#include "jlui_app/ui_api.h"

#else // #if CONFIG_JL_UI_ENABLE

enum ui_devices_type {
    LED_7,
    LCD_SEG3X9,
    TFT_LCD,//彩屏
    DOT_LCD,//点阵屏
};

//板级配置数据结构
struct ui_devices_cfg {
    enum ui_devices_type type;
    void *private_data;
};


#if (!TCFG_UI_ENABLE) || (CONFIG_UI_STYLE == STYLE_UI_SIMPLE)

#define UI_INIT(...)
#define UI_SHOW_WINDOW(...)
#define UI_HIDE_WINDOW(...)
#define UI_GET_WINDOW_ID()						(-1)
#define UI_HIDE_CURR_WINDOW()
#define UI_SHOW_MENU(...)
#define UI_MSG_POST(...)
#define UI_REFLASH_WINDOW(a)
#if CONFIG_LVGL_UI_ENABLE
/* 手表 LVGL 侧马达震动生效：映射到 watch_settings 驱动的 ui_moto_run（见
 * watch_320x385_v1/custom/ui_action_moto.c）。其它 LVGL 子项目(如儿童相机)未移植
 * 该马达驱动, 保持空宏, 避免 mode/rtc、mode/bt 等公共代码引用不存在的符号。 */
#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_WATCH)
extern void ui_moto_run(u8 run_mode);
#define UI_MOTO_RUN(a)       ui_moto_run(a)
#else
#define UI_MOTO_RUN(...)
#endif
#else
#define UI_MOTO_RUN(...)
#endif

#define UI_WINDOW_BACK_PUSH(a)
#define UI_WINDOW_BACK_SHOW(a)                  (-1)
#define UI_WINDOW_BACK_SPEC_SHOW(a)
#define UI_WINDOW_BACK_CLEAN()
#define UI_WINDOW_BACK_SUB()
#define UI_WINDOW_BACK_INDEX()
#define UI_WINDOW_BACK_DEL(a)

#define UI_WINDOW_PREEMPTION_POSH(a,b,c,d)		(-1)
#define UI_WINDOW_PREEMPTION_DEL(a)
#define UI_WINDOW_PREEMPTION_POP(a)

#define UI_WINDOW_PREEMPTION_CHECK()			(false)
#define CS_UI_POPUP_PAGE(a)

#define UI_SHOW_MULTI_PAGE()
#define UI_HIDE_MULTI_PAGE()

#define ID_WINDOW_POWER_ON              0
#define ID_WINDOW_POWERON_PASSWORD      0
#define ID_WINDOW_DIAL                  0
#define ID_WINDOW_SETTING               0

#ifndef ID_WINDOW_FINDPHONE
#define ID_WINDOW_FINDPHONE             		0
#endif
#ifndef ID_WINDOW_SPORTING
#define ID_WINDOW_SPORTING						0
#endif
#ifndef ID_WINDOW_PHONE_CALL_STATUS
#define ID_WINDOW_PHONE_CALL_STATUS				0
#endif

#endif

#endif // #if CONFIG_JL_UI_ENABLE


enum ui_card_run_type {
    UI_CARD_RUN_NULL,
    UI_CARD_RUN_SINGLE_PAGE,
    UI_CARD_RUN_MULTI_PAGE,
};


enum ui_card_run_type ui_card_get_status();
void ui_card_run_stop(void);

#endif
