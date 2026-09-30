/**
 * @file    jlgpu_image_filters.c
 * @brief   GPU 图像滤镜特效实现
 *
 * 基于 JLGPU 硬件实现常用图像滤镜。所有滤镜遵循统一模式:
 *   init → gen_task → exec_task → free
 */

#include "lv_draw_jlgpu.h"
#include "jieli/extra/libs/jl_bin/lv_jl_bin.h"
#include "jieli/extra/libs/jl_c3/jl_c3.h"
#include "fs/fs.h"
#include "../../misc/lv_area_private.h"
#include "../../core/lv_refr_private.h"
#include "jlgpu_utils.h"
#include "gxdma_driver.h"
#include "jlgpu_image_filters.h"
#include "asm/dcache.h"
#include "jlgpu_utils.h"

#include "draw/jlgpu/lv_draw_jlgpu.h"
#include "asm/psram_api.h"
#include "app_config.h"
#include "lv_port_disp.h"

extern void dbi_link_gpu(u8 en);

#if  LV_USE_DRAW_JLGPU_LIST
/* ========================================================================
 *  内部函数
 * ======================================================================== */

struct jlgpu_filter_session {
    lv_jlgpu_ctx_t  ctx;            // GPU context
    int             w, h;           // 固定尺寸
    int             src_format;     // 固定输入格式
    int             dst_format;     // 固定输出格式
    uint8_t        *task_buf;       // 预分配 GPU 命令缓冲
    int             task_buf_size;  // 缓冲区大小
    uint8_t        *pp_buf0;        // GXDMA 乒乓缓冲
    uint8_t        *pp_buf1;

    /* 分行处理配置 */
    int                     line_batch_count;   // 每批处理行数 (0=不分行)
    jlgpu_filter_line_cb_t  line_cb;            // 分行回调
    void                   *line_cb_data;       // 回调用户数据
};

/** 创建矩形区域 [0, w) × [0, h) */
static inline jlgpu_area_t make_area(int16_t x, int16_t y, int16_t w, int16_t h)
{
    jlgpu_area_t a;
    a.x_min = x;
    a.x_max = w;
    a.y_min = y;
    a.y_max = h;
    return a;
}

static inline u8 is_yuv_format(int format)
{
    return (format == JLGPU_FORMAT_YUV420_BT601 ||
            format == JLGPU_FORMAT_YUV420_BT709 ||
            format == JLGPU_FORMAT_YUV422_BT601 ||
            format == JLGPU_FORMAT_YUV422_BT709);
}

static inline u8 is_need_gxdma(int format)
{
    if (is_yuv_format(format)) {
        return 1;
    }
    return 0;
}

/** 初始化源图像信息 (YUV420 NV12 / YUV422 YUYV) */
static inline void init_img_src(jlgpu_img_info_t *info,
                                const uint8_t *data,
                                int w, int h,
                                jlgpu_area_t clip, int format)
{
    info->adr_mode      = JLGPU_ADRMODE_LINE;
    info->color_format  = format;
    info->data          = (uint8_t *)data;
    info->width         = w;
    info->height        = h;
    info->compress_mode = JLGPU_COMPRESS_MODE_NONE;
    info->premult       = JLGPU_PREMULT_TRUE;
    info->tex_clip      = clip;
}


/** 初始化源图像信息 (YUV420 NV12 / YUV422 YUYV) */
static inline void set_img_clip_area(jlgpu_img_info_t *info, jlgpu_area_t clip)
{
    info->tex_clip      = clip;
}

/** 获取 UV 平面地址 (仅 YUV420 需要独立 UV, YUV422 返回 NULL) */
static inline uint8_t *get_uv_ptr(const uint8_t *src, int w, int h, int format)
{
    if (format == JLGPU_FORMAT_YUV420_BT601 ||
        format == JLGPU_FORMAT_YUV420_BT709) {
        return (uint8_t *)src + w * h;
    }
    return NULL;
}

/** UV128: 仅 YUV420 支持 (UV 平面连续可用 memset) */
static inline int uv128_supported(int format)
{
    return (format == JLGPU_FORMAT_YUV420_BT601 ||
            format == JLGPU_FORMAT_YUV420_BT709);
}

/** 初始化 draw_img (no transform, nearest filter) */
static inline void init_draw_img(jlgpu_draw_img_t *img,
                                 jlgpu_img_info_t *info,
                                 jlgpu_tex_inter_params_t *inter)
{
    img->tex_trans_mode  = JLGPU_TRANS_MODE_NONE;
    img->surface2image   = 0;
    img->img_info        = *info;
    inter->inter_mode    = JLGPU_SAMPLE_MODE_NEAR;
    img->tex_inter_params = inter;
}

/** 初始化 draw_img transform */
static inline void set_draw_img_transform(jlgpu_draw_img_t *img,
        jlgpu_matrix_t *m)
{
    img->tex_trans_mode  = JLGPU_TRANS_MODE_AFFINE;
    img->surface2image   = m;
}

/** 初始化 draw_fill (白色, no transform) */
static inline void init_draw_fill_white(jlgpu_draw_fill_t *fill,
                                        jlgpu_area_t clip)
{
    fill->fill_trans_mode   = JLGPU_TRANS_MODE_NONE;
    fill->surface2paint     = 0;
    fill->paint_info.premult = JLGPU_PREMULT_TRUE;
    fill->paint_info.pt_clip = clip;
    fill->color.b = 255;
    fill->color.g = 255;
    fill->color.r = 255;
    fill->color.a = 255;
}

/** 初始化 render_params (layer enabled, SRC blend, given alpha) */
static inline void init_render_src(jlgpu_render_param_t *r,
                                   jlgpu_area_t area, uint8_t alpha)
{
    r->layer_en        = 1;
    r->render_area     = area;
    r->blend_mode      = JLGPU_BLEND_SRC;
    r->global_alpha    = alpha;
    r->render_quality  = JLGPU_RENDERING_QUALITY_MSAA_8X;
}

/** 初始化空 mask 参数 */
static inline void init_mask_none(jlgpu_mask_param_t *m)
{
    m->mask_mode = JLGPU_MASK_MODE_NONE;
    m->mask_ptr  = 0;
}

static inline jlgpu_err_t exec_task(jlgpu_t *ctx,
                                    jlgpu_surface_t *surf,
                                    jlgpu_uv_adr_t *uv,
                                    void *task)
{
    jlgpu_err_t err;
    jlgpu_set_params(ctx, surf, 0, 0, 0, uv);
    // jlgpu_dumcp_all_tasks(task);
    jlgpu_run(ctx, task);
    err = jlgpu_get_err(ctx);
    return err;
}

/** 初始化输出 surface */
static inline void init_surface(jlgpu_surface_t *surf,
                                uint8_t *dst, int w, int h,
                                jlgpu_area_t win, int format)
{
    surf->data   = dst;
    surf->window = win;
    surf->format = format;
    surf->width  = w;
    surf->rb_swap = is_yuv_format(format);
}

/** 确保 task_buf 足够大 */
static int sess_ensure_buf(jlgpu_filter_session_t *sess, int need)
{
    if (need <= sess->task_buf_size) {
        return 0;
    }
    uint8_t *p = (uint8_t *)malloc(need);
    if (!p) {
        return -1;
    }
    free(sess->task_buf);
    sess->task_buf      = p;
    sess->task_buf_size = need;
    return 0;
}
/** GXDMA 输出设置 (分配 pp_buf) */
static void sess_setup_output(jlgpu_filter_session_t *sess,
                              jlgpu_surface_t *surf, jlgpu_area_t area, uint8_t *dst)
{
    if (is_need_gxdma(sess->dst_format)) {
        uint32_t bw = 32;
        if (!sess->pp_buf0) {
            sess->pp_buf0 = (uint8_t *)malloc(bw * 32 * 2);
            ASSERT(sess->pp_buf0);
            sess->pp_buf1 = (uint8_t *)malloc(bw * 32 * 2);
            ASSERT(sess->pp_buf1);
        }
        uint8_t cf = JLGPU_FORMAT_RGB565;
        lv_area_t a = {0};
        a.x1 = area.x_min;
        a.y1 = area.y_min;
        a.x2 = area.x_max - 1;
        a.y2 = area.y_max - 1;
        lv_jlgpu_sfr_top_set(NULL, &a, bw, 32, sess->pp_buf0, sess->pp_buf1);
        surf->window.x_min = 0;
        surf->window.x_max = bw;
        surf->window.y_min = 0;
        surf->window.y_max = 0;
        surf->data   = dst;
        surf->format = cf;
        surf->width  = bw;
        lv_jlgpu_gxdma_sfr_set(NULL, &a, bw * 2, dst, cf, sess->dst_format, 0, 0, NULL);
    } else {
        jlgpu_area_t win = area;
        surf->data   = dst;
        surf->window = win;
        surf->format = sess->dst_format;
        surf->width  = sess->w;
        surf->rb_swap = is_yuv_format(sess->dst_format);
    }
}

