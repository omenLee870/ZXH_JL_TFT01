/**
 * @file res_image.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "res_font.h"
#include "res.h"
#include "../../../extra/libs/bin/lv_jl_image_convert.h"

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

// 压缩类型
typedef enum {
    COMPRESS_NONE = 1,
    COMPRESS_ZIP = 2,
    COMPRESS_RLE = 3,
    COMPRESS_ETC2 = 4,
    COMPRESS_LZ4 = 5,
    COMPRESS_FNT = 6
} CompressType;

typedef enum {
    RLE_BLOCK_NONE = -1,
    RLE_BLOCK_64x1,
    RLE_BLOCK_8x8,
    RLE_BLOCK_256x1,
    RLE_BLOCK_16x16,
} RleBlock;

typedef enum {
    RAW_BLOCK_NONE = -1,
    RAW_BLOCK_64x1,
    RAW_BLOCK_8x8,
    RAW_BLOCK_256x1,
    RAW_BLOCK_16x16,
} RawBlock;

typedef enum {
    LUT_FORMAT_NONE = -1,
    LUT_FORMAT_ARGB8888,
    LUT_FORMAT_ARGB8565,
    LUT_FORMAT_RGB888,
    LUT_FORMAT_RGB565,
} LutFormat;

typedef enum {
    JLDMA2D_FORMAT_ARGB8888 = 0x00,
    JLDMA2D_FORMAT_ARGB8565 = 0x01,
    JLDMA2D_FORMAT_ARGB1555 = 0x02,
    JLDMA2D_FORMAT_ARGB4444 = 0x03,
    JLDMA2D_FORMAT_RGB888 = 0x04,
    JLDMA2D_FORMAT_RGB565 = 0x05,
    JLDMA2D_FORMAT_YUV422_BT601 = 0x06,
    JLDMA2D_FORMAT_YUV422_BT709 = 0x07,
    JLDMA2D_FORMAT_AL88 = 0x08,
    JLDMA2D_FORMAT_AL44 = 0x10,
    JLDMA2D_FORMAT_AL22 = 0x11,
    JLDMA2D_FORMAT_L8 = 0x12,
    JLDMA2D_FORMAT_L4 = 0x13,
    JLDMA2D_FORMAT_L2 = 0x14,
    JLDMA2D_FORMAT_L1 = 0x15,
    JLDMA2D_FORMAT_A8 = 0x16,
    JLDMA2D_FORMAT_A4 = 0x17,
    JLDMA2D_FORMAT_A2 = 0x18,
    JLDMA2D_FORMAT_A1 = 0x19,
    JLDMA2D_FORMAT_UNKNOWN,
} JLDma2dFormat;

// RLE相关结构体定义
typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t format;    // 图像格式
    uint16_t block;     // 块大小
    uint16_t lut_format;// 色表格式
    uint16_t nop;
    uint32_t stride;    // 每行字节数
    uint32_t lut_addr;  // 色表地址
    uint32_t lut_len;   // 色表大小
    uint32_t addr;      // 压缩数据地址
    uint32_t len;       // 压缩数据长度
} rle_info_t;

// ETC2相关结构体定义
typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t format;    // 图像格式
    uint32_t img_width;
    uint32_t img_height;
} c2_info_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint8_t * parse_image_data(char * buf, uint32_t * data_size, uint32_t * stride);
static uint8_t * parse_image_data_bin(char * buf, uint32_t * data_size, uint32_t * stride);
static uint8_t * parse_image_data_rle(char * buf, uint32_t * data_size, uint32_t * stride);
static uint8_t * parse_image_data_etc(char * buf, uint32_t * data_size, uint32_t * stride);

static uint32_t get_image_block(uint32_t block, uint32_t compress_type);
static uint32_t get_lut_format_byte_size(LutFormat format);
static uint32_t get_row_size(uint32_t width, uint32_t bpp);
static uint32_t get_image_storage_stride(uint32_t width, lv_color_format_t cf);
static bool is_valid_rle_cf_pair(uint16_t rle_format, lv_color_format_t cf);
static bool is_valid_etc_cf_pair(uint16_t etc_format, lv_color_format_t cf);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * res_image_parse(char * buf)
{
    Resource * resource = (Resource *)buf;
    if(resource->header.res_type != RES_TYPE_IMAGE) {
        return NULL;
    }

    lv_image_dsc_t * img_dsc = lv_malloc(sizeof(lv_image_dsc_t));
    if(img_dsc == NULL) {
        return NULL;
    }

    // 解析图像数据
    uint32_t data_size = 0;
    uint32_t actual_stride = 0;
    uint8_t * data = parse_image_data(buf, &data_size, &actual_stride);
    if(data == NULL) {
        lv_free(img_dsc);
        return NULL;
    }

    // 设置图像描述符
    lv_image_dsc_t * src_dsc = (lv_image_dsc_t *)(buf + sizeof(ResourceDiskHeader));
    lv_memcpy(&img_dsc->header, &src_dsc->header, sizeof(lv_image_header_t));
    img_dsc->data_size = data_size;
    img_dsc->data = data;

    if(actual_stride != 0) {
        if(img_dsc->header.stride == 0) {
            LV_LOG_INFO("Dynamic image stride missing, use parsed stride: %" LV_PRIu32, actual_stride);
        }
        else if(img_dsc->header.stride != actual_stride) {
            LV_LOG_WARN("Dynamic image stride mismatch: resource=%" LV_PRIu32 ", parsed=%" LV_PRIu32,
                        img_dsc->header.stride, actual_stride);
        }
        img_dsc->header.stride = actual_stride;
    }

    /* 动态资源也复用仿真专用格式转换，避免与文件型 JL BIN 显示效果不一致。 */
    if(lv_jl_image_needs_convert(img_dsc->header.cf)) {
        lv_draw_buf_t * converted = lv_jl_image_convert(img_dsc);
        if(converted == NULL) {
            lv_free((void *)img_dsc->data);
            lv_free(img_dsc);
            return NULL;
        }

        lv_free((void *)img_dsc->data);
        lv_free(img_dsc);
        return converted;
    }

    return img_dsc;
}

