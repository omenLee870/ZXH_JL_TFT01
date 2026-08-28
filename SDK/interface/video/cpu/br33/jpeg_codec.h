#ifndef _JPEG_CODEC_H_
#define _JPEG_CODEC_H_
#include "generic/typedef.h"
#include "generic/list.h"
#include "system/task.h"
#include "video_info.h"

#define jpg_be16_to_cpu(x) (((x)[0] << 8) | (x)[1]) //大端16位字节序转换, 将2字节大端数据转为CPU本机序

//JPEG编解码器工作状态枚举
enum codec_state {
    JPEG_CODEC_IDLE = 0x0,  //空闲状态

    //ENC
    //AUTO
    JPEG_CODEC_ENC_AUTO,    //单通道自动编码
    JPEG_CODEC_NCH_AUTO,    //多通道自动编码
    //MANUAL
    JPEG_CODEC_ENC_MANU,    //单通道手动编码
    JPEG_CODEC_NCH_MANU,    //多通道手动编码(dri整帧输出首选)
    JPEG_CODEC_JX_MANU,     //分块手动编码

    //DEC
    JPEG_CODEC_DEC_MANU,    //单通道手动解码
    JPEG_CODEC_GXDEC_MANU,  //GXDEC手动解码

};

//JPEG编解码器硬件通道枚举
enum codec_channel {
    JPEG_CH_AUTO = 0x0, //默认不配置的话是0, 由程序自动选择CH
    //硬件模块JPG0
    JPEG_CH_NCH0,   //硬件模块JPG_NCH通道0
    JPEG_CH_NCH1,   //硬件模块JPG_NCH通道1
};

//MJPG中断码枚举
enum mjpg_irq_code {
    MJPG_IRQ_NO_BUFF,   //当前申请不到新的buf
    MJPG_IRQ_YUV_PEND,  //YUV处理完成中断
    MJPG_IRQ_PAUSE_PEND,    //暂停处理中断
    MJPG_IRQ_SPEED_PEND,    //速率不足中断
    MJPG_IRQ_MCU_PEND,  //MCU处理完成中断
    MJPG_IRQ_IN_BUFF_END,   //分段式解码用, 当前位流使用完
    MJPG_IRQ_IN_BUFF_ERR,   //整段解码用, 用完位流但是mcu不足, 或者位流有误
    MJPG_IRQ_TIMEOUT,   //信号量超时
    MJPG_IRQ_OUT_BUFF_FULL, //当前buf已用完(用于没有ops情况)
};

//MJPG帧类型枚举
enum mjpg_frame_type {
    MJPG_STREAM_FRAME,  //视频流(自动)
    MJPG_SINGLE_FRAME,  //单帧(手动)
};

//MJPG用户操作回调接口
struct mjpg_user_ops {
    void *(*malloc)(void *_video, u32 size);           //内存分配回调
    void *(*realloc)(void *_video, void *fb, int newsize); //内存重分配回调
    void (*free)(void *_video, void *fb);              //内存释放回调
    int (*size)(void *fb);                             //获取buffer大小回调
    int (*free_size)(void *_video);                    //获取剩余可用空间大小回调
    void (*output_frame_end)(void *_video, void *buf, u32 size); //帧输出完成回调
    int (*output_buf_full)(void *_video, void *buf, u32 size);   //输出buf已满回调
};

//JPEG解码YUV输出数据
struct jpeg_yuv {
    //读写
    u8 *y;          //Y分量数据指针

    //只读
    u32 y_len;  //当前次输出Y长度
    u32 uv_len; //当前次输出UV长度
    u8 *uv;     //UV分量数据指针, YUV420 LINE模式存储UV地址
    u32 line;   //当前次输出行数
    u32 width;  //YUV数据实际输出宽度
    u32 height; //YUV数据实际输出高度
};

//MJPG图片信息
struct mjpg_pic_info {
    u8 *jpeg_data;  //JPEG数据指针
    u32 jpeg_len;   //JPEG数据长度
};

/***************************
数据类型 (Type)	    1个元素的长度
2 (ASCII)	        1 字节
3 (SHORT)	        2 字节
4 (LONG)	        4 字节
5 (RATIONAL)	    8 字节
****************************/
//APP1条目配置
typedef struct {
    u16 tag;       // 标签 ID (如 0x010F 为 Make)
    u16 type;      // 数据类型 (2:ASCII, 3:SHORT, 4:LONG, 5:RATIONAL)
    u32 count;     // 数据个数 (如字符串长度)
    const void *data;   // 指向实际数据的指针
    u32 data_len;  // 数据的字节长度(数据个数X元素长度)
} exif_entry;

