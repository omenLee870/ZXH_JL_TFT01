#include "includes.h"
#include "gpio.h"
#include "ipc_spin_lock.h"
#include "icache.h"

#define LOG_TAG_CONST       MAIN
#define LOG_TAG             "[setup]"
#include "generic/log.h"

void app_main();
void p11_cmac_init(void);
void bredr_bd_init();
void maskrom_init();
void pmu_free_ram_init(void);

__attribute__((weak))
unsigned long audio_jiffies_usec(void)
{
    return 0;
}

AT(.volatile_ram_code)
int main(void)
{
    //分支预测使能
    pmc_q32DSP(core_num())->PMU_CON1 &= ~BIT(8);

    maskrom_init();
    local_irq_disable();
    IcuInitial();

    //clock_early_init(PLL_REF_RC16M, 16 * MHz, 96 * MHz);
    clock_early_init(PLL_REF_XOSC, 24 * MHz, 96 * MHz);

    power_early_flowing(0);

    pmu_free_ram_init();

#if CONFIG_UART_DEBUG_ENABLE
    debug_uart_init(CONFIG_DEBUG_UART_TX_PIN);
#endif /* #if CONFIG_UART_DEBUG_ENABLE */

    log_info("\n============ Hello BR33 PMC Q32F ============\n");

    clock_dump();

    debug_init(); //异常检测初始化

    board_power_init();

    local_irq_enable();

    app_main();
}



