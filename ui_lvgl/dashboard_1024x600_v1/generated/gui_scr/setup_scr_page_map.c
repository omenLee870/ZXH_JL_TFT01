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

static const char* page_map_imgList_gear_mode_imgs[3] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\SPORT.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\TTFAR.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\FREE.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000015.zip",
	"storage/res_nor_mode/C/lvgl/4b000016.zip",
	"storage/res_nor_mode/C/lvgl/4b000017.zip"
#endif
};
static const char* page_map_imgList_bluetooth_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_25.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_26.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000019.zip",
	"storage/res_nor_mode/C/lvgl/4b00001a.zip"
#endif
};
static const char* page_map_imgList_phone_bl_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_23.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_24.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001b.zip",
	"storage/res_nor_mode/C/lvgl/4b00001c.zip"
#endif
};
static const char* page_map_imgList_gps_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_29.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_30.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001d.zip",
	"storage/res_nor_mode/C/lvgl/4b00001e.zip"
#endif
};
static const char* page_map_imgList_networksignal_imgs[2] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_27.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_28.png"
#else
	"storage/res_nor_mode/C/lvgl/4b00001f.zip",
	"storage/res_nor_mode/C/lvgl/4b000020.zip"
#endif
};
static const char* page_map_imgList_soc_beam_imgs[3] = {
#if LV_USE_GUIBUILDER_SIMULATOR
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_20.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_21.png",
	"A:\\omenLee_project\\ZXH-TFT\\380tft_1\\tft\\ui_lvgl\\dashboard_1024x600_v1\\import\\image\\alarm\\Lamp_22.png"
#else
	"storage/res_nor_mode/C/lvgl/4b000023.zip",
	"storage/res_nor_mode/C/lvgl/4b000024.zip",
	"storage/res_nor_mode/C/lvgl/4b000025.zip"
#endif
};
static const char* page_map_imglist_bat_imgs[100] = {
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
	"storage/res_nor_mode/C/lvgl/4b000088.zip",
	"storage/res_nor_mode/C/lvgl/4b000089.zip"
#endif
};

