/**
 * @file lv_port_video_disp.c
 * @brief LVGL视频流显示功能实现
 *
 * 此文件实现了LVGL视频流显示功能，包括：
 * - 创建一个img控件用于显示视频流
 * - 支持动态设置视频源(set_src)
 * - 在渲染开始前更新视频源
 * - 在渲染结束后调用回调
 */

/*********************
 *      INCLUDES
 *********************/
#include "asm/dcache.h"
#include "lv_color.h"
#include "lv_image_dsc.h"
#define BOOL_DEFINE_CONFLICT
#include "lv_port_video_disp.h"
#include "jlgpu_image_filters.h"
#if  LV_USE_VIDEO_DISP
#include "src/draw/lv_draw_buf.h"

#include "fb.h"
#include "os/os_api.h"

/*********************
 *      DEFINES
 *********************/
#define LV_VIDEO_IMG_OBJ_NUM_MAX 9
/**********************
 *      TYPEDEFS
 **********************/

/* 视频显示控制结构 */
typedef struct {
    /* === 显示控件 === */
    lv_obj_t *img_obj[LV_VIDEO_IMG_OBJ_NUM_MAX];
    lv_image_dsc_t *video_dsc;
    lv_yuv_buf_t yuv_plane;
    void *video_frame_buf;
    uint32_t video_width;
    uint32_t video_height;
    lv_color_format_t video_format;
    lv_video_render_end_cb_t render_end_cb;
    void *render_end_user_data;
    volatile uint8_t need_update;
    volatile uint8_t async_flushing;
    uint8_t first_frame_shown;  /* 首帧是否已显示: create 默认隐藏, 首帧到达时显示一次 */

    /* === 特效 === */
    jlgpu_filter_mode_t effect_type;
    uint8_t img_obj_num;
    lv_obj_t *img_effect_obj;

    /* === 滤镜 === */
    lv_obj_t *invert_overlay;
    fisheye_lut_t *fisheye_lut;
    uint8_t *fisheye_frame_out;
    honeycomb_lut_t *honeycomb_lut;
    uint8_t *honeycomb_frame_out;

    /* === fb 设备 === */
    void *fb_dev;
    struct fb_map_user fb_map;
    uint8_t fb_index;
    char fb_name[8];

    /* === 注册表 === */
    int registry_slot;
} lv_video_disp_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void video_disp_render_start_event_cb(lv_event_t *e);
static void video_disp_render_ready_event_cb(lv_event_t *e);
static void video_disp_flush_start_event_cb(lv_event_t *e);

static int pipeline_video_init(void);
static int pipeline_video_uninit(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/* 实例注册表 */
static lv_video_disp_ctx_t *g_instances[LV_VIDEO_DISP_MAX_INSTANCES] = {0};



/* LVGL 渲染回调是否已注册 */
static volatile uint8_t g_render_cb_registered = 0;

/* 滤镜颜色常量 */
static const lv_color_t FILTER_RED    = {.red = 0xFA, .green = 0x50, .blue = 0x50};
static const lv_color_t FILTER_GREEN  = {.red = 0x00, .green = 0xDC, .blue = 0x00};
static const lv_color_t FILTER_YELLOW = {.red = 0xFF, .green = 0xFF, .blue = 0x64};
static const lv_color_t FILTER_SEPIA  = {.red = 0x70, .green = 0x29, .blue = 0x12};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 按 fb_index 查找实例
 */
lv_video_disp_handle_t *lv_port_video_disp_get_instance_by_fb_index(uint8_t fb_index)
{
    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_instances[i] && g_instances[i]->fb_index == fb_index) {
            return (lv_video_disp_handle_t *)g_instances[i];
        }
    }
    return NULL;
}

/**********************
 *   STATIC HELPERS
 **********************/

/* 实例注册表操作 */
static int instance_register(lv_video_disp_ctx_t *ctx)
{
    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_instances[i] == NULL) {
            g_instances[i] = ctx;
            ctx->registry_slot = i;
            return i;
        }
    }
    return -1;
}

static void instance_unregister(lv_video_disp_ctx_t *ctx)
{
    if (ctx->registry_slot >= 0 && ctx->registry_slot < LV_VIDEO_DISP_MAX_INSTANCES) {
        g_instances[ctx->registry_slot] = NULL;
    }
    ctx->registry_slot = -1;
}

static int instance_registry_is_empty(void)
{
    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_instances[i] != NULL) {
            return 0;
        }
    }
    return 1;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 创建视频显示功能
 * @param parent 父对象，如果为NULL则创建到默认屏幕
 * @param width 视频显示宽度
 * @param height 视频显示高度
 * @param x 视频显示位置X坐标
 * @param y 视频显示位置Y坐标
 * @return img控件对象指针，失败返回NULL
 */
lv_video_disp_handle_t *lv_port_video_disp_create(lv_obj_t *parent, int32_t width, int32_t height,
        int32_t x, int32_t y, lv_color_format_t format,
        const char *fb_name)
{
    lv_display_t *disp = NULL;
    lv_video_disp_ctx_t *ctx = NULL;
    u8 fb_format_u8;

    /* 同名 fb 已创建则直接返回已有 handle */
    if (fb_name && fb_name[0]) {
        for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
            if (g_instances[i] && strcmp(g_instances[i]->fb_name, fb_name) == 0) {
                printf("lv_port_video_disp_create: fb %s already exists\n", fb_name);
                return (lv_video_disp_handle_t *)g_instances[i];
            }
        }
    }

    /* 分配上下文 */
    ctx = (lv_video_disp_ctx_t *)lv_malloc_zeroed(sizeof(lv_video_disp_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }
    if (fb_name && fb_name[0]) {
        strcpy(ctx->fb_name, fb_name);
        ctx->fb_index = fb_name[2] - '0';
    } else {
        strcpy(ctx->fb_name, "fb0");
        ctx->fb_index = 0;
    }
    ctx->registry_slot = -1;

#if LV_VIDEO_DISP_FB_MODE
    ctx->img_obj[0] = lv_malloc(sizeof(lv_obj_t));
    if (ctx->img_obj[0] == NULL) {
        lv_free(ctx);
        return NULL;
    }
#else
    disp = lv_display_get_default();
    if (disp == NULL) {
        lv_free(ctx);
        return NULL;
    }

    if (parent == NULL) {
        parent = lv_screen_active();
    }
    if (lv_display_get_horizontal_resolution(disp) == width &&
        lv_display_get_vertical_resolution(disp) == height) {
        lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, 0);
    }

    ctx->img_obj[0] = lv_image_create(parent);
    if (ctx->img_obj[0] == NULL) {
        lv_free(ctx);
        return NULL;
    }

    /* 默认隐藏防白屏, 首帧到达时自动显示一次, 之后由 lv_port_video_disp_show() 控制 */
    lv_obj_add_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN);
    ctx->first_frame_shown = 0;

    lv_obj_set_pos(ctx->img_obj[0], x, y);
    lv_obj_set_size(ctx->img_obj[0], width, height);
    lv_obj_move_background(ctx->img_obj[0]);
    lv_image_set_inner_align(ctx->img_obj[0], LV_IMAGE_ALIGN_STRETCH);

    /* 第一个实例注册渲染回调 */
    if (!g_render_cb_registered) {
        lv_display_add_event_cb(disp, video_disp_render_start_event_cb,
                                LV_EVENT_RENDER_START, NULL);
        lv_display_add_event_cb(disp, video_disp_render_ready_event_cb,
                                LV_EVENT_RENDER_READY, NULL);
        g_render_cb_registered = 1;
    }
