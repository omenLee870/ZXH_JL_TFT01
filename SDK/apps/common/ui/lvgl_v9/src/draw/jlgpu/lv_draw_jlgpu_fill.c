/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "../../misc/lv_area_private.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_FILL_ENABLE == 1)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_draw_jlgpu_fill_with_grad(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);
static void lv_draw_jlgpu_fill_with_roundgrad(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);
/**********************
 *  STATIC VARIABLES
 **********************/
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


#define JLGPU_TEST_ 1

#if JLGPU_TEST_

//compute div
static inline float jlgpu_abs_(float x)
{
    float y;
    y = x < 0 ? -x : x;
    return y;
}

static inline float jlvg_point2_len_(jlgpu_point2_t *s0, jlgpu_point2_t *s1)
{
#if 0
    float len = (float)sqrtf((s0->x - s1->x) * (s0->x - s1->x) + (s0->y - s1->y) * (s0->y - s1->y));
    return len;
#else
    float len = (float)(jlgpu_abs_(s0->x - s1->x) + jlgpu_abs_(s0->y - s1->y));
    return len;
#endif
}

static inline unsigned  int ilog2_(unsigned  int x)
{
    unsigned  int l = 0;
    if (x >= 1 << 16) {
        x >>= 16;
        l |= 16;
    }
    if (x >= 1 << 8) {
        x >>= 8;
        l |= 8;
    }
    if (x >= 1 << 4) {
        x >>= 4;
        l |= 4;
    }
    if (x >= 1 << 2) {
        x >>= 2;
        l |= 2;
    }
    if (x >= 1 << 1) {
        l |= 1;
    }
    return l;
}

static inline float quad_control_length_(jlgpu_point2_t *a, jlgpu_point2_t *b, jlgpu_point2_t *c)
{
    float d1, d2;
    d1 = jlvg_point2_len_(b, a);
    d2 = jlvg_point2_len_(b, c);

    return d1 + d2;
}

static inline int quad_estimate_ndiv_(jlgpu_point2_t *a, jlgpu_point2_t *b, jlgpu_point2_t *c, float scale)
{
    float d;
    int ndiv;
    d = quad_control_length_(a, b, c);
    ndiv = (int)(d * scale);

    if (ndiv > 256) {
        ndiv = 256;
    }
    if (ndiv < 2) {
        ndiv = 2;
    }
    ndiv = ilog2_(ndiv) - 1;

    return ndiv;
}

static inline float cubic_control_length_(jlgpu_point2_t *a, jlgpu_point2_t *b, jlgpu_point2_t *c, jlgpu_point2_t *d)
{
    float d1, d2, d3;
    d1 = jlvg_point2_len_(b, a);
    d2 = jlvg_point2_len_(c, b);
    d3 = jlvg_point2_len_(d, c);;
    return d1 + d2 + d3;
}

static inline int cubic_estimate_ndiv_(jlgpu_point2_t *a, jlgpu_point2_t *b, jlgpu_point2_t *c, jlgpu_point2_t *d, float scale)
{
    float distance;
    int ndiv;
    distance = cubic_control_length_(a, b, c, d);
    ndiv = (int)(distance * scale);

    if (ndiv > 256) {
        ndiv = 256;
    }
    if (ndiv < 2) {
        ndiv = 2;
    }
    ndiv = ilog2_(ndiv) - 1;

    return ndiv;
}

static inline void set_coord_f32_(uint8_t *data, int i, float c)
{
    int16_t p = (int)(c * (8) + 0.5);
    int16_t *adr = (int16_t *)(data + i);
    *adr = p;
}

static inline float get_coord_f32_(void *data, int i, uint8_t data_type)
{
    if (data_type == JLGPU_PATH_DATATYPE_F32) {
        return ((float *)data)[i];
    } else {
        return (float)(((int32_t *)data)[i]);
    }
}

