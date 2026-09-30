#ifndef __WDT_H__
#define __WDT_H__

enum {
    WDT_1MS = 0,
    WDT_2MS,
    WDT_4MS,
    WDT_8MS,
    WDT_16MS,
    WDT_32MS,
    WDT_64MS,
    WDT_128MS,
    WDT_256MS,
    WDT_512MS,
    WDT_1S,
    WDT_2S,
    WDT_4S,
    WDT_8S,
    WDT_16S,
    WDT_32S,
    WDT_LRC_1MS,
    WDT_LRC_2MS,
    WDT_LRC_4MS,
    WDT_LRC_8MS,
    WDT_LRC_16MS,
    WDT_LRC_32MS,
    WDT_LRC_64MS,
    WDT_LRC_128MS,
    WDT_LRC_256MS,
    WDT_LRC_512MS,
    WDT_LRC_1S,
    WDT_LRC_2S,
    WDT_LRC_4S,
};



enum {
    WDT_CLK_RCLK,
    WDT_CLK_LRC,
};

void wdt_init(u32 time);
void wdt_config(u32 time, u32 clk, u32 lat);
void wdt_enable();
void wdt_disable();
void wdt_clear();
u32 wdt_get_time();//ms

#endif
