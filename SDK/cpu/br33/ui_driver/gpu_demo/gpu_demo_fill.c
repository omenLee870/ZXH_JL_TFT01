/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_demo_fill.c
 *
 * @brief GPU填充功能相关DEMO，可作为最简GPU渲染任务试验。
 * 包含DEMO：
 * 1.填充;
 * 2.填充仿射变换;
 * 3.填充透视变换;
 * 4.填充+MASK(MASK不支持变换);
 * 5.填充+PATH;
 * 6.填充+PATH+仿射变换;
 * 7.填充+PATH+透视变换
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-02-05
 */
/* ------------------------------------------------------------------------------------*/

#include "gpu_demo.h"


#define GPU_FILL_TASK_ID    	1        /*! 任务ID定义   */
#define GPU_FILL_ELEMENT    	1        /*! 控件ID定义   */

#define GPU_FILL_COLOR			ARGB8888(0xff, 0x00, 0xff, 0x00) /*! 颜色值定义   */

#define GPU_FILL_DRAW_W         128      /*! 填充宽度     */
#define GPU_FILL_DRAW_H         128      /*! 填充高度     */

#define GPU_FILL_AFF_RATIO		0.5f	 /*! 仿射缩放倍率 */
#define GPU_FILL_AFF_ANGLE		45.0f	 /*! 仿射旋转角度 */

#define GPU_FILL_PER_RATIO		0.5f     /*! 透视缩放倍率 */
#define GPU_FILL_PER_ANGLE		45.0f    /*! 透视旋转角度 */
#define GPU_FILL_PER_DISTANCE	1.0f     /*! 透视视角距离 */
#define GPU_FILL_PER_FOVX		90.0f    /*! 透视 X视场角 */
#define GPU_FILL_PER_FOVY		90.0f    /*! 透视 Y视场角 */

#define GPU_FILL_PATH_RADIUS	20 		 /*! PATH圆角半径 */