static void compute_tessellate_div_(jlgpu_path_param_t *path, float div_scale)
{
    jlgpu_point2_t o, p;

    o.x = 0.f;
    o.y = 0.f;
    p.x = 0.f;
    p.y = 0.f;

    int div = 0;

    int seg_index = 0;
    int coord_index = 0;

    float *coord_data = path->data;
    while (seg_index < path->nsegments) {
        uint8_t seg_type = path->segments[seg_index++];
        //int coords = compute_Coordinates(seg_type);


        switch (seg_type) {
        case JLGPU_CLOSE_PATH: {
            //Do not support JLGPU_CLOSE_PATH @ 20230512
            printf("not support seg type(%d)\n", seg_type);
            break;
        }

        case JLGPU_MOVE_TO: {
            jlgpu_point2_t c;
            c.x = coord_data[coord_index];
            c.y = coord_data[coord_index + 1];

            coord_index += 2;


            p = c;
            o = c;

            break;
        }

        case JLGPU_LINE_TO: {
            jlgpu_point2_t c;
            c.x = coord_data[coord_index];
            c.y = coord_data[coord_index + 1];

            coord_index += 2;

            p = c;
            o = c;
            break;
        }

        case JLGPU_HLINE_TO: {
            jlgpu_point2_t c;
            c.x = coord_data[coord_index];
            c.y = o.y;

            coord_index += 1;


            p = c;
            o = c;
            break;
        }

        case JLGPU_VLINE_TO: {
            jlgpu_point2_t c;
            c.x = o.x;
            c.y = coord_data[coord_index];

            coord_index += 1;

            p = c;
            o = c;

            break;
        }

        case JLGPU_QUAD_TO: {

            jlgpu_point2_t c0;
            jlgpu_point2_t c1;

            c0.x = coord_data[coord_index];
            c0.y = coord_data[coord_index + 1];
            c1.x = coord_data[coord_index + 2];
            c1.y = coord_data[coord_index + 3];

            coord_index += 4;

            div = quad_estimate_ndiv_(&o, &c0, &c1, div_scale);
            path->segments[seg_index - 1] = seg_type | (div << 4);

            p = c0;
            o = c1;

            break;
        }

        case JLGPU_SQUAD_TO: {
            jlgpu_point2_t c0;
            c0.x = 2 * o.x - p.x;
            c0.y = 2 * o.y - p.y;

            jlgpu_point2_t c1;
            c1.x = coord_data[coord_index];
            c1.y = coord_data[coord_index + 1];


            coord_index += 2;

            div = quad_estimate_ndiv_(&o, &c0, &c1, div_scale);
            path->segments[seg_index - 1] = seg_type | (div << 4);

            p = c0;
            o = c1;

            break;
        }

        case JLGPU_CUBIC_TO: {

            jlgpu_point2_t c0;
            jlgpu_point2_t c1;
            jlgpu_point2_t c2;

            c0.x = coord_data[coord_index];
            c0.y = coord_data[coord_index + 1];
            c1.x = coord_data[coord_index + 2];
            c1.y = coord_data[coord_index + 3];
            c2.x = coord_data[coord_index + 4];
            c2.y = coord_data[coord_index + 5];



            coord_index += 6;



            div = cubic_estimate_ndiv_(&o, &c0, &c1, &c2, div_scale);
            path->segments[seg_index - 1] = seg_type | (div << 4);

            p = c1;
            o = c2;

            break;
        }

        case JLGPU_SCUBIC_TO: {

            jlgpu_point2_t c0;
            c0.x = 2 * o.x - p.x;
            c0.y = 2 * o.y - p.y;

            jlgpu_point2_t c1;
            jlgpu_point2_t c2;

            c1.x = coord_data[coord_index];
            c1.y = coord_data[coord_index + 1];
            c2.x = coord_data[coord_index + 2];
            c2.y = coord_data[coord_index + 3];



            coord_index += 4;

            div = cubic_estimate_ndiv_(&o, &c0, &c1, &c2, div_scale);
            path->segments[seg_index - 1] = seg_type | (div << 4);

            p = c1;
            o = c2;

            break;
        }

        default: {
            printf("not support seg type(%d)\n", seg_type);
            break;
        }
        }

    }
}


