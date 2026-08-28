#include "lv_conf.h"
#if LV_USE_DEMO_MAP
#include "map_tiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define BOOL_DEFINE_CONFLICT       // GPS驱动通过cpu.h定义bool，与LVGL的stdbool.h冲突
#include "gps_driver.h"
#include "map_route.h"
#include "nav_hud.h"
#include "nav_guide.h"
static const char *TAG = "map_tiles";
void lv_image_cache_print_usage(void);
extern void psram_heap_stats(void);
extern void sdmmc_dev_lowpower_disable(char *sdx, u32 disable);

/* LVGL 线程投递接口 (见 lvgl_v9_main.c), 用于跨线程安全地操作 LVGL */
extern int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);

/* GPS 定位超时时间 (ms) */
#define GPS_LOCATE_TIMEOUT_MS  15000
/* GPS 轮询间隔 (ms) */
#define GPS_POLL_INTERVAL_MS   200

// Internal structure for map tiles instance
struct map_tiles_t {
    // Configuration
    char *base_path;
    char *tile_folders[MAP_TILES_MAX_TYPES];
    int tile_type_count;
    int current_tile_type;
    int grid_cols;
    int grid_rows;
    int tile_count;
    int zoom;
    bool use_spiram;
    bool initialized;

    // Tile management
    int tile_x;
    int tile_y;
    int marker_offset_x;
    int marker_offset_y;
    bool tile_loading_error;
};

map_tiles_handle_t map_tiles_init(const map_tiles_config_t *config)
{
    if (!config || !config->base_path || config->tile_type_count <= 0 ||
        config->tile_type_count > MAP_TILES_MAX_TYPES ||
        config->default_tile_type < 0 || config->default_tile_type >= config->tile_type_count) {
        return NULL;
    }

    // Validate grid size - use defaults if not specified or invalid
    int grid_cols = config->grid_cols;
    int grid_rows = config->grid_rows;

    if (grid_cols <= 0 || grid_cols > MAP_TILES_MAX_GRID_COLS) {
        grid_cols = MAP_TILES_DEFAULT_GRID_COLS;
    }

    if (grid_rows <= 0 || grid_rows > MAP_TILES_MAX_GRID_ROWS) {
        grid_rows = MAP_TILES_DEFAULT_GRID_ROWS;
    }

    int tile_count = grid_cols * grid_rows;

    // Validate that all tile folders are provided
    for (int i = 0; i < config->tile_type_count; i++) {
        if (!config->tile_folders[i]) {
            return NULL;
        }
    }

    map_tiles_handle_t handle = (map_tiles_handle_t)lv_calloc(1, sizeof(struct map_tiles_t));
    if (!handle) {
        return NULL;
    }

    // Copy base path
    handle->base_path = lv_strdup(config->base_path);
    if (!handle->base_path) {
        lv_free(handle);
        return NULL;
    }

    // Copy tile folder names
    handle->tile_type_count = config->tile_type_count;
    for (int i = 0; i < config->tile_type_count; i++) {
        handle->tile_folders[i] = lv_strdup(config->tile_folders[i]);
        if (!handle->tile_folders[i]) {
            for (int j = 0; j < i; j++) {
                lv_free(handle->tile_folders[j]);
            }
            lv_free(handle->base_path);
            lv_free(handle);
            return NULL;
        }
    }

    handle->zoom = config->default_zoom;
    handle->use_spiram = config->use_spiram;
    handle->current_tile_type = config->default_tile_type;
    handle->grid_cols = grid_cols;
    handle->grid_rows = grid_rows;
    handle->tile_count = tile_count;
    handle->initialized = true;
    handle->tile_loading_error = false;

    return handle;
}

void map_tiles_set_zoom(map_tiles_handle_t handle, int zoom_level)
{
    if (!handle || !handle->initialized) {
        return;
    }

    handle->zoom = zoom_level;
}

int map_tiles_get_zoom(map_tiles_handle_t handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    return handle->zoom;
}

bool map_tiles_set_tile_type(map_tiles_handle_t handle, int tile_type)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    if (tile_type < 0 || tile_type >= handle->tile_type_count) {
        return false;
    }

    handle->current_tile_type = tile_type;
    return true;
}

int map_tiles_get_tile_type(map_tiles_handle_t handle)
{
    if (!handle || !handle->initialized) {
        return -1;
    }

    return handle->current_tile_type;
}

int map_tiles_get_tile_type_count(map_tiles_handle_t handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    return handle->tile_type_count;
}

const char *map_tiles_get_tile_type_folder(map_tiles_handle_t handle, int tile_type)
{
    if (!handle || !handle->initialized) {
        return NULL;
    }

    if (tile_type < 0 || tile_type >= handle->tile_type_count) {
        return NULL;
    }

    return handle->tile_folders[tile_type];
}

bool map_tiles_get_tile_path(map_tiles_handle_t handle, int tile_x, int tile_y,
                             char *path_buf, size_t buf_size)
{
    if (!handle || !handle->initialized || !path_buf || buf_size == 0) {
        return false;
    }

    const char *folder = handle->tile_folders[handle->current_tile_type];
    snprintf(path_buf, buf_size, "%s/%s/%d/%d/%d.zip",
             handle->base_path, folder, handle->zoom, tile_x, tile_y);
    return true;
}

void map_tiles_gps_to_tile_xy(map_tiles_handle_t handle, double lat, double lon, double *x, double *y)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (!x || !y) {
        return;
    }

    double lat_rad = lat * M_PI / 180.0;
    int n = 1 << handle->zoom;
    *x = (lon + 180.0) / 360.0 * n;
    *y = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * n;
}

void map_tiles_tile_xy_to_gps(map_tiles_handle_t handle, double x, double y, double *lat, double *lon)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (!lat || !lon) {
        return;
    }

    int n = 1 << handle->zoom;
    *lon = x / n * 360.0 - 180.0;
    double lat_rad = atan(sinh(M_PI * (1 - 2 * y / n)));
    *lat = lat_rad * 180.0 / M_PI;
}

void map_tiles_get_center_gps(map_tiles_handle_t handle, double *lat, double *lon)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (!lat || !lon) {
        return;
    }

    double center_x = handle->tile_x + handle->grid_cols / 2.0;
    double center_y = handle->tile_y + handle->grid_rows / 2.0;

    map_tiles_tile_xy_to_gps(handle, center_x, center_y, lat, lon);
}

void map_tiles_set_center_from_gps(map_tiles_handle_t handle, double lat, double lon)
{
    if (!handle || !handle->initialized) {
        return;
    }

    double x, y;
    map_tiles_gps_to_tile_xy(handle, lat, lon, &x, &y);

    handle->tile_x = (int)x - handle->grid_cols / 2;
    handle->tile_y = (int)y - handle->grid_rows / 2;

    handle->marker_offset_x = (int)((x - (int)x) * MAP_TILES_TILE_SIZE);
    handle->marker_offset_y = (int)((y - (int)y) * MAP_TILES_TILE_SIZE);
}

bool map_tiles_is_gps_within_tiles(map_tiles_handle_t handle, double lat, double lon)
{
    if (!handle || !handle->initialized) {
        return false;
    }

    double x, y;
    map_tiles_gps_to_tile_xy(handle, lat, lon, &x, &y);

    int gps_tile_x = (int)x;
    int gps_tile_y = (int)y;

    bool within_x = (gps_tile_x >= handle->tile_x && gps_tile_x < handle->tile_x + handle->grid_cols);
    bool within_y = (gps_tile_y >= handle->tile_y && gps_tile_y < handle->tile_y + handle->grid_rows);

    return within_x && within_y;
}