#endif

    ctx->video_width  = (uint32_t)width;
    ctx->video_height = (uint32_t)height;
    ctx->video_format = format;
    ctx->img_obj_num  = 1;
    ctx->effect_type  = JLGPU_FILTER_NONE;

    /* 打开 fb 设备 */
    fb_format_u8 = ((format == LV_COLOR_FORMAT_NV12 || format == LV_COLOR_FORMAT_NV21)
                    ? FB_COLOR_FORMAT_YUV420P : FB_COLOR_FORMAT_YUV422);
    {
        u32 param = ((u32)fb_format_u8 << 16);
        ctx->fb_dev = dev_open(ctx->fb_name, (void *)param);
        if (!ctx->fb_dev) {
            printf(" ||| %s ,,, %d", __FUNCTION__, __LINE__);
        }
    }

    /* 注册到全局表 */
    if (instance_register(ctx) < 0) {
        if (ctx->fb_dev) {
            dev_close(ctx->fb_dev);
        }
#if !LV_VIDEO_DISP_FB_MODE
        lv_obj_delete(ctx->img_obj[0]);
#endif
        lv_free(ctx);
        return NULL;
    }

    printf("lv_port_video_disp_create w:%d h:%d fmt:%d fb:%s slot:%d\n",
           width, height, format, ctx->fb_name, ctx->registry_slot);

    return (lv_video_disp_handle_t *)ctx;
}

/**
 * @brief 删除视频显示功能
 */
void lv_port_video_disp_del(lv_video_disp_handle_t *handle)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;

    if (ctx == NULL || ctx->registry_slot < 0) {
        return;
    }

    instance_unregister(ctx);

#if LV_VIDEO_DISP_FB_MODE
    if (ctx->img_obj[0]) {
        lv_free(ctx->img_obj[0]);
        ctx->img_obj[0] = NULL;
    }
#else
    for (int i = 0; i < LV_VIDEO_IMG_OBJ_NUM_MAX; i++) {
        if (ctx->img_obj[i]) {
            lv_obj_delete(ctx->img_obj[i]);
            ctx->img_obj[i] = NULL;
        }
    }

    if (ctx->img_effect_obj) {
        lv_obj_delete(ctx->img_effect_obj);
        ctx->img_effect_obj = NULL;
    }

#if LV_USE_DRAW_JLGPU_LIST
    if (ctx->fisheye_lut) {
        fisheye_lut_destroy(ctx->fisheye_lut);
        ctx->fisheye_lut = NULL;
    }
    if (ctx->fisheye_frame_out) {
        filters_yuv_buffer_free(ctx->fisheye_frame_out);
        ctx->fisheye_frame_out = NULL;
    }
    if (ctx->honeycomb_lut) {
        honeycomb_lut_destroy(ctx->honeycomb_lut);
        ctx->honeycomb_lut = NULL;
    }
    if (ctx->honeycomb_frame_out) {
        filters_yuv_buffer_free(ctx->honeycomb_frame_out);
        ctx->honeycomb_frame_out = NULL;
    }
#endif
    if (ctx->invert_overlay) {
        lv_obj_delete(ctx->invert_overlay);
        ctx->invert_overlay = NULL;
    }

    if (instance_registry_is_empty() && g_render_cb_registered) {
        lv_display_t *disp = lv_display_get_default();
        if (disp) {
            lv_display_remove_event_cb_with_user_data(disp,
                    video_disp_render_start_event_cb, NULL);
            lv_display_remove_event_cb_with_user_data(disp,
                    video_disp_render_ready_event_cb, NULL);
        }
        g_render_cb_registered = 0;
    }
#endif

    if (ctx->video_dsc) {
        lv_free(ctx->video_dsc);
        ctx->video_dsc = NULL;
    }

    if (ctx->fb_dev) {
        dev_close(ctx->fb_dev);
        ctx->fb_dev = NULL;
    }

    lv_free(ctx);
}

/**
 * @brief 设置视频源
 * @param frame_buf 视频帧缓冲区地址
 * @param frame_uv_buf 视频帧uv缓冲区地址
 * @param width 视频宽度
 * @param height 视频高度
 * @param format 视频颜色格式
 * @param stride 视频行步长（字节），如果为0则自动计算
 * @return 0表示成功，非0表示失败
 */
int lv_port_video_disp_set_src(lv_video_disp_handle_t *handle, void *frame_buf, void *frame_uv_buf,
                               uint32_t width, uint32_t height, lv_color_format_t format, uint32_t stride)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    lv_image_dsc_t *dsc = NULL;
    uint32_t calc_stride = 0;

    if (ctx == NULL || ctx->img_obj[0] == NULL || frame_buf == NULL) {
        return -1;
    }

    if (stride == 0) {
        calc_stride = lv_draw_buf_width_to_stride(width, format);
    } else {
        calc_stride = stride;
    }

    if (ctx->video_dsc == NULL) {
        ctx->video_dsc = (lv_image_dsc_t *)lv_malloc_zeroed(sizeof(lv_image_dsc_t));
        if (ctx->video_dsc == NULL) {
            return -1;
        }
    }

    dsc = ctx->video_dsc;
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = format;
    dsc->header.flags = 0;
    dsc->header.w = width;
    dsc->header.h = height;
    dsc->header.stride = calc_stride;
    dsc->header.reserved_2 = 0;
    dsc->data = (const uint8_t *)frame_buf;
    dsc->data_size = calc_stride * height;

    if (format == LV_COLOR_FORMAT_NV12 || format == LV_COLOR_FORMAT_NV21) {
        if (frame_uv_buf) {
            dsc->header.flags |= LV_IMAGE_FLAGS_USER1;
            ctx->yuv_plane.semi_planar.y.buf = frame_buf;
            ctx->yuv_plane.semi_planar.uv.buf = frame_uv_buf;
            dsc->data = (const uint8_t *)&ctx->yuv_plane;
        }
    }

    ctx->video_frame_buf = frame_buf;
    ctx->video_width = width;
    ctx->video_height = height;
    ctx->video_format = format;
    ctx->need_update = 1;

    return 0;
}

lv_obj_t *lv_port_video_disp_get_img_obj(lv_video_disp_handle_t *handle)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx == NULL) {
        return NULL;
    }
    return ctx->img_obj[0];
}