/* ---- 胶片反相滤镜  ---- */
static jlgpu_err_t jlgpu_filter_negative(jlgpu_filter_session_t *sess, const uint8_t *src,
        uint8_t *dst, int w, int h,
        int in_format, int out_format)
{
    // printf(">>>>>> jlgpu_filter_negative2222\n");
    jlgpu_area_t            area = {0};
    jlgpu_draw_fill_t       draw_fill = {0};
    jlgpu_render_param_t    render_fill = {0};
    jlgpu_img_info_t        img_info = {0};
    jlgpu_draw_img_t        draw_img = {0};
    jlgpu_tex_inter_params_t tex_inter = {0};
    jlgpu_render_param_t    render_img = {0};
    jlgpu_mask_param_t      mask = {0};
    jlgpu_surface_t         surf = {0};
    jlgpu_uv_adr_t          uv_adr = {0};
    uint8_t                *uv_ptr = NULL;
    uint8_t                *fill_task = NULL;
    uint8_t                *img_task = NULL;
    int                     fill_len = 0, img_len = 0;
    jlgpu_err_t             err = 0;
    lv_area_t               a = {0};
    uint8_t                *pingping_buf0 = NULL;
    uint8_t                *pingping_buf1 = NULL;

    if (!sess || !src || !dst || w <= 0 || h <= 0) {
        return JLGPU_ERR_INVALID_ARG;
    }

    area = make_area(0, 0, w, h);

    /* ---- Task 1: GPU 填充白色 ---- */
    init_draw_fill_white(&draw_fill, area);
    init_render_src(&render_fill, area, JLGPU_MAX_GLOBAL_ALPHA);
    init_mask_none(&mask);

    fill_len = jlgpu_draw_fill_task_len(sess->ctx.ctx, &draw_fill, &mask);
    if (fill_len <= 0) {
        return JLGPU_ERR_TASK_LEN;
    }

    fill_task = sess->task_buf;//(uint8_t *)malloc(fill_len);
    if (!fill_task) {
        return JLGPU_ERR_MEMORY_NULL;
    }
    memset(fill_task, 0, fill_len);
    jlgpu_set_task(sess->ctx.ctx, fill_task, fill_len);
    jlgpu_draw_fill(sess->ctx.ctx, &render_fill, &draw_fill, &mask);

    /* ---- Task 2: draw_img SUBTRACTIVE (白 - 图 = 反相) ---- */
    uv_ptr = get_uv_ptr(src, w, h, in_format);
    init_img_src(&img_info, src, w, h, area, in_format);
    init_draw_img(&draw_img, &img_info, &tex_inter);
    init_mask_none(&mask);

    render_img.layer_en       = 1;
    render_img.render_area    = area;
    render_img.blend_mode     = JLGPU_BLEND_SUBTRACTIVE;
    render_img.global_alpha   = JLGPU_MAX_GLOBAL_ALPHA;
    render_img.render_quality = JLGPU_RENDERING_QUALITY_MSAA_8X;

    img_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &draw_img, &mask);
    if (img_len <= 0) {
        free(fill_task);
        return JLGPU_ERR_TASK_LEN;
    }

    u32 need = fill_len + img_len;
    if (sess_ensure_buf(sess, need)) {
        return JLGPU_ERR_TASK_LEN;
    }
    img_task = sess->task_buf + img_len;//(uint8_t *)malloc(img_len);
    if (!img_task) {
        free(fill_task);
        return JLGPU_ERR_MEMORY_NULL;
    }
    memset(img_task, 0, img_len);
    jlgpu_set_task(sess->ctx.ctx, img_task, img_len);
    jlgpu_draw_img(sess->ctx.ctx, &render_img, &draw_img, &mask);

    /* ---- 任务链: fill → img ---- */
    jlgpu_task_set_next(fill_task, img_task);

    /* ---- 输出 & 执行 ---- */
    sess_setup_output(sess, &surf, area, dst);
    if (uv_ptr) {
        uv_adr.uv_data = uv_ptr;
    }

    return exec_task(sess->ctx.ctx, &surf, uv_ptr ? &uv_adr : 0, fill_task);
}


/* ---- copy函数  ---- */
static jlgpu_err_t jlgpu_filter_copy(jlgpu_filter_session_t *sess, const uint8_t *src, uint8_t *dst, int w, int h, int in_format)
{
    jlgpu_area_t            area = {0};
    jlgpu_img_info_t        img_info = {0};
    jlgpu_draw_img_t        draw_img = {0};
    jlgpu_tex_inter_params_t tex_inter = {0};
    jlgpu_render_param_t    render_img = {0};
    jlgpu_mask_param_t      mask = {0};
    jlgpu_surface_t         surf = {0};
    jlgpu_uv_adr_t          uv_adr = {0};
    uint8_t                *uv_ptr = NULL;
    uint8_t                *img_task = NULL;
    int                     img_len = 0;
    jlgpu_err_t             err = 0;
    lv_area_t               a = {0};
    uint8_t                *pingping_buf0 = NULL;
    uint8_t                *pingping_buf1 = NULL;

    if (!sess || !src || !dst || w <= 0 || h <= 0) {
        return JLGPU_ERR_INVALID_ARG;
    }

    area = make_area(0, 0, w, h);

    /* ---- Task 2: draw_img SUBTRACTIVE (白 - 图 = 反相) ---- */
    uv_ptr = get_uv_ptr(src, w, h, in_format);
    init_img_src(&img_info, src, w, h, area, in_format);
    init_draw_img(&draw_img, &img_info, &tex_inter);
    init_mask_none(&mask);

    render_img.layer_en       = 1;
    render_img.render_area    = area;
    render_img.blend_mode     = JLGPU_BLEND_SRC;
    render_img.global_alpha   = JLGPU_MAX_GLOBAL_ALPHA;
    render_img.render_quality = JLGPU_RENDERING_QUALITY_MSAA_8X;

    img_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &draw_img, &mask);
    if (img_len <= 0) {
        return JLGPU_ERR_TASK_LEN;
    }

    u32 need = img_len;
    if (sess_ensure_buf(sess, need)) {
        return JLGPU_ERR_TASK_LEN;
    }
    img_task = sess->task_buf;//(uint8_t *)malloc(img_len);
    if (!img_task) {
        return JLGPU_ERR_MEMORY_NULL;
    }
    memset(img_task, 0, img_len);
    jlgpu_set_task(sess->ctx.ctx, img_task, img_len);
    jlgpu_draw_img(sess->ctx.ctx, &render_img, &draw_img, &mask);

    /* ---- 输出 & 执行 ---- */
    sess_setup_output(sess, &surf, area, dst);
    if (uv_ptr) {
        uv_adr.uv_data = uv_ptr;
    }

    return exec_task(sess->ctx.ctx, &surf, uv_ptr ? &uv_adr : 0, img_task);
}


/**
 * @brief 动态设置 N×M 宫格布局
 * @param count       子控件数量
 * @param cols        列数 (2=4宫格, 3=9宫格, 4=16宫格)
 * @param gap         控件间距(px)
 */
static void set_grid_layout(jlgpu_area_t *areas, uint32_t count, uint32_t cols, int32_t gap, int32_t parent_w, int32_t parent_h)
{
    if (count == 0 || cols == 0) {
        return;
    }

    uint32_t rows = (count + cols - 1) / cols;  // 向上取整计算行数

    // 总间隙数 = (列数-1)个水平间隙 + (行数-1)个垂直间隙
    int32_t cell_w = (parent_w - (int32_t)(cols - 1) * gap) / (int32_t)cols;
    int32_t cell_h = (parent_h - (int32_t)(rows - 1) * gap) / (int32_t)rows;

    for (uint32_t i = 0; i < count; i++) {
        int32_t col = i % cols;
        int32_t row = i / cols;

        int32_t x = col * (cell_w + gap);
        int32_t y = row * (cell_h + gap);
        areas[i] = make_area(x, y, x + cell_w, y + cell_w);
    }
}

