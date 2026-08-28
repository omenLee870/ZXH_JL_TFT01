#ifndef  __CLOCK_HAL_H__
#define  __CLOCK_HAL_H__

#include "typedef.h"
enum CLK_OUT_SOURCE0 {
    CLK_OUT_SRC0_NULL = 0,
    CLK_OUT_SRC0_RC_16M,
    CLK_OUT_SRC0_RC_250K,
    CLK_OUT_SRC0_STRC_32K,
    CLK_OUT_SRC0_HSB_CLK,
    CLK_OUT_SRC0_LSB_CLK,
    CLK_OUT_SRC0_STD_12M,
    CLK_OUT_SRC0_STD_24M,
    CLK_OUT_SRC0_STD_48M,
    CLK_OUT_SRC0_PLL_96M,
    CLK_OUT_SRC0_BTOSC_24M,
    CLK_OUT_SRC0_BTOSC_48M,
    CLK_OUT_SRC0_SYS_PLL_D1P0,
    CLK_OUT_SRC0_SYS_PLL_D1P5,
    CLK_OUT_SRC0_SYS_PLL_D2P0,
    CLK_OUT_SRC0_SYS_PLL_D2P5,
    CLK_OUT_SRC0_SYS_PLL_D3P5,
    CLK_OUT_SRC0_WAT_CLK,
    CLK_OUT_SRC0_RING_CLK,
    CLK_OUT_SRC0_PMC_DEBUG_CLK,
    CLK_OUT_SRC0_P33_DEBUG_CLK,
    CLK_OUT_SRC0_USB_CLK,
    CLK_OUT_SRC0_FLL_24M,
    CLK_OUT_SRC0_RF_CKO_CLK,
    CLK_OUT_SRC0_PLL_DSMCK,
    CLK_OUT_SRC0_PLL_SYNCK,
};

enum CLK_OUT_SOURCE1 {
    CLK_OUT_SRC1_NULL = 0,
    CLK_OUT_SRC1_RC_16M,
    CLK_OUT_SRC1_RC_250K,
    CLK_OUT_SRC1_STRC_32K,
    CLK_OUT_SRC1_HSB_CLK,
    CLK_OUT_SRC1_LSB_CLK,
    CLK_OUT_SRC1_STD_12M,
    CLK_OUT_SRC1_STD_24M,
    CLK_OUT_SRC1_STD_48M,
    CLK_OUT_SRC1_PLL_96M,
    CLK_OUT_SRC1_BTOSC_24M,
    CLK_OUT_SRC1_BTOSC_48M,
    CLK_OUT_SRC1_SYS_PLL_D1P0,
    CLK_OUT_SRC1_SYS_PLL_D1P5,
    CLK_OUT_SRC1_SYS_PLL_D2P0,
    CLK_OUT_SRC1_SYS_PLL_D2P5,
    CLK_OUT_SRC1_SYS_PLL_D3P5,
    CLK_OUT_SRC1_WAT_CLK,
    CLK_OUT_SRC1_RING_CLK,
    CLK_OUT_SRC1_PMC_DEBUG_CLK,
    CLK_OUT_SRC1_P33_DEBUG_CLK,
    CLK_OUT_SRC1_USB_CLK,
    CLK_OUT_SRC1_FLL_24M,
    CLK_OUT_SRC1_RF_CKO_CLK,
    CLK_OUT_SRC1_PLL_DSMCK,
    CLK_OUT_SRC1_PLL_SYNCK,
};

void clk_out0(u8 gpio, enum CLK_OUT_SOURCE0 clk);
void clk_out2(u8 gpio, enum CLK_OUT_SOURCE1 clk, u8 div);
//无 clk_out1

void clk_out0_close(u8 gpio);
void clk_out2_close(u8 gpio);



enum CLK_OUT_SOURCE {
    CLK_OUT0_NULL = 0x100,
    CLK_OUT0_RC_16M,
    CLK_OUT0_RC_250K,
    CLK_OUT0_STRC_32K,
    CLK_OUT0_HSB_CLK,
    CLK_OUT0_LSB_CLK,
    CLK_OUT0_STD_12M,
    CLK_OUT0_STD_24M,
    CLK_OUT0_STD_48M,
    CLK_OUT0_PLL_96M,
    CLK_OUT0_BTOSC_24M,
    CLK_OUT0_BTOSC_48M,
    CLK_OUT0_SYS_PLL_D1P0,
    CLK_OUT0_SYS_PLL_D1P5,
    CLK_OUT0_SYS_PLL_D2P0,
    CLK_OUT0_SYS_PLL_D2P5,
    CLK_OUT0_SYS_PLL_D3P5,
    CLK_OUT0_WAT_CLK,
    CLK_OUT0_RING_CLK,
    CLK_OUT0_PMC_DEBUG_CLK,
    CLK_OUT0_P33_DEBUG_CLK,
    CLK_OUT0_USB_CLK,
    CLK_OUT0_FLL_24M,
    CLK_OUT0_RF_CKO_CLK,
    CLK_OUT0_PLL_DSMCK,
    CLK_OUT0_PLL_SYNCK,

