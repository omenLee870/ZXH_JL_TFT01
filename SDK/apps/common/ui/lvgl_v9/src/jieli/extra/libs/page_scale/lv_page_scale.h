#ifndef LV_PAGE_SCALE_H
#define LV_PAGE_SCALE_H

#include "jlgpu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default scale range for page transition */
#define LV_PAGE_SCALE_MIN   0.5f
#define LV_PAGE_SCALE_MAX   1.0f
#define LV_PAGE_SCALE_DELTA (LV_PAGE_SCALE_MAX - LV_PAGE_SCALE_MIN)

/**
 * @brief Compute a scale-around-center transform matrix for the GPU.
 *
 * Builds the forward matrix: T(w/2,h/2) * Scale(ratio,ratio) * T(-w/2,-h/2)
 * and its inverse (backward) for GPU texture fetching.
 * The scale is always centered on the texture, not the screen.
 *
 * @param fw_mat    [out] forward matrix
 * @param bw_mat    [out] backward matrix
 * @param ratio     scale factor (e.g. 0.5 ~ 1.0)
 * @param tex_w     texture width in pixels
 * @param tex_h     texture height in pixels
 */
void lv_page_scale_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
                              float ratio, int tex_w, int tex_h);

#ifdef __cplusplus
}
#endif

#endif /* LV_PAGE_SCALE_H */