void res_image_free(void * buf)
{
    if(buf == NULL) {
        return;
    }

    lv_image_dsc_t * img = (lv_image_dsc_t *)buf;
    if(img->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
        lv_draw_buf_destroy((lv_draw_buf_t *)buf);
        return;
    }

    if(img->data != NULL) {
        lv_free((void *)img->data);
        img->data = NULL;
    }
    lv_free(img);
}

/**********************
 *   STATIC FUNCTIONS
 ***********************/

static uint8_t * parse_image_data(char * buf, uint32_t * data_size, uint32_t * stride)
{
    Resource * resource = (Resource *)buf;

    switch(resource->header.res_compress) {
        case COMPRESS_NONE:
            return parse_image_data_bin(buf, data_size, stride);
        case COMPRESS_RLE:
            return parse_image_data_rle(buf, data_size, stride);
        case COMPRESS_ETC2:
            return parse_image_data_etc(buf, data_size, stride);
        default:
            return NULL;
    }
}

static uint8_t * parse_image_data_bin(char * buf, uint32_t * data_size, uint32_t * stride)
{
    Resource * resource = (Resource *)buf;
    lv_image_dsc_t * src_dsc = (lv_image_dsc_t *)(buf + sizeof(ResourceDiskHeader));
    uint8_t * src_data = (uint8_t *)(buf + sizeof(ResourceDiskHeader) + sizeof(lv_image_dsc_t));

    uint8_t * data = NULL;
    uint32_t block = get_image_block(resource->header.nop.image.block, resource->header.res_compress);
    if(block == ENCODE_RAW_BLOCK_8x8 || block == ENCODE_RAW_BLOCK_16x16) {
        uint32 lut_len = 0; //颜色表大小
        if(src_dsc->header.cf == LV_COLOR_FORMAT_I1 ||
           src_dsc->header.cf == LV_COLOR_FORMAT_I2 ||
           src_dsc->header.cf == LV_COLOR_FORMAT_I4 ||
           src_dsc->header.cf == LV_COLOR_FORMAT_I8) {
            lut_len = (1 << lv_color_format_get_bpp(src_dsc->header.cf)) * 4; //bin-Lx 的颜色表一个颜色4字节ARGB8888
        }
        if(src_dsc->data_size < lut_len) {
            return NULL;
        }

        uint32_t bpp = lv_color_format_get_bpp(src_dsc->header.cf); //bpp
        uint8_t * src_img_data = (uint8_t *)src_data + lut_len; //解压的图片数据是颜色表后面的数据
        int out_size = get_row_size(src_dsc->header.w, bpp) * src_dsc->header.h; //图片数据大小
        data = lv_malloc(out_size + lut_len);
        if(data == NULL) {
            return NULL;
        }

        //复制颜色表
        lv_memcpy(data, src_data, lut_len);

        //解压
        int ret = image_rle_decode(src_dsc->header.w, src_dsc->header.h,
                                   bpp, block,
                                   src_img_data, src_dsc->data_size - lut_len,
                                   data + lut_len, out_size);

        if(ret < 0) {
            lv_free(data);
            return NULL;
        }

        *data_size = out_size + lut_len;
        *stride = get_image_storage_stride(src_dsc->header.w, src_dsc->header.cf);
        return data;
    }
    else {
        /* 64x1/256x1 的 RAW block 在文件里仍是行优先布局，直接复制即可。 */
        data = lv_malloc(src_dsc->data_size);
        if(data == NULL) {
            return NULL;
        }
        lv_memcpy(data, src_data, src_dsc->data_size);
        *data_size = src_dsc->data_size;
        *stride = src_dsc->header.stride;
        return data;
    }
}

