#include "lv_conf.h"
#if LV_USE_DEMO_MAP
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Map tiles component for LVGL 9.x
 *
 * This component provides functionality to load and display map tiles with GPS coordinate conversion.
 * Tiles are assumed to be 256x256 pixels in RGB565 format, stored in binary files.
 */

// Constants
#define MAP_TILES_TILE_SIZE 256
#define MAP_TILES_DEFAULT_GRID_COLS 5
#define MAP_TILES_DEFAULT_GRID_ROWS 5
#define MAP_TILES_MAX_GRID_COLS 9
#define MAP_TILES_MAX_GRID_ROWS 9
#define MAP_TILES_MAX_TILES (MAP_TILES_MAX_GRID_COLS * MAP_TILES_MAX_GRID_ROWS)
#define MAP_TILES_BYTES_PER_PIXEL 2

/* 地图显示区域尺寸 (像素) */
#define MAP_SCREEN_WIDTH   1024
#define MAP_SCREEN_HEIGHT  600

/* 瓦片矩阵边长 (正方形): 横向/纵向各多少块 256px 瓦片。
 * 需覆盖屏幕并留出拖动余量, 例如 1024x600 用 6x6。 */
#define MAP_TILE_GRID_N    6

/* LVGL v9 */
#define MAP_TILES_COLOR_FORMAT LV_COLOR_FORMAT_RGB565

#define MAP_TILES_MAX_TYPES 8
#define MAP_TILES_MAX_FOLDER_NAME 32

/**
 * @brief Configuration structure for map tiles
 */
typedef struct {
    const char *base_path;
    const char *tile_folders[MAP_TILES_MAX_TYPES];
    int tile_type_count;
    int grid_cols;
    int grid_rows;
    int default_zoom;
    bool use_spiram;
    int default_tile_type;
} map_tiles_config_t;

/**
 * @brief Map tiles handle
 */
typedef struct map_tiles_t *map_tiles_handle_t;

/**
 * @brief Initialize the map tiles system
 */
map_tiles_handle_t map_tiles_init(const map_tiles_config_t *config);

/**
 * @brief Set the zoom level
 */
void map_tiles_set_zoom(map_tiles_handle_t handle, int zoom_level);

/**
 * @brief Get the current zoom level
 */
int map_tiles_get_zoom(map_tiles_handle_t handle);

/**
 * @brief Set tile type
 */
bool map_tiles_set_tile_type(map_tiles_handle_t handle, int tile_type);

/**
 * @brief Get current tile type
 */
int map_tiles_get_tile_type(map_tiles_handle_t handle);

/**
 * @brief Get grid dimensions
 */
void map_tiles_get_grid_size(map_tiles_handle_t handle, int *cols, int *rows);

/**
 * @brief Get total tile count
 */
int map_tiles_get_tile_count(map_tiles_handle_t handle);

/**
 * @brief Get tile type count
 */
int map_tiles_get_tile_type_count(map_tiles_handle_t handle);

/**
 * @brief Get tile type folder
 */
const char *map_tiles_get_tile_type_folder(map_tiles_handle_t handle, int tile_type);

/**
 * @brief Get tile file path
 * @param handle map tiles handle
 * @param tile_x tile x coordinate
 * @param tile_y tile y coordinate
 * @param path_buf buffer to store the path string
 * @param buf_size size of path_buf
 * @return true on success
 */
bool map_tiles_get_tile_path(map_tiles_handle_t handle, int tile_x, int tile_y,
                             char *path_buf, size_t buf_size);

/**
 * @brief GPS -> Tile
 */
void map_tiles_gps_to_tile_xy(map_tiles_handle_t handle, double lat, double lon, double *x, double *y);

/**
 * @brief Tile -> GPS
 */
void map_tiles_tile_xy_to_gps(map_tiles_handle_t handle, double x, double y, double *lat, double *lon);

/**
 * @brief Get center GPS
 */
void map_tiles_get_center_gps(map_tiles_handle_t handle, double *lat, double *lon);

/**
 * @brief Set center GPS
 */
void map_tiles_set_center_from_gps(map_tiles_handle_t handle, double lat, double lon);

/**
 * @brief Check GPS inside current tiles
 */
bool map_tiles_is_gps_within_tiles(map_tiles_handle_t handle, double lat, double lon);

/**
 * @brief Get tile position
 */
void map_tiles_get_position(map_tiles_handle_t handle, int *tile_x, int *tile_y);

/**
 * @brief Set tile position
 */
void map_tiles_set_position(map_tiles_handle_t handle, int tile_x, int tile_y);

/**
 * @brief Get marker offset
 */
void map_tiles_get_marker_offset(map_tiles_handle_t handle, int *offset_x, int *offset_y);

/**
 * @brief Set marker offset
 */
void map_tiles_set_marker_offset(map_tiles_handle_t handle, int offset_x, int offset_y);

/**
 * @brief Cleanup
 */

void map_tiles_cleanup(map_tiles_handle_t handle);

void map_show(double center_lat, double center_lon, int zoom);
void map_zoom_plus_show();
void map_zoom_minus_show();
void map_init();
void map_get_screen_center(double *lat, double *lon);
void map_center_on_gps(int zoom);

/*
 * 销毁地图: 释放路线/道路缓存/map handle 及所有 LVGL 对象, 并重置状态。
 * 用于彻底卸载地图, 腾出 PSRAM/内存。
 */
void map_destroy(void);

/*
 * 离线导航入口 (由外部蓝牙 stream_media 回调调用)
 * 收到起终点坐标(e6 整数)与名称后, 开启导航并在地图上标出起终点名。
 * 名称需以 '\0' 结尾; 为空传 NULL 或空串。
 * 可在任意线程调用, 内部会投递到 LVGL 线程执行。
 */
void map_offline_nav_start(int32_t start_lon_e6, int32_t start_lat_e6,
                           int32_t end_lon_e6, int32_t end_lat_e6,
                           const char *start_name, const char *end_name);
#ifdef __cplusplus
}
#endif
#endif
