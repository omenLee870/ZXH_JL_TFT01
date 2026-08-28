/**
 * @file lv_jl_bin_decoder.c
 *
 */
#define BOOL_DEFINE_CONFLICT
/* #include <stdio.h> */
#include <assert.h>
#include "lv_jl_bin.h"
#include "lvgl.h"
#include "fs/fs.h"
#include "generic/typedef.h"

//uthash 使用lvgl自带内存堆
#define uthash_malloc(sz) lv_malloc(sz)
#define uthash_free(ptr,sz) lv_free(ptr)
#include "utils/uthash/uthash.h"

/*********************
 *      INCLUDES
 *********************/
#include "../../draw/lv_image_decoder_private.h"
#include "../../draw/lv_draw_image.h"
#include "../../draw/lv_draw_buf.h"
#include "../../stdlib/lv_string.h"
#include "../../stdlib/lv_sprintf.h"
#include "../../libs/rle/lv_rle.h"
#include "../../core/lv_global.h"

#include "lv_draw_jlgpu.h"

#include "asm/eva_rocache.h"
#include "asm/psram_api.h"
/*********************
 *      DEFINES
 *********************/
#define LV_BIN_FILE_NAME_LEN  (15)

#define DECODER_NAME    "JL_BIN"

#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

#define IS_JL_MAGIC(addr) \
    (((const uint8_t *)(addr))[0] == 0x4A && ((const uint8_t *)(addr))[1] == 0x4C)

#define JLRES_IS_PSRAM(p_res) \
    ((p_res)->jl_header.nop.image.storage_medium == LV_STORAGE_PSRAM)

#define JLRES_IS_SRAM(p_res) \
    ((p_res)->jl_header.nop.image.storage_medium == LV_STORAGE_SRAM)

//是否开启文件mmu表
#define LV_BIN_FLASH_FILE_MMU_TAB_ENABLE  (1)

//是否开启psram缓存
#define LV_BIN_FLASH_FILE_USE_PSRAM_ENABLE  (0)

//temp
#if 1

#include "app_config.h"
#if     TCFG_SDFILE_INSERT_FLASH_ENABLE
#define LVGL_MODE_PHY_BASE            TCFG_SDFILE_FLASH_DEV_PATY_BASE
#elif   TCFG_SDFILE_EXTERN_FLASH_ENABLE
#define LVGL_MODE_PHY_BASE            TCFG_SDFILE_FLASH_DEV_PATY_BASE
#else
#define LVGL_MODE_PHY_BASE  0
#endif

static const u32 lv_phy_addr = LVGL_MODE_PHY_BASE;

static u32 lv_get_phy_addr(void)
{
    return lv_phy_addr;
}
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _bin_file {
    char filename[LV_BIN_FILE_NAME_LEN];
    u32 addr;   // 文件地址
    lv_jlbin_tab_file_info_t tab_file_info; // tab文件信息
    UT_hash_handle hh;
} bin_file_t;

typedef struct _bin_file_sd {
    char filename[LV_BIN_FILE_NAME_LEN];
    JLBinResource jl_bin;
    lv_image_dsc_t img_dsc;
    jl_bin_user_data_t user_data;
    UT_hash_handle hh;
} bin_file_sd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static jl_bin_user_data_t *get_decoder_user_data(lv_image_decoder_dsc_t *dsc);
static void free_decoder_user_data(lv_image_decoder_dsc_t *dsc);

/**********************
 *  STATIC VARIABLES
 **********************/
static u8 pack_mode = LV_PACK_MODE_UNTIGHT;

static u32 g_bin_addr = 0;

static bin_file_t *__file_handles = NULL;
static bin_file_sd_t *__file_sd_handles = NULL;

//temp test
static uint8_t flash_src_use_crc = 0;