/* --- 分格滤镜  ---- */
static jlgpu_err_t jlgpu_filter_grid(jlgpu_filter_session_t *sess, const uint8_t *src,
                                     uint8_t *dst, int w, int h,
                                     int in_format, int out_format, uint8_t grid_num, bool vertical, bool mirror)
{

    if (grid_num != 1 && grid_num != 2  && grid_num != 4 && grid_num != 9) {
        printf("error grid_num invalid\n");
        return JLGPU_ERR_NOT_SUPPORTED;
    }

    /*printf(">>>>>> jlgpu_filter_grid2222\n");*/
    jlgpu_area_t            win_area = {0};//src
    jlgpu_area_t            area[9] = {0};//最多支持9宫格
    /*jlgpu_area_t            area1 = {0};*/
    /*jlgpu_area_t            area2 = {0};*/
    jlgpu_draw_fill_t       draw_fill = {0};
    jlgpu_render_param_t    render_fill = {0};
    jlgpu_img_info_t        img_info = {0};
    jlgpu_draw_img_t        draw_img = {0};
    jlgpu_tex_inter_params_t tex_inter = {0};
    jlgpu_render_param_t    render_img = {0};
    jlgpu_mask_param_t      mask = {0};
    jlgpu_surface_t         surf = {0};
    jlgpu_matrix_t          matrix = {0};
    jlgpu_uv_adr_t          uv_adr = {0};
    uint8_t                *uv_ptr = NULL;
    /*uint8_t                *fill_task = NULL;*/
    uint8_t                *img_task[9] = {NULL};
    /*uint8_t                *img_task1 = NULL;*/
    int                     fill_len = 0, img_len = 0;
    jlgpu_err_t             err = 0;
    lv_area_t               a = {0};
    uint8_t                *pingping_buf0 = NULL;
    uint8_t                *pingping_buf1 = NULL;


    if (!sess || !src || !dst || w <= 0 || h <= 0) {
        return JLGPU_ERR_INVALID_ARG;
    }

    win_area = make_area(0, 0, w, h);

    if (grid_num > 1) {
        if (grid_num == 2) {
            if (!vertical) {
                /* ---- 左右分屏 ---- */
                area[0] = make_area(0, 0, w / 2, h);
                area[1] = make_area(w / 2, 0, w, h);

                if (mirror) {
                    jlgpu_matrix_set_identity(&matrix);
                    jlgpu_matrix_translate(&matrix, - w / 2, - h / 2);
                    jlgpu_matrix_flip(&matrix, 1, 0);
                    jlgpu_matrix_translate(&matrix, w / 2, h / 2);
                }

            } else {
                /* ---- 上下分屏 ---- */
                area[0] = make_area(0, 0, w, h / 2);
                area[1] = make_area(0, h / 2, w, h);

                if (mirror) {
                    jlgpu_matrix_set_identity(&matrix);
                    jlgpu_matrix_translate(&matrix, - w / 2, - h / 2);
                    jlgpu_matrix_flip(&matrix, 0, 1);
                    jlgpu_matrix_translate(&matrix, w / 2, h / 2);
                }
            }

        } else {
            set_grid_layout(area, grid_num, (grid_num == 4) ? 2 : 3, 0, w, h);
        }

    } else {
        area[0] = win_area;
        area[1] = win_area;
    }

    uv_ptr = get_uv_ptr(src, w, h, in_format);
    init_img_src(&img_info, src, w, h, area[0], in_format);

    init_draw_img(&draw_img, &img_info, &tex_inter);
    init_mask_none(&mask);

    render_img.layer_en       = 1;
    render_img.render_area    = area[0];
    render_img.blend_mode     = JLGPU_BLEND_SRC;
    render_img.global_alpha   = JLGPU_MAX_GLOBAL_ALPHA;
    render_img.render_quality = JLGPU_RENDERING_QUALITY_MSAA_8X;

    img_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &draw_img, &mask);

    if (img_len <= 0) {
        return JLGPU_ERR_TASK_LEN;
    }

    u32 need = img_len * grid_num;
    if (sess_ensure_buf(sess, need)) {
        return JLGPU_ERR_TASK_LEN;
    }
    img_task[0] = sess->task_buf;//(uint8_t *)malloc(img_len);
    if (!img_task[0]) {
        return JLGPU_ERR_MEMORY_NULL;
    }
    memset(img_task[0], 0, img_len);
    jlgpu_set_task(sess->ctx.ctx, img_task[0], img_len);
    jlgpu_draw_img(sess->ctx.ctx, &render_img, &draw_img, &mask);

    for (uint32_t i = 1; i < grid_num; i++) {

        render_img.render_area = area[i];

        //镜像变换设置
        if (mirror && grid_num == 2) {
            set_img_clip_area(&img_info, win_area); //重置clip
            init_draw_img(&draw_img, &img_info, &tex_inter);

            set_draw_img_transform(&draw_img, &matrix); //应用变换

            img_task[i] = sess->task_buf + img_len * i; //(uint8_t *)malloc(img_len);
            img_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &draw_img, &mask);
            memset(img_task[i], 0, img_len);
            jlgpu_set_task(sess->ctx.ctx, img_task[i], img_len);
            jlgpu_draw_img(sess->ctx.ctx, &render_img, &draw_img, &mask);
        } else {
            img_task[i] = sess->task_buf + img_len * i; //(uint8_t *)malloc(img_len);
            memset(img_task[i], 0, img_len);
            jlgpu_set_task(sess->ctx.ctx, img_task[i], img_len);
            jlgpu_draw_img(sess->ctx.ctx, &render_img, &draw_img, &mask);
        }

        /* ---- 任务链串联 ---- */
        jlgpu_task_set_next(img_task[i - 1], img_task[i]);
    }

    /* ---- 输出 & 执行 ---- */
    u16 total_height = win_area.y_max;
    int batch_lines  = sess->line_batch_count > 0 ? sess->line_batch_count : total_height;

    win_area.y_min = 0;
    win_area.y_max = 0;
    for (int l = 0; l < total_height; l += batch_lines) {
        win_area.y_max += batch_lines;
        if (win_area.y_max > total_height) {
            win_area.y_max = total_height;
        }

        sess_setup_output(sess, &surf, win_area, dst);
        if (uv_ptr) {
            uv_adr.uv_data = uv_ptr;
        }

        exec_task(sess->ctx.ctx, &surf, uv_ptr ? &uv_adr : 0, img_task[0]);

        /* 回调用户: 可对当前批次 dst 缓冲进行操作 */
        if (sess->line_cb) {
            int line_count = win_area.y_max - win_area.y_min;
            sess->line_cb(sess, dst, sess->w, sess->h, win_area.y_min, line_count, sess->line_cb_data);
        }

        win_area.y_min += batch_lines;
    }
    return 0;
}


static lv_color_format_t get_lv_color_format(int fmt)
{

    lv_color_format_t format = 0xff;
    switch (fmt) {
    case JLGPU_FORMAT_YUV420_BT601:
        format = LV_COLOR_FORMAT_NV12;
        break;
    case JLGPU_FORMAT_YUV422_BT601:
        format = LV_COLOR_FORMAT_YUY2;
        break;
    default:
        printf("not supported this format 0x%x\n", fmt);
        break;
    }

    return format;
}
/* ========================================================================
 *  统一对外接口
 * ======================================================================== */

/**
 * @brief 贴纸滤镜处理 (STICKER)
 *
 * 流程: 加载贴纸资源 → 配置压缩/变换 → 构建GPU任务链 → 执行
 * 任务链: Task1(源图) → [GXDEC(C3)] → Task2(贴纸SRC_OVER)
 */
