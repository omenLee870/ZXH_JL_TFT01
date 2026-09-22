/**
 * @file video_dec_ui.c
 * @brief 视频解码回放 UI 空桩实现
 *
 * 本文件提供 video_dec_ui.h 的空桩实现，所有函数仅记录调用日志。
 * 待 LVGL 代码生成工具生成真实 UI 代码后替换此文件。
 *
 * @author  Coder2
 * @date    2026-05-29
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_dec_ui.data.bss")
#pragma data_seg(".video_dec_ui.data")
#pragma const_seg(".video_dec_ui.text.const")
#pragma code_seg(".video_dec_ui.text")
#endif

#include "system/includes.h"
#include "video_dec_ui.h"
#include "app_config.h"

#if TCFG_APP_VIDEO_DEC_EN

#if CONFIG_LVGL_UI_ENABLE
#include "lvgl.h"
#include "gui_guider.h"
#include "lv_port_video_disp.h"
#endif

#define LOG_TAG             "[VIDEO_DEC_UI]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if CONFIG_LVGL_UI_ENABLE
extern void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name);
extern void lvgl_video_exit(void *handle);
extern void lvgl_video_exit_drain(void *handle);
extern u8 lvgl_ui_is_suspended(void);
extern int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);
#endif

/* 竖屏面板(240x320)上把 320x240 回放画面旋转 90° 铺满全屏(与对讲/拍照/录像一致)。
 * 仅当"控件尺寸 == 显示尺寸对调"(320x240 <-> 240x320)时生效, 其它产品/布局跳过;
 * 旋转在显示层由 JLGPU 完成, 解码出帧仍 320x240。需在 LVGL 线程调用。 */
#if CONFIG_LVGL_UI_ENABLE
static void video_dec_preview_rotate(lv_video_disp_handle_t *h)
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

static void video_dec_preview_rotate_rpc(void *unused, ...)
{
    (void)unused;
    video_dec_preview_rotate(lv_port_video_disp_get_instance_by_fb_index(5));
}
#endif

#if CONFIG_LVGL_UI_ENABLE
/* 删除菜单 RPC(ui线程): roller_1=删除所有/取消/删除当前, roller_2=确认/取消。
 * 由 app_core 状态机(video_dec.c)经 lvgl_rpc_post_func 调用, 传值走 g_del_ui_sel。 */
static int g_del_ui_sel = 0;
static void *g_vdec_video_handle = NULL;
/* video disp img_obj 显示状态 (1=显示 0=隐藏): 删除所有后隐藏防最后一帧残留 */
static int g_vdec_disp_show = 1;
static void vdec_video_disp_show_rpc(void *unused, ...)
{
    (void)unused;
    lv_port_video_disp_show(g_vdec_video_handle, g_vdec_disp_show);
}
static void vdec_del_show_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    if (!ui_scr) {
        return;
    }
    if (ui_scr->video_dec_roller_2 && lv_obj_is_valid(ui_scr->video_dec_roller_2)) {
        lv_obj_add_flag(ui_scr->video_dec_roller_2, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_scr->video_dec_roller_1 && lv_obj_is_valid(ui_scr->video_dec_roller_1)) {
        lv_roller_set_selected(ui_scr->video_dec_roller_1, g_del_ui_sel, LV_ANIM_OFF);
        lv_obj_clear_flag(ui_scr->video_dec_roller_1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_scr->video_dec_roller_1);
    }
}
static void vdec_del_setsel_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    if (!ui_scr) {
        return;
    }
    if (ui_scr->video_dec_roller_1 && lv_obj_is_valid(ui_scr->video_dec_roller_1)) {
        lv_roller_set_selected(ui_scr->video_dec_roller_1, g_del_ui_sel, LV_ANIM_ON);
    }
}
static void vdec_del_confirm_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    if (!ui_scr) {
        return;
    }
    if (ui_scr->video_dec_roller_2 && lv_obj_is_valid(ui_scr->video_dec_roller_2)) {
        lv_roller_set_selected(ui_scr->video_dec_roller_2, 0, LV_ANIM_OFF); /* 默认"确认" */
        lv_obj_clear_flag(ui_scr->video_dec_roller_2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_scr->video_dec_roller_2);
    }
}
static void vdec_del_hide_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    if (!ui_scr) {
        return;
    }
    if (ui_scr->video_dec_roller_1 && lv_obj_is_valid(ui_scr->video_dec_roller_1)) {
        lv_obj_add_flag(ui_scr->video_dec_roller_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_scr->video_dec_roller_2 && lv_obj_is_valid(ui_scr->video_dec_roller_2)) {
        lv_obj_add_flag(ui_scr->video_dec_roller_2, LV_OBJ_FLAG_HIDDEN);
    }
}
#endif
void video_dec_ui_del_menu_show(int sel)
{
#if CONFIG_LVGL_UI_ENABLE
    g_del_ui_sel = sel;
    lvgl_rpc_post_func((void *)vdec_del_show_rpc, 0);
#else
    (void)sel;
#endif
    log_info("video_dec_ui_del_menu_show(%d)\n", sel);
}
void video_dec_ui_del_menu_set_sel(int sel)
{
#if CONFIG_LVGL_UI_ENABLE
    g_del_ui_sel = sel;
    lvgl_rpc_post_func((void *)vdec_del_setsel_rpc, 0);
#else
    (void)sel;
#endif
}
void video_dec_ui_del_menu_confirm(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)vdec_del_confirm_rpc, 0);
#endif
    log_info("video_dec_ui_del_menu_confirm()\n");
}
void video_dec_ui_del_menu_hide(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)vdec_del_hide_rpc, 0);
#endif
    log_info("video_dec_ui_del_menu_hide()\n");
}

