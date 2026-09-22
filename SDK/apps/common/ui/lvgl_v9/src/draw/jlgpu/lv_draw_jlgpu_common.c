/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "jlgpu_utils.h"
#include "../../misc/lv_area_private.h"

#include "ui/lcd/lcd_drive.h"
#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE


#if LV_USE_DRAW_JLGPU

//#define LOG_TAG_CONST       UI
#define LOG_TAG             "[lv_gpu]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#include "asm/psram_api.h"
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
uint8_t lv_jlgpu_get_color_format(lv_color_format_t lv_cf)
{
    /*printf("lv_cf  = 0x%02x\n", lv_cf);*/

    uint8_t jlgpu_cf = 0xff;

    switch (lv_cf) {
    case LV_COLOR_FORMAT_ARGB8888:
        jlgpu_cf = JLGPU_FORMAT_ARGB8888;
        break;
    case LV_COLOR_FORMAT_ARGB8565:
        jlgpu_cf = JLGPU_FORMAT_ARGB8565;
        break;
    case LV_COLOR_FORMAT_RGB888:
        jlgpu_cf = JLGPU_FORMAT_RGB888;
        break;
    case LV_COLOR_FORMAT_RGB565:
        jlgpu_cf = JLGPU_FORMAT_RGB565;
        break;
    case LV_COLOR_FORMAT_A8:
        jlgpu_cf = JLGPU_FORMAT_A8;
        break;
    case LV_COLOR_FORMAT_A4:
        jlgpu_cf = JLGPU_FORMAT_A4;
        break;
    case LV_COLOR_FORMAT_A2:
        jlgpu_cf = JLGPU_FORMAT_A2;
        break;
    case LV_COLOR_FORMAT_A1:
        jlgpu_cf = JLGPU_FORMAT_A1;
        break;
    case LV_COLOR_FORMAT_I8:
        jlgpu_cf = JLGPU_FORMAT_L8;
        break;
    case LV_COLOR_FORMAT_I4:
        jlgpu_cf = JLGPU_FORMAT_L4;
        break;
    case LV_COLOR_FORMAT_I2:
        jlgpu_cf = JLGPU_FORMAT_L2;
        break;
    case LV_COLOR_FORMAT_I1:
        jlgpu_cf = JLGPU_FORMAT_L1;
        break;
    case LV_COLOR_FORMAT_YUY2:
        jlgpu_cf = JLGPU_FORMAT_YUV422_BT601;
        break;
    case LV_COLOR_FORMAT_NV12:
        jlgpu_cf = JLGPU_FORMAT_YUV420_BT601;
        break;
    default:
        //printf("Unsupported color format.");
        break;
    }

    /*printf("jlgpu_cf  = 0x%02x\n", jlgpu_cf);*/
    return jlgpu_cf;
}

uint8_t lv_jlgpu_get_blend_mode(lv_blend_mode_t lv_blend_mode, bool *mode_flag)
{

    uint8_t jlgpu_blend_mode = JLGPU_BLEND_SRC;

    if (mode_flag != NULL) {
        *mode_flag = true;   // false : 不支持：true : 支持
    }

    //printf("lv_blend_mode 0x%02x.", lv_blend_mode);
    switch (lv_blend_mode) {
    case LV_BLEND_MODE_NORMAL:
        jlgpu_blend_mode = JLGPU_BLEND_SRC_OVER;
        break;
    case LV_BLEND_MODE_ADDITIVE:
        jlgpu_blend_mode = JLGPU_BLEND_ADDITIVE;
        break;
    case LV_BLEND_MODE_SUBTRACTIVE:
        jlgpu_blend_mode = JLGPU_BLEND_SUBTRACTIVE;
        break;
    case LV_BLEND_MODE_MULTIPLY:
        jlgpu_blend_mode = JLGPU_BLEND_MULTIPLY;
        break;
    case LV_BLEND_MODE_DIFFERENCE:
        jlgpu_blend_mode = JLGPU_BLEND_SUBTRACTIVE;//fixme
        break;
    default:
        if (mode_flag != NULL) {
            *mode_flag = false;   // false : 不支持：true : 支持
        }
        //printf("Unsupported blend mode.");
        break;
    }
    //printf("jlgpu_blend_mode  = 0x%02x.", jlgpu_blend_mode);
    return jlgpu_blend_mode;

}

