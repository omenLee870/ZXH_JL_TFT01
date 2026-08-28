#ifndef _COMMON_API_H_
#define _COMMON_API_H_

#include  "generic/includes.h"

//重启or关机
extern void common_api_reboot_or_shutdown(u8 flag, u8 erase);

//停止所有连接，恢复出厂，等待蓝牙配对
extern void common_api_disconnect_all_restore();

//断开蓝牙进入配对模式
extern void common_api_disconnect_start_pair();

//获取电量百分比
extern void common_api_get_bat_percent();

extern void update_bat_info_vm();
extern void alipay_power_on(void);
extern bool ble_list_clear_all(void);
extern int small_file_del_all(void);
extern void erase_UIInfo_in_vm();
//来电：拒接
//通话：开始/停止录音
extern void common_api_call_income_hangup();
#endif
