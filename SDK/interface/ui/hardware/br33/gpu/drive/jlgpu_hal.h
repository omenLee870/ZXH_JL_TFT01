#ifndef __JLGPU_HAL_H__
#define __JLGPU_HAL_H__

#include "jlgpu_hal_utils.h"

#if !JLGPU_PLATFORM_PC
#include "asm/eva_sfr.h"
#endif


typedef struct {
    jlgpu_mask_mode_t	mask_mode;
    jlgpu_fg_mode_t		fg_mode;
    jlgpu_trans_mode_t	pnt_trans_mode;
    jlgpu_trans_mode_t	tex_trans_mode;
    jlgpu_trans_mode_t	vtx_trans_mode;
    jlgpu_paint_mode_t			render_type;
} jlgpu_render_task_t;


typedef struct {
    jlgpu_render_task_t			render_task;
    jlgpu_draw_mode_t			draw_mode;

    jlgpu_extend_command_t	    extend_command;

    jlgpu_render_task_ptr_t		render_task_ptr;
    jlgpu_render_task_info_t	render_task_info;
    jlgpu_layer_mode_t			layer_mode;
    void *task;
} jlgpu_hal_context_t;

jlgpu_hal_context_t *jlgpu_hal_create_context();
void jlgpu_hal_destroy_context(jlgpu_hal_context_t *jlgpu_hal_context);
void gpu_hw_task_set_render_param(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_draw_mode_t	draw_mode, jlgpu_mask_mode_t mask_mode, jlgpu_paint_mode_t render_type,
                                  jlgpu_trans_mode_t	tex_trans_mode, jlgpu_trans_mode_t pnt_trans_mode, jlgpu_trans_mode_t path_trans_mode);
