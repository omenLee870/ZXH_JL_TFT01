/**
 * @file lv_jl_image_convert.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_jl_image_convert.h"
#include "../../../../core/lv_global.h"

/*********************
 *      DEFINES
 *********************/
#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if LV_USE_GUIBUILDER_SIMULATOR
static lv_result_t jl_decode_indexed_line(lv_color_format_t color_format, const lv_color32_t * palette,
                                          int32_t x, int32_t w_px, const uint8_t * in, lv_color32_t * out);
static lv_draw_buf_t * jl_decode_indexed_to_argb8888(const lv_image_dsc_t * src_dsc);
static lv_draw_buf_t * jl_convert_alpha_to_a8(const lv_image_dsc_t * src_dsc);
static lv_draw_buf_t * jl_convert_argb8565_to_rgb565a8(const lv_image_dsc_t * src_dsc);
static uint8_t jl_alpha_bit_extend(uint8_t value, uint8_t bpp);
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool lv_jl_image_needs_convert(lv_color_format_t cf)
{
#if LV_USE_GUIBUILDER_SIMULATOR
    return LV_COLOR_FORMAT_IS_INDEXED(cf)
           || cf == LV_COLOR_FORMAT_A1
           || cf == LV_COLOR_FORMAT_A2
           || cf == LV_COLOR_FORMAT_A4
           || cf == LV_COLOR_FORMAT_ARGB8565;
#else
    LV_UNUSED(cf);
    return false;
#endif
}

lv_draw_buf_t * lv_jl_image_convert(const lv_image_dsc_t * src_dsc)
{
#if LV_USE_GUIBUILDER_SIMULATOR
    if(src_dsc == NULL || src_dsc->data == NULL) {
        return NULL;
    }

    lv_color_format_t cf = src_dsc->header.cf;
    if(LV_COLOR_FORMAT_IS_INDEXED(cf)) {
        return jl_decode_indexed_to_argb8888(src_dsc);
    }

    if(cf == LV_COLOR_FORMAT_A1 || cf == LV_COLOR_FORMAT_A2 || cf == LV_COLOR_FORMAT_A4) {
        return jl_convert_alpha_to_a8(src_dsc);
    }

    if(cf == LV_COLOR_FORMAT_ARGB8565) {
        return jl_convert_argb8565_to_rgb565a8(src_dsc);
    }
#else
    LV_UNUSED(src_dsc);
#endif

    return NULL;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if LV_USE_GUIBUILDER_SIMULATOR
static lv_draw_buf_t * jl_decode_indexed_to_argb8888(const lv_image_dsc_t * src_dsc)
{
    lv_color_format_t cf = src_dsc->header.cf;
    uint32_t palette_size = LV_COLOR_INDEXED_PALETTE_SIZE(cf);
    if(palette_size == 0) {
        return NULL;
    }

    uint32_t src_stride = src_dsc->header.stride ? src_dsc->header.stride :
                          ((src_dsc->header.w * lv_color_format_get_bpp(cf) + 7) >> 3);
    uint32_t palette_len = palette_size * sizeof(lv_color32_t);
    uint32_t indexed_data_len = src_stride * src_dsc->header.h;
    if(src_dsc->data_size < palette_len + indexed_data_len) {
        LV_LOG_WARN("Invalid indexed image size: %" LV_PRIu32 ", expected at least: %" LV_PRIu32,
                    src_dsc->data_size, palette_len + indexed_data_len);
        return NULL;
    }

    lv_draw_buf_t * decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers,
                                                    src_dsc->header.w,
                                                    src_dsc->header.h,
                                                    LV_COLOR_FORMAT_ARGB8888,
                                                    LV_STRIDE_AUTO);
    if(decoded == NULL) {
        return NULL;
    }

    const lv_color32_t * palette = (const lv_color32_t *)src_dsc->data;
    const uint8_t * in = src_dsc->data + palette_len;
    uint8_t * out = decoded->data;

    for(uint32_t y = 0; y < src_dsc->header.h; y++) {
        if(jl_decode_indexed_line(cf, palette, 0, src_dsc->header.w, in, (lv_color32_t *)out) != LV_RESULT_OK) {
            lv_draw_buf_destroy(decoded);
            return NULL;
        }
        in += src_stride;
        out += decoded->header.stride;
    }

    return decoded;
}

