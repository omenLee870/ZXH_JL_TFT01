#include "nav_hud.h"
#include <stdio.h>

/*
 * 导航 HUD 悬浮层
 *
 * 右上角: 车速 (默认 25 km/h, 由外部实时写入)
 * 左上角: 转向指示 (左转/直行/右转图标 + 距离 + 道路名)
 * 左下角: 剩余距离 + 预计时间
 */

/* HUD 统一风格: 深灰半透明 + 圆角 */
#define HUD_BG_COLOR    0x333333
#define HUD_BG_OPA      LV_OPA_70
#define HUD_RADIUS      6
#define HUD_FG_COLOR    0xffffff
#define HUD_DIM_COLOR   0xbfbfbf

/* 默认车速 (km/h) */
#define HUD_DEFAULT_SPEED_KMH 25

/* 道路名中文字体 (FreeType 从 SD 卡加载) */
#define HUD_ROAD_FONT_PATH  "storage/sd1/C/font.ttf"
#define HUD_ROAD_FONT_SIZE  14

static lv_obj_t *s_speed_box;
static lv_obj_t *s_speed_num;
static lv_obj_t *s_turn_box;
static lv_obj_t *s_turn_icon;   /* 转向图标 (字体符号) */
static lv_obj_t *s_turn_dist;
static lv_obj_t *s_turn_road;
static lv_obj_t *s_remain_box;
static lv_obj_t *s_remain_label;
static lv_obj_t *s_route_box;   /* 顶部居中: 起终点名称 */
static lv_obj_t *s_route_label;

static lv_font_t *s_road_font = NULL;   /* 中文道路名字体 (FreeType) */

