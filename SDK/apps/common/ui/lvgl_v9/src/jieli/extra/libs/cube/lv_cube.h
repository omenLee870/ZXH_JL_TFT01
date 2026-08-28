#ifndef LV_CUBE_H
#define LV_CUBE_H

#include "lv_draw_jlgpu.h"

/**
 * @brief Initialize the 3D cube model.
 *
 * @param width[6]       width of each of the 6 cube faces
 * @param height[6]      height of each of the 6 cube faces
 * @param w_scale        horizontal scale factor
 * @param h_scale        vertical scale factor
 * @param view_distance  camera distance (smaller = closer/bigger)
 * @param win_x          screen x offset
 * @param win_y          screen y offset
 * @param win_w          screen width
 * @param win_h          screen height
 * @return 0 on success, -1 on error
 */
int lv_cube_init(int width[6], int height[6], float w_scale, float h_scale,
                 float view_distance, int win_x, int win_y, int win_w, int win_h);

/**
 * @brief Release cube resources.
 */
void lv_cube_uninit(void);

/**
 * @brief Draw the cube at the given rotation angles.
 *
 * Computes per-face affine matrices and stores them in
 * g_page_effect_state.global_matrix[] / draw_area[] / draw_order[].
 *
 * @param unit        JLGPU draw unit
 * @param x_angle     accumulated X-axis rotation angle (pixels)
 * @param y_angle     accumulated Y-axis rotation angle (pixels)
 * @param draw_order  [out] face draw order (back-to-front, size 6)
 * @return 0 on success
 */
int lv_cube_draw(lv_draw_jlgpu_unit_t *unit, float x_angle, float y_angle, uint8_t *draw_order);

/**
 * @brief Determine which cube face is at the given screen coordinates.
 *
 * @param x  screen x
 * @param y  screen y
 * @return face index (0-5) or -1 if no face at this position
 */
int lv_cube_get_face_index(int x, int y);

#endif
