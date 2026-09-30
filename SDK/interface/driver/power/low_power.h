#ifndef __LOW_POWER_H__
#define __LOW_POWER_H__

//config
#define LOWPOWER_CLOSE 0
#define SLEEP_EN 1
#define DEEP_SLEEP_EN 2

//osc_type
enum LOWPOWER_OSC_TYPE {
    OSC_TYPE_LRC,
    OSC_TYPE_BT_OSC,
    OSC_TYPE_NULL,
};

void low_power_sys_request(void *priv);

s32 low_power_trace_drift(u32 usec);

void low_power_reset_osc_type(u8 type);

u8 low_power_get_default_osc_type(void);

u8 low_power_get_osc_type(void);

void power_set_soft_poweroff(void);

void lowpower_init();

void lowpower_uninit();

void lowpower_dump();

#include "power_manage.h"

#endif

