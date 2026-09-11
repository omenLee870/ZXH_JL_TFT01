#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lwip_fixme.data.bss")
#pragma data_seg(".lwip_fixme.data")
#pragma const_seg(".lwip_fixme.const")
#pragma code_seg(".lwip_fixme.text")
#endif
#include "system/includes.h"

#define CPU_RAND()	(JL_RAND->R64L)
unsigned int random32(int type)
{
    return CPU_RAND();
}


/* u32 OSGetTime(void) */
/* { */
/* return jiffies; */
/* } */


#endif