extern u16 chip_crc16(const void *ptr, u32 len);
extern u32 sdfile_flash_addr2cpu_addr(u32 offset);
extern u32 sfc1_flash_addr2cpu_addr(u32 offset);
/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
* @brief 获取flash文件的tab文件信息
*
* @param src 文件名
* @param file_base_addr 文件基地址
* @param filelen 文件长度
* @return lv_jlbin_tab_file_info_t* tab文件信息指针
*/
lv_jlbin_tab_file_info_t *lv_get_src_flash_tab_info(const char *src, u32 file_base_addr, u32 filelen)
{
    int i;
    u32 flash_map_tab_num;

    bin_file_t *p = NULL;
    JLBinResource bin_res;
    static lv_jlbin_tab_file_info_t file_info;
    uint8_t cf;
    uint8_t *data;
    uint8_t compressed;

    rle_info_t *rle_info;
    c2_info_t *c2_info;
    c3_info_t *c3_info;

    if (pack_mode == LV_PACK_MODE_UNTIGHT) {

        int name_len = strlen(src);
        //先在文件哈希表里面查找
        HASH_FIND_STR(__file_handles, &src[name_len - LV_BIN_FILE_NAME_LEN], p);
        if (p) {
            return &p->tab_file_info;
        }
        lv_memset(&file_info, 0, sizeof(lv_jlbin_tab_file_info_t));

        lv_image_dsc_t *image = (lv_image_dsc_t *)(file_base_addr + sizeof(JLBinResource));
        bin_res = *(JLBinResource *)file_base_addr;
        data = (uint8_t *)(file_base_addr + sizeof(JLBinResource) + sizeof(lv_img_dsc_t));
        cf = image->header.cf;
        compressed = bin_res.jl_header.res_compress;
        // uint8_t  in_format  = lv_jlgpu_get_color_format(cf);
        // bool has_indexed = (in_format >= JLGPU_FORMAT_L8 && in_format <= JLGPU_FORMAT_L1) ? true : false;

        if (compressed == LV_COMPRESS_RLE) {
            rle_info = (rle_info_t *)data;
            //   if (has_indexed) {
            //     src_clut = (uint8_t *)data + rle_info->lut_addr;
            //     clut_format = rle_info->lut_format;
            //   }
            data = (uint8_t *)data + rle_info->addr;
        } else if (compressed == LV_COMPRESS_ETC2) {
            c2_info = (c2_info_t *)data;
            data = (uint8_t *)data + sizeof(c2_info_t);
        } else if (compressed == LV_COMPRESS_C3) {
            c3_info = (c3_info_t *)data;
            data = (uint8_t *)data + sizeof(c3_head_t);
        } else {
        }

        flash_map_tab_num = ((uint32_t)data % 4096 + filelen + 4096 - 1) / 4096;

        file_info.tab_size = flash_map_tab_num * sizeof(u32);
        file_info.tab = (u32 *)malloc(file_info.tab_size);
        file_info.offset = (uint32_t)data % 4096;
        file_info.last_tab_data_len = flash_map_tab_num * 4096 - filelen - file_info.offset;
        // printf("flash adddr= 0x%x offset = 0x%x, map_tab_num : %d\n", (uint32_t)data, file_info.offset, flash_map_tab_num);
        for (i = 0; i < flash_map_tab_num; i++) {
            file_info.tab[i] = (uint32_t)data / 4096 * 4096 + 4096 * i;
            // printf("info->tab[%d] : 0x%x\n", i, file_info.tab[i]);
            if (psram_cache_check((void *)file_info.tab[i])) {
                file_info.tab[i] = PSRAM_EVA_CACHE_ADDR(file_info.tab[i]);
            } else {
                file_info.tab[i] = FLASH_EVA_CACHE_ADDR(file_info.tab[i]);
            }
        }

        return &file_info;
    }

    return NULL;
}

