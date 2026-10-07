#include <stddef.h>
#include <stdlib.h>
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#endif
#include "hardware_test.h"

#if TCFG_DASHBOARD_HARDWARE_TEST_ENABLE

#include "system/includes.h"
#include "../generated/gui_guider.h"
#include "http/http_cli.h"
#include "gps_driver.h"
#include "sensor_hub.h"
#include "bt_common.h"
#include "audio_config.h"
#include "audio_adc.h"
#include "asm/audio_adc.h"

/*
 * Hardware test implementations (board-specific).
 * Refer to the original events_init.c implementation.
 */

/*==================== WiFi/4G 阻塞启动 worker (不在 LVGL 线程执行) ====================*/
enum {
    HW_NET_NONE = 0,
    HW_NET_WIFI_START,
    HW_NET_WIFI_STOP,
    HW_NET_LTE_RUN,
};

extern int wifi_test(void);
static int hw_net_lte_run(void);

static u8 hw_net_created = 0;                    /* worker 是否已创建 */
static volatile u8 hw_net_req = HW_NET_NONE;     /* 待执行请求(后到覆盖先到) */
static volatile u8 hw_net_result_valid = 0;      /* 有结果待 LVGL 线程刷新 */
static volatile int hw_net_result = 0;           /* >0 成功, <0 失败 */
#if TCFG_EXT_WIFI_ENABLE
static u8 wifi_test_started = 0;                 /* 本测试流程内是否已拉起外挂 WiFi */
static volatile u8 hw_net_wifi_off_done = 1;     /* 最近一次 WiFi 关闭请求是否已完成 */
#endif

static void hw_net_task(void *priv)
{
    while (1) {
        while (hw_net_req == HW_NET_NONE) {
            os_time_dly(2);
        }
        u8 req = hw_net_req;
        hw_net_req = HW_NET_NONE;

#if TCFG_EXT_WIFI_ENABLE
        if (req == HW_NET_WIFI_START) {
            int ret = wifi_test();
            if (ret == 0) {
                wifi_test_started = 1;
            }
            if (hardware_test_get_active_item() == HW_TEST_WIFI && ret != 0) {
                hw_net_result = -1;
                hw_net_result_valid = 1;
                hardware_test_item_done();   /* 启动失败: 复位活动项, 允许重试 */
            }
        } else if (req == HW_NET_WIFI_STOP) {
            extern void ext_wifi_off(void);
            extern void ext_wifi_power_set(u8 on);
            if (wifi_test_started) {
                ext_wifi_off();
                ext_wifi_power_set(0);
                wifi_test_started = 0;
            }
            hw_net_wifi_off_done = 1;
        } else
#endif
            if (req == HW_NET_LTE_RUN) {
                int ret = hw_net_lte_run();
                if (hardware_test_get_active_item() == HW_TEST_LTE) {
                    hw_net_result = (ret == 0) ? 1 : -1;
                    hw_net_result_valid = 1;
                    hardware_test_item_done();   /* 4G 同步执行完, 复位活动项 */
                }
            }
    }
}

static int hw_net_request(u8 req)
{
    if (!hw_net_created) {
        int err = os_task_create_affinity_core(hw_net_task, NULL, 10, 4096, 0, "hw_test_net", 1);
        if (err) {
            printf("hw_test_net task create failed: %d\n", err);
            return -1;
        }
        hw_net_created = 1;
    }
    hw_net_req = req;
    return 0;
}

/* LVGL 线程周期调用: 把 worker 产生的测试结果刷新到弹窗 */
static void hw_net_flush_result(void)
{
    lv_ui_hardware_test *ui_scr;

    if (!hw_net_result_valid) {
        return;
    }
    hw_net_result_valid = 0;
    ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
    if (ui_scr == NULL || !lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
        return;
    }
    hardware_test_show_result(ui_scr->hardware_test_edit_1,
                              (hw_net_result > 0) ? "SUCCESS" : "FAILED");
}

/*==================== 结果弹窗 UI 助手 (运行在 LVGL 线程) ====================*/
static lv_timer_t *hardware_test_result_hide_timer = NULL;

