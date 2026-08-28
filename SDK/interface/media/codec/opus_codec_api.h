#ifndef OPUS_CODEC_API_H
#define OPUS_CODEC_API_H

#include "audio_encode_common_api.h"
#include "audio_decode_common_api.h"


/******************* ENC *******************/

typedef struct _OPUS_EN_FILE_IO_ {
    void *priv;
    u16(*input_data)(void *priv, s16 *buf, u8 channel, u16 points);   //short   2字节一个点.
    u32(*output_data)(void *priv, u8 *buf, u16 len);  //bytes
} OPUS_EN_FILE_IO;


typedef struct _OPUS_ENC_PARA_ {
    int sr;   //samplerate: fixed_16000.
    int br;   //bitrate:    old:16000/32000/64000. --->new:16000~80000.16~80kbps.
    u16 nch;  //channels    fixed_1.  仅支持单声道编码.
    u16 format_mode;   //封装格式:   0:百度无头.      1:酷狗_eng+range.    2:ogg封装,pc软件可播放.  3:size+rangeFinal. 源码可兼容版本.
    u16 complexity;    //0.1.2.3.    3质量最好.速度要求最高.
    u16 frame_ms;      //20|40|60|80|100ms.
} OPUS_ENC_PARA;

typedef struct __OPUS_ENC_OPS {
    u32(*need_buf)(OPUS_ENC_PARA *para);
    u32(*open)(u8 *ptr, OPUS_EN_FILE_IO *audioIO, OPUS_ENC_PARA *para);
    u32(*run)(u8 *ptr);
} OPUS_ENC_OPS;

typedef struct _opus_deccfg_inf {
    u32 bitwidth;   //16_24(s32)      默认 16
    u32 channels;   //最多支持的声道数  默认 2
    u32 sr;         //最大支持的采样率. 默认 48000
    u32 dec_mode; // 设置解码模式   (OPUS_TYPE_SILK | OPUS_TYPE_CELT)：silk+celt  OPUS_TYPE_SILK:silk  OPUS_TYPE_CELT:celt
    u32 low_samplerate_oput; // 仅silk模式下，可设置输出通道数与采样率； 0:48k2ch  1:16k1ch； 默认0
    u32 container_oggo;  // 设置是否为ogg封装 0：非ogg封装。1：ogg封装，默认1
    u32 enable_plc;      // 设置是否使能PLC处理，0：不使能；1：使能
} opus_deccfg_inf;

typedef struct __OPUS_STENC_OPS {
    u32(*need_buf)(OPUS_ENC_PARA *para);       //samplerate=16k   ignore
    u32(*open)(u8 *ptr, OPUS_EN_FILE_IO *audioIO, OPUS_ENC_PARA *para);
    u32(*run)(u8 *ptr);
    u32(*run_wb)(u8 *ptr, short *pcmL, short *pcmR, u8 *obuf, int *encLen);  //run_with_buffer  仅限封装模式0 or 1.
} OPUS_STENC_OPS;

extern OPUS_ENC_OPS *get_opus_enc_ops(void);
extern OPUS_ENC_OPS *get_opus_stenc_ops(void);


/******************** DEC **********************/

typedef struct __OPUSLIB_DEC_OPS {
    u32(*need_buf)(int  bitwidth);
    u32(*open)(u32 *ptr, int br_index, int  bitwidth);         //0,1,2
    u32(*run)(u32 *ptr, char *frame_data, short *frame_out);
} opuslib_dec_ops;

typedef struct  _BR_CONTEXT_ {
    int br_index;
} BR_CONTEXT;


// opus (silkm模式)解码设置
#define SET_DEC_SR           0x91

// ogg_opus 解码设置
#define	SET_OPUS_RAWDTF      0x90 //设置OPUS 为raw 数据. 带8字节packet头(4字节大端包长+4字节range校验值)
#define	SET_OPUS_CBR_PKTLEN  0x91 //设置OPUS 为raw 数据 + CBR_OPUS 包长,可能有多帧共用TOC. 返回0设置成功.
#define SET_OGG_AUDIO_STREAM_MODE  0x92 //设置opus的ogg解码模式为数据流模式，读取多少数据就解码的多少的数据


typedef struct _AUDIO_OPUS_PKTLEN {
    u32 opus_pkt_len;    //SET_OPUS_CBR_PKTLEN
} AUDIO_OPUS_PKTLEN;


//优化opus解码库代码量的控制常量
extern const int SET_OPUS_TYPE;   //  (OPUS_TYPE_SILK | OPUS_TYPE_CELT): 编译silk+celt代码  OPUS_TYPE_SILK : 仅silk代码 OPUS_TYPE_CELT : 仅celt代码
extern const int SET_OPUS_FEATURE;   // OPUS_FEAT_PLC: 编译PLC功能代码 OPUS_FEAT_OGG: 编译OGG功能代码 (OPUS_FEAT_PLC | OPUS_FEAT_OGG): 两种功能代码都编译
extern const int SET_OPUS_OUTPUT_MODE; // OPUS_OUTPUT_16K_MONO: 编译16k1ch处理的代码 OPUS_OUTPUT_48K_STEREO:编译48k2ch处理的代码  (OPUS_OUTPUT_16K_MONO | OPUS_OUTPUT_48K_STEREO) ：两种处理代码都编译

extern opuslib_dec_ops *getopuslibdec_ops();


#define OPUS_INDATA_SUPPORT_FILE		1	// 支持文件类型数据


#define OPUS_SR_8000_OUT_POINTS			(160)
#define OPUS_SR_16000_OUT_POINTS		(320)


#endif


