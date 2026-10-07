/* 双备份升级专用“升级进度页”驱动。
 *
 * 页面本体已按 GUI-Guider 工程方式同步进主 generated 树：
 *   generated/gui_scr/setup_scr_upgrade_page.c (内置 simsun 字库 + 进度条 lv_bar)
 * 本文件只负责在升级(写 flash)期间把显示切到该页，并驱动其进度条/百分比/提示文字。
 *
 * 升级期间如何保证“其它仪表盘页面不再访问 flash 字库/图片”：
 *   - LVGL 同一时刻只绘制 activity screen；切到本升级页后其余页面不再被绘制，
 *     因此不会解码 flash 图片/取字形(图片懒加载、不驻留)；
 *   - 仪表盘 MiSans 字库是 NOR-flash 内存映射(lv_font_load_bin→lv_get_flash_src_addr)，
 *     字形数据一直在 flash 里、画时才读。这里【刻意不调】free_gui_fonts/init_gui_fonts：
 *       * free_gui_fonts 只释放 RAM 描述符，释放不掉 flash 里的字形，帮助有限；
 *       * init_gui_fonts 会在退出时对 12 个字库做一轮大块 flash 重读——这正是二次升级
 *         死机的来源：升级窗口内又发起 flash 读，与网络/WiFi 等并发访问 flash 的模块
 *         冲突 → xspi 异常(崩溃栈里出现 storage/res_nor_mode/C/lvgl/3500000b.rle)。
 *   - 只要保证“仅本升级页被绘制”，即已满足 0 次 flash 字形访问。
 *
 * 未删除其它页面对象的原因：本工程多页面存在无法安全停掉的 LVGL 周期定时器
 * (music 专辑旋转 / hardware_test 高亮等)，删除对象会令定时器访问已释放控件；
 * 而不激活的页面不会被绘制，保留对象不影响“升级窗口内 0 次 flash 资源访问”。
 */
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "lv_ui.h"
#endif
#include "lvgl.h"
#include "../generated/gui_guider.h"
#include "ui_update_page.h"

/* 开机是否直入升级页: HTTP预留区(UI资源)升级掉电中断(VM标记仍在)返回1; 其它构建返回0 */
#if defined(TCFG_HTTP_UPDATE_ENABLE) && TCFG_HTTP_UPDATE_ENABLE
int ui_update_page_should_boot_recover(void)
{
    extern int watch_http_ota_vm_get(void);   /* http_db_passive_update_demo.c 提供 */
    return watch_http_ota_vm_get();
}
#else
__attribute__((weak)) int ui_update_page_should_boot_recover(void)
{
    return 0;
}
#endif

#if !LV_USE_GUIBUILDER_SIMULATOR

#if TCFG_UPDATE_ENABLE
#include "update_loader_download.h"
#endif

/* custom/custom.c: 仪表盘后台自动翻屏定时器 暂停/恢复 */
extern void dashboard_ui_freeze_background(void);
extern void dashboard_ui_resume_background(void);

/* 提示文案须用内嵌字库已有字形(缺字会显示方框) */
#define UPD_TIP_DEFAULT         "正在升级，请勿断电"
#define UPD_TIP_SUCCESS         "升级成功！"
/* 资源升级掉电中断, 开机直入本页的提示 */
#define UPD_TIP_RES_INCOMPLETE  "升级失败"

static int  s_old_id = -1;                /* 进入升级页前的活动屏 id(退出时切回) */
static volatile bool s_entered = false;   /* 是否已在升级页 */
static bool s_tip_success = false;        /* 是否已显示“升级成功”(ui 任务内读写) */
static bool s_res_incomplete_tip = false; /* 标题为"升级失败", 等待重新升级(ui任务内读写) */

