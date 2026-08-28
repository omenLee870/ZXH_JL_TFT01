#ifndef NAV_HUD_H
#define NAV_HUD_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 转向类型
 */
typedef enum {
    NAV_TURN_NONE = 0,        /* 无提示 (清空转向框内容) */
    NAV_TURN_LEFT,            /* 左转 */
    NAV_TURN_SLIGHT_LEFT,     /* 缓左转 */
    NAV_TURN_STRAIGHT,        /* 直行 */
    NAV_TURN_SLIGHT_RIGHT,    /* 缓右转 */
    NAV_TURN_RIGHT,           /* 右转 */
    NAV_TURN_UTURN,           /* 掉头 */
    NAV_TURN_ARRIVE,          /* 到达终点 */
} nav_turn_type_t;

/*
 * 初始化导航 HUD (在 map_init 中调用一次)
 * parent: 地图容器对象 (canvas2590), HUD 作为其子对象悬浮固定在地图上方
 */
void nav_hud_init(lv_obj_t *parent);

/*
 * 显示 HUD
 */
void nav_hud_show(void);

/*
 * 隐藏 HUD
 */
void nav_hud_hide(void);

/*
 * 更新车速 (km/h)
 * speed_kmh: 车速, 单位 km/h
 */
void nav_hud_set_speed(uint16_t speed_kmh);

/*
 * 更新转向指示
 * dir:    转向类型 (LEFT/STRAIGHT/RIGHT/NONE)
 * dist_m: 距离转向点多少米 (直行/无提示时忽略)
 * road:   当前道路名 (可为 NULL 或空串, 无名道路不显示)
 */
void nav_hud_set_turn(nav_turn_type_t dir, int dist_m, const char *road);

/*
 * 更新剩余距离与时间
 * remain_km:  剩余距离, 单位 km
 * remain_min: 预计剩余时间, 单位 分钟
 */
void nav_hud_set_remain(float remain_km, int remain_min);

/*
 * 显示起终点名称 (顶部居中)
 * start/end: 起终点名 (UTF-8, 可为 NULL 或空串), 显示为 "起点 → 终点"
 */
void nav_hud_set_route_names(const char *start, const char *end);

#ifdef __cplusplus
}
#endif

#endif /* NAV_HUD_H */
