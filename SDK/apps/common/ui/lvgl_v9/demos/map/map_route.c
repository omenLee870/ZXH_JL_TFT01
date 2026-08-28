#include "map_route.h"
#include "map_tiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* 把大数组直接放到 PSRAM 的 bss 段（链接脚本已定义 .psram_bss），
 * 编译期定址，避免占用内部 SRAM，也避免 malloc 动态分配的 cache 问题 */
#define AT_PSRAM_BSS __attribute__((section(".psram_bss")))

#define ROUTE_MAX_POINT 5000

/*
 * 路径规划超时 (ms), 默认 30s
 */
#define ROUTE_TIMEOUT_MS  30000

/*
 * A*最大搜索节点 (双向共用)
 */
#define ASTAR_MAX_NODE 40000

/*
 * 哈希表
 */
#define HASH_BITS  15
#define HASH_SIZE  (1 << HASH_BITS)
#define HASH_MASK  (HASH_SIZE - 1)
#define HASH_EMPTY ((int16_t)(-1))

static int16_t node_hash[HASH_SIZE] AT_PSRAM_BSS;

extern bool map_tiles_gps_to_screen(double lat, double lon, int *x, int *y);

/*
 * 动态剪枝阈值 + 分级惩罚系数
 *
 * 起终点附近 (< PRUNE_RADIUS_CM): 所有道路 cost × 1
 * 中间地带 (> PRUNE_RADIUS_CM):
 *   HIGH  (class 2): × 1       主干，始终优先
 *   MID   (class 1): × MID_PENALTY   集散/自行车道，中等惩罚
 *   LOCAL (class 0): × LOCAL_PENALTY 小区内部，重惩罚
 *
 * 不跳过任何边: 保证可达。A* 通过 cost 自然选路。
 */
#define PRUNE_RADIUS_CM  200000  /* 2km */
#define MID_PENALTY      1       /* tertiary/unclassified/cycleway... */
#define LOCAL_PENALTY    8       /* residential/service/track */

/********************************************************
 * 当前路线
 ********************************************************/
static graph_node_id_t current_route[ROUTE_MAX_POINT] AT_PSRAM_BSS;
static int current_route_count = 0;
static lv_obj_t *route_line = NULL;
static lv_obj_t *route_line_done = NULL;   /* 已走过路段(灰色) */
static lv_point_precise_t route_points[ROUTE_MAX_POINT] AT_PSRAM_BSS;
static lv_point_precise_t route_points_done[ROUTE_MAX_POINT] AT_PSRAM_BSS;
static lv_point_precise_t route_points_todo[ROUTE_MAX_POINT] AT_PSRAM_BSS;

static int32_t route_lat[ROUTE_MAX_POINT] AT_PSRAM_BSS;
static int32_t route_lon[ROUTE_MAX_POINT] AT_PSRAM_BSS;

static bool route_timeout = false;  /* 上一次路径规划是否超时 */

/* 当前导航进度位置(用于抹除已走路段) */
static bool route_has_pos = false;
static double route_cur_lat = 0.0;
static double route_cur_lon = 0.0;

/********************************************************
 * 双向 A* 节点
 ********************************************************/
#define SIDE_NONE 0
#define SIDE_FWD  1
#define SIDE_BWD  2

typedef struct {
    graph_node_id_t id;
    int parent;
    uint32_t g;
    uint32_t f;
    uint8_t close;
    uint8_t side;
} astar_node_t;

static astar_node_t astar_buf[ASTAR_MAX_NODE] AT_PSRAM_BSS;

/********************************************************
 * heap
 ********************************************************/
typedef struct {
    int index;
    uint32_t f;
} heap_node_t;

#define MAX_HEAP (ASTAR_MAX_NODE / 2)

static heap_node_t heap_fwd[MAX_HEAP] AT_PSRAM_BSS;
static int heap_fwd_sz = 0;
static heap_node_t heap_bwd[MAX_HEAP] AT_PSRAM_BSS;
static int heap_bwd_sz = 0;

