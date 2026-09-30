#include "system/includes.h"
#include "video_photo_ui.h"
#include "app_config.h"
#include "lv_port_video_disp.h"
#include "jlgpu_image_filters.h"  /* video滤镜枚举 jlgpu_filter_mode_t (上游重构从 lv_port_video_disp.h 迁入) */

#if TCFG_APP_PHOTO_EN

#define LOG_TAG             "[PHOTO_UI]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if CONFIG_LVGL_UI_ENABLE
#include "gui_guider.h"
extern u8 get_vbat_percent(void);
#endif
extern char *gui_get_res_path(int32_t id);

#if CONFIG_LVGL_UI_ENABLE
extern void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name);
extern void lvgl_video_exit(void *handle);
extern void lvgl_video_exit_drain(void *handle);
extern u8 lvgl_ui_is_suspended(void);
extern int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

/* 返回键回主页: 弹LVGL屏幕栈回主页(仿 video_dec_ui_back_to_home), RPC在ui线程执行 */
static void video_photo_ui_back_to_home_rpc(void *unused, ...)
{
    (void)unused;
    ui_scr_stack_pop_anim(&guider_ui, LV_SCR_LOAD_ANIM_NONE, 100, 100, false, false, false);
}
#endif
void video_photo_ui_back_to_home(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)video_photo_ui_back_to_home_rpc, 0);
#endif
}

#define PHOTO_EFFECT_COUNT   (JLGPU_FILTER_NEGATIVE + 1)

static char photo_sticker_count = 0;
static uint32_t *g_photo_sticker_res_ids = NULL;
static uint32_t *g_photo_sticker_save_res_ids = NULL;

static void *g_photo_video_handle = NULL;

/* 拍照大图流程(suspend/resume)会销毁并重建 fb5 实例, 缓存句柄会悬空;
 * 统一从注册表现取, 避免 use-after-free(审查 2026-08-27) */
static lv_video_disp_handle_t *photo_ui_get_fb5(void)
{
#if CONFIG_LVGL_UI_ENABLE
    return lv_port_video_disp_get_instance_by_fb_index(5);
#else
    return NULL;
#endif
}

/* 预览旋转铺满: 控件尺寸恒等于预览源帧尺寸(lv_port_video_disp_set_src 每帧按帧尺寸
 * 重设 img 尺寸, 无法用 lv_obj_set_size 覆盖), 故这里读到的 w/hh 就是源的真实尺寸。
 * 仅当"控件尺寸 == 屏尺寸对调"(320x240 <-> 240x320)时旋转 90° 铺满;
 * 源与屏同尺寸同方向时不动(旋转反而会把 320x240 变成 240x320 而铺不满)。
 * 旋转在显示层由 JLGPU 完成, fb5 出帧/编码尺寸不变。需 LVGL 线程调用。 */
#if CONFIG_LVGL_UI_ENABLE
static void photo_ui_preview_rotate(lv_video_disp_handle_t *h)
{
    lv_obj_t *img;
    lv_display_t *disp;
    int32_t w, hh, dw, dh;

    if (!h) {
        return;
    }
    img = lv_port_video_disp_get_img_obj(h);
    disp = lv_display_get_default();
    if (!img || !disp) {
        return;
    }
    w  = lv_obj_get_width(img);
    hh = lv_obj_get_height(img);
    dw = lv_display_get_horizontal_resolution(disp);
    dh = lv_display_get_vertical_resolution(disp);
    if (w == dh && hh == dw) {
        lv_port_video_disp_set_rotation(h, 900);
        lv_port_video_disp_set_pivot(h, w / 2, hh / 2);
        lv_obj_set_pos(img, (dw - w) / 2, (dh - hh) / 2);
    }
}

static void photo_ui_preview_rotate_rpc(void *unused, ...)
{
    (void)unused;
    photo_ui_preview_rotate(photo_ui_get_fb5());
}
#endif

static const char *g_photo_sticker_path = NULL;

_WEAK_
char scr_get_photo_sticker_res_tab(uint32_t **res_ids, uint32_t **save_res_ids)
{
    return 0;
}

static void photo_ui_trigger_effect_rpc(int32_t effect_type, int32_t src, int32_t x, int32_t y)
{
    lv_video_disp_handle_t *h = photo_ui_get_fb5();
    if (h) {
        lv_port_video_set_effect(h, (uint8_t)effect_type, (const void *)(uintptr_t)src, x, y);
        /* set_effect 会把 img 切回 STRETCH(自动变换 align), LVGL 会清零 rotation/
         * pivot; 需重新套用竖屏预览旋转, 否则滤镜后画面退回横屏被截/留白 */
        photo_ui_preview_rotate(h);
    }
}

