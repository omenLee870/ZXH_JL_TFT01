#include "lvgl.h"
#include "src/widgets/label/lv_label_private.h"
#include "src/draw/lv_image_decoder_private.h"
#include "src/display/lv_display_private.h"
#include "lv_port_key_dispatch.h"
#include "gui_guider.h"
#include "chongwuyibiao_donghua.h"
#include "chongwuyibiao_yinyue.h"
#include "chongwuyibiao_yinyue_donghua.h"
#include "pet_dashboard_music.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static lv_font_t *title_font;
static uint16_t frame_pixels[1024 * 600] __attribute__((aligned(32)));
static unsigned int cover_draw_tasks;
lv_ui guider_ui;
static unsigned int camera_loads, camera_unloads;
static unsigned int keypad_reads;

/* 测试只替代硬件事件解码，执行与设备相同的分发入口及实际页面捷径。 */
bool lvgl_key_event_consumed(struct key_event *event)
{
    return chongwuyibiao_daoche_toggle(*(const bool *)event);
}

static void test_keypad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    keypad_reads++;
    data->state = LV_INDEV_STATE_REL;
}

/* 使用实际 GUI 页面管理和 LVGL 生命周期，仅替代生成代码的入口。 */
gui_scr_t *ui_get_setup_scr(int32_t id)
{
    return gui_scr_get(id);
}

void ui_load_scr_anim(lv_ui *ui, gui_scr_t *screen, lv_screen_load_anim_t anim,
                      uint32_t time, uint32_t delay, bool clean, bool auto_del, bool push)
{
    assert(!clean && !auto_del && !push);
    gui_scr_set_act_anim(screen, ui, anim, time, delay, clean, auto_del);
}

static void camera_page_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_SCREEN_LOADED) camera_loads++;
    if (lv_event_get_code(event) == LV_EVENT_SCREEN_UNLOAD_START) camera_unloads++;
}

/** @brief 软件渲染测试同步完成刷新，保留实际帧缓冲供像素断言。 */
static void flush_ready(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    (void)area;
    (void)pixels;
    lv_display_flush_ready(display);
}

/** @brief 核对图片自身圆形裁剪，不经过父容器创建的离屏图层。 */
static void check_cover_draw(lv_event_t *event)
{
    lv_draw_task_t *task = lv_event_get_draw_task(event);
    if (lv_draw_task_get_type(task) != LV_DRAW_TASK_TYPE_IMAGE) return;
    const lv_draw_image_dsc_t *draw = lv_draw_task_get_draw_dsc(task);
    assert(draw->clip_radius == 48);
    assert(draw->base.layer && draw->base.layer->parent == NULL);
    cover_draw_tasks++;
}

/** @brief 可见性测试不读取图片文件。 */
char *gui_get_res_path(int32_t id)
{
    (void)id;
    return NULL;
}

/** @brief 测试环境省略设备内存统计。 */
void neicun_zhuizong_jieduan(const char *name)
{
    (void)name;
}