void video_dec_ui_init(int width, int height)
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
#endif
#if CONFIG_LVGL_UI_ENABLE
    /* fb 只在进回放模式时创建一次, 全程照片/视频切换共用(对齐官方playback demo),
     * 避免每次切换重复 lvgl_video_init 导致 fb 泄漏白屏。
     * 回放画面保持 320x240(width/height 同时决定 LVGL 对 fb5 NV12 帧的读取) */
    g_vdec_video_handle = lvgl_video_init(width, height, 0, 0, "fb5");
    /* create 在 UI 线程完成(经 RPC+信号量); 竖屏旋转也需回 UI 线程执行 */
    lvgl_rpc_post_func((void *)video_dec_preview_rotate_rpc, 0);
#endif
    log_info("video_dec_ui_init()\n");
}

void video_dec_ui_deinit(void)
{
#if CONFIG_LVGL_UI_ENABLE
    if (g_vdec_video_handle) {
        /* 同步退出: 删视频 img + 排空双链表(LIST==2)异步渲染, 返回后再释放 buffer 才安全 */
        lvgl_video_exit_drain(g_vdec_video_handle);
        g_vdec_video_handle = NULL;
    }
#endif
    log_info("video_dec_ui_deinit()\n");
}

void video_dec_ui_pause_video_fetch(void)
{
    log_info("video_dec_ui_pause_video_fetch()\n");
}

void video_dec_ui_close_video(void)
{
    /* 切换文件时不销毁fb(否则下一张又要重建, 反复建销易泄漏); fb 由 deinit 统一释放。 */
    log_info("video_dec_ui_close_video()\n");
}

void video_dec_ui_on_state_change(enum video_dec_state old_state,
                                  enum video_dec_state new_state)
{
    /* 播放/列表切换不销毁fb, 由 deinit 统一释放 */
    log_info("video_dec_ui_on_state_change: %d -> %d\n", old_state, new_state);
}

#if CONFIG_LVGL_UI_ENABLE
static char g_vdec_title[64];
static int g_vdec_count;
static int g_vdec_focus;

static void video_dec_ui_refresh_grid_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    char index_text[16];

    if (!ui_scr) {
        return;
    }
    lv_label_set_text(ui_scr->video_dec_lbl_2, g_vdec_title);
    if (g_vdec_count > 0 && g_vdec_focus >= 0) {
        snprintf(index_text, sizeof(index_text), "%d/%d", g_vdec_focus + 1, g_vdec_count);
    } else {
        snprintf(index_text, sizeof(index_text), "0/0");
    }
    lv_label_set_text(ui_scr->video_dec_lbl_1, index_text);
}
#endif

