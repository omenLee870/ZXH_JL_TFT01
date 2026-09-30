#include "system/includes.h"
#include "video_rec_ui.h"
#include "app_config.h"

#if TCFG_APP_VIDEO_REC_EN


#define LOG_TAG             "[VREC_UI]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if CONFIG_LVGL_UI_ENABLE
#include "lvgl.h"
#include "gui_guider.h"
#include "lv_port_video_disp.h"
extern void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name);
extern void lvgl_video_exit(void *handle);
extern void lvgl_video_exit_drain(void *handle);
extern u8 lvgl_ui_is_suspended(void);
extern int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

/* 叠加控件经 lvgl_rpc_post_func 在 "ui" 任务创建/更新/删除, 保证 LVGL 线程安全 */
static lv_obj_t  *s_rec_dot = NULL;       /* REC 红点 */
static lv_obj_t  *s_rec_timer_lbl = NULL; /* 录像计时 MM:SS */
static lv_timer_t *s_rec_blink_timer = NULL;
static lv_obj_t  *s_rec_alert = NULL;
static lv_timer_t *s_rec_alert_timer = NULL;
static void *g_vrec_video_handle = NULL;
#endif

/* 预览旋转铺满: 控件尺寸恒等于预览源帧尺寸(lv_port_video_disp_set_src 每帧按帧尺寸
 * 重设 img 尺寸, 无法用 lv_obj_set_size 覆盖), 故这里读到的 w/hh 就是源的真实尺寸。
 * 仅当"控件尺寸 == 屏尺寸对调"(320x240 <-> 240x320)时旋转 90° 铺满;
 * 源与屏同尺寸同方向时不动(旋转反而会把 320x240 变成 240x320 而铺不满)。
 * 旋转在显示层由 JLGPU 完成, 出帧/编码尺寸不变。需 LVGL 线程调用。 */
#if CONFIG_LVGL_UI_ENABLE
static void video_rec_preview_rotate(lv_video_disp_handle_t *h)
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

static void video_rec_preview_rotate_rpc(void *unused, ...)
{
    (void)unused;
    lv_video_disp_handle_t *h = lv_port_video_disp_get_instance_by_fb_index(5);
    if (h) {
        video_rec_preview_rotate(h);
    }
}
#endif

void video_rec_ui_init(int width, int height, int show_video)
{
#if CONFIG_LVGL_UI_ENABLE
    int wait_ms = 0;
    while (lvgl_ui_is_suspended()) {
        os_time_dly(1);
        wait_ms += 10;
        if (wait_ms >= 3000) {
            log_error("lvgl ui not ready, skip\n");
            return;
        }
    }
    /* UVC H264 直通不显示: 不创建 fb5 视频显示对象。
     * 摄像头预览保持 320x240(width/height 同时决定 LVGL 对 fb5 NV12 帧的读取) */
    if (show_video) {
        g_vrec_video_handle = lvgl_video_init(width, height, 0, 0, "fb5");
        /* create 在 UI 线程完成(经 RPC+信号量); 旋转也需回 UI 线程执行 */
        lvgl_rpc_post_func((void *)video_rec_preview_rotate_rpc, 0);
    }
#endif
}

/* 返回键回主页: 弹LVGL屏幕栈回主页(仿 video_dec_ui_back_to_home), RPC在ui线程执行 */
#if CONFIG_LVGL_UI_ENABLE
static void video_rec_ui_back_to_home_rpc(void *unused, ...)
{
    (void)unused;
    ui_scr_stack_pop_anim(&guider_ui, LV_SCR_LOAD_ANIM_NONE, 100, 100, false, false, false);
}
#endif
void video_rec_ui_back_to_home(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)video_rec_ui_back_to_home_rpc, 0);
#endif
}

