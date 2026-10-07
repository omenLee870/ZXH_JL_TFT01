#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "image_rle.h"
#include "rle.h"

#ifdef __cplusplus
extern "C" {
#endif

//取消打印
#if 1
#define printf(...)
#endif

static int image_rle_encode_block(int width, int height, int bpp, int blocksize_w, int blocksize_h, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	int block_size;
	int nblockw = (width + blocksize_w - 1) / blocksize_w;
	int nblockh = (height + blocksize_h - 1) / blocksize_h;
	int headlen = nblockw * nblockh * sizeof(struct rle_header);
	int pixel_size = bpp / 8;

	struct rle_header *head = NULL;
	unsigned char *block_buf = NULL;
	unsigned char *block_buf_tmp = NULL;

	block_buf = (unsigned char *)malloc(blocksize_w * blocksize_h * pixel_size);
	block_buf_tmp = (unsigned char *)malloc(blocksize_w * blocksize_h * pixel_size * 2);
	if (!block_buf) {
		printf("malloc buffer failed! %s, %d\n", __func__, __LINE__);
		return 0;
	}

	if (outBuf && outSize) {
		head = (struct rle_header *)&outBuf[0];
	}

	int offset = headlen;
	for (int bh = 0; bh < nblockh; bh++) {
		for (int bw = 0; bw < nblockw; bw++) {
			int lenw = (bw < (nblockw - 1)) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
			int lenh = (bh < (nblockh - 1)) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

			if ((lenw < blocksize_w) || (lenh < blocksize_h)) {
				for (int j = 0; j < blocksize_h; j++) {
					for (int i = 0; i < blocksize_w; i++) {
						if (j >= lenh) {
							for (int c = 0; c < pixel_size; c++) {
								block_buf[(j * blocksize_w + i) * pixel_size + c] = inBuf[(((lenh - 1) + bh * blocksize_h) * width + bw * blocksize_w + (lenw - 1)) * pixel_size + c];
							}
						} else if (i >= lenw) {
							for (int c = 0; c < pixel_size; c++) {
								if ((j % 2) == 0) {
									block_buf[(j * blocksize_w + i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + (lenw - 1)) * pixel_size + c];
								} else {
									block_buf[(j * blocksize_w + blocksize_w - 1 - i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + (lenw - 1)) * pixel_size + c];
								}
							}
						} else {
							for (int c = 0; c < pixel_size; c++) {
								if ((j % 2) == 0) {
									block_buf[(j * blocksize_w + i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
								} else {
									block_buf[(j * blocksize_w + blocksize_w - 1 - i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
								}
							}
						}
					}
				}
			} else {
				for (int j = 0; j < blocksize_h; j++) {
					for (int i = 0; i < blocksize_w; i++) {
						for (int c = 0; c < pixel_size; c++) {
							if ((j % 2) == 0) {
								block_buf[(j * blocksize_w + i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
							} else {
								block_buf[(j * blocksize_w + blocksize_w - 1 - i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
							}
						}
					}
				}
			}

			if (!outBuf || !outSize) {
				if (blocksize_h == 1) {
					block_size = Rle_Encode(block_buf, lenw * pixel_size, NULL, INT_MAX, pixel_size);
				} else {
					block_size = Rle_Encode(block_buf, blocksize_w * blocksize_h * pixel_size, NULL, INT_MAX, pixel_size);
				}

				if (block_size >= blocksize_w * blocksize_h * pixel_size) {
					block_size = blocksize_w * blocksize_h * pixel_size;
				}

				offset += block_size;
			} else {

				if (blocksize_h == 1) {
					block_size = Rle_Encode(block_buf, lenw * pixel_size, block_buf_tmp, blocksize_w * blocksize_h * pixel_size * 2, pixel_size);
				} else {
					block_size = Rle_Encode(block_buf, blocksize_w * blocksize_h * pixel_size, block_buf_tmp, blocksize_w * blocksize_h * pixel_size * 2, pixel_size);
				}

				if (block_size >= blocksize_w * blocksize_h * pixel_size) {
					memcpy(&outBuf[offset], block_buf, blocksize_w * blocksize_h * pixel_size);
					block_size = blocksize_w * blocksize_h * pixel_size;
					head->uncompressed = 1;
				} else {
					memcpy(&outBuf[offset], block_buf_tmp, block_size);
					head->uncompressed = 0;
				}

				head->addr = offset;
				head->len = block_size;
				head++;

				offset += block_size;
				outSize -= block_size;
			}
		}
	}

	if (block_buf) {
		free(block_buf);
	}
	if (block_buf_tmp) {
		free(block_buf_tmp);
	}

	return offset;
}

static int image_block(int width, int height, int bpp, int blocksize_w, int blocksize_h, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	int block_size;
	int offset = 0;
	int pixel_size = bpp / 8;

	if (pixel_size > 0) {
		int nblockw = (width + blocksize_w - 1) / blocksize_w;
		int nblockh = (height + blocksize_h - 1) / blocksize_h;

		for (int bh = 0; bh < nblockh; bh++) {
			for (int bw = 0; bw < nblockw; bw++) {
				int lenw = (bw < (nblockw - 1)) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
				int lenh = (bh < (nblockh - 1)) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

				if (!outBuf || !outSize) {
					if (blocksize_h == 1) {
						block_size = lenw * pixel_size;
					} else {
						block_size = blocksize_w * blocksize_h * pixel_size;
					}
					offset += block_size;
				} else {
					if (blocksize_h == 1) {
						block_size = lenw * pixel_size;

						uint8_t *ptr = &outBuf[offset];
						for (int j = 0; j < blocksize_h; j++) {
							for (int i = 0; i < blocksize_w; i++) {
								for (int c = 0; c < pixel_size; c++) {
									if ((i < lenw) && (j < lenh)) {
										ptr[(j * blocksize_w + i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
									}
								}
							}
						}
					} else {
						block_size = blocksize_w * blocksize_h * pixel_size;

						uint8_t *ptr = &outBuf[offset];
						for (int j = 0; j < blocksize_h; j++) {
							for (int i = 0; i < blocksize_w; i++) {
								for (int c = 0; c < pixel_size; c++) {
									if ((i < lenw) && (j < lenh)) {
										ptr[(j * blocksize_w + i) * pixel_size + c] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c];
									} else {
										ptr[(j * blocksize_w + i) * pixel_size + c] = 0;
									}
								}
							}
						}
					}

					offset += block_size;
					outSize -= block_size;
				}
			}
		}
	} else {
		int pixel_align = 8 / bpp;
		width = (width + pixel_align - 1) / pixel_align * pixel_align;
		int nblockw = (width + blocksize_w - 1) / blocksize_w;
		int nblockh = (height + blocksize_h - 1) / blocksize_h;

		for (int bh = 0; bh < nblockh; bh++) {
			for (int bw = 0; bw < nblockw; bw++) {
				int lenw = (bw < (nblockw - 1)) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
				int lenh = (bh < (nblockh - 1)) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

				if (!outBuf || !outSize) {
					if (blocksize_h == 1) {
						block_size = lenw * bpp / 8;
					} else {
						block_size = blocksize_w * blocksize_h * bpp / 8;
					}
					offset += block_size;
				} else {
					if (blocksize_h == 1) {
						block_size = lenw * bpp / 8;

						uint8_t *ptr = &outBuf[offset];
						for (int j = 0; j < blocksize_h; j++) {
							for (int i = 0; i < blocksize_w; i += pixel_align) {
								if ((i < lenw) && (j < lenh)) {
									ptr[(j * blocksize_w + i) * bpp / 8] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * bpp / 8];
								}
							}
						}
					} else {
						block_size = blocksize_w * blocksize_h * bpp / 8;

						uint8_t *ptr = &outBuf[offset];
						for (int j = 0; j < blocksize_h; j++) {
							for (int i = 0; i < blocksize_w; i += pixel_align) {
								if ((i < lenw) && (j < lenh)) {
									ptr[(j * blocksize_w + i) * bpp / 8] = inBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * bpp / 8];
								} else {
									ptr[(j * blocksize_w + i) * bpp / 8] = 0;
								}
							}
						}
					}

					offset += block_size;
					outSize -= block_size;
				}
			}
		}
	}

	return offset;
}


static int image_rle_decode_block(int width, int height, int bpp, int blocksize_w, int blocksize_h, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	struct rle_header *head = (struct rle_header *)&inBuf[0];
	int nbytes = 0;
	int block_size;
	int pixel_size = bpp / 8;
	int nblockw = (width + blocksize_w - 1) / blocksize_w;
	int nblockh = (height + blocksize_h - 1) / blocksize_h;

	unsigned char *block_buf = NULL;

	block_buf = (unsigned char *)malloc(blocksize_w * blocksize_h * pixel_size);
	if (!block_buf) {
		printf("malloc buffer failed! %s, %d\n", __func__, __LINE__);
		return 0;
	}

	for (int bh = 0; bh < nblockh; bh++) {
		for (int bw = 0; bw < nblockw; bw++) {
			int lenw = (bw < nblockw - 1) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
			int lenh = (bh < nblockh - 1) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

			if (head->uncompressed) {
				uint8_t *ptr = &inBuf[head->addr];
				block_size = blocksize_w * blocksize_h * pixel_size;

				for (int i = 0; i < lenw; i++) {
					for (int j = 0; j < lenh; j++) {
						for (int c = 0; c < pixel_size; c++) {
							if ((j % 2) == 0) {
								outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c] = ptr[(j * blocksize_w + i) * pixel_size + c];
							} else {
								outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c] = ptr[(j * blocksize_w + blocksize_w - 1 - i) * pixel_size + c];
							}
						}
					}
				}
			} else {
				block_size = Rle_Decode(&inBuf[head->addr], head->len, block_buf, blocksize_w * blocksize_h * pixel_size, pixel_size);

				for (int i = 0; i < lenw; i++) {
					for (int j = 0; j < lenh; j++) {
						for (int c = 0; c < pixel_size; c++) {
							if ((j % 2) == 0) {
								outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c] = block_buf[(j * blocksize_w + i) * pixel_size + c];
							} else {
								outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c] = block_buf[(j * blocksize_w + blocksize_w - 1 - i) * pixel_size + c];
							}
						}
					}
				}
			}

			nbytes += block_size;
			head++;
		}
	}

	if (block_buf) {
		free(block_buf);
	}

	return nbytes;
}

static int image_unblock(int width, int height, int bpp, int blocksize_w, int blocksize_h, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	int nbytes = 0;
	int offset = 0;
	int block_size;
	int pixel_size = bpp / 8;

	if (pixel_size > 0) {
		int nblockw = (width + blocksize_w - 1) / blocksize_w;
		int nblockh = (height + blocksize_h - 1) / blocksize_h;

		for (int bh = 0; bh < nblockh; bh++) {
			for (int bw = 0; bw < nblockw; bw++) {
				int lenw = (bw < nblockw - 1) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
				int lenh = (bh < nblockh - 1) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

				uint8_t *ptr = &inBuf[offset];
				for (int i = 0; i < lenw; i++) {
					for (int j = 0; j < lenh; j++) {
						for (int c = 0; c < pixel_size; c++) {
							outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * pixel_size + c] = ptr[(j * blocksize_w + i) * pixel_size + c];
						}
					}
				}
				if (blocksize_h == 1) {
					block_size = lenw * pixel_size;
				} else {
					block_size = blocksize_w * blocksize_h * pixel_size;
				}
				offset += block_size;
				nbytes += block_size;
			}
		}
	} else {
		int pixel_align = 8 / bpp;
		width = (width + pixel_align - 1) / pixel_align * pixel_align;
		int nblockw = (width + blocksize_w - 1) / blocksize_w;
		int nblockh = (height + blocksize_h - 1) / blocksize_h;

		for (int bh = 0; bh < nblockh; bh++) {
			for (int bw = 0; bw < nblockw; bw++) {
				int lenw = (bw < (nblockw - 1)) ? blocksize_w : width - (nblockw - 1) * blocksize_w;
				int lenh = (bh < (nblockh - 1)) ? blocksize_h : height - (nblockh - 1) * blocksize_h;

				uint8_t *ptr = &inBuf[offset];
				for (int j = 0; j < lenh; j++) {
					for (int i = 0; i < lenw; i += pixel_align) {
						outBuf[((j + bh * blocksize_h) * width + bw * blocksize_w + i) * bpp / 8] = ptr[(j * blocksize_w + i) * bpp / 8];
					}
				}

				if (blocksize_h == 1) {
					block_size = lenw * bpp / 8;
				} else {
					block_size = blocksize_w * blocksize_h * bpp / 8;
				}

				offset += block_size;
				outSize -= block_size;
			}
		}
	}

	return nbytes;
}

int image_rle_encode(int width, int height, int bpp, int block, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	int ret = 0;

	if ((block >= ENCODE_RLE_BLOCK_64x1) && (block <= ENCODE_RLE_BLOCK_16x16)) {
		if (block == ENCODE_RLE_BLOCK_8x8) {
			ret = image_rle_encode_block(width, height, bpp, 8, 8, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RLE_BLOCK_64x1) {
			ret = image_rle_encode_block(width, height, bpp, 64, 1, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RLE_BLOCK_256x1) {
			ret = image_rle_encode_block(width, height, bpp, 256, 1, inBuf, inSize, outBuf, outSize);
		} else {
			ret = image_rle_encode_block(width, height, bpp, 16, 16, inBuf, inSize, outBuf, outSize);
		}
	} else if ((block >= ENCODE_RAW_BLOCK_64x1) && (block <= ENCODE_RAW_BLOCK_16x16)) {
		if (block == ENCODE_RAW_BLOCK_8x8) {
			ret = image_block(width, height, bpp, 8, 8, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RAW_BLOCK_64x1) {
			ret = image_block(width, height, bpp, 64, 1, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RAW_BLOCK_256x1) {
			ret = image_block(width, height, bpp, 256, 1, inBuf, inSize, outBuf, outSize);
		} else {
			ret = image_block(width, height, bpp, 16, 16, inBuf, inSize, outBuf, outSize);
		}
	}

	return ret;
}


int image_rle_decode(int width, int height, int bpp, int block, u8 *inBuf, int inSize, u8 *outBuf, int outSize)
{
	int ret;
	printf("image_rle_decode width: %d height: %d bpp: %d block: %d inSize: %d outSize: %d inBuf: %p outBuf: %p\n", width, height, bpp, block, inSize, outSize, inBuf, outBuf);

	if ((block >= ENCODE_RLE_BLOCK_64x1) && (block <= ENCODE_RLE_BLOCK_16x16)) {
		if (block == ENCODE_RLE_BLOCK_8x8) {
			ret = image_rle_decode_block(width, height, bpp, 8, 8, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RLE_BLOCK_64x1) {
			ret = image_rle_decode_block(width, height, bpp, 64, 1, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RLE_BLOCK_256x1) {
			ret = image_rle_decode_block(width, height, bpp, 256, 1, inBuf, inSize, outBuf, outSize);
		} else {
			ret = image_rle_decode_block(width, height, bpp, 16, 16, inBuf, inSize, outBuf, outSize);
		}
	} else if ((block >= ENCODE_RAW_BLOCK_64x1) && (block <= ENCODE_RAW_BLOCK_16x16)) {
		if (block == ENCODE_RAW_BLOCK_8x8) {
			ret = image_unblock(width, height, bpp, 8, 8, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RAW_BLOCK_64x1) {
			ret = image_unblock(width, height, bpp, 64, 1, inBuf, inSize, outBuf, outSize);
		} else if (block == ENCODE_RAW_BLOCK_256x1) {
			ret = image_unblock(width, height, bpp, 256, 1, inBuf, inSize, outBuf, outSize);
		} else {
			ret = image_unblock(width, height, bpp, 16, 16, inBuf, inSize, outBuf, outSize);
		}
	}

	return ret;
}

#ifdef __cplusplus
}
#endif
