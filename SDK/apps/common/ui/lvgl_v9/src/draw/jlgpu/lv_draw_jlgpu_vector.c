/**
 * @file lv_draw_jlgpu_vector.c
 *
 * JLGPU vector drawing: SOLID fill and SOLID stroke.
 * Converts `lv_vector_path_t` (as built by the SVG parser) into the JLGPU
 * `jlgpu_vec_t` segment stream and submits it to the hardware.
 *
 * The GPU has no hardware stroking, so the stroke outline is generated on the
 * CPU first (see `stroke_build()`) and then filled with the non-zero rule.
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

/* 描边轮廓的几何精度与规模上限 */
#define JLGPU_STROKE_EPS            0.01f   /* 小于该长度视为退化点/零长线段 */
#define JLGPU_STROKE_SEG_PX         3.0f    /* 曲线展平时每段的目标长度(像素) */
#define JLGPU_STROKE_MIN_DIV        4       /* 单条贝塞尔曲线的最少细分数 */
#define JLGPU_STROKE_MAX_DIV        32      /* 单条贝塞尔曲线的最多细分数 */
#define JLGPU_STROKE_CIRCLE_MIN_SEG 8       /* 圆(圆角/圆帽)的最少边数 */
#define JLGPU_STROKE_CIRCLE_MAX_SEG 24      /* 圆的最多边数 */
#define JLGPU_STROKE_CIRCLE_PX_SEG  2.0f    /* 半径每多少像素增加一条圆边 */
#define JLGPU_STROKE_DEFAULT_MITER  4.0f    /* 缺省尖角限制(与 SVG 缺省一致) */
#define JLGPU_STROKE_MAX_MITER      16.0f   /* 尖角限制上限, 防止尖角被拉得过长 */
#define JLGPU_STROKE_FLAT_INIT_PTS  64      /* 展平点缓存的初始容量(点数) */

/*
 * jlgpu_vec_t.total_len 是 int16_t, 且 jlgpu_path_*_to() 不做溢出保护。
 * 描边轮廓由大量独立多边形拼成, 段数远多于同一条路径的填充, 超过上限后
 * total_len 会回绕并破坏整个段流, 所以在到达硬上限前主动停止输出。
 */
#define JLGPU_STROKE_MAX_PATH_LEN   (INT16_MAX - 64)

/* 一个 edge 占 2 个 word, 见 jlgpu_framework.h 的 jlgpu_global_path_t.e_data 说明 */
#define JLGPU_EDGE_BYTES            8
/* cbuf_split = 0 时 cbuf 按 32 行分行, 见 jlgpu_global_path_t.cbuf_split 说明 */
#define JLGPU_CBUF_SPLIT_ROWS       32
/* 包围盒取整成整数区域时的外扩量, 覆盖光栅化取整误差 */
#define JLGPU_BBOX_MARGIN           1

/**********************
 *      TYPEDEFS
 **********************/

/* 展平后的子路径: 只剩直线段, 坐标已经是变换后的屏幕坐标 */
typedef struct {
    float *pts;             /* x0, y0, x1, y1, ... 相邻两点为一段 */
    uint32_t point_cnt;     /* 已使用的点数 */
    uint32_t capacity;      /* 已申请的点数容量 */
    bool closed;            /* 子路径是否以 CLOSE 结束 */
    bool is_oom;            /* 展平点缓存申请失败, 本子路径不再输出 */
} jlgpu_flat_path_t;

/* 描边参数, 线宽已按矩阵换算到屏幕像素 */
typedef struct {
    float half_width;               /* 线宽的一半 */
    lv_vector_stroke_cap_t cap;     /* 端点样式 */
    lv_vector_stroke_join_t join;   /* 拐角样式 */
    float miter_limit;              /* 尖角长度上限, 相对于半线宽 */
    const float *dash;              /* 虚线样式数组, NULL 表示实线 */
    uint32_t dash_cnt;              /* 虚线样式元素个数 */
} jlgpu_stroke_style_t;

/*
 * 立即绘制(目标不是显示缓冲区, 例如 canvas/layer)的批量提交上下文。
 *
 * 旧实现是"每条 path 一次完整 GPU 流程": ctx init(拆 GPU↔DBI 联动 + JLGXDMA_init
 * + malloc/memset context) + jlgpu_init + cbuf/ebuf malloc/memset + 阻塞 jlgpu_run
 * + teardown。一块地图瓦片约 282 条 path, 每帧 20~30 块瓦片, 相当于每帧约 7000 次
 * GPU 复位周期 —— 这才是 FlatBuffer 地图慢的主因, 而不是 FlatBuffer 解码。
 *
 * 这里把一次 lv_draw_vector 任务(即一块瓦片)的所有 path 攒成一条本地任务链,
 * 共用一套 GPU context / cbuf / ebuf, 最后用一次 jlgpu_run 跑完整批。
 */
typedef struct {
    lv_jlgpu_ctx_t ctx;         /* 整批共用的 GPU context, 只 init/uninit 一次 */
    bool is_hw_inited;          /* jlgpu_init 只调一次的守卫 */
    jlgpu_surface_t surface;    /* 只跟 layer 有关, 整批常量, 算一次 */
    jlgpu_global_path_t gpath;  /* 整批共用的 global path(含 cbuf/ebuf 指针) */
    uint8_t *cbuf;              /* mask 缓冲区, 整批共用, 只清一次 */
    uint32_t cbuf_size;
    uint8_t *ebuf;              /* 边缘缓冲区, 按整批最大 edge_num 按需增长 */
    uint32_t ebuf_size;
    void *head;                 /* 本地任务链头 */
    void *cur;                  /* 本地任务链尾 */
    uint32_t cnt;               /* 本批已挂节点数 */
    int32_t max_edge;           /* 本批 edge_num 最大值 */
} jlgpu_vec_batch_t;

typedef struct {
    lv_draw_task_t *t;
    lv_draw_jlgpu_unit_t *u;
    const lv_area_t *coords;
    jlgpu_flat_path_t flat;     /* 展平点缓存, 在所有子任务间复用 */
    jlgpu_flat_path_t dash;     /* 虚线切分缓存 */
    jlgpu_vec_batch_t *batch;   /* 批量提交上下文, NULL 表示逐条提交(旧行为) */
} jlgpu_vector_draw_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void task_draw_cb(void *user_data, const lv_vector_path_t *path, const lv_vector_path_ctx_t *ctx);
static void lv_path_to_jlgpu(jlgpu_vec_t *dest, const lv_vector_path_t *src, const lv_matrix_t *matrix);
static uint32_t lv_color32_to_argb(lv_color32_t color, lv_opa_t opa);
static jlgpu_fillrule_t lv_fill_to_jlgpu(lv_vector_fill_t rule);
static void jlgpu_vector_fill_submit(jlgpu_vector_draw_ctx_t *uc,
                                     const lv_area_t *blend_area,
                                     jlgpu_vec_t *path,
                                     uint32_t argb_color,
                                     jlgpu_fillrule_t fill_rule);
static void jlgpu_vector_fill_path(jlgpu_vector_draw_ctx_t *uc,
                                   const lv_area_t *blend_area,
                                   const lv_vector_path_t *path,
                                   const lv_vector_path_ctx_t *ctx);
static void jlgpu_vector_stroke_path(lv_draw_jlgpu_unit_t *u, jlgpu_vector_draw_ctx_t *uc,
                                     const lv_vector_path_t *path,
                                     const lv_vector_path_ctx_t *ctx,
                                     const lv_area_t *stroke_area);
static bool jlgpu_vec_batch_open(jlgpu_vec_batch_t *batch, lv_layer_t *layer, lv_draw_buf_t *draw_buf);
static void jlgpu_vec_batch_close(jlgpu_vec_batch_t *batch);
static void jlgpu_vec_batch_add(jlgpu_vec_batch_t *batch, void *task, int32_t edge_num);
static void jlgpu_vec_batch_flush(jlgpu_vec_batch_t *batch);
static void jlgpu_vec_batch_submit(jlgpu_vec_batch_t *batch, jlgpu_render_param_t *render_params,
                                   jlgpu_draw_fill_t *draw_fill_params, jlgpu_mask_param_t *mask_params,
                                   const jlgpu_vec_t *path);
