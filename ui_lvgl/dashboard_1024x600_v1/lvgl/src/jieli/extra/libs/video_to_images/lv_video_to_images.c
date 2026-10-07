/**
 * @file lv_video_to_images.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_video_to_images.h"
#include "lz4_assets.h"
#include "diff_image_to_video_types.h"

#if LV_USE_LZ4_INTERNAL == 0
    #error "lv_video_to_images needs LV_USE_LZ4_INTERNAL to be enabled. Please enable it in lv_conf.h"
#endif

/*********************
 *      DEFINES
 *********************/

#define MEMCPY_TINY_MAX_LENGTH   128
#define IS_ADDRESS_ALIGN_4(addr) !((((size_t)(addr)) & 0x3) | 0x0)



/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static diff_image_ret_t read_video_frame_info(lv_fs_file_t * fp, uint32_t * w,
                                              uint32_t * h, uint32_t * channels, uint32_t * line_length,
                                              uint32_t * frame_number_max,
                                              uint32_t * gif_delay,
                                              uint32_t * format,
                                              uint32_t * logical_width,
                                              uint32_t * logical_height,
                                              uint32_t * logical_line_length,
                                              uint16_t * flags, uint32_t * orientation,
                                              uint32_t * table_start_point, uint32_t * table_length);
static uint16_t * memcpy16(uint16_t * dst, uint16_t * src, uint32_t size);
static uint32_t * memcpy32(uint32_t * dst, uint32_t * src, uint32_t size);
static void * memcpy_tiny(void * dst, const void * src, uint32_t len);
static diff_image_ret_t create_image_data_for_align32bit(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                         uint32_t image_data_length, unsigned char * image_data, uint32_t line_length);
static diff_image_ret_t create_image_data_for_align16bit(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                         uint32_t image_data_length, unsigned char * image_data, uint32_t line_length);
static diff_image_ret_t create_image_data_for_noalign(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                      uint32_t image_data_length, unsigned char * image_data,
                                                      uint32_t image_width, uint32_t channels, uint32_t line_length);
static diff_image_ret_t get_lz4_data_to_diff_image_data(unsigned char * lz4_data, uint32_t lz4_length,
                                                        uint32_t image_data_length, unsigned char * image_data, uint32_t image_width,
                                                        uint32_t channels, uint32_t line_length, unsigned char * max_data_decompress);
static diff_image_ret_t read_video_info_and_frame_info(lv_fs_file_t * fp,  video_info_t * video_info,
                                                       frame_info_t * frame_info_list, uint32_t frame_number);
static diff_image_ret_t get_first_frame_data(lv_fs_file_t * fp, unsigned char * image_data);
static diff_image_ret_t read_image_data(lv_fs_file_t * fp, uint32_t frame_number, unsigned char * last_image_data,
                                        uint32_t line_length, uint32_t width, uint32_t height,
                                        uint32_t channels, unsigned char ** max_data_decompress);
static diff_image_ret_t read_frame_image(lv_fs_file_t * fp,  video_info_t * video_info, frame_info_t * frame_info,
                                         unsigned char * last_image_data, uint32_t line_length,
                                         unsigned char * max_data_decompress);
static uint32_t get_max_data_decompress_length(lv_fs_file_t * fp, uint32_t frame_number);
static void cleanup_file(lv_fs_file_t * fp, lv_image_dsc_t *** dsc_array, uint32_t count,
                         unsigned char * image_data, unsigned char * max_data_decompress);
static lv_result_t set_frame_descriptor(lv_image_dsc_t * img_dsc, uint32_t w, uint32_t h, unsigned char * data,
                                     uint32_t channels, lv_color_format_t cf);



/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

