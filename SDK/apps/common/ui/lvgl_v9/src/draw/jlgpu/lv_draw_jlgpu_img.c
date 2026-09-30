
/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"
#include "../lv_draw_image_private.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_IMG_ENABLE == 1)

/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/

/**********************
*  STATIC PROTOTYPES
**********************/
static void _jlgpu_draw_image_normal_helper(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords);

static bool _jlgpu_draw_image_transform_helper(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords);

static void _jlgpu_draw_image_tiled_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords);

static uint8_t *lv_jlgpu_c3_draw_task_create(lv_draw_jlgpu_unit_t *u, const lv_draw_image_dsc_t *dsc, lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t *img_buf, jlgpu_mask_param_t *mask_params, jlgpu_draw_img_t *draw_img_params, jlgpu_area_t *render_area, jlgpu_area_t *tex_clip_area, bool use_gpu_list);

static uint8_t *lv_jlgpu_yuv420_draw_task_create(lv_draw_jlgpu_unit_t *u, const lv_draw_image_dsc_t *dsc, lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t *img_buf, uint16_t flags, jlgpu_mask_param_t *mask_params, jlgpu_draw_img_t *draw_img_params, jlgpu_area_t *render_area, jlgpu_area_t *tex_clip_area, bool use_gpu_list);

/**********************
*  STATIC VARIABLES
**********************/

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/
void lv_draw_jlgpu_img(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords)
{
    if (dsc->opa <= LV_OPA_MIN) {
        return;
    }

    /*printf("coords (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", coords->x1, coords->y1, coords->x2, coords->y2);*/

    LV_PROFILER_BEGIN;

    lv_image_decoder_dsc_t decoder_dsc;
    lv_result_t res = lv_image_decoder_open(&decoder_dsc, dsc->src, NULL);
    if (res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        LV_PROFILER_END;
        return;
    }

    if (!dsc->tile) {
        /*The whole image is available, just draw it*/
        _jlgpu_draw_image_normal_helper(t, dsc, &decoder_dsc, coords);
    } else {

        int32_t img_w = dsc->header.w;
        int32_t img_h = dsc->header.h;

        lv_area_t tile_area;
        if (lv_area_get_width(&dsc->image_area) >= 0) {
            tile_area = dsc->image_area;
        } else {
            tile_area = *coords;
        }
        lv_area_set_width(&tile_area, img_w);
        lv_area_set_height(&tile_area, img_h);

        int32_t tile_x_start = tile_area.x1;

        lv_area_t relative_decoded_area = {
            .x1 = LV_COORD_MIN,
            .y1 = LV_COORD_MIN,
            .x2 = LV_COORD_MIN,
            .y2 = LV_COORD_MIN,
        };

        //save orign clip_area
        lv_area_t ori_clip_area = t->clip_area;

        while (tile_area.y1 <= coords->y2) {
            while (tile_area.x1 <= coords->x2) {
                lv_area_t clipped_img_area;
                if (lv_area_intersect(&clipped_img_area, &tile_area, coords)) {
                    t->clip_area = *coords;
                    _jlgpu_draw_image_normal_helper(t, dsc, &decoder_dsc, &tile_area);
                }
                tile_area.x1 += img_w;
                tile_area.x2 += img_w;
            }
            tile_area.y1 += img_h;
            tile_area.y2 += img_h;
            tile_area.x1 = tile_x_start;
            tile_area.x2 = tile_x_start + img_w - 1;
        }

        //drop clip_area
        t->clip_area = ori_clip_area;
    }

    lv_image_decoder_close(&decoder_dsc);


    LV_PROFILER_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _jlgpu_draw_image_normal_helper(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords)
{
    //printf("buf_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", layer->buf_area.x1, layer->buf_area.y1, layer->buf_area.x2, layer->buf_area.y2);
    jlgpu_err_t  gpu_err;
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    /* The coordinates passed in by coords are not transformed,
     * so the transformed area needs to be calculated once.
     */

    if (decoder_dsc->decoded == NULL) {
        LV_LOG_ERROR("decoder_dsc->decoded == NULL");
        return;
        //分行解码:todo
    }

    bool has_angle = dsc->rotation == 0 ? false : true ;
    bool has_zoom = (dsc->scale_x == LV_SCALE_NONE) && (dsc->scale_y == LV_SCALE_NONE) ? false : true;
    bool has_flip = (dsc->flip_h == 0) && (dsc->flip_v == 0) ? false : true;
    bool has_transform = (has_angle || has_zoom || has_flip);
    bool done = false;

    if (has_transform) {
        _jlgpu_draw_image_transform_helper(t, dsc, decoder_dsc, coords);
        return;
    }


    lv_area_t image_tf_area;
    lv_area_copy(&image_tf_area, coords);

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_image_tf_area;
    lv_area_copy(&rel_image_tf_area, &image_tf_area);
    lv_area_move(&rel_image_tf_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, &rel_image_tf_area, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }
    /*printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);*/

    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    int width = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);

    const lv_draw_buf_t *decoded = decoder_dsc->decoded;

    bool need_save_src_buf = false;
    if (decoded->header.reserved_2 == 0xff) {
        need_save_src_buf = true;
    }
    /* 解码器堆上分配的 buf(lv_bin_decoder 解码 .bin/.rle/.lzw 等得到的)会在
     * lv_image_decoder_close() -> free_decoder_data() 里被 lv_draw_buf_destroy() 释放,
     * 而 close 就发生在本函数返回之后的 lv_draw_jlgpu_img() 里。
     * 页面特效在 effect_enter 时把任务链整条留在 ctx 里逐帧重放, 链上引用的正是这里的
     * img_buf; buf 一旦被释放, 首帧之后堆被复用, 重放就会画出错乱内容
     * (表现为"第一帧正常, 第二帧开始跳动")。这类 buf 必须拷进常驻的 attach buf 链表。
     * 判据用 LV_IMAGE_FLAGS_ALLOCATED: 只有 lv_draw_buf_create_ex() 分配的堆 buf 带这个
     * 标志(见 lv_draw_buf_destroy/res_image_free 的同类判断); 直接指向 flash/SD 的
     * lv_jl_bin 描述符和 lv_draw_buf_from_image() 的 c_array 都不带, 不会多拷。 */
    if (decoded->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
        need_save_src_buf = true;
    }

    uint32_t src_width  = decoded->header.w;
    uint32_t src_height = decoded->header.h;
    uint32_t src_stride = decoded->header.stride;
    uint8_t  src_bpp    = lv_color_format_get_bpp(decoded->header.cf);
    uint8_t  in_format  = lv_jlgpu_get_color_format(decoded->header.cf);
    uint8_t *img_buf = decoded->data;

#if LV_USE_DRAW_JLGPU_LIST
    if (need_save_src_buf) {
        /* data_size 是 lv_draw_buf_create_ex() 分配时的整块大小(RGB565A8 的 A8 尾、
         * 索引图的调色板都算在内), 比 w*stride 更完整 */
        uint32_t save_size = decoded->data_size ? decoded->data_size : (src_width * src_stride);
        img_buf = lv_jlgpu_attach_buf_list_add(u, img_buf, save_size);
    }
#endif

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    jlgpu_draw_img_t draw_img_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};
    jlgpu_draw_img_grad_t draw_img_grad = {0};

    jlgpu_tex_inter_params_t tex_inter_params = {0};
    tex_inter_params.inter_mode = JLGPU_SAMPLE_MODE_NEAR;//jlgpu_tex_samplemode_t
    draw_img_params.tex_inter_params = &tex_inter_params;

    //recolor
    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_draw_img_fill_t draw_img_fill_params = {0};

    //径向渐变适配圆角
    jlgpu_matrix_t matrix = {0};
    jlgpu_roundgrad_param_t roundgrad_param = {0};
    jlgpu_paint_inter_params_t grad_inter_params = {0};
    jlgpu_draw_grad_t draw_grad = {0};
    int r0, g0, b0, a0, r1, g1, b1, a1;
    uint32_t color0 = 0;
    uint32_t color1 = 0;
    uint32_t clut_data[2] = {0};

    jlgpu_area_t render_area;
    render_area.x_min = blend_area.x1;
    render_area.x_max = blend_area.x2 + 1;
    render_area.y_min = blend_area.y1;
    render_area.y_max = blend_area.y2 + 1;

    jlgpu_area_t tex_clip_area;
    tex_clip_area.x_min = CLAMP_TO_ZERO(t->clip_area.x1 - coords->x1);
    tex_clip_area.x_max = src_width;
    tex_clip_area.y_min = CLAMP_TO_ZERO(t->clip_area.y1 - coords->y1);
    tex_clip_area.y_max = src_height;

    jlgpu_img_info_t img_info = {0};

    img_info.premult = 1;
    img_info.tex_clip = tex_clip_area;

    uint8_t adr_mode = JLGPU_ADRMODE_LINE;
    uint8_t rle_en = 0;
    uint8_t compress_type = LV_COMPRESS_NONE;
    uint8_t blocks = LV_RAW_BLOCK_NONE;
    lv_c3_image_t c3_image_info = {0};
    uint8_t *src_clut = NULL;
    uint16_t clut_format = JLGPU_FORMAT_ARGB8888;