void photo_ui_init(int width, int height, int x, int y, int show_video)
{
#if CONFIG_LVGL_UI_ENABLE
    photo_sticker_count = scr_get_photo_sticker_res_tab(&g_photo_sticker_res_ids, &g_photo_sticker_save_res_ids);
    ASSERT(photo_sticker_count, "photo_sticker_count is 0");
    int wait_ms = 0;
    while (lvgl_ui_is_suspended()) {
        os_time_dly(1);
        wait_ms += 10;
        if (wait_ms >= 3000) {
            log_error("lvgl ui not ready, skip\n");
            return;
        }
    }
    /* H264 直通不显示预览: 不创建 fb5 视频显示对象。
     * 摄像头预览保持 320x240(width/height 同时决定 LVGL 对 fb5 NV12 帧的读取) */
    if (show_video) {
        g_photo_video_handle = lvgl_video_init(width, height, x, y, "fb5");
        /* create 在 UI 线程完成(经 RPC+信号量); 旋转也需回 UI 线程执行 */
        lvgl_rpc_post_func((void *)photo_ui_preview_rotate_rpc, 0);
    }
#endif
}

#if CONFIG_LVGL_UI_ENABLE
static void photo_ui_video_refresh_rpc(void *unused, ...)
{
    (void)unused;
    lv_video_disp_handle_t *h = photo_ui_get_fb5();
    if (!h) {
        return;
    }
    /* 拍照大图流程会销毁并重建 fb5 实例, 这里重新应用预览旋转(幂等) */
    photo_ui_preview_rotate(h);
    lv_port_video_disp_show(h, 1);
    lv_obj_t *img = lv_port_video_disp_get_img_obj(h);
    if (img) {
        lv_obj_invalidate(img);
    }
}
#endif

void photo_ui_video_refresh(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)photo_ui_video_refresh_rpc, 0);
#endif
}

/* 倒计时大字 & 警告弹窗 —— 拍照屏 GUI Guider 未生成对应控件,
 * 这里在拍照屏(guider_ui.video_photo)上运行时动态叠加 lv_label,
 * 全部经 lvgl_rpc_post_func 在 ui 线程创建/更新/删除, 保证 LVGL 线程安全。 */
static lv_obj_t  *s_photo_countdown = NULL;
static lv_obj_t  *s_photo_alert = NULL;
static lv_timer_t *s_photo_alert_timer = NULL;

static void photo_overlay_cleanup_rpc(void)
{
#if CONFIG_LVGL_UI_ENABLE
    if (s_photo_countdown && lv_obj_is_valid(s_photo_countdown)) {
        lv_obj_delete(s_photo_countdown);
    }
    s_photo_countdown = NULL;
    if (s_photo_alert && lv_obj_is_valid(s_photo_alert)) {
        lv_obj_delete(s_photo_alert);
    }
    s_photo_alert = NULL;
    if (s_photo_alert_timer) {
        lv_timer_delete(s_photo_alert_timer);
        s_photo_alert_timer = NULL;
    }
#endif
}

void photo_ui_deinit(void)
{
#if CONFIG_LVGL_UI_ENABLE
    /* 退出前清理动态叠加层(倒计时/弹窗)及其定时器 */
    lvgl_rpc_post_func((void *)photo_overlay_cleanup_rpc, 0);
    lv_video_disp_handle_t *h = photo_ui_get_fb5();
    if (h) {
        /* 同步退出: 删除视频 img 并排空双链表(LIST==2)的异步渲染线程,
         * 返回后释放视频 buffer 才安全, 避免与异步渲染竞态。 */
        lvgl_video_exit_drain(h);
    }
    g_photo_video_handle = NULL;
#endif
}

void photo_ui_on_state_change(enum photo_state old_state,
                              enum photo_state new_state)
{
    (void)old_state;
    (void)new_state;
}

/* 变焦倍率标签 video_photo_lbl_2 (如 "1.5x"); 逻辑在 app_core 线程,
 * 经 lvgl_rpc_post_func 投递到 ui 线程更新, 保证 LVGL 线程安全。 */
