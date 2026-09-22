/**
 * @file lv_draw_jlgpu_triangle.c
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
#if (LV_USE_DRAW_JLGPU_TRIANGLE_ENABLE == 1)
/*********************
 *      DEFINES
 *********************/

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

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_jlgpu_triangle(lv_draw_task_t *t, const lv_draw_triangle_dsc_t *dsc)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    lv_area_t tri_area;
    tri_area.x1 = (int32_t)LV_MIN3(dsc->p[0].x, dsc->p[1].x, dsc->p[2].x);
    tri_area.y1 = (int32_t)LV_MIN3(dsc->p[0].y, dsc->p[1].y, dsc->p[2].y);
    tri_area.x2 = (int32_t)LV_MAX3(dsc->p[0].x, dsc->p[1].x, dsc->p[2].x);
    tri_area.y2 = (int32_t)LV_MAX3(dsc->p[0].y, dsc->p[1].y, dsc->p[2].y);

    bool is_common;
    lv_area_t clip_area;
    is_common = lv_area_intersect(&clip_area, &tri_area, &t->clip_area);
    if (!is_common) {
        return;
    }

    LV_PROFILER_DRAW_BEGIN;

    lv_area_t blend_area = clip_area;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

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
    jlgpu_draw_grad_t draw_grad = {0};

    draw_fill_params.color.b = dsc->color.blue;
    draw_fill_params.color.g = dsc->color.green;
    draw_fill_params.color.r = dsc->color.red;
    draw_fill_params.color.a = dsc->opa;

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
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num;
    int path_div = jlgpu_compute_div(render_params.render_area);

    jlgpu_vec_t *path = lv_jlgpu_path_get(u);
    lv_jlgpu_path_set_bez_div(path, LV_MIN(path_div, global_max_bez_div));

    jlgpu_path_move_to(path, dsc->p[0].x, dsc->p[0].y);
    jlgpu_path_line_to(path, dsc->p[1].x, dsc->p[1].y);
    jlgpu_path_line_to(path, dsc->p[2].x, dsc->p[2].y);
    jlgpu_path_line_to(path, dsc->p[0].x, dsc->p[0].y);
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

    jlgpu_path.fill_rule = JLGPU_EVEN_ODD;
    jlgpu_path.max_bez_div = path->bez_div;
    jlgpu_path.user2surface = 0;
    jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;

    jlgpu_matrix_t matrix;
    jlgpu_paint_inter_params_t grad_inter_params;
    jlgpu_lineargrad_param_t lineargrad_param;
    uint32_t stops_color = 0;
    uint32_t main_color = 0;
    uint32_t clut_data_2[2] = {main_color, stops_color};
    uint32_t *clut_data = clut_data_2;

    int task_len;
    lv_jlgpu_ctx_t jlgpu_ctx = {0};
    /* 链表模式(is_disp_buf)只构建 CPU 任务节点,下面链表分支里完全不使用本局部 ctx,
     * 也没有配对的 lv_jlgpu_ctx_uninit()。若在此处无条件 init:
     *   1) 会一直占住推屏独占门到函数返回 → s_jlgpu_gpu_submit 只增不减,
     *      推屏侧每帧都等满超时并打印 "!!! jlgpu_push_gate_enter timeout";
     *   2) 每次还白 malloc 一块 ctx_buf(uninit 不释放它),并空拆一次 TOP 联动。
     * 与 fill.c/arc.c/border.c/letter.c 一致:只在"立即绘制"路径创建。 */
    if (!(LV_USE_DRAW_JLGPU_LIST && is_disp_buf)) {
        lv_jlgpu_ctx_init(&jlgpu_ctx);
    }

    if (dsc->grad.dir != LV_GRAD_DIR_NONE) {
        lv_draw_jlgpu_premult_and_swizzle(&main_color, dsc->grad.stops[0].color, dsc->grad.stops[0].opa);

        lv_draw_jlgpu_premult_and_swizzle(&stops_color, dsc->grad.stops[dsc->grad.stops_count - 1].color, dsc->grad.stops[dsc->grad.stops_count - 1].opa);

        clut_data_2[0] = main_color;
        clut_data_2[1] = stops_color;

        clut_data = clut_data_2;

        if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
            clut_data = (uint32_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data_2, sizeof(clut_data_2));
#endif
        } else {
            clut_data = clut_data_2;
        }

        uint8_t lut_lvl = 0;

        draw_grad.clut_level = lut_lvl;
        jlgpu_matrix_set_identity(&matrix);

        lv_grad_dir_t grad_dir = dsc->grad.dir;

        if (grad_dir == LV_GRAD_DIR_HOR) {
            lineargrad_param.point_start.x = blend_area.x1;
            lineargrad_param.point_start.y = blend_area.y1;
            lineargrad_param.point_end.x = blend_area.x2 + 1;
            lineargrad_param.point_end.y = blend_area.y1;
        } else {
            lineargrad_param.point_start.x = blend_area.x1;
            lineargrad_param.point_start.y = blend_area.y1;
            lineargrad_param.point_end.x = blend_area.x1;
            lineargrad_param.point_end.y = blend_area.y2 + 1;
        }

        lineargrad_param.spread_mode = JLGPU_COLOR_RAMP_SPREAD_REPEAT;
        jlgpu_computeLinearParameters(draw_grad.clut_level, &lineargrad_param, &matrix);
        draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

        draw_grad.paint_info.premult = 1;
        draw_grad.paint_info.pt_clip.x_min = -4096;
        draw_grad.paint_info.pt_clip.x_max = 4095;
        draw_grad.paint_info.pt_clip.y_min = -4096;
        draw_grad.paint_info.pt_clip.y_max = 4095;
        draw_grad.lineargrad_ptr = &lineargrad_param;
        draw_grad.grad_mode = JLGPU_GRAD_MODE_LINEAR_GRADIENT;
        draw_grad.surface2paint = &matrix;
        draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;
        draw_grad.clut_adr = (uint8_t *)clut_data;

        /*jlgpu_paint_inter_params_t grad_inter_params;*/
        grad_inter_params.coeff0 = 0;
        grad_inter_params.coeff1 = 0;
        grad_inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;
        draw_grad.grad_inter_params = &grad_inter_params;

        if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
            task_len = jlgpu_draw_grad_task_len(u->jlgpu_ctx->ctx, &draw_grad, &mask_params);
