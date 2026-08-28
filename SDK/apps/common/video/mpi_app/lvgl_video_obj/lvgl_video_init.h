#ifndef LVGL_VIDEO_INIT_H
#define LVGL_VIDEO_INIT_H

#include "sdk_config.h"
#include "stdint.h"

void *lvgl_video_init(int32_t width, int32_t height, int32_t x, int32_t y, const char *fb_name);
void  lvgl_video_exit(void *handle);         /* 异步, 通过 RPC 投递到 LVGL 线程 */
void  lvgl_video_exit_sync(void *handle);    /* 同步, 直接在调用线程执行 (必须在 LVGL 线程上下文调用) */

#endif /* LVGL_VIDEO_INIT_H */
