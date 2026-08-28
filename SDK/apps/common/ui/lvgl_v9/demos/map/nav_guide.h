#ifndef NAV_GUIDE_H
#define NAV_GUIDE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 模拟导航开关
 *   NAV_GUIDE_SIM = 1: 无 GPS 环境下, 让当前位置沿路线按固定速度前进,
 *                      用于联调转向引导 / 路名 / 剩余里程。
 *   NAV_GUIDE_SIM = 0: 使用真实 GPS。
 */
#define NAV_GUIDE_SIM           1
#define NAV_GUIDE_SIM_SPEED_KMH 100   /* 模拟车速, 可改 */

/*
 * 转向提示距离阈值 (米)
 * 距下一转弯点大于该距离时不显示转向箭头(只显示当前道路名),
 * 小于等于该距离时才显示"左转/右转 + X 米"。
 */
#define NAV_GUIDE_TURN_PROMPT_M 1000

/*
 * 导航引导引擎
 *
 * 职责: 拿 map_route 算出的路线, 预计算每个节点的转向与累计里程,
 *       然后 GPS 每刷新一次, 匹配当前位置、算距下一转弯点距离、
 *       查当前道路名, 喂给 nav_hud。
 *
 * 用法:
 *   导航成功后  -> nav_guide_build()
 *   清除路线时  -> nav_guide_clear()
 *   GPS 更新时  -> nav_guide_update(lat_e7, lon_e7)
 *   模拟模式    -> nav_guide_sim_reset() + nav_guide_sim_tick()
 *   车速变化时  -> nav_guide_set_speed(kmh)  (用于剩余时间估算)
 */

/*
 * 预计算转向点与累计里程 (需 map_route 已出路线)
 */
void nav_guide_build(void);

/*
 * 清除引导状态 (路线清除/重新规划时调用)
 */
void nav_guide_clear(void);

/*
 * GPS 位置更新 (lat/lon 为 e7 整数, 例 116.397128° -> 1163971280)
 * 内部完成地图匹配 + 转向距离 + 路名, 并更新 HUD
 */
void nav_guide_update(int32_t lat_e7, int32_t lon_e7);

/*
 * 设置当前车速 (km/h), 用于剩余时间估算
 */
void nav_guide_set_speed(uint16_t kmh);

#if NAV_GUIDE_SIM
/*
 * 模拟导航: 推进一帧 (由上层定时器周期性调用)
 * 内部: 按速度推进模拟位置 -> 更新 HUD -> 输出当前模拟经纬度 (e7)
 * 返回 false 表示已到终点 (应停止继续推进)
 */
bool nav_guide_sim_tick(int32_t *lat_e7, int32_t *lon_e7);

/*
 * 重置模拟位置到路线起点
 */
void nav_guide_sim_reset(void);

/*
 * 当前模拟前进方向 (方位角 0~359, 北=0 顺时针)
 * 供上层旋转当前位置图标, 使箭头指向行进方向
 */
int16_t nav_guide_sim_heading(void);
#endif

#endif /* NAV_GUIDE_H */