void map_tiles_get_position(map_tiles_handle_t handle, int *tile_x, int *tile_y)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (tile_x) {
        *tile_x = handle->tile_x;
    }
    if (tile_y) {
        *tile_y = handle->tile_y;
    }
}

void map_tiles_set_position(map_tiles_handle_t handle, int tile_x, int tile_y)
{
    if (!handle || !handle->initialized) {
        return;
    }

    handle->tile_x = tile_x;
    handle->tile_y = tile_y;
}

void map_tiles_get_marker_offset(map_tiles_handle_t handle, int *offset_x, int *offset_y)
{
    if (!handle || !handle->initialized) {
        return;
    }

    if (offset_x) {
        *offset_x = handle->marker_offset_x;
    }
    if (offset_y) {
        *offset_y = handle->marker_offset_y;
    }
}

void map_tiles_set_marker_offset(map_tiles_handle_t handle, int offset_x, int offset_y)
{
    if (!handle || !handle->initialized) {
        return;
    }

    handle->marker_offset_x = offset_x;
    handle->marker_offset_y = offset_y;
}

void map_tiles_set_loading_error(map_tiles_handle_t handle, bool error)
{
    if (!handle || !handle->initialized) {
        return;
    }

    handle->tile_loading_error = error;
}

bool map_tiles_has_loading_error(map_tiles_handle_t handle)
{
    if (!handle || !handle->initialized) {
        return true;
    }

    return handle->tile_loading_error;
}

void map_tiles_cleanup(map_tiles_handle_t handle)
{
    if (!handle) {
        return;
    }

    if (handle->initialized) {
        handle->initialized = false;
    }

    if (handle->base_path) {
        lv_free(handle->base_path);
    }
    for (int i = 0; i < handle->tile_type_count; i++) {
        if (handle->tile_folders[i]) {
            lv_free(handle->tile_folders[i]);
        }
    }
    lv_free(handle);
}

void map_tiles_get_grid_size(map_tiles_handle_t handle, int *cols, int *rows)
{
    if (!handle || !handle->initialized || !cols || !rows) {
        if (cols) {
            *cols = 0;
        }
        if (rows) {
            *rows = 0;
        }
        return;
    }

    *cols = handle->grid_cols;
    *rows = handle->grid_rows;
}

int map_tiles_get_tile_count(map_tiles_handle_t handle)
{
    if (!handle || !handle->initialized) {
        return 0;
    }

    return handle->tile_count;
}

int lt = MAP_TILE_GRID_N;
int wt = MAP_TILE_GRID_N;
lv_obj_t *canvas2590;
lv_obj_t *img[MAP_TILE_GRID_N * MAP_TILE_GRID_N];
map_tiles_handle_t map = NULL;
double center_lat = 38.0626;
double center_lon = -122.2211;
lv_point_t last_point;
double start_lat;
double start_lon;
bool fr = 1;

/* GPS 定位按钮相关状态 */
static lv_obj_t *gps_marker = NULL;        /* 当前位置圆点(lv_obj, 叠在箭头上) */
static lv_obj_t *gps_marker_tri = NULL;    /* 方向箭头实心三角形(lv_canvas, 在圆点之下) */
static lv_obj_t *gps_status_label = NULL;  /* 定位状态提示 */
static lv_timer_t *gps_locate_timer = NULL;
static lv_timer_t *gps_status_hide_timer = NULL;  /* 成功后自动隐藏提示的定时器 */
static uint32_t gps_locate_start_tick = 0;
static bool gps_inited = false;
static bool gps_located = false;
static double gps_located_lat = 0.0;
static double gps_located_lon = 0.0;

/* 导航中 GPS 跟踪定时器 (持续轮询, 驱动转向引导) */
static lv_timer_t *nav_track_timer = NULL;
static double nav_last_lat = 0.0;
static double nav_last_lon = 0.0;
static uint32_t nav_last_tick = 0;

/* 离线导航 (蓝牙 stream_media 下发) 数据: BT 线程写入, LVGL 线程读取 */
static int32_t s_off_start_lon_e6 = 0;
static int32_t s_off_start_lat_e6 = 0;
static int32_t s_off_end_lon_e6 = 0;
static int32_t s_off_end_lat_e6 = 0;
static char s_off_start_name[64];
static char s_off_end_name[64];
static void map_offline_nav_rpc(void);

/* 模拟导航当前位置 (供地图拖动时刷新图标, 让图标钉在终点而非跟随手指) */
static bool sim_has_pos = false;
static double sim_cur_lat = 0.0;
static double sim_cur_lon = 0.0;
static int16_t sim_cur_heading = -1;

/* 地图中心锁定: 锁定后地图中心始终跟随 GPS/模拟位置 (默认开启) */
static bool center_lock = true;
static lv_obj_t *btn_center_lock = NULL;   /* 锁定按钮 */
static lv_obj_t *s_lock_body = NULL;       /* 锁体 (lv_obj) */
static lv_obj_t *s_lock_shackle = NULL;    /* 锁梁 (lv_line) */

/* 顶层按钮指针 (挂在 lv_scr_act 上, 销毁地图时一并删除) */
static lv_obj_t *btn0 = NULL;              /* 占位按钮 */
static lv_obj_t *btn_zoom_minus = NULL;    /* - */
static lv_obj_t *btn_zoom_plus = NULL;     /* + */
static lv_obj_t *btn_nav = NULL;           /* 导航面板 */
static lv_obj_t *btn_gps = NULL;           /* GPS 定位 */
static lv_obj_t *btn_destroy = NULL;       /* 销毁地图 */

/* 地图视觉旋转角(度, 顺时针为正), 0=北朝上。
 * 上锁时设为 -前进方向, 实现车头朝上; 解锁时归零。 */
static double map_rotation_deg = 0.0;

static void map_center_on(double lat, double lon);
static void center_lock_btn_cb(lv_event_t *e);

void map_zoom_minus_show1(lv_event_t *e)
{
    map_zoom_minus_show();
}
void map_zoom_plus_show1(lv_event_t *e)
{
    map_zoom_plus_show();
}

static void gps_locate_btn_cb(lv_event_t *e);
static void gps_marker_refresh(void);
static void gps_show_status(const char *text);
static void gps_marker_set_pos(double lat, double lon, int16_t heading_deg);
static void nav_track_timer_cb(lv_timer_t *timer);
static bool nav_start_common(double slat, double slon, double elat, double elon);

typedef struct {
    lv_obj_t *obj;
    int tile_x;
    int tile_y;
    bool used;
} tile_node_t;
tile_node_t node[MAP_TILE_GRID_N * MAP_TILE_GRID_N];
int start_touch_x;
int start_touch_y;
int cam_x = 0;
int cam_y = 0;
int cam_x_start;
int cam_y_start;
int tx, ty;
int center_offsetx;
int center_offsety;

/* 导航输入面板 */
static lv_obj_t *nav_panel = NULL;
static lv_obj_t *ta_start_lat, *ta_start_lon;
static lv_obj_t *ta_end_lat, *ta_end_lon;
static lv_obj_t *kb = NULL;

tile_node_t *find_node(int tx, int ty)
{
    for (int i = 0; i < MAP_TILE_GRID_N * MAP_TILE_GRID_N; i++) {
        if (node[i].tile_x == tx && node[i].tile_y == ty) {
            return &node[i];
        }
    }
    return NULL;
}
tile_node_t *find_unused()
{
    for (int i = 0; i < MAP_TILE_GRID_N * MAP_TILE_GRID_N; i++) {
        if (node[i].used == false) {
            return &node[i];
        }
    }
    return NULL;
}

