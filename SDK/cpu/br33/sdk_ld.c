// *INDENT-OFF*

#include "app_config.h"
#include "audio_config_def.h"

/******************************************************************
 *系统跑sdram或者ddr，配置在sdk_ld.c
 *系统不带sdram，配置在sdk_ld_sfc.c
 *****************************************************************/

#ifdef CONFIG_EXTRAM_CODE_ENABLE
#include "sdk_ld_sdram.c"
#else
#include "sdk_ld_sfc.c"
#endif

#include "sdk_ld_pmc_ram.c"

