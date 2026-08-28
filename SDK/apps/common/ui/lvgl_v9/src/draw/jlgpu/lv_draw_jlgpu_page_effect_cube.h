#ifndef LV_DRAW_JLGPU_PAGE_EFFECT_CUBE_H
#define LV_DRAW_JLGPU_PAGE_EFFECT_CUBE_H

#include "lv_draw_jlgpu_page_effect_ops.h"

/**
 * @brief Initialize the cube effect (3D model + page context setup).
 *
 * @param unit       JLGPU draw unit
 * @param list_flag  true to use default face ordering
 */
void lv_cube_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag);

/**
 * @brief Update the cube rotation and recompute per-face matrices.
 *
 * @param unit     JLGPU draw unit
 * @param xoffset  horizontal movement delta (pixels)
 * @param yoffset  vertical movement delta (pixels)
 * @return 0 on success
 */
int lv_ui_cube_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset);

/**
 * @brief Map a screen touch point to a page ID.
 *
 * @param x  touch x coordinate
 * @param y  touch y coordinate
 * @return page ID, or -1 if no face at this position
 */
int lv_cube_effect_get_face_scr_id(int x, int y);

/**
 * @brief Release all cube resources.
 */
void lv_cube_effect_uninit(void);

/**
 * @brief Set the page ID for a cube face.
 *
 * @param page_id     the window/page ID
 * @param page_index  cube face index (0-5)
 */
void lv_cube_effect_set_page_index(uint32_t page_id, uint32_t page_index);

/**
 * @brief Get the array of page IDs for all 6 cube faces.
 *
 * @param page_id_arr  [out] array of 6 page IDs
 */
void lv_cube_effect_get_page_id(uint32_t *page_id_arr);

/**
 * @brief Return the ops table for the cube sticky effect.
 */
const sticky_effect_ops_t *lv_cube_effect_get_ops(void);

#endif