#if (defined GPU_DEMO_FILL && GPU_DEMO_FILL)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_task 创建GPU填充任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将坐标计算到屏幕中心：坐标 = (屏幕尺寸-填充尺寸) / 2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw和jlgpu_task_fill_set_area的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /*
       创建GPU任务，并添加到head任务链中；
       任务通过TASK_ID 和 ELEMENT 作为ID标识，如果head链表中已存在相同ID任务，则会更新
       该任务的配置；如果head中不存在此ID任务，则会新创建该任务并追加到head链表中。
     */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_AFFINE && GPU_DEMO_FILL_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_affine_task 创建GPU填充仿射变换任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /* 设置填充任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /*
       设置仿射变换的缩放倍率；
       提示：填充功能直接设置填充区域为缩放后区域即可实现缩放效果，非必须通过仿射变换实现；
       这里仅用于展示填充的仿射变换使用方法。直接设置缩放后的填充区域效率更高。
     */
    jlgpu_task_fill_set_affine_scale(GPU_FILL_AFF_RATIO, GPU_FILL_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算缩放后输出的宽、高；
       如果有旋转，旋转中心需要在缩放后的区域内选取；
       默认缩放是左上角对齐的，如果要中心对齐，需在平移中增加缩放平移；
     */
    int scale_out_w = w * GPU_FILL_AFF_RATIO;
    int scale_out_h = h * GPU_FILL_AFF_RATIO;

    /*
       设置仿射变换的旋转参数；
       注意：如果有缩放功能，旋转中心需要在缩放后的区域内选取；
       这里将旋转中心设置为缩放后的正中心
     */
    jlgpu_task_fill_set_affine_rotate(scale_out_w / 2, scale_out_h / 2, GPU_FILL_AFF_ANGLE);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /*
       计算仿射变换的平移偏移量；
       由于有缩放效果，因此平移坐标需加上缩放后的偏移量，这里把渲染偏移到中心缩放。
     */
    int dx = (w - scale_out_w) / 2; /*! 缩放后的X偏移量，GPU默认往左上角缩放 */
    int dy = (h - scale_out_h) / 2; /*! 缩放后的Y偏移量，GPU默认往左上角缩放 */

    /*
       设置仿射变换平移参数；
       这里决定仿射变换矩阵的平移参数。
     */
    jlgpu_task_fill_set_affine_translate(x + dx, y + dy);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw经过矩阵变换后的区域与jlgpu_task_fill_set_area的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_PERSPECTIVE && GPU_DEMO_FILL_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_perspective_task 创建GPU填充透视变换任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /* 设置填充任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视变换缩放系数 */
    jlgpu_task_fill_set_perspective_scale(GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO);

    /* 设置透视变换旋转系数 */
    jlgpu_task_fill_set_perspective_rotate(GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE);

    /* 设置透视变换的摄像机距离和视场角 */
    jlgpu_task_fill_set_perspective_camera(GPU_FILL_PER_DISTANCE, GPU_FILL_PER_FOVX, GPU_FILL_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw经过矩阵变换后的区域与jlgpu_task_fill_set_area的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_MASK && GPU_DEMO_FILL_MASK)
#include "img_data/img_data.h"

#define TEST_MASK_FMT	JLGPU_MASK_FORMAT_A4 /*! 定义要测试的MASK格式                           */
#define IMG_SOURCE_FMT	IMG_PIXEL_A4         /*! 定义作为MASK的纹理格式，注意，要与MASK_FMT一致 */

/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_mask_task 创建GPU填充+MASK任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_mask_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /* 配置填充的父控件区域为整个屏幕 */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< MASK配置开始 >>================================= */
    /* MASK纹理格式，这里从纹理库获取 */
    int img_format = img_data_get_format(IMG_SOURCE_FMT);
    /* MASK纹理宽高，这里从纹理库获取 */
    int img_w = img_data_get_width(img_format);
    int img_h = img_data_get_height(img_format);
    /* 设置纹理MASK使能 */
    jlgpu_task_mask_set_enable();
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
    /* =================================<< MASK配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_PATH && GPU_DEMO_FILL_PATH)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_path_task 创建GPU填充+PATH任务示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_path_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - GPU_FILL_DRAW_W) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - GPU_FILL_DRAW_H) / 2) + rect_top(screen);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw、jlgpu_task_fill_set_area与path区域的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径，path不带变换时，要主动偏移到渲染位置 */

    /* 方式1：构建PATH时将坐标偏移 */
    /* jl_path_append_rect(path, 0 + x, 0 + y, w, h, GPU_FILL_PATH_RADIUS); */

    /* 方式2：先基于0点构建PATH，然后通过move_offset移动到指定位置 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_FILL_PATH_RADIUS);
    jl_path_move_offset(path, x, y);

    /* 添加路径结束符，矢量路径必须添加结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_PATH_AFFINE && GPU_DEMO_FILL_PATH_AFFINE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_path_affine_task 创建GPU填充+PATH仿射变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_path_affine_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /* 设置填充任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);

    /* 设置仿射变换的缩放倍率 */
    jlgpu_task_fill_set_affine_scale(GPU_FILL_AFF_RATIO, GPU_FILL_AFF_RATIO);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算缩放后输出的宽、高；
       如果有旋转，旋转中心需要在缩放后的区域内选取；
       默认缩放是左上角对齐的，如果要中心对齐，需在平移中增加缩放平移；
     */
    int scale_w = w * GPU_FILL_AFF_RATIO;
    int scale_h = h * GPU_FILL_AFF_RATIO;

    /*
       设置仿射变换的旋转参数；
       注意：如果有缩放功能，旋转中心需要在缩放后的区域内选取；
       这里将旋转中心设置为缩放后的正中心
     */
    jlgpu_task_fill_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_FILL_AFF_ANGLE);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - w) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - h) / 2) + rect_top(screen);

    /*
       计算仿射变换的平移偏移量；
       由于有缩放效果，因此平移坐标需加上缩放后的偏移量，这里把渲染偏移到中心缩放。
     */
    int dx = (w - scale_w) / 2; /*! 缩放后的X偏移量，GPU往左上角缩放 */
    int dy = (h - scale_h) / 2; /*! 缩放后的Y偏移量，GPU往左上角缩放 */

    /*
       设置仿射变换平移参数；
       这里决定仿射变换矩阵的平移参数。
     */
    jlgpu_task_fill_set_affine_translate(x + dx, y + dy);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw经过矩阵变换后的区域、
       path经过变换后的区域与jlgpu_task_fill_set_area的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    /* 注意：使用变换时，路径path需从0坐标开始生成，GPU会根据矩阵平移参数控制渲染位置 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_FILL_PATH_RADIUS);
    /* 添加路径结束符，矢量路径必须添加结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量仿射变换，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_AFFINE);
    /* 设置仿射变换缩放 */
    jlgpu_task_path_set_affine_scale(GPU_FILL_AFF_RATIO, GPU_FILL_AFF_RATIO);
    /* 设置仿射变换旋转 */
    jlgpu_task_path_set_affine_rotate(scale_w / 2, scale_h / 2, GPU_FILL_AFF_ANGLE);
    /* 设置仿射变换平移 */
    jlgpu_task_path_set_affine_translate(x + dx, y + dy);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



