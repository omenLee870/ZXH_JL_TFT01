#ifndef __GPCNT_H__
#define __GPCNT_H__

#include "typedef.h"

enum gpcnt_gss_css_src {
    GPCNT_SRC_DISABLE0,//disable
    GPCNT_SRC_RC16M,
    GPCNT_SRC_STRC_32K,
    GPCNT_SRC_RC250K,
    GPCNT_SRC_PAT_CLK,
    GPCNT_SRC_HSB_CLK,
    GPCNT_SRC_LSB_CLK,
    GPCNT_SRC_STD24M,
    GPCNT_SRC_STD48M,
    GPCNT_SRC_PLL96M,
    GPCNT_SRC_BTOSC_24M,//org
    GPCNT_SRC_BTOSC_48M,
    GPCNT_SRC_SYS_PLL_D1P0,//org
    GPCNT_SRC_WAT_CLK,
    GPCNT_SRC_RING_CLK,
    GPCNT_SRC_PMC_DBG_OUT,
    GPCNT_SRC_P33_CLK_DBG,
    GPCNT_SRC_FLL_24M,
    GPCNT_SRC_HUSB_SOF_PLS,
    GPCNT_SRC_XOSC_32K,
    GPCNT_SRC_XPLL_D1P0,
    GPCNT_SRC_MIPIPLL_D1P0,
    GPCNT_SRC_USBPLL_D1P0,
    GPCNT_SRC_P2M_PLL0_CLK,
    GPCNT_SRC_P2M_PLL1_CLK,
    GPCNT_SRC_USB_UTMI_CLK,
    GPCNT_SRC_ASS_DBG_CKO,
    GPCNT_SRC_CAP_MUX_IN,
    GPCNT_SRC_CLK_MUX_IN,

    // GPCNT_SRC_LRC_CLK,
    // GPCNT_SRC_P11_DBG_OUT,
    // GPCNT_SRC_P33_SIG_DBG0,
    // GPCNT_SRC_P33_SIG_DBG1,
    // GPCNT_SRC_BSB_PLL_D1P0,
    // GPCNT_SRC_LRC_24M,
    // GPCNT_SRC_IRFLT_IN,
};

#define GPCNT_MAX_HW_NUM                 4
#define GPCNT_HW_REG     JL_GPCNT0,JL_GPCNT1,JL_GPCNT2,JL_GPCNT3
typedef enum {
    GPCNT0,
    GPCNT1,
    GPCNT2,
    GPCNT3,
} gpcnt_dev;

#define GPCNT_TRIM_OUTSIDE_CLK_GSS_SRC      GPCNT_SRC_CLK_MUX_IN
#define GPCNT_TRIM_OUTSIDE_CLK_PORT_SRC     PORT_FUNC_CLK_PIN//PORT_FUNC_CLK_MUX
#define GPCNT_TRIM_OUTSIDE_CLK_CSS_SRC      GPCNT_SRC_PLL96M
#define GPCNT_TRIM_OUTSIDE_CLK_CSS_FRE      96000000
#define GPCNT_TRIM_INSIDE_CLK_CSS_SRC       GPCNT_SRC_PLL96M
#define GPCNT_TRIM_INSIDE_CLK_CSS_FRE       96000000


#define gpcnt_enable(reg)                ((reg)->CON |= BIT(0))
#define gpcnt_disable(reg)               ((reg)->CON &= ~BIT(0))
#define gpcnt_pnd(reg)                   ((reg)->CON & BIT(31))
#define gpcnt_clr_pnd(reg)               ((reg)->CON |= BIT(30))
#define gpcnt_ie_enable(reg)             ((reg)->CON |= BIT(29))
#define gpcnt_ie_disable(reg)            ((reg)->CON &= ~BIT(29))
#define gpcnt_gts_pre(reg, val)          SFR((reg)->CON, 22, 2, val)
#define gpcnt_gts(reg, val)              SFR((reg)->CON, 16, 4, val)
#define gpcnt_gss(reg, val)              SFR((reg)->CON, 8, 5, val) //
#define gpcnt_css(reg, val)              SFR((reg)->CON, 1, 5, val) //
#define gpcnt_close_cfg                  BIT(30)
#define gpcnt_w_reg_con(reg, val)        ((reg)->CON = (val))
#define gpcnt_r_reg_con(reg)             ((reg)->CON)
#define gpcnt_r_reg_num(reg)             ((reg)->NUM)

u32 gpcnt_get_num_blocking(gpcnt_dev gpcnt, enum gpcnt_gss_css_src gss_src, char n, enum gpcnt_gss_css_src css_src);
//test_time_ms:测试耗时(ms)
//xss_src:待测试的内部时钟
//return:待测试的内部时钟频率(hz)
u32 gpcnt_trim_inside_clk(gpcnt_dev gpcnt, u32 test_time_ms, enum gpcnt_gss_css_src xss_src);
//test_time_ms:测试耗时(400ms)
//test_clk_freq:待测试的外部时钟粗略频率(hz)(未知写0)
//test_gpio:待测试的外部时钟输入引脚
//return:待测试的外部时钟精确频率(hz)
u32 gpcnt_trim_outside_clk(gpcnt_dev gpcnt, u32 test_time_ms, u32 test_clk_freq, u8 test_gpio);
#endif