static void heap_push(heap_node_t *h, int *sz, int index, uint32_t f)
{
    if (*sz >= MAX_HEAP) {
        return;
    }

    int i = (*sz)++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h[p].f <= f) {
            break;
        }
        h[i] = h[p];
        i = p;
    }
    h[i].index = index;
    h[i].f = f;
}

static int heap_pop(heap_node_t *h, int *sz)
{
    if (*sz <= 0) {
        return -1;
    }

    int result = h[0].index;
    (*sz)--;

    if (*sz > 0) {
        heap_node_t last = h[*sz];
        int i = 0;
        while (1) {
            int left = i * 2 + 1;
            int right = left + 1;
            if (left >= *sz) {
                break;
            }
            int child = left;
            if (right < *sz && h[right].f < h[left].f) {
                child = right;
            }
            if (h[child].f >= last.f) {
                break;
            }
            h[i] = h[child];
            i = child;
        }
        h[i] = last;
    }
    return result;
}

/********************************************************
 * 前置声明
 ********************************************************/
static bool node_equal(graph_node_id_t *a, graph_node_id_t *b);

/********************************************************
 * node_id 哈希
 ********************************************************/
static uint32_t hash_id(graph_node_id_t *id)
{
    uint32_t h = (uint32_t)(id->tile_x);
    h = h * 31 + (uint32_t)(id->tile_y);
    h = h * 31 + id->node_id;
    return h;
}

static void hash_clear(void)
{
    for (int i = 0; i < HASH_SIZE; i++) {
        node_hash[i] = HASH_EMPTY;
    }
}

static int hash_find(graph_node_id_t *id)
{
    uint32_t h = hash_id(id);
    for (uint32_t probe = 0; probe < HASH_SIZE; probe++) {
        uint32_t slot = (h + probe) & HASH_MASK;
        int16_t idx = node_hash[slot];
        if (idx == HASH_EMPTY) {
            return -1;
        }
        if (node_equal(id, &astar_buf[idx].id)) {
            return (int)idx;
        }
    }
    return -1;
}

static void hash_insert(graph_node_id_t *id, int idx)
{
    uint32_t h = hash_id(id);
    for (uint32_t probe = 0; probe < HASH_SIZE; probe++) {
        uint32_t slot = (h + probe) & HASH_MASK;
        if (node_hash[slot] == HASH_EMPTY) {
            node_hash[slot] = (int16_t)idx;
            return;
        }
    }
}

/********************************************************
 * graph节点比较
 ********************************************************/
static bool node_equal(graph_node_id_t *a, graph_node_id_t *b)
{
    return a->tile_x == b->tile_x &&
           a->tile_y == b->tile_y &&
           a->node_id == b->node_id;
}

/********************************************************
 * 获取节点数据
 ********************************************************/
static graph_node_t *get_node(graph_node_id_t *id)
{
    graph_tile_t *tile = graph_tile_load(id->tile_x, id->tile_y);
    if (tile == NULL) {
        return NULL;
    }
    if (id->node_id >= tile->node_count) {
        return NULL;
    }
    return &tile->nodes[id->node_id];
}

/********************************************************
 * 两个节点直线距离 (启发式, cm)
 ********************************************************/
static uint32_t node_distance(graph_node_id_t *a, graph_node_id_t *b)
{
    graph_node_t *na = get_node(a);
    graph_node_t *nb = get_node(b);
    if (na == NULL || nb == NULL) {
        return 0xffffffff;
    }

    double lat1 = na->lat / 10000000.0;
    double lon1 = na->lon / 10000000.0;
    double lat2 = nb->lat / 10000000.0;
    double lon2 = nb->lon / 10000000.0;

    double cos_lat = cos((lat1 + lat2) / 2.0 * M_PI / 180.0);
    double dx = (lat1 - lat2) * 111000.0;
    double dy = (lon1 - lon2) * 111000.0 * cos_lat;
    double d = sqrt(dx * dx + dy * dy);
    return (uint32_t)(d * 100);
}

/********************************************************
 * 分级惩罚: 起终点附近所有道路正常, 中间地带按等级惩罚
 *
 * 返回 cost 乘数:
 *   1  → 正常
 *   MID_PENALTY  → 中间地带的 class 1 边
 *   LOCAL_PENALTY → 中间地带的 class 0 边
 ********************************************************/
