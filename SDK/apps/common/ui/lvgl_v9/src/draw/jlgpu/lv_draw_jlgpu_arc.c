/**
 * @file lv_draw_jlgpu_arc.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_ARC_ENABLE == 1)

/*********************
 *      DEFINES
 *********************/

#define SQ(x) ((x) * (x))

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

static float math_fast_inv_sqrtf(float number)
{
    /* From https://en.wikipedia.org/wiki/Fast_inverse_square_root#Avoiding_undefined_behavior */
    union {
        float   f;
        int32_t i;
    } conv = { .f = number };
    conv.i  = 0x5f3759df - (conv.i >> 1);
    conv.f *= 1.5F - (number * 0.5F * conv.f * conv.f);
    return conv.f;
}


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_jlgpu_arc(lv_draw_task_t *t, const lv_draw_arc_dsc_t *dsc, const lv_area_t *coords)
{

    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_area_t clip_area;
    if (!lv_area_intersect(&clip_area, coords, &t->clip_area)) {
        /*Fully clipped, nothing to do*/
        return;
    }

    float start_angle = dsc->start_angle;
    float end_angle = dsc->end_angle;
    float sweep_angle = end_angle - start_angle;

    while (sweep_angle < 0) {
        sweep_angle += 360;
    }

    while (sweep_angle > 360) {
        sweep_angle -= 360;
    }

    /*If the angles are the same then there is nothing to draw*/
    if (math_zero(sweep_angle)) {
        return;
    }

    LV_PROFILER_DRAW_BEGIN;

    lv_area_t blend_area = clip_area;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    //绘制输出buf信息
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);
    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint32_t color;
    lv_draw_jlgpu_premult_and_swizzle(&color, dsc->color, dsc->opa);

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};
    jlgpu_draw_img_t draw_img_params = {0};

    draw_fill_params.color.b = color & 0xFF;
    draw_fill_params.color.g = (color >> 8) & 0xFF;
    draw_fill_params.color.r = (color >> 16) & 0xFF;
    draw_fill_params.color.a = (color >> 24) & 0xFF;

    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;

    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    draw_fill_params.paint_info.pt_clip.x_min = 0;
    draw_fill_params.paint_info.pt_clip.x_max = width;
    draw_fill_params.paint_info.pt_clip.y_min = 0;
    draw_fill_params.paint_info.pt_clip.y_max = height;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area.x_min = blend_area.x1;
    render_params.render_area.x_max = blend_area.x2 + 1;
    render_params.render_area.y_min = blend_area.y1;
    render_params.render_area.y_max = blend_area.y2 + 1;

    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_PATH_MSAA_LVL;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    int edge_num;
    int path_div = jlgpu_compute_div(render_params.render_area);

    jlgpu_vec_t *path = lv_jlgpu_path_get(u);

    lv_jlgpu_path_set_bez_div(path, LV_MIN(path_div, global_max_bez_div));

    float radius_out = dsc->radius;
    float radius_in = dsc->radius - dsc->width;
    float cx = dsc->center.x;
    float cy = dsc->center.y;

    jlgpu_fillrule_t fill_rule =  JLGPU_NON_ZERO;

    if (math_equal(sweep_angle, 360)) {
        jlgpu_path_append_circle(path, cx, cy, radius_out, radius_out);

        /* radius_in <= 0, normal fill circle */
        if (radius_in > 0) {
            jlgpu_path_append_circle(path, cx, cy, radius_in, radius_in);
        }
        fill_rule = JLGPU_EVEN_ODD;
    } else {
        float start_angle_rad = MATH_RADIANS(start_angle);
        float end_angle_rad = MATH_RADIANS(end_angle);

        if (radius_in > 0) {
            /* radius_out start point */
            float start_x = radius_out * MATH_COSF(start_angle_rad) + cx;
            float start_y = radius_out * MATH_SINF(start_angle_rad) + cy;

            /* radius_in start point */
            float end_x = radius_in * MATH_COSF(end_angle_rad) + cx;
            float end_y = radius_in * MATH_SINF(end_angle_rad) + cy;

            jlgpu_path_move_to(path, start_x, start_y);

            /* radius_out arc */
            jlgpu_path_append_arc(path,
                                  cx, cy,
                                  radius_out,
                                  start_angle,
                                  sweep_angle,
                                  false);

            /* line to radius_in */
            jlgpu_path_line_to(path, end_x, end_y);

            /* radius_in arc */
            jlgpu_path_append_arc(path,
                                  cx, cy,
                                  radius_in,
                                  end_angle,
                                  -sweep_angle,
                                  false);

            jlgpu_path_line_to(path, start_x, start_y);
        } else {
            /* draw a normal arc pie shape */
            jlgpu_path_append_arc(path, cx, cy, radius_out, start_angle, sweep_angle, true);
        }

        /* draw round */
        if (dsc->rounded && dsc->width > 0) {
            float round_radius = radius_out > dsc->width ? dsc->width / 2.0f : radius_out / 2.0f;
            float round_center = radius_out - round_radius;
            float rcx1 = cx + round_center * MATH_COSF(end_angle_rad);
            float rcy1 = cy + round_center * MATH_SINF(end_angle_rad);
            jlgpu_path_append_circle(path, rcx1, rcy1, round_radius, round_radius);

            float rcx2 = cx + round_center * MATH_COSF(start_angle_rad);
            float rcy2 = cy + round_center * MATH_SINF(start_angle_rad);
            jlgpu_path_append_circle(path, rcx2, rcy2, round_radius, round_radius);
        }
    }

    jlgpu_path_end(path);

    edge_num = path->edge_num;
    jlgpu_path.seg_len = path->total_len;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        jlgpu_path.seg_data = lv_jlgpu_attach_buf_list_add(u, path->base, path->total_len);
