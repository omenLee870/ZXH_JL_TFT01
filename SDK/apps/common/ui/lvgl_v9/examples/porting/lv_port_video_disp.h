/**
 * @file lv_port_video_disp.h
 * @brief LVGL视频流显示功能头文件（多实例版本）
 */

#ifndef LV_PORT_VIDEO_DISP_H
#define LV_PORT_VIDEO_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../../lvgl.h"
#endif


#if LV_USE_VIDEO_DISP
/*********************
 *      DEFINES
 *********************/

/* 最大实例数，可通过 lv_conf.h 或编译选项覆盖 */
#ifndef LV_VIDEO_DISP_MAX_INSTANCES
#define LV_VIDEO_DISP_MAX_INSTANCES 3
#endif

/**********************
 *      TYPEDEFS
 **********************/

/* 渲染结束回调函数类型 */
typedef void (*lv_video_render_end_cb_t)(void *user_data);

/* 不透明 handle 类型 */
typedef struct lv_video_disp_ctx lv_video_disp_handle_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief 创建视频显示实例
 * @param parent 父对象，NULL则创建到默认屏幕
 * @param width  视频显示宽度
 * @param height 视频显示高度
 * @param x      位置X
 * @param y      位置Y
 * @param format 视频颜色格式（如LV_COLOR_FORMAT_RGB565, LV_COLOR_FORMAT_RGB888等）
 * @param fb_name fb 设备名（如 "fb5"）
 * @return handle 指针，失败返回 NULL
 */
    lv_video_disp_handle_t *lv_port_video_disp_create(lv_obj_t *parent, int32_t width, int32_t height,
    int32_t x, int32_t y, lv_color_format_t format,
    const char *fb_name);

/**
 * @brief 删除视频显示实例
 * @param handle 实例 handle
 */
    void lv_port_video_disp_del(lv_video_disp_handle_t *handle);

/**
 * @brief 设置视频源
 * @param frame_buf 视频帧缓冲区地址
 * @param frame_uv_buf 视频帧uv缓冲区地址
 * @param width 视频宽度
 * @param height 视频高度
 * @param format 视频颜色格式（如LV_COLOR_FORMAT_RGB565, LV_COLOR_FORMAT_RGB888等）
 * @param stride 视频行步长（字节），如果为0则自动计算
 * @return 0表示成功，非0表示失败
 */
    int lv_port_video_disp_set_src(lv_video_disp_handle_t *handle, void *frame_buf, void *frame_uv_buf,
    uint32_t width, uint32_t height, lv_color_format_t format, uint32_t stride);

/**
 * @brief 获取 img 控件对象
 */
    lv_obj_t *lv_port_video_disp_get_img_obj(lv_video_disp_handle_t *handle);

/**
 * @brief 设置渲染结束回调
 */
    void lv_port_video_disp_set_render_end_cb(lv_video_disp_handle_t *handle,
    lv_video_render_end_cb_t cb, void *user_data);

/**
 * @brief 设置 img 控件位置
 */
    void lv_port_video_disp_set_pos(lv_video_disp_handle_t *handle, int32_t x, int32_t y);

/**
 * @brief 设置 img 控件大小
 */
    void lv_port_video_disp_set_size(lv_video_disp_handle_t *handle, int32_t width, int32_t height);

/**
 * @brief 显示/隐藏 img 控件
 */
    void lv_port_video_disp_show(lv_video_disp_handle_t *handle, uint8_t show);

/**
 * @brief 标记异步刷新完成
 */
    void lv_port_video_disp_ready(lv_video_disp_handle_t *handle);

/**
 * @brief 获取下一个视频帧
 */
    struct fb_map_user *lv_get_next_video_frame(lv_video_disp_handle_t *handle);

    /**
     * @brief 释放视频帧
     */
        int lv_put_next_video_frame(lv_video_disp_handle_t *handle, struct fb_map_user *fb_map);

        /**
         * @brief 设置视频特效
         */
            void lv_port_video_set_effect(lv_video_disp_handle_t *handle, uint8_t effect_type,
            const void *src, int32_t x, int32_t y);

        /**
         * @brief 按 fb_index 查询实例 handle（供 lv_port_disp.c 等使用）
         * @param fb_index fb 设备索引号
         * @return handle 指针，未找到返回 NULL
         */
            lv_video_disp_handle_t *lv_port_video_disp_get_instance_by_fb_index(uint8_t fb_index);

        /**
         * @brief 启动视频流模拟测试（多路）
         * @return sim 上下文指针，失败返回 NULL
         */
            typedef struct lv_video_sim_ctx lv_video_sim_ctx_t;
                lv_video_sim_ctx_t *lv_port_video_disp_sim_test_start(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t fb_format, const char *fb_name);

            /**
             * @brief 停止视频流模拟测试
             */
                void lv_port_video_disp_sim_test_stop(lv_video_sim_ctx_t *sim_ctx);

            /**********************
             *      MACROS
             **********************/

#endif

#ifdef __cplusplus
            } /*extern "C"*/
#endif

#endif /*LV_PORT_VIDEO_DISP_H*/
