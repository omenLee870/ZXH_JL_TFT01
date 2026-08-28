/**@file  		power_api.h
* @brief        电源、低功耗
* @details
* @author
* @date     	2021-8-26
* @version  	V1.0
* @copyright    Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
 */
#ifndef __POWER_API__
#define __POWER_API__

#define P11_POWEROFF_DATA	//AT(.p11_poweroff_data)
#define P11_POWEROFF_BSS	//AT(.p11_poweroff_bss)
#define P11_POWEROFF_CODE	//_NOINLINE_ AT(.p11_poweroff_code)

#define AT_VOLATILE_RAM_POWER        	//AT(.power_driver.data)
#define AT_VOLATILE_RAM_BSS_POWER      	//AT(.power_driver.data.bss)
#define AT_VOLATILE_RAM_CODE_POWER      AT(.power_driver.text.cache.L1)

#define AT_VOLATILE_RAM_LOWPOWER        //AT_VOLATILE_RAM_POWER		//AT(.power_driver.data.overlay)
#define AT_VOLATILE_RAM_BSS_LOWPOWER    //AT(.power_driver.data.bss.overlay)//AT_VOLATILE_RAM_BSS_POWER
#define AT_VOLATILE_RAM_CODE_LOWPOWER	AT_VOLATILE_RAM_CODE_POWER	//AT(.power_driver.text.cache.L1.overlay)

#define AT_VOLATILE_CACHE_LOWPOWER
#define AT_VOLATILE_CACHE_BSS_LOWPOWER
#define AT_VOLATILE_CACHE_CODE_LOWPOWER	//AT(.power_driver.text.cache.fetch)



//
//
//                    platform_data
//
//
//
//******************************************************************
struct _power_pdata {
};

//
//
//                    power_api
//
//
//
//******************************************************************
#include "power_manage.h"

void power_early_init(u32 arg);

void power_later_init(u32 arg);

void power_init(struct _power_pdata *pdata);


//
//
//                  lowpower
//
//
//
//******************************************************************
void __power_recover(void);

void p11_lowpower_schedule(void);

void low_power_enable();

void low_power_disable();
#endif
