#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio_config.h"
#include "asm/clock_hal.h"

#if (TCFG_DVDD_POWER_MODE==CLOCK_DVDD_POWER_SHORT) && (TCFG_LOWPOWER_LOWPOWER_SEL==2)
#error "dvdd短接dcvdd不允许进入deepsleep"
#endif

//-----------------------------------------------------------------------------------------------------------------------
/* power_param
 */
struct _power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,
    .btosc_hz       = TCFG_CLOCK_OSC_HZ,
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,
    .osc_type       = TCFG_LOWPOWER_OSC_TYPE,
};

//----------------------------------------------------------------------------------------------------------------------
/* power_pdata
 */
struct _power_pdata power_pdata = {
    .power_param_p  = &power_param,
};

//----------------------------------------------------------------------------------------------------------------------
void key_wakeup_init();
int ram_share_init();

void board_power_init()
{
    ram_share_init();

    gpio_config_init();

    power_control(PCONTROL_PD_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);
    power_control(PCONTROL_SF_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);

    power_control(PCONTROL_SF_KEEP_LRC, 1);

    power_set_dcdc_type(TCFG_DCDC_TYPE);

    /*poff p11 默认进入poff模式*/
    power_control(PCONTROL_PD_KEEP_NVDD, 1);
    /*soff p11 掉电*/
    power_control(PCONTROL_SF_KEEP_PVDD, 0);
    power_control(PCONTROL_SF_KEEP_NVDD, 0);

    power_init(&power_pdata);

#if (!TCFG_CHARGE_ENABLE)
    power_set_mode(TCFG_LOWPOWER_POWER_SEL);
#endif

    /* #if !defined(BR33_FPGA_CODE_VERIFY) */
    key_wakeup_init();
    /* #endif */
}