static void jlgpu_sticker_apply(jlgpu_filter_session_t *sess,
                                const uint8_t *src, uint8_t *dst,
                                jlgpu_area_t area, jlgpu_sticker_t *sticker)
{
    lv_img_dsc_t bin_dsc = {0};
    JLBinResource bin_res;

    lv_get_JLBinResource((const char *)sticker->path, &bin_res);
    lv_res_t ret = lv_get_img_dsc_bin((const char *)sticker->path, &bin_dsc);
    if (ret != LV_RESULT_OK) {
        printf("lv_get_img_dsc_bin failed\n");
        return;
    }

    uint32_t src_stride = bin_dsc.header.stride;
    uint8_t  src_bpp    = lv_color_format_get_bpp(bin_dsc.header.cf);
    bool has_indexed = (bin_dsc.header.cf >= LV_COLOR_FORMAT_I1 &&
                        bin_dsc.header.cf <= LV_COLOR_FORMAT_I8);

    /* GPU 任务参数 */
    jlgpu_img_info_t         info  = {0};
    jlgpu_draw_img_t         draw_img = {0};
    jlgpu_img_info_t         st_info  = {0};
    jlgpu_draw_img_t         st_img   = {0};
    jlgpu_tex_inter_params_t inter = {0};
    jlgpu_tex_inter_params_t st_inter = {0};
    jlgpu_render_param_t     r     = {0};
    jlgpu_render_param_t     st_r  = {0};
    jlgpu_mask_param_t       m     = {0};
    jlgpu_area_t             st_area;
    jlgpu_matrix_t           st_matrix = {0};
    jlgpu_extend_param_t     ex_params = {0};
    jlgpu_gxdec_t            gxdec_params = {0};
    uint8_t                  is_c3 = 0;
    int t1_len, t2_len, need;

    uint8_t *uv_ptr = get_uv_ptr(src, sess->w, sess->h, sess->src_format);

    /* ---- Task1: 渲染源图 ---- */
    init_img_src(&info, src, sess->w, sess->h, area, sess->src_format);
    init_draw_img(&draw_img, &info, &inter);
    init_render_src(&r, area, JLGPU_MAX_GLOBAL_ALPHA);
    init_mask_none(&m);

    t1_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &draw_img, &m);

    /* ---- 贴纸资源解析 ---- */
    uint8_t  adr_mode = 0;
    uint8_t *src_clut   = NULL;
    uint16_t clut_format = 0;
    rle_info_t *rle_info = NULL;
    c2_info_t *c2_info = NULL;
    c3_info_t *c3_info = NULL;

    init_img_src(&st_info, bin_dsc.data, bin_dsc.header.w, bin_dsc.header.h,
                 make_area(0, 0, bin_dsc.header.w, bin_dsc.header.h), bin_dsc.header.cf);

    if (bin_res.jl_header.res_compress == LV_COMPRESS_RLE) {
        rle_info = (rle_info_t *)bin_dsc.data;
        lv_rle_get_stride(rle_info, &adr_mode);
        if (has_indexed) {
            src_clut    = (uint8_t *)bin_dsc.data + rle_info->lut_addr;
            clut_format = rle_info->lut_format;
        }
        st_info.adr_mode      = adr_mode;
        st_info.color_format  = rle_info->format;
        st_info.data          = (uint8_t *)bin_dsc.data + rle_info->addr;
        st_info.width         = rle_info->width;
        st_info.height        = rle_info->height;
        st_info.compress_mode = JLGPU_COMPRESS_MODE_RLE;
        st_info.compress_size = rle_info->len;

    } else if (bin_res.jl_header.res_compress == LV_COMPRESS_ETC2) {
        adr_mode = JLGPU_ADRMODE_TILE;
        c2_info = (c2_info_t *)bin_dsc.data;
        st_info.adr_mode      = adr_mode;
        st_info.color_format  = c2_info->format;
        st_info.data          = (uint8_t *)bin_dsc.data + sizeof(c2_info_t);
        st_info.width         = c2_info->img_width;
        st_info.height        = c2_info->img_height;
        st_info.compress_mode = JLGPU_COMPRESS_MODE_ETC2;
        st_info.compress_size = 0;

    } else if (bin_res.jl_header.res_compress == LV_COMPRESS_C3) {
#if LV_USE_JL_C3
        is_c3 = 1;
        lv_jlgpu_mutex_pend();

        uint8_t has_table = bin_res.jl_header.nop.image.has_table;
        if (has_table) {
            lv_c3_load_qtable_type_app((uint8_t *)bin_dsc.data, bin_dsc.data_size);
        } else {
            printf("[warning]:c3 should has table!!!\n");
        }

        uint8_t *c3_data = (uint8_t *)bin_dsc.data + sizeof(c3_head_t);
        c3_info = (c3_info_t *)c3_data;

        gxdec_params.img_base = c3_data;
        gxdec_params.mmu_en  = 0;
        ex_params.layer_en   = 1;
        ex_params.breakpoint_en = 0;
        ex_params.command    = JLGPU_EXTEND_COMMAND_CONFIG_GXDEC;
        ex_params.command_gxdec = &gxdec_params;

        st_info.adr_mode      = JLGPU_ADRMODE_TILE;
        st_info.color_format  = JLGPU_FORMAT_ARGB8888;
        st_info.data          = (uint8_t *)0x50000000;
        st_info.width         = c3_info->img_width;
        st_info.height        = c3_info->img_height;
        st_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
        st_info.compress_size = 0;

        lv_jlgpu_mutex_post();
#else
        printf("LV_COMPRESS_C3 sticker not supported\n");
        return;
#endif

    } else {
        if (has_indexed) {
            uint32_t palette_len = sizeof(lv_color32_t) *
                                   LV_COLOR_INDEXED_PALETTE_SIZE(bin_dsc.header.cf);
            src_clut = (uint8_t *)bin_dsc.data;
            bin_dsc.data += palette_len;
        }
        st_info.adr_mode      = adr_mode;
        st_info.color_format  = bin_dsc.header.cf;
        st_info.data          = (uint8_t *)bin_dsc.data;
        if ((src_bpp >> 3) != 0) {
            st_info.width = src_stride / (src_bpp >> 3);
        } else {
            st_info.width = bin_dsc.header.w;
        }
        st_info.height         = bin_dsc.header.h;
        st_info.compress_mode = JLGPU_COMPRESS_MODE_NONE;
        st_info.compress_size = 0;
    }

    /* ---- 贴纸变换矩阵 ---- */
    int sticker_src_w = is_c3 ? c3_info->img_width  : bin_dsc.header.w;
    int sticker_src_h = is_c3 ? c3_info->img_height : bin_dsc.header.h;

    if (src_clut) {
        st_img.clut_format = clut_format;
        st_img.clut_adr    = src_clut;
    }
    init_draw_img(&st_img, &st_info, &st_inter);

    if (sticker->angle || (sticker->dst_w > 0 && sticker->dst_h > 0)) {
        if (sticker->dst_w == 0) {
            sticker->dst_w = sticker_src_w;
        }
        if (sticker->dst_h == 0) {
            sticker->dst_h = sticker_src_h;
        }
        st_inter.inter_mode = JLGPU_SAMPLE_MODE_BILI;
        int dst_cx = sticker->x + sticker->dst_w / 2;
        int dst_cy = sticker->y + sticker->dst_h / 2;

        if (sticker->angle != 0.0f) {
            float rad = sticker->angle * 3.14159265f / 180.0f;
            if (rad < 0) {
                rad = -rad;
            }
            float cos_a = jlgpu_cosf(rad);
            float sin_a = jlgpu_sinf(rad);
            int bb_w = (int)((float)sticker->dst_w * cos_a +
                             (float)sticker->dst_h * sin_a + 0.5f);
            int bb_h = (int)((float)sticker->dst_w * sin_a +
                             (float)sticker->dst_h * cos_a + 0.5f);
            st_area.x_min = dst_cx - bb_w / 2;
            if (st_area.x_min < 0) {
                st_area.x_min = 0;
            }
            st_area.x_max = dst_cx + (bb_w - bb_w / 2);
            st_area.y_min = dst_cy - bb_h / 2;
            if (st_area.y_min < 0) {
                st_area.y_min = 0;
            }
            st_area.y_max = dst_cy + (bb_h - bb_h / 2);
        } else {
            st_area.x_min = sticker->x;
            st_area.x_max = sticker->x + sticker->dst_w;
            st_area.y_min = sticker->y;
            st_area.y_max = sticker->y + sticker->dst_h;
        }

        jlgpu_matrix_set_identity(&st_matrix);
        jlgpu_matrix_translate(&st_matrix, -(float)dst_cx, -(float)dst_cy);
        jlgpu_matrix_rotate(&st_matrix, -sticker->angle * 3.14159265f / 180.0f);
        jlgpu_matrix_scale(&st_matrix,
                           (float)sticker_src_w / (float)sticker->dst_w,
                           (float)sticker_src_h / (float)sticker->dst_h);
        jlgpu_matrix_translate(&st_matrix,
                               (float)sticker_src_w / 2.0f,
                               (float)sticker_src_h / 2.0f);
        set_draw_img_transform(&st_img, &st_matrix);
    } else {
        st_area.x_min = sticker->x;
        st_area.x_max = sticker->x + sticker_src_w;
        st_area.y_min = sticker->y;
        st_area.y_max = sticker->y + sticker_src_h;
    }

    init_mask_none(&m);
    st_r.layer_en       = 1;
    st_r.render_area    = st_area;
    st_r.blend_mode     = JLGPU_BLEND_SRC_OVER;
    st_r.global_alpha   = JLGPU_MAX_GLOBAL_ALPHA;
    st_r.render_quality = JLGPU_RENDERING_QUALITY_MSAA_8X;

    /* ---- 构建 GPU 任务链 ---- */

    /* Task1: 源图 */
    need = t1_len + 256;
    if (sess_ensure_buf(sess, need)) {
        return;
    }
    memset(sess->task_buf, 0, t1_len);
    jlgpu_set_task(sess->ctx.ctx, sess->task_buf, t1_len);
    jlgpu_draw_img(sess->ctx.ctx, &r, &draw_img, &m);

    int task2_off;

    if (is_c3) {
        /* ---- C3 路径: 用独立 HAL context 构建 GXDEC + draw_img 链 ---- */
        jlgpu_hal_context_t *c3_hal = jlgpu_hal_create_context();
        jlgpu_trans_mode_t tex_trans = st_img.tex_trans_mode;
        jlgpu_trans_mode_t path_trans = JLGPU_TRANS_MODE_NONE;

        /* GXDEC extend */
        int c3_ex_len = jlgpu_extend_command_task_len(sess->ctx.ctx, &ex_params);
        need = t1_len + c3_ex_len + 256;
        if (sess_ensure_buf(sess, need)) {
            jlgpu_hal_destroy_context(c3_hal);
            return;
        }
        memset(sess->task_buf + t1_len, 0, c3_ex_len);
        jlgpu_set_task(sess->ctx.ctx, sess->task_buf + t1_len, c3_ex_len);
        jlgpu_extend_command(sess->ctx.ctx, &ex_params);

        /* C3 draw_img: 用独立 HAL context */
        gpu_hw_task_set_render_param(c3_hal, JLGPU_DRAW_IMG, JLGPU_MASK_MODE_NONE,
                                     JLGPU_PAINT_MODE_MULTIPLY, tex_trans,
                                     JLGPU_TRANS_MODE_NONE, path_trans);
        int c3_img_len = gpu_hw_task_get_len_render(c3_hal);
        task2_off = t1_len + c3_ex_len;
        need = task2_off + c3_img_len;
        if (sess_ensure_buf(sess, need)) {
            jlgpu_hal_destroy_context(c3_hal);
            return;
        }

        memset(sess->task_buf + task2_off, 0, c3_img_len);
        gpu_hw_task_set_task(c3_hal, sess->task_buf + task2_off);

        gpu_hw_task_set_render_common(c3_hal);
        gpu_hw_task_set_layer_en(c3_hal, 1);
        gpu_hw_task_set_breakpoint_en(c3_hal, 0);
        gpu_hw_task_set_blend_mode(c3_hal, JLGPU_BLEND_SRC_OVER);
        gpu_hw_task_set_dither_en(c3_hal, 0);
        gpu_hw_task_set_gb_trans_en(c3_hal, 0);
        gpu_hw_task_set_rgb_limit_en(c3_hal, 0);
        gpu_hw_task_set_global_alpha(c3_hal, JLGPU_MAX_GLOBAL_ALPHA);

        if (tex_trans == JLGPU_TRANS_MODE_AFFINE) {
            gpu_hw_task_set_layer_quality(c3_hal, JLGPU_RENDER_QUALITY);
        } else {
            gpu_hw_task_set_layer_quality(c3_hal, JLGPU_RENDERING_QUALITY_NONANTIALIASED);
        }

        gpu_hw_task_set_bbox(c3_hal, st_area.x_min, st_area.x_max,
                             st_area.y_min, st_area.y_max);

        /* tex */
        gpu_hw_task_set_color_ext(c3_hal, JLGPU_COLOR_EXTEND_HIGH);
        gpu_hw_task_set_tex_premult(c3_hal, JLGPU_PREMULT_TRUE);
        gpu_hw_task_set_boundary_ext(c3_hal, JLGPU_BOUNDARY_EXTEND_ZERO);
        gpu_hw_task_set_colorkey_en(c3_hal, 0);
        gpu_hw_task_set_tex_clip(c3_hal, 0, c3_info->img_width,
                                 0, c3_info->img_height);

        gpu_hw_task_set_tex_info(c3_hal, c3_info->img_width, c3_info->img_height,
                                 4096 * 4 * 8, JLGPU_FORMAT_ARGB8888,
                                 JLGPU_ADRMODE_TILE, JLGPU_COMPRESS_MODE_NONE,
                                 JLGPU_RBS_ARGB, JLGPU_ENDIAN_LITTLE,
                                 JLGPU_ALPHA_END_ARGB, 0,
                                 (uint8_t *)0x50000000);

        if (st_img.surface2image) {
            gpu_hw_task_set_tex_matrix(c3_hal, (float *)st_img.surface2image);
        }
        gpu_hw_task_set_tex_sample_param(c3_hal, JLGPU_SAMPLE_MODE_BILI, 6, NULL);

        jlgpu_hal_destroy_context(c3_hal);

        /* Task chain: Task1 → GXDEC → C3_img */
        jlgpu_task_set_next(sess->task_buf, sess->task_buf + t1_len);
        jlgpu_task_set_next(sess->task_buf + t1_len, sess->task_buf + task2_off);

    } else {
        /* ---- 非 C3 路径: 标准 jlgpu API ---- */
        t2_len = jlgpu_draw_img_task_len(sess->ctx.ctx, &st_img, &m);
        task2_off = t1_len;
        need = task2_off + t2_len;
        if (sess_ensure_buf(sess, need)) {
            return;
        }
        memset(sess->task_buf + task2_off, 0, t2_len);
        jlgpu_set_task(sess->ctx.ctx, sess->task_buf + task2_off, t2_len);
        jlgpu_draw_img(sess->ctx.ctx, &st_r, &st_img, &m);

        jlgpu_task_set_next(sess->task_buf, sess->task_buf + task2_off);
    }

    /* 输出 & 执行 */
    jlgpu_surface_t surf = {0};
    jlgpu_uv_adr_t  uv   = {0};
    sess_setup_output(sess, &surf, area, dst);
    if (uv_ptr) {
        uv.uv_data = uv_ptr;
    }
    exec_task(sess->ctx.ctx, &surf, uv_ptr ? &uv : 0, sess->task_buf);
}