static lv_draw_buf_t * jl_convert_alpha_to_a8(const lv_image_dsc_t * src_dsc)
{
    lv_color_format_t cf = src_dsc->header.cf;
    uint8_t bpp = lv_color_format_get_bpp(cf);
    if(bpp == 0 || bpp >= 8) {
        return NULL;
    }

    uint32_t min_src_stride = (src_dsc->header.w * bpp + 7) >> 3;
    uint32_t src_stride = src_dsc->header.stride ? src_dsc->header.stride : min_src_stride;
    uint32_t src_data_len = src_stride * src_dsc->header.h;
    if(src_stride < min_src_stride || src_dsc->data_size < src_data_len) {
        LV_LOG_WARN("Invalid alpha image layout, cf: %d, stride: %" LV_PRIu32 ", size: %" LV_PRIu32,
                    cf, src_stride, src_dsc->data_size);
        return NULL;
    }

    lv_draw_buf_t * decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers,
                                                    src_dsc->header.w,
                                                    src_dsc->header.h,
                                                    LV_COLOR_FORMAT_A8,
                                                    LV_STRIDE_AUTO);
    if(decoded == NULL) {
        return NULL;
    }

    uint8_t mask = (1U << bpp) - 1U;
    const uint8_t * src_row = src_dsc->data;

    for(uint32_t y = 0; y < src_dsc->header.h; y++) {
        uint8_t * out = (uint8_t *)decoded->data + decoded->header.stride * y;
        const uint8_t * in = src_row;
        int8_t shift = 8 - bpp;

        for(uint32_t x = 0; x < src_dsc->header.w; x++) {
            uint8_t value = (*in >> shift) & mask;
            out[x] = jl_alpha_bit_extend(value, bpp);

            shift -= bpp;
            if(shift < 0) {
                shift = 8 - bpp;
                in++;
            }
        }

        src_row += src_stride;
    }

    return decoded;
}

static lv_draw_buf_t * jl_convert_argb8565_to_rgb565a8(const lv_image_dsc_t * src_dsc)
{
    uint32_t min_src_stride = src_dsc->header.w * 3;
    uint32_t src_stride = src_dsc->header.stride ? src_dsc->header.stride : min_src_stride;
    uint32_t src_data_len = src_stride * src_dsc->header.h;
    if(src_stride < min_src_stride || src_dsc->data_size < src_data_len) {
        LV_LOG_WARN("Invalid ARGB8565 image layout, stride: %" LV_PRIu32 ", size: %" LV_PRIu32,
                    src_stride, src_dsc->data_size);
        return NULL;
    }

    lv_draw_buf_t * decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers,
                                                    src_dsc->header.w,
                                                    src_dsc->header.h,
                                                    LV_COLOR_FORMAT_RGB565A8,
                                                    LV_STRIDE_AUTO);
    if(decoded == NULL) {
        return NULL;
    }

    uint8_t * color_plane = decoded->data;
    uint8_t * alpha_plane = color_plane + decoded->header.stride * decoded->header.h;
    uint32_t alpha_stride = decoded->header.stride / 2;
    const uint8_t * src_row = src_dsc->data;

    for(uint32_t y = 0; y < src_dsc->header.h; y++) {
        uint8_t * color_row = color_plane + decoded->header.stride * y;
        uint8_t * alpha_row = alpha_plane + alpha_stride * y;

        for(uint32_t x = 0; x < src_dsc->header.w; x++) {
            color_row[x * 2 + 0] = src_row[x * 3 + 0];
            color_row[x * 2 + 1] = src_row[x * 3 + 1];
            alpha_row[x] = src_row[x * 3 + 2];
        }

        src_row += src_stride;
    }

    return decoded;
}

static uint8_t jl_alpha_bit_extend(uint8_t value, uint8_t bpp)
{
    if(value == 0) return 0;

    uint8_t res = value;
    uint8_t bpp_now = bpp;
    while(bpp_now < 8) {
        res |= value << (8 - bpp_now);
        bpp_now += bpp;
    }

    return res;
}

static lv_result_t jl_decode_indexed_line(lv_color_format_t color_format, const lv_color32_t * palette,
                                          int32_t x, int32_t w_px, const uint8_t * in, lv_color32_t * out)
{
    uint8_t px_size;
    uint16_t mask;

    int8_t shift = 0;
    switch(color_format) {
        case LV_COLOR_FORMAT_I1:
            px_size = 1;
            in += x / 8;
            shift = 7 - (x & 0x7);
            break;
        case LV_COLOR_FORMAT_I2:
            px_size = 2;
            in += x / 4;
            shift = 6 - 2 * (x & 0x3);
            break;
        case LV_COLOR_FORMAT_I4:
            px_size = 4;
            in += x / 2;
            shift = 4 - 4 * (x & 0x1);
            break;
        case LV_COLOR_FORMAT_I8:
            px_size = 8;
            in += x;
            shift = 0;
            break;
        default:
            return LV_RESULT_INVALID;
    }

    mask = (1U << px_size) - 1U;

    for(int32_t i = 0; i < w_px; i++) {
        uint8_t val_act = (*in >> shift) & mask;
        out[i] = palette[val_act];

        shift -= px_size;
        if(shift < 0) {
            shift = 8 - px_size;
            in++;
        }
    }

    return LV_RESULT_OK;
}
#endif