/** @brief 创建最小控件组，使用真实 LVGL 对象和点击回调。 */
static void create_view(lv_ui_chongwuyibiao *ui)
{
    ui->chongwuyibiao = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui->chongwuyibiao, lv_color_black(), 0);
    ui->chongwuyibiao_caodirongqi = lv_obj_create(ui->chongwuyibiao);
    ui->chongwuyibiao_caodi = lv_image_create(ui->chongwuyibiao_caodirongqi);
    ui->chongwuyibiao_yun1 = lv_image_create(ui->chongwuyibiao_caodirongqi);
    ui->chongwuyibiao_yun2 = lv_image_create(ui->chongwuyibiao_caodirongqi);
    ui->chongwuyibiao_yun3 = lv_image_create(ui->chongwuyibiao_caodirongqi);
    ui->chongwuyibiao_yun4 = lv_image_create(ui->chongwuyibiao_caodirongqi);
    ui->chongwuyibiao_xiaogou = lv_image_create(ui->chongwuyibiao);
    ui->chongwuyibiao_chesushuzi = lv_label_create(ui->chongwuyibiao);
    lv_label_set_text(ui->chongwuyibiao_chesushuzi, "46");
    ui->chongwuyibiao_yinyuerongqi = lv_obj_create(ui->chongwuyibiao);
    lv_obj_remove_style_all(ui->chongwuyibiao_yinyuerongqi);
    lv_obj_set_pos(ui->chongwuyibiao_yinyuerongqi, 152, 210);
    lv_obj_set_size(ui->chongwuyibiao_yinyuerongqi, 720, 236);
    lv_obj_remove_flag(ui->chongwuyibiao_yinyuerongqi, LV_OBJ_FLAG_SCROLLABLE);
    ui->chongwuyibiao_tinggexiaogou = lv_image_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_zhuanjirongqi = lv_obj_create(ui->chongwuyibiao_yinyuerongqi);
    lv_obj_remove_style_all(ui->chongwuyibiao_zhuanjirongqi);
    lv_obj_set_pos(ui->chongwuyibiao_zhuanjirongqi, 16, 130);
    lv_obj_set_size(ui->chongwuyibiao_zhuanjirongqi, 96, 96);
    lv_obj_set_style_clip_corner(ui->chongwuyibiao_zhuanjirongqi, false, 0);
    lv_obj_remove_flag(ui->chongwuyibiao_zhuanjirongqi, LV_OBJ_FLAG_SCROLLABLE);
    ui->chongwuyibiao_zhuanjifengmian = lv_image_create(ui->chongwuyibiao_zhuanjirongqi);
    lv_obj_set_size(ui->chongwuyibiao_zhuanjifengmian, 96, 96);
    lv_obj_set_style_radius(ui->chongwuyibiao_zhuanjifengmian, 48, 0);
    lv_obj_set_style_clip_corner(ui->chongwuyibiao_zhuanjifengmian, true, 0);
    lv_obj_add_flag(ui->chongwuyibiao_zhuanjifengmian, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(ui->chongwuyibiao_zhuanjifengmian, check_cover_draw, LV_EVENT_DRAW_TASK_ADDED, NULL);
    ui->chongwuyibiao_yinfuyou = lv_image_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_yinfuzuo = lv_image_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_gequmingcheng = lv_label_create(ui->chongwuyibiao_yinyuerongqi);
    lv_obj_set_size(ui->chongwuyibiao_gequmingcheng, 288, 38);
    lv_obj_set_style_pad_all(ui->chongwuyibiao_gequmingcheng, 0, 0);
    lv_obj_set_style_text_font(ui->chongwuyibiao_gequmingcheng, title_font, 0);
    lv_label_set_long_mode(ui->chongwuyibiao_gequmingcheng, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    ui->chongwuyibiao_geciwenzi = lv_label_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_bofangjian = lv_imagebutton_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_zantingjian = lv_imagebutton_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_shangyiqu = lv_imagebutton_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_xiayiqu = lv_imagebutton_create(ui->chongwuyibiao_yinyuerongqi);
    ui->chongwuyibiao_lanyalianjietubiao = lv_image_create(ui->chongwuyibiao);
    chongwuyibiao_donghua_bangding(ui);
    chongwuyibiao_yinyue_bangding(ui);
}

/** @brief 检查两组控件显示互斥，并核对蓝牙和播放按钮。 */
static void expect(lv_ui_chongwuyibiao *ui, bool connected, bool playing, bool music)
{
    assert(lv_obj_has_flag(ui->chongwuyibiao_yinyuerongqi, LV_OBJ_FLAG_HIDDEN) == !music);
    assert(lv_obj_has_flag(ui->chongwuyibiao_caodirongqi, LV_OBJ_FLAG_HIDDEN) == music);
    assert(lv_obj_has_flag(ui->chongwuyibiao_xiaogou, LV_OBJ_FLAG_HIDDEN) == music);
    assert(lv_obj_has_flag(ui->chongwuyibiao_lanyalianjietubiao, LV_OBJ_FLAG_HIDDEN) == !connected);
    assert(lv_obj_has_flag(ui->chongwuyibiao_zantingjian, LV_OBJ_FLAG_HIDDEN) == !playing);
    assert(lv_obj_has_flag(ui->chongwuyibiao_bofangjian, LV_OBJ_FLAG_HIDDEN) == playing);
}

/** @brief 推进真实 LVGL 定时器，用于检查动画而非复制实现。 */
static void advance(uint32_t ms)
{
    while (ms) {
        uint32_t step = ms > 30 ? 30 : ms;
        lv_tick_inc(step);
        lv_timer_handler();
        ms -= step;
    }
}

static void set_heading(const char *heading)
{
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    pet_dashboard_music_receive_text(1, (const uint8_t *)heading, strlen(heading));
    pet_dashboard_music_receive_text(2, (const uint8_t *)"Artist", 6);
#else
    pet_dashboard_music_receive_text(2, (const uint8_t *)heading, strlen(heading));
#endif
}

/** @brief 歌词立即显示，歌曲信息的分隔符不决定歌词是否有效。 */
static void verify_metadata(lv_ui_chongwuyibiao *ui)
{
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    set_heading("J字情书");
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), "J字情书"));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), ""));
    advance(200); /* 普通信息先到，歌词稍晚到时不能闪无歌词提示。 */
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), ""));
    const char *heading = "J字情书 - 孙睡觉/四七";
    const char *lyric = "你要去找太阳";
    pet_dashboard_music_receive_text(1, (const uint8_t *)lyric, strlen(lyric));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), "J字情书"));
    pet_dashboard_music_receive_text(2, (const uint8_t *)heading, strlen(heading));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), heading));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    advance(900); /* 原等待期限到达不能再覆盖真实歌词。 */
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    set_heading("J字情书");
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), "J字情书"));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    puts("PASS: ordinary metadata displays song title and preserves real lyrics on actual LVGL labels");
#else
    const char *heading = "歌曲信息不带分隔符";
    const char *lyric = "第一句实际歌词";
    pet_dashboard_music_receive_text(1, (const uint8_t *)lyric, strlen(lyric));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    pet_dashboard_music_receive_text(2, (const uint8_t *)heading, strlen(heading));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), heading));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    lyric = "下一句实际歌词";
    pet_dashboard_music_receive_text(1, (const uint8_t *)lyric, strlen(lyric));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), heading));
    set_heading(heading);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), lyric));
    puts("PASS: lyrics immediately reach their LVGL label without requiring a heading separator");
