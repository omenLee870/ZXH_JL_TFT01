#include "system/includes.h"
#include "cpu/includes.h"
#include "app_config.h"


#include "asm/sdmmc.h"


static int sdpg_power_level = 0xff;

/**
 * @param power_level 驱动等级, sdio_operat_enum
 */

void sdpg_config(int power_level)
{
    if (power_level == sdpg_power_level) {
        return;
    }

    sdpg_power_level = power_level;

#if defined(TCFG_SDPG_TIEUP_PORT) && (TCFG_SDPG_TIEUP_PORT!=NO_CONFIG_PORT)
    gpio_set_mode(IO_PORT_SPILT(TCFG_SDPG_TIEUP_PORT), PORT_HIGHZ);
#endif

    if (power_level == SDPG_IO_HI_Z) {                  //断电，IO设高阻

        PMC_PORTP->DIR |=  BIT(2);
        PMC_PORTP->DIE &= ~BIT(2);
        PMC_PORTP->OUT &= ~BIT(2);
        SFR(PMC_PORTP->PU, 2 * 2, 2, 0b00);
        SFR(PMC_PORTP->PD, 2 * 2, 2, 0b00);

    } else if (power_level <= SDPG_IO_OUTPUT_H_HD_HD0_SDPG_JL) {           //供电，IO_HD常开

        if (PMC_PORTP->OUT & BIT(2)) {
            return;
        }

        SFR(PMC_IOMC->SDPG_CON, 3, 1, 1);//soft_en
        udelay(1);
        SFR(PMC_PORTP->OUT, 2, 1, 1);//dir
        SFR(PMC_PORTP->DIR, 2, 1, 0);//dir
        mdelay(4);
        SFR(PMC_IOMC->SDPG_CON, 3, 1, 0);//soft_en

    } else {                            //断电，IO输出0

        PMC_PORTP->DIR |=  BIT(2);
        PMC_PORTP->DIE &= ~BIT(2);
        PMC_PORTP->OUT &= ~BIT(2);
        SFR(PMC_PORTP->PU, 2 * 2, 2, 0b00);
        SFR(PMC_PORTP->PD, 2 * 2, 2, 0b00);

#if defined(TCFG_SDPG_TIEUP_PORT) && (TCFG_SDPG_TIEUP_PORT!=NO_CONFIG_PORT)
        gpio_set_mode(IO_PORT_SPILT(TCFG_SDPG_TIEUP_PORT), PORT_INPUT_PULLDOWN_LEVEL1);
        u32 cnt = 0;
        while (1) {
            if (0 == gpio_read(TCFG_SDPG_TIEUP_PORT)) {
                break;
            }
            udelay(100);
            cnt ++;
            if (cnt > 100) {
                break;
            }
        }
        gpio_set_mode(IO_PORT_SPILT(TCFG_SDPG_TIEUP_PORT), PORT_OUTPUT_LOW);
#endif
    }
}

/**
 * @return power_level驱动等级, 0:高阻，1~4:输出高逐级增强，>=5:则输出低
 */
int get_sdpg_status(void)
{
    if (sdpg_power_level == 0xff) {
        return SDPG_IO_HI_Z;
    }
    return sdpg_power_level;
}

static void sdpg_close_pre_softoff_cbfunc(void)
{
    int sdpg_state = get_sdpg_status();
    if (sdpg_state == SDPG_IO_HI_Z) {
        return;
    }
    sdpg_config(SDPG_IO_HI_Z);
}
platform_uninitcall(sdpg_close_pre_softoff_cbfunc);

#if TCFG_SDPG_ALWAYS_ENABLE

static int sdpg_early_open(void)
{
    sdpg_config(SDPG_IO_OUTPUT_H);
    os_time_dly(1);
    sdpg_config(SDPG_IO_OUTPUT_H_HD_HD0);
    os_time_dly(1);
    sdpg_config(SDPG_IO_OUTPUT_H_HD_HD0_SDPG_USER);
    return 0;
}
early_initcall(sdpg_early_open);

#endif