int lv_get_flash_src_addr(const char *src)
{
    bin_file_t *p = NULL;

    if (pack_mode == LV_PACK_MODE_UNTIGHT) {

        int name_len = strlen(src);
        //先在文件哈希表里面查找
        HASH_FIND_STR(__file_handles, &src[name_len - LV_BIN_FILE_NAME_LEN], p);
        if (p) {
            return p->addr;
        }

        FILE *profile_fp = fopen(src, "r");
        if (profile_fp == NULL) {
            LV_LOG_ERROR("user_get_flash_addr ERROR!!!\r\n");
            return LV_RES_INV;
        }

        uint32_t data_len = flen(profile_fp);
        struct vfs_attr file_attr;
        fget_attrs(profile_fp, &file_attr);
        fclose(profile_fp);

        uint8_t *addr = NULL;
#if TCFG_SDFILE_EXTERN_FLASH_ENABLE
        addr = (uint8_t *)sfc1_flash_addr2cpu_addr(file_attr.sclust + lv_get_phy_addr());
#elif TCFG_SDFILE_INSERT_FLASH_ENABLE
        addr = (uint8_t *)sdfile_flash_addr2cpu_addr(file_attr.sclust + lv_get_phy_addr());
#endif
        p = lv_malloc_zeroed(sizeof(bin_file_t));
        JLBinResource jlres = {0};
        if (IS_JL_MAGIC(addr) && file_attr.fsize) {
            memcpy(&jlres, addr, sizeof(JLBinResource));
        }
#if LV_BIN_FLASH_FILE_USE_PSRAM_ENABLE
        //将flash文件内容复制到psram缓存中,指定资源
#if 1
        if (file_attr.fsize && JLRES_IS_PSRAM(&jlres)) {
            uint8_t *paddr = malloc_psram(file_attr.fsize);
            if (paddr == NULL) {
                LV_LOG_WARN("malloc_psram faild no enough memory!!!\r\n");
            } else {
                memcpy(paddr, addr, file_attr.fsize);
                DcuFlushRegion((uint32_t *)paddr, file_attr.fsize);
                addr = paddr;
            }
        }

        //将flash文件内容复制到psram缓存中,所有资源
#else
        if (file_attr.fsize) {
            uint8_t *paddr = malloc_psram(file_attr.fsize);
            if (paddr == NULL) {
                LV_LOG_WARN("malloc_psram faild no enough memory!!!\r\n");
            } else {
                memcpy(paddr, addr, file_attr.fsize);
                DcuFlushRegion((uint32_t *)paddr, file_attr.fsize);
                addr = paddr;
            }
        }
#endif

#endif
        if (IS_JL_MAGIC(addr) && flash_src_use_crc) {
            uint8_t *addr_data = addr + sizeof(JLBinResource);
            uint16_t crc_hw = chip_crc16(addr_data, data_len - sizeof(JLBinResource));
            if (crc_hw != jlres.jl_header.res_crc) {
                LV_LOG_ERROR("CRC verification error !!! src:%s", src);
                p->addr = 0;
#if LV_BIN_FLASH_FILE_USE_PSRAM_ENABLE
                //检查是否是psram内存，释放psram
                if (psram_cache_check((void *)(addr))) {
                    free_psram((void *)(addr));
                }
#endif
            } else {
                p->addr = (uint32_t)addr;
            }
        } else {
            p->addr = (uint32_t)addr;
        }
#if  LV_BIN_FLASH_FILE_MMU_TAB_ENABLE
        lv_jlbin_tab_file_info_t *tab_file_info_ptr = lv_get_src_flash_tab_info(src, (uint32_t)addr, data_len);
        if (tab_file_info_ptr) {
            memcpy(&p->tab_file_info, tab_file_info_ptr, sizeof(lv_jlbin_tab_file_info_t));
        }
#endif
        //加入文件哈希表
        strcpy(p->filename, &src[name_len - LV_BIN_FILE_NAME_LEN]);
        HASH_ADD_KEYPTR(hh, __file_handles, p->filename, strlen(p->filename), p);
        return p->addr;
    } else if (pack_mode == LV_PACK_MODE_TIGHT) {
        uint32_t src_id = (*((uint32_t *)src) >> 8) & 0x00FFFFFF;
        uint32_t id = *(uint32_t *)(g_bin_addr + src_id * 4);
        return g_bin_addr + (id & 0xffffff);
    } else {
        LV_LOG_ERROR("please try use lv_ui_src_pack_mode_init\r\n");
        return 0;
    }

}

lv_res_t lv_release_flash_src_addr(const char *src)
{
    bin_file_t *p = NULL;

    if (pack_mode == LV_PACK_MODE_UNTIGHT) {

        int name_len = strlen(src);
        HASH_FIND_STR(__file_handles, &src[name_len - LV_BIN_FILE_NAME_LEN], p);
        if (p) {
#if LV_BIN_FLASH_FILE_USE_PSRAM_ENABLE
            //检查是否是psram内存，释放psram
            if (psram_cache_check((void *)(p->addr))) {
                free_psram((void *)(p->addr));
            }
#endif
#if  LV_BIN_FLASH_FILE_MMU_TAB_ENABLE
            //清空tab表
            if (p->tab_file_info.tab) {
                lv_free(p->tab_file_info.tab);
                p->tab_file_info.tab = NULL;
            }
#endif
            //从HASH表删除
            HASH_DEL(__file_handles, p);
            lv_free(p);
            return LV_RESULT_OK;
        }
    }

    return LV_RESULT_INVALID;
}

lv_res_t lv_release_all_flash_src_addr(void)
{
    bin_file_t *p, *tmp;

    if (pack_mode != LV_PACK_MODE_UNTIGHT) {
        return LV_RESULT_INVALID;
    }

    HASH_ITER(hh, __file_handles, p, tmp) {
#if LV_BIN_FLASH_FILE_USE_PSRAM_ENABLE
        //检查是否是psram内存，释放psram
        if (psram_cache_check((void *)(p->addr))) {
            free_psram((void *)(p->addr));
        }
#endif
#if  LV_BIN_FLASH_FILE_MMU_TAB_ENABLE
        //清空tab表
        if (p->tab_file_info.tab) {
            lv_free(p->tab_file_info.tab);
            p->tab_file_info.tab = NULL;
        }
#endif
        HASH_DEL(__file_handles, p);
        lv_free(p);
    }

    return LV_RESULT_OK;
}