jlgpu_filter_session_t *jlgpu_filter_session_init(int w, int h,
        int src_format, int dst_format)
{
    jlgpu_filter_session_t *sess;

    sess = (jlgpu_filter_session_t *)malloc(sizeof(*sess));
    if (!sess) {
        return NULL;
    }
    memset(sess, 0, sizeof(*sess));
    sess->w = w;
    sess->h = h;
    sess->src_format = src_format;
    sess->dst_format = dst_format;

    jlgpu_ctx_buffer_init(&sess->ctx);

    /* 预分配: 512B  */
    sess->task_buf_size = 512;
    sess->task_buf = (uint8_t *)malloc(sess->task_buf_size);
    if (!sess->task_buf) {
        /* 本文件用的是 jlgpu_ctx_buffer_init(不走 lv_jlgpu_ctx_init),
         * 这里补占门以匹配 lv_jlgpu_ctx_uninit 内的 gate_leave,保持计数对称 */
        jlgpu_gpu_gate_enter();
        lv_jlgpu_ctx_uninit(&sess->ctx);
        free(sess);
        return NULL;
    }

    return sess;
}


void jlgpu_filter_session_process(jlgpu_filter_session_t *sess,
                                  const uint8_t *src, uint8_t *dst,
                                  jlgpu_filter_mode_t mode,
                                  void *user_data)
{
    jlgpu_area_t  area  = make_area(0, 0, sess->w, sess->h);
    jlgpu_color_t color = {0};
    jlgpu_surface_t surf  = {0};
    jlgpu_uv_adr_t  uv    = {0};
    uint8_t        *uv_ptr;
    int             task_len;

    lv_color_format_t lv_fmt = get_lv_color_format(sess->src_format);

    lv_jlgpu_mutex_pend();

    uint8_t gpu_link_dbi = lv_jl_lcd_get_need_gpu_link_dbi();
    if (gpu_link_dbi) {
        //关闭gpu联动dbi
        dbi_link_gpu(0);
    }

    /* 每帧重置 GPU 硬件状态, 保持 ctx 内部干净 */
    JLGXDMA_link_gpu(0);
    jlgpu_hw_set_sfr_top(NULL);
    lv_jlgpu_pnd_sem_clr();
    JLGXDMA_init();

    jlgpu_init(sess->ctx.ctx);

    switch (mode) {
    case JLGPU_FILTER_RED:
        color.b = 80;
        color.g = 80;
        color.r = 250;
        color.a = 255;
        goto do_tint;
    case JLGPU_FILTER_GREEN:
        color.b = 0;
        color.g = 220;
        color.r = 0;
        color.a = 255;
        goto do_tint;
    case JLGPU_FILTER_YELLOW:
        color.b = 100;
        color.g = 255;
        color.r = 255;
        color.a = 255;
        goto do_tint;
    case JLGPU_FILTER_SEPIA:
        color.b = 18;
        color.g = 41;
        color.r = 112;
        color.a = 255;
        goto do_tint;

do_tint: {
            jlgpu_img_info_t         info = {0};
            jlgpu_draw_img_fill_t    imf  = {0};
            jlgpu_tex_inter_params_t inter = {0};
            jlgpu_render_param_t     r    = {0};
            jlgpu_mask_param_t       m    = {0};

            uv_ptr = get_uv_ptr(src, sess->w, sess->h, sess->src_format);
            init_img_src(&info, src, sess->w, sess->h, area, sess->src_format);
            init_draw_img(&imf.draw_img, &info, &inter);
            imf.draw_fill.fill_trans_mode   = JLGPU_TRANS_MODE_NONE;
            imf.draw_fill.surface2paint     = 0;
            imf.draw_fill.paint_info.premult = JLGPU_PREMULT_TRUE;
            imf.draw_fill.paint_info.pt_clip = area;
            imf.draw_fill.color = color;
            imf.paint_mode = JLGPU_PAINT_MODE_GAIN;
            init_render_src(&r, area, JLGPU_MAX_GLOBAL_ALPHA);
            init_mask_none(&m);

            task_len = jlgpu_draw_img_fill_task_len(sess->ctx.ctx, &imf, &m);
            if (sess_ensure_buf(sess, task_len)) {
                break;
            }

            memset(sess->task_buf, 0, task_len);
            jlgpu_set_task(sess->ctx.ctx, sess->task_buf, task_len);
            jlgpu_draw_img_fill(sess->ctx.ctx, &r, &imf, &m);

            sess_setup_output(sess, &surf, area, dst);
            if (uv_ptr) {
                uv.uv_data = uv_ptr;
            }
            exec_task(sess->ctx.ctx, &surf, uv_ptr ? &uv : 0, sess->task_buf);
            break;
        }
    case JLGPU_FILTER_GRAY: {

        if (!uv128_supported(sess->src_format)) {
            printf("uv128 not supported this format %d\n", sess->src_format);
            break;
        }

        memset((uint8_t *)src + sess->w * sess->h, 128, sess->w * sess->h / 2);
        if (psram_cache_check((void *)src)) {
            DcuFlushRegion((uint32_t *)src + sess->w * sess->h, sess->w * sess->h / 2);
        }

        break;
    }
    case JLGPU_FILTER_NEGATIVE: {

        jlgpu_filter_negative(sess, src, dst,
                              sess->w, sess->h, sess->src_format, sess->dst_format);
        break;
    }
    case JLGPU_FILTER_STICKER: {
        jlgpu_sticker_t *sticker = (jlgpu_sticker_t *)user_data;
        if (!sticker || !sticker->path) {
            break;
        }
        jlgpu_sticker_apply(sess, src, dst, area, sticker);
        break;
    }
    case JLGPU_FILTER_HONEYCOMB_NORMAL: {
        if ((sess->w % 2 == 0) && (sess->h % 2 == 0) && (sess->src_format == sess->dst_format)) { //图像宽高必须为偶数
            int batch = sess->line_batch_count > 0 ? sess->line_batch_count : JLGPU_FILTER_DEFAULT_BATCH;
            honeycomb_lut_t *honeycomb_lut = honeycomb_lut_create(sess->w, sess->h, 80, batch);
            if (!honeycomb_lut) {
                break;
            }
            if (src == dst) {
                uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
                uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
                honeycomb_process(sess, src, process_buf, sess->w, sess->h, lv_fmt, honeycomb_lut);
                //memcpy(dst, process_buf, process_buf_size);
                jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
                filters_yuv_buffer_free(process_buf);
            } else {
                honeycomb_process(sess, src, dst, sess->w, sess->h, lv_fmt, honeycomb_lut);
            }
            honeycomb_lut_destroy(honeycomb_lut);
        } else {
            printf("The image width and height must be even numbers\n");
        }
        break;
    }
    case JLGPU_FILTER_FISHEYE_NORMAL: {
        if (sess->src_format == sess->dst_format) {
            lv_color_format_t lv_fmt = get_lv_color_format(sess->src_format);
            int batch = sess->line_batch_count > 0 ? sess->line_batch_count : JLGPU_FILTER_DEFAULT_BATCH;
            fisheye_lut_t *fisheye_lut = fisheye_lut_create(sess->w, sess->h, 1.0f, batch);
            if (!fisheye_lut) {
                break;
            }
            if (src == dst) {
                uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
                uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
                fisheye_process(sess, src, process_buf, sess->w, sess->h, lv_fmt, fisheye_lut);
                /*memcpy(dst, process_buf, process_buf_size);*/
                jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
                filters_yuv_buffer_free(process_buf);
            } else {
                fisheye_process(sess, src, dst, sess->w, sess->h, lv_fmt, fisheye_lut);
            }
            fisheye_lut_destroy(fisheye_lut);
        } else {
            printf("sess->src_format != sess->dst_format\n");
        }
        break;
    }
    case JLGPU_FILTER_NONE: {
        break;
    }
    case JLGPU_FILTER_TILE_HORIZ: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 0, 0);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 0, 0);
        }
        break;
    }
    case JLGPU_FILTER_TILE_VERT: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 1, 0);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 1, 0);
        }
        break;
    }
    case JLGPU_FILTER_MIRROR_HORIZ: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 0, 1);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 0, 1);
        }
        break;
    }
    case JLGPU_FILTER_MIRROR_VERT: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 1, 1);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 2, 1, 1);
        }
        break;
    }
    case JLGPU_FILTER_GRID_4: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 4, 0, 0);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 4, 0, 0);
        }
        break;
    }
    case JLGPU_FILTER_GRID_9: {
        if (src == dst) {
            uint32_t process_buf_size = filters_yuv_buffer_size(sess->w, sess->h, lv_fmt);
            uint8_t *process_buf = filters_yuv_buffer_alloc(sess->w, sess->h, lv_fmt);
            jlgpu_filter_grid(sess, src, process_buf, sess->w, sess->h, sess->src_format, sess->dst_format, 9, 0, 0);
            jlgpu_filter_copy(sess, process_buf, dst, sess->w, sess->h, sess->src_format);
            filters_yuv_buffer_free(process_buf);
        } else {
            jlgpu_filter_grid(sess, src, dst, sess->w, sess->h, sess->src_format, sess->dst_format, 9, 0, 0);
        }
        break;
    }
    default:
        break;
    }

    if (gpu_link_dbi) {
        //打开gpu联动dbi
        dbi_link_gpu(1);
    }

    lv_jlgpu_mutex_post();
}

