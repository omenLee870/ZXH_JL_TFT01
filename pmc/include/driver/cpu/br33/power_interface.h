#ifndef __POWER_INTERFACE_H__
#define __POWER_INTERFACE_H__

#include "includes.h"

//-------------------------------------------------------
/* p33
 */
#include "power/p33/p33_sfr.h"
#include "power/p33/p33_api.h"
#include "power/p33/p33_access.h"

//-------------------------------------------------------
/* pmc
 */
#include "power/pmc/pmc_csfr.h"
#include "power/pmc/pmc_sfr.h"
#include "power/pmc/lp_ipc.h"
#include "power/pmc/pmc_mmap.h"
#include "power/pmc/pmc_api.h"
#include "power/pmc/pmc_rom_api.h"
#include "power/pmc/pmc_clock_hw.h"
#include "power/pmc/pmc_q32dsp_csfr.h"

//-------------------------------------------------------
/* power
 */
#include "power/power_app.h"
#include "power/power_api.h"
#include "power/power_wakeup.h"
#include "power/power_port.h"
#include "power/lib_power_config.h"


#endif
