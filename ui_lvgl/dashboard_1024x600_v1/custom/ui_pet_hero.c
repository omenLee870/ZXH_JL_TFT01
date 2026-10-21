/**
 * @file ui_pet_hero.c
 * @brief 前进飞行、定向转弯、撞玻璃刹车及短开机动画。
 * @note 主体和独立披风来自 JLUI 编译资源，特效由 LVGL 绘制；
 *       所有控件由 JLUI 模型创建，页面卸载时由原动画模块停止刷新。
 */
#include "ui_pet_hero.h"

#define HERO_FRAME_MS 50u
#define HERO_BOOT_MS 1600u
#define HERO_TRIGO(a, r) ((int32_t)lv_trigo_sin((a) % 360) * (r) / 32767)

typedef enum {
    HERO_IDLE, HERO_FORWARD, HERO_LEFT, HERO_RIGHT, HERO_BRAKE
} hero_scene_t;

typedef struct {
    lv_obj_t *layer, *body, *cape;
    bool boot;
} hero_view_t;

static struct {
    lv_obj_t *root, *boot_cover, *original;
    hero_view_t ride, boot;
    uint16_t speed;
    uint32_t flags, changed_ms, boot_ms, frame_ms;
    hero_scene_t scene;
    unsigned phase;
    bool valid, visible, boot_pending, boot_active;
} hero;
static bool cold_boot_used;
static lv_opa_t draw_opacity = LV_OPA_COVER; /**< 开机绘图淡入淡出，避免整屏透明图层分配。 */