#define CREATE_IMAGE_DATA_FOR_ALIG(data_for_image_diff_data_list_lz4_dec, image_data_length, image_data, line_length, is_su, memcpy_fun, ptr) {                          \
        uint32_t y = 0;                                                                                                                                                                            \
        uint32_t data_length = 0;                                                                                                                                                                  \
        uint32_t width_length = 0;                                                                                                                                                                 \
        image_diff_rect_t* diff_rect = NULL;                                                                                                                                                       \
        uint32_t image_data_list_point = 0;                                                                                                                                                        \
        do {                                                                                                                                                                                       \
            uint32_t image_x_point = 0;                                                                                                                                                              \
            uint32_t image_y_point = 0;                                                                                                                                                              \
            uint32_t data_width_length = 0;                                                                                                                                                          \
            uint32_t tmp_image_data_length = 0;                                                                                                                                                      \
            diff_rect = (image_diff_rect_t*)(data_for_image_diff_data_list_lz4_dec + image_data_list_point);                                                                                         \
            image_data_list_point += sizeof(image_diff_rect_t);                                                                                                                                      \
            \
            width_length = line_length;                                                                                                                                                              \
            image_x_point = diff_rect->x << (sizeof(ptr) / 2);                                                                                                                                       \
            image_y_point = diff_rect->y * width_length;                                                                                                                                             \
            data_width_length = diff_rect->width << (sizeof(ptr) / 2);                                                                                                                               \
            data_length = diff_rect->line_length * diff_rect->height;                                                                                                                                \
            \
            for (y = 0; y < diff_rect->height; y++) {                                                                                                                                                \
                if (data_width_length < MEMCPY_TINY_MAX_LENGTH) {                                                                                                                                      \
                    void* dst = memcpy_tiny(image_data + image_x_point + image_y_point, data_for_image_diff_data_list_lz4_dec + image_data_list_point + tmp_image_data_length, data_width_length);       \
                    if (dst == NULL) {                                                                                                                                                                   \
                        memcpy_fun((ptr *)(image_data + image_x_point + image_y_point), (ptr *)(data_for_image_diff_data_list_lz4_dec + image_data_list_point + tmp_image_data_length), diff_rect->width); \
                    }                                                                                                                                                                                    \
                } else {                                                                                                                                                                               \
                    memcpy_fun((ptr *)(image_data + image_x_point + image_y_point), (ptr *)(data_for_image_diff_data_list_lz4_dec + image_data_list_point + tmp_image_data_length), diff_rect->width);   \
                }                                                                                                                                                                                      \
                image_y_point += width_length;                                                                                                                                                         \
                tmp_image_data_length += diff_rect->line_length;                                                                                                                                       \
            }                                                                                                                                                                                        \
            image_data_list_point += data_length;                                                                                                                                                    \
            is_su = DIFF_IMAGE_RET_OK;                                                                                                                                                               \
        } while (image_data_list_point < image_data_length);                                                                                                                                       \
    }

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_result_t lv_video_to_images_convert(const char * src, lv_image_dsc_t *** dsc_array, uint32_t * pic_count)
{
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    // Read the video frame info
    uint32_t w = 0, h = 0, channels = 0, line_length = 0, frame_number_max = 0, gif_delay = 0;
    uint32_t format;
    uint32_t logical_width = 0, logical_height = 0, logical_line_length = 0;
    uint16_t flags = 0;
    uint32_t orientation = 0;
    uint32_t table_start_point = 0, table_length = 0;
    diff_image_ret_t ret = read_video_frame_info(&file, &w, &h, &channels, &line_length,
                                                 &frame_number_max, &gif_delay, &format, &logical_width,
                                                 &logical_height, &logical_line_length, &flags, &orientation,
                                                 &table_start_point, &table_length);
    if(ret != DIFF_IMAGE_RET_OK) {
        cleanup_file(&file, NULL, 0, NULL, NULL);
        return LV_RESULT_INVALID;
    }

    *dsc_array = lv_malloc(sizeof(lv_image_dsc_t *) * frame_number_max);
    if(*dsc_array == NULL) {
        cleanup_file(&file, NULL, 0, NULL, NULL);
        return LV_RESULT_INVALID;
    }

    // Allocate memory for each frame descriptor
    for(uint32_t i = 0; i < frame_number_max; i++) {
        (*dsc_array)[i] = lv_malloc(sizeof(lv_image_dsc_t));
        if((*dsc_array)[i] == NULL) {
            cleanup_file(&file, dsc_array, i, NULL, NULL);
            return LV_RESULT_INVALID;
        }
    }

    // Allocate memory for the first frame image data
    uint32_t image_data_length = w * h * channels + table_length;
    unsigned char * image_data = lv_malloc(image_data_length);
    if(image_data == NULL) {
        cleanup_file(&file, dsc_array, frame_number_max, NULL, NULL);
        return LV_RESULT_INVALID;
    }

    ret = get_first_frame_data(&file, image_data);
    if(ret != DIFF_IMAGE_RET_OK) {
        cleanup_file(&file, dsc_array, frame_number_max, image_data, NULL);
        return LV_RESULT_INVALID;
    }
    set_frame_descriptor((*dsc_array)[0], w, h, image_data, channels, format);

    //get the max_data_decompress_length
    uint32_t max_data_decompress_length = get_max_data_decompress_length(&file, frame_number_max);

    // Create the other frame images
    unsigned char * max_data_decompress = lv_malloc(max_data_decompress_length);
    if(max_data_decompress == NULL) {
        cleanup_file(&file, dsc_array, frame_number_max, image_data, NULL);
        return LV_RESULT_INVALID;
    }
    for(uint32_t i = 1; i < frame_number_max; i++) {
        // Allocate memory for the frame data
        ret = read_image_data(&file, i, image_data, line_length, w, h, channels, &max_data_decompress);
        if(ret != DIFF_IMAGE_RET_OK) {
            cleanup_file(&file, dsc_array, frame_number_max, image_data, max_data_decompress);
            return LV_RESULT_INVALID;
        }

        set_frame_descriptor((*dsc_array)[i], w, h, image_data, channels, format);
    }

    *pic_count = frame_number_max;
    cleanup_file(&file, NULL, 0, image_data, max_data_decompress);
    return LV_RESULT_OK;
}

