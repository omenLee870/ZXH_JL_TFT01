/* Exercise real environment state + real LVGL rendering adapter, including page recreation. */
#define METER_ENVIRONMENT_UI_TEST 1
#define main environment_state_test_main
#include "runtime_test.c"
#undef main
#include "lvgl.h"
#include "ui_environment.h"
static int requested_image;
char *gui_get_res_path(int32_t id){requested_image=id;return NULL;}
#include "../../ui_lvgl/dashboard_1024x600_v1/custom/ui_environment.c"
static void create_widgets(lv_ui_chongwuyibiao *ui){
    memset(ui,0,sizeof(*ui));ui->chongwuyibiao=lv_obj_create(NULL);
    ui->chongwuyibiao_shijian=lv_label_create(ui->chongwuyibiao);
    ui->chongwuyibiao_riqixingqi=lv_label_create(ui->chongwuyibiao);
    ui->chongwuyibiao_wendu=lv_label_create(ui->chongwuyibiao);
    ui->chongwuyibiao_weather_icon=lv_image_create(ui->chongwuyibiao);
    lv_obj_add_flag(ui->chongwuyibiao_weather_icon,LV_OBJ_FLAG_HIDDEN);
}
int main(void){
    assert(!environment_state_test_main());
    lv_init();lv_display_t *display=lv_display_create(1024,600);assert(display);
    lv_ui_chongwuyibiao ui;create_widgets(&ui);
    ui_environment_bind(&ui);
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_wendu),"25°C"));
    assert(strcmp(lv_label_get_text(ui.chongwuyibiao_shijian),"--:--"));
    assert(requested_image==ENV_WEATHER_CLOUD&&!lv_obj_has_flag(ui.chongwuyibiao_weather_icon,LV_OBJ_FLAG_HIDDEN));
    rtc=(struct sys_time){2026,10,19,21,36,0};boot();request_weather(10,-55,METER_WEATHER_CLEAR);
    ui_environment_refresh(&ui);
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_shijian),"21:36"));
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_wendu),"-6°C")&&requested_image==ENV_WEATHER_MOON);
    assert(lv_obj_get_style_opa(ui.chongwuyibiao_weather_icon,0)==LV_OPA_COVER);
    now+=WEATHER_TTL*1000u;tick(NULL);ui_environment_refresh(&ui);
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_wendu),"-6°C"));
    assert(!lv_obj_has_flag(ui.chongwuyibiao_weather_icon,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_opa(ui.chongwuyibiao_weather_icon,0)==LV_OPA_60);
    lv_obj_delete(ui.chongwuyibiao);create_widgets(&ui);requested_image=0;ui_environment_bind(&ui);
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_wendu),"-6°C")&&requested_image==ENV_WEATHER_MOON);
    lv_obj_delete(ui.chongwuyibiao);lv_display_delete(display);
    puts("Environment LVGL: default labels/icon, live data, negative temperature, night icon, expired cache and page recreation PASS");
    return 0;
}
