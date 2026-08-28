/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __LIB_GPU_FORMAT_H__
#define __LIB_GPU_FORMAT_H__

/**
 * @brief 资源格式转GPU格式
 * @param res_format 资源格式
 * @return GPU格式
 */
int res_format_to_gpu(int res_format);

/**
 * @brief GPU格式转资源格式
 * @param gpu_format GPU格式
 * @return 资源格式
 */
int gpu_format_to_res(int gpu_format);

/**
 * @brief 文本格式转GPU格式
 * @param res_format 资源格式
 * @return GPU格式
 */
int text_format_to_gpu(int res_format);

/**
 * @brief GPU格式转文本格式
 * @param gpu_format GPU格式
 * @return 文本格式
 */
int gpu_format_to_text(int gpu_format);

/**
 * @brief 获取CLUT格式表大小
 * @param res_format 资源格式
 * @param clut_format CLUT格式
 * @return 表大小
 */
int get_clut_format_tabsize(int res_format, int clut_format);

/**
 * @brief GPU格式转LUT大小
 * @param gpu_format GPU格式
 * @param clut_format CLUT格式
 * @return LUT大小
 */
int gpu_format_to_lut_size(int gpu_format, int clut_format);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief texture_line_to_tile_base 纹理行排列转为块排列
 *
 * @Params dst 纹理输出(注意对齐)
 * @Params src 纹理输入
 * @Params src_w 纹理输入宽度
 * @Params src_h 纹理输入高度
 * @Params format 纹理格式，参考 GPU_format_t
 */
/* ------------------------------------------------------------------------------------*/
void texture_line_to_tile_base(void *dst, void *src, int src_w, int src_h, int format);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief image_file_get_gpu_format 获取图片文件对应的GPU格式
 *
 * @Params image 图片文件句柄
 *
 * @return GPU格式
 */
/* ------------------------------------------------------------------------------------*/
int image_file_get_gpu_format(struct image_file *image);
#endif