static char g_photo_zoom_text[16];
static void photo_ui_update_zoom_rpc(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lv_ui_video_photo *ui_scr = guider_ui.video_photo;
    if (!ui_scr || !ui_scr->video_photo_lbl_2 ||
        !lv_obj_is_valid(ui_scr->video_photo_lbl_2)) {
        return;
    }
    lv_label_set_text(ui_scr->video_photo_lbl_2, g_photo_zoom_text);
#endif
}
void photo_ui_update_zoom(float level)
{
#if CONFIG_LVGL_UI_ENABLE
    snprintf(g_photo_zoom_text, sizeof(g_photo_zoom_text), "%.1fx", level);
    lvgl_rpc_post_func((void *)photo_ui_update_zoom_rpc, 0);
#else
    (void)level;
#endif
}

/* 定时自拍倒计时大字(数字, 屏幕居中); seconds<=0 时移除。
 * 数字字形取自 digitclock 字库(含 0-9), 显示安全。 */
static void photo_ui_countdown_rpc(int seconds)
{
#if CONFIG_LVGL_UI_ENABLE
    lv_ui_video_photo *ui_scr = guider_ui.video_photo;
    if (!ui_scr || !ui_scr->video_photo || !lv_obj_is_valid(ui_scr->video_photo)) {
        return;
    }
    if (seconds > 0) {
        if (!s_photo_countdown || !lv_obj_is_valid(s_photo_countdown)) {
            s_photo_countdown = lv_label_create(ui_scr->video_photo);
            lv_obj_set_style_text_color(s_photo_countdown, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(s_photo_countdown, &lv_font_FangZhengKaiTiJianTi_1_20, 0);
            lv_obj_set_style_bg_color(s_photo_countdown, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(s_photo_countdown, LV_OPA_50, 0);
            lv_obj_set_style_pad_all(s_photo_countdown, 16, 0);
            lv_obj_set_style_radius(s_photo_countdown, 8, 0);
        }
        lv_label_set_text_fmt(s_photo_countdown, "%d", seconds);
        lv_obj_center(s_photo_countdown);
        lv_obj_move_foreground(s_photo_countdown);
    } else {
        if (s_photo_countdown && lv_obj_is_valid(s_photo_countdown)) {
            lv_obj_delete(s_photo_countdown);
        }
        s_photo_countdown = NULL;
    }
#endif
}
void photo_ui_update_countdown(int seconds)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)photo_ui_countdown_rpc, 1, (int)seconds);
#else
    (void)seconds;
#endif
}

void photo_ui_trigger_effect(int effect, const char *path)
{
#if CONFIG_LVGL_UI_ENABLE
    jlgpu_filter_mode_t effect_type = JLGPU_FILTER_NONE;
    const void *src = NULL;

    if (effect >= 0 && effect < PHOTO_EFFECT_COUNT) {
        effect_type = (jlgpu_filter_mode_t)effect;
    }
    if (effect_type == JLGPU_FILTER_STICKER) {
        if (path && path[0]) {
            g_photo_sticker_path = path;
        } else {
            g_photo_sticker_path = gui_get_res_path(g_photo_sticker_res_ids[0]);
        }
        src = g_photo_sticker_path;
    }
    lvgl_rpc_post_func((void *)photo_ui_trigger_effect_rpc, 4,
                       (int32_t)effect_type,
                       (int32_t)(uintptr_t)src,
                       0,
                       0);
#else
    (void)effect;
    (void)path;
#endif
}

/* 警告弹窗(居中提示, 2 秒自动消失)。
 * 注意: 中文字形依赖 GUI Guider 字体子集, 若字库未含相应汉字将显示缺字方块,
 *       需在 UI 工具里把提示文案纳入字库(属 generated/资源范畴)。 */
