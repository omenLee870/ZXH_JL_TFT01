/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
#include "img_data.h"


#if (defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE)



#define img_data_search(fmt, val, convert) \
	for (int _idx = 0; _idx < img_data_get_number(); _idx++) { \
		if (img_list[_idx].format == (fmt)) { \
			return convert(img_list[_idx].val); \
		} \
	} \
	ASSERT(0, "Not find format in image list! format: %d\n", fmt);

#define DATA_CONVERT


uint16_t img_data_get_width(int fmt)
{
    img_data_search(fmt, width, DATA_CONVERT);
    return 0;
}

uint16_t img_data_get_height(int fmt)
{
    img_data_search(fmt, height, DATA_CONVERT);
    return 0;
}

uint8_t img_data_get_format(int fmt)
{
    img_data_search(fmt, format, DATA_CONVERT);
    return 0xff;
}

uint32_t img_data_get_size(int fmt)
{
    img_data_search(fmt, size, DATA_CONVERT);
    return 0;
}

#define DATA_ADDR(addr) \
	HAS_CLUT(fmt) ? (addr + img_list[_idx].clut.size) : (addr);

uint8_t *img_data_get_ram_addr(int fmt)
{
    img_data_search(fmt, ram_addr, DATA_ADDR);
    return NULL;
}

uint8_t *img_data_get_psram_addr(int fmt)
{
#if (IMG_SRC_PSRAM_CACHE)
    img_data_search(fmt, psram_addr, DATA_CONVERT);
    return NULL;
#elif (IMG_SRC_PSRAM_NOCACHE)
    img_data_search(fmt, psram_addr, PSRAM_CACHE_TO_NOCACHE);
    return NULL;
#else
    return NULL;
#endif
}

uint8_t *img_data_get_flash_addr(int fmt)
{
#if (IMG_SRC_FLASH_CACHE)
    img_data_search(fmt, flash_addr, DATA_CONVERT);
    return NULL;
#elif (IMG_SRC_FLASH_NOCACHE)
    img_data_search(fmt, flash_addr, FLASH_CACHE_TO_NOCACHE);
    return NULL;
#else
    return NULL;
#endif
}

uint8_t img_data_get_clut_format(int fmt)
{
    if (HAS_CLUT(fmt)) {
        img_data_search(fmt, clut.format, DATA_CONVERT);
        return -1;
    } else {
        return -1;
    }
}

uint16_t img_data_get_clut_size(int fmt)
{
    if (HAS_CLUT(fmt)) {
        img_data_search(fmt, clut.size, DATA_CONVERT);
        return 0;
    } else {
        return 0;
    }
}

uint8_t *img_data_get_clut_addr(int fmt)
{
    if (HAS_CLUT(fmt)) {
        img_data_search(fmt, clut.addr, DATA_CONVERT);
        return NULL;
    } else {
        return NULL;
    }
}


/* 把测试图片拷贝到flash */
static void img_data_copy_to_flash(void)
{
}

/* 把测试图片拷贝到psram */
static void img_data_copy_to_psram(void)
{
}

void img_data_src_init(void)
{
#if (IMG_SRC_PSRAM_CACHE || IMG_SRC_PSRAM_NOCACHE)
    img_data_copy_to_psram();
#elif (IMG_SRC_FLASH_CACHE || IMG_SRC_FLASH_NOCACHE)
    img_data_copy_to_flash();
#else

#endif
}


#endif