lv_res_t lv_get_sd_src_header(const char *src, lv_image_header_t *header, JLBinResource *jl_resource)
{
    bin_file_sd_t *p = NULL;
    lv_fs_file_t file;

    int name_len = strlen(src);

    //先在文件哈希表里面查找
    HASH_FIND_STR(__file_sd_handles, &src[name_len - LV_BIN_FILE_NAME_LEN], p);
    if (p) {
        if (header) {
            memcpy(header, &p->img_dsc.header, sizeof(lv_image_header_t));
        }
        if (jl_resource) {
            memcpy(jl_resource, &p->jl_bin, sizeof(JLBinResource));
        }
        return LV_RESULT_OK;
    }

    p = lv_malloc_zeroed(sizeof(bin_file_sd_t));
    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }
    if (lv_fs_read(&file, &p->jl_bin, sizeof(JLBinResource), NULL) != LV_FS_RES_OK) {
        lv_fs_close(&file);
        lv_free(p);
        return LV_RESULT_INVALID;
    }
    if (jl_resource) {
        memcpy(jl_resource, &p->jl_bin, sizeof(JLBinResource));
    }
    p->user_data.compressed = p->jl_bin.jl_header.res_compress;
    p->user_data.blocks = (p->jl_bin.jl_header.nop.image.block == LV_RAW_BLOCK_64x1
                           || p->jl_bin.jl_header.nop.image.block == LV_RAW_BLOCK_256x1) ? LV_RAW_BLOCK_NONE : p->jl_bin.jl_header.nop.image.block;

    res = lv_fs_seek(&file, sizeof(JLBinResource), LV_FS_SEEK_SET);
    if (res != LV_FS_RES_OK || lv_fs_read(&file, &p->img_dsc, sizeof(lv_image_dsc_t), NULL) != LV_FS_RES_OK) {
        lv_fs_close(&file);
        lv_free(p);
        return LV_RESULT_INVALID;
    }

    if (p->user_data.compressed == LV_COMPRESS_C3) {
        p->user_data.c3_image_info.has_table = p->jl_bin.jl_header.nop.image.has_table;
        p->user_data.c3_image_info.data_size = p->img_dsc.data_size;
    }

    p->img_dsc.data_size = 0;
    p->img_dsc.data = NULL;
    if (header) {
        memcpy(header, &p->img_dsc.header, sizeof(lv_image_header_t));
    }

    strcpy(p->filename, &src[name_len - LV_BIN_FILE_NAME_LEN]);
    HASH_ADD_KEYPTR(hh, __file_sd_handles, p->filename, strlen(p->filename), p);

    lv_fs_close(&file);

    return LV_RESULT_OK;
}

lv_res_t lv_free_img_dsc_bin(const char *src, lv_img_dsc_t *bin_dsc)
{
    bin_file_sd_t *p = NULL;
    if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        if (bin_dsc->data && bin_dsc->data_size) {
            lv_free((void *)(bin_dsc->data));
            bin_dsc->data = NULL;
        }
        HASH_FIND_STR(__file_sd_handles, &src[strlen(src) - LV_BIN_FILE_NAME_LEN], p);
        if (p) {
            p->img_dsc.data_size = 0;
            p->img_dsc.data = NULL;
        }
    }

    return LV_RESULT_OK;
}

