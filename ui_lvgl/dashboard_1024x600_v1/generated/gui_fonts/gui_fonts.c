#include "lvgl.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include "./gui_fonts.h"
#include "../common.h"

lv_font_t lv_font_MiSans_Medium_34;
lv_font_t * lv_font_MiSans_Medium_34_ptr;
void lv_font_MiSans_Medium_34_file()
{
  if (lv_font_MiSans_Medium_34_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_34_ptr);
    lv_font_MiSans_Medium_34_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_34, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_34_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000000.rle");
#else
  lv_font_MiSans_Medium_34_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_34_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_34_ptr != NULL) {
    lv_font_MiSans_Medium_34 = *lv_font_MiSans_Medium_34_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000000.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_34_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_34 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_36;
lv_font_t * lv_font_MiSans_Medium_36_ptr;
void lv_font_MiSans_Medium_36_file()
{
  if (lv_font_MiSans_Medium_36_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_36_ptr);
    lv_font_MiSans_Medium_36_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_36, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_36_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000001.rle");
#else
  lv_font_MiSans_Medium_36_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_36_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_36_ptr != NULL) {
    lv_font_MiSans_Medium_36 = *lv_font_MiSans_Medium_36_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000001.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_36_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_36 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_32;
lv_font_t * lv_font_MiSans_Medium_32_ptr;
void lv_font_MiSans_Medium_32_file()
{
  if (lv_font_MiSans_Medium_32_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_32_ptr);
    lv_font_MiSans_Medium_32_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_32, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_32_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000002.rle");
#else
  lv_font_MiSans_Medium_32_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_32_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_32_ptr != NULL) {
    lv_font_MiSans_Medium_32 = *lv_font_MiSans_Medium_32_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000002.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_32_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_32 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_18;
lv_font_t * lv_font_MiSans_Medium_18_ptr;
void lv_font_MiSans_Medium_18_file()
{
  if (lv_font_MiSans_Medium_18_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_18_ptr);
    lv_font_MiSans_Medium_18_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_18, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_18_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000003.rle");
#else
  lv_font_MiSans_Medium_18_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_18_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_18_ptr != NULL) {
    lv_font_MiSans_Medium_18 = *lv_font_MiSans_Medium_18_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000003.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_18_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_18 = *LV_FONT_DEFAULT;
  }
}
lv_font_info_t lv_font_lyrics_38_font_info;
lv_font_t lv_font_lyrics_38;
lv_font_t * lv_font_lyrics_38_ptr;
void lv_font_lyrics_38_file()
{
  if (lv_font_lyrics_38_ptr != NULL) {
    lv_freetype_font_delete(lv_font_lyrics_38_ptr);
    lv_font_lyrics_38_ptr = NULL;
    lv_memset(&lv_font_lyrics_38_font_info, 0, sizeof(lv_font_info_t));
    lv_memset(&lv_font_lyrics_38, 0, sizeof(lv_font_t));
  }
  lv_freetype_init_font_info(&lv_font_lyrics_38_font_info);
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_lyrics_38_font_info.name = "D:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\font\\lyrics.ttf";
#else
  lv_font_lyrics_38_font_info.name = gui_get_res_path(GUI_RES_LYRICS_TTF_FLASH_FILE);
#endif
  lv_font_lyrics_38_font_info.size = 38;
  lv_font_lyrics_38_font_info.render_mode = LV_FREETYPE_FONT_RENDER_MODE_BITMAP;
  lv_font_lyrics_38_font_info.style = LV_FREETYPE_FONT_STYLE_NORMAL;
  lv_font_lyrics_38_ptr = lv_freetype_font_create_with_info(&lv_font_lyrics_38_font_info);
  if (lv_font_lyrics_38_ptr == NULL) {
    LV_LOG_ERROR("Create Font Failed %s\n", lv_font_lyrics_38_font_info.name);
    lv_memset(&lv_font_lyrics_38_font_info, 0, sizeof(lv_font_info_t));
    lv_memset(&lv_font_lyrics_38, 0, sizeof(lv_font_t));
    lv_font_lyrics_38 = *LV_FONT_DEFAULT;
  } else {
    lv_font_lyrics_38 = *lv_font_lyrics_38_ptr;
  }
}
lv_font_t lv_font_MiSans_Medium_14;
lv_font_t * lv_font_MiSans_Medium_14_ptr;
void lv_font_MiSans_Medium_14_file()
{
  if (lv_font_MiSans_Medium_14_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_14_ptr);
    lv_font_MiSans_Medium_14_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_14, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_14_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000005.rle");
#else
  lv_font_MiSans_Medium_14_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_14_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_14_ptr != NULL) {
    lv_font_MiSans_Medium_14 = *lv_font_MiSans_Medium_14_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000005.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_14_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_14 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_38;
lv_font_t * lv_font_MiSans_Medium_38_ptr;
void lv_font_MiSans_Medium_38_file()
{
  if (lv_font_MiSans_Medium_38_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_38_ptr);
    lv_font_MiSans_Medium_38_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_38, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_38_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000006.rle");
#else
  lv_font_MiSans_Medium_38_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_38_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_38_ptr != NULL) {
    lv_font_MiSans_Medium_38 = *lv_font_MiSans_Medium_38_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000006.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_38_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_38 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_120;
lv_font_t * lv_font_MiSans_Medium_120_ptr;
void lv_font_MiSans_Medium_120_file()
{
  if (lv_font_MiSans_Medium_120_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_120_ptr);
    lv_font_MiSans_Medium_120_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_120, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_120_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000007.rle");
#else
  lv_font_MiSans_Medium_120_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_120_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_120_ptr != NULL) {
    lv_font_MiSans_Medium_120 = *lv_font_MiSans_Medium_120_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000007.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_120_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_120 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_29;
lv_font_t * lv_font_MiSans_Medium_29_ptr;
void lv_font_MiSans_Medium_29_file()
{
  if (lv_font_MiSans_Medium_29_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_29_ptr);
    lv_font_MiSans_Medium_29_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_29, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_29_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000008.rle");
#else
  lv_font_MiSans_Medium_29_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_29_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_29_ptr != NULL) {
    lv_font_MiSans_Medium_29 = *lv_font_MiSans_Medium_29_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000008.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_29_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_29 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_31;
lv_font_t * lv_font_MiSans_Medium_31_ptr;
void lv_font_MiSans_Medium_31_file()
{
  if (lv_font_MiSans_Medium_31_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_31_ptr);
    lv_font_MiSans_Medium_31_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_31, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_31_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000009.rle");
#else
  lv_font_MiSans_Medium_31_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_31_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_31_ptr != NULL) {
    lv_font_MiSans_Medium_31 = *lv_font_MiSans_Medium_31_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000009.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_31_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_31 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_24;
lv_font_t * lv_font_MiSans_Medium_24_ptr;
void lv_font_MiSans_Medium_24_file()
{
  if (lv_font_MiSans_Medium_24_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_24_ptr);
    lv_font_MiSans_Medium_24_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_24, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_24_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000a.rle");
#else
  lv_font_MiSans_Medium_24_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_24_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_24_ptr != NULL) {
    lv_font_MiSans_Medium_24 = *lv_font_MiSans_Medium_24_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000a.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_24_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_24 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_26;
lv_font_t * lv_font_MiSans_Medium_26_ptr;
void lv_font_MiSans_Medium_26_file()
{
  if (lv_font_MiSans_Medium_26_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_26_ptr);
    lv_font_MiSans_Medium_26_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_26, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_26_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000b.rle");
#else
  lv_font_MiSans_Medium_26_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_26_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_26_ptr != NULL) {
    lv_font_MiSans_Medium_26 = *lv_font_MiSans_Medium_26_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000b.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_26_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_26 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_30;
lv_font_t * lv_font_MiSans_Medium_30_ptr;
void lv_font_MiSans_Medium_30_file()
{
  if (lv_font_MiSans_Medium_30_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_30_ptr);
    lv_font_MiSans_Medium_30_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_30, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_30_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000c.rle");
#else
  lv_font_MiSans_Medium_30_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_30_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_30_ptr != NULL) {
    lv_font_MiSans_Medium_30 = *lv_font_MiSans_Medium_30_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000c.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_30_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_30 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_MiSans_Medium_40;
lv_font_t * lv_font_MiSans_Medium_40_ptr;
void lv_font_MiSans_Medium_40_file()
{
  if (lv_font_MiSans_Medium_40_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_40_ptr);
    lv_font_MiSans_Medium_40_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_40, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_MiSans_Medium_40_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000d.rle");
#else
  lv_font_MiSans_Medium_40_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MISANS_MEDIUM_40_RLE_FLASH));
#endif
  if (lv_font_MiSans_Medium_40_ptr != NULL) {
    lv_font_MiSans_Medium_40 = *lv_font_MiSans_Medium_40_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000d.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MISANS_MEDIUM_40_RLE_FLASH));
#endif
    lv_font_MiSans_Medium_40 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_Lato_Regular_20;
lv_font_t * lv_font_Lato_Regular_20_ptr;
void lv_font_Lato_Regular_20_file()
{
  if (lv_font_Lato_Regular_20_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_20_ptr);
    lv_font_Lato_Regular_20_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_20, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_Lato_Regular_20_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000e.rle");
#else
  lv_font_Lato_Regular_20_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_LATO_REGULAR_20_RLE_FLASH));
#endif
  if (lv_font_Lato_Regular_20_ptr != NULL) {
    lv_font_Lato_Regular_20 = *lv_font_Lato_Regular_20_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000e.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_LATO_REGULAR_20_RLE_FLASH));
#endif
    lv_font_Lato_Regular_20 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_Lato_Regular_88;
lv_font_t * lv_font_Lato_Regular_88_ptr;
void lv_font_Lato_Regular_88_file()
{
  if (lv_font_Lato_Regular_88_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_88_ptr);
    lv_font_Lato_Regular_88_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_88, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_Lato_Regular_88_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000f.rle");
#else
  lv_font_Lato_Regular_88_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_LATO_REGULAR_88_RLE_FLASH));
#endif
  if (lv_font_Lato_Regular_88_ptr != NULL) {
    lv_font_Lato_Regular_88 = *lv_font_Lato_Regular_88_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\3500000f.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_LATO_REGULAR_88_RLE_FLASH));
#endif
    lv_font_Lato_Regular_88 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_Lato_Regular_44;
lv_font_t * lv_font_Lato_Regular_44_ptr;
void lv_font_Lato_Regular_44_file()
{
  if (lv_font_Lato_Regular_44_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_44_ptr);
    lv_font_Lato_Regular_44_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_44, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_Lato_Regular_44_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000010.rle");
#else
  lv_font_Lato_Regular_44_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_LATO_REGULAR_44_RLE_FLASH));
#endif
  if (lv_font_Lato_Regular_44_ptr != NULL) {
    lv_font_Lato_Regular_44 = *lv_font_Lato_Regular_44_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000010.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_LATO_REGULAR_44_RLE_FLASH));
#endif
    lv_font_Lato_Regular_44 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_NotoSansSC_Medium_20;
lv_font_t * lv_font_NotoSansSC_Medium_20_ptr;
void lv_font_NotoSansSC_Medium_20_file()
{
  if (lv_font_NotoSansSC_Medium_20_ptr != NULL) {
    lv_font_free_bin(lv_font_NotoSansSC_Medium_20_ptr);
    lv_font_NotoSansSC_Medium_20_ptr = NULL;
    lv_memset(&lv_font_NotoSansSC_Medium_20, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_NotoSansSC_Medium_20_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000011.rle");
#else
  lv_font_NotoSansSC_Medium_20_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_NOTOSANSSC_MEDIUM_20_RLE_FLASH));
#endif
  if (lv_font_NotoSansSC_Medium_20_ptr != NULL) {
    lv_font_NotoSansSC_Medium_20 = *lv_font_NotoSansSC_Medium_20_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000011.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_NOTOSANSSC_MEDIUM_20_RLE_FLASH));
#endif
    lv_font_NotoSansSC_Medium_20 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_Lato_Regular_24;
lv_font_t * lv_font_Lato_Regular_24_ptr;
void lv_font_Lato_Regular_24_file()
{
  if (lv_font_Lato_Regular_24_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_24_ptr);
    lv_font_Lato_Regular_24_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_24, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_Lato_Regular_24_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000012.rle");
#else
  lv_font_Lato_Regular_24_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_LATO_REGULAR_24_RLE_FLASH));
#endif
  if (lv_font_Lato_Regular_24_ptr != NULL) {
    lv_font_Lato_Regular_24 = *lv_font_Lato_Regular_24_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000012.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_LATO_REGULAR_24_RLE_FLASH));