static uint8_t * parse_image_data_rle(char * buf, uint32_t * data_size, uint32_t * stride)
{
    Resource * resource = (Resource *)buf;
    lv_image_dsc_t * src_dsc = (lv_image_dsc_t *)(buf + sizeof(ResourceDiskHeader));
    rle_info_t * rle_info = (rle_info_t *)((uint8_t *)src_dsc + sizeof(lv_image_dsc_t));
    lv_color_format_t cf = src_dsc->header.cf;

    if(src_dsc->data_size < sizeof(rle_info_t)) {
        return NULL;
    }

    if(!is_valid_rle_cf_pair(rle_info->format, cf)) {
        LV_LOG_WARN("Unsupported RLE format/cf pair: format=%u, cf=%u", rle_info->format, cf);
        return NULL;
    }

    uint32_t bpp = lv_color_format_get_bpp(cf);
    if(bpp < 8) {
        return NULL; //颜色位深小于8不支持
    }

    uint32_t block = get_image_block(rle_info->block, resource->header.res_compress);
    uint32_t src_lut_len = 0;
    uint32_t lut_len = 0;
    if(src_dsc->header.cf == LV_COLOR_FORMAT_I8) {
        src_lut_len = (1 << bpp) * get_lut_format_byte_size(rle_info->lut_format); //颜色表大小
        lut_len = (1 << bpp) * 4; //bin-Lx 的颜色表一个颜色4字节
    }
    else if(src_dsc->header.cf == LV_COLOR_FORMAT_A8) {
        src_lut_len = get_lut_format_byte_size(rle_info->lut_format);
    }

    if(rle_info->lut_addr > src_dsc->data_size ||
       rle_info->addr > src_dsc->data_size ||
       rle_info->lut_addr + src_lut_len > src_dsc->data_size ||
       rle_info->addr + rle_info->len > src_dsc->data_size) {
        LV_LOG_WARN("Invalid RLE payload layout: size=%" LV_PRIu32 ", lut_addr=%" LV_PRIu32 ", lut_len=%" LV_PRIu32
                    ", addr=%" LV_PRIu32 ", len=%" LV_PRIu32,
                    src_dsc->data_size, rle_info->lut_addr, src_lut_len, rle_info->addr, rle_info->len);
        return NULL;
    }

    const uint8_t * lut_data = (const uint8_t *)rle_info + rle_info->lut_addr;
    uint8_t * img_data = (uint8_t *)rle_info + rle_info->addr;
    uint32_t img_data_len = rle_info->len;

    int out_size = get_row_size(src_dsc->header.w, bpp) * src_dsc->header.h; //图片数据大小
    uint8_t * data = lv_malloc(out_size + lut_len); //颜色表+图片数据
    if(data == NULL) {
        return NULL;
    }

    // 赋值颜色表，根据lut_format将颜色表转为ARGB8888
    if(lut_len > 0) {
        uint32_t lut_cnt = (1 << bpp);
        if(rle_info->lut_format == LUT_FORMAT_ARGB8888) {
            // ARGB8888格式,直接复制
            lv_memcpy(data, lut_data, lut_cnt * 4);
        }
        else if(rle_info->lut_format == LUT_FORMAT_ARGB8565) {
            // ARGB8565格式转ARGB8888
            for(uint32_t i = 0; i < lut_cnt; i++) {
                // 从RGB565转换到RGB888
                uint16_t rgb = (lut_data[i * 3 + 1] << 8) | lut_data[i * 3];
                data[i * 4] = (rgb & 0x1F) << 3;              // B
                data[i * 4 + 1] = ((rgb >> 5) & 0x3F) << 2;   // G
                data[i * 4 + 2] = ((rgb >> 11) & 0x1F) << 3;  // R
                data[i * 4 + 3] = lut_data[i * 3 + 2];        // A
            }
        }
        else if(rle_info->lut_format == LUT_FORMAT_RGB888) {
            // RGB888格式转ARGB8888
            for(uint32_t i = 0; i < lut_cnt; i++) {
                data[i * 4] = lut_data[i * 3];          // B
                data[i * 4 + 1] = lut_data[i * 3 + 1];  // G
                data[i * 4 + 2] = lut_data[i * 3 + 2];  // R
                data[i * 4 + 3] = 0xFF;                 // A
            }
        }
        else if(rle_info->lut_format == LUT_FORMAT_RGB565) {
            // RGB565格式转ARGB8888
            for(uint32_t i = 0; i < lut_cnt; i++) {
                uint16_t rgb = (lut_data[i * 2 + 1] << 8) | lut_data[i * 2];
                data[i * 4 + 0] = (rgb & 0x1F) << 3;          // B
                data[i * 4 + 1] = ((rgb >> 5) & 0x3F) << 2;   // G
                data[i * 4 + 2] = ((rgb >> 11) & 0x1F) << 3;  // R
                data[i * 4 + 3] = 0xFF;                       // A
            }
        }
    }

    int ret = image_rle_decode(src_dsc->header.w, src_dsc->header.h,
                               bpp, block,
                               img_data, img_data_len,
                               data + lut_len, out_size);

    if(ret < 0) {
        lv_free(data);
        return NULL;
    }

    *data_size = out_size + lut_len;
    *stride = get_image_storage_stride(src_dsc->header.w, src_dsc->header.cf);
    return data;
}