void video_dec_ui_refresh_grid(const char *focus_name, int count, int focus)
{
    const char *name = focus_name ? focus_name : "No file";

    log_info("video_dec_ui_refresh_grid: count=%d, focus=%d, name=%s\n", count, focus, name);
#if CONFIG_LVGL_UI_ENABLE
    snprintf(g_vdec_title, sizeof(g_vdec_title), "%s", name);
    g_vdec_count = count;
    g_vdec_focus = focus;
    lvgl_rpc_post_func((void *)video_dec_ui_refresh_grid_rpc, 0);
#else
    (void)focus_name;
#endif
}

void video_dec_ui_show_photo(const char *path)
{
    /* fb 已在 video_dec_ui_init 建好, 此处不再重复 lvgl_video_init(否则fb泄漏白屏) */
    log_info("video_dec_ui_show_photo: %s\n", path ? path : "NULL");
#if CONFIG_LVGL_UI_ENABLE
    /* 显示照片前确保 img_obj 可见 (删除清屏后可能被隐藏) */
    g_vdec_disp_show = 1;
    lvgl_rpc_post_func((void *)vdec_video_disp_show_rpc, 0);
#endif
}

void video_dec_ui_show_video(const char *path)
{
    /* fb 已在 video_dec_ui_init 建好, 此处不再重复 lvgl_video_init(否则fb泄漏白屏) */
    log_info("video_dec_ui_show_video: %s\n", path ? path : "NULL");
#if CONFIG_LVGL_UI_ENABLE
    /* 显示视频画面前确保 img_obj 可见 (删除清屏后可能被隐藏) */
    g_vdec_disp_show = 1;
    lvgl_rpc_post_func((void *)vdec_video_disp_show_rpc, 0);
#endif
}

void video_dec_ui_clear_video(void)
{
    /* 删除所有后隐藏 img_obj, 避免最后一帧残留 */
    log_info("video_dec_ui_clear_video()\n");
#if CONFIG_LVGL_UI_ENABLE
    g_vdec_disp_show = 0;
    lvgl_rpc_post_func((void *)vdec_video_disp_show_rpc, 0);
#endif
}

void video_dec_ui_update_time(u32 elapsed_sec)
{
    log_info("video_dec_ui_update_time: %u sec\n", elapsed_sec);
}

#if CONFIG_LVGL_UI_ENABLE
/* 回放当前/总时长覆盖标签: 事件经 lvgl_rpc_post_func 到 ui 线程操作 LVGL(线程安全) */
static lv_obj_t *s_vdec_time_lbl = NULL;
static u32 s_vdec_cur_sec = 0;
static u32 s_vdec_total_sec = 0;
static void vdec_time_update_rpc(void *unused, ...)
{
    (void)unused;
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    if (!ui_scr || !ui_scr->video_dec) {
        return;
    }
    if (!s_vdec_time_lbl || !lv_obj_is_valid(s_vdec_time_lbl)) {
        s_vdec_time_lbl = lv_label_create(ui_scr->video_dec);
        lv_obj_set_pos(s_vdec_time_lbl, 6, 6);
        lv_obj_set_style_text_color(s_vdec_time_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(s_vdec_time_lbl, &lv_font_FangZhengKaiTiJianTi_1_20, 0);
        lv_obj_set_style_bg_color(s_vdec_time_lbl, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_vdec_time_lbl, LV_OPA_50, 0);
        lv_obj_set_style_pad_all(s_vdec_time_lbl, 2, 0);
    }
    lv_label_set_text_fmt(s_vdec_time_lbl, "%02u:%02u/%02u:%02u",
                          (unsigned)s_vdec_cur_sec / 60u, (unsigned)s_vdec_cur_sec % 60u,
                          (unsigned)s_vdec_total_sec / 60u, (unsigned)s_vdec_total_sec % 60u);
    lv_obj_clear_flag(s_vdec_time_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_vdec_time_lbl);
}
static void vdec_time_hide_rpc(void *unused, ...)
{
    (void)unused;
    if (s_vdec_time_lbl && lv_obj_is_valid(s_vdec_time_lbl)) {
        lv_obj_add_flag(s_vdec_time_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}
#endif

void video_dec_ui_update_progress(u32 current_sec, u32 total_sec)
{
#if CONFIG_LVGL_UI_ENABLE
    s_vdec_cur_sec = current_sec;
    s_vdec_total_sec = total_sec;
    lvgl_rpc_post_func((void *)vdec_time_update_rpc, 0);
#else
    log_info("video_dec_ui_update_progress: %u/%u\n", current_sec, total_sec);
#endif
}

void video_dec_ui_time_hide(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)vdec_time_hide_rpc, 0);
#endif
}

void video_dec_ui_update_storage(video_dec_storage_t *info)
{
    if (info)
        log_info("video_dec_ui_update_storage: free=%uKB total=%uKB\n",
                 info->free_kb, info->total_kb);
}

void video_dec_ui_confirm_delete(const char *path)
{
    log_info("video_dec_ui_confirm_delete: %s\n", path ? path : "NULL");
}

#if CONFIG_LVGL_UI_ENABLE
static void video_dec_ui_back_to_home_rpc(void *unused, ...)
{
    (void)unused;
    ui_scr_stack_pop_anim(&guider_ui, LV_SCR_LOAD_ANIM_NONE, 100, 100, false, false, false);
}
#endif

void video_dec_ui_back_to_home(void)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)video_dec_ui_back_to_home_rpc, 0);
#endif
}

