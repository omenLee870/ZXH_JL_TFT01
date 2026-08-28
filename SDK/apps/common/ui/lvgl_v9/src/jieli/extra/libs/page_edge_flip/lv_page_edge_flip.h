#ifndef LV_PAGE_EDGE_FLIP_H
#define LV_PAGE_EDGE_FLIP_H

#include "jlgpu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default edge flip angle (total rotation range) */
#define LV_EDGE_FLIP_ANGLE           45.0f

/**
 * @brief Compute h_offset for the bottom-hinge pivot.
 *
 * The hinge is located at (screen_w/2, screen_h + h_ofs), below the screen.
 * The page rotates in the 2D plane (Z-axis rotation) around this pivot,
 * creating a "hinged flap" visual effect.
 *
 * @param angle     total flip angle in degrees (default 45°)
 * @param screen_w  screen width in pixels
 * @return          h_ofs: vertical offset from screen bottom to hinge pivot
 */
float lv_page_edge_flip_get_h_offset(float angle, int screen_w);

/**
 * @brief Compute forward and backward matrices for one face of edge flip.
 *
 * Uses a 2D affine rotation (Z-axis) with the pivot at the bottom of the screen.
 * Matrix chain: T(px, py) * RotZ(rot) * T(-px, -py)
 *
 * - face_flag=0 (current/old page): rot = -degree * PI/180  (0 -> -angle)
 * - face_flag=1 (next/new page):    rot = (-degree+angle) * PI/180  (angle -> 0)
 *
 * @param fw_mat    [out] forward matrix (source -> screen)
 * @param bw_mat    [out] backward matrix (screen -> source, for GPU texel fetch)
 * @param degree    current flip degree (0 ~ angle)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 * @param angle     total flip angle in degrees (default 45°)
 * @param face_flag 0 = current/old page, 1 = next/new page
 */
void lv_page_edge_flip_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
                                  float degree, int screen_w, int screen_h,
                                  float angle, int face_flag);

/**
 * @brief Convenience wrapper: fill a jlgpu_global_matrix_t for one face.
 *
 * @param gm        [out] global matrix ready for jlgpu_set_global_trans_param
 * @param degree    current flip degree (0 ~ angle)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 * @param angle     total flip angle in degrees (default 45°)
 * @param face_flag 0 = current/old page, 1 = next/new page
 */
void lv_page_edge_flip_get_full_screen_matrix(jlgpu_global_matrix_t *gm,
        float degree, int screen_w, int screen_h,
        float angle, int face_flag);

#ifdef __cplusplus
}
#endif

#endif /* LV_PAGE_EDGE_FLIP_H */
