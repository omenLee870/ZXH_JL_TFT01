#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".battery_offset.data.bss")
#pragma data_seg(".battery_offset.data")
#pragma const_seg(".battery_offset.text.const")
#pragma code_seg(".battery_offset.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "battery_manager.h"
#include "gpadc.h"

#define LOG_TAG             "[BATTERY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



#if TCFG_BATTER_OFFSET_EN

#define VBAT_OFFSET_NUM				5

static const u16 vbat_offset_clk_Mhz[VBAT_OFFSET_NUM] = {
    48, 96, 144, 230, 288,
};
static const u8 vbat_offset_value_max[VBAT_OFFSET_NUM] = {
    0, 3, 5, 13, 16
};
static u8 vbat_offset_value[VBAT_OFFSET_NUM];
static u8 vbat_offset_idx;
static u16 vbat_adc_1V; //每1V对应的adc数值
static u8 vbat_usr_mA[VBAT_OFFSET_USR_MAX]; //用户层耗电补偿

static void vbat_offset_refresh(void)
{
    int offset = vbat_offset_value[vbat_offset_idx];
    int total_mA = 0;
    for (int i = 0; i < ARRAY_SIZE(vbat_usr_mA); i++) {
        total_mA += vbat_usr_mA[i];
    }
    offset += (total_mA * vbat_adc_1V * 1 / 10 / 1000); // 0.1欧内阻
    log_info("offset:%d \n", offset);
    gpadc_battery_set_offset(offset);
}

static void clock_critical_enter(void)
{
}
static void clock_critical_exit(void)
{
    int hsb_clk = clk_get("sys") / MHz;
    int i = VBAT_OFFSET_NUM - 1;
    for (; i > 0; i--) {
        if (hsb_clk >= vbat_offset_clk_Mhz[i]) {
            break;
        }
    }
    vbat_offset_idx = i;
    /* log_info("vbat_offset_idx:%d \n", vbat_offset_idx); */
    vbat_offset_refresh();
}
HSB_CRITICAL_HANDLE_REG(vbat_adc, clock_critical_enter, clock_critical_exit)


void gpadc_battery_offset_trim(u16 ref_vol, u16 adc_value)
{
    vbat_adc_1V = ((u32)adc_value) * 1000 / ref_vol;
    log_info("vbat_adc_1V:%d \n", vbat_adc_1V);

    u32 sys_clk = clk_get("sys");

    u32 vbat_adc[VBAT_OFFSET_NUM];
    u32 vbat_offset_tmp[VBAT_OFFSET_NUM];
    for (int i = 0; i < VBAT_OFFSET_NUM; i++) {
        clk_set_api("sys", vbat_offset_clk_Mhz[i] * MHz);
        vbat_adc[i] = adc_get_value_blocking_filter(AD_CH_PMU_VBAT, 30);
        vbat_offset_tmp[i] = vbat_adc[0] - vbat_adc[i];
        // 补一部分
        if (vbat_offset_tmp[i] * 3 / 4 > vbat_offset_value_max[i]) {
            vbat_offset_value[i] = vbat_offset_value_max[i];
        } else {
            vbat_offset_value[i] = vbat_offset_tmp[i] * 3 / 4;
        }
    }

    clk_set_api("sys", sys_clk);

    for (int i = 0; i < VBAT_OFFSET_NUM; i++) {
        log_info("sys:%4dMhz, vbat adc:%4d, offset tmp:%4d, offset value:%4d \n",
                 vbat_offset_clk_Mhz[i], vbat_adc[i], vbat_offset_tmp[i], vbat_offset_value[i]);
    }
    vbat_offset_refresh();

    /* os_time_dly(150); */
}

void battery_offset_usr_set(enum battery_offset_usr idx, u8 mA)
{
    if (idx < VBAT_OFFSET_USR_MAX) {
        vbat_usr_mA[idx] = mA;
        vbat_offset_refresh();
    }
}


#else /* #if TCFG_BATTER_OFFSET_EN */

void battery_offset_usr_set(enum battery_offset_usr idx, u8 mA)
{
}

#endif /* #if TCFG_BATTER_OFFSET_EN */

