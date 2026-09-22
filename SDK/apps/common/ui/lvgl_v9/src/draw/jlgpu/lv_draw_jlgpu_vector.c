/**
 * @file lv_draw_jlgpu_vector.c
 *
 * JLGPU vector drawing. Phase 1: SOLID fill only.
 * Converts `lv_vector_path_t` (as built by the SVG parser) into the JLGPU
 * `jlgpu_vec_t` segment stream and submits it to the hardware.
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"
#include "../../misc/lv_ll.h"
#include "../lv_draw_vector_private.h"

#include "app_config.h"

extern uint32_t get_system_ms(void);

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_VECTOR_ENABLE == 1)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_task_t *t;
    lv_draw_jlgpu_unit_t *u;
    const lv_area_t *coords;
} jlgpu_vector_draw_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void task_draw_cb(void *user_data, const lv_vector_path_t *path, const lv_vector_path_ctx_t *ctx);
static void lv_path_to_jlgpu(jlgpu_vec_t *dest, const lv_vector_path_t *src, const lv_matrix_t *matrix);
static uint32_t lv_color32_to_argb(lv_color32_t color, lv_opa_t opa);
static jlgpu_fillrule_t lv_fill_to_jlgpu(lv_vector_fill_t rule);
static void jlgpu_vector_fill_submit(lv_draw_jlgpu_unit_t *u, lv_draw_task_t *t,
                                     const lv_area_t *blend_area,
                                     jlgpu_vec_t *path,
                                     uint32_t argb_color,
                                     jlgpu_fillrule_t fill_rule);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_jlgpu_vector(lv_draw_task_t *t, const lv_draw_vector_dsc_t *dsc, const lv_area_t *coords)
{
    uint32_t t0 = get_system_ms();
    // printf("[JLGPU-VEC] enter: task_list=%p\n", (void *)dsc->task_list);
    if (dsc->task_list == NULL) {
        // printf("[JLGPU-VEC] task_list NULL, bail\n");
        return;
    }

    lv_layer_t *layer = t->target_layer;
    if (layer->draw_buf == NULL) {
        // printf("[JLGPU-VEC] draw_buf NULL, bail\n");
        return;
    }
    // printf("[JLGPU-VEC] draw_buf=%p data=%p buf_area=(%d,%d,%d,%d)\n",
    //             (void *)layer->draw_buf, (void *)layer->draw_buf->data,
    //             layer->buf_area.x1, layer->buf_area.y1, layer->buf_area.x2, layer->buf_area.y2);

    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    LV_PROFILER_DRAW_BEGIN;

    jlgpu_vector_draw_ctx_t user_ctx;
    user_ctx.t = t;
    user_ctx.u = u;
    user_ctx.coords = coords;

    /* Iterate and destroy the accumulated vector subtasks */
    lv_vector_for_each_destroy_tasks(dsc->task_list, task_draw_cb, &user_ctx);

    LV_PROFILER_DRAW_END;
    uint32_t t1 = get_system_ms();
    printf("lv_draw_jlgpu_vector时间:%d", t1 - t0);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void task_draw_cb(void *user_data, const lv_vector_path_t *path, const lv_vector_path_ctx_t *ctx)
{
    jlgpu_vector_draw_ctx_t *uc = (jlgpu_vector_draw_ctx_t *)user_data;
    lv_draw_task_t *t = uc->t;
    lv_draw_jlgpu_unit_t *u = uc->u;

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    if (draw_buf == NULL) {
        return;
    }

    /* Intersect task area with clip area and subtask scissor area */
    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, uc->coords, &t->clip_area)) {
        // printf("[JLGPU-VEC] cb: coords-clip no intersect, bail\n");
        return;
    }

    /* `ctx->scissor_area` is already in absolute screen coordinates (set from
     * `clip_area` in `svg_draw` / clipped against `layer->_clip_area` in
     * `lv_draw_vector_dsc_add_path`). The path points are transformed into
     * absolute coords separately by `lv_path_to_jlgpu(..., &ctx->matrix)`, so
     * do NOT transform the scissor by the matrix again — that would double-
     * count the object position (e.g. a widget at x=267 would get its clip
     * shifted to x=534 and silently cull every subtask). */
    const lv_area_t scissor_area = ctx->scissor_area;

    if (!lv_area_intersect(&blend_area, &blend_area, &scissor_area)) {
        // printf("[JLGPU-VEC] cb: scissor no intersect, bail\n");
        return;
    }

    // printf("[JLGPU-VEC] cb: path=%p fill.opa=%d fill.style=%d stroke.opa=%d blend=(%d,%d,%d,%d)\n",
    //             (void *)path, ctx->fill_dsc.opa, ctx->fill_dsc.style, ctx->stroke_dsc.opa,
    //             blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);

    /* Clear-area subtask: path == NULL, fill the scissor rect with fill color */
    if (path == NULL) {
        jlgpu_vec_t *jp = lv_jlgpu_path_get(u);

        jlgpu_area_t render_area;
        render_area.x_min = blend_area.x1;
        render_area.x_max = blend_area.x2 + 1;
        render_area.y_min = blend_area.y1;
        render_area.y_max = blend_area.y2 + 1;
        lv_jlgpu_path_set_bez_div(jp, LV_MIN(jlgpu_compute_div(render_area), (int)JLGPU_GLOBAL_MAX_BEZ_DIV));

        jlgpu_path_append_rect(jp,
                               (float)blend_area.x1, (float)blend_area.y1,
                               (float)lv_area_get_width(&blend_area), (float)lv_area_get_height(&blend_area),
                               0.0f);
        jlgpu_path_end(jp);

        uint32_t argb = lv_color32_to_argb(ctx->fill_dsc.color, LV_OPA_MIX2(ctx->fill_dsc.opa, t->opa));
        jlgpu_vector_fill_submit(u, t, &blend_area, jp, argb, JLGPU_NON_ZERO);
        return;
    }

    if (ctx->fill_dsc.opa == LV_OPA_TRANSP && ctx->stroke_dsc.opa == LV_OPA_TRANSP) {
        // printf("[JLGPU-VEC] cb: fill+stroke both transparent, bail\n");
        return;
    }

    /* Phase 1: SOLID fill only */
    if (ctx->stroke_dsc.opa) {
        //LV_LOG_WARN("jlgpu vector: stroke is not supported yet");
    }

    if (ctx->fill_dsc.style != LV_VECTOR_DRAW_STYLE_SOLID) {
        //LV_LOG_WARN("jlgpu vector: only SOLID fill is supported yet (style=%d)", ctx->fill_dsc.style);
        // printf("[JLGPU-VEC] cb: non-solid style=%d, bail\n", ctx->fill_dsc.style);
        return;
    }

    /* Build the JLGPU path from the vector path, applying the subtask matrix */
    jlgpu_vec_t *jp = lv_jlgpu_path_get(u);

    jlgpu_area_t render_area;
    render_area.x_min = blend_area.x1;
    render_area.x_max = blend_area.x2 + 1;
    render_area.y_min = blend_area.y1;
    render_area.y_max = blend_area.y2 + 1;
    lv_jlgpu_path_set_bez_div(jp, LV_MIN(jlgpu_compute_div(render_area), (int)JLGPU_GLOBAL_MAX_BEZ_DIV));

    lv_path_to_jlgpu(jp, path, &ctx->matrix);
    jlgpu_path_end(jp);

    // printf("[JLGPU-VEC] cb: bez_div=%d edge_num=%d total_len=%d\n",
    //             jp->bez_div, jp->edge_num, jp->total_len);

    uint32_t argb = lv_color32_to_argb(ctx->fill_dsc.color, LV_OPA_MIX2(ctx->fill_dsc.opa, t->opa));
    jlgpu_vector_fill_submit(u, t, &blend_area, jp, argb, lv_fill_to_jlgpu(ctx->fill_dsc.fill_rule));
}

