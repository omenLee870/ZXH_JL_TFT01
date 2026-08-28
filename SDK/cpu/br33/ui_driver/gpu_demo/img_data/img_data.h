#ifndef __GPU_DEMO_IMG_DATA_H__
#define __GPU_DEMO_IMG_DATA_H__


#include "typedef.h"
#include "app_config.h"

#include "gpu_demo.h"

#if (defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE)


/*! 指定图片存储位置，只能选一个 */
#define IMG_SRC_RAM				1 /*! 图片放在RAM  中                  */
#define IMG_SRC_PSRAM_CACHE		0 /*! 待完善 图片放在PSRAM中，通过cache访问   */
#define IMG_SRC_PSRAM_NOCACHE	0 /*! 待完善 图片放在PSRAM中，通过nocache访问 */
#define IMG_SRC_FLASH_CACHE		0 /*! 待完善 图片放在FLASH中，通过cache访问   */
#define IMG_SRC_FLASH_NOCACHE	0 /*! 待完善 图片放在FLASH中，通过nocache访问 */


#define IMG_COMPRESS_MODE		IMG_DATA_RAW /*! 参考下方的“数据格式定义” */


/*! 数据格式定义 */
#define IMG_DATA_RAW			1 /*! 原始数据，无压缩 */
#define IMG_DATA_RAW_LINE		2 /*! ARGB8888/ARGB8565/ARGB1555/ARGB4444/RGB888/RGB565/ALxx/Ax/Lx */
#define IMG_DATA_RAW_TILE		3 /*! ARGB8888/ARGB8565/ARGB1555/ARGB4444/RGB888/RGB565/ALxx/Ax/Lx */
#define IMG_DATA_RLE_LINE		4 /*! ARGB8888/ARGB8565/ARGB1555/ARGB4444/RGB888/RGB565/AL88 */
#define IMG_DATA_RLE_TILE		5 /*! ARGB8888/ARGB8565/ARGB1555/ARGB4444/RGB888/RGB565/AL88/AL44/L8/A8 */
#define IMG_DATA_COMPRESS		6 /*! ARGB8888/RGB888/A8 */
#define IMG_DATA_ALPHA_END		7 /*! alpha 结尾       */
#define IMG_DATA_BIG_ENDIAN		8 /*! 大端格式         */
#define IMG_DATA_RB_SWAP		9 /*! R, B 位置交换    */


/* 纹理格式使能，如果压缩方式不支持的颜色，使能会忽略  */
#define IMG_FORMAT_A1			1 /*! 包含纹理格式：A1 */
#define IMG_FORMAT_A2			1 /*! 包含纹理格式：A2 */
#define IMG_FORMAT_A4			1 /*! 包含纹理格式：A4 */
#define IMG_FORMAT_A8			1 /*! 包含纹理格式：A8 */
#define IMG_FORMAT_L1			1 /*! 包含纹理格式：L1 */
#define IMG_FORMAT_L2			1 /*! 包含纹理格式：L2 */
#define IMG_FORMAT_L4			1 /*! 包含纹理格式：L4 */
#define IMG_FORMAT_L8			1 /*! 包含纹理格式：L8 */
#define IMG_FORMAT_AL22			1 /*! 包含纹理格式：AL22 */
#define IMG_FORMAT_AL44			1 /*! 包含纹理格式：AL44 */
#define IMG_FORMAT_AL88			1 /*! 包含纹理格式：AL88 */
#define IMG_FORMAT_RGB565		1 /*! 包含纹理格式：RGB565 */
#define IMG_FORMAT_RGB888		1 /*! 包含纹理格式：RGB888 */
#define IMG_FORMAT_ARGB4444		1 /*! 包含纹理格式：ARGB4444 */
#define IMG_FORMAT_ARGB1555		1 /*! 包含纹理格式：ARGB1555 */
#define IMG_FORMAT_ARGB8565		1 /*! 包含纹理格式：ARGB8565 */
#define IMG_FORMAT_ARGB8888		1 /*! 包含纹理格式：ARGB8888 */




struct img_clut {
    uint8_t format;
    uint16_t size;
    uint8_t *addr;
};

struct img_data {
    uint32_t format: 8;
    uint32_t size: 24;

    uint16_t width;
    uint16_t height;

    /* 联合体，用于不同格式的不同数据，如颜色表、jpeg等独有的信息 */
    union {
        struct img_clut clut;
    };

    uint8_t *ram_addr;
    uint8_t *psram_addr;
    uint8_t *flash_addr;
};
extern struct img_data img_list[];


void img_data_src_init(void);

uint16_t img_data_get_number(void);
uint8_t  img_data_has_clut(int fmt);

uint16_t img_data_get_width(int fmt);
uint16_t img_data_get_height(int fmt);
uint8_t  img_data_get_format(int fmt);
uint32_t img_data_get_size(int fmt);

uint8_t *img_data_get_ram_addr(int fmt);
uint8_t *img_data_get_psram_addr(int fmt);
uint8_t *img_data_get_flash_addr(int fmt);

uint8_t  img_data_get_clut_format(int fmt);
uint16_t img_data_get_clut_size(int fmt);
uint8_t *img_data_get_clut_addr(int fmt);

static inline uint8_t *img_data_get_addr(int fmt)
{
#if (IMG_SRC_RAM)
    return img_data_get_ram_addr(fmt);
#elif (IMG_SRC_PSRAM_CACHE || IMG_SRC_PSRAM_NOCACHE)
    return img_data_get_psram_addr(fmt);
#elif (IMG_SRC_FLASH_CACHE || IMG_SRC_FLASH_NOCACHE)
    return img_data_get_flash_addr(fmt);
#else
    return NULL;
#endif
}

