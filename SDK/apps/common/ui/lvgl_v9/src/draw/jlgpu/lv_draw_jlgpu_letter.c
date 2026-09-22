/**
 * @file lv_draw_jlgpu_letter.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE


#if LV_USE_DRAW_JLGPU
#if (LV_USE_DRAW_JLGPU_LABEL_ENABLE == 1)

#include "../../lvgl.h"
#include "../../../lvgl_private.h"
#include "../../libs/freetype/lv_freetype_private.h"

#include "jlgpu_math.h"
/*********************
 *      DEFINES
 *********************/

#define FT_F26DOT6_SHIFT 6
#define FT_F26DOT6_TO_PATH_SCALE(x) (LV_FREETYPE_F26DOT6_TO_FLOAT(x) / (1 << FT_F26DOT6_SHIFT))

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void /* LV_ATTRIBUTE_FAST_MEM */ draw_letter_jlgpu_cb(lv_draw_unit_t *draw_unit, \
        lv_draw_glyph_dsc_t *glyph_draw_dsc, \
        lv_draw_fill_dsc_t *fill_draw_dsc, \
        const lv_area_t *fill_area);
static void draw_letter_cb(lv_draw_task_t *t, lv_draw_glyph_dsc_t *glyph_draw_dsc,
                           lv_draw_fill_dsc_t *fill_draw_dsc, const lv_area_t *fill_area);
static void draw_letter_bitmap(lv_draw_task_t *t, const lv_draw_glyph_dsc_t *dsc);
static void draw_letter_outline(lv_draw_task_t *t, const lv_draw_glyph_dsc_t *dsc);

#if LV_USE_FREETYPE
static void freetype_outline_event_cb(lv_event_t *e);
#endif
/**********************
 *  STATIC VARIABLES
 **********************/
/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void multiply(jlgpu_matrix_t *matrix, jlgpu_matrix_t *mult)
{
    jlgpu_matrix_t temp;
    int row, column;

    /* Process all rows. */
    for (row = 0; row < 3; row++) {
        /* Process all columns. */
        for (column = 0; column < 3; column++) {
            /* Compute matrix entry. */
            temp.m[row][column] = (matrix->m[row][0] * mult->m[0][column])
                                  + (matrix->m[row][1] * mult->m[1][column])
                                  + (matrix->m[row][2] * mult->m[2][column]);
        }
    }

    memcpy(matrix, &temp, sizeof(temp));
}

static void vg_lite_rotate_test(jlgpu_matrix_t *matrix, float angle)
{
    /* Convert degrees into radians. */
    /*float_t angle = (degrees / 180.0f) * 3.141592654f;*/

    /* Compute cosine and sine values. */
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);

    /* Set rotation matrix. */
    /*jlgpu_matrix_t r = { { {cos_angle, -sin_angle, 0.0f},*/
    /*{sin_angle, cos_angle, 0.0f},*/
    /*{0.0f, 0.0f, 1.0f},*/
    /*},*/
    /*};*/

    /* Set rotation matrix. */
    jlgpu_matrix_t r = { {
            /*{cos_angle, -sin_angle, 0.0f},*/
            /*{sin_angle, cos_angle, 0.0f},*/
            /*{0.0f, 0.0f, 1.0f},*/


            {1.0f, 0.0f, 0.0f},
            {0.0f, cos_angle, -sin_angle},
            {0.0f, sin_angle, cos_angle},

        },
    };


    multiply(matrix, &r);

    /*return VG_LITE_SUCCESS;*/
}

void lv_draw_jlgpu_label_init(lv_draw_jlgpu_unit_t *u)
{
    LV_ASSERT_NULL(u);

#if LV_USE_FREETYPE
    /*Set up the freetype outline event*/
    lv_freetype_outline_add_event(freetype_outline_event_cb, LV_EVENT_ALL, u);
#endif /* LV_USE_FREETYPE */

}

