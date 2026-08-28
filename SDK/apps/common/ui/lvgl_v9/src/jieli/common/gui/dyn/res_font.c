/**
 * @file res_font.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "res_font.h"
#include "res.h"

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    char *buf;
    uint32_t pos;
    uint32_t size;
} mem_reader_t;

typedef struct {
    mem_reader_t *reader;
    int8_t bit_pos;
    uint8_t byte_value;
} mem_bit_iterator_t;

typedef struct font_header_bin {
    uint32_t version;
    uint16_t tables_count;
    uint16_t font_size;
    uint16_t ascent;
    int16_t descent;
    uint16_t typo_ascent;
    int16_t typo_descent;
    uint16_t typo_line_gap;
    int16_t min_y;
    int16_t max_y;
    uint16_t default_advance_width;
    uint16_t kerning_scale;
    uint8_t index_to_loc_format;
    uint8_t glyph_id_format;
    uint8_t advance_width_format;
    uint8_t bits_per_pixel;
    uint8_t xy_bits;
    uint8_t wh_bits;
    uint8_t advance_width_bits;
    uint8_t compression_id;
    uint8_t subpixels_mode;
    uint8_t padding;
    int16_t underline_position;
    uint16_t underline_thickness;
} font_header_bin_t;

typedef struct cmap_table_bin {
    uint32_t data_offset;
    uint32_t range_start;
    uint16_t range_length;
    uint16_t glyph_id_start;
    uint16_t data_entries_count;
    uint8_t format_type;
    uint8_t padding;
} cmap_table_bin_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_font_t *parse_font_bin(char *buf, uint32_t len);
static lv_font_t *parse_font_fnt(char *buf, uint32_t len);
static void font_bin_reset_dsc_runtime_ptrs(lv_font_fmt_txt_dsc_t *font_dsc);
static void font_bin_reset_cmap_runtime_ptrs(lv_font_fmt_txt_cmap_t *cmaps, uint32_t cmap_num);
static void font_bin_reset_kern_class_runtime_ptrs(lv_font_fmt_txt_kern_classes_t *kern_classes);
static void font_bin_reset_kern_pair_runtime_ptrs(lv_font_fmt_txt_kern_pair_t *kern_pairs);
static mem_bit_iterator_t init_mem_bit_iterator(mem_reader_t *reader);
static unsigned int mem_read_bits(mem_bit_iterator_t *it, int n_bits, bool *success);
static int mem_read_bits_signed(mem_bit_iterator_t *it, int n_bits, bool *success);
static int32_t mem_read_label(mem_reader_t *reader, uint32_t start, const char *label);
static bool mem_read(mem_reader_t *reader, void *dst, uint32_t size);
static bool mem_seek(mem_reader_t *reader, uint32_t pos);
static uint32_t mem_read_le32(const char *data);

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void *res_font_parse(char *buf)
{
    if (buf == NULL) {
        return NULL;
    }

    if (buf[0] == 'J' && buf[1] == 'L') {
        return parse_font_bin(buf, 0xFFFFFFFFU);
    } else {
        return parse_font_fnt(buf, 0xFFFFFFFFU);
    }
}

void *res_font_parse_view(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len < 8U) {
        return NULL;
    }

    if (data[0] == 'J' && data[1] == 'L') {
        return parse_font_bin((char *)data, len);
    }
    return parse_font_fnt((char *)data, len);
}

void res_font_free(void *buf)
{
    lv_binfont_destroy(buf);
}

bool res_font_is_fnt(char *buf)
{
    if (buf == NULL) {
        return false;
    }

    //通过判断4字节后是否为"head"来判断是否为fnt字体
    if (lv_memcmp(buf + 4, "head", 4) == 0) {
        return true;
    }

    return false;
}

/**********************
 *   STATIC FUNCTIONS
 ***********************/

static void font_bin_reset_dsc_runtime_ptrs(lv_font_fmt_txt_dsc_t *font_dsc)
{
    font_dsc->glyph_bitmap = NULL;
    font_dsc->glyph_dsc = NULL;
    font_dsc->cmaps = NULL;
    font_dsc->kern_dsc = NULL;
}