void lv_port_video_disp_set_render_end_cb(lv_video_disp_handle_t *handle,
        lv_video_render_end_cb_t cb, void *user_data)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx == NULL) {
        return;
    }
    ctx->render_end_cb = cb;
    ctx->render_end_user_data = user_data;
}

void lv_port_video_disp_set_pos(lv_video_disp_handle_t *handle, int32_t x, int32_t y)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx && ctx->img_obj[0]) {
        lv_obj_set_pos(ctx->img_obj[0], x, y);
    }
}

void lv_port_video_disp_set_size(lv_video_disp_handle_t *handle, int32_t width, int32_t height)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx && ctx->img_obj[0]) {
        lv_obj_set_size(ctx->img_obj[0], width, height);
    }
}

void lv_port_video_disp_show(lv_video_disp_handle_t *handle, uint8_t show)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx && ctx->img_obj[0]) {
        if (show) {
            lv_obj_clear_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void lv_port_video_disp_ready(lv_video_disp_handle_t *handle)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;
    if (ctx) {
        ctx->async_flushing = 0;
    }
}

static uint8_t get_effect_img_obj_num(jlgpu_filter_mode_t effect_type)
{
    uint8_t img_obj_num = 1;
    switch (effect_type) {
    case JLGPU_FILTER_NONE:
    case JLGPU_FILTER_STICKER:
    case JLGPU_FILTER_HONEYCOMB_NORMAL:
    case JLGPU_FILTER_FISHEYE_NORMAL:
    case JLGPU_FILTER_RED:
    case JLGPU_FILTER_GREEN:
    case JLGPU_FILTER_YELLOW:
    case JLGPU_FILTER_GRAY:
    case JLGPU_FILTER_SEPIA:
    case JLGPU_FILTER_NEGATIVE:
        img_obj_num = 1;
        break;
    case JLGPU_FILTER_TILE_HORIZ:
    case JLGPU_FILTER_TILE_VERT:
    case JLGPU_FILTER_MIRROR_HORIZ:
    case JLGPU_FILTER_MIRROR_VERT:
        img_obj_num = 2;
        break;
    case JLGPU_FILTER_GRID_4:
        img_obj_num = 4;
        break;
    case JLGPU_FILTER_GRID_9:
        img_obj_num = 9;
        break;
    default:
        LV_LOG_ERROR("Invalid effect type");
        img_obj_num = 0;
        break;
    }
    return img_obj_num;
}

/**
 * @brief 动态设置 N×M 宫格布局
 * @param parent      父控件
 * @param objs        子控件数组
 * @param count       子控件数量
 * @param cols        列数 (2=4宫格, 3=9宫格, 4=16宫格)
 * @param gap         控件间距(px)
 */
static void set_grid_layout(lv_obj_t *parent, lv_obj_t **objs, uint32_t count, uint32_t cols, int32_t gap)
{
    if (count == 0 || cols == 0) {
        return;
    }

    int32_t parent_w = lv_obj_get_content_width(parent);
    int32_t parent_h = lv_obj_get_content_height(parent);
    uint32_t rows = (count + cols - 1) / cols;  // 向上取整计算行数

    // 总间隙数 = (列数-1)个水平间隙 + (行数-1)个垂直间隙
    int32_t cell_w = (parent_w - (int32_t)(cols - 1) * gap) / (int32_t)cols;
    int32_t cell_h = (parent_h - (int32_t)(rows - 1) * gap) / (int32_t)rows;

    for (uint32_t i = 0; i < count; i++) {
        int32_t col = i % cols;
        int32_t row = i / cols;

        lv_obj_set_size(objs[i], cell_w, cell_h);
        lv_obj_set_pos(objs[i],
                       col * (cell_w + gap),
                       row * (cell_h + gap));

        lv_image_set_inner_align(objs[i], LV_IMAGE_ALIGN_DEFAULT);
    }
}


/**
 * @brief 双控件分屏布局（支持镜像）
 * @param parent    父控件
 * @param obj1      第一个控件
 * @param obj2      第二个控件
 * @param vertical  true=上下分屏, false=左右分屏
 * @param mirror    是否对obj2进行镜像翻转
 * @param gap       两控件之间的间距(px)
 */
void set_dual_layout(lv_obj_t *parent, lv_obj_t *obj1, lv_obj_t *obj2, bool vertical, bool mirror, int32_t gap)
{
    if (obj1 == NULL || obj2 == NULL) {
        return;
    }

    int32_t pw = lv_obj_get_content_width(parent);
    int32_t ph = lv_obj_get_content_height(parent);

    int32_t w, h, x1, y1, x2, y2;

    if (!vertical) {
        /* ---- 左右分屏 ---- */
        w  = (pw - gap) / 2;
        h  = ph;
        x1 = 0;
        y1 = 0;
        x2 = w + gap;
        y2 = 0;
    } else {
        /* ---- 上下分屏 ---- */
        w  = pw;
        h  = (ph - gap) / 2;
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = h + gap;
    }

    /* 设置尺寸与位置 */
    lv_obj_set_size(obj1, w, h);
    lv_obj_set_pos(obj1, x1, y1);

    lv_obj_set_size(obj2, w, h);
    lv_obj_set_pos(obj2, x2, y2);

#if LV_USE_DRAW_JLGPU
    if (mirror) {
        //镜像情况下需要特殊处理
        lv_obj_set_size(obj2, pw, ph);
        lv_obj_set_pos(obj2, 0, 0);
        lv_obj_move_background(obj2);

        if (vertical) {
            lv_image_set_flip_v(obj2, 1);
        } else {
            lv_image_set_flip_h(obj2, 1);
        }

        //不设置变换原点，默认为图像中心点
        /* 将变换原点设为控件中心，避免镜像后位置偏移 */
        /*lv_image_set_pivot_x(obj2, w / 2);*/
        /*lv_image_set_pivot_y(obj2, h / 2);*/

    } else {
        /* 取消镜像恢复原始状态 */
        lv_image_set_flip_v(obj2, 0);
        lv_image_set_flip_h(obj2, 0);
        lv_image_set_rotation(obj2, 0);
    }
#endif

    lv_image_set_inner_align(obj1, LV_IMAGE_ALIGN_DEFAULT);
    lv_image_set_inner_align(obj2, LV_IMAGE_ALIGN_DEFAULT);
}

void lv_port_video_set_effect(lv_video_disp_handle_t *handle, uint8_t effect_type,
                              const void *src, int32_t x, int32_t y)
{
    if (handle == NULL) {
        LV_LOG_WARN("handle is NULL");
        return;
    }
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;

    uint8_t img_obj_num = get_effect_img_obj_num(effect_type);

    if (ctx->img_obj[0] == NULL || !img_obj_num) {
        LV_LOG_WARN("The video obj has not been created");
        return;
    }

    lv_obj_t *parent = lv_obj_get_parent(ctx->img_obj[0]);
    /* 摄像头主控件 img_obj[0] 尺寸应取视频帧尺寸(如 320x240), 不能取 parent 屏尺寸:
     * 物理屏可能高于视频(如 320x386 屏显示 320x240 预览), 若用屏尺寸 STRETCH 会把
     * 320x240 竖向拉满 386 全屏变形。video_width/height 由 lvgl_video_init/set_src
     * 设为真实视频尺寸。(分屏滤镜 set_dual/grid_layout 内部自读 parent, 不受此变量影响) */
    int32_t width  = (int32_t)ctx->video_width;
    int32_t height = (int32_t)ctx->video_height;
    if (width <= 0 || height <= 0) {
        width  = lv_obj_get_width(parent);
        height = lv_obj_get_height(parent);
    }

    if (img_obj_num > ctx->img_obj_num) { //创建控件

        for (int i = ctx->img_obj_num; i < img_obj_num; i++) {
            //创建img_obj[i]
            if (!ctx->img_obj[i]) {
                ctx->img_obj[i] = lv_image_create(parent);
                lv_obj_move_background(ctx->img_obj[i]);
            }
        }

    } else if (img_obj_num < ctx->img_obj_num) { //删除控件

        for (int i = img_obj_num; i < ctx->img_obj_num; i++) {
            //删除img_obj[i]
            if (ctx->img_obj[i]) {
                lv_obj_delete(ctx->img_obj[i]);
                ctx->img_obj[i] = NULL;
            }
        }

    } else {//不变

    }

    ctx->img_obj_num = img_obj_num;

    //重置obj_size
    lv_obj_set_size(ctx->img_obj[0], width, height);

    //重置重着色
    lv_obj_set_style_image_recolor(ctx->img_obj[0], lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_image_recolor_opa(ctx->img_obj[0], LV_OPA_TRANSP, 0);

    //重置贴图
    if (ctx->img_effect_obj) {
        lv_obj_delete(ctx->img_effect_obj);
        ctx->img_effect_obj = NULL;
    }

#if LV_USE_DRAW_JLGPU
    //重置镜像
    if (ctx->img_obj[1]) {
        lv_image_set_flip_v(ctx->img_obj[1], 0);
        lv_image_set_flip_h(ctx->img_obj[1], 0);
    }
#endif

#if LV_USE_DRAW_JLGPU_LIST
    //重置鱼眼效果
    if (effect_type != JLGPU_FILTER_FISHEYE_NORMAL) {
        if (ctx->fisheye_lut) {
            fisheye_lut_destroy(ctx->fisheye_lut);
            ctx->fisheye_lut = NULL;
        }
        if (ctx->fisheye_frame_out) {
            lv_free(ctx->fisheye_frame_out);
            ctx->fisheye_frame_out = NULL;
        }
    }

    //重置蜂窝效果
    if (effect_type != JLGPU_FILTER_HONEYCOMB_NORMAL) {
        if (ctx->honeycomb_lut) {
            honeycomb_lut_destroy(ctx->honeycomb_lut);
            ctx->honeycomb_lut = NULL;
        }
        if (ctx->honeycomb_frame_out) {
            lv_free(ctx->honeycomb_frame_out);
            ctx->honeycomb_frame_out = NULL;
        }
    }
#endif

    //重置反色效果滤镜
    if (ctx->invert_overlay) {
        lv_obj_delete(ctx->invert_overlay);
        ctx->invert_overlay = NULL;
        lv_image_set_blend_mode(ctx->img_obj[0], LV_BLEND_MODE_NORMAL);
    }

    switch (effect_type) {
    case JLGPU_FILTER_NONE:
        lv_obj_set_size(ctx->img_obj[0], width, height);
        lv_image_set_inner_align(ctx->img_obj[0], LV_IMAGE_ALIGN_STRETCH);
        break;
    case JLGPU_FILTER_STICKER:
        lv_obj_set_size(ctx->img_obj[0], width, height);
        lv_image_set_inner_align(ctx->img_obj[0], LV_IMAGE_ALIGN_STRETCH);
        ctx->img_effect_obj = lv_image_create(parent);
        lv_obj_move_to_index(ctx->img_effect_obj, 1);
        lv_img_set_src(ctx->img_effect_obj, src);
        lv_obj_set_pos(ctx->img_effect_obj, x, y);
        break;
    case JLGPU_FILTER_TILE_HORIZ:
        set_dual_layout(parent, ctx->img_obj[0], ctx->img_obj[1], 0, 0, 0);
        break;
    case JLGPU_FILTER_TILE_VERT:
        set_dual_layout(parent, ctx->img_obj[0], ctx->img_obj[1], 1, 0, 0);
        break;
    case JLGPU_FILTER_MIRROR_HORIZ:
        set_dual_layout(parent, ctx->img_obj[0], ctx->img_obj[1], 0, 1, 0);
        break;
    case JLGPU_FILTER_MIRROR_VERT:
        set_dual_layout(parent, ctx->img_obj[0], ctx->img_obj[1], 1, 1, 0);
        break;
    case JLGPU_FILTER_GRID_4:
        set_grid_layout(parent, ctx->img_obj, 4, 2, 0);
        break;
    case JLGPU_FILTER_GRID_9:
        set_grid_layout(parent, ctx->img_obj, 9, 3, 0);
        break;
    case JLGPU_FILTER_FISHEYE_NORMAL:
#if LV_USE_DRAW_JLGPU_LIST
        if (!ctx->fisheye_lut) {
            ctx->fisheye_lut = fisheye_lut_create(ctx->video_width, ctx->video_height, 1.0f, JLGPU_FILTER_DEFAULT_BATCH);
        }
        if (!ctx->fisheye_frame_out) {
            ctx->fisheye_frame_out = filters_yuv_buffer_alloc(ctx->video_width, ctx->video_height, ctx->video_format);
        }
#endif
        break;
    case JLGPU_FILTER_HONEYCOMB_NORMAL:
#if LV_USE_DRAW_JLGPU_LIST
        if (!ctx->honeycomb_lut) {
            ctx->honeycomb_lut = honeycomb_lut_create(ctx->video_width, ctx->video_height, 80, JLGPU_FILTER_DEFAULT_BATCH);
        }
        if (!ctx->honeycomb_frame_out) {
            ctx->honeycomb_frame_out = filters_yuv_buffer_alloc(ctx->video_width, ctx->video_height, ctx->video_format);
        }
#endif
        break;
    case JLGPU_FILTER_RED:
        lv_obj_set_style_image_recolor(ctx->img_obj[0], FILTER_RED, 0);
        lv_obj_set_style_image_recolor_opa(ctx->img_obj[0], LV_OPA_50, 0);
        break;
    case JLGPU_FILTER_GREEN:
        lv_obj_set_style_image_recolor(ctx->img_obj[0], FILTER_GREEN, 0);
        lv_obj_set_style_image_recolor_opa(ctx->img_obj[0], LV_OPA_50, 0);
        break;
    case JLGPU_FILTER_YELLOW:
        lv_obj_set_style_image_recolor(ctx->img_obj[0], FILTER_YELLOW, 0);
        lv_obj_set_style_image_recolor_opa(ctx->img_obj[0], LV_OPA_50, 0);
        break;
    case JLGPU_FILTER_GRAY:
        break;
    case JLGPU_FILTER_SEPIA:
        lv_obj_set_style_image_recolor(ctx->img_obj[0], FILTER_SEPIA, 0);
        lv_obj_set_style_image_recolor_opa(ctx->img_obj[0], LV_OPA_50, 0);
        break;
    case JLGPU_FILTER_NEGATIVE:
        /* Invert mask overlay  white, shown only during invert */
        if (!ctx->invert_overlay) {
            ctx->invert_overlay = lv_obj_create(parent);
        }
        lv_obj_remove_style_all(ctx->invert_overlay);
        lv_obj_set_size(ctx->invert_overlay, ctx->video_width, ctx->video_height);
        lv_obj_center(ctx->invert_overlay);
        lv_obj_set_style_bg_color(ctx->invert_overlay, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(ctx->invert_overlay, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->invert_overlay, 0, 0);
        lv_obj_clear_flag(ctx->invert_overlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_move_background(ctx->invert_overlay);

        lv_obj_clear_flag(ctx->invert_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_image_set_blend_mode(ctx->img_obj[0], LV_BLEND_MODE_SUBTRACTIVE);
        break;
    default:
        lv_obj_set_size(ctx->img_obj[0], width, height);
        lv_image_set_inner_align(ctx->img_obj[0], LV_IMAGE_ALIGN_STRETCH);
        LV_LOG_ERROR("Invalid effect type");
        break;
    }

    ctx->effect_type = effect_type;
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _lv_video_obj_async_flush(void *p)
{
    /* RPC 消息可能滞后到 deinit 释放 ctx 之后才执行, 不能依赖消息里携带的
     * ctx 指针; 改为按 fb_index 重新查 g_instances, 查不到说明已注销, 安全跳过 */
    u8 fb_index = (u8)(int)p;
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)lv_port_video_disp_get_instance_by_fb_index(fb_index);

    if (ctx == NULL || ctx->img_obj[0] == NULL) {
        return;
    }

    /* 首帧到达即显示 img: HIDDEN 对象 invalidate 不触发渲染,
     * render_start 回调因此不执行, 首帧永远不置位 -> 画面一直空白 */
    if (!ctx->first_frame_shown) {
        ctx->first_frame_shown = 1;
        lv_obj_clear_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN);
    }

    for (int i = 0; i < ctx->img_obj_num; i++) {
        if (ctx->img_obj[i]) {
            lv_obj_invalidate(ctx->img_obj[i]);
        }
    }

    ctx->async_flushing = 0;
}

int fb_data_updata(u8 fb_index, u8 *baddr)
{
    lv_video_disp_ctx_t *ctx = NULL;

    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_instances[i] && g_instances[i]->fb_index == (uint8_t)fb_index) {
            ctx = g_instances[i];
            break;
        }
    }

    if (ctx == NULL) {
        return 0;
    }

    if (ctx->async_flushing) {
        return 0;
    }
    ctx->async_flushing = 1;

#if LV_VIDEO_DISP_FB_MODE
    extern void disp_flush_post_msg(void);
    disp_flush_post_msg();
#else
    int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);
    int err = lvgl_rpc_post_func((void *)_lv_video_obj_async_flush, 1, (int)fb_index);
    if (err != 0) {
        printf("post async flush failed\n");
        ctx->async_flushing = 0;
    }
#endif

    return 0;
}

struct fb_map_user *lv_get_next_video_frame(lv_video_disp_handle_t *handle)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;

    if (ctx && ctx->fb_dev) {
        dev_ioctl(ctx->fb_dev, FBIOGET_FBUFFER_IDLE, (int)&ctx->fb_map);
        if ((u32)ctx->fb_map.baddr) {
            return &ctx->fb_map;
        }
    }
    return NULL;
}

