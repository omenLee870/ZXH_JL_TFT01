#ifndef __UI_UPDATE_PAGE_H__
#define __UI_UPDATE_PAGE_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 双备份升级专用“升级进度页”驱动。
 * 页面本体在 generated/gui_scr/setup_scr_upgrade_page.c(内置 simsun 字库 + 进度条)。
 *
 * 升级(写 flash)期间如何保证“不再访问 flash 字库/图片”：
 *   - LVGL 同一时刻只绘制 activity screen。切到本升级页后，其余仪表盘页面不再被
 *     绘制，也就不会去解码 flash 图片/取字形(图片是懒加载、不驻留)；
 *   - 注意: 仪表盘 MiSans 字库是 NOR-flash 内存映射(lv_font_load_bin→lv_get_flash_src_addr)，
 *     字形数据本就不在 RAM 里，画字时才从 flash 读。因此这里【刻意不去】调
 *     free_gui_fonts()/init_gui_fonts()：free 释放不了 flash 里的字形，init 反而会在
 *     “升级期间”产生一轮大块 flash 重读，与网络/WiFi 等并发访问 flash 的模块冲突
 *     (xspi 异常、二次升级死机)。只要保证只画本页(内置字库在 RAM/代码里)，即为安全。
 *
 * 用法(线程安全，内部切到 "ui" 任务再操作 LVGL)：
 *   - boot 时调用一次 ui_update_page_init()：注册双备份升级进度回调，
 *     收到写 flash 的进度即自动切入本页并刷新；
 *   - OTA 入口也可直接 ui_update_page_enter()；
 *   - 进度刷新 ui_update_page_set_percent()；进度到 100 会自动把标题 tip 置为
 *     “升级成功！”(见 ui_update_page_set_tip)；
 *   - 升级失败/中止需要回到仪表盘时调 ui_update_page_exit()。
 *
 * 注意：
 *   - 内置字库 lv_font_simsun_32 只含 GUI-Guider 导出时的汉字(“正在升级请勿断电”等)
 *     与 ASCII；若“升级成功！”等新文案里的字缺失(显示空白/方框)，需在 GUI-Guider
 *     里给升级页加对应文本后重新导出字库，替换 generated/gui_fonts/lv_font_simsun_32.c。
 */

void ui_update_page_init(void);          /* 注册双备份升级进度回调(boot 调一次) */
void ui_update_page_enter(void);         /* 切到升级页(任意线程) */
void ui_update_page_set_percent(int pct);/* 刷新进度条+百分比 0~100(任意线程) */
void ui_update_page_set_tip(const char *tip); /* 更新提示文案(任意线程, 字符串须保持有效) */
void ui_update_page_exit(void);          /* 退出升级页、恢复原界面(任意线程) */
int  ui_update_page_is_active(void);     /* 当前是否处于升级页(供按键/切屏拦截判断) */
int  ui_update_page_should_boot_recover(void);/* 上次HTTP预留区(UI资源)升级未完成则返回1(开机判定) */
void ui_update_page_boot_upgrade_only(void);/* UI资源升级未完成时, 开机直入升级页提示重新升级(jl_gui_init内, ui任务) */

#ifdef __cplusplus
}
#endif

#endif /* __UI_UPDATE_PAGE_H__ */
