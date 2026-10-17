#ifndef YOUYIBAO_SIF_UI_H
#define YOUYIBAO_SIF_UI_H
#include "../generated/gui_guider.h"

/**
 * @brief 在宠物页显示前绑定车辆数据显示及里程区域的点按事件。
 * @param[in] ui 已创建的宠物页控件；允许为空。
 * @retval 无。
 * @note 仅在 UI 线程调用；重复绑定同一页面不会重复注册事件，不修改车辆数据。
 */
void youyibao_sif_ui_bind_pet(lv_ui_chongwuyibiao *ui);

void youyibao_sif_ui_init(void);
/** @brief 仅在 UI 线程执行仪表本机显示控制；0 成功，-2 升级中暂忙。 */
int youyibao_sif_ui_meter_apply(int power_on, int light_on, int mode);
/** @brief 升级页面临时亮屏，不改变仪表业务的开关机状态。 */
void youyibao_sif_ui_update_wake(void);
#endif
