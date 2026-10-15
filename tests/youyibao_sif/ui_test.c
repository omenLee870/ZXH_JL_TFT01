#define YOUYIBAO_UI_TEST 1
#include "../../SDK/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/youyibao_sif_ui.c"
#include <assert.h>
#include <stdlib.h>

lv_ui guider_ui;
lv_font_t lv_font_Lato_Regular_24;
static struct yyb_view mock_view;
static unsigned notice_active;
static bool ota_active;
static lv_ui_chongwuyibiao pet;
static unsigned char pixels[1024 * 600 * 2] __attribute__((aligned(32)));

void youyibao_sif_snapshot(struct yyb_view *v) { *v = mock_view; }
int pet_reverse_notice_request(bool on) { notice_active = on; return 0; }
int ui_update_page_is_active(void) { return ota_active; }
void chongwuyibiao_chesu_gengxin(uint16_t speed) { (void)speed; }
void *ui_get_scr_ptr(lv_ui *ui, int32_t id) { (void)ui; return id == GUI_SCREEN_CHONGWUYIBIAO ? &pet : NULL; }
gui_scr_t *ui_get_setup_scr(int32_t id) { return gui_scr_get(id); }
void ui_load_scr_anim(lv_ui *ui, gui_scr_t *s, lv_screen_load_anim_t anim,
                      uint32_t time, uint32_t delay, bool clean, bool del, bool push)
{
    gui_scr_set_act_anim(s, ui, anim, time, delay, clean, del);
    (void)push;
}
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *p)
{ (void)a; (void)p; lv_display_flush_ready(d); }
static lv_obj_t *label(lv_obj_t *root, int x, int y)
{
    lv_obj_t *o = lv_label_create(root);
    lv_obj_set_pos(o,x,y);
    lv_label_set_text(o,"sample");
    return o;
}
static void export_ppm(const char *name, lv_display_t *display)
{
    unsigned i;
    lv_refr_now(display);
    FILE *f = fopen(name,"wb"); assert(f);
    fprintf(f,"P6\n1024 600\n255\n");
    for (i=0;i<1024*600;++i) {
        uint16_t p = pixels[i*2] | ((uint16_t)pixels[i*2+1] << 8);
        unsigned char rgb[3] = {(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
int main(void)
{
    lv_init(); gui_scr_init();
    lv_font_t *font = lv_freetype_font_create("C:/Windows/Fonts/arial.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,24,LV_FREETYPE_FONT_STYLE_NORMAL);
    assert(font); lv_font_Lato_Regular_24 = *font;
    lv_display_t *display=lv_display_create(1024,600);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display,flush);
    pet.chongwuyibiao=lv_obj_create(NULL);
    pet.chongwuyibiao_chesushuzi=label(pet.chongwuyibiao,150,206);
    pet.chongwuyibiao_jiashimoshi=label(pet.chongwuyibiao,715,466);
    pet.chongwuyibiao_dianliangshuzi=label(pet.chongwuyibiao,160,120);
    pet.chongwuyibiao_dianliangtiao=lv_bar_create(pet.chongwuyibiao);
    pet.chongwuyibiao_guzhangxinxi=label(pet.chongwuyibiao,390,466);
    pet.chongwuyibiao_shachetubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_lichengshuzi=label(pet.chongwuyibiao,223,466);
    pet.chongwuyibiao_wendu=label(pet.chongwuyibiao,650,253);
    gui_scr_t *pet_scr=gui_scr_create(GUI_SCREEN_CHONGWUYIBIAO,"pet",pet.chongwuyibiao,NULL,NULL);
    gui_scr_t *video=gui_scr_create(GUI_SCREEN_PAGE_VIDEO,"camera",lv_obj_create(NULL),NULL,NULL);
    gui_scr_set_act_anim(pet_scr,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    mock_view.online=1; mock_view.speed_dkph=360; mock_view.soc=83; mock_view.soc_valid=1;
    mock_view.gear=3; mock_view.flags=YYB_BRAKE; mock_view.odometer_mm=12345000;
    youyibao_sif_ui_init();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"36"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"83%"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"12.3"));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_shachetubiao,LV_OBJ_FLAG_HIDDEN));
    mock_view.faults=YYB_HALL_FAULT; refresh(NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_guzhangxinxi),"HALL"));
    export_ppm("output/youyibao_dashboard.ppm",display);
    mock_view.flags=YYB_REVERSE; refresh(NULL);
    assert(gui_scr_get_act()==video && notice_active);
    mock_view.flags=0; refresh(NULL);
    assert(gui_scr_get_act()==pet_scr && !notice_active);
    /* Manually leaving automatic reverse cancels ownership, without reopening. */
    mock_view.flags=YYB_REVERSE; refresh(NULL);
    gui_scr_set_act_anim(pet_scr,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    refresh(NULL); assert(gui_scr_get_act()==pet_scr && !notice_active);
    mock_view.online=0; mock_view.speed_dkph=-1; mock_view.soc_valid=0; refresh(NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"--"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"--%"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_guzhangxinxi),"SIF OFFLINE"));
    ota_active=true; refresh(NULL); ota_active=false; refresh(NULL);
    gui_scr_set_act_anim(video,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    lv_obj_delete(pet.chongwuyibiao);
    refresh(NULL);
    puts("Youyibao UI: telemetry, reverse lifecycle, offline, OTA, deletion PASS");
    return 0;
}
