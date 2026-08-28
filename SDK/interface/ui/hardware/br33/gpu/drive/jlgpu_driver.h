/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
#ifndef __JLGPU_DRIVER_H__
#define __JLGPU_DRIVER_H__



#include "jlgpu_def.h"

#include "jlgpu_constomize.h"

#include "jlgpu_framework.h"

#include "jlgpu_hal.h"

#include "jlgpu_hal_utils.h"

#include "jlgpu_math.h"

#include "jlgpu_path.h"

#include "jlgpu_utils.h"



#define GPU_GET_TEX_COMPRESS_SIZE(w, h)   (w)
#define GPU_GET_C2_COMPRESS_SIZE(w, h)    (0x7ff)


#define GPU_API_CODE			AT(.gpu_api.text.cache.L2)


/* ==========<<< 纹理Ax格式 >>>========== */
#define GPU_IMG_FMT_AX(fmt) \
	((JLGPU_FORMAT_A1 == (fmt)) || \
	 (JLGPU_FORMAT_A2 == (fmt)) || \
	 (JLGPU_FORMAT_A4 == (fmt)) || \
	 (JLGPU_FORMAT_A8 == (fmt)))

/* ==========<<< 纹理Lx格式 >>>========== */
#define GPU_IMG_FMT_LX(fmt) \
	((JLGPU_FORMAT_L1 == (fmt)) || \
	 (JLGPU_FORMAT_L2 == (fmt)) || \
	 (JLGPU_FORMAT_L4 == (fmt)) || \
	 (JLGPU_FORMAT_L8 == (fmt)))

/* ==========<<< 纹理ALxx格式 >>>========== */
#define GPU_IMG_FMT_ALXX(fmt) \
	((JLGPU_FORMAT_AL22 == (fmt)) || \
	 (JLGPU_FORMAT_AL44 == (fmt)) || \
	 (JLGPU_FORMAT_AL88 == (fmt)))


/* ==========<<< 纹理格式需要配置颜色表 >>>========== */
#define GPU_IMG_NEED_CLUT(fmt) (GPU_IMG_FMT_LX(fmt) || GPU_IMG_FMT_ALXX(fmt))


/* ==========<<< 纹理格式需要配置颜色 >>>========== */
#define GPU_IMG_NEED_COLOR(fmt) GPU_IMG_FMT_AX(fmt)


/* ==========<<< GPU输出Ax格式 >>>========== */
#define GPU_OUT_FORMAT_AX(fmt) \
	(((fmt) == JLGPU_OUT_FORMAT_A1) || \
	 ((fmt) == JLGPU_OUT_FORMAT_A2) || \
	 ((fmt) == JLGPU_OUT_FORMAT_A4) || \
	 ((fmt) == JLGPU_OUT_FORMAT_A8))


/* ==========<<< 结构体定义映射 >>>========== */
#define GPU_mask_format_t		jlgpu_mask_format_t
#define GPU_format_t			jlgpu_tex_format_t
#define gpu_matrix_t			jlgpu_matrix_t
#define gpu_boundbox_t			jlgpu_boundbox_t
#define GPU_out_format_t		jlgpu_out_format_t