lv_result_t lv_video_get_image(const char * src, lv_image_dsc_t * image, uint32_t frame_count, uint32_t cur_frame_number,
                            uint32_t max_data_decompress_length)
{
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    // Read the video frame info
    uint32_t w = 0, h = 0, channels = 0, line_length = 0, frame_number_max = 0, gif_delay = 0;
    uint32_t format;
    uint32_t logical_width = 0, logical_height = 0, logical_line_length = 0;
    uint16_t flags = 0;
    uint32_t orientation = 0;
    uint32_t table_start_point = 0, table_length = 0;
    diff_image_ret_t ret = read_video_frame_info(&file, &w, &h, &channels, &line_length,
                                                 &frame_number_max, &gif_delay, &format, &logical_width,
                                                 &logical_height, &logical_line_length, &flags, &orientation,
                                                 &table_start_point, &table_length);
    if(ret != DIFF_IMAGE_RET_OK) {
        lv_fs_close(&file);
        return LV_RESULT_INVALID;
    }

    if(cur_frame_number == 0 || cur_frame_number >= frame_number_max) {
        // Allocate memory for the first frame image data
        unsigned char * image_data = lv_malloc(w * h * channels + table_length);
        if(image_data == NULL) {
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        ret = get_first_frame_data(&file, image_data);
        if(ret != DIFF_IMAGE_RET_OK) {
            lv_free(image_data);
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        if(image && image->data && image->header.w == w && image->header.h == h && image->header.cf == format) {
            lv_memcpy((unsigned char *)image->data, image_data, image->data_size);
        }
        else {
            if(image && image->data) {
                lv_free((unsigned char *)image->data);
            }
            set_frame_descriptor(image, w, h, image_data, channels, format);
        }
        lv_free(image_data);
    }
    else {
        unsigned char * max_data_decompress = lv_malloc(max_data_decompress_length);
        if(max_data_decompress == NULL) {
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }

        ret = read_image_data(&file, cur_frame_number, (unsigned char *)image->data + table_length, line_length, w, h, channels,
                              &max_data_decompress);
        if(ret != DIFF_IMAGE_RET_OK) {
            lv_free(max_data_decompress);
            lv_fs_close(&file);
            return LV_RESULT_INVALID;
        }
        lv_free(max_data_decompress);
    }

    lv_fs_close(&file);
    return LV_RESULT_OK;
}

lv_result_t lv_video_get_info(const char * src, uint32_t * frame_count, uint32_t * max_data_decompress)
{
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        return LV_RESULT_INVALID;
    }

    // Read the video frame info
    uint32_t w = 0, h = 0, channels = 0, line_length = 0, frame_number_max = 0, gif_delay = 0;
    uint32_t format;
    uint32_t logical_width = 0, logical_height = 0, logical_line_length = 0;
    uint16_t flags = 0;
    uint32_t orientation = 0;
    uint32_t table_start_point = 0, table_length = 0;
    diff_image_ret_t ret = read_video_frame_info(&file, &w, &h, &channels, &line_length,
                                                 &frame_number_max, &gif_delay, &format, &logical_width,
                                                 &logical_height, &logical_line_length, &flags, &orientation,
                                                 &table_start_point, &table_length);
    if(ret != DIFF_IMAGE_RET_OK) {
        lv_fs_close(&file);
        return LV_RESULT_INVALID;
    }

    *frame_count = frame_number_max;
    *max_data_decompress = get_max_data_decompress_length(&file, frame_number_max);

    lv_fs_close(&file);
    return LV_RESULT_OK;
}

lv_result_t lv_video_to_images_free(lv_image_dsc_t *** dsc_array, uint32_t pic_count)
{
    if(dsc_array == NULL || *dsc_array == NULL) {
        return LV_RESULT_INVALID;
    }

    for(uint32_t i = 0; i < pic_count; i++) {
        if((*dsc_array)[i] != NULL) {
            lv_free((void *)(*dsc_array)[i]->data);
            lv_free((*dsc_array)[i]);
            (*dsc_array)[i] = NULL;
        }
    }

    lv_free(*dsc_array);
    *dsc_array = NULL;

    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static diff_image_ret_t read_video_frame_info(lv_fs_file_t * fp, uint32_t * w,
                                              uint32_t * h, uint32_t * channels, uint32_t * line_length,
                                              uint32_t * frame_number_max,
                                              uint32_t * gif_delay,
                                              uint32_t * format,
                                              uint32_t * logical_width,
                                              uint32_t * logical_height,
                                              uint32_t * logical_line_length,
                                              uint16_t * flags, uint32_t * orientation,
                                              uint32_t * table_start_point, uint32_t * table_length)
{
    video_info_t video_info;
    uint32_t frame_image_type;
    lv_fs_res_t res = lv_fs_seek(fp, 0, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    res = lv_fs_read(fp, &video_info, sizeof(video_info_t), NULL);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    diff_image_video_info_from_big_endian(&video_info);

    if(lv_memcmp(video_info.video_info_head, s_video_info_head, sizeof(s_video_info_head)) != 0 ||
       lv_memcmp(video_info.video_info_head + sizeof(s_video_info_head), VIDEO_INFO_HEAD_STRING,
              sizeof(VIDEO_INFO_HEAD_STRING)) != 0) {
        return DIFF_IMAGE_RET_UNKNOWN;
    }

    if(video_info.version != VIDEO_INFO_VERSION) {
        if(video_info.version < VIDEO_INFO_MIN_VERSION) {
            return DIFF_IMAGE_RET_ERROR_VERSION;
        }
    }

    *w = video_info.width;
    *h = video_info.height;
    *channels = video_info.channels;
    *line_length = video_info.line_length;
    *frame_number_max = video_info.frame_number;
    *flags = video_info.flags;
    *gif_delay = video_info.delays;
    *orientation = video_info.orientation;
    *logical_width = video_info.logical_width;
    *logical_height = video_info.logical_height;
    *logical_line_length = video_info.logical_line_length;
    *format = video_info.frame_image_type;
    *table_start_point = video_info.table_start_point;
    *table_length = video_info.table_length;
    return DIFF_IMAGE_RET_OK;
}

/**
 * 拷贝数据2字节
 * @param dst 目标
 * @param src 源
 * @param size 个数
 * @return uint16_t* 返回设置好的buff
 */
static uint16_t * memcpy16(uint16_t * dst, uint16_t * src, uint32_t size)
{
    uint16_t * d = dst;
    uint16_t * s = src;

    if(!(dst != NULL && src != NULL)) {
        return NULL;
    }

    while(size-- > 0) {
        *d++ = *s++;
    }

    return dst;
}

/**
 * 拷贝数据4字节
 * @param dst 目标
 * @param src 源
 * @param size 个数
 * @return uint32_t* 返回设置好的buff
 */
static uint32_t * memcpy32(uint32_t * dst, uint32_t * src, uint32_t size)
{
    uint32_t * d = dst;
    uint32_t * s = src;

    if(!(dst != NULL && src != NULL)) {
        return NULL;
    }

    while(size-- > 0) {
        *d++ = *s++;
    }

    return dst;
}

static void * memcpy_tiny(void * dst, const void * src, uint32_t len)
{
    if(len < MEMCPY_TINY_MAX_LENGTH && IS_ADDRESS_ALIGN_4(dst) && IS_ADDRESS_ALIGN_4(src)) {
        register unsigned char * dd = (unsigned char *)dst + len;
        register const unsigned char * ss = (const unsigned char *)src + len;
        switch(len) {
            case 128:
                *((int *)(dd - 128)) = *((int *)(ss - 128));
            case 124:
                *((int *)(dd - 124)) = *((int *)(ss - 124));
            case 120:
                *((int *)(dd - 120)) = *((int *)(ss - 120));
            case 116:
                *((int *)(dd - 116)) = *((int *)(ss - 116));
            case 112:
                *((int *)(dd - 112)) = *((int *)(ss - 112));
            case 108:
                *((int *)(dd - 108)) = *((int *)(ss - 108));
            case 104:
                *((int *)(dd - 104)) = *((int *)(ss - 104));
            case 100:
                *((int *)(dd - 100)) = *((int *)(ss - 100));
            case 96:
                *((int *)(dd - 96)) = *((int *)(ss - 96));
            case 92:
                *((int *)(dd - 92)) = *((int *)(ss - 92));
            case 88:
                *((int *)(dd - 88)) = *((int *)(ss - 88));
            case 84:
                *((int *)(dd - 84)) = *((int *)(ss - 84));
            case 80:
                *((int *)(dd - 80)) = *((int *)(ss - 80));
            case 76:
                *((int *)(dd - 76)) = *((int *)(ss - 76));
            case 72:
                *((int *)(dd - 72)) = *((int *)(ss - 72));
            case 68:
                *((int *)(dd - 68)) = *((int *)(ss - 68));
            case 64:
                *((int *)(dd - 64)) = *((int *)(ss - 64));
            case 60:
                *((int *)(dd - 60)) = *((int *)(ss - 60));
            case 56:
                *((int *)(dd - 56)) = *((int *)(ss - 56));
            case 52:
                *((int *)(dd - 52)) = *((int *)(ss - 52));
            case 48:
                *((int *)(dd - 48)) = *((int *)(ss - 48));
            case 44:
                *((int *)(dd - 44)) = *((int *)(ss - 44));
            case 40:
                *((int *)(dd - 40)) = *((int *)(ss - 40));
            case 36:
                *((int *)(dd - 36)) = *((int *)(ss - 36));
            case 32:
                *((int *)(dd - 32)) = *((int *)(ss - 32));
            case 28:
                *((int *)(dd - 28)) = *((int *)(ss - 28));
            case 24:
                *((int *)(dd - 24)) = *((int *)(ss - 24));
            case 20:
                *((int *)(dd - 20)) = *((int *)(ss - 20));
            case 16:
                *((int *)(dd - 16)) = *((int *)(ss - 16));
            case 12:
                *((int *)(dd - 12)) = *((int *)(ss - 12));
            case  8:
                *((int *)(dd - 8)) = *((int *)(ss - 8));
            case  4:
                *((int *)(dd - 4)) = *((int *)(ss - 4));
                break;
            case 127:
                *((int *)(dd - 127)) = *((int *)(ss - 127));
            case 123:
                *((int *)(dd - 123)) = *((int *)(ss - 123));
            case 119:
                *((int *)(dd - 119)) = *((int *)(ss - 119));
            case 115:
                *((int *)(dd - 115)) = *((int *)(ss - 115));
            case 111:
                *((int *)(dd - 111)) = *((int *)(ss - 111));
            case 107:
                *((int *)(dd - 107)) = *((int *)(ss - 107));
            case 103:
                *((int *)(dd - 103)) = *((int *)(ss - 103));
            case 99:
                *((int *)(dd - 99)) = *((int *)(ss - 99));
            case 95:
                *((int *)(dd - 95)) = *((int *)(ss - 95));
            case 91:
                *((int *)(dd - 91)) = *((int *)(ss - 91));
            case 87:
                *((int *)(dd - 87)) = *((int *)(ss - 87));
            case 83:
                *((int *)(dd - 83)) = *((int *)(ss - 83));
            case 79:
                *((int *)(dd - 79)) = *((int *)(ss - 79));
            case 75:
                *((int *)(dd - 75)) = *((int *)(ss - 75));
            case 71:
                *((int *)(dd - 71)) = *((int *)(ss - 71));
            case 67:
                *((int *)(dd - 67)) = *((int *)(ss - 67));
            case 63:
                *((int *)(dd - 63)) = *((int *)(ss - 63));
            case 59:
                *((int *)(dd - 59)) = *((int *)(ss - 59));
            case 55:
                *((int *)(dd - 55)) = *((int *)(ss - 55));
            case 51:
                *((int *)(dd - 51)) = *((int *)(ss - 51));
            case 47:
                *((int *)(dd - 47)) = *((int *)(ss - 47));
            case 43:
                *((int *)(dd - 43)) = *((int *)(ss - 43));
            case 39:
                *((int *)(dd - 39)) = *((int *)(ss - 39));
            case 35:
                *((int *)(dd - 35)) = *((int *)(ss - 35));
            case 31:
                *((int *)(dd - 31)) = *((int *)(ss - 31));
            case 27:
                *((int *)(dd - 27)) = *((int *)(ss - 27));
            case 23:
                *((int *)(dd - 23)) = *((int *)(ss - 23));
            case 19:
                *((int *)(dd - 19)) = *((int *)(ss - 19));
            case 15:
                *((int *)(dd - 15)) = *((int *)(ss - 15));
            case 11:
                *((int *)(dd - 11)) = *((int *)(ss - 11));
            case  7:
                *((int *)(dd - 7)) = *((int *)(ss - 7));
            case  3:
                *((short *)(dd - 3)) = *((short *)(ss - 3));
                dd[-1] = ss[-1];
                break;
            case 126:
                *((int *)(dd - 126)) = *((int *)(ss - 126));
            case 122:
                *((int *)(dd - 122)) = *((int *)(ss - 122));
            case 118:
                *((int *)(dd - 118)) = *((int *)(ss - 118));
            case 114:
                *((int *)(dd - 114)) = *((int *)(ss - 114));
            case 110:
                *((int *)(dd - 110)) = *((int *)(ss - 110));
            case 106:
                *((int *)(dd - 106)) = *((int *)(ss - 106));
            case 102:
                *((int *)(dd - 102)) = *((int *)(ss - 102));
            case 98:
                *((int *)(dd - 98)) = *((int *)(ss - 98));
            case 94:
                *((int *)(dd - 94)) = *((int *)(ss - 94));
            case 90:
                *((int *)(dd - 90)) = *((int *)(ss - 90));
            case 86:
                *((int *)(dd - 86)) = *((int *)(ss - 86));
            case 82:
                *((int *)(dd - 82)) = *((int *)(ss - 82));
            case 78:
                *((int *)(dd - 78)) = *((int *)(ss - 78));
            case 74:
                *((int *)(dd - 74)) = *((int *)(ss - 74));
            case 70:
                *((int *)(dd - 70)) = *((int *)(ss - 70));
            case 66:
                *((int *)(dd - 66)) = *((int *)(ss - 66));
            case 62:
                *((int *)(dd - 62)) = *((int *)(ss - 62));
            case 58:
                *((int *)(dd - 58)) = *((int *)(ss - 58));
            case 54:
                *((int *)(dd - 54)) = *((int *)(ss - 54));
            case 50:
                *((int *)(dd - 50)) = *((int *)(ss - 50));
            case 46:
                *((int *)(dd - 46)) = *((int *)(ss - 46));
            case 42:
                *((int *)(dd - 42)) = *((int *)(ss - 42));
            case 38:
                *((int *)(dd - 38)) = *((int *)(ss - 38));
            case 34:
                *((int *)(dd - 34)) = *((int *)(ss - 34));
            case 30:
                *((int *)(dd - 30)) = *((int *)(ss - 30));
            case 26:
                *((int *)(dd - 26)) = *((int *)(ss - 26));
            case 22:
                *((int *)(dd - 22)) = *((int *)(ss - 22));
            case 18:
                *((int *)(dd - 18)) = *((int *)(ss - 18));
            case 14:
                *((int *)(dd - 14)) = *((int *)(ss - 14));
            case 10:
                *((int *)(dd - 10)) = *((int *)(ss - 10));
            case  6:
                *((int *)(dd - 6)) = *((int *)(ss - 6));
            case  2:
                *((short *)(dd - 2)) = *((short *)(ss - 2));
                break;
            case 125:
                *((int *)(dd - 125)) = *((int *)(ss - 125));
            case 121:
                *((int *)(dd - 121)) = *((int *)(ss - 121));
            case 117:
                *((int *)(dd - 117)) = *((int *)(ss - 117));
            case 113:
                *((int *)(dd - 113)) = *((int *)(ss - 113));
            case 109:
                *((int *)(dd - 109)) = *((int *)(ss - 109));
            case 105:
                *((int *)(dd - 105)) = *((int *)(ss - 105));
            case 101:
                *((int *)(dd - 101)) = *((int *)(ss - 101));
            case 97:
                *((int *)(dd - 97)) = *((int *)(ss - 97));
            case 93:
                *((int *)(dd - 93)) = *((int *)(ss - 93));
            case 89:
                *((int *)(dd - 89)) = *((int *)(ss - 89));
            case 85:
                *((int *)(dd - 85)) = *((int *)(ss - 85));
            case 81:
                *((int *)(dd - 81)) = *((int *)(ss - 81));
            case 77:
                *((int *)(dd - 77)) = *((int *)(ss - 77));
            case 73:
                *((int *)(dd - 73)) = *((int *)(ss - 73));
            case 69:
                *((int *)(dd - 69)) = *((int *)(ss - 69));
            case 65:
                *((int *)(dd - 65)) = *((int *)(ss - 65));
            case 61:
                *((int *)(dd - 61)) = *((int *)(ss - 61));
            case 57:
                *((int *)(dd - 57)) = *((int *)(ss - 57));
            case 53:
                *((int *)(dd - 53)) = *((int *)(ss - 53));
            case 49:
                *((int *)(dd - 49)) = *((int *)(ss - 49));
            case 45:
                *((int *)(dd - 45)) = *((int *)(ss - 45));
            case 41:
                *((int *)(dd - 41)) = *((int *)(ss - 41));
            case 37:
                *((int *)(dd - 37)) = *((int *)(ss - 37));
            case 33:
                *((int *)(dd - 33)) = *((int *)(ss - 33));
            case 29:
                *((int *)(dd - 29)) = *((int *)(ss - 29));
            case 25:
                *((int *)(dd - 25)) = *((int *)(ss - 25));
            case 21:
                *((int *)(dd - 21)) = *((int *)(ss - 21));
            case 17:
                *((int *)(dd - 17)) = *((int *)(ss - 17));
            case 13:
                *((int *)(dd - 13)) = *((int *)(ss - 13));
            case  9:
                *((int *)(dd - 9)) = *((int *)(ss - 9));
            case  5:
                *((int *)(dd - 5)) = *((int *)(ss - 5));
            case  1:
                dd[-1] = ss[-1];
                break;
            case 0:
            default:
                break;
        }
        return dd;
    }
    return NULL;
}

static diff_image_ret_t create_image_data_for_align32bit(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                         uint32_t image_data_length, unsigned char * image_data, uint32_t line_length)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;
    CREATE_IMAGE_DATA_FOR_ALIG(data_for_image_diff_data_list_lz4_dec, image_data_length, image_data,
                               line_length, is_su, memcpy32, uint32_t);
    return is_su;
}

static diff_image_ret_t create_image_data_for_align16bit(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                         uint32_t image_data_length, unsigned char * image_data, uint32_t line_length)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;
    CREATE_IMAGE_DATA_FOR_ALIG(data_for_image_diff_data_list_lz4_dec, image_data_length, image_data,
                               line_length, is_su, memcpy16, uint16_t);
    return is_su;
}

static diff_image_ret_t create_image_data_for_noalign(unsigned char * data_for_image_diff_data_list_lz4_dec,
                                                      uint32_t image_data_length, unsigned char * image_data,
                                                      uint32_t image_width, uint32_t channels, uint32_t line_length)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;
    uint32_t y = 0;
    uint32_t data_length = 0;
    uint32_t width_length = 0;
    uint32_t data_width_length = 0;

    image_diff_rect_t * diff_rect = NULL;
    uint32_t image_data_list_point = 0;

    do {
        uint32_t image_x_point = 0;
        uint32_t image_y_point = 0;
        uint32_t tmp_image_data_length = 0;

        diff_rect = (image_diff_rect_t *)(data_for_image_diff_data_list_lz4_dec + image_data_list_point);
        image_data_list_point += sizeof(image_diff_rect_t);

        data_length = diff_rect->height * diff_rect->line_length;

        width_length = line_length;
        image_x_point = diff_rect->x * channels;
        image_y_point = diff_rect->y * width_length;
        data_width_length = channels * diff_rect->width;

        for(y = 0; y < diff_rect->height; y++) {
            lv_memcpy(image_data + image_x_point + image_y_point,
                      data_for_image_diff_data_list_lz4_dec + image_data_list_point + tmp_image_data_length, data_width_length);
            image_y_point += width_length;
            tmp_image_data_length += diff_rect->line_length;
        }

        image_data_list_point += data_length;

        is_su = DIFF_IMAGE_RET_OK;
    } while(image_data_list_point < image_data_length);
    return is_su;
}