static uint8_t * parse_image_data_etc(char * buf, uint32_t * data_size, uint32_t * stride)
{
    lv_image_dsc_t * src_dsc = (lv_image_dsc_t *)(buf + sizeof(ResourceDiskHeader));
    c2_info_t * etc_info = (c2_info_t *)((uint8_t *)src_dsc + sizeof(lv_image_dsc_t));
    lv_color_format_t cf = src_dsc->header.cf;

    // 检查支持的格式
    if(etc_info->format != JLDMA2D_FORMAT_ARGB8888 &&
       etc_info->format != JLDMA2D_FORMAT_RGB888 &&
       etc_info->format != JLDMA2D_FORMAT_A8) {
        return NULL;
    }

    if(!is_valid_etc_cf_pair(etc_info->format, cf)) {
        LV_LOG_WARN("Unsupported ETC2 format/cf pair: format=%u, cf=%u", etc_info->format, cf);
        return NULL;
    }

    uint32_t bpp = lv_color_format_get_bpp(cf);
    if(bpp == 0 || (LV_COLOR_DEPTH < 16 && bpp > 8)) {
        return NULL;
    }

    uint32_t width = etc_info->width;
    uint32_t height = etc_info->height;
    uint32_t in_size = 0;
    uint32_t out_size = 0;
    uint8_t * img = NULL;
    uint8_t * alpha = NULL;
    int32_t etc2_format = ETC2PACKAGE_RGBA_NO_MIPMAPS;

    // 根据格式分配内存和设置参数
    if(etc_info->format == JLDMA2D_FORMAT_ARGB8888) {
        etc2_format = ETC2PACKAGE_RGBA_NO_MIPMAPS;
        in_size = etc_info->img_width * etc_info->img_height * 4;
        out_size = width * height * 4;
        img = lv_malloc(out_size);
        alpha = lv_malloc(out_size);
        if(!img || !alpha) {
            lv_free(img);
            lv_free(alpha);
            return NULL;
        }
    }
    else if(etc_info->format == JLDMA2D_FORMAT_RGB888) {
        etc2_format = ETC2PACKAGE_RGB_NO_MIPMAPS;
        in_size = etc_info->img_width * etc_info->img_height * 3;
        out_size = width * height * 3;
        img = lv_malloc(out_size);
        if(!img) {
            return NULL;
        }
    }
    else if(etc_info->format == JLDMA2D_FORMAT_A8) {
        etc2_format = ETC2PACKAGE_A8_NO_MIPMAPS;
        in_size = etc_info->img_width * etc_info->img_height;
        out_size = width * height;
        alpha = lv_malloc(out_size);
        if(!alpha) {
            return NULL;
        }
    }

    // 解压缩ETC2数据
    uint8_t * etc_data = (uint8_t *)etc_info + sizeof(c2_info_t);
    int32_t ret = ETC2UnCompressRawData(etc_data, in_size, width, height, etc2_format, img, alpha);
    if(ret < 0) {
        lv_free(img);
        lv_free(alpha);
        return NULL;
    }

    // 根据lvgl的format，重新构建data
    uint8_t * data = NULL;
    if(etc_info->format == JLDMA2D_FORMAT_ARGB8888) {
        uint32_t data_size_bytes = width * height * 4;
        data = lv_malloc(data_size_bytes);
        if(!data) {
            lv_free(img);
            lv_free(alpha);
            return NULL;
        }

        for(uint32_t i = 0; i < width * height; i++) {
            data[i * 4] = img[i * 3 + 2];        // B
            data[i * 4 + 1] = img[i * 3 + 1];    // G
            data[i * 4 + 2] = img[i * 3];        // R
            data[i * 4 + 3] = alpha[i];          // A
        }
        *data_size = data_size_bytes;
    }
    else if(etc_info->format == JLDMA2D_FORMAT_RGB888) {
        uint32_t data_size_bytes = width * height * 3;
        data = lv_malloc(data_size_bytes);
        if(!data) {
            lv_free(img);
            return NULL;
        }

        for(uint32_t i = 0; i < width * height; i++) {
            data[i * 3] = img[i * 3 + 2];        // B
            data[i * 3 + 1] = img[i * 3 + 1];    // G
            data[i * 3 + 2] = img[i * 3];        // R
        }
        *data_size = data_size_bytes;
    }
    else if(etc_info->format == JLDMA2D_FORMAT_A8) {
        data = alpha;
        *data_size = out_size;
        alpha = NULL; // 防止被释放
    }

    *stride = get_image_storage_stride(width, cf);

    lv_free(img);
    lv_free(alpha);
    return data;
}