#endif
}

/** @brief 无歌词超时、重复刷新以及连续切歌使用实际 LVGL 定时器验证。 */
static void verify_lyric_wait(lv_ui_chongwuyibiao *ui)
{
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    set_heading("纯音乐 A");
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), ""));
    advance(700);
    pet_dashboard_music_request(PET_MUSIC_REFRESH);
    set_heading("纯音乐 A");
    pet_dashboard_music_receive_text(8, (const uint8_t *)"0000054", 7);
    advance(200);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), ""));
    advance(130);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "暂无歌词"));
    pet_dashboard_music_request(PET_MUSIC_REFRESH);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "暂无歌词"));
    /* 歌词超过等待期才到，仍立即替换提示。 */
    pet_dashboard_music_receive_text(0x80, (const uint8_t *)"Late lyric", 10);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "Late lyric"));
    set_heading("纯音乐 B");
    advance(700);
    set_heading("纯音乐 C");
    advance(400);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), ""));
    advance(630);
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "暂无歌词"));
    puts("PASS: delayed lyrics show immediately without placeholder flash; no-lyrics timeout survives refresh/cover updates and restarts on new song");
#else
    (void)ui;
#endif
}

/** @brief 用真实字体验证短歌名静止、长歌名左滑循环及重复通知不中断。 */
static void verify_title_scroll(lv_ui_chongwuyibiao *ui)
{
    lv_label_t *label = (lv_label_t *)ui->chongwuyibiao_gequmingcheng;
    const char *short_title = "晴天";
    const char *long_title = "把心事交给晚风让音乐陪伴我们一路向前";
    set_heading(short_title);
    lv_obj_update_layout(ui->chongwuyibiao);
    advance(600);
    assert(label->offset.x == 0 && label->offset.y == 0);
    set_heading(long_title);
    lv_obj_update_layout(ui->chongwuyibiao);
    advance(1000);
    assert(label->offset.x < 0 && label->offset.y == 0);
    int32_t previous = label->offset.x;
    pet_dashboard_music_request(PET_MUSIC_REFRESH);
    advance(300);
    assert(label->offset.x < previous && label->offset.y == 0);
    bool repeated = false;
    unsigned int i;
    for (i = 0; i < 200; i++) {
        previous = label->offset.x;
        advance(60);
        assert(label->offset.y == 0);
        if (label->offset.x > previous + 4) repeated = true;
    }
    assert(repeated);
    set_heading(short_title);
    lv_obj_update_layout(ui->chongwuyibiao);
    advance(600);
    assert(label->offset.x == 0 && label->offset.y == 0);
    puts("PASS: short title stationary, long title scrolls left and repeats, refresh preserves scrolling");
}