/* 设置升级页提示文案(调用点须在 "ui" 任务内；tip 字符串须保持有效)。 */
static void ui_update_page_set_tip_internal(const char *tip)
{
    lv_ui_upgrade_page *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_UPGRADE_PAGE);
    if (ui_scr == NULL || ui_scr->upgrade_page_lbl_upgrade_title == NULL) {
        return;
    }
    if (lv_obj_is_valid(ui_scr->upgrade_page_lbl_upgrade_title)) {
        lv_label_set_text(ui_scr->upgrade_page_lbl_upgrade_title, tip);
    }
}

/* 刷新升级页控件(必须运行在 "ui" 任务内)。页面控件经由 guider_ui 访问。 */
static void ui_update_page_refresh(int pct)
{
    if (pct < 0) {
        pct = 0;
    } else if (pct > 100) {
        pct = 100;
    }

    lv_ui_upgrade_page *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_UPGRADE_PAGE);
    if (ui_scr == NULL) {
        return;
    }
    if (ui_scr->upgrade_page_bar_upgrade != NULL &&
        lv_obj_is_valid(ui_scr->upgrade_page_bar_upgrade)) {
        lv_bar_set_value(ui_scr->upgrade_page_bar_upgrade, pct, LV_ANIM_OFF);
    }
    if (ui_scr->upgrade_page_lbl_upgrade_percent != NULL &&
        lv_obj_is_valid(ui_scr->upgrade_page_lbl_upgrade_percent)) {
        lv_label_set_text_fmt(ui_scr->upgrade_page_lbl_upgrade_percent, "%d%%", pct);
    }

    /* 升级完成：tip 更新为“升级成功！”(仅置一次) */
    if (pct >= 100 && !s_tip_success) {
        s_tip_success = true;
        ui_update_page_set_tip_internal(UPD_TIP_SUCCESS);
    }
}

