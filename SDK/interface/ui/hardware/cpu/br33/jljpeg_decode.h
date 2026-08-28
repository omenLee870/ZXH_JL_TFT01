/*----------------------------------------------*/
/* JL HW JEPG CODEC                             */
/*----------------------------------------------*/
#ifndef _JL_HW_JPGDEC
#define _JL_HW_JPGDEC
#include "typedef.h"
#include "generic/rect.h"


#ifdef __cplusplus
extern "C" {
#endif

//#define MCU_BUFF_NUM    1
#ifndef MCU_BUFF_NUM
#define MCU_BUFF_NUM    2
#endif
/* Allow the number of cached MUC blocks , Default to 2 , 默认双buff乒乓，支持多 buff 模式，最小可以设置为 1 */
/* 当上层触发解码动作的速度比解码速度快时，适当改大可以提高速度，比如直接设置整幅图片的总 MCU 数量, 即硬件直接解完，效率只受限于解码速度*/
/* 当上层触发解码动作的速度比解码速度慢时，使用 2 个buff即保证每次来取数据都有已经解码完成的内容，改大没有意义 */

//#define BIT_BUFF_NUM    1
#ifndef BIT_BUFF_NUM
#define BIT_BUFF_NUM    2
#endif
/* Allow the number of cached MUC blocks , Default to 2 , 双buff乒乓或者单buff阻塞使用 */
/* 大致情况和 MCU 类似，主要考虑到输入bit流需要时32位对齐的地址空间，原始 bit 流需要从 flash 或者 SD 卡之中进行拷贝到RAM中，因此建议乒乓操作, 更多的 buff 没有意义，因为取 bit 流的动作会比解码动作慢 */

//#define JDEC_SZBUF	        1024 * 8
#ifndef JDEC_SZBUF
#define JDEC_SZBUF          512
#endif
/* Specifies size of stream input buffer , >= 17 + 256 */

#define JDEC_MAGIC      "JLHWJPEG"

typedef enum {
    JPG_TEM = 0X01,			//算术编码中作临时之用

    //非层次哈夫曼编码
    JPIF_SOF0 = 0XC0,		//基线离散余弦变换/*只支持这种编码格式的图片*/
    JPIF_SOF1, 				//扩展顺序离散余弦变换
    JPIF_SOF2, 				//递进离散余弦变换
    JPIF_SOF3, 				//空间顺序无损

    JPIF_DHT = 0XC4,		//哈夫曼表

    //层次哈夫曼编码
    JPIF_SOF5, 				//差分离散余弦变换
    JPIF_SOF6, 				//差分层次离散余弦变换
    JPIF_SOF7, 				//差分空间无损

    //非层次算术编码
    JPIF_SOF8, 				//为JPEG扩展保留
    JPIF_SOF9, 				//扩展顺序离散余弦变换
    JPIF_SOF10, 			//递进离散余弦变换
    JPIF_SOF11, 			//空间顺序无损

    JPIF_DAC = 0XCC,		//算术编码表

    //层次算术编码
    JPIF_SOF13, 			//差分离散余弦变换
    JPIF_SOF14, 			//差分层次离散余弦变换
    JPIF_SOF15, 			//差分空间无损

    JPIF_RST0 = 0XD0,		//每隔ｎ个MCU 块就有一个RST标记
    JPIF_RST1,
    JPIF_RST2,
    JPIF_RST3,
    JPIF_RST4,
    JPIF_RST5,
    JPIF_RST6,
    JPIF_RST7 = 0XD7,

    JPIF_SOI = 0XD8,		//文件头
    JPIF_EOI = 0XD9,		//文件尾
    JPIF_SOS = 0XDA,		//扫描行开始
    JPIF_DQT = 0XDB,		//量化表
    JPIF_DNL = 0XDC,		//线数
    JPIF_DRI = 0XDD,		//重新开始间隔
    JPIF_DHP = 0XDE,		//层次级数
    JPIF_EXP = 0XDF,		//展开参考图像

    JPIF_APP0 = 0XE0,		//交换格式和图像识别信息
    JPIF_APP1,
    JPIF_APP2,
    JPIF_APP3,
    JPIF_APP4,
    JPIF_APP5,
    JPIF_APP6,
    JPIF_APP7,
    JPIF_APP8,
    JPIF_APP9,
    JPIF_APP10,
    JPIF_APP11,
    JPIF_APP12,
    JPIF_APP13,
    JPIF_APP14,
    JPIF_APP15,
    JPIF_COM = 0XFE,		//注释
    JPIF_MASK = 0XFF,
} JPG_TYPE;

/* Decode event code */
typedef enum {
    JDEC_INTER_MCU,	    /* 0: The specified number of MCU blocks finishes decoding */
    JDEC_INTER_BIT,	    /* 1: End of bit stream */
    JDEC_INTER_FINISH,	/* 2: End of file decoding */
} jdec_msg;

/* Error code */
typedef enum {
    JDEC_OK = 0,	/* 0: Succeeded */
    JDEC_INP,	/* 1: Device error or wrong termination of input stream */
    JDEC_MEM1,	/* 2: Memory request failed */
    JDEC_MEM2,	/* 3: Memory space not requested */
    JDEC_MEM3,  /* 4: Exceeds input buffer size : > JDEC_SZBUF */
    JDEC_FMT1,	/* 5: Data format error (may be broken data) */
    JDEC_FMT2,	/* 6: Right format but not supported */
    JDEC_FMT3,	/* 7: Not supported JPEG standard */
    JDEC_FMT4,  /* 8: Decoding execution error*/
    JDEC_STA,	/* 9: Jpeg work status error*/
    JDEC_TIMEOUT,/*10: DEC_TIMEOUT*/
    JDEC_HD_ERR,/*11:DEC_HD ERR*/
} jdec_err;

/* HW JPEG work state code */
typedef enum {
    JDEC_WORK_STATE_CLOSE = 0,	/* The jpeg decoding module is turned off */
    JDEC_WORK_STATE_INIT,		/* The jpeg decoding module is inited*/
    JDEC_WORK_STATE_OPEN,	    /* The jpeg decoding module is enabled */
    JDEC_WORK_STATE_START,	    /* jpeg decoding begins */
    JDEC_WORK_STATE_STOP,	    /* End of jpeg decoding */
    JDEC_WORK_STATE_SUSPEND,    /* Pause during jpeg decoding */
} jdec_work_state;

/* MCU/BIT buff state code */
typedef enum {
    JDEC_BUFF_STATE_INVALID = 0,	/* 0: The buff is invalid, and the data is outdated or invalid */
    JDEC_BUFF_STATE_UNDECODED,	    /* 1: The mcu block or bit stream that has not yet begun decoding */
    JDEC_BUFF_STATE_DECODING,	    /* 2: Wait for the mcu block or bit stream to decode */
    JDEC_BUFF_STATE_DECODED,	    /* 3: The decoding of the mcu block or bit stream is completed */
    JDEC_BUFF_STATE_COPY,			/* 4: MCU buf copy to line buf  */
} jdec_buff_state;

/* Mcu buff information structure */
typedef struct _jdec_muc_buff {
    jdec_buff_state	state;          /* mcu block usage status */
    u16 mcuid;                 		/* mcu block ID */
    u8 *buf;			        	/* data buffer */
} jdec_mcu_buff;

/* Bit buff information structure */
typedef struct _jdec_bit_buff {
    jdec_buff_state	state;          /* bit stream usage status */
    u32 len;                   		/* data len */
    u8 *buf;			        	/* data buffer */
} jdec_bit_buff;

/* Huffman table structs */
typedef struct _huff_tab {
    u16 *min;
    u8	*index;
    u8	*pval;
} huff_tab;

/* Decompressor object structure */
typedef struct _jdec_opj {
    void *magic;                            /* Decoder magic */

    jdec_work_state work_state: 4;            /* Working state of the jpeg decoder module */
    u8 msx: 4;
    u8 msy: 4;								/* MCU size in unit of block (width, height) */

    u8 ycnt: 2;                          	/* Y in the proportion of MCU components: 0:YUV444;1:YUV422;3:YUV411/YUV420 */

    u8 isr_flags: 4;						/* BIT0:MCU BIT2:BITS BIT3:LINE	BIT4:REV */
    u8 ncomp: 2;							/* Number of color components 1:grayscale, 3:color */
    u8 format: 1;							/*1:565 0:888*/
    u8 line_buf_vaild: 1;					/*line buf 有效位*/
    u32 rev: 10;								/* 预留拓展*/

    u16 wmcu_num;                      		/* The number of mcu blocks in the horizontal direction */
    u16 hmcu_num;                      		/* The number of mcu blocks in the vertical direction */

    u16 all_mcucnt;                    		/* The total number of mcu blocks of the picture */
    u16 curr_mcuid;           				/* The mcu block ID currently to be decodeid: 0 - (all_mcucnt - 1) */
    u16 start_mcuid;						/*单次解码的开始id*/
    u16 end_mcuid;							/*单次解码的结束id*/
    u16 mcu_cnt;                            /*单次解码 MCU 块的数量*/
    u16 mcu_size;                           /*单个 MCU 块的大小*/

    u8 qtid[3];								/* Quantization table ID of each component, Y, Cb, Cr */

    u16 width;								/* Size of the input image (pixel) */
    u16 height;								/* Size of the input image (pixel) */

    u32(*infunc)(struct _jdec_opj *, u8 *buf, u32 len);  /* Pointer to jpeg stream input function */
    void (*decode_msg)(struct _jdec_opj *, jdec_msg);   /* Pointer to the jpeg decoding event function */
    void (*decode_wait)(struct _jdec_opj *);    /* The decoder event waits for the callback */
    void *device;							/* Pointer to I/O device identifiler for the session */
    int curr_offset;						/*文件内偏移*/

    u8 *outbuf;								/* YUV422 to RGB buff: 这个 buff 从上层应用传下来装载数据 */
    huff_tab hufftbl[2][2];                 /* Huffman tables [id][dcac] */
    u16 *qt_table;
    s16 *qttbl[2];							/* Dequantizer tables [id] */
    u16 *std_huffman_table;
    u32 *huffman;
    jdec_mcu_buff mcu_buff[MCU_BUFF_NUM];	/* MCU_BUFF_NUM mcu block buffers: 这些 buff 负责暂存解码后的 MCU 块数据和 MCU 块信息 */
    jdec_bit_buff bit_buff[BIT_BUFF_NUM];	/* BIT_BUFF_NUM bit stream buffers: 这些 buff 负责暂存原始的 JPEG 数据或者 JPEG 段信息 */
    u8 *line_buf;							/*mcu解码后数据先暂存到linebuf再统一变换输出，同时缓存部分数据给下一次解码使用，避免重复*/
    struct rect draw_rect;					/*jpeg在屏幕的绘制区域*/
    struct rect jpeg_rect;					/*在jpeg解码的相对位置*/
    struct rect obuf_rect;					/*输出buffer的区域*/
    struct rect cover_rect;
    void *matrix;							/* affine matrix: 用于合成这个JPG时做GPU变换 */
    s16 surplus_line;						/* line buf 剩余未使用数据*/
    u32 bit_cnt_start;						/*位流起始地址*/
} jdec_opj;

extern const char jljpeg_magic[];  //识别解码器的标志字

/* JL JPEG DEC API functions */
void jljpeg_os_init(void);
void jljpeg_hw_config_stage1(jdec_opj *opj, bool os_flag);
jdec_err jljpeg_hw_config_stage2(void (*decode_msg)(struct _jdec_opj *, jdec_msg), void (*decode_wait)(struct _jdec_opj *), \
                                 uint16_t mcu_cnt, uint8_t *mcu_buff);
jdec_err jljpeg_hw_config_stage3(uint16_t x, uint16_t y, uint8_t *buf);
void jljpeg_hw_release(void);
void jljpeg_hw_resource_release(bool os_flag);
jdec_opj *jpeg_hw_get(void);
void jpeg_hw_set(jdec_opj *opj);

void jljpeg_hw_init(jdec_opj *opj);
void jljpeg_hw_reset(void);
void jljpeg_hw_restart(void);
void jljpeg_hw_mcubuff_reset(jdec_opj *opj, uint16_t mcu_cnt, uint8_t *mcu_buff);

void *jpeg_malloc(int size);
void jpeg_free(void *p);
#define JPEG_MALLOC(a)		jpeg_malloc(a)
#define JPEG_FREE(a) 		jpeg_free(a)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_img_info_get 读取文件头
 *
 * @param opj
 * @param infunc
 * @param dev
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
jdec_err jljpeg_img_info_get(jdec_opj *opj, u32(*infunc)(jdec_opj *opj, u8 *buf, u32 size), void *dev);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_decode_module_init jpeg 模块初始化
 *
 * @param malloc	申请
 * @param free		释放
 * @param gpu_blend 绘图
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/

int jpeg_decode_module_init(
    void *(*malloc)(int size, u32 ram_type, u32 module_type),
    void (*free)(void *p, u32 ram_type, u32 module_type),
    int (*gpu_blend)(u8 *dst_buf, struct rect *dst_rect, u8 *src_buf, struct rect *src_rect, struct rect *draw_rect, int src_stride, int format, void *p, int line_surpule, struct rect *cover)
);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_decode_init 初始化解码
 *
 * @param opj
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_decode_init(jdec_opj *opj);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_decode_set_callback 注册回调
 *
 * @param opj			句柄
 * @param infunc		输入数据流回调
 * @param decode_msg
 * @param decode_wait
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_decode_set_callback(jdec_opj *opj,
                               u32(*infunc)(struct _jdec_opj *opj, u8 *buf, u32 len),
                               void (*decode_msg)(struct _jdec_opj *, jdec_msg),
                               void (*decode_wait)(struct _jdec_opj *)
                              );

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_decode_start	启动jpeg解码
 *
 * @param opj					解码句柄
 * @param draw_rect				图片绘制区域
 * @param jpeg_rect				在jpeg中的解码区域
 * @param cover_rect			图片限制区域
 * @param obuf_rect				输出buf区域
 * @param dec_out_buf			输出buf
 *
 * @return	jdec_err
 */
/* ------------------------------------------------------------------------------------*/
jdec_err jljpeg_decode_start(jdec_opj *opj, struct rect *draw_rect, struct rect *jpeg_rect, struct rect *cover_rect, struct rect *obuf_rect, void *dec_out_buf);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_decode_release
 *
 * @param opj				解码句柄
 * @param global_hd_clr		将全局解码句柄置为NULL，需应用层free句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_decode_release(jdec_opj *opj, int global_hd_clr);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_get_decode_hd 将当前解码句柄置为NULL,需应用层free句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void jljpeg_get_decode_hd_clr();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_get_decode_hd 获取文件句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void *jljpeg_get_decode_hd();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_decode_reset_curr 重新解码当前文件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_decode_reset_curr();



#ifdef __cplusplus
}
#endif

#endif /* _JL_HW_JPGDEC */