#endif
        } else {
            task_len = jlgpu_draw_grad_task_len(jlgpu_ctx.ctx, &draw_grad, &mask_params);
        }

    } else {
        if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
            task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
#endif
        } else {
            task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        }
    }

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表

#if LV_USE_DRAW_JLGPU_LIST
        if (dsc->grad.dir != LV_GRAD_DIR_NONE) {
            uint8_t *task = NULL;
            task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
            if (task == NULL) {
                LV_LOG_ERROR("malloc fail");
                return;
            }
            lv_memset(task, 0, task_len);

            jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
            jlgpu_draw_grad(u->jlgpu_ctx->ctx, &render_params, &draw_grad, &mask_params);
            lv_jlgpu_add_task_node(u, task);

        } else {
            uint8_t *task = NULL;
            task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
            if (task == NULL) {
                LV_LOG_ERROR("malloc fail");
                return;
            }
            lv_memset(task, 0, task_len);

            jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
            jlgpu_draw_grad(u->jlgpu_ctx->ctx, &render_params, &draw_grad, &mask_params);
            lv_jlgpu_add_task_node(u, task);
        }
#endif

    } else {//单任务

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

        if (dsc->grad.dir != LV_GRAD_DIR_NONE) {
            jlgpu_draw_grad(jlgpu_ctx.ctx, &render_params, &draw_grad, &mask_params);
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

    LV_PROFILER_DRAW_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
