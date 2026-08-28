#ifndef __A8_COMPRESS_H__
#define __A8_COMPRESS_H__

#include "system/includes.h"

struct compress_info {
    u8 *compress_buf;
    u32 compress_size;
};

typedef int (*LineInCallback)(int width, int h, u8 **line_buf, int *line_buf_size);
typedef int (*LineOutCallback)(u8 **line_buf, int *line_buf_size);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 对a8特定格式的数据压缩
 *
 * @param width         图像宽度
 * @param height        图像高度
 * @param in_callback   行数据输入回调
 * @param out_callback  行数据处理完成回调
 * @note  1.该接口内部会申请存储压缩数据所需要的缓存，用完请使用free接口自行释放
 *        2.申请的内存可能会比实际需求的多些，建议根据compress_size进行拷贝，用完释放，避免造成内存的浪费
 */
/* ------------------------------------------------------------------------------------*/
struct compress_info *image_a8_format_compress(int width, int height, LineInCallback in_callback, LineOutCallback out_callback);

#endif