void lv_jlgpu_flush_inv_dcache(uint8_t *buf, uint32_t w, uint32_t h, uint32_t stride, uint8_t color_depth)
{

#if 0
    /* 区域刷新Cache */
    uint8_t *area_ptr = (uint8_t *)buf;
    uint32_t width_bytes = w * color_depth >> 3;
    uint32_t offset = stride * color_depth >> 3;
    for (int y = 0; y < h; y++) {
        DcuFlushinvRegion((u32 *)area_ptr, width_bytes);
        area_ptr += offset;
    }
#endif

}

// Helper function to calculate the angle between two vectors
static float __vecang(float ux, float uy, float vx, float vy)
{
    float r = (float)((ux * vx + uy * vy) / ((float)sqrtf(ux * ux + uy * uy) * (float)sqrtf(vx * vx + vy * vy)));
    if (r < -1.0f) {
        r = -1.0f;
    }
    if (r > 1.0f) {
        r = 1.0f;
    }
    return ((ux * vy < uy * vx) ? -1.0f : 1.0f) * acosf(r);
}

void lv_jlgpu_path_circle2bezier(uint8_t *segments, uint32_t *nseg, float *data, uint32_t *ndata,
                                 float cx, float cy, float r, float sx, float sy, float ex, float ey, uint8_t cw, uint8_t half)
{

    float x1, y1, x2, y2;
    float a1, da, hda, kappa;
    int ndivs;
    float x, y, tanx, tany, px = 0, py = 0, ptanx = 0, ptany = 0;

    *nseg = 0;
    *ndata = 0;

    // Start point
    x1 = sx;
    y1 = sy;
    x2 = ex;
    y2 = ey;

    if (half) {
        // Handle half circle case
        float ux = x1 - cx;
        float uy = y1 - cy;
        float vx = x2 - cx;
        float vy = y2 - cy;

        hda = JLGPU_PI / 4.0f;
        hda = (1.0f - cosf(hda)) / sinf(hda);
        kappa = fabsf(4.0f / 3.0f * hda);

        px = x1;
        py = y1;
        if (cw) {
            x = cx + uy;
            y = cy - ux;
            ptanx = uy * kappa;
            ptany = -ux * kappa;
            tanx = -ux * kappa;
            tany = -uy * kappa;
        } else {
            x = cx - uy;
            y = cy + ux;
            ptanx = -uy * kappa;
            ptany = ux * kappa;
            tanx = -ux * kappa;
            tany = -uy * kappa;
        }

        segments[(*nseg)++] = JLGPU_CUBIC_TO;
        data[(*ndata)++] = px + ptanx;
        data[(*ndata)++] = py + ptany;
        data[(*ndata)++] = x - tanx;
        data[(*ndata)++] = y - tany;
        data[(*ndata)++] = x;
        data[(*ndata)++] = y;

        float tanx_t = ptanx;
        float tany_t = ptany;

        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
        x = x2;
        y = y2;

        tanx = -tanx_t;
        tany = -tany_t;

        segments[(*nseg)++] = JLGPU_CUBIC_TO;
        data[(*ndata)++] = px + ptanx;
        data[(*ndata)++] = py + ptany;
        data[(*ndata)++] = x - tanx;
        data[(*ndata)++] = y - tany;
        data[(*ndata)++] = x;
        data[(*ndata)++] = y;
    } else {
        // Calculate initial and delta angles
        float ux = x1 - cx;
        float uy = y1 - cy;
        float vx = x2 - cx;
        float vy = y2 - cy;

        a1 = __vecang(r, 0.0f, ux, uy);  // Initial angle

        if (!cw) {



            da = __vecang(ux, uy, vx, vy);  // Delta angle
        } else {
            da = -__vecang(vx, vy, ux, uy);  // Delta angle
        }


        // Split arc into max 90 degree segments
        ndivs = (int)ceilf(fabsf(da) / (JLGPU_PI * 0.5f)) + 1;
        hda = (da / (float)ndivs) / 2.0f;
        // Avoid division by zero
        if (fabsf(hda) < 1e-6f) {
            hda = 1e-6f;
        }
        hda = (1.0f - cosf(hda)) / sinf(hda);
        kappa = fabsf(4.0f / 3.0f * hda);
        if (da < 0.0f) {
            kappa = -kappa;
        }

        for (int i = 0; i <= ndivs; i++) {
            float a = a1 + da * ((float)i / (float)ndivs);
            float dx = cosf(a);
            float dy = sinf(a);
            x = dx * r + cx;
            y = dy * r + cy;
            tanx = -dy * r * kappa;
            tany = dx * r * kappa;

            if (i == ndivs) {
                x = x2;
                y = y2;
            }

            if (i > 0) {

                segments[(*nseg)++] = JLGPU_CUBIC_TO;
                data[(*ndata)++] = px + ptanx;
                data[(*ndata)++] = py + ptany;
                data[(*ndata)++] = x - tanx;
                data[(*ndata)++] = y - tany;
                data[(*ndata)++] = x;
                data[(*ndata)++] = y;

            }
            px = x;
            py = y;
            ptanx = tanx;
            ptany = tany;
        }
    }





}