int lv_put_next_video_frame(lv_video_disp_handle_t *handle, struct fb_map_user *fb_map)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)handle;

    if (ctx && ctx->fb_dev) {
        return dev_ioctl(ctx->fb_dev, FBIOPUT_FBUFFER_IDLE, (int)fb_map);
    }
    return -1;
}

static void _lv_video_disp_render_end_cb(void *user_data)
{
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)user_data;

    if (ctx && ctx->fb_dev && ctx->fb_map.baddr != NULL) {
        dev_ioctl(ctx->fb_dev, FBIOPUT_FBUFFER_IDLE, (int)&ctx->fb_map);
        ctx->fb_map.baddr = NULL;
    }
}

static lv_result_t lv_port_video_disp_update_frame(lv_video_disp_ctx_t *ctx)
{
    uint32_t stride = 0;

    struct fb_map_user *fb_map = lv_get_next_video_frame((lv_video_disp_handle_t *)ctx);
    void *frame_buf = fb_map->baddr;
    void *frame_uv_buf = fb_map->uv_baddr;

    if (fb_map == NULL || frame_buf == NULL) {
        return LV_RESULT_INVALID;
    }

    if (ctx->video_format == LV_COLOR_FORMAT_I420 || ctx->video_format == LV_COLOR_FORMAT_NV12) {
        stride = ctx->video_width;
    } else {
        stride = lv_draw_buf_width_to_stride(ctx->video_width, ctx->video_format);
    }

#if LV_USE_DRAW_JLGPU_LIST
    if (ctx->effect_type == JLGPU_FILTER_FISHEYE_NORMAL && ctx->fisheye_frame_out && ctx->fisheye_lut) {
        fisheye_process(NULL, frame_buf, ctx->fisheye_frame_out,
                        ctx->video_width, ctx->video_height, ctx->video_format, ctx->fisheye_lut);
        frame_buf = ctx->fisheye_frame_out;
    }

    if (ctx->effect_type == JLGPU_FILTER_HONEYCOMB_NORMAL && ctx->honeycomb_frame_out && ctx->honeycomb_lut) {
        honeycomb_process(NULL, frame_buf, ctx->honeycomb_frame_out,
                          ctx->video_width, ctx->video_height, ctx->video_format, ctx->honeycomb_lut);
        frame_buf = ctx->honeycomb_frame_out;
    }
#endif

    if (ctx->effect_type == JLGPU_FILTER_GRAY) {
        if (ctx->video_format == LV_COLOR_FORMAT_I420 || ctx->video_format == LV_COLOR_FORMAT_NV12) {
            memset((uint8_t *)frame_buf + ctx->video_width * ctx->video_height,
                   128, ctx->video_width * ctx->video_height / 2);
        } else {
            printf("uv128 not supported this format %d\n", ctx->video_format);
        }
    }

    if (lv_port_video_disp_set_src((lv_video_disp_handle_t *)ctx,
                                   frame_buf, frame_uv_buf,
                                   ctx->video_width, ctx->video_height,
                                   ctx->video_format, stride) != 0) {
        return LV_RESULT_INVALID;
    }

    lv_port_video_disp_set_render_end_cb((lv_video_disp_handle_t *)ctx,
                                         _lv_video_disp_render_end_cb, ctx);

    return LV_RESULT_OK;
}
/**
 * @brief 渲染开始事件回调
 * 在整个disp开始渲染前调用，此时更新img控件的src
 */
