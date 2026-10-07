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
const char sd_dir[] = "S:/";
char path_buf[MAX2(sizeof(flash_dir), sizeof(sd_dir)) + 20];
#endif

char * gui_get_res_path(int32_t id)
{
#if LV_USE_GUIBUILDER_SIMULATOR
	switch (id) {
		case GUI_RES_MONTSERRATMEDIUM_17_TTF_FLASH: return "F:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000000.rle";
		case GUI_RES_MONTSERRATMEDIUM_18_TTF_FLASH: return "F:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000001.rle";
		case GUI_RES_MONTSERRATMEDIUM_16_TTF_FLASH: return "F:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000002.rle";
		case GUI_RES_MONTSERRATMEDIUM_9_TTF_FLASH: return "F:\\br33_verify\\watch\\ui_prj\\Application2\\lvgl-simulator\\board\\ui_res\\flash\\rle\\font\\35000003.rle";
		case GUI_RES_SPORT_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\SPORT.png";
		case GUI_RES_TTFAR_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\TTFAR.png";
		case GUI_RES_FREE_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\FREE.png";
		case GUI_RES_TEM_01_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\main\\tem_01.png";
		case GUI_RES_LAMP_25_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_25.png";
		case GUI_RES_LAMP_26_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_26.png";
		case GUI_RES_LAMP_23_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_23.png";
		case GUI_RES_LAMP_24_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_24.png";
		case GUI_RES_LAMP_29_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_29.png";
		case GUI_RES_LAMP_30_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_30.png";
		case GUI_RES_LAMP_27_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_27.png";
		case GUI_RES_LAMP_28_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_28.png";
		case GUI_RES_READY_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\READY.png";
		case GUI_RES_LAMP_19_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_19.png";
		case GUI_RES_LAMP_20_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_20.png";
		case GUI_RES_LAMP_21_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_21.png";
		case GUI_RES_LAMP_22_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_22.png";
		case GUI_RES_BAT_001_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_001.png";
		case GUI_RES_BAT_002_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_002.png";
		case GUI_RES_BAT_003_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_003.png";
		case GUI_RES_BAT_004_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_004.png";
		case GUI_RES_BAT_005_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_005.png";
		case GUI_RES_BAT_006_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_006.png";
		case GUI_RES_BAT_007_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_007.png";
		case GUI_RES_BAT_008_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_008.png";
		case GUI_RES_BAT_009_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_009.png";
		case GUI_RES_BAT_010_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_010.png";
		case GUI_RES_BAT_011_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_011.png";
		case GUI_RES_BAT_012_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_012.png";
		case GUI_RES_BAT_013_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_013.png";
		case GUI_RES_BAT_014_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_014.png";
		case GUI_RES_BAT_015_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_015.png";
		case GUI_RES_BAT_016_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_016.png";
		case GUI_RES_BAT_017_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_017.png";
		case GUI_RES_BAT_018_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_018.png";
		case GUI_RES_BAT_019_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_019.png";
		case GUI_RES_BAT_020_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_020.png";
		case GUI_RES_BAT_021_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_021.png";
		case GUI_RES_BAT_022_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_022.png";
		case GUI_RES_BAT_023_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_023.png";
		case GUI_RES_BAT_024_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_024.png";
		case GUI_RES_BAT_025_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_025.png";
		case GUI_RES_BAT_026_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_026.png";
		case GUI_RES_BAT_027_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_027.png";
		case GUI_RES_BAT_028_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_028.png";
		case GUI_RES_BAT_029_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_029.png";
		case GUI_RES_BAT_030_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_030.png";
		case GUI_RES_BAT_031_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_031.png";
		case GUI_RES_BAT_032_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_032.png";
		case GUI_RES_BAT_033_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_033.png";
		case GUI_RES_BAT_034_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_034.png";
		case GUI_RES_BAT_035_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_035.png";
		case GUI_RES_BAT_036_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_036.png";
		case GUI_RES_BAT_037_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_037.png";
		case GUI_RES_BAT_038_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_038.png";
		case GUI_RES_BAT_039_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_039.png";
		case GUI_RES_BAT_040_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_040.png";
		case GUI_RES_BAT_041_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_041.png";
		case GUI_RES_BAT_042_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_042.png";
		case GUI_RES_BAT_043_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_043.png";
		case GUI_RES_BAT_044_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_044.png";
		case GUI_RES_BAT_045_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_045.png";
		case GUI_RES_BAT_046_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_046.png";
		case GUI_RES_BAT_047_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_047.png";
		case GUI_RES_BAT_048_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_048.png";
		case GUI_RES_BAT_049_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_049.png";
		case GUI_RES_BAT_050_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_050.png";
		case GUI_RES_BAT_051_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_051.png";
		case GUI_RES_BAT_052_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_052.png";
		case GUI_RES_BAT_053_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_053.png";
		case GUI_RES_BAT_054_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_054.png";
		case GUI_RES_BAT_055_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_055.png";
		case GUI_RES_BAT_056_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_056.png";
		case GUI_RES_BAT_057_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_057.png";
		case GUI_RES_BAT_058_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_058.png";
		case GUI_RES_BAT_059_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_059.png";
		case GUI_RES_BAT_060_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_060.png";
		case GUI_RES_BAT_061_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_061.png";
		case GUI_RES_BAT_062_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_062.png";
		case GUI_RES_BAT_063_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_063.png";
		case GUI_RES_BAT_064_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_064.png";
		case GUI_RES_BAT_065_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_065.png";
		case GUI_RES_BAT_066_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_066.png";
		case GUI_RES_BAT_067_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_067.png";
		case GUI_RES_BAT_068_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_068.png";
		case GUI_RES_BAT_069_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_069.png";
		case GUI_RES_BAT_070_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_070.png";
		case GUI_RES_BAT_071_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_071.png";
		case GUI_RES_BAT_072_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_072.png";
		case GUI_RES_BAT_073_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_073.png";
		case GUI_RES_BAT_074_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_074.png";
		case GUI_RES_BAT_075_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_075.png";
		case GUI_RES_BAT_076_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_076.png";
		case GUI_RES_BAT_077_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_077.png";
		case GUI_RES_BAT_078_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_078.png";
		case GUI_RES_BAT_079_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_079.png";
		case GUI_RES_BAT_080_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_080.png";
		case GUI_RES_BAT_081_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_081.png";
		case GUI_RES_BAT_082_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_082.png";
		case GUI_RES_BAT_083_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_083.png";
		case GUI_RES_BAT_084_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_084.png";
		case GUI_RES_BAT_085_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_085.png";
		case GUI_RES_BAT_086_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_086.png";
		case GUI_RES_BAT_087_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_087.png";
		case GUI_RES_BAT_088_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_088.png";
		case GUI_RES_BAT_089_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_089.png";
		case GUI_RES_BAT_090_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_090.png";
		case GUI_RES_BAT_091_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_091.png";
		case GUI_RES_BAT_092_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_092.png";
		case GUI_RES_BAT_093_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_093.png";
		case GUI_RES_BAT_094_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_094.png";
		case GUI_RES_BAT_095_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_095.png";
		case GUI_RES_BAT_096_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_096.png";
		case GUI_RES_BAT_097_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_097.png";
		case GUI_RES_BAT_098_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_098.png";
		case GUI_RES_BAT_099_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_099.png";
		case GUI_RES_BAT_100_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\battery\\bat_100.png";
		case GUI_RES_LAMP_01_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_01.png";
		case GUI_RES_TIPS_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\tips\\tips.png";
		case GUI_RES_ROTATE_00000_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00000.png";
		case GUI_RES_ROTATE_00001_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00001.png";
		case GUI_RES_ROTATE_00002_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00002.png";
		case GUI_RES_ROTATE_00003_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00003.png";
		case GUI_RES_ROTATE_00004_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00004.png";
		case GUI_RES_ROTATE_00005_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00005.png";
		case GUI_RES_ROTATE_00006_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00006.png";
		case GUI_RES_ROTATE_00007_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00007.png";
		case GUI_RES_ROTATE_00010_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00010.png";
		case GUI_RES_ROTATE_00011_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00011.png";
		case GUI_RES_ROTATE_00012_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00012.png";
		case GUI_RES_ROTATE_00013_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00013.png";
		case GUI_RES_ROTATE_00014_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00014.png";
		case GUI_RES_ROTATE_00015_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00015.png";
		case GUI_RES_ROTATE_00016_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00016.png";
		case GUI_RES_ROTATE_00017_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00017.png";
		case GUI_RES_ROTATE_00020_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00020.png";
		case GUI_RES_ROTATE_00021_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00021.png";
		case GUI_RES_ROTATE_00022_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00022.png";
		case GUI_RES_ROTATE_00023_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00023.png";
		case GUI_RES_ROTATE_00024_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00024.png";
		case GUI_RES_ROTATE_00025_PAGE_METER_IMGLIST_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\meter\\rotate_00025.png";
		case GUI_RES_NUMSN_0_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_0.png";
		case GUI_RES_NUMSN_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_1.png";
		case GUI_RES_NUMSN_2_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_2.png";
		case GUI_RES_NUMSN_3_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_3.png";
		case GUI_RES_NUMSN_4_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_4.png";
		case GUI_RES_NUMSN_5_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_5.png";
		case GUI_RES_NUMSN_6_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_6.png";
		case GUI_RES_NUMSN_7_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_7.png";
		case GUI_RES_NUMSN_8_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_8.png";
		case GUI_RES_NUMSN_9_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\speed\\gongli\\numsn_9.png";
		case GUI_RES_READYC_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\gonglihouzhui\\readyc.png";
		case GUI_RES_LAMP_02_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_02.png";
		case GUI_RES_LAMP_06_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_06.png";
		case GUI_RES_LAMP_03_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_03.png";
		case GUI_RES_LAMP_07_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_07.png";
		case GUI_RES_LAMP_17_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_17.png";
		case GUI_RES_LAMP_04_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\alarm\\Lamp_04.png";
		case GUI_RES_GEAR_1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\new\\englist\\shuzi\\dangwei\\gear_1.png";
		case GUI_RES_MILEAGE_KM_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\new\\englist\\shuzi\\licheng\\mileage_km.png";
		case GUI_RES_VOLTAGE_72V_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\new\\englist\\tubiao\\voltage_72v.png";
		case GUI_RES_LOGO_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\new\\englist\\tubiao\\logo.png";
		case GUI_RES_MILEAGE_TRIP_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\new\\englist\\shuzi\\licheng\\mileage_trip.png";
		case GUI_RES_SINGER100_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\music\\singer100.png";
		case GUI_RES_DIE1_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\music\\die1.png";
		case GUI_RES_ZHEN_PNG: return "A:\\br33_verify\\watch\\ui_prj\\Application2\\import\\image\\music\\zhen.png";
		default: return NULL;
	}
#else
	lv_memset(path_buf, 0, sizeof(path_buf));
	if (id >= 0x30000000 && id < 0x30800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "ttf");
	} else if (id >= 0x30800000 && id < 0x31000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "ttf");
	} else if (id >= 0x31000000 && id < 0x31800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "woff");
	} else if (id >= 0x31800000 && id < 0x32000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "woff");
	} else if (id >= 0x32000000 && id < 0x32800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "woff2");
	} else if (id >= 0x32800000 && id < 0x33000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "woff2");
	} else if (id >= 0x33000000 && id < 0x33800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "otf");
	} else if (id >= 0x33800000 && id < 0x34000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "otf");
	} else if (id >= 0x34000000 && id < 0x34800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x34800000 && id < 0x35000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x35000000 && id < 0x35800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x35800000 && id < 0x36000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x36000000 && id < 0x36800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "fnt");
	} else if (id >= 0x36800000 && id < 0x37000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "fnt");
	} else if (id >= 0x37000000 && id < 0x37800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "lzw");
	} else if (id >= 0x37800000 && id < 0x38000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "lzw");
	} else if (id >= 0x39000000 && id < 0x39800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "json");
	} else if (id >= 0x39800000 && id < 0x3A000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "json");
	} else if (id >= 0x3A000000 && id < 0x3A800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x3A800000 && id < 0x3B000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x3B000000 && id < 0x3B800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x3B800000 && id < 0x3C000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x3D000000 && id < 0x3D800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "mp4");
	} else if (id >= 0x3D800000 && id < 0x3E000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "mp4");
	} else if (id >= 0x3E000000 && id < 0x3E800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "avi");
	} else if (id >= 0x3E800000 && id < 0x3F000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "avi");
	} else if (id >= 0x3F000000 && id < 0x3F800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bin");
	} else if (id >= 0x3F800000 && id < 0x40000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bin");
	} else if (id >= 0x40000000 && id < 0x40800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "rle");
	} else if (id >= 0x40800000 && id < 0x41000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "rle");
	} else if (id >= 0x42000000 && id < 0x42800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "gif");
	} else if (id >= 0x42800000 && id < 0x43000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "gif");
	} else if (id >= 0x43000000 && id < 0x43800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "zip");
	} else if (id >= 0x43800000 && id < 0x44000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "zip");
	} else if (id >= 0x46000000 && id < 0x46800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "png");
	} else if (id >= 0x46800000 && id < 0x47000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "png");
	} else if (id >= 0x47000000 && id < 0x47800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "jpg");
	} else if (id >= 0x47800000 && id < 0x48000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "jpg");
	} else if (id >= 0x48000000 && id < 0x48800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "jpeg");
	} else if (id >= 0x48800000 && id < 0x49000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "jpeg");
	} else if (id >= 0x49000000 && id < 0x49800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "bmp");
	} else if (id >= 0x49800000 && id < 0x4A000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "bmp");
	} else if (id >= 0x4A000000 && id < 0x4A800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "svg");
	} else if (id >= 0x4A800000 && id < 0x4B000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "svg");
	} else if (id >= 0x4B000000 && id < 0x4B800000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", flash_dir, id, "zip");
	} else if (id >= 0x4B800000 && id < 0x4C000000) {
		snprintf(path_buf, sizeof(path_buf), "%s%x.%s", sd_dir, id, "zip");
	} else {
		return NULL;
	}
	return path_buf;
#endif
}
