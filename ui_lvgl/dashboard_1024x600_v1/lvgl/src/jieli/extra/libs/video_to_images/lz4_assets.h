/**
 * @file lz4_assets.h
 *
 */

#ifndef LZ4_ASSETS_H
#define LZ4_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lz4_decompress_doubleBufferr_buffer(unsigned char * out_buff, long out_buff_length, unsigned char * in_buff,
                                         long in_buff_length);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LZ4_ASSETS_H */