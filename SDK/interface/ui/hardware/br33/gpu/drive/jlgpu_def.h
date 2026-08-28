#ifndef __JLGPU_DEF_H__
#define __JLGPU_DEF_H__

#define JLGPU_PLATFORM_PC 0

#include <math.h>


#if JLGPU_PLATFORM_PC
#include <stdint.h>
#include <string.h>
#include<stdlib.h>
#include<stdio.h>
#include <assert.h>
#define JLGPU_ASSERT assert
#else
#include "asm/cpu.h"
#include "system/includes.h"
#define JLGPU_ASSERT ASSERT
#endif

#define JLGPU_WIN_MIN           0
#define JLGPU_WIN_MAX           4095
#define JLGPU_BBOX_MIN          0
#define JLGPU_BBOX_MAX          4095
#define JLGPU_MASK_MIN          0
#define JLGPU_MASK_MAX          4095
#define JLGPU_PNT_CLIP_MIN      -4096
#define JLGPU_PNT_CLIP_MAX      4095
#define JLGPU_TEX_CLIP_MIN      0
#define JLGPU_TEX_CLIP_MAX      4095
#define JLGPU_MAX_EDGE          8191
#define JLGPU_MAX_GLOBAL_ALPHA  128

typedef enum {
    JLGPU_FGMODE_TEXTURE = 0,
    JLGPU_FGMODE_PAINT = 1,
    JLGPU_FGMODE_TEXTURE_PAINT_MULTIPLY = 2,
    JLGPU_FGMODE_TEXTURE_PAINT_GAIN = 3,
    JLGPU_FGMODE_LINE_RADIAL_MULTIPLY = 4,
    JLGPU_FGMODE_RADIAL_ANGLE_MULTIPLY = 5,
} jlgpu_fg_mode_t;

typedef enum {
    JLGPU_PAINT_TYPE_COLOR = 0,
    JLGPU_PAINT_TYPE_LINEAR_GRADIENT = 1,
    JLGPU_PAINT_TYPE_RADIAL_GRADIENT = 2,
    JLGPU_PAINT_TYPE_ANGLE_GRADIENT = 3,
} jlgpu_paint_type_t;


typedef enum {
    JLGPU_LAYER_MODE_RENDER = 0,
    JLGPU_LAYER_MODE_EXTEND = 1,
} jlgpu_layer_mode_t;

typedef enum {
    JLGPU_DRAW_FILL = 0,
    JLGPU_DRAW_GRAD = 1,
    JLGPU_DRAW_IMG = 2,
    JLGPU_DRAW_IMG_FILL = 3,
    JLGPU_DRAW_IMG_GRAD = 4,
    JLGPU_DRAW_LINEGRAD_RADIALGRAD = 5,
    JLGPU_DRAW_RADIALGRAD_ANGLEGRAD = 6,
} jlgpu_draw_mode_t;


typedef enum {
    JLGPU_EXTEND_COMMAND_MODIFY_SURFACE = 0,
    JLGPU_EXTEND_COMMAND_MODIFY_TRANS = 1,
    JLGPU_EXTEND_COMMAND_MODIFY_COLORKEY = 2,
    JLGPU_EXTEND_COMMAND_MODIFY_UVADR = 3,
    JLGPU_EXTEND_COMMAND_CONFIG_GXDEC = 4,
} jlgpu_extend_command_t;

typedef enum {
    JLGPU_TRANS_MODE_NONE = 0,
    JLGPU_TRANS_MODE_AFFINE = 1,
    JLGPU_TRANS_MODE_PROJECTION = 2,
} jlgpu_trans_mode_t;


typedef enum {
    JLGPU_COLOR_RAMP_SPREAD_PAD = 0,
    JLGPU_COLOR_RAMP_SPREAD_REFLECT = 1,
    JLGPU_COLOR_RAMP_SPREAD_REPEAT = 2,
    JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO = 3,
} jlgpu_spread_mode_t;

typedef enum {
    JLGPU_SAMPLE_MODE_NEAR = 0,
    JLGPU_SAMPLE_MODE_BILI = 1,
    JLGPU_SAMPLE_MODE_3X3 = 3,
    JLGPU_SAMPLE_MODE_5X5 = 4,
    JLGPU_SAMPLE_MODE_MEAN = 5,
} jlgpu_tex_samplemode_t;

