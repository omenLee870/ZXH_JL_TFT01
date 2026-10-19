/**
 * @file youyibao_sif_ui.c
 * @brief 将统一车辆状态显示到现有普通仪表页和宠物仪表页。
 * @note 本文件只在 UI 线程操作 LVGL；车辆数据通过业务快照读取，
 *       同时保留现有自动倒车切页和返回规则，不承担协议解码或里程累计。
 */
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#endif
#include "lvgl.h"
#include "youyibao_sif_ui.h"
#ifndef YOUYIBAO_UI_TEST
#include "ui_environment.h"
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR || defined(YOUYIBAO_UI_TEST)
#include "app_vehicle.h"
#ifndef YOUYIBAO_UI_TEST
#include "meter_settings.h"
#else
static int meter_settings_miles(void) { return 0; }
#endif
#include "app_meter_control.h"
#include "../generated/gui_guider.h"
#include "../generated/gui_fonts/gui_fonts.h"
#include "chongwuyibiao_donghua.h"
#include "pet_reverse_notice.h"
#include "ui_update_page.h"
#include <stdio.h>
#include <string.h>

#if TCFG_YYB_SIF_ENABLE
static lv_timer_t *refresh_timer;       /**< UI 线程中的 100ms 刷新定时器。 */
static lv_obj_t *auto_return_root;      /**< 自动倒车前的页面根对象，用于核对返回页是否仍有效。 */
static int32_t auto_return_id;          /**< 自动倒车前的页面编号。 */
static unsigned previous_reverse;      /**< 上次倒车状态，用于识别进入倒车的边沿。 */
static struct vehicle_state latest;    /**< 本次刷新获取的独立业务状态副本。 */
static lv_obj_t *pet_root;             /**< 已绑定的宠物页根对象，删除页面时清空。 */
/* 关闭页为独立屏幕根对象，无父对象；只用 lv_screen_load 切换，禁止设置或移除 HIDDEN。 */
static lv_obj_t *meter_standby_cover, *meter_resume_root;
static lv_timer_t *control_timer;
static unsigned meter_update_visible;
static unsigned meter_background_resume;
static struct app_meter_state meter_display = {1, 0, 1, 0};
enum pet_distance_mode {
    PET_DISTANCE_ODO,  /**< 仪表累计总里程。 */
    PET_DISTANCE_TRIP, /**< 本次小计里程。 */
    PET_DISTANCE_TIME, /**< 本次骑行时长，显示小时和分钟。 */
    PET_DISTANCE_COUNT
};
static enum pet_distance_mode pet_distance_mode; /**< 显示选择独立于页面生命周期，重建页面时保留。 */
static void refresh(lv_timer_t *timer);
static unsigned pet_status(uint32_t flags);
static void pet_refresh(lv_ui_chongwuyibiao *ui);

/**
 * @brief 设置控件是否显示。
 * @param[in] o 目标控件，允许为空。
 * @param[in] show 非 0 显示，0 隐藏。
 * @retval 无。
 */