static void hardware_test_update_message(lv_obj_t *edit, const char *text)
{
    if (!lv_obj_is_valid(edit)) {
        return;
    }
    lv_textarea_set_text(edit, text);
    lv_obj_clear_flag(edit, LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(edit);
    lv_obj_align(lv_textarea_get_label(edit), LV_ALIGN_CENTER, 0, 0);
}

static void hardware_test_result_hide_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *edit = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (lv_obj_is_valid(edit)) {
        lv_obj_add_flag(edit, LV_OBJ_FLAG_HIDDEN);
    }
    hardware_test_result_hide_timer = NULL;
}

void hardware_test_hide_message(lv_obj_t *edit)
{
    if (hardware_test_result_hide_timer != NULL) {
        lv_timer_delete(hardware_test_result_hide_timer);
        hardware_test_result_hide_timer = NULL;
    }
    if (lv_obj_is_valid(edit)) {
        lv_obj_add_flag(edit, LV_OBJ_FLAG_HIDDEN);
    }
}

void hardware_test_show_message(lv_obj_t *edit, const char *text, uint32_t timeout_ms)
{
    if (hardware_test_result_hide_timer != NULL) {
        lv_timer_delete(hardware_test_result_hide_timer);
        hardware_test_result_hide_timer = NULL;
    }
    hardware_test_update_message(edit, text);
    if (timeout_ms > 0) {
        hardware_test_result_hide_timer = lv_timer_create(hardware_test_result_hide_timer_cb, timeout_ms, edit);
        if (hardware_test_result_hide_timer != NULL) {
            lv_timer_set_repeat_count(hardware_test_result_hide_timer, 1);
        }
    }
}

void hardware_test_show_result(lv_obj_t *edit, const char *text)
{
    hardware_test_show_message(edit, text, 3000);
}

/* ADKEY 按键回显: 1~5 用丝印名, 其余显示 ADKEYn */
void hardware_test_ui_adkey_down(int adkey_number)
{
    char text[24];
    static const char *adkey_name[] = {
        NULL,          /* 0 */
        "KEY_ON/OFF",  /* 1 */
        "KEY_OK",      /* 2 */
        "KEY_DOWN",    /* 3 */
        "KEY_UP",      /* 4 */
        "KEY_BACK",    /* 5 */
    };
    lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
    if (ui_scr == NULL || !lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
        return;
    }
    if (adkey_number >= 1 && adkey_number <= 5) {
        lv_snprintf(text, sizeof(text), "%s DOWN", adkey_name[adkey_number]);
    } else {
        lv_snprintf(text, sizeof(text), "ADKEY%d DOWN", adkey_number);
    }
    hardware_test_show_message(ui_scr->hardware_test_edit_1, text, 0);
}

/*==================== 4G / LTE (SPI -> WL82 -> 4G) HTTP GET ====================*/
int hardware_test_lte_4g_start(void)
{
    return hw_net_request(HW_NET_LTE_RUN);   /* 后台执行, 不阻塞 LVGL 线程 */
}

static int hw_net_lte_run(void)
{
    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        printf("4G HTTP GET: calloc ctx failed\n");
        return -1;
    }

    http_body_obj http_body = {
        .buf_len = 4 * 1024, .buf_count = 1,
        .p = (char *)malloc(4 * 1024),
    };
    if (!http_body.p) {
        printf("4G HTTP GET: malloc recv buf failed\n");
        free(ctx);
        return -1;
    }

    ctx->url             = "http://www.baidu.com";
    ctx->timeout_millsec = 30000;
    ctx->priv            = &http_body;
    ctx->connection      = "close";

    int ret = httpcli_get(ctx);
    if (ret != HERROR_OK) {
        printf("4G HTTP GET failed, err=%d\n", ret);
    } else {
        printf("4G HTTP GET success, received %u bytes\n", http_body.recv_len);
    }

    httpcli_close(ctx);
    if (http_body.p) {
        free(http_body.p);
    }
    free(ctx);
    return (ret == HERROR_OK) ? 0 : -1;
}

/*==================== MIC(回声监听) ====================*/
#define HW_MIC_TEST_GAIN      11      /* 增益 0(-8dB)~19(30dB), 4=0dB */
#define HW_MIC_TEST_SR        16000   /* 采样率 */
#define HW_MIC_TEST_MS        10000   /* 单次测量时长 */
#define HW_MIC_PASS_PEAK      800     /* 幅度超过该值判定 MIC 有声 */

extern void pt_audio_adc_mic_demo_open(u8 mic_idx, u8 gain, int sr, u8 mic_2_dac);
extern void pt_audio_adc_mic_demo_close(void);