static int edge_penalty(uint32_t g_value,
                        graph_node_id_t *next,
                        graph_node_id_t *target,
                        uint16_t flags)
{
    /* 起终点附近: 不惩罚 */
    if (g_value <= PRUNE_RADIUS_CM) {
        return 1;
    }
    uint32_t to_target = node_distance(next, target);
    if (to_target <= PRUNE_RADIUS_CM) {
        return 1;
    }

    /* 中间地带: 按等级惩罚 */
    int cls = (flags & FLAG_ROAD_CLASS_MASK) >> FLAG_ROAD_CLASS_SHIFT;
    if (cls == ROAD_CLASS_HIGH) {
        return 1;
    }
    if (cls == ROAD_CLASS_MID) {
        return MID_PENALTY;
    }
    return LOCAL_PENALTY;
}

/********************************************************
 * 加权 A*
 ********************************************************/
#define ASTAR_WEIGHT 3

#if ASTAR_WEIGHT > 0
static uint32_t astar_f(uint32_t g, uint32_t h)
{
    return g + h + (h / 10) * ASTAR_WEIGHT;
}
#else
static uint32_t astar_f(uint32_t g, uint32_t h)
{
    return g + h;
}
#endif

/********************************************************
 * 初始化graph
 ********************************************************/
bool map_route_load(const char *path)
{
    if (!graph_tile_init(path)) {
        return false;
    }
    /* 路名字符串表, 加载失败不致命(导航仍可用, 只是无路名) */
    graph_names_load(path);
    return true;
}

/********************************************************
 * 释放
 ********************************************************/
void map_route_free(void)
{
    graph_tile_free_all();
}

/********************************************************
 * 上次导航是否超时
 ********************************************************/
bool map_route_is_timeout(void)
{
    return route_timeout;
}

/********************************************************
 * 查找最近节点
 ********************************************************/
bool map_route_find_nearest(double lat, double lon, graph_node_id_t *result)
{
    if (result == NULL) {
        return false;
    }

    int tx, ty;
    graph_coord_to_tile(lat, lon, &tx, &ty);

    double min = 999999999;
    bool found = false;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            graph_tile_t *tile = graph_tile_load(tx + x, ty + y);
            if (tile == NULL) {
                continue;
            }

            for (uint32_t i = 0; i < tile->node_count; i++) {
                graph_node_t *node = &tile->nodes[i];
                double nlat = node->lat / 10000000.0;
                double nlon = node->lon / 10000000.0;
                double dx = nlat - lat;
                double dy = nlon - lon;
                double dis = dx * dx + dy * dy;

                if (dis < min) {
                    min = dis;
                    result->tile_x = tile->tile_x;
                    result->tile_y = tile->tile_y;
                    result->node_id = i;
                    found = true;
                }
            }
        }
    }

    if (found) {
        printf("nearest tile=%d,%d node=%u\n",
               result->tile_x, result->tile_y, result->node_id);
    }
    return found;
}

/********************************************************
 * 双向 A* (动态剪枝)
 *
 * 不是分层搜索 → 只在一张全量图上跑一次 A*。
 * 展开邻居时:
 *   - 当前已走距离 < 2km 或 目标离终点 < 2km: 走所有边
 *   - 二者都 > 2km:                          跳过 LOCAL 边
 *
 * 效果: 起终点附近走"毛细血管"路网，中间长途走"主干+集散"路网。
 ********************************************************/
