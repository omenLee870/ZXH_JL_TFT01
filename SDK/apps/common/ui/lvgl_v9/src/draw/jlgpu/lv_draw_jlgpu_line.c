/**
 * @file lv_draw_jlgpu_line.c
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
#if (LV_USE_DRAW_JLGPU_LINE_ENABLE == 1)


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
void lv_draw_jlgpu_line(lv_draw_task_t *t, const lv_draw_line_dsc_t *dsc)
{

    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    float p1_x = dsc->p1.x;
    float p1_y = dsc->p1.y;
    float p2_x = dsc->p2.x;
    float p2_y = dsc->p2.y;

    if (p1_x == p2_x && p1_y == p2_y) {
        return;
    }

    float half_w = dsc->width * 0.5f;

    /* 线自己的外接矩形(未裁剪), is_slender 要按它判断 */
    lv_area_t line_area;
    line_area.x1 = (int32_t)(LV_MIN(p1_x, p2_x) - half_w);
    line_area.x2 = (int32_t)(LV_MAX(p1_x, p2_x) + half_w);
    line_area.y1 = (int32_t)(LV_MIN(p1_y, p2_y) - half_w);
    line_area.y2 = (int32_t)(LV_MAX(p1_y, p2_y) + half_w);

    lv_area_t rel_clip_area = line_area;
    if (!lv_area_intersect(&rel_clip_area, &rel_clip_area, &t->clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }

    LV_PROFILER_DRAW_BEGIN;

    lv_area_t blend_area = rel_clip_area;   //仅考虑帧buf情况
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;
    bool is_slender = false;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);

    //两个像素以内无需计算path直接填充即可
    /* 必须按线自己的尺寸判断, 不能用裁剪后的窗口尺寸: 窗口可能把一条斜线裁成很窄的一条,
     * 用窗口尺寸会把斜线误判成"细线", 于是跳过矢量路径、直接按矩形填充整个窗口,
     * 画出来的就不是这条线了。 */
    if (lv_area_get_width(&line_area) <= 2 || lv_area_get_height(&line_area) <= 2) {
        is_slender = true;
    }

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint32_t color;
    lv_draw_jlgpu_premult_and_swizzle(&color, dsc->color, dsc->opa);

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

    mask_params.mask_mode = is_slender ? JLGPU_MASK_MODE_NONE : JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num = 0;
    int path_div = jlgpu_compute_div(render_params.render_area);

    if (!is_slender) {
        jlgpu_vec_t *path = lv_jlgpu_path_get(u);
        lv_jlgpu_path_set_bez_div(path, LV_MIN(path_div, global_max_bez_div));

        int32_t dash_width = dsc->dash_width;
        int32_t dash_gap = dsc->dash_gap;
        int32_t dash_l = dash_width + dash_gap;

        float dx = p2_x - p1_x;
        float dy = p2_y - p1_y;
        float inv_dl = math_fast_inv_sqrtf(SQ(dx) + SQ(dy));
        float w_dx = dsc->width * dy * inv_dl;
        float w_dy = dsc->width * dx * inv_dl;
        float w2_dx = w_dx / 2;
        float w2_dy = w_dy / 2;

        int32_t ndash = 0;
        if (dash_width && dash_l * inv_dl < 1.0f) {
            ndash = (int32_t)((1.0f / inv_dl + dash_l - 1) / dash_l);
        }

        /* head point */
        float head_start_x = p1_x + w2_dx;
        float head_start_y = p1_y - w2_dy;
        float head_end_x = p1_x - w2_dx;
        float head_end_y = p1_y + w2_dy;

        /* tail point */
        float tail_start_x = p2_x - w2_dx;
        float tail_start_y = p2_y + w2_dy;
        float tail_end_x = p2_x + w2_dx;
        float tail_end_y = p2_y - w2_dy;

        /*
              head_start        tail_end
                  *-----------------*
                 /|                 |\
                / |                 | \
        arc_c *(  *p1             p2*  )* arc_c
                \ |                 | /
                 \|                 |/
                  *-----------------*
              head_end          tail_start
        */

        /* move to start point */
        jlgpu_path_move_to(path, head_start_x, head_start_y);

        /* draw line head */
        if (dsc->round_start) {
            float arc_cx = p1_x - w2_dy;
            float arc_cy = p1_y - w2_dx;

            /* start 90deg arc */
            jlgpu_path_append_arc_right_angle(path,
                                              head_start_x, head_start_y,
                                              p1_x, p1_y,
                                              arc_cx, arc_cy);

            /* end 90deg arc */
            jlgpu_path_append_arc_right_angle(path,
                                              arc_cx, arc_cy,
                                              p1_x, p1_y,
                                              head_end_x, head_end_y);
        } else {
            jlgpu_path_line_to(path, head_end_x, head_end_y);
        }

        /* draw line body */
        jlgpu_path_line_to(path, tail_start_x, tail_start_y);

        /* draw line tail */
        if (dsc->round_end) {
            float arc_cx = p2_x + w2_dy;
            float arc_cy = p2_y + w2_dx;
            jlgpu_path_append_arc_right_angle(path,
                                              tail_start_x, tail_start_y,
                                              p2_x, p2_y,
                                              arc_cx, arc_cy);
            jlgpu_path_append_arc_right_angle(path,
                                              arc_cx, arc_cy,
                                              p2_x, p2_y,
                                              tail_end_x, tail_end_y);
        } else {
            jlgpu_path_line_to(path, tail_end_x, tail_end_y);
        }

        /* close draw line body */
        jlgpu_path_line_to(path, head_start_x, head_start_y);

        for (int32_t i = 0; i < ndash; i++) {
            float start_x = p1_x - w2_dx + dx * (i * dash_l + dash_width) * inv_dl;
            float start_y = p1_y + w2_dy + dy * (i * dash_l + dash_width) * inv_dl;

            jlgpu_path_move_to(path, start_x, start_y);
            jlgpu_path_line_to(path,
                               p1_x + w2_dx + dx * (i * dash_l + dash_width) * inv_dl,
                               p1_y - w2_dy + dy * (i * dash_l + dash_width) * inv_dl);
            jlgpu_path_line_to(path,
                               p1_x + w2_dx + dx * (i + 1) * dash_l * inv_dl,
                               p1_y - w2_dy + dy * (i + 1) * dash_l * inv_dl);
            jlgpu_path_line_to(path,
                               p1_x - w2_dx + dx * (i + 1) * dash_l * inv_dl,
                               p1_y + w2_dy + dy * (i + 1) * dash_l * inv_dl);
            jlgpu_path_line_to(path, start_x, start_y);
        }

        jlgpu_path_end(path);

        edge_num = path->edge_num;
        jlgpu_path.seg_len = path->total_len;
        jlgpu_path.seg_data = path->base;

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
        jlgpu_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;

    }

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

#endif /*LV_USE_DRAW_JLGPU*/
#endif /*LV_USE_DRAW_JLGPU_BORDER_ENABLE*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