/* 绕屏幕中心做视觉顺时针旋转 (与 lv_image_set_rotation 的正方向一致) */
static void map_screen_rotate(int x, int y, int *rx, int *ry)
{
    if (map_rotation_deg == 0.0) {
        *rx = x;
        *ry = y;
        return;
    }
    double cx = MAP_SCREEN_WIDTH / 2.0;
    double cy = MAP_SCREEN_HEIGHT / 2.0;
    double rad = map_rotation_deg * M_PI / 180.0;
    double dx = x - cx;
    double dy = y - cy;
    double c = cos(rad);
    double s = sin(rad);
    *rx = (int)(cx + dx * c - dy * s);
    *ry = (int)(cy + dx * s + dy * c);
}

/* 摆放一块瓦片: 瓦片中心绕屏幕中心旋转, 瓦片自身旋转同角度 */
static void map_tile_place(lv_obj_t *img, int top_x, int top_y)
{
    int cxp = top_x + 128;
    int cyp = top_y + 128;
    int rx, ry;
    map_screen_rotate(cxp, cyp, &rx, &ry);
    lv_image_set_rotation(img, (int32_t)(map_rotation_deg * 10.0));
    lv_obj_set_pos(img, rx - 128, ry - 128);
}

void map_update_view()
{
    int tile_size = 256;
    tile_node_t *p;
    int start_tile_x = (int)floor((double) - cam_x / tile_size);
    int start_tile_y = (int)floor((double) - cam_y / tile_size);
    int offset_x = cam_x - (-start_tile_x) * tile_size;
    int offset_y = cam_y - (-start_tile_y) * tile_size;
    for (int i = 0; i < lt * wt; i++) {
        node[i].used = false;
    }
    for (int y = 0; y < lt; y++) {
        for (int x = 0; x < wt; x++) {
            int tile_x = tx + start_tile_x + x;
            int tile_y = ty + start_tile_y + y;
            p = find_node(tile_x, tile_y);
            if (p != NULL) {
                p->used = true;
                map_tile_place(p->obj, x * tile_size + offset_x, y * tile_size + offset_y);
            }
        }
    }
    for (int y = 0; y < lt; y++) {
        for (int x = 0; x < wt; x++) {
            int tile_x = tx + start_tile_x + x;
            int tile_y = ty + start_tile_y + y;
            p = find_node(tile_x, tile_y);
            if (p == NULL) {
                p = find_unused();
                p->tile_x = tile_x;
                p->tile_y = tile_y;
                p->used = true;
                char path[256];
                map_tiles_get_tile_path(map, tile_x, tile_y, path, sizeof(path));
                lv_image_set_src(p->obj, path);
                map_tile_place(p->obj, x * tile_size + offset_x, y * tile_size + offset_y);
            }
        }
    }
    map_route_update();
    gps_marker_refresh();
    //lv_timer_handler();  // 跑一帧，触发渲染 → 解码 → 写入缓存
    psram_heap_stats();
    lv_image_cache_print_usage();
}

/* 地图中心锁定: 将指定经纬度平移到屏幕中心 (不重新加载瓦片) */
static void map_center_on(double lat, double lon)
{
    double twx, twy;
    map_tiles_gps_to_tile_xy(map, lat, lon, &twx, &twy);
    double world_x = twx * 256.0;
    double world_y = twy * 256.0;
    double origin_x = (double)tx * 256.0;
    double origin_y = (double)ty * 256.0;
    cam_x = (int)(MAP_SCREEN_WIDTH / 2.0 - (world_x - origin_x));
    cam_y = (int)(MAP_SCREEN_HEIGHT / 2.0 - (world_y - origin_y));
    map_update_view();
}

/* 锁梁顶点 (lv_line_set_points 只存指针, 必须用持久数组) */
static lv_point_precise_t lock_shackle_pts[4];

/* 锁定按钮: 点一下解锁(地图中心不再跟随), 再点一下重新锁定。
 * 用锁梁形状区分状态: 锁上=完整倒U, 解锁=左端翘起 */
static void center_lock_btn_cb(lv_event_t *e)
{
    (void)e;
    center_lock = !center_lock;

    if (!center_lock) {
        /* 解锁: 恢复北朝上 */
        map_rotation_deg = 0.0;
        map_update_view();
    }

    if (s_lock_shackle) {
        if (center_lock) {
            /* 锁上: 完整倒 U (相对锁图标容器 24x24) */
            lock_shackle_pts[0].x = 4;
            lock_shackle_pts[0].y = 9;
            lock_shackle_pts[1].x = 4;
            lock_shackle_pts[1].y = 1;
            lock_shackle_pts[2].x = 20;
            lock_shackle_pts[2].y = 1;
            lock_shackle_pts[3].x = 20;
            lock_shackle_pts[3].y = 9;
        } else {
            /* 解锁: 左端翘起 */
            lock_shackle_pts[0].x = 4;
            lock_shackle_pts[0].y = 2;
            lock_shackle_pts[1].x = 4;
            lock_shackle_pts[1].y = 1;
            lock_shackle_pts[2].x = 20;
            lock_shackle_pts[2].y = 1;
            lock_shackle_pts[3].x = 20;
            lock_shackle_pts[3].y = 9;
        }
        lv_line_set_points(s_lock_shackle, lock_shackle_pts, 4);
    }
}

/*
 * 销毁地图: 释放路线/道路缓存/map handle 及所有 LVGL 对象, 并重置状态。
 * 用于彻底卸载地图, 腾出 PSRAM/内存。
 */
void map_destroy(void)
{
    /* 1) 停掉所有定时器 */
    if (nav_track_timer) {
        lv_timer_del(nav_track_timer);
        nav_track_timer = NULL;
    }
    if (gps_locate_timer) {
        lv_timer_del(gps_locate_timer);
        gps_locate_timer = NULL;
    }
    if (gps_status_hide_timer) {
        lv_timer_del(gps_status_hide_timer);
        gps_status_hide_timer = NULL;
    }

    /* 2) 释放路线显示 + 道路缓存 (graph tile / names / graph.dat / index) */
    map_route_clear();
    nav_guide_clear();
    map_route_free();

    /* 3) 释放地图 handle (base_path / tile_folders / handle) */
    if (map) {
        map_tiles_cleanup(map);
        map = NULL;
    }

    /* 4) 删除所有 LVGL 对象 (canvas 会递归删掉 img / nav_hud / gps 图标) */
    if (canvas2590) {
        lv_obj_del(canvas2590);
        canvas2590 = NULL;
    }
    if (btn0)           {
        lv_obj_del(btn0);
        btn0 = NULL;
    }
    if (btn_zoom_minus) {
        lv_obj_del(btn_zoom_minus);
        btn_zoom_minus = NULL;
    }
    if (btn_zoom_plus)  {
        lv_obj_del(btn_zoom_plus);
        btn_zoom_plus = NULL;
    }
    if (btn_nav)        {
        lv_obj_del(btn_nav);
        btn_nav = NULL;
    }
    if (btn_gps)        {
        lv_obj_del(btn_gps);
        btn_gps = NULL;
    }
    if (btn_center_lock) {
        lv_obj_del(btn_center_lock);
        btn_center_lock = NULL;
    }
    if (btn_destroy)    {
        lv_obj_del(btn_destroy);
        btn_destroy = NULL;
    }
    if (nav_panel)      {
        lv_obj_del(nav_panel);
        nav_panel = NULL;
    }
    if (kb)             {
        lv_obj_del(kb);
        kb = NULL;
    }
    if (gps_status_label) {
        lv_obj_del(gps_status_label);
        gps_status_label = NULL;
    }

    /* 5) 重置状态, 便于再次 map_init */
    s_lock_body = NULL;
    s_lock_shackle = NULL;
    gps_marker = NULL;
    gps_marker_tri = NULL;
    fr = 1;                 /* 下次 map_show 重新加载路线 */
    center_lock = true;
    map_rotation_deg = 0.0;
    gps_inited = false;
    gps_located = false;
    sim_has_pos = false;
    nav_last_tick = 0;
    psram_heap_stats();
    printf("map: destroyed, memory released\n");
}

