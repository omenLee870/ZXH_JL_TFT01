/*Generate Code, Do NOT Edit!*/
#ifndef __RES_COMMON_H__
#define __RES_COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#if LV_USE_GUIBUILDER_SIMULATOR
	#define GUI_WEAKREF __attribute__((weakref))
	#define GUI_WEAK __attribute__((weak))
#else
	#define GUI_WEAKREF
	#define GUI_WEAK __attribute__((weak))
#endif

#define CONV_RES_ID(id) (((id) >> 24) | ((id) & 0x00FFFFFF) << 8)
typedef enum {
	GUI_RES_MONTSERRATMEDIUM_17_TTF_FLASH = 0x35000000,   //F:\br33_verify\watch\ui_prj\Application2\lvgl-simulator\board\ui_res\flash\rle\font\35000000.rle
	GUI_RES_MONTSERRATMEDIUM_18_TTF_FLASH = 0x35000001,   //F:\br33_verify\watch\ui_prj\Application2\lvgl-simulator\board\ui_res\flash\rle\font\35000001.rle
	GUI_RES_MONTSERRATMEDIUM_16_TTF_FLASH = 0x35000002,   //F:\br33_verify\watch\ui_prj\Application2\lvgl-simulator\board\ui_res\flash\rle\font\35000002.rle
	GUI_RES_MONTSERRATMEDIUM_9_TTF_FLASH = 0x35000003,   //F:\br33_verify\watch\ui_prj\Application2\lvgl-simulator\board\ui_res\flash\rle\font\35000003.rle

	GUI_RES_SPORT_PNG = 0x4B000004,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\SPORT.png
	GUI_RES_TTFAR_PNG = 0x4B000005,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\TTFAR.png
	GUI_RES_FREE_PNG = 0x4B000006,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\FREE.png
	GUI_RES_TEM_01_PNG = 0x4B000007,   //F:\br33_verify\watch\ui_prj\Application2\import\image\main\tem_01.png
	GUI_RES_LAMP_25_PNG = 0x4B000008,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_25.png
	GUI_RES_LAMP_26_PNG = 0x4B000009,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_26.png
	GUI_RES_LAMP_23_PNG = 0x4B00000A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_23.png
	GUI_RES_LAMP_24_PNG = 0x4B00000B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_24.png
	GUI_RES_LAMP_29_PNG = 0x4B00000C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_29.png
	GUI_RES_LAMP_30_PNG = 0x4B00000D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_30.png
	GUI_RES_LAMP_27_PNG = 0x4B00000E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_27.png
	GUI_RES_LAMP_28_PNG = 0x4B00000F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_28.png
	GUI_RES_READY_PNG = 0x4B000010,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\READY.png
	GUI_RES_LAMP_19_PNG = 0x4B000011,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_19.png
	GUI_RES_LAMP_20_PNG = 0x4B000012,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_20.png
	GUI_RES_LAMP_21_PNG = 0x4B000013,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_21.png
	GUI_RES_LAMP_22_PNG = 0x4B000014,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_22.png
	GUI_RES_BAT_001_PNG = 0x4B000015,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_001.png
	GUI_RES_BAT_002_PNG = 0x4B000016,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_002.png
	GUI_RES_BAT_003_PNG = 0x4B000017,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_003.png
	GUI_RES_BAT_004_PNG = 0x4B000018,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_004.png
	GUI_RES_BAT_005_PNG = 0x4B000019,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_005.png
	GUI_RES_BAT_006_PNG = 0x4B00001A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_006.png
	GUI_RES_BAT_007_PNG = 0x4B00001B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_007.png
	GUI_RES_BAT_008_PNG = 0x4B00001C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_008.png
	GUI_RES_BAT_009_PNG = 0x4B00001D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_009.png
	GUI_RES_BAT_010_PNG = 0x4B00001E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_010.png
	GUI_RES_BAT_011_PNG = 0x4B00001F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_011.png
	GUI_RES_BAT_012_PNG = 0x4B000020,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_012.png
	GUI_RES_BAT_013_PNG = 0x4B000021,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_013.png
	GUI_RES_BAT_014_PNG = 0x4B000022,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_014.png
	GUI_RES_BAT_015_PNG = 0x4B000023,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_015.png
	GUI_RES_BAT_016_PNG = 0x4B000024,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_016.png
	GUI_RES_BAT_017_PNG = 0x4B000025,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_017.png
	GUI_RES_BAT_018_PNG = 0x4B000026,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_018.png
	GUI_RES_BAT_019_PNG = 0x4B000027,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_019.png
	GUI_RES_BAT_020_PNG = 0x4B000028,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_020.png
	GUI_RES_BAT_021_PNG = 0x4B000029,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_021.png
	GUI_RES_BAT_022_PNG = 0x4B00002A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_022.png
	GUI_RES_BAT_023_PNG = 0x4B00002B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_023.png
	GUI_RES_BAT_024_PNG = 0x4B00002C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_024.png
	GUI_RES_BAT_025_PNG = 0x4B00002D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_025.png
	GUI_RES_BAT_026_PNG = 0x4B00002E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_026.png
	GUI_RES_BAT_027_PNG = 0x4B00002F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_027.png
	GUI_RES_BAT_028_PNG = 0x4B000030,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_028.png
	GUI_RES_BAT_029_PNG = 0x4B000031,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_029.png
	GUI_RES_BAT_030_PNG = 0x4B000032,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_030.png
	GUI_RES_BAT_031_PNG = 0x4B000033,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_031.png
	GUI_RES_BAT_032_PNG = 0x4B000034,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_032.png
	GUI_RES_BAT_033_PNG = 0x4B000035,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_033.png
	GUI_RES_BAT_034_PNG = 0x4B000036,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_034.png
	GUI_RES_BAT_035_PNG = 0x4B000037,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_035.png
	GUI_RES_BAT_036_PNG = 0x4B000038,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_036.png
	GUI_RES_BAT_037_PNG = 0x4B000039,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_037.png
	GUI_RES_BAT_038_PNG = 0x4B00003A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_038.png
	GUI_RES_BAT_039_PNG = 0x4B00003B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_039.png
	GUI_RES_BAT_040_PNG = 0x4B00003C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_040.png
	GUI_RES_BAT_041_PNG = 0x4B00003D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_041.png
	GUI_RES_BAT_042_PNG = 0x4B00003E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_042.png
	GUI_RES_BAT_043_PNG = 0x4B00003F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_043.png
	GUI_RES_BAT_044_PNG = 0x4B000040,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_044.png
	GUI_RES_BAT_045_PNG = 0x4B000041,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_045.png
	GUI_RES_BAT_046_PNG = 0x4B000042,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_046.png
	GUI_RES_BAT_047_PNG = 0x4B000043,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_047.png
	GUI_RES_BAT_048_PNG = 0x4B000044,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_048.png
	GUI_RES_BAT_049_PNG = 0x4B000045,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_049.png
	GUI_RES_BAT_050_PNG = 0x4B000046,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_050.png
	GUI_RES_BAT_051_PNG = 0x4B000047,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_051.png
	GUI_RES_BAT_052_PNG = 0x4B000048,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_052.png
	GUI_RES_BAT_053_PNG = 0x4B000049,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_053.png
	GUI_RES_BAT_054_PNG = 0x4B00004A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_054.png
	GUI_RES_BAT_055_PNG = 0x4B00004B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_055.png
	GUI_RES_BAT_056_PNG = 0x4B00004C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_056.png
	GUI_RES_BAT_057_PNG = 0x4B00004D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_057.png
	GUI_RES_BAT_058_PNG = 0x4B00004E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_058.png
	GUI_RES_BAT_059_PNG = 0x4B00004F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_059.png
	GUI_RES_BAT_060_PNG = 0x4B000050,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_060.png
	GUI_RES_BAT_061_PNG = 0x4B000051,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_061.png
	GUI_RES_BAT_062_PNG = 0x4B000052,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_062.png
	GUI_RES_BAT_063_PNG = 0x4B000053,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_063.png
	GUI_RES_BAT_064_PNG = 0x4B000054,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_064.png
	GUI_RES_BAT_065_PNG = 0x4B000055,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_065.png
	GUI_RES_BAT_066_PNG = 0x4B000056,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_066.png
	GUI_RES_BAT_067_PNG = 0x4B000057,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_067.png
	GUI_RES_BAT_068_PNG = 0x4B000058,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_068.png
	GUI_RES_BAT_069_PNG = 0x4B000059,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_069.png
	GUI_RES_BAT_070_PNG = 0x4B00005A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_070.png
	GUI_RES_BAT_071_PNG = 0x4B00005B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_071.png
	GUI_RES_BAT_072_PNG = 0x4B00005C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_072.png
	GUI_RES_BAT_073_PNG = 0x4B00005D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_073.png
	GUI_RES_BAT_074_PNG = 0x4B00005E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_074.png
	GUI_RES_BAT_075_PNG = 0x4B00005F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_075.png
	GUI_RES_BAT_076_PNG = 0x4B000060,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_076.png
	GUI_RES_BAT_077_PNG = 0x4B000061,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_077.png
	GUI_RES_BAT_078_PNG = 0x4B000062,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_078.png
	GUI_RES_BAT_079_PNG = 0x4B000063,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_079.png
	GUI_RES_BAT_080_PNG = 0x4B000064,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_080.png
	GUI_RES_BAT_081_PNG = 0x4B000065,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_081.png
	GUI_RES_BAT_082_PNG = 0x4B000066,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_082.png
	GUI_RES_BAT_083_PNG = 0x4B000067,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_083.png
	GUI_RES_BAT_084_PNG = 0x4B000068,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_084.png
	GUI_RES_BAT_085_PNG = 0x4B000069,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_085.png
	GUI_RES_BAT_086_PNG = 0x4B00006A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_086.png
	GUI_RES_BAT_087_PNG = 0x4B00006B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_087.png
	GUI_RES_BAT_088_PNG = 0x4B00006C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_088.png
	GUI_RES_BAT_089_PNG = 0x4B00006D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_089.png
	GUI_RES_BAT_090_PNG = 0x4B00006E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_090.png
	GUI_RES_BAT_091_PNG = 0x4B00006F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_091.png
	GUI_RES_BAT_092_PNG = 0x4B000070,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_092.png
	GUI_RES_BAT_093_PNG = 0x4B000071,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_093.png
	GUI_RES_BAT_094_PNG = 0x4B000072,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_094.png
	GUI_RES_BAT_095_PNG = 0x4B000073,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_095.png
	GUI_RES_BAT_096_PNG = 0x4B000074,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_096.png
	GUI_RES_BAT_097_PNG = 0x4B000075,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_097.png
	GUI_RES_BAT_098_PNG = 0x4B000076,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_098.png
	GUI_RES_BAT_099_PNG = 0x4B000077,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_099.png
	GUI_RES_BAT_100_PNG = 0x4B000078,   //F:\br33_verify\watch\ui_prj\Application2\import\image\battery\bat_100.png
	GUI_RES_LAMP_01_PNG = 0x4B000079,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_01.png
	GUI_RES_TIPS_PNG = 0x4B00007A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\tips\tips.png
	GUI_RES_ROTATE_00000_PAGE_METER_IMGLIST_1_PNG = 0x4B00007B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00000.png
	GUI_RES_ROTATE_00001_PAGE_METER_IMGLIST_1_PNG = 0x4B00007C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00001.png
	GUI_RES_ROTATE_00002_PAGE_METER_IMGLIST_1_PNG = 0x4B00007D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00002.png
	GUI_RES_ROTATE_00003_PAGE_METER_IMGLIST_1_PNG = 0x4B00007E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00003.png
	GUI_RES_ROTATE_00004_PAGE_METER_IMGLIST_1_PNG = 0x4B00007F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00004.png
	GUI_RES_ROTATE_00005_PAGE_METER_IMGLIST_1_PNG = 0x4B000080,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00005.png
	GUI_RES_ROTATE_00006_PAGE_METER_IMGLIST_1_PNG = 0x4B000081,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00006.png
	GUI_RES_ROTATE_00007_PAGE_METER_IMGLIST_1_PNG = 0x4B000082,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00007.png
	GUI_RES_ROTATE_00010_PAGE_METER_IMGLIST_1_PNG = 0x4B000083,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00010.png
	GUI_RES_ROTATE_00011_PAGE_METER_IMGLIST_1_PNG = 0x4B000084,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00011.png
	GUI_RES_ROTATE_00012_PAGE_METER_IMGLIST_1_PNG = 0x4B000085,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00012.png
	GUI_RES_ROTATE_00013_PAGE_METER_IMGLIST_1_PNG = 0x4B000086,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00013.png
	GUI_RES_ROTATE_00014_PAGE_METER_IMGLIST_1_PNG = 0x4B000087,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00014.png
	GUI_RES_ROTATE_00015_PAGE_METER_IMGLIST_1_PNG = 0x4B000088,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00015.png
	GUI_RES_ROTATE_00016_PAGE_METER_IMGLIST_1_PNG = 0x4B000089,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00016.png
	GUI_RES_ROTATE_00017_PAGE_METER_IMGLIST_1_PNG = 0x4B00008A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00017.png
	GUI_RES_ROTATE_00020_PAGE_METER_IMGLIST_1_PNG = 0x4B00008B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00020.png
	GUI_RES_ROTATE_00021_PAGE_METER_IMGLIST_1_PNG = 0x4B00008C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00021.png
	GUI_RES_ROTATE_00022_PAGE_METER_IMGLIST_1_PNG = 0x4B00008D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00022.png
	GUI_RES_ROTATE_00023_PAGE_METER_IMGLIST_1_PNG = 0x4B00008E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00023.png
	GUI_RES_ROTATE_00024_PAGE_METER_IMGLIST_1_PNG = 0x4B00008F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00024.png
	GUI_RES_ROTATE_00025_PAGE_METER_IMGLIST_1_PNG = 0x4B000090,   //F:\br33_verify\watch\ui_prj\Application2\import\image\meter\rotate_00025.png
	GUI_RES_NUMSN_0_PNG = 0x4B000091,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_0.png
	GUI_RES_NUMSN_1_PNG = 0x4B000092,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_1.png
	GUI_RES_NUMSN_2_PNG = 0x4B000093,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_2.png
	GUI_RES_NUMSN_3_PNG = 0x4B000094,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_3.png
	GUI_RES_NUMSN_4_PNG = 0x4B000095,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_4.png
	GUI_RES_NUMSN_5_PNG = 0x4B000096,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_5.png
	GUI_RES_NUMSN_6_PNG = 0x4B000097,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_6.png
	GUI_RES_NUMSN_7_PNG = 0x4B000098,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_7.png
	GUI_RES_NUMSN_8_PNG = 0x4B000099,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_8.png
	GUI_RES_NUMSN_9_PNG = 0x4B00009A,   //F:\br33_verify\watch\ui_prj\Application2\import\image\speed\gongli\numsn_9.png
	GUI_RES_READYC_PNG = 0x4B00009B,   //F:\br33_verify\watch\ui_prj\Application2\import\image\gonglihouzhui\readyc.png
	GUI_RES_LAMP_02_PNG = 0x4B00009C,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_02.png
	GUI_RES_LAMP_06_PNG = 0x4B00009D,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_06.png
	GUI_RES_LAMP_03_PNG = 0x4B00009E,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_03.png
	GUI_RES_LAMP_07_PNG = 0x4B00009F,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_07.png
	GUI_RES_LAMP_17_PNG = 0x4B0000A0,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_17.png
	GUI_RES_LAMP_04_PNG = 0x4B0000A1,   //F:\br33_verify\watch\ui_prj\Application2\import\image\alarm\Lamp_04.png
	GUI_RES_GEAR_1_PNG = 0x4B0000A2,   //F:\br33_verify\watch\ui_prj\Application2\import\image\new\englist\shuzi\dangwei\gear_1.png
	GUI_RES_MILEAGE_KM_PNG = 0x4B0000A3,   //F:\br33_verify\watch\ui_prj\Application2\import\image\new\englist\shuzi\licheng\mileage_km.png
	GUI_RES_VOLTAGE_72V_PNG = 0x4B0000A4,   //F:\br33_verify\watch\ui_prj\Application2\import\image\new\englist\tubiao\voltage_72v.png
	GUI_RES_LOGO_PNG = 0x4B0000A5,   //F:\br33_verify\watch\ui_prj\Application2\import\image\new\englist\tubiao\logo.png
	GUI_RES_MILEAGE_TRIP_PNG = 0x4B0000A6,   //F:\br33_verify\watch\ui_prj\Application2\import\image\new\englist\shuzi\licheng\mileage_trip.png
	GUI_RES_SINGER100_PNG = 0x4B0000A7,   //F:\br33_verify\watch\ui_prj\Application2\import\image\music\singer100.png
	GUI_RES_DIE1_PNG = 0x4B0000A8,   //F:\br33_verify\watch\ui_prj\Application2\import\image\music\die1.png
	GUI_RES_ZHEN_PNG = 0x4B0000A9,   //F:\br33_verify\watch\ui_prj\Application2\import\image\music\zhen.png
} GUI_RES_ID;

extern char * gui_get_res_path(int32_t id);

#ifdef __cplusplus
}
#endif
#endif
