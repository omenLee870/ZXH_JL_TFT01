/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_lineargrad.c
 *
 * @brief 线性渐变相关示例
 * 包含DEMO：
 * 1.线性渐变；
 * 2.线性渐变+仿射变换；
 * 3.线性渐变+透视变换；
 * 4.线性渐变+MASK
 * 5.线性渐变+PATH
 * 6.线性渐变+PATH+仿射变换
 * 7.线性渐变+PATH+透视变换
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-03-16
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"


#define GPU_LINEARGRAD_TASK_ID		1	  /*! 任务ID */
#define GPU_LINEARGRAD_ELEMENT		1 	  /*! 控件ID */

#define GPU_LINEARGRAD_DRAW_W		128   /*! 渲染宽度 */
#define GPU_LINEARGRAD_DRAW_H		128   /*! 渲染高度 */

#define GPU_LINEARGRAD_AFF_RATIO	0.5f  /*! 仿射变换缩放倍数 */
#define GPU_LINEARGRAD_AFF_ANGLE	30.0f /*! 仿射变换旋转角度 */

#define GPU_LINEARGRAD_PER_RATIO	0.5F  /*! 透视变换缩放倍数 */
#define GPU_LINEARGRAD_PER_ANGLE	45.0f /*! 透视变换旋转角度 */
#define GPU_LINEARGRAD_PER_DISTANCE	1.0f  /*! 透视变换视角距离 */
#define GPU_LINEARGRAD_PER_FOVX		90.0f /*! 透视变换 X视场角 */
#define GPU_LINEARGRAD_PER_FOVY		90.0f /*! 透视变换 Y视场角 */

#define GPU_LINEARGRAD_PATH_RADIUS	20    /*! 矢量路径圆角半径 */



#if (defined GPU_DEMO_LINEGRAD && GPU_DEMO_LINEGRAD)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_task 创建线性渐变任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /*
       设置渐变任务颜色表；
       颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义
       clut_lvl对应颜色表中颜色数量：颜色数量 = 2 ^ clut_lvl + 1
     */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /*
       设置渐变扩展模式；
       参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO
     */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于启动了仿射变换功能，这里设置仿射变换的平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /*
       设置线性渐变参数；
       这里设置为渲染区域X方向从起始到结束坐标渐变
     */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_AFFINE && GPU_DEMO_LINEGRAD_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_affine_task 线性渐变仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，仿射、透视变换均可 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    /*
       设置渐变任务颜色表；
       颜色表格式参考jlgpu_def.h的jlgpu_clut_format_t定义
       clut_lvl对应颜色表中颜色数量：颜色数量 = 2 ^ clut_lvl + 1
     */
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /*
       设置渐变扩展模式；
       参考jlgpu_def.h的jlgpu_spread_mode_t定义，但不允许JLGPU_COLOR_RAMP_SPREAD_FILL_ZREO
     */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /*
       设置仿射变换缩放系数；
       注意：渐变缩放时会把颜色间距离进行压缩/拉伸，与纯色填充不同，无法直接调整渲染区域进行缩放
     */
    jlgpu_task_gradient_set_affine_scale(GPU_LINEARGRAD_AFF_RATIO, GPU_LINEARGRAD_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /* 计算缩放后的宽、高大小 */
    int scale_w = w * GPU_LINEARGRAD_AFF_RATIO;
    int scale_h = h * GPU_LINEARGRAD_AFF_RATIO;

    /*
       设置仿射变换旋转参数；
       注意：如果有缩放功能，旋转中心需要在缩放后的区域内选取；
       这里将旋转中心设置为正中心
     */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_LINEARGRAD_AFF_ANGLE);

    /* 渐变区域起始坐标 */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /*
       计算缩放后的偏移坐标；
       用于把缩放后的渲染区域偏移到原来中心，GPU默认向左上角缩放
     */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 平移坐标，设置到变换后的位置 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_PERSPECTIVE && GPU_DEMO_LINEGRAD_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_perspective_task 线性渐变透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

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
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_LINEARGRAD_PER_DISTANCE, GPU_LINEARGRAD_PER_FOVX, GPU_LINEARGRAD_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_MASK && GPU_DEMO_LINEGRAD_MASK)
#include "img_data/img_data.h"

#define TEST_MASK_FMT	JLGPU_MASK_FORMAT_A4 /*! 定义要测试的MASK格式                           */
#define IMG_SOURCE_FMT	IMG_PIXEL_A4         /*! 定义作为MASK的纹理格式，注意，要与MASK_FMT一致 */

/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_mask_task 线性渐变+MASK任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_mask_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

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
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

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

    /* 设置MASK纹理大小端 */
    jlgpu_task_mask_set_endian(img_data_get_endian(img_format));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_PATH && GPU_DEMO_LINEGRAD_PATH)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_path_task 线性渐变+PATH任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_path_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定使能缩放、旋转等变换效果 */
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
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0 + x, 0 + y, w, h, GPU_LINEARGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_PATH_AFFINE && GPU_DEMO_LINEGRAD_PATH_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_path_affine_task 线性渐变+PATH仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

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
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置仿射变换缩放系数 */
    jlgpu_task_gradient_set_affine_scale(GPU_LINEARGRAD_AFF_RATIO, GPU_LINEARGRAD_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 计算缩放后的宽、高大小 */
    int scale_w = w * GPU_LINEARGRAD_AFF_RATIO;
    int scale_h = h * GPU_LINEARGRAD_AFF_RATIO;

    /* 设置仿射变换旋转参数，旋转中心为正中心 */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_LINEARGRAD_AFF_ANGLE);

    /* 计算缩放后的偏移坐标 */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 平移坐标，设置到变换后的位置 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_LINEARGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量仿射变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 设置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(GPU_LINEARGRAD_AFF_RATIO, GPU_LINEARGRAD_AFF_RATIO);
    /* 设置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_LINEARGRAD_AFF_ANGLE);
    /* 设置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x + dx, y + dy);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_LINEGRAD_PATH_PERSPECTIVE && GPU_DEMO_LINEGRAD_PATH_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_linegrad_path_perspective_task 线性渐变+PATH透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_linegrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化渐变任务参数结构，并设置任务ID、控件ID */
    jlgpu_task_gradient_param_init(GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT);

    /* 设置渐变任务变换模式，渐变必须使能变换，但不一定配置缩放、旋转等变换效果 */
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
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_LINEARGRAD_PER_DISTANCE, GPU_LINEARGRAD_PER_FOVX, GPU_LINEARGRAD_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_LINEARGRAD_DRAW_W);
    int h = GET_H(GPU_LINEARGRAD_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置渐变位置区域 */
    jlgpu_task_gradient_set_draw(x, y, w, h);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_gradient_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_LINEARGRAD_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量透视变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);
    /* 配置透视变换缩放 */
    jlgpu_task_path_set_perspective_scale(GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO, GPU_LINEARGRAD_PER_RATIO);
    /* 配置透视变换旋转 */
    jlgpu_task_path_set_perspective_rotate(GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE, GPU_LINEARGRAD_PER_ANGLE);
    /* 配置透视变换视角 */
    jlgpu_task_path_set_perspective_camera(GPU_LINEARGRAD_PER_DISTANCE, GPU_LINEARGRAD_PER_FOVX, GPU_LINEARGRAD_PER_FOVY);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_LINEARGRAD_TASK_ID, GPU_LINEARGRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