#endif
    lv_font_Lato_Regular_24 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_montserratMedium_16;
lv_font_t * lv_font_montserratMedium_16_ptr;
void lv_font_montserratMedium_16_file()
{
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_16_ptr);
    lv_font_montserratMedium_16_ptr = NULL;
    lv_memset(&lv_font_montserratMedium_16, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_montserratMedium_16_ptr = lv_font_load_bin("A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000013.rle");
#else
  lv_font_montserratMedium_16_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_16_RLE_FLASH));
#endif
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_montserratMedium_16 = *lv_font_montserratMedium_16_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\sdk\\ui_res\\flash\\rle\\font\\35000013.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_16_RLE_FLASH));
#endif
    lv_font_montserratMedium_16 = *LV_FONT_DEFAULT;
  }
}

GUI_WEAK void init_gui_fonts()
{
  lv_font_MiSans_Medium_34_file();
  lv_font_MiSans_Medium_36_file();
  lv_font_MiSans_Medium_32_file();
  lv_font_MiSans_Medium_18_file();
  lv_font_lyrics_38_file();
  lv_font_MiSans_Medium_14_file();
  lv_font_MiSans_Medium_38_file();
  lv_font_MiSans_Medium_120_file();
  lv_font_MiSans_Medium_29_file();
  lv_font_MiSans_Medium_31_file();
  lv_font_MiSans_Medium_24_file();
  lv_font_MiSans_Medium_26_file();
  lv_font_MiSans_Medium_30_file();
  lv_font_MiSans_Medium_40_file();
  lv_font_Lato_Regular_20_file();
  lv_font_Lato_Regular_88_file();
  lv_font_Lato_Regular_44_file();
  lv_font_NotoSansSC_Medium_20_file();
  lv_font_Lato_Regular_24_file();
  lv_font_montserratMedium_16_file();


}

