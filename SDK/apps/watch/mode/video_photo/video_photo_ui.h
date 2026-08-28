/**
 * @file    video_photo_ui.h
 * @brief   拍照 UI 层接口声明
 *
 * 本文件声明拍照模式所有 UI 回调函数。UI 实现由 LVGL_v9 代码生成工具
 * 根据此接口自动生成 video_photo_ui.c。
 *
 * ## 依赖关系
 *
 *   video_photo.c (Coder1 核心逻辑)
 *       │
 *       ├── 调用 → photo_ui_on_state_change()   状态变化通知
 *       ├── 调用 → photo_ui_update_zoom()       变焦倍率变化
 *       ├── 调用 → photo_ui_update_countdown()  倒计时每秒更新
 *       ├── 调用 → photo_ui_trigger_effect()    拍照瞬间特效
 *       └── 调用 → photo_ui_show_alert()        异常弹窗
 *
 * ## 设计原则
 *
 *   - UI 层不访问摄像头/MPI/文件系统
 *   - 所有数据由 Coder1 通过参数传入
 *   - 屏幕 320×386，LVGL 对象叠加在摄像头预览层之上
 *   - 模式栏底部固定，点击按钮 → 调用 photo_mode_set() 通知 Coder1
 *
 * ## 待生成文件
 *
 *   mode/video_photo/video_photo_ui.c   — LVGL_v9 代码生成工具输出
 *
 * @author  Coder2 (接口定义)
 * @date    2026-05-28
 */

#ifndef CONFIG_VIDEO_PHOTO_UI_H
#define CONFIG_VIDEO_PHOTO_UI_H

#include "system/includes.h"
#include "video_photo.h"

/*==============================================================================
 * UI 初始化 / 清理
 *
 * 调用时机:
 *   init:   photo_init() 末尾，摄像头预览启动之后
 *   deinit: photo_exit() 开头，释放摄像头之前
 *
 * 实现要点:
 *   init   — 创建透明 overlay 容器，包含以下子对象:
 *            1. 变焦标签 (右上角)
 *            2. 画质警告标签 (顶部居中, 默认隐藏)
 *            3. 倒计时大字标签 (屏幕居中, 默认隐藏)
 *            4. 闪光覆盖层 (全屏白色, 默认隐藏)
 *            5. 缩略图预览 (右下角, 默认隐藏)
 *            6. 模式选择栏 (底部, 3个按钮)
 *   deinit — 删除所有对象和定时器
 *==============================================================================
 */
void photo_ui_init(int width, int height, int x, int y, int show_video);
void photo_ui_deinit(void);
/* 摄像头预览启动后调用: 强制 LVGL 刷新/显示视频对象, 避免首帧未触发渲染导致白屏/背景覆盖 */
void photo_ui_video_refresh(void);

/*==============================================================================
 * 状态变化通知
 *
 * 调用时机:
 *   video_photo.c 状态机每次完成状态切换后调用
 *
 * @param old_state  旧状态 (enum photo_state)
 * @param new_state  新状态 (enum photo_state)
 *
 * 实现要求:
 *
 *   PHOTO_STATE_BUSY:
 *     - 调用 photo_ui_trigger_effect(1, NULL) 触发闪光动画
 *     - （快门音效由 Coder1 控制音频子系统，UI 层不处理）
 *
 *   PHOTO_STATE_COUNTDOWN:
 *     - 显示倒计时大字标签 (lv_obj_clear_flag HIDDEN)
 *     - 具体数字由 photo_ui_update_countdown() 更新
 *
 *   PHOTO_STATE_BURST:
 *     - 显示连拍计数标签 (格式: "BURST N")
 *     - 具体数字由 Coder1 在 photo_burst_tick() 中通过
 *       photo_ui_on_state_change 间接触发更新
 *
 *   PHOTO_STATE_IDLE:
 *     - 隐藏倒计时/连拍标签
 *     - 刷新模式栏高亮（调用 photo_mode_get() 获取当前模式）
 *==============================================================================
 */
void photo_ui_on_state_change(enum photo_state old_state,
                              enum photo_state new_state);