static void tessellate_div_default_(jlgpu_path_param_t *path)
{
    int div = 7;
    int seg_index = 0;
    int coord_index = 0;

    while (seg_index < path->nsegments) {
        uint8_t seg_type = path->segments[seg_index++];


        switch (seg_type) {
        case JLGPU_CLOSE_PATH: {
            //Do not support JLGPU_CLOSE_PATH @ 20230512
            printf("not support seg type(%d)\n", seg_type);
            break;
        }

        case JLGPU_MOVE_TO: {
            break;
        }

        case JLGPU_LINE_TO: {
            break;
        }

        case JLGPU_HLINE_TO: {
            break;
        }

        case JLGPU_VLINE_TO: {
            break;
        }

        case JLGPU_QUAD_TO: {
            path->segments[seg_index - 1] = seg_type | (div << 4);


            break;
        }

        case JLGPU_SQUAD_TO: {
            path->segments[seg_index - 1] = seg_type | (div << 4);
            break;
        }

        case JLGPU_CUBIC_TO: {
            path->segments[seg_index - 1] = seg_type | (div << 4);
            break;
        }

        case JLGPU_SCUBIC_TO: {
            path->segments[seg_index - 1] = seg_type | (div << 4);
            break;
        }

        default: {
            printf("not support seg type(%d)\n", seg_type);
            break;
        }
        }

    }
}

static int compute_Coordinates(uint8_t segment)
{
    static const int coords[13] = { 0, 2, 2, 1, 1, 4, 2, 6, 4, 5, 5, 5, 5 };
    return coords[(int)segment];
}

int jlgpu_path_update_(jlgpu_path_param_t *path, jlgpu_path_t *jlgpu_path, uint8_t global_max_bez_div)
{
    if (!path->nsegments) {
        return 0;
    }
    // if (jlgpu_path->div_en)
    // {
    //     compute_tessellate_div_(path, jlgpu_path->div_scale);
    // }
    // else
    // {
    //     tessellate_div_default_(path);
    // }

    int seg_len = path->nsegments + path->ndata * 2 + 1;
    uint16_t *seg = (uint16_t *)jlgpu_path->seg_data; //(uint8_t*)malloc(seg_len);

    int max_bez_div = global_max_bez_div;
    if (max_bez_div > jlgpu_path->max_bez_div) {
        max_bez_div = jlgpu_path->max_bez_div;
    }
    max_bez_div += 1;

    int edges = (1 << max_bez_div);
    //int cur_div = 7;
    int edge_num = 0;


    int dstIdx = 0;
    int srcIdx = 0;
    uint8_t segment, segment0, segment1;
    int nsegments_half = path->nsegments / 2;
    int segments_not_align = path->nsegments % 2;

    float *tmp_data = path->data;
    int coord_len;
    int16_t coord_value;
    for (int i = 0; i < nsegments_half; i++) {
        segment0 = path->segments[i * 2];
        segment1 = path->segments[i * 2 + 1];

        if (segment0 >= JLGPU_QUAD_TO) {
            segment0 =   segment0 | (7 << 4);
            edge_num = edge_num + edges;
        }
        if (segment1 >= JLGPU_QUAD_TO) {
            segment1 =   segment1 | (7 << 4);
            edge_num = edge_num + edges;
        }


        *seg++ = (segment0 | (segment1 << 8));


        segment = segment0 & 0xf;

        coord_len = compute_Coordinates(segment);
        for (int k = 0; k < coord_len; k++) {
            coord_value = (*(tmp_data++)) * 8;
            *(seg++) = coord_value;
        }



        //second
        segment = segment1 & 0xf;

        coord_len = compute_Coordinates(segment);
        for (int k = 0; k < coord_len; k++) {
            coord_value = (*(tmp_data++)) * 8;
            *(seg++) = coord_value;
        }



    }

    if (segments_not_align) { //not align
        segment0 = path->segments[path->nsegments - 1];

        if (segment0 >= JLGPU_QUAD_TO) {
            segment0 =   segment0 | (7 << 4);
            edge_num = edge_num + edges;
        }

        *seg++ = (segment0 | (0xf << 8));

        segment = segment0 & 0xf;

        coord_len = compute_Coordinates(segment);
        for (int k = 0; k < coord_len; k++) {
            coord_value = (*(tmp_data++)) * 8;
            *(seg++) = coord_value;
        }
    } else {
        *((uint8_t *)seg) = 0xf;
    }

    //jlgpu_path->seg_data = seg;
    jlgpu_path->seg_len = seg_len;

    //JLGPU_ASSERT(dstIdx == seg_len);

    return edge_num;
}

