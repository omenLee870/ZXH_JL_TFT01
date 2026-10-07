/**
 * @file diff_image_to_video_types.h
 *
 */

#ifndef DIFF_IMAGE_TO_VIDEO_TYPES_H
#define DIFF_IMAGE_TO_VIDEO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

#define VIDEO_INFO_HEAD_STRING                 "JL_IMAGE_VIDEO"
const static unsigned char s_video_info_head[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,           //兼容区
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17    //标记开头区
};

#define VIDEO_INFO_HEAD_SIZE                   sizeof(s_video_info_head) + sizeof(VIDEO_INFO_HEAD_STRING)
#define VIDEO_INFO_VERSION                     3
#define VIDEO_INFO_MIN_VERSION                 3

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _Point_t {
    int32_t x;
    int32_t y;
} Point;

typedef struct _Rect_t {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} Rect;

typedef enum _compress_type_t {
    compress_type_lz4 = 0,
    compress_type_diff_lz4,
    compress_type_no_change,
} compress_type_t;

typedef struct _image_diff_rect_t {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t line_length;
} image_diff_rect_t;

typedef struct _image_diff_data_t {
    image_diff_rect_t rect;
    unsigned char * data;
} image_diff_data_t;

typedef struct _frame_info_t {
    uint32_t start_point;
    uint32_t data_length;
    uint32_t data_decompress_length;
    uint32_t compress_type;
} frame_info_t;

typedef struct _video_info_t {
    unsigned char video_info_head[VIDEO_INFO_HEAD_SIZE];
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    uint32_t delays;
    uint32_t channels;
    uint32_t line_length;
    uint32_t time_length;
    uint32_t frame_number;
    uint32_t frame_image_type;
    uint32_t orientation;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t logical_line_length;
    uint32_t start_point;
    uint32_t table_start_point; // 颜色表起始位置
    uint32_t table_length; // 颜色表长度
} video_info_t;

typedef struct _video_head_t {
    video_info_t video_info;

    frame_info_t * frame_info_list;
} video_head_t;

/**
 * @enum diff_image_ret_t
 * @annotation ["scriptable"]
 * 函数返回值常量定义。
 */
typedef enum _diff_image_ret_t {
    /**
     * @const DIFF_IMAGE_RET_OK
     * 成功。
     */
    DIFF_IMAGE_RET_OK = 0,
    /**
     * @const DIFF_IMAGE_RET_OK
     * 无效参数。
     */
    DIFF_IMAGE_RET_BAD_PARAMS,
    /**
     * @const DIFF_IMAGE_RET_FAIL
     * 无效参数。
     */
    DIFF_IMAGE_RET_FAIL,
    /**
     * @const DIFF_IMAGE_RET_OOM
     * oom。
     */
    DIFF_IMAGE_RET_OOM,
    /**
     * @const DIFF_IMAGE_RET_UNKNOWN
     * 无法识别。
     */
    DIFF_IMAGE_RET_UNKNOWN,
    /**
     * @const DIFF_IMAGE_RET_ERROR_VERSION
     * 不支持该版本
     */
    DIFF_IMAGE_RET_ERROR_VERSION,
    /**
     * @const DIFF_IMAGE_RET_ERROR_BITMAP_FORM
     * 不支持该位图颜色格式
     */
    DIFF_IMAGE_RET_ERROR_BITMAP_FORM,
} diff_image_ret_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/


static inline uint8_t diff_image_is_little_endian(void)
{
    uint16_t val = 0x1122;
    return *((uint8_t *)&val) == 0x22;
}

static inline void diff_image_swap_uint8(uint8_t * p1, uint8_t * p2)
{
    uint8_t c = *p1;
    *p1 = *p2;
    *p2 = c;
}

static inline int32_t diff_image_int32_endian_invert(int32_t value)
{
    int32_t ret = value;
    uint8_t * p = (uint8_t *)&ret;

    diff_image_swap_uint8(p, p + 3);
    diff_image_swap_uint8(p + 1, p + 2);

    return ret;
}