void gpu_hw_task_set_extend_param(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_extend_command_t extend_command);
uint8_t gpu_hw_task_get_len_render(jlgpu_hal_context_t *jlgpu_hal_context);
uint8_t gpu_hw_task_get_len_extend(jlgpu_hal_context_t *jlgpu_hal_context);
void gpu_hw_task_set_task(jlgpu_hal_context_t *jlgpu_hal_context, void *task);
void gpu_hw_task_set_render_common(jlgpu_hal_context_t *jlgpu_hal_context);
void gpu_hw_task_set_extend_common(jlgpu_hal_context_t *jlgpu_hal_context);
void gpu_hw_task_set_layer_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t layer_en);
void gpu_hw_task_set_breakpoint_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t breakpoint_en);
void gpu_hw_task_set_blend_mode(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_blendmode_t blend_mode);
void gpu_hw_task_set_dither_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t dither_en);
void gpu_hw_task_set_gb_trans_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t gb_trans_en);
void gpu_hw_task_set_rgb_limit_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t rgb_limit_en);
void gpu_hw_task_set_global_alpha(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t global_alpha);
void gpu_hw_task_set_layer_quality(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_render_quality_t render_quality);
void gpu_hw_task_set_bbox(jlgpu_hal_context_t *jlgpu_hal_context, int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
void gpu_hw_task_set_path(jlgpu_hal_context_t *jlgpu_hal_context, int32_t path_len, uint8_t *path);
void gpu_hw_task_set_pnt_type(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_paint_type_t paint_type, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_spread_mode(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_spread_mode_t spread_mode, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_premult(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_premult_mode_t premult, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_mmu(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t clut_en, jlgpu_page_size_t page_size, uint8_t *tb_base, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_clut(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t *clut_adr, uint8_t clut_lvl, jlgpu_clut_format_t clut_format, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_sample_param(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_pt_samplemode_t pt_samplemode, int16_t coeff0, int16_t coeff1, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_color(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t r, uint8_t g, uint8_t b, uint8_t a, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_radial_param(jlgpu_hal_context_t *jlgpu_hal_context, int16_t in_r, int16_t out_r, int16_t w, int16_t h, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_angle_param(jlgpu_hal_context_t *jlgpu_hal_context, int16_t offset, int32_t gain, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_clip(jlgpu_hal_context_t *jlgpu_hal_context, int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_pnt_matrix(jlgpu_hal_context_t *jlgpu_hal_context, float *matrix, jlgpu_paint_idx_t idx);
void gpu_hw_task_set_tex_info(jlgpu_hal_context_t *jlgpu_hal_context, int width, int height, int stride, jlgpu_tex_format_t tex_fmt, jlgpu_adr_mode_t adr_mode,
                              jlgpu_compress_mode_t compress_mode, jlgpu_rbs_t rbs, jlgpu_endian_t endian, jlgpu_alpha_end_t alpha_end, uint32_t compress_size, uint8_t *data);
void gpu_hw_task_set_color_ext(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_color_extend_t color_ext);
void gpu_hw_task_set_tex_premult(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_premult_mode_t premult);
void gpu_hw_task_set_boundary_ext(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_boundary_extend_t boundary_ext);
void gpu_hw_task_set_colorkey_en(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t colorkey_en);
void gpu_hw_task_set_tex_clut(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t *clut_adr, jlgpu_clut_format_t clut_format);
void gpu_hw_task_set_tex_mmu(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t tex_en, uint8_t clut_en, jlgpu_page_size_t page_size, uint8_t *tb_base);
void gpu_hw_task_set_tex_clip(jlgpu_hal_context_t *jlgpu_hal_context, int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
void gpu_hw_task_set_tex_color(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t r, uint8_t g, uint8_t b);
void gpu_hw_task_set_tex_sample_param(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_tex_samplemode_t tex_samplemode, uint8_t shift, uint8_t *coeff);
void gpu_hw_task_set_tex_matrix(jlgpu_hal_context_t *jlgpu_hal_context, float *matrix);
void gpu_hw_task_set_mask_info(jlgpu_hal_context_t *jlgpu_hal_context, int stride, jlgpu_mask_format_t mask_fmt, jlgpu_endian_t endian, uint8_t mask_inv, uint8_t *data);
void gpu_hw_task_set_mask_area(jlgpu_hal_context_t *jlgpu_hal_context, int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
void gpu_hw_task_set_fill_rule(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_fillrule_t fill_rule);
void gpu_hw_task_set_path_render_quality(jlgpu_hal_context_t *jlgpu_hal_context, jlgpu_render_quality_t render_quality);
void gpu_hw_task_set_bez_div(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t bez_div);
void gpu_hw_task_set_path_matrix(jlgpu_hal_context_t *jlgpu_hal_context, float *matrix);
void gpu_hw_task_set_extend_out_info(jlgpu_hal_context_t *jlgpu_hal_context, int stride, jlgpu_out_format_t out_fmt, jlgpu_rbs_t rbs, jlgpu_endian_t endian, jlgpu_alpha_end_t alpha_end, uint8_t *data);
void gpu_hw_task_set_extend_out_area(jlgpu_hal_context_t *jlgpu_hal_context, int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
void gpu_hw_task_set_extend_global_trans(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t en, float *fw_matrix, float *bw_matrix);
void gpu_hw_task_set_extend_colorkey(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t r, uint8_t g, uint8_t b);
void gpu_hw_task_set_extend_uv_adr(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t *uv_data);
void gpu_hw_task_set_extend_gxdec(jlgpu_hal_context_t *jlgpu_hal_context, uint8_t *img_base, uint8_t mmu_en, jlgpu_page_size_t page_size, uint8_t *tb_base);


void gpu_hw_sfr_set_out_info(int stride, jlgpu_out_format_t out_fmt, jlgpu_rbs_t rbs, jlgpu_endian_t endian, jlgpu_alpha_end_t alpha_end, uint8_t *data);
void gpu_hw_sfr_set_out_area(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max);
void gpu_hw_sfr_set_global_trans(uint8_t en, float *fw_matrix, float *bw_matrix);
void gpu_hw_sfr_set_colorkey(uint8_t r, uint8_t g, uint8_t b);
void gpu_hw_sfr_set_vtx_bez_div(uint8_t bez_div);
void gpu_hw_sfr_set_vtx_render_quality(jlgpu_render_quality_t render_quality);
void gpu_hw_sfr_set_vtx_cbuf_split(jlgpu_cbuf_split_t cbuf_split);
void gpu_hw_sfr_set_vtx_full_frag(uint8_t full_frag);
void gpu_hw_sfr_set_vtx_buf(uint16_t cbuf_stride, uint8_t *cbuf, uint8_t *ebuf);
void gpu_hw_sfr_set_vtx_max_edge(uint16_t max_edge);
void gpu_hw_sfr_set_vtx_clear_edge_en(uint8_t en);
void gpu_hw_sfr_set_uv_adr(uint8_t *uv_data);
void gpu_hw_sfr_set_top_info(uint16_t top_w, uint16_t top_h);
void gpu_hw_sfr_set_top_offset(uint16_t offset_x, uint16_t offset_y);
void gpu_hw_sfr_set_top_buf_info(uint16_t buf_w, uint16_t buf_h);
void gpu_hw_sfr_set_top_buf0(uint8_t *buf0);
void gpu_hw_sfr_set_top_buf1(uint8_t *buf1);
void gpu_hw_sfr_set_sph_mode(jlgpu_sph_mode_t sph_mode);
void gpu_hw_sfr_set_sph_info(int16_t src_len, int16_t dst_len, float *src, float *dst);
void gpu_hw_sfr_set_sph_matrix(float *matrix);


//get
jlgpu_err_t gpu_hw_task_get_layer_mode(void *task, uint8_t *layer_en, jlgpu_layer_mode_t *layer_mode);
jlgpu_err_t gpu_hw_task_get_render_param(void *task, jlgpu_draw_mode_t *draw_mode, jlgpu_mask_mode_t *mask_mode,
        jlgpu_paint_mode_t *render_type, jlgpu_trans_mode_t *tex_trans_mode, jlgpu_trans_mode_t *pnt_trans_mode, jlgpu_trans_mode_t *path_trans_mode);
jlgpu_err_t gpu_hw_task_get_extend_param(void *task, jlgpu_extend_command_t *extend_command);
jlgpu_err_t gpu_hw_task_get_layer_en(void *task, uint8_t *layer_en);
jlgpu_err_t gpu_hw_task_get_breakpoint_en(void *task, uint8_t *breakpoint_en);
jlgpu_err_t gpu_hw_task_get_blend_mode(void *task, jlgpu_blendmode_t *blend_mode);
jlgpu_err_t gpu_hw_task_get_dither_en(void *task, uint8_t *dither_en);
jlgpu_err_t gpu_hw_task_get_gb_trans_en(void *task, uint8_t *gb_trans_en);
jlgpu_err_t gpu_hw_task_get_rgb_limit_en(void *task, uint8_t *rgb_limit_en);
jlgpu_err_t gpu_hw_task_get_global_alpha(void *task, uint8_t *global_alpha);
jlgpu_err_t gpu_hw_task_get_layer_quality(void *task, jlgpu_render_quality_t *render_quality);
jlgpu_err_t gpu_hw_task_get_bbox(void *task, int16_t *x_min, int16_t *x_max, int16_t *y_min, int16_t *y_max);
jlgpu_err_t gpu_hw_task_get_path(void *task, int32_t *path_len, uint8_t **path);
jlgpu_err_t gpu_hw_task_get_pnt_type(void *task, jlgpu_paint_type_t *paint_type, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_spread_mode(void *task, jlgpu_spread_mode_t *spread_mode, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_premult(void *task, jlgpu_premult_mode_t *premult, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_mmu(void *task, uint8_t *clut_en, jlgpu_page_size_t *page_size, uint8_t **tb_base, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_clut(void *task, uint8_t **clut_adr, uint8_t *clut_lvl, jlgpu_clut_format_t *clut_format, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_sample_param(void *task, jlgpu_pt_samplemode_t *pt_samplemode, int16_t *coeff0, int16_t *coeff1, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_color(void *task, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_radial_param(void *task, int16_t *in_r, int16_t *out_r, int16_t *w, int16_t *h, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_angle_param(void *task, int16_t *offset, int32_t *gain, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_clip(void *task, int16_t *x_min, int16_t *x_max, int16_t *y_min, int16_t *y_max, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_pnt_matrix(void *task, float *matrix, jlgpu_paint_idx_t idx);
jlgpu_err_t gpu_hw_task_get_tex_info(void *task, int *width, int *height, int *stride, jlgpu_tex_format_t *tex_fmt, jlgpu_adr_mode_t *adr_mode,
                                     jlgpu_compress_mode_t *compress_mode, jlgpu_rbs_t *rbs, jlgpu_endian_t *endian, jlgpu_alpha_end_t *alpha_end, uint32_t *compress_size, uint8_t **data);
jlgpu_err_t gpu_hw_task_get_color_ext(void *task, jlgpu_color_extend_t *color_ext);
jlgpu_err_t gpu_hw_task_get_tex_premult(void *task, jlgpu_premult_mode_t *premult);
jlgpu_err_t gpu_hw_task_get_boundary_ext(void *task, jlgpu_boundary_extend_t *boundary_ext);
jlgpu_err_t gpu_hw_task_get_colorkey_en(void *task, uint8_t *colorkey_en);
jlgpu_err_t gpu_hw_task_get_tex_clut(void *task, uint8_t **clut_adr, jlgpu_clut_format_t *clut_format);
jlgpu_err_t gpu_hw_task_get_tex_mmu(void *task, uint8_t *tex_en, uint8_t *clut_en, jlgpu_page_size_t *page_size, uint8_t **tb_base);
jlgpu_err_t gpu_hw_task_get_tex_clip(void *task, int16_t *x_min, int16_t *x_max, int16_t *y_min, int16_t *y_max);
jlgpu_err_t gpu_hw_task_get_tex_color(void *task, uint8_t *r, uint8_t *g, uint8_t *b);
jlgpu_err_t gpu_hw_task_get_tex_sample_param(void *task, jlgpu_tex_samplemode_t *tex_samplemode, uint8_t *shift, uint8_t *coeff);
jlgpu_err_t gpu_hw_task_get_tex_matrix(void *task, float *matrix);
jlgpu_err_t gpu_hw_task_get_mask_info(void *task, int *stride, jlgpu_mask_format_t *mask_fmt, jlgpu_endian_t *endian, uint8_t *mask_inv, uint8_t **data);
jlgpu_err_t gpu_hw_task_get_mask_area(void *task, int16_t *x_min, int16_t *x_max, int16_t *y_min, int16_t *y_max);
jlgpu_err_t gpu_hw_task_get_fill_rule(void *task, jlgpu_fillrule_t *fill_rule);
jlgpu_err_t gpu_hw_task_get_path_render_quality(void *task, jlgpu_render_quality_t *render_quality);
jlgpu_err_t gpu_hw_task_get_bez_div(void *task, uint8_t *bez_div);
jlgpu_err_t gpu_hw_task_get_path_matrix(void *task, float *matrix);
jlgpu_err_t gpu_hw_task_get_extend_out_info(void *task, int *stride, jlgpu_out_format_t *out_fmt, jlgpu_rbs_t *rbs, jlgpu_endian_t *endian, jlgpu_alpha_end_t *alpha_end, uint8_t **data);
jlgpu_err_t gpu_hw_task_get_extend_out_area(void *task, int16_t *x_min, int16_t *x_max, int16_t *y_min, int16_t *y_max);
jlgpu_err_t gpu_hw_task_get_extend_global_trans(void *task, uint8_t *en, float *fw_matrix, float *bw_matrix);
jlgpu_err_t gpu_hw_task_get_extend_colorkey(void *task, uint8_t *r, uint8_t *g, uint8_t *b);
jlgpu_err_t gpu_hw_task_get_extend_uv_adr(void *task, uint8_t **uv_data);
jlgpu_err_t gpu_hw_task_get_extend_gxdec(void *task, uint8_t **img_base, uint8_t *mmu_en, jlgpu_page_size_t *page_size, uint8_t **tb_base);






#if JLGPU_PLATFORM_PC
void gpu_run_hal(void *task_base);
int gpu_init_hal();
int gpu_free_hal();
int gpu_matrix_run_hal();
int gpu_matrix_init_hal();
#endif

#endif
