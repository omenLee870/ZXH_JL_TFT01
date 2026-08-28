/* Copyright (C)
 * 2026 - ZhuHai JieLi Technology Co.,LTD
 * This program is free software; you can redistribute it and/|=* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/
/**
 * @file gpu_demo_jlui_res.c
 * \Brief : 杰理UI资源读取DEMO，从杰理UI资源包中读取制定图片进行渲染。注意，本DEMO未处理颜色表图片等特殊情况，完整纹理DEMO请参考gpu_demo_texture.c
 * @author ZhuHai JieLi Technology Co.,LTD
 * @version V1.0.0
 * @date 2026-07-22
 */

#include "gpu_demo.h"

#if (defined GPU_DEMO_JLUI_RES && GPU_DEMO_JLUI_RES)

#include "typedef.h"
#include "res/resfile.h"
#include "ui_resource.h"
#include "result_pic_index.h"  /* 图片ID定义 */

#define JLUI_RES_PATH   "storage/res_nor_mode/C/JL/JL.res"  /* UI资源文件路径 */
#define JLUI_RES_BASE   0xa200000  /* 资源在flash的其地址 */
#define JLUI_RES_IMG_ID PAGE66_ebb4_SLEEP  /* 参照result_pic_index.h */
#define JLUI_RES_FLIP_DEGREE 30  /* 2.5D翻转角度 */

extern int __ui_res_flash_info_get(struct flash_file_info *file_info, char *path, char *str, u8 phy_dev, u32 phy_addr, int mmu_tab);


/* ***************************************************************************/
/**
 * \Brief :       GPU任务链翻转处理
 *
 * \Param :        head 待翻转处理的GPU任务链
 *
 * \Return :        翻转后的任务链
 */
/* ***************************************************************************/
static pJLGPUChainHead_t gpu_demo_flip(pJLGPUChainHead_t head)
{
    struct rect win;
    gpu_matrix_t matrix;
    jlgpu_get_win_rect(&win);  /* 获取GPU窗口位置 */
    gpu_matrix_set_identity(&matrix);  /* 设置单位阵 */

    /* 获取flip矩阵 */
    extern void get_flip_matrix(gpu_matrix_t *matrix, int screen_x, int screen_y, int screen_w, int screen_h, float degree);
    get_flip_matrix(&matrix, rect_left(&win), rect_top(&win), rect_width(&win), rect_height(&win), JLUI_RES_FLIP_DEGREE);

    /* 创建为变换任务链 */
    pJLGPUTaskHead_t new_head = jlgpu_task_list_copy_create(NULL, head, NULL, 0);  /* 将普通任务链转为透视变换任务链 */
    jlgpu_task_list_copy_mul_matrix_by_group(new_head, &matrix, 0);  /* GPU任务链乘以翻转矩阵 */

    /* 删除原来任务链，防止内存泄露 */
    jlgpu_chain_delete_head(head);

    return new_head;
}


/* ***************************************************************************/
/**
 * \Brief :        创建JLUI资源图片DEMO
 *
 * \Param :        head GPU任务链头
 * \Param :        screen 平面窗口位置
 *
 * \Return :        包含JLUI资源图片任务的任务链
 */
/* ***************************************************************************/
pJLGPUChainHead_t gpu_demo_create_jlui_res(pJLGPUChainHead_t head, struct rect *screen)
{
    UI_RESFILE *fp = res_fopen(JLUI_RES_PATH, "r");  /* 打开JLUI资源包文件 */
    if (!fp) {
        printf("%s() open file %s failed!\n", __func__, JLUI_RES_PATH);
        return NULL;
    }

    struct flash_file_info file_info = {0};
    __ui_res_flash_info_get(&file_info, JLUI_RES_PATH, "res", PHY_JL_EXTERN_FLASH, JLUI_RES_BASE, false);  /* 获取JLUI图片资源信息 */

    struct image_file image = {0};
    struct flash_file_info info = {0};
    int err = jlui_res_get_image_info(0, JLUI_RES_IMG_ID, fp, &image, &info);  /* 获取JLUI资源指定ID的图片信息 */
    if (err) {
        printf("%s() get image info failed! err: %d\n", __func__, err);
        res_fclose(fp);
        return NULL;
    }

    int img_gpu_format = res_format_to_gpu(image.format);  /* 获取JLUI资源格式对应的GPU纹理格式 */
    /* printf("img_gpu_format: %d, image offset: 0x%x\n", img_gpu_format, (u32)image.offset); */

    /* 特殊格式纹理资源偏移处理 */
    if (GPU_IMG_FMT_AX(img_gpu_format) ||
        GPU_IMG_FMT_LX(img_gpu_format) ||
        GPU_IMG_FMT_ALXX(img_gpu_format)) {
        int tab_size = get_clut_format_tabsize(image.format, image.clut_format);
        image.offset += tab_size;
        image.len -= tab_size;
    }
    /* printf("info offset: 0x%x, tab: 0x%x\n", (u32)info.offset, (u32)info.tab); */
    ui_res_get_image_flash_info(&info, &file_info, &image);  /* 获取指定纹理的实际地址 */
    res_fclose(fp);

    /* 计算纹理渲染坐标，这里放到屏幕中间 */
    int img_w = image.width;
    int img_h = image.height;
    int img_x = (rect_width(screen) - img_w) / 2 + rect_left(screen);
    int img_y = (rect_height(screen) - img_h) / 2 + rect_top(screen);
    /* printf("image rect[%d, %d, %d, %d]\n", img_x, img_y, img_w, img_h); */

    uint8_t adr, cmp;
    int cmp_size;
    int stride = res_get_image_stride(img_gpu_format, &image, &adr, &cmp, &cmp_size);  /* 获取纹理对应的stride等信息 */
    /* printf("adr: %d, cmp: %d, cmp_size: %d, stride: %d\n", adr, cmp, cmp_size, stride); */

    /* 配置纹理GPU任务 */
    jlgpu_task_texture_param_init(1, 1);
    jlgpu_task_img_set_format(img_gpu_format);
    jlgpu_task_img_set_size(img_w, img_h);
    jlgpu_task_img_set_data(info.offset);
    jlgpu_task_img_set_endian(JLGPU_ENDIAN_LITTLE);
    jlgpu_task_img_set_alpha_end(JLGPU_ALPHA_END_ARGB);
    jlgpu_task_img_set_draw(img_x, img_y, img_w, img_h);
    jlgpu_task_img_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));
    if (cmp < 2) {
        jlgpu_task_img_set_stride(stride);
    }
    jlgpu_task_img_set_adr_mode(adr);
    jlgpu_task_img_set_compress(cmp);
    jlgpu_task_img_set_compress_size(cmp_size);
    if (info.tab) {
        jlgpu_task_img_set_mmu(JLGPU_PAGE_SIZE_4, (uint8_t *)info.tab);
    }

    if (GPU_IMG_NEED_CLUT(img_gpu_format)) {
        /* 颜色表格式Ax, Lx, ALxx的颜色表等处理 */
        // TODO
    }
    /*  */
    /* jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE); */
    /* jlgpu_task_img_set_affine_scale(1.0f, 1.0f); */
    /* jlgpu_task_img_set_affine_translate(img_x, img_y); */
    /*  */
    jlgpu_update_task_by_id(head, task_param.task_id, task_param.element_id, &task_param);

    return gpu_demo_flip(head);
}

#endif