static void destroy_map_btn_cb(lv_event_t *e)
{
    (void)e;
    map_destroy();
}

void map_touch_begin(int x, int y)
{
    start_touch_x = x;
    start_touch_y = y;

    cam_x_start = cam_x;
    cam_y_start = cam_y;
}
void map_touch_update(int x, int y)
{
    int dx = x - start_touch_x;
    int dy = y - start_touch_y;

    cam_x = cam_x_start + dx;
    cam_y = cam_y_start + dy;

    map_update_view();
}
static void touch_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);
    switch (code) {
    case LV_EVENT_PRESSED:
        map_touch_begin(point.x, point.y);
        /* 仅当定位已结束(成功/超时)时, 拖动地图隐藏结果提示; 定位中保持显示 */
        if (!gps_locate_timer) {
            gps_show_status("");
        }
        break;
    case LV_EVENT_PRESSING:
        map_touch_update(point.x, point.y);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
    default:
        break;
    }
}

/********************************************************
 * 导航面板 — 按钮回调
 ********************************************************/
static void nav_panel_show_cb(lv_event_t *e)
{
    if (nav_panel) {
        lv_obj_clear_flag(nav_panel, LV_OBJ_FLAG_HIDDEN);
        /* 移到最顶层 */
        lv_obj_move_foreground(nav_panel);
    }
}

/* 导航输入面板消息标签 */
static lv_obj_t *nav_msg = NULL;