static void photo_ui_alert_close_cb(lv_timer_t *timer)
{
#if CONFIG_LVGL_UI_ENABLE
    (void)timer;
    if (s_photo_alert && lv_obj_is_valid(s_photo_alert)) {
        lv_obj_delete(s_photo_alert);
    }
    s_photo_alert = NULL;
    if (s_photo_alert_timer) {
        lv_timer_delete(s_photo_alert_timer);
        s_photo_alert_timer = NULL;
    }
#endif
}
static void photo_ui_alert_rpc(int type)
{
#if CONFIG_LVGL_UI_ENABLE
    lv_ui_video_photo *ui_scr = guider_ui.video_photo;
    const char *txt;
    if (!ui_scr || !ui_scr->video_photo || !lv_obj_is_valid(ui_scr->video_photo)) {
        return;
    }
    switch (type) {
    case 0:
        txt = "存储已满";
        break;
    case 1:
        txt = "请插入SD卡";
        break;
    case 3:
        txt = "Please use FAT32 SD card";
        break;
    default:
        txt = "拍照失败";
        break;
    }
    if (s_photo_alert && lv_obj_is_valid(s_photo_alert)) {
        lv_obj_delete(s_photo_alert);
    }
    s_photo_alert = lv_label_create(ui_scr->video_photo);
    lv_label_set_text(s_photo_alert, txt);
    lv_obj_set_style_text_color(s_photo_alert, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_photo_alert, &lv_font_FangZhengKaiTiJianTi_1_20, 0);
    lv_obj_set_style_bg_color(s_photo_alert, lv_color_hex(0xCC0000), 0);
    lv_obj_set_style_bg_opa(s_photo_alert, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(s_photo_alert, 12, 0);
    lv_obj_set_style_radius(s_photo_alert, 8, 0);
    lv_obj_center(s_photo_alert);
    lv_obj_move_foreground(s_photo_alert);
    if (s_photo_alert_timer) {
        lv_timer_delete(s_photo_alert_timer);
    }
    s_photo_alert_timer = lv_timer_create(photo_ui_alert_close_cb, 2000, NULL);
#endif
}
void photo_ui_show_alert(int type)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)photo_ui_alert_rpc, 1, (int)type);
#else
    (void)type;
#endif
}

const char *photo_ui_get_sticker_path(void)
{
    if (g_photo_sticker_path) {
        return g_photo_sticker_path;
    }
#if CONFIG_LVGL_UI_ENABLE
    return gui_get_res_path(g_photo_sticker_res_ids[0]);
#else
    return NULL;
#endif
}

const char *photo_ui_get_sticker_path_by_idx(uint8_t idx)
{
    if (idx >= photo_sticker_count) {
        idx = 0;
    }
#if CONFIG_LVGL_UI_ENABLE
    return gui_get_res_path(g_photo_sticker_res_ids[idx]);
#else
    return NULL;
#endif
}

const char *photo_ui_get_sticker_save_path_by_idx(uint8_t idx, uint16_t width, uint16_t height)
{
    (void)width;
    (void)height;
    if (idx >= photo_sticker_count) {
        idx = 0;
    }
#if CONFIG_LVGL_UI_ENABLE
    return gui_get_res_path(g_photo_sticker_save_res_ids[idx]);
#else
    return NULL;
#endif
}


/* 电池电量图标 video_photo_imglist_3 (0空/1低/2半/3满);
 * 每秒由 video_photo.c 主循环调用, 档位无变化时不刷新。 */
static int g_photo_batt_level = -1;
static void photo_ui_update_battery_rpc(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lv_ui_video_photo *ui_scr = guider_ui.video_photo;
    if (!ui_scr || !ui_scr->video_photo_imglist_3 ||
        !lv_obj_is_valid(ui_scr->video_photo_imglist_3)) {
        return;
    }
    lv_imglist_set_act(ui_scr->video_photo_imglist_3, g_photo_batt_level);
#endif
}
void photo_ui_update_battery(void)
{
#if CONFIG_LVGL_UI_ENABLE
    /* get_ldo5v_online_hw(): CPU 底层直读 P33 LDO5V_DET 寄存器, 检测充电器5V是否插入,
     * 不依赖 app 层充电模块(TCFG_CHARGE_ENABLE), 充电宏关闭也可靠。 */
    extern u8 get_ldo5v_online_hw(void);
    static u8 s_blink = 0;
    int lvl, show;
    u8 pct = get_vbat_percent();

    if (pct >= 80) {
        lvl = 3;    /* full  */
    } else if (pct >= 50) {
        lvl = 2;    /* half  */
    } else if (pct >= 20) {
        lvl = 1;    /* low   */
    } else {
        lvl = 0;    /* empty */
    }

    if (get_ldo5v_online_hw()) {
        if (lvl >= 3) {
            show = 3;                   /* 充电器插入且电量满档: 定格满电 */
        } else if (pct == 0) {
            show = 3;                   // 无电量检测电路时插电定格满格, 避免空/满每秒闪烁
        } else {
            s_blink ^= 1;               /* 充电中: 当前档<->满档交替, 形成充电闪烁 */
            show = s_blink ? 3 : lvl;
        }
    } else {
        show = lvl;                     /* 未插充电器: 显示真实电量档 */
    }

    if (show == g_photo_batt_level) {
        return;                         /* 无变化不刷新 */
    }
    g_photo_batt_level = show;
    lvgl_rpc_post_func((void *)photo_ui_update_battery_rpc, 0);
#endif
}

#endif /* #if TCFG_APP_PHOTO_EN */
