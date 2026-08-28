/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_BORDER_ENABLE == 1)

/*********************
*      DEFINES
*********************/
#define HAS_BORDER_SIDE(dsc_side, side) (((dsc_side) & (side)) == (side))
/**********************
*      TYPEDEFS
**********************/

/**********************
*  STATIC PROTOTYPES
**********************/
static jlgpu_fillrule_t path_append_inner_rect(jlgpu_vec_t *path, const lv_draw_border_dsc_t *dsc, float x, float y, float w, float h, float r);
/**********************
*  STATIC VARIABLES
**********************/

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/
void lv_draw_jlgpu_border(lv_draw_task_t *t, const lv_draw_border_dsc_t *dsc, const lv_area_t *coords)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    if (dsc->opa <= LV_OPA_MIN) {
        return;
    }
    LV_PROFILER_BEGIN;

    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
        return;
    }

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, coords, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);
    int border_width  = lv_area_get_width(coords);
    int border_height = lv_area_get_height(coords);

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint32_t color;
    lv_draw_jlgpu_premult_and_swizzle(&color, dsc->color, dsc->opa);

    int32_t short_side = LV_MIN(width, height);
    int32_t rout = LV_MIN(dsc->radius, short_side >> 1);

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

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
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num;
    int path_div = jlgpu_compute_div(render_params.render_area);
    jlgpu_vec_t *rect_path = lv_jlgpu_path_get(u);

    lv_jlgpu_path_set_bez_div(rect_path, LV_MIN(path_div, global_max_bez_div));
    //外框
    jlgpu_path_append_rect(rect_path, (float)coords->x1, (float)coords->y1, border_width, border_height, rout);

    //内框
    jlgpu_fillrule_t fill_rule = path_append_inner_rect(rect_path, dsc, (float)coords->x1, (float)coords->y1, border_width, border_height, rout);

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

    jlgpu_path.fill_rule = fill_rule;
    jlgpu_path.max_bez_div = rect_path->bez_div;
    jlgpu_path.user2surface = 0;
    jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;
    jlgpu_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_fill(u->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
        lv_jlgpu_add_task_node(u, task);
#endif
    } else {

        lv_jlgpu_ctx_t jlgpu_ctx;

        uint8_t *task = NULL;
        uint8_t *cbuf = NULL;
        uint8_t *ebuf = NULL;

        lv_jlgpu_ctx_init(&jlgpu_ctx);

        int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        task = lv_malloc(task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            goto malloc_fail;
        }

        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
        jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);

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

        int mask_h = global_path.cbuf_split ? 16 : 32; //8 + global_path.cbuf_split * 4;
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

    LV_PROFILER_END;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static jlgpu_fillrule_t path_append_inner_rect(jlgpu_vec_t *path,
        const lv_draw_border_dsc_t *dsc,
        float x, float y, float w, float h,
        float r)
{
    LV_PROFILER_DRAW_BEGIN;

    const float half_w = w / 2.0f;
    const float half_h = h / 2.0f;
    const int32_t border_w = dsc->width;
    const float border_w_max = LV_MIN(half_w, half_h);

    /* normal fill, no inner rect */
    if (border_w >= border_w_max) {
        LV_PROFILER_DRAW_END;
        return JLGPU_EVEN_ODD;
    }

    const float r_in = r - border_w;

    /* full border, simple rect */
    if (dsc->side == LV_BORDER_SIDE_FULL) {
        jlgpu_path_append_rect(path,
                               x + border_w, y + border_w,
                               w - border_w * 2, h - border_w * 2,
                               r_in < 0 ? 0 : r_in);

        LV_PROFILER_DRAW_END;
        return JLGPU_EVEN_ODD;
    }

    /* no-radius case, simple inner rect */
    if (dsc->radius <= 0) {
        int32_t x_offset = 0;
        int32_t y_offset = 0;
        int32_t w_inner = w;
        int32_t h_inner = h;

        if (dsc->side & LV_BORDER_SIDE_TOP) {
            y_offset += border_w;
            h_inner -= border_w;
        }
        if (dsc->side & LV_BORDER_SIDE_LEFT) {
            x_offset += border_w;
            w_inner -= border_w;
        }
        if (dsc->side & LV_BORDER_SIDE_BOTTOM) {
            h_inner -= border_w;
        }
        if (dsc->side & LV_BORDER_SIDE_RIGHT) {
            w_inner -= border_w;
        }

        jlgpu_path_append_rect(path,
                               x + x_offset,
                               y + y_offset,
                               w_inner,
                               h_inner,
                               0);
        LV_PROFILER_DRAW_END;
        return JLGPU_EVEN_ODD;
    }

    /* reset outter rect path */
    jlgpu_path_reset(path);

    /* coordinate reference map: https://github.com/lvgl/lvgl/pull/6796 */
    const float c1_x = x + r;
    const float c1_y = y + r;
    const float c2_x = x + w - r;
    const float c2_y = c1_y;
    const float c3_x = c2_x;
    const float c3_y = y + h - r;
    const float c4_x = c1_x;
    const float c4_y = c3_y;

    /* When border_w > r, No need to calculate the intersection of the arc and the line */
    if (r_in <= 0) {
        const float p1_x = x;
        const float p1_y = y + border_w;
        const float p2_x = x;
        const float p2_y = y + r;
        const float p3_x = x + r;
        const float p3_y = y;
        const float p4_x = x + border_w;
        const float p4_y = y;

        const float p5_x = x + w - border_w;
        const float p5_y = y;
        const float p6_x = x + w - r;
        const float p6_y = y;
        const float p7_x = x + w;
        const float p7_y = y + r;
        const float p8_x = x + w;
        const float p8_y = y + border_w;

        const float p9_x = x + w;
        const float p9_y = y + h - border_w;
        const float p10_x = x + w;
        const float p10_y = y + h - r;
        const float p11_x = x + w - r;
        const float p11_y = y + h;
        const float p12_x = x + w - border_w;
        const float p12_y = y + h;

        const float p13_x = x + border_w;
        const float p13_y = y + h;
        const float p14_x = x + r;
        const float p14_y = y + h;
        const float p15_x = x;
        const float p15_y = y + h - r;
        const float p16_x = x;
        const float p16_y = y + h - border_w;

        if (dsc->side & LV_BORDER_SIDE_BOTTOM) {
            jlgpu_path_move_to(path, p16_x, p16_y);
            jlgpu_path_line_to(path, p9_x, p9_y);
            jlgpu_path_line_to(path, p10_x, p10_y);
            jlgpu_path_append_arc_right_angle(path, p10_x, p10_y, c3_x, c3_y, p11_x, p11_y);
            jlgpu_path_line_to(path, p14_x, p14_y);
            jlgpu_path_append_arc_right_angle(path, p14_x, p14_y, c4_x, c4_y, p15_x, p15_y);
            /*close(path);*/
            jlgpu_path_line_to(path, p16_x, p16_y);
        }

        if (dsc->side & LV_BORDER_SIDE_TOP) {
            jlgpu_path_move_to(path, p1_x, p1_y);
            jlgpu_path_line_to(path, p2_x, p2_y);
            jlgpu_path_append_arc_right_angle(path, p2_x, p2_y, c1_x, c1_y, p3_x, p3_y);
            jlgpu_path_line_to(path, p6_x, p6_y);
            jlgpu_path_append_arc_right_angle(path, p6_x, p6_y, c2_x, c2_y, p7_x, p7_y);
            jlgpu_path_line_to(path, p8_x, p8_y);

            /*close(path);*/
            jlgpu_path_line_to(path, p1_x, p1_y);
        }

        if (dsc->side & LV_BORDER_SIDE_LEFT) {
            jlgpu_path_move_to(path, p4_x, p4_y);
            jlgpu_path_line_to(path, p13_x, p13_y);
            jlgpu_path_line_to(path, p14_x, p14_y);
            jlgpu_path_append_arc_right_angle(path, p14_x, p14_y, c4_x, c4_y, p15_x, p15_y);
            jlgpu_path_line_to(path, p2_x, p2_y);
            jlgpu_path_append_arc_right_angle(path, p2_x, p2_y, c1_x, c1_y, p3_x, p3_y);

            /*close(path);*/
            jlgpu_path_line_to(path, p4_x, p4_y);
        }

        if (dsc->side & LV_BORDER_SIDE_RIGHT) {
            jlgpu_path_move_to(path, p5_x, p5_y);
            jlgpu_path_line_to(path, p6_x, p6_y);
            jlgpu_path_append_arc_right_angle(path, p6_x, p6_y, c2_x, c2_y, p7_x, p7_y);
            jlgpu_path_line_to(path, p10_x, p10_y);
            jlgpu_path_append_arc_right_angle(path, p10_x, p10_y, c3_x, c3_y, p11_x, p11_y);
            jlgpu_path_line_to(path, p12_x, p12_y);

            /*close(path);*/
            jlgpu_path_line_to(path, p5_x, p5_y);
        }

        LV_PROFILER_DRAW_END;
        return JLGPU_NON_ZERO;
    }


    /* When border_w < r, Calculate the intersection of an arc and a line */

    /* r^2 - r_in^2 = offset^2 */
    const float offset = MATH_SQRTF((2 * r - border_w) * border_w);
    const float sweep_alpha = MATH_DEGREES(MATH_ACOSF(r_in / r));
    const float sweep_beta = 90 - sweep_alpha;

    const float p1_x = x + border_w;
    const float p1_y = y + r;
    const float p2_x = x;
    const float p2_y = y + r;
    const float p3_x = x + border_w;
    const float p3_y = y + r - offset;
    const float p4_x = x + r - offset;
    const float p4_y = y + border_w;
    const float p5_x = x + r;
    const float p5_y = y;
    const float p6_x = x + r;
    const float p6_y = y + border_w;

    const float p7_x = x + w - r;
    const float p7_y = y + border_w;
    const float p8_x = x + w - r;
    const float p8_y = y;
    const float p10_x = x + w - border_w;
    const float p10_y = y + r - offset;
    const float p11_x = x + w;
    const float p11_y = y + r;
    const float p12_x = x + w - border_w;
    const float p12_y = y + r;

    const float p13_x = x + w - border_w;
    const float p13_y = y + h - r;
    const float p14_x = x + w;
    const float p14_y = y + h - r;
    const float p16_x = x + w - r + offset;
    const float p16_y = y + h - border_w;
    const float p17_x = x + w - r;
    const float p17_y = y + h;
    const float p18_x = x + w - r;
    const float p18_y = y + h - border_w;

    const float p19_x = x + r;
    const float p19_y = y + h - border_w;
    const float p20_x = x + r;
    const float p20_y = y + h;
    const float p21_x = x + r - offset;
    const float p21_y = y + h - border_w;

    const float p22_x = x + border_w;
    const float p22_y = y + h - r + offset;
    const float p23_x = x;
    const float p23_y = y + h - r;
    const float p24_x = x + border_w;
    const float p24_y = y + h - r;

    if (dsc->side & LV_BORDER_SIDE_BOTTOM) {
        jlgpu_path_move_to(path, p21_x, p21_y);
        jlgpu_path_line_to(path, p16_x, p16_y);
        jlgpu_path_append_arc(path, c3_x, c3_y, r, sweep_beta, sweep_alpha, false);
        jlgpu_path_line_to(path, p20_x, p20_y);
        jlgpu_path_append_arc(path, c4_x, c4_y, r, 90, sweep_alpha, false);
        /*close(path);*/
        jlgpu_path_line_to(path, p21_x, p21_y);
    }

    if (dsc->side & LV_BORDER_SIDE_TOP) {
        jlgpu_path_move_to(path, p4_x, p4_y);
        jlgpu_path_append_arc(path, c1_x, c1_y, r, 270 - sweep_alpha, sweep_alpha, false);
        jlgpu_path_line_to(path, p8_x, p8_y);
        jlgpu_path_append_arc(path, c2_x, c2_y, r, 270, sweep_alpha, false);
        /*close(path);*/
        jlgpu_path_line_to(path, p4_x, p4_y);
    }

    if (dsc->side & LV_BORDER_SIDE_LEFT) {
        jlgpu_path_move_to(path, p3_x, p3_y);
        jlgpu_path_line_to(path, p22_x, p22_y);
        jlgpu_path_append_arc(path, c4_x, c4_y, r, 90 + sweep_beta, sweep_alpha, false);
        jlgpu_path_line_to(path, p2_x, p2_y);
        jlgpu_path_append_arc(path, c1_x, c1_y, r, 180, sweep_alpha, false);
        /*close(path);*/
        jlgpu_path_line_to(path, p3_x, p3_y);
    }

    if (dsc->side & LV_BORDER_SIDE_RIGHT) {
        jlgpu_path_move_to(path, p10_x, p10_y);
        jlgpu_path_append_arc(path, c2_x, c2_y, r, 270 + sweep_beta, sweep_alpha, false);
        jlgpu_path_line_to(path, p14_x, p14_y);
        jlgpu_path_append_arc(path, c3_x, c3_y, r, 0, sweep_alpha, false);
        /*close(path);*/
        jlgpu_path_line_to(path, p10_x, p10_y);
    }

    /* Draw the rounded corners adjacent to the border */

    if (HAS_BORDER_SIDE(dsc->side, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT)) {
        jlgpu_path_move_to(path, p2_x, p2_y);
        jlgpu_path_append_arc_right_angle(path, p2_x, p2_y, c1_x, c1_y, p5_x, p5_y);
        jlgpu_path_line_to(path, p6_x, p6_y);
        jlgpu_path_append_arc_right_angle(path, p6_x, p6_y, c1_x, c1_y, p1_x, p1_y);
        /*close(path);*/
        jlgpu_path_line_to(path, p2_x, p2_y);
    }

    if (HAS_BORDER_SIDE(dsc->side, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT)) {
        jlgpu_path_move_to(path, p8_x, p8_y);
        jlgpu_path_append_arc_right_angle(path, p8_x, p8_y, c2_x, c2_y, p11_x, p11_y);
        jlgpu_path_line_to(path, p12_x, p12_y);
        jlgpu_path_append_arc_right_angle(path, p12_x, p12_y, c2_x, c2_y, p7_x, p7_y);
        /*close(path);*/
        jlgpu_path_line_to(path, p8_x, p8_y);
    }

    if (HAS_BORDER_SIDE(dsc->side, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT)) {
        jlgpu_path_move_to(path, p20_x, p20_y);
        jlgpu_path_append_arc_right_angle(path, p20_x, p20_y, c4_x, c4_y, p23_x, p23_y);
        jlgpu_path_line_to(path, p24_x, p24_y);
        jlgpu_path_append_arc_right_angle(path, p24_x, p24_y, c4_x, c4_y, p19_x, p19_y);
        /*close(path);*/
        jlgpu_path_line_to(path, p20_x, p20_y);
    }

    if (HAS_BORDER_SIDE(dsc->side, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT)) {
        jlgpu_path_move_to(path, p14_x, p14_y);
        jlgpu_path_append_arc_right_angle(path, p14_x, p14_y, c3_x, c3_y, p17_x, p17_y);
        jlgpu_path_line_to(path, p18_x, p18_y);
        jlgpu_path_append_arc_right_angle(path, p18_x, p18_y, c3_x, c3_y, p13_x, p13_y);
        /*close(path);*/
        jlgpu_path_line_to(path, p14_x, p14_y);
    }

    LV_PROFILER_DRAW_END;
    return JLGPU_NON_ZERO;
}


#endif /*LV_USE_DRAW_JLGPU_BORDER_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
