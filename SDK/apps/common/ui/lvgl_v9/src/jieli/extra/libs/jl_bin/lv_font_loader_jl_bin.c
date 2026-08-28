
/*********************
 *      INCLUDES
 *********************/
#define BOOL_DEFINE_CONFLICT
#include "lv_font_loader_jl_bin.h"
#include "lv_jl_bin.h"
#include "fs/fs.h"
/* #include <stdio.h> */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static uint8_t *lv_load_font_from_flash(const char *src)
{

    return (uint8_t *)lv_get_flash_src_addr(src);

#if 0
    FILE *profile_fp = fopen(src, "r");
    if (profile_fp == NULL) {
        LV_LOG_ERROR("user_get_flash_addr ERROR!!!");
        return LV_RESULT_INVALID;
    }

    uint32_t data_len = flen(profile_fp);
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    fclose(profile_fp);
    uint8_t *addr = (uint8_t *)sdfile_flash_addr2cpu_addr(file_attr.sclust + lv_get_phy_addr());
    if (flash_src_use_crc) {
        JLBinResource jlres;
        memcpy(&jlres, addr, sizeof(JLBinResource));
        uint8_t *addr_data = addr + sizeof(JLBinResource);
        uint16_t crc_hw = chip_crc16(addr_data, data_len - sizeof(JLBinResource));
        if (crc_hw != jlres.jl_header.res_crc) {
            LV_LOG_ERROR("CRC verification error !!! src:%s", src);
            return NULL;
        }
    }

    return addr;
#endif

}

static uint8_t *lv_load_font_from_sd(const char *src)
{
    uint8_t *addr;
    lv_fs_file_t file;
    uint32_t data_size;

    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("[lv_get_img_dsc_bin] lv_fs_open err!!!\r\n");
        return NULL;
    }

    res = lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    if (res != LV_FS_RES_OK) {
        lv_fs_close(&file);
        LV_LOG_ERROR("[lv_get_img_dsc_bin] lv_fs_seek err!!!\r\n");
        return NULL;
    }

    res = lv_fs_tell(&file, &data_size);
    if (res != LV_FS_RES_OK) {
        lv_fs_close(&file);
        LV_LOG_ERROR("[lv_get_img_dsc_bin] lv_fs_tell err!!!\r\n");
        return NULL;
    }

    addr = lv_malloc(data_size);
    if (addr == NULL) {
        LV_LOG_ERROR("[lv_get_img_dsc_bin] malloc err!!!\r\n");
        return NULL;
    }

    res = lv_fs_seek(&file, 0, LV_FS_SEEK_SET);
    if (res != LV_FS_RES_OK || lv_fs_read(&file, addr, data_size, NULL) != LV_FS_RES_OK) {
        lv_free(addr);
        addr = NULL;
        lv_fs_close(&file);
        return NULL;
    }

    DcuFlushRegion((uint32_t *)addr, data_size);

    lv_fs_close(&file);

    return addr;
}