static void video_disp_render_start_event_cb(lv_event_t *e)
{
    (void)e;

    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        lv_video_disp_ctx_t *ctx = g_instances[i];
        if (ctx == NULL || ctx->img_obj[0] == NULL) {
            continue;
        }

        /* 仅当 img 所在屏为当前活动屏才取帧: 退 DEC 后 home 重绘也会触发本回调,
         * 此时 img 挂在已卸载的 DEC 屏上, 空取帧会把双缓冲器耗尽并刷
         * "fb_get_fbuffer_idle: no available buffer" (参考官方 demo 同款守卫)。 */
        if (lv_obj_get_screen(ctx->img_obj[0]) != lv_screen_active()) {
            continue;
        }

        lv_result_t res = lv_port_video_disp_update_frame(ctx);
        if (res != LV_RESULT_OK) {
            continue;
        }

        /* 手动隐藏后无 render, 立即放回 fb buffer 防双缓冲耗尽; 不能只判 HIDDEN, 否则误伤首帧 */
        if (ctx->first_frame_shown && lv_obj_has_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN)) {
            _lv_video_disp_render_end_cb(ctx->render_end_user_data);
            ctx->render_end_cb = NULL;
            continue;
        }

        /* 首帧到达时自动显示一次, 之后显隐由 lv_port_video_disp_show() 控制 */
        if (!ctx->first_frame_shown) {
            ctx->first_frame_shown = 1;
            lv_obj_clear_flag(ctx->img_obj[0], LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->need_update && ctx->video_dsc != NULL && ctx->video_dsc->data != NULL) {
            for (int j = 0; j < ctx->img_obj_num; j++) {
                if (ctx->img_obj[j]) {
                    lv_image_set_src(ctx->img_obj[j], ctx->video_dsc);
                }
            }
            ctx->need_update = 0;
        }
    }
}

