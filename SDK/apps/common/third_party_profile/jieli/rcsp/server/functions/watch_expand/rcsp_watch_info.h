#ifndef __RCSP_SMARTBOX_COMMON_INFO_H__
#define __RCSP_SMARTBOX_COMMON_INFO_H__

#include "typedef.h"
#include "app_config.h"

#define SMARTBOX_COMMON_INFO_SCREEN_BRIGHTENESS     (0x0001) // 屏幕亮度信息
#define SMARTBOX_COMMON_INFO_FUNCTION_RESOURCES     (0x0004) // 功能使用资源
#define SMARTBOX_COMMON_INFO_CAT1_MODULE		    (0x0005) // 4G模块信息
#define SMARTBOX_COMMON_INFO_WATCH_DIAL_EXT		    (0x0007) // 表盘扩展信息
#define SMARTBOX_COMMON_INFO_DEVICE_SDK_INFO        (0x0008) // 设备SDK信息

/*设备SDK信息 */
#define RCSP_COMMON_INFO_CHARGING_CASE              (0x0001) //充电仓
#define RCSP_COMMON_INFO_SCREEN_BOX                 (0x0001) //彩屏舱
#define RCSP_COMMON_INFO_CHIP_BR28                  (0x0001) //芯片系列 AC701N
#define RCSP_COMMON_INFO_CHIP_BR35                  (0x0002) //芯片系列 AC707N

/*彩屏仓*/
#define RCSP_SCREEN_BOX_FUNC_SCREEN_SAVER_CODE        0x01 //屏幕保护程序
#define RCSP_SCREEN_BOX_FUNC_BOOT_ANIMATION_CODE      0x02 //开机动画
#define RCSP_SCREEN_BOX_FUNC_WALLPAPER_CODE           0x03 //墙纸

#define RCSP_DEVICE_SDK_INFO_BIT_SUPPORT_GIF		(0)	//是否支持gif

typedef struct {
    u16 function;
    u8 version;
    u8 op;
    u8 data[0];
} smartbox_common_info_set_cmd_t;

extern u8 rcsp_common_info_set_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);


#endif//__RCSP_SMARTBOX_COMMON_INFO_H__

