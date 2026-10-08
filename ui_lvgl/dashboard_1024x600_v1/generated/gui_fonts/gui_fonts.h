#ifndef GUIDER_FONTS_H
#define GUIDER_FONTS_H
#ifdef __cplusplus
extern "C" {
#endif

#if LVGL_VERSION_MAJOR == 7
#include "lv_font/lv_font.h"
#else
#include "src/font/lv_font.h"
#endif
#include "../common.h"

extern lv_font_t lv_font_MiSans_Medium_34;
extern lv_font_t lv_font_MiSans_Medium_36;
extern lv_font_t lv_font_MiSans_Medium_32;
extern lv_font_t lv_font_MiSans_Medium_18;
extern lv_font_t lv_font_lyrics_38;
extern lv_font_t lv_font_MiSans_Medium_14;
extern lv_font_t lv_font_MiSans_Medium_38;
extern lv_font_t lv_font_MiSans_Medium_120;
extern lv_font_t lv_font_MiSans_Medium_29;
extern lv_font_t lv_font_MiSans_Medium_31;
extern lv_font_t lv_font_MiSans_Medium_24;
extern lv_font_t lv_font_MiSans_Medium_26;
extern lv_font_t lv_font_MiSans_Medium_30;
extern lv_font_t lv_font_MiSans_Medium_40;
extern lv_font_t lv_font_Lato_Regular_20;
extern lv_font_t lv_font_Lato_Regular_88;
extern lv_font_t lv_font_Lato_Regular_44;
extern lv_font_t lv_font_NotoSansSC_Medium_20;
extern lv_font_t lv_font_Lato_Regular_24;
extern lv_font_t lv_font_montserratMedium_16;
extern lv_font_t lv_font_MiSans_Medium_12;
extern lv_font_t lv_font_lyrics_26;
extern lv_font_t lv_font_lyrics_20;
LV_FONT_DECLARE(lv_font_simsun_32);

#if LV_USE_GUIBUILDER_SIMULATOR && LV_USE_CUSTOM_FONT_FN
extern void init_gui_fonts();
extern void free_gui_fonts();
#else
extern GUI_WEAKREF void init_gui_fonts();
extern GUI_WEAKREF void free_gui_fonts();
#endif

#ifdef __cplusplus
}
#endif
#endif