struct hw_mic_test {
    u8 running;
    u8 mic_idx;                     /* 0=MIC1 1=MIC2 */
    struct audio_adc_output_hdl output;
    volatile u32 peak;              /* 输出回调刷新 */
    lv_timer_t *timer;
    u32 elapsed_ms;
    u32 max_peak;
};
static struct hw_mic_test hw_mic;

/* ADC 采样输出回调: 统计峰值(pt demo 已单独挂自己的 handler 送 DAC) */
static void hw_mic_output(void *priv, s16 *data, int len)
{
    u32 pk = hw_mic.peak;
    int n = len >> 1;
    for (int i = 0; i < n; i++) {
        u32 v = (data[i] < 0) ? (u32)(-data[i]) : (u32)data[i];
        if (v > pk) {
            pk = v;
        }
    }
    hw_mic.peak = pk;
}

/* 关闭 mic/回声 */
static void hw_mic_close(void)
{
    if (hw_mic.timer) {
        lv_timer_delete(hw_mic.timer);
        hw_mic.timer = NULL;
    }
    if (hw_mic.running) {
        audio_adc_del_output_handler(&adc_hdl, &hw_mic.output);
        pt_audio_adc_mic_demo_close();
        hw_mic.running = 0;
    }
    hw_mic.peak = 0;
}

/* 供切走 MIC 测试项时停止 */
static int hardware_test_mic_stop(void)
{
    hw_mic_close();
    return 0;
}

static void hw_mic_timer_cb(lv_timer_t *timer)
{
    char text[48];
    lv_ui_hardware_test *ui_scr;
    u32 pk;

    pk = hw_mic.peak;
    hw_mic.peak = 0;
    if (pk > hw_mic.max_peak) {
        hw_mic.max_peak = pk;
    }
    hw_mic.elapsed_ms += 100;

    ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
    if (ui_scr && lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
        lv_snprintf(text, sizeof(text), "MIC%d TEST RUNNING\npeak: %d",
                    hw_mic.mic_idx + 1, hw_mic.max_peak);
        hardware_test_show_message(ui_scr->hardware_test_edit_1, text, 0);
    }

    if (hw_mic.elapsed_ms >= HW_MIC_TEST_MS) {
        int pass = (hw_mic.max_peak >= HW_MIC_PASS_PEAK);
        printf("mic%d test done, peak=%d, pass=%d\n", hw_mic.mic_idx + 1, hw_mic.max_peak, pass);
        hw_mic_close();
        if (ui_scr && lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
            hardware_test_show_result(ui_scr->hardware_test_edit_1, pass ? "PASS" : "FAIL");
        }
        hardware_test_item_done();
    }
}

int hardware_test_mic_start(int mic_idx)
{
    u32 mic_bit;

    if (mic_idx < 0 || mic_idx > 1) {
        return -1;
    }
    hw_mic_close();

    mic_bit = (mic_idx == 0) ? AUDIO_ADC_MIC_0 : AUDIO_ADC_MIC_1;
    pt_audio_adc_mic_demo_open(mic_bit, HW_MIC_TEST_GAIN, HW_MIC_TEST_SR, 1);   /* mic_2_dac=1 回声 */

    hw_mic.output.handler = hw_mic_output;
    hw_mic.output.priv = NULL;
    audio_adc_add_output_handler(&adc_hdl, &hw_mic.output);

    hw_mic.running = 1;
    hw_mic.mic_idx = mic_idx;
    hw_mic.peak = 0;
    hw_mic.elapsed_ms = 0;
    hw_mic.max_peak = 0;

    hw_mic.timer = lv_timer_create(hw_mic_timer_cb, 100, NULL);
    if (!hw_mic.timer) {
        printf("mic%d timer create fail\n", mic_idx + 1);
        hw_mic_close();
        return -1;
    }
    return 0;
}

/*==================== CAN ====================*/
extern int c_main_can(void);
extern int can_test_stop(void);

static u8 can_started = 0;   /* CAN demo 任务是否已启动(切走时被停止并清零, 可再次进入) */

/* 修改IO组控制权,避免PMC占用CAN引脚，根据实际配置决定是否调用 */
static void can_release_pmc_rx_pin(void)
{
    u32 can_mask = BIT(0) | BIT(1);
    PMC_PORTD->DIR |= can_mask;
    PMC_PORTD->SEL &= ~can_mask;
    __asm__ volatile("csync");
}

