#include "nav_guide.h"
#include "map_route.h"
#include "graph_tile.h"
#include "nav_hud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* 与 map_route.c 相同的 PSRAM bss 段 */
#define AT_PSRAM_BSS __attribute__((section(".psram_bss")))

#define NAV_GUIDE_MAX 5000

/*
 * 转向判定阈值 (角度)
 *   |转角| > 150°  -> 掉头
 *   |转角| >  60°  -> 大转 (左/右)
 *   |转角| >  30°  -> 缓转 (缓左/缓右)
 *   否则           -> 直行
 */
#define TURN_UTURN_DEG   150
#define TURN_FULL_DEG     60
#define TURN_SLIGHT_DEG   30

/* 距终点小于该距离(米)判定为到达 */
#define ARRIVE_DIST_M     30

/*
 * 转向判定用"前后 LOOKAHEAD 米的方向差"来算, 而不是相邻两节点的夹角。
 * 这样弧线弯道(平滑曲线, 单节点夹角小)也能被识别成一次转弯。
 */
#define TURN_LOOKAHEAD_CM  3000   /* 30 米 */
#define TURN_MERGE_CM      4000   /* 相邻转向点合并距离(40米), 弧线上只留转角最大的点 */

/*
 * 若上机发现左右反了, 把这里改成 -1
 * 约定: 方位角北=0 顺时针增加, 转角为负 = 逆时针 = 左转
 */
#define TURN_SIGN 1

/* 每节点累计里程 (cm), 起点=0 */
static int32_t cum_dist[NAV_GUIDE_MAX] AT_PSRAM_BSS;
/* 到达节点 i 时的转向 (i=0 恒为直行) */
static uint8_t turn_at[NAV_GUIDE_MAX] AT_PSRAM_BSS;
/* 节点 i 处的有符号转角(度), 用于弧线合并 */
static int16_t turn_deg[NAV_GUIDE_MAX] AT_PSRAM_BSS;
/* 段 i→i+1 的道路名 id */
static uint16_t seg_name[NAV_GUIDE_MAX] AT_PSRAM_BSS;

static int route_n = 0;
static int cursor = 0;          /* 地图匹配游标(所在段), 单调前移防回退 */
static uint16_t speed_kmh = 25;

/* 方位角: 北=0, 顺时针 (返回 0~359) */
static int16_t bearing(double lat1, double lon1, double lat2, double lon2)
{
    double dy = lat2 - lat1;
    double dx = lon2 - lon1;
    double a = atan2(dx, dy) * 180.0 / M_PI;
    int r = (int)(a + (a >= 0 ? 0.5 : -0.5));
    if (r < 0) {
        r += 360;
    }
    return (int16_t)r;
}

/* 由转角(度, 已乘 TURN_SIGN, 归一化到 [-180,180])判定转向类型 */
static uint8_t classify_turn_by_deg(int16_t d)
{
    int ad = d < 0 ? -d : d;
    if (ad >= TURN_UTURN_DEG) {
        return NAV_TURN_UTURN;
    }
    if (d >= TURN_FULL_DEG) {
        return NAV_TURN_RIGHT;
    }
    if (d >= TURN_SLIGHT_DEG) {
        return NAV_TURN_SLIGHT_RIGHT;
    }
    if (d <= -TURN_FULL_DEG) {
        return NAV_TURN_LEFT;
    }
    if (d <= -TURN_SLIGHT_DEG) {
        return NAV_TURN_SLIGHT_LEFT;
    }
    return NAV_TURN_STRAIGHT;
}

