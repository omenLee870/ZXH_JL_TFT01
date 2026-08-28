#ifndef MAP_ROUTE_H
#define MAP_ROUTE_H

#include "lvgl.h"
#include "graph_tile.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * A*节点唯一编号
 * 因为现在不是一个graph.bin
 * 一个节点由：tile_x, tile_y, node_id 三部分组成
 */
typedef struct {
    int16_t tile_x;
    int16_t tile_y;
    uint32_t node_id;
} graph_node_id_t;

/*
 * 初始化道路数据
 * path: 例如 storage/sd0/C/graph_tiles
 */
bool map_route_load(const char *path);

/*
 * 释放道路缓存
 */
void map_route_free(void);

/*
 * 初始化路线显示
 * parent: LVGL地图对象
 */
void map_route_init(lv_obj_t *parent);

/*
 * 导航计算
 * 输入: 起点GPS, 终点GPS
 */
bool map_route_navigation(double start_lat, double start_lon,
                          double end_lat, double end_lon);

/*
 * 清除当前路线 (释放显示资源, 重置数据)
 */
void map_route_clear(void);

/*
 * 上次导航是否超时
 */
bool map_route_is_timeout(void);

/*
 * 地图移动/缩放后更新路线显示
 */
void map_route_update(void);

/*
 * 设置导航当前进度位置 (经纬度)
 * 用于把已走过的路线段抹除(灰显), 未走过部分保持高德蓝。
 * 由导航引导在每次位置更新时调用。
 */
void map_route_set_current_pos(double lat, double lon);

/*
 * 查找最近道路节点
 * 返回: tile坐标 + node编号
 */
bool map_route_find_nearest(double lat, double lon, graph_node_id_t *result);

/*
 * 当前路线总距离 (米)
 * 导航成功后有效, 无路线时返回 0
 */
double map_route_total_distance_m(void);

/*
 * 当前路线节点数
 * 导航成功后有效, 无路线时返回 0
 */
int map_route_get_count(void);

/*
 * 获取路线第 idx 个节点的经纬度
 * 返回 false 表示索引越界
 */
bool map_route_get_point(int idx, double *lat, double *lon);

/*
 * 获取路线段 idx→idx+1 的路名 id 与长度(cm)
 * 返回 false 表示越界或找不到边
 */
bool map_route_get_segment(int idx, uint16_t *name_id, uint32_t *dist_cm);

#endif