/* ui 任务内执行：真正切页 */
static void ui_update_page_enter_on_ui(void *arg, ...)
{
    (void)arg;

    if (s_entered) {
        return;
    }

    gui_scr_t *act = gui_scr_get_act();
    s_old_id = (act != NULL) ? (int)gui_scr_get_id(act) : (int)GUI_SCREEN_PAGE_APP_SELECT;

    gui_scr_t *up = ui_get_scr(GUI_SCREEN_UPGRADE_PAGE);
    if (up == NULL) {
        s_old_id = -1;
        return;
    }

    /* 把升级页设为唯一 activity screen(auto_del=false 保留旧屏对象但不绘制)；
     * 首次进入会经 setup_scr_upgrade_page 建页(只含内置字库 + bar/label)。 */
    ui_load_scr_anim(&guider_ui, up, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
    s_entered = true;
    s_tip_success = false;
    s_res_incomplete_tip = false;

    /* 冻结仪表盘后台自动翻屏，避免其把 app_select 拉回来重绘 */
    dashboard_ui_freeze_background();

    ui_update_page_set_tip_internal(UPD_TIP_DEFAULT);  /* 恢复默认提示 */
    ui_update_page_refresh(0);                         /* 进度/百分比归零 */
}

/* ui 任务内执行：刷新进度(0~100) */
static void ui_update_page_progress_on_ui(void *arg, ...)
{
    int pct = (int)(intptr_t)arg;

    /* 掉电恢复开机: 重新升级一有进度, 标题由"升级失败"切回"正在升级" */
    if (s_res_incomplete_tip && pct > 0) {
        s_res_incomplete_tip = false;
        ui_update_page_set_tip_internal(UPD_TIP_DEFAULT);
    }

    ui_update_page_refresh(pct);
}

/* ui 任务内执行：更新 tip 文案 */
static void ui_update_page_tip_on_ui(void *arg, ...)
{
    ui_update_page_set_tip_internal((const char *)(intptr_t)arg);
}

/* ui 任务内执行：退出升级页，恢复原界面(升级失败/中止等仍需继续运行的场景) */
static void ui_update_page_exit_on_ui(void *arg, ...)
{
    (void)arg;
    if (!s_entered) {
        return;
    }

    int back_id = (s_old_id >= 0) ? s_old_id : (int)GUI_SCREEN_PAGE_APP_SELECT;
    s_old_id = -1;
    s_entered = false;
    s_tip_success = false;

    dashboard_ui_resume_background();

    /* 切回进入前的仪表盘屏；升级页随 LV 卸载事件被自动清掉(scr 置空), 可再次进入 */
    gui_scr_t *back = ui_get_scr(back_id);
    if (back == NULL) {
        back = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
    }
    if (back != NULL) {
        ui_load_scr_anim(&guider_ui, back, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
    }
}

/* ---------------- 线程安全对外接口 ---------------- */

void ui_update_page_enter(void)
{
    if (s_entered) {
        return;
    }
    lvgl_rpc_post_func(ui_update_page_enter_on_ui, 0);
}

void ui_update_page_set_percent(int pct)
{
    /* 首包进度可能先于 enter 的 rpc 到达: 确保页面已进入(enter 幂等) */
    if (!s_entered) {
        ui_update_page_enter();
    }
    lvgl_rpc_post_func(ui_update_page_progress_on_ui, 1, pct);
}

void ui_update_page_set_tip(const char *tip)
{
    if (!s_entered) {
        return;
    }
    lvgl_rpc_post_func(ui_update_page_tip_on_ui, 1, (int)(intptr_t)tip);
}

void ui_update_page_exit(void)
{
    if (!s_entered) {
        return;
    }
    lvgl_rpc_post_func(ui_update_page_exit_on_ui, 0);
}

int ui_update_page_is_active(void)
{
    return s_entered ? 1 : 0;
}

#if TCFG_UPDATE_ENABLE
/* 双备份升级进度回调(update.a 在推送/写 flash 过程中调用，所在线程不定)：
 * 第一次被回调就切到升级页，之后每次回调刷新百分比；到 100 自动置“升级成功！”。 */
static void ui_update_percent_info_cb(update_percent_info *info)
{
    if (info == NULL) {
        return;
    }
    ui_update_page_enter();
    ui_update_page_set_percent((int)info->percent);
}
#endif

void ui_update_page_init(void)
{
#if TCFG_UPDATE_ENABLE
    /* 该进度信息接口注释明确：只有双备份升级可以获取(见 update_loader_download.h) */
    register_update_percent_info_callback_handle(ui_update_percent_info_cb);
#endif
}

/* 开机直入升级页(UI资源升级未完成时由 jl_gui_init 调用, 须在 ui 任务内):
 * 跳过 flash 资源访问, 只建内置字库升级页显示"升级失败", 等重新OTA刷新。 */
void ui_update_page_boot_upgrade_only(void)
{
#if TCFG_HTTP_UPDATE_ENABLE
    gui_scr_t *up = ui_get_scr(GUI_SCREEN_UPGRADE_PAGE);
    if (up == NULL) {
        return;
    }

    /* 懒构建升级页(仅内置字库+bar/label, 0次flash访问)并设为唯一活动屏 */
    ui_load_scr_anim(&guider_ui, up, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);

    s_entered = true;                 /* 后续 set_percent()/进度回调不再重复进入或重置tip */
    s_tip_success = false;
    s_res_incomplete_tip = true;      /* 重新升级一有进度, 标题切回"正在升级" */
    s_old_id = GUI_SCREEN_PAGE_APP_SELECT;

    ui_update_page_set_tip_internal(UPD_TIP_RES_INCOMPLETE);
    ui_update_page_refresh(0);

    /* 本分支不走 custom_init, 需自行注册升级进度回调, 便于重新OTA刷新本页 */
    ui_update_page_init();

    /* 停掉后台翻屏/动画(本分支尚未建屏, 通常为空操作; 显式调用保证不变量) */
    dashboard_ui_freeze_background();
#endif
}

#endif /* !LV_USE_GUIBUILDER_SIMULATOR */