/*==============================================================================
 * 变焦倍率更新
 *
 * 调用时机:
 *   Coder1 每次变焦后调用（按键 +/- 变焦、photo_zoom_set 后）
 *
 * @param level  当前变焦倍率 (1.0f ~ 4.0f)
 *
 * 实现要求:
 *   1. 右上角标签更新为 " %.1fx " — 注意前后空格，1位小数
 *      例: 1.0→" 1.0x ", 2.5→" 2.5x ", 4.0→" 4.0x "
 *      标签尺寸: 80×24, 半透明深灰底 (bg_opa=180), 白字
 *
 *   2. 画质警告:
 *      - level > PHOTO_ZOOM_QUALITY_WARN (3.0f) → 显示黄色警告
 *        标签内容: "画质降低"
 *        样式: 黄色文字(#FFFF00), 红色背景(#FF0000, opa=180), 顶部居中
 *      - level ≤ 3.0f → 隐藏警告标签
 *
 *   注意:
 *      使用 lv_label_set_text_fmt() 格式化
 *      格式字符串: " %.1fx "
 *==============================================================================
 */
void photo_ui_update_zoom(float level);
void photo_ui_update_battery(void);

/*==============================================================================
 * 倒计时更新
 *
 * 调用时机:
 *   Coder1 在 photo_timer_tick() 中每秒调用一次
 *
 * @param seconds  剩余秒数 (int)
 *   > 0: 显示数字 (例: 3, 2, 1)
 *   = 0: 倒计时结束, 隐藏标签
 *
 * 实现要求:
 *   - 大字居中显示，推荐字体: lv_font_montserrat_48 或更大
 *   - 使用 lv_label_set_text_fmt(label, "%d", seconds)
 *   - seconds <= 0 时 lv_obj_add_flag(HIDDEN)
 *   - 居中: lv_obj_center(label)
 *==============================================================================
 */
void photo_ui_update_countdown(int seconds);

/*==============================================================================
 * 拍照特效触发
 *
 * 调用时机:
 *   Coder1 在拍照完成后调用，提供视觉反馈
 *
 * @param effect  特效类型
 *   0 = 快门音效 (UI: 屏幕闪黑)
 *   1 = 闪光动画 (UI: 全屏白色闪烁)
 *   2 = 缩略图预览 (UI: 右下角显示小图)
 * @param path    文件路径 — 仅 effect=2 时有效，其他传 NULL
 *
 * 实现要求:
 *
 *   effect=0 (快门闪黑):
 *     - lv_scr_act() 背景设为黑色 (bg_opa=255)
 *     - lv_anim: bg_opa 255→0, 150ms, ease_out
 *     - 动画执行回调: _flash_anim_cb 设置 bg_opa
 *
 *   effect=1 (闪光动画):
 *     - 显示 g_flash_overlay (全屏白色, 320×386)
 *     - lv_anim: bg_opa 255→0, 200ms, ease_out
 *     - 完成回调: lv_obj_add_flag(HIDDEN)
 *
 *   effect=2 (缩略图预览):
 *     - lv_img_set_src(g_thumbnail_img, path) 加载缩略图
 *     - 显示在右下角 (尺寸建议: 96×72, bottom_right align, offset -8,-48)
 *     - lv_timer_create(1500ms) → 自动隐藏
 *     - 缩略图加载失败时分优雅降级（不崩溃）
 *==============================================================================
 */
void photo_ui_trigger_effect(int effect, const char *path);
const char *photo_ui_get_sticker_path(void);
const char *photo_ui_get_sticker_path_by_idx(uint8_t idx);
const char *photo_ui_get_sticker_save_path_by_idx(uint8_t idx, uint16_t width, uint16_t height);

/*==============================================================================
 * 警告弹窗
 *
 * 调用时机:
 *   Coder1 检测到异常时调用
 *
 * @param type  弹窗类型 (int)
 *   0 = "存储已满"    — SD卡空间不足
 *   1 = "请插入SD卡"  — SD卡未插入或拔出
 *   2 = "拍照失败"    — 编码失败或摄像头异常
 *
 * 实现要求:
 *   - 弹窗样式与 video_rec_ui_show_alert 一致
 *   - 半透明遮罩 + 深灰圆角弹窗(220×110) + 确定按钮
 *   - 点击确定 → 删除弹窗
 *   - 重复调用时先删除旧弹窗
 *==============================================================================
 */