int hardware_test_can_start(void)
{
    if (can_started) {
        printf("can test already started\n");
        return 0;
    }

    can_release_pmc_rx_pin();
    int ret = c_main_can();
    if (ret != 0) {
        printf("can test start failed: %d\n", ret);
        return -1;
    }
    can_started = 1;
    return 0;
}

static int hardware_test_can_stop(void)
{
    if (!can_started) {
        return 0;
    }
    can_test_stop();
    can_started = 0;
    return 0;
}

/*==================== GPS ====================*/
#define GPS_TEST_POLL_TICKS     10      /* 100ms, OS tick = 10ms */
#define GPS_TEST_TIMEOUT_TICKS  24000   /* 2min */

static volatile u8 gps_test_running = 0;   /* 一轮 GPS 测试进行中 */
static volatile u8 gps_test_cancel = 0;    /* 取消当前这轮 */
static volatile u8 gps_test_request = 0;   /* 请求启动新一轮 */
static u8 gps_worker_created = 0;          /* 常驻工作线程是否已创建 */
static u8 gps_drv_init = 0;                /* gps_init 仅执行一次, 避免重复初始化 */

/* worker 采样结束后的结果, 交 LVGL 线程 gps_monitor_cb 显示 */
static lv_timer_t *gps_monitor_timer = NULL;
static volatile s32 gps_disp_lat = 0;
static volatile s32 gps_disp_lon = 0;
static volatile u8  gps_disp_ok = 0;
static volatile u8  gps_disp_done = 0;

/* E7(int*1e7) → "度.7位小数" */
static void gps_fmt_e7(char *buf, u32 len, s32 e7)
{
    u32 v = (e7 < 0) ? (u32) - e7 : (u32)e7;
    u32 f = v % 10000000;
    int n = snprintf(buf, len, "%s%d.", (e7 < 0) ? "-" : "", (int)(v / 10000000));
    for (u32 p = 1000000; p && n < (int)len - 1; p /= 10) {
        buf[n++] = (char)('0' + f / p);
        f %= p;
    }
    buf[n] = '\0';
}

/* LVGL 线程: GPS 结束后把结果显示到弹窗 */
static void gps_monitor_cb(lv_timer_t *timer)
{
    lv_ui_hardware_test *ui_scr;
    char lat[24], lon[24];
    char text[128];

    if (!gps_disp_done) {
        return;
    }
    gps_disp_done = 0;
    if (gps_monitor_timer) {
        lv_timer_delete(gps_monitor_timer);
        gps_monitor_timer = NULL;
    }

    hardware_test_item_done();   /* 复位活动项: 屏幕已不在也执行, 避免活动项卡在 GPS */

    ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
    if (ui_scr == NULL || !lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
        return;
    }

    if (gps_disp_ok) {
        gps_fmt_e7(lat, sizeof(lat), gps_disp_lat);
        gps_fmt_e7(lon, sizeof(lon), gps_disp_lon);
        lv_snprintf(text, sizeof(text), "GPS OK\nlat: %s\nlon: %s", lat, lon);
    } else {
        lv_snprintf(text, sizeof(text), "GPS FAIL\nNo fix within 2min");
    }
    hardware_test_show_message(ui_scr->hardware_test_edit_1, text, 0);
}

/* GPS 常驻工作线程: 单线程反复被复用, 不会重复 task_create。 */
static void gps_test_task(void *priv)
{
    while (1) {
        while (!gps_test_request) {
            os_time_dly(5);
        }
        gps_test_request = 0;
        gps_test_running = 1;
        gps_test_cancel = 0;

        gps_data_t gps;
        int success = 0;
        int elapsed_ticks = 0;

        while (elapsed_ticks < GPS_TEST_TIMEOUT_TICKS && !gps_test_cancel) {
            gps_poll();
            gps_get_info(&gps);
            if (gps.valid && gps.fix_mode > 0) {
                success = 1;
                break;
            }
            os_time_dly(GPS_TEST_POLL_TICKS);
            elapsed_ticks += GPS_TEST_POLL_TICKS;
        }

        printf("gps get info loop exit, success=%d, cancel=%d\n", success, gps_test_cancel);
        if (success) {
            char lat[24], lon[24];
            gps_fmt_e7(lat, sizeof(lat), gps.latitude_e7);
            gps_fmt_e7(lon, sizeof(lon), gps.longitude_e7);
            printf("gps location: lat=%s, lon=%s\n", lat, lon);
        }

        if (!gps_test_cancel) {          /* 自然结束(定位成功/超时): 交 LVGL 显示 */
            gps_disp_lat = gps.latitude_e7;
            gps_disp_lon = gps.longitude_e7;
            gps_disp_ok = (u8)success;
            gps_disp_done = 1;
        }

        gps_test_running = 0;
        gps_test_cancel = 0;
    }
}