#define diff_image_int32_to_big_endian(value) (int32_t)(diff_image_is_little_endian() != 0 ? diff_image_int32_endian_invert(value) : (value));
#define diff_image_int32_from_big_endian(value) (int32_t)diff_image_int32_to_big_endian(value);

#define diff_image_uint32_to_big_endian(value) (uint32_t)(diff_image_is_little_endian() != 0 ? diff_image_int32_endian_invert(value) : (value));
#define diff_image_uint32_from_big_endian(value) (uint32_t)diff_image_int32_to_big_endian(value);

static inline void diff_image_video_info_to_big_endian(video_info_t * info)
{
    info->version = diff_image_uint32_to_big_endian(info->version);
    info->width = diff_image_uint32_to_big_endian(info->width);
    info->height = diff_image_uint32_to_big_endian(info->height);
    info->flags = diff_image_uint32_to_big_endian(info->flags);
    info->delays = diff_image_uint32_to_big_endian(info->delays);
    info->channels = diff_image_uint32_to_big_endian(info->channels);
    info->line_length = diff_image_uint32_to_big_endian(info->line_length);
    info->time_length = diff_image_uint32_to_big_endian(info->time_length);
    info->frame_number = diff_image_uint32_to_big_endian(info->frame_number);
    info->frame_image_type = diff_image_uint32_to_big_endian(info->frame_image_type);
    info->orientation = diff_image_uint32_to_big_endian(info->orientation);
    info->logical_width = diff_image_uint32_to_big_endian(info->logical_width);
    info->logical_height = diff_image_uint32_to_big_endian(info->logical_height);
    info->logical_line_length = diff_image_uint32_to_big_endian(info->logical_line_length);
    info->start_point = diff_image_uint32_to_big_endian(info->start_point);
}

static inline void diff_image_video_info_from_big_endian(video_info_t * info)
{
    info->version = diff_image_uint32_from_big_endian(info->version);
    info->width = diff_image_uint32_from_big_endian(info->width);
    info->height = diff_image_uint32_from_big_endian(info->height);
    info->flags = diff_image_uint32_from_big_endian(info->flags);
    info->delays = diff_image_uint32_from_big_endian(info->delays);
    info->channels = diff_image_uint32_from_big_endian(info->channels);
    info->line_length = diff_image_uint32_from_big_endian(info->line_length);
    info->time_length = diff_image_uint32_from_big_endian(info->time_length);
    info->frame_number = diff_image_uint32_from_big_endian(info->frame_number);
    info->frame_image_type = diff_image_uint32_from_big_endian(info->frame_image_type);
    info->orientation = diff_image_uint32_from_big_endian(info->orientation);
    info->logical_width = diff_image_uint32_from_big_endian(info->logical_width);
    info->logical_height = diff_image_uint32_from_big_endian(info->logical_height);
    info->logical_line_length = diff_image_uint32_from_big_endian(info->logical_line_length);
    info->start_point = diff_image_uint32_from_big_endian(info->start_point);
}

static inline void diff_image_frame_info_to_big_endian(frame_info_t * frame_info)
{
    frame_info->compress_type = diff_image_uint32_to_big_endian(frame_info->compress_type);
    frame_info->data_decompress_length = diff_image_uint32_to_big_endian(frame_info->data_decompress_length);
    frame_info->data_length = diff_image_uint32_to_big_endian(frame_info->data_length);
    frame_info->start_point = diff_image_uint32_to_big_endian(frame_info->start_point);
}

static inline void diff_image_frame_info_from_big_endian(frame_info_t * frame_info)
{
    frame_info->compress_type = diff_image_uint32_from_big_endian(frame_info->compress_type);
    frame_info->data_decompress_length = diff_image_uint32_from_big_endian(frame_info->data_decompress_length);
    frame_info->data_length = diff_image_uint32_from_big_endian(frame_info->data_length);
    frame_info->start_point = diff_image_uint32_from_big_endian(frame_info->start_point);
}

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DIFF_IMAGE_TO_VIDEO_TYPES_H */