static void font_bin_reset_cmap_runtime_ptrs(lv_font_fmt_txt_cmap_t *cmaps, uint32_t cmap_num)
{
    for (uint32_t i = 0; i < cmap_num; i++) {
        cmaps[i].unicode_list = NULL;
        cmaps[i].glyph_id_ofs_list = NULL;
    }
}

static void font_bin_reset_kern_class_runtime_ptrs(lv_font_fmt_txt_kern_classes_t *kern_classes)
{
    kern_classes->class_pair_values = NULL;
    kern_classes->left_class_mapping = NULL;
    kern_classes->right_class_mapping = NULL;
}

static void font_bin_reset_kern_pair_runtime_ptrs(lv_font_fmt_txt_kern_pair_t *kern_pairs)
{
    kern_pairs->glyph_ids = NULL;
    kern_pairs->values = NULL;
}

static lv_font_t *parse_font_bin(char *buf, uint32_t len)
{
    (void)len;
    Resource *resource = (Resource *)buf;

    lv_font_bin_offset *offset = (lv_font_bin_offset *)(buf + sizeof(ResourceDiskHeader));
    lv_offset_cmaps_list *offset_cmaps_list_arr = (lv_offset_cmaps_list *)(buf + sizeof(ResourceDiskHeader) + sizeof(
                lv_font_bin_offset));

    lv_font_t *font = (lv_font_t *)lv_malloc(sizeof(lv_font_t));
    if (font == NULL) {
        return NULL;
    }

    lv_memcpy(font, (buf + offset->offset_font), sizeof(lv_font_t));
    font->dsc = NULL;
    font->get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
    font->get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;

    lv_font_fmt_txt_dsc_t *font_dsc = (lv_font_fmt_txt_dsc_t *)lv_malloc(sizeof(lv_font_fmt_txt_dsc_t));
    if (font_dsc == NULL) {
        goto error_handler;
    }

    lv_memcpy(font_dsc, (buf + offset->offset_dsc), sizeof(lv_font_fmt_txt_dsc_t));
    font_bin_reset_dsc_runtime_ptrs(font_dsc);
    font->dsc = font_dsc;

    //glyph_bitmap
    uint32_t glyph_bitmap_size = offset->offset_glyph_dsc - offset->offset_glyph_bitmap;
    uint8_t *glyph_bitmap = (uint8_t *)lv_malloc(glyph_bitmap_size);
    if (glyph_bitmap == NULL) {
        goto error_handler;
    }
    lv_memcpy(glyph_bitmap, (buf + offset->offset_glyph_bitmap), glyph_bitmap_size);
    font_dsc->glyph_bitmap = glyph_bitmap;

    //glyph_dsc
    uint32_t glyph_dsc_size = offset->offset_cmaps - offset->offset_glyph_dsc;
    lv_font_fmt_txt_glyph_dsc_t *glyph_dsc = (lv_font_fmt_txt_glyph_dsc_t *)lv_malloc(glyph_dsc_size);
    if (glyph_dsc == NULL) {
        goto error_handler;
    }
    lv_memcpy(glyph_dsc, (buf + offset->offset_glyph_dsc), glyph_dsc_size);
    font_dsc->glyph_dsc = glyph_dsc;

    //cmaps
    lv_font_fmt_txt_cmap_t *cmaps = lv_malloc(sizeof(lv_font_fmt_txt_cmap_t) * font_dsc->cmap_num);
    if (cmaps == NULL) {
        goto error_handler;
    }

    lv_memcpy(cmaps, (buf + offset->offset_cmaps), sizeof(lv_font_fmt_txt_cmap_t) * font_dsc->cmap_num);
    font_bin_reset_cmap_runtime_ptrs(cmaps, font_dsc->cmap_num);
    font_dsc->cmaps = cmaps;

    for (int i = 0; i < font_dsc->cmap_num; i++) {
        //unicode_list
        if (offset_cmaps_list_arr[i].offset_unicode_list_arr != 0) {
            uint32_t unicode_list_size = cmaps[i].list_length * sizeof(uint16_t);

            uint16_t *unicode_list = (uint16_t *)lv_malloc(unicode_list_size);
            if (unicode_list == NULL) {
                goto error_handler;
            }
            lv_memcpy(unicode_list, (buf + offset_cmaps_list_arr[i].offset_unicode_list_arr), unicode_list_size);
            cmaps[i].unicode_list = unicode_list;
        }

        //glyph_id_ofs_list
        if (offset_cmaps_list_arr[i].offset_glyph_id_ofs_list_arr != 0) {
            uint32_t ofs_list_size;

            if (cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL ||
                cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY) {
                ofs_list_size = cmaps[i].list_length * sizeof(uint8_t);
            } else {
                ofs_list_size = cmaps[i].list_length * sizeof(uint16_t);
            }

            void *glyph_id_ofs_list = lv_malloc(ofs_list_size);
            if (glyph_id_ofs_list == NULL) {
                goto error_handler;
            }
            lv_memcpy(glyph_id_ofs_list, (buf + offset_cmaps_list_arr[i].offset_glyph_id_ofs_list_arr), ofs_list_size);
            cmaps[i].glyph_id_ofs_list = glyph_id_ofs_list;
        }
    }
    //kern_dsc
    if (offset->offset_kern_dsc_class != offset->offset_values) { // kern_dsc != NULL
        if (font_dsc->kern_classes) {
            //kern_classes
            lv_font_fmt_txt_kern_classes_t *kern_classes = (lv_font_fmt_txt_kern_classes_t *)lv_malloc(sizeof(
                        lv_font_fmt_txt_kern_classes_t));
            if (kern_classes == NULL) {
                goto error_handler;
            }
            lv_memcpy(kern_classes, (buf + offset->offset_kern_dsc_class), sizeof(lv_font_fmt_txt_kern_classes_t));
            font_bin_reset_kern_class_runtime_ptrs(kern_classes);
            font_dsc->kern_dsc = kern_classes;

            //class_pair_values
            if (offset->offset_class_pair_values != 0) {
                uint32_t values_size = kern_classes->left_class_cnt * kern_classes->right_class_cnt * sizeof(int8_t);
                int8_t *class_pair_values = (int8_t *)lv_malloc(values_size);
                if (class_pair_values == NULL) {
                    goto error_handler;
                }
                lv_memcpy(class_pair_values, (buf + offset->offset_class_pair_values), values_size);
                kern_classes->class_pair_values = class_pair_values;
            }

            //left_class_mapping
            if (offset->offset_left_class_mapping != 0) {
                uint32_t mapping_size = 256 * sizeof(uint8_t);  // 假设映射表大小为256
                uint8_t *left_class_mapping = (uint8_t *)lv_malloc(mapping_size);
                if (left_class_mapping == NULL) {
                    goto error_handler;
                }
                lv_memcpy(left_class_mapping, (buf + offset->offset_left_class_mapping), mapping_size);
                kern_classes->left_class_mapping = left_class_mapping;
            }

            //right_class_mapping
            if (offset->offset_right_class_mapping != 0) {
                uint32_t mapping_size = 256 * sizeof(uint8_t);  // 假设映射表大小为256
                uint8_t *right_class_mapping = (uint8_t *)lv_malloc(mapping_size);
                if (right_class_mapping == NULL) {
                    goto error_handler;
                }
                lv_memcpy(right_class_mapping, (buf + offset->offset_right_class_mapping), mapping_size);
                kern_classes->right_class_mapping = right_class_mapping;
            }
        } else {
            //kern_pairs
            lv_font_fmt_txt_kern_pair_t *kern_pairs = (lv_font_fmt_txt_kern_pair_t *)lv_malloc(sizeof(
                        lv_font_fmt_txt_kern_pair_t));
            if (kern_pairs == NULL) {
                goto error_handler;
            }
            lv_memcpy(kern_pairs, (buf + offset->offset_kern_dsc_pair), sizeof(lv_font_fmt_txt_kern_pair_t));
            font_bin_reset_kern_pair_runtime_ptrs(kern_pairs);
            font_dsc->kern_dsc = kern_pairs;

            //glyph_ids
            if (offset->offset_glyph_ids != 0) {
                uint32_t ids_size;
                if (kern_pairs->glyph_ids_size == 0) {
                    ids_size = kern_pairs->pair_cnt * 2 * sizeof(uint8_t);
                } else {
                    ids_size = kern_pairs->pair_cnt * 2 * sizeof(uint16_t);
                }

                void *glyph_ids = lv_malloc(ids_size);
                if (glyph_ids == NULL) {
                    goto error_handler;
                }
                lv_memcpy(glyph_ids, (buf + offset->offset_glyph_ids), ids_size);
                kern_pairs->glyph_ids = glyph_ids;
            }

            //values
            if (offset->offset_values != 0) {
                uint32_t values_size = kern_pairs->pair_cnt * sizeof(int8_t);
                int8_t *values = (int8_t *)lv_malloc(values_size);
                if (values == NULL) {
                    goto error_handler;
                }
                lv_memcpy(values, (buf + offset->offset_values), values_size);
                kern_pairs->values = values;
            }
        }
    } else {
        font_dsc->kern_dsc = NULL;
    }

    return font;

error_handler:
    if (font != NULL && font->dsc == NULL) {
        lv_free(font);
    } else {
        lv_binfont_destroy(font);
    }
    return NULL;
}

