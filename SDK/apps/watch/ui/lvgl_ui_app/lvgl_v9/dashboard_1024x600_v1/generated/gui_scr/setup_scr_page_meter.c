/*Generate Code, Do NOT Edit!*/
#include "lvgl.h"
#if LV_USE_GUIBUILDER_SIMULATOR
#include <stdio.h>
#endif
#include <stdlib.h>
#include "../gui_guider.h"
#include "../gui_events/events_init.h"
#include "../gui_events/callback_handler.h"
#include "../gui_timelines/gui_timelines.h"
#include "../../custom/custom.h"
#include "string.h"

static const char* page_meter_animimg_3_imgs[44] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000015.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000017.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000019.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000021.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000023.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000025.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000027.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000029.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000031.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000033.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000035.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000037.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000039.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000041.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000043.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000045.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000047.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000049.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000051.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000053.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000055.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000057.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000059.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000061.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000063.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000065.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000067.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000069.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000071.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000073.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000075.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000077.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000079.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000081.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000083.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000085.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000087.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000089.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000092.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000095.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000097.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000099.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000101.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\frames\\frame_000103.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0000b3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000b9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ba.zip",
	"storage/res_nor_mode/C/lvgl/4b0000bb.zip",
	"storage/res_nor_mode/C/lvgl/4b0000bc.zip",
	"storage/res_nor_mode/C/lvgl/4b0000bd.zip",
	"storage/res_nor_mode/C/lvgl/4b0000be.zip",
	"storage/res_nor_mode/C/lvgl/4b0000bf.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c0.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c1.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c2.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000c9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ca.zip",
	"storage/res_nor_mode/C/lvgl/4b0000cb.zip",
	"storage/res_nor_mode/C/lvgl/4b0000cc.zip",
	"storage/res_nor_mode/C/lvgl/4b0000cd.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ce.zip",
	"storage/res_nor_mode/C/lvgl/4b0000cf.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d0.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d1.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d2.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000d9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000da.zip",
	"storage/res_nor_mode/C/lvgl/4b0000db.zip",
	"storage/res_nor_mode/C/lvgl/4b0000dc.zip",
	"storage/res_nor_mode/C/lvgl/4b0000dd.zip",
	"storage/res_nor_mode/C/lvgl/4b0000de.zip"
#endif
};
static const char* page_meter_animimg_1_imgs[100] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_001.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_002.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_003.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_004.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_005.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_006.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_007.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_008.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_009.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_010.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_011.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_012.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_013.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_014.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_015.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_016.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_017.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_018.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_019.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_020.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_021.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_022.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_023.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_024.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_025.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_026.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_027.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_028.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_029.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_030.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_031.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_032.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_033.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_034.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_035.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_036.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_037.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_038.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_039.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_040.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_041.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_042.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_043.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_044.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_045.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_046.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_047.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_048.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_049.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_050.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_051.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_052.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_053.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_054.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_055.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_056.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_057.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_058.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_059.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_060.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_061.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_062.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_063.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_064.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_065.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_066.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_067.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_068.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_069.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_070.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_071.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_072.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_073.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_074.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_075.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_076.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_077.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_078.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_079.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_080.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_081.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_082.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_083.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_084.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_085.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_086.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_087.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_088.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_089.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_090.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_091.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_092.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_093.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_094.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_095.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_096.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_097.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_098.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_099.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power\\pw_100.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0000df.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e0.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e1.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e2.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000e9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ea.zip",
	"storage/res_nor_mode/C/lvgl/4b0000eb.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ec.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ed.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ee.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ef.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f0.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f1.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f2.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f3.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f4.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f5.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000f9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000fa.zip",
	"storage/res_nor_mode/C/lvgl/4b0000fb.zip",
	"storage/res_nor_mode/C/lvgl/4b0000fc.zip",
	"storage/res_nor_mode/C/lvgl/4b0000fd.zip",
	"storage/res_nor_mode/C/lvgl/4b0000fe.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ff.zip",
	"storage/res_nor_mode/C/lvgl/4b000100.zip",
	"storage/res_nor_mode/C/lvgl/4b000101.zip",
	"storage/res_nor_mode/C/lvgl/4b000102.zip",
	"storage/res_nor_mode/C/lvgl/4b000103.zip",
	"storage/res_nor_mode/C/lvgl/4b000104.zip",
	"storage/res_nor_mode/C/lvgl/4b000105.zip",
	"storage/res_nor_mode/C/lvgl/4b000106.zip",
	"storage/res_nor_mode/C/lvgl/4b000107.zip",
	"storage/res_nor_mode/C/lvgl/4b000108.zip",
	"storage/res_nor_mode/C/lvgl/4b000109.zip",
	"storage/res_nor_mode/C/lvgl/4b00010a.zip",
	"storage/res_nor_mode/C/lvgl/4b00010b.zip",
	"storage/res_nor_mode/C/lvgl/4b00010c.zip",
	"storage/res_nor_mode/C/lvgl/4b00010d.zip",
	"storage/res_nor_mode/C/lvgl/4b00010e.zip",
	"storage/res_nor_mode/C/lvgl/4b00010f.zip",
	"storage/res_nor_mode/C/lvgl/4b000110.zip",
	"storage/res_nor_mode/C/lvgl/4b000111.zip",
	"storage/res_nor_mode/C/lvgl/4b000112.zip",
	"storage/res_nor_mode/C/lvgl/4b000113.zip",
	"storage/res_nor_mode/C/lvgl/4b000114.zip",
	"storage/res_nor_mode/C/lvgl/4b000115.zip",
	"storage/res_nor_mode/C/lvgl/4b000116.zip",
	"storage/res_nor_mode/C/lvgl/4b000117.zip",
	"storage/res_nor_mode/C/lvgl/4b000118.zip",
	"storage/res_nor_mode/C/lvgl/4b000119.zip",
	"storage/res_nor_mode/C/lvgl/4b00011a.zip",
	"storage/res_nor_mode/C/lvgl/4b00011b.zip",
	"storage/res_nor_mode/C/lvgl/4b00011c.zip",
	"storage/res_nor_mode/C/lvgl/4b00011d.zip",
	"storage/res_nor_mode/C/lvgl/4b00011e.zip",
	"storage/res_nor_mode/C/lvgl/4b00011f.zip",
	"storage/res_nor_mode/C/lvgl/4b000120.zip",
	"storage/res_nor_mode/C/lvgl/4b000121.zip",
	"storage/res_nor_mode/C/lvgl/4b000122.zip",
	"storage/res_nor_mode/C/lvgl/4b000123.zip",
	"storage/res_nor_mode/C/lvgl/4b000124.zip",
	"storage/res_nor_mode/C/lvgl/4b000125.zip",
	"storage/res_nor_mode/C/lvgl/4b000126.zip",
	"storage/res_nor_mode/C/lvgl/4b000127.zip",
	"storage/res_nor_mode/C/lvgl/4b000128.zip",
	"storage/res_nor_mode/C/lvgl/4b000129.zip",
	"storage/res_nor_mode/C/lvgl/4b00012a.zip",
	"storage/res_nor_mode/C/lvgl/4b00012b.zip",
	"storage/res_nor_mode/C/lvgl/4b00012c.zip",
	"storage/res_nor_mode/C/lvgl/4b00012d.zip",
	"storage/res_nor_mode/C/lvgl/4b00012e.zip",
	"storage/res_nor_mode/C/lvgl/4b00012f.zip",
	"storage/res_nor_mode/C/lvgl/4b000130.zip",
	"storage/res_nor_mode/C/lvgl/4b000131.zip",
	"storage/res_nor_mode/C/lvgl/4b000132.zip",
	"storage/res_nor_mode/C/lvgl/4b000133.zip",
	"storage/res_nor_mode/C/lvgl/4b000134.zip",
	"storage/res_nor_mode/C/lvgl/4b000135.zip",
	"storage/res_nor_mode/C/lvgl/4b000136.zip",
	"storage/res_nor_mode/C/lvgl/4b000137.zip",
	"storage/res_nor_mode/C/lvgl/4b000138.zip",
	"storage/res_nor_mode/C/lvgl/4b000139.zip",
	"storage/res_nor_mode/C/lvgl/4b00013a.zip",
	"storage/res_nor_mode/C/lvgl/4b00013b.zip",
	"storage/res_nor_mode/C/lvgl/4b00013c.zip",
	"storage/res_nor_mode/C/lvgl/4b00013d.zip",
	"storage/res_nor_mode/C/lvgl/4b00013e.zip",
	"storage/res_nor_mode/C/lvgl/4b00013f.zip",
	"storage/res_nor_mode/C/lvgl/4b000140.zip",
	"storage/res_nor_mode/C/lvgl/4b000141.zip",
	"storage/res_nor_mode/C/lvgl/4b000142.zip"