static int map_route_astar(graph_node_id_t *start, graph_node_id_t *end,
                           graph_node_id_t *path, int max_path)
{
    memset(astar_buf, 0, sizeof(astar_buf));
    hash_clear();
    heap_fwd_sz = 0;
    heap_bwd_sz = 0;

    /* 正向: start → end */
    astar_buf[0].id     = *start;
    astar_buf[0].parent = -1;
    astar_buf[0].g      = 0;
    astar_buf[0].f      = astar_f(0, node_distance(start, end));
    astar_buf[0].side   = SIDE_FWD;
    hash_insert(start, 0);
    heap_push(heap_fwd, &heap_fwd_sz, 0, astar_buf[0].f);

    /* 反向: end → start */
    astar_buf[1].id     = *end;
    astar_buf[1].parent = -1;
    astar_buf[1].g      = 0;
    astar_buf[1].f      = astar_f(0, node_distance(end, start));
    astar_buf[1].side   = SIDE_BWD;
    hash_insert(end, 1);
    heap_push(heap_bwd, &heap_bwd_sz, 1, astar_buf[1].f);

    int node_count = 2;
    uint32_t best_cost = 0xffffffff;
    int best_fwd_idx = -1;
    int best_bwd_idx = -1;

    int penalty_mid = 0, penalty_local = 0;  /* 被惩罚的 MID/LOCAL 边数 */

    uint32_t start_tick = lv_tick_get();

    while (heap_fwd_sz > 0 && heap_bwd_sz > 0) {
        /* 超时检测 */
        if (lv_tick_elaps(start_tick) >= ROUTE_TIMEOUT_MS) {
            printf("astar timeout (%d s)\n", ROUTE_TIMEOUT_MS / 1000);
            route_timeout = true;
            return -2;
        }

        if (best_cost != 0xffffffff) {
            uint32_t min_sum = heap_fwd[0].f + heap_bwd[0].f;
            if (min_sum >= best_cost) {
                break;
            }
        }

        int side, current_index;
        if (heap_fwd_sz <= heap_bwd_sz) {
            side = SIDE_FWD;
            current_index = heap_pop(heap_fwd, &heap_fwd_sz);
        } else {
            side = SIDE_BWD;
            current_index = heap_pop(heap_bwd, &heap_bwd_sz);
        }
        if (current_index < 0) {
            break;
        }

        astar_node_t *current = &astar_buf[current_index];
        if (current->close) {
            continue;
        }
        current->close = 1;

        graph_tile_t *cur_tile = graph_tile_load(current->id.tile_x,
                                 current->id.tile_y);
        if (cur_tile == NULL) {
            continue;
        }

        graph_node_t *node = &cur_tile->nodes[current->id.node_id];
        if (current->id.node_id >= cur_tile->node_count) {
            continue;
        }

        graph_node_id_t *target = (side == SIDE_FWD) ? end : start;

        uint32_t  edge_cnt   = (side == SIDE_FWD) ? node->edge_count
                               : node->rev_edge_count;
        uint32_t  edge_start = (side == SIDE_FWD) ? node->edge_start
                               : node->rev_edge_start;
        uint32_t  tile_ec    = (side == SIDE_FWD) ? cur_tile->edge_count
                               : cur_tile->rev_edge_count;
        graph_edge_t *edge_pool = (side == SIDE_FWD) ? cur_tile->edges
                                  : cur_tile->rev_edges;

        for (uint32_t i = 0; i < edge_cnt; i++) {
            uint32_t ei = edge_start + i;
            if (ei >= tile_ec) {
                continue;
            }
            graph_edge_t *edge = &edge_pool[ei];

            graph_node_id_t next;
            next.tile_x = edge->target_tile_x;
            next.tile_y = edge->target_tile_y;
            if (next.tile_x == 0 && next.tile_y == 0) {
                next.tile_x = current->id.tile_x;
                next.tile_y = current->id.tile_y;
            }
            next.node_id = edge->target_node;

            /*
             * 分级惩罚: 中间地带按道路等级加权 cost
             * HIGH ×1, MID ×2, LOCAL ×5
             * A* 自然优先走高等级路，但低等级也始终可用
             */
            int penalty = edge_penalty(current->g, &next, target, edge->flags);
            if (penalty > 1) {
                if (penalty == MID_PENALTY) {
                    penalty_mid++;
                } else {
                    penalty_local++;
                }
            }

            uint32_t new_g = current->g + edge->distance * penalty;
            int exist = hash_find(&next);

            if (exist < 0) {
                if (node_count >= ASTAR_MAX_NODE) {
                    continue;
                }

                exist = node_count++;
                astar_buf[exist].id     = next;
                astar_buf[exist].parent = current_index;
                astar_buf[exist].g      = new_g;
                astar_buf[exist].f      = astar_f(new_g, node_distance(&next, target));
                astar_buf[exist].side   = side;
                hash_insert(&next, exist);

                if (side == SIDE_FWD) {
                    heap_push(heap_fwd, &heap_fwd_sz, exist, astar_buf[exist].f);
                } else {
                    heap_push(heap_bwd, &heap_bwd_sz, exist, astar_buf[exist].f);
                }

            } else if (astar_buf[exist].side == side) {
                if (new_g < astar_buf[exist].g) {
                    astar_buf[exist].g      = new_g;
                    astar_buf[exist].f      = astar_f(new_g, node_distance(&next, target));
                    astar_buf[exist].parent = current_index;

                    if (side == SIDE_FWD) {
                        heap_push(heap_fwd, &heap_fwd_sz, exist, astar_buf[exist].f);
                    } else {
                        heap_push(heap_bwd, &heap_bwd_sz, exist, astar_buf[exist].f);
                    }
                }

            } else {
                /* 另一侧节点：无论 open 还是 close 都算相遇 */
                uint32_t total = new_g + astar_buf[exist].g;
                if (total < best_cost) {
                    best_cost = total;
                    if (side == SIDE_FWD) {
                        best_fwd_idx = current_index;
                        best_bwd_idx = exist;
                    } else {
                        best_fwd_idx = exist;
                        best_bwd_idx = current_index;
                    }
                }
            }
        }
    }

    printf("astar nodes=%d penalty(mid=%d,local=%d) heap(fwd=%d,bwd=%d)\n",
           node_count, penalty_mid, penalty_local, heap_fwd_sz, heap_bwd_sz);

    if (best_cost == 0xffffffff) {
        printf("astar fail\n");
        return -1;
    }

    /* 重建路径 */
    int count = 0;
    static int tmp[ROUTE_MAX_POINT] AT_PSRAM_BSS;

    int fwd_cnt = 0;
    int p = best_fwd_idx;
    while (p >= 0 && fwd_cnt < ROUTE_MAX_POINT) {
        tmp[fwd_cnt++] = p;
        p = astar_buf[p].parent;
    }
    for (int i = fwd_cnt - 1; i >= 0; i--) {
        if (count >= max_path) {
            break;
        }
        path[count++] = astar_buf[tmp[i]].id;
    }

    int bwd_cnt = 0;
    p = best_bwd_idx;
    while (p >= 0 && bwd_cnt < ROUTE_MAX_POINT) {
        tmp[bwd_cnt++] = p;
        p = astar_buf[p].parent;
    }
    int bwd_start = (best_fwd_idx == best_bwd_idx) ? 1 : 0;
    for (int i = bwd_start; i < bwd_cnt; i++) {
        if (count >= max_path) {
            break;
        }
        path[count++] = astar_buf[tmp[i]].id;
    }

    printf("astar success path=%d nodes=%d\n", count, node_count);
    return count;
}