typedef enum {
    JLGPU_SAMPLE_MODE_X1 = 0,
    JLGPU_SAMPLE_MODE_X4 = 1,
} jlgpu_pt_samplemode_t;

typedef enum {
    JLGPU_COMPRESS_MODE_NONE = 0,
    JLGPU_COMPRESS_MODE_RLE = 1,
    JLGPU_COMPRESS_MODE_ETC2 = 2,
    JLGPU_COMPRESS_MODE_JSTC = 3,
} jlgpu_compress_mode_t;

typedef enum {
    JLGPU_MASK_MODE_NONE = 0,
    JLGPU_MASK_MODE_VECTOR = 1,
    JLGPU_MASK_MODE_EXT = 2,
} jlgpu_mask_mode_t;

typedef enum {
    JLGPU_NON_ZERO = 0,
    JLGPU_EVEN_ODD = 1,
} jlgpu_fillrule_t;

typedef enum {
    JLGPU_RENDERING_QUALITY_NONANTIALIASED = 0,
    JLGPU_RENDERING_QUALITY_MSAA_4X = 1,
    JLGPU_RENDERING_QUALITY_MSAA_8X = 2,
} jlgpu_render_quality_t;

typedef enum {
    JLGPU_CLOSE_PATH = (0),
    JLGPU_MOVE_TO = (1),
    JLGPU_LINE_TO = (2),
    JLGPU_HLINE_TO = (3),
    JLGPU_VLINE_TO = (4),
    JLGPU_QUAD_TO = (5),
    JLGPU_SQUAD_TO = (6),
    JLGPU_CUBIC_TO = (7),
    JLGPU_SCUBIC_TO = (8),
    JLGPU_SCCWARC_TO = (9),
    JLGPU_SCWARC_TO = (10),
    JLGPU_LCCWARC_TO = (11),
    JLGPU_LCWARC_TO = (12),
} jlgpu_path_segment_t;


typedef enum {
    JLGPU_BLEND_SRC = 0,
    JLGPU_BLEND_SRC_OVER = 1,
    JLGPU_BLEND_DST_OUT = 2,
    JLGPU_BLEND_DST_IN = 3,
    JLGPU_BLEND_MAX = 4,
    JLGPU_BLEND_DST_OVER = 5,
    JLGPU_BLEND_SRC_OUT = 6,
    JLGPU_BLEND_SRC_IN = 7,
    JLGPU_BLEND_MULTIPLY = 8,
    JLGPU_BLEND_ADDITIVE = 9,
    JLGPU_BLEND_SUBTRACTIVE = 10,

} jlgpu_blendmode_t;

////////////////////////
//format

typedef enum {
    JLGPU_FORMAT_ARGB8888 = 0x00,
    JLGPU_FORMAT_ARGB8565 = 0x01,
    JLGPU_FORMAT_ARGB1555 = 0x02,
    JLGPU_FORMAT_ARGB4444 = 0x03,
    JLGPU_FORMAT_RGB888 = 0x04,
    JLGPU_FORMAT_RGB565 = 0x05,

    JLGPU_FORMAT_YUV422_BT601 = 0x06,
    JLGPU_FORMAT_YUV422_BT709 = 0x07,

    JLGPU_FORMAT_AL88 = 0x08,

    JLGPU_FORMAT_YUV420_BT601 = 0x09,
    JLGPU_FORMAT_YUV420_BT709 = 0x0a,

    JLGPU_FORMAT_YUV444_BT601 = 0x0b,
    JLGPU_FORMAT_YUV444_BT709 = 0x0c,
    JLGPU_FORMAT_A8_T8 = 0x0d,

    JLGPU_FORMAT_AL44 = 0x10,
    JLGPU_FORMAT_AL22 = 0x11,
    JLGPU_FORMAT_L8 = 0x12,
    JLGPU_FORMAT_L4 = 0x13,
    JLGPU_FORMAT_L2 = 0x14,
    JLGPU_FORMAT_L1 = 0x15,
    JLGPU_FORMAT_A8 = 0x16,
    JLGPU_FORMAT_A4 = 0x17,
    JLGPU_FORMAT_A2 = 0x18,
    JLGPU_FORMAT_A1 = 0x19,
} jlgpu_tex_format_t;