static void nav_panel_hide_cb(lv_event_t *e)
{
    if (nav_panel) {
        lv_obj_add_flag(nav_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (kb) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (nav_msg) {
        lv_label_set_text(nav_msg, "");
    }
}

/* 验证坐标字符串: 数字/负号/小数点, 不允许 . 开头 */
static bool is_valid_coord(const char *s)
{
    if (!s || !*s) {
        return false;
    }
    /* 首字符必须是数字或负号, 不能是小数点 */
    if (*s != '-' && (*s < '0' || *s > '9')) {
        return false;
    }
    int dots = 0;
    bool has_digit = (*s >= '0' && *s <= '9');
    for (const char *p = s + 1; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            has_digit = true;
            continue;
        }
        if (*p == '.') {
            if (++dots > 1) {
                return false;
            }
            continue;
        }
        return false;
    }
    return has_digit;
}

static void nav_start_cb(lv_event_t *e)
{
    const char *s1 = lv_textarea_get_text(ta_start_lat);
    const char *s2 = lv_textarea_get_text(ta_start_lon);
    const char *s3 = lv_textarea_get_text(ta_end_lat);
    const char *s4 = lv_textarea_get_text(ta_end_lon);

    /* 输入校验 */
    if (!is_valid_coord(s1) || !is_valid_coord(s2) ||
        !is_valid_coord(s3) || !is_valid_coord(s4)) {
        if (nav_msg) {
            lv_label_set_text(nav_msg, "Invalid input! Only: 0-9 . -");
        }
        return;
    }

    double slat = atof(s1);
    double slon = atof(s2);
    double elat = atof(s3);
    double elon = atof(s4);

    /* 范围校验: lat[-90, 90], lon[-180, 180] */
    if (slat < -90.0 || slat > 90.0 || elat < -90.0 || elat > 90.0) {
        if (nav_msg) {
            lv_label_set_text(nav_msg, "Latitude must be -90 ~ 90!");
        }
        return;
    }
    if (slon < -180.0 || slon > 180.0 || elon < -180.0 || elon > 180.0) {
        if (nav_msg) {
            lv_label_set_text(nav_msg, "Longitude must be -180 ~ 180!");
        }
        return;
    }

    printf("nav input: %.6f,%.6f -> %.6f,%.6f\n", slat, slon, elat, elon);

    /* 隐藏键盘, 面板保持显示直到导航完成 */
    if (kb) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (nav_msg) {
        lv_label_set_text(nav_msg, "Calculating...");
    }

    /* 强制刷新 UI 让提示文字先显示 */
    lv_refr_now(NULL);

    if (!nav_start_common(slat, slon, elat, elon)) {
        if (nav_msg) {
            if (map_route_is_timeout()) {
                lv_label_set_text(nav_msg, "Timeout! Route too long.");
            } else {
                lv_label_set_text(nav_msg, "Route failed! No path found.");
            }
        }
        return;
    }

    /* 导航成功 → 隐藏面板 */
    if (nav_panel) {
        lv_obj_add_flag(nav_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (nav_msg) {
        lv_label_set_text(nav_msg, "");
    }
}

/*
 * 导航启动公共流程: 清旧路线 -> 计算新路线 -> 建转向引导 -> 显示HUD -> 启动跟踪
 * 返回 false 表示路线规划失败(超时/无路径)。
 */
static bool nav_start_common(double slat, double slon, double elat, double elon)
{
    /* 清除旧路线, 并隐藏上一次的 HUD */
    map_route_clear();
    nav_guide_clear();
    nav_hud_hide();

    /* 停止上一次导航的 GPS 跟踪定时器 */
    if (nav_track_timer) {
        lv_timer_del(nav_track_timer);
        nav_track_timer = NULL;
    }

    /* 开始新导航 */
    if (!map_route_navigation(slat, slon, elat, elon)) {
        return false;
    }

    /* 预计算转向/里程, 并用路线起点立即算一次, 显示第一个转弯
     * (无需等 GPS/模拟推进到第一个点) */
    nav_guide_build();
    double first_lat, first_lon;
    if (map_route_get_point(0, &first_lat, &first_lon)) {
        nav_guide_update((int32_t)(first_lat * 10000000.0),
                         (int32_t)(first_lon * 10000000.0));
    }

    nav_guide_set_speed(NAV_GUIDE_SIM ? NAV_GUIDE_SIM_SPEED_KMH : 25);
    nav_hud_set_speed(NAV_GUIDE_SIM ? NAV_GUIDE_SIM_SPEED_KMH : 25);
    nav_hud_show();

#if NAV_GUIDE_SIM
    nav_guide_sim_reset();
#endif

    /* 启动 GPS 跟踪/模拟定时器 (持续轮询驱动转向引导) */
    if (!nav_track_timer) {
        if (!gps_inited) {
            gps_init();
            gps_inited = true;
        }
        nav_last_tick = 0;
        nav_track_timer = lv_timer_create(nav_track_timer_cb, GPS_POLL_INTERVAL_MS, NULL);
    }
    return true;
}

/* 离线导航实际执行 (运行于 LVGL 线程) */
static void map_offline_nav_rpc(void)
{
    double slat = s_off_start_lat_e6 / 1e6;
    double slon = s_off_start_lon_e6 / 1e6;
    double elat = s_off_end_lat_e6 / 1e6;
    double elon = s_off_end_lon_e6 / 1e6;

    printf("map: offline nav %.6f,%.6f -> %.6f,%.6f  \"%s\" -> \"%s\"\n",
           slat, slon, elat, elon, s_off_start_name, s_off_end_name);

    /* 地图上标出起终点名 */
    nav_hud_set_route_names(s_off_start_name, s_off_end_name);

    /* 隐藏面板并开始导航 */
    if (nav_panel) {
        lv_obj_add_flag(nav_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (nav_msg) {
        lv_label_set_text(nav_msg, "");
    }

    nav_start_common(slat, slon, elat, elon);
}

void map_offline_nav_start(int32_t start_lon_e6, int32_t start_lat_e6,
                           int32_t end_lon_e6, int32_t end_lat_e6,
                           const char *start_name, const char *end_name)
{
    /* BT 线程写入, LVGL 线程读取 */
    s_off_start_lon_e6 = start_lon_e6;
    s_off_start_lat_e6 = start_lat_e6;
    s_off_end_lon_e6 = end_lon_e6;
    s_off_end_lat_e6 = end_lat_e6;
    strncpy(s_off_start_name, start_name ? start_name : "", sizeof(s_off_start_name) - 1);
    s_off_start_name[sizeof(s_off_start_name) - 1] = '\0';
    strncpy(s_off_end_name, end_name ? end_name : "", sizeof(s_off_end_name) - 1);
    s_off_end_name[sizeof(s_off_end_name) - 1] = '\0';

    lvgl_rpc_post_func((void *)map_offline_nav_rpc, 0);
}

/*
 * 导航中 GPS 跟踪: 轮询定位 -> 估算车速 -> 更新转向引导
 * 模拟模式: 位置沿路线前进, 无需真实 GPS
 */
static void nav_track_timer_cb(lv_timer_t *timer)
{
    (void)timer;

#if NAV_GUIDE_SIM
    /* 模拟: 让当前位置图标沿路线走, 并驱动转向引导 */
    int32_t sim_lat, sim_lon;
    bool active = nav_guide_sim_tick(&sim_lat, &sim_lon);
    nav_hud_set_speed(NAV_GUIDE_SIM_SPEED_KMH);

    /* 记录模拟位置, 供地图拖动时刷新图标(钉在路线上, 不跟随手指) */
    sim_cur_lat = sim_lat / 10000000.0;
    sim_cur_lon = sim_lon / 10000000.0;
    sim_cur_heading = nav_guide_sim_heading();
    sim_has_pos = true;
    gps_marker_set_pos(sim_cur_lat, sim_cur_lon, sim_cur_heading);

    /* 地图中心跟随 + 车头朝上 */
    if (center_lock) {
        map_rotation_deg = -(double)sim_cur_heading;
        map_center_on(sim_cur_lat, sim_cur_lon);
    }

    if (!active) {
        return;   /* 已到终点, 停止推进(图标已钉在终点) */
    }
#else
    gps_poll();
    gps_data_t gps;
    gps_get_info(&gps);

    if (!gps.valid || gps.fix_mode <= 0) {
        return;   /* 无定位, 等下一轮 */
    }

    double lat = gps.latitude_e7 / 10000000.0;
    double lon = gps.longitude_e7 / 10000000.0;

    /* 由相邻两次定位估算车速 (km/h) + 前进方向 */
    uint32_t now = lv_tick_get();
    int16_t heading = -1;
    if (nav_last_tick != 0) {
        double dt = (double)(now - nav_last_tick) / 1000.0;
        if (dt > 0.0) {
            double cos_lat = cos(lat * M_PI / 180.0);
            double dx = (lat - nav_last_lat) * 111000.0;
            double dy = (lon - nav_last_lon) * 111000.0 * cos_lat;
            double dist_m = sqrt(dx * dx + dy * dy);
            uint16_t kmh = (uint16_t)(dist_m / dt * 3.6);
            if (kmh < 150) {   /* 过滤 GPS 跳变 */
                nav_guide_set_speed(kmh);
                nav_hud_set_speed(kmh);
            }
            if (dist_m > 0.5) {   /* 移动足够远才更新方向, 避免漂移 */
                double a = atan2(dx, dy) * 180.0 / M_PI;
                int r = (int)(a + (a >= 0 ? 0.5 : -0.5));
                if (r < 0) {
                    r += 360;
                }
                heading = (int16_t)r;
            }
        }
    }
    nav_last_lat = lat;
    nav_last_lon = lon;
    nav_last_tick = now;

    nav_guide_update(gps.latitude_e7, gps.longitude_e7);
    gps_marker_set_pos(lat, lon, heading);

    /* 地图中心跟随 + 车头朝上 */
    if (center_lock) {
        if (heading >= 0) {
            map_rotation_deg = -(double)heading;
        }
        map_center_on(lat, lon);
    }
#endif
}

/* 点击 textarea 时弹出键盘 */
static void ta_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
        if (kb) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(kb);
        }
    }
}

/* 键盘 ✓ 或点击外部时隐藏键盘 */
static void kb_ready_cb(lv_event_t *e)
{
    if (kb) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 辅助: 创建无滚动条的单行 textarea */
#define ROW_H 42

static lv_obj_t *nav_ta_create(lv_obj_t *parent)
{
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_scrollbar_mode(ta, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(ta, 180, ROW_H);
    lv_obj_add_event_cb(ta, ta_kb_event_cb, LV_EVENT_SHORT_CLICKED, NULL);
    return ta;
}

/* 统一按钮风格: 深灰半透明背景 + 圆角, 与提示框一致 */
static lv_obj_t *map_btn_create(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_set_pos(btn, x, y);

    if (text) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_center(label);
    }
    return btn;
}

void map_init()
{
    psram_heap_stats();

    canvas2590 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(canvas2590, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(canvas2590, MAP_SCREEN_WIDTH, MAP_SCREEN_HEIGHT);
    lv_obj_set_pos(canvas2590, 0, 0);
    btn0 = map_btn_create(lv_scr_act(), NULL, -810, -490);
    btn_zoom_minus = map_btn_create(lv_scr_act(), "-", MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 60);
    lv_obj_add_event_cb(btn_zoom_minus, map_zoom_minus_show1, LV_EVENT_SHORT_CLICKED, NULL);
    btn_zoom_plus = map_btn_create(lv_scr_act(), "+", MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 120);
    lv_obj_add_event_cb(btn_zoom_plus, map_zoom_plus_show1, LV_EVENT_SHORT_CLICKED, NULL);

    /* 导航按钮 (在 + - 上面) */
    btn_nav = map_btn_create(lv_scr_act(), "N", MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 180);
    lv_obj_add_event_cb(btn_nav, nav_panel_show_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 定位按钮 (获取当前GPS位置, 作为默认导航起点) */
    btn_gps = map_btn_create(lv_scr_act(), LV_SYMBOL_GPS, MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 240);
    lv_obj_add_event_cb(btn_gps, gps_locate_btn_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 地图中心锁定按钮 (定位按钮上面, 风格同其他按钮, 锁体+锁梁图标区分状态) */
    btn_center_lock = lv_btn_create(lv_scr_act());
    lv_obj_set_style_border_width(btn_center_lock, 0, 0);
    lv_obj_set_style_shadow_width(btn_center_lock, 0, 0);
    lv_obj_set_style_bg_color(btn_center_lock, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(btn_center_lock, LV_OPA_70, 0);
    lv_obj_set_style_radius(btn_center_lock, 6, 0);
    lv_obj_set_size(btn_center_lock, 50, 50);
    lv_obj_set_pos(btn_center_lock, MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 300);

    /* 锁图标容器: 透明, 精确居中到按钮, 锁体+锁梁都相对它定位 */
    lv_obj_t *lock_cont = lv_obj_create(btn_center_lock);
    lv_obj_set_size(lock_cont, 24, 24);
    lv_obj_set_style_pad_all(lock_cont, 0, 0);
    lv_obj_set_style_bg_opa(lock_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lock_cont, 0, 0);
    lv_obj_set_style_shadow_width(lock_cont, 0, 0);
    lv_obj_clear_flag(lock_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lock_cont, LV_OBJ_FLAG_CLICKABLE);   /* 不拦截触摸, 让按钮收到点击 */
    lv_obj_center(lock_cont);

    /* 锁体: 白色圆角矩形 (相对容器) */
    s_lock_body = lv_obj_create(lock_cont);
    lv_obj_set_size(s_lock_body, 20, 14);
    lv_obj_set_pos(s_lock_body, 2, 9);
    lv_obj_set_style_radius(s_lock_body, 3, 0);
    lv_obj_set_style_bg_color(s_lock_body, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(s_lock_body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_lock_body, 0, 0);
    lv_obj_set_style_pad_all(s_lock_body, 0, 0);
    lv_obj_set_style_shadow_width(s_lock_body, 0, 0);
    lv_obj_clear_flag(s_lock_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_lock_body, LV_OBJ_FLAG_CLICKABLE);  /* 不拦截触摸, 让按钮收到点击 */

    /* 锁梁: 白色倒 U (默认锁上, 相对容器) */
    s_lock_shackle = lv_line_create(lock_cont);
    lv_obj_set_style_line_width(s_lock_shackle, 3, 0);
    lv_obj_set_style_line_rounded(s_lock_shackle, true, 0);
    lv_obj_set_style_line_color(s_lock_shackle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(s_lock_shackle, 0, 0);
    lv_obj_set_pos(s_lock_shackle, 0, 0);
    lock_shackle_pts[0].x = 4;
    lock_shackle_pts[0].y = 9;
    lock_shackle_pts[1].x = 4;
    lock_shackle_pts[1].y = 1;
    lock_shackle_pts[2].x = 20;
    lock_shackle_pts[2].y = 1;
    lock_shackle_pts[3].x = 20;
    lock_shackle_pts[3].y = 9;
    lv_line_set_points(s_lock_shackle, lock_shackle_pts, 4);

    lv_obj_add_event_cb(btn_center_lock, center_lock_btn_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 销毁地图按钮 (锁定按钮上面): 卸载地图并释放所有相关内存 */
    btn_destroy = map_btn_create(lv_scr_act(), LV_SYMBOL_CLOSE, MAP_SCREEN_WIDTH - 60, MAP_SCREEN_HEIGHT - 360);
    lv_obj_add_event_cb(btn_destroy, destroy_map_btn_cb, LV_EVENT_SHORT_CLICKED, NULL);

    map_tiles_config_t cfg = {
        .base_path = "storage/sd1/C",
        .tile_folders = { "jlzip" },
        .tile_type_count = 1,
        .grid_cols = wt,
        .grid_rows = lt,
        .default_zoom = 10,
        .use_spiram = false,
        .default_tile_type = 0,
    };
    map = map_tiles_init(&cfg);
    lv_obj_add_event_cb(canvas2590, touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(canvas2590, touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(canvas2590, touch_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(canvas2590, touch_cb, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_set_style_pad_all(canvas2590, 0, 0);
    lv_obj_set_style_border_width(canvas2590, 0, 0);
    lv_obj_set_style_radius(canvas2590, 0, 0);
    lv_obj_set_style_shadow_width(canvas2590, 0, 0);
    lv_obj_set_style_bg_opa(canvas2590, LV_OPA_TRANSP, 0);
    for (int i = 0; i < lt * wt; i++) {
        img[i] = lv_image_create(canvas2590);
        node[i].obj = img[i];
        node[i].used = true;
    }
    /* ===== 导航 HUD (初始隐藏, 导航成功后显示) ===== */
    nav_hud_init(canvas2590);
    /* ===== 导航输入面板 (初始隐藏) ===== */
    nav_panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(nav_panel, 420, 420);
    lv_obj_set_pos(nav_panel, 10, 30);  /* 面板水平中心线(30+420/2=240)对齐地图水平中心线(480/2=240) */
    lv_obj_add_flag(nav_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_all(nav_panel, 15, 0);
    lv_obj_clear_flag(nav_panel, LV_OBJ_FLAG_SCROLLABLE);
    /* 统一深灰半透明 + 圆角风格 */
    lv_obj_set_style_border_width(nav_panel, 0, 0);
    lv_obj_set_style_shadow_width(nav_panel, 0, 0);
    lv_obj_set_style_bg_color(nav_panel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(nav_panel, LV_OPA_70, 0);
    lv_obj_set_style_radius(nav_panel, 6, 0);

    /* 标题 */
    lv_obj_t *nav_title = lv_label_create(nav_panel);
    lv_label_set_text(nav_title, "Navigation");
    lv_obj_set_style_text_font(nav_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(nav_title, lv_color_hex(0xffffff), 0);
    lv_obj_align(nav_title, LV_ALIGN_TOP_MID, 0, 5);

#define LABEL_W   95
#define ROW_GAP   45    /* 行间距 */
#define FIRST_Y   50    /* 第一行 y 偏移 */

    /* 起点纬度 */
    lv_obj_t *lb_slat = lv_label_create(nav_panel);
    lv_label_set_text(lb_slat, "Start Lat:");
    lv_obj_set_width(lb_slat, LABEL_W);
    lv_obj_set_style_text_align(lb_slat, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lb_slat, lv_color_hex(0xffffff), 0);
    lv_obj_align(lb_slat, LV_ALIGN_TOP_LEFT, 5, FIRST_Y);
    ta_start_lat = nav_ta_create(nav_panel);
    lv_obj_align_to(ta_start_lat, lb_slat, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_textarea_set_text(ta_start_lat, "");

    /* 起点经度 */
    lv_obj_t *lb_slon = lv_label_create(nav_panel);
    lv_label_set_text(lb_slon, "Start Lon:");
    lv_obj_set_width(lb_slon, LABEL_W);
    lv_obj_set_style_text_align(lb_slon, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lb_slon, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(lb_slon, lb_slat, LV_ALIGN_OUT_BOTTOM_LEFT, 0, ROW_GAP);
    ta_start_lon = nav_ta_create(nav_panel);
    lv_obj_align_to(ta_start_lon, lb_slon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_textarea_set_text(ta_start_lon, "");

    /* 终点纬度 */
    lv_obj_t *lb_elat = lv_label_create(nav_panel);
    lv_label_set_text(lb_elat, "End Lat:");
    lv_obj_set_width(lb_elat, LABEL_W);
    lv_obj_set_style_text_align(lb_elat, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lb_elat, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(lb_elat, lb_slon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, ROW_GAP);
    ta_end_lat = nav_ta_create(nav_panel);
    lv_obj_align_to(ta_end_lat, lb_elat, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_textarea_set_text(ta_end_lat, "");

    /* 终点经度 */
    lv_obj_t *lb_elon = lv_label_create(nav_panel);
    lv_label_set_text(lb_elon, "End Lon:");
    lv_obj_set_width(lb_elon, LABEL_W);
    lv_obj_set_style_text_align(lb_elon, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lb_elon, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(lb_elon, lb_elat, LV_ALIGN_OUT_BOTTOM_LEFT, 0, ROW_GAP);
    ta_end_lon = nav_ta_create(nav_panel);
    lv_obj_align_to(ta_end_lon, lb_elon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_textarea_set_text(ta_end_lon, "");

#undef LABEL_W
#undef ROW_GAP
#undef FIRST_Y

    /* 开始导航按钮 */
    lv_obj_t *btn_start = lv_btn_create(nav_panel);
    lv_obj_set_style_border_width(btn_start, 0, 0);
    lv_obj_set_style_shadow_width(btn_start, 0, 0);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(btn_start, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_start, 6, 0);
    lv_obj_set_size(btn_start, 140, 44);
    lv_obj_align(btn_start, LV_ALIGN_BOTTOM_LEFT, 15, -20);
    lv_obj_t *lb_start = lv_label_create(btn_start);
    lv_label_set_text(lb_start, "Start");
    lv_obj_set_style_text_color(lb_start, lv_color_hex(0xffffff), 0);
    lv_obj_center(lb_start);
    lv_obj_add_event_cb(btn_start, nav_start_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 取消按钮 */
    lv_obj_t *btn_cancel = lv_btn_create(nav_panel);
    lv_obj_set_style_border_width(btn_cancel, 0, 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(btn_cancel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_cancel, 6, 0);
    lv_obj_set_size(btn_cancel, 140, 44);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -15, -20);
    lv_obj_t *lb_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lb_cancel, "Cancel");
    lv_obj_set_style_text_color(lb_cancel, lv_color_hex(0xffffff), 0);
    lv_obj_center(lb_cancel);
    lv_obj_add_event_cb(btn_cancel, nav_panel_hide_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 消息标签 (放在按钮上方, 水平居中于面板) */
    nav_msg = lv_label_create(nav_panel);
    lv_obj_set_style_text_color(nav_msg, lv_color_hex(0xff4444), 0);
    lv_label_set_text(nav_msg, "");
    lv_obj_align(nav_msg, LV_ALIGN_BOTTOM_MID, 0, -90);

    /* 数字键盘 (底部, 初始隐藏) */
    kb = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(kb, 440, 180);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_event_cb(kb, kb_ready_cb, LV_EVENT_READY, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    /* GPS定位状态提示标签 (初始隐藏) */
    gps_status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(gps_status_label, "");
    lv_obj_set_width(gps_status_label, 200);
    lv_obj_set_style_text_align(gps_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(gps_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(gps_status_label, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(gps_status_label, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(gps_status_label, 6, 0);
    lv_obj_set_style_radius(gps_status_label, 6, 0);
    lv_obj_add_flag(gps_status_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(gps_status_label, canvas2590, LV_ALIGN_TOP_MID, 0, 10);
}
int zoom0;
void map_show(double center_lat, double center_lon, int zoom)
{
    psram_heap_stats();
    zoom0 = zoom;
    lv_obj_set_pos(canvas2590, 0, 0);
    map_tiles_set_zoom(map, zoom);
    map_tiles_set_center_from_gps(map, center_lat, center_lon);
    map_tiles_get_position(map, &tx, &ty);
    int offset_x, offset_y;
    map_tiles_get_marker_offset(map, &offset_x, &offset_y);
    int index = 0;
    for (int y = 0; y < lt; y++) {
        for (int x = 0; x < wt; x++) {
            node[index].tile_x = tx + x;
            node[index].tile_y = ty + y;
            char path[256];
            map_tiles_get_tile_path(map, node[index].tile_x, node[index].tile_y,
                                    path, sizeof(path));
            lv_image_set_src(node[index].obj, path);
            index++;
        }
    }
    index = 0;
    center_offsetx = (int)(MAP_SCREEN_WIDTH / 2.0 - (MAP_TILE_GRID_N / 2) * 256) - offset_x;
    center_offsety = (int)(MAP_SCREEN_HEIGHT / 2.0 - (MAP_TILE_GRID_N / 2) * 256) - offset_y;
    for (int y = 0; y < lt; y++) {
        for (int x = 0; x < wt; x++) {
            map_tile_place(node[index].obj, x * 256 + center_offsetx, y * 256 + center_offsety);
            index++;
        }
    }
    cam_x = center_offsetx;
    cam_y = center_offsety;
    printf("offset_x : %d,offset_y : %d\n", offset_x, offset_y);
    if (fr) {
        map_route_load("storage/sd1/C/graph_tiles");
        map_route_init(canvas2590);
        fr = 0;
    } else {
        map_route_update();
    }
    gps_marker_refresh();
    //lv_timer_handler();  // 跑一帧，触发渲染 → 解码 → 写入缓存
    lv_image_cache_print_usage();
}
void map_zoom_plus_show()
{
    if (zoom0 + 1 > 17) {
        return;
    }
    map_get_screen_center(&center_lat, &center_lon);
    map_show(center_lat, center_lon, zoom0 + 1);
}
void map_zoom_minus_show()
{
    if (zoom0 - 1 < 8) {
        return;
    }
    map_get_screen_center(&center_lat, &center_lon);
    map_show(center_lat, center_lon, zoom0 - 1);
}
void map_get_screen_center(double *lat, double *lon)
{
    int start_tile_x = (int)floor((double) - cam_x / 256);
    int start_tile_y = (int)floor((double) - cam_y / 256);
    int offset_x = cam_x - (-start_tile_x) * 256;
    int offset_y = cam_y - (-start_tile_y) * 256;
    double tile_x = tx + start_tile_x + (MAP_SCREEN_WIDTH / 2.0 - offset_x) / 256.0;
    double tile_y = ty + start_tile_y + (MAP_SCREEN_HEIGHT / 2.0 - offset_y) / 256.0;
    map_tiles_tile_xy_to_gps(map, tile_x, tile_y, lat, lon);
}
bool map_tiles_gps_to_screen(double lat, double lon, int *x, int *y)
{
    if (!map || !x || !y) {
        return false;
    }
    double tile_x;
    double tile_y;
    map_tiles_gps_to_tile_xy(map, lat, lon, &tile_x, &tile_y);
    // 当前地图左上角全球坐标
    double origin_x = tx * 256;
    double origin_y = ty * 256;
    double world_x = tile_x * 256;
    double world_y = tile_y * 256;
    int screen_x = (int)(world_x - origin_x + cam_x);
    int screen_y = (int)(world_y - origin_y + cam_y);
    map_screen_rotate(screen_x, screen_y, &screen_x, &screen_y);
    *x = screen_x;
    *y = screen_y;
    return true;
}
void map_center_on_gps(int zoom)
{
    gps_data_t gps;
    gps_get_info(&gps);
    if (gps.valid && gps.fix_mode > 0) {
        double lat = gps.latitude_e7 / 10000000.0;
        double lon = gps.longitude_e7 / 10000000.0;
        printf("GPS定位: %.5f, %.5f  fix:%d sat:%d\r\n",
               lat, lon, gps.fix_mode, gps.satellites_used);
        map_show(lat, lon, zoom);
    } else {
        printf("GPS未定位, valid:%d fix_mode:%d\r\n",
               gps.valid, gps.fix_mode);
    }
}

/********************************************************
 * GPS 定位按钮 — 获取当前位置, 画标记, 作为默认导航起点
 ********************************************************/
static void gps_show_status(const char *text)
{
    if (!gps_status_label) {
        return;
    }
    if (text && text[0]) {
        lv_label_set_text(gps_status_label, text);
        lv_obj_clear_flag(gps_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(gps_status_label);
    } else {
        lv_label_set_text(gps_status_label, "");
        lv_obj_add_flag(gps_status_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gps_status_hide_cb(lv_timer_t *timer)
{
    gps_show_status("");
    lv_timer_del(timer);
    gps_status_hide_timer = NULL;
}

/* 方向箭头 canvas 尺寸: 40x40, 中心即定位点, 三角形顶点朝前进方向 */
#define GPS_MARKER_TRI_SIZE 40
LV_DRAW_BUF_DEFINE_STATIC(gps_marker_tri_buf, GPS_MARKER_TRI_SIZE, GPS_MARKER_TRI_SIZE,
                          LV_COLOR_FORMAT_ARGB8888);

static void gps_marker_create(void)
{
    if (gps_marker) {
        return;
    }

    /* 先创建三角形 canvas(在下层) */
    LV_DRAW_BUF_INIT_STATIC(gps_marker_tri_buf);
    gps_marker_tri = lv_canvas_create(canvas2590);
    lv_canvas_set_draw_buf(gps_marker_tri, &gps_marker_tri_buf);

    /* 再创建蓝色圆点(在上层, 盖住三角形底边) */
    gps_marker = lv_obj_create(canvas2590);
    lv_obj_set_size(gps_marker, 18, 18);
    lv_obj_set_style_radius(gps_marker, 9, 0);
    lv_obj_set_style_bg_color(gps_marker, lv_color_hex(0x1a73e8), 0);
    lv_obj_set_style_bg_opa(gps_marker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(gps_marker, 2, 0);
    lv_obj_set_style_border_color(gps_marker, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(gps_marker, 0, 0);
    lv_obj_set_style_shadow_width(gps_marker, 0, 0);
    lv_obj_clear_flag(gps_marker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(gps_marker);
}

/* 叉积: 判断点 (px,py) 相对边 (a->b) 的符号 */
static int gps_tri_sign(int px, int py, int ax, int ay, int bx, int by)
{
    return (px - bx) * (ay - by) - (ax - bx) * (py - by);
}

/* 在三角形 canvas 内逐像素填充实心三角形 (重心坐标法) */
static void gps_fill_triangle(int ax, int ay, int bx, int by, int cx, int cy, lv_color_t c)
{
    for (int y = 0; y < GPS_MARKER_TRI_SIZE; y++) {
        for (int x = 0; x < GPS_MARKER_TRI_SIZE; x++) {
            int d1 = gps_tri_sign(x, y, ax, ay, bx, by);
            int d2 = gps_tri_sign(x, y, bx, by, cx, cy);
            int d3 = gps_tri_sign(x, y, cx, cy, ax, ay);
            bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(neg && pos)) {
                lv_canvas_set_px(gps_marker_tri, x, y, c, LV_OPA_COVER);
            }
        }
    }
}

/*
 * 设置当前位置图标
 * heading_deg: 前进方向 (0~359, 北=0 顺时针); -1 表示不改变方向
 */
static void gps_marker_set_pos(double lat, double lon, int16_t heading_deg)
{
    int x, y;

    if (!map_tiles_gps_to_screen(lat, lon, &x, &y)) {
        return;
    }

    gps_marker_create();

    /* 1) 先画实心三角形(在下层) */
    int c = GPS_MARKER_TRI_SIZE / 2;   /* 画布中心 = 定位点 */

    lv_obj_set_pos(gps_marker_tri, x - c, y - c);
    lv_canvas_fill_bg(gps_marker_tri, lv_color_hex(0x000000), LV_OPA_TRANSP);

    if (heading_deg >= 0) {
        /* 箭头相对屏幕的角度 = 前进方向(北=0) + 地图旋转角。
         * 地图旋转后所有地图元素在屏幕上都顺时针转了 map_rotation_deg,
         * 箭头也要跟着转, 这样车头朝上时箭头指向屏幕正上方。 */
        double h = (heading_deg + map_rotation_deg) * M_PI / 180.0;
        double dx = sin(h);
        double dy = -cos(h);
        double lx = dy;     /* 左法向 */
        double ly = -dx;

        double tip = 16.0;  /* 顶点到圆心距离 */
        double r   = 5.0;   /* 底边中心到圆心(前方)距离 */
        double side = 5.0;  /* 底边半宽 */

        int ax = c + (int)(dx * tip);
        int ay = c + (int)(dy * tip);
        int bx = c + (int)(dx * r) + (int)(lx * side);
        int by = c + (int)(dy * r) + (int)(ly * side);
        int cx2 = c + (int)(dx * r) - (int)(lx * side);
        int cy2 = c + (int)(dy * r) - (int)(ly * side);

        gps_fill_triangle(ax, ay, bx, by, cx2, cy2, lv_color_hex(0xff0000));
    }
    lv_obj_invalidate(gps_marker_tri);

    /* 2) 再定位圆点(在上层, 盖住三角形底边) */
    lv_obj_set_pos(gps_marker, x - 9, y - 9);
    lv_obj_move_foreground(gps_marker);
}

/* 地图平移/缩放后, 让标记跟随坐标点 */
static void gps_marker_refresh(void)
{
    if (gps_located) {
        gps_marker_set_pos(gps_located_lat, gps_located_lon, -1);
    }
    /* 模拟导航: 拖动地图时让图标重新钉到当前路线位置(而非跟随手指) */
    if (sim_has_pos) {
        gps_marker_set_pos(sim_cur_lat, sim_cur_lon, sim_cur_heading);
    }
}

/* 将 GPS 坐标填入导航起点作为默认起点 */
static void nav_set_default_start(double lat, double lon)
{
    char buf[32];

    if (ta_start_lat) {
        snprintf(buf, sizeof(buf), "%.6f", lat);
        lv_textarea_set_text(ta_start_lat, buf);
    }
    if (ta_start_lon) {
        snprintf(buf, sizeof(buf), "%.6f", lon);
        lv_textarea_set_text(ta_start_lon, buf);
    }
}

static void gps_locate_timer_cb(lv_timer_t *timer)
{
    gps_poll();
    gps_data_t gps;
    gps_get_info(&gps);

    if (gps.valid && gps.fix_mode > 0) {
        double lat = gps.latitude_e7 / 10000000.0;
        double lon = gps.longitude_e7 / 10000000.0;

        printf("GPS定位成功: %.6f, %.6f  fix:%d sat:%d\r\n",
               lat, lon, gps.fix_mode, gps.satellites_used);

        gps_located = true;
        gps_located_lat = lat;
        gps_located_lon = lon;
        start_lat = lat;
        start_lon = lon;

        /* 居中地图, 画标记, 并把坐标作为默认导航起点 */
        map_show(lat, lon, zoom0);
        gps_marker_set_pos(lat, lon, -1);
        nav_set_default_start(lat, lon);

        gps_show_status("GPS located!");
        gps_status_hide_timer = lv_timer_create(gps_status_hide_cb, 2000, NULL);

        lv_timer_del(timer);
        gps_locate_timer = NULL;
        return;
    }

    if (lv_tick_elaps(gps_locate_start_tick) >= GPS_LOCATE_TIMEOUT_MS) {
        printf("GPS定位超时\n");
        gps_show_status("GPS locate timeout!");
        lv_timer_del(timer);
        gps_locate_timer = NULL;
        return;
    }

    int poll_cnt, byte_cnt;
    gps_get_debug_counters(&poll_cnt, &byte_cnt);
    printf("GPS定位中... valid:%d fix:%d sat:%d poll:%d byte:%d\r\n",
           gps.valid, gps.fix_mode, gps.satellites_used, poll_cnt, byte_cnt);
}

static void gps_locate_btn_cb(lv_event_t *e)
{
    (void)e;

    if (gps_locate_timer) {
        return; /* 已在定位中 */
    }
    /* 清理上一次成功提示的自动隐藏定时器, 避免误隐藏本次 Locating 提示 */
    if (gps_status_hide_timer) {
        lv_timer_del(gps_status_hide_timer);
        gps_status_hide_timer = NULL;
    }
    if (!gps_inited) {
        gps_init();
        gps_inited = true;
    }

    gps_show_status("Locating...");
    gps_locate_start_tick = lv_tick_get();
    gps_locate_timer = lv_timer_create(gps_locate_timer_cb, GPS_POLL_INTERVAL_MS, NULL);
}
#endif