void jlgpu_filter_session_uninit(jlgpu_filter_session_t *sess)
{
    if (!sess) {
        return;
    }
    free(sess->task_buf);
    if (sess->pp_buf0) {
        free(sess->pp_buf0);
    }
    if (sess->pp_buf1) {
        free(sess->pp_buf1);
    }
    /* 同上:补占门,匹配 lv_jlgpu_ctx_uninit 内的 gate_leave */
    jlgpu_gpu_gate_enter();
    lv_jlgpu_ctx_uninit(&sess->ctx);
    free(sess);
}

void jlgpu_filter_session_set_line_config(jlgpu_filter_session_t *sess,
        int batch_lines,
        jlgpu_filter_line_cb_t line_cb,
        void *user_data)
{
    if (!sess) {
        return;
    }
    sess->line_batch_count = batch_lines;
    sess->line_cb          = line_cb;
    sess->line_cb_data     = user_data;
}

/* ======================== 缓冲区分配 ======================== */

size_t filters_yuv_buffer_size(int w, int h, lv_color_format_t fmt)
{
    if (w <= 0 || h <= 0 || (w & 1) || (h & 1)) {
        return 0;
    }
    size_t wh = (size_t)w * h;
    if (fmt == LV_COLOR_FORMAT_NV12) {
        return wh * 3 / 2;
    }
    if (fmt == LV_COLOR_FORMAT_YUY2) {
        return wh * 2;
    }
    return 0;
}

uint8_t *filters_yuv_buffer_alloc(int w, int h, lv_color_format_t fmt)
{
    size_t size = filters_yuv_buffer_size(w, h, fmt);
    if (size == 0) {
        return NULL;
    }

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    uint8_t *buf = (uint8_t *)malloc_psram(size);
#else
    uint8_t *buf = (uint8_t *)malloc(size);
#endif

    if (!buf) {
        return NULL;
    }

#if 0
    memset(buf, 0, size);

    if (fmt == LV_COLOR_FORMAT_NV12) {
        memset(buf + (size_t)w * h, 128, size - (size_t)w * h);
    } else if (fmt == LV_COLOR_FORMAT_YUY2) {
        for (size_t i = 0; i < size; i += 4) {
            buf[i] = 0;
            buf[i + 1] = 128;
            buf[i + 2] = 0;
            buf[i + 3] = 128;
        }
    }
#endif

    return buf;
}

void filters_yuv_buffer_free(uint8_t *free_buf)
{
    if (free_buf) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        free_psram(free_buf);
#else
        free(free_buf);
#endif
    }
}
/* ======================== 鱼眼镜头效果 ======================== */
/* ======================== 坐标镜像反射工具函数 ======================== */
//将越界坐标镜像反射回图像内部，消除黑边
static int mirror_coord(int val, int max_val)
{
    if (val < 0) {
        val = -val - 1;
    } else if (val >= max_val) {
        val = 2 * max_val - val - 2;
    }
    // 兜底保护（防止 strength 极大导致一次反射不够）
    if (val < 0) {
        val = 0;
    }
    if (val >= max_val) {
        val = max_val - 1;
    }
    return val;
}

/* ======================== LUT 构建与销毁 ======================== */
void fisheye_lut_destroy(fisheye_lut_t *lut)
{
    if (lut) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        free_psram(lut->y_lut_x);
        free_psram(lut->y_lut_y);
        free_psram(lut->uv_lut_x);
        free_psram(lut->uv_lut_y);
        free_psram(lut);
#else
        free(lut->y_lut_x);
        free(lut->y_lut_y);
        free(lut->uv_lut_x);
        free(lut->uv_lut_y);
        free(lut);
#endif
    }
}

/**
 * @brief 创建鱼眼LUT (分片分配版)
 *
 * 仅分配 batch_lines 行所需的 LUT 内存，而非全帧。
 * LUT 数据在 fisheye_process 中按片填充后立即消费。
 *
 * @param w           图像宽度 (必须为偶数)
 * @param h           图像高度 (必须为偶数)
 * @param strength    鱼眼强度 [0.0, 1.0]
 * @param batch_lines 每批处理行数 (必须为偶数，决定内存占用)
 */
fisheye_lut_t *fisheye_lut_create(int w, int h, float strength, int batch_lines)
{
    if (w <= 0 || h <= 0 || batch_lines <= 0 || (w & 1) || (h & 1) || (batch_lines & 1)) {
        return NULL;
    }
    if (batch_lines > h) {
        batch_lines = h;
    }

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    fisheye_lut_t *lut = (fisheye_lut_t *)malloc_psram(sizeof(fisheye_lut_t));
#else
    fisheye_lut_t *lut = (fisheye_lut_t *)malloc(sizeof(fisheye_lut_t));
#endif
    if (!lut) {
        return NULL;
    }
    memset(lut, 0, sizeof(fisheye_lut_t));

    lut->width       = w;
    lut->height      = h;
    lut->strength    = strength;
    lut->batch_lines = batch_lines;

    size_t y_pixels  = (size_t)w * batch_lines;
    int    uv_w       = w / 2;
    int    uv_lines   = batch_lines / 2;
    size_t uv_pixels  = (size_t)uv_w * uv_lines;

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    lut->y_lut_x  = (int16_t *)malloc_psram(y_pixels  * sizeof(int16_t));
    lut->y_lut_y  = (int16_t *)malloc_psram(y_pixels  * sizeof(int16_t));
    lut->uv_lut_x = (int16_t *)malloc_psram(uv_pixels * sizeof(int16_t));
    lut->uv_lut_y = (int16_t *)malloc_psram(uv_pixels * sizeof(int16_t));
#else
    lut->y_lut_x  = (int16_t *)malloc(y_pixels  * sizeof(int16_t));
    lut->y_lut_y  = (int16_t *)malloc(y_pixels  * sizeof(int16_t));
    lut->uv_lut_x = (int16_t *)malloc(uv_pixels * sizeof(int16_t));
    lut->uv_lut_y = (int16_t *)malloc(uv_pixels * sizeof(int16_t));
#endif

    if (!lut->y_lut_x || !lut->y_lut_y || !lut->uv_lut_x || !lut->uv_lut_y) {
        fisheye_lut_destroy(lut);
        return NULL;
    }
    return lut;
}

