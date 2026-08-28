#ifndef LV_PAGE_CUBE_FLIP_H
#define LV_PAGE_CUBE_FLIP_H

#include "jlgpu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Perspective depth for cube flip: smaller = more dramatic 3D effect */
#define LV_CUBE_FLIP_PROJECTION_DEEP  500

/**
 * @brief Compute forward and backward matrices for one face of the cube flip.
 *
 * The cube flip simulates a 3D cube rotating in Y-axis perspective.
 * Two pages form two adjacent faces of the cube, each using its own matrix
 * with both Y-axis rotation and horizontal translation.
 *
 * Degree range: 0 -> 90 (touch_x * 90 / screen_w).
 *
 * Matrix chain (forward):
 *     T(-t0) * RotY(rot) * T(-t1)
 *
 *   face_flag=0 (old page, left face):
 *     t0 = (-w*degree/90, -h/2),  t1 = (0, h/2),  rot = -degree
 *     Slides right as degree increases, rotates away from viewer.
 *
 *   face_flag=1 (new page, right face):
 *     t0 = (-w*(1-degree/90), -h/2),  t1 = (0, h/2),  rot = -degree+90
 *     Slides from right edge, rotates from edge-on to face viewer.
 *
 * @param fw_mat    [out] forward matrix (source -> screen)
 * @param bw_mat    [out] backward matrix (screen -> source, for GPU texel fetch)
 * @param degree    current cube rotation degree (0 ~ 90)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 * @param face_flag 0 = current/old page, 1 = next/new page
 */
void lv_page_cube_flip_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
                                  float degree, int screen_w, int screen_h,
                                  int face_flag);

/**
 * @brief Convenience wrapper: fill a jlgpu_global_matrix_t for one face.
 *
 * @param gm        [out] global matrix ready for jlgpu_set_global_trans_param
 * @param degree    current cube rotation degree (0 ~ 90)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 * @param face_flag 0 = current/old page, 1 = next/new page
 */
void lv_page_cube_flip_get_full_screen_matrix(jlgpu_global_matrix_t *gm,
        float degree, int screen_w, int screen_h,
        int face_flag);

#ifdef __cplusplus
}
#endif

#endif /* LV_PAGE_CUBE_FLIP_H */