typedef enum {
    JLGPU_OUT_FORMAT_ARGB8888 = 0x00,
    JLGPU_OUT_FORMAT_ARGB8565 = 0x01,
    JLGPU_OUT_FORMAT_RGB888 = 0x04,
    JLGPU_OUT_FORMAT_RGB565 = 0x05,

    JLGPU_OUT_FORMAT_A8 = 0x16,
    JLGPU_OUT_FORMAT_A4 = 0x17,
    JLGPU_OUT_FORMAT_A2 = 0x18,
    JLGPU_OUT_FORMAT_A1 = 0x19,
} jlgpu_out_format_t;


typedef enum {
    JLGPU_MASK_FORMAT_A8 = 0x00,
    JLGPU_MASK_FORMAT_A4 = 0x01,
    JLGPU_MASK_FORMAT_A2 = 0x02,
    JLGPU_MASK_FORMAT_A1 = 0x03,
} jlgpu_mask_format_t;

typedef enum {
    JLGPU_CLUT_FORMAT_ARGB8888 = 0x00,
    JLGPU_CLUT_FORMAT_ARGB8565 = 0x01,
    JLGPU_CLUT_FORMAT_RGB888 = 0x02,
    JLGPU_CLUT_FORMAT_RGB565 = 0x03,
    JLGPU_CLUT_FORMAT_A8 = 0x04,
} jlgpu_clut_format_t;

typedef enum {
    JLGPU_ADRMODE_TILE = 0,
    JLGPU_ADRMODE_LINE = 1,
} jlgpu_adr_mode_t;

typedef enum {
    JLGPU_RBS_ARGB = 0,
    JLGPU_RBS_ABGR = 1,
} jlgpu_rbs_t;

typedef enum {
    JLGPU_ALPHA_END_ARGB = 0,
    JLGPU_ALPHA_END_RGBA = 1,
} jlgpu_alpha_end_t;

typedef enum {
    JLGPU_ENDIAN_LITTLE = 0,
    JLGPU_ENDIAN_BIG = 1,
} jlgpu_endian_t;

typedef enum {
    JLGPU_BOUNDARY_EXTEND_ZERO = 0,
    JLGPU_BOUNDARY_EXTEND_NEAR = 1,
} jlgpu_boundary_extend_t;

typedef enum {
    JLGPU_COLOR_EXTEND_HIGH = 0,
    JLGPU_COLOR_EXTEND_ZERO = 1,
} jlgpu_color_extend_t;

typedef enum {
    JLGPU_PREMULT_FALSE = 0,
    JLGPU_PREMULT_TRUE = 1,
} jlgpu_premult_mode_t;

typedef enum {
    JLGPU_CBUF_32X256 = 0,
    JLGPU_CBUF_16X256 = 1,
    JLGPU_CBUF_16X512 = 2,
} jlgpu_cbuf_split_t;

typedef enum {
    JLGPU_GRAD_MODE_LINEAR_GRADIENT = 0,
    JLGPU_GRAD_MODE_RADIAL_GRADIENT = 1,
    JLGPU_GRAD_MODE_ROUND_GRADIENT = 2,
    JLGPU_GRAD_MODE_RING_GRADIENT = 3,
    JLGPU_GRAD_MODE_ANGLE_GRADIENT = 4,
} jlgpu_grad_mode_t;

typedef enum {
    JLGPU_PAINT_MODE_MULTIPLY = 0,
    JLGPU_PAINT_MODE_GAIN = 1,
} jlgpu_paint_mode_t;

typedef enum {
    JLGPU_PAINT_INDEX_0 = 0,
    JLGPU_PAINT_INDEX_1 = 1,
} jlgpu_paint_idx_t;

typedef enum {
    JLGPU_PAGE_SIZE_4 = 0,
    JLGPU_PAGE_SIZE_16 = 1,
    JLGPU_PAGE_SIZE_32 = 2,
    JLGPU_PAGE_SIZE_64 = 3,
} jlgpu_page_size_t;

typedef enum {
    JLGPU_SPH_MODE_3D_3D = 0,
    JLGPU_SPH_MODE_3D_2D = 1,
} jlgpu_sph_mode_t;


typedef enum {
    JLGPU_SUCCESS = 0,
    JLGPU_ERR_INVALID_ARG = 1,
    JLGPU_ERR_MEMORY_NULL = 2,
    JLGPU_ERR_NON_ALIGN = 3,
    JLGPU_ERR_TASK_LEN = 4,
    JLGPU_ERR_NOT_SUPPORTED = 5,
} jlgpu_err_t;
#endif
