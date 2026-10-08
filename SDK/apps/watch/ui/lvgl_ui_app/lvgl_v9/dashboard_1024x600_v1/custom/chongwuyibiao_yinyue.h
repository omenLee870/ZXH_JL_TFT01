#ifndef CHONGWUYIBIAO_YINYUE_H
#define CHONGWUYIBIAO_YINYUE_H

#include "../generated/gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 工具页面初始化入口，仅绑定显示和点击事件。 */
void chongwuyibiao_yinyue_bangding(lv_ui_chongwuyibiao *jiemian);
/* 解码结果由 LVGL 线程交给显示适配层。 */
void chongwuyibiao_yinyue_fengmian(const lv_image_dsc_t *image);

#ifdef __cplusplus
}
#endif
#endif