#if LV_USE_GPU_COMPRESS
    if (decoder_dsc && decoder_dsc->user_data && ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed) {
        compress_type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed;
        blocks = ((jl_bin_user_data_t *)decoder_dsc->user_data)->blocks;
        if (blocks == LV_RAW_BLOCK_8x8 || blocks == LV_RAW_BLOCK_16x16) {
            adr_mode = JLGPU_ADRMODE_TILE;
        }
        if (compress_type == LV_COMPRESS_C3) {
            c3_image_info.has_table = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.has_table;
            c3_image_info.data_size = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.data_size;
            c3_image_info.type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.type;
        }
    }
    bool has_indexed = (in_format >= JLGPU_FORMAT_L8 && in_format <= JLGPU_FORMAT_L1) ? true : false;
    rle_info_t *rle_info;
    c2_info_t *c2_info;
    c3_info_t *c3_info;

    if (compress_type == LV_COMPRESS_RLE) {
        rle_en = 1;
        rle_info = (rle_info_t *)img_buf;
        uint16_t rle_img_stride = lv_rle_get_stride(rle_info, &adr_mode);
        if (has_indexed) {
            src_clut = (uint8_t *)img_buf + rle_info->lut_addr;
            clut_format = rle_info->lut_format;
        }
        img_info.adr_mode = adr_mode;//jlgpu_adr_mode_t
        img_info.color_format = rle_info->format;
        img_info.data = (uint8_t *)img_buf + rle_info->addr;
        img_info.width  = rle_info->width;
        img_info.height = rle_info->height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_RLE;
        img_info.compress_size = rle_info->len;
    } else if (compress_type == LV_COMPRESS_ETC2) {
        adr_mode = JLGPU_ADRMODE_TILE;
        c2_info = (c2_info_t *)img_buf;
        img_info.adr_mode = adr_mode;
        img_info.color_format = c2_info->format;
        img_info.data = (uint8_t *)img_buf + sizeof(c2_info_t);
        img_info.width  = c2_info->img_width;
        img_info.height = c2_info->img_height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_ETC2;
        img_info.compress_size = 0;
    } else if (compress_type == LV_COMPRESS_C3) {

        lv_jlgpu_mutex_pend();

        if (c3_image_info.has_table) {
            if (c3_image_info.type == LV_C3_TYPE_UITOOL) {
                lv_c3_load_qtable_type_uitool((uint8_t *)img_buf, c3_image_info.data_size);
            } else if (c3_image_info.type == LV_C3_TYPE_APP) {
                lv_c3_load_qtable_type_app((uint8_t *)img_buf, c3_image_info.data_size);
            } else {
                lv_c3_load_global_qtable();
            }
        } else {
            lv_c3_load_global_qtable();
        }

        //加载q表后需要偏移头部
        img_buf = (img_buf + sizeof(c3_head_t));

        lv_jlgpu_mutex_post();

    } else {
        if (has_indexed) {
            uint32_t palette_len = sizeof(lv_color32_t) * LV_COLOR_INDEXED_PALETTE_SIZE(decoded->header.cf);
            src_clut = img_buf;
            img_buf += palette_len;
        }
        img_info.adr_mode = adr_mode;
        img_info.color_format = in_format;
        img_info.data = img_buf;

        if ((src_bpp >> 3) != 0) {
            img_info.width = src_stride / (src_bpp >> 3);
        } else {
            img_info.width = src_width;
        }

        img_info.height = src_height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
        img_info.compress_size = 0;
    }
#else
    img_info.adr_mode = adr_mode;
    img_info.color_format = in_format;
    img_info.data = img_buf;
    if ((src_bpp >> 3) != 0) {
        img_info.width = src_stride / (src_bpp >> 3);
    } else {
        img_info.width = src_width;
    }
    img_info.height = src_height;
    img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
    img_info.compress_size = 0;