lv_obj_t * setup_scr_page_map(lv_ui *ui)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
	if (ui_scr == NULL) {
		ui_scr = lv_malloc(sizeof(lv_ui_page_map));
		if (ui_scr == NULL) return NULL;
		lv_memset(ui_scr, 0, sizeof(lv_ui_page_map));
		ui->page_map = ui_scr;
	}

	//Write codes page_map
	ui_scr->page_map = lv_obj_create(NULL);
	lv_obj_set_scrollbar_mode(ui_scr->page_map, LV_SCROLLBAR_MODE_OFF);
	lv_obj_remove_flag(ui_scr->page_map, LV_OBJ_FLAG_SCROLLABLE);
	ui_style_set(ui_scr->page_map, GUI_CTRL_SCR);

	//Set style for page_map. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map, lv_color_make(0x55, 0xaa, 0xa5), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_map, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui_scr->page_map, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui_scr->page_map, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_view_1
	ui_scr->page_map_view_1 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_1, 0, 0);
	lv_obj_set_size(ui_scr->page_map_view_1, 1024, 89);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_1, GUI_CTRL_CONT);

	//Set style for page_map_view_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->page_map_view_1, 51, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_gear_mode
	ui_scr->page_map_imgList_gear_mode = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_gear_mode, 202, 33);
	lv_obj_set_size(ui_scr->page_map_imgList_gear_mode, 100, 23);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_gear_mode, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_gear_mode, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_gear_mode. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_gear_mode, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_gear_mode, (const void **) page_map_imgList_gear_mode_imgs, 3);
	lv_imglist_set_act(ui_scr->page_map_imgList_gear_mode, 0);

	//Write codes page_map_lbl_time
	ui_scr->page_map_lbl_time = lv_label_create(ui_scr->page_map_view_1);
	lv_label_set_text(ui_scr->page_map_lbl_time, "10:30");
	lv_label_set_long_mode(ui_scr->page_map_lbl_time, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_time, 400, 14);
	lv_obj_set_size(ui_scr->page_map_lbl_time, 125, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_time, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_time, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_time. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_time, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_time, &lv_font_MiSans_Medium_34, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_weather
	ui_scr->page_map_img_weather = lv_image_create(ui_scr->page_map_view_1);
	lv_image_set_src(ui_scr->page_map_img_weather, gui_get_res_path(GUI_RES_TEM_01_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_weather, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_weather, 0);
	lv_image_set_scale(ui_scr->page_map_img_weather, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_weather, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_weather, 573, 15);
	lv_obj_set_size(ui_scr->page_map_img_weather, 54, 51);
	lv_obj_add_flag(ui_scr->page_map_img_weather, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_weather, GUI_CTRL_IMG);

	//Set style for page_map_img_weather. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_weather, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_temp
	ui_scr->page_map_lbl_temp = lv_label_create(ui_scr->page_map_view_1);
	lv_label_set_text(ui_scr->page_map_lbl_temp, "25°C");
	lv_label_set_long_mode(ui_scr->page_map_lbl_temp, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_temp, 638, 14);
	lv_obj_set_size(ui_scr->page_map_lbl_temp, 112, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_temp, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_temp, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_temp. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_temp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_temp, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_temp, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_temp, &lv_font_MiSans_Medium_36, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_bluetooth
	ui_scr->page_map_imgList_bluetooth = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_bluetooth, 791, 20);
	lv_obj_set_size(ui_scr->page_map_imgList_bluetooth, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_bluetooth, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_bluetooth, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_bluetooth, (const void **) page_map_imgList_bluetooth_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_bluetooth, 0);

	//Write codes page_map_imgList_phone_bl
	ui_scr->page_map_imgList_phone_bl = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_phone_bl, 849, 20);
	lv_obj_set_size(ui_scr->page_map_imgList_phone_bl, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_phone_bl, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_phone_bl, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_phone_bl. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_phone_bl, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_phone_bl, (const void **) page_map_imgList_phone_bl_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_phone_bl, 0);

	//Write codes page_map_imgList_gps
	ui_scr->page_map_imgList_gps = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_gps, 899, 20);
	lv_obj_set_size(ui_scr->page_map_imgList_gps, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_gps, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_gps, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_gps, (const void **) page_map_imgList_gps_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_gps, 0);

	//Write codes page_map_imgList_networksignal
	ui_scr->page_map_imgList_networksignal = lv_imglist_create(ui_scr->page_map_view_1);
	lv_obj_set_pos(ui_scr->page_map_imgList_networksignal, 956, 20);
	lv_obj_set_size(ui_scr->page_map_imgList_networksignal, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_networksignal, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_networksignal, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imgList_networksignal, (const void **) page_map_imgList_networksignal_imgs, 2);
	lv_imglist_set_act(ui_scr->page_map_imgList_networksignal, 0);

	//Write codes page_map_img_ready
	ui_scr->page_map_img_ready = lv_image_create(ui_scr->page_map_view_1);
	lv_image_set_src(ui_scr->page_map_img_ready, gui_get_res_path(GUI_RES_READY_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_ready, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_ready, 0);
	lv_image_set_scale(ui_scr->page_map_img_ready, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_ready, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_ready, 35, 31);
	lv_obj_set_size(ui_scr->page_map_img_ready, 131, 29);
	lv_obj_add_flag(ui_scr->page_map_img_ready, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_ready, GUI_CTRL_IMG);

	//Set style for page_map_img_ready. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_ready, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_view_2
	ui_scr->page_map_view_2 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_2, 0, 523);
	lv_obj_set_size(ui_scr->page_map_view_2, 1024, 75);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_2, GUI_CTRL_CONT);

	//Write codes page_map_lbl_endurance
	ui_scr->page_map_lbl_endurance = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_endurance, "100km");
	lv_label_set_long_mode(ui_scr->page_map_lbl_endurance, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_endurance, 95, 10);
	lv_obj_set_size(ui_scr->page_map_lbl_endurance, 163, 53);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_endurance, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_endurance, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_endurance. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_endurance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_endurance, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_endurance, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_endurance, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_charge
	ui_scr->page_map_img_charge = lv_image_create(ui_scr->page_map_view_2);
	lv_image_set_src(ui_scr->page_map_img_charge, gui_get_res_path(GUI_RES_LAMP_19_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_charge, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_charge, 0);
	lv_image_set_scale(ui_scr->page_map_img_charge, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_charge, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_charge, 25, 14);
	lv_obj_set_size(ui_scr->page_map_img_charge, 52, 51);
	lv_obj_add_flag(ui_scr->page_map_img_charge, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_charge, GUI_CTRL_IMG);

	//Set style for page_map_img_charge. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_img_charge, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imgList_soc_beam
	ui_scr->page_map_imgList_soc_beam = lv_imglist_create(ui_scr->page_map_view_2);
	lv_obj_set_pos(ui_scr->page_map_imgList_soc_beam, 394, 14);
	lv_obj_set_size(ui_scr->page_map_imgList_soc_beam, 52, 51);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imgList_soc_beam, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imgList_soc_beam, GUI_CTRL_IMGLIST);

	//Set style for page_map_imgList_soc_beam. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_imgList_soc_beam, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_imglist_set_src(ui_scr->page_map_imgList_soc_beam, (const void **) page_map_imgList_soc_beam_imgs, 3);
	lv_imglist_set_act(ui_scr->page_map_imgList_soc_beam, 1);

	//Write codes page_map_lbl_bat
	ui_scr->page_map_lbl_bat = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_bat, "100%");
	lv_label_set_long_mode(ui_scr->page_map_lbl_bat, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_bat, 467, 10);
	lv_obj_set_size(ui_scr->page_map_lbl_bat, 109, 53);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_bat, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_bat, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_bat. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_bat, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_bat, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_bat, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_bat, &lv_font_MiSans_Medium_32, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_odo_num
	ui_scr->page_map_lbl_odo_num = lv_label_create(ui_scr->page_map_view_2);
	lv_label_set_text(ui_scr->page_map_lbl_odo_num, "10260km");
	lv_label_set_long_mode(ui_scr->page_map_lbl_odo_num, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_odo_num, 811, 6);
	lv_obj_set_size(ui_scr->page_map_lbl_odo_num, 203, 61);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_odo_num, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_odo_num, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_odo_num. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(ui_scr->page_map_lbl_odo_num, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_color(ui_scr->page_map_lbl_odo_num, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_outline_opa(ui_scr->page_map_lbl_odo_num, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_odo_num, &lv_font_MiSans_Medium_34, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_odo_num, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_imglist_bat
	ui_scr->page_map_imglist_bat = lv_imglist_create(ui_scr->page_map_view_2);
	lv_obj_set_pos(ui_scr->page_map_imglist_bat, 606, 30);
	lv_obj_set_size(ui_scr->page_map_imglist_bat, 205, 13);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_imglist_bat, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_imglist_bat, GUI_CTRL_IMGLIST);
	lv_imglist_set_src(ui_scr->page_map_imglist_bat, (const void **) page_map_imglist_bat_imgs, 100);
	lv_imglist_set_act(ui_scr->page_map_imglist_bat, 99);

	// Write Timer page_map_timer_1
	if(ui_scr->page_map_timer_1 != NULL) lv_timer_delete(ui_scr->page_map_timer_1);
	ui_scr->page_map_timer_1 = lv_timer_create(page_map_timer_1_timer_cb, 20, "");
	lv_timer_set_repeat_count(ui_scr->page_map_timer_1, -1);

	//Write codes page_map_view_3
	ui_scr->page_map_view_3 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_3, 35, 156);
	lv_obj_set_size(ui_scr->page_map_view_3, 265, 145);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_3, GUI_CTRL_CONT);

	//Set style for page_map_view_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_view_3, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_view_3, lv_color_make(0x08, 0x17, 0x27), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_1
	ui_scr->page_map_lbl_1 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_1, "328");
	lv_label_set_long_mode(ui_scr->page_map_lbl_1, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_1, 106, 23);
	lv_obj_set_size(ui_scr->page_map_lbl_1, 92, 49);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_1, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_1, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_1, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_2
	ui_scr->page_map_lbl_2 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_2, "22");
	lv_label_set_long_mode(ui_scr->page_map_lbl_2, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_2, 4, 81);
	lv_obj_set_size(ui_scr->page_map_lbl_2, 92, 49);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_2, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_2, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_2, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_3
	ui_scr->page_map_lbl_3 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_3, "45");
	lv_label_set_long_mode(ui_scr->page_map_lbl_3, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_3, 74, 81);
	lv_obj_set_size(ui_scr->page_map_lbl_3, 92, 49);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_3, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_3, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_3, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_3, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_4
	ui_scr->page_map_lbl_4 = lv_label_create(ui_scr->page_map_view_3);
	lv_label_set_text(ui_scr->page_map_lbl_4, "12:20");
	lv_label_set_long_mode(ui_scr->page_map_lbl_4, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_4, 157, 81);
	lv_obj_set_size(ui_scr->page_map_lbl_4, 92, 49);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_4, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_4, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_4, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui_scr->page_map_lbl_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_1
	ui_scr->page_map_img_1 = lv_image_create(ui_scr->page_map_view_3);
	lv_image_set_src(ui_scr->page_map_img_1, gui_get_res_path(GUI_RES_LAMP_01_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_1, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_1, 0);
	lv_image_set_scale(ui_scr->page_map_img_1, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_1, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_1, 23, 20);
	lv_obj_set_size(ui_scr->page_map_img_1, 52, 51);
	lv_obj_add_flag(ui_scr->page_map_img_1, LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_1, GUI_CTRL_IMG);

	// Write Timer page_map_timer_2
	if(ui_scr->page_map_timer_2 != NULL) lv_timer_delete(ui_scr->page_map_timer_2);
	ui_scr->page_map_timer_2 = lv_timer_create(page_map_timer_2_timer_cb, 5000, "");
	lv_timer_set_repeat_count(ui_scr->page_map_timer_2, -1);

	//Write codes page_map_btn_1
	ui_scr->page_map_btn_1 = lv_btn_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_btn_1, 435, 99);
	lv_obj_set_size(ui_scr->page_map_btn_1, 74, 44);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_btn_1, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_btn_1, GUI_CTRL_BUTTON);

	//Set style for page_map_btn_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_btn_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_btn_1, lv_color_make(0x40, 0xEC, 0xC2), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_btn_1, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_btn_1, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->page_map_btn_1_label = lv_label_create(ui_scr->page_map_btn_1);
	lv_label_set_text(ui_scr->page_map_btn_1_label, "back");
	lv_obj_set_style_pad_all(ui_scr->page_map_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_map_btn_1_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_map_btn_2
	ui_scr->page_map_btn_2 = lv_btn_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_btn_2, 516, 99);
	lv_obj_set_size(ui_scr->page_map_btn_2, 74, 44);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_btn_2, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_btn_2, GUI_CTRL_BUTTON);

	//Set style for page_map_btn_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_btn_2, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_btn_2, lv_color_make(0x40, 0xEC, 0xC2), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_btn_2, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_btn_2, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
	ui_scr->page_map_btn_2_label = lv_label_create(ui_scr->page_map_btn_2);
	lv_label_set_text(ui_scr->page_map_btn_2_label, "next");
	lv_obj_set_style_pad_all(ui_scr->page_map_btn_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(ui_scr->page_map_btn_2_label, LV_ALIGN_CENTER, 0, 0);

	//Write codes page_map_view_4
	ui_scr->page_map_view_4 = lv_obj_create(ui_scr->page_map);
	lv_obj_set_pos(ui_scr->page_map_view_4, 691, 93);
	lv_obj_set_size(ui_scr->page_map_view_4, 314, 210);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_view_4, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_view_4, GUI_CTRL_CONT);

	//Set style for page_map_view_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_radius(ui_scr->page_map_view_4, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_scr->page_map_view_4, lv_color_make(0xFF, 0xFF, 0xFF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui_scr->page_map_view_4, 128, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_5
	ui_scr->page_map_lbl_5 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_5, "resolution:");
	lv_label_set_long_mode(ui_scr->page_map_lbl_5, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_5, 4, 3);
	lv_obj_set_size(ui_scr->page_map_lbl_5, 147, 35);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_5, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_5, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_5. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_5, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_5, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_5, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_6
	ui_scr->page_map_lbl_6 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_6, "fps:max,min,avg");
	lv_label_set_long_mode(ui_scr->page_map_lbl_6, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_6, 4, 49);
	lv_obj_set_size(ui_scr->page_map_lbl_6, 279, 43);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_6, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_6, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_6. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_6, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_6, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_6, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_7
	ui_scr->page_map_lbl_7 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_7, "0,0,0");
	lv_label_set_long_mode(ui_scr->page_map_lbl_7, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_7, 4, 86);
	lv_obj_set_size(ui_scr->page_map_lbl_7, 295, 35);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_7, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_7, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_7. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_7, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_7, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_7, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_8
	ui_scr->page_map_lbl_8 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_8, "jpgsize:max,min,avg");
	lv_label_set_long_mode(ui_scr->page_map_lbl_8, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_8, 4, 119);
	lv_obj_set_size(ui_scr->page_map_lbl_8, 305, 43);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_8, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_8, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_8. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_8, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_8, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_8, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_9
	ui_scr->page_map_lbl_9 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_9, "0,0,0");
	lv_label_set_long_mode(ui_scr->page_map_lbl_9, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_9, 1, 160);
	lv_obj_set_size(ui_scr->page_map_lbl_9, 295, 35);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_9, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_9, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_9. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_9, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_9, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_9, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_lbl_10
	ui_scr->page_map_lbl_10 = lv_label_create(ui_scr->page_map_view_4);
	lv_label_set_text(ui_scr->page_map_lbl_10, "1024x600");
	lv_label_set_long_mode(ui_scr->page_map_lbl_10, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_pos(ui_scr->page_map_lbl_10, 145, 5);
	lv_obj_set_size(ui_scr->page_map_lbl_10, 150, 35);
	lv_obj_set_scrollbar_mode(ui_scr->page_map_lbl_10, LV_SCROLLBAR_MODE_OFF);
	ui_style_set(ui_scr->page_map_lbl_10, GUI_CTRL_LABEL);

	//Set style for page_map_lbl_10. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_color(ui_scr->page_map_lbl_10, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui_scr->page_map_lbl_10, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_scr->page_map_lbl_10, &lv_font_MiSans_Medium_18, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes page_map_img_2
	ui_scr->page_map_img_2 = lv_image_create(ui_scr->page_map);
	lv_image_set_src(ui_scr->page_map_img_2, gui_get_res_path(GUI_RES_TIPS_PNG));
	lv_image_set_pivot(ui_scr->page_map_img_2, 0, 0);
	lv_image_set_rotation(ui_scr->page_map_img_2, 0);
	lv_image_set_scale(ui_scr->page_map_img_2, 256);
	lv_image_set_inner_align(ui_scr->page_map_img_2, LV_IMAGE_ALIGN_TOP_LEFT);
	lv_obj_set_pos(ui_scr->page_map_img_2, 460, 183);
	lv_obj_set_size(ui_scr->page_map_img_2, 460, 164);
	lv_obj_add_flag(ui_scr->page_map_img_2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	ui_style_set(ui_scr->page_map_img_2, GUI_CTRL_IMG);

	//Set style for page_map_img_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
	lv_obj_set_style_bg_opa(ui_scr->page_map_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_update_layout(ui_scr->page_map);
	ui_scr->page_map_del = false;

	//Init events for screen
	events_init_page_map(ui);
	return ui_scr->page_map;
}
void unload_scr_page_map(lv_ui *ui)
{
	lv_ui_page_map * ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
	if(ui_scr->page_map_timer_1 != NULL) {
		lv_timer_delete(ui_scr->page_map_timer_1);
		ui_scr->page_map_timer_1 = NULL;
	}
	if(ui_scr->page_map_timer_2 != NULL) {
		lv_timer_delete(ui_scr->page_map_timer_2);
		ui_scr->page_map_timer_2 = NULL;
	}
	ui_free_scr_ptr(ui, GUI_SCREEN_PAGE_MAP);
}
