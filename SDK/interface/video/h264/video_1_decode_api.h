#ifndef __VIDEO_1_DECODE_API__
#define  __VIDEO_1_DECODE_API__



struct video_1_output {
    u8 *frame_buf;
    u8 type;
    int width;
    int height;
    int timescale;
    int numunits;
};

enum {
    video_1_DEC_RDY,
    video_1_DEC_PIC_RDY,
    video_1_DEC_HDRS_RDY,
    video_1_DEC_ERROR,
    video_1_DEC_PARAM_SET_ERROR,
    video_1_DEC_MEMALLOC_ERROR
};

/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_alloc申请解码句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void *video_1_alloc();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_free 释放解码句柄
 *
 * @param st
 */
/* ------------------------------------------------------------------------------------*/
void video_1_free(void *st);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_init 初始化句柄
 *
 * @param st
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u32 video_1_init(void *st);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_deinit 逆初始化句柄
 *
 * @param st
 */
/* ------------------------------------------------------------------------------------*/
void video_1_deinit(void *st);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_decode 启动解码
 *
 * @param st
 * @param byteStrm
 * @param len
 * @param readBytes
 * @param dev
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u32 video_1_decode(void *st, u8 *byteStrm, u32 len,	u32 *readBytes, void *dev);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_decode_output 获取解码输出
 *
 * @param st
 * @param out
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u32 video_1_decode_output(void *st, struct video_1_output *out);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief video_1_register_alloc 注册内存管理
 *
 * @param malloc
 * @param free
 */
/* ------------------------------------------------------------------------------------*/
void video_1_register_alloc(void *(*malloc)(int size, u32 ram_type, u32 module_type), void (*free)(void *buf, u32 ram_type, u32 module_type));
#endif



