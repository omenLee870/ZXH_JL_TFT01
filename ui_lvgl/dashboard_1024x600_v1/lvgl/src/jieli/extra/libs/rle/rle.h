#ifndef RLE_H
#define RLE_H

#include <string.h>
#include <stdint.h>
#include <assert.h>

// 定义类型
typedef uint8_t  u8;
typedef uint32_t u32;

// 定义宏
#define ASSERT(x) assert(x)
#define REPEAT_THRESHOLD 3

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明
int Rle_Encode(u8 *inbuf, int inSize, u8 *outbuf, int onuBufSize, int pixel_size);
int Rle_Decode(u8 *inbuf, int inSize, u8 *outbuf, int onuBufSize, int pixel_size);

#ifdef __cplusplus 
}
#endif

#endif /* RLE_H */