/********************************************************
 * 初始化路线显示
 ********************************************************/
void map_route_init(lv_obj_t *parent)
{
    route_line = lv_line_create(parent);
    if (route_line == NULL) {
        printf("route line create fail\n");
        return;
    }

    lv_obj_set_style_line_width(route_line, 4, 0);
    lv_obj_set_style_line_rounded(route_line, true, 0);
    lv_obj_set_style_line_color(route_line, lv_color_hex(0x1a73e8), 0);

    /* 已走过路段: 灰色 */
    route_line_done = lv_line_create(parent);
    if (route_line_done != NULL) {
        lv_obj_set_style_line_width(route_line_done, 4, 0);
        lv_obj_set_style_line_rounded(route_line_done, true, 0);
        lv_obj_set_style_line_color(route_line_done, lv_color_hex(0x999999), 0);
    }
    printf("route init ok\n");
}

/********************************************************
 * 导航计算 — 单次 A* + 动态剪枝
 ********************************************************/
bool map_route_navigation(double start_lat, double start_lon,
                          double end_lat, double end_lon)
{
    printf("ENTER navigation\n");
    printf("route start %.6f %.6f\n", start_lat, start_lon);
    printf("route end   %.6f %.6f\n", end_lat, end_lon);

    route_timeout = false;

    graph_node_id_t start;
    graph_node_id_t end;

    if (!map_route_find_nearest(start_lat, start_lon, &start)) {
        printf("start node fail\n");
        return false;
    }

    if (!map_route_find_nearest(end_lat, end_lon, &end)) {
        printf("end node fail\n");
        return false;
    }

    static graph_node_id_t path[ROUTE_MAX_POINT] AT_PSRAM_BSS;

    int count = map_route_astar(&start, &end, path, ROUTE_MAX_POINT);
    if (count <= 0) {
        printf("route fail\n");
        return false;
    }

    memcpy(current_route, path, sizeof(graph_node_id_t) * count);
    current_route_count = count;

    /* 缓存坐标 */
    for (int i = 0; i < count; i++) {
        graph_node_t *n = get_node(&path[i]);
        if (n) {
            route_lat[i] = n->lat;
            route_lon[i] = n->lon;
        }
    }

    printf("route done: path=%d\n", count);
    map_route_update();
    return true;
}