/**
 * 创建圆角矩形的vg路径数据。
 * @param area 区域坐标
 * @param radius 圆角半径
 * @param op_buf 输出的操作符缓冲区
 * @param coord_buf 输出的坐标缓冲区
 * @param op_size 输出的路径操作数量
 * @param coord_size 输出的路径坐标数量
 */
void create_rounded_rect_jlgpu_path(lv_area_t *coords, uint16_t final_radius,
                                    uint8_t *op_buf, float *coord_buf, int *op_size, int *coord_size)
{
    if (!(op_buf) || !(coord_buf)) {
        return;
    }


#if 0
    /* Path data element index */
    uint8_t op_idx = 0;
    uint32_t coord_idx = 0;

    if (final_radius > 0) {
        /* Get the control point offset for rounded cases */
        int32_t cpoff = (int32_t)((float)final_radius * KAPPA);

        /* Rounded rectangle case */
        /* Starting point */
        op_buf[op_idx++] = VGHW_MOVE_TO;
        coord_buf[coord_idx++] = coords->x1 + final_radius;  // 起始点 x
        coord_buf[coord_idx++] = coords->y1;                // 起始点 y

        /* Top side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x2 - final_radius + 1;  // Extended for VGLite
        coord_buf[coord_idx++] = coords->y1;

        /* Top-right corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x2 - final_radius + cpoff;  // 控制点1 x
        coord_buf[coord_idx++] = coords->y1;                        // 控制点1 y
        coord_buf[coord_idx++] = coords->x2;                        // 控制点2 x
        coord_buf[coord_idx++] = coords->y1 + final_radius - cpoff; // 控制点2 y
        coord_buf[coord_idx++] = coords->x2;                        // 终点 x
        coord_buf[coord_idx++] = coords->y1 + final_radius;         // 终点 y

        /* Right side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x2 + 1;                     // Extended for VGLite
        coord_buf[coord_idx++] = coords->y2 - final_radius + 1;     // Extended for VGLite

        /* Bottom-right corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x2;                        // 控制点1 x
        coord_buf[coord_idx++] = coords->y2 - final_radius + cpoff; // 控制点1 y
        coord_buf[coord_idx++] = coords->x2 - final_radius + cpoff; // 控制点2 x
        coord_buf[coord_idx++] = coords->y2;                        // 控制点2 y
        coord_buf[coord_idx++] = coords->x2 - final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y2;                        // 终点 y

        /* Bottom side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x1 + final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y2 + 1;                    // Extended for VGLite

        /* Bottom-left corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x1 + final_radius - cpoff; // 控制点1 x
        coord_buf[coord_idx++] = coords->y2;                        // 控制点1 y
        coord_buf[coord_idx++] = coords->x1;                        // 控制点2 x
        coord_buf[coord_idx++] = coords->y2 - final_radius + cpoff; // 控制点2 y
        coord_buf[coord_idx++] = coords->x1;                        // 终点 x
        coord_buf[coord_idx++] = coords->y2 - final_radius;         // 终点 y

        /* Left side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x1;                        // 终点 x
        coord_buf[coord_idx++] = coords->y1 + final_radius;         // 终点 y

        /* Top-left corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x1;                        // 控制点1 x
        coord_buf[coord_idx++] = coords->y1 + final_radius - cpoff; // 控制点1 y
        coord_buf[coord_idx++] = coords->x1 + final_radius - cpoff; // 控制点2 x
        coord_buf[coord_idx++] = coords->y1;                        // 控制点2 y
        coord_buf[coord_idx++] = coords->x1 + final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y1;                        // 终点 y
    }

    /* Resulting path size */
    *op_size = op_idx;
    *coord_size = coord_idx;

#endif

}

lv_result_t lv_draw_jlgpu_premult_and_swizzle(uint32_t *dest, lv_color_t src, lv_opa_t opa)
{
    if (dest == NULL) {
        return LV_RESULT_INVALID;
    }

    // 由于lv_color_t没有alpha，默认alpha为255（完全不透明）
    uint32_t alpha = (uint32_t)opa;  // opa本身就是0~255范围的不透明度

    // 预乘RGB通道
    uint32_t red = ((uint32_t)src.red * alpha) / 255;
    uint32_t green = ((uint32_t)src.green * alpha) / 255;
    uint32_t blue = ((uint32_t)src.blue * alpha) / 255;

    // 按ARGB格式打包
    *dest = (alpha << 24) | (red << 16) | (green << 8) | blue;

    return LV_RESULT_OK;
}

jlgpu_trans_mode_t compute_jlgpu_trans_mode(jlgpu_matrix_t *matrix)
{
    jlgpu_trans_mode_t trans_mode = JLGPU_TRANS_MODE_NONE;

    if (!matrix) {
        return trans_mode;
    }

    if (matrix->m[2][0] == 0.f && matrix->m[2][1] == 0.f && matrix->m[2][2] == 1.f) {
        trans_mode = JLGPU_TRANS_MODE_AFFINE;
    } else {
        trans_mode = JLGPU_TRANS_MODE_PROJECTION;
    }

    printf("compute_jlgpu_trans_mode = %d\n", trans_mode);

    return trans_mode;
}

void lv_jlgpu_draw_buf_copy(lv_draw_buf_t *dest, const lv_area_t *dest_area, const lv_draw_buf_t *src, const lv_area_t *src_area, int use_gpu_list)
{
    uint8_t *dest_bufc;
    uint8_t *src_bufc;
    int32_t line_width;

    /*Source and dest color format must be same. Color conversion is not supported yet.*/
    LV_ASSERT_FORMAT_MSG(dest->header.cf == src->header.cf, "Color format mismatch: %d != %d",
                         dest->header.cf, src->header.cf);

    if (dest_area == NULL) {
        line_width = dest->header.w;
    } else {
        line_width = lv_area_get_width(dest_area);
    }

    /* For indexed image, copy the palette if we are copying full image area*/
    if (dest_area == NULL || src_area == NULL) {
        if (LV_COLOR_FORMAT_IS_INDEXED(dest->header.cf)) {
            lv_memcpy(dest->data, src->data, LV_COLOR_INDEXED_PALETTE_SIZE(dest->header.cf) * sizeof(lv_color32_t));
        }
    }

    /*Check source and dest area have same width*/
    if ((src_area == NULL && line_width != src->header.w) || \
        (src_area != NULL && line_width != lv_area_get_width(src_area))) {
        LV_ASSERT_MSG(0, "Source and destination areas have different width");
        LV_PROFILER_DRAW_END;
        return;
    }

    extern struct lcd_platform_data *lcd_get_platform_data();
    struct lcd_platform_data *lcd_pd = lcd_get_platform_data();
    uint8_t jstc_en = lcd_pd->dbi_hw.jstc_en;

    //dest
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf = dest->data;
    lv_color_format_t dest_cf = dest->header.cf;
    fb_width  = dest->header.w;
    fb_height = dest->header.h;
    uint8_t out_format = lv_jlgpu_get_color_format(dest_cf);

    //src
    uint32_t src_width  = src->header.w;
    uint32_t src_height = src->header.h;
    uint32_t src_stride = src->header.stride;
    uint8_t  src_bpp    = lv_color_format_get_bpp(src->header.cf);
    uint8_t  in_format  = lv_jlgpu_get_color_format(src->header.cf);
    uint8_t *img_buf    = src->data;

    uint8_t global_max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    jlgpu_draw_img_t draw_img_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_path_t	jlgpu_path = {0};

    //绘制(复制)在dest的位置
    jlgpu_area_t render_area;
    render_area.x_min = dest_area->x1;
    render_area.x_max = dest_area->x2 + 1;
    render_area.y_min = dest_area->y1;
    render_area.y_max = dest_area->y2 + 1;

    //src根据src_area裁剪
    jlgpu_area_t tex_clip_area;
    tex_clip_area.x_min = src_area->x1;
    tex_clip_area.x_max = src_area->x2 + 1;
    tex_clip_area.y_min = src_area->y1;
    tex_clip_area.y_max = src_area->y2 + 1;

    jlgpu_img_info_t img_info = {0};

    img_info.premult = 1;
    img_info.tex_clip = tex_clip_area;
    uint8_t adr_mode = JLGPU_ADRMODE_LINE;
    uint8_t *src_clut = NULL;
    uint16_t clut_format = JLGPU_FORMAT_ARGB8888;

    if (jstc_en) {
        img_info.adr_mode = JLGPU_ADRMODE_TILE;
        img_info.color_format = JLGPU_FORMAT_RGB888;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_JSTC;
    } else {
        img_info.adr_mode = adr_mode;
        img_info.color_format = in_format;
        img_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
    }

    img_info.data = (uint8_t *)NO_CACHE_ADDR(img_buf);
    if ((src_bpp >> 3) != 0) {
        img_info.width = src_stride / (src_bpp >> 3);
    } else {
        img_info.width = src_width;
    }
    img_info.height = src_height;
    img_info.compress_size = 0;

    if (img_info.color_format == JLGPU_FORMAT_YUV422_BT601) {
        img_info.rb_swap  = JLGPU_RBS_ABGR;
    }

    draw_img_params.clut_format = clut_format;
    draw_img_params.clut_adr = src_clut;

    draw_img_params.tex_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_img_params.img_info = img_info;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area = render_area;
    render_params.blend_mode = JLGPU_BLEND_SRC;
    render_params.render_quality = JLGPU_RENDERING_QUALITY_NONANTIALIASED;//jlgpu_render_quality_t
    render_params.dither_en = 0;
    render_params.global_alpha = 128;

    int edge_num = 0;
    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    if (LV_USE_DRAW_JLGPU_LIST && use_gpu_list) {
#if LV_USE_DRAW_JLGPU_LIST
        lv_draw_jlgpu_unit_t *unit = lv_get_draw_jlgpu_unit();
        int task_len = jlgpu_draw_img_task_len(unit->jlgpu_ctx->ctx, &draw_img_params, &mask_params);
        uint8_t *task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("draw_img task malloc fail");
            return;
        }
        lv_memset(task, 0, task_len);
        jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_img(unit->jlgpu_ctx->ctx, &render_params, &draw_img_params, &mask_params);
        lv_jlgpu_add_task_node(unit, task);
#endif
    } else {
        lv_jlgpu_ctx_t jlgpu_ctx = {0};
        lv_jlgpu_ctx_init(&jlgpu_ctx);

        int task_len = jlgpu_draw_img_task_len(jlgpu_ctx.ctx, &draw_img_params, &mask_params);
        uint8_t task[100] = {0};

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);

        jlgpu_draw_img(jlgpu_ctx.ctx, &render_params, &draw_img_params, &mask_params);

        /*jlgpu_dump_task(task);*/

        jlgpu_surface_t surface = {0};
        jlgpu_global_matrix_t global_matrix = {0};
        jlgpu_colorkey_t colorkey = {0};
        jlgpu_global_path_t global_path = {0};
        jlgpu_uv_adr_t uv_adr = {0};

        surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);
        surface.format = out_format;
        surface.window.x_min = 0;
        surface.window.x_max = fb_width;
        surface.window.y_min = 0;
        surface.window.y_max = fb_height;
        surface.width = fb_width;
        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, &global_path, 0);
        jlgpu_run(jlgpu_ctx.ctx, task);

        lv_jlgpu_ctx_uninit(&jlgpu_ctx);
    }
}