#endif

void lv_draw_jlgpu_fill(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    /*PB0_HIGH();*/
    //log_debug("%s()", __func__);
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

    //绘制渐变填充,暂时只支持LV_GRAD_DIR_VER,LV_GRAD_DIR_HOR
    if (dsc->grad.dir == LV_GRAD_DIR_VER || dsc->grad.dir == LV_GRAD_DIR_HOR) {
        lv_draw_jlgpu_fill_with_grad(t, dsc, coords);
        return;
    }

    if (dsc->radius && LV_USE_GPU_GRAD_TO_ROUND) {
        lv_draw_jlgpu_fill_with_roundgrad(t, dsc, coords);
        return;
    }

    /*printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);*/

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);
    int fill_width  = lv_area_get_width(coords);
    int fill_height = lv_area_get_height(coords);

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);
    fb_width  = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint32_t color;
    lv_draw_jlgpu_premult_and_swizzle(&color, dsc->color, dsc->opa);

    /* 圆角半径要按控件自己的尺寸夹, 不能用裁剪后的窗口尺寸:
     * 脏区渲染时窗口可能远小于控件, 会把半径夹小 → 圆角画错。 */
    int32_t short_side = LV_MIN(fill_width, fill_height);
    int32_t rout = LV_MIN(dsc->radius, short_side >> 1);

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

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

    render_params.blend_mode = JLGPU_BLEND_SRC;
    if (dsc->opa == LV_OPA_COVER) {
        render_params.global_alpha = 128;
    } else {
        render_params.global_alpha = dsc->opa / 2;
    }

    render_params.render_quality = JLGPU_RENDERING_QUALITY_NONANTIALIASED;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int path_div = jlgpu_compute_div(render_params.render_area);

    jlgpu_vec_t *rect_path = lv_jlgpu_path_get(u);

    lv_jlgpu_path_set_bez_div(rect_path, LV_MIN(path_div, global_max_bez_div));

    jlgpu_path_append_rect(rect_path, (float)coords->x1, (float)coords->y1, fill_width, fill_height, rout);
    jlgpu_path_end(rect_path);

    int edge_num = rect_path->edge_num;
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
    } else {//layer/canvas需要立即绘制

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
    /*PB0_LOW();*/
}

//fixme:使用lv_obj_center()后出现渐变错误问题
static void lv_draw_jlgpu_fill_with_grad(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
        return;
    }

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    //lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;   //仅考虑帧buf情况
    if (!lv_area_intersect(&blend_area, coords, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }
    /*printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);*/

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);
    int fill_width  = lv_area_get_width(coords);
    int fill_height = lv_area_get_height(coords);

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint32_t main_color;
    lv_draw_jlgpu_premult_and_swizzle(&main_color, dsc->grad.stops[0].color, dsc->grad.stops[0].opa);

    uint32_t stops_color;
    lv_draw_jlgpu_premult_and_swizzle(&stops_color, dsc->grad.stops[dsc->grad.stops_count - 1].color, dsc->grad.stops[dsc->grad.stops_count - 1].opa);
