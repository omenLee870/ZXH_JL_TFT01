#ifndef CAMERA_DISPLAY_UI_H
#define CAMERA_DISPLAY_UI_H

/**
 * @file camera_display_ui.h
 * @brief 摄像头推屏到LVGL的注册/卸载接口
 *
 * 使用示例:
 *   // 在进入摄像头屏幕时:
 *   camera_display_ui_init(1024, 600, 0, 0);
 *
 *   // 在退出摄像头屏幕时:
 *   camera_display_ui_deinit();
 */

#include <stdint.h>

/**
 * @brief 初始化摄像头推屏
 * @param width   显示区域宽度
 * @param height  显示区域高度
 * @param x       显示区域左上角 X 坐标
 * @param y       显示区域左上角 Y 坐标
 * @return 0 成功, 负数失败
 */
int camera_display_ui_init(int width, int height, int x, int y);

/**
 * @brief 卸载摄像头推屏, 释放所有资源
 */
void camera_display_ui_deinit(void);

/**
 * @brief 查询摄像头推屏是否已激活
 * @return 1=已激活, 0=未激活
 */
int camera_display_ui_is_active(void);

#endif /* CAMERA_DISPLAY_UI_H */
