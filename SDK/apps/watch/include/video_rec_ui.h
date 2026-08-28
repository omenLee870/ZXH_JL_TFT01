/**
 * @file    video_rec_ui.h
 * @brief   录像 UI 层接口声明
 *
 * 本文件声明录像模式所有 UI 回调函数。UI 实现由 LVGL_v9 代码生成工具
 * 根据此接口自动生成 video_rec_ui.c。
 *
 * ## 依赖关系
 *
 *   video_rec.c (Coder1 核心逻辑)
 *       │
 *       ├── 调用 → video_rec_ui_on_state_change()  状态变化时通知UI
 *       ├── 调用 → video_rec_ui_update_timer()     每500ms更新计时
 *       └── 调用 → video_rec_ui_show_alert()       异常/完成弹窗
 *
 * ## 预览层架构
 *
 *   屏幕采用双层结构：底层为摄像头实时预览(VO通道0直接输出到 LCD),
 *   上层为 LVGL 渲染的 UI 覆盖层(透明 overlay)。
 *
 *     ┌─────────────────────────────────────┐  ← LCD 物理层 320×386
 *     │  LVGL overlay (透明, alpha=0)       │  ← 第 2 层 (UI 控件)
 *     │  ┌─────────────────────────────┐    │
 *     │  │   红点 + 计时 + 进度条      │    │  ← 仅这些区域不透明
 *     │  └─────────────────────────────┘    │
 *     ├─────────────────────────────────────┤
 *     │  Camera Preview (VO Ch0, YUV)       │  ← 第 1 层 (摄像头实时数据)
 *     │  持续输出, 不被 UI 阻塞             │
 *     └─────────────────────────────────────┘
 *
 *   关键约束:
 *     - UI 层只创建透明对象, 不要设置全屏 bg_color, 否则会遮挡预览
 *     - overlay 必须设 LV_OBJ_FLAG_CLICKABLE 阻止触摸穿透到下层预览
 *       (虽然预览层不响应触摸, 但防止事件冒泡误触)
 *     - 弹窗(modal)需要半透明遮罩(192×386, bg_opa=128), 此时预览被
 *       暂时遮挡, 弹窗关闭后自动恢复显示
 *     - 红点/计时/进度条的位置不能盖住屏幕中心预览主体区域,
 *       推荐: 红点+计时放顶部状态栏(8,8), 进度条放底部(60,360)
 *
 *   分辨率适配:
 *     预览源是 640×480 (4:3), LCD 是 320×386 (~5:6 竖屏)
 *     VPSS 缩放/裁剪由 video_rec.c 的 display_attr 配置,
 *     UI 不需要关心缩放, 只需在 320×386 坐标系内定位
 *
 * ## 设计原则
 *
 *   - UI 层不访问 MPI / 摄像头 / 文件系统
 *   - 所有数据由 Coder1 通过参数传入
 *   - 屏幕 320×386，LVGL 对象叠加在摄像头预览层之上
 *
 * ## 待生成文件
 *
 *   mode/video_rec/video_rec_ui.c   — LVGL_v9 代码生成工具输出
 *
 * @author  Coder2 (接口定义)
 * @date    2026-05-28
 * @update  2026-06-02 补充预览层架构 + display_attr 接口说明
 */

#ifndef CONFIG_VIDEO_REC_UI_H
#define CONFIG_VIDEO_REC_UI_H

#include "system/includes.h"
#include "video_rec.h"

