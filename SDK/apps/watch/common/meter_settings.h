/**
 * @file meter_settings.h
 * @brief 手机与仪表共用的本机设置；不修改控制器参数。
 */
#ifndef METER_SETTINGS_H
#define METER_SETTINGS_H
#include <stdint.h>
enum meter_setting_key { METER_BRIGHTNESS, METER_MUSIC, METER_TONE, METER_KEY_TONE,
                         METER_UNIT, METER_SLEEP, METER_TRIP_RESET, METER_RESTORE, METER_SETTING_COUNT
                       };
struct meter_preferences
{
    uint32_t brightness, music, tone, key_tone, unit, sleep_minutes, revision;
};
/** @brief 在 app_core 初始化持久化设置；重复调用不重复创建资源。 */
int meter_settings_init(void);
/** @brief 读取已保存设置的副本；尚未初始化时返回默认值。 */
void meter_settings_snapshot(struct meter_preferences *out);
/** @brief 非阻塞投递本地操作；app_core 执行保存，0 表示已入队。 */
int meter_settings_post(enum meter_setting_key key, int value);
/** @brief BLE 专用主题入口，仅从 app_core 调用，返回是否识别该主题。 */
int meter_settings_receive(void *client, const char *topic, const char *json);
/** @brief 读取通用显示单位或按键提示开关，不访问硬件。 */
int meter_settings_miles(void);
int meter_settings_key_tone(void);
/** @brief 返回最后一次本地操作结果，用于 UI 显示必要的失败反馈。 */
unsigned meter_settings_result(int *code);
#endif
