#ifndef LV_DRAW_JLGPU_COMMON_H
#define LV_DRAW_JLGPU_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#if LV_USE_DRAW_JLGPU

#include "jlgpu_framework.h"
#include "jlgpu_constomize.h"
#include "jlgpu_path.h"
#define BOOL_DEFINE_CONFLICT

/*#include "asm/gpu/jlgpu.h"*/
/*#include "asm/jlgpu_driver.h"*/

/*********************
 *      DEFINES
 *********************/
#define JLGPU_PI 3.14159265358979323846f
// 计算贝塞尔曲线控制点偏移量的系数
#define KAPPA 0.5522847498f

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

uint8_t lv_jlgpu_get_color_format(lv_color_format_t lv_cf);
uint8_t lv_jlgpu_get_blend_mode(lv_blend_mode_t lv_blend_mode, bool *mode_flag);
void lv_jlgpu_flush_inv_dcache(uint8_t *buf, uint32_t w, uint32_t h, uint32_t stride, uint8_t color_depth);
void lv_jlgpu_path_circle2bezier(uint8_t *segments, uint32_t *nseg, float *data, uint32_t *ndata,
                                 float cx, float cy, float r, float sx, float sy, float ex, float ey, uint8_t cw, uint8_t half);
void create_rounded_rect_jlgpu_path(lv_area_t *coords, uint16_t final_radius,
                                    uint8_t *op_buf, float *coord_buf, int *op_size, int *coord_size);

lv_result_t lv_draw_jlgpu_premult_and_swizzle(uint32_t *dest, lv_color_t src, lv_opa_t opa);
jlgpu_trans_mode_t compute_jlgpu_trans_mode(jlgpu_matrix_t *matrix);
/**********************
 *      MACROS
 **********************/
#endif /*LV_USE_DRAW_JLGPU*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_JLGPU_COMMON_H*/