static void jlgpu_vector_fill_submit_single(jlgpu_vector_draw_ctx_t *uc,
        jlgpu_draw_fill_t *draw_fill_params,
        jlgpu_mask_param_t *mask_params,
        jlgpu_render_param_t *render_params,
        const jlgpu_vec_t *path,
        uint8_t *dest_buf, uint8_t out_format, uint32_t fb_width);
static jlgpu_area_t jlgpu_area_from_lv(const lv_area_t *area);
static bool jlgpu_path_area_estimate(const lv_vector_path_t *path, const lv_matrix_t *matrix,
                                     const lv_area_t *clip, lv_area_t *out);
static bool jlgpu_bbox_to_area(const jlvec_bbox_t *bbox, const lv_area_t *clip, lv_area_t *out);
static bool stroke_area_expand(const lv_area_t *src, float half_width, const lv_draw_task_t *t,
                               const lv_area_t *coords, lv_area_t *out);
static float stroke_matrix_scale(const lv_matrix_t *matrix);
static bool stroke_style_init(jlgpu_stroke_style_t *style, const lv_vector_stroke_dsc_t *dsc,
                              const lv_matrix_t *matrix);
static void stroke_flat_path_deinit(jlgpu_flat_path_t *flat);
static bool stroke_flat_push(jlgpu_flat_path_t *flat, float x, float y);
static void stroke_flat_subpath_start(jlgpu_flat_path_t *flat, const lv_fpoint_t *cur);
static int stroke_flatten_div(float len);
static void stroke_flatten_quad(jlgpu_flat_path_t *flat, const lv_fpoint_t *p0,
                                const lv_fpoint_t *c, const lv_fpoint_t *p1);
static void stroke_flatten_cubic(jlgpu_flat_path_t *flat, const lv_fpoint_t *p0,
                                 const lv_fpoint_t *c1, const lv_fpoint_t *c2,
                                 const lv_fpoint_t *p1);
static uint32_t stroke_op_point_cnt(lv_vector_path_op_t op);
static void stroke_next_points(const lv_fpoint_t **point, lv_fpoint_t *out, uint32_t cnt,
                               const lv_matrix_t *matrix, bool has_matrix);
static void stroke_flatten_op(jlgpu_flat_path_t *flat, lv_vector_path_op_t op,
                              const lv_fpoint_t *from, const lv_fpoint_t *pts);
static void stroke_build(jlgpu_vec_t *dest, const lv_vector_path_t *src,
                         const lv_matrix_t *matrix, const jlgpu_stroke_style_t *style,
                         jlgpu_flat_path_t *flat, jlgpu_flat_path_t *dash_buf);
static void stroke_flush_subpath(jlgpu_vec_t *dest, jlgpu_flat_path_t *flat,
                                 jlgpu_flat_path_t *scratch, const jlgpu_stroke_style_t *style);
static void stroke_emit_run(jlgpu_vec_t *dest, const float *pts, uint32_t point_cnt,
                            bool closed, const jlgpu_stroke_style_t *style);
static void stroke_emit_poly(jlgpu_vec_t *dest, const float *xy, int point_cnt);
static void stroke_emit_circle(jlgpu_vec_t *dest, float cx, float cy, float radius);
static void stroke_emit_segment(jlgpu_vec_t *dest, const float *p0, const float *p1,
                                float half_width);
static void stroke_emit_join_sides(jlgpu_vec_t *dest, const float *cur, const float *n1,
                                   const float *n2, const float *m, bool has_miter);
static void stroke_emit_join(jlgpu_vec_t *dest, const float *prev, const float *cur,
                             const float *next, const jlgpu_stroke_style_t *style);
static void stroke_emit_cap(jlgpu_vec_t *dest, const float *p, const float *dir,
                            const jlgpu_stroke_style_t *style);
static void stroke_emit_dashed(jlgpu_vec_t *dest, const jlgpu_flat_path_t *flat, uint32_t point_cnt,
                               jlgpu_flat_path_t *scratch, const jlgpu_stroke_style_t *style);
static float stroke_dash_advance(const float *dash, uint32_t dash_cnt, uint32_t *idx, bool *is_on);
static float stroke_dash_total(const float *dash, uint32_t dash_cnt);
static void stroke_dash_flush(jlgpu_vec_t *dest, jlgpu_flat_path_t *scratch,
                              const jlgpu_stroke_style_t *style);

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
    user_ctx.batch = NULL;
    lv_memset(&user_ctx.flat, 0, sizeof(user_ctx.flat));
    lv_memset(&user_ctx.dash, 0, sizeof(user_ctx.dash));

    /* 目标不是显示缓冲区(canvas/layer)时开一批: 整批共用一套 GPU context,
     * 所有 path 攒成一条链后一次 jlgpu_run, 而不是每条 path 一次。 */
    jlgpu_vec_batch_t batch;
    if (jlgpu_vec_batch_open(&batch, layer, layer->draw_buf)) {
        user_ctx.batch = &batch;
    }

    /* Iterate and destroy the accumulated vector subtasks */
    lv_vector_for_each_destroy_tasks(dsc->task_list, task_draw_cb, &user_ctx);

    if (user_ctx.batch != NULL) {
        jlgpu_vec_batch_close(&batch);
    }

    stroke_flat_path_deinit(&user_ctx.flat);
    stroke_flat_path_deinit(&user_ctx.dash);

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

        jlgpu_area_t render_area = jlgpu_area_from_lv(&blend_area);
        lv_jlgpu_path_set_bez_div(jp, LV_MIN(jlgpu_compute_div(render_area), (int)JLGPU_GLOBAL_MAX_BEZ_DIV));

        jlgpu_path_append_rect(jp,
                               (float)blend_area.x1, (float)blend_area.y1,
                               (float)lv_area_get_width(&blend_area), (float)lv_area_get_height(&blend_area),
                               0.0f);
        jlgpu_path_end(jp);

        uint32_t argb = lv_color32_to_argb(ctx->fill_dsc.color, LV_OPA_MIX2(ctx->fill_dsc.opa, t->opa));
        jlgpu_vector_fill_submit(uc, &blend_area, jp, argb, JLGPU_NON_ZERO);
        return;
    }

    if (ctx->fill_dsc.opa == LV_OPA_TRANSP && ctx->stroke_dsc.opa == LV_OPA_TRANSP) {
        // printf("[JLGPU-VEC] cb: fill+stroke both transparent, bail\n");
        return;
    }

    /* 先填充后描边, 与 SVG 的绘制顺序一致 */
    if (ctx->fill_dsc.opa) {
        jlgpu_vector_fill_path(uc, &blend_area, path, ctx);
    }

    if (ctx->stroke_dsc.opa) {
        lv_area_t stroke_area;
        float half_width = ctx->stroke_dsc.width * stroke_matrix_scale(&ctx->matrix) * 0.5f;

        /* 描边轮廓会超出路径自身的包围盒, 渲染区域要按半线宽外扩 */
        if (stroke_area_expand(&blend_area, half_width, t, uc->coords, &stroke_area)) {
            jlgpu_vector_stroke_path(u, uc, path, ctx, &stroke_area);
        }
    }
}