#endif
};
static const char* page_meter_animimg_2_imgs[100] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_001.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_002.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_003.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_004.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_005.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_006.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_007.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_008.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_009.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_010.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_011.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_012.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_013.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_014.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_015.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_016.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_017.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_018.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_019.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_020.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_021.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_022.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_023.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_024.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_025.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_026.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_027.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_028.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_029.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_030.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_031.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_032.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_033.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_034.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_035.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_036.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_037.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_038.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_039.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_040.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_041.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_042.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_043.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_044.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_045.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_046.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_047.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_048.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_049.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_050.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_051.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_052.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_053.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_054.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_055.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_056.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_057.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_058.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_059.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_060.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_061.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_062.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_063.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_064.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_065.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_066.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_067.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_068.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_069.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_070.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_071.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_072.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_073.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_074.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_075.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_076.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_077.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_078.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_079.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_080.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_081.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_082.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_083.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_084.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_085.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_086.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_087.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_088.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_089.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_090.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_091.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_092.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_093.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_094.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_095.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_096.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_097.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_098.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_099.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\power_R\\pwr_100.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000143.zip",
	"storage/res_nor_mode/C/lvgl/4b000144.zip",
	"storage/res_nor_mode/C/lvgl/4b000145.zip",
	"storage/res_nor_mode/C/lvgl/4b000146.zip",
	"storage/res_nor_mode/C/lvgl/4b000147.zip",
	"storage/res_nor_mode/C/lvgl/4b000148.zip",
	"storage/res_nor_mode/C/lvgl/4b000149.zip",
	"storage/res_nor_mode/C/lvgl/4b00014a.zip",
	"storage/res_nor_mode/C/lvgl/4b00014b.zip",
	"storage/res_nor_mode/C/lvgl/4b00014c.zip",
	"storage/res_nor_mode/C/lvgl/4b00014d.zip",
	"storage/res_nor_mode/C/lvgl/4b00014e.zip",
	"storage/res_nor_mode/C/lvgl/4b00014f.zip",
	"storage/res_nor_mode/C/lvgl/4b000150.zip",
	"storage/res_nor_mode/C/lvgl/4b000151.zip",
	"storage/res_nor_mode/C/lvgl/4b000152.zip",
	"storage/res_nor_mode/C/lvgl/4b000153.zip",
	"storage/res_nor_mode/C/lvgl/4b000154.zip",
	"storage/res_nor_mode/C/lvgl/4b000155.zip",
	"storage/res_nor_mode/C/lvgl/4b000156.zip",
	"storage/res_nor_mode/C/lvgl/4b000157.zip",
	"storage/res_nor_mode/C/lvgl/4b000158.zip",
	"storage/res_nor_mode/C/lvgl/4b000159.zip",
	"storage/res_nor_mode/C/lvgl/4b00015a.zip",
	"storage/res_nor_mode/C/lvgl/4b00015b.zip",
	"storage/res_nor_mode/C/lvgl/4b00015c.zip",
	"storage/res_nor_mode/C/lvgl/4b00015d.zip",
	"storage/res_nor_mode/C/lvgl/4b00015e.zip",
	"storage/res_nor_mode/C/lvgl/4b00015f.zip",
	"storage/res_nor_mode/C/lvgl/4b000160.zip",
	"storage/res_nor_mode/C/lvgl/4b000161.zip",
	"storage/res_nor_mode/C/lvgl/4b000162.zip",
	"storage/res_nor_mode/C/lvgl/4b000163.zip",
	"storage/res_nor_mode/C/lvgl/4b000164.zip",
	"storage/res_nor_mode/C/lvgl/4b000165.zip",
	"storage/res_nor_mode/C/lvgl/4b000166.zip",
	"storage/res_nor_mode/C/lvgl/4b000167.zip",
	"storage/res_nor_mode/C/lvgl/4b000168.zip",
	"storage/res_nor_mode/C/lvgl/4b000169.zip",
	"storage/res_nor_mode/C/lvgl/4b00016a.zip",
	"storage/res_nor_mode/C/lvgl/4b00016b.zip",
	"storage/res_nor_mode/C/lvgl/4b00016c.zip",
	"storage/res_nor_mode/C/lvgl/4b00016d.zip",
	"storage/res_nor_mode/C/lvgl/4b00016e.zip",
	"storage/res_nor_mode/C/lvgl/4b00016f.zip",
	"storage/res_nor_mode/C/lvgl/4b000170.zip",
	"storage/res_nor_mode/C/lvgl/4b000171.zip",
	"storage/res_nor_mode/C/lvgl/4b000172.zip",
	"storage/res_nor_mode/C/lvgl/4b000173.zip",
	"storage/res_nor_mode/C/lvgl/4b000174.zip",
	"storage/res_nor_mode/C/lvgl/4b000175.zip",
	"storage/res_nor_mode/C/lvgl/4b000176.zip",
	"storage/res_nor_mode/C/lvgl/4b000177.zip",
	"storage/res_nor_mode/C/lvgl/4b000178.zip",
	"storage/res_nor_mode/C/lvgl/4b000179.zip",
	"storage/res_nor_mode/C/lvgl/4b00017a.zip",
	"storage/res_nor_mode/C/lvgl/4b00017b.zip",
	"storage/res_nor_mode/C/lvgl/4b00017c.zip",
	"storage/res_nor_mode/C/lvgl/4b00017d.zip",
	"storage/res_nor_mode/C/lvgl/4b00017e.zip",
	"storage/res_nor_mode/C/lvgl/4b00017f.zip",
	"storage/res_nor_mode/C/lvgl/4b000180.zip",
	"storage/res_nor_mode/C/lvgl/4b000181.zip",
	"storage/res_nor_mode/C/lvgl/4b000182.zip",
	"storage/res_nor_mode/C/lvgl/4b000183.zip",
	"storage/res_nor_mode/C/lvgl/4b000184.zip",
	"storage/res_nor_mode/C/lvgl/4b000185.zip",
	"storage/res_nor_mode/C/lvgl/4b000186.zip",
	"storage/res_nor_mode/C/lvgl/4b000187.zip",
	"storage/res_nor_mode/C/lvgl/4b000188.zip",
	"storage/res_nor_mode/C/lvgl/4b000189.zip",
	"storage/res_nor_mode/C/lvgl/4b00018a.zip",
	"storage/res_nor_mode/C/lvgl/4b00018b.zip",
	"storage/res_nor_mode/C/lvgl/4b00018c.zip",
	"storage/res_nor_mode/C/lvgl/4b00018d.zip",
	"storage/res_nor_mode/C/lvgl/4b00018e.zip",
	"storage/res_nor_mode/C/lvgl/4b00018f.zip",
	"storage/res_nor_mode/C/lvgl/4b000190.zip",
	"storage/res_nor_mode/C/lvgl/4b000191.zip",
	"storage/res_nor_mode/C/lvgl/4b000192.zip",
	"storage/res_nor_mode/C/lvgl/4b000193.zip",
	"storage/res_nor_mode/C/lvgl/4b000194.zip",
	"storage/res_nor_mode/C/lvgl/4b000195.zip",
	"storage/res_nor_mode/C/lvgl/4b000196.zip",
	"storage/res_nor_mode/C/lvgl/4b000197.zip",
	"storage/res_nor_mode/C/lvgl/4b000198.zip",
	"storage/res_nor_mode/C/lvgl/4b000199.zip",
	"storage/res_nor_mode/C/lvgl/4b00019a.zip",
	"storage/res_nor_mode/C/lvgl/4b00019b.zip",
	"storage/res_nor_mode/C/lvgl/4b00019c.zip",
	"storage/res_nor_mode/C/lvgl/4b00019d.zip",
	"storage/res_nor_mode/C/lvgl/4b00019e.zip",
	"storage/res_nor_mode/C/lvgl/4b00019f.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a0.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a1.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a2.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a3.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a4.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a5.zip",
	"storage/res_nor_mode/C/lvgl/4b0001a6.zip"