/*==============================================================================
 * 预览显示参数 (display_attr) 接口说明
 *
 * video_rec.c 在初始化摄像头预览通道时, 通过此结构体向底层 (apps/common/video
 * 封装层) 描述"预览画面如何显示在 LCD 上"。UI 层不直接使用此结构, 但需要
 * 了解这些参数决定了预览的最终呈现效果, 便于 UI 控件在 LCD 上的定位。
 *
 * ## 典型用法 (供 Coder1 参考, UI 不调用)
 *
 *     app_video_rec_display_attr_t attr = {
 *         .fb_name    = "fb5",        // LCD framebuffer 设备名
 *         .fb_num     = 0,            // 多缓冲序号(0/1, 双缓冲)
 *         .width      = 320,          // 显示宽度(像素)
 *         .height     = 386,          // 显示高度(像素)
 *         .left       = 0,            // 屏幕左上 X 偏移
 *         .top        = 0,            // 屏幕左上 Y 偏移
 *         .rotate     = 0,            // 旋转角度 0/90/180/270
 *         .mirror     = 0,            // 镜像 0=正常, 1=水平, 2=垂直, 3=水平+垂直
 *     };
 *     app_video_rec_open(&handle, &attr);
 *
 * ## 字段语义
 *
 *   fb_name (const char *)
 *     LCD framebuffer 设备节点名, 杰理 SDK 通常为 "fb0"~"fb5"。
 *     kid_camera 项目用 fb5 (主显示), 由 board panel 配置决定。
 *
 *   fb_num (int)
 *     framebuffer 缓冲数量。
 *     0 = 单缓冲 (无撕裂保护, 性能高)
 *     1 = 双缓冲 (推荐, 避免预览画面撕裂)
 *
 *   width / height (int)
 *     预览在 LCD 上的最终显示尺寸 (像素)。
 *     与源分辨率 (640×480) 不同时由 VPSS 做缩放。
 *     通常等于 LCD 物理分辨率 320×386, 填满全屏。
 *
 *   left / top (int)
 *     预览窗口在 LCD 上的左上角坐标 (像素)。
 *     填 0,0 即全屏显示; 非零可实现画中画 (P2 暂不支持)。
 *
 *   rotate (int)
 *     旋转角度 (度): 0 / 90 / 180 / 270。
 *     kid_camera 摄像头模组默认方向 0, 视外壳装配调整。
 *     旋转后 width/height 需对调。
 *
 *   mirror (int)
 *     镜像模式:
 *       0 = 正常
 *       1 = 水平镜像 (左右翻转, 类似前置摄像头自拍)
 *       2 = 垂直镜像 (上下翻转)
 *       3 = 水平+垂直 (180° 旋转等价)
 *
 * ## 与 UI 层的协作
 *
 *   - UI 控件 (红点/计时/进度条) 始终使用 LCD 物理坐标系 320×386 定位,
 *     不受 rotate/mirror 影响 (旋转由 VPSS 在底层完成, LVGL 看到的是
 *     旋转后的逻辑屏幕)
 *   - 若 rotate=90/270, LVGL 的屏幕尺寸需在 lvgl_init 时配置为
 *     (height, width) = (386, 320), 由 Coder1 统一处理, UI 不感知
 *   - 弹窗遮罩 (192×386) 在旋转后需同步旋转, 由代码生成工具根据
 *     当前 display_attr 自动生成对应坐标
 *
 * @note
 *   此结构体与 apps/common/video/video_rec.h 中的 app_video_rec_display_attr_t
 *   保持一致 (P1 阶段由 video_rec.c 内部定义, 此处仅作 UI 文档参考)。
 *==============================================================================
 */

/*==============================================================================
 * UI 初始化 / 清理
 *
 * 调用时机:
 *   init:  video_rec_init() 末尾，在启动摄像头预览之后
 *   deinit: video_rec_exit() 开头，在释放摄像头之前
 *
 * 实现要点:
 *   init   — 在 lv_scr_act() 上创建透明 overlay 容器，
 *            包含红点、计时标签、进度条三个子对象，全部默认隐藏。
 *            overlay 需设置 LV_OBJ_FLAG_CLICKABLE 阻止点击穿透到预览层。
 *   deinit — 停止所有动画定时器，删除 overlay 及其子对象，指针置 NULL。
 *==============================================================================
 */
void video_rec_ui_init(int width, int height, int show_video);
void video_rec_ui_deinit(void);

/*==============================================================================
 * 状态变化通知
 *
 * 调用时机:
 *   video_rec.c 状态机每次完成状态切换后立即调用。
 *   例如: IDLE→RECORDING, RECORDING→SAVING, SAVING→IDLE, *→ERROR
 *
 * @param old_state  切换前的状态 (enum video_rec_state)
 * @param new_state  切换后的状态 (enum video_rec_state)
 *
 * 实现要求 — 按 new_state 分支:
 *
 *   VIDEO_STATE_RECORDING:
 *     - 显示红点对象（圆形, 直径12px, 红色 #FF0000）
 *     - 显示计时标签 "00:00"（紧挨红点右侧, 白色 #FFFFFF）
 *     - 启动 500ms 周期闪烁定时器(lv_timer) —
 *       回调中切换红点的 LV_OBJ_FLAG_HIDDEN 实现闪烁
 *
 *   VIDEO_STATE_SAVING:
 *     - 停止红点闪烁定时器
 *     - 隐藏红点和计时标签
 *     - 显示保存进度条（居中, 200x16, lv_bar, 范围0-100）
 *       进度条指示色: 红色 #FF0000
 *
 *   VIDEO_STATE_IDLE (从任何状态进入):
 *     - 停止红点闪烁定时器
 *     - 隐藏红点、计时标签、进度条
 *     - 恢复纯预览状态
 *
 *   VIDEO_STATE_ERROR:
 *     - 停止红点闪烁定时器，隐藏所有覆盖元素
 *     - 不在此处弹窗（弹窗由 video_rec_ui_show_alert 单独处理）
 *
 *   VIDEO_STATE_PAUSED (P1, 当前暂不实现):
 *     - 停止闪烁，红点常亮或显示暂停图标
 *==============================================================================
 */
void video_rec_ui_on_state_change(enum video_rec_state old_state,
                                  enum video_rec_state new_state);