GUI_WEAK void free_gui_fonts()
{
  if (lv_font_MiSans_Medium_34_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_34_ptr);
    lv_font_MiSans_Medium_34_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_34, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_36_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_36_ptr);
    lv_font_MiSans_Medium_36_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_36, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_32_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_32_ptr);
    lv_font_MiSans_Medium_32_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_32, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_18_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_18_ptr);
    lv_font_MiSans_Medium_18_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_18, 0, sizeof(lv_font_t));
  }
  if (lv_font_lyrics_38_ptr != NULL) {
    lv_freetype_font_delete(lv_font_lyrics_38_ptr);
    lv_font_lyrics_38_ptr = NULL;
    lv_memset(&lv_font_lyrics_38_font_info, 0, sizeof(lv_font_info_t));
    lv_memset(&lv_font_lyrics_38, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_14_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_14_ptr);
    lv_font_MiSans_Medium_14_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_14, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_38_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_38_ptr);
    lv_font_MiSans_Medium_38_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_38, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_120_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_120_ptr);
    lv_font_MiSans_Medium_120_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_120, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_29_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_29_ptr);
    lv_font_MiSans_Medium_29_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_29, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_31_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_31_ptr);
    lv_font_MiSans_Medium_31_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_31, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_24_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_24_ptr);
    lv_font_MiSans_Medium_24_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_24, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_26_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_26_ptr);
    lv_font_MiSans_Medium_26_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_26, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_30_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_30_ptr);
    lv_font_MiSans_Medium_30_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_30, 0, sizeof(lv_font_t));
  }
  if (lv_font_MiSans_Medium_40_ptr != NULL) {
    lv_font_free_bin(lv_font_MiSans_Medium_40_ptr);
    lv_font_MiSans_Medium_40_ptr = NULL;
    lv_memset(&lv_font_MiSans_Medium_40, 0, sizeof(lv_font_t));
  }
  if (lv_font_Lato_Regular_20_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_20_ptr);
    lv_font_Lato_Regular_20_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_20, 0, sizeof(lv_font_t));
  }
  if (lv_font_Lato_Regular_88_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_88_ptr);
    lv_font_Lato_Regular_88_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_88, 0, sizeof(lv_font_t));
  }
  if (lv_font_Lato_Regular_44_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_44_ptr);
    lv_font_Lato_Regular_44_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_44, 0, sizeof(lv_font_t));
  }
  if (lv_font_NotoSansSC_Medium_20_ptr != NULL) {
    lv_font_free_bin(lv_font_NotoSansSC_Medium_20_ptr);
    lv_font_NotoSansSC_Medium_20_ptr = NULL;
    lv_memset(&lv_font_NotoSansSC_Medium_20, 0, sizeof(lv_font_t));
  }
  if (lv_font_Lato_Regular_24_ptr != NULL) {
    lv_font_free_bin(lv_font_Lato_Regular_24_ptr);
    lv_font_Lato_Regular_24_ptr = NULL;
    lv_memset(&lv_font_Lato_Regular_24, 0, sizeof(lv_font_t));
  }
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_16_ptr);
    lv_font_montserratMedium_16_ptr = NULL;
    lv_memset(&lv_font_montserratMedium_16, 0, sizeof(lv_font_t));
  }
}

