#ifndef __LPCTMU_H__
#define __LPCTMU_H__


#include "gpio.h"


#define LPCTMU_CH_SIZE  8


enum lpctmu_clk_src {
    LPCTMU_STRC_32K,
    LPCTMU_STD_12M,
};

struct ch_en_cfg {
    u8 ch0: 1;
    u8 ch1: 1;
    u8 ch2: 1;
    u8 ch3: 1;
    u8 ch4: 1;
    u8 ch5: 1;
    u8 ch6: 1;
    u8 ch7: 1;
};

struct lpctmu_platform_data {
    u16 aim_vol_delta;
    u16 aim_charge_khz;
    enum lpctmu_clk_src clk_sel;
    u8 sample_window_time;
    u8 sample_scan_time;
    union {
        u8 val;
        struct ch_en_cfg cfg;
    } ch_en;
    u8 wkp_en;
    u8 wkp_ch;
    u8 sf_keep;
};

#define LPCTMU_PLATFORM_DATA_BEGIN(data) \
    static const struct lpctmu_platform_data data = {

#define LPCTMU_PLATFORM_DATA_END() \
};

void lpctmu_init(const struct lpctmu_platform_data *pdata);
void lpctmu_kstart(void *priv);
u32 get_lpctmu_value(u8 ch);
u32 get_lpctmu_ch_by_ch_idx(u32 ch_idx);
u32 get_lpctmu_ch_idx_by_ch(u32 ch);

void lpctmu_get_p33_res_limit(u16 *limh, u16 *liml);

u32 lpctmu_lptimer_is_working(void);
u32 lpctmu_is_working(void);

u32 lpctmu_is_sf_keep(void);
void lpctmu_enable(void);
void lpctmu_disable(void);
void lpctmu_lowpower_enter(u32 lowpwr_mode);
void lpctmu_lowpower_exit(u32 lowpwr_mode);



#endif