void nav_guide_build(void)
{
    route_n = map_route_get_count();
    cursor = 0;

    if (route_n < 2) {
        return;
    }

    /* 第一遍: 累计里程 + 段名 */
    cum_dist[0] = 0;
    turn_at[0] = NAV_TURN_STRAIGHT;
    seg_name[0] = 0;

    for (int i = 1; i < route_n; i++) {
        uint16_t nm = 0;
        uint32_t d = 0;
        map_route_get_segment(i - 1, &nm, &d);
        cum_dist[i] = cum_dist[i - 1] + d;
        seg_name[i - 1] = nm;
        turn_at[i] = NAV_TURN_STRAIGHT;
    }
    seg_name[route_n - 1] = 0;

    /* 第二遍: 用"前后 LOOKAHEAD 米方向差"算每个节点的转角, 平滑弧线弯道 */
    for (int i = 1; i < route_n - 1; i++) {
        /* 后方节点 h: 距 i 至少 LOOKAHEAD 米 (不够则用起点) */
        int h = i - 1;
        while (h > 0 && (cum_dist[i] - cum_dist[h]) < TURN_LOOKAHEAD_CM) {
            h--;
        }
        /* 前方节点 j: 距 i 至少 LOOKAHEAD 米 (不够则用终点) */
        int j = i + 1;
        while (j < route_n - 1 && (cum_dist[j] - cum_dist[i]) < TURN_LOOKAHEAD_CM) {
            j++;
        }

        double ah, oh, ai, oi, aj, oj;
        if (!map_route_get_point(h, &ah, &oh) ||
            !map_route_get_point(i, &ai, &oi) ||
            !map_route_get_point(j, &aj, &oj)) {
            turn_deg[i] = 0;
            continue;
        }

        int16_t in  = bearing(ah, oh, ai, oi);
        int16_t out = bearing(ai, oi, aj, oj);
        int d = out - in;
        if (d > 180) {
            d -= 360;
        }
        if (d < -180) {
            d += 360;
        }
        turn_deg[i] = (int16_t)(d * TURN_SIGN);
    }

    /* 第三遍: 弧线合并——连续的转向节点只留转角最大的那个(峰值检测) */
    for (int i = 1; i < route_n - 1; i++) {
        int ad = turn_deg[i] < 0 ? -turn_deg[i] : turn_deg[i];
        if (ad < TURN_SLIGHT_DEG) {
            continue;
        }
        /* 前后 TURN_MERGE_CM 距离内若有转角更大的节点, 本节点让位 */
        bool peak = true;
        for (int k = i - 1; k > 0 && (cum_dist[i] - cum_dist[k]) < TURN_MERGE_CM; k--) {
            int adk = turn_deg[k] < 0 ? -turn_deg[k] : turn_deg[k];
            if (adk > ad) {
                peak = false;
                break;
            }
        }
        if (peak) {
            for (int k = i + 1; k < route_n - 1 && (cum_dist[k] - cum_dist[i]) < TURN_MERGE_CM; k++) {
                int adk = turn_deg[k] < 0 ? -turn_deg[k] : turn_deg[k];
                if (adk > ad) {
                    peak = false;
                    break;
                }
            }
        }
        if (!peak) {
            turn_deg[i] = 0;
        }
    }

    /* 第四遍: 由转角生成转向类型 */
    for (int i = 1; i < route_n - 1; i++) {
        turn_at[i] = classify_turn_by_deg(turn_deg[i]);
    }

    printf("nav_guide: built %d nodes, total %.1f m\n",
           route_n, cum_dist[route_n - 1] / 100.0);
}

void nav_guide_clear(void)
{
    route_n = 0;
    cursor = 0;
}

void nav_guide_set_speed(uint16_t kmh)
{
    speed_kmh = kmh;
}

/*
 * 点到段 (i→i+1) 的垂足投影, 返回垂足到点的距离(米),
 * 并把垂足在段内的偏移比例写入 *t (0~1)。
 * 平面近似, 与 map_route_total_distance_m 同一套系数。
 */
static double point_to_segment(double lat, double lon, int i, double *t)
{
    double a1, o1, a2, o2;
    map_route_get_point(i, &a1, &o1);
    map_route_get_point(i + 1, &a2, &o2);

    double cos_lat = cos(lat * M_PI / 180.0);
    double x = lon, y = lat;
    double x1 = o1, y1 = a1;
    double x2 = o2, y2 = a2;

    double dx = x2 - x1;
    double dy = y2 - y1;
    double len2 = dx * dx + dy * dy;
    if (len2 <= 0.0) {
        *t = 0.0;
        /* 退化为点距离 */
        double ddx = (x - x1) * 111000.0 * cos_lat;
        double ddy = (y - y1) * 111000.0;
        return sqrt(ddx * ddx + ddy * ddy);
    }

    double tt = ((x - x1) * dx + (y - y1) * dy) / len2;
    if (tt < 0.0) {
        tt = 0.0;
    } else if (tt > 1.0) {
        tt = 1.0;
    }
    *t = tt;

    double px = x1 + tt * dx;
    double py = y1 + tt * dy;
    double ddx = (x - px) * 111000.0 * cos_lat;
    double ddy = (y - py) * 111000.0;
    return sqrt(ddx * ddx + ddy * ddy);
}

