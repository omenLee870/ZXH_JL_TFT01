/**
 * @file    jlgpu_image_filters.h
 * @brief   GPU 图像滤镜特效 API
 *
 * 提供基于 JLGPU 硬件加速的常用图像滤镜处理接口:
 * - 灰度滤镜
 * - 胶片反相滤镜
 * - 颜色着色滤镜 (GAIN 模式)
 * - 怀旧棕褐滤镜 (Sepia 预设)
 *
 * 输入格式支持: JLGPU_FORMAT_YUV420_BT601 (NV12) / JLGPU_FORMAT_YUV422_BT601 (YUYV)
 * 输出格式: 通过 out_format 灵活指定 (RGB565 / ARGB8888 / RGB888 等)
 */

#ifndef JLGPU_IMAGE_FILTERS_H
#define JLGPU_IMAGE_FILTERS_H

#include "jlgpu_framework.h"

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../../lvgl.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define JLGPU_FILTER_DEFAULT_BATCH 16

/** 滤镜模式 */
typedef enum {
    JLGPU_FILTER_NONE = 0,          /**< 无特效 */
    JLGPU_FILTER_STICKER,           /**< 贴纸合成 */
    JLGPU_FILTER_RED,               /**< 红色着色滤镜 */
    JLGPU_FILTER_GREEN,             /**< 绿色着色滤镜 */
    JLGPU_FILTER_YELLOW,            /**< 黄色着色滤镜 */
    JLGPU_FILTER_GRAY,              /**< 黑白灰度滤镜 */
    JLGPU_FILTER_SEPIA,             /**< 怀旧棕褐滤镜 */
    JLGPU_FILTER_NEGATIVE,          /**< 胶片反相滤镜 */
    JLGPU_FILTER_TILE_HORIZ,        /**< 水平左右复制 */
    JLGPU_FILTER_TILE_VERT,         /**< 垂直上下复制 */
    JLGPU_FILTER_MIRROR_HORIZ,      /**< 水平左右镜像 */
    JLGPU_FILTER_MIRROR_VERT,       /**< 垂直上下镜像 */
    JLGPU_FILTER_GRID_4,            /**< 4宫格 */
    JLGPU_FILTER_GRID_9,            /**< 9宫格 */
    JLGPU_FILTER_HONEYCOMB_NORMAL,  /**< 6边形蜂窝阵列*/
    JLGPU_FILTER_FISHEYE_NORMAL,    /**< 标准圆形鱼眼 */
} jlgpu_filter_mode_t;

/** 贴纸参数 */
typedef struct {
    const char *path;   /**< 贴纸图像文件路径 */
    int   x, y;         /**< 贴纸目标位置 (左上角) */
    int   dst_w;        /**< 目标宽度 (0=使用源图宽度, 不启用变换) */
    int   dst_h;        /**< 目标高度 (0=使用源图高度, 不启用变换) */
    float angle;        /**< 旋转角度(度), 绕贴纸中心顺时针旋转 (0= 不启用变换)*/
} jlgpu_sticker_t;

typedef struct jlgpu_filter_session jlgpu_filter_session_t;

/**
 * @brief 分行处理回调 (每处理完 batch_lines 行后调用)
 *
 * @param sess       会话句柄
 * @param dst        目标缓冲区 (指向当前批次的起始位置)
 * @param w          图像宽度
 * @param h          图像高度
 * @param line_start 当前批次起始行号
 * @param line_count 当前批次行数
 * @param user_data  用户数据指针
 */
typedef void (*jlgpu_filter_line_cb_t)(jlgpu_filter_session_t *sess,
                                       uint8_t *dst,
                                       int w, int h,
                                       int line_start,
                                       int line_count,
                                       void *user_data);

typedef struct {
    int width;
    int height;
    float strength;
    int16_t *y_lut_x;        // size = W * batch_lines
    int16_t *y_lut_y;        // size = W * batch_lines
    int16_t *uv_lut_x;       // size = (W/2) * (batch_lines/2)
    int16_t *uv_lut_y;       // size = (W/2) * (batch_lines/2)
    int     batch_lines;     // 每批处理行数 (控制内存占用)
} fisheye_lut_t;