#if (defined GPU_DEMO_FILL_PATH_PERSPECTIVE && GPU_DEMO_FILL_PATH_PERSPECTIVE)
/* ------------------------------------------------------------------------------------*/
/**
 * @brief gpu_demo_create_fill_path_perspective_task 创建GPU填充+PATH透视变换示例
 */
/* ------------------------------------------------------------------------------------*/
pJLGPUChainHead_t gpu_demo_create_fill_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen)
{
    /* 填充颜色ARGB8888 */
    u8 a = ARGB8888_A(GPU_FILL_COLOR);
    u8 r = ARGB8888_R(GPU_FILL_COLOR);
    u8 g = ARGB8888_G(GPU_FILL_COLOR);
    u8 b = ARGB8888_B(GPU_FILL_COLOR);

    /* 初始化fill任务参数结构，并设置任务ID，控件ID和填充颜色 */
    jlgpu_task_fill_param_init(GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, a, r, g, b);

    /* 设置填充任务的变换模式，参考jlgpu_def.h的jlgpu_trans_mode_t定义 */
    jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);

    /* 设置透视变换缩放系数 */
    jlgpu_task_fill_set_perspective_scale(GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO);

    /* 设置透视变换旋转系数 */
    jlgpu_task_fill_set_perspective_rotate(GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE);

    /* 设置透视变换的摄像机距离和视场角 */
    jlgpu_task_fill_set_perspective_camera(GPU_FILL_PER_DISTANCE, GPU_FILL_PER_FOVX, GPU_FILL_PER_FOVY);

    /*
       计算填充宽、高，根据实际需要配置即可。
       这里GET_W/H用于DEMO统一测试，无需参考。
     */
    int w = GET_W(GPU_FILL_DRAW_W);
    int h = GET_H(GPU_FILL_DRAW_H);

    /*
       计算填充坐标，根据实际需要配置即可；
       这里GET_X/Y用于DEMO统一测试，无需参考；
       这里将填充设置到屏幕中心：坐标 = （屏幕尺寸-填充尺寸）/2
     */
    int x = GET_X((rect_width(screen) - GPU_FILL_DRAW_W) / 2) + rect_left(screen);
    int y = GET_Y((rect_height(screen) - GPU_FILL_DRAW_H) / 2) + rect_top(screen);

    /* 配置填充位置区域 */
    jlgpu_task_fill_set_draw(x, y, w, h);

    /*
       设置填充任务的限制区域，一般等于父控件区域。
       填充实际渲染区域为jlgpu_task_fill_set_draw经过矩阵变换后的区域、
       path经过变换的区域与jlgpu_task_fill_set_area的交集。
     */
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));

    /* =================================<< PATH配置开始 >>================================= */
    /* 创建一个矢量句柄 */
    jl_path_t *path = jl_path_create();
    /* 添加一个矩形路径 */
    /* 注意：这里path坐标放到0点，变换时矩阵会将它平移到draw坐标 */
    jl_path_append_rect(path, 0, 0, w, h, GPU_FILL_PATH_RADIUS);
    /* 添加路径结束符，矢量路径必须添加结束符 */
    jl_path_end(path);
    /* 使能矢量任务 */
    jlgpu_task_path_set_enable();
    /* 配置矢量路径 */
    jlgpu_task_path_set_path(path);
    /* 使能矢量透视变换 */
    jlgpu_task_path_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION);
    /* 配置透视变换缩放 */
    jlgpu_task_path_set_perspective_scale(GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO, GPU_FILL_PER_RATIO);
    /* 配置透视变换旋转 */
    jlgpu_task_path_set_perspective_rotate(GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE, GPU_FILL_PER_ANGLE);
    /* 配置透视变换视角 */
    jlgpu_task_path_set_perspective_camera(GPU_FILL_PER_DISTANCE, GPU_FILL_PER_FOVX, GPU_FILL_PER_FOVY);
    /* =================================<< PATH配置结束 >>================================= */

    /* 创建GPU任务，并添加到head任务链中 */
    jlgpu_update_task_by_id(head, GPU_FILL_TASK_ID, GPU_FILL_ELEMENT, &task_param);

    return NULL;
}
#endif



