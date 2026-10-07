#ifndef __HARDWARE_TEST_H_
#define __HARDWARE_TEST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/*==================== 单活动测试项管理 ====================*/
typedef enum {
    HW_TEST_NONE = 0,
    HW_TEST_LTE,       /* 4G */
    HW_TEST_MIC1,      /* MIC1 */
    HW_TEST_MIC2,      /* MIC2 */
    HW_TEST_CAN,       /* CAN */
    HW_TEST_GPS,       /* GPS */
    HW_TEST_WIFI,      /* WiFi */
    HW_TEST_GSENSOR,   /* G-sensor */
    HW_TEST_ADKEY,     /* ADKEY */
    HW_TEST_BT,        /* BT */
} hw_test_item_t;

/*
 * 点击测试项按钮时先调用: 停掉当前活动项(Gsensor 停刷新/关传感器, GPS 取消轮询,
 * WiFi 关机, ADKEY 复位), 再把活动项切到 next。
 * 返回 0 = 已切换; 返回 1 = next 就是当前活动项(重复点击, 调用方可忽略)。
 */
int hardware_test_switch_item(hw_test_item_t next);

/* 当前活动项自然结束(如 4G 同步返回)后调用, 活动项回到 NONE。 */
void hardware_test_item_done(void);

/* 当前活动项。 */
hw_test_item_t hardware_test_get_active_item(void);

/*
 * 运行中测试项按钮高亮: hardware_test_ui_highlight_sync() 按当前活动项框住对应按钮;
 * hardware_test_ui_highlight_timer_cb() 供 LVGL 线程周期调用(events_init_hardware_test
 * 建 200ms 定时器驱动), 用于异步结束(如 GPS 完成后)自动取消高亮。均在 LVGL 线程调用。
 */
void hardware_test_ui_highlight_sync(void);
void hardware_test_ui_highlight_timer_cb(lv_timer_t *timer);

/*==================== hardware operations ====================*/

/* 4G / LTE HTTP test. Returns 0 on success, non-zero on failure. */
int hardware_test_lte_4g_start(void);

/* MIC ADC loopback test. mic_idx: 0 = MIC1(ADC0), 1 = MIC2(ADC1). */
int hardware_test_mic_start(int mic_idx);

/* CAN bus test. Returns 0 on success, non-zero on failure. */
int hardware_test_can_start(void);

/* GPS signal test. Returns 0 on success, non-zero on failure. */
int hardware_test_gps_start(void);

/* WiFi test. Returns 0 on success, non-zero on failure. */
int hardware_test_wifi_start(void);

void hardware_test_wifi_stop_wait(int timeout_ms);

/* G-sensor test. Returns 0 on success, non-zero on failure. */
int hardware_test_gsensor_start(void);

/* Stop G-sensor periodic on-screen refresh + release sensor. Called when the
 * hardware test screen is left (e.g. UI Effect button). */
int hardware_test_gsensor_stop(void);

/* ADKEY test enable. */
int hardware_test_adkey_start(void);

/* Whether the ADKEY test is active (used by the key input path). */
int hardware_test_adkey_is_active(void);

/* Report an ADKEY press. adkey_number: 1..20. */
int hardware_test_adkey_key_down(int adkey_number);

/* Local Bluetooth name, or NULL if unavailable. */
const char *hardware_test_bt_get_name(void);

/*==================== on-screen result helpers ====================*/

/* Show message in the result textarea. timeout_ms > 0 auto-hides after that. */
void hardware_test_show_message(lv_obj_t *edit, const char *text, uint32_t timeout_ms);

/* Show a result for 3 s then auto-hide. */
void hardware_test_show_result(lv_obj_t *edit, const char *text);

/* Hide the result textarea and cancel any pending auto-hide timer. */
void hardware_test_hide_message(lv_obj_t *edit);

/* Show "ADKEY<n> DOWN" / silk-screen key name on the popup. */
void hardware_test_ui_adkey_down(int adkey_number);

#ifdef __cplusplus
}
#endif
#endif /* __HARDWARE_TEST_H_ */