int hardware_test_gps_start(void)
{
    if (gps_test_running) {
        printf("gps test is already running\n");
        return 0;
    }

    if (!gps_worker_created) {
        int err = task_create(gps_test_task, NULL, "gps_test");
        if (err) {
            printf("gps test task create failed: %d\n", err);
            return -1;
        }
        gps_worker_created = 1;
    }

    gps_disp_done = 0;
    gps_disp_ok = 0;
    if (!gps_monitor_timer) {
        gps_monitor_timer = lv_timer_create(gps_monitor_cb, 200, NULL);
        if (!gps_monitor_timer) {
            printf("gps monitor timer create failed\n");
            return -1;
        }
    }
    if (!gps_drv_init) {
        gps_init();
        gps_drv_init = 1;
    }
    gps_test_request = 1;
    return 0;
}

/* 停止: 取消当前这轮 GPS 轮询, 工作线程随即回到等待新一轮请求的空闲态。 */
static int hardware_test_gps_stop(void)
{
    if (gps_test_running) {
        gps_test_cancel = 1;
    }
    if (gps_monitor_timer) {
        lv_timer_delete(gps_monitor_timer);
        gps_monitor_timer = NULL;
    }
    return 0;
}

/*==================== WIFI ====================*/
int hardware_test_wifi_start(void)
{
    return hw_net_request(HW_NET_WIFI_START);   /* 后台执行, 不阻塞 LVGL 线程 */
}

/* 停止: 交 worker 后台关闭外挂 WiFi 并下电(不阻塞 LVGL 线程)。 */
static int hardware_test_wifi_stop(void)
{
#if TCFG_EXT_WIFI_ENABLE
    hw_net_wifi_off_done = 0;
#endif
    hw_net_request(HW_NET_WIFI_STOP);
    return 0;
}

void hardware_test_wifi_stop_wait(int timeout_ms)
{
#if TCFG_EXT_WIFI_ENABLE
    while (hw_net_wifi_off_done == 0 && timeout_ms > 0) {
        os_time_dly(1);           /* 10ms/tick */
        timeout_ms -= 10;
    }
#else
    (void)timeout_ms;
#endif
}

/*==================== G-sensor (数据刷到 UI, 退出 test 屏即停) ====================*/
static volatile u8 gsensor_test_running = 0;
static u8 gsensor_opened_here = 0;
static lv_timer_t *gsensor_monitor_timer = NULL;

static void gsensor_monitor_cb(lv_timer_t *timer)
{
    axis_data_t samples[32];
    u16 points;
    char text[64];
    lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);

    if (ui_scr == NULL || !lv_obj_is_valid(ui_scr->hardware_test_edit_1)) {
        return;
    }
    points = sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, samples, sizeof(samples));
    if (points == 0) {
        return;
    }
    points--;
    lv_snprintf(text, sizeof(text), "X: %d\nY: %d\nZ: %d",
                samples[points].x, samples[points].y, samples[points].z);
    hardware_test_show_message(ui_scr->hardware_test_edit_1, text, 0);
}

int hardware_test_gsensor_stop(void)
{
    if (gsensor_monitor_timer != NULL) {
        lv_timer_delete(gsensor_monitor_timer);
        gsensor_monitor_timer = NULL;
    }
    gsensor_test_running = 0;
    if (gsensor_opened_here) {
        sensor_hub_enable(SENSOR_DRV_ACCELER, 0);
        gsensor_opened_here = 0;
    }
    return 0;
}