static lv_font_t *parse_font_fnt(char *buf, uint32_t len)
{
    if (buf == NULL) {
        return NULL;
    }

    // 初始化内存读取器
    mem_reader_t reader;
    reader.buf = buf;
    reader.pos = 0;
    reader.size = len; // dyn 资源项的 data_len 是字体解析器的硬边界，不能越过该视图读取。

    //font
    lv_font_t *font = (lv_font_t *)lv_malloc(sizeof(lv_font_t));
    if (font == NULL) {
        return NULL;
    }
    lv_memset(font, 0, sizeof(lv_font_t));
    cmap_table_bin_t *cmap_tables = NULL;
    uint32_t *glyph_offset = NULL;

    //font descriptor
    lv_font_fmt_txt_dsc_t *font_dsc = (lv_font_fmt_txt_dsc_t *)lv_malloc(sizeof(lv_font_fmt_txt_dsc_t));
    if (font_dsc == NULL) {
        lv_free(font);
        return NULL;
    }
    lv_memset(font_dsc, 0, sizeof(lv_font_fmt_txt_dsc_t));

    font->dsc = font_dsc;

    //head
    int32_t header_length = mem_read_label(&reader, 0, "head");
    if (header_length < 0) {
        goto error_handler;
    }

    font_header_bin_t font_header;
    if (!mem_read(&reader, &font_header, sizeof(font_header_bin_t))) {
        goto error_handler;
    }

    font->base_line = -font_header.descent;
    font->line_height = font_header.ascent - font_header.descent;
    font->get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
    font->get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
    font->subpx = font_header.subpixels_mode;
    font->underline_position = font_header.underline_position;
    font->underline_thickness = font_header.underline_thickness;

    font_dsc->bpp = font_header.bits_per_pixel;
    font_dsc->kern_scale = font_header.kerning_scale;
    font_dsc->bitmap_format = font_header.compression_id;

    // 解析字符映射表(cmaps)
    uint32_t cmaps_start = header_length;
    int32_t cmaps_length = mem_read_label(&reader, cmaps_start, "cmap");
    if (cmaps_length < 0) {
        goto error_handler;
    }

    uint32_t cmaps_subtables_count;
    if (!mem_read(&reader, &cmaps_subtables_count, sizeof(uint32_t))) {
        goto error_handler;
    }

    lv_font_fmt_txt_cmap_t *cmaps = lv_malloc(cmaps_subtables_count * sizeof(lv_font_fmt_txt_cmap_t));
    if (cmaps == NULL) {
        goto error_handler;
    }
    lv_memset(cmaps, 0, cmaps_subtables_count * sizeof(lv_font_fmt_txt_cmap_t));

    font_dsc->cmaps = cmaps;
    font_dsc->cmap_num = cmaps_subtables_count;

    // 读取cmap表
    cmap_tables = lv_malloc(sizeof(cmap_table_bin_t) * font_dsc->cmap_num);
    if (cmap_tables == NULL) {
        goto error_handler;
    }

    if (!mem_read(&reader, cmap_tables, font_dsc->cmap_num * sizeof(cmap_table_bin_t))) {
        goto error_handler;
    }

    // 处理每个cmap表
    for (unsigned int i = 0; i < font_dsc->cmap_num; ++i) {
        if (!mem_seek(&reader, cmaps_start + cmap_tables[i].data_offset)) {
            goto error_handler;
        }

        lv_font_fmt_txt_cmap_t *cmap = &cmaps[i];
        cmap->range_start = cmap_tables[i].range_start;
        cmap->range_length = cmap_tables[i].range_length;
        cmap->glyph_id_start = cmap_tables[i].glyph_id_start;
        cmap->type = cmap_tables[i].format_type;

        switch (cmap_tables[i].format_type) {
        case LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL: {
            uint8_t ids_size = sizeof(uint8_t) * cmap_tables[i].data_entries_count;
            uint8_t *glyph_id_ofs_list = lv_malloc(ids_size);
            if (glyph_id_ofs_list == NULL) {
                goto error_handler;
            }

            cmap->glyph_id_ofs_list = glyph_id_ofs_list;
            if (!mem_read(&reader, glyph_id_ofs_list, ids_size)) {
                goto error_handler;
            }

            cmap->list_length = cmap->range_length;
            break;
        }
        case LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY:
            break;
        case LV_FONT_FMT_TXT_CMAP_SPARSE_FULL:
        case LV_FONT_FMT_TXT_CMAP_SPARSE_TINY: {
            uint32_t list_size = sizeof(uint16_t) * cmap_tables[i].data_entries_count;
            uint16_t *unicode_list = (uint16_t *)lv_malloc(list_size);
            if (unicode_list == NULL) {
                goto error_handler;
            }

            cmap->unicode_list = unicode_list;
            cmap->list_length = cmap_tables[i].data_entries_count;

            if (!mem_read(&reader, unicode_list, list_size)) {
                goto error_handler;
            }

            if (cmap_tables[i].format_type == LV_FONT_FMT_TXT_CMAP_SPARSE_FULL) {
                uint16_t *buf = lv_malloc(sizeof(uint16_t) * cmap->list_length);
                if (buf == NULL) {
                    goto error_handler;
                }

                cmap->glyph_id_ofs_list = buf;

                if (!mem_read(&reader, buf, sizeof(uint16_t) * cmap->list_length)) {
                    goto error_handler;
                }
            }
            break;
        }
        default:
            goto error_handler;
        }
    }
    lv_free(cmap_tables);
    cmap_tables = NULL;

    // 解析字形位置表(loca)
    uint32_t loca_start = cmaps_start + cmaps_length;
    int32_t loca_length = mem_read_label(&reader, loca_start, "loca");
    if (loca_length < 0) {
        goto error_handler;
    }

    uint32_t loca_count;
    if (!mem_read(&reader, &loca_count, sizeof(uint32_t))) {
        goto error_handler;
    }

    glyph_offset = lv_malloc(sizeof(uint32_t) * (loca_count + 1));
    if (glyph_offset == NULL) {
        goto error_handler;
    }

    // 读取字形偏移
    if (font_header.index_to_loc_format == 0) {
        for (unsigned int i = 0; i < loca_count; ++i) {
            uint16_t offset;
            if (!mem_read(&reader, &offset, sizeof(uint16_t))) {
                goto error_handler;
            }
            glyph_offset[i] = offset;
        }
    } else if (font_header.index_to_loc_format == 1) {
        if (!mem_read(&reader, glyph_offset, loca_count * sizeof(uint32_t))) {
            goto error_handler;
        }
    } else {
        goto error_handler;
    }

    // 解析字形数据(glyph)
    uint32_t glyph_start = loca_start + loca_length;
    int32_t glyph_length = mem_read_label(&reader, glyph_start, "glyf");
    if (glyph_length < 0) {
        goto error_handler;
    }

    // 分配并初始化字形描述符
    lv_font_fmt_txt_glyph_dsc_t *glyph_dsc = lv_malloc(loca_count * sizeof(lv_font_fmt_txt_glyph_dsc_t));
    if (glyph_dsc == NULL) {
        goto error_handler;
    }
    lv_memset(glyph_dsc, 0, loca_count * sizeof(lv_font_fmt_txt_glyph_dsc_t));
    font_dsc->glyph_dsc = glyph_dsc;

    // 计算所需的位图大小并解析字形数据
    int cur_bmp_size = 0;
    for (unsigned int i = 0; i < loca_count; ++i) {
        lv_font_fmt_txt_glyph_dsc_t *gdsc = &glyph_dsc[i];

        if (!mem_seek(&reader, glyph_start + glyph_offset[i])) {
            goto error_handler;
        }

        mem_bit_iterator_t bit_it = init_mem_bit_iterator(&reader);
        bool success = true;

        if (font_header.advance_width_bits == 0) {
            gdsc->adv_w = font_header.default_advance_width;
        } else {
            gdsc->adv_w = mem_read_bits(&bit_it, font_header.advance_width_bits, &success);
            if (!success) {
                goto error_handler;
            }
        }

        if (font_header.advance_width_format == 0) {
            gdsc->adv_w *= 16;
        }

        gdsc->ofs_x = mem_read_bits_signed(&bit_it, font_header.xy_bits, &success);
        if (!success) {
            goto error_handler;
        }

        gdsc->ofs_y = mem_read_bits_signed(&bit_it, font_header.xy_bits, &success);
        if (!success) {
            goto error_handler;
        }

        gdsc->box_w = mem_read_bits(&bit_it, font_header.wh_bits, &success);
        if (!success) {
            goto error_handler;
        }

        gdsc->box_h = mem_read_bits(&bit_it, font_header.wh_bits, &success);
        if (!success) {
            goto error_handler;
        }

        int nbits = font_header.advance_width_bits + 2 * font_header.xy_bits + 2 * font_header.wh_bits;
        int next_offset = (i < loca_count - 1) ? glyph_offset[i + 1] : (uint32_t)glyph_length;
        int bmp_size = next_offset - glyph_offset[i] - nbits / 8;

        if (i == 0) {
            gdsc->adv_w = 0;
            gdsc->box_w = 0;
            gdsc->box_h = 0;
            gdsc->ofs_x = 0;
            gdsc->ofs_y = 0;
        }

        gdsc->bitmap_index = cur_bmp_size;
        if (gdsc->box_w * gdsc->box_h != 0) {
            cur_bmp_size += bmp_size;
        }
    }

    // 为字形位图分配内存
    uint8_t *glyph_bmp = lv_malloc(sizeof(uint8_t) * cur_bmp_size);
    if (glyph_bmp == NULL) {
        goto error_handler;
    }
    font_dsc->glyph_bitmap = glyph_bmp;

    // 读取字形位图数据
    cur_bmp_size = 0;
    for (unsigned int i = 1; i < loca_count; ++i) {
        if (!mem_seek(&reader, glyph_start + glyph_offset[i])) {
            goto error_handler;
        }

        mem_bit_iterator_t bit_it = init_mem_bit_iterator(&reader);
        bool success = true;

        int nbits = font_header.advance_width_bits + 2 * font_header.xy_bits + 2 * font_header.wh_bits;
        mem_read_bits(&bit_it, nbits, &success);

        if (!success) {
            goto error_handler;
        }

        if (glyph_dsc[i].box_w * glyph_dsc[i].box_h == 0) {
            continue;
        }

        int next_offset = (i < loca_count - 1) ? glyph_offset[i + 1] : (uint32_t)glyph_length;
        int bmp_size = next_offset - glyph_offset[i] - nbits / 8;

        // 读取位图数据
        if (nbits % 8 == 0) { // 快速路径
            lv_memcpy(&glyph_bmp[cur_bmp_size], reader.buf + reader.pos, bmp_size);
            reader.pos += bmp_size;
        } else {
            for (int k = 0; k < bmp_size - 1; ++k) {
                glyph_bmp[cur_bmp_size + k] = mem_read_bits(&bit_it, 8, &success);
                if (!success) {
                    goto error_handler;
                }
            }
            glyph_bmp[cur_bmp_size + bmp_size - 1] = mem_read_bits(&bit_it, 8 - nbits % 8, &success);
            if (!success) {
                goto error_handler;
            }

            // 最后一个片段应该在MSB上，但mem_read_bits()会将其放在LSB上
            glyph_bmp[cur_bmp_size + bmp_size - 1] = glyph_bmp[cur_bmp_size + bmp_size - 1] << (nbits % 8);
        }

        cur_bmp_size += bmp_size;
    }

    lv_free(glyph_offset);
    glyph_offset = NULL;

    // 如果有kerning数据，则解析
    if (font_header.tables_count >= 4) {
        uint32_t kern_start = glyph_start + glyph_length;
        int32_t kern_length = mem_read_label(&reader, kern_start, "kern");

        if (kern_length >= 0) {
            uint8_t kern_format_type;
            uint8_t padding[3];

            if (!mem_read(&reader, &kern_format_type, sizeof(uint8_t)) ||
                !mem_read(&reader, padding, 3 * sizeof(uint8_t))) {
                goto error_handler;
            }

            if (0 == kern_format_type) { // 排序对
                lv_font_fmt_txt_kern_pair_t *kern_pair = lv_malloc(sizeof(lv_font_fmt_txt_kern_pair_t));
                if (kern_pair == NULL) {
                    goto error_handler;
                }
                lv_memset(kern_pair, 0, sizeof(lv_font_fmt_txt_kern_pair_t));

                font_dsc->kern_dsc = kern_pair;
                font_dsc->kern_classes = 0;

                uint32_t glyph_entries;
                if (!mem_read(&reader, &glyph_entries, sizeof(uint32_t))) {
                    goto error_handler;
                }

                int ids_size;
                if (font_header.glyph_id_format == 0) {
                    ids_size = sizeof(int8_t) * 2 * glyph_entries;
                } else {
                    ids_size = sizeof(int16_t) * 2 * glyph_entries;
                }

                uint8_t *glyph_ids = lv_malloc(ids_size);
                int8_t *values = lv_malloc(glyph_entries);

                if (glyph_ids == NULL || values == NULL) {
                    if (glyph_ids) {
                        lv_free(glyph_ids);
                    }
                    if (values) {
                        lv_free(values);
                    }
                    goto error_handler;
                }

                kern_pair->glyph_ids_size = font_header.glyph_id_format;
                kern_pair->pair_cnt = glyph_entries;
                kern_pair->glyph_ids = glyph_ids;
                kern_pair->values = values;

                if (!mem_read(&reader, glyph_ids, ids_size) ||
                    !mem_read(&reader, values, glyph_entries)) {
                    goto error_handler;
                }
            } else if (3 == kern_format_type) { // M*N类数组
                lv_font_fmt_txt_kern_classes_t *kern_classes = lv_malloc(sizeof(lv_font_fmt_txt_kern_classes_t));
                if (kern_classes == NULL) {
                    goto error_handler;
                }
                lv_memset(kern_classes, 0, sizeof(lv_font_fmt_txt_kern_classes_t));

                font_dsc->kern_dsc = kern_classes;
                font_dsc->kern_classes = 1;

                uint16_t kern_class_mapping_length;
                uint8_t kern_table_rows;
                uint8_t kern_table_cols;

                if (!mem_read(&reader, &kern_class_mapping_length, sizeof(uint16_t)) ||
                    !mem_read(&reader, &kern_table_rows, sizeof(uint8_t)) ||
                    !mem_read(&reader, &kern_table_cols, sizeof(uint8_t))) {
                    goto error_handler;
                }

                int kern_values_length = sizeof(int8_t) * kern_table_rows * kern_table_cols;

                uint8_t *kern_left = lv_malloc(kern_class_mapping_length);
                uint8_t *kern_right = lv_malloc(kern_class_mapping_length);
                int8_t *kern_values = lv_malloc(kern_values_length);

                if (kern_left == NULL || kern_right == NULL || kern_values == NULL) {
                    if (kern_left) {
                        lv_free(kern_left);
                    }
                    if (kern_right) {
                        lv_free(kern_right);
                    }
                    if (kern_values) {
                        lv_free(kern_values);
                    }
                    goto error_handler;
                }

                kern_classes->left_class_mapping = kern_left;
                kern_classes->right_class_mapping = kern_right;
                kern_classes->left_class_cnt = kern_table_rows;
                kern_classes->right_class_cnt = kern_table_cols;
                kern_classes->class_pair_values = kern_values;

                if (!mem_read(&reader, kern_left, kern_class_mapping_length) ||
                    !mem_read(&reader, kern_right, kern_class_mapping_length) ||
                    !mem_read(&reader, kern_values, kern_values_length)) {
                    goto error_handler;
                }
            } else {
                goto error_handler;
            }
        }
    } else {
        font_dsc->kern_dsc = NULL;
        font_dsc->kern_classes = 0;
        font_dsc->kern_scale = 0;
    }

    return font;

error_handler:
    lv_free(cmap_tables);
    lv_free(glyph_offset);
    lv_binfont_destroy(font);
    return NULL;
}

