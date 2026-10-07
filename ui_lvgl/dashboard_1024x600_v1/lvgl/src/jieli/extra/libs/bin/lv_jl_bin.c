/**
 * @file lv_jl_bin_decoder.c
 *
 */
#define BOOL_DEFINE_CONFLICT
/* #include <stdio.h> */
#include <assert.h>
#include <stddef.h>
#include "lv_jl_bin.h"
#include "lvgl.h"


/*********************
 *      INCLUDES
 *********************/
#include "../../../../draw/lv_image_decoder_private.h"
#include "../../../../draw/lv_draw_image.h"
#include "../../../../draw/lv_draw_buf.h"
#include "../../../../stdlib/lv_string.h"
#include "../../../../stdlib/lv_sprintf.h"
#include "../../../../core/lv_global.h"
#include "lv_jl_image_convert.h"

lv_image_src_t lv_get_img_src_type(const char *src);
lv_result_t lv_get_img_dsc_bin_nodata(const char *src, lv_image_dsc_t *bin_dsc);
/*********************
 *      DEFINES
 *********************/
#define LV_BIN_FILE_NAME_LEN  (15)
#define DECODER_NAME    "JL_BIN"

#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)


/**********************
 *      TYPEDEFS
 **********************/
#define JL_BIN_DSC_OFFSET_FIELD offsetof(JLBinResource, resource)
#define JL_BIN_DEFAULT_DSC_OFFSET ((uint32_t)sizeof(JLBinResource))
#define JL_BIN_DEFAULT_DATA_OFFSET ((uint32_t)(sizeof(JLBinResource) + sizeof(lv_image_dsc_t)))

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t jl_read_img_dsc_nodata(const char * src, lv_image_dsc_t * bin_dsc, uint32_t * data_offset);
static bool jl_color_format_is_supported(lv_color_format_t cf);
static void jl_release_decoded_buf(lv_draw_buf_t * decoded);


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
lv_result_t lv_get_img_dsc_bin(const char *src, lv_image_dsc_t *bin_dsc)
{
    uint32_t rn;
    lv_fs_file_t f;
    uint32_t data_offset = 0;

    if(jl_read_img_dsc_nodata(src, bin_dsc, &data_offset) != LV_RESULT_OK) {
        return LV_RESULT_INVALID;
    }

    /* 描述符和像素数据在 JL BIN 中分开存放，这里按 data_offset 一次性读入像素区。 */
    lv_fs_res_t res = lv_fs_open( &f, src, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    uint32_t img_size = bin_dsc->data_size;
    uint8_t *img_buffer = lv_malloc(img_size);
    if (img_buffer == NULL) {
        lv_fs_close( &f);
        return LV_RESULT_INVALID;
    }

    lv_fs_seek( &f, data_offset, LV_FS_SEEK_SET);
    lv_fs_read( &f, img_buffer, img_size, &rn);
    if(rn != img_size) {
        lv_free(img_buffer);
        lv_fs_close( &f);
        return LV_RESULT_INVALID;
    }

    bin_dsc->data = img_buffer;
    lv_fs_close( &f);

    return LV_RESULT_OK;
}

lv_result_t lv_get_JLBinResource(const char *src, JLBinResource *bin_res)
{
    uint32_t rn;
    const char *fn = src;
    lv_fs_file_t f;

    lv_memzero(bin_res, sizeof(*bin_res));

    /* 只读取 JL 资源头，用于快速判断压缩类型等元信息。 */
    lv_fs_res_t res = lv_fs_open( &f, fn, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    lv_fs_read( &f, bin_res, sizeof(JLBinResource), &rn);
    lv_fs_close( &f);
    if(rn != sizeof(JLBinResource)) {
        lv_memzero(bin_res, sizeof(*bin_res));
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}


lv_image_src_t lv_get_img_src_type(const char *src)
{
    uint32_t rn;
    const char *fn = src;
    lv_fs_file_t f;
    char check_head[2];

    lv_fs_res_t res = lv_fs_open( &f, fn, LV_FS_MODE_RD);

    if (res != LV_FS_RES_OK) {
         LV_LOG_ERROR("open %s error",fn);
        return LV_IMAGE_SRC_FILE;
    }
    lv_fs_read( &f, check_head, sizeof(check_head), &rn);
    lv_fs_close( &f);
    if(rn != sizeof(check_head)) {
        return LV_IMAGE_SRC_FILE;
    }

    if (check_head[0] == 0x4a && check_head[1] == 0x4c) {
        return LV_IMAGE_SRC_BIN;
    } else {
        return LV_IMAGE_SRC_FILE;
    }
}

//判断压缩类型
uint8_t lv_get_compress_type(const void *src)
{
    lv_image_src_t src_type = lv_image_src_get_type(src);
    if (src_type == LV_IMAGE_SRC_BIN) {
        JLBinResource bin_res;
        if(lv_get_JLBinResource(src, &bin_res) != LV_RESULT_OK) {
            return LV_COMPRESS_NONE;
        }
        uint8_t compress = bin_res.jl_header.res_compress;
        return compress;
    } else {
        return LV_COMPRESS_NONE;
    }
}


lv_result_t lv_jl_bin_decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header)
{
    LV_UNUSED(decoder); /*Unused*/

    const void *src = dsc->src;
    lv_image_src_t src_type = dsc->src_type;
    if (src_type == LV_IMAGE_SRC_VARIABLE) {
        return LV_RESULT_INVALID;
    } else if (src_type == LV_IMAGE_SRC_BIN) {
        lv_image_dsc_t src_dsc;
        /* info 阶段只读取描述符，不加载像素数据，避免无谓内存开销。 */
        if(lv_get_img_dsc_bin_nodata(src, &src_dsc) != LV_RESULT_OK) {
            return LV_RESULT_INVALID;
        }
        if (!jl_color_format_is_supported(src_dsc.header.cf)) {
            return LV_RESULT_INVALID;
        }
        header->w  = src_dsc.header.w;
        header->h  = src_dsc.header.h;
        header->cf = src_dsc.header.cf;
        header->stride = src_dsc.header.stride;

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
lv_result_t lv_get_img_dsc_bin_nodata(const char *src, lv_image_dsc_t *bin_dsc)
{
    return jl_read_img_dsc_nodata(src, bin_dsc, NULL);
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
    lv_color_format_t cf = 0;

    if(dsc->src_type != LV_IMAGE_SRC_BIN) {
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_t * decoded = NULL;
    lv_image_dsc_t tmp = {0};
    if(lv_get_img_dsc_bin((const char *)dsc->src, &tmp) != LV_RESULT_OK) {
        return LV_RESULT_INVALID;
    }
    cf = tmp.header.cf;
    if(!jl_color_format_is_supported(cf) || lv_color_format_get_bpp(cf) == 0) {
        lv_free((void *)tmp.data);
        return LV_RESULT_INVALID;
    }

    uint32_t min_stride = (tmp.header.w * lv_color_format_get_bpp(cf) + 7) >> 3;
    uint32_t stride_expect = lv_draw_buf_width_to_stride(tmp.header.w, cf);
    if(tmp.header.stride == 0) {
        tmp.header.stride = dsc->header.stride;
    }
    else if(tmp.header.stride < min_stride) {
        LV_LOG_WARN("JL BIN image stride too small: %" LV_PRIu32 ", minimum: %" LV_PRIu32,
                    tmp.header.stride, min_stride);
        lv_free((void *)tmp.data);
        return LV_RESULT_INVALID;
    }

    if(tmp.header.stride != stride_expect) {
        LV_LOG_WARN("JL BIN image stride mismatch: resource=%" LV_PRIu32 ", lvgl=%" LV_PRIu32,
                    tmp.header.stride, stride_expect);
    }

    if(lv_jl_image_needs_convert(cf)) {
        /* DRAW_SW 不支持的格式统一走公共 helper，保持与动态资源一致。 */
        decoded = lv_jl_image_convert(&tmp);
        lv_free((void *)tmp.data);
        tmp.data = NULL;
        if(decoded == NULL) {
            return LV_RESULT_INVALID;
        }
    }
    else
    {
        decoded = lv_malloc_zeroed(sizeof(lv_draw_buf_t));
        if(decoded == NULL) {
            lv_free((void *)tmp.data);
            return LV_RESULT_INVALID;
        }

        //lv_draw_buf_from_image(decoded, &tmp);
        /* 普通格式无需转换，draw_buf 直接持有 lv_get_img_dsc_bin 分配的像素数据。 */
        decoded->header.w = tmp.header.w;
        decoded->header.h = tmp.header.h;
        decoded->header.cf = tmp.header.cf;
        decoded->header.stride = tmp.header.stride;
        decoded->header.magic = LV_IMAGE_HEADER_MAGIC;
        decoded->header.flags = tmp.header.flags | LV_IMAGE_FLAGS_MODIFIABLE;
        decoded->data = (void *)tmp.data;
        decoded->unaligned_data = (void *)tmp.data;
        decoded->handlers = lv_draw_buf_get_handlers();
        decoded->data_size = tmp.data_size;
    }

    lv_draw_buf_t * adjusted = lv_image_decoder_post_process(dsc, decoded);
    if(adjusted == NULL) {
        jl_release_decoded_buf(decoded);
        return LV_RESULT_INVALID;
    }
    if(adjusted != decoded) {
        jl_release_decoded_buf(decoded);
        decoded = adjusted;
    }

    dsc->decoded = decoded;

    if(dsc->args.no_cache) {
        return LV_RESULT_OK;
    }
    /*If the image cache is disabled, just return the decoded image*/
    if(!lv_image_cache_is_enabled()) {
        return LV_RESULT_OK;
    }

    /*Add the decoded image to the cache*/
    lv_image_cache_data_t search_key;
    search_key.src_type = dsc->src_type;
    search_key.src = dsc->src;
    search_key.slot.size = decoded->data_size;
    /* 入缓存后由 image_cache_free_cb 统一销毁 draw_buf 及其像素数据。 */
    decoded->header.flags |= LV_IMAGE_FLAGS_ALLOCATED;

    lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);
    if(entry == NULL) {
        if(decoded->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
            lv_draw_buf_destroy(decoded);
        }
        else {
            lv_free((void *)decoded->data);
            lv_free(decoded);
        }
        return LV_RESULT_INVALID;
    }

    dsc->cache_entry = entry;
    return LV_RESULT_OK;
}

void lv_jl_bin_decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder); /*Unused*/
    if(dsc->args.no_cache || !lv_image_cache_is_enabled()) {
        if(dsc->decoded) {
            lv_draw_buf_t * decoded_tmp = (lv_draw_buf_t *)dsc->decoded;
            if(decoded_tmp->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
                lv_draw_buf_destroy(decoded_tmp);
            }
            else {
                if(decoded_tmp->data) {
                    lv_free((void *)decoded_tmp->data);
                    decoded_tmp->data = NULL;
                    decoded_tmp->data_size = 0;
                }
                lv_free((void *)dsc->decoded);
            }
        }
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

static lv_result_t jl_read_img_dsc_nodata(const char * src, lv_image_dsc_t * bin_dsc, uint32_t * data_offset)
{
    uint32_t rn;
    uint32_t dsc_offset = JL_BIN_DEFAULT_DSC_OFFSET;
    uint32_t data_offset_tmp = JL_BIN_DEFAULT_DATA_OFFSET;
    lv_fs_file_t f;

    lv_memzero(bin_dsc, sizeof(*bin_dsc));

    lv_fs_res_t res = lv_fs_open(&f, src, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    /* resource 字段在文件中保存 lv_image_dsc_t 的偏移；缺省则使用紧凑布局。 */
    lv_fs_seek(&f, JL_BIN_DSC_OFFSET_FIELD, LV_FS_SEEK_SET);
    lv_fs_read(&f, &dsc_offset, sizeof(dsc_offset), &rn);
    if(rn != sizeof(dsc_offset) || dsc_offset == 0) {
        dsc_offset = JL_BIN_DEFAULT_DSC_OFFSET;
    }

    lv_fs_seek(&f, dsc_offset, LV_FS_SEEK_SET);
    lv_fs_read(&f, &bin_dsc->header, sizeof(lv_image_header_t), &rn);
    if(rn != sizeof(lv_image_header_t)) {
        lv_fs_close(&f);
        return LV_RESULT_INVALID;
    }

    lv_fs_read(&f, &bin_dsc->data_size, sizeof(bin_dsc->data_size), &rn);
    if(rn != sizeof(bin_dsc->data_size)) {
        lv_fs_close(&f);
        return LV_RESULT_INVALID;
    }

    lv_fs_read(&f, &data_offset_tmp, sizeof(data_offset_tmp), &rn);
    if(rn != sizeof(data_offset_tmp) || data_offset_tmp == 0) {
        /* 兼容旧资源：描述符后未显式记录 data_offset 时，按连续布局计算。 */
        data_offset_tmp = dsc_offset + sizeof(lv_image_header_t) + sizeof(uint32_t) + sizeof(uint32_t);
    }

    lv_fs_close(&f);

    if(data_offset) {
        *data_offset = data_offset_tmp;
    }

    return LV_RESULT_OK;
}

static bool jl_color_format_is_supported(lv_color_format_t cf)
{
    /* 白名单只包含当前解码路径能直接使用或能转换的 LVGL v9 格式。 */
    return LV_COLOR_FORMAT_IS_INDEXED(cf)
           || LV_COLOR_FORMAT_IS_ALPHA_ONLY(cf)
           || cf == LV_COLOR_FORMAT_L8
           || cf == LV_COLOR_FORMAT_AL88
           || cf == LV_COLOR_FORMAT_ARGB8888
           || cf == LV_COLOR_FORMAT_XRGB8888
           || cf == LV_COLOR_FORMAT_RGB888
           || cf == LV_COLOR_FORMAT_RGB565
           || cf == LV_COLOR_FORMAT_RGB565_SWAPPED
           || cf == LV_COLOR_FORMAT_ARGB8565
           || cf == LV_COLOR_FORMAT_RGB565A8;
}

static void jl_release_decoded_buf(lv_draw_buf_t * decoded)
{
    if(decoded == NULL) {
        return;
    }

    if(decoded->header.flags & LV_IMAGE_FLAGS_ALLOCATED) {
        lv_draw_buf_destroy(decoded);
    }
    else {
        lv_free((void *)decoded->data);
        lv_free(decoded);
    }
}



lv_result_t lv_get_type_bin(const char *src, JL_get_type *type)
{
    uint32_t rn;
    const char * fn = src;
    lv_fs_file_t f;

    lv_memzero(type, sizeof(*type));

    lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);

    if(res != LV_FS_RES_OK) {
		LV_LOG_ERROR("open %s error",fn);
        return LV_RESULT_INVALID;
    }
    lv_fs_read(&f, type, sizeof(JL_get_type), &rn);
    lv_fs_close(&f);
    if(rn != sizeof(JL_get_type)) {
        lv_memzero(type, sizeof(*type));
        return LV_RESULT_INVALID;
    }
    return LV_RESULT_OK;
}