void photo_ui_show_alert(int type);

/*==============================================================================
 * 模式选择栏 — 底部固定交互
 *
 * 布局:
 *   g_mode_bar (lv_obj) — 320×40, 底部, 半透明深灰(bg_opa=200)
 *   ├── btn[0] (lv_btn) — "单拍",  106×36, 左侧
 *   ├── btn[1] (lv_btn) — "定时",  106×36, 中间
 *   └── btn[2] (lv_btn) — "连拍",  106×36, 右侧
 *
 * 交互:
 *   点击 btn[i] → 调用 photo_mode_set(i) 通知 Coder1 切换模式
 *   Coder1 切换完成后 → photo_ui_on_state_change → 刷新高亮
 *
 * 高亮逻辑:
 *   调用 photo_mode_get() 获取当前模式:
 *     - 模式 0 (SINGLE)           → btn[0] 绿色, 其他深灰
 *     - 模式 1-3 (TIMER_3S/5S/10S) → btn[1] 绿色, 其他深灰
 *     - 模式 4 (BURST)            → btn[2] 绿色, 其他深灰
 *
 *   颜色:
 *     选中(高亮): #00CC00 (绿色)
 *     未选中:     #222222 (深灰)
 *
 *   注意: 定时模式的3个变体(TIMER_3S/5S/10S)映射到同一个"定时"按钮。
 *         按下"定时"按钮后可能需要子菜单选择 3/5/10 秒，
 *         或循环切换 (P0 简化: 默认3秒，通过长按切换时长)。
 *==============================================================================
 */

/*==============================================================================
 * LVGL 对象结构建议 (供代码生成工具参考)
 *
 *   lv_scr_act()                            ← 当前活动屏幕
 *   └── g_overlay (lv_obj)                  ← 透明容器, 320×386, clickable
 *       │
 *       ├── g_zoom_label (lv_label)         ← " 1.0x ", 右上角(232,8), 80×24
 *       │                                      半透明深灰底, 白字
 *       │
 *       ├── g_warn_label (lv_label)         ← "画质降低", 顶部居中, 120×20
 *       │                                      黄字(#FFFF00)红底(#FF0000), HIDDEN
 *       │
 *       ├── g_countdown_label (lv_label)    ← 倒计时数字, 屏幕居中
 *       │                                      48号字体, 白色, HIDDEN
 *       │
 *       ├── g_flash_overlay (lv_obj)        ← 全屏白色, 320×386, HIDDEN
 *       │                                      bg_opa 由 lv_anim 控制
 *       │
 *       ├── g_thumbnail_img (lv_img)        ← 缩略图预览, 右下角 96×72, HIDDEN
 *       │
 *       ├── g_alert_mask (lv_obj)           ← 弹窗遮罩, 动态创建/销毁
 *       │   └── g_alert_box (lv_obj)        ← 弹窗主体 220×110
 *       │
 *       └── g_mode_bar (lv_obj)             ← 底部模式栏, 320×40
 *           ├── g_mode_btns[0] "单拍"
 *           ├── g_mode_btns[1] "定时"
 *           └── g_mode_btns[2] "连拍"
 *
 *   定时器:
 *     g_thumbnail_timer (lv_timer)          ← 1500ms 一次性, 隐藏缩略图
 *
 *   颜色常量:
 *     #define COLOR_YELLOW    lv_color_hex(0xFFFF00)
 *     #define COLOR_RED       lv_color_hex(0xFF0000)
 *     #define COLOR_WHITE     lv_color_hex(0xFFFFFF)
 *     #define COLOR_BLACK     lv_color_hex(0x000000)
 *     #define COLOR_GREY_DARK lv_color_hex(0x222222)
 *     #define COLOR_GREEN     lv_color_hex(0x00CC00)
 *==============================================================================
 */

void video_photo_ui_back_to_home(void);

#endif /* CONFIG_VIDEO_PHOTO_UI_H */
