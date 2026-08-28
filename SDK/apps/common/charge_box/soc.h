#ifndef __SOC__
#define __SOC__

#include "cpu.h"
#define SOC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define SOC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SOC_CLAMP(min, val, max) (SOC_MAX(min, (SOC_MIN(val, max))))
//  开机前电压判断
void usr_check_power_on_voltage(void);

//  初始化
void soc_check_init();

// VDDIO上电、update等
void set_soc_first_work(u8 i);

//  电量获取
int get_curr_soc();

#endif