//get lv_img_dsc_t
lv_res_t lv_get_img_dsc_bin(const char *src, lv_img_dsc_t *bin_dsc)
{
    u32 addr = 0;
    bin_file_sd_t *p = NULL;
    if (src[PREFIX_NUM] == FLASH_PREFIX) {
        addr = lv_get_flash_src_addr(src);
        if (addr == LV_RES_INV) {
            return LV_RESULT_INVALID;
        }
        memcpy(bin_dsc, (void *)(addr + sizeof(JLBinResource)), sizeof(lv_img_dsc_t));
        bin_dsc->data = (addr + (bin_dsc->data));

        return LV_RESULT_OK;

    } else if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        lv_fs_file_t file;
        uint32_t data_size;
        uint8_t header_size = sizeof(JLBinResource) + sizeof(lv_img_dsc_t);

        if (!lv_image_cache_is_enabled()) {
            HASH_FIND_STR(__file_sd_handles, &src[strlen(src) - LV_BIN_FILE_NAME_LEN], p);
            if (p && p->img_dsc.data_size && p->img_dsc.data) {
                memcpy(bin_dsc, &p->img_dsc, sizeof(lv_image_dsc_t));
                return LV_RESULT_OK;
            }
        }

        lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
        if (res != LV_FS_RES_OK) {
            return LV_RESULT_INVALID;
        }

        res = lv_fs_seek(&file, sizeof(JLBinResource), LV_FS_SEEK_SET);
        if (res != LV_FS_RES_OK || lv_fs_read(&file, bin_dsc, sizeof(lv_img_dsc_t), NULL) != LV_FS_RES_OK) {
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        res = lv_fs_seek(&file, 0, LV_FS_SEEK_END);
        if (res != LV_FS_RES_OK) {
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        res = lv_fs_tell(&file, &data_size);
        if (res != LV_FS_RES_OK) {
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        /* malloc_stats(); */
        bin_dsc->data = lv_malloc(data_size - header_size);
        if (bin_dsc->data == NULL) {
            LV_LOG_ERROR("[lv_get_img_dsc_bin] malloc err!!!\r\n");
            return LV_RESULT_INVALID;
        }


        res = lv_fs_seek(&file, header_size, LV_FS_SEEK_SET);
        if (res != LV_FS_RES_OK || lv_fs_read(&file, (void *)(bin_dsc->data), data_size - header_size, NULL) != LV_FS_RES_OK) {
            lv_free((void *)bin_dsc->data);
            bin_dsc->data = NULL;
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        DcuFlushRegion((uint32_t *)(bin_dsc->data), data_size - header_size);

        lv_fs_close(&file);

        lv_get_sd_src_header(src, NULL, NULL);

        if (!lv_image_cache_is_enabled()) {
            HASH_FIND_STR(__file_sd_handles, &src[strlen(src) - LV_BIN_FILE_NAME_LEN], p);
            if (p) {
                p->img_dsc.data_size = data_size - header_size;
                p->img_dsc.data = bin_dsc->data;
            }
        }

        return LV_RESULT_OK;

    } else {
        return LV_RESULT_INVALID;
    }

    return LV_RES_OK;
}

lv_res_t lv_get_img_dsc_bin_header(const char *src, lv_image_header_t *header, JLBinResource *jl_resource)
{
    int  addr = 0;
    lv_img_dsc_t src_dsc;
    lv_img_dsc_t *src_dsc_p;
    if (src[PREFIX_NUM] == FLASH_PREFIX) {
        addr = lv_get_flash_src_addr(src);
        if (addr == LV_RES_INV) {
            return LV_RESULT_INVALID;
        }
        if (jl_resource != NULL) {
            *jl_resource = *(JLBinResource *)addr;
        }
        if (header != NULL) {
            src_dsc_p = (lv_img_dsc_t *)(addr + sizeof(JLBinResource));
            header->w = src_dsc_p->header.w;
            header->h = src_dsc_p->header.h;
            header->cf = src_dsc_p->header.cf;

            /*printf("-----header->w = %d\n",header->w);*/
            /*printf("-----header->h = %d\n",header->h);*/
            /*printf("-----header->cf = %d\n",header->cf);*/
        }

    } else if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        return lv_get_sd_src_header(src, header, jl_resource);
    } else {
        LV_LOG_ERROR("UNKNOWN STORE TYPE \r\n");
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

lv_res_t lv_get_JLBinResource(const char *src, JLBinResource *bin_res)
{
    if (src[PREFIX_NUM] == FLASH_PREFIX) {
        int addr = lv_get_flash_src_addr(src);
        if (addr == LV_RES_INV) {
            return LV_RESULT_INVALID;
        }
        *bin_res = *(JLBinResource *)addr;
        return LV_RESULT_OK;
    } else {
        return lv_get_sd_src_header(src, NULL, bin_res);
    }
}

lv_image_src_t lv_get_img_src_type(const char *src)
{
    if (src[PREFIX_NUM] == FLASH_PREFIX) {
        int addr = lv_get_flash_src_addr(src);
        if (addr == LV_RES_INV) {
            return LV_IMAGE_SRC_UNKNOWN;
        }
        if (IS_JL_MAGIC(addr)) {
            return LV_IMAGE_SRC_BIN;
        } else {
            return LV_IMAGE_SRC_FILE;
        }
    } else if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        JLBinResource bin_res = {0};
        lv_get_sd_src_header(src, NULL, &bin_res);
        if (bin_res.JL[0] == 0x4a && bin_res.JL[1] == 0x4c) {
            return LV_IMAGE_SRC_BIN;
        } else {
            return LV_IMAGE_SRC_FILE;
        }

        return LV_IMAGE_SRC_BIN;
    } else {
        return LV_IMAGE_SRC_FILE;
    }

}

//判断压缩类型
uint8_t lv_get_compress_type(const void *src)
{

    lv_image_src_t src_type = (lv_image_src_t)lv_get_img_src_type(src);
    if (src_type == LV_IMAGE_SRC_BIN) {
        JLBinResource bin_res;
        lv_get_JLBinResource(src, &bin_res);
        return bin_res.jl_header.res_compress;
    } else {
        return LV_COMPRESS_NONE;
    }

}
uint8_t lv_get_res_block(const void *src)
{
    lv_image_src_t src_type = (lv_image_src_t)lv_get_img_src_type(src);
    if (src_type == LV_IMAGE_SRC_BIN) {
        JLBinResource bin_res;
        lv_get_JLBinResource(src, &bin_res);
        if (bin_res.jl_header.nop.image.block == LV_RAW_BLOCK_64x1 || bin_res.jl_header.nop.image.block == LV_RAW_BLOCK_256x1) {
            return LV_RAW_BLOCK_NONE;
        }
        return bin_res.jl_header.nop.image.block;
    } else {
        return LV_RAW_BLOCK_NONE;
    }

}

lv_result_t lv_jl_bin_decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header)
{
    LV_UNUSED(decoder); /*Unused*/

    const void *src = dsc->src;
    lv_image_src_t src_type = dsc->src_type;

    if (src_type == LV_IMAGE_SRC_VARIABLE) {
        lv_image_dsc_t *image = (lv_image_dsc_t *)src;
        if (!image->header.res_compress) {
            return LV_RESULT_INVALID;
        }
        lv_memcpy(header, &image->header, sizeof(lv_image_header_t));
    } else if (src_type == LV_IMAGE_SRC_BIN) {

        lv_get_img_dsc_bin_header(src, header, NULL);

    } else if (src_type == LV_IMAGE_SRC_SYMBOL) {
        /*The size depend on the font but it is unknown here. It should be handled outside of the
         *function*/
        header->w = 1;
        header->h = 1;
        /*Symbols always have transparent parts. Important because of cover check in the draw
         *function. The actual value doesn't matter because lv_draw_label will draw it*/
        header->cf = LV_COLOR_FORMAT_A8;
    } else {
        LV_LOG_WARN("Image get info found unknown src type");
        return LV_RESULT_INVALID;
    }

    /*For backward compatibility, all images are not premultiplied for now.*/
    if (header->magic != LV_IMAGE_HEADER_MAGIC) {
        header->flags &= ~LV_IMAGE_FLAGS_PREMULTIPLIED;
    }

    return LV_RESULT_OK;
}

/**
 * Decode an image from a binary file
 * @param decoder pointer to the decoder
 * @param dsc     pointer to the decoder descriptor
 * @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
lv_result_t lv_jl_bin_decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);
    JLBinResource bin_res;
    bin_file_sd_t *p = NULL;

    lv_result_t res = LV_RESULT_INVALID;
    lv_fs_res_t fs_res = LV_FS_RES_UNKNOWN;
    lv_color_format_t cf = 0;

    if (dsc->src_type == LV_IMAGE_SRC_BIN) {
        jl_bin_user_data_t *user_data = get_decoder_user_data(dsc);
        lv_draw_buf_t *decoded = lv_malloc_zeroed(sizeof(lv_draw_buf_t));

        if (decoded == NULL) {
            LV_LOG_ERROR("Out of memory");
            return LV_RESULT_INVALID;
        }

        int addr = 0;
        lv_image_dsc_t tmp = {0};
        char *src = (char *)dsc->src;
        if (src[PREFIX_NUM] == FLASH_PREFIX) {
            addr = lv_get_flash_src_addr(src);
            if (addr == LV_RES_INV) {
                if (decoder) {
                    lv_free(decoder);
                }
                return LV_RESULT_INVALID;
            }
            lv_image_dsc_t *image = (lv_image_dsc_t *)(addr + sizeof(JLBinResource));
            bin_res = *(JLBinResource *)addr;
            tmp = *image;
            tmp.data = (uint8_t *)(addr + sizeof(JLBinResource) + sizeof(lv_img_dsc_t));
            cf = image->header.cf;
        } else if (src[PREFIX_NUM] == SDCARD_PREFIX) {
            if (!lv_image_cache_is_enabled()) {
                LV_LOG_ERROR("The resource is loaded from the SD card. (If CACHE is not enabled, please use the FLASH resource.)");
            }
            lv_get_img_dsc_bin(src, &tmp);
            lv_get_JLBinResource(src, &bin_res);
            cf = tmp.header.cf;
        }


        tmp.header.stride = (tmp.header.w * lv_color_format_get_bpp(cf) + 7) >> 3;
        //lv_draw_buf_from_image(decoded, &tmp);
        decoded->header.w = tmp.header.w;
        decoded->header.h = tmp.header.h;
        decoded->header.cf = tmp.header.cf;
        decoded->header.stride = tmp.header.stride;
        decoded->header.magic = LV_IMAGE_HEADER_MAGIC;
        decoded->header.flags = tmp.header.flags;
        decoded->data = (void *)tmp.data;
        decoded->unaligned_data = (void *)tmp.data;
        decoded->handlers = lv_draw_buf_get_handlers();
        decoded->data_size = tmp.data_size;


        if (user_data) {
            user_data->compressed = bin_res.jl_header.res_compress;
            user_data->blocks = (bin_res.jl_header.nop.image.block == LV_RAW_BLOCK_64x1
                                 || bin_res.jl_header.nop.image.block == LV_RAW_BLOCK_256x1) ? LV_RAW_BLOCK_NONE : bin_res.jl_header.nop.image.block;

            if (user_data->compressed == LV_COMPRESS_C3) {
                user_data->c3_image_info.has_table = bin_res.jl_header.nop.image.has_table;
                user_data->c3_image_info.data_size = tmp.data_size;
            }
#if  LV_BIN_FLASH_FILE_MMU_TAB_ENABLE
            lv_jlbin_tab_file_info_t *tab_file_info_ptr = lv_get_src_flash_tab_info(src, (uint32_t)tmp.data, tmp.data_size);
            if (tab_file_info_ptr) {
                memcpy(&user_data->tab_file_info, tab_file_info_ptr, sizeof(lv_jlbin_tab_file_info_t));
            }
#endif

        } else {
            HASH_FIND_STR(__file_sd_handles, &src[strlen(src) - LV_BIN_FILE_NAME_LEN], p);
            if (p) {
                user_data = &p->user_data;
            }
            if (user_data) {
                LV_LOG_WARN("user_data null\n");
            }
        }
        dsc->decoded = decoded;
        dsc->user_data = user_data;

        if (src[PREFIX_NUM] != SDCARD_PREFIX) {
            return LV_RESULT_OK;
        }
        if (dsc->args.no_cache) {
            return LV_RESULT_OK;
        }
        /*If the image cache is disabled, just return the decoded image*/
        if (!lv_image_cache_is_enabled()) {
            return LV_RESULT_OK;
        }
        /*Add the decoded image to the cache*/
        lv_image_cache_data_t search_key;
        search_key.src_type = dsc->src_type;
        search_key.src = dsc->src;
        search_key.slot.size = decoded->data_size;
        decoded->header.flags |= LV_IMAGE_FLAGS_ALLOCATED;

        lv_cache_entry_t *entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, user_data);

        if (entry == NULL) {
            lv_free(decoded);
            return LV_RESULT_INVALID;
        }
        dsc->cache_entry = entry;
        return LV_RESULT_OK;    /*If not returned earlier then it failed*/

    } else if (dsc->src_type == LV_IMAGE_SRC_VARIABLE) {

        lv_image_dsc_t *img = (lv_image_dsc_t *)(dsc->src);
        lv_draw_buf_t *decoded = lv_malloc_zeroed(sizeof(lv_draw_buf_t));

        if (decoded == NULL) {
            LV_LOG_ERROR("Out of memory");
            return LV_RESULT_INVALID;
        }

        decoded->header.w = img->header.w;
        decoded->header.h = img->header.h;
        decoded->header.cf = img->header.cf;
        decoded->header.stride = img->header.stride;
        decoded->header.magic = LV_IMAGE_HEADER_MAGIC;
        decoded->header.flags = img->header.flags;
        decoded->data = (void *)img->data;
        decoded->unaligned_data = (void *)img->data;
        decoded->handlers = lv_draw_buf_get_handlers();
        decoded->data_size = img->data_size;

        if (img->header.res_compress) {
            jl_bin_user_data_t *jl_bin_user_data = (jl_bin_user_data_t *)(img->reserved);
            jl_bin_user_data_t *user_data = get_decoder_user_data(dsc);

            if (user_data) {

                user_data->compressed = img->header.res_compress;

                if (jl_bin_user_data) {
                    user_data->blocks = jl_bin_user_data->blocks;

                    if (user_data->compressed == LV_COMPRESS_C3) {
                        user_data->c3_image_info.has_table = jl_bin_user_data->c3_image_info.has_table;
                        user_data->c3_image_info.type = jl_bin_user_data->c3_image_info.type;
                        user_data->c3_image_info.data_size = img->data_size;
                    }

                }

                dsc->user_data = user_data;
            }
        }

        dsc->decoded = decoded;

        return LV_RESULT_OK;
    }

    if (dsc->decoded == NULL) {
        return LV_RESULT_OK;    /*Need to read via get_area_cb*/
    }

    return LV_RESULT_OK;
}

