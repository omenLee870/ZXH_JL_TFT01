/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_radialgrad.c
 *
 * @brief 径向渐变任务相关示例
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-03-16
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"


#define GPU_RADIALGRAD_TASK_ID		1
#define GPU_RADIALGRAD_ELEMENT		1

#define GPU_RADIALGRAD_DRAW_W		128
#define GPU_RADIALGRAD_DRAW_H		128

#define GPU_RADIALGRAD_AFF_RATIO	0.5f
#define GPU_RADIALGRAD_AFF_ANGLE	30.0f

#define GPU_RADIALGRAD_PER_RATIO	0.5f
#define GPU_RADIALGRAD_PER_ANGLE	45.0f
#define GPU_RADIALGRAD_PER_DISTANCE	1.0f
#define GPU_RADIALGRAD_PER_FOVX		90.0f
#define GPU_RADIALGRAD_PER_FOVY		90.0f

#define GPU_RADIALGRAD_PATH_RADIUS	20



#if (defined GPU_DEMO_RADIALGRAD && GPU_DEMO_RADIALGRAD)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_task 径向渐变任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /* 设置渐变任务颜色表，颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义 */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式，参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于使能了仿射变换，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_AFFINE && GPU_DEMO_RADIALGRAD_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_affine_task 径向渐变仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /* 设置渐变任务颜色表，颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义 */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式，参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /* 设置仿射变换缩放系数 */
    jlgpu_task_gradient_set_affine_scale(GPU_RADIALGRAD_AFF_RATIO, GPU_RADIALGRAD_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 计算缩放后的宽、高大小 */
    int scale_w = w * GPU_RADIALGRAD_AFF_RATIO;
    int scale_h = h * GPU_RADIALGRAD_AFF_RATIO;

    /* 设置仿射变换旋转参数，旋转中心为正中心 */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_RADIALGRAD_AFF_ANGLE);

    /* 计算缩放后的坐标平移，把缩放后平移到控件中心 */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 由于使能了仿射变换，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_PERSPECTIVE && GPU_DEMO_RADIALGRAD_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_perspective_task 径向渐变透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，仿射、透视变换均可 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /* 设置渐变任务颜色表，颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义 */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式，参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_RADIALGRAD_PER_DISTANCE, GPU_RADIALGRAD_PER_FOVX, GPU_RADIALGRAD_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_MASK && GPU_DEMO_RADIALGRAD_MASK)
#include "img_data/img_data.h"

#define TEST_MASK_FMT	JLGPU_MASK_FORMAT_A4 /*! 定义要测试的MASK格式                           */
#define IMG_SOURCE_FMT	IMG_PIXEL_A4         /*! 定义作为MASK的纹理格式，注意，要与MASK_FMT一致 */

/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_mask_task 径向渐变+MASK任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_mask_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /* 设置渐变任务颜色表，颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义 */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式，参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于使能了仿射变换，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 设置MASK模式，外部纹理MASK */
    jlgpu_task_mask_set_enable();

    /* 获取MASK纹理格式 */
    int img_format = img_data_get_format(IMG_SOURCE_FMT);

    /* 获取MASK纹理宽高 */
    int img_w = img_data_get_width(img_format);
    int img_h = img_data_get_height(img_format);

    /* 设置MASK格式，注意这里是jlgpu_mask_format_t，不是jlgpu_tex_format_t */
    jlgpu_task_mask_set_format(TEST_MASK_FMT);

    /* 设置MASK宽高 */
    jlgpu_task_mask_set_size(img_w, img_h);

    /* 设置MASK纹理地址 */
    jlgpu_task_mask_set_data(img_data_get_addr(img_format));

    /* 设置MASK渲染区域 */
    jlgpu_task_mask_set_rect(x, y, img_w, img_h);

    /* 设置MASK数据的字节序 */
    jlgpu_task_mask_set_endian(img_data_get_endian(img_format));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_PATH && GPU_DEMO_RADIALGRAD_PATH)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_path_task 径向渐变+PATH任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_path_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为反射填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于使能了仿射变换，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0 + x, 0 + y, w, h, GPU_RADIALGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_PATH_AFFINE && GPU_DEMO_RADIALGRAD_PATH_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_path_affine_task 径向渐变+PATH仿射变换任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为反射填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /* 设置仿射变换缩放系数 */
    jlgpu_task_gradient_set_affine_scale(GPU_RADIALGRAD_AFF_RATIO, GPU_RADIALGRAD_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 计算缩放后的宽、高大小 */
    int scale_w = w * GPU_RADIALGRAD_AFF_RATIO;
    int scale_h = h * GPU_RADIALGRAD_AFF_RATIO;

    /* 设置仿射变换旋转参数，旋转中心为正中心 */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_RADIALGRAD_AFF_ANGLE);

    /* 计算缩放后的坐标平移，把缩放后平移到控件中心 */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 由于使能了仿射变换，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_RADIALGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量仿射变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 设置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(GPU_RADIALGRAD_AFF_RATIO, GPU_RADIALGRAD_AFF_RATIO);
    /* 计算缩放后输出的宽高 */
    int scale_out_w = w * GPU_RADIALGRAD_AFF_RATIO;
    int scale_out_h = h * GPU_RADIALGRAD_AFF_RATIO;
    /* 设置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(scale_out_w / 2, scale_out_h / 2, GPU_RADIALGRAD_AFF_ANGLE);
    /* 设置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x + dx, y + dy);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_RADIALGRAD_PATH_PERSPECTIVE && GPU_DEMO_RADIALGRAD_PATH_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_radialgrad_path_perspective_task 径向渐变+PATH透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_radialgrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为反射填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_REFLECT);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_RADIALGRAD_PER_DISTANCE, GPU_RADIALGRAD_PER_FOVX, GPU_RADIALGRAD_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_RADIALGRAD_DRAW_W);
    int h = GET_H(GPU_RADIALGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置径向渐变参数 */
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    int in_radius = w / 4;
    int out_radius = w / 2;
    jlgpu_task_gradient_set_radial(center_x, center_y, in_radius, out_radius);

    /* 设置渐变模式为径向渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_RADIAL_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_RADIALGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量透视变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);
    /* 配置透视变换缩放 */
    jlgpu_task_path_set_perspective_scale(GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO, GPU_RADIALGRAD_PER_RATIO);
    /* 配置透视变换旋转 */
    jlgpu_task_path_set_perspective_rotate(GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE, GPU_RADIALGRAD_PER_ANGLE);
    /* 配置透视变换视角 */
    jlgpu_task_path_set_perspective_camera(GPU_RADIALGRAD_PER_DISTANCE, GPU_RADIALGRAD_PER_FOVX, GPU_RADIALGRAD_PER_FOVY);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_RADIALGRAD_TASK_ID, GPU_RADIALGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