/* 应用 HUD 统一底色样式 */
static void hud_style_box(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(HUD_BG_COLOR), 0);
    lv_obj_set_style_bg_opa(obj, HUD_BG_OPA, 0);
    lv_obj_set_style_radius(obj, HUD_RADIUS, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

/* 转向类型 → 图标符号
 * 注: LV_SYMBOL 没有斜向箭头, 缓转暂用同向箭头,
 *     掉头用 LOOP(环形)符号, 后续可换成 font.ttf 里的专用箭头 */
static const char *hud_turn_symbol(nav_turn_type_t dir)
{
    switch (dir) {
    case NAV_TURN_LEFT:
        return LV_SYMBOL_LEFT;
    case NAV_TURN_SLIGHT_LEFT:
        return LV_SYMBOL_LEFT;
    case NAV_TURN_RIGHT:
        return LV_SYMBOL_RIGHT;
    case NAV_TURN_SLIGHT_RIGHT:
        return LV_SYMBOL_RIGHT;
    case NAV_TURN_UTURN:
        return LV_SYMBOL_LOOP;
    case NAV_TURN_ARRIVE:
        return LV_SYMBOL_OK;
    case NAV_TURN_STRAIGHT:
        return LV_SYMBOL_UP;
    default:
        return "";
    }
}

/* 加载中文道路名字体 (懒加载 + 失败重试) */
static void hud_load_road_font(void)
{
    if (s_road_font != NULL) {
        return;
    }

    s_road_font = lv_freetype_font_create(HUD_ROAD_FONT_PATH,
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          HUD_ROAD_FONT_SIZE,
                                          LV_FREETYPE_FONT_STYLE_NORMAL);
    if (s_road_font != NULL) {
        if (s_turn_road) {
            lv_obj_set_style_text_font(s_turn_road, s_road_font, 0);
        }
        if (s_route_label) {
            lv_obj_set_style_text_font(s_route_label, s_road_font, 0);
        }
        printf("nav_hud: road font loaded: %s\n", HUD_ROAD_FONT_PATH);
    } else {
        printf("nav_hud: road font load FAIL: %s\n", HUD_ROAD_FONT_PATH);
    }
}

void nav_hud_init(lv_obj_t *parent)
{
    /* 右上角: 车速框 */
    s_speed_box = lv_obj_create(parent);
    hud_style_box(s_speed_box);
    lv_obj_set_size(s_speed_box, 120, 64);
    lv_obj_align(s_speed_box, LV_ALIGN_TOP_RIGHT, -10, 10);

    s_speed_num = lv_label_create(s_speed_box);
    lv_label_set_text_fmt(s_speed_num, "%d", HUD_DEFAULT_SPEED_KMH);
    lv_obj_set_style_text_font(s_speed_num, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_speed_num, lv_color_hex(HUD_FG_COLOR), 0);
    lv_obj_align(s_speed_num, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *unit = lv_label_create(s_speed_box);
    lv_label_set_text(unit, "km/h");
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(unit, lv_color_hex(HUD_DIM_COLOR), 0);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* 左上角: 转向指示框 */
    s_turn_box = lv_obj_create(parent);
    hud_style_box(s_turn_box);
    lv_obj_set_size(s_turn_box, 240, 92);
    lv_obj_align(s_turn_box, LV_ALIGN_TOP_LEFT, 10, 10);

    s_turn_icon = lv_label_create(s_turn_box);
    lv_label_set_text(s_turn_icon, "");
    lv_obj_set_style_text_font(s_turn_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_turn_icon, lv_color_hex(HUD_FG_COLOR), 0);
    lv_obj_align(s_turn_icon, LV_ALIGN_LEFT_MID, 16, 0);

    s_turn_dist = lv_label_create(s_turn_box);
    lv_label_set_text(s_turn_dist, "");
    lv_obj_set_style_text_font(s_turn_dist, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_turn_dist, lv_color_hex(HUD_FG_COLOR), 0);
    lv_obj_align(s_turn_dist, LV_ALIGN_LEFT_MID, 60, -16);

    s_turn_road = lv_label_create(s_turn_box);
    lv_label_set_text(s_turn_road, "");
    lv_obj_set_style_text_font(s_turn_road, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_turn_road, lv_color_hex(HUD_DIM_COLOR), 0);
    lv_label_set_long_mode(s_turn_road, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_turn_road, 160);
    lv_obj_align(s_turn_road, LV_ALIGN_LEFT_MID, 60, 16);

    /* 顶部居中: 起终点名称框 */
    s_route_box = lv_obj_create(parent);
    hud_style_box(s_route_box);
    lv_obj_set_size(s_route_box, 400, 40);
    lv_obj_align(s_route_box, LV_ALIGN_TOP_MID, 0, 10);

    s_route_label = lv_label_create(s_route_box);
    lv_label_set_text(s_route_label, "");
    lv_obj_set_style_text_font(s_route_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_route_label, lv_color_hex(HUD_FG_COLOR), 0);
    lv_label_set_long_mode(s_route_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_route_label, 380);
    lv_obj_center(s_route_label);

    /* 尝试加载中文道路名字体 (SD 卡可能尚未挂载, 失败会后续重试) */
    hud_load_road_font();

    /* 左下角: 剩余距离 + 时间框 */
    s_remain_box = lv_obj_create(parent);
    hud_style_box(s_remain_box);
    lv_obj_set_size(s_remain_box, 180, 64);
    lv_obj_align(s_remain_box, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    s_remain_label = lv_label_create(s_remain_box);
    lv_label_set_text(s_remain_label, "");
    lv_obj_set_style_text_font(s_remain_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_remain_label, lv_color_hex(HUD_FG_COLOR), 0);
    lv_obj_center(s_remain_label);

    nav_hud_hide();
}

void nav_hud_show(void)
{
    lv_obj_clear_flag(s_speed_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_turn_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_remain_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_route_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_speed_box);
    lv_obj_move_foreground(s_turn_box);
    lv_obj_move_foreground(s_remain_box);
    lv_obj_move_foreground(s_route_box);
}

void nav_hud_hide(void)
{
    lv_obj_add_flag(s_speed_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_turn_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_remain_box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_route_box, LV_OBJ_FLAG_HIDDEN);
}

void nav_hud_set_speed(uint16_t speed_kmh)
{
    if (s_speed_num == NULL) {
        return;
    }
    lv_label_set_text_fmt(s_speed_num, "%d", speed_kmh);
}

void nav_hud_set_turn(nav_turn_type_t dir, int dist_m, const char *road)
{
    if (s_turn_icon == NULL) {
        return;
    }

    /* 若路名非空且字体未加载成功, 重试 (此时 SD 卡应已挂载) */
    if (road != NULL && road[0] != '\0') {
        hud_load_road_font();
    }

    lv_label_set_text(s_turn_icon, hud_turn_symbol(dir));

    /* 米数: 只要给了距离就显示 (直行也一样), 0 或负数不显示;
     * >= 1000m 用 km, 否则用 m */
    if (dist_m > 0) {
        if (dist_m >= 1000) {
            lv_label_set_text_fmt(s_turn_dist, "%.1f km", dist_m / 1000.0);
        } else {
            lv_label_set_text_fmt(s_turn_dist, "%d m", dist_m);
        }
    } else {
        lv_label_set_text(s_turn_dist, "");
    }

    lv_label_set_text(s_turn_road, road ? road : "");
}

void nav_hud_set_remain(float remain_km, int remain_min)
{
    if (s_remain_label == NULL) {
        return;
    }
    lv_label_set_text_fmt(s_remain_label, "%.1f km\n%d min", remain_km, remain_min);
}

void nav_hud_set_route_names(const char *start, const char *end)
{
    if (s_route_label == NULL) {
        return;
    }

    /* 有中文名时确保中文 FreeType 字体已加载 */
    if ((start && start[0]) || (end && end[0])) {
        hud_load_road_font();
    }

    const char *s = (start && start[0]) ? start : "";
    const char *e = (end && end[0]) ? end : "";

    if (e[0]) {
        lv_label_set_text_fmt(s_route_label, "%s → %s", s, e);
    } else {
        lv_label_set_text(s_route_label, s);
    }
}
