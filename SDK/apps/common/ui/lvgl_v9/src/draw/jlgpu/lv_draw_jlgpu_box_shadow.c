/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_BOX_SHADOW_ENABLE == 1)

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* 方角/小圆角对象的阴影：用 round gradient（径向渐变）绘制。
 * 逐环 border 在方角对象上，圆角半径每环增量 <1px 只能取整数，会在四个角产生
 * 台阶（呈锐角三角形）。round gradient 是连续的 signed distance field，角部从
 * 直角平滑过渡到圆角，没有台阶。 */
static void lv_draw_jlgpu_box_shadow_roundgrad(lv_draw_task_t *t, const lv_draw_box_shadow_dsc_t *dsc,
        const lv_area_t *coords, const lv_area_t *center_area)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, coords, &rel_clip_area)) {
        return;
    }

    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int fill_width  = lv_area_get_width(coords);   /* shadow_area 宽 */
    int fill_height = lv_area_get_height(coords);

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);
    uint32_t fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;

    /* 颜色按 opa 预乘进 clut，opacity 完全由 clut 的 alpha 控制，global_alpha 固定 128 */
    uint32_t shadow_color;
    lv_draw_jlgpu_premult_and_swizzle(&shadow_color, dsc->color, dsc->opa);
    uint32_t stops_color = 0;
    uint32_t clut_data_2[2] = {shadow_color, stops_color};
    uint32_t *clut_data = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        clut_data = (uint32_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data_2, sizeof(clut_data_2));
#endif
    } else {
        clut_data = clut_data_2;
    }

    jlgpu_draw_grad_t draw_grad = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};

    uint8_t lut_lvl = 0;
    draw_grad.clut_level = lut_lvl;
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    /* round gradient 的内层矩形尺寸必须满足 in_width = 外层宽 - 2*out_r 的关系，
     * 由 in_width(中心宽) 反推 out_r，使渐变距离 = 中心到外层的真实距离(≈width)。 */
    jlgpu_roundgrad_param_t roundgrad_param;
    roundgrad_param.point_center.x = coords->x1 + fill_width / 2;
    roundgrad_param.point_center.y = coords->y1 + fill_height / 2;
    roundgrad_param.in_width   = lv_area_get_width(center_area);
    roundgrad_param.in_height  = lv_area_get_height(center_area);
    roundgrad_param.in_radius  = 0;   /* 方角/小圆角：中心圆角已收为 0 */
    roundgrad_param.out_radius = (fill_width - roundgrad_param.in_width) / 2;

    jlgpu_computeRoundParameters(draw_grad.clut_level, &roundgrad_param, &matrix);
    draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

    draw_grad.paint_info.premult = 1;
    draw_grad.paint_info.pt_clip.x_min = -4096;
    draw_grad.paint_info.pt_clip.x_max = 4095;
    draw_grad.paint_info.pt_clip.y_min = -4096;
    draw_grad.paint_info.pt_clip.y_max = 4095;
    draw_grad.roundgrad_ptr = &roundgrad_param;
    draw_grad.grad_mode = JLGPU_GRAD_MODE_ROUND_GRADIENT;
    draw_grad.surface2paint = &matrix;
    draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;
    draw_grad.clut_adr = (uint8_t *)clut_data;

    jlgpu_paint_inter_params_t grad_inter_params;
    grad_inter_params.coeff0 = 0;
    grad_inter_params.coeff1 = 0;
    grad_inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;
    draw_grad.grad_inter_params = &grad_inter_params;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area.x_min = blend_area.x1;
    render_params.render_area.x_max = blend_area.x2 + 1;
    render_params.render_area.y_min = blend_area.y1;
    render_params.render_area.y_max = blend_area.y2 + 1;

    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    render_params.global_alpha = 128;   /* 固定 1.0，opacity 由 clut 预乘 alpha 控制 */
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    int edge_num = 32;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_grad_task_len(u->jlgpu_ctx->ctx, &draw_grad, &mask_params);
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_grad(u->jlgpu_ctx->ctx, &render_params, &draw_grad, &mask_params);
        lv_jlgpu_add_task_node(u, task);
