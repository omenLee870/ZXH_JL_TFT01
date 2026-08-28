#ifndef GRAPH_TILE_H
#define GRAPH_TILE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 道路等级 — A* 动态剪枝用
 */
#define ROAD_CLASS_LOCAL 0  /* 小区内部: residential, service, track */
#define ROAD_CLASS_MID   1  /* 集散/自行车: tertiary, unclassified, living_street, cycleway, path */
#define ROAD_CLASS_HIGH  2  /* 主干: primary, secondary */

/*
 * p_flag 中的位定义
 */
#define FLAG_ONEWAY          0x01   /* bit 0: 单行道 */
#define FLAG_ROAD_CLASS_SHIFT 1     /* bits 1-2: road class */
#define FLAG_ROAD_CLASS_MASK  0x06

/*
 * 单个tile节点格式 (Python: "<iiIIII", 24 bytes)
 */
#pragma pack(push,1)
typedef struct {
    int32_t lat;
    int32_t lon;
    uint32_t edge_start;
    uint32_t edge_count;
    uint32_t rev_edge_start;
    uint32_t rev_edge_count;
} graph_node_t;

/*
 * edge格式 (Python: "<hhIIHH", 16 bytes)
 */
typedef struct {
    int16_t target_tile_x;
    int16_t target_tile_y;
    uint32_t target_node;
    uint32_t distance;    /* cm */
    uint16_t flags;
    uint16_t name_id;     /* 道路名字符串表索引 (names.bin), 0 = 无名 */
} graph_edge_t;
#pragma pack(pop)

/*
 * tile缓存数量
 */
#define GRAPH_TILE_CACHE_SIZE 48

/*
 * 单个tile
 */
typedef struct {
    int16_t tile_x;
    int16_t tile_y;
    graph_node_t *nodes;
    graph_edge_t *edges;
    graph_edge_t *rev_edges;
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t rev_edge_count;
    uint32_t last_used;
    uint8_t used;
} graph_tile_t;

/*
 * 初始化graph目录
 * path: 例如 storage/sd0/C/graph_tiles
 */
bool graph_tile_init(const char *path);

/*
 * 加载tile
 */
graph_tile_t *graph_tile_load(int tile_x, int tile_y);

/*
 * 查找已经加载的tile
 */
graph_tile_t *graph_tile_find(int tile_x, int tile_y);

/*
 * 释放全部缓存
 */
void graph_tile_free_all(void);

/*
 * 根据经纬度获取tile编号
 */
void graph_coord_to_tile(double lat, double lon, int *tile_x, int *tile_y);

/*
 * 加载道路名字符串表 (names.bin)
 * path: 例如 storage/sd0/C/graph_tiles
 */
bool graph_names_load(const char *path);

/*
 * 按 name_id 查询路名 (UTF-8, 含结尾 '\0')
 * 无名或查不到返回 false, out 置空串
 */
bool graph_name_lookup(uint16_t id, char *out, int out_size);

/*
 * 释放字符串表
 */
void graph_names_free(void);

#endif
