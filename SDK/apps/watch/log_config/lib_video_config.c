#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_video_config.data.bss")
#pragma data_seg(".lib_video_config.data")
#pragma const_seg(".lib_video_config.text.const")
#pragma code_seg(".lib_video_config.text")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "debug.h"

//================================================//
//        		  ISC 模块 变量控制				  //
//================================================//
const u8 g_isc_keep_open = 1;   //isc摄像头保持打开变量
const u8 config_isc_frame_st_ie = 0;
const u8 config_isc_frame_ed_ie = 0;
const u8 config_isc_line_st_ie = 0;
const u8 config_isc_line_ed_ie = 0;

//================================================//
//        		  IMC 模块 变量控制				  //
//================================================//
const u8 config_imc_line_dst_ie = 0;
const u8 config_imc_frame_dst_ie = 1;

//================================================//
//        		  JPG 模块 变量控制				  //
//================================================//
const u8 g_dynamic_huffman_en = 0;  //使能动态huffman编码功能
const u8 g_jpg_abr_en = 1;  //使能动态码流

//============================================================//
//          VIDEO驱动 内存是否使用psram
//============================================================//
#if TCFG_PSRAM_DEV_ENABLE
const int config_video_malloc_use_psram = 1;
#else
const int config_video_malloc_use_psram = 0;
#endif


//               verb   debug   info  warn  error char
//================================================//
//        		  JPG 模块打印				      //
//================================================//
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_HAL)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_ENCODE)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_DECODE)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_GXDEC)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_ABR)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_HUFFMAN)


REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, NCHENC_HAL)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_NCH)

REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JXENC_HAL)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, JPEG_JX)


REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, REP_HAL)

REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, VBUS_HAL)

REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, ISC_HAL)

REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, IMC_HAL)

REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_MEM)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_LCD)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_COMB)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_MANAGER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_DEV)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FB_VIR)

//================================================//
//        	   VSTREAM 传输模块打印			      //
//================================================//
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, VSTREAM_RECEIVER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, VSTREAM_SENDER)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, KWT_VIDEO)

