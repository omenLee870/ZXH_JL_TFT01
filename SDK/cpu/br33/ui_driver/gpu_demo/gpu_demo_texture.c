/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_texture.c
 *
 * @brief GPU纹理功能相关DEMO，作为纹理渲染功能参考；
 * 包含DEMO：
 * 1.纹理;
 * 2.纹理仿射变换;
 * 3.纹理透视变换;
 * 4.纹理+MASK(MASK不支持变换);
 * 5.纹理+PATH;
 * 6.纹理+PATH+仿射变换;
 * 7.纹理+PATH+透视变换
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-02-05
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"


#include "img_data/img_data.h"
#define TEXTURE_FORMAT	IMG_PIXEL_A8      /* demo纹理格式 */

#define GPU_TEXTURE_TASK_ID 		1     /*! 任务ID定义 */
#define GPU_TEXTURE_ELEMENT 		1     /*! 控件ID定义 */

#define GPU_TEXTURE_COLOR 			RGB888(0xff, 0x00, 0x00) /*! 颜色值定义 */

#define GPU_TEXTURE_AFF_RATIO		0.5f  /*! 仿射变换缩放倍率 */
#define GPU_TEXTURE_AFF_ANGLE		45    /*! 仿射变换旋转角度 */

#define GPU_TEXTURE_PER_RATIO		0.5f  /*! 透视变换缩放倍率 */
#define GPU_TEXTURE_PER_ANGLE		45.0f /*! 透视变换旋转角度 */
#define GPU_TEXTURE_PER_DISTANCE	1.0f  /*! 透视视角距离 */
#define GPU_TEXTURE_PER_FOVX		90.0f /*! 透视 X视场角 */
#define GPU_TEXTURE_PER_FOVY		90.0f /*! 透视 Y视场角 */

#define GPU_TEXTURE_MASK_IMG		IMG_PIXEL_A1         /*! 要测试的MASK格式，用于从纹理库获取纹理资源 */
#define GPU_TEXTURE_MASK_FMT		JLGPU_MASK_FORMAT_A1 /*! 测试的MASK格式，参考jlgpu_mask_format_t */

#define GPU_TEXTURE_PATH_RADIUS		20    /*! PATH圆角半径 */



