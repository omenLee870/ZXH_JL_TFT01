#ifndef __FONT_ALL_H__
#define __FONT_ALL_H__

#include "generic/typedef.h"
#include "font/font_sdfs.h"

typedef struct {
    u8  width;
    u8  size;
    u16 addr;
} ASCSTRUCT;

typedef struct {
    unsigned int width: 6;
    unsigned int height: 6;
    unsigned int addr: 20;
} ASCSTRUCTV2;

//标志位
#define FONT_GET_WIDTH    		0x01 /* 获取文本宽度 */
#define FONT_SHOW_PIXEL   		0x02 /* 显示单行文本 */
#define FONT_SHOW_MULTI_LINE 	0x04 /* 显示多行文本(默认显示一行) */
#define FONT_SHOW_SCROLL    	0x08 /* 滚动显示*/
#define FONT_HIGHLIGHT_SCROLL 	0x10 /* 高亮滚动显示*/
#define FONT_VERTICAL_SCROLL 	0x20 /* 垂直滚动显示*/
#define FONT_SCROLL_CIRCULAR	0x40 /* 首尾相接的环形滚动*/
#define FONT_DEFAULT			(FONT_SHOW_PIXEL)

#define FONT_ENCODE_ANSI		0x00 /* 内码格式 */
#define FONT_ENCODE_UNICODE		0x01 /* unicode编码格式 */
#define FONT_ENCODE_UTF8		0x02 /* utf8编码格式 */

#define FONT_ENDIAN_BIG			0x00 /* utf16 大端 */
#define FONT_ENDIAN_SMALL		0x01 /* utf16 小端 */


extern const int FONT_USE_PTR;
extern const int ARABIC_MODE_SWITCH;
extern const int HEBREW_MODE_SWITCH;
extern const int THAI_MODE_SWITCH;
extern const int MYANMAR_MODE_SWITCH;
extern const int BENGALI_MODE_SWITCH;
extern const int KHMER_MODE_SWITCH;
extern const int INDIC_MODE_SWITCH;
extern const int TIBETAN_MODE_SWITCH;
extern const int MIXLEFT_MODE_SWITCH;
extern const int MIXRIGHT_MODE_SWITCH;
extern const int FONT_UNIC_SWITCH;
extern const int FONT_FREETYPE_SWITCH;

struct font_file {
    char *name;
    FILE *fd;
};

struct font {
    struct font_file file;
    u16 nbytes;
    u8 size;
    u8 version;
    u8 *pixelbuf;
};

struct dispbuf {
    int format;
    u32 color;
    void *rect;
    void *map;
};

enum FONT_STATUS {
    FT_ERROR_NONE,
    FT_ERROR_NOPIXFILE = 0x01,			//没有字模文件
    FT_ERROR_NOASCPIXFILE = 0x02,		//没有ASCII字模文件
    FT_ERROR_NOTABFILE = 0x04,			//没有TAB文件
    FT_ERROR_NOMEM = 0x08,				//内存不足
    FT_ERROR_CODEPAGE = 0x10,			//代码页错误
};

enum BIT_DEPTH {
    BIT_DEPTH_1BPP,
    BIT_DEPTH_2BPP,
    BIT_DEPTH_4BPP,
    BIT_DEPTH_8BPP,
};

struct font_info {
    struct font ascpixel;		//ASCII像素
    struct font pixel;			//像素
    struct font_file tabfile;	//UNICODE转内码文件
    struct font_file extfile;	//UNICODE转内码文件
    u8 sta;						//状态
    u8 ratio;					//放大倍数,默认为1
    u8 language_id;				//语言ID
    u8 bigendian;				//大端模式(unicode编码)
    u8 isgb2312;                //是否GB2312,用以区分GBK以及GB2312字库
    u8 codepage;				//代码页
    u16 x;
    u16 y;
    u16 text_width;				//文本宽度
    u16 text_height;			//文本高度
    u16 string_width;			//字符串宽度
    u16 string_height;			//字符串高度
    u16 offset;                 //显示偏移
    u32 flags;					//标志位

    u8 *text_image_buf;         //文本点阵缓存buf
    u32 text_image_buf_size;    //文本点阵缓存buf size
    u32 text_image_stride;      //文本点阵缓存buf对齐宽度
    u16 text_image_width;       //文本点阵缓存buf实际宽度
    u16 text_image_height;      //文本点阵缓存buf实际高度

    struct dispbuf disp;		//显示相关信息
    void (*putchar)(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y);
    void *dc;
    u16 line_num;
    u16 default_code;   //字库文件中不存在待显示字符时的默认替换字符编码
    void *text;
    short word_space;   //补充字间距
    short line_space;   //补充行间距
    short xpos_offset;
    short extra_word_space_for_dep;
    u32 tool_version;
    int top_extra_fill : 16;
    int bottom_extra_fill : 15;
    int mix_start : 1;
    u16 *each_line_width_info;
    /* >>>>>>>>>>> FONT FREETYPE >>>>>>>>>>> */
    char *ft_filename;
    u8 fontsize;
    u8 alpha;
    u16 rgb565;
    u32 task_id;
    u32 elm_id;
    void *gpu_head;
    /* <<<<<<<<<<< FONT FREETYPE <<<<<<<<<<< */
};

typedef struct {
    u32 width : 6;
    u32 height : 7;
    s32 top : 8;
    u32 size : 11;
    s32 left : 7;
    u32 addr : 25;
    u8 advance_x;
} __attribute__((packed, aligned(1))) UnicInfo_new;

typedef struct {
    u8 width;
    u8 height;
    s8 left;
    s8 top;
    u32 addr;
} __attribute__((packed, aligned(1))) UnicInfo_old;

typedef struct {
    u8 width;
    u8 height;
    s8 left;
    s8 top;
    u8 advance_x;
} __attribute__((packed, aligned(1))) UnicInfo;

#define font_ntohl(x) (unsigned long)((x>>24)|((x>>8)&0xff00)|(x<<24)|((x&0xff00)<<8))
#define font_ntoh(x) (unsigned short int )((x>>8&0x00ff)|x<<8&0xff00)

extern const struct font_info font_info_table[];


typedef struct {
    u8 codepage;
    u32 ansi_offset;
    u32 table_offset;
} LANG_TABLE;


struct font_new_cb_priv {
    u8 *pixbuf;
    u16 color;
    short x;
    short y;
    u16 width;
    u16 height;
    u16 bufsize;
    u16 dis_width;
    u16 scroll_offset;
};

#define     CP874    (1)
#define     CP937    (2)
#define     CP1250   (3)
#define     CP1251   (4)
#define     CP1252   (5)
#define     CP1253   (6)
#define     CP1254   (7)
#define     CP1255   (8)
#define     CP1256   (9)
#define     CP1257   (10)
#define     CP1258   (11)
#define     CPKSC    (12)
#define     CPSJIS   (13)
#define     CPBIG5   (14)
#define     INDIC    (15)
#define     TIBETAN  (16)

/**
 * @brief 设置内码偏移表,混合内码字库需要 *
 * @param table 内码偏移表地址
 * @returns 0
 */
extern int font_set_offset_table(const LANG_TABLE *table);

/* 内码偏移表 */
extern const LANG_TABLE *lange_info_table;


/**
 * @brief 获取字库位深
 *
 * @returns 字库位深
 */
extern int Font_GetBitDepth();

/***************************************************************/

#include "font/font_textout.h"

#endif