static lv_obj_t *s_vdec_alert = NULL;
static lv_timer_t *s_vdec_alert_timer = NULL;

static void vdec_alert_close_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_vdec_alert && lv_obj_is_valid(s_vdec_alert)) {
        lv_obj_delete(s_vdec_alert);
    }
    s_vdec_alert = NULL;
    if (s_vdec_alert_timer) {
        lv_timer_delete(s_vdec_alert_timer);
        s_vdec_alert_timer = NULL;
    }
}

static void vdec_alert_rpc(int type)
{
#if CONFIG_LVGL_UI_ENABLE
    lv_ui_video_dec *ui_scr = guider_ui.video_dec;
    const char *txt;
    if (!ui_scr || !ui_scr->video_dec || !lv_obj_is_valid(ui_scr->video_dec)) {
        return;
    }
    switch (type) {
    case 1:
        txt = "请插入SD卡";
        break;
    default:
        /* 原为"系统错误": "错/误"不在 LVGL 字体子集内(子集只含生成 UI 出现过的字),
         * 真机表现为红底白框无字。"文件损坏"同样不可用("损"零命中, "坏"仅出现在注释里),
         * 故用"文件打开失败"——六字均可在生成 UI 串里找到出处 */
        txt = "文件打开失败";
        break;
    }
    if (s_vdec_alert && lv_obj_is_valid(s_vdec_alert)) {
        lv_obj_delete(s_vdec_alert);
    }
    s_vdec_alert = lv_label_create(ui_scr->video_dec);
    lv_label_set_text(s_vdec_alert, txt);
    lv_obj_set_style_text_color(s_vdec_alert, lv_color_hex(0xFFFFFF), 0);
    /* 用回放屏其它 label 同款字体(1_18): 1_20 的字形子集与生成 UI 不一致, 会出白框 */
    lv_obj_set_style_text_font(s_vdec_alert, &lv_font_FangZhengKaiTiJianTi_1_18, 0);
    lv_obj_set_style_bg_color(s_vdec_alert, lv_color_hex(0xCC0000), 0);
    lv_obj_set_style_bg_opa(s_vdec_alert, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(s_vdec_alert, 12, 0);
    lv_obj_set_style_radius(s_vdec_alert, 8, 0);
    lv_obj_center(s_vdec_alert);
    lv_obj_move_foreground(s_vdec_alert);
    if (s_vdec_alert_timer) {
        lv_timer_delete(s_vdec_alert_timer);
    }
    s_vdec_alert_timer = lv_timer_create(vdec_alert_close_cb, 2000, NULL);
#endif
}

void video_dec_ui_show_alert(int type)
{
#if CONFIG_LVGL_UI_ENABLE
    lvgl_rpc_post_func((void *)vdec_alert_rpc, 1, (int)type);
#else
    (void)type;
#endif
}

#endif /* #if TCFG_APP_PHOTO_EN */