int hardware_test_gsensor_start(void)
{
    sensor_info_t *info;
    sensor_state_t init_state;

    if (gsensor_test_running) {
        printf("gsensor test is already running\n");
        return 0;
    }

    if (sensor_driver_bus_init() != 0) {
        printf("sensor_driver_bus_init fail -- \n");
        return -1;
    }

    info = sensor_hub_get_info(SENSOR_DRV_ACCELER);
    if (info == NULL) {
        printf("gsensor not found\n");
        return -1;
    }

    init_state = sensor_hub_get_sensor_state(SENSOR_DRV_ACCELER);
    gsensor_opened_here = 0;
    if (init_state != SENSOR_STATE_OPEN) {
        sensor_hub_enable(SENSOR_DRV_ACCELER, 1);
        gsensor_opened_here = 1;
    }
    if (sensor_hub_get_sensor_state(SENSOR_DRV_ACCELER) != SENSOR_STATE_OPEN) {
        printf("gsensor open failed\n");
        return -1;
    }

    gsensor_monitor_timer = lv_timer_create(gsensor_monitor_cb, 100, NULL);
    if (gsensor_monitor_timer == NULL) {
        printf("gsensor monitor timer create failed\n");
        return -1;
    }
    lv_timer_ready(gsensor_monitor_timer);
    gsensor_test_running = 1;
    return 0;
}

/*==================== ADKEY ====================*/
static u8 adkey_test_enabled = 0;

int hardware_test_adkey_start(void)
{
    adkey_test_enabled = 1;
    return 0;
}

/* 复位 ADKEY 测试使能(切换测试项时调用)。 */
static int hardware_test_adkey_stop(void)
{
    adkey_test_enabled = 0;
    return 0;
}

int hardware_test_adkey_is_active(void)
{
    gui_scr_t *screen;

    if (!adkey_test_enabled) {
        return 0;
    }
    screen = gui_scr_get_act();
    if (screen == NULL || screen->id != GUI_SCREEN_HARDWARE_TEST) {
        return 0;
    }
    return 1;
}

int hardware_test_adkey_key_down(int adkey_number)
{
    if (!hardware_test_adkey_is_active()) {
        return 0;
    }
    if (adkey_number == 0 || adkey_number > 20) {
        return 0;
    }
    printf("ADKEY%d DOWN\n", adkey_number);
    return 1;
}

/*==================== BT ====================*/
const char *hardware_test_bt_get_name(void)
{
    return bt_get_local_name();
}

/*==================== 单活动测试项管理 ====================*/
static hw_test_item_t hw_active_item = HW_TEST_NONE;

hw_test_item_t hardware_test_get_active_item(void)
{
    return hw_active_item;
}

/*==================== 运行中测试项按钮高亮(须在 LVGL 线程调用) ====================*/
static lv_obj_t *hw_framed_btn = NULL;

static lv_style_t *hw_highlight_style(void)
{
    static u8 inited = 0;
    static lv_style_t s;
    if (!inited) {
        lv_style_init(&s);
        lv_style_set_border_width(&s, 3);
        lv_style_set_border_color(&s, lv_color_make(0xff, 0x00, 0x00));
        lv_style_set_border_opa(&s, 255);
        inited = 1;
    }
    return &s;
}

static lv_obj_t *hw_test_button_of(hw_test_item_t it)
{
    lv_ui_hardware_test *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_HARDWARE_TEST);
    if (ui_scr == NULL) {
        return NULL;
    }
    switch (it) {
    case HW_TEST_LTE:
        return ui_scr->hardware_test_lte_4g;
    case HW_TEST_MIC1:
        return ui_scr->hardware_test_mic0;
    case HW_TEST_MIC2:
        return ui_scr->hardware_test_btn_6;
    case HW_TEST_CAN:
        return ui_scr->hardware_test_can;
    case HW_TEST_GPS:
        return ui_scr->hardware_test_btn_4;
    case HW_TEST_WIFI:
        return ui_scr->hardware_test_btn_5;
    case HW_TEST_GSENSOR:
        return ui_scr->hardware_test_btn_gsensor;
    case HW_TEST_ADKEY:
        return ui_scr->hardware_test_btn_7;
    case HW_TEST_BT:
        return ui_scr->hardware_test_btn_bt;
    default:
        return NULL;
    }
}

/* 按当前活动项刷新高亮: 框住正在运行的测试按钮, 无活动项则清空。 */
void hardware_test_ui_highlight_sync(void)
{
    lv_obj_t *target = hw_test_button_of(hw_active_item);
    if (target == hw_framed_btn) {
        return;   /* 无变化 */
    }
    if (hw_framed_btn != NULL && lv_obj_is_valid(hw_framed_btn)) {
        lv_obj_remove_style(hw_framed_btn, hw_highlight_style(), LV_PART_MAIN);
    }
    hw_framed_btn = target;
    if (target != NULL) {
        lv_obj_add_style(target, hw_highlight_style(), LV_PART_MAIN);
    }
}

