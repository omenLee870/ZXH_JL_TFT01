#ifndef __FONT_UNIC_H__
#define __FONT_UNIC_H__

#include "generic/typedef.h"

/* 0xe00->0xe7f:Thai    Unicode编码范围 */
/* 0x900->0x97f:Indic   Unicode编码范围 */
/* 0x600->0x6ff:Arabic  Unicode编码范围 */
/* 0x590->0x5ff:Hebrew  Unicode编码范围 */
/* 0xf00->0xfff:Tibetan Unicode编码范围 */
enum unic_type {
    UNIC_ERR,
    UNIC_THAI,
    UNIC_INDIC,
    UNIC_ARABIC,
    UNIC_HEBREW,
    UNIC_TIBETAN,
    UNIC_MYANMAR,
    UNIC_BENGALI,
    UNIC_KHMER,
    UNIC_OTHER
};

enum myanmar_type {
    myanmar_top,
    myanmar_below,
    myanmar_103c,
    myanmar_half,
    myanmar_follow,
    myanmar_other,
};


int Font_GetCharBits(struct font_info *info, u16 unicode);
int Font_GetCharWidth(struct font_info *info, u16 unicode);
UnicInfo *Font_GetCharbuf(struct font_info *info, u16 unicode);
UnicInfo *Font_GetCharUnicInfo(struct font_info *info, u16 unicode);
bool InitFont_Unicode(struct font_info *info);
u16 TextOutW_Unicode(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

u16 TextOutW_UnicMixRightword(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
u16 TextOutW_UnicMixLeftword(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

u8 font_code_inrang(u16 code, const u16 *unicbuf, u16 len);
u16 font_left_drawbuf(struct font_info *info, u16(*buffer)[2], int *buffer_data, int *offset_xy);
u16 font_left_get_offset(struct font_info *info, u16(*buffer)[2], int *buffer_data, int left_width);
u16 font_left_get_width(struct font_info *info, u16(*buffer)[2], u16 len);

u16 ft_font_left_drawbuf(struct font_info *info, u16(*buffer)[2], int *buffer_data, int *offset_xy);
u16 ft_font_left_get_offset(struct font_info *info, u16(*buffer)[2], int *buffer_data, int left_width);
u16 ft_font_left_get_width(struct font_info *info, u16(*buffer)[2], u16 len);

//缅甸语
enum myanmar_type font_myanmar_get_type(u16 code);
void font_myanmar_update(u16 *unicbuf, u16 len);
u16 font_myanmar_replace(struct font_info *info, u16 *unicbuf, u16 len);
void find_syllables_myanmar(uint16_t *buffer, uint32_t len, uint8_t *syllable);
u16 font_myanmar_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);
u16 TextOutW_UnicMyanmar(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

//孟加拉语
u8 bengali_getBaseType(u16 base);
int bengali_getMarkIndex(u16 code);
u16 font_bengali_type4_replace(struct font_info *info, u16 *unicbuf, u16 len);
u16 font_bengali_type6_replace(struct font_info *info, u16 *unicbuf, u16 len);
void bengali_getOffset(u16 text, u16 prev_text, s8 *offsety);
u16 font_bengali_reorder(struct font_info *info, u16 *unicbuf, u16 len);
u16 font_bengali_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);
u16 TextOutW_UnicBengali(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

//高棉语
u16 font_khmer_get_mark_type(u16 *curr, u16 *prev, u16 *next);
u16 font_khmer_replace(struct font_info *info, u16 *unicbuf, u16 len);
u16 font_khmer_reorder(struct font_info *info, u16 *unicbuf, u16 len);
u16 font_khmer_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);
u16 TextOutW_UnicKhmer(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

//希伯来语
u16 TextOutW_Hebrew(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
u16 TextOutW_UnicHebrew(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

//藏语
u16 TextOutW_Tibetan(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
u16 TextOutW_UnicTibetan(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
u16 font_tibetan_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);
u16 TextW_TibetanReplace(struct font_info *info, u16 *str_buf, u16 len);

//印地语
u16 TextOutW_Indic(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
u16 TextOutW_UnicIndic(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
void find_syllables_indic(uint16_t *buffer, uint32_t len, uint8_t *syllable);
u16 TextW_IndicReorder(struct font_info *info, u8 *str, u16 len);
u16 TextW_IndicReplace(struct font_info *info, u8 *str, u16 len);
u16 font_indic_drawbuf(struct font_info *info, u16(*buffer)[2], u16 len, int x, int y, int *offset_xy);

#endif