static void visible(lv_obj_t *o, unsigned show)
{
    if (!o) return;
    if (show) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

/** @brief 刷新本机灯光和模式；一线通掉线也不清除 APP 对仪表的显示选择。 */
static void pet_meter_refresh(lv_ui_chongwuyibiao *ui)
{
    static const char *names[] = {"ECO", "CITY", "SPORT"};
    static const uint32_t colors[] = {0x31c48d, 0x36a9ff, 0xff9f43};
    unsigned mode = meter_display.mode < 0 || meter_display.mode > 2 ? 1 : meter_display.mode;
    const struct vehicle_values *v = &latest.values;
    char gear[16];
    visible(ui->chongwuyibiao_jinguangdengtubiao, meter_display.light_on);
    if (pet_status(VEHICLE_PARK)) strcpy(gear, "P");
    else if (pet_status(VEHICLE_REVERSE)) strcpy(gear, "R");
    else if (latest.link == VEHICLE_LINK_ONLINE && (v->valid & VEHICLE_GEAR_VALID)) snprintf(gear, sizeof(gear), "GEAR %u", (unsigned)v->gear);
    else strcpy(gear, "--");
    /* 复用原有底部信息控件，不在图片区域额外创建模式文字。 */
    lv_label_set_text_fmt(ui->chongwuyibiao_jiashimoshi, "%s / %s", names[mode], gear);
    lv_obj_set_style_text_color(ui->chongwuyibiao_jiashimoshi, lv_color_hex(colors[mode]), 0);
    lv_obj_set_style_text_color(ui->chongwuyibiao_chesushuzi, lv_color_hex(colors[mode]), 0);
}

/** @brief 在 UI 线程调用背光驱动，屏幕基础驱动保留以支持 BLE 再开机。 */
static int meter_backlight(int power_on)
{
#ifdef YOUYIBAO_UI_TEST
    extern int meter_ui_test_backlight(int power_on);
    return meter_ui_test_backlight(power_on);
#else
    extern int lcd_drv_backlight_ctrl(unsigned char percent);
    extern unsigned char lcd_user_brightness_percent(void);
    unsigned char brightness = lcd_user_brightness_percent();
    return lcd_drv_backlight_ctrl(power_on ? (brightness ? brightness : 100) : 0);
#endif
}

/**
 * @brief 执行电源状态机中的 UI 退出或恢复阶段。
 * @param[in] power_on 非 0 恢复正常界面，0 退出到无业务控件的关闭页。
 * @param[in] light_on 大灯图标状态。
 * @param[in] mode 已保存的显示模式。
 * @return 0 完成，-1 背光或页面申请失败，-2 正在升级。
 * @note 页面真实卸载负责退出动画、音乐动效、摄像头与投屏解码；不只覆盖黑色遮罩。
 */
int youyibao_sif_ui_meter_apply(int power_on, int light_on, int mode)
{
    if (ui_update_page_is_active()) return -2;
    if (!power_on) {
        if (!meter_standby_cover) {
            meter_standby_cover = lv_obj_create(NULL);
            if (!meter_standby_cover) return -1;
            lv_obj_remove_style_all(meter_standby_cover);
            lv_obj_set_size(meter_standby_cover, lv_pct(100), lv_pct(100));
            lv_obj_set_style_bg_color(meter_standby_cover, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(meter_standby_cover, LV_OPA_COVER, 0);
            lv_obj_remove_flag(meter_standby_cover, LV_OBJ_FLAG_SCROLLABLE);
        }
        if (meter_backlight(0)) return -1;
        if (meter_display.power_on) {
            gui_scr_t *active = gui_scr_get_act();
            meter_resume_root = lv_screen_active();
            /* 倒车/投屏来源必须在开机后重新确认，不能恢复关机前的摄像头或画面。 */
            if (active && (active->id == GUI_SCREEN_PAGE_VIDEO || active->id == GUI_SCREEN_PAGE_MAP))
                meter_resume_root = pet_root;
        }
        if (refresh_timer) lv_timer_pause(refresh_timer);
        pet_reverse_notice_request(false);
        auto_return_root = NULL; previous_reverse = 0;
        /* 根页面没有父对象，不能用普通控件的 visible()：
         * SDK 移除 HIDDEN 时会更新父对象布局，第二次关机会解引用空指针。 */
        if (lv_screen_active() != meter_standby_cover) lv_screen_load(meter_standby_cover);
#if !LV_USE_GUIBUILDER_SIMULATOR
        extern void dashboard_ui_freeze_background(void);
        dashboard_ui_freeze_background();
        if (guider_ui.default_group) lv_group_remove_all_objs(guider_ui.default_group);
#endif
        meter_display.power_on = 0; meter_display.light_on = 0;
        meter_update_visible = 0;
        return 0;
    }
    if (!meter_display.power_on) {
        if (!meter_resume_root || !lv_obj_is_valid(meter_resume_root)) {
            gui_scr_t *screen = ui_get_setup_scr(GUI_SCREEN_CHONGWUYIBIAO);
            if (!screen || !screen->scr) return -1;
            meter_resume_root = screen->scr;
        }
        /* 先读重新初始化后的未知/新鲜快照，再加载页面，最后才开背光。 */
        app_vehicle_snapshot(&latest);
        meter_display.light_on = light_on; meter_display.mode = mode;
        if (pet_root) {
            lv_ui_chongwuyibiao *ui = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_CHONGWUYIBIAO);
            if (ui && ui->chongwuyibiao == pet_root) pet_refresh(ui);
        }
        lv_screen_load(meter_resume_root);
#if !LV_USE_GUIBUILDER_SIMULATOR
        extern void set_default_group_cb(void);
        set_default_group_cb();
#endif
        if (meter_backlight(1)) {
            lv_screen_load(meter_standby_cover);
#if !LV_USE_GUIBUILDER_SIMULATOR
            if (guider_ui.default_group) lv_group_remove_all_objs(guider_ui.default_group);
#endif
            return -1;
        }
        /* 关闭页已被切出，不会显示；保留根对象用于下次关机，不修改其可见标志。 */
        if (refresh_timer) { lv_timer_reset(refresh_timer); lv_timer_resume(refresh_timer); }
        meter_background_resume = 1;
    }
    meter_display.power_on = 1; meter_display.light_on = light_on; meter_display.mode = mode;
    if (pet_root) {
        lv_ui_chongwuyibiao *ui = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_CHONGWUYIBIAO);
        if (ui && ui->chongwuyibiao == pet_root) pet_meter_refresh(ui);
    }
    return 0;
}