/** @brief 非业务控件显隐，不对无父对象的屏幕根使用 HIDDEN。 */
static void show(lv_obj_t *obj, bool visible)
{
    if (!obj) return;
    if (visible) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

/** @brief 统一动作优先级；双转向同时亮不随闪烁周期左右切换。 */
static hero_scene_t select_scene(void)
{
    if (hero.flags & UI_PET_HERO_BRAKE) return HERO_BRAKE;
    if ((hero.flags & (UI_PET_HERO_LEFT | UI_PET_HERO_RIGHT)) == UI_PET_HERO_LEFT) return HERO_LEFT;
    if ((hero.flags & (UI_PET_HERO_LEFT | UI_PET_HERO_RIGHT)) == UI_PET_HERO_RIGHT) return HERO_RIGHT;
    return hero.valid && hero.speed ? HERO_FORWARD : HERO_IDLE;
}

/** @brief 仅在动作变化时换图片，不按帧交替手臂或重复解码整只小狗。 */
static void scene_refresh(void)
{
    hero_scene_t scene = select_scene();
    if (scene != hero.scene) {
        hero.scene = scene;
        hero.changed_ms = lv_tick_get();
        if (hero.ride.body) {
            uint32_t resource = scene == HERO_LEFT ? PET_HERO_LEFT :
                                scene == HERO_RIGHT ? PET_HERO_RIGHT :
                                scene == HERO_BRAKE ? PET_HERO_BRAKE : PET_HERO_FORWARD;
            lv_image_set_src(hero.ride.body, gui_get_res_path(resource));
            lv_image_set_scale(hero.ride.body, 256);
            lv_image_set_rotation(hero.ride.body, 0);
            bool braking = scene == HERO_BRAKE;
            lv_obj_set_size(hero.ride.body, braking ? 360 : 230, braking ? 280 : 230);
            lv_obj_set_pos(hero.ride.body, braking ? 0 : 65, braking ? 0 : 30);
            lv_image_set_pivot(hero.ride.body, braking ? 180 : 115, braking ? 170 : 150);
            /* 刹车素材已包含披风、裂纹与星星，禁止叠加飞行披风。 */
            show(hero.ride.cape, !braking);
        }
    }
    show(hero.ride.layer, hero.visible && scene != HERO_IDLE && !hero.boot_active);
}

/**
 * @brief 由车辆 UI 适配层提交新鲜显示快照。
 * @param[in] speed_dkph 原始车速，单位 0.1km/h，不受公英制切换影响。
 * @param[in] valid 车速是否在线且有效。
 * @param[in] flags 已确认有效的刹车和转向位。
 * @note 未知、掉线数据不能触发飞行；刹车优先于转向和前进。
 */
void ui_pet_hero_update(uint16_t speed_dkph, bool valid, uint32_t flags)
{
    hero.speed = speed_dkph;
    hero.valid = valid;
    hero.flags = flags;
    /* 行驶/刹车反馈到达时立即让出开机遮罩，保持车辆信息可见。 */
    if ((valid && speed_dkph) || (flags & UI_PET_HERO_BRAKE)) {
        hero.boot_active = hero.boot_pending = false;
        show(hero.boot_cover, false);
    }
    scene_refresh();
}

/** @brief 保留旧车速接口及工具标签预览；完整快照随后覆盖有效位和状态位。 */
void ui_pet_hero_speed(uint16_t speed_dkph)
{
    ui_pet_hero_update(speed_dkph, true, hero.flags);
}

/** @brief 正常仪表显示阶段才播放动画，音乐/导航/关机卸载时同步停画。 */
void ui_pet_hero_visible(bool visible)
{
    hero.visible = visible;
    if (!visible) {
        hero.boot_active = hero.boot_pending = false;
        show(hero.boot_cover, false);
    } else if (hero.boot_pending && hero.boot_cover) {
        hero.boot_pending = false;
        cold_boot_used = true;
        if (!hero.speed && !(hero.flags & UI_PET_HERO_BRAKE)) {
            hero.boot_active = true;
            hero.boot_ms = lv_tick_get();
            lv_obj_set_style_opa(hero.boot_cover, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_opa(hero.boot_cover, LV_OPA_COVER, 0);
            lv_obj_set_style_image_opa(hero.boot.body, 0, 0);
            lv_obj_set_style_image_opa(hero.boot.cape, 0, 0);
            lv_image_set_scale(hero.boot.body, 160);
            show(hero.boot_cover, true);
        }
    }
    hero.frame_ms = lv_tick_get() - HERO_FRAME_MS;
    scene_refresh();
}

/** @brief 供原静止眨眼模块判断是否需要临时隐藏坐姿小狗。 */
bool ui_pet_hero_active(void)
{
    return hero.root && hero.visible && (hero.boot_active || hero.scene != HERO_IDLE);
}

/** @brief 软件重新开机时请求一次动画；切页或返回宠物页不会重复请求。 */
void ui_pet_hero_boot_request(void)
{
    hero.boot_pending = true;
}

/** @brief 绘制一段描边线，坐标相对 JLUI 动作容器。 */
static void line(lv_layer_t *layer, const lv_area_t *a, int x1, int y1, int x2, int y2,
                 uint32_t color, unsigned width, lv_opa_t opa)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.p1.x = a->x1 + x1; d.p1.y = a->y1 + y1;
    d.p2.x = a->x1 + x2; d.p2.y = a->y1 + y2;
    d.color = lv_color_hex(color); d.width = width; d.opa = (lv_opa_t)((unsigned)opa * draw_opacity / 255u);
    d.round_start = d.round_end = 1;
    lv_draw_line(layer, &d);
}

/** @brief 仅变换完整透明披风，固定肩部支点，避免三角拼接缝和主体换手。 */
static void cape_frame(hero_view_t *view, hero_scene_t scene, unsigned phase)
{
    bool turning = scene == HERO_LEFT || scene == HERO_RIGHT;
    /* 转向偏移前收窄布面，保持尾端在动作容器内，避免披风被切出直边。 */
    lv_image_set_scale_x(view->cape, turning ? 210 : 256);
    lv_image_set_scale_y(view->cape, (uint32_t)(256 + HERO_TRIGO(phase, 18)));
    lv_image_set_rotation(view->cape, HERO_TRIGO(phase + 40, 15));
    lv_obj_set_x(view->cape, scene == HERO_LEFT ? 20 : scene == HERO_RIGHT ? -20 : 0);
}

/** @brief 风线仅在角色两侧划过，不改变草地图、不覆盖车速和时间。 */
static void wind(lv_layer_t *layer, const lv_area_t *a, unsigned phase)
{
    for (unsigned i = 0; i < 3; ++i) {
        unsigned progress = (phase + i * 120) % 360;
        int offset = 40 + (int)progress * 95 / 360;
        int y = 172 + (int)i * 23;
        lv_opa_t opa = (lv_opa_t)((360 - progress) * 160 / 360);
        line(layer, a, 180-offset, y, 180-offset-20, y+5, 0xd6f7ff, 2, opa);
        line(layer, a, 180+offset, y, 180+offset+20, y+5, 0xd6f7ff, 2, opa);
    }
}

/** @brief 同一绘图入口用于骑行层和开机层；只在各自可见时参与绘制。 */
static void draw(lv_event_t *event)
{
    hero_view_t *view = lv_event_get_user_data(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t area;
    lv_obj_get_coords(view->layer, &area);
    uint32_t now = lv_tick_get();
    draw_opacity = LV_OPA_COVER;
    if (view->boot) {
        uint32_t elapsed = now - hero.boot_ms;
        draw_opacity = elapsed < 300 ? (lv_opa_t)(elapsed * 255u / 300u) :
                       elapsed < 1200 ? LV_OPA_COVER :
                       elapsed < HERO_BOOT_MS ? (lv_opa_t)((HERO_BOOT_MS-elapsed)*255u/400u) : 0;
    }
    hero_scene_t scene = view->boot ? HERO_FORWARD : hero.scene;
    unsigned phase = hero.phase;
    /* 刹车使用完整审核素材，避免程序画裂纹改变已确认的画面。 */
    if (scene != HERO_BRAKE) wind(layer, &area, phase);
}

/** @brief 使用原有 50ms 动画节拍，避免额外 RTOS 定时器和卸载后回调。 */
void ui_pet_hero_tick(void)
{
    if (!hero.visible || !hero.root) return;
    uint32_t now = lv_tick_get();
    if (now - hero.frame_ms < HERO_FRAME_MS) return;
    hero.frame_ms = now;
    hero.phase = (hero.phase + (hero.scene == HERO_BRAKE ? 2u : 5u + LV_MIN(hero.speed / 60u, 8u))) % 360u;
    if (hero.scene != HERO_IDLE && hero.scene != HERO_BRAKE)
        cape_frame(&hero.ride, hero.scene, hero.phase);
    if (hero.boot_active) {
        uint32_t elapsed = now - hero.boot_ms;
        if (elapsed >= HERO_BOOT_MS) {
            hero.boot_active = false;
            show(hero.boot_cover, false);
            scene_refresh();
        } else {
            unsigned approach = LV_MIN(elapsed, 650u);
            lv_image_set_scale(hero.boot.body, 160u + approach * 96u / 650u);
            lv_opa_t fade = elapsed < 1200 ? LV_OPA_COVER :
                           (lv_opa_t)((HERO_BOOT_MS-elapsed)*255u/400u);
            lv_opa_t image_opa = elapsed < 300 ? (lv_opa_t)(elapsed*255u/300u) : fade;
            lv_obj_set_style_image_opa(hero.boot.body, image_opa, 0);
            lv_obj_set_style_image_opa(hero.boot.cape, image_opa, 0);
            cape_frame(&hero.boot, HERO_FORWARD, hero.phase);
            lv_obj_set_style_bg_opa(hero.boot_cover, fade, 0);
            lv_obj_invalidate(hero.boot.layer);
        }
    }
    if (hero.scene == HERO_BRAKE) {
        uint32_t elapsed = now - hero.changed_ms;
        /* 上升沿仅撞近一次，随后缓慢轻晃；松刹车立即恢复实时动作。 */
        unsigned scale = elapsed < 100 ? 256u + elapsed * 12u / 100u :
                         elapsed < 300 ? 268u - (elapsed-100u)*12u/200u : 256u;
        lv_image_set_scale(hero.ride.body, scale);
        lv_image_set_rotation(hero.ride.body, elapsed < 300 ? 0 :
                              HERO_TRIGO(((elapsed - 300u) % 2400u) * 360u / 2400u, 8));
    }
    if (hero.scene != HERO_IDLE && !hero.boot_active) lv_obj_invalidate(hero.ride.layer);
}

/** @brief 页面销毁时只清除当前根的引用，避免旧页删除误清新页面。 */
static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) == hero.root) lv_memzero(&hero, sizeof(hero));
}