static void video_disp_render_ready_event_cb(lv_event_t *e)
{
    (void)e;

    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        lv_video_disp_ctx_t *ctx = g_instances[i];
        if (ctx == NULL) {
            continue;
        }

        if (ctx->render_end_cb) {
            ctx->render_end_cb(ctx->render_end_user_data);
        }
    }
}

static void video_disp_flush_start_event_cb(lv_event_t *e)
{
    (void)e;
}


#if 0
/**********************
 *  模拟测试流程 (SIMULATION TEST) — 多路支持
 **********************/

struct lv_video_sim_ctx {
    lv_video_disp_handle_t *handle;
    void *fb_draw;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t frame_cnt;
    volatile int running;
    int pid;
    jlgpu_filter_session_t *filter_sess;
};

static lv_video_sim_ctx_t *g_sim_ctxs[LV_VIDEO_DISP_MAX_INSTANCES];

/* ============ YUV422(YUYV) 图案生成器 ============ */

/**
 * @brief 生成YUV422色条测试图案
 */
static void sim_gen_color_bars_yuv422(uint8_t *buf, uint32_t w, uint32_t h, uint32_t frame_idx)
{
    uint32_t bar_width = w / 7; /* 7条色带: 白/黄/青/绿/品红/红/蓝 */
    uint32_t x, y;
    uint32_t bar_idx;

    /* YUV422(YUYV): 每2像素占4字节 Y0:U:Y1:V */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += 2) {
            bar_idx = x / bar_width;
            if (bar_idx > 6) {
                bar_idx = 6;
            }

            uint8_t Y0, U, Y1, V;
            uint32_t offset = y * w * 2 + x * 2;

            switch (bar_idx) {
            case 0: /* 白 */
                Y0 = 235;
                U = 128;
                Y1 = 235;
                V = 128;
                break;
            case 1: /* 黄 */
                Y0 = 210;
                U = 16;
                Y1 = 210;
                V = 146;
                break;
            case 2: /* 青 */
                Y0 = 170;
                U = 166;
                Y1 = 170;
                V = 16;
                break;
            case 3: /* 绿 */
                Y0 = 145;
                U = 54;
                Y1 = 145;
                V = 34;
                break;
            case 4: /* 品红 */
                Y0 = 106;
                U = 202;
                Y1 = 106;
                V = 222;
                break;
            case 5: /* 红 */
                Y0 = 81;
                U = 90;
                Y1 = 81;
                V = 240;
                break;
            case 6: /* 蓝 */
                Y0 = 41;
                U = 240;
                Y1 = 41;
                V = 110;
                break;
            default:
                Y0 = 128;
                U = 128;
                Y1 = 128;
                V = 128;
                break;
            }

            /* 移动竖线效果(白色) */
            uint32_t moving_line = (frame_idx * 4) % w;
            if (x == moving_line || x + 1 == moving_line) {
                Y0 = 235;
                U = 128;
                Y1 = 235;
                V = 128;
            }

            buf[offset + 0] = Y0;
            buf[offset + 1] = U;
            buf[offset + 2] = Y1;
            buf[offset + 3] = V;
        }
    }
}

/**
 * @brief 生成YUV422移动渐变测试图案
 */
static void sim_gen_gradient_yuv422(uint8_t *buf, uint32_t w, uint32_t h, uint32_t frame_idx)
{
    uint32_t x, y;
    uint32_t center_x = w / 2;
    uint32_t center_y = h / 2;
    uint32_t max_dist = (center_x > center_y) ? center_x : center_y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += 2) {
            int32_t dx = (int32_t)(x - center_x);
            int32_t dy = (int32_t)(y - center_y);
            uint32_t dist = (uint32_t)((dx * dx + dy * dy) / (max_dist * max_dist / 256));
            if (dist > 255) {
                dist = 255;
            }

            uint32_t hue = ((frame_idx * 2) + dist) % 256;
            uint8_t Y0, U, Y1, V;
            uint32_t offset = y * w * 2 + x * 2;

            Y0 = (uint8_t)(128 + dist / 2);
            Y1 = Y0;
            U = (uint8_t)(128 + (int8_t)(127 * (int32_t)(hue < 128 ? hue : 256 - hue) / 128));
            V = (uint8_t)(128 + (int8_t)(127 * (int32_t)((hue + 64) % 256 < 128 ? (hue + 64) % 256 : 256 - (hue + 64) % 256) / 128));

            buf[offset + 0] = Y0;
            buf[offset + 1] = U;
            buf[offset + 2] = Y1;
            buf[offset + 3] = V;
        }
    }
}