/* 提交一条路径的 SOLID 填充 */
static void jlgpu_vector_fill_path(jlgpu_vector_draw_ctx_t *uc,
                                   const lv_area_t *blend_area,
                                   const lv_vector_path_t *path,
                                   const lv_vector_path_ctx_t *ctx)
{
    if (ctx->fill_dsc.style != LV_VECTOR_DRAW_STYLE_SOLID) {
        //LV_LOG_WARN("jlgpu vector: only SOLID fill is supported yet (style=%d)", ctx->fill_dsc.style);
        return;
    }

    /* bez_div 必须在建路径之前定: jlgpu_path_*_to() 在写入时就按它展开曲线段数。
     * 这里用路径自身的包围盒而不是整块瓦片, 否则面积一大就饱和到最大细分数。 */
    lv_area_t tight;
#if JLGPU_VEC_BATCH_EN
    if (!jlgpu_path_area_estimate(path, &ctx->matrix, blend_area, &tight)) {
        return;
    }
#else
    tight = *blend_area;
#endif

    jlgpu_vec_t *jp = lv_jlgpu_path_get(uc->u);
    jlgpu_area_t render_area = jlgpu_area_from_lv(&tight);
    lv_jlgpu_path_set_bez_div(jp, LV_MIN(jlgpu_compute_div(render_area), (int)JLGPU_GLOBAL_MAX_BEZ_DIV));

    lv_path_to_jlgpu(jp, path, &ctx->matrix);
    jlgpu_path_end(jp);

#if JLGPU_VEC_BATCH_EN
    /* 建完才有精确包围盒(含曲线实际展开范围), 用它再把渲染区收到最紧 */
    if (!jlgpu_bbox_to_area(&jp->path_bbox, &tight, &tight)) {
        return;
    }
#endif

    uint32_t argb = lv_color32_to_argb(ctx->fill_dsc.color, LV_OPA_MIX2(ctx->fill_dsc.opa, uc->t->opa));
    jlgpu_vector_fill_submit(uc, &tight, jp, argb, lv_fill_to_jlgpu(ctx->fill_dsc.fill_rule));
}

/* 提交一条路径的 SOLID 描边: 轮廓在 CPU 上生成, 再当作 Non-Zero 填充提交 */
static void jlgpu_vector_stroke_path(lv_draw_jlgpu_unit_t *u, jlgpu_vector_draw_ctx_t *uc,
                                     const lv_vector_path_t *path,
                                     const lv_vector_path_ctx_t *ctx,
                                     const lv_area_t *stroke_area)
{
    jlgpu_stroke_style_t style;

    if (ctx->stroke_dsc.style != LV_VECTOR_DRAW_STYLE_SOLID) {
        //LV_LOG_WARN("jlgpu vector: only SOLID stroke is supported yet (style=%d)", ctx->stroke_dsc.style);
        return;
    }

    if (!stroke_style_init(&style, &ctx->stroke_dsc, &ctx->matrix)) {
        return;
    }

    jlgpu_vec_t *jp = lv_jlgpu_path_get(u);

    /* 同 fill: bez_div 用描边自身的可见区域来定, 不用整块瓦片 */
    jlgpu_area_t render_area = jlgpu_area_from_lv(stroke_area);
    lv_jlgpu_path_set_bez_div(jp, LV_MIN(jlgpu_compute_div(render_area), (int)JLGPU_GLOBAL_MAX_BEZ_DIV));

    stroke_build(jp, path, &ctx->matrix, &style, &uc->flat, &uc->dash);
    jlgpu_path_end(jp);

    if (jp->edge_num == 0) {
        /* 全部线段都退化(例如零长路径), 没有几何可画 */
        return;
    }

    /* path_bbox 由 CPU 生成的外扩轮廓给出, 已含线宽/端点/拐角, 可直接收窄渲染区 */
    lv_area_t tight = *stroke_area;
#if JLGPU_VEC_BATCH_EN
    if (!jlgpu_bbox_to_area(&jp->path_bbox, stroke_area, &tight)) {
        return;
    }
#endif

    uint32_t argb = lv_color32_to_argb(ctx->stroke_dsc.color, LV_OPA_MIX2(ctx->stroke_dsc.opa, uc->t->opa));
    jlgpu_vector_fill_submit(uc, &tight, jp, argb, JLGPU_NON_ZERO);
}

/* 把渲染区域按半线宽外扩, 并夹到任务可见范围内(越界会导致 GPU 写到窗口外) */
static bool stroke_area_expand(const lv_area_t *src, float half_width, const lv_draw_task_t *t,
                               const lv_area_t *coords, lv_area_t *out)
{
    int32_t expand = (int32_t)MATH_FABSF(half_width) + 1;
    lv_area_t area = *src;

    area.x1 -= expand;
    area.y1 -= expand;
    area.x2 += expand;
    area.y2 += expand;

    if (!lv_area_intersect(out, &area, coords)) {
        return false;
    }
    if (!lv_area_intersect(out, out, &t->clip_area)) {
        return false;
    }
    return lv_area_intersect(out, out, &t->target_layer->buf_area);
}

/* 描边宽度按矩阵的缩放系数换算到屏幕像素(SVG 的线宽是用户单位) */
static float stroke_matrix_scale(const lv_matrix_t *matrix)
{
    if (lv_matrix_is_identity(matrix)) {
        return 1.0f;
    }

    float det = matrix->m[0][0] * matrix->m[1][1] - matrix->m[0][1] * matrix->m[1][0];
    return MATH_SQRTF(MATH_FABSF(det));
}

