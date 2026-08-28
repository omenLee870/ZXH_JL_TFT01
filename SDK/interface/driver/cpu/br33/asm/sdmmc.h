#ifndef ARCH_SDMMC_H
#define ARCH_SDMMC_H


#include "device/sdmmc/sdmmc.h"


u8 sd_io_suspend(u8 sdx, u8 sdx_io);
u8 sd_io_resume(u8 sdx, u8 sdx_io);

void sdx_dev_detect_timer_add();
void sdx_dev_detect_timer_del();

#endif

