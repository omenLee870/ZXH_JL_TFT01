/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_img_grad.c
 *
 * @brief 纹理+渐变相关示例
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-03-16
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"


#include "img_data/img_data.h"
#define TEXTURE_FORMAT			IMG_PIXEL_A8

#define GPU_IMG_GRAD_MASK_FMT	JLGPU_MASK_FORMAT_A1
#define GPU_IMG_GRAD_MASK_IMG	IMG_PIXEL_A1

#define GPU_IMG_GRAD_TASK_ID	1
#define GPU_IMG_GRAD_ELEMENT	1

#define GPU_IMG_GRAD_ALPHA		50
#define GPU_IMG_GRAD_COLOR		RGB565(0xff, 0x00, 0x00)

#define GPU_IMG_GRAD_DRAW_W		128
#define GPU_IMG_GRAD_DRAW_H		128

#define GPU_IMG_GRAD_RATIO		0.5f
#define GPU_IMG_GRAD_ANGLE		45.0f
#define GPU_IMG_GRAD_DISTANCE	1.0f
#define GPU_IMG_GRAD_FOVX		90.0f
#define GPU_IMG_GRAD_FOVY		90.0f

#define GPU_PATH_RADIUS			20



#if (defined GPU_DEMO_IMG_GRAD && GPU_DEMO_IMG_GRAD)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_task 纹理+渐变任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_AFF && GPU_DEMO_IMG_GRAD_AFF)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_aff_task 纹理+渐变仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_aff_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置变换模式 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 设置仿射缩放倍数 */
    jlgpu_task_img_set_affine_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 计算缩放后的宽高 */
    int scale_w = w * GPU_IMG_GRAD_RATIO;
    int scale_h = h * GPU_IMG_GRAD_RATIO;

    /* 设置仿射旋转倍数 */
    jlgpu_task_img_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_IMG_GRAD_ANGLE);

    /* 计算仿射缩放后平移调整参数 */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 设置平移变量 */
    jlgpu_task_img_set_affine_translate(x + dx, y + dy);

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置仿射变换缩放系数 */
    jlgpu_task_gradient_set_affine_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置仿射变换旋转参数，旋转中心为正中心 */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_IMG_GRAD_ANGLE);

    /* 平移坐标，设置到变换后的位置 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_PER && GPU_DEMO_IMG_GRAD_PER)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_per_task 纹理+渐变透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_per_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* 设置纹理变换为透视变换模式 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视缩放 */
    jlgpu_task_img_set_perspective_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置透视旋转 */
    jlgpu_task_img_set_perspective_rotate(GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE);

    /* 设置摄像机距离和视场角 */
    jlgpu_task_img_set_perspective_camera(GPU_IMG_GRAD_DISTANCE, GPU_IMG_GRAD_FOVX, GPU_IMG_GRAD_FOVY);

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_IMG_GRAD_DISTANCE, GPU_IMG_GRAD_FOVX, GPU_IMG_GRAD_FOVY);

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_MASK && GPU_DEMO_IMG_GRAD_MASK)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_mask_task 纹理+渐变+MASK任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_mask_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + width, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< MASK配置 >>================================= */
    /* 获取MASK纹理格式 */
    int img_format = img_data_get_format(GPU_IMG_GRAD_MASK_IMG);

    /* 获取MASK纹理宽高 */
    int img_w = img_data_get_width(img_format);
    int img_h = img_data_get_height(img_format);

    /* 设置MASK模式，外部纹理MASK */
    jlgpu_task_mask_set_enable();

    /* 设置MASK格式，注意这里是jlgpu_mask_format_t，不是jlgpu_tex_format_t */
    jlgpu_task_mask_set_format(GPU_IMG_GRAD_MASK_FMT);

    /* 设置MASK宽高 */
    jlgpu_task_mask_set_size(img_w, img_h);

    /* 设置MASK纹理地址 */
    jlgpu_task_mask_set_data(img_data_get_addr(img_format));

    /* 设置MASK渲染区域 */
    jlgpu_task_mask_set_rect(x, y, img_w, img_h);

    /* 设置MASK纹理大小端 */
    jlgpu_task_mask_set_endian(img_data_get_endian(img_format));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_PATH && GPU_DEMO_IMG_GRAD_PATH)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_path_task 纹理+渐变+PATH任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_path_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + width, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0 + x, 0 + y, w, h, GPU_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_PATH_AFF && GPU_DEMO_IMG_GRAD_PATH_AFF)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_path_aff_task 纹理+渐变+PATH仿射变换
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_path_aff_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置变换模式 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 设置仿射缩放倍数 */
    jlgpu_task_img_set_affine_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 计算缩放后的宽高 */
    int scale_w = w * GPU_IMG_GRAD_RATIO;
    int scale_h = h * GPU_IMG_GRAD_RATIO;

    /* 设置仿射旋转倍数 */
    jlgpu_task_img_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_IMG_GRAD_ANGLE);

    /* 计算仿射缩放后平移调整参数 */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /* 设置平移变量 */
    jlgpu_task_img_set_affine_translate(x + dx, y + dy);

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 由于启动了变换功能，这里设置平移坐标 */
    jlgpu_task_gradient_set_affine_translate(x, y);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + width, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置仿射变换缩放系数 */
    jlgpu_task_gradient_set_affine_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置仿射变换旋转参数，旋转中心为正中心 */
    jlgpu_task_gradient_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_IMG_GRAD_ANGLE);

    /* 平移坐标，设置到变换后的位置 */
    jlgpu_task_gradient_set_affine_translate(x + dx, y + dy);

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量仿射变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 设置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);
    /* 设置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_IMG_GRAD_ANGLE);
    /* 设置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x + dx, y + dy);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_IMG_GRAD_PATH_PER && GPU_DEMO_IMG_GRAD_PATH_PER)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_img_grad_path_per_task 纹理+渐变+PATH透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_img_grad_path_per_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 创建task_param参数结构体，并设置任务ID，控件ID，填充颜色 */
    jlgpu_task_img_grad_param_init(GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H哟关于DEMO统一测试，无需参考
     */
    int w = GET_W(width);
    int h = GET_H(height);

    /*
       计算渲染坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = （屏幕尺寸-渲染尺寸）/ 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 如果渲染宽、高小于纹理实际宽、高，对纹理进行裁剪 */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：采取起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，可以配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(RGB565_R(GPU_IMG_GRAD_COLOR), RGB565_G(GPU_IMG_GRAD_COLOR), RGB565_B(GPU_IMG_GRAD_COLOR));
    }

    /* 如果是Lx系列格式图片，需要配置颜色表 */
    if (GPU_IMG_NEED_CLUT(TEXTURE_FORMAT)) {
        uint8_t clut_fmt = img_data_get_clut_format(TEXTURE_FORMAT);
        uint16_t clut_size = img_data_get_clut_size(TEXTURE_FORMAT);
        uint8_t *clut_addr = img_data_get_clut_addr(TEXTURE_FORMAT);

        jlgpu_task_img_set_clut_en(true);              /*! 使能颜色表           */
        jlgpu_task_img_set_clut_format(clut_fmt);      /*! 设置颜色表颜色格式   */
        jlgpu_task_img_set_clut(clut_size, clut_addr); /*! 设置颜色表大小、地址 */
    }

    /* 设置纹理变换为透视变换模式 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视缩放 */
    jlgpu_task_img_set_perspective_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置透视旋转 */
    jlgpu_task_img_set_perspective_rotate(GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE);

    /* 设置摄像机距离和视场角 */
    jlgpu_task_img_set_perspective_camera(GPU_IMG_GRAD_DISTANCE, GPU_IMG_GRAD_FOVX, GPU_IMG_GRAD_FOVY);

    /* =================================<< 渐变配置 >>================================= */
    /* 渐变任务变换模式 */
    jlgpu_task_gradient_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 渐变任务颜色表 */
    static uint32_t clut[] = {
        ARGB8888(0xff, 0xff, 0x00, 0x00),
        ARGB8888(0xff, 0xff, 0xff, 0x00),
        ARGB8888(0xff, 0x00, 0xff, 0x00)
    };
    jlgpu_task_gradient_set_clut(clut, 1, JLGPU_CLUT_FORMAT_ARGB8888);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置线性渐变参数 */
    jlgpu_task_gradient_set_linear(x, y, x + w, y);

    /* 设置渐变模式为线性渐变 */
    jlgpu_task_gradient_set_grad_mode(JLGPU_GRAD_MODE_LINEAR_GRADIENT);

    /* 设置渐变扩展模式为纯色填充 */
    jlgpu_task_gradient_set_spread_mode(JLGPU_COLOR_RAMP_SPREAD_PAD);

    /* 设置透视变换缩放系数 */
    jlgpu_task_gradient_set_perspective_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);

    /* 设置透视变换旋转角度 */
    jlgpu_task_gradient_set_perspective_rotate(GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE);

    /* 设置透视变换视距和视场角 */
    jlgpu_task_gradient_set_perspective_camera(GPU_IMG_GRAD_DISTANCE, GPU_IMG_GRAD_FOVX, GPU_IMG_GRAD_FOVY);

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量透视变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);
    /* 配置透视变换缩放 */
    jlgpu_task_path_set_perspective_scale(GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO, GPU_IMG_GRAD_RATIO);
    /* 配置透视变换旋转 */
    jlgpu_task_path_set_perspective_rotate(GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE, GPU_IMG_GRAD_ANGLE);
    /* 配置透视变换视角 */
    jlgpu_task_path_set_perspective_camera(GPU_IMG_GRAD_DISTANCE, GPU_IMG_GRAD_FOVX, GPU_IMG_GRAD_FOVY);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理渲染区域 */
    jlgpu_task_img_grad_set_draw(x, y, w, h);

    /* 配置渐变的父控件区域为整个屏幕 */
    jlgpu_task_img_grad_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_IMG_GRAD_TASK_ID, GPU_IMG_GRAD_ELEMENT, &task_param);

    return NULL;
}
#endif



