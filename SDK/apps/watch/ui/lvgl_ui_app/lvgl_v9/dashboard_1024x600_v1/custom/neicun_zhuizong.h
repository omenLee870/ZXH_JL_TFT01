#ifndef NEICUN_ZHUIZONG_H
#define NEICUN_ZHUIZONG_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void neicun_zhuizong_kaishi(void);
void neicun_zhuizong_jieduan(const char *jieduan);

/* SDK 绘图和字体加载器的 weak 调试入口，不改变内存分配方式。 */
void lv_draw_buf_ram_trace(const lv_draw_buf_t *huanchong, bool fenpei);
void lv_font_ram_trace(const char *lujing, bool kaishi);

#ifdef __cplusplus
}
#endif
#endif