/** @brief 升级仅临时显示进度，不重启采集、动画或媒体，也不改变业务电源状态。 */
void youyibao_sif_ui_update_wake(void)
{
    if (!meter_display.power_on && !meter_backlight(1)) meter_update_visible = 1;
}

/** @brief 唯一保留的 UI 控制定时器：处理启动/关闭和升级结束后的关闭页恢复。 */
static void meter_control_bridge(lv_timer_t *timer)
{
    (void)timer;
    app_meter_control_ui_poll();
#if !LV_USE_GUIBUILDER_SIMULATOR
    if (meter_background_resume && app_meter_control_media_allowed()) {
        extern void dashboard_ui_resume_background(void);
        meter_background_resume = 0;
        dashboard_ui_resume_background();
    }
#endif
    if (!meter_display.power_on && !ui_update_page_is_active() &&
        (meter_update_visible || (meter_standby_cover && lv_screen_active() != meter_standby_cover)))
        youyibao_sif_ui_meter_apply(0, 0, meter_display.mode);
}

/**
 * @brief 根据业务故障位选择当前需要显示的故障名称。
 * @param[in] f 统一车辆故障位图。
 * @param[in] tick UI 毫秒时基，用于多个故障每 1.5 秒轮播。
 * @return 指向静态故障名称字符串的指针，无需释放。
 * @note 显示名称不是厂家故障码；通信掉线由刷新函数单独显示。
 */
static const char *fault_name(uint32_t f, uint32_t tick)
{
    static const struct { uint32_t flag; const char *name; } names[] = {
        {VEHICLE_HALL_FAULT, "HALL"}, {VEHICLE_THROTTLE_FAULT, "THROTTLE"},
        {VEHICLE_CONTROLLER_FAULT, "CONTROLLER"}, {VEHICLE_UNDERVOLTAGE, "LOW BATTERY"},
        {VEHICLE_PHASE_FAULT, "MOTOR PHASE"}, {VEHICLE_PROTECTION, "PROTECTION"},
        {VEHICLE_ANTI_RUNAWAY, "RUNAWAY"}, {VEHICLE_OVERCURRENT, "OVERCURRENT"},
        {VEHICLE_STALL, "MOTOR STALL"}
    };
    unsigned n = 0, i, selected;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (f & names[i].flag) ++n;
    if (!n) return "OK";
    selected = (tick / 1500) % n;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if ((f & names[i].flag) && selected-- == 0) return names[i].name;
    return "FAULT";
}

/**
 * @brief 检查当前接入方式已知且有效的车辆状态。
 * @param[in] flags 待检查的状态位；任意一位置位即返回真。
 * @return 非 0 表示在线且状态有效，0 表示未激活或数据未知。
 */
static unsigned pet_status(uint32_t flags)
{
    const struct vehicle_values *v = &latest.values;
    return latest.link == VEHICLE_LINK_ONLINE && (v->valid & VEHICLE_STATUS_VALID) &&
        (v->known_status & v->status & flags);
}

/**
 * @brief 将无符号 64 位计数转换为十进制文本。
 * @param[in] value 待显示的计数值。
 * @param[out] buffer 至少 21 字节的缓冲区。
 * @return 指向缓冲区中第一个数字的指针。
 * @note 避免先转成 32 位导致里程截断，也不依赖嵌入式 printf 的 64 位格式支持。
 */
