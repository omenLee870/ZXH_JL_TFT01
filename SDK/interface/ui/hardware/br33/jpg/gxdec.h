/**
 * @file gxdec.h
 *
 * @brief jpeg与 gpu 联动模块
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2025-06-23
 */
/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __GXDEC_H__
#define __GXDEC_H__


typedef enum {
    GXDEC_BLOCK_32X16,
    GXDEC_BLOCK_32X32,
} gxdec_block_mode_t;


typedef enum {
    GXDEC_FMT_YUV444,
    GXDEC_FMT_YUV422,
    GXDEC_FMT_YUV420,
    GXDEC_FMT_A,
    GXDEC_FMT_AYUV444,
    GXDEC_FMT_AYUV422,
    GXDEC_FMT_AYUV420,
} gxdec_img_format_t;


typedef enum {
    GXDEC_YUV_709,
    GXDEC_YUV_601,
} gxdec_yuv_mode_t;


/* GXDEC和JPEG图像数据缓存区个数 */
typedef enum gxdec_cache_cnt {
    GXDEC_CACHE_CNT_2 = 2,
    GXDEC_CACHE_CNT_4 = 4,
    GXDEC_CACHE_CNT_8 = 8,
    GXDEC_CACHE_CNT_16 = 16,
} gxdec_cache_cnt_t;


typedef struct gxdec_config_table {
    unsigned int img_width : 12;
    unsigned int img_height : 12;
    unsigned int img_format : 4;
    unsigned int yuv_mode : 1;
    unsigned int block_mode : 1;
    unsigned int reserved : 2;
    unsigned int bs_base_addr : 32;
} gxdec_config_table_t;


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_get_file_info 从jpg资源文件中获取文件信息
 *
 * @Params jpg_addr jpg资源起始地址
 *
 * @return 文件头信息，内容为 gxdec_config_table_t 结构体
 */
/* ------------------------------------------------------------------------------------*/
gxdec_config_table_t *jpg_get_file_info(u8 *jpg_addr);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_gxdec_get_cache_size 获取gxdec 模块所需缓存大小
 *
 * @Params mode jpg 块模式，参考 gxdec_block_mode_t 定义
 * @Params cache_cnt 缓存块数量，必须为 gxdec_cache_cnt_t 定义
 * @Params yuv_size 输出：jpg 图像 yuv 数据缓存区所需大小
 * @Params a_size 输出：jpg 图像 alpha 数据缓存区所需大小
 */
/* ------------------------------------------------------------------------------------*/
void jpg_gxdec_get_cache_size(gxdec_block_mode_t mode, gxdec_cache_cnt_t cache_cnt, int *yuv_size, int *a_size);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_gxdec_init GPU与JPG联动模块初始化
 *
 * @Params yuv_addr yuv数据缓存区起始地址
 * @Params a_addr alpha数据缓存区起始地址
 * @Params cache_cnt 数据缓存区个数
 *
 * @return 0 初始化成功，-1 初始化失败
 */
/* ------------------------------------------------------------------------------------*/
int jpg_gxdec_init(u32 yuv_addr, u32 a_addr, gxdec_cache_cnt_t cache_cnt);



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpg_dump_info 打印jpeg图片信息（调试接口）
 *
 * @Params jpg_data jpeg图片数据地址
 */
/* ------------------------------------------------------------------------------------*/
void jpg_dump_info(u32 jpg_data);



#endif