// 根据block和压缩类型，获取image_rle_decode的block参数
static uint32_t get_image_block(uint32_t block, uint32_t compress_type)
{
    if(compress_type == COMPRESS_RLE) {
        if(block == RLE_BLOCK_8x8) {
            return ENCODE_RLE_BLOCK_8x8;

        }
        else if(block == RLE_BLOCK_16x16) {
            return ENCODE_RLE_BLOCK_16x16;
        }
        else if(block == RLE_BLOCK_64x1) {
            return ENCODE_RLE_BLOCK_64x1;
        }
        else if(block == RLE_BLOCK_256x1) {
            return ENCODE_RLE_BLOCK_256x1;
        }
    }
    else if(compress_type == COMPRESS_NONE) {
        if(block == RAW_BLOCK_8x8) {
            return ENCODE_RAW_BLOCK_8x8;
        }
        else if(block == RAW_BLOCK_16x16) {
            return ENCODE_RAW_BLOCK_16x16;
        }
        else if(block == RAW_BLOCK_64x1) {
            return ENCODE_RAW_BLOCK_64x1;
        }
        else if(block == RAW_BLOCK_256x1) {
            return ENCODE_RAW_BLOCK_256x1;
        }
    }
    return block;
}

// 获取不同颜色格式占用的字节数
static uint32_t get_lut_format_byte_size(LutFormat format)
{
    switch(format) {
        case LUT_FORMAT_ARGB8888:
            return 4;
        case LUT_FORMAT_ARGB8565:
        case LUT_FORMAT_RGB888:
            return 3;
        case LUT_FORMAT_RGB565:
            return 2;
        default:
            return 0;
    }
}

