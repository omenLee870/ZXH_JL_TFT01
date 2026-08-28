/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#include "../gpu_demo.h"


#if (defined GPU_IMAGE_AUTO_TEST && GPU_IMAGE_AUTO_TEST)

#include "img_data.h"


#define AUTO_SWITCH_COUNT	50

struct img_test_prev {
    uint16_t count;
    uint16_t index;
    uint16_t number;
};
static struct img_test_prev img_test = {0};
#define __this	(&img_test)

extern struct img_data img_list[]; /*! 待测试的图片列表 */


pJLGPUChainHead_t gpu_image_auto_test_cb(pJLGPUTaskHead_t head, struct rect *screen)
{
    /* 记录刷新次数，切换图片索引 */
    __this->count += 1;
    __this->number = img_data_get_number();

    /* 如果没有纹理数据，则返回，避免访问野指针 */
    if (__this->number == 0) {
        printf("Error, Image data number is 0! Please check IMG_COMPRESS_MODE and IMG_FORMAT_xx in img_data.h!\n");
        __this->count = 0;
        return NULL;
    }

    /* 索引偏移和回环 */
    if (__this->count >= AUTO_SWITCH_COUNT) {
        __this->count = 0;
        __this->index += 1;
        if (__this->index >= __this->number) {
            __this->index = 0;
        }
    }

    uint8_t format = img_list[__this->index].format;

    /* 打印纹理信息 */
    printf("Image number: %d, Current index: %d\n", __this->number, __this->index);
    img_data_dump_info(format);

    /* 初始化纹理任务参数 */
    jlgpu_task_texture_param_init(1, 1);

    /* 设置纹理颜色格式 */
    jlgpu_task_img_set_format(img_data_get_format(format));

    /* 获取纹理宽高，计算纹理坐标 */
    int width = img_data_get_width(format);
    int height = img_data_get_height(format);
    int x = (screen->width - width) / 2;
    int y = (screen->height - height) / 2;

    /* 设置纹理的宽、高 */
    jlgpu_task_img_set_size(width, height);

    /* 设置纹理绘制区域 */
    jlgpu_task_img_set_draw(x, y, width, height);

    /* 设置纹理地址 */
    jlgpu_task_img_set_data(img_data_get_addr(format));

    /* 设置纹理父控件区域 */
    jlgpu_task_img_set_area(screen->left, screen->top, screen->width, screen->height);

    /* 设置纹理字节序 */
    jlgpu_task_img_set_endian(img_data_get_endian(format));

    /* 设置纹理压缩格式 */
    jlgpu_task_img_set_compress(img_data_get_compress_mode(format));

    /* 设置压缩大小，如果有压缩格式才需要设置 */
    if (img_data_get_compress_mode(format)) {
        jlgpu_task_img_set_compress_size(img_data_get_size(format));
    }

    /* 设置纹理排列方式 */
    jlgpu_task_img_set_adr_mode(img_data_get_adr_mode(format));

    /* 设置纹理的alpha end */
    jlgpu_task_img_set_alpha_end(img_data_get_alpha_end(format));

    /* 如果纹理需要设置颜色 */
    if (IMG_NEED_COLOR(format)) {
        /* 如果数据中含有颜色数据 */
        if (img_data_has_clut(format)) {
            uint8_t *clut_addr = img_data_get_clut_addr(format);
            if (GPU_IMG_FMT_AX(format)) {
                /* Ax格式的纹理，设置纹理颜色即可 */
                jlgpu_task_img_set_color(clut_addr[2], clut_addr[1], clut_addr[0]);
            } else {
                /* Lx和ALxx格式的纹理，需通过颜色表设置颜色 */
                uint8_t clut_fmt = img_data_get_clut_format(format);
                uint16_t clut_size = img_data_get_clut_size(format);
                jlgpu_task_img_set_clut_en(true);
                jlgpu_task_img_set_clut_format(clut_fmt);
                jlgpu_task_img_set_clut(clut_size, clut_addr);
            }
        } else {
            if (GPU_IMG_FMT_AX(format)) {
                /* Ax格式的纹理，可以设置自定义颜色 */
                jlgpu_task_img_set_color(0xff, 0xff, 0x00);
            } else {
                /* 需要颜色表的纹理没有颜色表，这里报错 */
                printf("Error! image need clut!\n");
            }
        }
    }

    /* 创建GPU任务并添加到链表中 */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return NULL;
}

#endif