lv_font_t *lv_font_load_bin(const char *src)
{
    uint8_t *ptr = NULL;
    if (src[PREFIX_NUM] == FLASH_PREFIX) {
        ptr = lv_load_font_from_flash(src);
        if (ptr == NULL) {
            LV_LOG_ERROR("lv load file data from flash error");
            return NULL;
        }
    } else if (src[PREFIX_NUM] == SDCARD_PREFIX) {
        ptr = lv_load_font_from_sd(src);
        if (ptr == NULL) {
            LV_LOG_ERROR("lv load file data from sd error");
            return NULL;
        }
    }

    //JLBinResource
    JLBinResource *JL_head = (JLBinResource *)ptr;
    //lv_font_bin_offset
    lv_font_bin_offset *offset = (lv_font_bin_offset *)(ptr + sizeof(JLBinResource));
    //lv_offset_cmaps_list
    lv_offset_cmaps_list *offset_cmaps_list_arr = (lv_offset_cmaps_list *)(ptr + sizeof(JLBinResource) + sizeof(lv_font_bin_offset));
    //lv_font_t
    lv_font_t *font = lv_malloc(sizeof(lv_font_t));
    if (font == NULL) {
        LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
        return NULL;
    }
    lv_memcpy(font, (ptr + offset->offset_font), sizeof(lv_font_t));
    font->get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
    font->get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;

    //font_dsc
    lv_font_fmt_txt_dsc_t *font_dsc = lv_malloc(sizeof(lv_font_fmt_txt_dsc_t));
    if (font_dsc == NULL) {
        LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
        return NULL;
    }
    lv_memcpy(font_dsc, (ptr + offset->offset_dsc), sizeof(lv_font_fmt_txt_dsc_t));
    font->dsc = font_dsc;
    font_dsc->glyph_bitmap = (uint8_t *)(ptr + offset->offset_glyph_bitmap);
    font_dsc->glyph_dsc = (lv_font_fmt_txt_glyph_dsc_t *)(ptr + offset->offset_glyph_dsc);

    //cmaps
    lv_font_fmt_txt_cmap_t *cmaps = lv_malloc_zeroed(sizeof(lv_font_fmt_txt_cmap_t) * (font_dsc->cmap_num));
    if (cmaps == NULL) {
        LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
        return NULL;
    }
    lv_memcpy(cmaps, (ptr + offset->offset_cmaps), sizeof(lv_font_fmt_txt_cmap_t) * (font_dsc->cmap_num));
    for (int i = 0; i < font_dsc->cmap_num; i++) {
        cmaps[i].unicode_list = (uint16_t *)(ptr + offset_cmaps_list_arr[i].offset_unicode_list_arr);
        cmaps[i].glyph_id_ofs_list = (ptr + offset_cmaps_list_arr[i].offset_glyph_id_ofs_list_arr);
    }
    font_dsc->cmaps = (lv_font_fmt_txt_cmap_t *)cmaps;

    //kern_dsc
    if (offset->offset_kern_dsc_class != offset->offset_values) { //kern_dsc == NULL
        if (font_dsc->kern_classes) {
            lv_font_fmt_txt_kern_classes_t *kern_classes = lv_malloc(sizeof(lv_font_fmt_txt_kern_classes_t));
            if (kern_classes == NULL) {
                LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
                return NULL;
            }
            lv_memcpy(kern_classes, (ptr + offset->offset_kern_dsc_class), sizeof(lv_font_fmt_txt_kern_classes_t));
            kern_classes->class_pair_values = (int8_t *)(ptr + offset->offset_class_pair_values);
            kern_classes->left_class_mapping = (uint8_t *)(ptr + offset->offset_left_class_mapping);
            kern_classes->right_class_mapping = (uint8_t *)(ptr + offset->offset_right_class_mapping);
            font_dsc->kern_dsc = kern_classes;
        } else {
            lv_font_fmt_txt_kern_pair_t *kern_pairs = lv_malloc(sizeof(lv_font_fmt_txt_kern_pair_t));
            if (kern_pairs == NULL) {
                LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
                return NULL;
            }
            lv_memcpy(kern_pairs, (ptr + offset->offset_kern_dsc_pair), sizeof(lv_font_fmt_txt_kern_pair_t));
            kern_pairs->glyph_ids = (ptr + offset->offset_glyph_ids);
            kern_pairs->values = (int8_t *)(ptr + offset->offset_values);
            font_dsc->kern_dsc = kern_pairs;
        }
    } else {
        font_dsc->kern_dsc = NULL;
    }
    //cache
    //font_dsc->cache = NULL;

    return font;
}

/**
 * Frees the memory allocated by the `lv_load_font_from_flash()` function
 * @param font lv_font_t object created by the lv_font_load function
 */

void lv_font_free_bin(lv_font_t *font)
{
    if (NULL != font) {
        lv_font_fmt_txt_dsc_t *dsc = (lv_font_fmt_txt_dsc_t *)font->dsc;
        if (NULL != dsc) {
            if (NULL != dsc->kern_dsc) {
                lv_free((void *)(dsc->kern_dsc));
            }
            if (NULL != dsc->cmaps) {
                lv_free((void *)(dsc->cmaps));
            }
            lv_free((void *)dsc);
        }
        lv_free((void *)font);
    }
}