void lv_draw_jlgpu_label(lv_draw_task_t *t, const lv_draw_label_dsc_t *dsc, const lv_area_t *coords)
{
    LV_PROFILER_DRAW_BEGIN;
    lv_draw_label_iterate_characters(t, dsc, coords, draw_letter_cb);
    LV_PROFILER_DRAW_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void draw_letter_cb(lv_draw_task_t *t, lv_draw_glyph_dsc_t *glyph_draw_dsc,
                           lv_draw_fill_dsc_t *fill_draw_dsc, const lv_area_t *fill_area)
{
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_draw_unit_t *draw_unit = (lv_draw_unit_t *)u;

    if (glyph_draw_dsc) {

        switch (glyph_draw_dsc->format) {
        case LV_FONT_GLYPH_FORMAT_A1:
        case LV_FONT_GLYPH_FORMAT_A2:
        case LV_FONT_GLYPH_FORMAT_A3:
        case LV_FONT_GLYPH_FORMAT_A4:
        case LV_FONT_GLYPH_FORMAT_A8: {

            glyph_draw_dsc->glyph_data = lv_font_get_glyph_bitmap(glyph_draw_dsc->g, glyph_draw_dsc->_draw_buf);

            if (!glyph_draw_dsc->glyph_data) {
                return;
            }

            draw_letter_bitmap(t, glyph_draw_dsc);
        }
        break;

#if LV_USE_FREETYPE
        case LV_FONT_GLYPH_FORMAT_VECTOR: {

            if (lv_freetype_is_outline_font(glyph_draw_dsc->g->resolved_font)) {
                if (!glyph_draw_dsc->glyph_data) {
                    return;
                }

                draw_letter_outline(t, glyph_draw_dsc);
            }
        }
        break;
#endif /* LV_USE_FREETYPE */

        case LV_FONT_GLYPH_FORMAT_IMAGE: {

            glyph_draw_dsc->glyph_data = lv_font_get_glyph_bitmap(glyph_draw_dsc->g, glyph_draw_dsc->_draw_buf);
            if (!glyph_draw_dsc->glyph_data) {
                return;
            }

            lv_draw_image_dsc_t image_dsc;
            lv_draw_image_dsc_init(&image_dsc);
            image_dsc.opa = glyph_draw_dsc->opa;
            image_dsc.src = glyph_draw_dsc->glyph_data;
            image_dsc.rotation = glyph_draw_dsc->rotation;
#if LV_USE_DRAW_JLGPU_IMG_ENABLE
            lv_draw_jlgpu_img(t, t->draw_dsc, &t->area);
#endif
        }
        break;

#if LV_USE_FONT_PLACEHOLDER

        case LV_FONT_GLYPH_FORMAT_NONE: {

            if (glyph_draw_dsc->bg_coords == NULL) {
                break;
            }
            /* Draw a placeholder rectangle*/
            lv_draw_border_dsc_t border_draw_dsc;
            lv_draw_border_dsc_init(&border_draw_dsc);
            border_draw_dsc.opa = glyph_draw_dsc->opa;
            border_draw_dsc.color = glyph_draw_dsc->color;
            border_draw_dsc.width = 1;

#if LV_USE_DRAW_JLGPU_BORDER_ENABLE
            lv_draw_jlgpu_border(t, &border_draw_dsc, glyph_draw_dsc->bg_coords);
#endif
        }
        break;

#endif /* LV_USE_FONT_PLACEHOLDER */

        default:
            break;
        }
    }

    if (fill_draw_dsc && fill_area) {
#if LV_USE_DRAW_JLGPU_FILL_ENABLE
        lv_draw_jlgpu_fill(t, t->draw_dsc, &t->area);
#endif
    }

    /* Flush in time to avoid accumulation of drawing commands */
    /*u->letter_count++;*/
    /*if (u->letter_count > PATH_FLUSH_COUNT_MAX) {*/
    /*lv_vg_lite_flush(u);*/
    /*}*/

}

static void draw_letter_bitmap(lv_draw_task_t *t, const lv_draw_glyph_dsc_t *dsc)
{

    LV_PROFILER_DRAW_BEGIN;

    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_area_t clip_area;
    if (!lv_area_intersect(&clip_area, &t->clip_area, dsc->letter_coords)) {
        return;
    }

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    lv_area_t coords = *dsc->letter_coords;

    lv_area_t image_tf_area;
    lv_area_copy(&image_tf_area, &coords);
    bool is_rotated = dsc->rotation % 3600 != 0;
    if (is_rotated) {
        lv_image_buf_get_transformed_area(
            &image_tf_area,
            lv_area_get_width(&coords),
            lv_area_get_height(&coords),
            dsc->rotation,
            256,
            256,
            &dsc->pivot);

        lv_area_move(&image_tf_area, coords.x1, coords.y1);
    }

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, &coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_image_tf_area;
    lv_area_copy(&rel_image_tf_area, &image_tf_area);
    lv_area_move(&rel_image_tf_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, &rel_image_tf_area, &rel_clip_area)) {
        LV_LOG_WARN("!lv_area_intersect");
        return;
    }

    const lv_draw_buf_t *src_draw_buf = dsc->glyph_data;

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

    uint32_t src_width  = src_draw_buf->header.w;
    uint32_t src_height = src_draw_buf->header.h;
    uint32_t src_stride = src_draw_buf->header.stride;
    uint8_t  src_bpp    = lv_color_format_get_bpp(src_draw_buf->header.cf);
    uint8_t in_format = lv_jlgpu_get_color_format(src_draw_buf->header.cf);

    uint8_t *img_buf = src_draw_buf->data;

    uint8_t adr_mode = JLGPU_ADRMODE_LINE;
    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    jlgpu_draw_img_t draw_img_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

    lv_area_t src_area;
    src_area.x1 = coords.x1;
    src_area.y1 = coords.y1;
    src_area.x2 = src_area.x1 + lv_area_get_width(&coords)  - 1;
    src_area.y2 = src_area.y1 + lv_area_get_height(&coords) - 1;

    jlgpu_area_t render_area;
    render_area.x_min = blend_area.x1;
    render_area.x_max = blend_area.x2 + 1;
    render_area.y_min = blend_area.y1;
    render_area.y_max = blend_area.y2 + 1;

    jlgpu_area_t tex_clip_area;
    tex_clip_area.x_min = CLAMP_TO_ZERO(t->clip_area.x1 - coords.x1);
    tex_clip_area.x_max = src_width;
    tex_clip_area.y_min = CLAMP_TO_ZERO(t->clip_area.y1 - coords.y1);
    tex_clip_area.y_max = src_height;

    jlgpu_img_info_t img_info = {0};
    img_info.premult = 1;
    img_info.tex_clip = tex_clip_area;
    img_info.adr_mode = adr_mode;
    img_info.color_format = in_format;
    img_info.data = img_buf;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        /* 字形位图必须拷进链表内存:链表是延后执行的,源buf在此期间可能已失效。
         * 申请失败只能放弃本字形,绝不能让 img_info.data == NULL 交给GPU(会读到非法纹理地址导致GPU挂死) */
        img_info.data = lv_jlgpu_mem_pool_get_buf(u, img_buf, src_height * src_stride);
        if (img_info.data == NULL) {
            /* 不要静默跳过:否则只是"少几个字",看不出是内存问题 */
            LV_LOG_ERROR("letter glyph buf malloc fail");
            return;
        }
#endif
    } else {
        img_info.data = img_buf;
    }

    img_info.height = src_height;
    img_info.width = src_width;
    img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
    img_info.compress_size = 0;

    if (is_rotated) {
        draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_AFFINE;
    } else {
        draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_NONE;
    }

    draw_img_params.img_info = img_info;
    draw_img_params.ax_color.b = dsc->color.blue;
    draw_img_params.ax_color.g = dsc->color.green;
    draw_img_params.ax_color.r = dsc->color.red;
    draw_img_params.ax_color.a = 255;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = render_area;
    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    render_params.global_alpha = 128;

    //变换
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);
    int x_ofs = src_area.x1;
    int y_ofs = src_area.y1;
    float rotation = -(dsc->rotation / 10) * 3.1415926 / 180.0f;

    if (is_rotated) {
        jlgpu_matrix_translate(&matrix, - dsc->pivot.x - x_ofs, - dsc->pivot.y - y_ofs);
        jlgpu_matrix_rotate(&matrix, rotation);
        jlgpu_matrix_translate(&matrix, dsc->pivot.x, dsc->pivot.y);
    }

    draw_img_params.surface2image = &matrix;
    jlgpu_tex_inter_params_t tex_inter_params = {0};
    tex_inter_params.inter_mode = JLGPU_SAMPLE_MODE_NEAR;
    draw_img_params.tex_inter_params = &tex_inter_params;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        int task_len = jlgpu_draw_img_task_len(u->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
        uint8_t *task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
        if (task == NULL) {
            /* 池和堆都申请失败:这里如果直接往下走就是 lv_memset(NULL, 0, len) 踩空,
             * 表现为无任何打印直接黑屏,所以必须判空 */
            LV_LOG_ERROR("letter task malloc fail");
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(u->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_img(u->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
        lv_jlgpu_add_task_node(u, task);
#endif
    } else {//layer/canvas需要立即绘制

        lv_jlgpu_ctx_t jlgpu_ctx = {0};
        lv_jlgpu_ctx_init(&jlgpu_ctx);

        int task_len = jlgpu_draw_img_task_len(jlgpu_ctx.ctx, &draw_img_params, &mask_params);
        uint8_t *task = lv_malloc(task_len);
        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
        jlgpu_draw_img(jlgpu_ctx.ctx, &render_params, &draw_img_params, &mask_params);
        jlgpu_surface_t surface = {0};
        jlgpu_global_matrix_t global_matrix = {0};
        jlgpu_colorkey_t colorkey = {0};
        jlgpu_global_path_t global_path = {0};
        jlgpu_uv_adr_t uv_adr = {0};

        int edge_num = 8;
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
        surface.window.x_min = 0;
        surface.window.x_max = fb_width;
        surface.window.y_min = 0;
        surface.window.y_max = fb_height;
        surface.format = out_format;
        surface.width = fb_width;

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

#if LV_USE_FREETYPE

static void draw_letter_outline(lv_draw_task_t *t, const lv_draw_glyph_dsc_t *dsc)
{
    /* get clip area */
    lv_draw_jlgpu_unit_t *u = (lv_draw_jlgpu_unit_t *)t->draw_unit;
    lv_area_t path_clip_area;
    if (!lv_area_intersect(&path_clip_area, &t->clip_area, dsc->letter_coords)) {
        return;
    }

    /*LV_PROFILER_DRAW_BEGIN;*/
    path_clip_area.x2++;
    path_clip_area.y2++;

    const lv_point_t pos = {dsc->letter_coords->x1, dsc->letter_coords->y1};
    /* scale size */
    float scale = FT_F26DOT6_TO_PATH_SCALE(lv_freetype_outline_get_scale(dsc->g->resolved_font)) * (1 << 3); // 处理矢量字库坐标被放太大问题

    bool is_rotated = false;
    float z_angle_rad = 0;

#if LV_USE_DRAW_JLGPU_LIST
    is_rotated = (dsc->rotation % 3600 != 0 || dsc->x_angle % 3600 != 0 || dsc->y_angle % 3600 != 0);
    z_angle_rad = (dsc->rotation / 10.0f) * 3.1415926f / 180.0f;//转换成弧度
#endif

    /* calc convert matrix */
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    if (!is_rotated) {
        jlgpu_matrix_scale(&matrix, scale, scale);
        jlgpu_matrix_translate(&matrix, pos.x - dsc->g->ofs_x, pos.y + dsc->g->box_h + dsc->g->ofs_y);
    } else {
        jlgpu_matrix_scale(&matrix, scale, scale);
        jlgpu_matrix_translate(&matrix, -dsc->pivot.x, 0);

        jlgpu_matrix_t flipx_mat, flipy_mat, marix_ori;
        memcpy(&marix_ori, &matrix, sizeof(jlgpu_matrix_t));

#if LV_USE_DRAW_JLGPU_LIST
        if (dsc->x_angle != 0) {
            float x_angle_rad = (dsc->x_angle / 10.0f) * M_PI / 180.0f;
            jlgpu_get_flipx_matrix(&flipx_mat, x_angle_rad);
            jlgpu_matrix_multiply(&matrix, &flipx_mat, &marix_ori);
            memcpy(&marix_ori, &matrix, sizeof(jlgpu_matrix_t));
        }
        if (dsc->y_angle != 0) {
            float y_angle_rad = (dsc->y_angle / 10.0f) * M_PI / 180.0f;
            jlgpu_get_flipy_matrix(&flipy_mat, y_angle_rad);
            jlgpu_matrix_multiply(&matrix, &flipy_mat, &marix_ori);
            memcpy(&marix_ori, &matrix, sizeof(jlgpu_matrix_t));
        }
#endif
        // 绕Z轴旋转
        jlgpu_matrix_rotate(&matrix, z_angle_rad);

        jlgpu_matrix_translate(&matrix, pos.x - dsc->g->ofs_x + dsc->pivot.x, pos.y + dsc->g->box_h + dsc->g->ofs_y);
    }

    lv_area_t blend_area = path_clip_area;

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    jlgpu_vec_t *outline = (jlgpu_vec_t *)dsc->glyph_data;

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

    draw_fill_params.color.b = dsc->color.blue;
    draw_fill_params.color.g = dsc->color.green;
    draw_fill_params.color.r = dsc->color.red;
    draw_fill_params.color.a = dsc->opa;

    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    draw_fill_params.paint_info.pt_clip.x_min = 0;
    draw_fill_params.paint_info.pt_clip.x_max = 4095;
    draw_fill_params.paint_info.pt_clip.y_min = 0;
    draw_fill_params.paint_info.pt_clip.y_max = 4095;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = jlgpu_compute_bbox(&matrix, &outline->path_bbox, fb_width, fb_height);

    /*printf("before  blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", render_params.render_area.x_min, render_params.render_area.y_min, render_params.render_area.x_max, render_params.render_area.y_max);*/

    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    int edge_num = outline->edge_num;
    jlgpu_path.seg_len = outline->total_len;
    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        jlgpu_path.seg_data = lv_jlgpu_attach_buf_list_add(u, outline->base, outline->total_len);
#endif
    } else {
        jlgpu_path.seg_data = outline->base;
    }
    jlgpu_path.fill_rule = JLGPU_NON_ZERO;
    jlgpu_path.max_bez_div = outline->bez_div;
    jlgpu_path.user2surface = &matrix;
    jlgpu_path.path_trans_mode = compute_jlgpu_trans_mode(&matrix);

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        int task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
        uint8_t *task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
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

        jlgpu_dump_task(task);

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
        global_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;
        global_path.cbuf_stride = fb_width;
        global_path.max_bez_div = global_max_bez_div;
        global_path.max_edge_cfg = edge_num;

        int mask_h = global_path.cbuf_split ? 16 : 32; //8 + global_path.cbuf_split * 4;
        uint8_t *cbuf = (uint8_t *)lv_malloc((mask_h * fb_width));
        lv_memset(cbuf, 0, (mask_h * fb_width));

        uint8_t *ebuf = (uint8_t *)lv_malloc(8 * edge_num); //8*edge_num
        lv_memset(ebuf, 0, 8 * edge_num);//8*edge_num

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

static void jlgpu_outline_push(const lv_freetype_outline_event_param_t *param)
{
    LV_PROFILER_DRAW_BEGIN;
    jlgpu_vec_t *outline = param->outline;

    LV_ASSERT_NULL(outline);

    lv_freetype_outline_type_t type = param->type;
    switch (type) {
    /**
     * Reverse the Y-axis coordinate direction to achieve
     * the conversion from Cartesian coordinate system to LCD coordinate system
     */
    case LV_FREETYPE_OUTLINE_END:
        jlgpu_path_end(outline);
        break;
    case LV_FREETYPE_OUTLINE_MOVE_TO:
        jlgpu_path_move_to(outline, param->to.x, -param->to.y);
        break;
    case LV_FREETYPE_OUTLINE_LINE_TO:
        jlgpu_path_line_to(outline, param->to.x, -param->to.y);
        break;
    case LV_FREETYPE_OUTLINE_CUBIC_TO:
        jlgpu_path_cubic_to(outline, param->control1.x, -param->control1.y,
                            param->control2.x, -param->control2.y,
                            param->to.x, -param->to.y);
        break;
    case LV_FREETYPE_OUTLINE_CONIC_TO:
        jlgpu_path_quad_to(outline, param->control1.x, -param->control1.y,
                           param->to.x, -param->to.y);
        break;
    default:
        LV_LOG_ERROR("unknown point type: %d", type);
        LV_ASSERT(false);
        break;
    }
    LV_PROFILER_DRAW_END;

}

static void freetype_outline_event_cb(lv_event_t *e)
{

    LV_PROFILER_DRAW_BEGIN;
    lv_event_code_t code = lv_event_get_code(e);
    lv_freetype_outline_event_param_t *param = lv_event_get_param(e);
    switch (code) {
    case LV_EVENT_CREATE:
        //创建path
        param->outline = jlgpu_path_create();//配置了默认长度和默认div
        break;
    case LV_EVENT_DELETE:
        //释放path
        jlgpu_path_destroy(param->outline);
        break;
    case LV_EVENT_INSERT:
        //生成path
        jlgpu_outline_push(param);
        break;
    default:
        LV_LOG_WARN("unknown event code: %d", code);
        break;
    }
    LV_PROFILER_DRAW_END;
}

#endif /* LV_USE_FREETYPE */





#endif /*LV_USE_DRAW_JLGPU_LABEL_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