void lv_jlgpu_draw_buf_clear(lv_draw_buf_t *draw_buf, const lv_area_t *a)
{
    LV_ASSERT_NULL(draw_buf);
    LV_PROFILER_DRAW_BEGIN;

#if LV_USE_DRAW_JLGPU_LIST
    lv_draw_jlgpu_unit_t *unit = lv_get_draw_jlgpu_unit();
#endif

    const lv_image_header_t *header = &draw_buf->header;
    uint8_t *dest_buf = draw_buf->data;

    bool is_disp_buf = lv_is_draw_buf(dest_buf);

    uint8_t out_format = lv_jlgpu_get_color_format(header->cf);
    int out_bpp = jlgpu_get_format_bpp(out_format);

    int out_width  = header->w;
    int out_height = header->h;
    int out_stride = header->stride;

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};

    draw_fill_params.color.b = 0;
    draw_fill_params.color.g = 0;
    draw_fill_params.color.r = 0;
    draw_fill_params.color.a = 0;
    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    render_params.layer_en = 1;
    render_params.blend_mode = JLGPU_BLEND_SRC;
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    jlgpu_surface_t surface = {0};
    surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);
    surface.window.x_min = 0;
    surface.window.x_max = out_width;
    surface.window.y_min = 0;
    surface.window.y_max = out_height;
    surface.format = out_format;
    surface.width  = out_width;

    if (a == NULL) {
        draw_fill_params.paint_info.pt_clip.x_min = 0;
        draw_fill_params.paint_info.pt_clip.x_max = out_width;
        draw_fill_params.paint_info.pt_clip.y_min = 0;
        draw_fill_params.paint_info.pt_clip.y_max = out_height;

        render_params.render_area.x_min = 0;
        render_params.render_area.x_max = out_width;
        render_params.render_area.y_min = 0;
        render_params.render_area.y_max = out_height;

        if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
            uint8_t *task = NULL;
            int task_len = jlgpu_draw_fill_task_len(unit->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
            task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
            if (task == NULL) {
                LV_LOG_ERROR("malloc fail");
                return;
            }
            lv_memset(task, 0, task_len);

            jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
            jlgpu_draw_fill(unit->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
            lv_jlgpu_add_task_node(unit, task);
#endif
        } else {

            lv_jlgpu_ctx_t jlgpu_ctx;
            lv_jlgpu_ctx_init(&jlgpu_ctx);
            uint8_t *task = NULL;
            int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
            task = lv_malloc(task_len);
            lv_memset(task, 0, task_len);

            jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
            jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);
            jlgpu_init(jlgpu_ctx.ctx);
            jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, 0, 0);
            jlgpu_run(jlgpu_ctx.ctx, task);

            if (task) {
                lv_free(task);
            }
            lv_jlgpu_ctx_uninit(&jlgpu_ctx);

            lv_draw_buf_flush_cache(draw_buf, a);
        }

        LV_PROFILER_DRAW_END;
        return;
    }

    lv_area_t a_draw_buf;
    a_draw_buf.x1 = 0;
    a_draw_buf.y1 = 0;
    a_draw_buf.x2 = draw_buf->header.w - 1;
    a_draw_buf.y2 = draw_buf->header.h - 1;

    lv_area_t a_clipped;
    if (!lv_area_intersect(&a_clipped, a, &a_draw_buf)) {
        LV_PROFILER_DRAW_END;
        return;
    }

    if (lv_area_get_width(&a_clipped) <= 0) {
        LV_PROFILER_DRAW_END;
        return;
    }

    if (lv_area_get_height(&a_clipped) <= 0) {
        LV_PROFILER_DRAW_END;
        return;
    }

    draw_fill_params.paint_info.pt_clip.x_min = a_clipped.x1;
    draw_fill_params.paint_info.pt_clip.x_max = a_clipped.x2 + 1;
    draw_fill_params.paint_info.pt_clip.y_min = a_clipped.y1;
    draw_fill_params.paint_info.pt_clip.y_max = a_clipped.y2 + 1;

    render_params.render_area.x_min = a_clipped.x1;
    render_params.render_area.x_max = a_clipped.x2 + 1;
    render_params.render_area.y_min = a_clipped.y1;
    render_params.render_area.y_max = a_clipped.y2 + 1;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) {
#if LV_USE_DRAW_JLGPU_LIST
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_fill_task_len(unit->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
        task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
        jlgpu_draw_fill(unit->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
        lv_jlgpu_add_task_node(unit, task);
#endif
    } else {
        lv_jlgpu_ctx_t jlgpu_ctx;
        lv_jlgpu_ctx_init(&jlgpu_ctx);
        uint8_t *task = NULL;
        int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
        task = lv_malloc(task_len);
        lv_memset(task, 0, task_len);

        jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
        jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);

        jlgpu_init(jlgpu_ctx.ctx);
        jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, 0, 0);
        jlgpu_run(jlgpu_ctx.ctx, task);

        if (task) {
            lv_free(task);
        }
        lv_jlgpu_ctx_uninit(&jlgpu_ctx);

        lv_draw_buf_flush_cache(draw_buf, a);//handle不能为空
    }

    LV_PROFILER_DRAW_END;
}