/********************************************************
 * 清除当前路线
 ********************************************************/
void map_route_clear(void)
{
    current_route_count = 0;
    route_has_pos = false;
    if (route_line != NULL) {
        lv_line_set_points(route_line, route_points, 0);
    }
    if (route_line_done != NULL) {
        lv_line_set_points(route_line_done, route_points, 0);
    }
    printf("route cleared\n");
}

/********************************************************
 * 当前路线总距离 (米)
 * 沿缓存坐标逐段累加, 用于导航开始时的剩余里程估算
 ********************************************************/
double map_route_total_distance_m(void)
{
    double total = 0.0;

    for (int i = 1; i < current_route_count; i++) {
        double lat1 = route_lat[i - 1] / 10000000.0;
        double lon1 = route_lon[i - 1] / 10000000.0;
        double lat2 = route_lat[i] / 10000000.0;
        double lon2 = route_lon[i] / 10000000.0;

        double cos_lat = cos((lat1 + lat2) / 2.0 * M_PI / 180.0);
        double dx = (lat1 - lat2) * 111000.0;
        double dy = (lon1 - lon2) * 111000.0 * cos_lat;
        total += sqrt(dx * dx + dy * dy);
    }

    return total;
}

/********************************************************
 * 设置导航当前进度位置
 ********************************************************/
void map_route_set_current_pos(double lat, double lon)
{
    route_cur_lat = lat;
    route_cur_lon = lon;
    route_has_pos = true;
}

/********************************************************
 * 路线更新显示
 * 有当前进度位置时: 已走过段灰显, 未走过段高德蓝
 ********************************************************/
