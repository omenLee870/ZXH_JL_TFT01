/**
 * @file ui_meter_settings.c
 * @brief 连接 JLUI 生成的控件和本机设置服务，管理自动熄屏与触摸/按键唤醒。
 * @note 布局和事件保存在 JLUI 工程；本文件不创建替代设置界面，不处理车辆控制器参数。
 */
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "system/includes.h"
#endif
#include "lvgl.h"
#include "../generated/gui_guider.h"
#include "ui_meter_settings.h"
#include <stdlib.h>
#include <string.h>
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "meter_settings.h"
#include "app_vehicle.h"
#include "app_meter_control.h"
#include "screen_mirror.h"
#include "meter_environment.h"
#else
#include <stdint.h>
struct meter_preferences
{
    uint32_t brightness, music, tone, key_tone, unit, sleep_minutes, revision;
};
static struct meter_preferences simulated = {70, 50, 30, 1, 0, 0, 1};
static void meter_settings_snapshot(struct meter_preferences *p)
{
    *p = simulated;
}
#endif
#define W(name) ui->meter_settings_##name
static unsigned selected, local_pending, result_seen;
static unsigned time_pending, time_result_seen, time_loading;
static int confirm_key = -1, last_brightness = -1;
static volatile unsigned asleep, wake_requested;
static unsigned last_revision;
static lv_timer_t *settings_timer;
static void time_load(void);
static lv_ui_meter_settings *page(void)
{
    return ui_get_scr_ptr(&guider_ui, GUI_SCREEN_METER_SETTINGS);
}
static void hidden(lv_obj_t *obj, int hide)
{
    if (hide)
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
static void highlight(lv_obj_t *obj, int active)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(active ? 0x268bff : 0x0d1522), 0);
}
void ui_meter_settings_select(unsigned index)
{
    lv_ui_meter_settings *ui = page();
    if (!ui || index > 6)
        return;
    selected = index;
    lv_obj_t *panels[] = {W(panel_0), W(panel_1), W(panel_2), W(panel_3), W(panel_4), W(panel_5), W(panel_6)};
    lv_obj_t *menus[] = {W(menu_0), W(menu_1), W(menu_2), W(menu_3), W(menu_4), W(menu_5), W(menu_6)};
    for (unsigned i = 0; i < 7; i++)
    {
        hidden(panels[i], i != index);
        highlight(menus[i], i == index);
    }
    ui_meter_settings_cancel();
    if (index == 6) time_load();
}
/** @brief 滚轮只编辑草稿；闰年和小月同步调整日期项，不能产生无效公历日期。 */
void ui_meter_settings_time_edit(void)
{
    lv_ui_meter_settings *ui=page();
    if (!ui || time_loading) return;
    unsigned year=2020+lv_roller_get_selected(W(time_year));
    unsigned month=1+lv_roller_get_selected(W(time_month));
    static const unsigned char lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned count=lengths[month-1]+(month==2 && !(year%4) && (year%100 || !(year%400)));
    if (lv_roller_get_option_count(W(time_day)) != count) {
        unsigned day=lv_roller_get_selected(W(time_day)); char options[96]; unsigned used=0;
        for (unsigned i=1;i<=count;i++) used+=snprintf(options+used,sizeof(options)-used,i==count?"%02u":"%02u\n",i);
        time_loading=1;
        lv_roller_set_options(W(time_day),options,LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(W(time_day),day<count?day:count-1,LV_ANIM_OFF);
        time_loading=0;
    }
}
static void time_load(void)
{
    lv_ui_meter_settings *ui=page();
    if (!ui) return;
    unsigned year=atoi(__DATE__+7),month=1,day=atoi(__DATE__+4),hour=0,minute=0;
    const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
    for (unsigned i=0;i<12;i++) if (!strncmp(__DATE__,months+i*3,3)) month=i+1;
#if !LV_USE_GUIBUILDER_SIMULATOR
    struct meter_local_time value;
    if (!meter_environment_local_time(&value)) {
        year=value.year;month=value.month;day=value.day;hour=value.hour;minute=value.minute;
    }
#endif
    if (year<2020 || year>2099) year=2026;
    time_loading=1;
    lv_roller_set_selected(W(time_year),year-2020,LV_ANIM_OFF);
    lv_roller_set_selected(W(time_month),month-1,LV_ANIM_OFF);
    lv_roller_set_selected(W(time_hour),hour,LV_ANIM_OFF);
    lv_roller_set_selected(W(time_minute),minute,LV_ANIM_OFF);
    time_loading=0;
    ui_meter_settings_time_edit();
    lv_roller_set_selected(W(time_day),day-1,LV_ANIM_OFF);
}
static void slider(lv_obj_t *obj, lv_obj_t *label, unsigned value)
{
    /* 拖动期间及等待保存结果时保持手指选定值，避免 100ms 轮询把滑块拉回旧位置。 */
    if (!local_pending && !lv_slider_is_dragged(obj) && !lv_obj_has_state(obj, LV_STATE_PRESSED))
        lv_slider_set_value(obj, value, LV_ANIM_OFF);
    lv_label_set_text_fmt(label, "%u%%", (unsigned)lv_slider_get_value(obj));
}
static void mileage(lv_obj_t *label, uint64_t mm, unsigned miles)
{
    uint64_t denominator = miles ? 1609344 : 1000000;
    uint64_t tenth = mm / denominator * 10 + (mm % denominator) * 10 / denominator;
    char buffer[21],*end = buffer + 20;
    *end = 0;
    unsigned decimal = tenth % 10;
    tenth /= 10;
    do
    {
        *--end = (char)('0' + tenth % 10);
        tenth /= 10;
    }
    while (tenth);
    lv_label_set_text_fmt(label, "%s.%u %s", end, decimal, miles ? "mi" : "km");
}
static void refresh_page(void)
{
    lv_ui_meter_settings *ui = page();
    struct meter_preferences p;
    if (!ui || lv_screen_active() != ui->meter_settings)
        return;
    meter_settings_snapshot(&p);
    slider(W(brightness), W(brightness_value), p.brightness);
    slider(W(music), W(music_value), p.music);
    slider(W(tone), W(tone_value), p.tone);
    if (!local_pending)
    {
        if (p.key_tone)
            lv_obj_add_state(W(key_tone), LV_STATE_CHECKED);
        else
            lv_obj_remove_state(W(key_tone), LV_STATE_CHECKED);
    }
    highlight(W(unit_km), !p.unit);
    highlight(W(unit_mi), p.unit);
    highlight(W(sleep_0), !p.sleep_minutes);
    highlight(W(sleep_1), p.sleep_minutes == 1);
    highlight(W(sleep_3), p.sleep_minutes == 3);
    highlight(W(sleep_5), p.sleep_minutes == 5);
    lv_obj_t *time_controls[]={W(time_year),W(time_month),W(time_day),W(time_hour),W(time_minute),W(time_save)};
    for (unsigned i=0;i<sizeof(time_controls)/sizeof(time_controls[0]);i++) {
        if (time_pending) lv_obj_add_state(time_controls[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(time_controls[i],LV_STATE_DISABLED);
    }
#if !LV_USE_GUIBUILDER_SIMULATOR
    struct vehicle_state v;
    app_vehicle_snapshot(&v);
    mileage(W(trip), v.values.trip_mm, p.unit);
    mileage(W(odo), v.values.odometer_mm, p.unit);
    lv_label_set_text(W(firmware), __DATE__ "\n" __TIME__);
#else
    mileage(W(trip), 0, p.unit);
    mileage(W(odo), 0, p.unit);
    lv_label_set_text(W(firmware), "SIMULATOR");
#endif
}
/** @brief 仅失败时显示简短原因，不能把关机、忙等状态统称为保存失败。 */
static void show_error(int code)
{
    lv_ui_meter_settings *ui = page();
    if (!ui || lv_screen_active() != ui->meter_settings)
        return;
    const char *message = code == 2003 ? "行驶中不能清零" :
                          code == 2006 ? "请先启动仪表" :
                          code == 1004 ? "稍后重试" : "保存失败";
    lv_label_set_text(W(confirm_title), message);
    hidden(W(confirm_cancel), 1);
    hidden(W(confirm), 0);
    lv_obj_move_foreground(W(confirm));
    confirm_key = -1;
}
/** @brief 保存请求由 app_core 写 RTC，UI 只等待执行结果，不在触摸回调里访问 Flash。 */
void ui_meter_settings_time_save(void)
{
    lv_ui_meter_settings *ui=page();
    if (!ui || selected!=6 || time_pending || local_pending) return;
    ui_meter_settings_time_edit();
#if !LV_USE_GUIBUILDER_SIMULATOR
    struct meter_local_time value={2020+lv_roller_get_selected(W(time_year)),
        1+lv_roller_get_selected(W(time_month)),1+lv_roller_get_selected(W(time_day)),
        lv_roller_get_selected(W(time_hour)),lv_roller_get_selected(W(time_minute))};
    int code;time_result_seen=meter_environment_time_result(&code);
    if (meter_environment_time_post(&value)) {show_error(1004);return;}
    time_pending=1;
#endif
    lv_display_trigger_activity(NULL);refresh_page();
}
void ui_meter_settings_change(unsigned key, int value)
{
    if (local_pending || time_pending)
        return;
#if !LV_USE_GUIBUILDER_SIMULATOR
    int code;
    result_seen = meter_settings_result(&code);
    if (meter_settings_post(key, value))
    {
        show_error(1004);
        refresh_page();
        return;
    }
    local_pending = 1;
#else
    uint32_t *fields[] = {&simulated.brightness, &simulated.music, &simulated.tone, &simulated.key_tone, &simulated.unit, &simulated.sleep_minutes};
    if (key < 6)
        * fields[key] = value;
    else if (key == 7)
    {
        struct meter_preferences p = {70, 50, 30, 1, 0, 0, 1};
        simulated = p;
    }
#endif
    lv_display_trigger_activity(NULL);
    refresh_page();
}
void ui_meter_settings_confirm(unsigned key)
{
    lv_ui_meter_settings *ui = page();
    if (!ui || local_pending)
        return;
    confirm_key = key;
    lv_label_set_text(W(confirm_title), key == 6 ? "清零小计里程？" : "恢复默认设置？");
    hidden(W(confirm_cancel), 0);
    hidden(W(confirm), 0);
    lv_obj_move_foreground(W(confirm));
}
void ui_meter_settings_cancel(void)
{
    lv_ui_meter_settings *ui = page();
    if (ui)
        hidden(W(confirm), 1);
    confirm_key = -1;
}
void ui_meter_settings_commit(void)
{
    int key = confirm_key;
    ui_meter_settings_cancel();
    if (key >= 0)
        ui_meter_settings_change(key, 0);
}
void ui_meter_settings_open(void)
{
#if !LV_USE_GUIBUILDER_SIMULATOR
    if (!app_meter_control_media_allowed())
        return;
#endif
    ui_load_scr_anim(&guider_ui, ui_get_scr(GUI_SCREEN_METER_SETTINGS), LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
    refresh_page();
}
void ui_meter_settings_close(void)
{
    ui_meter_settings_cancel();
    ui_load_scr_anim(&guider_ui, ui_get_scr(GUI_SCREEN_CHONGWUYIBIAO), LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
}
int ui_meter_settings_key_activity(void)
{
    unsigned was_asleep = asleep;
    wake_requested = 1;
    return was_asleep;
}
/** @brief 熄屏后的首次触摸只唤醒，避免在黑屏时误按清零、恢复等控件。 */
static void input_pressed(lv_event_t *e)
{
    if (asleep)
    {
        wake_requested = 1;
        lv_indev_wait_release((lv_indev_t *)lv_event_get_target(e));
    }
}
void ui_meter_settings_poll(void)
{
    struct meter_preferences p;
    meter_settings_snapshot(&p);
#if !LV_USE_GUIBUILDER_SIMULATOR
    int code;
    unsigned result = meter_settings_result(&code);
    if (local_pending && result != result_seen)
    {
        local_pending = 0;
        result_seen = result;
        if (code)
            show_error(code);
    }
    result=meter_environment_time_result(&code);
    if (time_pending && result!=time_result_seen) {
        time_pending=0;time_result_seen=result;
        if (code) show_error(code);
    }
    if (!app_meter_control_media_allowed())
    {
        last_brightness = -1;
        asleep = 0;
        return;
    }
    struct vehicle_state v;
    app_vehicle_snapshot(&v);
    unsigned keep_awake = v.motion == VEHICLE_MOVING || map_video_page_active();
    if (wake_requested || last_revision != p.revision || keep_awake)
    {
        lv_display_trigger_activity(NULL);
        wake_requested = 0;
        last_revision = p.revision;
    }
    unsigned should_sleep = p.sleep_minutes&&!keep_awake&&lv_display_get_inactive_time(NULL) >= p.sleep_minutes * 60000u;
    int brightness = should_sleep ? 0 : p.brightness;
    if (last_brightness != brightness)
    {
        extern int lcd_drv_backlight_ctrl(unsigned char);
        if (!lcd_drv_backlight_ctrl(brightness))
        {
            last_brightness = brightness;
            asleep = should_sleep;
        }
    }
#endif
    refresh_page();
}
static void settings_poll_timer(lv_timer_t *t)
{
    (void)t;
    ui_meter_settings_poll();
}
void ui_meter_settings_bind(void)
{
    lv_ui_meter_settings *ui = page();
    if (ui)
    {
        /* 菜单与详情仅允许上下滚动；滑条的触摸区扩大，细线外侧也可按住拖动。 */
        lv_obj_t *panels[] = {W(settings_menu), W(panel_0), W(panel_1), W(panel_2), W(panel_3), W(panel_4), W(panel_5), W(panel_6)};
        for (unsigned i = 0; i < sizeof(panels) / sizeof(panels[0]); i++)
            lv_obj_set_scroll_dir(panels[i], LV_DIR_VER);
        lv_obj_t *sliders[] = {W(brightness), W(music), W(tone)};
        for (unsigned i = 0; i < sizeof(sliders) / sizeof(sliders[0]); i++)
            lv_obj_set_ext_click_area(sliders[i], 24);
        lv_obj_remove_flag(ui->meter_settings, LV_OBJ_FLAG_SCROLLABLE);
    }
    ui_meter_settings_select(selected);
    refresh_page();
    if (!settings_timer)
    {
        settings_timer = lv_timer_create(settings_poll_timer, 100, NULL);
        for (lv_indev_t *input = lv_indev_get_next(NULL); input; input = lv_indev_get_next(input))
            lv_indev_add_event_cb(input, input_pressed, LV_EVENT_PRESSED, NULL);
    }
}