/* 供 events_init 在 LVGL 线程周期刷新(异步结束如 GPS 完成后取消高亮) */
void hardware_test_ui_highlight_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    hardware_test_ui_highlight_sync();
    hw_net_flush_result();   /* 顺带刷新后台 worker(WiFi/4G)产生的测试结果 */
}

int hardware_test_switch_item(hw_test_item_t next)
{
    if (next == hw_active_item) {
        return 1;   /* 重复点击当前运行项 → 忽略 */
    }

    /* 停掉上一个活动项 */
    switch (hw_active_item) {
    case HW_TEST_GSENSOR:
        hardware_test_gsensor_stop();
        break;
    case HW_TEST_GPS:
        hardware_test_gps_stop();
        break;
    case HW_TEST_WIFI:
        /* UI Effect 退出到 app_select/MAP 时保留 WiFi, 便于测试地图导航 */
        if (next != HW_TEST_NONE) {
            hardware_test_wifi_stop();
        } else {
#if TCFG_EXT_WIFI_ENABLE
            /*  确保 CTP(手机APP/导航命令通道)已初始化 */
            extern int ctp_is_init(void);
            extern int ctp_init(u8 use_alive);
            if (!ctp_is_init()) {
                ctp_init(1);
            }
#endif
        }
        break;
    case HW_TEST_CAN:
        hardware_test_can_stop();
        break;
    case HW_TEST_MIC1:
    case HW_TEST_MIC2:
        hardware_test_mic_stop();
        break;
    case HW_TEST_ADKEY:
        hardware_test_adkey_stop();
        break;
    default:
        break;   /* 4G/BT: 无后台操作, 仅切状态 */
    }

    hw_active_item = next;
    hardware_test_ui_highlight_sync();   /* 立即框住新按钮 */
    return 0;
}

void hardware_test_item_done(void)
{
    hw_active_item = HW_TEST_NONE;
    /* 注意: item_done 可能在 GPS 等后台线程调用, 不在此直接碰 LVGL,
       高亮由 LVGL 线程的周期定时器 hardware_test_ui_highlight_timer_cb 刷新清除。 */
}

#else /* !TCFG_DASHBOARD_HARDWARE_TEST_ENABLE */

int hardware_test_lte_4g_start(void)
{
    return -1;
}

int hardware_test_mic_start(int mic_idx)
{
    (void)mic_idx;
    return -1;
}

int hardware_test_can_start(void)
{
    return -1;
}

int hardware_test_gps_start(void)
{
    return -1;
}

int hardware_test_wifi_start(void)
{
    return -1;
}

void hardware_test_wifi_stop_wait(int timeout_ms)
{
    (void)timeout_ms;
}

int hardware_test_gsensor_start(void)
{
    return -1;
}

int hardware_test_gsensor_stop(void)
{
    return -1;
}

int hardware_test_adkey_start(void)
{
    return -1;
}

int hardware_test_adkey_is_active(void)
{
    return 0;
}

int hardware_test_adkey_key_down(int adkey_number)
{
    (void)adkey_number;
    return 0;
}

const char *hardware_test_bt_get_name(void)
{
    return NULL;
}

void hardware_test_show_message(lv_obj_t *edit, const char *text, uint32_t timeout_ms)
{
    (void)edit;
    (void)text;
    (void)timeout_ms;
}

void hardware_test_show_result(lv_obj_t *edit, const char *text)
{
    (void)edit;
    (void)text;
}

void hardware_test_hide_message(lv_obj_t *edit)
{
    (void)edit;
}

void hardware_test_ui_adkey_down(int adkey_number)
{
    (void)adkey_number;
}

int hardware_test_switch_item(hw_test_item_t next)
{
    (void)next;
    return 0;
}

void hardware_test_item_done(void)
{
}

hw_test_item_t hardware_test_get_active_item(void)
{
    return HW_TEST_NONE;
}

void hardware_test_ui_highlight_sync(void)
{
}

void hardware_test_ui_highlight_timer_cb(lv_timer_t *timer)
{
    (void)timer;
}

#endif /* TCFG_DASHBOARD_HARDWARE_TEST_ENABLE */