#endif

    if (img_info.color_format == JLGPU_FORMAT_YUV422_BT601 || img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
        img_info.rb_swap  = JLGPU_RBS_ABGR;
    }

    if ((img_info.color_format >= JLGPU_FORMAT_A4 && img_info.color_format <= JLGPU_FORMAT_A1) ||
        (img_info.color_format >= JLGPU_FORMAT_L4 && img_info.color_format <= JLGPU_FORMAT_L1)) {
        img_info.big_end = JLGPU_ENDIAN_BIG;
    }

    if (decoder_dsc && decoder_dsc->user_data) {
        lv_jlbin_tab_file_info_t *mmu_tab_file_info = &((jl_bin_user_data_t *)decoder_dsc->user_data)->tab_file_info; // mmu tab文件信息
        if (mmu_tab_file_info->tab && mmu_tab_file_info->tab_size) {
            //tex的mmu配置
            draw_img_params.tex_mmu.clut_en = 0;
            draw_img_params.tex_mmu.page_size = 0;
            draw_img_params.tex_mmu.tb_base = (uint8_t *)mmu_tab_file_info->tab;
            draw_img_params.tex_mmu.tex_en = 1;
            img_info.data = (uint8_t *)mmu_tab_file_info->offset;
        }
    }
    draw_img_params.clut_format = clut_format;
    draw_img_params.clut_adr = src_clut;

    draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_img_params.img_info = img_info;

    draw_img_params.ax_color.b = dsc->recolor.blue;
    draw_img_params.ax_color.g = dsc->recolor.green;
    draw_img_params.ax_color.r = dsc->recolor.red;
    draw_img_params.ax_color.a = dsc->recolor_opa;

    //recolor
    if (has_recolor) {
        draw_fill_params.paint_info.premult = 1;
        draw_fill_params.paint_info.pt_clip = render_area;
        draw_fill_params.color.b = dsc->recolor.blue;
        draw_fill_params.color.g = dsc->recolor.green;
        draw_fill_params.color.r = dsc->recolor.red;
        draw_fill_params.color.a = dsc->recolor_opa;
    }

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = render_area;
    render_params.blend_mode = lv_jlgpu_get_blend_mode(dsc->blend_mode, NULL);
    render_params.render_quality = JLGPU_RENDERING_QUALITY_NONANTIALIASED;//jlgpu_render_quality_t
    render_params.dither_en = 1;

    if (dsc->opa == LV_OPA_COVER) {
        render_params.global_alpha = 128;
    } else {
        render_params.global_alpha = dsc->opa / 2;
    }

    /* 圆角裁剪要按图像自己的外接矩形算, 不能用裁剪后的窗口:
     * 脏区渲染时窗口可能只覆盖图像的一部分, 拿窗口算会把圆角画到窗口的角上。 */
    int img_w = lv_area_get_width(&rel_image_tf_area);
    int img_h = lv_area_get_height(&rel_image_tf_area);
    int32_t short_side = LV_MIN(img_w, img_h);
    int32_t rout = LV_MIN(dsc->clip_radius, short_side >> 1);

    int edge_num = 0;

    if (rout) {
        if (LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
            r0 = 255;
            g0 = 255;
            b0 = 255;
            a0 = 255;

            r1 = 0;
            g1 = 0;
            b1 = 0;
            a1 = 0;

            color0 = ((uint32_t)a0 << 24)
                     | ((uint32_t)r0 << 16)
                     | ((uint32_t)g0 << 8)
                     | (uint32_t)b0;

            color1 = ((uint32_t)a1 << 24)
                     | ((uint32_t)r1 << 16)
                     | ((uint32_t)g1 << 8)
                     | (uint32_t)b1;

            clut_data[0] = color0;
            clut_data[1] = color1;

            uint8_t lut_lvl = 0;

            draw_grad.clut_level = lut_lvl;
            jlgpu_matrix_set_identity(&matrix);

            //圆角矩圆中心点
            roundgrad_param.point_center.x = rel_image_tf_area.x1 + img_w / 2;
            roundgrad_param.point_center.y = rel_image_tf_area.y1 + img_h / 2;

            //内框
            roundgrad_param.in_width  = (img_w - 2 * rout);
            roundgrad_param.in_height = (img_h - 2 * rout);

            //内外
            roundgrad_param.in_radius  = rout;
            roundgrad_param.out_radius = rout + 1;

            jlgpu_computeRoundParameters(draw_grad.clut_level, &roundgrad_param, &matrix);
            draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

            draw_grad.paint_info.premult = 1;
            draw_grad.paint_info.pt_clip.x_min = -4096;
            draw_grad.paint_info.pt_clip.x_max = 4095;
            draw_grad.paint_info.pt_clip.y_min = -4096;
            draw_grad.paint_info.pt_clip.y_max = 4095;
            draw_grad.grad_mode = JLGPU_GRAD_MODE_ROUND_GRADIENT;
            draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;

            grad_inter_params.coeff0 = 0;
            grad_inter_params.coeff1 = 0;
            grad_inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;//jlgpu_pt_samplemode_t

            if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
                draw_grad.roundgrad_ptr = (jlgpu_roundgrad_param_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&roundgrad_param, sizeof(jlgpu_roundgrad_param_t));
                draw_grad.clut_adr = (uint8_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data, sizeof(clut_data));
                draw_grad.grad_inter_params = (jlgpu_paint_inter_params_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&grad_inter_params, sizeof(jlgpu_paint_inter_params_t));
                draw_grad.surface2paint = (jlgpu_matrix_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&matrix, sizeof(jlgpu_matrix_t));
#endif
            } else {
                draw_grad.roundgrad_ptr = &roundgrad_param;
                draw_grad.clut_adr = (uint8_t *)clut_data;
                draw_grad.grad_inter_params = &grad_inter_params;
                draw_grad.surface2paint = &matrix;
            }
            //禁用path
            mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
            mask_params.mask_ptr = 0;
            draw_img_grad.draw_grad = draw_grad;
            draw_img_grad.draw_img = draw_img_params;
            draw_img_grad.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;

        } else {

            mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
            mask_params.mask_ptr = &jlgpu_path;

            int path_div = jlgpu_compute_div(render_params.render_area);

            jlgpu_vec_t *rect_path = lv_jlgpu_path_get(u);
            lv_jlgpu_path_set_bez_div(rect_path, LV_MIN(path_div, global_max_bez_div));

            jlgpu_path_append_rect(rect_path, (float)rel_image_tf_area.x1, (float)rel_image_tf_area.y1, img_w, img_h, rout);
            jlgpu_path_end(rect_path);

            edge_num = rect_path->edge_num;
            jlgpu_path.seg_len = rect_path->total_len;

            if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
                jlgpu_path.seg_data = lv_jlgpu_attach_buf_list_add(u, rect_path->base, rect_path->total_len);
#endif
            } else {
                jlgpu_path.seg_data = rect_path->base;
            }

            jlgpu_path.fill_rule = JLGPU_NON_ZERO;
            jlgpu_path.max_bez_div = rect_path->bez_div;
            jlgpu_path.user2surface = 0;
            jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;
            jlgpu_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;//jlgpu_render_quality_t
        }

    } else {
        mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
        mask_params.mask_ptr = 0;
    }

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        if (compress_type == LV_COMPRESS_C3) {
            uint8_t *ex_task = lv_jlgpu_c3_draw_task_create(u, dsc, u->jlgpu_ctx, img_buf, &mask_params, &draw_img_params, &render_area, &tex_clip_area, true);
            if (ex_task == NULL) {
                LV_LOG_ERROR("c3 task malloc fail");
                return;
            }
            lv_jlgpu_add_task_node(u, ex_task);
            uint8_t *task = jlgpu_task_get_next(ex_task);
            u->jlgpu_ctx->jlgpu_task_num++;
            u->jlgpu_ctx->cur_task = task;
        } else if (img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
            uint8_t *ex_task = lv_jlgpu_yuv420_draw_task_create(u, dsc, u->jlgpu_ctx, img_buf, decoded->header.flags, &mask_params, &draw_img_params, &render_area, &tex_clip_area, true);
            if (ex_task == NULL) {
                LV_LOG_ERROR("yuv420 task malloc fail");
                return;
            }
            lv_jlgpu_add_task_node(u, ex_task);
            uint8_t *task = jlgpu_task_get_next(ex_task);
            u->jlgpu_ctx->jlgpu_task_num++;
            u->jlgpu_ctx->cur_task = task;
        } else {
            uint8_t *task = NULL;
            int task_len = 0;
            if (rout && LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
                task_len = jlgpu_draw_img_grad_task_len(u->jlgpu_ctx->ctx, &draw_img_grad, &mask_params);
                task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                memset(task, 0, task_len);
                jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                jlgpu_draw_img_grad(u->jlgpu_ctx->ctx, &render_params, &draw_img_grad, &mask_params);
            } else {
                //recolor
                if (has_recolor) {
                    draw_img_fill_params.draw_fill = draw_fill_params;
                    draw_img_fill_params.draw_img = draw_img_params;
                    draw_img_fill_params.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;
                    task_len = jlgpu_draw_img_fill_task_len(u->jlgpu_ctx->ctx, &draw_img_fill_params, &mask_params);
                    task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                    jlgpu_draw_img_fill(u->jlgpu_ctx->ctx, &render_params, &draw_img_fill_params, &mask_params);
                } else {
                    task_len = jlgpu_draw_img_task_len(u->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
                    task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                    jlgpu_draw_img(u->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
                }

            }
            if (task == NULL) {
                /* 池+堆都申请失败:直接往下走会 lv_jlgpu_add_task_node(NULL) 破坏链表 */
                LV_LOG_ERROR("draw img task malloc fail");
                return;
            }
            /*jlgpu_dump_task(task);*/
            lv_jlgpu_add_task_node(u, task);
        }
#endif
    } else {//layer/canvas需要立即绘制
        lv_jlgpu_ctx_t jlgpu_ctx = {0};
        lv_jlgpu_ctx_init(&jlgpu_ctx);
        uint8_t *ex_task = NULL;
        uint8_t *task = NULL;
        int task_len = 0;
        jlgpu_err_t err = 0;

        if (compress_type == LV_COMPRESS_C3) {
            ex_task = lv_jlgpu_c3_draw_task_create(u, dsc, &jlgpu_ctx, img_buf, &mask_params, &draw_img_params, &render_area, &tex_clip_area, false);
            task = jlgpu_task_get_next(ex_task);
        } else if (img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
            ex_task = lv_jlgpu_yuv420_draw_task_create(u, dsc, &jlgpu_ctx, img_buf,  decoded->header.flags, &mask_params, &draw_img_params, &render_area, &tex_clip_area, false);
            task = jlgpu_task_get_next(ex_task);
        } else {
            if (rout && LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
                task_len = jlgpu_draw_img_grad_task_len(jlgpu_ctx.ctx, &draw_img_grad, &mask_params);
                task = lv_malloc(task_len);
                memset(task, 0, task_len);
                jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                jlgpu_draw_img_grad(jlgpu_ctx.ctx, &render_params, &draw_img_grad, &mask_params);
            } else {
                if (has_recolor) {
                    draw_img_fill_params.draw_fill = draw_fill_params;
                    draw_img_fill_params.draw_img = draw_img_params;
                    draw_img_fill_params.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;
                    task_len = jlgpu_draw_img_fill_task_len(jlgpu_ctx.ctx, &draw_img_fill_params, &mask_params);
                    task = lv_malloc(task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                    jlgpu_draw_img_fill(jlgpu_ctx.ctx, &render_params, &draw_img_fill_params, &mask_params);
                } else {
                    task_len = jlgpu_draw_img_task_len(jlgpu_ctx.ctx, &draw_img_params, &mask_params);
                    task = lv_malloc(task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                    jlgpu_draw_img(jlgpu_ctx.ctx, &render_params, &draw_img_params, &mask_params);
                }
            }
        }

        jlgpu_surface_t surface = {0};
        jlgpu_global_matrix_t global_matrix = {0};
        jlgpu_colorkey_t colorkey = {0};
        jlgpu_global_path_t global_path = {0};
        jlgpu_uv_adr_t uv_adr = {0};

        global_path.cbuf_split = 0;
        global_path.cbuf_stride = fb_width;
        global_path.max_bez_div = global_max_bez_div;
        global_path.max_edge_cfg = edge_num;

        int mask_h = global_path.cbuf_split ? 16 : 32; //8 + global_path.cbuf_split * 4;
        uint8_t *cbuf = (uint8_t *)lv_malloc(mask_h * fb_width);
        if (cbuf == NULL) {
            LV_LOG_ERROR("malloc cbuf error");
        }
        memset(cbuf, 0, mask_h * fb_width);
        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num); //edge 2word
        if (ebuf == NULL) {
            LV_LOG_ERROR("malloc ebuf error");
        }
        memset(ebuf, 0, 8 * edge_num);

        global_path.c_data = cbuf;
        global_path.e_data = ebuf;

        surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);
        surface.format = out_format;
        surface.window.x_min = 0;
        surface.window.x_max = fb_width;
        surface.window.y_min = 0;
        surface.window.y_max = fb_height;
        surface.width = fb_width;
        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);
        //jlgpu_dump_task(task);

        if (ex_task) {
            jlgpu_run(jlgpu_ctx.ctx, ex_task);
            lv_free(ex_task);
        } else {
            jlgpu_run(jlgpu_ctx.ctx, task);
        }

        lv_free(task);
        lv_free(cbuf);
        lv_free(ebuf);

        lv_jlgpu_ctx_uninit(&jlgpu_ctx);
    }

    done = true;

