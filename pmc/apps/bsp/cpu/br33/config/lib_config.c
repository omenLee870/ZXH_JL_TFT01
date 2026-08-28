#include "includes.h"

//==========================================================//
//              DVAD第三方算法使能开关                      //
//==========================================================//
const int CONFIG_DVAD_DEVELOP_ENABLE = 0;

const int CONFIG_DISTURB_SCAN_ENABLE    = 0;//config_disturb_scan_enable
const int CONFIG_BLE_CIS_SUPPORT        = 0;

const int CONFIG_BT_PMC_MSG_INFO  = 0;
const int CONFIG_WIFI_DETECT_ENABLE  = 0;


//gptimer
const u32 lib_gptimer_src_lsb_clk = 0; //时钟源不支持lsb_clk, :Hz
const u32 lib_gptimer_src_std_clk = 12 * 1000 * 1000; //td_x_clk, µ¥Î»:Hz
const u8 lib_gptimer_timer_mode_en = 1; //gptimer timer功能使能
const u8 lib_gptimer_pwm_mode_en = 1; //gptimer pwm功能使能
const u8 lib_gptimer_capture_mode_en = 1; //gptimer capture功能使能
const u8 lib_gptimer_auto_tid_en = 1; //gptimer_tid 内部自动分配使能
