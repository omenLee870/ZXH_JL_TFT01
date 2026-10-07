/**
 * @file lz4_assets.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lz4_assets.h"
#include "../../../../../src/libs/lz4/lz4.h"
#include "../../../../../lvgl.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/

#define BLOCK_BYTES 1024 * 8

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

size_t lz4_read_int32(const unsigned char * buff, long start, long buff_length, unsigned int * i);
size_t lz4_read_buff(const unsigned char * buff, long start, long buff_length, char ** array_buff,
                     unsigned int arrayBytes);
size_t lz4_write_buff(unsigned char * buff, long start, long buff_length, const void * array_buff,
                      unsigned int arrayBytes);


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lz4_decompress_doubleBufferr_buffer(unsigned char * out_buff, long out_buff_length, unsigned char * in_buff,
                                         long in_buff_length)
{
    long in_start = 0;
    unsigned int decBytes = 0;
    unsigned int cmpBytes = 0;
    unsigned int out_start = 0;
    unsigned int decBufIndex = 0;

    size_t readCount0 = 0;
    size_t readCount1 = 0;

    char * cmpBuf = NULL;
    char * decPtr = NULL;
    char decBuf[2][BLOCK_BYTES];

    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t * lz4StreamDecode = &lz4StreamDecode_body;
    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);

    for(;;) {
        readCount0 = lz4_read_int32(in_buff, in_start, in_buff_length, &cmpBytes);
        if(readCount0 != 1 || cmpBytes <= 0) {
            break;
        }
        in_start += sizeof(cmpBytes);
        readCount1 =
            lz4_read_buff(in_buff, in_start, in_buff_length, &cmpBuf, cmpBytes);
        if(readCount1 != (size_t)cmpBytes) {
            break;
        }
        in_start += cmpBytes;

        decPtr = decBuf[decBufIndex];
        decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decPtr,
                                                cmpBytes, BLOCK_BYTES);
        if(decBytes <= 0) {
            break;
        }
        lz4_write_buff(out_buff, out_start, out_buff_length, decPtr, decBytes);
        out_start += decBytes;

        decBufIndex = (decBufIndex + 1) % 2;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

size_t lz4_read_int32(const unsigned char * buff, long start, long buff_length, unsigned int * i)
{
    if(start >= buff_length) {
        return 0;
    }
    if(start + sizeof(int) < buff_length) {
        memcpy(i, buff + start, sizeof(unsigned int));
    }
    return 1;
}

size_t lz4_read_buff(const unsigned char * buff, long start, long buff_length, char ** array_buff,
                     unsigned int arrayBytes)
{
    if(start >= buff_length) {
        return 0;
    }
    *array_buff = (char *)(buff + start);
    if(start + arrayBytes < (unsigned int)buff_length) {
        return arrayBytes;
    }
    else {
        return arrayBytes - (start + arrayBytes - buff_length);
    }
}

size_t lz4_write_buff(unsigned char * buff, long start, long buff_length, const void * array_buff,
                      unsigned int arrayBytes)
{
    if(start >= buff_length) {
        return 0;
    }
    size_t size = 0;
    if(start + arrayBytes < (unsigned int)buff_length) {
        size = arrayBytes;
    }
    else {
        size = arrayBytes - (start + arrayBytes - buff_length);
    }
    memcpy(buff + start, array_buff, size);

    return size;
}