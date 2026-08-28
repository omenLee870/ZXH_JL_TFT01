#include "asm/power_interface.h"
#include "init.h"
#include "msys_pmc_app.h"

//ram_psel[0]: msys ram0/ram1 pmc control selection, 0: msys control, 1: pmc control
//ram_psel[1]: msys ram2/ram3 pmc control selection, 0: msys control, 1: pmc control
//ram_psel[2]: msys ram4/ram5 pmc control selection, 0: msys control, 1: pmc control
//ram_psel[3]: msys ram6/ram7 pmc control selection, 0: msys control, 1: pmc control
void msys_ram_share_ctl(u8 ram_psel)
{
    u8 cmng;
    cmng = P3_MNV_PSEL0 & 0xf;
    cmng = cmng ^ ram_psel;

    SFR(JL_CMNG->CON0, 8, 4, ~(cmng & 0xf));
    SFR(JL_PMC_CMNG->CON0, 8, 4, ~(cmng & 0xf));
    asm volatile("csync");
    SFR(P3_MNV_PSEL0, 0, 4, ram_psel); // psel
    asm volatile("csync");
    SFR(JL_PMC_CMNG->CON0, 8, 4, 0xf);
    SFR(JL_CMNG->CON0, 8, 4, 0xf);
    asm volatile("csync");
}

//static int ram_share_init()
int ram_share_init()
{
#if TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM
    printf("P3_MNV_PSEL0: %d", P3_MNV_PSEL0);
    printf("ram_share_init, size: %d", TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM);
    u8 psel = BIT(TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM - 1);
    msys_ram_share_ctl(psel);
    printf("P3_MNV_PSEL0: %d", P3_MNV_PSEL0);
#endif

    return 0;
}

