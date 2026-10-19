/** @file ui_environment.h @brief 宠物页环境信息渲染，仅从 UI 线程调用。 */
#ifndef UI_ENVIRONMENT_H
#define UI_ENVIRONMENT_H
#include "../generated/gui_guider.h"
void ui_environment_bind(lv_ui_chongwuyibiao *ui);
void ui_environment_refresh(lv_ui_chongwuyibiao *ui);
#endif
