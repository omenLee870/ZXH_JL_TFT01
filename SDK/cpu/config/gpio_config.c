#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gpio_config.data.bss")
#pragma data_seg(".gpio_config.data")
#pragma const_seg(".gpio_config.text.const")
#pragma code_seg(".gpio_config.text")
#endif

/*
important, sdk gpio_config!!!
important, sdk gpio config!!!
important, sdk gpio config!!!

1.soff/poweron reset
2.power_early_flowing, 所有io初始化为高阻
3.gpio_init, 根据配置初始化io
4.latch release
5.sdk io init
6.soff flowing, do platform_uninitcall{
	1.gpio_uninit，配置关机io的状态
	2.other flowing......
}
 */
#include "app_config.h"
#include "gpio_config.h"
#include "syscfg_id.h"

//--------------------------------------------------------
/*config
 */
#define __TCFG_IO_CFG_AT_POWER_ON	TCFG_IO_CFG_AT_POWER_ON
#define __TCFG_IO_CFG_AT_POWER_OFF	TCFG_IO_CFG_AT_POWER_OFF


//--------------------------------------------------------
/*app flowing
 */
struct gpio_cfg_item {
    u8 len;
    u16 uuid;
    u8  mode;
    u8  hd;
} __attribute__((packed));

int gpio_config_init()
{
#if __TCFG_IO_CFG_AT_POWER_ON
    struct gpio_cfg_item item;

    puts("gpio_cfg_at_power_on\n");

    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWERON, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        gpio_init(gpio / 16, &config);
    }
#endif
    return 0;
}

int gpio_config_uninit()
{
#if __TCFG_IO_CFG_AT_POWER_OFF
    struct gpio_cfg_item item;
    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWEROFF, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        gpio_init(gpio / 16, &config);
    }
#endif

    return 0;
}