static const char *pet_counter_text(uint64_t value, char buffer[21])
{
    char *end = buffer + 20;
    *end = '\0';
    do {
        *--end = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    return end;
}

/**
 * @brief 根据显示选择刷新底部里程或骑行时长。
 * @param[in] ui 当前宠物页控件，不能为空。
 * @retval 无。
 * @note 只读取业务结果，不累计或清零数据；掉线后仍可查看已累计里程与时长。
 */
static void pet_distance_refresh(lv_ui_chongwuyibiao *ui)
{
    char text[32], whole[21];
    uint64_t distance = pet_distance_mode == PET_DISTANCE_TRIP ?
        latest.values.trip_mm : latest.values.odometer_mm;
    if (meter_settings_miles()) {
        uint64_t tenths = distance / 1609344 * 10 + (distance % 1609344) * 10 / 1609344;
        distance = tenths * 100000;
    }
    if (pet_distance_mode == PET_DISTANCE_TIME) {
        uint64_t minutes = latest.ride_time_ms / 60000;
        uint64_t hours = minutes / 60;
        snprintf(text, sizeof(text), "%s%s:%02u", hours < 10 ? "0" : "",
            pet_counter_text(hours, whole), (unsigned)(minutes % 60));
    } else if (latest.values.valid & VEHICLE_DISTANCE_VALID) {
        snprintf(text, sizeof(text), "%s.%u", pet_counter_text(distance / 1000000, whole),
            (unsigned)(distance / 100000 % 10));
    } else strcpy(text, "--");
    lv_label_set_text(ui->chongwuyibiao_lichengbiaoti,
        pet_distance_mode == PET_DISTANCE_ODO ? "ODO" : pet_distance_mode == PET_DISTANCE_TRIP ? "TRIP" : "TIME");
    lv_label_set_text(ui->chongwuyibiao_lichengshuzi, text);
    lv_label_set_text(ui->chongwuyibiao_lichengdanwei, pet_distance_mode == PET_DISTANCE_TIME ? "H:M" : meter_settings_miles() ? "MI" : "KM");
}

/**
 * @brief 把统一车辆快照显示到宠物仪表控件。
 * @param[in] ui 当前宠物页控件，不能为空。
 * @retval 无。
 * @note 车速、电量和档位分别检查有效位；首次等待与通信掉线分开提示。
 *       刹车和巡航只显示实际反馈，不从动画或界面示例反推车辆状态。
 */
static void pet_refresh(lv_ui_chongwuyibiao *ui)
{
    const struct vehicle_values *v = &latest.values;
    unsigned online = latest.link == VEHICLE_LINK_ONLINE;
    unsigned valid_speed = online && (v->valid & VEHICLE_SPEED_VALID);
    unsigned speed = valid_speed ? (unsigned)((v->speed_dkph * (meter_settings_miles() ? 62137u : 100000u) + 500000u) / 1000000u) : 0;
    lv_label_set_text(ui->chongwuyibiao_chesudanwei, meter_settings_miles() ? "mph" : "km/h");
    char text[24];
    chongwuyibiao_chesu_gengxin(speed > 65535 ? 65535 : (uint16_t)speed);
    if (valid_speed) snprintf(text, sizeof(text), "%u", speed);
    else strcpy(text, "--");
    lv_label_set_text(ui->chongwuyibiao_chesushuzi, text);
    if (online && (v->valid & VEHICLE_SOC_VALID)) snprintf(text, sizeof(text), "%u%%", (unsigned)v->soc);
    else strcpy(text, "--%");
    lv_label_set_text(ui->chongwuyibiao_dianliangshuzi, text);
    lv_bar_set_value(ui->chongwuyibiao_dianliangtiao, online && (v->valid & VEHICLE_SOC_VALID) ? v->soc : 0, LV_ANIM_OFF);
    visible(ui->chongwuyibiao_guzhangxinxi, !online || ((v->valid & VEHICLE_FAULTS_VALID) && v->faults));
    lv_label_set_text(ui->chongwuyibiao_guzhangxinxi, latest.link == VEHICLE_LINK_WAITING ?
        "SIF WAIT" : !online ? "SIF OFFLINE" : fault_name(v->faults, lv_tick_get()));
    visible(ui->chongwuyibiao_shachetubiao, pet_status(VEHICLE_BRAKE | VEHICLE_EBRAKE));
    visible(ui->chongwuyibiao_xunhangtubiao, pet_status(VEHICLE_CRUISE));
    pet_distance_refresh(ui);
    pet_meter_refresh(ui);
#ifndef YOUYIBAO_UI_TEST
    ui_environment_refresh(ui);
#endif
}

/**
 * @brief 点按底部信息区域，依次切换总里程、小计里程和骑行时长。
 * @param[in] e LVGL 点击事件。
 * @retval 无。
 * @note 切换仅影响显示，不提交清零或车辆控制操作；升级期间不处理事件。
 */
static void pet_distance_clicked(lv_event_t *e)
{
    gui_scr_t *screen = gui_scr_get_act();
    if (ui_update_page_is_active() || !screen || screen->id != GUI_SCREEN_CHONGWUYIBIAO || screen->scr != pet_root) return;
    if (lv_obj_get_screen(lv_event_get_current_target(e)) != pet_root) return;
    pet_distance_mode = (enum pet_distance_mode)((pet_distance_mode + 1) % PET_DISTANCE_COUNT);
    refresh(NULL);
}

/**
 * @brief 页面删除时释放显示层保存的根对象引用。
 * @param[in] e 页面删除事件。
 * @retval 无。
 * @note 不保存可能先于控件释放的页面结构指针，也不重置车辆数据或显示选择。
 */
static void pet_deleted(lv_event_t *e)
{
    if (lv_event_get_target(e) == pet_root) pet_root = NULL;
}

/**
 * @brief 自动倒车页面开始卸载时取消其返回页归属和倒车播报。
 * @param[in] e LVGL 页面卸载事件，函数中未使用事件参数。
 * @retval 无。
 * @note 用户主动离开后清除归属，同一次倒车状态不会强行重新打开视频页。
 */
static void auto_video_unload(lv_event_t *e)
{
    (void)e;
    if (auto_return_root) pet_reverse_notice_request(false);
    auto_return_root = NULL;
}

/**
 * @brief 按有效倒车状态处理摄像头页面切入和返回。
 * @param[in] screen 当前页面，允许为空。
 * @retval 无。
 * @note 仅从现有仪表页自动切入，不抢占升级页；倒车结束或通信掉线后，
 *       只有本模块仍持有返回页归属且页面有效时才自动返回。
 */
static void reverse_update(gui_scr_t *screen)
{
    unsigned reverse = (latest.link == VEHICLE_LINK_ONLINE) && !!(latest.values.status & VEHICLE_REVERSE);
    if (reverse && !previous_reverse && screen &&
        (screen->id == GUI_SCREEN_CHONGWUYIBIAO || screen->id == GUI_SCREEN_PAGE_METER) &&
        !ui_update_page_is_active()) {
        gui_scr_t *video = ui_get_setup_scr(GUI_SCREEN_PAGE_VIDEO);
        if (video && video->scr) {
            lv_obj_remove_event_cb(video->scr, auto_video_unload);
            lv_obj_add_event_cb(video->scr, auto_video_unload, LV_EVENT_SCREEN_UNLOAD_START, NULL);
            auto_return_root = screen->scr;
            auto_return_id = screen->id;
            ui_load_scr_anim(&guider_ui, video, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
            if (gui_scr_get_act() == video) pet_reverse_notice_request(true);
            else auto_return_root = NULL;
        }
    } else if (!reverse && auto_return_root) {
        gui_scr_t *back = gui_scr_get(auto_return_id);
        if (screen && screen->id == GUI_SCREEN_PAGE_VIDEO && back &&
            back->scr == auto_return_root && lv_obj_is_valid(auto_return_root))
            ui_load_scr_anim(&guider_ui, back, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        pet_reverse_notice_request(false);
        auto_return_root = NULL;
    }
    previous_reverse = reverse;
}

/**
 * @brief 获取车辆业务快照并刷新当前仪表页面。
 * @param[in] timer LVGL 定时器，本函数未使用该参数。
 * @retval 无。
 * @note 在 UI 线程执行；先检查升级及倒车切页，再刷新实际活动页面。
 *       车速、电量无效时显示未知，不自行读取协议内部变量或计算里程。
 */
static void refresh(lv_timer_t *timer)
{
    gui_scr_t *screen = gui_scr_get_act();
    struct vehicle_values *v = &latest.values;
    char speed[16], gear[16];
    int valid_speed, online;
    (void)timer;
    struct app_meter_state local;
    if (app_meter_control_snapshot(&local) != ZXH_OK || !local.power_on) return;
    meter_display.light_on = local.light_on; meter_display.mode = local.mode;
    if (!meter_display.power_on) return;
    app_vehicle_snapshot(&latest);
    online = latest.link == VEHICLE_LINK_ONLINE;
    /* 资源升级恢复页没有普通仪表控件，升级期间不访问这些控件。 */
    if (ui_update_page_is_active()) return;
    reverse_update(screen);
    screen = gui_scr_get_act();
    if (!screen || (screen->id != GUI_SCREEN_CHONGWUYIBIAO && screen->id != GUI_SCREEN_PAGE_METER)) {
        return;
    }
    valid_speed = online && (v->valid & VEHICLE_SPEED_VALID);
    if (valid_speed) snprintf(speed, sizeof(speed), "%ld", (long)((v->speed_dkph + 5) / 10));
    else strcpy(speed, "--");
    if (!online) strcpy(gear, "--");
    else if (v->status & VEHICLE_PARK) strcpy(gear, "P");
    else if (v->status & VEHICLE_REVERSE) strcpy(gear, "R");
    else snprintf(gear, sizeof(gear), "GEAR %u", v->gear);
    if (screen->id == GUI_SCREEN_CHONGWUYIBIAO) {
        lv_ui_chongwuyibiao *ui = ui_get_scr_ptr(&guider_ui, screen->id);
        if (!ui) return;
        youyibao_sif_ui_bind_pet(ui);
        pet_refresh(ui);
    } else {
        lv_ui_page_meter *ui = ui_get_scr_ptr(&guider_ui, screen->id);
        if (!ui) return;
        if (ui->page_meter_timer_1) lv_timer_pause(ui->page_meter_timer_1);
        if (ui->page_meter_timer_2) lv_timer_pause(ui->page_meter_timer_2);
        if (ui->page_meter_timer_3) lv_timer_pause(ui->page_meter_timer_3);
        /* 一线通没有对应数据的示例灯光和警告图标保持隐藏，避免伪造车辆状态。 */
        lv_obj_t *sample_lamps[] = {ui->page_meter_img_1, ui->page_meter_img_2,
            ui->page_meter_img_3, ui->page_meter_img_4, ui->page_meter_img_5,
            ui->page_meter_img_7, ui->page_meter_img_10, ui->page_meter_img_11,
            ui->page_meter_img_12, ui->page_meter_img_13, ui->page_meter_img_14,
            ui->page_meter_img_15, ui->page_meter_img_16, ui->page_meter_img_17,
            ui->page_meter_img_18, ui->page_meter_img_20, ui->page_meter_img_21,
            ui->page_meter_imglist_4, ui->page_meter_imglist_5};
        unsigned i;
        for (i = 0; i < sizeof(sample_lamps) / sizeof(sample_lamps[0]); ++i)
            visible(sample_lamps[i], 0);
        visible(ui->page_meter_img_left_turn, 0);
        visible(ui->page_meter_img_right_turn, 0);
        visible(ui->page_meter_imgList_gear_mode, 0);
        visible(ui->page_meter_img_ready, 0); /* 当前一线通协议没有 READY 状态位。 */
        visible(ui->page_meter_imglist_speed_tens, valid_speed && v->speed_dkph < 995);
        visible(ui->page_meter_imglist_speed_ones, valid_speed && v->speed_dkph < 995);
        if (valid_speed && v->speed_dkph < 995) {
            unsigned s = (v->speed_dkph + 5) / 10;
            lv_imglist_set_act(ui->page_meter_imglist_speed_tens, s / 10);
            lv_imglist_set_act(ui->page_meter_imglist_speed_ones, s % 10);
        }
        lv_label_set_text_fmt(ui->page_meter_lbl_1, "%s", gear);
        if (online && (v->valid & VEHICLE_SOC_VALID)) lv_label_set_text_fmt(ui->page_meter_lbl_3, "%u%%", v->soc);
        else lv_label_set_text(ui->page_meter_lbl_3, "--%");
        visible(ui->page_meter_imglist_bat, online && (v->valid & VEHICLE_SOC_VALID));
        if (online && (v->valid & VEHICLE_SOC_VALID)) lv_imglist_set_act(ui->page_meter_imglist_bat, v->soc ? v->soc - 1 : 0);
        lv_label_set_text_fmt(ui->page_meter_lbl_5, "%lu.%01lukm",
            (unsigned long)(v->odometer_mm / 1000000), (unsigned long)(v->odometer_mm / 100000 % 10));
        lv_obj_set_width(ui->page_meter_lbl_2, 245);
        lv_obj_set_style_text_font(ui->page_meter_lbl_2, &lv_font_Lato_Regular_24, 0);
        lv_label_set_long_mode(ui->page_meter_lbl_2, LV_LABEL_LONG_MODE_CLIP);
        lv_label_set_text_fmt(ui->page_meter_lbl_2, "%s %s", speed,
            !online ? "SIF OFF" : v->faults ? fault_name(v->faults, lv_tick_get()) : "km/h");
    }
}
#endif

/**
 * @brief 绑定宠物页的车辆数据显示和信息区域事件。
 * @param[in] ui 已创建的宠物页控件，允许为空。
 * @retval 无。
 * @note 在首次动画运行前替换预览数据；保留页面布局，不新增页面或设置入口。
 */
void youyibao_sif_ui_bind_pet(lv_ui_chongwuyibiao *ui)
{
#if TCFG_YYB_SIF_ENABLE
    if (!ui || !ui->chongwuyibiao || pet_root == ui->chongwuyibiao) return;
    pet_root = ui->chongwuyibiao;
    lv_obj_set_width(ui->chongwuyibiao_jiashimoshi, 270);
    lv_obj_add_event_cb(pet_root, pet_deleted, LV_EVENT_DELETE, NULL);
    lv_obj_t *unsupported[] = {ui->chongwuyibiao_zuozhuanxiang, ui->chongwuyibiao_youzhuanxiang,
        ui->chongwuyibiao_yuanguangdengtubiao,
        ui->chongwuyibiao_shikuodengtubiao, ui->chongwuyibiao_tianqitubiao,
        ui->chongwuyibiao_xinhaozhu_1, ui->chongwuyibiao_xinhaozhu_2,
        ui->chongwuyibiao_xinhaozhu_3, ui->chongwuyibiao_xinhaozhu_4};
    lv_obj_t *distance[] = {ui->chongwuyibiao_lichengbiaoti, ui->chongwuyibiao_lichengshuzi,
        ui->chongwuyibiao_lichengdanwei};
    unsigned i;
    for (i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) visible(unsupported[i], 0);
    for (i = 0; i < sizeof(distance) / sizeof(distance[0]); ++i) if (distance[i]) {
        lv_obj_add_flag(distance[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(distance[i], 10);
        lv_obj_add_event_cb(distance[i], pet_distance_clicked, LV_EVENT_CLICKED, NULL);
    }
    /* 无校时或天气过期时显示未知值；数据就绪后由环境快照自动刷新。 */
    if (ui->chongwuyibiao_shijian) lv_label_set_text(ui->chongwuyibiao_shijian, "--:--");
    if (ui->chongwuyibiao_riqixingqi) lv_label_set_text(ui->chongwuyibiao_riqixingqi, "-- --");
    if (ui->chongwuyibiao_wendu) lv_label_set_text(ui->chongwuyibiao_wendu, "--");
    if (ui->chongwuyibiao_lichengdanwei) lv_obj_set_width(ui->chongwuyibiao_lichengdanwei, 48);
    app_vehicle_snapshot(&latest);
#ifndef YOUYIBAO_UI_TEST
    ui_environment_bind(ui);
#endif
    pet_refresh(ui);
#else
    (void)ui;
#endif
}

/**
 * @brief 初始化车辆数据到仪表页面的显示适配。
 * @retval 无。
 * @note 由 UI 初始化流程调用；一线通开启时创建一次 100ms 定时器并立即刷新。
 *       重复调用不会重复创建定时器。
 */
void youyibao_sif_ui_init(void)
{
#if TCFG_YYB_SIF_ENABLE
    if (!control_timer) control_timer = lv_timer_create(meter_control_bridge, 100, NULL);
    if (control_timer) meter_control_bridge(control_timer);
    if (!refresh_timer) refresh_timer = lv_timer_create(refresh, 100, NULL);
    if (refresh_timer) refresh(refresh_timer);
#endif
}
#else
/**
 * @brief 提供未接入硬件数据的仿真构建所需的空初始化入口。
 * @retval 无。
 */
void youyibao_sif_ui_init(void) {}
/**
 * @brief 仿真构建保留 UI 工具的预览内容。
 * @param[in] ui 宠物页控件，本构建未使用。
 * @retval 无。
 */
void youyibao_sif_ui_bind_pet(lv_ui_chongwuyibiao *ui) { (void)ui; }
int youyibao_sif_ui_meter_apply(int power_on, int light_on, int mode) { return -1; }
void youyibao_sif_ui_update_wake(void) {}
#endif
