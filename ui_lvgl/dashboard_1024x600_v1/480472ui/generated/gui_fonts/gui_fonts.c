#include "lvgl.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include "./gui_fonts.h"
#include "../common.h"

lv_font_t lv_font_montserratMedium_17;
lv_font_t * lv_font_montserratMedium_17_ptr;
void lv_font_montserratMedium_17_file()
{
  if (lv_font_montserratMedium_17_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_17_ptr);
    lv_font_montserratMedium_17_ptr = NULL;
    memset(&lv_font_montserratMedium_17, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_montserratMedium_17_ptr = lv_font_load_bin("A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000000.rle");
#else
  lv_font_montserratMedium_17_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_17_TTF_FLASH));
#endif
  if (lv_font_montserratMedium_17_ptr != NULL) {
    lv_font_montserratMedium_17 = *lv_font_montserratMedium_17_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000000.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_17_TTF_FLASH));
#endif
    lv_font_montserratMedium_17 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_montserratMedium_18;
lv_font_t * lv_font_montserratMedium_18_ptr;
void lv_font_montserratMedium_18_file()
{
  if (lv_font_montserratMedium_18_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_18_ptr);
    lv_font_montserratMedium_18_ptr = NULL;
    memset(&lv_font_montserratMedium_18, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_montserratMedium_18_ptr = lv_font_load_bin("A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000001.rle");
#else
  lv_font_montserratMedium_18_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_18_TTF_FLASH));
#endif
  if (lv_font_montserratMedium_18_ptr != NULL) {
    lv_font_montserratMedium_18 = *lv_font_montserratMedium_18_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000001.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_18_TTF_FLASH));
#endif
    lv_font_montserratMedium_18 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_montserratMedium_16;
lv_font_t * lv_font_montserratMedium_16_ptr;
void lv_font_montserratMedium_16_file()
{
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_16_ptr);
    lv_font_montserratMedium_16_ptr = NULL;
    memset(&lv_font_montserratMedium_16, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_montserratMedium_16_ptr = lv_font_load_bin("A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000002.rle");
#else
  lv_font_montserratMedium_16_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_16_TTF_FLASH));
#endif
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_montserratMedium_16 = *lv_font_montserratMedium_16_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000002.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_16_TTF_FLASH));
#endif
    lv_font_montserratMedium_16 = *LV_FONT_DEFAULT;
  }
}
lv_font_t lv_font_montserratMedium_9;
lv_font_t * lv_font_montserratMedium_9_ptr;
void lv_font_montserratMedium_9_file()
{
  if (lv_font_montserratMedium_9_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_9_ptr);
    lv_font_montserratMedium_9_ptr = NULL;
    memset(&lv_font_montserratMedium_9, 0, sizeof(lv_font_t));
  }
#if LV_USE_GUIBUILDER_SIMULATOR
  lv_font_montserratMedium_9_ptr = lv_font_load_bin("A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000003.rle");
#else
  lv_font_montserratMedium_9_ptr = lv_font_load_bin(gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_9_TTF_FLASH));
#endif
  if (lv_font_montserratMedium_9_ptr != NULL) {
    lv_font_montserratMedium_9 = *lv_font_montserratMedium_9_ptr;
  } else {
#if LV_USE_GUIBUILDER_SIMULATOR
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", "A:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000003.rle");
#else
    LV_LOG_ERROR("Load Fnt Font Failed %s\n", gui_get_res_path(GUI_RES_MONTSERRATMEDIUM_9_TTF_FLASH));
#endif
    lv_font_montserratMedium_9 = *LV_FONT_DEFAULT;
  }
}

void init_gui_fonts()
{
  lv_font_montserratMedium_17_file();
  lv_font_montserratMedium_18_file();
  lv_font_montserratMedium_16_file();
  lv_font_montserratMedium_9_file();


}

void free_gui_fonts()
{
  if (lv_font_montserratMedium_17_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_17_ptr);
    lv_font_montserratMedium_17_ptr = NULL;
    memset(&lv_font_montserratMedium_17, 0, sizeof(lv_font_t));
  }
  if (lv_font_montserratMedium_18_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_18_ptr);
    lv_font_montserratMedium_18_ptr = NULL;
    memset(&lv_font_montserratMedium_18, 0, sizeof(lv_font_t));
  }
  if (lv_font_montserratMedium_16_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_16_ptr);
    lv_font_montserratMedium_16_ptr = NULL;
    memset(&lv_font_montserratMedium_16, 0, sizeof(lv_font_t));
  }
  if (lv_font_montserratMedium_9_ptr != NULL) {
    lv_font_free_bin(lv_font_montserratMedium_9_ptr);
    lv_font_montserratMedium_9_ptr = NULL;
    memset(&lv_font_montserratMedium_9, 0, sizeof(lv_font_t));
  }
}