jlgpu_quit:

    if (!done) {
        LV_LOG_ERROR("Failed to gpu draw image");
    }

}

static bool _jlgpu_draw_image_transform_helper(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords)
{
    /*printf("coords (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", coords->x1, coords->y1, coords->x2, coords->y2);*/
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    /* The coordinates passed in by coords are not transformed,
     * so the transformed area needs to be calculated once.
     */

    if (decoder_dsc->decoded == NULL) {
        LV_LOG_ERROR("decoder_dsc->decoded == NULL");
        return false;
    }

    lv_area_t image_tf_area;
    lv_area_copy(&image_tf_area, coords);
    bool has_angle = dsc->rotation == 0 ? false : true ;
    bool has_zoom = (dsc->scale_x == LV_SCALE_NONE) && (dsc->scale_y == LV_SCALE_NONE) ? false : true;

    bool has_flip_h = (dsc->flip_h) ? true : false;
    bool has_flip_v = (dsc->flip_v) ? true : false;

    if (has_angle || has_zoom) {
        lv_image_buf_get_transformed_area(
            &image_tf_area,
            lv_area_get_width(coords),
            lv_area_get_height(coords),
            dsc->rotation,
            dsc->scale_x,
            dsc->scale_y,
            &dsc->pivot);

        lv_area_move(&image_tf_area, coords->x1, coords->y1);

        /* 缩放分支 lv_point_transform 用 (x*scale)>>8 向下取整，非整数倍缩放时
         * 右/下边界会被截断少 1 像素；旋转分支走三角函数，边界已准确。
         * 因此仅对纯缩放(angle==0)且非整数倍时补回右/下 1 像素，旋转不补偿*/
        if (has_zoom && !has_angle) {
            if (((int32_t)lv_area_get_width(coords) * dsc->scale_x) & 0xff) {
                image_tf_area.x2++;
            }
            if (((int32_t)lv_area_get_height(coords) * dsc->scale_y) & 0xff) {
                image_tf_area.y2++;
            }
        }
    }

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_image_tf_area;
    lv_area_copy(&rel_image_tf_area, &image_tf_area);
    lv_area_move(&rel_image_tf_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, &rel_image_tf_area, &rel_clip_area)) {
        return true; /*Fully clipped, nothing to do*/
    }

    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;
    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    int width = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);

    const lv_draw_buf_t *decoded = decoder_dsc->decoded;

    bool need_save_src_buf = false;
    if (decoded->header.reserved_2 == 0xff) {
        need_save_src_buf = true;
    }
    /* 同 _jlgpu_draw_image_normal_helper: 解码器堆上的 buf 会在
     * lv_image_decoder_close() 里被释放, 而页面特效的任务链要跨帧重放, 必须拷贝留底 */
    if (decoded->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
        need_save_src_buf = true;
    }

    uint32_t src_width = decoded->header.w;
    uint32_t src_height = decoded->header.h;
    uint32_t src_stride = decoded->header.stride;

    uint8_t  src_bpp    = lv_color_format_get_bpp(decoded->header.cf);
    uint8_t in_format = lv_jlgpu_get_color_format(decoded->header.cf);
    uint8_t *img_buf = decoded->data;

#if LV_USE_DRAW_JLGPU_LIST
    if (need_save_src_buf) {
        /* data_size 是 lv_draw_buf_create_ex() 分配时的整块大小(RGB565A8 的 A8 尾、
         * 索引图的调色板都算在内), 比 w*stride 更完整 */
        uint32_t save_size = decoded->data_size ? decoded->data_size : (src_width * src_stride);
        img_buf = lv_jlgpu_attach_buf_list_add(u, img_buf, save_size);//A8
    }
#endif

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    float map_zoom = (dsc->scale_x * 1.0f) / 256;
    float zoom_x = dsc->scale_x;
    float zoom_y = dsc->scale_y;

    zoom_x = 256 / (zoom_x * 1.0f);
    zoom_y = 256 / (zoom_y * 1.0f);

    lv_area_t src_area;
    src_area.x1 = coords->x1;
    src_area.y1 = coords->y1;
    src_area.x2 = src_area.x1 + lv_area_get_width(coords)  - 1;
    src_area.y2 = src_area.y1 + lv_area_get_height(coords) - 1;

    jlgpu_draw_img_t draw_img_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};
    jlgpu_draw_img_grad_t draw_img_grad = {0};

    //recolor
    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_draw_img_fill_t draw_img_fill_params = {0};

    jlgpu_matrix_t matrix;

    //径向渐变适配圆角
    jlgpu_matrix_t matrix_grad = {0};
    jlgpu_roundgrad_param_t roundgrad_param = {0};
    jlgpu_paint_inter_params_t grad_inter_params = {0};
    jlgpu_draw_grad_t draw_grad = {0};
    int r0, g0, b0, a0, r1, g1, b1, a1;
    uint32_t color0 = 0;
    uint32_t color1 = 0;
    uint32_t clut_data[2] = {0};

    jlgpu_area_t render_area;
    render_area.x_min = CLAMP_TO_ZERO(blend_area.x1);
    render_area.x_max = CLAMP_TO_ZERO(blend_area.x2 + 1);
    render_area.y_min = CLAMP_TO_ZERO(blend_area.y1);
    render_area.y_max = CLAMP_TO_ZERO(blend_area.y2 + 1);

    jlgpu_area_t tex_clip_area;
    tex_clip_area.x_min = 0;
    tex_clip_area.x_max = src_width;
    tex_clip_area.y_min = 0;
    tex_clip_area.y_max = src_height;

    jlgpu_img_info_t img_info = {0};

    img_info.premult = 1;
    img_info.tex_clip = tex_clip_area;

    uint8_t adr_mode = JLGPU_ADRMODE_LINE;
    uint8_t rle_en = 0;
    uint8_t compress_type = LV_COMPRESS_NONE;
    uint8_t blocks = LV_RAW_BLOCK_NONE;
    lv_c3_image_t c3_image_info = {0};
    uint8_t *src_clut = NULL;
    uint16_t clut_format = JLGPU_FORMAT_ARGB8888;

#if LV_USE_GPU_COMPRESS

    if (decoder_dsc && decoder_dsc->user_data && ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed) {
        compress_type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed;
        blocks = ((jl_bin_user_data_t *)decoder_dsc->user_data)->blocks;
        if (blocks == LV_RAW_BLOCK_8x8 || blocks == LV_RAW_BLOCK_16x16) {
            adr_mode = JLGPU_ADRMODE_TILE;
        }
        if (compress_type == LV_COMPRESS_C3) {
            c3_image_info.has_table = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.has_table;
            c3_image_info.data_size = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.data_size;
            c3_image_info.type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->c3_image_info.type;
        }
    }

    bool has_indexed = (in_format >= JLGPU_FORMAT_L8 && in_format <= JLGPU_FORMAT_L1) ? true : false;
    rle_info_t *rle_info;
    c2_info_t *c2_info;
    c3_info_t *c3_info;

    if (compress_type == LV_COMPRESS_RLE) {
        rle_en = 1;
        rle_info = (rle_info_t *)img_buf;
        uint16_t rle_img_stride = lv_rle_get_stride(rle_info, &adr_mode);
        if (has_indexed) {
            src_clut = (uint8_t *)img_buf + rle_info->lut_addr;
            clut_format = rle_info->lut_format;
        }

        img_info.adr_mode = adr_mode;//jlgpu_adr_mode_t
        img_info.color_format = rle_info->format;
        img_info.data = (uint8_t *)img_buf + rle_info->addr;
        img_info.width  = rle_info->width;
        img_info.height = rle_info->height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_RLE;
        img_info.compress_size = rle_info->len;

    } else if (compress_type == LV_COMPRESS_ETC2) {
        adr_mode = JLGPU_ADRMODE_TILE;
        c2_info = (c2_info_t *)img_buf;
        img_info.adr_mode = adr_mode;
        img_info.color_format = c2_info->format;
        img_info.data = (uint8_t *)img_buf + sizeof(c2_info_t);
        img_info.width  = c2_info->img_width;
        img_info.height = c2_info->img_height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_ETC2;
        img_info.compress_size = 0;

    } else if (compress_type == LV_COMPRESS_C3) {

        lv_jlgpu_mutex_pend();

        if (c3_image_info.has_table) {
            if (c3_image_info.type == LV_C3_TYPE_UITOOL) {
                lv_c3_load_qtable_type_uitool((uint8_t *)img_buf, c3_image_info.data_size);
            } else if (c3_image_info.type == LV_C3_TYPE_APP) {
                lv_c3_load_qtable_type_app((uint8_t *)img_buf, c3_image_info.data_size);
            } else {
                lv_c3_load_global_qtable();
            }
        } else {
            lv_c3_load_global_qtable();
        }

        //加载q表后需要偏移头部
        img_buf = img_buf + sizeof(c3_head_t);

        lv_jlgpu_mutex_post();

    } else {
        if (has_indexed) {
            src_clut = (uint8_t *)img_buf;
            uint32_t palette_len = sizeof(lv_color32_t) * LV_COLOR_INDEXED_PALETTE_SIZE(decoded->header.cf);
            src_clut = img_buf;
            img_buf += palette_len;
        }

        img_info.adr_mode = adr_mode;
        img_info.color_format = in_format;
        img_info.data = img_buf;
        if ((src_bpp >> 3) != 0) {
            img_info.width = src_stride / (src_bpp >> 3);
        } else {
            img_info.width = src_width;
        }
        img_info.height = src_height;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
        img_info.compress_size = 0;
    }