/* ==========<<< 枚举、宏映射 >>>========== */
#define jlgpu_mmu_page_size_t 	jlgpu_page_size_t
#define MMU_PAGE_4K				JLGPU_PAGE_SIZE_4
#define	MMU_PAGE_16K			JLGPU_PAGE_SIZE_16
#define MMU_PAGE_32K			JLGPU_PAGE_SIZE_32
#define MMU_PAGE_64K			JLGPU_PAGE_SIZE_64
// 输出格式
#define GPU_OUT_FORMAT_ARGB8565	JLGPU_OUT_FORMAT_ARGB8565
#define GPU_OUT_FORMAT_RGB565	JLGPU_OUT_FORMAT_RGB565
#define GPU_OUT_FORMAT_A8		JLGPU_OUT_FORMAT_A8
#define GPU_OUT_FORMAT_A4		JLGPU_OUT_FORMAT_A4
#define GPU_OUT_FORMAT_A2		JLGPU_OUT_FORMAT_A2
#define GPU_OUT_FORMAT_A1		JLGPU_OUT_FORMAT_A1
// 纹理格式
#define GPU_FORMAT_ARGB8888		JLGPU_FORMAT_ARGB8888
#define GPU_FORMAT_ARGB8565		JLGPU_FORMAT_ARGB8565
#define GPU_FORMAT_ARGB1555		JLGPU_FORMAT_ARGB1555
#define GPU_FORMAT_ARGB4444		JLGPU_FORMAT_ARGB4444
#define GPU_FORMAT_RGB888		JLGPU_FORMAT_RGB888
#define GPU_FORMAT_RGB565		JLGPU_FORMAT_RGB565
#define GPU_FORMAT_YUV422_BT601	JLGPU_FORMAT_YUV422_BT601
#define GPU_FORMAT_YUV422_BT709	JLGPU_FORMAT_YUV422_BT709
#define GPU_FORMAT_AL88			JLGPU_FORMAT_AL88
#define GPU_FORMAT_YUV420_BT601	JLGPU_FORMAT_YUV420_BT601
#define GPU_FORMAT_YUV420_BT709	JLGPU_FORMAT_YUV420_BT709
#define GPU_FORMAT_YUV444_BT601	JLGPU_FORMAT_YUV444_BT601
#define GPU_FORMAT_YUV444_BT709	JLGPU_FORMAT_YUV444_BT709
#define GPU_FORMAT_A8_T8		JLGPU_FORMAT_A8_T8
#define GPU_FORMAT_AL44			JLGPU_FORMAT_AL44
#define GPU_FORMAT_AL22			JLGPU_FORMAT_AL22
#define GPU_FORMAT_L8			JLGPU_FORMAT_L8
#define GPU_FORMAT_L4			JLGPU_FORMAT_L4
#define GPU_FORMAT_L2			JLGPU_FORMAT_L2
#define GPU_FORMAT_L1			JLGPU_FORMAT_L1
#define GPU_FORMAT_A8			JLGPU_FORMAT_A8
#define GPU_FORMAT_A4			JLGPU_FORMAT_A4
#define GPU_FORMAT_A2			JLGPU_FORMAT_A2
#define GPU_FORMAT_A1			JLGPU_FORMAT_A1
// 渲染方式
#define GPU_BLEND_SRC           JLGPU_BLEND_SRC
#define GPU_BLEND_SRC_OVER      JLGPU_BLEND_SRC_OVER
#define GPU_BLEND_DST_OUT       JLGPU_BLEND_DST_OUT
#define GPU_BLEND_DST_IN        JLGPU_BLEND_DST_IN
#define GPU_BLEND_MAX           JLGPU_BLEND_MAX
#define GPU_BLEND_DST_OVER      JLGPU_BLEND_DST_OVER
#define GPU_BLEND_SRC_OUT       JLGPU_BLEND_SRC_OUT
#define GPU_BLEND_SRC_IN        JLGPU_BLEND_SRC_IN
// 颜色表格式
#define GPU_CLUT_FORMAT_ARGB8888	JLGPU_CLUT_FORMAT_ARGB8888
#define GPU_CLUT_FORMAT_ARGB8565	JLGPU_CLUT_FORMAT_ARGB8565
#define GPU_CLUT_FORMAT_RGB888		JLGPU_CLUT_FORMAT_RGB888
#define GPU_CLUT_FORMAT_RGB565		JLGPU_CLUT_FORMAT_RGB565
// 变换方式
#define GPU_FILL_AFFINE				JLGPU_TRANS_MODE_AFFINE
#define GPU_FILL_PERSPECTIVE		JLGPU_TRANS_MODE_PROJECTION
#define GPU_TEXTURE_AFFINE			JLGPU_TRANS_MODE_AFFINE
#define GPU_TEXTURE_PERSPECTIVE		JLGPU_TRANS_MODE_PROJECTION



/* ==========<<< API 矩阵 >>>========== */
#define gpu_sinf					jlgpu_sinf
#define gpu_cosf					jlgpu_cosf
#define gpu_matrix_free_			jlgpu_matrix_free_
#define gpu_matrix_set_identity		jlgpu_matrix_set_identity
#define gpu_matrix_set_translate	jlgpu_matrix_set_translate
#define gpu_matrix_set_scale		jlgpu_matrix_set_scale
#define gpu_matrix_set_shear		jlgpu_matrix_set_shear
#define gpu_matrix_set_rotate		jlgpu_matrix_set_rotate
#define gpu_matrix_set_flip			jlgpu_matrix_set_flip
#define gpu_matrix_multiply			jlgpu_matrix_multiply
#define gpu_matrix_mul_const		jlgpu_matrix_mul_const
#define gpu_matrix_is_affine		jlgpu_matrix_is_affine
#define gpu_matrix_invert			jlgpu_matrix_invert
#define gpu_matrix_translate		jlgpu_matrix_translate
#define gpu_matrix_scale			jlgpu_matrix_scale
#define gpu_matrix_shear			jlgpu_matrix_shear
#define gpu_matrix_rotate			jlgpu_matrix_rotate
#define gpu_matrix_flip				jlgpu_matrix_flip
#define gpu_transform_point2		jlgpu_transform_point2
#define gpu_point2_add				jlgpu_point2_add
#define gpu_point2_sub				jlgpu_point2_sub
#define gpu_point2_mul				jlgpu_point2_mul
#define gpu_point2_len				jlgpu_point2_len
#define gpu_matrix_get_boundbox		jlgpu_matrix_get_boundbox
#define gpu_matrix_calc_by_point	jlgpu_matrix_calc_by_point


/* ==========<<< API bpp >>>========== */
#define gpu_get_format_bpp				jlgpu_get_format_bpp
#define gpu_task_get_format_bpp			jlgpu_get_format_bpp
#define gpu_get_mask_format_bpp			jlgpu_get_mask_format_bpp
#define gpu_task_get_mask_format_bpp	jlgpu_get_mask_format_bpp



#endif