#endif
};
static const char* page_meter_imgList_bluetooth_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_25.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_26.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000018.zip",
	"storage/res_nor_mode/C/lvgl/4b000019.zip"
#endif
};
static const char* page_meter_imgList_phone_bl_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_23.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_24.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001a.zip",
	"storage/res_nor_mode/C/lvgl/4b00001b.zip"
#endif
};
static const char* page_meter_imgList_gps_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_29.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_30.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001c.zip",
	"storage/res_nor_mode/C/lvgl/4b00001d.zip"
#endif
};
static const char* page_meter_imgList_networksignal_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_27.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_28.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001e.zip",
	"storage/res_nor_mode/C/lvgl/4b00001f.zip"
#endif
};
static const char* page_meter_imglist_4_imgs[9] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_duoyun.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_baoyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_dayu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_leiyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_qing.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_wumai.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_xiaoyu.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_ye.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\weather\\tianqi_yintian.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0000a6.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a7.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a8.zip",
	"storage/res_nor_mode/C/lvgl/4b0000a9.zip",
	"storage/res_nor_mode/C/lvgl/4b0000aa.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ab.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ac.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ad.zip",
	"storage/res_nor_mode/C/lvgl/4b0000ae.zip"
#endif
};
static const char* page_meter_imgList_gear_mode_imgs[3] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\SPORT.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\TTFAR.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\FREE.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000014.zip",
	"storage/res_nor_mode/C/lvgl/4b000015.zip",
	"storage/res_nor_mode/C/lvgl/4b000016.zip"