/** @brief 验证文本接入、封面复制以及动画暂停和退出。 */
static void verify_media(lv_ui_chongwuyibiao *ui)
{
    /* 前面的歌词等待测试已推进时钟，动画断言从固定音符相位开始。 */
    chongwuyibiao_yinyue_donghua_yunxu(false);
    chongwuyibiao_yinyue_donghua_yunxu(true);
    /* 歌曲信息保留完整组合原文。 */
    const char *song = "Test song - Artist";
    const char *next_song = "Another song - Artist";
    pet_dashboard_music_receive_text(1, (const uint8_t *)"Real lyric", 10);
    pet_dashboard_music_receive_text(2, (const uint8_t *)song, strlen(song));
    pet_dashboard_music_receive_text(8, (const uint8_t *)"0000052", 7);
    assert(strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), song) == 0);
    assert(strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "Real lyric") == 0);
    uint16_t pixels[96 * 96] = {0};
    unsigned int pixel;
    for (pixel = 0; pixel < 96 * 96; pixel++) pixels[pixel] = 0xf800;
    lv_image_dsc_t cover = {.header = {.magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565, .w = 96, .h = 96, .stride = 192},
        .data_size = sizeof(pixels), .data = (const uint8_t *)pixels};
    chongwuyibiao_yinyue_fengmian(&cover);
    const lv_image_dsc_t *shown = lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian);
    assert(shown != &cover && shown->data != cover.data);
    assert((uintptr_t)shown->data % LV_DRAW_BUF_ALIGN == 0);
    assert((uintptr_t)shown->data % 32 == 0);
    lv_image_decoder_dsc_t decoded;
    assert(lv_image_decoder_open(&decoded, shown, NULL) == LV_RESULT_OK);
    assert(decoded.decoded && decoded.decoded->header.w == 96 &&
           decoded.decoded->header.h == 96 && decoded.decoded->header.stride == 192);
    assert(decoded.decoded->data == shown->data);
    lv_image_decoder_close(&decoded);
    cover_draw_tasks = 0;
    lv_refr_now(NULL);
    lv_area_t area;
    lv_obj_get_coords(ui->chongwuyibiao_zhuanjifengmian, &area);
    unsigned int center = ((area.y1 + area.y2) / 2) * 1024 + (area.x1 + area.x2) / 2;
    unsigned int corner = area.y1 * 1024 + area.x1;
    assert(cover_draw_tasks > 0);
    assert(frame_pixels[center] == 0xf800 && frame_pixels[corner] == 0);
    pixels[0] = 0;
    assert(((const uint16_t *)shown->data)[0] == 0xf800);
    for (pixel = 0; pixel < 96 * 96; pixel++) pixels[pixel] = 0x07e0;
    chongwuyibiao_yinyue_fengmian(&cover);
    assert(lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian) == shown);
    assert(((const uint16_t *)shown->data)[0] == 0x07e0);
    lv_refr_now(NULL);
    assert(frame_pixels[center] == 0x07e0 && frame_pixels[corner] == 0);
    puts("PASS: album pixels reach framebuffer, round corners clipped, image draws directly without child layers");
    puts("PASS: album pixels aligned, decoded from owned buffer, replacement updates pixels");
    advance(300);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) > 0);
    uint32_t revision = pet_dashboard_music_get_view()->track_revision;
    int32_t lyric_angle = lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian);
    pet_dashboard_music_receive_text(1, (const uint8_t *)"Next lyric line", 15);
    pet_dashboard_music_receive_text(2, (const uint8_t *)song, strlen(song));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_gequmingcheng), song));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "Next lyric line"));
    assert(pet_dashboard_music_get_view()->track_revision == revision);
    assert(lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian) == shown);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) == lyric_angle);
    puts("PASS: TITLE updates lyric label while ARTIST heading, album pixels and rotation remain stable");
    assert(!lv_obj_has_flag(ui->chongwuyibiao_yinfuyou, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(ui->chongwuyibiao_yinfuzuo, LV_OBJ_FLAG_HIDDEN));
    advance(450);
    assert(lv_obj_get_style_image_opa(ui->chongwuyibiao_yinfuyou, 0) < 255);
    advance(500);
    assert(lv_obj_has_flag(ui->chongwuyibiao_yinfuyou, LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(ui->chongwuyibiao_yinfuzuo, LV_OBJ_FLAG_HIDDEN));
    pet_dashboard_music_update(true, false, 1500);
    int32_t angle = lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian);
    advance(400);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) == angle);
    assert(lv_obj_has_flag(ui->chongwuyibiao_yinfuyou, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(ui->chongwuyibiao_yinfuzuo, LV_OBJ_FLAG_HIDDEN));
    pet_dashboard_music_update(true, true, 1600);
    advance(100);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) > angle);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    angle = lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian);
    advance(150);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) == angle);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    pet_dashboard_music_receive_text(1, (const uint8_t *)"New first lyric", 15);
    pet_dashboard_music_receive_text(2, (const uint8_t *)next_song, strlen(next_song));
    assert(lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian) == shown);
    pet_dashboard_music_receive_text(8, (const uint8_t *)"0000053", 7);
    assert(!lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian));
    chongwuyibiao_yinyue_fengmian_identified(&cover, "0000052");
    assert(!lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian));
    chongwuyibiao_yinyue_fengmian_identified(&cover, "0000053");
    assert(lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian) == shown);
    assert(strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "New first lyric") == 0);
    puts("PASS: heading keeps cover; new handle clears it; stale decoded cover rejected");
}

