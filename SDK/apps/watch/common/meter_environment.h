/**
 * @file meter_environment.h
 * @brief 手机校时及天气快照；接收在 app_core，UI 线程只读取快照。
 */
#ifndef METER_ENVIRONMENT_H
#define METER_ENVIRONMENT_H
#include <stdint.h>
enum meter_weather { METER_WEATHER_UNKNOWN, METER_WEATHER_CLEAR, METER_WEATHER_CLOUD,
    METER_WEATHER_OVERCAST, METER_WEATHER_RAIN, METER_WEATHER_SNOW,
    METER_WEATHER_STORM, METER_WEATHER_FOG, METER_WEATHER_DUST };
struct meter_environment {
    uint32_t utc, observed_utc;
    int timezone_minutes, temperature_dc;
    unsigned clock_valid, weather_valid, weather, hour;
    unsigned weather_cached; /**< 有上次天气；过期仍可显示，但不标为实时有效。 */
    char clock[6], date[24];
};
/** @brief 本地日期时间，按仪表当前时区解释；手动保存时秒数归零。 */
struct meter_local_time { unsigned year, month, day, hour, minute; };
int meter_environment_init(void);
void meter_environment_snapshot(struct meter_environment *out);
int meter_environment_receive(void *client, const char *topic, const char *json);
/** @brief 有效时返回当前本地时间；无效返回 -1，调用方不能把占位日期当成已校时。 */
int meter_environment_local_time(struct meter_local_time *out);
/** @brief UI 非阻塞投递，RTC 和保存标记统一由 app_core 写入。 */
int meter_environment_time_post(const struct meter_local_time *value);
/** @brief 返回本地保存结果序号；与 BLE 校时确认相互独立。 */
unsigned meter_environment_time_result(int *code);
#endif