#else
    img_info.adr_mode = adr_mode;
    img_info.color_format = in_format;
    img_info.data = img_buf;
    img_info.height = src_height;
    if ((src_bpp >> 3) != 0) {
        img_info.width = src_stride / (src_bpp >> 3);
    } else {
        img_info.width = src_width;
    }
    img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
    img_info.compress_size = 0;
#endif

    if (img_info.color_format == JLGPU_FORMAT_YUV422_BT601 || img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
        img_info.rb_swap  = JLGPU_RBS_ABGR;
    }

    if ((img_info.color_format >= JLGPU_FORMAT_A4 && img_info.color_format <= JLGPU_FORMAT_A1) ||
        (img_info.color_format >= JLGPU_FORMAT_L4 && img_info.color_format <= JLGPU_FORMAT_L1)) {
        img_info.big_end = JLGPU_ENDIAN_BIG;
    }

    if (decoder_dsc && decoder_dsc->user_data) {
        lv_jlbin_tab_file_info_t *mmu_tab_file_info = &((jl_bin_user_data_t *)decoder_dsc->user_data)->tab_file_info; // mmu tab文件信息
        if (mmu_tab_file_info->tab && mmu_tab_file_info->tab_size) {
            //tex的mmu配置
            draw_img_params.tex_mmu.clut_en = 0;
            draw_img_params.tex_mmu.page_size = 0;
            draw_img_params.tex_mmu.tb_base = (uint8_t *)mmu_tab_file_info->tab;
            draw_img_params.tex_mmu.tex_en = 1;
            img_info.data = (uint8_t *)mmu_tab_file_info->offset;
        }
    }

    draw_img_params.clut_format = clut_format;
    draw_img_params.clut_adr = src_clut;

    draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_AFFINE;
    draw_img_params.img_info = img_info;

    draw_img_params.ax_color.b = dsc->recolor.blue;
    draw_img_params.ax_color.g = dsc->recolor.green;
    draw_img_params.ax_color.r = dsc->recolor.red;
    draw_img_params.ax_color.a = dsc->recolor_opa;

    //recolor
    if (has_recolor) {
        draw_fill_params.paint_info.premult = 1;
        draw_fill_params.paint_info.pt_clip = render_area;
        draw_fill_params.color.b = dsc->recolor.blue;
        draw_fill_params.color.g = dsc->recolor.green;
        draw_fill_params.color.r = dsc->recolor.red;
        draw_fill_params.color.a = dsc->recolor_opa;
    }

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = render_area;
    render_params.blend_mode = lv_jlgpu_get_blend_mode(dsc->blend_mode, NULL);
    render_params.render_quality = JLGPU_RENDER_QUALITY;//jlgpu_render_quality_t
    render_params.dither_en = 1;

    if (dsc->opa == LV_OPA_COVER) {
        render_params.global_alpha = 128;
    } else {
        render_params.global_alpha = dsc->opa / 2;
    }

    int32_t short_side = LV_MIN(src_width, src_height);
    int32_t rout = LV_MIN(dsc->clip_radius, short_side >> 1);

    int edge_num = 0;

    if (rout) {
        if (LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
            r0 = 255;
            g0 = 255;
            b0 = 255;
            a0 = 255;

            r1 = 0;
            g1 = 0;
            b1 = 0;
            a1 = 0;

            color0 = ((uint32_t)a0 << 24)
                     | ((uint32_t)r0 << 16)
                     | ((uint32_t)g0 << 8)
                     | (uint32_t)b0;

            color1 = ((uint32_t)a1 << 24)
                     | ((uint32_t)r1 << 16)
                     | ((uint32_t)g1 << 8)
                     | (uint32_t)b1;

            clut_data[0] = color0;
            clut_data[1] = color1;

            uint8_t lut_lvl = 0;
            draw_grad.clut_level = lut_lvl;
            jlgpu_matrix_set_identity(&matrix_grad);

            //圆角矩圆中心点
            roundgrad_param.point_center.x = src_area.x1 + src_width / 2;
            roundgrad_param.point_center.y = src_area.y1 + src_height / 2;

            //内框
            roundgrad_param.in_width  = (src_width - 2 * rout);
            roundgrad_param.in_height = (src_height - 2 * rout);

            //内外
            roundgrad_param.in_radius  = rout - 1;
            roundgrad_param.out_radius = rout;

            jlgpu_computeRoundParameters(draw_grad.clut_level, &roundgrad_param, &matrix_grad);
            draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

            draw_grad.paint_info.premult = 1;
            draw_grad.paint_info.pt_clip.x_min = -4096;
            draw_grad.paint_info.pt_clip.x_max = 4095;
            draw_grad.paint_info.pt_clip.y_min = -4096;
            draw_grad.paint_info.pt_clip.y_max = 4095;
            draw_grad.grad_mode = JLGPU_GRAD_MODE_ROUND_GRADIENT;
            draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;

            grad_inter_params.coeff0 = 0;
            grad_inter_params.coeff1 = 0;
            grad_inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;//jlgpu_pt_samplemode_t

            if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
                draw_grad.roundgrad_ptr = (jlgpu_roundgrad_param_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&roundgrad_param, sizeof(jlgpu_roundgrad_param_t));
                draw_grad.clut_adr = (uint8_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data, sizeof(clut_data));
                draw_grad.grad_inter_params = (jlgpu_paint_inter_params_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&grad_inter_params, sizeof(jlgpu_paint_inter_params_t));
                draw_grad.surface2paint = (jlgpu_matrix_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)&matrix_grad, sizeof(jlgpu_matrix_t));
#endif
            } else {
                draw_grad.roundgrad_ptr = &roundgrad_param;
                draw_grad.clut_adr = (uint8_t *)clut_data;
                draw_grad.grad_inter_params = &grad_inter_params;
                draw_grad.surface2paint = &matrix_grad;
            }

            //禁用path
            mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
            mask_params.mask_ptr = 0;
            draw_img_grad.draw_grad = draw_grad;
            draw_img_grad.draw_img = draw_img_params;
            draw_img_grad.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;

        } else {

            mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
            mask_params.mask_ptr = &jlgpu_path;

            int path_div = jlgpu_compute_div(render_params.render_area);

            jlgpu_vec_t *rect_path = lv_jlgpu_path_get(u);
            lv_jlgpu_path_set_bez_div(rect_path, LV_MIN(path_div, global_max_bez_div));

            jlgpu_path_append_rect(rect_path, (float)src_area.x1, (float)src_area.y1, src_width, src_height, rout);
            jlgpu_path_end(rect_path);

            edge_num = rect_path->edge_num;
            jlgpu_path.seg_len = rect_path->total_len;

            if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
                jlgpu_path.seg_data = lv_jlgpu_attach_buf_list_add(u, rect_path->base, rect_path->total_len);
#endif
            } else {
                jlgpu_path.seg_data = rect_path->base;
            }

            jlgpu_path.fill_rule = JLGPU_NON_ZERO;
            jlgpu_path.max_bez_div = rect_path->bez_div;
            jlgpu_path.user2surface = 0;
            jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;
            jlgpu_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;
        }

    } else {
        mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
        mask_params.mask_ptr = 0;
    }

    jlgpu_matrix_set_identity(&matrix);
    int x_ofs = src_area.x1;
    int y_ofs = src_area.y1;

    float rotation = -(dsc->rotation / 10) * 3.1415926 / 180.0f;

    if (has_angle || has_zoom) {
        if (has_angle && (!has_zoom)) { // 仅旋转
            jlgpu_matrix_translate(&matrix, - dsc->pivot.x - x_ofs, - dsc->pivot.y - y_ofs);
            jlgpu_matrix_rotate(&matrix, rotation);
            jlgpu_matrix_translate(&matrix, dsc->pivot.x, dsc->pivot.y);
        } else if (has_zoom && (!has_angle)) {  // 仅缩放
            x_ofs = lv_map(x_ofs, 0, x_ofs, 0, x_ofs * zoom_x);
            y_ofs = lv_map(y_ofs, 0, y_ofs, 0, y_ofs * zoom_y);

            jlgpu_matrix_translate(&matrix, - dsc->pivot.x, - dsc->pivot.y);
            jlgpu_matrix_scale(&matrix, zoom_x, zoom_y);
            jlgpu_matrix_translate(&matrix, dsc->pivot.x - x_ofs, dsc->pivot.y - y_ofs);
        } else {// 缩放+旋转
            jlgpu_matrix_translate(&matrix, -dsc->pivot.x - x_ofs, -dsc->pivot.y - y_ofs);
            jlgpu_matrix_scale(&matrix, zoom_x, zoom_y);
            jlgpu_matrix_rotate(&matrix, rotation);
            jlgpu_matrix_translate(&matrix, dsc->pivot.x, dsc->pivot.y);
        }
    } else if (has_flip_h || has_flip_v) {

        jlgpu_matrix_translate(&matrix, - dsc->pivot.x, - dsc->pivot.y);

        if (has_flip_h) {
            jlgpu_matrix_flip(&matrix, 1, 0);
            jlgpu_matrix_translate(&matrix, dsc->pivot.x + x_ofs, dsc->pivot.y - y_ofs);
        } else if (has_flip_v) {
            jlgpu_matrix_flip(&matrix, 0, 1);
            jlgpu_matrix_translate(&matrix, dsc->pivot.x - x_ofs, dsc->pivot.y + y_ofs);
        }
    }

    draw_img_params.surface2image = &matrix;
    jlgpu_tex_inter_params_t tex_inter_params = {0};
    tex_inter_params.inter_mode = JLGPU_SAMPLE_MODE_NEAR;
    draw_img_params.tex_inter_params = &tex_inter_params;