#endif
    } else {
        jlgpu_path.seg_data = path->base;
    }

    jlgpu_path.fill_rule = fill_rule;
    jlgpu_path.max_bez_div = path->bez_div;
    jlgpu_path.user2surface = 0;
    jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;
    jlgpu_path.msaa_lvl = JLGPU_RENDER_QUALITY;

    lv_result_t img_res = LV_RESULT_INVALID;
    jlgpu_img_info_t img_info = {0};
    lv_image_decoder_dsc_t decoder_dsc;

    if (dsc->img_src) {
        img_res = lv_image_decoder_open(&decoder_dsc, dsc->img_src, NULL);
        if (img_res != LV_RESULT_OK) {
            LV_LOG_ERROR("Failed to open image");
            LV_PROFILER_END;
        }

        const lv_draw_buf_t *decoded = decoder_dsc.decoded;
        uint32_t src_width = decoded->header.w;
        uint32_t src_height = decoded->header.h;
        uint8_t in_format = lv_jlgpu_get_color_format(decoded->header.cf);
        uint8_t *img_buf = decoded->data;

        jlgpu_area_t tex_clip_area;
        tex_clip_area.x_min = 0;
        tex_clip_area.x_max = src_width;
        tex_clip_area.y_min = 0;
        tex_clip_area.y_max = src_height;

        img_info.premult = 1;
        img_info.tex_clip = tex_clip_area;
        uint8_t adr_mode = JLGPU_ADRMODE_LINE;


        img_info.adr_mode = adr_mode;
        img_info.color_format = in_format;
        img_info.data = img_buf;
        img_info.height = src_height;
        img_info.width = src_width;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
        img_info.compress_size = 0;

        draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_NONE;
        draw_img_params.img_info = img_info;

    }

    //链表
    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        int task_len;
        uint8_t *task = NULL;
        if (img_res == LV_RESULT_OK) {
            task_len = jlgpu_draw_img_task_len(u->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
            task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
            if (task == NULL) {
                LV_LOG_ERROR("malloc fail");
                return;
            }
            lv_memset(task, 0, task_len);
            jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
            jlgpu_draw_img(u->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
        } else {
            task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
            task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
            if (task == NULL) {
                LV_LOG_ERROR("malloc fail");
                return;
            }
            lv_memset(task, 0, task_len);
            jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
            jlgpu_draw_fill(u->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
        }

        lv_jlgpu_add_task_node(u, task);
#endif
    } else {//layer/canvas需要立即绘制

        int task_len;
        lv_jlgpu_ctx_t jlgpu_ctx;
        lv_jlgpu_ctx_init(&jlgpu_ctx);

        if (img_res == LV_RESULT_OK) {
            task_len = jlgpu_draw_img_task_len(jlgpu_ctx.ctx, &draw_img_params, &mask_params);
        } else {
            task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        }

        uint8_t *task = NULL;
        uint8_t *cbuf = NULL;
        uint8_t *ebuf = NULL;

        task = lv_malloc(task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            goto malloc_fail;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);

        if (img_res == LV_RESULT_OK) {
            jlgpu_draw_img(jlgpu_ctx.ctx, &render_params, &draw_img_params, &mask_params);
        } else {
            jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);
        }

        /*jlgpu_dump_task(task);*/

        jlgpu_surface_t surface = {0};
        jlgpu_global_path_t global_path = {0};

        surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);

        surface.window.x_min = layer->buf_area.x1;
        surface.window.x_max = layer->buf_area.x2 + 1;
        surface.window.y_min = layer->buf_area.y1;
        surface.window.y_max = layer->buf_area.y2 + 1;

        surface.format = out_format;
        surface.width = fb_width;

        global_path.cbuf_split = 0;
        global_path.cbuf_stride = fb_width;
        global_path.max_bez_div = global_max_bez_div;
        global_path.max_edge_cfg = edge_num;

        int mask_h = global_path.cbuf_split ? 16 : 32;
        cbuf = (uint8_t *)lv_malloc(mask_h * fb_width);
        if (cbuf == NULL) {
            LV_LOG_ERROR("malloc fail");
            goto malloc_fail;
        }
        lv_memset(cbuf, 0, mask_h * fb_width);
        ebuf = (uint8_t *)lv_malloc(8 * edge_num);
        if (ebuf == NULL) {
            LV_LOG_ERROR("malloc fail");
            goto malloc_fail;
        }
        lv_memset(ebuf, 0, 8 * edge_num);

        global_path.c_data = cbuf;
        global_path.e_data = ebuf;

        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);

        jlgpu_run(jlgpu_ctx.ctx, task);

malloc_fail:

        if (task) {
            lv_free(task);
        }
        if (cbuf) {
            lv_free(cbuf);
        }
        if (ebuf) {
            lv_free(ebuf);
        }

        lv_jlgpu_ctx_uninit(&jlgpu_ctx);
    }

    if (img_res == LV_RESULT_OK) {
        lv_image_decoder_close(&decoder_dsc);
    }

    LV_PROFILER_DRAW_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_JLGPU*/
#endif /*LV_USE_DRAW_JLGPU_ARC_ENABLE*/

#endif /* #if CONFIG_LVGL_UI_ENABLE */
