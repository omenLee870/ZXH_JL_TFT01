/**
 * @file lv_font_loader_jl_bin.h
 *
 */

#ifndef LV_FONT_LOADER_JL_BIN_H
#define LV_FONT_LOADER_JL_BIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../../../lvgl.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

//储存偏移信息
typedef struct {
    uint32_t offset_font;
    uint32_t offset_dsc;
    uint32_t offset_glyph_bitmap;
    uint32_t offset_glyph_dsc;
    uint32_t offset_cmaps;

    //有两种kern_dsc类型或者NULL,取决于kern_classes
    //kern_classes
    uint32_t offset_kern_dsc_class;
    uint32_t offset_class_pair_values;
    uint32_t offset_left_class_mapping;
    uint32_t offset_right_class_mapping;
    //kern_classes_end

    //kern_pairs
    uint32_t offset_kern_dsc_pair;
    uint32_t offset_glyph_ids;
    uint32_t offset_values;
    //kern_pairs_end

    //uint32_t offset_cache;
} lv_font_bin_offset;

//储存cmaps_list偏移信息:针对lv_font_fmt_txt_cmap_t cmaps[]结构体数组里面的两个指针,共font.dsc.cmap_num组
typedef struct {
    uint32_t offset_unicode_list_arr;
    uint32_t offset_glyph_id_ofs_list_arr;
} lv_offset_cmaps_list;
/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_font_t *lv_font_load_bin(const char *font_name);
void lv_font_free_bin(lv_font_t *font);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_FONT_LOADER_BIN_H*/