#if (defined GPU_DEMO_TEXTURE && GPU_DEMO_TEXTURE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_task 创建GPU纹理任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* img_data_dump_info(TEXTURE_FORMAT); [>! 打印纹理信息 <] */
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
    jlgpu_task_img_set_size(width, height);

    /*
       计算渲染区域宽、高，根据实配置即可;
       这里GET_W/H用于DEMO统一测试，无需参考
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：裁剪起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理地址，根据实际纹理地址配置 */
    jlgpu_task_img_set_data(img_data_get_addr(TEXTURE_FORMAT));

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(TEXTURE_FORMAT));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(TEXTURE_FORMAT));

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(TEXTURE_FORMAT));

    /* alpha位置在末尾 */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(TEXTURE_FORMAT));

    /* 如果是Ax系列格式图片，需要配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* 设置纹理渲染区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_AFFINE && GPU_DEMO_TEXTURE_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_affine_task 创建GPU纹理仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
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
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 设置仿射变换的缩放倍率 */
    jlgpu_task_img_set_affine_scale(GPU_TEXTURE_AFF_RATIO, GPU_TEXTURE_AFF_RATIO);

    /*
       计算缩放后输出的宽、高；
       如果有旋转，旋转中心需要在缩放后的区域内选取；
       默认缩放是左上角对齐的，如果要中心对齐，需在平移中增加缩放平移；
     */
    int scale_w = w * GPU_TEXTURE_AFF_RATIO;
    int scale_h = h * GPU_TEXTURE_AFF_RATIO;

    /* 设置仿射变换的旋转中心坐标和旋转角度 */
    jlgpu_task_img_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_TEXTURE_AFF_ANGLE);

    /*
       计算仿射变换的平移偏移量；
       由于有缩放效果，因此平移坐标需加上缩放后的偏移量，这里把渲染偏移到中心缩放。
     */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /*
       设置仿射变换平移参数；
       这里决定仿射变换矩阵的平移参数。
     */
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

    /* 如果是Ax系列格式图片，需要配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* 配置填充位置区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_PERSPECTIVE && GPU_DEMO_TEXTURE_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_perspective_task 创建GPU纹理透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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
    int x = GET_X((rect_width(screen) - width) / 2)  + rect_left(screen);
    int y = GET_Y((rect_height(screen) - height) / 2) + rect_top(screen);

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
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
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视缩放 */
    jlgpu_task_img_set_perspective_scale(GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO);

    /* 设置透视旋转 */
    jlgpu_task_img_set_perspective_rotate(GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE);

    /* 设置摄像机距离和视场角 */
    jlgpu_task_img_set_perspective_camera(GPU_TEXTURE_PER_DISTANCE, GPU_TEXTURE_PER_FOVX, GPU_TEXTURE_PER_FOVY);

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

    /* 如果是Ax系列格式图片，必须配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));
    __DUMP_RECT(&task_param.draw);
    __DUMP_RECT(&task_param.area);

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_MASK && GPU_DEMO_TEXTURE_MASK)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_mask_task 创建GPU纹理+MASK示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_mask_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
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
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
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

    /* 如果是Ax系列格式图片，必须配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* =================================<< MASK配置开始 >>================================= */
    /* 获取MASK纹理格式，这里从纹理库获取 */
    int mask_img_fmt = img_data_get_format(GPU_TEXTURE_MASK_IMG);
    /* 获取MASK纹理宽高，这里从纹理库获取 */
    int mask_img_w = img_data_get_width(mask_img_fmt);
    int mask_img_h = img_data_get_height(mask_img_fmt);
    /* 设置MASK使能 */
    jlgpu_task_mask_set_enable();
    /* 设置MASK格式，注意：这里对应jlgpu_mask_format_t，而不是jlgpu_tex_format_t */
    jlgpu_task_mask_set_format(GPU_TEXTURE_MASK_FMT);
    /* 设置MASK图片大小，必须是纹理实际宽、高 */
    jlgpu_task_mask_set_size(mask_img_w, mask_img_h);
    /* 设置MASK图片地址 */
    jlgpu_task_mask_set_data(img_data_get_addr(mask_img_fmt));
    /* 设置MASK渲染区域 */
    jlgpu_task_mask_set_rect(x, y, mask_img_w, mask_img_h);
    /* 设置MASK图片大小端 */
    jlgpu_task_mask_set_endian(img_data_get_endian(mask_img_fmt));
    /* =================================<< MASK配置结束 >>================================= */

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_PATH && GPU_DEMO_TEXTURE_PATH)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_path_task 创建GPU纹理+PATH示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_path_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
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
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
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

    /* 如果是Ax系列格式图片，必须配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0 + x, 0 + y, w, w, GPU_TEXTURE_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_PATH_AFFINE && GPU_DEMO_TEXTURE_PATH_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_path_affine_task 创建GPU纹理+PATH仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_path_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
    if ((w < width) || (h < height)) {
        /*
           计算裁剪的起始坐标；
           注意：以纹理左上角为0点计算，这里进行居中裁剪
         */
        int clip_l = (width - w) / 2;
        int clip_t = (height - h) / 2;

        /* 注意：裁剪起始坐标必须大于0 */
        clip_l = (clip_l < 0) ? 0 : clip_l;
        clip_t = (clip_t < 0) ? 0 : clip_t;

        /*
           配置纹理裁剪区域；
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 设置仿射变换的缩放倍率 */
    jlgpu_task_img_set_affine_scale(GPU_TEXTURE_AFF_RATIO, GPU_TEXTURE_AFF_RATIO);

    /*
       计算缩放后输出的宽、高；
       如果有旋转，旋转中心需要在缩放后的区域内选取；
       默认缩放是左上角对齐的，如果要中心对齐，需在平移中增加缩放平移；
     */
    int scale_w = w * GPU_TEXTURE_AFF_RATIO;
    int scale_h = h * GPU_TEXTURE_AFF_RATIO;

    /* 设置仿射变换的旋转中心坐标和旋转角度 */
    jlgpu_task_img_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_TEXTURE_AFF_ANGLE);

    /*
       计算仿射变换的平移偏移量；
       由于有缩放效果，因此平移坐标需加上缩放后的偏移量，这里把渲染偏移到中心缩放。
     */
    int dx = (w - scale_w) / 2;
    int dy = (h - scale_h) / 2;

    /*
       设置仿射变换平移参数；
       这里决定仿射变换矩阵的平移参数。
     */
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

    /* 如果是Ax系列格式图片，必须配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_TEXTURE_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能PATH仿射变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 配置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(GPU_TEXTURE_AFF_RATIO, GPU_TEXTURE_AFF_RATIO);
    /* 配置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_TEXTURE_AFF_ANGLE);
    /* 配置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x + dx, y + dy);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_TEXTURE_PATH_PERSPECTIVE && GPU_DEMO_TEXTURE_PATH_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_texture_path_perspective_task 创建GPU纹理+PATH透视示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_texture_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 初始化纹理任务参数结构，并设置任务ID，控件ID */
    jlgpu_task_texture_param_init(GPU_TEXTURE_TASK_ID, GPU_TEXTURE_ELEMENT);

    /* 设置纹理颜色格式，这里纹理格式从纹理库获取，根据实际设置即可 */
    jlgpu_task_img_set_format(img_data_get_format(TEXTURE_FORMAT));

    /*
       获取纹理实际宽、高，用于设置纹理size等；
       这里通过纹理库获取纹理宽高，根据实际设置即可，无需参考
     */
    int width = img_data_get_width(TEXTURE_FORMAT);
    int height = img_data_get_height(TEXTURE_FORMAT);

    /*
       设置纹理的实际宽、高；
       注意：这里必须时实际的纹理宽高，与是否裁剪无关
     */
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

    /*
       判断是否裁剪；
       如果要渲染的宽、高小于纹理的实际宽、高，须要手动配置裁剪参数
     */
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
           注意：
           1. 裁剪起始坐标必须大于0，小于纹理实际宽、高；
           2. 裁剪的宽、高不能大于纹理实际的宽、高；
           这里裁剪宽、高采用两个尺寸的较小值，实际根据需要配置即可，无需参考
         */
        jlgpu_task_img_set_clip(clip_l, clip_t, MIN(w, width), MIN(h, height));
    }

    /* 设置纹理任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视缩放 */
    jlgpu_task_img_set_perspective_scale(GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO);

    /* 设置透视旋转 */
    jlgpu_task_img_set_perspective_rotate(GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE);

    /* 设置摄像机距离和视场角 */
    jlgpu_task_img_set_perspective_camera(GPU_TEXTURE_PER_DISTANCE, GPU_TEXTURE_PER_FOVX, GPU_TEXTURE_PER_FOVY);

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

    /* 如果是Ax系列格式图片，必须配置颜色 */
    if (GPU_IMG_NEED_COLOR(TEXTURE_FORMAT)) {
        jlgpu_task_img_set_color(\
                                 RGB888_R(GPU_TEXTURE_COLOR), \
                                 RGB888_G(GPU_TEXTURE_COLOR), \
                                 RGB888_B(GPU_TEXTURE_COLOR));
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

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_TEXTURE_PATH_RADIUS);
    /* 添加路径结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能PATH透视变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);
    /* 配置透视变换缩放 */
    jlgpu_task_path_set_perspective_scale(GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO, GPU_TEXTURE_PER_RATIO);
    /* 配置透视变换旋转 */
    jlgpu_task_path_set_perspective_rotate(GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE, GPU_TEXTURE_PER_ANGLE);
    /* 配置透视变换视角 */
    jlgpu_task_path_set_perspective_camera(GPU_TEXTURE_PER_DISTANCE, GPU_TEXTURE_PER_FOVX, GPU_TEXTURE_PER_FOVY);
    /* =================================<< PATH配置结束 >>================================= */

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, w, h);

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}
#endif




