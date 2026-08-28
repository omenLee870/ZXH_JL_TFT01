/**
 * @file lv_draw_jlgpu_mask_rect.c
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
#if (LV_USE_DRAW_JLGPU_MASK_RECTANGLE_ENABLE == 1)
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
void lv_draw_jlgpu_mask_rect(lv_draw_task_t *t, const lv_draw_mask_rect_dsc_t *dsc, const lv_area_t *coords)
{

    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    LV_UNUSED(coords);
    lv_area_t draw_area;

    if (!lv_area_intersect(&draw_area, &dsc->area, &t->clip_area)) {
        return;
    }

    LV_PROFILER_DRAW_BEGIN;

    lv_area_t blend_area = *coords;
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

    uint32_t color = 0;

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

    render_params.blend_mode = JLGPU_BLEND_DST_IN;

    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num;
    int path_div = jlgpu_compute_div(render_params.render_area);

    jlgpu_vec_t *path = lv_jlgpu_path_get(u);

    lv_jlgpu_path_set_bez_div(path, LV_MIN(path_div, global_max_bez_div));

    jlgpu_path_append_rect(
        path,
        dsc->area.x1, dsc->area.y1,
        lv_area_get_width(&dsc->area), lv_area_get_height(&dsc->area),
        dsc->radius);

    jlgpu_path_append_rect(
        path,
        t->clip_area.x1, t->clip_area.y1,
        lv_area_get_width(&t->clip_area), lv_area_get_height(&t->clip_area),
        0);

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
    } else {//单任务

        lv_jlgpu_ctx_t jlgpu_ctx;
        lv_jlgpu_ctx_init(&jlgpu_ctx);

        int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        uint8_t *task = lv_malloc(task_len);
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

    LV_PROFILER_DRAW_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