#if 1
    /* 预计算完整渐变 CLUT 并提高 clut_level，降低渐变分层(色带)。
     * clut_level=0 时 CLUT 只有 2 级颜色(起/止)，硬件在两级之间插值精度不足会出现明显色带；
     * 提高到 8 级(2^8+1=257 色)后相邻颜色差异足够小，分层不可见 */
    uint8_t lut_lvl = 8;
    uint32_t clut_data_2[(1 << 8) + 1];

    int ca = (int)((main_color >> 24) & 0xff);
    int cr = (int)((main_color >> 16) & 0xff);
    int cg = (int)((main_color >> 8) & 0xff);
    int cb = (int)(main_color & 0xff);
    int sa = (int)((stops_color >> 24) & 0xff);
    int sr = (int)((stops_color >> 16) & 0xff);
    int sg = (int)((stops_color >> 8) & 0xff);
    int sb = (int)(stops_color & 0xff);

    for (int i = 0; i < (1 << 8) + 1; i++) {
        uint32_t a = (uint32_t)(ca + ((sa - ca) * i) / 256);
        uint32_t r = (uint32_t)(cr + ((sr - cr) * i) / 256);
        uint32_t g = (uint32_t)(cg + ((sg - cg) * i) / 256);
        uint32_t b = (uint32_t)(cb + ((sb - cb) * i) / 256);
        clut_data_2[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

#else
    uint32_t clut_data_2[2] = {main_color, stops_color};
    uint8_t lut_lvl = 0;
#endif

    uint32_t *clut_data = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        clut_data = (uint32_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data_2, sizeof(clut_data_2));
#endif
    } else {
        clut_data = clut_data_2;
    }

    /* 圆角半径要按控件自己的尺寸夹, 不能用裁剪后的窗口尺寸:
     * 脏区渲染时窗口可能远小于控件, 会把半径夹小 → 圆角画错。 */
    int32_t short_side = LV_MIN(fill_width, fill_height);
    int32_t rout = LV_MIN(dsc->radius, short_side >> 1);

    jlgpu_draw_grad_t draw_grad = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

    draw_grad.clut_level = lut_lvl;
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    jlgpu_lineargrad_param_t lineargrad_param;

    lv_grad_dir_t grad_dir = dsc->grad.dir;

    /* 渐变起止点必须按控件自己的区域(coords)算, 不能用裁剪后的 blend_area:
     * 脏区渲染时 clip 只覆盖控件的一部分, 拿 blend_area 会把整条渐变压缩进这块脏区,
     * 起点的颜色/透明度就算错了; 同一块渐变分散在不同帧里画时还会互相对不上, 出现断层。 */
    if (grad_dir == LV_GRAD_DIR_HOR) {
        lineargrad_param.point_start.x = coords->x1;
        lineargrad_param.point_start.y = coords->y1;
        lineargrad_param.point_end.x = coords->x2 + 1;
        lineargrad_param.point_end.y = coords->y1;
    } else {
        lineargrad_param.point_start.x = coords->x1;
        lineargrad_param.point_start.y = coords->y1;
        lineargrad_param.point_end.x = coords->x1;
        lineargrad_param.point_end.y = coords->y2 + 1;
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

    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;
    render_params.dither_en = 1;   //使能抖动,降低渐变色带(RGB565 输出时尤其明显)

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    jlgpu_path_param_t path_params;
    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num;
    int path_div = jlgpu_compute_div(render_params.render_area);

    jlgpu_vec_t *rect_path = lv_jlgpu_path_get(u);
    lv_jlgpu_path_set_bez_div(rect_path, LV_MIN(path_div, global_max_bez_div));

    jlgpu_path_append_rect(rect_path, (float)coords->x1, (float)coords->y1, fill_width, fill_height, rout);

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

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
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
    } else {//layer/canvas需要立即绘制

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
        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num); //edge 2word
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

//使用径向渐变实现填充圆角
static void lv_draw_jlgpu_fill_with_roundgrad(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;

    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
        return;
    }

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    //lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, coords, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }
    /*printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);*/

    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = draw_buf->data;
    bool is_disp_buf = lv_is_draw_buf(dest_buf);
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int width  = lv_area_get_width(&blend_area);
    int height = lv_area_get_height(&blend_area);
    int fill_width  = lv_area_get_width(coords);
    int fill_height = lv_area_get_height(coords);

    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;

    uint8_t r0, g0, b0, a0;
    r0 = dsc->color.red;
    g0 = dsc->color.green;
    b0 = dsc->color.blue;
    a0 = 255;

    uint32_t main_color = ((uint32_t)a0 << 24)
                          | ((uint32_t)r0 << 16)
                          | ((uint32_t)g0 << 8)
                          | (uint32_t)b0;

    uint32_t stops_color = 0;

    uint32_t clut_data_2[2] = {main_color, stops_color};
    uint32_t *clut_data = NULL;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        clut_data = (uint32_t *)lv_jlgpu_attach_buf_list_add(u, (uint8_t *)clut_data_2, sizeof(clut_data_2));
#endif
    } else {
        clut_data = clut_data_2;
    }

    /* 圆角半径要按控件自己的尺寸夹, 不能用裁剪后的窗口尺寸:
     * 脏区渲染时窗口可能远小于控件, 会把半径夹小 → 圆角画错。 */
    int32_t short_side = LV_MIN(fill_width, fill_height);
    int32_t rout = LV_MIN(dsc->radius, short_side >> 1);

    jlgpu_draw_grad_t draw_grad = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

    uint8_t lut_lvl = 0;

    draw_grad.clut_level = lut_lvl;
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    jlgpu_roundgrad_param_t roundgrad_param;
    roundgrad_param.point_center.x = coords->x1 + fill_width / 2;
    roundgrad_param.point_center.y = coords->y1 + fill_height / 2;
    roundgrad_param.in_width   = (fill_width - 2 * rout);
    roundgrad_param.in_height  = (fill_height - 2 * rout);
    /* 羽化带放在按钮内部(in→out = rout-1→rout)，避免被 render_area 裁剪；
     * 并保证 out_radius > in_radius，避免 jlgpu_computeRoundParameters 内
     * 1/(out_radius - in_radius) 除0 */
    roundgrad_param.in_radius  = rout > 0 ? rout - 1 : 0;
    roundgrad_param.out_radius = rout > 0 ? rout : 1;

    jlgpu_computeRoundParameters(draw_grad.clut_level, &roundgrad_param, &matrix);
    draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

    draw_grad.paint_info.premult = 1;
    draw_grad.paint_info.pt_clip.x_min = -4096;
    draw_grad.paint_info.pt_clip.x_max = 4095;
    draw_grad.paint_info.pt_clip.y_min = -4096;
    draw_grad.paint_info.pt_clip.y_max = 4095;
    draw_grad.lineargrad_ptr = &roundgrad_param;
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

    if (dsc->opa == LV_OPA_COVER) {
        render_params.global_alpha = 128;
    } else {
        render_params.global_alpha = dsc->opa / 2;
    }

    render_params.render_quality = JLGPU_RENDER_QUALITY;
    render_params.dither_en = 1;   //使能抖动,降低羽化边缘色带

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    /*mask_params.mask_ptr = &jlgpu_path;*/

    jlgpu_path_param_t path_params;
    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num = 32;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
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
    } else {//layer/canvas需要立即绘制

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
        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num); //edge 2word
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
 *   STATIC FUNCTIONS
 **********************/


#endif /*LV_USE_DRAW_JLGPU_FILL_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
