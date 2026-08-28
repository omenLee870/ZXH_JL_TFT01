#include "rdec_soft.h"
#include "gptimer.h"
#include "clock.h"
#include "gpio.h"
#include "debug.h"

extern void wdt_clear();

void rdec_soft_demo()
{
    printf("rdec_soft_demo()\n");
    const struct rdec_soft_config rdec_config = {
        .rdec_a = IO_PORTA_02,
        .rdec_b = IO_PORTA_03,
        .filter_us = 1000,
        .mode = RDEC_SOFT_PHASE_2,
        .tid = TIMER0,
    };
    u32 rdec_id = rdec_soft_init(&rdec_config);
    rdec_soft_start(rdec_id);

    while (1) {
        int value = rdec_soft_get_value(rdec_id);
        if (value != 0) {
            printf("( %d )", value);
        }
        mdelay(100);
        wdt_clear();
    }
}


#include "rdec.h"
void rdec_hw_demo()
{
    printf("rdec_hw_demo()\n");
    //常用demo
    const struct rdec_config user_cfg = {
        .rdec_a = IO_PORTA_02,
        .rdec_b = IO_PORTA_03,
        .phase_mode = RDEC_PHASE_2,
        .work_mode = RDEC_WORK_AUTO,
        .rdec_x = RDEC_0,
    };
    //根据实际电路选择输入模式
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_INPUT_PULLUP_10K);
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_PULLUP_10K);
    u32 id = rdec_init(&user_cfg);
    rdec_start(id);
    rdec_dump(id);

    //用户自己添加定时器,调用rtdec_get_value轮询
    while (1) {
        s8 data = rdec_get_value(id);
        if (data != 0) {
            printf("(%d)\n", data);
        }
        wdt_clear();
        udelay(100 * 1000);
    }
}

