/**
 * @file ui_meter_settings.h
 * @brief JLUI 设置页事件与运行期绑定；所有 LVGL 操作仅在 UI 线程执行。
 */
#ifndef UI_METER_SETTINGS_H
#define UI_METER_SETTINGS_H
void ui_meter_settings_bind(void);
void ui_meter_settings_open(void);
void ui_meter_settings_close(void);
void ui_meter_settings_select(unsigned index);
void ui_meter_settings_change(unsigned key, int value);
void ui_meter_settings_confirm(unsigned key);
void ui_meter_settings_cancel(void);
void ui_meter_settings_commit(void);
void ui_meter_settings_poll(void);
/** @brief 年月变化时修正日期范围；草稿只在按保存后提交。 */
void ui_meter_settings_time_edit(void);
void ui_meter_settings_time_save(void);
/** @brief 按键任务只登记唤醒请求；返回非零时吞掉本次唤醒按键。 */
int ui_meter_settings_key_activity(void);
#endif