    CLK_OUT1_NULL = 0x200,
    CLK_OUT1_RC_16M,
    CLK_OUT1_RC_250K,
    CLK_OUT1_STRC_32K,
    CLK_OUT1_HSB_CLK,
    CLK_OUT1_LSB_CLK,
    CLK_OUT1_STD_12M,
    CLK_OUT1_STD_24M,
    CLK_OUT1_STD_48M,
    CLK_OUT1_PLL_96M,
    CLK_OUT1_BTOSC_24M,
    CLK_OUT1_BTOSC_48M,
    CLK_OUT1_SYS_PLL_D1P0,
    CLK_OUT1_SYS_PLL_D1P5,
    CLK_OUT1_SYS_PLL_D2P0,
    CLK_OUT1_SYS_PLL_D2P5,
    CLK_OUT1_SYS_PLL_D3P5,
    CLK_OUT1_WAT_CLK,
    CLK_OUT1_RING_CLK,
    CLK_OUT1_PMC_DEBUG_CLK,
    CLK_OUT1_P33_DEBUG_CLK,
    CLK_OUT1_USB_CLK,
    CLK_OUT1_FLL_24M,
    CLK_OUT1_RF_CKO_CLK,
    CLK_OUT1_PLL_DSMCK,
    CLK_OUT1_PLL_SYNCK,
};

#define CLK_OUT_CH_MASK             0b111
#define CLK_OUT_CH0_SEL(clk)        SFR(JL_LSBCLK->CLK_OUT0,0,5,clk)
#define CLK_OUT_CH0_GET_CLK()       ((JL_LSBCLK->CLK_OUT0>>0)&0x01f)
#define CLK_OUT_CH0_DIV(div)        SFR(JL_LSBCLK->CLK_OUT0,10,9,div)
#define CLK_OUT_CH0_EN(en)          SFR(JL_LSBCLK->CLK_OUT0,9,1,en)
#define CLK_OUT0_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT0_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH1_SEL(clk)        SFR(JL_LSBCLK->CLK_OUT1,0,5,clk)
#define CLK_OUT_CH1_GET_CLK()       ((JL_LSBCLK->CLK_OUT1>>0)&0x01f)
#define CLK_OUT_CH1_DIV(div)        SFR(JL_LSBCLK->CLK_OUT1,10,9,div)
#define CLK_OUT_CH1_EN(en)          SFR(JL_LSBCLK->CLK_OUT1,9,1,en)
#define CLK_OUT1_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT1_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH2_SEL(clk)
#define CLK_OUT_CH2_GET_CLK()       (0)
#define CLK_OUT_CH2_DIV(div)
#define CLK_OUT_CH2_EN(en)         //no en
#define CLK_OUT2_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT2_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH3_SEL(clk)        //no ch3
#define CLK_OUT_CH3_GET_CLK()       (0)//no ch3
#define CLK_OUT_CH3_DIV(div)        //no div
#define CLK_OUT_CH3_EN(en)         //no en
#define CLK_OUT3_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT3_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH4_SEL(clk)        //no ch4
#define CLK_OUT_CH4_GET_CLK()       (0)//no ch4
#define CLK_OUT_CH4_DIV(div)        //no div
#define CLK_OUT_CH4_EN(en)         //no en
#define CLK_OUT4_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT4_FIXED_IO()     (0)//no fix
u32 clk_out_fixed_io_check(u32 gpio);

//for dvdd
#define CLOCK_DVDD_POWER_SELF		0 //自主供电
#define CLOCK_DVDD_POWER_EXT		1 //外接电源
#define CLOCK_DVDD_POWER_SHORT		2 //短接DCVDD12

//for bt
void clk_set_osc_cap(u8 sel_l, u8 sel_r);
u32 clk_get_osc_cap();
u32 get_btosc_info_for_update(void *info);
void xosc_common_init(void(*udly)(u32));
void xosc_set_sys_cfg(u32 sys_hcs, u32 sys_cls, u32 sys_crs);
void xosc_get_sys_cfg(u32 *sys_hcs, u32 *sys_cls, u32 *sys_crs);
void xosc_set_cap_fine(u32 clsf, u32 crsf);
void xosc_get_cap_fine(u32 *clsf, u32 *crsf);
void xosc_set_cap(u32 sel_l, u32 sel_r);
void xosc_get_cap(u32 *sel_l, u32 *sel_r);
void xosc_hcs_trim_disable(void);
void xosc_close(void);


void xosc32k_change_cfg(u32 is_level, u32 hd_level);
void xosc32k_hw_init(u32 is_level, u32 hd_level);
u32 xosc32k_is_working(void);
int xosc32k_init(void);
void xosc32k_close(void);


#define BT_CLOCK_IN(x)          //SFR(JL_CLOCK->CLK_CON1,  14,  2,  x)
//for MACRO - BT_CLOCK_IN
enum {
    BT_CLOCK_IN_PLL48M = 0,
    BT_CLOCK_IN_HSB,
    BT_CLOCK_IN_LSB,
    BT_CLOCK_IN_DISABLE,
};


#endif  /*CLOCK_HAL_H*/