#if 0
    jlgpu_path.path_trans_mode = compute_jlgpu_trans_mode(&matrix_path);
    if (jlgpu_path.path_trans_mode != JLGPU_TRANS_MODE_NONE) {
        jlgpu_matrix_invert(&matrix_path, &matrix);
        jlgpu_path.user2surface = &matrix_path;
    }
#endif

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        if (compress_type == LV_COMPRESS_C3) {
            uint8_t *ex_task = lv_jlgpu_c3_draw_task_create(u, dsc, u->jlgpu_ctx, img_buf, &mask_params, &draw_img_params, &render_area, &tex_clip_area, true);
            if (ex_task == NULL) {
                LV_LOG_ERROR("c3 task malloc fail");
                return false;
            }
            lv_jlgpu_add_task_node(u, ex_task);
            uint8_t *task = jlgpu_task_get_next(ex_task);
            u->jlgpu_ctx->jlgpu_task_num++;
            u->jlgpu_ctx->cur_task = task;
        } else if (img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
            uint8_t *ex_task = lv_jlgpu_yuv420_draw_task_create(u, dsc, u->jlgpu_ctx, img_buf, decoded->header.flags, &mask_params, &draw_img_params, &render_area, &tex_clip_area, true);
            if (ex_task == NULL) {
                LV_LOG_ERROR("yuv420 task malloc fail");
                return false;
            }
            lv_jlgpu_add_task_node(u, ex_task);
            uint8_t *task = jlgpu_task_get_next(ex_task);
            u->jlgpu_ctx->jlgpu_task_num++;
            u->jlgpu_ctx->cur_task = task;
        } else {
            uint8_t *task = NULL;
            int task_len = 0;
            if (rout && LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
                draw_img_grad.draw_img = draw_img_params;
                int task_len = jlgpu_draw_img_grad_task_len(u->jlgpu_ctx->ctx, &draw_img_grad, &mask_params);
                task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                memset(task, 0, task_len);
                jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                jlgpu_draw_img_grad(u->jlgpu_ctx->ctx, &render_params, &draw_img_grad, &mask_params);
            } else {
                //recolor
                if (has_recolor) {
                    draw_fill_params.surface2paint = &matrix;
                    draw_img_fill_params.draw_fill = draw_fill_params;
                    draw_img_fill_params.draw_img = draw_img_params;
                    draw_img_fill_params.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;
                    task_len = jlgpu_draw_img_fill_task_len(u->jlgpu_ctx->ctx, &draw_img_fill_params, &mask_params);
                    task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                    jlgpu_draw_img_fill(u->jlgpu_ctx->ctx, &render_params, &draw_img_fill_params, &mask_params);
                } else {
                    task_len = jlgpu_draw_img_task_len(u->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
                    task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
                    jlgpu_draw_img(u->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
                }

            }
            if (task == NULL) {
                /* 池+堆都申请失败:直接往下走会 lv_jlgpu_add_task_node(NULL) 破坏链表 */
                LV_LOG_ERROR("draw img task malloc fail");
                return false;
            }
            /*jlgpu_dump_task(task);*/
            lv_jlgpu_add_task_node(u, task);
        }

#endif
    } else {//layer/canvas需要立即绘制
        lv_jlgpu_ctx_t jlgpu_ctx = {0};
        lv_jlgpu_ctx_init(&jlgpu_ctx);
        uint8_t *ex_task = NULL;
        uint8_t *task = NULL;
        int task_len = 0;
        jlgpu_err_t err = 0;

        if (compress_type == LV_COMPRESS_C3) {
            ex_task = lv_jlgpu_c3_draw_task_create(u, dsc, &jlgpu_ctx, img_buf, &mask_params, &draw_img_params, &render_area, &tex_clip_area, false);
            task = jlgpu_task_get_next(ex_task);
        } else if (img_info.color_format == JLGPU_FORMAT_YUV420_BT601) {
            ex_task = lv_jlgpu_yuv420_draw_task_create(u, dsc, &jlgpu_ctx, img_buf, decoded->header.flags, &mask_params, &draw_img_params, &render_area, &tex_clip_area, false);
            task = jlgpu_task_get_next(ex_task);
        } else {
            if (rout && LV_USE_GPU_GRAD_TO_ROUND && !has_recolor) {
                draw_img_grad.draw_img = draw_img_params;
                task_len = jlgpu_draw_img_grad_task_len(jlgpu_ctx.ctx, &draw_img_grad, &mask_params);
                task = lv_malloc(task_len);
                memset(task, 0, task_len);
                jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                jlgpu_draw_img_grad(jlgpu_ctx.ctx, &render_params, &draw_img_grad, &mask_params);
            } else {
                if (has_recolor) {
                    draw_img_fill_params.draw_fill = draw_fill_params;
                    draw_img_fill_params.draw_img = draw_img_params;
                    draw_img_fill_params.paint_mode = JLGPU_PAINT_MODE_MULTIPLY;
                    task_len = jlgpu_draw_img_fill_task_len(jlgpu_ctx.ctx, &draw_img_fill_params, &mask_params);
                    task = lv_malloc(task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                    jlgpu_draw_img_fill(jlgpu_ctx.ctx, &render_params, &draw_img_fill_params, &mask_params);
                } else {
                    task_len = jlgpu_draw_img_task_len(jlgpu_ctx.ctx, &draw_img_params, &mask_params);
                    task = lv_malloc(task_len);
                    lv_memset(task, 0, task_len);
                    jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
                    jlgpu_draw_img(jlgpu_ctx.ctx, &render_params, &draw_img_params, &mask_params);
                }
            }
        }

        /*jlgpu_err_t gpu_err = jlgpu_get_err(jlgpu_ctx.ctx);*/

        jlgpu_surface_t surface = {0};
        jlgpu_global_matrix_t global_matrix = {0};
        jlgpu_colorkey_t colorkey = {0};
        jlgpu_global_path_t global_path = {0};
        jlgpu_uv_adr_t uv_adr = {0};

        global_path.cbuf_split = 0;
        global_path.cbuf_stride = fb_width;
        global_path.max_bez_div = global_max_bez_div;
        global_path.max_edge_cfg = edge_num;

        int mask_h = global_path.cbuf_split ? 16 : 32; //8 + global_path.cbuf_split * 4;
        uint8_t *cbuf = (uint8_t *)lv_malloc((mask_h * fb_width + 1) >> 1);
        lv_memset(cbuf, 0, (mask_h * fb_width + 1) >> 1);

        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num); //edge 2word
        lv_memset(ebuf, 0, 8 * edge_num);

        global_path.c_data = cbuf;
        global_path.e_data = ebuf;

        surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);
        surface.window.x_min = layer->buf_area.x1;
        surface.window.x_max = layer->buf_area.x2 + 1;
        surface.window.y_min = layer->buf_area.y1;
        surface.window.y_max = layer->buf_area.y2 + 1;

        surface.format = out_format;
        surface.width = fb_width;

        jlgpu_init(jlgpu_ctx.ctx);

        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);

        if (ex_task) {
            jlgpu_run(jlgpu_ctx.ctx, ex_task);
            lv_free(ex_task);
        } else {
            jlgpu_run(jlgpu_ctx.ctx, task);
        }

        lv_free(task);
        lv_free(cbuf);
        lv_free(ebuf);

        lv_jlgpu_ctx_uninit(&jlgpu_ctx);
    }

    return true;