/** @brief 飞行图片沿用原宠物的点按入口，保持音乐切换交互可用。 */
static void clicked(lv_event_t *event)
{
    (void)event;
    if (hero.visible && !hero.boot_active && hero.original)
        lv_obj_send_event(hero.original, LV_EVENT_CLICKED, NULL);
}

/** @brief 绑定工具创建的容器与图片，控件指针独立于生成页面结构生命周期。 */
void ui_pet_hero_bind(lv_ui_chongwuyibiao *ui)
{
    if (!ui || !ui->chongwuyibiao || hero.root == ui->chongwuyibiao) return;
    if (!ui->chongwuyibiao_hero_scene || !ui->chongwuyibiao_hero_body || !ui->chongwuyibiao_hero_cape ||
        !ui->chongwuyibiao_hero_boot || !ui->chongwuyibiao_boot_hero_scene ||
        !ui->chongwuyibiao_boot_hero_body || !ui->chongwuyibiao_boot_hero_cape) return;
    lv_memzero(&hero, sizeof(hero));
    hero.root = ui->chongwuyibiao;
    hero.original = ui->chongwuyibiao_xiaogou;
    hero.ride.layer = ui->chongwuyibiao_hero_scene;
    hero.ride.body = ui->chongwuyibiao_hero_body;
    hero.ride.cape = ui->chongwuyibiao_hero_cape;
    hero.boot_cover = ui->chongwuyibiao_hero_boot;
    hero.boot.layer = ui->chongwuyibiao_boot_hero_scene;
    hero.boot.body = ui->chongwuyibiao_boot_hero_body;
    hero.boot.cape = ui->chongwuyibiao_boot_hero_cape;
    hero.boot.boot = true;
    lv_obj_add_flag(hero.boot_cover, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(hero.ride.body, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hero.ride.body, clicked, LV_EVENT_CLICKED, NULL);
    hero.boot_pending = !cold_boot_used;
    hero_view_t *views[] = {&hero.ride, &hero.boot};
    for (unsigned i = 0; i < 2; ++i) {
        lv_image_set_pivot(views[i]->body, 115, 150);
        lv_image_set_pivot(views[i]->cape, 180, 90);
        lv_obj_move_to_index(views[i]->cape, 0);
        lv_obj_add_event_cb(views[i]->layer, draw, LV_EVENT_DRAW_MAIN, views[i]);
    }
    lv_obj_add_event_cb(hero.root, deleted, LV_EVENT_DELETE, NULL);
}