// 获取每行字节数
static uint32_t get_row_size(uint32_t width, uint32_t bpp)
{
    return ((width * bpp) + 7) / 8;
}

static uint32_t get_image_storage_stride(uint32_t width, lv_color_format_t cf)
{
    if(cf == LV_COLOR_FORMAT_RGB565A8) {
        return width * 2;
    }

    return get_row_size(width, lv_color_format_get_bpp(cf));
}

static bool is_valid_rle_cf_pair(uint16_t rle_format, lv_color_format_t cf)
{
    switch(rle_format) {
        case JLDMA2D_FORMAT_ARGB8888:
            return cf == LV_COLOR_FORMAT_ARGB8888;
        case JLDMA2D_FORMAT_ARGB8565:
            return cf == LV_COLOR_FORMAT_ARGB8565;
        case JLDMA2D_FORMAT_RGB888:
            return cf == LV_COLOR_FORMAT_RGB888;
        case JLDMA2D_FORMAT_RGB565:
            return cf == LV_COLOR_FORMAT_RGB565;
        case JLDMA2D_FORMAT_L8:
            return cf == LV_COLOR_FORMAT_I8;
        case JLDMA2D_FORMAT_A8:
            return cf == LV_COLOR_FORMAT_A8;
        default:
            return false;
    }
}

static bool is_valid_etc_cf_pair(uint16_t etc_format, lv_color_format_t cf)
{
    switch(etc_format) {
        case JLDMA2D_FORMAT_ARGB8888:
            return cf == LV_COLOR_FORMAT_ARGB8888;
        case JLDMA2D_FORMAT_RGB888:
            return cf == LV_COLOR_FORMAT_RGB888;
        case JLDMA2D_FORMAT_A8:
            return cf == LV_COLOR_FORMAT_A8;
        default:
            return false;
    }
}

#endif /*LV_USE_GUI_PARSE_DYN*/