typedef struct {
    int width;
    int height;
    int cell_size;
    int16_t *sample_x;        // 每个像素的采样X坐标, size = W * batch_lines
    int16_t *sample_y;        // 每个像素的采样Y坐标, size = W * batch_lines
    int     batch_lines;      // 每批处理行数 (控制内存占用)
} honeycomb_lut_t;


uint8_t *filters_yuv_buffer_alloc(int w, int h, lv_color_format_t fmt);
void filters_yuv_buffer_free(uint8_t *free_buf);
size_t filters_yuv_buffer_size(int w, int h, lv_color_format_t fmt);

/* ======================== 鱼眼镜头效果 ======================== */
fisheye_lut_t *fisheye_lut_create(int w, int h, float strength, int batch_lines);
/**
 * @brief 鱼眼镜头效果处理
 * @param src             源图像
 * @param batch_buf       输出缓冲区
 * @param w               图像宽度（必须为偶数）
 * @param h               图像高度（必须为偶数）
 * @param fmt             颜色格式
 * @param lut             鱼眼LUT表
 * @return 0成功，-1失败
 */
int fisheye_process(jlgpu_filter_session_t *sess, const uint8_t *src, uint8_t *batch_buf, int w, int h, lv_color_format_t fmt,
                    fisheye_lut_t *lut);

void fisheye_lut_destroy(fisheye_lut_t *lut);

/* ======================== 蜂窝阵列效果 ======================== */
honeycomb_lut_t *honeycomb_lut_create(int w, int h, int cell_size, int batch_lines);//图像宽高必须为偶数
/**
 * @brief 蜂窝阵列效果处理
 * @param src             源图像
 * @param batch_buf       输出缓冲区
 * @param w               图像宽度（必须为偶数）
 * @param h               图像高度（必须为偶数）
 * @param fmt             颜色格式
 * @param lut             蜂窝LUT表
 * @return 0成功，-1失败
 */
int honeycomb_process(jlgpu_filter_session_t *sess, const uint8_t *src, uint8_t *batch_buf, int w, int h, lv_color_format_t fmt, honeycomb_lut_t *lut);

void honeycomb_lut_destroy(honeycomb_lut_t *lut);

/**
 * @brief 初始化滤镜会话, 预分配 GPU 上下文和命令缓冲
 *
 * 尺寸和格式在 init 时固定, 帧间通过 mode 切换滤镜效果。
 *
 * @param w          图像宽度
 * @param h          图像高度
 * @param src_format 源图像格式
 * @param dst_format 目标图像格式
 * @return           会话句柄, NULL 表示失败
 */
jlgpu_filter_session_t *jlgpu_filter_session_init(int w, int h,
        int src_format, int dst_format);

/**
 * @brief 执行滤镜处理
 *
 * @param sess   init 返回的会话句柄
 * @param src    源图像缓冲区
 * @param dst    目标图像缓冲区
 * @param mode      滤镜模式
 * @param user_data 用户数据指针 (STICKER 模式指向 jlgpu_sticker_t, 其他模式传 NULL)
 */
void jlgpu_filter_session_process(jlgpu_filter_session_t *sess,
                                  const uint8_t *src, uint8_t *dst,
                                  jlgpu_filter_mode_t mode,
                                  void *user_data);

/**
 * @brief 配置分行处理参数 (用于分片输出场景)
 *
 * 设置后, GPU 滤镜处理将按 batch_lines 行一组分批输出。
 * 每批处理完成后回调 line_cb, 用户可在回调中对当前批次的 dst 缓冲进行操作
 * (如 DMA 传输到编码等)。
 *
 * @param sess        会话句柄
 * @param batch_lines 每批处理的行数 (0=不分行, 一次性输出)
 * @param line_cb     分行回调 (可为 NULL)
 * @param user_data   回调用户数据
 */
void jlgpu_filter_session_set_line_config(jlgpu_filter_session_t *sess,
        int batch_lines,
        jlgpu_filter_line_cb_t line_cb,
        void *user_data);

/**
 * @brief 释放滤镜会话, 归还所有资源
 *
 * @param sess   init 返回的会话句柄
 */
void jlgpu_filter_session_uninit(jlgpu_filter_session_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* JLGPU_IMAGE_FILTERS_H */