void lv_jl_bin_decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder); /*Unused*/
    char *src = (char *)dsc->src;
    if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        if (dsc->args.no_cache || !lv_image_cache_is_enabled()) {
            if (dsc->decoded) {
                lv_free((void *)(dsc->decoded));
            }
            bin_file_sd_t *p = NULL;
            HASH_FIND_STR(__file_sd_handles, &src[strlen(src) - LV_BIN_FILE_NAME_LEN], p);
            if (p) {
                if (p->img_dsc.data) {
                    lv_free((void *)p->img_dsc.data);
                    p->img_dsc.data = NULL;
                    p->img_dsc.data_size = 0;
                }
            }
            /* free_decoder_user_data(dsc); */
        }
    } else {
        if (dsc->decoded) {
            lv_free((void *)(dsc->decoded));
        }
        free_decoder_user_data(dsc);
    }

}

/**
 * Initialize the lvgl jl binary image decoder module
 */
void lv_jl_bin_decoder_init(void)
{
    lv_image_decoder_t *decoder;

    decoder = lv_image_decoder_create();
    LV_ASSERT_MALLOC(decoder);
    if (decoder == NULL) {
        LV_LOG_WARN("Out of memory");
        return;
    }
    lv_image_decoder_set_info_cb(decoder, lv_jl_bin_decoder_info);
    lv_image_decoder_set_open_cb(decoder, lv_jl_bin_decoder_open);
    //lv_image_decoder_set_get_area_cb(decoder, lv_jl_bin_decoder_get_area);
    lv_image_decoder_set_close_cb(decoder, lv_jl_bin_decoder_close);

    decoder->name = DECODER_NAME;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static jl_bin_user_data_t *get_decoder_user_data(lv_image_decoder_dsc_t *dsc)
{
    const char *src = dsc->src;
    if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        return NULL;
    }

    jl_bin_user_data_t *data = dsc->user_data;
    if (data == NULL) {
        data = lv_malloc_zeroed(sizeof(jl_bin_user_data_t));
        LV_ASSERT_MALLOC(data);
        if (data == NULL) {
            LV_LOG_ERROR("Out of memory");
            return NULL;
        }

        dsc->user_data = data;
    }

    return data;
}

