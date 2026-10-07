#ifndef CHONGWUYIBIAO_DONGHUA_H
#define CHONGWUYIBIAO_DONGHUA_H

#include "../generated/gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 由 UI 工具的页面初始化代码调用，绑定页面生命周期。 */
void chongwuyibiao_donghua_bangding(lv_ui_chongwuyibiao *jiemian);

/* 在 LVGL 线程调用；同时更新车速文字和小狗的停车/跑步状态。 */
void chongwuyibiao_chesu_gengxin(uint16_t chesu);

#ifdef __cplusplus
}
#endif

#endif
