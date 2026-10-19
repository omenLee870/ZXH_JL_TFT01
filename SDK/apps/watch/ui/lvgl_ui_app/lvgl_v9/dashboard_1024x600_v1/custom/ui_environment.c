/**
 * @file ui_environment.c
 * @brief 显示时间日期及天气；实时数据优先，上次状态或默认值保持画面完整。
 */
#if !LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)
#include "app_config.h"
#include "system/includes.h"
#endif
#include "lvgl.h"
#include "ui_environment.h"
#if (!LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)) || defined(METER_ENVIRONMENT_UI_TEST)
#include "meter_environment.h"
#include "../generated/gui_res/res_common.h"
#include <string.h>
static unsigned last_icon = 255;
static void label(lv_obj_t *obj, const char *text) {
    if (obj) {
        if (strcmp(lv_label_get_text(obj), text)) lv_label_set_text(obj, text);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}
#endif
void ui_environment_refresh(lv_ui_chongwuyibiao *ui) {
#if (!LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)) || defined(METER_ENVIRONMENT_UI_TEST)
    if (!ui) return;
    struct meter_environment p; meter_environment_snapshot(&p);
    label(ui->chongwuyibiao_shijian, p.clock); label(ui->chongwuyibiao_riqixingqi, p.date);
    char temperature[20];
    {
        int dc = (p.weather_valid || p.weather_cached) ? p.temperature_dc : 250;
        int degrees = dc < 0 ? -((-dc + 5) / 10) : (dc + 5) / 10;
        snprintf(temperature, sizeof(temperature), "%d°C", degrees);
    }
    label(ui->chongwuyibiao_wendu, temperature);
    if (ui->chongwuyibiao_wendu) lv_obj_set_style_text_color(ui->chongwuyibiao_wendu,
        lv_color_hex(p.weather_valid ? 0xd8ecf4 : 0xaab8bf), 0);
    if (ui->chongwuyibiao_shijian) lv_obj_set_style_text_color(ui->chongwuyibiao_shijian,
        lv_color_hex(p.clock_valid ? 0xffffff : 0xaab8bf), 0);
    unsigned icon = (p.weather_valid || p.weather_cached) ? p.weather : METER_WEATHER_CLOUD;
    if (icon < METER_WEATHER_CLEAR || icon > METER_WEATHER_DUST) icon = METER_WEATHER_CLOUD;
    if (icon == METER_WEATHER_CLEAR && (p.hour < 6 || p.hour >= 18)) icon = 9;
    lv_obj_t *obj = ui->chongwuyibiao_weather_icon;
    if (!obj) return;
    {
        static const int images[] = {0, ENV_WEATHER_CLEAR, ENV_WEATHER_CLOUD, ENV_WEATHER_OVERCAST,
            ENV_WEATHER_RAIN, ENV_WEATHER_SNOW, ENV_WEATHER_STORM, ENV_WEATHER_FOG,
            ENV_WEATHER_DUST, ENV_WEATHER_MOON};
        if (last_icon != icon) lv_image_set_src(obj, gui_get_res_path(images[icon]));
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(obj, p.weather_valid ? LV_OPA_COVER : LV_OPA_60, 0);
    }
    last_icon = icon;
#else
    (void)ui;
#endif
}
void ui_environment_bind(lv_ui_chongwuyibiao *ui) {
#if (!LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST)) || defined(METER_ENVIRONMENT_UI_TEST)
    last_icon = 255;
#endif
    ui_environment_refresh(ui);
}