/**
 * @brief 填充鱼眼LUT的一片数据
 *
 * 计算 [start_y, start_y+num_lines) 行的 Y 坐标映射，
 * 以及对应 UV 行的坐标映射，写入 lut 的预分配缓冲区。
 * 内存占用固定为 lut->batch_lines 行，与 num_lines 无关。
 */
static void fisheye_lut_fill_slice(fisheye_lut_t *lut, int start_y, int num_lines)
{
    int w = lut->width, h = lut->height;
    float strength = lut->strength;
    float cx = w / 2.0f, cy = h / 2.0f;
    float max_r = (w < h) ? cx : cy;

    /* ---- Y LUT: 行 [start_y, start_y+num_lines) ---- */
    for (int dy = 0; dy < num_lines; dy++) {
        int y = start_y + dy;
        int row_off = dy * w;
        for (int x = 0; x < w; x++) {
            float dx = (float)x - cx;
            float dy_f = (float)y - cy;
            float r = sqrtf(dx * dx + dy_f * dy_f);
            float theta = atan2f(dy_f, dx);
            float norm_r = r / max_r;
            float rn = strength * norm_r * norm_r * max_r;

            int sx = (int)(cx + rn * cosf(theta) + 0.5f);
            int sy = (int)(cy + rn * sinf(theta) + 0.5f);
            sx = mirror_coord(sx, w);
            sy = mirror_coord(sy, h);

            lut->y_lut_x[row_off + x] = (int16_t)sx;
            lut->y_lut_y[row_off + x] = (int16_t)sy;
        }
    }

    /* ---- UV LUT: 行 [start_y/2, start_y/2+num_lines/2) ---- */
    int uv_w       = w / 2;
    int uv_start   = start_y / 2;
    int uv_lines   = num_lines / 2;
    for (int dy = 0; dy < uv_lines; dy++) {
        int y = uv_start + dy;
        int row_off = dy * uv_w;
        for (int x = 0; x < uv_w; x++) {
            float dx   = (x * 2.0f + 1.0f) - cx;
            float dy_f = (y * 2.0f + 1.0f) - cy;
            float r = sqrtf(dx * dx + dy_f * dy_f);
            float theta = atan2f(dy_f, dx);
            float norm_r = r / max_r;
            float rn = strength * norm_r * norm_r * max_r;

            int sx = (int)(cx + rn * cosf(theta) + 0.5f);
            int sy = (int)(cy + rn * sinf(theta) + 0.5f);
            sx = mirror_coord(sx, w);
            sy = mirror_coord(sy, h);

            lut->uv_lut_x[row_off + x] = (int16_t)sx;
            lut->uv_lut_y[row_off + x] = (int16_t)sy;
        }
    }
}

/* ======================== 核心处理接口 ======================== */
/**
 * @brief 鱼眼镜头效果处理 (统一分片路径)
 *
 * 按 lut->batch_lines 分片处理。每片先填充 LUT，再逐像素重映射，
 * 最后调用 line_cb 回调（如果已配置）。
 *
 * @param sess     滤镜会话 (可为NULL，无回调时)
 * @param src      源图像
 * @param dst      输出缓冲区 (大小为 batch_lines 行)
 * @param w        图像宽度 (必须为偶数)
 * @param h        图像高度 (必须为偶数)
 * @param fmt      颜色格式
 * @param lut      鱼眼LUT表 (内部填充/消费)
 * @return 0成功，-1失败
 */
int fisheye_process(jlgpu_filter_session_t *sess, const uint8_t *src, uint8_t *dst,
                    int w, int h, lv_color_format_t fmt,
                    fisheye_lut_t *lut)
{
    if (!src || !dst || !lut || w <= 0 || h <= 0 || (w & 1) || (h & 1)) {
        return -1;
    }
    if (lut->width != w || lut->height != h) {
        return -1;
    }

    int batch = lut->batch_lines;
    if (batch <= 0 || (batch & 1)) {
        return -1;
    }

    int has_cb = (sess && sess->line_cb) ? 1 : 0;

    switch (fmt) {
    case LV_COLOR_FORMAT_NV12: {
        const uint8_t *src_y  = src;
        const uint8_t *src_uv = src + (size_t)w * h;
        int uv_w = w / 2, src_uv_stride = w;

        for (int start_y = 0; start_y < h; start_y += batch) {
            int num_lines = ((start_y + batch) <= h) ? batch : (h - start_y);
            if (num_lines & 1) {
                num_lines--;
            }
            if (num_lines <= 0) {
                break;
            }

            /* 1. 填充当前片段的 LUT */
            fisheye_lut_fill_slice(lut, start_y, num_lines);

            /* 2. Y 平面重映射 */
            // 回调模式: 写到 dst 头部; 直接模式: 写到正确的行偏移
            uint8_t *dst_y = has_cb ? dst : (dst + (size_t)start_y * w);
            for (int dy = 0; dy < num_lines; dy++) {
                int row_off = dy * w;
                uint8_t *dst_row = dst_y + (size_t)dy * w;
                for (int x = 0; x < w; x++) {
                    int sx = lut->y_lut_x[row_off + x];
                    int sy = lut->y_lut_y[row_off + x];
                    dst_row[x] = src_y[(size_t)sy * w + sx];
                }
            }

            /* 3. UV 平面重映射 */
            uint8_t *dst_uv = has_cb ? (dst + (size_t)num_lines * w)
                              : (dst + (size_t)w * h + (size_t)(start_y / 2) * uv_w * 2);
            int uv_start = start_y / 2;
            int uv_lines = num_lines / 2;
            for (int dy = 0; dy < uv_lines; dy++) {
                int row_off = dy * uv_w;
                int dst_v = dy;
                for (int x = 0; x < uv_w; x++) {
                    int uv_x = lut->uv_lut_x[row_off + x] / 2;
                    int uv_y = lut->uv_lut_y[row_off + x] / 2;
                    int dst_off = dst_v * uv_w * 2 + x * 2;
                    int src_off = uv_y * src_uv_stride + uv_x * 2;
                    dst_uv[dst_off]     = src_uv[src_off];
                    dst_uv[dst_off + 1] = src_uv[src_off + 1];
                }
            }

            /* 4. 回调 */
            if (has_cb) {
                sess->line_cb(sess, dst, w, h, start_y, num_lines, sess->line_cb_data);
            }
        }
        break;
    }
    case LV_COLOR_FORMAT_YUY2: {
        int stride = w * 2;

        for (int start_y = 0; start_y < h; start_y += batch) {
            int num_lines = ((start_y + batch) <= h) ? batch : (h - start_y);
            if (num_lines <= 0) {
                break;
            }

            /* 1. 填充当前片段的 LUT */
            fisheye_lut_fill_slice(lut, start_y, num_lines);

            /* 2. YUY2 重映射 */
            uint8_t *slice_dst = has_cb ? dst : (dst + (size_t)start_y * stride);
            for (int dy = 0; dy < num_lines; dy++) {
                int row_off = dy * w;
                int doff_base = dy * stride;
                for (int x = 0; x < w; x += 2) {
                    int sx0 = lut->y_lut_x[row_off + x];
                    int sy0 = lut->y_lut_y[row_off + x];
                    int sx1 = lut->y_lut_x[row_off + x + 1];
                    int sy1 = lut->y_lut_y[row_off + x + 1];

                    int doff = doff_base + x * 2;

                    slice_dst[doff]     = src[sy0 * stride + (sx0 / 2) * 4 + (sx0 % 2 == 0 ? 0 : 2)];
                    slice_dst[doff + 2] = src[sy1 * stride + (sx1 / 2) * 4 + (sx1 % 2 == 0 ? 0 : 2)];

                    int soff = sy0 * stride + (sx0 / 2) * 4;
                    slice_dst[doff + 1] = src[soff + 1];
                    slice_dst[doff + 3] = src[soff + 3];
                }
            }

            /* 3. 回调 */
            if (has_cb) {
                sess->line_cb(sess, dst, w, h, start_y, num_lines, sess->line_cb_data);
            }
        }
        break;
    }
    default:
        return -1;
    }
    return 0;
}



/* ======================== 蜂窝阵列效果 ======================== */

