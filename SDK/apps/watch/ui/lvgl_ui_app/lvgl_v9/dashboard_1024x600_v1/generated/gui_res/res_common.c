/*Generate Code, Do NOT Edit!*/
#include "../gui_guider.h"
#include <stdlib.h>
#include <string.h>
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif

#if !LV_USE_GUIBUILDER_SIMULATOR
#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) MAX2(MAX2(a, b), c)
#define MAX4(a, b, c, d) MAX2(MAX3(a, b, c), d)
#define MAX5(a, b, c, d, e) MAX2(MAX4(a, b, c, d), e)
#define MAX6(a, b, c, d, e, f) MAX2(MAX5(a, b, c, d, e), f)
#define MAX7(a, b, c, d, e, f, g) MAX2(MAX6(a, b, c, d, e, f), g)
#define MAX8(a, b, c, d, e, f, g, h) MAX2(MAX7(a, b, c, d, e, f, g), h)

const char flash_dir[] = "storage/res_nor_mode/C/lvgl/";
const char sd_dir[] = "storage/sd0/C/";
char path_buf[MAX2(sizeof(flash_dir), sizeof(sd_dir)) + 20];
#endif

char * gui_get_res_path(int32_t id)
{
#if LV_USE_GUIBUILDER_SIMULATOR
	switch (id) {
		case GUI_RES_MISANS_MEDIUM_34_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000000.rle";
		case GUI_RES_MISANS_MEDIUM_36_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000001.rle";
		case GUI_RES_MISANS_MEDIUM_32_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000002.rle";
		case GUI_RES_MISANS_MEDIUM_18_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000003.rle";
		case GUI_RES_LYRICS_TTF_FLASH_FILE: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\font\\lyrics.ttf";
		case GUI_RES_MISANS_MEDIUM_14_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000005.rle";
		case GUI_RES_MISANS_MEDIUM_38_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000006.rle";
		case GUI_RES_MISANS_MEDIUM_120_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000007.rle";
		case GUI_RES_MISANS_MEDIUM_29_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000008.rle";
		case GUI_RES_MISANS_MEDIUM_31_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000009.rle";
		case GUI_RES_MISANS_MEDIUM_24_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\3500000a.rle";
		case GUI_RES_MISANS_MEDIUM_26_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\3500000b.rle";
		case GUI_RES_MISANS_MEDIUM_30_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\3500000c.rle";
		case GUI_RES_MISANS_MEDIUM_40_RLE_FLASH: return "E:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\3500000d.rle";
		case GUI_RES_SPORT_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\SPORT.png";
		case GUI_RES_TTFAR_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\TTFAR.png";
		case GUI_RES_FREE_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\FREE.png";
		case GUI_RES_TEM_01_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\tem_01.png";
		case GUI_RES_LAMP_25_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_25.png";
		case GUI_RES_LAMP_26_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_26.png";
		case GUI_RES_LAMP_23_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_23.png";
		case GUI_RES_LAMP_24_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_24.png";
		case GUI_RES_LAMP_29_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_29.png";
		case GUI_RES_LAMP_30_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_30.png";
		case GUI_RES_LAMP_27_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_27.png";
		case GUI_RES_LAMP_28_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_28.png";
		case GUI_RES_READY_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\READY.png";
		case GUI_RES_LAMP_19_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_19.png";
		case GUI_RES_LAMP_20_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_20.png";
		case GUI_RES_LAMP_21_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_21.png";
		case GUI_RES_LAMP_22_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_22.png";
		case GUI_RES_BAT001_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat001.png";
		case GUI_RES_BAT002_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat002.png";
		case GUI_RES_BAT003_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat003.png";
		case GUI_RES_BAT004_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat004.png";
		case GUI_RES_BAT005_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat005.png";
		case GUI_RES_BAT006_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat006.png";
		case GUI_RES_BAT007_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat007.png";
		case GUI_RES_BAT008_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat008.png";
		case GUI_RES_BAT009_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat009.png";
		case GUI_RES_BAT010_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat010.png";
		case GUI_RES_BAT011_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat011.png";
		case GUI_RES_BAT012_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat012.png";
		case GUI_RES_BAT013_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat013.png";
		case GUI_RES_BAT014_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat014.png";
		case GUI_RES_BAT015_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat015.png";
		case GUI_RES_BAT016_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat016.png";
		case GUI_RES_BAT017_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat017.png";
		case GUI_RES_BAT018_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat018.png";
		case GUI_RES_BAT019_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat019.png";
		case GUI_RES_BAT020_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat020.png";
		case GUI_RES_BAT021_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat021.png";
		case GUI_RES_BAT022_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat022.png";
		case GUI_RES_BAT023_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat023.png";
		case GUI_RES_BAT024_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat024.png";
		case GUI_RES_BAT025_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat025.png";
		case GUI_RES_BAT026_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat026.png";
		case GUI_RES_BAT027_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat027.png";
		case GUI_RES_BAT028_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat028.png";
		case GUI_RES_BAT029_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat029.png";
		case GUI_RES_BAT030_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat030.png";
		case GUI_RES_BAT031_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat031.png";
		case GUI_RES_BAT032_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat032.png";
		case GUI_RES_BAT033_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat033.png";
		case GUI_RES_BAT034_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat034.png";
		case GUI_RES_BAT035_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat035.png";
		case GUI_RES_BAT036_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat036.png";
		case GUI_RES_BAT037_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat037.png";
		case GUI_RES_BAT038_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat038.png";
		case GUI_RES_BAT039_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat039.png";
		case GUI_RES_BAT040_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat040.png";
		case GUI_RES_BAT041_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat041.png";
		case GUI_RES_BAT042_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat042.png";
		case GUI_RES_BAT043_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat043.png";
		case GUI_RES_BAT044_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat044.png";
		case GUI_RES_BAT045_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat045.png";
		case GUI_RES_BAT046_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat046.png";
		case GUI_RES_BAT047_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat047.png";
		case GUI_RES_BAT048_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat048.png";
		case GUI_RES_BAT049_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat049.png";
		case GUI_RES_BAT050_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat050.png";
		case GUI_RES_BAT051_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat051.png";
		case GUI_RES_BAT052_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat052.png";
		case GUI_RES_BAT053_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat053.png";
		case GUI_RES_BAT054_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat054.png";
		case GUI_RES_BAT055_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat055.png";
		case GUI_RES_BAT056_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat056.png";
		case GUI_RES_BAT057_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat057.png";
		case GUI_RES_BAT058_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat058.png";
		case GUI_RES_BAT059_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat059.png";
		case GUI_RES_BAT060_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat060.png";
		case GUI_RES_BAT061_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat061.png";
		case GUI_RES_BAT062_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat062.png";
		case GUI_RES_BAT063_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat063.png";
		case GUI_RES_BAT064_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat064.png";
		case GUI_RES_BAT065_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat065.png";
		case GUI_RES_BAT066_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat066.png";
		case GUI_RES_BAT067_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat067.png";
		case GUI_RES_BAT068_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat068.png";
		case GUI_RES_BAT069_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat069.png";
		case GUI_RES_BAT070_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat070.png";
		case GUI_RES_BAT071_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat071.png";
		case GUI_RES_BAT072_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat072.png";
		case GUI_RES_BAT073_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat073.png";
		case GUI_RES_BAT074_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat074.png";
		case GUI_RES_BAT075_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat075.png";
		case GUI_RES_BAT076_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat076.png";
		case GUI_RES_BAT077_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat077.png";
		case GUI_RES_BAT078_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat078.png";
		case GUI_RES_BAT079_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat079.png";
		case GUI_RES_BAT080_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat080.png";
		case GUI_RES_BAT081_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat081.png";
		case GUI_RES_BAT082_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat082.png";
		case GUI_RES_BAT083_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat083.png";
		case GUI_RES_BAT084_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat084.png";
		case GUI_RES_BAT085_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat085.png";
		case GUI_RES_BAT086_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat086.png";
		case GUI_RES_BAT087_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat087.png";
		case GUI_RES_BAT088_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat088.png";
		case GUI_RES_BAT089_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat089.png";
		case GUI_RES_BAT090_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat090.png";
		case GUI_RES_BAT091_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat091.png";
		case GUI_RES_BAT092_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat092.png";
		case GUI_RES_BAT093_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat093.png";
		case GUI_RES_BAT094_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat094.png";
		case GUI_RES_BAT095_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat095.png";
		case GUI_RES_BAT096_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat096.png";
		case GUI_RES_BAT097_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat097.png";
		case GUI_RES_BAT098_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat098.png";
		case GUI_RES_BAT099_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat099.png";
		case GUI_RES_BAT100_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\battery\\bat100.png";
		case GUI_RES_LAMP_01_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_01.png";
		case GUI_RES_TIPS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\tips\\tips.png";
		case GUI_RES_DIE_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\die.png";
		case GUI_RES_IMAGE_4225207_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\image_4225207_3.png";
		case GUI_RES_ZHEN_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\zhen.png";
		case GUI_RES_IMAGE_4225207_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\image_4225207_2.png";
		case GUI_RES_IMAGE_4225207_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\image_4225207_1.png";
		case GUI_RES_IMAGE_4225207_0_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\music\\image_4225207_0.png";
		case GUI_RES_IMAGE_0882465_0_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_0.png";
		case GUI_RES_IMAGE_0882465_1_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_1.png";
		case GUI_RES_IMAGE_0882465_2_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_2.png";
		case GUI_RES_IMAGE_0882465_3_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_3.png";
		case GUI_RES_IMAGE_0882465_4_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_4.png";
		case GUI_RES_IMAGE_0882465_5_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_5.png";
		case GUI_RES_IMAGE_0882465_6_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_6.png";
		case GUI_RES_IMAGE_0882465_7_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_7.png";
		case GUI_RES_IMAGE_0882465_8_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_8.png";
		case GUI_RES_IMAGE_0882465_9_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882465_9.png";
		case GUI_RES_IMAGE_0882473_0_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_0.png";
		case GUI_RES_IMAGE_0882473_1_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_1.png";
		case GUI_RES_IMAGE_0882473_2_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_2.png";
		case GUI_RES_IMAGE_0882473_3_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_3.png";
		case GUI_RES_IMAGE_0882473_4_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_4.png";
		case GUI_RES_IMAGE_0882473_5_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_5.png";
		case GUI_RES_IMAGE_0882473_6_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_6.png";
		case GUI_RES_IMAGE_0882473_7_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_7.png";
		case GUI_RES_IMAGE_0882473_8_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_8.png";
		case GUI_RES_IMAGE_0882473_9_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_9.png";
		case GUI_RES_IMAGE_0882473_10_PAGE_APP_SELECT_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\anim_copy\\image_0882473_10.png";
		case GUI_RES_TIANQI_DUOYUN_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_duoyun.png";
		case GUI_RES_TIANQI_BAOYU_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_baoyu.png";
		case GUI_RES_TIANQI_DAYU_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_dayu.png";
		case GUI_RES_TIANQI_LEIYU_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_leiyu.png";
		case GUI_RES_TIANQI_QING_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_qing.png";
		case GUI_RES_TIANQI_WUMAI_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_wumai.png";
		case GUI_RES_TIANQI_XIAOYU_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_xiaoyu.png";
		case GUI_RES_TIANQI_YE_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_ye.png";
		case GUI_RES_TIANQI_YINTIAN_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_yintian.png";
		case GUI_RES_A_138SHEZHIXITONGSHEZHIGONGNENGSHEZHISHUXING_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\app_icon\\a_138shezhixitongshezhigongnengshezhishuxing.png";
		case GUI_RES_YINLE_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\app_icon\\yinle_1.png";
		case GUI_RES_DAOHANG_TIANCHONG_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\app_icon\\daohang_tianchong.png";
		case GUI_RES_IMAGE_9158879_0_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\image_9158879_0.png";
		case GUI_RES_FRAME_000015_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000015.png";
		case GUI_RES_FRAME_000017_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000017.png";
		case GUI_RES_FRAME_000019_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000019.png";
		case GUI_RES_FRAME_000021_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000021.png";
		case GUI_RES_FRAME_000023_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000023.png";
		case GUI_RES_FRAME_000025_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000025.png";
		case GUI_RES_FRAME_000027_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000027.png";
		case GUI_RES_FRAME_000029_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000029.png";
		case GUI_RES_FRAME_000031_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000031.png";
		case GUI_RES_FRAME_000033_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000033.png";
		case GUI_RES_FRAME_000035_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000035.png";
		case GUI_RES_FRAME_000037_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000037.png";
		case GUI_RES_FRAME_000039_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000039.png";
		case GUI_RES_FRAME_000041_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000041.png";
		case GUI_RES_FRAME_000043_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000043.png";
		case GUI_RES_FRAME_000045_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000045.png";
		case GUI_RES_FRAME_000047_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000047.png";
		case GUI_RES_FRAME_000049_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000049.png";
		case GUI_RES_FRAME_000051_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000051.png";
		case GUI_RES_FRAME_000053_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000053.png";
		case GUI_RES_FRAME_000055_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000055.png";
		case GUI_RES_FRAME_000057_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000057.png";
		case GUI_RES_FRAME_000059_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000059.png";
		case GUI_RES_FRAME_000061_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000061.png";
		case GUI_RES_FRAME_000063_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000063.png";
		case GUI_RES_FRAME_000065_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000065.png";
		case GUI_RES_FRAME_000067_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000067.png";
		case GUI_RES_FRAME_000069_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000069.png";
		case GUI_RES_FRAME_000071_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000071.png";
		case GUI_RES_FRAME_000073_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000073.png";
		case GUI_RES_FRAME_000075_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000075.png";
		case GUI_RES_FRAME_000077_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000077.png";
		case GUI_RES_FRAME_000079_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000079.png";
		case GUI_RES_FRAME_000081_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000081.png";
		case GUI_RES_FRAME_000083_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000083.png";
		case GUI_RES_FRAME_000085_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000085.png";
		case GUI_RES_FRAME_000087_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000087.png";
		case GUI_RES_FRAME_000089_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000089.png";
		case GUI_RES_FRAME_000092_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000092.png";
		case GUI_RES_FRAME_000095_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000095.png";
		case GUI_RES_FRAME_000097_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000097.png";
		case GUI_RES_FRAME_000099_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000099.png";
		case GUI_RES_FRAME_000101_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000101.png";
		case GUI_RES_FRAME_000103_PAGE_METER_ANIMIMG_3_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000103.png";
		case GUI_RES_PW_001_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_001.png";
		case GUI_RES_PW_002_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_002.png";
		case GUI_RES_PW_003_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_003.png";
		case GUI_RES_PW_004_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_004.png";
		case GUI_RES_PW_005_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_005.png";
		case GUI_RES_PW_006_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_006.png";
		case GUI_RES_PW_007_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_007.png";
		case GUI_RES_PW_008_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_008.png";
		case GUI_RES_PW_009_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_009.png";
		case GUI_RES_PW_010_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_010.png";
		case GUI_RES_PW_011_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_011.png";
		case GUI_RES_PW_012_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_012.png";
		case GUI_RES_PW_013_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_013.png";
		case GUI_RES_PW_014_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_014.png";
		case GUI_RES_PW_015_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_015.png";
		case GUI_RES_PW_016_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_016.png";
		case GUI_RES_PW_017_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_017.png";
		case GUI_RES_PW_018_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_018.png";
		case GUI_RES_PW_019_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_019.png";
		case GUI_RES_PW_020_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_020.png";
		case GUI_RES_PW_021_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_021.png";
		case GUI_RES_PW_022_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_022.png";
		case GUI_RES_PW_023_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_023.png";
		case GUI_RES_PW_024_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_024.png";
		case GUI_RES_PW_025_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_025.png";
		case GUI_RES_PW_026_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_026.png";
		case GUI_RES_PW_027_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_027.png";
		case GUI_RES_PW_028_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_028.png";
		case GUI_RES_PW_029_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_029.png";
		case GUI_RES_PW_030_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_030.png";
		case GUI_RES_PW_031_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_031.png";
		case GUI_RES_PW_032_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_032.png";
		case GUI_RES_PW_033_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_033.png";
		case GUI_RES_PW_034_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_034.png";
		case GUI_RES_PW_035_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_035.png";
		case GUI_RES_PW_036_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_036.png";
		case GUI_RES_PW_037_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_037.png";
		case GUI_RES_PW_038_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_038.png";
		case GUI_RES_PW_039_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_039.png";
		case GUI_RES_PW_040_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_040.png";
		case GUI_RES_PW_041_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_041.png";
		case GUI_RES_PW_042_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_042.png";
		case GUI_RES_PW_043_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_043.png";
		case GUI_RES_PW_044_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_044.png";
		case GUI_RES_PW_045_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_045.png";
		case GUI_RES_PW_046_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_046.png";
		case GUI_RES_PW_047_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_047.png";
		case GUI_RES_PW_048_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_048.png";
		case GUI_RES_PW_049_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_049.png";
		case GUI_RES_PW_050_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_050.png";
		case GUI_RES_PW_051_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_051.png";
		case GUI_RES_PW_052_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_052.png";
		case GUI_RES_PW_053_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_053.png";
		case GUI_RES_PW_054_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_054.png";
		case GUI_RES_PW_055_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_055.png";
		case GUI_RES_PW_056_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_056.png";
		case GUI_RES_PW_057_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_057.png";
		case GUI_RES_PW_058_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_058.png";
		case GUI_RES_PW_059_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_059.png";
		case GUI_RES_PW_060_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_060.png";
		case GUI_RES_PW_061_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_061.png";
		case GUI_RES_PW_062_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_062.png";
		case GUI_RES_PW_063_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_063.png";
		case GUI_RES_PW_064_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_064.png";
		case GUI_RES_PW_065_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_065.png";
		case GUI_RES_PW_066_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_066.png";
		case GUI_RES_PW_067_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_067.png";
		case GUI_RES_PW_068_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_068.png";
		case GUI_RES_PW_069_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_069.png";
		case GUI_RES_PW_070_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_070.png";
		case GUI_RES_PW_071_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_071.png";
		case GUI_RES_PW_072_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_072.png";
		case GUI_RES_PW_073_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_073.png";
		case GUI_RES_PW_074_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_074.png";
		case GUI_RES_PW_075_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_075.png";
		case GUI_RES_PW_076_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_076.png";
		case GUI_RES_PW_077_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_077.png";
		case GUI_RES_PW_078_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_078.png";
		case GUI_RES_PW_079_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_079.png";
		case GUI_RES_PW_080_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_080.png";
		case GUI_RES_PW_081_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_081.png";
		case GUI_RES_PW_082_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_082.png";
		case GUI_RES_PW_083_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_083.png";
		case GUI_RES_PW_084_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_084.png";
		case GUI_RES_PW_085_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_085.png";
		case GUI_RES_PW_086_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_086.png";
		case GUI_RES_PW_087_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_087.png";
		case GUI_RES_PW_088_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_088.png";
		case GUI_RES_PW_089_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_089.png";
		case GUI_RES_PW_090_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_090.png";
		case GUI_RES_PW_091_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_091.png";
		case GUI_RES_PW_092_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_092.png";
		case GUI_RES_PW_093_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_093.png";
		case GUI_RES_PW_094_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_094.png";
		case GUI_RES_PW_095_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_095.png";
		case GUI_RES_PW_096_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_096.png";
		case GUI_RES_PW_097_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_097.png";
		case GUI_RES_PW_098_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_098.png";
		case GUI_RES_PW_099_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_099.png";
		case GUI_RES_PW_100_PAGE_METER_ANIMIMG_1_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power\\pw_100.png";
		case GUI_RES_PWR_001_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_001.png";
		case GUI_RES_PWR_002_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_002.png";
		case GUI_RES_PWR_003_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_003.png";
		case GUI_RES_PWR_004_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_004.png";
		case GUI_RES_PWR_005_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_005.png";
		case GUI_RES_PWR_006_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_006.png";
		case GUI_RES_PWR_007_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_007.png";
		case GUI_RES_PWR_008_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_008.png";
		case GUI_RES_PWR_009_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_009.png";
		case GUI_RES_PWR_010_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_010.png";
		case GUI_RES_PWR_011_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_011.png";
		case GUI_RES_PWR_012_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_012.png";
		case GUI_RES_PWR_013_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_013.png";
		case GUI_RES_PWR_014_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_014.png";
		case GUI_RES_PWR_015_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_015.png";
		case GUI_RES_PWR_016_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_016.png";
		case GUI_RES_PWR_017_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_017.png";
		case GUI_RES_PWR_018_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_018.png";
		case GUI_RES_PWR_019_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_019.png";
		case GUI_RES_PWR_020_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_020.png";
		case GUI_RES_PWR_021_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_021.png";
		case GUI_RES_PWR_022_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_022.png";
		case GUI_RES_PWR_023_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_023.png";
		case GUI_RES_PWR_024_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_024.png";
		case GUI_RES_PWR_025_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_025.png";
		case GUI_RES_PWR_026_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_026.png";
		case GUI_RES_PWR_027_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_027.png";
		case GUI_RES_PWR_028_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_028.png";
		case GUI_RES_PWR_029_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_029.png";
		case GUI_RES_PWR_030_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_030.png";
		case GUI_RES_PWR_031_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_031.png";
		case GUI_RES_PWR_032_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_032.png";
		case GUI_RES_PWR_033_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_033.png";
		case GUI_RES_PWR_034_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_034.png";
		case GUI_RES_PWR_035_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_035.png";
		case GUI_RES_PWR_036_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_036.png";
		case GUI_RES_PWR_037_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_037.png";
		case GUI_RES_PWR_038_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_038.png";
		case GUI_RES_PWR_039_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_039.png";
		case GUI_RES_PWR_040_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_040.png";
		case GUI_RES_PWR_041_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_041.png";
		case GUI_RES_PWR_042_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_042.png";
		case GUI_RES_PWR_043_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_043.png";
		case GUI_RES_PWR_044_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_044.png";
		case GUI_RES_PWR_045_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_045.png";
		case GUI_RES_PWR_046_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_046.png";
		case GUI_RES_PWR_047_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_047.png";
		case GUI_RES_PWR_048_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_048.png";
		case GUI_RES_PWR_049_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_049.png";
		case GUI_RES_PWR_050_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_050.png";
		case GUI_RES_PWR_051_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_051.png";
		case GUI_RES_PWR_052_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_052.png";
		case GUI_RES_PWR_053_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_053.png";
		case GUI_RES_PWR_054_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_054.png";
		case GUI_RES_PWR_055_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_055.png";
		case GUI_RES_PWR_056_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_056.png";
		case GUI_RES_PWR_057_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_057.png";
		case GUI_RES_PWR_058_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_058.png";
		case GUI_RES_PWR_059_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_059.png";
		case GUI_RES_PWR_060_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_060.png";
		case GUI_RES_PWR_061_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_061.png";
		case GUI_RES_PWR_062_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_062.png";
		case GUI_RES_PWR_063_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_063.png";
		case GUI_RES_PWR_064_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_064.png";
		case GUI_RES_PWR_065_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_065.png";
		case GUI_RES_PWR_066_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_066.png";
		case GUI_RES_PWR_067_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_067.png";
		case GUI_RES_PWR_068_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_068.png";
		case GUI_RES_PWR_069_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_069.png";
		case GUI_RES_PWR_070_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_070.png";
		case GUI_RES_PWR_071_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_071.png";
		case GUI_RES_PWR_072_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_072.png";
		case GUI_RES_PWR_073_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_073.png";
		case GUI_RES_PWR_074_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_074.png";
		case GUI_RES_PWR_075_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_075.png";
		case GUI_RES_PWR_076_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_076.png";
		case GUI_RES_PWR_077_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_077.png";
		case GUI_RES_PWR_078_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_078.png";
		case GUI_RES_PWR_079_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_079.png";
		case GUI_RES_PWR_080_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_080.png";
		case GUI_RES_PWR_081_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_081.png";
		case GUI_RES_PWR_082_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_082.png";
		case GUI_RES_PWR_083_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_083.png";
		case GUI_RES_PWR_084_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_084.png";
		case GUI_RES_PWR_085_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_085.png";
		case GUI_RES_PWR_086_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_086.png";
		case GUI_RES_PWR_087_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_087.png";
		case GUI_RES_PWR_088_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_088.png";
		case GUI_RES_PWR_089_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_089.png";
		case GUI_RES_PWR_090_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_090.png";
		case GUI_RES_PWR_091_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_091.png";
		case GUI_RES_PWR_092_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_092.png";
		case GUI_RES_PWR_093_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_093.png";
		case GUI_RES_PWR_094_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_094.png";
		case GUI_RES_PWR_095_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_095.png";
		case GUI_RES_PWR_096_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_096.png";
		case GUI_RES_PWR_097_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_097.png";
		case GUI_RES_PWR_098_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_098.png";
		case GUI_RES_PWR_099_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_099.png";
		case GUI_RES_PWR_100_PAGE_METER_ANIMIMG_2_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_100.png";
		case GUI_RES_LAMP_02_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_02.png";
		case GUI_RES_LAMP_06_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_06.png";
		case GUI_RES_ARROW_ECO_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\arrow_eco.png";
		case GUI_RES_LAMP_17_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_17.png";
		case GUI_RES_LAMP_04_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_04.png";
		case GUI_RES_LAMP_08_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_08.png";
		case GUI_RES_LAMP_14_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_14.png";
		case GUI_RES_LAMP_07_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_07.png";
		case GUI_RES_LAMP_12_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_12.png";
		case GUI_RES_LAMP_13_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_13.png";
		case GUI_RES_LAMP_18_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_18.png";
		case GUI_RES_0_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\0.png";
		case GUI_RES_1_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\1.png";
		case GUI_RES_2_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\2.png";
		case GUI_RES_3_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\3.png";
		case GUI_RES_4_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\4.png";
		case GUI_RES_5_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\5.png";
		case GUI_RES_6_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\6.png";
		case GUI_RES_7_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\7.png";
		case GUI_RES_8_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\8.png";
		case GUI_RES_9_PAGE_METER_IMGLIST_SPEED_TENS_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\main\\9.png";
		case GUI_RES_ASTERN_PNG: return "A:\\watch_sdk\\standard\\watch_master\\JL380N\\v0.0.1\\ui_project\\dashboard_1024x600_v1\\project\\dashboard_1024x600_v1\\import\\image\\page_video\\astern.png";
		default: return NULL;
	}
#else
	memset(path_buf, 0, sizeof(path_buf));
	if (id >= 0x30000000 && id < 0x30100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "ttf");
	} else if (id >= 0x30100000 && id < 0x30200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "ttf");
	} else if (id >= 0x31000000 && id < 0x31100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "woff");
	} else if (id >= 0x31100000 && id < 0x31200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "woff");
	} else if (id >= 0x32000000 && id < 0x32100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "woff2");
	} else if (id >= 0x32100000 && id < 0x32200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "woff2");
	} else if (id >= 0x33000000 && id < 0x33100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "otf");
	} else if (id >= 0x33100000 && id < 0x33200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "otf");
	} else if (id >= 0x34000000 && id < 0x34100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x34100000 && id < 0x34200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x35000000 && id < 0x35100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x35100000 && id < 0x35200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x36000000 && id < 0x36100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "fnt");
	} else if (id >= 0x36100000 && id < 0x36200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "fnt");
	} else if (id >= 0x37000000 && id < 0x37100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "lzw");
	} else if (id >= 0x37100000 && id < 0x37200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "lzw");
	} else if (id >= 0x39000000 && id < 0x39100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "json");
	} else if (id >= 0x39100000 && id < 0x39200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "json");
	} else if (id >= 0x3A000000 && id < 0x3A100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x3A100000 && id < 0x3A200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x3B000000 && id < 0x3B100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x3B100000 && id < 0x3B200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x3D000000 && id < 0x3D100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "mp4");
	} else if (id >= 0x3D100000 && id < 0x3D200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "mp4");
	} else if (id >= 0x3E000000 && id < 0x3E100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "avi");
	} else if (id >= 0x3E100000 && id < 0x3E200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "avi");
	} else if (id >= 0x3F000000 && id < 0x3F100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x3F100000 && id < 0x3F200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x40000000 && id < 0x40100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x40100000 && id < 0x40200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x42000000 && id < 0x42100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "gif");
	} else if (id >= 0x42100000 && id < 0x42200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "gif");
	} else if (id >= 0x43000000 && id < 0x43100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "zip");
	} else if (id >= 0x43100000 && id < 0x43200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "zip");
	} else if (id >= 0x46000000 && id < 0x46100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "png");
	} else if (id >= 0x46100000 && id < 0x46200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "png");
	} else if (id >= 0x47000000 && id < 0x47100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "jpg");
	} else if (id >= 0x47100000 && id < 0x47200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "jpg");
	} else if (id >= 0x48000000 && id < 0x48100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "jpeg");
	} else if (id >= 0x48100000 && id < 0x48200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "jpeg");
	} else if (id >= 0x49000000 && id < 0x49100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bmp");
	} else if (id >= 0x49100000 && id < 0x49200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bmp");
	} else if (id >= 0x4A000000 && id < 0x4A100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "svg");
	} else if (id >= 0x4A100000 && id < 0x4A200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "svg");
	} else if (id >= 0x4B000000 && id < 0x4B100000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "zip");
	} else if (id >= 0x4B100000 && id < 0x4B200000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "zip");
	} else {
		return NULL;
	}
	return path_buf;
#endif
}

lv_event_cb_t gui_get_event_cb(int32_t id)
{
	switch (id) {
		case RES_EVENT_MUSIC_PAUSE_PLAY_ID: return res_event_music_pause_play;
		case RES_EVENT_MUSIC_NEXT_ID: return res_event_music_next;
		case RES_EVENT_MUSIC_PREV_ID: return res_event_music_prev;
		default: return NULL;
	}
}
GUI_WEAK void res_event_music_pause_play(lv_event_t *e)
{
}
GUI_WEAK void res_event_music_next(lv_event_t *e)
{
}
GUI_WEAK void res_event_music_prev(lv_event_t *e)
{
}
