

#ifndef __JLGPU_UTILS_H__
#define __JLGPU_UTILS_H__

#include "jlgpu_def.h"


uint32_t jlgpu_get_format_bpp(uint32_t format);
uint32_t jlgpu_get_mask_format_bpp(uint32_t format);
uint32_t jlgpu_compute_mask_stride(jlgpu_mask_format_t format, uint32_t width);
uint32_t jlgpu_compute_tex_stride(jlgpu_tex_format_t format, jlgpu_adr_mode_t adr_mode, jlgpu_compress_mode_t compress_mode, uint32_t width);
uint32_t jlgpu_compute_out_stride(jlgpu_out_format_t format, uint32_t width);

#endif
