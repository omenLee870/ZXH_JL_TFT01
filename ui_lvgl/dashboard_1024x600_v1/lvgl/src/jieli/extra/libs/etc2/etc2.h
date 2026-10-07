#ifndef __ETC2_H__
#define __ETC2_H__

#ifdef __cplusplus
extern "C" {
#endif

// Typedefs
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef short int16;

enum { ETC1_RGB_NO_MIPMAPS, ETC2PACKAGE_RGB_NO_MIPMAPS, ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD, ETC2PACKAGE_RGBA_NO_MIPMAPS, ETC2PACKAGE_RGBA1_NO_MIPMAPS, ETC2PACKAGE_R_NO_MIPMAPS, ETC2PACKAGE_RG_NO_MIPMAPS, ETC2PACKAGE_R_SIGNED_NO_MIPMAPS, ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS, ETC2PACKAGE_sRGB_NO_MIPMAPS, ETC2PACKAGE_sRGBA_NO_MIPMAPS, ETC2PACKAGE_sRGBA1_NO_MIPMAPS, ETC2PACKAGE_A8_NO_MIPMAPS, ETC2PACKAGE_sA8_NO_MIPMAPS };
enum { MODE_COMPRESS, MODE_UNCOMPRESS, MODE_PSNR };
enum { SPEED_SLOW, SPEED_FAST, SPEED_MEDIUM };
enum { METRIC_PERCEPTUAL, METRIC_NONPERCEPTUAL };
enum { CODEC_ETC, CODEC_ETC2 };
enum { CODEC_ETC2_ALL, CODEC_ETC2_P, CODEC_ETC2_H, CODEC_ETC2_T };

enum {MODE_HIGH_QUALITY, MODE_FAST};

int ETC2CompressRawData(uint8 *ref_rgb, uint8 *ref_alpha, uint8 *pixel_rgb, uint8 *pixel_alpha, int width, int height, int _format, int _codec, int _metric, int _speed, uint8 *dstimg);
int ETC2UnCompressRawData(uint8 *etc2_data, int etc2_data_size, int width, int height, int _format, uint8 *img, uint8 *alphaimg);

#ifdef __cplusplus
}
#endif

#endif