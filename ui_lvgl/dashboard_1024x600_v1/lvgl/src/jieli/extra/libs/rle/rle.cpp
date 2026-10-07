#include <stdio.h>
#include "rle.h"

#ifdef __cplusplus
extern "C" {
#endif

//取消打印
#if 1
#define printf(...)
#endif

u32 GetBytes(u8 *src, int pixel_size)
{
	u32 dat = 0;
	if (pixel_size == 1) {
		dat = src[0];
	} else if (pixel_size == 2) {
		dat = src[0] | src[1] << 8;
	} else if (pixel_size == 3) {
		dat = src[0] | src[1] << 8 | src[2] << 16;
	} else {
		dat = src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24;
	}

	return dat;
}


int IsRepetitionStart(u8 *src, int len, int pixel_size)
{
	u8 *p = src;
	u32 first_byte;
	int byte_count = 0;

	first_byte = GetBytes(p, pixel_size);

	while (len--) {
		if (GetBytes(p, pixel_size) == first_byte) {
			byte_count++;
		} else {
			byte_count = 0;
		}

		if (byte_count >= REPEAT_THRESHOLD) { /*有连续3个以上的重复数据才认为是重复数据*/
			return 1;
		}

		if (((p - src) / pixel_size) == REPEAT_THRESHOLD) { /*只统计3个数据*/
			break;
		}

		p += pixel_size;
	}

	/*数据结束*/
	return 0;
}

int GetRepetitionCount(u8 *src, int len, int pixel_size)
{
	u8 *p = src;
	u32 first_byte;
	u8 byte_count = 0;

	first_byte = GetBytes(p, pixel_size);
	while (len--) {
		if (GetBytes(p, pixel_size) == first_byte) { /*重复的字节*/
			byte_count++;
			if (byte_count == 127) { /*重复计数已经超过计数的上限 127*/
				return byte_count;
			}
		} else {/*非重复的字节*/
			return byte_count;
		}
		p += pixel_size;
	}
	/*数据结尾*/
	return byte_count;
}

int GetNonRepetitionCount(u8 *src, int len, int pixel_size)
{
	u8 *p = src;
	u32 begin;
	u8 byte_count = 0;
	u8 index = 0;

	begin = GetBytes(p, pixel_size);

	while (len--) {
		if (GetBytes(p, pixel_size) == begin) {
			byte_count++;
			if (byte_count == REPEAT_THRESHOLD) {
				p += pixel_size;
				return (p - src + 1 - byte_count * pixel_size) / pixel_size;
			}
		} else {
			begin = GetBytes(p, pixel_size);
			byte_count = 1;
		}
		p += pixel_size;
		index++;
		if (index == 127) { /*计数上限*/
			return index;
		}
	}

	return (p - src) / pixel_size;
}

int Rle_Encode(u8 *inbuf, int inSize, u8 *outbuf, int onuBufSize, int pixel_size)
{
	u8 *src = inbuf;
	int i;
	int encSize = 0;
	int srcLeft;

	ASSERT((inSize % pixel_size) == 0);
	srcLeft = inSize / pixel_size;

	while (srcLeft > 0) {
		int count = 0;
#ifdef RLE_DEBUG
		printf("\n\n-----------srcLeft : %d--------------\n", srcLeft);
		dump(src, srcLeft * PIXEL_SIZE);
#endif
		int ret = IsRepetitionStart(src, srcLeft, pixel_size);


		if (ret) { /*是否连续三个字节数据相同？ */
			if ((encSize + 1 + pixel_size) > onuBufSize) { /* 输出缓冲区空间不够了 */
				return -1;
			}
			count = GetRepetitionCount(src, srcLeft, pixel_size);
#ifdef RLE_DEBUG
			if (pixel_size == 1) {
				printf("Continues : 0x%02x , %d\n", src[0], count);
			} else if (pixel_size == 2) {
				printf("Continues : 0x%02x%02x , %d\n", src[0], src[1], count);
			} else if (pixel_size == 3) {
				printf("Continues : 0x%02x%02x%02x , %d\n", src[0], src[1], src[2], count);
			} else {
				printf("Continues : 0x%02x%02x%02x%02x , %d\n", src[0], src[1], src[2], src[3], count);
			}
			dump(src, count * PIXEL_SIZE);
#endif
			if (outbuf) {
				/*重复的数据个数*/
				outbuf[encSize++] = count | 0x80;
				/*重复数据*/
				if (pixel_size == 1) {
					outbuf[encSize++] = src[0];
				} else if (pixel_size == 2) {
					outbuf[encSize++] = src[0];
					outbuf[encSize++] = src[1];
				} else if (pixel_size == 3) {
					outbuf[encSize++] = src[0];
					outbuf[encSize++] = src[1];
					outbuf[encSize++] = src[2];
				} else {
					outbuf[encSize++] = src[0];
					outbuf[encSize++] = src[1];
					outbuf[encSize++] = src[2];
					outbuf[encSize++] = src[3];
				}
			} else {
				encSize++;
				encSize += pixel_size;
			}
			src += count * pixel_size;
			srcLeft -= count;
		} else {
			count = GetNonRepetitionCount(src, srcLeft, pixel_size);
#ifdef RLE_DEBUG
			printf("Non-continues : %d\n", count);
			dump(src, count * PIXEL_SIZE);
#endif
			if ((encSize + 1 + count * pixel_size) > onuBufSize) { /* 输出缓冲区空间不够了 */
				return -1;
			}
			/*非重复的数据个数*/
			if (outbuf) {
				outbuf[encSize++] = count;
			} else {
				encSize++;
			}
			/*非重复数据*/
			for (i = 0; i < count; i++) { /*逐个复制这些数据*/
				if (outbuf) {
					if (pixel_size == 1) {
						outbuf[encSize++] = src[0];
					} else if (pixel_size == 2) {
						outbuf[encSize++] = src[0];
						outbuf[encSize++] = src[1];
					} else if (pixel_size == 3) {
						outbuf[encSize++] = src[0];
						outbuf[encSize++] = src[1];
						outbuf[encSize++] = src[2];
					} else {
						outbuf[encSize++] = src[0];
						outbuf[encSize++] = src[1];
						outbuf[encSize++] = src[2];
						outbuf[encSize++] = src[3];
					}
				} else {
					encSize += pixel_size;
				}
				src += pixel_size;
			}
			srcLeft -= count;
		}
	}

	return encSize;
}

int Rle_Decode(u8 *inbuf, int inSize, u8 *outbuf, int onuBufSize, int pixel_size)
{
	u8 *src = inbuf;
	int i;
	int decSize = 0;
	int count = 0;
	/* int pixel_size = PIXEL_SIZE; */

	while (src < (inbuf + inSize)) {
		u8 sign = *src++;
		int count = sign & 0x7F;
		if ((decSize + count * pixel_size) > onuBufSize) { /* 输出缓冲区空间不够了 */
			printf("decSize : %d\n", decSize);
			printf("count : %d\n", count);
			printf("onuBufSize : %d\n", onuBufSize);
			printf("outbuf overflow!\n");
			return -1;
		}

		if ((sign & 0x80) == 0x80) { /* 连续重复数据标志 */
			for (i = 0; i < count; i++) {
				if (pixel_size == 1) {
					outbuf[decSize++] = src[0];
				} else if (pixel_size == 2) {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
				} else if (pixel_size == 3) {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
					outbuf[decSize++] = src[2];
				} else {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
					outbuf[decSize++] = src[2];
					outbuf[decSize++] = src[3];
				}
			}
			src += pixel_size;
		} else {
			for (i = 0; i < count; i++) {
				if (pixel_size == 1) {
					outbuf[decSize++] = src[0];
				} else if (pixel_size == 2) {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
				} else if (pixel_size == 3) {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
					outbuf[decSize++] = src[2];
				} else {
					outbuf[decSize++] = src[0];
					outbuf[decSize++] = src[1];
					outbuf[decSize++] = src[2];
					outbuf[decSize++] = src[3];
				}
				src += pixel_size;
			}
		}
	}

	return decSize;
}

#ifdef __cplusplus
}
#endif