static void free_decoder_user_data(lv_image_decoder_dsc_t *dsc)
{
    jl_bin_user_data_t *decoder_data = dsc->user_data;
    if (decoder_data == NULL) {
        return;
    }

    lv_free(decoder_data);
    dsc->user_data = NULL;
}

void lv_image_cache_print_usage(void)
{
    if (LV_GLOBAL_DEFAULT()->img_cache == NULL) {
        LV_LOG_ERROR("[IMAGE cache] not initialized");
        return;
    }

    if (!lv_cache_is_enabled(LV_GLOBAL_DEFAULT()->img_cache)) {
        LV_LOG_ERROR("[IMAGE cache] CACHE DISABLE");
        return;
    }

    size_t used = lv_cache_get_size(LV_GLOBAL_DEFAULT()->img_cache, NULL);
    size_t max_size = lv_cache_get_max_size(LV_GLOBAL_DEFAULT()->img_cache, NULL);
    size_t free_size = lv_cache_get_free_size(LV_GLOBAL_DEFAULT()->img_cache, NULL);
    uint32_t pct = max_size > 0 ? (uint32_t)(used * 100 / max_size) : 0;

    LV_LOG_USER("[IMAGE cache] used: %" LV_PRIu32 " / %" LV_PRIu32 " (%" LV_PRIu32 "%%) free: %" LV_PRIu32,
                (uint32_t)used, (uint32_t)max_size, pct, (uint32_t)free_size);

    if (pct >= 90) {
        LV_LOG_WARN("[IMAGE cache] CRITICAL: usage >= 90%% cache may be full!");
    } else if (pct >= 70) {
        LV_LOG_WARN("[IMAGE cache] WARNING: usage >= 70%%");
    }
}