#endif
    } else {
        lv_jlgpu_ctx_t jlgpu_ctx;
        lv_jlgpu_ctx_init(&jlgpu_ctx);

        int task_len = jlgpu_draw_grad_task_len(jlgpu_ctx.ctx, &draw_grad, &mask_params);
        uint8_t *task = lv_malloc(task_len);
        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
        jlgpu_draw_grad(jlgpu_ctx.ctx, &render_params, &draw_grad, &mask_params);

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
        uint8_t *cbuf = (uint8_t *)lv_malloc(mask_h * fb_width);
        lv_memset(cbuf, 0, mask_h * fb_width);
        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num);
        lv_memset(ebuf, 0, 8 * edge_num);

        global_path.c_data = cbuf;
        global_path.e_data = ebuf;

        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);
        jlgpu_run(jlgpu_ctx.ctx, task);

        lv_free(task);
        lv_free(cbuf);
        lv_free(ebuf);
        lv_jlgpu_ctx_uninit(&jlgpu_ctx);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_jlgpu_box_shadow(lv_draw_task_t *t, const lv_draw_box_shadow_dsc_t *dsc, const lv_area_t *coords)
{

    /*Calculate the rectangle which is blurred to get the shadow in `shadow_area`*/
    lv_area_t core_area;
    core_area.x1 = coords->x1  + dsc->ofs_x - dsc->spread;
    core_area.x2 = coords->x2  + dsc->ofs_x + dsc->spread;
    core_area.y1 = coords->y1  + dsc->ofs_y - dsc->spread;
    core_area.y2 = coords->y2  + dsc->ofs_y + dsc->spread;

    /*Calculate the bounding box of the shadow*/
    lv_area_t shadow_area;
    shadow_area.x1 = core_area.x1 - dsc->width / 2 - 1;
    shadow_area.x2 = core_area.x2 + dsc->width / 2 + 1;
    shadow_area.y1 = core_area.y1 - dsc->width / 2 - 1;
    shadow_area.y2 = core_area.y2 + dsc->width / 2 + 1;

    /*Get clipped draw area which is the real draw area.
     *It is always the same or inside `shadow_area`*/
    lv_area_t draw_area;
    if (!lv_area_intersect(&draw_area, &shadow_area, &t->clip_area)) {
        return;
    }
    LV_UNUSED(draw_area);

    LV_PROFILER_DRAW_BEGIN;

    /* 软件端高斯模糊几何（渐变大小/起点正确）：
     * - 不透明中心 = core_area 向内收 width/2，圆角 = max(r - width/2, 0)
     * - 从中心向外 width 像素对称羽化，外缘圆角 = r + width/2
     * 分两种方式画，避免逐环 border 在方角对象上产生锐角三角形：
     *   - 方角/小圆角(r < width/2)：round gradient，角部连续平滑；
     *   - 大圆角(r >= width/2)：逐环 border，半径每环正好 +1，无台阶。 */
    int32_t half_w = dsc->width / 2;
    int32_t total_w = half_w * 2;
    int32_t r_start = LV_MAX(dsc->radius - half_w, 0);
    int32_t r_end = dsc->radius + half_w;

    /* 不透明中心 = core_area 向内收 half_w */
    lv_area_t center_area = core_area;
    lv_area_increase(&center_area, -half_w, -half_w);

    if (dsc->radius < half_w) {
        /* 方角/小圆角：round gradient */
        lv_draw_jlgpu_box_shadow_roundgrad(t, dsc, &shadow_area, &center_area);
    } else {
        /* 大圆角：不透明中心 + 逐环 border 羽化 */

        if (center_area.x1 <= center_area.x2 && center_area.y1 <= center_area.y2) {
            lv_draw_border_dsc_t core_dsc;
            lv_draw_border_dsc_init(&core_dsc);
            core_dsc.radius = r_start;
            core_dsc.opa = dsc->opa;
            core_dsc.color = dsc->color;
            /* width >= 短边一半，走 border 整块填充分支（SRC_OVER + 预乘 + 抗锯齿） */
            core_dsc.width = LV_MAX(lv_area_get_width(&center_area), lv_area_get_height(&center_area));

#if LV_USE_DRAW_JLGPU_BORDER_ENABLE
            lv_draw_jlgpu_border(t, &core_dsc, &center_area);
#endif
        }

        lv_draw_border_dsc_t border_dsc;
        lv_draw_border_dsc_init(&border_dsc);
        border_dsc.width = 1;
        border_dsc.color = dsc->color;

        lv_area_t ring_area = center_area;
        for (int32_t i = 0; i < total_w; i++) {
            lv_area_increase(&ring_area, 1, 1);
            /* r >= width/2 时 r_end - r_start == total_w，半径每环正好 +1 */
            border_dsc.radius = r_start + ((i + 1) * (r_end - r_start) + total_w / 2) / total_w;
            border_dsc.opa = lv_map(i, 0, total_w, dsc->opa, LV_OPA_0);

#if LV_USE_DRAW_JLGPU_BORDER_ENABLE
            lv_draw_jlgpu_border(t, &border_dsc, &ring_area);
#endif
        }
    }

    LV_PROFILER_DRAW_END;
}




#endif /*LV_USE_DRAW_JLGPU_BOX_SHADOW_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