#endif
};
static const char* page_meter_imglist_speed_tens_imgs[10] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\0.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\1.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\2.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\3.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\4.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\5.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\6.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\7.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\8.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\9.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0001b2.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b3.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b4.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b5.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b6.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b7.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b8.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b9.zip",
	"storage/res_nor_mode/C/lvgl/4b0001ba.zip",
	"storage/res_nor_mode/C/lvgl/4b0001bb.zip"
#endif
};
static const char* page_meter_imglist_speed_ones_imgs[10] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\0.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\1.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\2.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\3.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\4.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\5.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\6.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\7.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\8.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\main\\9.png"
#else
	"storage/res_nor_mode/C/lvgl/4b0001b2.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b3.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b4.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b5.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b6.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b7.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b8.zip",
	"storage/res_nor_mode/C/lvgl/4b0001b9.zip",
	"storage/res_nor_mode/C/lvgl/4b0001ba.zip",
	"storage/res_nor_mode/C/lvgl/4b0001bb.zip"
#endif
};
static const char* page_meter_imglist_5_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_21.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_20.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000023.zip",
	"storage/res_nor_mode/C/lvgl/4b000022.zip"
#endif
};
static const char* page_meter_imglist_bat_imgs[100] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat001.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat002.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat003.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat004.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat005.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat006.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat007.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat008.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat009.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat010.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat011.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat012.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat013.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat014.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat015.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat016.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat017.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat018.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat019.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat020.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat021.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat022.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat023.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat024.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat025.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat026.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat027.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat028.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat029.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat030.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat031.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat032.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat033.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat034.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat035.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat036.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat037.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat038.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat039.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat040.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat041.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat042.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat043.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat044.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat045.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat046.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat047.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat048.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat049.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat050.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat051.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat052.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat053.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat054.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat055.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat056.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat057.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat058.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat059.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat060.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat061.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat062.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat063.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat064.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat065.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat066.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat067.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat068.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat069.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat070.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat071.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat072.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat073.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat074.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat075.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat076.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat077.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat078.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat079.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat080.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat081.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat082.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat083.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat084.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat085.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat086.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat087.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat088.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat089.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat090.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat091.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat092.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat093.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat094.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat095.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat096.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat097.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat098.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat099.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\battery\\bat100.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000025.zip",
	"storage/res_nor_mode/C/lvgl/4b000026.zip",
	"storage/res_nor_mode/C/lvgl/4b000027.zip",
	"storage/res_nor_mode/C/lvgl/4b000028.zip",
	"storage/res_nor_mode/C/lvgl/4b000029.zip",
	"storage/res_nor_mode/C/lvgl/4b00002a.zip",
	"storage/res_nor_mode/C/lvgl/4b00002b.zip",
	"storage/res_nor_mode/C/lvgl/4b00002c.zip",
	"storage/res_nor_mode/C/lvgl/4b00002d.zip",
	"storage/res_nor_mode/C/lvgl/4b00002e.zip",
	"storage/res_nor_mode/C/lvgl/4b00002f.zip",
	"storage/res_nor_mode/C/lvgl/4b000030.zip",
	"storage/res_nor_mode/C/lvgl/4b000031.zip",
	"storage/res_nor_mode/C/lvgl/4b000032.zip",
	"storage/res_nor_mode/C/lvgl/4b000033.zip",
	"storage/res_nor_mode/C/lvgl/4b000034.zip",
	"storage/res_nor_mode/C/lvgl/4b000035.zip",
	"storage/res_nor_mode/C/lvgl/4b000036.zip",
	"storage/res_nor_mode/C/lvgl/4b000037.zip",
	"storage/res_nor_mode/C/lvgl/4b000038.zip",
	"storage/res_nor_mode/C/lvgl/4b000039.zip",
	"storage/res_nor_mode/C/lvgl/4b00003a.zip",
	"storage/res_nor_mode/C/lvgl/4b00003b.zip",
	"storage/res_nor_mode/C/lvgl/4b00003c.zip",
	"storage/res_nor_mode/C/lvgl/4b00003d.zip",
	"storage/res_nor_mode/C/lvgl/4b00003e.zip",
	"storage/res_nor_mode/C/lvgl/4b00003f.zip",
	"storage/res_nor_mode/C/lvgl/4b000040.zip",
	"storage/res_nor_mode/C/lvgl/4b000041.zip",
	"storage/res_nor_mode/C/lvgl/4b000042.zip",
	"storage/res_nor_mode/C/lvgl/4b000043.zip",
	"storage/res_nor_mode/C/lvgl/4b000044.zip",
	"storage/res_nor_mode/C/lvgl/4b000045.zip",
	"storage/res_nor_mode/C/lvgl/4b000046.zip",
	"storage/res_nor_mode/C/lvgl/4b000047.zip",
	"storage/res_nor_mode/C/lvgl/4b000048.zip",
	"storage/res_nor_mode/C/lvgl/4b000049.zip",
	"storage/res_nor_mode/C/lvgl/4b00004a.zip",
	"storage/res_nor_mode/C/lvgl/4b00004b.zip",
	"storage/res_nor_mode/C/lvgl/4b00004c.zip",
	"storage/res_nor_mode/C/lvgl/4b00004d.zip",
	"storage/res_nor_mode/C/lvgl/4b00004e.zip",
	"storage/res_nor_mode/C/lvgl/4b00004f.zip",
	"storage/res_nor_mode/C/lvgl/4b000050.zip",
	"storage/res_nor_mode/C/lvgl/4b000051.zip",
	"storage/res_nor_mode/C/lvgl/4b000052.zip",
	"storage/res_nor_mode/C/lvgl/4b000053.zip",
	"storage/res_nor_mode/C/lvgl/4b000054.zip",
	"storage/res_nor_mode/C/lvgl/4b000055.zip",
	"storage/res_nor_mode/C/lvgl/4b000056.zip",
	"storage/res_nor_mode/C/lvgl/4b000057.zip",
	"storage/res_nor_mode/C/lvgl/4b000058.zip",
	"storage/res_nor_mode/C/lvgl/4b000059.zip",
	"storage/res_nor_mode/C/lvgl/4b00005a.zip",
	"storage/res_nor_mode/C/lvgl/4b00005b.zip",
	"storage/res_nor_mode/C/lvgl/4b00005c.zip",
	"storage/res_nor_mode/C/lvgl/4b00005d.zip",
	"storage/res_nor_mode/C/lvgl/4b00005e.zip",
	"storage/res_nor_mode/C/lvgl/4b00005f.zip",
	"storage/res_nor_mode/C/lvgl/4b000060.zip",
	"storage/res_nor_mode/C/lvgl/4b000061.zip",
	"storage/res_nor_mode/C/lvgl/4b000062.zip",
	"storage/res_nor_mode/C/lvgl/4b000063.zip",
	"storage/res_nor_mode/C/lvgl/4b000064.zip",
	"storage/res_nor_mode/C/lvgl/4b000065.zip",
	"storage/res_nor_mode/C/lvgl/4b000066.zip",
	"storage/res_nor_mode/C/lvgl/4b000067.zip",
	"storage/res_nor_mode/C/lvgl/4b000068.zip",
	"storage/res_nor_mode/C/lvgl/4b000069.zip",
	"storage/res_nor_mode/C/lvgl/4b00006a.zip",
	"storage/res_nor_mode/C/lvgl/4b00006b.zip",
	"storage/res_nor_mode/C/lvgl/4b00006c.zip",
	"storage/res_nor_mode/C/lvgl/4b00006d.zip",
	"storage/res_nor_mode/C/lvgl/4b00006e.zip",
	"storage/res_nor_mode/C/lvgl/4b00006f.zip",
	"storage/res_nor_mode/C/lvgl/4b000070.zip",
	"storage/res_nor_mode/C/lvgl/4b000071.zip",
	"storage/res_nor_mode/C/lvgl/4b000072.zip",
	"storage/res_nor_mode/C/lvgl/4b000073.zip",
	"storage/res_nor_mode/C/lvgl/4b000074.zip",
	"storage/res_nor_mode/C/lvgl/4b000075.zip",
	"storage/res_nor_mode/C/lvgl/4b000076.zip",
	"storage/res_nor_mode/C/lvgl/4b000077.zip",
	"storage/res_nor_mode/C/lvgl/4b000078.zip",
	"storage/res_nor_mode/C/lvgl/4b000079.zip",
	"storage/res_nor_mode/C/lvgl/4b00007a.zip",
	"storage/res_nor_mode/C/lvgl/4b00007b.zip",
	"storage/res_nor_mode/C/lvgl/4b00007c.zip",
	"storage/res_nor_mode/C/lvgl/4b00007d.zip",
	"storage/res_nor_mode/C/lvgl/4b00007e.zip",
	"storage/res_nor_mode/C/lvgl/4b00007f.zip",
	"storage/res_nor_mode/C/lvgl/4b000080.zip",
	"storage/res_nor_mode/C/lvgl/4b000081.zip",
	"storage/res_nor_mode/C/lvgl/4b000082.zip",
	"storage/res_nor_mode/C/lvgl/4b000083.zip",
	"storage/res_nor_mode/C/lvgl/4b000084.zip",
	"storage/res_nor_mode/C/lvgl/4b000085.zip",
	"storage/res_nor_mode/C/lvgl/4b000086.zip",
	"storage/res_nor_mode/C/lvgl/4b000087.zip",
	"storage/res_nor_mode/C/lvgl/4b000088.zip"