/** @brief 实际页面往返保留音乐、封面、手动主界面及速度，倒车页有完整生命周期。 */
static void verify_ok_camera(lv_ui_chongwuyibiao *ui)
{
    gui_scr_t *pet = gui_scr_create(GUI_SCREEN_CHONGWUYIBIAO, "pet", ui->chongwuyibiao, NULL, NULL);
    lv_obj_t *video_obj = lv_obj_create(NULL);
    gui_scr_t *video = gui_scr_create(GUI_SCREEN_PAGE_VIDEO, "video", video_obj, NULL, NULL);
    lv_obj_add_event_cb(video_obj, camera_page_event, LV_EVENT_ALL, NULL);
    const void *cover = lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian);
    pet_dashboard_music_view_t saved = *pet_dashboard_music_get_view();
    assert(chongwuyibiao_daoche_toggle(false)); /* DOWN/UP/HOLD 不切屏，也不触发确认。 */
    assert(gui_scr_get_act() == pet);
    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, test_keypad_read);
    lv_indev_enable(keypad, false);
    lv_display_t *display = lv_display_get_default();
    display->prev_scr = video_obj; /* 模拟动画遗留标志：原来的 read_cb 会直接被跳过。 */
    lv_indev_read(keypad);
    assert(keypad_reads == 0 && gui_scr_get_act() == pet);
    bool click = true;
    lv_port_key_dispatch(keypad, (struct key_event *)&click);
    advance(30);
    assert(gui_scr_get_act() == video && camera_loads == 1);
    assert(keypad_reads == 0); /* OK 绕过 LVGL blocked/disabled，且不误发 ENTER。 */
    lv_indev_delete(keypad);
    assert(lv_obj_is_valid(ui->chongwuyibiao));
    int32_t angle = lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian);
    advance(300);
    assert(lv_image_get_rotation(ui->chongwuyibiao_zhuanjifengmian) == angle);
    assert(chongwuyibiao_daoche_toggle(false));
    assert(gui_scr_get_act() == video);
    assert(chongwuyibiao_daoche_toggle(true));
    advance(30);
    assert(gui_scr_get_act() == pet && camera_unloads == 1);
    assert(pet->scr == ui->chongwuyibiao);
    assert(lv_image_get_src(ui->chongwuyibiao_zhuanjifengmian) == cover);
    assert(!memcmp(&saved, pet_dashboard_music_get_view(), sizeof(saved)));
    expect(ui, true, true, true);

    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    assert(chongwuyibiao_daoche_toggle(true));
    advance(30);
    pet_dashboard_music_receive_text(0x80, (const uint8_t *)"Camera lyric", 12);
    assert(chongwuyibiao_daoche_toggle(true));
    advance(30);
    expect(ui, true, true, false); /* 手动留在草地的状态必须保留。 */
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_chesushuzi), "46"));
    assert(!strcmp(lv_label_get_text(ui->chongwuyibiao_geciwenzi), "Camera lyric"));
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);

    /* 从其他入口进倒车页不接管 OK，临时往返标记必须随退出清掉。 */
    gui_scr_set_act_anim(video, &guider_ui, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
    advance(30);
    assert(!chongwuyibiao_daoche_toggle(false));
    assert(!chongwuyibiao_daoche_toggle(true));
    assert(gui_scr_get_act() == video);
    gui_scr_set_act_anim(pet, &guider_ui, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
    advance(30);
    assert(camera_loads == 3 && camera_unloads == 3);
    gui_scr_del(video);
    puts("PASS: OK reaches pet/camera even with blocked/disabled LVGL input; original page, cover, manual view and live lyrics survive; camera load/unload paired");
}

/** @brief 验证实际控件显隐、点击以及页面删除和重建。 */
int main(void)
{
    lv_init();
    gui_scr_init();
    title_font = lv_freetype_font_create("ui_lvgl/dashboard_1024x600_v1/import/font/lyrics.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP, 26, LV_FREETYPE_FONT_STYLE_NORMAL);
    assert(title_font && title_font->line_height == 36);
    lv_display_t *display = lv_display_create(1024, 600);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flush_ready);
    lv_display_set_buffers(display, frame_pixels, NULL, sizeof(frame_pixels), LV_DISPLAY_RENDER_MODE_DIRECT);
    pet_dashboard_music_reset();
    pet_dashboard_music_start();
    lv_ui_chongwuyibiao ui = {0};
    create_view(&ui);
    lv_screen_load(ui.chongwuyibiao);
    expect(&ui, false, false, false);
    pet_dashboard_music_update(true, true, 100);
    expect(&ui, true, true, true);
    verify_metadata(&ui);
    verify_media(&ui);
    verify_ok_camera(&ui);
    verify_title_scroll(&ui);
    verify_lyric_wait(&ui);
    lv_obj_send_event(ui.chongwuyibiao_tinggexiaogou, LV_EVENT_CLICKED, NULL);
    expect(&ui, true, true, false);
    assert(lv_strcmp(lv_label_get_text(ui.chongwuyibiao_chesushuzi), "46") == 0);
    pet_dashboard_music_poll(2000);
    expect(&ui, true, true, false);
    lv_obj_send_event(ui.chongwuyibiao_xiaogou, LV_EVENT_CLICKED, NULL);
    expect(&ui, true, true, true);
    pet_dashboard_music_update(true, false, 3000);
    expect(&ui, true, false, true);
    pet_dashboard_music_poll(5999);
    expect(&ui, true, false, true);
    pet_dashboard_music_poll(6000);
    expect(&ui, true, false, false);
    pet_dashboard_music_update(true, true, 7000);
    lv_obj_send_event(ui.chongwuyibiao_tinggexiaogou, LV_EVENT_CLICKED, NULL);
    expect(&ui, true, true, false);

    lv_obj_t *other = lv_obj_create(NULL);
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    set_heading("等待期间删除页面");
    advance(300);
    assert(!strcmp(lv_label_get_text(ui.chongwuyibiao_geciwenzi), ""));
#endif
    lv_screen_load(other);
    lv_obj_delete(ui.chongwuyibiao);
    advance(1600); /* 已删除页面的等待回调不能继续执行。 */
    pet_dashboard_music_poll(8000); /* 删除页面后仍可推进应用状态。 */
    lv_ui_chongwuyibiao rebuilt = {0};
    create_view(&rebuilt);
    lv_screen_load(rebuilt.chongwuyibiao);
    expect(&rebuilt, true, true, false); /* 手动返回状态不随重建丢失。 */
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    assert(!strcmp(lv_label_get_text(rebuilt.chongwuyibiao_geciwenzi), ""));
    advance(300);
#endif
    pet_dashboard_music_update(false, false, 9000);
    expect(&rebuilt, false, false, false);
    advance(1600); /* 断开时取消等待。 */
    lv_screen_load(other);
    lv_obj_delete(rebuilt.chongwuyibiao);
    pet_dashboard_music_update(true, true, 10000);
    lv_ui_chongwuyibiao playing = {0};
    create_view(&playing);
    lv_screen_load(playing.chongwuyibiao);
    expect(&playing, true, true, true);
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    assert(!strcmp(lv_label_get_text(playing.chongwuyibiao_geciwenzi), ""));
    advance(1030);
    assert(!strcmp(lv_label_get_text(playing.chongwuyibiao_geciwenzi), "暂无歌词"));
    set_heading("再次删除等待中的页面");
#endif
    lv_screen_load(other);
    lv_obj_delete(playing.chongwuyibiao);
    advance(1600);
    lv_freetype_font_delete(title_font);
    puts("PASS: LVGL visibility, metadata, album copy/reset, rotation/pause, alternating note/fade, deletion/recreation");
    return 0;
}
