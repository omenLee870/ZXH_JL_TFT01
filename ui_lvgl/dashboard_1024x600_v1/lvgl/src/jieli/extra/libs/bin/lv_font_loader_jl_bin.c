/*********************
 *      INCLUDES
 *********************/
#define BOOL_DEFINE_CONFLICT
#include "lv_font_loader_jl_bin.h"
#include "lv_jl_bin.h"
//#include "fs/fs.h"
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
static uint8_t *lv_load_font_from_pc(const char *src)
{
    uint32_t rn;
    const char * fn = src;
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);

    if(res != LV_FS_RES_OK) {
        return NULL;
    }
    JLBinResource font_res;
    lv_fs_read(&f, &font_res, sizeof(JLBinResource), &rn);
    uint32_t font_size = font_res.jl_header.res_size;

    uint8_t * font_buffer = lv_malloc(font_size + sizeof(JLBinResource));

    if(font_buffer == NULL) {
        return NULL;
    }
    lv_fs_seek(&f, 0, LV_FS_SEEK_SET);
    lv_fs_read(&f, font_buffer, font_size + sizeof(JLBinResource), &rn);
    lv_fs_close(&f);
    return font_buffer;
}

lv_font_t *lv_font_load_bin(const char *font_name)
{
    uint8_t *ptr = (uint8_t *)lv_load_font_from_pc(font_name);
    if (ptr == NULL) {
        LV_LOG_ERROR("lv load file data from flash error");
        return NULL;
    }

    //JLBinResource (header is at the beginning of buffer)
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
    font_dsc->glyph_bitmap = (const uint8_t *)(ptr + offset->offset_glyph_bitmap);
    font_dsc->glyph_dsc = (const lv_font_fmt_txt_glyph_dsc_t *)(ptr + offset->offset_glyph_dsc);

    //cmaps
    lv_font_fmt_txt_cmap_t *cmaps = lv_malloc_zeroed(sizeof(lv_font_fmt_txt_cmap_t) * (font_dsc->cmap_num));
    if (cmaps == NULL) {
        LV_LOG_ERROR("[font_loader] user_malloc_ERROR!!!\r\n");
        return NULL;
    }
    lv_memcpy(cmaps, (ptr + offset->offset_cmaps), sizeof(lv_font_fmt_txt_cmap_t) * (font_dsc->cmap_num));
    for (int i = 0; i < font_dsc->cmap_num; i++) {
        cmaps[i].unicode_list = (const uint16_t *)(ptr + offset_cmaps_list_arr[i].offset_unicode_list_arr);
        cmaps[i].glyph_id_ofs_list = (const void *)(ptr + offset_cmaps_list_arr[i].offset_glyph_id_ofs_list_arr);
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
                lv_free((void *)dsc->kern_dsc);
            }
            if (NULL != dsc->cmaps) {
                lv_free((void *)dsc->cmaps);
            }
            lv_free(dsc);
        }
        lv_free(font);
    }
}