honeycomb_lut_t *honeycomb_lut_create(int w, int h, int cell_size, int batch_lines)
{
    if (w <= 0 || h <= 0 || batch_lines <= 0 || (w & 1) || (h & 1) || (batch_lines & 1)) {
        return NULL;
    }
    if (batch_lines > h) {
        batch_lines = h;
    }
    if (cell_size < 2) {
        cell_size = 2;
    }

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    honeycomb_lut_t *lut = (honeycomb_lut_t *)malloc_psram(sizeof(honeycomb_lut_t));
#else
    honeycomb_lut_t *lut = (honeycomb_lut_t *)malloc(sizeof(honeycomb_lut_t));
#endif
    if (!lut) {
        return NULL;
    }
    memset(lut, 0, sizeof(honeycomb_lut_t));

    lut->width       = w;
    lut->height      = h;
    lut->cell_size   = cell_size;
    lut->batch_lines = batch_lines;

    size_t pixels = (size_t)w * batch_lines;

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
    lut->sample_x = (int16_t *)malloc_psram(pixels * sizeof(int16_t));
    lut->sample_y = (int16_t *)malloc_psram(pixels * sizeof(int16_t));
#else
    lut->sample_x = (int16_t *)malloc(pixels * sizeof(int16_t));
    lut->sample_y = (int16_t *)malloc(pixels * sizeof(int16_t));
#endif

    if (!lut->sample_x || !lut->sample_y) {
        honeycomb_lut_destroy(lut);
        return NULL;
    }
    return lut;
}

void honeycomb_lut_destroy(honeycomb_lut_t *lut)
{
    if (lut) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        free_psram(lut->sample_x);
        free_psram(lut->sample_y);
        free_psram(lut);
#else
        free(lut->sample_x);
        free(lut->sample_y);
        free(lut);
#endif
    }
}

/**
 * @brief 填充蜂窝LUT的一片数据
 *
 * 计算 [start_y, start_y+num_lines) 行的六边形蜂窝采样坐标，
 * 写入 lut 的预分配缓冲区。
 */
static void honeycomb_lut_fill_slice(honeycomb_lut_t *lut, int start_y, int num_lines)
{
    int w = lut->width, h = lut->height;
    float cell_size = (float)lut->cell_size;
    float sqrt3 = 1.7320508f;
    float img_cx = w / 2.0f;
    float img_cy = h / 2.0f;

    for (int dy = 0; dy < num_lines; dy++) {
        int py = start_y + dy;
        int row_off = dy * w;
        for (int px = 0; px < w; px++) {
            /* 严格的六边形坐标舍入 (Hex Rounding) */
            float f_q    = (px * 2.0f / 3.0f) / cell_size;
            float f_r_ax = (-px / 3.0f + sqrt3 / 3.0f * py) / cell_size;
            float f_s    = -f_q - f_r_ax;

            int q    = (int)roundf(f_q);
            int r_ax = (int)roundf(f_r_ax);
            int s    = (int)roundf(f_s);

            float q_diff = fabsf((float)q - f_q);
            float r_diff = fabsf((float)r_ax - f_r_ax);
            float s_diff = fabsf((float)s - f_s);

            if (q_diff > r_diff && q_diff > s_diff) {
                q = -r_ax - s;
            } else if (r_diff > s_diff) {
                r_ax = -q - s;
            }

            /* 六边形中心坐标 */
            float best_cx_f = cell_size * (1.5f * q);
            float best_cy_f = cell_size * (sqrt3 * 0.5f * q + sqrt3 * r_ax);

            /* 平铺采样坐标：以图像中心为基准 + 相对偏移 */
            int sx = (int)(img_cx + (px - best_cx_f) + 0.5f);
            int sy = (int)(img_cy + (py - best_cy_f) + 0.5f);

            /* 边界钳位 */
            if (sx < 0) {
                sx = 0;
            } else if (sx >= w) {
                sx = w - 1;
            }
            if (sy < 0) {
                sy = 0;
            } else if (sy >= h) {
                sy = h - 1;
            }

            lut->sample_x[row_off + px] = (int16_t)sx;
            lut->sample_y[row_off + px] = (int16_t)sy;
        }
    }
}

/**
 * @brief 蜂窝阵列效果处理 (统一分片路径)
 *
 * 按 lut->batch_lines 分片处理。每片先填充 LUT，再逐像素重映射，
 * 最后调用 line_cb 回调（如果已配置）。
 */
int honeycomb_process(jlgpu_filter_session_t *sess, const uint8_t *src, uint8_t *dst,
                      int w, int h, lv_color_format_t fmt,
                      honeycomb_lut_t *lut)
{
    if (!src || !dst || !lut || w <= 0 || h <= 0) {
        return -1;
    }
    if (lut->width != w || lut->height != h) {
        return -1;
    }

    int batch = lut->batch_lines;
    if (batch <= 0 || (batch & 1)) {
        return -1;
    }

    int has_cb = (sess && sess->line_cb) ? 1 : 0;
    size_t y_size = (size_t)w * h;

    switch (fmt) {
    case LV_COLOR_FORMAT_NV12: {
        const uint8_t *src_y  = src;
        const uint8_t *src_uv = src + y_size;
        int uv_w = w / 2, src_uv_stride = w;

        for (int start_y = 0; start_y < h; start_y += batch) {
            int num_lines = ((start_y + batch) <= h) ? batch : (h - start_y);
            if (num_lines & 1) {
                num_lines--;
            }
            if (num_lines <= 0) {
                break;
            }

            /* 1. 填充当前片段的 LUT */
            honeycomb_lut_fill_slice(lut, start_y, num_lines);

            /* 2. Y 平面重映射 */
            uint8_t *dst_y = has_cb ? dst : (dst + (size_t)start_y * w);
            for (int dy = 0; dy < num_lines; dy++) {
                int row_off = dy * w;
                uint8_t *dst_row = dst_y + (size_t)dy * w;
                for (int x = 0; x < w; x++) {
                    int sx = lut->sample_x[row_off + x];
                    int sy = lut->sample_y[row_off + x];
                    dst_row[x] = src_y[(size_t)sy * w + sx];
                }
            }

            /* 3. UV 平面重映射 */
            uint8_t *dst_uv = has_cb ? (dst + (size_t)num_lines * w)
                              : (dst + y_size + (size_t)(start_y / 2) * uv_w * 2);
            int uv_start = start_y / 2;
            int uv_lines = num_lines / 2;
            for (int dy = 0; dy < uv_lines; dy++) {
                int dst_v = dy;
                int y_abs = uv_start + dy;
                for (int x = 0; x < uv_w; x++) {
                    /* 使用中心像素 (2x+1, 2y+1) 的采样坐标 */
                    int abs_row = y_abs * 2 + 1;
                    int idx = (abs_row - start_y) * w + (x * 2 + 1);
                    int sx = lut->sample_x[idx];
                    int sy = lut->sample_y[idx];
                    int src_off = (sy / 2) * src_uv_stride + (sx / 2) * 2;
                    int dst_off = dst_v * uv_w * 2 + x * 2;
                    dst_uv[dst_off]     = src_uv[src_off];
                    dst_uv[dst_off + 1] = src_uv[src_off + 1];
                }
            }

            /* 4. 回调 */
            if (has_cb) {
                sess->line_cb(sess, dst, w, h, start_y, num_lines, sess->line_cb_data);
            }
        }
        break;
    }
    case LV_COLOR_FORMAT_YUY2: {
        int stride = w * 2;

        for (int start_y = 0; start_y < h; start_y += batch) {
            int num_lines = ((start_y + batch) <= h) ? batch : (h - start_y);
            if (num_lines <= 0) {
                break;
            }

            /* 1. 填充当前片段的 LUT */
            honeycomb_lut_fill_slice(lut, start_y, num_lines);

            /* 2. YUY2 重映射 */
            uint8_t *slice_dst = has_cb ? dst : (dst + (size_t)start_y * stride);
            for (int dy = 0; dy < num_lines; dy++) {
                int row_off = dy * w;
                int doff_base = dy * stride;
                for (int x = 0; x < w; x += 2) {
                    int sx0 = lut->sample_x[row_off + x];
                    int sy0 = lut->sample_y[row_off + x];
                    int sx1 = lut->sample_x[row_off + x + 1];
                    int sy1 = lut->sample_y[row_off + x + 1];

                    int doff = doff_base + x * 2;

                    slice_dst[doff]     = src[sy0 * stride + (sx0 / 2) * 4 + (sx0 % 2 == 0 ? 0 : 2)];
                    slice_dst[doff + 2] = src[sy1 * stride + (sx1 / 2) * 4 + (sx1 % 2 == 0 ? 0 : 2)];

                    int soff = sy0 * stride + (sx0 / 2) * 4;
                    slice_dst[doff + 1] = src[soff + 1];
                    slice_dst[doff + 3] = src[soff + 3];
                }
            }

            /* 3. 回调 */
            if (has_cb) {
                sess->line_cb(sess, dst, w, h, start_y, num_lines, sess->line_cb_data);
            }

        }
        break;
    }
    default:
        return -1;
    }
    return 0;
}

#endif
