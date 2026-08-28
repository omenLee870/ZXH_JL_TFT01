#ifndef __JLUI_EFFECT__
#define __JLUI_EFFECT__

#include "gpu_port.h"

void get_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree);
int get_flip_width(int screen_w, float degree);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_cube_flip_matrix
 *
 * @Params matrix
 * @Params screen_x
 * @Params screen_y
 * @Params screen_w
 * @Params screen_h
 * @Params degree
 * @Params is_reflection    : 是否为镜像面, 0 - 非镜像; 1 - 镜像
 * @Params flip_dir         : 翻转方向, 0 - 从右向左; 1 - 从左向右
 * @Params face_flag        : 是否为当前页面, 0 - 当前页面; 1 - 上一页或下一页
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void get_cube_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree, u8 is_reflection, u8 flip_dir, u8 face_flag);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_drift_flip_matrix
 *
 * @Params matrix
 * @Params screen_x
 * @Params screen_y
 * @Params screen_w
 * @Params screen_h
 * @Params degree
 * @Params is_reflection    : 是否为镜像面, 0 - 非镜像; 1 - 镜像
 * @Params flip_dir         : 翻转方向, 0 - 从右向左; 1 - 从左向右
 * @Params face_flag        : 是否为当前页面, 0 - 当前页面; 1 - 上一页或下一页
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void get_drift_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree, u8 is_reflection, u8 flip_dir, u8 face_flag);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_edge_flip_matrix
 *
 * @Params matrix
 * @Params screen_x
 * @Params screen_y
 * @Params screen_w
 * @Params screen_h
 * @Params degree
 * @Params flip_dir         : 翻转方向, 0 - 从右向左; 1 - 从左向右
 * @Params face_flag        : 是否为当前页面, 0 - 当前页面; 1 - 上一页或下一页
 * @Params angle            : 两个页面之间的夹角
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void get_edge_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree, u8 flip_dir, u8 face_flag, float angle);

/*----------------------------------------------------------------------------*/
/**@brief    立方体模型初始化
   @param    width : 立方体每个面的宽度
   @param    heihgt : 立方体每个面的高度
   @param    w_scale : 立方体每个面宽度的缩放系数
   @param    h_scale : 立方体每个面高度的缩放系数
   @param    win_x : win区域x坐标偏移
   @param    win_y : win区域y坐标偏移
   @param    win_w : win区域宽度
   @param    win_h : win区域高度
   @note
*/
/*----------------------------------------------------------------------------*/
int board_cube_init(int width, int height, float w_scale, float h_scale, int win_x, int win_y, int win_w, int win_h);

/*----------------------------------------------------------------------------*/
/**@brief   立方体资源释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void board_cube_uninit(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_board_flip_matrix
 *
 * @Params matrix
 * @Params degree
 * @Params flip_dir         : 翻转方向, 0 - 从右向左; 1 - 从左向右
 * @Params face_flag        : 选择当前页面, 0 - 当前页面; 1 - 上一页或下一页; 2 - 左侧板宽; 3 - 右侧板宽
 */
/* ------------------------------------------------------------------------------------*/
void get_board_flip_matrix(gpu_matrix_t *matrix, float degree, u8 flip_dir, u8 face_flag);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief create_board_flip_fill_task
 *
 * @Params head
 * @Params draw_rect
 * @Params color
 * @Params degree
 * @Params flip_dir         : 翻转方向, 0 - 从右向左; 1 - 从左向右
 */
/* ------------------------------------------------------------------------------------*/
void create_board_flip_fill_task(pJLGPUTaskHead_t head, struct rect draw_rect, u32 color, float degree, u8 flip_dir);

#endif //__JLUI_EFFECT__