#if CONFIG_LVGL_UI_ENABLE
/* REC 红点闪烁定时器(500ms 切换显隐) */
static void rec_blink_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_rec_dot && lv_obj_is_valid(s_rec_dot)) {
        if (lv_obj_has_flag(s_rec_dot, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_rec_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_rec_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* 进入录像态: 创建 REC 红点 + 计时标签 + 启闪烁 */
static void rec_ui_enter_recording_rpc(void)
{
    lv_ui_video_rec *ui_scr = guider_ui.video_rec;
    if (!ui_scr || !ui_scr->video_rec || !lv_obj_is_valid(ui_scr->video_rec)) {
        return;
    }
    if (!s_rec_dot || !lv_obj_is_valid(s_rec_dot)) {
        s_rec_dot = lv_obj_create(ui_scr->video_rec);
        lv_obj_set_size(s_rec_dot, 14, 14);
        lv_obj_set_pos(s_rec_dot, 8, 40);
        lv_obj_set_style_radius(s_rec_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_rec_dot, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_border_width(s_rec_dot, 0, 0);
    }
    lv_obj_clear_flag(s_rec_dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_rec_dot);

    if (!s_rec_timer_lbl || !lv_obj_is_valid(s_rec_timer_lbl)) {
        s_rec_timer_lbl = lv_label_create(ui_scr->video_rec);
        lv_obj_set_pos(s_rec_timer_lbl, 28, 38);
        lv_obj_set_style_text_color(s_rec_timer_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(s_rec_timer_lbl, &lv_font_FangZhengKaiTiJianTi_1_20, 0);
        lv_label_set_text(s_rec_timer_lbl, "00:00");  /* 仅新建时清零; 暂停恢复复用旧标签不清零 */
    }
    lv_obj_clear_flag(s_rec_timer_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_rec_timer_lbl);

    if (!s_rec_blink_timer) {
        s_rec_blink_timer = lv_timer_create(rec_blink_cb, 500, NULL);
    }
}

/* 暂停态: 计时标签保留(冻结在暂停值), 红点停止闪烁保持可见 */
static void rec_ui_enter_paused_rpc(void)
{
    if (s_rec_blink_timer) {
        lv_timer_delete(s_rec_blink_timer);
        s_rec_blink_timer = NULL;
    }
    if (s_rec_dot && lv_obj_is_valid(s_rec_dot)) {
        lv_obj_clear_flag(s_rec_dot, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_rec_timer_lbl && lv_obj_is_valid(s_rec_timer_lbl)) {
        lv_obj_clear_flag(s_rec_timer_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 退出录像态: 停闪烁 + 删红点/计时 */
static void rec_ui_exit_recording_rpc(void)
{
    if (s_rec_blink_timer) {
        lv_timer_delete(s_rec_blink_timer);
        s_rec_blink_timer = NULL;
    }
    if (s_rec_dot && lv_obj_is_valid(s_rec_dot)) {
        lv_obj_delete(s_rec_dot);
    }
    s_rec_dot = NULL;
    if (s_rec_timer_lbl && lv_obj_is_valid(s_rec_timer_lbl)) {
        lv_obj_delete(s_rec_timer_lbl);
    }
    s_rec_timer_lbl = NULL;
}

static void rec_ui_cleanup_all_rpc(void)
{
    rec_ui_exit_recording_rpc();
    if (s_rec_alert_timer) {
        lv_timer_delete(s_rec_alert_timer);
        s_rec_alert_timer = NULL;
    }
    if (s_rec_alert && lv_obj_is_valid(s_rec_alert)) {
        lv_obj_delete(s_rec_alert);
    }
    s_rec_alert = NULL;
}
#endif

void video_rec_ui_deinit(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)rec_ui_cleanup_all_rpc, 0);
    if (g_vrec_video_handle) {
        /* 同步退出: 删视频 img + 排空双链表(LIST==2)异步渲染, 返回后再释放 buffer 才安全 */
        lvgl_video_exit_drain(g_vrec_video_handle);
        g_vrec_video_handle = NULL;
    }
#endif
}

void video_rec_ui_on_state_change(enum video_rec_state old_state,
                                  enum video_rec_state new_state)
{
#if CONFIG_LVGL_UI_ENABLE
    (void)old_state;
    if (new_state == VIDEO_STATE_RECORDING) {
        lvgl_rpc_post_func((void *)rec_ui_enter_recording_rpc, 0);
    } else if (new_state == VIDEO_STATE_PAUSED) {
        /* 暂停保留计时显示, 冻结不闪; 恢复后继续走, 不从 00:00 跳 */
        lvgl_rpc_post_func((void *)rec_ui_enter_paused_rpc, 0);
    } else {
        /* IDLE/SAVING/ERROR: 退出录像态显示 */
        lvgl_rpc_post_func((void *)rec_ui_exit_recording_rpc, 0);
    }
#else
    (void)old_state;
    (void)new_state;
#endif
}

#if CONFIG_LVGL_UI_ENABLE
/* 录像计时刷新(MM:SS); elapsed_sec 经 msg[3] 传入 */
static void rec_ui_timer_rpc(int elapsed_sec)
{
    //log_debug("[VREC] rpc_timer: elapsed_sec=%d lbl_valid=%d\n",
    //          elapsed_sec, (s_rec_timer_lbl && lv_obj_is_valid(s_rec_timer_lbl)) ? 1 : 0);
    if (!s_rec_timer_lbl || !lv_obj_is_valid(s_rec_timer_lbl)) {
        return;
    }
    lv_label_set_text_fmt(s_rec_timer_lbl, "%02u:%02u",
                          (unsigned)elapsed_sec / 60u, (unsigned)elapsed_sec % 60u);
}
#endif

void video_rec_ui_update_timer(u32 elapsed_sec)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)rec_ui_timer_rpc, 1, (int)elapsed_sec);
#else
    (void)elapsed_sec;
#endif
}

#if CONFIG_LVGL_UI_ENABLE
static void rec_ui_alert_close_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_rec_alert && lv_obj_is_valid(s_rec_alert)) {
        lv_obj_delete(s_rec_alert);
    }
    s_rec_alert = NULL;
    if (s_rec_alert_timer) {
        lv_timer_delete(s_rec_alert_timer);
        s_rec_alert_timer = NULL;
    }
}
static void rec_ui_alert_rpc(int type)
{
    lv_ui_video_rec *ui_scr = guider_ui.video_rec;
    const char *txt;
    if (!ui_scr || !ui_scr->video_rec || !lv_obj_is_valid(ui_scr->video_rec)) {
        return;
    }
    switch (type) {
    case 0:
        txt = "存储已满";
        break;
    case 1:
        txt = "请插入SD卡";
        break;
    case 2:
        txt = "电量不足";
        break;
    case 3:
        txt = "Please use FAT32 SD card";
        break;
    default:
        txt = "录像失败";
        break;
    }
    if (s_rec_alert && lv_obj_is_valid(s_rec_alert)) {
        lv_obj_delete(s_rec_alert);
    }
    s_rec_alert = lv_label_create(ui_scr->video_rec);
    lv_label_set_text(s_rec_alert, txt);
    lv_obj_set_style_text_color(s_rec_alert, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_rec_alert, &lv_font_FangZhengKaiTiJianTi_1_20, 0);
    lv_obj_set_style_bg_color(s_rec_alert, lv_color_hex(0xCC0000), 0);
    lv_obj_set_style_bg_opa(s_rec_alert, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(s_rec_alert, 12, 0);
    lv_obj_set_style_radius(s_rec_alert, 8, 0);
    lv_obj_center(s_rec_alert);
    lv_obj_move_foreground(s_rec_alert);
    if (s_rec_alert_timer) {
        lv_timer_delete(s_rec_alert_timer);
    }
    s_rec_alert_timer = lv_timer_create(rec_ui_alert_close_cb, 2000, NULL);
}
#endif

void video_rec_ui_show_alert(int type)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)rec_ui_alert_rpc, 1, (int)type);
#else
    (void)type;
#endif
}