static mem_bit_iterator_t init_mem_bit_iterator(mem_reader_t *reader)
{
    mem_bit_iterator_t it;
    it.reader = reader;
    it.bit_pos = -1;
    it.byte_value = 0;
    return it;
}

static unsigned int mem_read_bits(mem_bit_iterator_t *it, int n_bits, bool *success)
{
    unsigned int value = 0;
    while (n_bits--) {
        it->byte_value = it->byte_value << 1;
        it->bit_pos--;

        if (it->bit_pos < 0) {
            it->bit_pos = 7;
            if (it->reader->pos < it->reader->size) {
                it->byte_value = (uint8_t)(it->reader->buf[it->reader->pos++]);
            } else {
                *success = false;
                return 0;
            }
        }
        int8_t bit = (it->byte_value & 0x80) ? 1 : 0;

        value |= (bit << n_bits);
    }
    *success = true;
    return value;
}

static int mem_read_bits_signed(mem_bit_iterator_t *it, int n_bits, bool *success)
{
    unsigned int value = mem_read_bits(it, n_bits, success);
    if (!*success) {
        return 0;
    }

    if (value & (1 << (n_bits - 1))) {
        value |= ~0u << n_bits;
    }
    return value;
}

static int32_t mem_read_label(mem_reader_t *reader, uint32_t start, const char *label)
{
    if (start > reader->size || reader->size - start < 8U) {
        return -1;
    }

    reader->pos = start;

    uint32_t length = mem_read_le32(reader->buf + reader->pos);
    reader->pos += 4;

    char *buf = reader->buf + reader->pos;
    reader->pos += 4;

    if (lv_memcmp(label, buf, 4) != 0) {
        return -1;
    }
    if (length < 8U || length > reader->size - start) {
        return -1;
    }

    return length;
}

static bool mem_read(mem_reader_t *reader, void *dst, uint32_t size)
{
    if (reader->pos > reader->size || size > reader->size - reader->pos) {
        return false;
    }

    lv_memcpy(dst, reader->buf + reader->pos, size);
    reader->pos += size;
    return true;
}

static bool mem_seek(mem_reader_t *reader, uint32_t pos)
{
    if (pos > reader->size) {
        return false;
    }

    reader->pos = pos;
    return true;
}

static uint32_t mem_read_le32(const char *data)
{
    const uint8_t *p = (const uint8_t *)data;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

#endif /*LV_USE_GUI_PARSE_DYN*/