/* Convert an LVGL vector path into the JLGPU segment stream, applying `matrix`. */
static void lv_path_to_jlgpu(jlgpu_vec_t *dest, const lv_vector_path_t *src, const lv_matrix_t *matrix)
{
    const lv_vector_path_op_t *ops = lv_array_front(&src->ops);
    const lv_fpoint_t *point = lv_array_front(&src->points);
    const uint32_t op_size = lv_array_size(&src->ops);

    bool has_matrix = !lv_matrix_is_identity(matrix);

    /* Track the current subpath start point so CLOSE can be emulated */
    float start_x = 0.0f;
    float start_y = 0.0f;

    for (uint32_t i = 0; i < op_size; i++) {
        switch (ops[i]) {
        case LV_VECTOR_PATH_OP_MOVE_TO: {
            lv_fpoint_t p = *point++;
            if (has_matrix) {
                lv_matrix_transform_point(matrix, &p);
            }
            start_x = p.x;
            start_y = p.y;
            jlgpu_path_move_to(dest, p.x, p.y);
        }
        break;

        case LV_VECTOR_PATH_OP_LINE_TO: {
            lv_fpoint_t p = *point++;
            if (has_matrix) {
                lv_matrix_transform_point(matrix, &p);
            }
            jlgpu_path_line_to(dest, p.x, p.y);
        }
        break;

        case LV_VECTOR_PATH_OP_QUAD_TO: {
            lv_fpoint_t c = *point++;
            lv_fpoint_t p = *point++;
            if (has_matrix) {
                lv_matrix_transform_point(matrix, &c);
                lv_matrix_transform_point(matrix, &p);
            }
            jlgpu_path_quad_to(dest, c.x, c.y, p.x, p.y);
        }
        break;

        case LV_VECTOR_PATH_OP_CUBIC_TO: {
            lv_fpoint_t c1 = *point++;
            lv_fpoint_t c2 = *point++;
            lv_fpoint_t p = *point++;
            if (has_matrix) {
                lv_matrix_transform_point(matrix, &c1);
                lv_matrix_transform_point(matrix, &c2);
                lv_matrix_transform_point(matrix, &p);
            }
            jlgpu_path_cubic_to(dest, c1.x, c1.y, c2.x, c2.y, p.x, p.y);
        }
        break;

        case LV_VECTOR_PATH_OP_CLOSE: {
            /* `jlgpu_path_close` is a no-op: close the subpath with an explicit line */
            jlgpu_path_line_to(dest, start_x, start_y);
        }
        break;

        default:
            break;
        }
    }
}