void nav_guide_update(int32_t lat_e7, int32_t lon_e7)
{
    if (route_n < 2) {
        return;
    }

    double lat = lat_e7 / 10000000.0;
    double lon = lon_e7 / 10000000.0;

    /*
     * 地图匹配: 从游标向后找最近段 (前瞻一段即可, 不回溯)。
     * 找不到更近的段时保持游标不动, 用最近的一段。
     */
    int best = -1;
    double best_perp = 1e18;
    double best_t = 0.0;

    int end = route_n - 1;
    if (cursor + 8 < end) {
        end = cursor + 8;
    }
    for (int i = cursor; i < end; i++) {
        double t = 0.0;
        double perp = point_to_segment(lat, lon, i, &t);
        if (perp < best_perp) {
            best_perp = perp;
            best = i;
            best_t = t;
        }
    }

    if (best < 0) {
        return;
    }
    cursor = best;

    /* 当前位置在整条路线上的累计里程 (cm) */
    int32_t seg_len = cum_dist[best + 1] - cum_dist[best];
    int32_t cur_cm = cum_dist[best] + (int32_t)(best_t * seg_len);

    /* 剩余里程 */
    int32_t remain_cm = cum_dist[route_n - 1] - cur_cm;
    if (remain_cm < 0) {
        remain_cm = 0;
    }

    /* 当前道路名 */
    char road[128];
    road[0] = 0;
    if (seg_name[best] != 0) {
        graph_name_lookup(seg_name[best], road, sizeof(road));
    }

    /* 找下一个非直行节点 */
    int next = -1;
    for (int i = best + 1; i < route_n; i++) {
        if (turn_at[i] != NAV_TURN_STRAIGHT) {
            next = i;
            break;
        }
    }

    /* 到达判定 */
    if (remain_cm < (int32_t)ARRIVE_DIST_M * 100) {
        nav_hud_set_turn(NAV_TURN_ARRIVE, 0, road);
    } else if (next >= 0) {
        int32_t dist_cm = cum_dist[next] - cur_cm;
        if (dist_cm < 0) {
            dist_cm = 0;
        }
        int32_t dist_m = dist_cm / 100;
        if (dist_m <= NAV_GUIDE_TURN_PROMPT_M) {
            /* 进入转弯提示范围: 显示转向箭头 + 距离 */
            nav_hud_set_turn((nav_turn_type_t)turn_at[next], (int)dist_m, road);
        } else {
            /* 距转弯太远: 不显示转向, 只显示当前道路名 */
            nav_hud_set_turn(NAV_TURN_STRAIGHT, 0, road);
        }
    } else {
        /* 一路直行到终点 */
        nav_hud_set_turn(NAV_TURN_STRAIGHT, (int)(remain_cm / 100), road);
    }

    /* 剩余距离 + 预计时间 */
    float remain_km = remain_cm / 100000.0f;
    int remain_min = speed_kmh > 0
                     ? (int)(remain_km / speed_kmh * 60.0f) : 0;
    nav_hud_set_remain(remain_km, remain_min);

    /* 更新路线进度位置, 抹除已走过路段 */
    map_route_set_current_pos(lat, lon);
    map_route_update();
}

#if NAV_GUIDE_SIM
/*
 * 模拟导航: 让当前位置沿路线按 NAV_GUIDE_SIM_SPEED_KMH 前进,
 * 无需真实 GPS。用于无定位环境下的转向/路名联调。
 */
static double sim_progress_cm = 0.0;   /* 模拟位置在路线上的累计里程 (cm) */
static uint32_t sim_last_tick = 0;

void nav_guide_sim_reset(void)
{
    sim_progress_cm = 0.0;
    sim_last_tick = 0;
}

/* 按累计里程插值出路线上的经纬度 */
static bool sim_latlon_at(double progress_cm, double *lat, double *lon)
{
    if (route_n < 2) {
        return false;
    }

    int i;
    for (i = 0; i < route_n - 1; i++) {
        if (progress_cm <= (double)cum_dist[i + 1]) {
            break;
        }
    }
    if (i >= route_n - 1) {
        i = route_n - 2;
        progress_cm = (double)cum_dist[route_n - 1];
    }

    double a1, o1, a2, o2;
    if (!map_route_get_point(i, &a1, &o1) ||
        !map_route_get_point(i + 1, &a2, &o2)) {
        return false;
    }

    double seg = (double)(cum_dist[i + 1] - cum_dist[i]);
    double t = seg > 0.0 ? (progress_cm - (double)cum_dist[i]) / seg : 0.0;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }

    *lat = a1 + (a2 - a1) * t;
    *lon = o1 + (o2 - o1) * t;
    return true;
}

bool nav_guide_sim_tick(int32_t *lat_e7, int32_t *lon_e7)
{
    if (route_n < 2) {
        return false;
    }

    uint32_t now = lv_tick_get();
    if (sim_last_tick == 0) {
        sim_last_tick = now;
    }
    double dt = (double)(now - sim_last_tick) / 1000.0;
    sim_last_tick = now;

    /* 前进: km/h -> cm */
    double advance_cm = (double)NAV_GUIDE_SIM_SPEED_KMH * 100000.0 / 3600.0 * dt;
    sim_progress_cm += advance_cm;

    double total = (double)cum_dist[route_n - 1];
    bool arrived = sim_progress_cm >= total;
    if (arrived) {
        sim_progress_cm = total;
    }

    double lat, lon;
    if (!sim_latlon_at(sim_progress_cm, &lat, &lon)) {
        return false;
    }

    if (lat_e7) {
        *lat_e7 = (int32_t)(lat * 10000000.0);
    }
    if (lon_e7) {
        *lon_e7 = (int32_t)(lon * 10000000.0);
    }

    nav_guide_update((int32_t)(lat * 10000000.0), (int32_t)(lon * 10000000.0));

    return !arrived;
}

int16_t nav_guide_sim_heading(void)
{
    if (route_n < 2) {
        return 0;
    }

    /* 找到当前模拟位置所在的段 */
    int i;
    for (i = 0; i < route_n - 1; i++) {
        if (sim_progress_cm <= (double)cum_dist[i + 1]) {
            break;
        }
    }
    if (i >= route_n - 1) {
        i = route_n - 2;
    }

    double a1, o1, a2, o2;
    if (!map_route_get_point(i, &a1, &o1) ||
        !map_route_get_point(i + 1, &a2, &o2)) {
        return 0;
    }
    return bearing(a1, o1, a2, o2);
}
#endif