void map_route_update(void)
{
    if (route_line == NULL) {
        return;
    }
    if (current_route_count <= 0) {
        return;
    }

    int valid = 0;
    for (int i = 0; i < current_route_count; i++) {
        double lat = route_lat[i] / 10000000.0;
        double lon = route_lon[i] / 10000000.0;
        int x, y;

        bool ret = map_tiles_gps_to_screen(lat, lon, &x, &y);
        if (ret) {
            if (valid < ROUTE_MAX_POINT) {
                route_points[valid].x = x;
                route_points[valid].y = y;
                valid++;
            }
        }
    }

    if (valid <= 1) {
        return;
    }

    /* 无进度位置: 整条高德蓝 */
    if (!route_has_pos) {
        lv_line_set_points(route_line, route_points, valid);
        if (route_line_done != NULL) {
            lv_line_set_points(route_line_done, route_points, 0);
        }
        return;
    }

    /* 有进度位置: 找当前屏幕位置到路线折线上的最近投影点 */
    int px, py;
    if (!map_tiles_gps_to_screen(route_cur_lat, route_cur_lon, &px, &py)) {
        lv_line_set_points(route_line, route_points, valid);
        return;
    }

    int split_idx = -1;
    double split_x = px, split_y = py;
    double best_d2 = 1e18;
    for (int i = 0; i < valid - 1; i++) {
        double ax = route_points[i].x, ay = route_points[i].y;
        double bx = route_points[i + 1].x, by = route_points[i + 1].y;
        double dx = bx - ax, dy = by - ay;
        double len2 = dx * dx + dy * dy;
        double t = 0.0;
        if (len2 > 0.0) {
            t = ((px - ax) * dx + (py - ay) * dy) / len2;
            if (t < 0.0) {
                t = 0.0;
            } else if (t > 1.0) {
                t = 1.0;
            }
        }
        double fx = ax + t * dx, fy = ay + t * dy;
        double ddx = px - fx, ddy = py - fy;
        double d2 = ddx * ddx + ddy * ddy;
        if (d2 < best_d2) {
            best_d2 = d2;
            split_idx = i;
            split_x = fx;
            split_y = fy;
        }
    }

    if (split_idx < 0) {
        lv_line_set_points(route_line, route_points, valid);
        return;
    }

    /* 已走过段: [0 .. split_idx] + 投影点 */
    int dn = 0;
    for (int i = 0; i <= split_idx && dn < ROUTE_MAX_POINT; i++) {
        route_points_done[dn].x = route_points[i].x;
        route_points_done[dn].y = route_points[i].y;
        dn++;
    }
    if (dn < ROUTE_MAX_POINT) {
        route_points_done[dn].x = split_x;
        route_points_done[dn].y = split_y;
        dn++;
    }
    if (route_line_done != NULL) {
        lv_line_set_points(route_line_done, route_points_done, dn > 1 ? dn : 0);
    }

    /* 未走过段: 投影点 + [split_idx+1 .. valid-1] */
    int tn = 0;
    if (tn < ROUTE_MAX_POINT) {
        route_points_todo[tn].x = split_x;
        route_points_todo[tn].y = split_y;
        tn++;
    }
    for (int i = split_idx + 1; i < valid && tn < ROUTE_MAX_POINT; i++) {
        route_points_todo[tn].x = route_points[i].x;
        route_points_todo[tn].y = route_points[i].y;
        tn++;
    }
    lv_line_set_points(route_line, route_points_todo, tn > 1 ? tn : 0);
}

/********************************************************
 * 当前路线节点数
 ********************************************************/
int map_route_get_count(void)
{
    return current_route_count;
}

/********************************************************
 * 获取路线第 idx 个节点经纬度
 ********************************************************/
bool map_route_get_point(int idx, double *lat, double *lon)
{
    if (idx < 0 || idx >= current_route_count) {
        return false;
    }
    if (lat) {
        *lat = route_lat[idx] / 10000000.0;
    }
    if (lon) {
        *lon = route_lon[idx] / 10000000.0;
    }
    return true;
}

/********************************************************
 * 获取路线段 idx→idx+1 的路名 id 与长度(cm)
 * 在图中正向查找从 current_route[idx] 连到 current_route[idx+1] 的边
 ********************************************************/
bool map_route_get_segment(int idx, uint16_t *name_id, uint32_t *dist_cm)
{
    if (idx < 0 || idx + 1 >= current_route_count) {
        return false;
    }

    graph_node_id_t *a = &current_route[idx];
    graph_node_id_t *b = &current_route[idx + 1];

    graph_tile_t *tile = graph_tile_load(a->tile_x, a->tile_y);
    if (tile == NULL || a->node_id >= tile->node_count) {
        return false;
    }

    graph_node_t *node = &tile->nodes[a->node_id];
    for (uint32_t i = 0; i < node->edge_count; i++) {
        uint32_t ei = node->edge_start + i;
        if (ei >= tile->edge_count) {
            continue;
        }
        graph_edge_t *e = &tile->edges[ei];

        int16_t tx = e->target_tile_x;
        int16_t ty = e->target_tile_y;
        if (tx == 0 && ty == 0) {
            tx = a->tile_x;
            ty = a->tile_y;
        }
        if (tx == b->tile_x && ty == b->tile_y &&
            e->target_node == b->node_id) {
            if (name_id) {
                *name_id = e->name_id;
            }
            if (dist_cm) {
                *dist_cm = e->distance;
            }
            return true;
        }
    }
    return false;
}