/* ============ NV12(YUV420SP) 图案生成器 ============ */

/* ITU-R BT.601 标准色条 Y/U/V 分量值 */
static const uint8_t sim_nv12_bar_y[7] = {235, 210, 170, 145, 106, 81, 41};
static const uint8_t sim_nv12_bar_u[7] = {128, 16,  166, 54,  202, 90, 240};
static const uint8_t sim_nv12_bar_v[7] = {128, 146, 16,  34,  222, 240, 110};

/**
 * @brief 生成NV12色条测试图案 - Y平面(全分辨率)
 */
static void sim_gen_color_bars_nv12_y(uint8_t *y_buf, uint32_t w, uint32_t h, uint32_t frame_idx)
{
    uint32_t bar_width = w / 7;
    uint32_t x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            uint32_t bar_idx = x / bar_width;
            if (bar_idx > 6) {
                bar_idx = 6;
            }
            uint8_t Y = sim_nv12_bar_y[bar_idx];

            /* 移动竖线(白色) */
            uint32_t moving_line = (frame_idx * 4) % w;
            if (x == moving_line) {
                Y = 235;
            }

            y_buf[y * w + x] = Y;
        }
    }
}

/**
 * @brief 生成NV12色条测试图案 - UV平面(半分辨率, U和V交错: U0 V0 U1 V1 ...)
 */
static void sim_gen_color_bars_nv12_uv(uint8_t *uv_buf, uint32_t w, uint32_t h)
{
    uint32_t bar_width = w / 7;
    uint32_t uv_w = w / 2;
    uint32_t uv_h = h / 2;
    uint32_t x, y;

    for (y = 0; y < uv_h; y++) {
        for (x = 0; x < uv_w; x++) {
            /* 将UV采样点映射回全分辨率色条索引 */
            uint32_t bar_idx = (x * 2) / bar_width;
            if (bar_idx > 6) {
                bar_idx = 6;
            }

            uv_buf[y * uv_w * 2 + x * 2 + 0] = sim_nv12_bar_u[bar_idx];
            uv_buf[y * uv_w * 2 + x * 2 + 1] = sim_nv12_bar_v[bar_idx];
        }
    }
}

/**
 * @brief 生成NV12移动渐变测试图案 - Y平面(全分辨率)
 */
static void sim_gen_gradient_nv12_y(uint8_t *y_buf, uint32_t w, uint32_t h, uint32_t frame_idx)
{
    uint32_t center_x = w / 2;
    uint32_t center_y = h / 2;
    uint32_t max_dist = (center_x > center_y) ? center_x : center_y;
    uint32_t x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int32_t dx = (int32_t)(x - center_x);
            int32_t dy = (int32_t)(y - center_y);
            uint32_t dist = (uint32_t)((dx * dx + dy * dy) / (max_dist * max_dist / 256));
            if (dist > 255) {
                dist = 255;
            }

            /* 亮度随距离变化 + 帧旋转效果 */
            uint32_t angle = (frame_idx * 2 + dist) % 256;
            y_buf[y * w + x] = (uint8_t)(128 + (dist * (128 + (int8_t)(angle < 128 ? angle : 256 - angle))) / 256);
        }
    }
}

/**
 * @brief 生成NV12移动渐变测试图案 - UV平面(半分辨率)
 */
static void sim_gen_gradient_nv12_uv(uint8_t *uv_buf, uint32_t w, uint32_t h, uint32_t frame_idx)
{
    uint32_t center_x = w / 4;  /* UV平面宽度为全分辨率的1/2 */
    uint32_t center_y = h / 4;  /* UV平面高度为全分辨率的1/2 */
    uint32_t max_dist = (center_x > center_y) ? center_x : center_y;
    uint32_t uv_w = w / 2;
    uint32_t uv_h = h / 2;
    uint32_t x, y;

    if (max_dist == 0) {
        max_dist = 1;
    }

    for (y = 0; y < uv_h; y++) {
        for (x = 0; x < uv_w; x++) {
            int32_t dx = (int32_t)(x - center_x);
            int32_t dy = (int32_t)(y - center_y);
            uint32_t dist = (uint32_t)((dx * dx + dy * dy) * 256 / (max_dist * max_dist));
            if (dist > 255) {
                dist = 255;
            }

            uint32_t hue = (frame_idx * 3 + dist) % 256;
            uint32_t offset = y * uv_w * 2 + x * 2;

            uv_buf[offset + 0] = (uint8_t)(128 + (int8_t)(127 * (int32_t)(hue < 128 ? hue : 256 - hue) / 128));
            uv_buf[offset + 1] = (uint8_t)(128 + (int8_t)(127 * (int32_t)((hue + 85) % 256 < 128 ? (hue + 85) % 256 : 256 - (hue + 85) % 256) / 128));
        }
    }
}

/* ============ 测试线程 ============ */

/**
 * @brief 模拟测试线程
 * 通过fb_draw_getmap获取可用buf，填充YUV数据后通过fb_draw_putmap推送给硬件刷新
 */
static void sim_test_thread(void *arg)
{
    struct fb_map_user map;
    lv_video_sim_ctx_t *sim = (lv_video_sim_ctx_t *)arg;

    printf("sim_test: thread started\n");

    while (sim->running) {
        /* 检查kill请求 */
        if (thread_kill_req()) {
            break;
        }

        /* 获取可用帧缓冲区 */
        memset(&map, 0, sizeof(map));
        if (fb_draw_getmap(sim->fb_draw, &map) != 0) {
            printf("sim_test: fb_draw_getmap failed\n");
            os_time_dly(1);
            continue;
        }

        /* 根据格式选择图案生成器，每5秒切换色条/渐变 */
        uint32_t pattern_sel = (sim->frame_cnt / 150) % 2; /* 150帧@30fps = 5秒 */

        if (sim->format == LV_COLOR_FORMAT_YUY2) {
            /* YUV422: 单平面打包格式, baddr指向整个buffer */
            uint8_t *buf = map.baddr;
            if (buf == NULL) {
                goto put_map;
            }
            if (pattern_sel == 0) {
                sim_gen_color_bars_yuv422(buf, sim->width, sim->height, sim->frame_cnt);
            } else {
                sim_gen_gradient_yuv422(buf, sim->width, sim->height, sim->frame_cnt);
            }
        } else {
            /* NV12(YUV420SP): 双平面, yaddr=Y平面, uaddr=UV交错平面 */
            uint8_t *uv_buf = NULL;
            uint8_t *y_buf  = map.baddr;
            if (map.uv_baddr) {
                uv_buf = map.uv_baddr;
            } else {
                uv_buf = map.baddr + sim->width * sim->height;
            }
            if (y_buf == NULL || uv_buf == NULL) {
                goto put_map;
            }
            if (pattern_sel == 0) {
                sim_gen_color_bars_nv12_y(y_buf, sim->width, sim->height, sim->frame_cnt);
                sim_gen_color_bars_nv12_uv(uv_buf, sim->width, sim->height);
            } else {
                sim_gen_gradient_nv12_y(y_buf, sim->width, sim->height, sim->frame_cnt);
                sim_gen_gradient_nv12_uv(uv_buf, sim->width, sim->height, sim->frame_cnt);
            }
        }

        sim->frame_cnt++;

        DcuFlushinvRegion((void *)map.baddr, map.width * map.height * 2);
#if 0
        /* 执行滤镜  */
        char *sticker_path = "storage/res_nor_mode/C/lvgl/4b0000f5.zip";
        jlgpu_sticker_t sticker = {
            .x = 0,
            .y = 0,
            .path = sticker_path,
        };
        jlgpu_filter_session_process(sim->filter_sess, map.baddr, map.baddr,
                                     JLGPU_FILTER_STICKER, &sticker);
#endif
        printf("sim_test: frame %d\n", sim->frame_cnt);
        /* 推送缓冲区到硬件刷新 */
        fb_draw_putmap(sim->fb_draw, &map);

        /* ~30fps帧率 */
        os_time_dly(3);
        continue;

put_map:
        fb_draw_putmap(sim->fb_draw, &map);
        os_time_dly(1);
    }

    if (sim->filter_sess) {
        jlgpu_filter_session_uninit(sim->filter_sess);
    }

    printf("sim_test: thread exit\n");
}