static diff_image_ret_t get_lz4_data_to_diff_image_data(unsigned char * lz4_data, uint32_t lz4_length,
                                                        uint32_t image_data_length, unsigned char * image_data, uint32_t image_width,
                                                        uint32_t channels, uint32_t line_length, unsigned char * max_data_decompress)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;

    if(max_data_decompress == NULL) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    lz4_decompress_doubleBufferr_buffer(max_data_decompress,
                                        image_data_length, lz4_data, lz4_length);

    if(image_data_length > 0) {
        if(channels == 2) {
            is_su = create_image_data_for_align16bit(
                        max_data_decompress, image_data_length, image_data,
                        line_length);
        }
        else if(channels == 4) {
            is_su = create_image_data_for_align32bit(
                        max_data_decompress, image_data_length, image_data,
                        line_length);
        }
        else {
            is_su = create_image_data_for_noalign(
                        max_data_decompress, image_data_length, image_data,
                        image_width, channels, line_length);
        }
    }

    return is_su;
}

static diff_image_ret_t read_video_info_and_frame_info(lv_fs_file_t * fp, video_info_t * video_info,
                                                       frame_info_t * frame_info_list, uint32_t frame_number)
{
    uint32_t file_point = 0;
    lv_fs_res_t res = lv_fs_seek(fp, 0, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    res = lv_fs_read(fp, video_info, sizeof(video_info_t), NULL);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    diff_image_video_info_from_big_endian(video_info);
    file_point += video_info->start_point + (sizeof(frame_info_t) * frame_number);

    res = lv_fs_seek(fp, file_point, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    res = lv_fs_read(fp, frame_info_list, sizeof(frame_info_t), NULL);
    if(res != LV_FS_RES_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    diff_image_frame_info_from_big_endian(frame_info_list);
    return DIFF_IMAGE_RET_OK;
}

static diff_image_ret_t get_first_frame_data(lv_fs_file_t * fp, unsigned char * image_data)
{
    video_info_t video_info;
    frame_info_t frame_info;
    uint32_t frame_number = 0;
    uint32_t image_data_length = 0;
    unsigned char * lz4_data = NULL;
    unsigned char * max_data_decompress = NULL;
    if(read_video_info_and_frame_info(fp, &video_info, &frame_info, frame_number) != DIFF_IMAGE_RET_OK) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    if(frame_info.compress_type != compress_type_lz4) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    //第一帧数据，默认为LZ4压缩数据
    uint32_t line_length = video_info.width * video_info.channels;
    lz4_data = lv_malloc(frame_info.data_length);
    if(lz4_data == NULL) {
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    lv_fs_res_t res = lv_fs_seek(fp, frame_info.start_point, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        lv_free(lz4_data);
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    res = lv_fs_read(fp, lz4_data, frame_info.data_length, NULL);
    if(res != LV_FS_RES_OK) {
        lv_free(lz4_data);
        return DIFF_IMAGE_RET_BAD_PARAMS;
    }

    //读取颜色表
    if (video_info.table_length > 0) {
        res = lv_fs_seek(fp, video_info.table_start_point, LV_FS_SEEK_SET);
        if(res != LV_FS_RES_OK) {
            lv_free(lz4_data);
            return DIFF_IMAGE_RET_BAD_PARAMS;
        }
        
        res = lv_fs_read(fp, image_data, video_info.table_length, NULL);
        if(res != LV_FS_RES_OK) {
            lv_free(lz4_data);
            return DIFF_IMAGE_RET_BAD_PARAMS;
        }

        image_data += video_info.table_length;
    }

    //将LZ4数据解压缩到image_data
    lv_memzero(image_data, line_length * video_info.height);
    lz4_decompress_doubleBufferr_buffer(image_data, line_length * video_info.height, lz4_data, frame_info.data_length);

    lv_free(lz4_data);
    return DIFF_IMAGE_RET_OK;
}

static diff_image_ret_t read_image_data(lv_fs_file_t * fp, uint32_t frame_number, unsigned char * last_image_data,
                                        uint32_t line_length, uint32_t width, uint32_t height,
                                        uint32_t channels, unsigned char ** max_data_decompress)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;
    video_info_t video_info;
    frame_info_t frame_info_list;

    if(read_video_info_and_frame_info(fp, &video_info, &frame_info_list, frame_number) == DIFF_IMAGE_RET_OK) {
        if(width != video_info.width || height != video_info.height || channels != video_info.channels) {
            LV_ASSERT(!"bitmap's width != video's width or bitmap's height != video's height or bitmap's channels != video's channels");
        }
        else if(line_length != video_info.line_length) {
            LV_ASSERT(!"line_length != video_info.line_length");
        }
        else {
            is_su = read_frame_image(fp, &video_info, &frame_info_list, last_image_data, line_length, *max_data_decompress);
        }
    }

    return is_su;
}

static diff_image_ret_t read_frame_image(lv_fs_file_t * fp, video_info_t * video_info, frame_info_t * frame_info,
                                         unsigned char * last_image_data, uint32_t line_length,
                                         unsigned char * max_data_decompress)
{
    diff_image_ret_t is_su = DIFF_IMAGE_RET_FAIL;
    lv_fs_res_t res = lv_fs_seek(fp, frame_info->start_point, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        return is_su;
    }

    unsigned char * data_com = lv_malloc(frame_info->data_length);
    if(data_com == NULL) {
        return is_su;
    }

    res = lv_fs_read(fp, data_com, frame_info->data_length, NULL);
    if(res != LV_FS_RES_OK) {
        lv_free(data_com);
        return is_su;
    }

    uint32_t width = video_info->width;
    uint32_t height = video_info->height;
    uint32_t channels = video_info->channels;
    compress_type_t compress_type = (compress_type_t)frame_info->compress_type;
    switch(frame_info->compress_type) {
        case compress_type_lz4:
            lv_memzero(last_image_data, height * line_length);
            lz4_decompress_doubleBufferr_buffer(last_image_data, height * line_length, data_com, frame_info->data_length);
            break;
        case compress_type_diff_lz4:
            is_su = get_lz4_data_to_diff_image_data(data_com, frame_info->data_length, frame_info->data_decompress_length,
                                                    last_image_data, width, channels, line_length, max_data_decompress);
            break;
        case compress_type_no_change:
            break;
        default:
            break;
    }

    is_su = DIFF_IMAGE_RET_OK;
    lv_free(data_com);
    return is_su;
}

static uint32_t get_max_data_decompress_length(lv_fs_file_t * fp, uint32_t frame_number)
{
    uint32_t max_length = 0;
    frame_info_t frame_info;

    for(uint32_t i = 0; i < frame_number; i++) {
        lv_fs_res_t res = lv_fs_seek(fp, sizeof(video_info_t) + i * sizeof(frame_info_t), LV_FS_SEEK_SET);
        if(res != LV_FS_RES_OK) {
            return LV_RESULT_INVALID;
        }

        res = lv_fs_read(fp, &frame_info, sizeof(frame_info_t), NULL);
        if(res != LV_FS_RES_OK) {
            return LV_RESULT_INVALID;
        }

        diff_image_frame_info_from_big_endian(&frame_info);
        max_length = max_length > frame_info.data_decompress_length ? max_length : frame_info.data_decompress_length;
    }

    return max_length;
}

static void cleanup_file(lv_fs_file_t * fp, lv_image_dsc_t *** dsc_array, uint32_t count,
                         unsigned char * image_data, unsigned char * max_data_decompress)
{
    if(dsc_array != NULL && *dsc_array != NULL) {
        for(uint32_t i = 0; i < count; i++) {
            if((*dsc_array)[i] != NULL) {
                lv_free((void *)(*dsc_array)[i]->data);
                lv_free((*dsc_array)[i]);
                (*dsc_array)[i] = NULL;
            }
        }
        lv_free(*dsc_array);
        *dsc_array = NULL;
    }
    if(image_data != NULL) {
        lv_free(image_data);
    }
    if(max_data_decompress != NULL) {
        lv_free(max_data_decompress);
    }
    if(fp != NULL) {
        lv_fs_close(fp);
    }
}

static lv_result_t set_frame_descriptor(lv_image_dsc_t * img_dsc, uint32_t w, uint32_t h, unsigned char * data,
                                     uint32_t channels, lv_color_format_t cf)
{
    if(img_dsc == NULL || data == NULL) return LV_RESULT_INVALID;
    uint32_t data_length = w * h * channels;
    if (cf >= LV_COLOR_FORMAT_I1 && cf <= LV_COLOR_FORMAT_I8) {
        data_length += (1 << lv_color_format_get_bpp(cf)) * 4;
    }
    unsigned char * frame_data = lv_malloc(data_length);
    if(frame_data == NULL) return LV_RESULT_INVALID;

    lv_memcpy(frame_data, data, data_length);

    img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc->header.w = w;
    img_dsc->header.h = h;
    img_dsc->header.cf = cf;
    img_dsc->header.flags = 0;
    img_dsc->header.stride = lv_draw_buf_width_to_stride(w, cf);
    img_dsc->header.reserved_2 = 0;
    img_dsc->data_size = data_length;
    img_dsc->data = frame_data;

    return LV_RESULT_OK;
}