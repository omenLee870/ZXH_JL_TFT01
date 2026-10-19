/**
 * @file ui_environment.c
 * @brief 将已校准时间、当地日期及有效天气显示到 JLUI 控件；不在 UI 线程联网或读写 RTC。
 */
#if !LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)
#include "app_config.h"
#include "system/includes.h"
#endif
#include "lvgl.h"
#include "ui_environment.h"
#if !LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)
#include "meter_environment.h"
#include "../generated/gui_res/res_common.h"
#include <string.h>
static unsigned last_icon = 255;
static void label(lv_obj_t *obj, const char *text) {
    if (obj && strcmp(lv_label_get_text(obj), text)) lv_label_set_text(obj, text);
}
#endif
void ui_environment_refresh(lv_ui_chongwuyibiao *ui) {
#if !LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)
    if (!ui) return;
    struct meter_environment p; meter_environment_snapshot(&p);
    label(ui->chongwuyibiao_shijian, p.clock); label(ui->chongwuyibiao_riqixingqi, p.date);
    char temperature[20];
    if (p.weather_valid) {
        int degrees = p.temperature_dc < 0 ? -((-p.temperature_dc + 5) / 10) : (p.temperature_dc + 5) / 10;
        snprintf(temperature, sizeof(temperature), "%d°C", degrees);
    }
    else strcpy(temperature, "--");
    label(ui->chongwuyibiao_wendu, temperature);
    unsigned icon = p.weather_valid ? p.weather : 0;
    if (icon == METER_WEATHER_CLEAR && (p.hour < 6 || p.hour >= 18)) icon = 9;
    lv_obj_t *obj = ui->chongwuyibiao_weather_icon;
    if (!obj) return;
    if (!icon) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else {
        static const int images[] = {0, ENV_WEATHER_CLEAR, ENV_WEATHER_CLOUD, ENV_WEATHER_OVERCAST,
            ENV_WEATHER_RAIN, ENV_WEATHER_SNOW, ENV_WEATHER_STORM, ENV_WEATHER_FOG,
            ENV_WEATHER_DUST, ENV_WEATHER_MOON};
        if (last_icon != icon) lv_image_set_src(obj, gui_get_res_path(images[icon]));
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    last_icon = icon;
#else
    (void)ui;
#endif
}
void ui_environment_bind(lv_ui_chongwuyibiao *ui) {
#if !LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)
    last_icon = 255;
#endif
    ui_environment_refresh(ui);
}
