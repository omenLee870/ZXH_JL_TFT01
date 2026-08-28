
/**
 * @file lv_jl_bin.h
 *
 */


#ifndef LV_JL_BIN_H
#define LV_JL_BIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define PREFIX_NUM      8
#define FLASH_PREFIX   'r'
#define SDCARD_PREFIX  's'

//fixme
/*extern const uint8_t flash_src_use_crc;*/
enum {
    LV_PACK_MODE_UNINIT = 0,
    LV_PACK_MODE_TIGHT,
    LV_PACK_MODE_UNTIGHT,
};

enum {
    LV_STORAGE_UNSPECIFIED = 0,//不指定
    LV_STORAGE_SRAM,           //SRAM
    LV_STORAGE_PSRAM,          //PSAM
};

enum {
    LV_C3_TYPE_UITOOL = 0,
    LV_C3_TYPE_APP,
};

/**********************
 *      TYPEDEFS
 **********************/
/*******************JL************************/
typedef union _JLBinResourceHeaderNop {
    struct {
        uint32_t block : 2;
        uint32_t has_table : 1;
        uint32_t storage_medium : 3;
        uint32_t nop: 26;
    } image;
    uint32_t nop;
} JLBinResourceHeaderNop;
typedef struct {
    uint8_t res_type;
    uint8_t res_compress;
    uint16_t res_crc;
    uint32_t res_size;
    //uint32_t nop;
    JLBinResourceHeaderNop nop;
} JLBinResourceHeader;

typedef struct {
    char JL[2];
    uint16_t version;
    JLBinResourceHeader jl_header;
    void *resource;
} JLBinResource;
/*******************JL************************/

typedef struct _lv_jlbin_tab_file_info {
    uint32_t *tab;                    // tab表(默认4k对齐)
    uint16_t tab_size;                // tab表字节大小(每个tab 4个字节, tab数量=tab_size/4)
    uint32_t offset;                  // 针对tab[0]的偏移
    uint32_t last_tab_data_len;       // 最后tab的长度
} lv_jlbin_tab_file_info_t;

typedef struct {
    uint8_t has_table;
    uint32_t data_size;
    uint8_t type;
} lv_c3_image_t;

typedef struct {
    uint8_t compressed; /*lv_image_decoder.h的lv_jl_compress_t*/
    uint8_t blocks;     /*lv_image_decoder.h的lv_jl_raw_block_t*/

    lv_c3_image_t c3_image_info;

    lv_jlbin_tab_file_info_t tab_file_info; // mmu tab文件信息

} jl_bin_user_data_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_res_t lv_get_img_dsc_bin(const char *src, lv_img_dsc_t *bin_dsc);
lv_res_t lv_get_JLBinResource(const char *src, JLBinResource *bin_res);
lv_image_src_t lv_get_img_src_type(const char *src);
uint8_t lv_get_compress_type(const void *src);
int lv_get_flash_src_addr(const char *src);
lv_res_t lv_release_flash_src_addr(const char *src);
lv_res_t lv_release_all_flash_src_addr(void);


/**
 * Initialize the lvgl jl binary image decoder module
 */
void lv_jl_bin_decoder_init(void);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_JL_BIN_H*/