/* 组装描边参数, 返回 false 表示线宽为 0, 不需要描边 */
static bool stroke_style_init(jlgpu_stroke_style_t *style, const lv_vector_stroke_dsc_t *dsc,
                              const lv_matrix_t *matrix)
{
    float width = dsc->width * stroke_matrix_scale(matrix);

    if (width <= JLGPU_STROKE_EPS) {
        return false;
    }

    style->half_width = width * 0.5f;
    style->cap = dsc->cap;
    style->join = dsc->join;
    style->miter_limit = (dsc->miter_limit > 0) ? dsc->miter_limit : JLGPU_STROKE_DEFAULT_MITER;
    if (style->miter_limit > JLGPU_STROKE_MAX_MITER) {
        style->miter_limit = JLGPU_STROKE_MAX_MITER;
    }

    style->dash_cnt = lv_array_size(&dsc->dash_pattern);
    style->dash = (style->dash_cnt > 0) ? (const float *)lv_array_front(&dsc->dash_pattern) : NULL;
    if (style->dash == NULL) {
        style->dash_cnt = 0;
    }
    return true;
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

/* ---------------------------------------------------------------------------
 * 描边轮廓生成
 *
 * GPU 没有硬件描边, 这里在 CPU 上把路径描成轮廓多边形, 再交给硬件按
 * Non-Zero 规则填充。轮廓由三部分组成, 每部分都是独立的凸多边形:
 *   1. 每段线的线宽矩形(平头端帽)
 *   2. 每个拐角两侧的补角(斜角三角形, 尖角样式再加一个四边形)
 *   3. 开放路径两端的端帽(方头补一个矩形, 圆头补一个圆)
 * 所有多边形输出时统一绕向, 因此重叠部分在 Non-Zero 规则下累加而不是
 * 互相抵消, 整体结果就是它们的并集; 内侧那一份补角落在矩形并集内, 直接
 * 生成也不会画错。
 * ------------------------------------------------------------------------- */

/* Emit a convex polygon. `xy` is x0,y0,x1,y1,... with `point_cnt` points. */
static void stroke_emit_poly(jlgpu_vec_t *dest, const float *xy, int point_cnt)
{
    float area2 = 0.0f;

    /* 每个多边形都以 MOVE_TO 开头, 从多边形边界上截断不会破坏段流, 只是少画一部分 */
    if (dest->total_len > JLGPU_STROKE_MAX_PATH_LEN) {
        return;
    }

    for (int i = 0; i < point_cnt; i++) {
        int j = (i + 1) % point_cnt;
        area2 += xy[2 * i] * xy[2 * j + 1] - xy[2 * j] * xy[2 * i + 1];
    }

    jlgpu_path_move_to(dest, xy[0], xy[1]);
    if (area2 < 0.0f) {
        for (int i = point_cnt - 1; i >= 1; i--) {
            jlgpu_path_line_to(dest, xy[2 * i], xy[2 * i + 1]);
        }
    } else {
        for (int i = 1; i < point_cnt; i++) {
            jlgpu_path_line_to(dest, xy[2 * i], xy[2 * i + 1]);
        }
    }
    jlgpu_path_line_to(dest, xy[0], xy[1]);
}

/* Emit a circle as a regular polygon */
static void stroke_emit_circle(jlgpu_vec_t *dest, float cx, float cy, float radius)
{
    float xy[2 * JLGPU_STROKE_CIRCLE_MAX_SEG];
    int seg = (int)(radius * JLGPU_STROKE_CIRCLE_PX_SEG) + JLGPU_STROKE_CIRCLE_MIN_SEG;

    if (seg > JLGPU_STROKE_CIRCLE_MAX_SEG) {
        seg = JLGPU_STROKE_CIRCLE_MAX_SEG;
    }

    for (int i = 0; i < seg; i++) {
        float angle = MATH_TWO_PI * (float)i / (float)seg;
        xy[2 * i] = cx + radius * MATH_COSF(angle);
        xy[2 * i + 1] = cy + radius * MATH_SINF(angle);
    }
    stroke_emit_poly(dest, xy, seg);
}

/* Emit the line-width rectangle of one segment */
static void stroke_emit_segment(jlgpu_vec_t *dest, const float *p0, const float *p1,
                                float half_width)
{
    float dx = p1[0] - p0[0];
    float dy = p1[1] - p0[1];
    float len = MATH_SQRTF(dx * dx + dy * dy);
    float quad[8];

    if (len < JLGPU_STROKE_EPS) {
        return;
    }

    float nx = -dy / len * half_width;
    float ny = dx / len * half_width;

    quad[0] = p0[0] + nx;
    quad[1] = p0[1] + ny;
    quad[2] = p1[0] + nx;
    quad[3] = p1[1] + ny;
    quad[4] = p1[0] - nx;
    quad[5] = p1[1] - ny;
    quad[6] = p0[0] - nx;
    quad[7] = p0[1] - ny;
    stroke_emit_poly(dest, quad, 4);
}

/* 在拐角两侧各补一块: 必定有斜角三角形, 尖角样式再加一个四边形 */
static void stroke_emit_join_sides(jlgpu_vec_t *dest, const float *cur, const float *n1,
                                   const float *n2, const float *m, bool has_miter)
{
    for (int s = 1; s >= -1; s -= 2) {
        float bevel[6] = {
            cur[0], cur[1],
            cur[0] + s *n1[0], cur[1] + s *n1[1],
            cur[0] + s *n2[0], cur[1] + s *n2[1]
        };
        stroke_emit_poly(dest, bevel, 3);

        if (has_miter) {
            float quad[8] = {
                cur[0], cur[1],
                cur[0] + s *n1[0], cur[1] + s *n1[1],
                cur[0] + s *m[0], cur[1] + s *m[1],
                cur[0] + s *n2[0], cur[1] + s *n2[1]
            };
            stroke_emit_poly(dest, quad, 4);
        }
    }
}

/* Emit the join geometry at `cur` between segment prev->cur and cur->next */
static void stroke_emit_join(jlgpu_vec_t *dest, const float *prev, const float *cur,
                             const float *next, const jlgpu_stroke_style_t *style)
{
    float d1x = cur[0] - prev[0];
    float d1y = cur[1] - prev[1];
    float d2x = next[0] - cur[0];
    float d2y = next[1] - cur[1];
    float l1 = MATH_SQRTF(d1x * d1x + d1y * d1y);
    float l2 = MATH_SQRTF(d2x * d2x + d2y * d2y);
    float hw = style->half_width;
    float n1[2];
    float n2[2];
    float m[2] = { 0.0f, 0.0f };
    bool has_miter = false;

    if (l1 < JLGPU_STROKE_EPS || l2 < JLGPU_STROKE_EPS) {
        return;
    }
    d1x /= l1;
    d1y /= l1;
    d2x /= l2;
    d2y /= l2;

    float cross = d1x * d2y - d1y * d2x;
    if (MATH_FABSF(cross) < JLGPU_STROKE_EPS && (d1x * d2x + d1y * d2y) > 0.0f) {
        return;     /* 直行, 没有缺口需要补 */
    }

    if (style->join == LV_VECTOR_STROKE_JOIN_ROUND) {
        stroke_emit_circle(dest, cur[0], cur[1], hw);
        return;
    }

    n1[0] = -d1y * hw;
    n1[1] = d1x * hw;
    n2[0] = -d2y * hw;
    n2[1] = d2x * hw;

    if (style->join == LV_VECTOR_STROKE_JOIN_MITER && MATH_FABSF(cross) >= JLGPU_STROKE_EPS) {
        /* 两条外扩边线的交点即尖角顶点(相对 cur 的偏移), 两侧互为反向 */
        float u = ((n2[0] - n1[0]) * d2y - (n2[1] - n1[1]) * d2x) / cross;
        m[0] = n1[0] + u * d1x;
        m[1] = n1[1] + u * d1y;
        has_miter = (MATH_SQRTF(m[0] * m[0] + m[1] * m[1]) <= style->miter_limit * hw);
    }

    stroke_emit_join_sides(dest, cur, n1, n2, m, has_miter);
}

/* Emit the cap at `p`, `dir` points from the line outwards */
static void stroke_emit_cap(jlgpu_vec_t *dest, const float *p, const float *dir,
                            const jlgpu_stroke_style_t *style)
{
    float len = MATH_SQRTF(dir[0] * dir[0] + dir[1] * dir[1]);
    float hw = style->half_width;
    float quad[8];

    if (len < JLGPU_STROKE_EPS) {
        return;
    }

    float dx = dir[0] / len;
    float dy = dir[1] / len;

    if (style->cap == LV_VECTOR_STROKE_CAP_ROUND) {
        stroke_emit_circle(dest, p[0], p[1], hw);
        return;
    }
    if (style->cap != LV_VECTOR_STROKE_CAP_SQUARE) {
        return;     /* 平头端: 线宽矩形本身就是平头, 不用补 */
    }

    float nx = -dy * hw;
    float ny = dx * hw;
    float ex = p[0] + dx * hw;
    float ey = p[1] + dy * hw;

    quad[0] = p[0] + nx;
    quad[1] = p[1] + ny;
    quad[2] = ex + nx;
    quad[3] = ey + ny;
    quad[4] = ex - nx;
    quad[5] = ey - ny;
    quad[6] = p[0] - nx;
    quad[7] = p[1] - ny;
    stroke_emit_poly(dest, quad, 4);
}

/* Stroke one flattened polyline: segment rectangles + joins + caps */
static void stroke_emit_run(jlgpu_vec_t *dest, const float *pts, uint32_t point_cnt,
                            bool closed, const jlgpu_stroke_style_t *style)
{
    const uint32_t seg_cnt = closed ? point_cnt : point_cnt - 1;
    const uint32_t join_first = closed ? 0 : 1;
    const uint32_t join_end = closed ? point_cnt : point_cnt - 1;

    for (uint32_t i = 0; i < seg_cnt; i++) {
        const uint32_t j = (i + 1) % point_cnt;
        stroke_emit_segment(dest, &pts[2 * i], &pts[2 * j], style->half_width);
    }

    for (uint32_t i = join_first; i < join_end; i++) {
        stroke_emit_join(dest, &pts[2 * ((i + point_cnt - 1) % point_cnt)], &pts[2 * i],
                         &pts[2 * ((i + 1) % point_cnt)], style);
    }

    if (!closed) {
        const uint32_t last = point_cnt - 1;
        float head_dir[2] = { pts[0] - pts[2], pts[1] - pts[3] };
        float tail_dir[2] = { pts[2 * last] - pts[2 * (last - 1)],
                              pts[2 * last + 1] - pts[2 * (last - 1) + 1]
                            };
        stroke_emit_cap(dest, &pts[0], head_dir, style);
        stroke_emit_cap(dest, &pts[2 * last], tail_dir, style);
    }
}

/* ---------------------------------------------------------------------------
 * 展平: 把 lv 矢量路径的曲线拆成折线, 坐标带上 subtask 矩阵
 * ------------------------------------------------------------------------- */

static void stroke_flat_path_deinit(jlgpu_flat_path_t *flat)
{
    if (flat->pts != NULL) {
        lv_free(flat->pts);
        flat->pts = NULL;
    }
    flat->point_cnt = 0;
    flat->capacity = 0;
}

/* 追加一个展平点, 与上一点重合时跳过(避免零长线段把拐角方向算错) */
static bool stroke_flat_push(jlgpu_flat_path_t *flat, float x, float y)
{
    if (flat->is_oom) {
        return false;
    }

    if (flat->point_cnt > 0) {
        float dx = x - flat->pts[2 * (flat->point_cnt - 1)];
        float dy = y - flat->pts[2 * (flat->point_cnt - 1) + 1];
        if (MATH_FABSF(dx) < JLGPU_STROKE_EPS && MATH_FABSF(dy) < JLGPU_STROKE_EPS) {
            return true;
        }
    }

    if (flat->point_cnt == flat->capacity) {
        uint32_t capacity = (flat->capacity > 0) ? flat->capacity * 2 : JLGPU_STROKE_FLAT_INIT_PTS;
        float *pts = (flat->pts != NULL)
                     ? (float *)lv_realloc(flat->pts, capacity * 2 * sizeof(float))
                     : (float *)lv_malloc(capacity * 2 * sizeof(float));
        if (pts == NULL) {
            LV_LOG_ERROR("jlgpu vector: stroke flatten buffer alloc fail");
            flat->is_oom = true;
            return false;
        }
        flat->pts = pts;
        flat->capacity = capacity;
    }

    flat->pts[2 * flat->point_cnt] = x;
    flat->pts[2 * flat->point_cnt + 1] = y;
    flat->point_cnt++;
    return true;
}

/* CLOSE 之后又出现画线指令时, 子路径是空的, 先把起点补回去 */
static void stroke_flat_subpath_start(jlgpu_flat_path_t *flat, const lv_fpoint_t *cur)
{
    if (flat->point_cnt == 0) {
        stroke_flat_push(flat, cur->x, cur->y);
    }
}

/* 曲线按控制多边形长度估算细分数, 长曲线多拆几段 */
static int stroke_flatten_div(float len)
{
    int div = (int)(len / JLGPU_STROKE_SEG_PX) + 1;

    if (div < JLGPU_STROKE_MIN_DIV) {
        div = JLGPU_STROKE_MIN_DIV;
    }
    if (div > JLGPU_STROKE_MAX_DIV) {
        div = JLGPU_STROKE_MAX_DIV;
    }
    return div;
}

static void stroke_flatten_quad(jlgpu_flat_path_t *flat, const lv_fpoint_t *p0,
                                const lv_fpoint_t *c, const lv_fpoint_t *p1)
{
    float dx = c->x - p0->x;
    float dy = c->y - p0->y;
    float ex = p1->x - c->x;
    float ey = p1->y - c->y;
    int div = stroke_flatten_div(MATH_SQRTF(dx * dx + dy * dy) + MATH_SQRTF(ex * ex + ey * ey));

    for (int i = 1; i <= div; i++) {
        float t = (float)i / (float)div;
        float u = 1.0f - t;
        float x = u * u * p0->x + 2.0f * u * t * c->x + t * t * p1->x;
        float y = u * u * p0->y + 2.0f * u * t * c->y + t * t * p1->y;
        stroke_flat_push(flat, x, y);
    }
}

static void stroke_flatten_cubic(jlgpu_flat_path_t *flat, const lv_fpoint_t *p0,
                                 const lv_fpoint_t *c1, const lv_fpoint_t *c2,
                                 const lv_fpoint_t *p1)
{
    float dx1 = c1->x - p0->x;
    float dy1 = c1->y - p0->y;
    float dx2 = c2->x - c1->x;
    float dy2 = c2->y - c1->y;
    float dx3 = p1->x - c2->x;
    float dy3 = p1->y - c2->y;
    float len = MATH_SQRTF(dx1 * dx1 + dy1 * dy1) + MATH_SQRTF(dx2 * dx2 + dy2 * dy2) +
                MATH_SQRTF(dx3 * dx3 + dy3 * dy3);
    int div = stroke_flatten_div(len);

    for (int i = 1; i <= div; i++) {
        float t = (float)i / (float)div;
        float u = 1.0f - t;
        float x = u * u * u * p0->x + 3.0f * u * u * t * c1->x + 3.0f * u * t * t * c2->x + t * t * t * p1->x;
        float y = u * u * u * p0->y + 3.0f * u * u * t * c1->y + 3.0f * u * t * t * c2->y + t * t * t * p1->y;
        stroke_flat_push(flat, x, y);
    }
}

/* 一条绘制指令后面跟着几个控制点 */
static uint32_t stroke_op_point_cnt(lv_vector_path_op_t op)
{
    switch (op) {
    case LV_VECTOR_PATH_OP_LINE_TO:
        return 1;
    case LV_VECTOR_PATH_OP_QUAD_TO:
        return 2;
    case LV_VECTOR_PATH_OP_CUBIC_TO:
        return 3;
    default:
        return 0;
    }
}

/* 从路径点流里顺序取 cnt 个点, 按需变换到屏幕坐标 */
static void stroke_next_points(const lv_fpoint_t **point, lv_fpoint_t *out, uint32_t cnt,
                               const lv_matrix_t *matrix, bool has_matrix)
{
    for (uint32_t i = 0; i < cnt; i++) {
        out[i] = *(*point)++;
        if (has_matrix) {
            lv_matrix_transform_point(matrix, &out[i]);
        }
    }
}

/* 把一条直线/贝塞尔指令展平进当前子路径, `pts` 是变换后的控制点 */
static void stroke_flatten_op(jlgpu_flat_path_t *flat, lv_vector_path_op_t op,
                              const lv_fpoint_t *from, const lv_fpoint_t *pts)
{
    stroke_flat_subpath_start(flat, from);

    switch (op) {
    case LV_VECTOR_PATH_OP_LINE_TO:
        stroke_flat_push(flat, pts[0].x, pts[0].y);
        break;
    case LV_VECTOR_PATH_OP_QUAD_TO:
        stroke_flatten_quad(flat, from, &pts[0], &pts[1]);
        break;
    case LV_VECTOR_PATH_OP_CUBIC_TO:
        stroke_flatten_cubic(flat, from, &pts[0], &pts[1], &pts[2]);
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------
 * 描边主流程
 * ------------------------------------------------------------------------- */

static void stroke_dash_flush(jlgpu_vec_t *dest, jlgpu_flat_path_t *scratch,
                              const jlgpu_stroke_style_t *style)
{
    if (!scratch->is_oom && scratch->point_cnt >= 2) {
        stroke_emit_run(dest, scratch->pts, scratch->point_cnt, false, style);
    }
    scratch->point_cnt = 0;
}

/* 取下一段虚线长度并翻转画/不画, 用于跳过样式里长度为 0 的项 */
static float stroke_dash_advance(const float *dash, uint32_t dash_cnt, uint32_t *idx, bool *is_on)
{
    float remain = 0.0f;

    for (uint32_t i = 0; i < dash_cnt; i++) {
        *idx = (*idx + 1) % dash_cnt;
        *is_on = !*is_on;
        remain += dash[*idx];
        if (remain > JLGPU_STROKE_EPS) {
            break;
        }
    }
    return remain;
}

/* 虚线样式的总长度, 为 0 说明整条样式退化, 按实线处理 */
static float stroke_dash_total(const float *dash, uint32_t dash_cnt)
{
    float total = 0.0f;

    for (uint32_t i = 0; i < dash_cnt; i++) {
        total += dash[i];
    }
    return total;
}

/* 虚线: 沿折线按样式切成一段段, 每段按开放折线描边 */
static void stroke_emit_dashed(jlgpu_vec_t *dest, const jlgpu_flat_path_t *flat, uint32_t point_cnt,
                               jlgpu_flat_path_t *scratch, const jlgpu_stroke_style_t *style)
{
    const float *dash = style->dash;
    const uint32_t dash_cnt = style->dash_cnt;
    const float *pts = flat->pts;
    const uint32_t seg_cnt = flat->closed ? point_cnt : point_cnt - 1;
    uint32_t idx = 0;
    float remain = dash[0];
    bool is_on = true;

    if (stroke_dash_total(dash, dash_cnt) <= JLGPU_STROKE_EPS) {
        /* 样式全为 0, 退化成实线 */
        stroke_emit_run(dest, pts, point_cnt, flat->closed, style);
        return;
    }

    scratch->point_cnt = 0;

    for (uint32_t i = 0; i < seg_cnt; i++) {
        const uint32_t j = (i + 1) % point_cnt;
        const float x0 = pts[2 * i];
        const float y0 = pts[2 * i + 1];
        const float dx = pts[2 * j] - x0;
        const float dy = pts[2 * j + 1] - y0;
        const float seg_len = MATH_SQRTF(dx * dx + dy * dy);
        float used = 0.0f;

        if (seg_len < JLGPU_STROKE_EPS) {
            continue;
        }

        while (used < seg_len) {
            float rest = seg_len - used;
            float take = (remain < rest) ? remain : rest;
            float in = used / seg_len;
            float out = (used + take) / seg_len;

            if (is_on) {
                if (scratch->point_cnt == 0) {
                    stroke_flat_push(scratch, x0 + dx * in, y0 + dy * in);
                }
                stroke_flat_push(scratch, x0 + dx * out, y0 + dy * out);
            }

            used += take;
            remain -= take;
            if (remain <= JLGPU_STROKE_EPS) {
                remain = stroke_dash_advance(dash, dash_cnt, &idx, &is_on);
                if (!is_on) {
                    stroke_dash_flush(dest, scratch, style);
                }
            }
        }
    }

    stroke_dash_flush(dest, scratch, style);
}

/* 一条子路径展平结束, 输出它的描边轮廓 */
static void stroke_flush_subpath(jlgpu_vec_t *dest, jlgpu_flat_path_t *flat,
                                 jlgpu_flat_path_t *scratch, const jlgpu_stroke_style_t *style)
{
    uint32_t cnt = flat->point_cnt;

    if (flat->is_oom) {
        cnt = 0;
        flat->is_oom = false;
    }

    /* 闭合子路径若在 CLOSE 前又补了一条回到起点的线, 首尾会重合, 去掉重复点 */
    if (flat->closed && cnt >= 2 &&
        MATH_FABSF(flat->pts[0] - flat->pts[2 * (cnt - 1)]) < JLGPU_STROKE_EPS &&
        MATH_FABSF(flat->pts[1] - flat->pts[2 * (cnt - 1) + 1]) < JLGPU_STROKE_EPS) {
        cnt--;
    }

    if (cnt >= 2) {
        if (style->dash_cnt > 0) {
            stroke_emit_dashed(dest, flat, cnt, scratch, style);
        } else {
            stroke_emit_run(dest, flat->pts, cnt, flat->closed, style);
        }
    } else if (cnt == 1 && style->cap == LV_VECTOR_STROKE_CAP_ROUND) {
        /* 圆头端点样式下的孤立点画成一个圆点 */
        stroke_emit_circle(dest, flat->pts[0], flat->pts[1], style->half_width);
    }

    flat->point_cnt = 0;
    flat->closed = false;
}

/*
 * 把矢量路径描成轮廓写进 dest。曲线先展平成折线, 再逐段生成线宽矩形、
 * 拐角补角和端帽。输出只有 MOVE_TO/LINE_TO, 由硬件一次性填充。
 */
static void stroke_build(jlgpu_vec_t *dest, const lv_vector_path_t *src,
                         const lv_matrix_t *matrix, const jlgpu_stroke_style_t *style,
                         jlgpu_flat_path_t *flat, jlgpu_flat_path_t *dash_buf)
{
    const lv_vector_path_op_t *ops = lv_array_front(&src->ops);
    const lv_fpoint_t *point = lv_array_front(&src->points);
    const uint32_t op_size = lv_array_size(&src->ops);
    const bool has_matrix = !lv_matrix_is_identity(matrix);

    lv_fpoint_t cur = { 0, 0 };     /* 当前点 */
    lv_fpoint_t start = { 0, 0 };   /* 当前子路径起点 */
    lv_fpoint_t seg[3];             /* 本指令的控制点, 已变换到屏幕坐标 */
    bool has_cur = false;

    flat->point_cnt = 0;
    flat->closed = false;
    flat->is_oom = false;
    dash_buf->point_cnt = 0;
    dash_buf->is_oom = false;

    for (uint32_t i = 0; i < op_size; i++) {
        const lv_vector_path_op_t op = ops[i];

        if (op == LV_VECTOR_PATH_OP_MOVE_TO) {
            stroke_next_points(&point, seg, 1, matrix, has_matrix);
            stroke_flush_subpath(dest, flat, dash_buf, style);
            cur = seg[0];
            start = cur;
            has_cur = true;
            continue;
        }

        if (op == LV_VECTOR_PATH_OP_CLOSE) {
            if (flat->point_cnt > 0) {
                flat->closed = true;
                stroke_flush_subpath(dest, flat, dash_buf, style);
            }
            cur = start;
            continue;
        }

        const uint32_t pt_cnt = stroke_op_point_cnt(op);
        if (pt_cnt == 0) {
            continue;
        }

        stroke_next_points(&point, seg, pt_cnt, matrix, has_matrix);
        if (has_cur) {
            stroke_flatten_op(flat, op, &cur, seg);
        }
        cur = seg[pt_cnt - 1];
        start = has_cur ? start : cur;
        has_cur = true;
    }

    stroke_flush_subpath(dest, flat, dash_buf, style);
}

/* lv_area_t 是闭区间, jlgpu_area_t 是半开区间 */
static jlgpu_area_t jlgpu_area_from_lv(const lv_area_t *area)
{
    jlgpu_area_t out;

    out.x_min = area->x1;
    out.x_max = area->x2 + 1;
    out.y_min = area->y1;
    out.y_max = area->y2 + 1;

    return out;
}

/*
 * 把 jlgpu_vec_t.path_bbox(已是 device 空间的浮点包围盒)取整成整数区域。
 * 该包围盒含贝塞尔控制点, 本身就是真实曲线的保守上界, 这里再外扩 JLGPU_BBOX_MARGIN
 * 覆盖光栅化取整。返回 false 表示路径没有输出任何几何, 或与 clip 无交集。
 */
static bool jlgpu_bbox_to_area(const jlvec_bbox_t *bbox, const lv_area_t *clip, lv_area_t *out)
{
    lv_area_t area;

    /* 初值是 +MAX/-MAX, 一条线段都没写进去时 x_min > x_max */
    if (bbox->x_min > bbox->x_max || bbox->y_min > bbox->y_max) {
        return false;
    }

    area.x1 = (int32_t)MATH_FLOORF(bbox->x_min) - JLGPU_BBOX_MARGIN;
    area.y1 = (int32_t)MATH_FLOORF(bbox->y_min) - JLGPU_BBOX_MARGIN;
    area.x2 = (int32_t)MATH_CEILF(bbox->x_max) + JLGPU_BBOX_MARGIN;
    area.y2 = (int32_t)MATH_CEILF(bbox->y_max) + JLGPU_BBOX_MARGIN;

    return lv_area_intersect(out, &area, clip);
}

/*
 * 用路径自身(而非整块瓦片)的可见区域来定曲线细分精度。
 * 取的是控制点包围盒再经矩阵变换, 对曲线是保守偏大的上界 —— 只会多分不会少分, 安全。
 * 返回 false 表示路径为空或不可见, 调用者可以直接丢弃。
 */
static bool jlgpu_path_area_estimate(const lv_vector_path_t *path, const lv_matrix_t *matrix,
                                     const lv_area_t *clip, lv_area_t *out)
{
    if (lv_array_size(&path->points) == 0) {
        return false;
    }

    lv_area_t local;
    lv_vector_path_get_bounding(path, &local);

    lv_area_t device = lv_matrix_transform_area(matrix, &local);

    return lv_area_intersect(out, &device, clip);
}

/*
 * 提交一条已建好并 end 过的路径为纯色填充。
 * 三种目标各走一条路:
 *   显示缓冲区 -> 链表模式, 任务挂到显示链, 由推屏侧统一提交;
 *   离屏 canvas/layer -> uc->batch 有效时攒进本批任务链, 一批一次 jlgpu_run;
 *   离屏 canvas/layer 且批量不可用 -> 逐条立即绘制(旧行为, 兜底)。
 * `blend_area` 是调用方算好的紧致渲染区, 会同时用作 render_area 和 pt_clip 的尺寸。
 */
static void jlgpu_vector_fill_submit(jlgpu_vector_draw_ctx_t *uc,
                                     const lv_area_t *blend_area,
                                     jlgpu_vec_t *path,
                                     uint32_t argb_color,
                                     jlgpu_fillrule_t fill_rule)
{
    lv_draw_jlgpu_unit_t *u = uc->u;
    lv_draw_task_t *t = uc->t;
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
    render_params.render_quality = JLGPU_RENDER_QUALITY;

    mask_params.mask_mode = JLGPU_MASK_MODE_VECTOR;
    mask_params.mask_ptr = &jlgpu_path;

    int edge_num = path->edge_num;
    jlgpu_path.seg_len = path->total_len;

    /* seg_data 由下面各分支按自己的存储策略设置:
     * 链表模式拷进附加 buf, 批量模式接在任务节点后面, 逐条模式直接指向 path->base。 */
    jlgpu_path.fill_rule = fill_rule;
    jlgpu_path.max_bez_div = path->bez_div;
    jlgpu_path.user2surface = 0;
    jlgpu_path.path_trans_mode = JLGPU_TRANS_MODE_NONE;
    /* 注意这两个不能写反: render_quality 取全局渲染质量, msaa_lvl 取 path 的
     * 超采样档位。原来这里交叉写成了 render_quality=JLGPU_PATH_MSAA_LVL /
     * msaa_lvl=JLGPU_RENDER_QUALITY, 因为两个宏当前都是 MSAA_8X 才没暴露。
     * 其它 jlgpu 绘制单元(fill/line/border/img)用的都是下面这个配对。 */
    jlgpu_path.msaa_lvl = JLGPU_PATH_MSAA_LVL;

    if (LV_USE_DRAW_JLGPU_LIST && is_disp_buf) { //链表
#if LV_USE_DRAW_JLGPU_LIST
        // printf("[JLGPU-VEC] submit: LIST mode\n");
        uint8_t *task = NULL;
        /* 段数据要拷进附加 buf: 任务链是延后执行的, 而 path->base 是复用缓冲,
         * 下一条 path 会把它覆盖掉 */
        jlgpu_path.seg_data = lv_jlgpu_attach_buf_list_add(u, path->base, path->total_len);

        int task_len = jlgpu_draw_fill_task_len(u->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
        task = lv_jlgpu_mem_pool_get_buf(u, NULL, task_len);
        if (task == NULL) {
            LV_LOG_ERROR("malloc fail");
            // printf("[JLGPU-VEC] submit: mem_pool_get_buf NULL (task_len=%d), bail\n", task_len);
            return;
        }
        lv_memset(task, 0, task_len);

        jlgpu_set_task(u->jlgpu_ctx->ctx, task, (uint8_t)task_len);
        jlgpu_draw_fill(u->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
        lv_jlgpu_add_task_node(u, task);
#endif
    } else if (uc->batch != NULL) { //layer/canvas: 挂到本批任务链, 攒够一批一次提交
        jlgpu_vec_batch_submit(uc->batch, &render_params, &draw_fill_params, &mask_params, path);
    } else { //layer/canvas: 旧行为, 每条 path 一次同步 GPU 全流程
        jlgpu_vector_fill_submit_single(uc, &draw_fill_params, &mask_params, &render_params, path,
                                        dest_buf, out_format, fb_width);
    }
}

/*
 * 立即绘制(批量): 只生成任务节点并挂到本批链上, 真正的 jlgpu_run 由 flush 统一发起。
 *
 * 段数据必须拷一份: jlgpu_draw_fill() 会就地改写 seg_data 里的曲线分段数, 而 seg_data
 * 指向的是所有 path 复用的 unit->global_path->base。整批任务链是延后执行的, 不拷贝的话
 * 每条任务读到的都会是最后一条 path 的段流(链表模式同样要拷, 见 lv_jlgpu_attach_buf_list_add)。
 * 这里把段数据接在任务节点后面一次分配, flush 时释放任务即可一并释放。
 */
static void jlgpu_vec_batch_submit(jlgpu_vec_batch_t *batch, jlgpu_render_param_t *render_params,
                                   jlgpu_draw_fill_t *draw_fill_params, jlgpu_mask_param_t *mask_params,
                                   const jlgpu_vec_t *path)
{
    jlgpu_path_t *jp = (jlgpu_path_t *)mask_params->mask_ptr;
    int task_len = jlgpu_draw_fill_task_len(batch->ctx.ctx, draw_fill_params, mask_params);
    /* 段流按 2 字节对齐(段流内部会按 int16_t 读坐标) */
    uint32_t seg_off = ((uint32_t)task_len + 1u) & ~1u;
    uint32_t total = seg_off + (uint32_t)path->total_len;

    uint8_t *task = lv_malloc(total);
    if (task == NULL) {
        LV_LOG_ERROR("jlgpu vector: malloc task fail (%u bytes)", total);
        return;
    }
    lv_memset(task, 0, total);

    uint8_t *seg = task + seg_off;
    lv_memcpy(seg, path->base, (uint32_t)path->total_len);
    jp->seg_data = seg;

    jlgpu_set_task(batch->ctx.ctx, task, (uint8_t)task_len);
    jlgpu_draw_fill(batch->ctx.ctx, render_params, draw_fill_params, mask_params);

    jlgpu_vec_batch_add(batch, task, path->edge_num);
}

/*
 * 开一批"立即绘制"任务: 整批共用一套 GPU context, cbuf 也只分配并清零一次。
 * 返回 false 表示本目标不适合批量(显示缓冲区走显示侧的链表, 或内存不足), 调用者退回逐条提交。
 * 返回 false 时本函数已把申请到的资源全部释放, 调用者不需要再调 close。
 */
static bool jlgpu_vec_batch_open(jlgpu_vec_batch_t *batch, lv_layer_t *layer, lv_draw_buf_t *draw_buf)
{
    lv_memset(batch, 0, sizeof(*batch));

#if JLGPU_VEC_BATCH_EN
    /* 显示缓冲区由显示侧的链表统一提交, 不走本批 */
#if LV_USE_DRAW_JLGPU_LIST
    if (lv_is_draw_buf(draw_buf->data)) {
        return false;
    }
#endif

    lv_jlgpu_ctx_init(&batch->ctx);

    /* surface 只跟 layer 有关, 整批是常量, 只算一次 */
    batch->surface.data = (uint8_t *)NO_CACHE_ADDR(draw_buf->data);
    batch->surface.window.x_min = layer->buf_area.x1;
    batch->surface.window.x_max = layer->buf_area.x2 + 1;
    batch->surface.window.y_min = layer->buf_area.y1;
    batch->surface.window.y_max = layer->buf_area.y2 + 1;
    batch->surface.format = lv_jlgpu_get_color_format(draw_buf->header.cf);
    batch->surface.width = (uint32_t)lv_area_get_width(&layer->buf_area);

    /* cbuf 整批共用, 只清一次 —— 显示侧的链表模式同样整帧共用一块 cbuf */
    batch->gpath.cbuf_split = 0;
    batch->gpath.cbuf_stride = (uint16_t)batch->surface.width;
    batch->gpath.max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;

    batch->cbuf_size = (uint32_t)JLGPU_CBUF_SPLIT_ROWS * batch->surface.width;
    batch->cbuf = (uint8_t *)lv_malloc(batch->cbuf_size);
    if (batch->cbuf == NULL) {
        LV_LOG_ERROR("jlgpu vector: malloc cbuf fail (%u bytes)", batch->cbuf_size);
        lv_jlgpu_ctx_uninit(&batch->ctx);
        return false;
    }
    lv_memset(batch->cbuf, 0, batch->cbuf_size);

    return true;
#else
    (void)layer;
    (void)draw_buf;

    return false;
#endif
}

/* 把本批剩下不足一批的任务提交掉, 然后释放整批的资源 */
static void jlgpu_vec_batch_close(jlgpu_vec_batch_t *batch)
{
    jlgpu_vec_batch_flush(batch);

    lv_jlgpu_ctx_uninit(&batch->ctx);

    if (batch->cbuf) {
        lv_free(batch->cbuf);
        batch->cbuf = NULL;
    }
    if (batch->ebuf) {
        lv_free(batch->ebuf);
        batch->ebuf = NULL;
    }
}

/*
 * 把一个任务节点挂到本批链尾, 攒够 JLGPU_VEC_BATCH_CHUNK 个就提交一次。
 * 这里不能借用 lv_jlgpu_add_task_node(): 那个操作的是显示侧那条链, 而且它的节点计数是
 * uint8_t, 一块瓦片 282 条 path 会直接溢出。
 */
static void jlgpu_vec_batch_add(jlgpu_vec_batch_t *batch, void *task, int32_t edge_num)
{
    if (batch->head == NULL) {
        batch->head = task;
    } else {
        lv_jlgpu_task_chain_link(batch->cur, task);
    }

    batch->cur = task;
    batch->cnt++;

    if (edge_num > batch->max_edge) {
        batch->max_edge = edge_num;
    }

    if (batch->cnt >= JLGPU_VEC_BATCH_CHUNK) {
        jlgpu_vec_batch_flush(batch);
    }
}

/*
 * 把本批任务链一次性提交给 GPU。
 * ebuf 要按整批最大的 edge 数给足(任务节点是延后执行的, 建节点时还不知道全批的规模);
 * run 返回后节点即可释放 —— LV_DRAW_JLGPU_ASYNC=0, jlgpu_run 是同步的。
 */
static void jlgpu_vec_batch_flush(jlgpu_vec_batch_t *batch)
{
    if (batch->head == NULL) {
        return;
    }

    bool can_run = true;

    /* ebuf 跨 chunk 复用, 只增不减, 避免每个 chunk 都 malloc/free 一次 */
    uint32_t edge_cnt = (batch->max_edge > 0) ? (uint32_t)batch->max_edge : 1;
    uint32_t need = JLGPU_EDGE_BYTES * edge_cnt;

    if (batch->ebuf_size < need) {
        if (batch->ebuf) {
            lv_free(batch->ebuf);
        }
        batch->ebuf = (uint8_t *)lv_malloc(need);
        if (batch->ebuf == NULL) {
            LV_LOG_ERROR("jlgpu vector: malloc ebuf fail (%u bytes)", need);
            batch->ebuf_size = 0;
            can_run = false;
        } else {
            lv_memset(batch->ebuf, 0, need);
            batch->ebuf_size = need;
        }
    }

    if (can_run) {
        batch->gpath.max_edge_cfg = (uint16_t)batch->max_edge;
        batch->gpath.c_data = batch->cbuf;
        batch->gpath.e_data = batch->ebuf;

        /* lv_jlgpu_ctx_init() 里已经调过 jlgpu_init(), 整批只要一次 */
        if (!batch->is_hw_inited) {
            jlgpu_init(batch->ctx.ctx);
            batch->is_hw_inited = true;
        }

        jlgpu_set_params(batch->ctx.ctx, &batch->surface, 0, 0, &batch->gpath, 0);
        jlgpu_run(batch->ctx.ctx, batch->head);
    }

    /* 段数据接在任务节点后面一起分配(见 jlgpu_vec_batch_submit), 释放节点即一并释放 */
    void *task = batch->head;
    while (task) {
        void *next = lv_jlgpu_task_chain_next(task);
        lv_free(task);
        task = next;
    }

    batch->head = NULL;
    batch->cur = NULL;
    batch->cnt = 0;
    batch->max_edge = 0;
}

/* 立即绘制(逐条): 每条 path 一次 ctx init + jlgpu_init + cbuf/ebuf 分配 + 阻塞 run + teardown */
static void jlgpu_vector_fill_submit_single(jlgpu_vector_draw_ctx_t *uc,
        jlgpu_draw_fill_t *draw_fill_params,
        jlgpu_mask_param_t *mask_params,
        jlgpu_render_param_t *render_params,
        const jlgpu_vec_t *path,
        uint8_t *dest_buf, uint8_t out_format, uint32_t fb_width)
{
    jlgpu_path_t *jp = (jlgpu_path_t *)mask_params->mask_ptr;
    int edge_num = path->edge_num;

    jp->seg_data = path->base;

    uint8_t *cbuf = NULL;
    uint8_t *ebuf = NULL;
    uint8_t *task = NULL;

    lv_jlgpu_ctx_t jlgpu_ctx;
    lv_jlgpu_ctx_init(&jlgpu_ctx);
    int task_len = jlgpu_draw_fill_task_len(jlgpu_ctx.ctx, draw_fill_params, mask_params);
    task = lv_malloc(task_len);
    if (task == NULL) {
        LV_LOG_ERROR("malloc fail");
        goto malloc_fail;
    }
    lv_memset(task, 0, task_len);

    jlgpu_set_task(jlgpu_ctx.ctx, task, (uint8_t)task_len);
    jlgpu_draw_fill(jlgpu_ctx.ctx, render_params, draw_fill_params, mask_params);

    jlgpu_surface_t surface = {0};
    jlgpu_global_path_t global_path = {0};

    surface.data = (uint8_t *)NO_CACHE_ADDR(dest_buf);

    surface.window.x_min = uc->t->target_layer->buf_area.x1;
    surface.window.x_max = uc->t->target_layer->buf_area.x2 + 1;
    surface.window.y_min = uc->t->target_layer->buf_area.y1;
    surface.window.y_max = uc->t->target_layer->buf_area.y2 + 1;

    surface.format = out_format;
    surface.width = fb_width;

    global_path.cbuf_split = 0;
    global_path.cbuf_stride = fb_width;
    global_path.max_bez_div = JLGPU_GLOBAL_MAX_BEZ_DIV;
    global_path.max_edge_cfg = edge_num;

    int mask_h = global_path.cbuf_split ? 16 : 32;
    cbuf = (uint8_t *)lv_malloc(mask_h * fb_width);
    if (cbuf == NULL) {
        LV_LOG_ERROR("malloc cbuf fail");
        goto malloc_fail;
    }
    lv_memset(cbuf, 0, mask_h * fb_width);

    ebuf = (uint8_t *)lv_malloc(JLGPU_EDGE_BYTES * edge_num);
    if (ebuf == NULL) {
        LV_LOG_ERROR("malloc ebuf fail");
        goto malloc_fail;
    }
    lv_memset(ebuf, 0, JLGPU_EDGE_BYTES * edge_num);

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

#endif /*LV_USE_DRAW_JLGPU_VECTOR_ENABLE*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /* #if CONFIG_LVGL_UI_ENABLE */
