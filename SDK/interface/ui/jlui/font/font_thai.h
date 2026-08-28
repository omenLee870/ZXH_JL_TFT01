#ifndef __FONT_Thai_H__
#define __FONT_Thai_H__

#include "typedef.h"

typedef struct {
    u16 unicode;
    u16 ansi;
    u8  *buf;
    u8  attr: 4;
    u8  bigendian: 4;
    u16 width;
    u16 height;
} THaiASCSTRUCT;

#define    BASE_LEVEL   (1)
#define    ABOVE_LEVEL  (2)
#define    BELOW_LEVEL  (3)
#define    TOP_LEVEL	(4)

typedef enum {
    THAI_BASE_CODE,       //主体字符
    THAI_ABOVE_CODE,      //帽子字符
    THAI_BELOW_CODE,      //鞋子字符
    THAI_PHONETICS_CODE,  //音标字符
    THAI_OTHER_CODE       //独立字符
} THAI_CODE_TYPE;

THAI_CODE_TYPE font_thai_code_type(u16 unicode);

extern bool IsThaiOneWord_W(u8 *pstr, const int iSize, int *i, THaiASCSTRUCT *w2, THaiASCSTRUCT *w3, u8 unicode, u8 bigendian);
extern u8 GetThaiLanguageCharacterData(struct font_info *info, u16 asc, u8 **buf);
extern u8 ThaiLanguagecompose(struct font_info *info, u16 *width, THaiASCSTRUCT *w1, THaiASCSTRUCT *w2, THaiASCSTRUCT *w3);

u8 codetype(u16 utf);
extern u16 TextOut_Thai(struct font_info *info, u8 *str,  u16 len,  u16 x,  u16 y);
extern u16 TextOutW_Thai(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_UnicThai(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 font_thai_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);
#endif
