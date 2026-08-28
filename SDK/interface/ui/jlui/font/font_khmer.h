#ifndef __FONT_KHMER__H__
#define __FONT_KHMER__H__


typedef enum {
    khmer_topl,
    khmer_tops,
    khmer_belowl,
    khmer_belows,
    khmer_normol
} KHMER_CODE_TYPE;

u16 font_khmer_replace(struct font_info *info, u16 *unicbuf, u16 len);

#endif