//MJPG编码单帧属性配置
struct mjpg_enc_s_attr {
    u8 nch_ch;  //获取当前NCH通道号

    struct jpg_q_table *qt; //量化表指针
    u8 quality; //图片编码质量
    u8 samp_fmt; //采样格式(yuv420/yuv422/yuv444)
    u8 order_mode; //字节排列模式(YUV422PACK: 0:YUYV 1:UYVY; RGB: 0:小端 1:大端)
    u8 adr_mode;   //地址模式(0:tile_base 1:line_base)

    //RGB
    u8 rgb_bt601;  //RGB色彩标准(0:BT709 1:BT601)

    u16 width;      //图片宽度
    u16 height;     //图片高度
    u16 stride;     //像素步进

    u32 abr_kbps;   //图片码率(kbps)
    u32 fps;        //帧率

    u32 jpeg_dri_mcu_cnt;  //JPEG分段位流MCU配置
    u32 file_size;  //编码后图片大小, 多用于清除上一帧图片大小信息

    u8 whole_frame_flag;    //编码一整帧图片置1
    u8 auto_flush;          //自动刷新标志
    u8 auto_dri;            //自动DRI标志

    //JXENC
    u8 block;  //分块大小(0:32x16 1:32x32)
    u8 thumbnail_create;    //缩略图编码标志(1:当前编码的是缩略图,无APP1)
    u8 *head_app1;          //APP1头数据指针
    u32 head_app1_len;      //APP1头数据长度
};

//MJPG解码单帧属性配置
struct mjpg_dec_s_attr {
    u8  *cbuf;      //解码输出缓冲区指针
    int line_cnt;   //行计数

    u8  order_mode; //字节排列模式(YUV422PACK: 0:YUYV 1:UYVY)
    u8  adr_mode;   //地址模式(0:tile_base 1:line_base)
    u8 whole_frame_flag;    //解码一整帧图片置1

    u8 auto_dri;    //自动DRI标志
};

//JPEG基本图像信息
struct jpeg_basic_info {
    int width;         //图像宽度
    int height;        //图像高度
    int align_width;   //对齐后宽度
    int align_height;  //对齐后高度
    int format;        //像素格式
};

//JPEG编解码器硬件句柄
struct jpeg_codec_handle {
    volatile int state; //当前硬件模块工作模式
    spinlock_t lock;    //状态切换自旋锁
    struct list_head encoder;   //已申请的编码句柄链表
    struct list_head decoder;   //已申请的解码句柄链表
    int (*frame_end)(void *priv);   //帧间处理函数, 用于资源回收
    OS_SEM sem; //硬件模块空闲状态控制信号量
    u8 nch_bitmap;  //NCH通道使用位图
    u8 nch_init;    //编码器NCH初始化状态
};

/********************************************************/
/*
 * jpeg enum
 * JPEG枚举
 */

//JPEG YUV像素格式枚举
typedef enum {
    JPG_FORMAT_YUV444 = 0, //YUV444格式
    JPG_FORMAT_YUV422,     //YUV422格式
    JPG_FORMAT_YUV420,     //YUV420格式
    JPG_FORMAT_A,          //A格式
    JPG_FORMAT_BGR, //仅用于编码, 解码无效
    JPG_FORMAT_RGB, //仅用于编码, 解码无效
} jpg_yuv_format_t;

//JPEG排列模式枚举
typedef enum {
    JPG_TILE_BASE = 0, //Tile排列
    JPG_LINE_BASE,     //Line排列
} jpg_adr_mode_t;

//JPEG字节排列模式枚举
typedef enum {
    JPG_ORDER_YUYV = 0, //YUYV排列
    JPG_ORDER_UYVY,     //UYVY排列
    JPG_RGB_SMALL_END = 0,  //RGB小端排列
    JPG_RGB_BIG_END,        //RGB大端排列
} jpg_order_mode_t;

//JPEG RGB色彩标准枚举
typedef enum {
    JPG_BT709 = 0, //BT709色彩标准
    JPG_BT601,     //BT601色彩标准
} jpg_rgb_mode_t;