static inline uint8_t img_data_get_compress_mode(int fmt)
{
#if ((IMG_COMPRESS_MODE == IMG_DATA_RLE_LINE) || \
	(IMG_COMPRESS_MODE == IMG_DATA_RLE_TILE))
    return 1;
#elif (IMG_COMPRESS_MODE == IMG_DATA_COMPRESS)
    return 2;
#else
    return 0;
#endif
}

static inline uint8_t img_data_get_endian(int fmt)
{
#if (IMG_COMPRESS_MODE == IMG_DATA_BIG_ENDIAN)
    return 1;
#else
    return 0;
#endif
}

static inline uint8_t img_data_get_adr_mode(int fmt)
{
#if ((IMG_COMPRESS_MODE == IMG_DATA_RAW_TILE) || \
	 (IMG_COMPRESS_MODE == IMG_DATA_RLE_TILE) || \
	 (IMG_COMPRESS_MODE == IMG_DATA_COMPRESS))
    return 0;
#else
    return 1;
#endif
}

static inline uint8_t img_data_get_alpha_end(int fmt)
{
#if (IMG_COMPRESS_MODE == IMG_DATA_ALPHA_END)
    return 1;
#else
    return 0;
#endif
}

static inline void img_data_dump_info(int fmt)
{
    printf("@@@ ===>> Image Info Dump: %d\n", fmt);
    printf("Image Data format: %d\n",			img_data_get_format(fmt));
    printf("Image Data size: %d x %d\n",		img_data_get_width(fmt), img_data_get_height(fmt));
    printf("Image Data addr: 0x%x\n", (u32)img_data_get_addr(fmt));
    printf("Image Data endian: %d\n",			img_data_get_endian(fmt));
    printf("Image Data compress: %d\n",			img_data_get_compress_mode(fmt));
    printf("Image Data adr mode: %d\n",			img_data_get_adr_mode(fmt));
    printf("Image Data alpha end: %d\n",		img_data_get_alpha_end(fmt));
    printf("Image Data clut format: %d\n",		img_data_get_clut_format(fmt));
    printf("Image Data clut size: %d\n", 		img_data_get_clut_size(fmt));
    printf("Image Data clut addr: 0x%x\n", (u32)img_data_get_clut_addr(fmt));
    printf("\n");
}


/*! addr cache to nocache */
#define PSRAM_CACHE_TO_NOCACHE(addr)	(addr)
#define FLASH_CACHE_TO_NOCACHE(addr)	(addr)


/*! image format map */
#include "gpu.h"

#define IMG_PIXEL_A1			JLGPU_FORMAT_A1
#define IMG_PIXEL_A2			JLGPU_FORMAT_A2
#define IMG_PIXEL_A4			JLGPU_FORMAT_A4
#define IMG_PIXEL_A8			JLGPU_FORMAT_A8
#define IMG_PIXEL_L1			JLGPU_FORMAT_L1
#define IMG_PIXEL_L2			JLGPU_FORMAT_L2
#define IMG_PIXEL_L4			JLGPU_FORMAT_L4
#define IMG_PIXEL_L8			JLGPU_FORMAT_L8
#define IMG_PIXEL_AL22			JLGPU_FORMAT_AL22
#define IMG_PIXEL_AL44			JLGPU_FORMAT_AL44
#define IMG_PIXEL_AL88			JLGPU_FORMAT_AL88
#define IMG_PIXEL_A8T8			JLGPU_FORMAT_A8_T8
#define IMG_PIXEL_RGB565		JLGPU_FORMAT_RGB565
#define IMG_PIXEL_RGB888		JLGPU_FORMAT_RGB888
#define IMG_PIXEL_ARGB4444		JLGPU_FORMAT_ARGB4444
#define IMG_PIXEL_ARGB1555		JLGPU_FORMAT_ARGB1555
#define IMG_PIXEL_ARGB8565		JLGPU_FORMAT_ARGB8565
#define IMG_PIXEL_ARGB8888		JLGPU_FORMAT_ARGB8888
#define IMG_PIXEL_YUV422_BT601	JLGPU_FORMAT_YUV422_BT601
#define IMG_PIXEL_YUV422_BT709	JLGPU_FORMAT_YUV422_BT709

/*! clut format map */
#define IMG_CLUT_FMT_ARGB8888	JLGPU_CLUT_FORMAT_ARGB8888
#define IMG_CLUT_FMT_ARGB8565	JLGPU_CLUT_FORMAT_ARGB8565
#define IMG_CLUT_FMT_RGB888		JLGPU_CLUT_FORMAT_RGB888
#define IMG_CLUT_FMT_RGB565		JLGPU_CLUT_FORMAT_RGB565
#define	IMG_CLUT_FMT_A8			JLGPU_CLUT_FORMAT_A8

/*! Does the image format have clut */
#define HAS_CLUT(fmt)			img_data_has_clut(fmt)

/*! If image format is Ax，allow custom color */
#define CONSTOM_COLOR(fmt)		GPU_IMG_NEED_COLOR(fmt)

#define IMG_NEED_COLOR(fmt)		(GPU_IMG_FMT_AX(fmt) || GPU_IMG_FMT_LX(fmt) || GPU_IMG_FMT_ALXX(fmt))


#endif /*! GPU_DEMO_ENABLE */

#endif /*! __GPU_DEMO_IMG_DATA_H__ */