#if CONFIG_LVGL_UI_ENABLE
/* 贴纸预览特效(关/手指/狐狸/小狗/魔法帽): 纯LVGL预览层叠加, 不烧录进录像文件 */
static u8 s_rec_sticker_idx = 0;  /* 0=关 1=finger 2=fox 3=dog 4=hat */

static void rec_ui_hide_all_stickers(lv_ui_video_rec *ui_scr)
{
#if 0
    if (ui_scr->video_rec_animimg_finger) {
        lv_obj_add_flag(ui_scr->video_rec_animimg_finger, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_scr->video_rec_animimg_fox) {
        lv_obj_add_flag(ui_scr->video_rec_animimg_fox, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_scr->video_rec_animimg_dog) {
        lv_obj_add_flag(ui_scr->video_rec_animimg_dog, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_scr->video_rec_img_hat) {
        lv_obj_add_flag(ui_scr->video_rec_img_hat, LV_OBJ_FLAG_HIDDEN);
    }
#endif
}

static void rec_ui_sticker_next_rpc(void)
{
#if 0
    lv_ui_video_rec *ui_scr = guider_ui.video_rec;
    if (!ui_scr || !ui_scr->video_rec || !lv_obj_is_valid(ui_scr->video_rec)) {
        return;
    }
    s_rec_sticker_idx = (s_rec_sticker_idx + 1) % 5;
    rec_ui_hide_all_stickers(ui_scr);

    switch (s_rec_sticker_idx) {
    case 1:
        if (ui_scr->video_rec_animimg_finger) {
            lv_obj_clear_flag(ui_scr->video_rec_animimg_finger, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case 2:
        if (ui_scr->video_rec_animimg_fox) {
            lv_obj_clear_flag(ui_scr->video_rec_animimg_fox, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case 3:
        if (ui_scr->video_rec_animimg_dog) {
            lv_obj_clear_flag(ui_scr->video_rec_animimg_dog, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case 4:
        if (ui_scr->video_rec_img_hat) {
            lv_obj_clear_flag(ui_scr->video_rec_img_hat, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    default:
        break;  /* 0 = 全部隐藏(已在 hide_all 完成) */
    }

    /* 贴纸卡片指示: 有贴纸时短暂显示选中卡, 1.2s 后自动隐藏 */
    if (ui_scr->video_rec_imglist_1) {
        if (s_rec_sticker_idx > 0) {
            lv_imglist_set_act(ui_scr->video_rec_imglist_1, (s_rec_sticker_idx - 1) % 4);
            lv_obj_clear_flag(ui_scr->video_rec_imglist_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ui_scr->video_rec_imglist_1);
        } else {
            lv_obj_add_flag(ui_scr->video_rec_imglist_1, LV_OBJ_FLAG_HIDDEN);
        }
    }
#endif
}

void video_rec_ui_sticker_next(void)
{
    lvgl_rpc_post_func((void *)rec_ui_sticker_next_rpc, 0);
}
#else
void video_rec_ui_sticker_next(void) { }
#endif


#if CONFIG_LVGL_UI_ENABLE
/* 录像变焦倍率标签 video_rec_lbl_2 */
static char s_rec_zoom_text[16];
static void rec_ui_zoom_rpc(void)
{
    lv_ui_video_rec *ui_scr = guider_ui.video_rec;
    if (!ui_scr || !ui_scr->video_rec_lbl_2 || !lv_obj_is_valid(ui_scr->video_rec_lbl_2)) {
        return;
    }
    lv_obj_clear_flag(ui_scr->video_rec_lbl_2, LV_OBJ_FLAG_HIDDEN);  /* 变焦时显示 */
    lv_label_set_text(ui_scr->video_rec_lbl_2, s_rec_zoom_text);
}
void video_rec_ui_update_zoom(float level)
{
    snprintf(s_rec_zoom_text, sizeof(s_rec_zoom_text), "%.1fx", level);
    lvgl_rpc_post_func((void *)rec_ui_zoom_rpc, 0);
}
#else
void video_rec_ui_update_zoom(float level)
{
    (void)level;
}
#endif


#if CONFIG_LVGL_UI_ENABLE
/* 慢动作 UI: imglist_4(0开/1关) + lbl_3 文字 */
static u8 s_rec_slowmo_on = 0;
static void rec_ui_slowmo_rpc(void)
{
    lv_ui_video_rec *ui_scr = guider_ui.video_rec;
    if (!ui_scr) {
        return;
    }
    if (ui_scr->video_rec_imglist_4 && lv_obj_is_valid(ui_scr->video_rec_imglist_4)) {
        lv_obj_clear_flag(ui_scr->video_rec_imglist_4, LV_OBJ_FLAG_HIDDEN);
        lv_imglist_set_act(ui_scr->video_rec_imglist_4, s_rec_slowmo_on ? 0 : 1);
    }
    if (ui_scr->video_rec_lbl_3 && lv_obj_is_valid(ui_scr->video_rec_lbl_3)) {
        if (s_rec_slowmo_on) {
            lv_obj_clear_flag(ui_scr->video_rec_lbl_3, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_scr->video_rec_lbl_3, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
void video_rec_ui_update_slowmo(u8 on)
{
    s_rec_slowmo_on = on;
    lvgl_rpc_post_func((void *)rec_ui_slowmo_rpc, 0);
}
#else
void video_rec_ui_update_slowmo(u8 on)
{
    (void)on;
}
#endif

#endif /* #if TCFG_APP_VIDEO_REC_EN */
