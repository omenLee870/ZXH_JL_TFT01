#ifndef __RES_CONFIG_H__
#define __RES_CONFIG_H__
#include "app_config.h"

#if   TCFG_NANDFLASH_UI_FAT_ENABLE &&(!TCFG_VIRFAT_INSERT_FLASH_ENABLE)
#define EXTERN_PATH "storage/"TCFG_NANDFLASH_UI_FAT_LOGO"/C/"   // 用于存储JL sidebar font
#else
#define EXTERN_PATH "storage/virfat_flash/C/"
#endif
#define INTERN_PATH "mnt/sdfile/res/"

#define RES_PATH   EXTERN_PATH

#define UPGRADE_PATH   INTERN_PATH"ui_upgrade/"


#if TCFG_NANDFLASH_DEV_ENABLE&&(!TCFG_SDFILE_INSERT_FLASH_ENABLE)
#if   TCFG_NANDFLASH_UI_FAT_ENABLE
#define MODE_PATH "storage/"TCFG_NANDFLASH_UI_FAT_LOGO"/C/"   // 用于存储JL sidebar font
#else
#define MODE_PATH "storage/virfat_flash/C/"   // 用于存储JL sidebar font
#endif
#else
#define MODE_PATH "storage/res_nor_mode/C/"   // 用于存储JL sidebar font
#endif



#define JL_PATH   MODE_PATH"JL/"
#define SIDEBAR_PATH   MODE_PATH"sidebar/"
#define FONT_PATH  MODE_PATH"font/"

#define UI_STY_CHECK_PATH     \
    MODE_PATH"JL/JL.sty",      \
    RES_PATH"watch/watch.sty",\
    RES_PATH"watch1/watch1.sty",\
    RES_PATH"watch2/watch2.sty",\
    RES_PATH"watch3/watch3.sty",\
    RES_PATH"watch4/watch4.sty",\
    RES_PATH"watch5/watch5.sty",

#define UI_RES_CHECK_PATH  \
    MODE_PATH"JL/JL.res",   \
    RES_PATH"watch/watch.res",\
    RES_PATH"watch1/watch1.res",\
    RES_PATH"watch2/watch2.res",\
    RES_PATH"watch3/watch3.res",\
    RES_PATH"watch4/watch4.res",\
    RES_PATH"watch5/watch5.res",\

#define UI_STR_CHECK_PATH      \
    MODE_PATH"JL/JL.str",       \
    RES_PATH"watch/watch.str", \
    RES_PATH"watch1/watch1.str",\
    RES_PATH"watch2/watch2.str",\
    RES_PATH"watch3/watch3.str",\
    RES_PATH"watch4/watch4.str",\
    RES_PATH"watch5/watch5.str",


#define UI_STY_WATCH_PATH     \
    RES_PATH"watch/watch.sty",\
    RES_PATH"watch1/watch1.sty",\
    RES_PATH"watch2/watch2.sty",\
    RES_PATH"watch3/watch3.sty",\
    RES_PATH"watch4/watch4.sty",\
    RES_PATH"watch5/watch5.sty",


#define UI_USED_DOUBLE_BUFFER   1//使用双buf推屏
#define UI_WATCH_RES_ENABLE     1//表盘功能
#define UI_UPGRADE_RES_ENABLE   1//升级界面功能

#ifdef CONFIG_BOARD_JL707N_CSC_DEMO    //仓
#define WATCH_RES_NAME			"WATCH"
#define WATCH_RES_NAME_SMALL	"watch"
#define BGP_RES_NAME			"VIE"
#define BGP_RES_NAME_SMALL		"vie"
#else//手表
#define WATCH_RES_NAME			"WATCH"
#define WATCH_RES_NAME_SMALL	"watch"
#define BGP_RES_NAME			"BGP_W"
#define BGP_RES_NAME_SMALL		"bgp_w"
#define AVI_RES_NAME			"AVI"
#define AVI_RES_NAME_SMALL		"avi"
#endif

#endif
