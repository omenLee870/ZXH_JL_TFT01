/**
 * @file lv_video_to_images.h
 *
 */

#ifndef LV_VIDEO_TO_IMAGES_H
#define LV_VIDEO_TO_IMAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../../../lvgl.h"
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Convert a video file to a series of images
 * @param src video file path
 * @param dsc_array array of image descriptors
 * @param pic_count number of images
 * @return lv_result_t
 */
lv_result_t lv_video_to_images_convert(const char * src, lv_image_dsc_t *** dsc_array, uint32_t * pic_count);

/**
 * Get the next image from the video file
 * @param src video file path
 * @param image image descriptor
 * @param frame_count number of frames
 * @param cur_frame_number current frame number
 * @param max_data_decompress maximum data decompress length
 * @return lv_result_t
 */
lv_result_t lv_video_get_image(const char * src, lv_image_dsc_t * image, uint32_t frame_count, uint32_t cur_frame_number,
                            uint32_t max_data_decompress);

/**
 * Get the information of a video file
 * @param src video file path
 * @param frame_count number of frames
 * @param max_data_decompress maximum data decompress length
 * @return lv_result_t
 */
lv_result_t lv_video_get_info(const char * src, uint32_t * frame_count, uint32_t * max_data_decompress);

/**
 * Free the memory allocated by `lv_video_to_images_convert`
 * @param dsc_array
 * @param pic_count
 * @return lv_result_t
 */
lv_result_t lv_video_to_images_free(lv_image_dsc_t *** dsc_array, uint32_t pic_count);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_VIDEO_TO_IMAGES_H */