/* Pack a lv_color32_t into a premultiplied ARGB8888 word. */
static uint32_t lv_color32_to_argb(lv_color32_t color, lv_opa_t opa)
{
    uint8_t a = LV_OPA_MIX2(color.alpha, opa);
    if (a < LV_OPA_COVER) {
        color.red = LV_UDIV255(color.red * a);
        color.green = LV_UDIV255(color.green * a);
        color.blue = LV_UDIV255(color.blue * a);
    }
    return ((uint32_t)a << 24) | ((uint32_t)color.red << 16) | ((uint32_t)color.green << 8) | (uint32_t)color.blue;
}

static jlgpu_fillrule_t lv_fill_to_jlgpu(lv_vector_fill_t rule)
{
    return (rule == LV_VECTOR_FILL_EVENODD) ? JLGPU_EVEN_ODD : JLGPU_NON_ZERO;
}

/* Submit a fully-built (and ended) path as a solid fill, in list or immediate mode. */
static void jlgpu_vector_fill_submit(lv_draw_jlgpu_unit_t *u, lv_draw_task_t *t,
                                     const lv_area_t *blend_area,
                                     jlgpu_vec_t *path,
                                     uint32_t argb_color,
                                     jlgpu_fillrule_t fill_rule)
{
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    // printf("[JLGPU-VEC] submit: is_disp_buf=%d edge_num=%d total_len=%d dest_cf=%d\n",
    //             is_disp_buf, path->edge_num, path->total_len, dest_cf);

    int width  = lv_area_get_width(blend_area);
    int height = lv_area_get_height(blend_area);
    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    uint32_t fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t jlgpu_path = {0};

    draw_fill_params.color.b = argb_color & 0xFF;
    draw_fill_params.color.g = (argb_color >> 8) & 0xFF;
    draw_fill_params.color.r = (argb_color >> 16) & 0xFF;
    draw_fill_params.color.a = (argb_color >> 24) & 0xFF;

    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    draw_fill_params.paint_info.pt_clip.x_min = 0;
    draw_fill_params.paint_info.pt_clip.x_max = width;
    draw_fill_params.paint_info.pt_clip.y_min = 0;
    draw_fill_params.paint_info.pt_clip.y_max = height;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area.x_min = blend_area->x1;
    render_params.render_area.x_max = blend_area->x2 + 1;
    render_params.render_area.y_min = blend_area->y1;
    render_params.render_area.y_max = blend_area->y2 + 1;

    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_PATH_MSAA_LVL;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num = path->edge_num;
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

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        // printf("[JLGPU-VEC] submit: LIST mode\n");
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            // printf("[JLGPU-VEC] submit: mem_pool_get_buf NULL (task_len=%d), bail\n", task_len);
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_fill(u->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
        lv_jlgpu_add_task_node(u, task);
#endif
    } else { //layer/canvas需要立即绘制
        // printf("[JLGPU-VEC] submit: IMMEDIATE mode\n");
        uint8_t *cbuf = NULL;
        uint8_t *ebuf = NULL;

        lv_jlgpu_ctx_t jlgpu_ctx;
        lv_jlgpu_ctx_init(&jlgpu_ctx);
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        task = lv_malloc(task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            goto malloc_fail;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
        jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);

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
            LV_LOG_ERROR("malloc cbuf fail");
            goto malloc_fail;
        }
        lv_memset(cbuf, 0, mask_h * fb_width);

        ebuf = (uint8_t *)lv_malloc(8 * edge_num);
        if (ebuf == NULL) {
            LV_LOG_ERROR("malloc ebuf fail");
            goto malloc_fail;
        }
        lv_memset(ebuf, 0, 8 * edge_num);

        global_path.c_data = cbuf;
        global_path.e_data = ebuf;

        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);

        uint32_t t0 = get_system_ms();
        jlgpu_run(jlgpu_ctx.ctx, task);
        uint32_t t1 = get_system_ms();
        // printf("[JLGPU-VEC] IMMEDIATE jlgpu_run rasterize: %u ms (edges=%d, %dx%d)\n",t1 - t0, edge_num, width, height);

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
}

#endif /*LV_USE_DRAW_JLGPU_VECTOR_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
