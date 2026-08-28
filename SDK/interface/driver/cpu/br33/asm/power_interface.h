#ifndef __POWER_INTERFACE_H__
#define __POWER_INTERFACE_H__

#include "generic/typedef.h"
#include "gpio.h"

//-------------------------------------------------------
/* p33
 */
#include "power/p33/p33_sfr.h"
#include "power/p33/p33_access.h"
#include "power/p33/charge_hw.h"
#include "power/p33/p33_api.h"
#include "power/wdt.h"

//-------------------------------------------------------
/* p11
 */
#include "power/pmc/pmc_csfr.h"
#include "power/pmc/pmc_sfr.h"
#include "power/pmc/pmc_mmap.h"
#include "power/pmc/pmc_api.h"
#include "power/pmc/lp_ipc.h"
#include "power/pmc/pmc_q32dsp_csfr.h"


//-------------------------------------------------------
/* power
 */
#include "power/power_api.h"
#include "power/power_wakeup.h"
#include "power/power_reset.h"
#include "power/power_port.h"
#include "power/port.h"
#include "power/power_gate.h"
#include "power/ipc_spin_lock.h"

//-------------------------------------------------------
/* other
 */
#include "power/power_app.h"
#include "power/power_compat.h"

#endif