void lv_jlgpu_clean_buf(lv_draw_jlgpu_unit_t *unit, uint8_t *draw_buf, uint8_t buf_format, const lv_area_t *a)
{
    uint8_t *dest_buf = draw_buf;
    uint8_t out_format = buf_format;

    int out_width  = lv_area_get_width(a);
    int out_height = lv_area_get_height(a);

    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};

    draw_fill_params.color.b = 0;
    draw_fill_params.color.g = 0;
    draw_fill_params.color.r = 0;
    draw_fill_params.color.a = 0;
    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    render_params.layer_en = 1;
    render_params.blend_mode = JLGPU_BLEND_SRC;
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    jlgpu_surface_t surface = {0};
    surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);
    surface.window.x_min = 0;
    surface.window.x_max = out_width;
    surface.window.y_min = 0;
    surface.window.y_max = out_height;
    surface.format = out_format;
    surface.width  = out_width;

    draw_fill_params.paint_info.pt_clip.x_min = 0;
    draw_fill_params.paint_info.pt_clip.x_max = out_width;
    draw_fill_params.paint_info.pt_clip.y_min = 0;
    draw_fill_params.paint_info.pt_clip.y_max = out_height;

    render_params.render_area.x_min = 0;
    render_params.render_area.x_max = out_width;
    render_params.render_area.y_min = 0;
    render_params.render_area.y_max = out_height;


    lv_jlgpu_ctx_t jlgpu_ctx;
    lv_jlgpu_ctx_init(&jlgpu_ctx);
    uint8_t *task = NULL;
    int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, &draw_fill_params, &mask_params);
    task = lv_malloc(task_len);
    lv_memset(task, 0, task_len);

    jlgpu_set_task(jlgpu_ctx.ctx, task, task_len);
    jlgpu_draw_fill(jlgpu_ctx.ctx, &render_params, &draw_fill_params, &mask_params);
    jlgpu_init(jlgpu_ctx.ctx);
    jlgpu_set_params(jlgpu_ctx.ctx, &surface, 0, 0, 0, 0);
    jlgpu_run(jlgpu_ctx.ctx, task);

    if (task) {
        lv_free(task);
    }
    lv_jlgpu_ctx_uninit(&jlgpu_ctx);

    /* GPU写完后无效化DCache，确保CPU后续读取时拿到GPU写入的数据 */
    uint32_t clean_size = out_width * out_height * jlgpu_get_format_bpp(out_format) / 8;
    //检查是否是psram内存，释放psram
    if (psram_cache_check((void *)(draw_buf))) {
        psram_flush_invaild_cache((void *)draw_buf, clean_size);
    }
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
