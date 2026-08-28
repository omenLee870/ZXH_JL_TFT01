#ifndef __CLOCK_H__
#define __CLOCK_H__


#define MHz         (1000000L)



enum pll_ref_source : u8 {
    PLL_REF_NULL,
    PLL_REF_PAT_CLK,
    PLL_REF_RC16M,
    PLL_REF_EXT_CLK,
    PLL_REF_XOSC,
    PLL_REF_FLL24M,
};


enum pll_clk_div {
    CLK_DIV1 = 0,
    CLK_DIV3,
    CLK_DIV5,
    CLK_DIV7,

    CLK_DIV1X2 = 4,
    CLK_DIV3X2,
    CLK_DIV5X2,
    CLK_DIV7X2,

    CLK_DIV1X4 = 8,
    CLK_DIV3X4,
    CLK_DIV5X4,
    CLK_DIV7X4,

    CLK_DIV1X8 = 12,
    CLK_DIV3X8,
    CLK_DIV5X8,
    CLK_DIV7X8,
};


enum pll_sys_src {
    PLL_SYS_SRC_NULL = 0,
    PLL_SYS_SRC_DPLL0_D1P0,
    PLL_SYS_SRC_DPLL0_D2P0,
    PLL_SYS_SRC_DPLL1_D1P0,
    PLL_SYS_SRC_DPLL1_D2P0,
    PLL_SYS_SRC_M2P_PLL0_CLK,
    PLL_SYS_SRC_M2P_PLL1_CLK,
};

enum sys_clock_src {
    SYS_CLOCK_RC16M,
    SYS_CLOCK_RCLK,
    SYS_CLOCK_PLL_CLK,
    SYS_CLOCK_STD24M,
    SYS_CLOCK_STD48M,
    SYS_CLOCK_FLL24M,
    SYS_CLOCK_CLK_X2,
    SYS_CLOCK_TEST_CLK,
};

void clock_set_sys_clk_src(enum sys_clock_src src);
enum sys_clock_src clock_get_sys_clk_src();


enum CLK_OUT_SOURCE0 {
    CLK_OUT_SRC0_NULL = 0,
    CLK_OUT_SRC0_RC_16M,
    CLK_OUT_SRC0_RCCLK,
    CLK_OUT_SRC0_STRC_32K,
    CLK_OUT_SRC0_SRC_CLK,
    CLK_OUT_SRC0_SYS_CLK,
    CLK_OUT_SRC0_SLEEP_R,
    CLK_OUT_SRC0_STD_12M,
    CLK_OUT_SRC0_STD_24M,
    CLK_OUT_SRC0_STD_48M,
    CLK_OUT_SRC0_PLL_96M,
    CLK_OUT_SRC0_BTOSC_24M,
    CLK_OUT_SRC0_BTOSC_48M,
    CLK_OUT_SRC0_FLL_24M,
    CLK_OUT_SRC0_XOSC_32K,
    CLK_OUT_SRC0_DPLL0_D1P0,
    CLK_OUT_SRC0_DPLL1_D1P0,
    CLK_OUT_SRC0_M2P_PLL0_CLK,
    CLK_OUT_SRC0_M2P_PLL1_CLK,
    CLK_OUT_SRC0_CLK_X2,
    CLK_OUT_SRC0_P33_DEBUG_CLK,
    CLK_OUT_SRC0_DVAD_COP0,
    CLK_OUT_SRC0_DVAD_CON0,
};

enum CLK_OUT_SOURCE1 {
    CLK_OUT_SRC1_NULL = 0,
    CLK_OUT_SRC1_RC_16M,
    CLK_OUT_SRC1_RCCLK,
    CLK_OUT_SRC1_STRC_32K,
    CLK_OUT_SRC1_SRC_CLK,
    CLK_OUT_SRC1_SYS_CLK,
    CLK_OUT_SRC1_SLEEP_R,
    CLK_OUT_SRC1_STD_12M,
    CLK_OUT_SRC1_STD_24M,
    CLK_OUT_SRC1_STD_48M,
    CLK_OUT_SRC1_PLL_96M,
    CLK_OUT_SRC1_BTOSC_24M,
    CLK_OUT_SRC1_BTOSC_48M,
    CLK_OUT_SRC1_FLL_24M,
    CLK_OUT_SRC1_XOSC_32K,
    CLK_OUT_SRC1_DPLL0_D1P0,
    CLK_OUT_SRC1_DPLL1_D1P0,
    CLK_OUT_SRC1_M2P_PLL0_CLK,
    CLK_OUT_SRC1_M2P_PLL1_CLK,
    CLK_OUT_SRC1_CLK_X2,
    CLK_OUT_SRC1_P33_DEBUG_CLK,
    CLK_OUT_SRC1_DVAD_COP0,
    CLK_OUT_SRC1_DVAD_CON0,
};

void clk_out0(u8 gpio, enum CLK_OUT_SOURCE0 clk);
void clk_out1(u8 gpio, enum CLK_OUT_SOURCE1 clk, u8 div);
void clk_out0_close(u8 gpio);
void clk_out1_close(u8 gpio);

void clk_ndelay(u32 nops);

void clk_udelay(u32 us);

void clk_mdelay(u32 ms);

u32 clk_set_api(const char *name, u32 freq);
u32 clk_get_api(const char *name);

void clock_reduce_system_frequency(void);
void clock_raise_system_frequency(void);

void clock_early_init(enum pll_ref_source pll_ref, u32 ref_frequency, u32 pll_frequency);

void clock_dump(void);


u32 lrc_get_avg_freq(void);


#endif
