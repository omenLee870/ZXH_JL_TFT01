
#ifndef __SYSCFG_VOTP_ID_H__
#define __SYSCFG_VOTP_ID_H__

#include "typedef.h"
#include "device/vm.h"


//==========================================//
//         votp 配置项ID分配说明         //
//==========================================//
//存储在虚拟VOTP区域
enum votp_id {
    CFG_ID_VOTP_FLASH_INFO_V1 = 0,
    CFG_ID_VOTP_FLASH_INFO_V2 = 1,
    // CFG_ID_VOTP_CALIBRATION, //校准信息
};
#define     CFG_ID_VOTP_LRC_TRIM_VALUE      0x3fe




//==========================================//
//         OTP 配置项ID分配说明         //
//==========================================//

#define     CFG_ID_OTP_LRC_TRIM_VALUE       0x3fe



#endif