//JXENC分块大小枚举
typedef enum {
    JPG_BLOCK_32X16 = 0, //分块大小32x16
    JPG_BLOCK_32X32,     //分块大小32x32
} jpg_block_size_t;


typedef enum {
    JPG_QTABLE_YUV = 0,
    JPG_QTABLE_A,
} jpg_dec_qtable_t;

/*
 * jpeg encode
 * JPEG编码使用函数
 */

int mjpg_enc_get_s_attr(void *_fh, struct mjpg_enc_s_attr *attr);   //获取编码单帧属性
int mjpg_enc_set_s_attr(void *_fh, struct mjpg_enc_s_attr *attr);   //设置编码单帧属性
void mjpg_enc_set_input_buf(void *_fh, struct video_cap_buffer *b); //设置编码输入缓冲区
void mjpg_enc_set_output_ops(void *_fh, void *priv, const struct mjpg_user_ops *ops); //设置编码输出回调操作接口
void mjpg_enc_set_irq_handler(void *_fh, void *priv,
                              int (*handler)(void *, enum mjpg_irq_code, enum mjpg_frame_type)); //设置编码中断处理函数
int mjpg_enc_start(void *_fh, u8 force);        //启动自动编码(force:是否强制启动)
int mjpg_enc_restart(void *_fh);                 //重启编码
int mjpg_enc_manual_start(void *_fh, struct yuv_frame_data *input_frame, u8 *bits_buf, int buf_len, u8 q_val); //启动手动编码(指定输入帧、输出buf、buf长度、质量值)
int mjpg_enc_stop(void *_fh);                    //停止编码
int mjpg_enc_pause(void *_fh);                   //暂停编码
int mjpg_enc_close(void *_fh);                   //关闭编码
int mjpg_start_next_frame(void *_fh);            //启动下一帧编码
int mjpg_enc_resume_bits(void *_fh, void *buf);  //恢复位流编码
void mjpg_enc_reset_bits_rate(void *_fh, u32 bits_rate); //重置编码码率
void mjpg_enc_check_space(void *_fh, u32 size);  //检查编码剩余空间
int mjpg_generate_app1(u8 *buffer, exif_entry *entries, int entry_count, u8 *thumb_data, u32 thumb_len) ; //生成APP1 EXIF数据(含缩略图)

/*
 * jpeg decode
 * JPEG解码使用函数
 */

int mjpg_dec_get_s_attr(void *_fh, struct mjpg_dec_s_attr *attr);   //获取解码单帧属性
int mjpg_dec_set_s_attr(void *_fh, struct mjpg_dec_s_attr *attr);   //设置解码单帧属性
void mjpg_dec_set_irq_handler(void *_fh, void *priv,
                              int (*handler)(void *, enum mjpg_irq_code, enum mjpg_frame_type)); //设置解码中断处理函数
int mjpg_dec_manual_start(void *_fh, struct jpeg_yuv *yuv, struct mjpg_pic_info *info); //启动手动解码(指定YUV输出和JPEG图片信息)
int mjpg_dec_stop(void *_fh);                    //停止解码
int mjpg_dec_pause(void *_fh);                   //暂停解码
int mjpg_dec_close(void *_fh);                   //关闭解码
s16 *get_dec_qtable(void);                      //获取解码用的转换后的Qtable
void set_dec_qtable(u8 *data, u8 q_val, jpg_dec_qtable_t type);   //转换Qtable, 用于解码 type: 0->YUV 1->A(透明度)


/*
 * jpeg common
 * JPEG通用函数
 */

char *jpg_format_str(int format);          //获取像素格式的字符串描述
char *jpg_adr_str(int adr);                //获取地址模式的字符串描述
char *jpg_yuv_order_str(int order);        //获取YUV排列顺序的字符串描述
char *jpg_rgb_order_str(int order);        //获取RGB排列顺序的字符串描述
void *mjpg_codec_open(enum codec_state mode, u8 ch); //打开JPEG编解码器(指定模式和通道)
float jpg_format_factor(int format);       //获取像素格式的压缩因子
u8 *find_jpg_frame(u8 *buf, int limit, struct jpeg_basic_info *info); //在缓冲区中查找JPEG帧并获取基本图像信息

void jpg_yuv_cbuf_max();                   //获取YUV缓存最大值

#endif