/*==============================================================================
 * 录像计时更新
 *
 * 调用时机:
 *   video_rec.c 主循环每 500ms 调用一次，传入当前已录制秒数。
 *
 * @param elapsed_sec  从录像开始至今的累计秒数 (u32)
 *
 * 实现要求:
 *   - 将 elapsed_sec 格式化为 MM:SS 字符串
 *     例: 0→"00:00", 65→"01:05", 3661→"61:01"
 *   - 调用 lv_label_set_text() 更新计时标签
 *   - 如果计时标签被隐藏(RECORDING以外状态)，此函数应安全忽略
 *
 * 注意:
 *   - 分钟数可以超过 60（支持长时间录像）
 *   - 使用 snprintf 格式化: "%02u:%02u", mins, secs
 *==============================================================================
 */
void video_rec_ui_update_timer(u32 elapsed_sec);

/*==============================================================================
 * 警告/提示弹窗
 *
 * 调用时机:
 *   Coder1 检测到以下事件时调用:
 *     type=0: SD卡空间不足 / 存储满
 *     type=1: SD卡被拔出
 *     type=2: 电量低于10%
 *     type=3: AVI文件保存完成
 *
 * COD1-P1 扩展:
 *   video_rec.c 在以下额外场景调用本接口:
 *     - 显示通道/摄像头 sensor 启动失败     (type=1 复用, 因 sensor 失败 ≈ 无 SD)
 *     - 编码器/录制启动失败 (start/resume/worker)
 *     - 系统状态异常 (重复 start)
 *     - 任务/线程创建失败
 *     以上场景统一用 type=2 (复用低电量弹窗, P2 阶段会扩展独立 type 编号).
 *
 *   video_rec_worker() 线程在退出路径调用 type=3 ("保存完成") 提示用户.
 *   实现者应保证弹窗函数是线程安全的; 当前 stub 是空函数, 无锁访问;
 *   真实 LVGL 实现需要将弹窗构造派发到主线程 (例如 os_post_event 到
 *   app_enter_video_mode 的消息循环, 或用 sys_timer_add 延后执行).
 *
 * @param type  弹窗类型 (int)
 *   0 = "存储已满"    + 确定按钮
 *   1 = "请插入SD卡"  + 确定按钮
 *   2 = "电量不足"    + 确定按钮
 *   3 = "保存完成"    无按钮, 2秒后自动消失
 *
 * 实现要求:
 *   通用弹窗结构:
 *     - 底层: 半透明黑色遮罩 (320×386, bg_opa=128)
 *     - 上层: 深灰圆角弹窗 (220×120, #333333, radius=8, 白色2px边框)
 *     - 标题: 可选（type=3 无标题，其他有标题）
 *     - 正文: 居中白色文字
 *     - 确定按钮: 80×30, 底部居中, 点击→删除整个弹窗(含遮罩)
 *     - type=3: 无按钮, lv_timer_create(2000ms)→自动删除
 *
 *   重复调用:
 *     - 如果已有弹窗未关闭，先删除旧弹窗再创建新的
 *
 *   线程安全 (P2 待办):
 *     - 当前实现从主线程和 worker 线程都可能调用
 *     - 若 LVGL 不允许非主线程访问, 实现者应通过 os_post_event 或
 *       sys_timer_add 将构造动作延后到主线程
 *==============================================================================
 */
void video_rec_ui_show_alert(int type);
void video_rec_ui_back_to_home(void);

/*==============================================================================
 * LVGL 对象结构建议 (供代码生成工具参考)
 *
 *   lv_scr_act()                          ← 当前活动屏幕（摄像头预览在下层）
 *   └── g_overlay (lv_obj)                ← 透明容器, 320×386, clickable
 *       ├── g_red_dot (lv_obj)            ← 圆形, 12×12, radius=6, #FF0000, hidden
 *       ├── g_timer_label (lv_label)      ← "00:00", 红点右侧, 白色, hidden
 *       ├── g_progress_bar (lv_bar)       ← 200×16, 居中, 范围0-100, hidden
 *       └── g_alert_mask (lv_obj)         ← 弹窗遮罩, 动态创建/销毁
 *           └── g_alert_box (lv_obj)      ← 弹窗主体, 220×120, #333333
 *               ├── title_label           ← 可选
 *               ├── text_label            ← 正文
 *               └── ok_btn (lv_btn)       ← 确定按钮(type≠3)
 *
 *   动画:
 *     g_blink_timer (lv_timer)            ← 500ms周期, 切换红点 HIDDEN flag
 *
 *   颜色常量:
 *     #define COLOR_RED    lv_color_hex(0xFF0000)
 *     #define COLOR_WHITE  lv_color_hex(0xFFFFFF)
 *     #define COLOR_BLACK  lv_color_hex(0x000000)
 *     #define COLOR_GREY_BG lv_color_hex(0x333333)
 *==============================================================================
 */

#endif /* CONFIG_VIDEO_REC_UI_H */