jlgpu_transform_quit:

    return false;
}


static void _jlgpu_draw_image_tiled_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords)
{

}

static uint8_t *lv_jlgpu_c3_draw_task_create(lv_draw_jlgpu_unit_t *u, const lv_draw_image_dsc_t *dsc, lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t *img_buf, jlgpu_mask_param_t *mask_params, jlgpu_draw_img_t *draw_img_params, jlgpu_area_t *render_area, jlgpu_area_t *tex_clip_area, bool use_gpu_list)
{
    c3_info_t *c3_info = (c3_info_t *)img_buf;

    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);

    //创建拓展指令task
    jlgpu_extend_param_t extend_params = {0};
    jlgpu_gxdec_t gxdec_params = {0};

    if (draw_img_params->tex_mmu.tex_en) {
        gxdec_params.img_base = draw_img_params->img_info.data;
        gxdec_params.mmu_en = 1;
        gxdec_params.page_size = 0;
        gxdec_params.tb_base = draw_img_params->tex_mmu.tb_base ;//table基地址
    } else {
        gxdec_params.img_base = img_buf;
    }
    extend_params.layer_en = 1;
    extend_params.breakpoint_en = 0;
    extend_params.command = JLGPU_EXTEND_COMMAND_CONFIG_GXDEC;
    extend_params.command_gxdec = &gxdec_params;

    int ex_task_len = jlgpu_extend_command_task_len(jlgpu_ctx->ctx, &extend_params);
    uint8_t *ex_task = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && use_gpu_list) {
#if LV_USE_DRAW_JLGPU_LIST
        ex_task = lv_jlgpu_mem_pool_get_buf(u, NULL, ex_task_len);
#endif
    } else {
        ex_task = lv_malloc(ex_task_len);
    }

    if (ex_task == NULL) {
        LV_LOG_ERROR("c3 ex_task malloc fail");
        return NULL;
    }

    lv_memset(ex_task, 0, ex_task_len);
    jlgpu_set_task(jlgpu_ctx->ctx, ex_task, ex_task_len);
    jlgpu_extend_command(jlgpu_ctx->ctx, &extend_params);
    /*jlgpu_dump_task(ex_task);*/

    //创建一个draw img hal上下文
    jlgpu_hal_context_t *ctx_hal = NULL;
    ctx_hal = jlgpu_hal_create_context();

    jlgpu_draw_mode_t draw_mode = JLGPU_DRAW_IMG;

    //recolor
    if (has_recolor) {
        draw_mode = JLGPU_DRAW_IMG_FILL;
    }

    jlgpu_mask_mode_t mask_mode = mask_params->mask_mode;
    jlgpu_paint_mode_t render_type = JLGPU_PAINT_MODE_MULTIPLY;
    jlgpu_trans_mode_t	tex_trans_mode = draw_img_params->tex_trans_mode;
    jlgpu_trans_mode_t	pnt_trans_mode = JLGPU_TRANS_MODE_NONE;
    jlgpu_trans_mode_t	path_trans_mode = JLGPU_TRANS_MODE_NONE;

    jlgpu_path_t *jlgpu_path = NULL;
    jlgpu_mask_t *jlgpu_mask = NULL;

    if (mask_mode == JLGPU_MASK_MODE_VECTOR) {
        jlgpu_path = (jlgpu_path_t *)(mask_params->mask_ptr);
        path_trans_mode = jlgpu_path->path_trans_mode;
    } else if (mask_mode == JLGPU_MASK_MODE_EXT) {
        jlgpu_mask = (jlgpu_mask_t *)(mask_params->mask_ptr);
    }

    gpu_hw_task_set_render_param(ctx_hal, draw_mode, mask_mode, render_type, tex_trans_mode, pnt_trans_mode, path_trans_mode);

    uint8_t task_len = gpu_hw_task_get_len_render(ctx_hal);
    uint8_t *task = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && use_gpu_list) {
#if LV_USE_DRAW_JLGPU_LIST
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
#endif
    } else {
        task = lv_malloc(task_len);
    }

    if (task == NULL) {
        LV_LOG_ERROR("img task malloc fail");
        jlgpu_hal_destroy_context(ctx_hal);
        return NULL;
    }

    memset(task, 0, task_len);
    gpu_hw_task_set_task(ctx_hal, task);
    //basic
    gpu_hw_task_set_render_common(ctx_hal);
    gpu_hw_task_set_layer_en(ctx_hal, 1);
    gpu_hw_task_set_breakpoint_en(ctx_hal, 0);
    gpu_hw_task_set_blend_mode(ctx_hal, lv_jlgpu_get_blend_mode(dsc->blend_mode, NULL));
    gpu_hw_task_set_dither_en(ctx_hal, 1);
    gpu_hw_task_set_gb_trans_en(ctx_hal, 0);
    gpu_hw_task_set_rgb_limit_en(ctx_hal, 0);
    gpu_hw_task_set_global_alpha(ctx_hal, 128);

    if (draw_img_params->tex_trans_mode == JLGPU_TRANS_MODE_AFFINE) {
        gpu_hw_task_set_layer_quality(ctx_hal, JLGPU_RENDER_QUALITY);
    } else {
        gpu_hw_task_set_layer_quality(ctx_hal, JLGPU_RENDERING_QUALITY_NONANTIALIASED);
    }

    gpu_hw_task_set_bbox(ctx_hal, render_area->x_min, render_area->x_max, render_area->y_min, render_area->y_max);
    //tex
    gpu_hw_task_set_color_ext(ctx_hal, JLGPU_COLOR_EXTEND_HIGH);
    gpu_hw_task_set_tex_premult(ctx_hal, 1);
    gpu_hw_task_set_boundary_ext(ctx_hal, JLGPU_BOUNDARY_EXTEND_ZERO);
    gpu_hw_task_set_colorkey_en(ctx_hal, 0);
    gpu_hw_task_set_tex_clip(ctx_hal, tex_clip_area->x_min, tex_clip_area->x_max, tex_clip_area->y_min, tex_clip_area->y_max);

    jlgpu_alpha_end_t alpha_end = JLGPU_ALPHA_END_ARGB;
    jlgpu_rbs_t rb_swap = JLGPU_RBS_ARGB;
    jlgpu_endian_t big_end = JLGPU_ENDIAN_LITTLE;

    uint8_t *input_data = (uint8_t *)0x50000000;
    gpu_hw_task_set_tex_info(ctx_hal, c3_info->img_width, c3_info->img_height, 4096 * 4 * 8, JLGPU_FORMAT_ARGB8888, JLGPU_ADRMODE_TILE, JLGPU_COMPRESS_MODE_NONE, rb_swap, big_end, alpha_end, 0, input_data);

    //recolor
    if (has_recolor) {
        gpu_hw_task_set_pnt_type(ctx_hal, JLGPU_PAINT_TYPE_COLOR, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_premult(ctx_hal, JLGPU_PREMULT_TRUE, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_color(ctx_hal, dsc->recolor.red, dsc->recolor.green, dsc->recolor.blue, dsc->recolor_opa, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_clip(ctx_hal, render_area->x_min, render_area->x_max, render_area->y_min, render_area->y_max, JLGPU_PAINT_INDEX_0);
    }

    //path
    if (mask_mode == JLGPU_MASK_MODE_VECTOR) {
        gpu_hw_task_set_fill_rule(ctx_hal, JLGPU_NON_ZERO);
        gpu_hw_task_set_path_render_quality(ctx_hal, JLGPU_PATH_MSAA_LVL);
        gpu_hw_task_set_bez_div(ctx_hal, JLGPU_GLOBAL_MAX_BEZ_DIV);
        gpu_hw_task_set_path(ctx_hal, jlgpu_path->seg_len, jlgpu_path->seg_data);

        if (path_trans_mode != JLGPU_TRANS_MODE_NONE) {
            gpu_hw_task_set_path_matrix(ctx_hal, (float *)jlgpu_path->user2surface);
        }
    }

    if (draw_img_params->surface2image) {
        gpu_hw_task_set_tex_matrix(ctx_hal, (float *)(draw_img_params->surface2image));
    }
    gpu_hw_task_set_tex_sample_param(ctx_hal, JLGPU_SAMPLE_MODE_BILI, 6, 0);
    jlgpu_hal_destroy_context(ctx_hal);

    //联动task
    jlgpu_task_set_next(ex_task, task);
    /*jlgpu_dumcp_all_tasks(ex_task);*/

    return ex_task;
}

static uint8_t *lv_jlgpu_yuv420_draw_task_create(lv_draw_jlgpu_unit_t *u, const lv_draw_image_dsc_t *dsc, lv_jlgpu_ctx_t *jlgpu_ctx, uint8_t *img_buf, uint16_t flags, jlgpu_mask_param_t *mask_params, jlgpu_draw_img_t *draw_img_params, jlgpu_area_t *render_area, jlgpu_area_t *tex_clip_area, bool use_gpu_list)
{

    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);

    uint8_t *y_buf = (uint8_t *)img_buf;
    //创建拓展指令task
    jlgpu_extend_param_t extend_params = {0};
    jlgpu_uv_adr_t jlgpu_uv_adr = {0};

    uint32_t src_width  = draw_img_params->img_info.width;
    uint32_t src_height = draw_img_params->img_info.height;

    if (flags & LV_IMAGE_FLAGS_USER1) {
        lv_yuv_buf_t *yuv_plane = (lv_yuv_buf_t *)img_buf;
        y_buf = yuv_plane->semi_planar.y.buf;
        jlgpu_uv_adr.uv_data = yuv_plane->semi_planar.uv.buf;
    } else {
        jlgpu_uv_adr.uv_data = img_buf + src_width * src_height;
    }

    extend_params.layer_en = 1;
    extend_params.breakpoint_en = 0;
    extend_params.command = JLGPU_EXTEND_COMMAND_MODIFY_UVADR;
    extend_params.command_uv_adr = &jlgpu_uv_adr;


    int ex_task_len = jlgpu_extend_command_task_len(jlgpu_ctx->ctx, &extend_params);
    uint8_t *ex_task = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && use_gpu_list) {
#if LV_USE_DRAW_JLGPU_LIST
        ex_task = lv_jlgpu_mem_pool_get_buf(u, NULL, ex_task_len);
#endif
    } else {
        ex_task = lv_malloc(ex_task_len);
    }

    if (ex_task == NULL) {
        LV_LOG_ERROR("yuv420 ex_task malloc fail");
        return NULL;
    }

    lv_memset(ex_task, 0, ex_task_len);
    jlgpu_set_task(jlgpu_ctx->ctx, ex_task, ex_task_len);
    jlgpu_extend_command(jlgpu_ctx->ctx, &extend_params);
    /*jlgpu_dump_task(ex_task);*/

    //创建一个draw img hal上下文
    jlgpu_hal_context_t *ctx_hal = NULL;
    ctx_hal = jlgpu_hal_create_context();

    jlgpu_draw_mode_t draw_mode = JLGPU_DRAW_IMG;
    //recolor
    if (has_recolor) {
        draw_mode = JLGPU_DRAW_IMG_FILL;
    }

    jlgpu_mask_mode_t mask_mode = mask_params->mask_mode;
    jlgpu_paint_mode_t render_type = JLGPU_PAINT_MODE_MULTIPLY;
    jlgpu_trans_mode_t	tex_trans_mode = draw_img_params->tex_trans_mode;
    jlgpu_trans_mode_t	pnt_trans_mode = JLGPU_TRANS_MODE_NONE;
    jlgpu_trans_mode_t	path_trans_mode = JLGPU_TRANS_MODE_NONE;

    jlgpu_path_t *jlgpu_path = NULL;
    jlgpu_mask_t *jlgpu_mask = NULL;

    if (mask_mode == JLGPU_MASK_MODE_VECTOR) {
        jlgpu_path = (jlgpu_path_t *)(mask_params->mask_ptr);
        path_trans_mode = jlgpu_path->path_trans_mode;
    } else if (mask_mode == JLGPU_MASK_MODE_EXT) {
        jlgpu_mask = (jlgpu_mask_t *)(mask_params->mask_ptr);
    }

    gpu_hw_task_set_render_param(ctx_hal, draw_mode, mask_mode, render_type, tex_trans_mode, pnt_trans_mode, path_trans_mode);

    uint8_t task_len = gpu_hw_task_get_len_render(ctx_hal);
    uint8_t *task = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && use_gpu_list) {
#if LV_USE_DRAW_JLGPU_LIST
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
#endif
    } else {
        task = lv_malloc(task_len);
    }

    if (task == NULL) {
        LV_LOG_ERROR("img task malloc fail");
        jlgpu_hal_destroy_context(ctx_hal);
        return NULL;
    }

    memset(task, 0, task_len);
    gpu_hw_task_set_task(ctx_hal, task);
    //basic
    gpu_hw_task_set_render_common(ctx_hal);
    gpu_hw_task_set_layer_en(ctx_hal, 1);
    gpu_hw_task_set_breakpoint_en(ctx_hal, 0);
    gpu_hw_task_set_blend_mode(ctx_hal, lv_jlgpu_get_blend_mode(dsc->blend_mode, NULL));
    gpu_hw_task_set_dither_en(ctx_hal, 1);
    gpu_hw_task_set_gb_trans_en(ctx_hal, 0);
    gpu_hw_task_set_rgb_limit_en(ctx_hal, 0);
    gpu_hw_task_set_global_alpha(ctx_hal, 128);

    if (draw_img_params->tex_trans_mode == JLGPU_TRANS_MODE_AFFINE) {
        gpu_hw_task_set_layer_quality(ctx_hal, JLGPU_RENDER_QUALITY);
    } else {
        gpu_hw_task_set_layer_quality(ctx_hal, JLGPU_RENDERING_QUALITY_NONANTIALIASED);
    }

    gpu_hw_task_set_bbox(ctx_hal, render_area->x_min, render_area->x_max, render_area->y_min, render_area->y_max);
    //tex
    gpu_hw_task_set_color_ext(ctx_hal, JLGPU_COLOR_EXTEND_HIGH);
    gpu_hw_task_set_tex_premult(ctx_hal, 1);
    gpu_hw_task_set_boundary_ext(ctx_hal, JLGPU_BOUNDARY_EXTEND_ZERO);
    gpu_hw_task_set_colorkey_en(ctx_hal, 0);
    gpu_hw_task_set_tex_clip(ctx_hal, tex_clip_area->x_min, tex_clip_area->x_max, tex_clip_area->y_min, tex_clip_area->y_max);

    jlgpu_alpha_end_t alpha_end = JLGPU_ALPHA_END_ARGB;
    jlgpu_rbs_t rb_swap = JLGPU_RBS_ARGB;
    jlgpu_endian_t big_end = JLGPU_ENDIAN_LITTLE;


    uint8_t *input_data = (uint8_t *)y_buf;
    gpu_hw_task_set_tex_info(ctx_hal, src_width, src_height, draw_img_params->img_info.width, JLGPU_FORMAT_YUV420_BT601, JLGPU_ADRMODE_LINE, JLGPU_COMPRESS_MODE_NONE, rb_swap, big_end, alpha_end, 0, input_data);

    //recolor
    if (has_recolor) {
        gpu_hw_task_set_pnt_type(ctx_hal, JLGPU_PAINT_TYPE_COLOR, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_premult(ctx_hal, JLGPU_PREMULT_TRUE, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_color(ctx_hal, dsc->recolor.red, dsc->recolor.green, dsc->recolor.blue, dsc->recolor_opa, JLGPU_PAINT_INDEX_0);
        gpu_hw_task_set_pnt_clip(ctx_hal, render_area->x_min, render_area->x_max, render_area->y_min, render_area->y_max, JLGPU_PAINT_INDEX_0);
    }

    //path
    if (mask_mode == JLGPU_MASK_MODE_VECTOR) {
        gpu_hw_task_set_fill_rule(ctx_hal, JLGPU_NON_ZERO);
        gpu_hw_task_set_path_render_quality(ctx_hal, JLGPU_PATH_MSAA_LVL);
        gpu_hw_task_set_bez_div(ctx_hal, JLGPU_GLOBAL_MAX_BEZ_DIV);
        gpu_hw_task_set_path(ctx_hal, jlgpu_path->seg_len, jlgpu_path->seg_data);

        if (path_trans_mode != JLGPU_TRANS_MODE_NONE) {
            gpu_hw_task_set_path_matrix(ctx_hal, (float *)jlgpu_path->user2surface);
        }
    }

    if (draw_img_params->surface2image) {
        gpu_hw_task_set_tex_matrix(ctx_hal, (float *)(draw_img_params->surface2image));
    }
    gpu_hw_task_set_tex_sample_param(ctx_hal, JLGPU_SAMPLE_MODE_BILI, 6, 0);
    jlgpu_hal_destroy_context(ctx_hal);

    //联动task
    jlgpu_task_set_next(ex_task, task);
    /*jlgpu_dumcp_all_tasks(ex_task);*/

    return ex_task;
}

#endif /*LV_USE_DRAW_JLGPU_IMG_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
