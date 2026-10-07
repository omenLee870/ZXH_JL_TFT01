#ifndef IMAGE_RLE_H
#define IMAGE_RLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "TypeExt.h"

typedef enum {
    JLGPU2D_FORMAT_ARGB8888     = 0x00,
    JLGPU2D_FORMAT_ARGB8565     = 0x01,
    JLGPU2D_FORMAT_ARGB1555     = 0x02,
    JLGPU2D_FORMAT_ARGB4444     = 0x03,
    JLGPU2D_FORMAT_RGB888       = 0x04,
    JLGPU2D_FORMAT_RGB565       = 0x05,
    JLGPU2D_FORMAT_YUV422_BT601 = 0x06,
    JLGPU2D_FORMAT_YUV422_BT709 = 0x07,
    JLGPU2D_FORMAT_AL88         = 0x08,

    JLGPU2D_FORMAT_AL44         = 0x10,
    JLGPU2D_FORMAT_AL22         = 0x11,
    JLGPU2D_FORMAT_L8           = 0x12,
    JLGPU2D_FORMAT_L4           = 0x13,
    JLGPU2D_FORMAT_L2           = 0x14,
    JLGPU2D_FORMAT_L1           = 0x15,
    JLGPU2D_FORMAT_A8           = 0x16,
    JLGPU2D_FORMAT_A4           = 0x17,
    JLGPU2D_FORMAT_A2           = 0x18,
    JLGPU2D_FORMAT_A1           = 0x19,

    JLGPU2D_FORMAT_UNKNOWN,
} JLGPU2D_format_t;

enum {
	ENCODE_NONE,            //0
	ENCODE_RLE_BLOCK_64x1,  //1  line base
	ENCODE_RLE_BLOCK_8x8,   //2  tile base
	ENCODE_RLE_BLOCK_256x1, //3  line base
	ENCODE_RLE_BLOCK_16x16, //4  tile base
	ENCODE_RAW_BLOCK_64x1,  //5  line base
	ENCODE_RAW_BLOCK_8x8,   //6  tile base
	ENCODE_RAW_BLOCK_256x1, //7  line base
	ENCODE_RAW_BLOCK_16x16, //8  tile base
};


#define RLE_LEN_BITS    (9)
#define RLE_ADDR_BITS   (32 - RLE_LEN_BITS - 1)

struct rle_header {
    u32 addr : RLE_ADDR_BITS;
    u32 len : RLE_LEN_BITS;
    u32 uncompressed : 1;
};


int image_rle_encode(int width, int height, int bpp, int block, u8 *inBuf, int inSize, u8 *outBuf, int outSize);
int image_rle_decode(int width, int height, int bpp, int block, u8 *inBuf, int inSize, u8 *outBuf, int outSize);

#ifdef __cplusplus
}
#endif

#endif // IMAGE_RLE_H