/* ============ 启动/停止接口 ============ */

/**
 * @brief 启动视频流模拟测试
 * @param width      视频宽度
 * @param height     视频高度
 * @param fb_format  fb像素格式(FB_COLOR_FORMAT_YUV422 或 FB_COLOR_FORMAT_YUV420P 用于NV12)
 *
 * 使用说明:
 * 1. 在ui界面初始化阶段调用 lv_port_video_disp_sim_test_start(x, y, w, h, fmt, fb_name) 启动测试
 * 2. 测试线程自动生成YUV色条/渐变图案，通过fb_draw_putmap推送到硬件显示
 * 3. 调用 lv_port_video_disp_sim_test_stop(ctx) 停止测试
 *
 * 示例:
 *   lv_video_sim_ctx_t *sim = lv_port_video_disp_sim_test_start(0, 0, 320, 240, LV_COLOR_FORMAT_YUY2, "fb5");
 *   lv_port_video_disp_sim_test_stop(sim);
 */
lv_video_sim_ctx_t *lv_port_video_disp_sim_test_start(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t fb_format, const char *fb_name)
{
    struct fb_draw_info info;
    int slot = -1;

    /* 分配 sim 槽位 */
    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_sim_ctxs[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        printf("sim_test: no available sim slot\n");
        return NULL;
    }

    lv_video_sim_ctx_t *sim = (lv_video_sim_ctx_t *)lv_malloc_zeroed(sizeof(lv_video_sim_ctx_t));
    if (sim == NULL) {
        return NULL;
    }
    g_sim_ctxs[slot] = sim;

    /* 步骤1: 创建视频流控件 */
    sim->handle = lv_port_video_disp_create(NULL, width, height, x, y, fb_format, fb_name);
    if (sim->handle == NULL) {
        printf("sim_test: create video disp failed!\n");
        lv_free(sim);
        g_sim_ctxs[slot] = NULL;
        return NULL;
    }

    /* 步骤2: fb_draw_open打开fb设备 */
    memset(&info, 0, sizeof(info));
    lv_video_disp_ctx_t *ctx = (lv_video_disp_ctx_t *)sim->handle;
    info.name   = ctx->fb_name;
    info.width  = width;
    info.height = height;
    info.format = (fb_format == LV_COLOR_FORMAT_YUY2) ? FB_COLOR_FORMAT_YUV422 : FB_COLOR_FORMAT_YUV420P;
    info.fb_num = 2;  /* 双缓冲 */
    info.combine = 1;

    sim->fb_draw = fb_draw_open(&info);
    if (sim->fb_draw == NULL) {
        printf("sim_test: fb_draw_open failed!\n");
        lv_port_video_disp_del(sim->handle);
        lv_free(sim);
        g_sim_ctxs[slot] = NULL;
        return NULL;
    }

    sim->width  = width;
    sim->height = height;
    sim->format = fb_format;
    sim->frame_cnt = 0;


    /* 步骤3: thread_fork创建测试线程，~30fps生成YUV数据并通过fb_draw_putmap推送 */
    sim->running = 1;
    char task_name[16];
    snprintf(task_name, sizeof(task_name), "sim_video_%d", slot);

    printf("%s: video disp created, fb_draw_open ok, w=%d h=%d fmt=0x%x\n", task_name,
           width, height, fb_format);

    if (thread_fork(task_name, 20, 2048, 0, &sim->pid, sim_test_thread, sim) != 0) {
        printf("%s: thread_fork failed!\n", task_name);
        sim->running = 0;
        fb_draw_close(sim->fb_draw);
        sim->fb_draw = NULL;
        lv_port_video_disp_del(sim->handle);
        lv_free(sim);
        g_sim_ctxs[slot] = NULL;
        return NULL;
    }

    /* 初始化 session */
    uint8_t gpu_format = (fb_format == LV_COLOR_FORMAT_YUY2) ? JLGPU_FORMAT_YUV422_BT601 : JLGPU_FORMAT_YUV420_BT601;
    sim->filter_sess = jlgpu_filter_session_init(width, height,
                       gpu_format, gpu_format);
    if (!sim->filter_sess) {
        printf("%s: filter session init failed\n", task_name);
        return sim;
    }

    printf("%s: started (30fps, fmt=0x%x, pid=%d, fb_draw mode)\n", task_name, fb_format, sim->pid);
    return sim;
}

/**
 * @brief 停止视频流模拟测试
 */
void lv_port_video_disp_sim_test_stop(lv_video_sim_ctx_t *sim)
{
    if (sim == NULL) {
        return;
    }

    /* 从全局表中移除 */
    for (int i = 0; i < LV_VIDEO_DISP_MAX_INSTANCES; i++) {
        if (g_sim_ctxs[i] == sim) {
            g_sim_ctxs[i] = NULL;
            break;
        }
    }

    /* 通知线程退出 */
    sim->running = 0;
    if (sim->pid) {
        thread_kill(&sim->pid, KILL_WAIT);
        sim->pid = 0;
    }
    if (sim->fb_draw) {
        fb_draw_close(sim->fb_draw);
        sim->fb_draw = NULL;
    }
    if (sim->handle) {
        lv_port_video_disp_del(sim->handle);
        sim->handle = NULL;
    }
    sim->frame_cnt = 0;
    sim->width = 0;
    sim->height = 0;
    sim->format = 0;

    lv_free(sim);
    printf("sim_test: stopped\n");
}
#endif

#endif
