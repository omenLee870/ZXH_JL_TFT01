#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#endif
#include "lvgl.h"
#if !LV_USE_GUIBUILDER_SIMULATOR || defined(YOUYIBAO_UI_TEST)
#include "youyibao_sif.h"
#include "youyibao_sif_ui.h"
#include "../generated/gui_guider.h"
#include "../generated/gui_fonts/gui_fonts.h"
#include "chongwuyibiao_donghua.h"
#include "pet_reverse_notice.h"
#include "ui_update_page.h"
#include <stdio.h>
#include <string.h>

#if TCFG_YYB_SIF_ENABLE
static lv_timer_t *refresh_timer;
static lv_obj_t *auto_return_root;
static int32_t auto_return_id;
static unsigned previous_reverse;
static struct yyb_view latest;

static void visible(lv_obj_t *o, unsigned show)
{
    if (!o) return;
    if (show) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static const char *fault_name(uint32_t f, uint32_t tick)
{
    static const struct { uint32_t flag; const char *name; } names[] = {
        {YYB_HALL_FAULT, "HALL"}, {YYB_THROTTLE_FAULT, "THROTTLE"},
        {YYB_CONTROLLER_FAULT, "CONTROLLER"}, {YYB_UNDERVOLTAGE, "LOW BATTERY"},
        {YYB_PHASE_FAULT, "MOTOR PHASE"}, {YYB_PROTECTION, "PROTECTION"},
        {YYB_ANTI_RUNAWAY, "RUNAWAY"}, {YYB_OVERCURRENT, "OVERCURRENT"},
        {YYB_STALL, "MOTOR STALL"}
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

static void auto_video_unload(lv_event_t *e)
{
    (void)e;
    if (auto_return_root) pet_reverse_notice_request(false);
    auto_return_root = NULL;
}

static void reverse_update(gui_scr_t *screen)
{
    unsigned reverse = latest.online && !!(latest.flags & YYB_REVERSE);
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

static void refresh(lv_timer_t *timer)
{
    gui_scr_t *screen = gui_scr_get_act();
    struct yyb_view *v = &latest;
    char speed[16], gear[16];
    int valid_speed;
    (void)timer;
    youyibao_sif_snapshot(v);
    /* Flash-resource OTA recovery screen has no generated dashboard widgets. */
    if (ui_update_page_is_active()) return;
    reverse_update(screen);
    screen = gui_scr_get_act();
    if (!screen || (screen->id != GUI_SCREEN_CHONGWUYIBIAO && screen->id != GUI_SCREEN_PAGE_METER)) {
        return;
    }
    valid_speed = v->online && v->speed_dkph >= 0;
    if (valid_speed) snprintf(speed, sizeof(speed), "%ld", (long)((v->speed_dkph + 5) / 10));
    else strcpy(speed, "--");
    if (!v->online) strcpy(gear, "--");
    else if (v->flags & YYB_PARK) strcpy(gear, "P");
    else if (v->flags & YYB_REVERSE) strcpy(gear, "R");
    else snprintf(gear, sizeof(gear), "GEAR %u", v->gear);
    if (screen->id == GUI_SCREEN_CHONGWUYIBIAO) {
        lv_ui_chongwuyibiao *ui = ui_get_scr_ptr(&guider_ui, screen->id);
        if (!ui) return;
        chongwuyibiao_chesu_gengxin(valid_speed ? (uint16_t)((v->speed_dkph + 5) / 10) : 0);
        lv_label_set_text(ui->chongwuyibiao_chesushuzi, speed);
        lv_label_set_text(ui->chongwuyibiao_jiashimoshi, gear);
        if (v->online && v->soc_valid) lv_label_set_text_fmt(ui->chongwuyibiao_dianliangshuzi, "%u%%", v->soc);
        else lv_label_set_text(ui->chongwuyibiao_dianliangshuzi, "--%");
        lv_bar_set_value(ui->chongwuyibiao_dianliangtiao, v->online && v->soc_valid ? v->soc : 0, LV_ANIM_OFF);
        visible(ui->chongwuyibiao_guzhangxinxi, !v->online || v->faults);
        lv_label_set_text(ui->chongwuyibiao_guzhangxinxi,
            v->online ? fault_name(v->faults, lv_tick_get()) : "SIF OFFLINE");
        visible(ui->chongwuyibiao_shachetubiao, v->online && (v->flags & (YYB_BRAKE | YYB_EBRAKE)));
        lv_label_set_text_fmt(ui->chongwuyibiao_lichengshuzi, "%lu.%01lu",
            (unsigned long)(v->odometer_mm / 1000000), (unsigned long)(v->odometer_mm / 100000 % 10));
        lv_label_set_text(ui->chongwuyibiao_wendu, "--");
    } else {
        lv_ui_page_meter *ui = ui_get_scr_ptr(&guider_ui, screen->id);
        if (!ui) return;
        if (ui->page_meter_timer_1) lv_timer_pause(ui->page_meter_timer_1);
        if (ui->page_meter_timer_2) lv_timer_pause(ui->page_meter_timer_2);
        if (ui->page_meter_timer_3) lv_timer_pause(ui->page_meter_timer_3);
        /* Remove sample warning/lamp graphics without corresponding SIF bits. */
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
        visible(ui->page_meter_img_ready, 0); /* No READY bit in the supplied protocol. */
        visible(ui->page_meter_imglist_speed_tens, valid_speed && v->speed_dkph < 995);
        visible(ui->page_meter_imglist_speed_ones, valid_speed && v->speed_dkph < 995);
        if (valid_speed && v->speed_dkph < 995) {
            unsigned s = (v->speed_dkph + 5) / 10;
            lv_imglist_set_act(ui->page_meter_imglist_speed_tens, s / 10);
            lv_imglist_set_act(ui->page_meter_imglist_speed_ones, s % 10);
        }
        lv_label_set_text_fmt(ui->page_meter_lbl_1, "%s", gear);
        if (v->online && v->soc_valid) lv_label_set_text_fmt(ui->page_meter_lbl_3, "%u%%", v->soc);
        else lv_label_set_text(ui->page_meter_lbl_3, "--%");
        visible(ui->page_meter_imglist_bat, v->online && v->soc_valid);
        if (v->online && v->soc_valid) lv_imglist_set_act(ui->page_meter_imglist_bat, v->soc ? v->soc - 1 : 0);
        lv_label_set_text_fmt(ui->page_meter_lbl_5, "%lu.%01lukm",
            (unsigned long)(v->odometer_mm / 1000000), (unsigned long)(v->odometer_mm / 100000 % 10));
        lv_obj_set_width(ui->page_meter_lbl_2, 245);
        lv_obj_set_style_text_font(ui->page_meter_lbl_2, &lv_font_Lato_Regular_24, 0);
        lv_label_set_long_mode(ui->page_meter_lbl_2, LV_LABEL_LONG_MODE_CLIP);
        lv_label_set_text_fmt(ui->page_meter_lbl_2, "%s %s", speed,
            !v->online ? "SIF OFF" : v->faults ? fault_name(v->faults, lv_tick_get()) : "km/h");
    }
}
#endif

void youyibao_sif_ui_init(void)
{
#if TCFG_YYB_SIF_ENABLE
    if (!refresh_timer) refresh_timer = lv_timer_create(refresh, 100, NULL);
    if (refresh_timer) refresh(refresh_timer);
#endif
}
#else
void youyibao_sif_ui_init(void) {}
#endif
