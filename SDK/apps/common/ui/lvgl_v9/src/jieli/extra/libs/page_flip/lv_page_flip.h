#ifndef LV_PAGE_FLIP_H
#define LV_PAGE_FLIP_H

#include "jlgpu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Perspective depth for flip effect: smaller = more dramatic 3D effect */
#define LV_FLIP_PROJECTION_DEEP  500

/**
 * @brief Compute the projected width of the flipped (middle) section
 *
 * Used to split the screen into 3 regions (left static / middle flipped / right static)
 * during a page-turn flip transition.
 *
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 * @param degree    current flip angle in degrees (0 ~ 180)
 * @return          width of the perspective-projected middle section (0 ~ screen_w/2)
 */
int lv_page_flip_get_affine_width(int screen_w, int screen_h, float degree);

/**
 * @brief Compute the forward and backward flip matrices for the GPU
 *
 * The forward matrix maps source texture -> screen coordinates.
 * The backward matrix (inverse) maps screen coordinates -> source texture,
 * which is what the GPU uses for texel fetching.
 *
 * The rotation is centered horizontally at screen_w/2 and vertically at screen_h/2,
 * so the page appears to fold from the center line.
 *
 * @param fw_mat    [out] forward matrix (source -> screen)
 * @param bw_mat    [out] backward matrix (screen -> source, for GPU texture fetch)
 * @param degree    flip angle in degrees (0 ~ 180)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 */
void lv_page_flip_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
                             float degree, int screen_w, int screen_h);

/**
 * @brief Build a ready-to-use jlgpu_global_matrix_t for full-screen flip.
 *
 * Convenience wrapper: calls lv_page_flip_get_matrix and fills a
 * jlgpu_global_matrix_t with trans_en=1 and both fw/bw matrices set.
 *
 * @param gm        [out] global matrix ready for jlgpu_set_global_trans_param
 * @param degree    flip angle in degrees (0 ~ 180)
 * @param screen_w  screen width in pixels
 * @param screen_h  screen height in pixels
 */
void lv_page_flip_get_full_screen_matrix(jlgpu_global_matrix_t *gm,
        float degree, int screen_w, int screen_h);

#ifdef __cplusplus
}
#endif

#endif /* LV_PAGE_FLIP_H */