#endif
};

lv_obj_t * setup_scr_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_meter));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_meter));
		ui->page_meter = ui_scr;
	}

	//Write codes page_meter
	ui_scr->page_meter = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->page_meter, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->page_meter, GUI_CTRL_SCR);

	//Write codes page_meter_animimg_3
	ui_scr->page_meter_animimg_3 = lv_animimg_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_animimg_3, 0, 0);
	lv_obj_set_size(ui_scr->page_meter_animimg_3, 1024, 600);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_animimg_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_animimg_3, GUI_CTRL_ANIMIMG);
	lv_animimg_set_duration(ui_scr->page_meter_animimg_3, 2596);
	lv_animimg_set_repeat_count(ui_scr->page_meter_animimg_3, -1);
	lv_animimg_set_src(ui_scr->page_meter_animimg_3, (const void **) page_meter_animimg_3_imgs, 44);
	lv_animimg_start(ui_scr->page_meter_animimg_3);

	//Write codes page_meter_animimg_1
	ui_scr->page_meter_animimg_1 = lv_animimg_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_animimg_1, 52, 108);
	lv_obj_set_size(ui_scr->page_meter_animimg_1, 203, 350);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_animimg_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_animimg_1, GUI_CTRL_ANIMIMG);
	lv_animimg_set_duration(ui_scr->page_meter_animimg_1, 2000);
	lv_animimg_set_repeat_count(ui_scr->page_meter_animimg_1, -1);
	lv_animimg_set_src(ui_scr->page_meter_animimg_1, (const void **) page_meter_animimg_1_imgs, 100);
	lv_animimg_start(ui_scr->page_meter_animimg_1);

	//Write codes page_meter_animimg_2
	ui_scr->page_meter_animimg_2 = lv_animimg_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_animimg_2, 754, 108);
	lv_obj_set_size(ui_scr->page_meter_animimg_2, 203, 350);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_animimg_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_animimg_2, GUI_CTRL_ANIMIMG);
	lv_animimg_set_duration(ui_scr->page_meter_animimg_2, 2000);
	lv_animimg_set_repeat_count(ui_scr->page_meter_animimg_2, -1);
	lv_animimg_set_src(ui_scr->page_meter_animimg_2, (const void **) page_meter_animimg_2_imgs, 100);
	lv_animimg_start(ui_scr->page_meter_animimg_2);

	//Write codes page_meter_img_19
	ui_scr->page_meter_img_19 = lv_image_create(ui_scr->page_meter);
	lv_image_set_pivot(ui_scr->page_meter_img_19, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_19, 0);
	lv_image_set_scale(ui_scr->page_meter_img_19, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_19, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_19, 4, 269);
	lv_obj_set_size(ui_scr->page_meter_img_19, 959, 333);
	lv_obj_add_flag(ui_scr->page_meter_img_19, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_19, GUI_CTRL_IMG);

	//Write codes page_meter_img_2
	ui_scr->page_meter_img_2 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_2, gui_get_res_path(GUI_RES_LAMP_02_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_2, 0);
	lv_image_set_scale(ui_scr->page_meter_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_2, 221, 78);
	lv_obj_set_size(ui_scr->page_meter_img_2, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_2, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_2, GUI_CTRL_IMG);

	//Write codes page_meter_img_4
	ui_scr->page_meter_img_4 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_4, gui_get_res_path(GUI_RES_LAMP_06_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_4, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_4, 0);
	lv_image_set_scale(ui_scr->page_meter_img_4, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_4, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_4, 156, 143);
	lv_obj_set_size(ui_scr->page_meter_img_4, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_4, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_4, GUI_CTRL_IMG);

	//Write codes page_meter_img_20
	ui_scr->page_meter_img_20 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_20, gui_get_res_path(GUI_RES_ARROW_ECO_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_20, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_20, 0);
	lv_image_set_scale(ui_scr->page_meter_img_20, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_20, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_20, 429, 368);
	lv_obj_set_size(ui_scr->page_meter_img_20, 181, 158);
	lv_obj_add_flag(ui_scr->page_meter_img_20, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_20, GUI_CTRL_IMG);

	//Write codes page_meter_img_5
	ui_scr->page_meter_img_5 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_5, gui_get_res_path(GUI_RES_LAMP_30_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_5, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_5, 0);
	lv_image_set_scale(ui_scr->page_meter_img_5, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_5, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_5, 751, 74);
	lv_obj_set_size(ui_scr->page_meter_img_5, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_5, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_5, GUI_CTRL_IMG);

	//Write codes page_meter_img_7
	ui_scr->page_meter_img_7 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_7, gui_get_res_path(GUI_RES_LAMP_17_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_7, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_7, 0);
	lv_image_set_scale(ui_scr->page_meter_img_7, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_7, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_7, 826, 288);
	lv_obj_set_size(ui_scr->page_meter_img_7, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_7, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_7, GUI_CTRL_IMG);

	//Write codes page_meter_img_left_turn
	ui_scr->page_meter_img_left_turn = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_left_turn, gui_get_res_path(GUI_RES_LAMP_01_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_left_turn, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_left_turn, 0);
	lv_image_set_scale(ui_scr->page_meter_img_left_turn, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_left_turn, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_left_turn, 43, 99);
	lv_obj_set_size(ui_scr->page_meter_img_left_turn, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_left_turn, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_left_turn, GUI_CTRL_IMG);

	//Write codes page_meter_img_right_turn
	ui_scr->page_meter_img_right_turn = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_right_turn, gui_get_res_path(GUI_RES_LAMP_04_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_right_turn, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_right_turn, 0);
	lv_image_set_scale(ui_scr->page_meter_img_right_turn, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_right_turn, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_right_turn, 927, 108);
	lv_obj_set_size(ui_scr->page_meter_img_right_turn, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_right_turn, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_right_turn, GUI_CTRL_IMG);

	//Write codes page_meter_imgList_bluetooth
	ui_scr->page_meter_imgList_bluetooth = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imgList_bluetooth, 612, 23);
	lv_obj_set_size(ui_scr->page_meter_imgList_bluetooth, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imgList_bluetooth, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imgList_bluetooth, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imgList_bluetooth, (const void **) page_meter_imgList_bluetooth_imgs, 2);
	lv_imglist_set_act(ui_scr->page_meter_imgList_bluetooth, 0);

	//Write codes page_meter_imgList_phone_bl
	ui_scr->page_meter_imgList_phone_bl = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imgList_phone_bl, 679, 23);
	lv_obj_set_size(ui_scr->page_meter_imgList_phone_bl, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imgList_phone_bl, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imgList_phone_bl, GUI_CTRL_IMGLIST);

	//Set style for page_meter_imgList_phone_bl. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_meter_imgList_phone_bl, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_meter_imgList_phone_bl, (const void **) page_meter_imgList_phone_bl_imgs, 2);
	lv_imglist_set_act(ui_scr->page_meter_imgList_phone_bl, 0);

	//Write codes page_meter_imgList_gps
	ui_scr->page_meter_imgList_gps = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imgList_gps, 745, 23);
	lv_obj_set_size(ui_scr->page_meter_imgList_gps, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imgList_gps, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imgList_gps, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imgList_gps, (const void **) page_meter_imgList_gps_imgs, 2);
	lv_imglist_set_act(ui_scr->page_meter_imgList_gps, 0);

	//Write codes page_meter_imgList_networksignal
	ui_scr->page_meter_imgList_networksignal = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imgList_networksignal, 813, 23);
	lv_obj_set_size(ui_scr->page_meter_imgList_networksignal, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imgList_networksignal, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imgList_networksignal, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imgList_networksignal, (const void **) page_meter_imgList_networksignal_imgs, 2);
	lv_imglist_set_act(ui_scr->page_meter_imgList_networksignal, 0);

	//Write codes page_meter_img_ready
	ui_scr->page_meter_img_ready = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_ready, gui_get_res_path(GUI_RES_READY_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_ready, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_ready, 0);
	lv_image_set_scale(ui_scr->page_meter_img_ready, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_ready, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_ready, 375, 23);
	lv_obj_set_size(ui_scr->page_meter_img_ready, 131, 29);
	lv_obj_add_flag(ui_scr->page_meter_img_ready, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_ready, GUI_CTRL_IMG);

	//Set style for page_meter_img_ready. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_meter_img_ready, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_imglist_4
	ui_scr->page_meter_imglist_4 = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_4, 28, 5);
	lv_obj_set_size(ui_scr->page_meter_imglist_4, 68, 68);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_4, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_4, (const void **) page_meter_imglist_4_imgs, 9);
	lv_imglist_set_act(ui_scr->page_meter_imglist_4, 0);

	//Write codes page_meter_lbl_1
	ui_scr->page_meter_lbl_1 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text(ui_scr->page_meter_lbl_1, "32℃");
	lv_label_set_long_mode(ui_scr->page_meter_lbl_1, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_meter_lbl_1, 114, 11);
	lv_obj_set_size(ui_scr->page_meter_lbl_1, 131, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_lbl_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_lbl_1, GUI_CTRL_LABEL);

	//Set style for page_meter_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_lbl_1, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_imgList_gear_mode
	ui_scr->page_meter_imgList_gear_mode = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imgList_gear_mode, 241, 27);
	lv_obj_set_size(ui_scr->page_meter_imgList_gear_mode, 100, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imgList_gear_mode, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imgList_gear_mode, GUI_CTRL_IMGLIST);

	//Set style for page_meter_imgList_gear_mode. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_meter_imgList_gear_mode, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_meter_imgList_gear_mode, (const void **) page_meter_imgList_gear_mode_imgs, 3);
	lv_imglist_set_act(ui_scr->page_meter_imgList_gear_mode, 0);

	//Write codes page_meter_img_1
	ui_scr->page_meter_img_1 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_1, gui_get_res_path(GUI_RES_LAMP_08_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_1, 0);
	lv_image_set_scale(ui_scr->page_meter_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_1, 751, 141);
	lv_obj_set_size(ui_scr->page_meter_img_1, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_1, GUI_CTRL_IMG);

	//Write codes page_meter_img_3
	ui_scr->page_meter_img_3 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_3, gui_get_res_path(GUI_RES_LAMP_14_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_3, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_3, 0);
	lv_image_set_scale(ui_scr->page_meter_img_3, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_3, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_3, 751, 206);
	lv_obj_set_size(ui_scr->page_meter_img_3, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_3, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_3, GUI_CTRL_IMG);

	//Write codes page_meter_img_10
	ui_scr->page_meter_img_10 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_10, gui_get_res_path(GUI_RES_LAMP_26_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_10, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_10, 0);
	lv_image_set_scale(ui_scr->page_meter_img_10, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_10, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_10, 821, 206);
	lv_obj_set_size(ui_scr->page_meter_img_10, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_10, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_10, GUI_CTRL_IMG);

	//Write codes page_meter_img_11
	ui_scr->page_meter_img_11 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_11, gui_get_res_path(GUI_RES_LAMP_07_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_11, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_11, 0);
	lv_image_set_scale(ui_scr->page_meter_img_11, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_11, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_11, 826, 141);
	lv_obj_set_size(ui_scr->page_meter_img_11, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_11, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_11, GUI_CTRL_IMG);

	//Write codes page_meter_img_12
	ui_scr->page_meter_img_12 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_12, gui_get_res_path(GUI_RES_LAMP_12_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_12, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_12, 0);
	lv_image_set_scale(ui_scr->page_meter_img_12, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_12, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_12, 156, 209);
	lv_obj_set_size(ui_scr->page_meter_img_12, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_12, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_12, GUI_CTRL_IMG);

	//Write codes page_meter_img_13
	ui_scr->page_meter_img_13 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_13, gui_get_res_path(GUI_RES_LAMP_14_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_13, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_13, 0);
	lv_image_set_scale(ui_scr->page_meter_img_13, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_13, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_13, 156, 288);
	lv_obj_set_size(ui_scr->page_meter_img_13, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_13, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_13, GUI_CTRL_IMG);

	//Write codes page_meter_img_14
	ui_scr->page_meter_img_14 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_14, gui_get_res_path(GUI_RES_LAMP_08_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_14, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_14, 0);
	lv_image_set_scale(ui_scr->page_meter_img_14, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_14, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_14, 221, 143);
	lv_obj_set_size(ui_scr->page_meter_img_14, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_14, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_14, GUI_CTRL_IMG);

	//Write codes page_meter_img_15
	ui_scr->page_meter_img_15 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_15, gui_get_res_path(GUI_RES_LAMP_13_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_15, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_15, 0);
	lv_image_set_scale(ui_scr->page_meter_img_15, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_15, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_15, 221, 208);
	lv_obj_set_size(ui_scr->page_meter_img_15, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_15, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_15, GUI_CTRL_IMG);

	//Write codes page_meter_img_16
	ui_scr->page_meter_img_16 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_16, gui_get_res_path(GUI_RES_LAMP_18_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_16, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_16, 0);
	lv_image_set_scale(ui_scr->page_meter_img_16, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_16, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_16, 547, 23);
	lv_obj_set_size(ui_scr->page_meter_img_16, 50, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_16, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_16, GUI_CTRL_IMG);

	//Write codes page_meter_img_17
	ui_scr->page_meter_img_17 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_17, gui_get_res_path(GUI_RES_LAMP_24_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_17, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_17, 0);
	lv_image_set_scale(ui_scr->page_meter_img_17, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_17, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_17, 43, 172);
	lv_obj_set_size(ui_scr->page_meter_img_17, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_17, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_17, GUI_CTRL_IMG);

	//Write codes page_meter_img_18
	ui_scr->page_meter_img_18 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_18, gui_get_res_path(GUI_RES_LAMP_22_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_18, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_18, 0);
	lv_image_set_scale(ui_scr->page_meter_img_18, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_18, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_18, 927, 176);
	lv_obj_set_size(ui_scr->page_meter_img_18, 52, 51);
	lv_obj_add_flag(ui_scr->page_meter_img_18, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_18, GUI_CTRL_IMG);

	// Write Timer page_meter_timer_1
	if(ui_scr->page_meter_timer_1 != NULL) lv_timer_delete(ui_scr->page_meter_timer_1);
	ui_scr->page_meter_timer_1 = lv_timer_create(page_meter_timer_1_timer_cb, 50, "");
	lv_timer_set_repeat_count(ui_scr->page_meter_timer_1, -1);

	// Write Timer page_meter_timer_2
	if(ui_scr->page_meter_timer_2 != NULL) lv_timer_delete(ui_scr->page_meter_timer_2);
	ui_scr->page_meter_timer_2 = lv_timer_create(page_meter_timer_2_timer_cb, 1000, "");
	lv_timer_set_repeat_count(ui_scr->page_meter_timer_2, -1);

	// Write Timer page_meter_timer_3
	if(ui_scr->page_meter_timer_3 != NULL) lv_timer_delete(ui_scr->page_meter_timer_3);
	ui_scr->page_meter_timer_3 = lv_timer_create(page_meter_timer_3_timer_cb, 1000, "");
	lv_timer_set_repeat_count(ui_scr->page_meter_timer_3, -1);

	//Write codes page_meter_imglist_speed_tens
	ui_scr->page_meter_imglist_speed_tens = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_speed_tens, 336, 149);
	lv_obj_set_size(ui_scr->page_meter_imglist_speed_tens, 168, 168);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_speed_tens, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_speed_tens, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_speed_tens, (const void **) page_meter_imglist_speed_tens_imgs, 10);
	lv_imglist_set_act(ui_scr->page_meter_imglist_speed_tens, 0);

	//Write codes page_meter_imglist_speed_ones
	ui_scr->page_meter_imglist_speed_ones = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_speed_ones, 532, 150);
	lv_obj_set_size(ui_scr->page_meter_imglist_speed_ones, 168, 168);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_speed_ones, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_speed_ones, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_speed_ones, (const void **) page_meter_imglist_speed_ones_imgs, 10);
	lv_imglist_set_act(ui_scr->page_meter_imglist_speed_ones, 0);

	//Write codes page_meter_img_21
	ui_scr->page_meter_img_21 = lv_image_create(ui_scr->page_meter);
	lv_image_set_src(ui_scr->page_meter_img_21, gui_get_res_path(GUI_RES_LAMP_19_PNG));
	lv_image_set_pivot(ui_scr->page_meter_img_21, 0, 0);
	lv_image_set_rotation(ui_scr->page_meter_img_21, 0);
	lv_image_set_scale(ui_scr->page_meter_img_21, 256);
	lv_image_set_inner_align(ui_scr->page_meter_img_21, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_meter_img_21, 43, 540);
	lv_obj_set_size(ui_scr->page_meter_img_21, 52, 48);
	lv_obj_add_flag(ui_scr->page_meter_img_21, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_meter_img_21, GUI_CTRL_IMG);

	//Write codes page_meter_imglist_5
	ui_scr->page_meter_imglist_5 = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_5, 319, 540);
	lv_obj_set_size(ui_scr->page_meter_imglist_5, 55, 44);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_5, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_5, (const void **) page_meter_imglist_5_imgs, 2);
	lv_imglist_set_act(ui_scr->page_meter_imglist_5, 0);

	//Write codes page_meter_lbl_2
	ui_scr->page_meter_lbl_2 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text(ui_scr->page_meter_lbl_2, "100km");
	lv_label_set_long_mode(ui_scr->page_meter_lbl_2, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_meter_lbl_2, 128, 535);
	lv_obj_set_size(ui_scr->page_meter_lbl_2, 156, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_lbl_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_lbl_2, GUI_CTRL_LABEL);

	//Set style for page_meter_lbl_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_lbl_2, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_lbl_3
	ui_scr->page_meter_lbl_3 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text(ui_scr->page_meter_lbl_3, "100%");
	lv_label_set_long_mode(ui_scr->page_meter_lbl_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_meter_lbl_3, 612, 535);
	lv_obj_set_size(ui_scr->page_meter_lbl_3, 110, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_lbl_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_lbl_3, GUI_CTRL_LABEL);

	//Set style for page_meter_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_lbl_3, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_lbl_4
	ui_scr->page_meter_lbl_4 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text(ui_scr->page_meter_lbl_4, "ODO");
	lv_label_set_long_mode(ui_scr->page_meter_lbl_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_meter_lbl_4, 728, 540);
	lv_obj_set_size(ui_scr->page_meter_lbl_4, 88, 48);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_lbl_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_lbl_4, GUI_CTRL_LABEL);

	//Set style for page_meter_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_lbl_4, &lv_font_MiSans_Medium_29, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_lbl_5
	ui_scr->page_meter_lbl_5 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text(ui_scr->page_meter_lbl_5, "10260km");
	lv_label_set_long_mode(ui_scr->page_meter_lbl_5, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_meter_lbl_5, 821, 535);
	lv_obj_set_size(ui_scr->page_meter_lbl_5, 198, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_lbl_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_lbl_5, GUI_CTRL_LABEL);

	//Set style for page_meter_lbl_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_lbl_5, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_digitclock_1
	ui_scr->page_meter_digitclock_1 = lv_label_create(ui_scr->page_meter);
	lv_label_set_text_fmt(ui_scr->page_meter_digitclock_1, "%02d:%02d", 21 % 12, 25);
	lv_obj_set_style_text_align(ui_scr->page_meter_digitclock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_pos(ui_scr->page_meter_digitclock_1, 868, 20);
	lv_obj_set_size(ui_scr->page_meter_digitclock_1, 137, 68);
	ui_style_set(ui_scr->page_meter_digitclock_1, GUI_CTRL_DIGITALCLOCK);

	//Set style for page_meter_digitclock_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_text_font(ui_scr->page_meter_digitclock_1, &lv_font_MiSans_Medium_38, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_meter_imglist_bat
	ui_scr->page_meter_imglist_bat = lv_imglist_create(ui_scr->page_meter);
	lv_obj_set_pos(ui_scr->page_meter_imglist_bat, 385, 556);
	lv_obj_set_size(ui_scr->page_meter_imglist_bat, 205, 13);
	lv_obj_set_scrollbar_mode(ui_scr->page_meter_imglist_bat, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_meter_imglist_bat, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_meter_imglist_bat, (const void **) page_meter_imglist_bat_imgs, 100);
	lv_imglist_set_act(ui_scr->page_meter_imglist_bat, 99);

	lv_obj_update_layout(ui_scr->page_meter);
	ui_scr->page_meter_del = false;

	//Init events for screen
	events_init_page_meter(ui);
	return ui_scr->page_meter;
}
void unload_scr_page_meter(lv_ui *ui)
{
	lv_ui_page_meter * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
	if(ui_scr->page_meter_timer_1 != NULL) {
		lv_timer_delete(ui_scr->page_meter_timer_1);
		ui_scr->page_meter_timer_1 = NULL;
	}
	if(ui_scr->page_meter_timer_2 != NULL) {
		lv_timer_delete(ui_scr->page_meter_timer_2);
		ui_scr->page_meter_timer_2 = NULL;
	}
	if(ui_scr->page_meter_timer_3 != NULL) {
		lv_timer_delete(ui_scr->page_meter_timer_3);
		ui_scr->page_meter_timer_3 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_METER);
}
