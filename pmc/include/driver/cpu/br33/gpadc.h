#ifndef __GPADC_H__
#define __GPADC_H__

#include "gpadc_hw.h"


enum {
    M2P_MSG_GPADC_SUSPEND,
    M2P_MSG_GPADC_RESUME,
    M2P_MSG_GPADC_INFO_INIT,
};


u32 adc_get_voltage(enum AD_CH ch);

void adc_suspend(void);

void adc_resume(void);

void adc_info_init(void);

void sortDescending(u16 arr[], u32 n);

u32 gpadc_battery_get_voltage(void);

void pmc_gpadc_resume(void);

void pmc_gpadc_suspend(void);

u32 pmc_gpadc_is_working(void);

#endif

