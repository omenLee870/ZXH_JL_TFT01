#ifndef  __BOOT_XOSC_H__
#define  __BOOT_XOSC_H__

#include "config.h"


#define RCH_CLK     (16000000) //

#define XOSC_LDO    0x8
#define LRC24M_CAPS  0b011
#define LRC24M_IS    0b1
#define LRC24M_RS    0b0111

#define wla_con8_init_doublepin \
/*XOSC_EN_11v_1bit           */   ((0&0x1)<<0)   | \
/*XMDET_EN_1bit              */   ((0&0x1)<<1)   | \
/*XMDET_S_2bit               */   ((0&0x3)<<2)   | \
/*XOSC_AAC_EN_1bit           */   ((0&0x1)<<4)   | \
/*XOSC_AAC_S_2bit            */   ((2&0x3)<<5)   | \
/*XOSC_BIAS_EN_1bit          */   ((1&0x1)<<7)   | \
/*XOSC_BT_OE_1bit            */   ((0&0x1)<<8)   | \
/*XOSC_CK1X_OE_1bit          */   ((0&0x1)<<9)   | \
/*XOSC_CK2X_OE_1bit          */   ((0&0x1)<<10)  | \
/*XOSC_CK2X_S_3bit           */   ((4&0x7)<<11)  | \
/*XOSC_CKAIN_EN_1bit         */   ((0&0x1)<<14)  | \
/*XOSC_CKAIN_S_2bit          */   ((2&0x3)<<15)  | \
/*XOSC_CLS_S_5bit            */  ((10&0x1f)<<17) | \
/*XOSC_CRS_S_5bit            */  ((10&0x1f)<<22) | \
/*XOSC_EXT_EN_1bit           */   ((0&0x1)<<27)  | \
/*XOSC_GMBST_EN_1bit         */   ((1&0x1)<<28)  | \
/*XOSC_HCS_3bit              */   ((6&0x7)<<29)

#define wla_con9_init_doublepin \
/*XOSC_LDO_BYPASS_1bit       */   ((0&0x1)<<0)   | \
/*XOSC_LDO_S_4bit            */   ((XOSC_LDO&0xf)<<1)   | \
/*XOSC_PMU_OE_1bit           */   ((0&0x1)<<5)   | \
/*XOSC_RESV_1bit             */   ((0&0x1)<<6)   | \
/*XOSC_SPIN_EN_1bit          */   ((0&0x1)<<7)   | \
/*XOSC_SPIN_S_2bit           */   ((0&0x3)<<8)   | \
/*XOSC_SYNEN_1bit            */   ((0&0x1)<<10)  | \
/*XOSC_SYS_OE_1bit           */   ((0&0x1)<<11)  | \
/*XOSC_TEST_OE_1bit          */   ((0&0x1)<<12)  | \
/*XOSC_TEST_S_2bit           */   ((0&0x3)<<13)

#define wla_con8_init_singlepin \
/*XOSC_EN_11v_1bit           */   ((0&0x1)<<0)   | \
/*XMDET_EN_1bit              */   ((0&0x1)<<1)   | \
/*XMDET_S_2bit               */   ((0&0x3)<<2)   | \
/*XOSC_AAC_EN_1bit           */   ((0&0x1)<<4)   | \
/*XOSC_AAC_S_2bit            */   ((2&0x3)<<5)   | \
/*XOSC_BIAS_EN_1bit          */   ((1&0x1)<<7)   | \
/*XOSC_BT_OE_1bit            */   ((0&0x1)<<8)   | \
/*XOSC_CK1X_OE_1bit          */   ((0&0x1)<<9)   | \
/*XOSC_CK2X_OE_1bit          */   ((0&0x1)<<10)  | \
/*XOSC_CK2X_S_3bit           */   ((4&0x7)<<11)  | \
/*XOSC_CKAIN_EN_1bit         */   ((0&0x1)<<14)  | \
/*XOSC_CKAIN_S_2bit          */   ((2&0x3)<<15)  | \
/*XOSC_CLS_S_5bit            */   ((0x1a&0x1f)<<17) | \
/*XOSC_CRS_S_5bit            */   ((0xc&0x1f)<<22) | \
/*XOSC_EXT_EN_1bit           */   ((0&0x1)<<27)  | \
/*XOSC_GMBST_EN_1bit         */   ((1&0x1)<<28)  | \
/*XOSC_HCS_3bit              */   ((6&0x7)<<29)

#define wla_con9_init_singlepin \
/*XOSC_LDO_BYPASS_1bit       */   ((0&0x1)<<0)   | \
/*XOSC_LDO_S_4bit            */   ((XOSC_LDO&0xf)<<1)   | \
/*XOSC_PMU_OE_1bit           */   ((0&0x1)<<5)   | \
/*XOSC_RESV_1bit             */   ((0&0x1)<<6)   | \
/*XOSC_SPIN_EN_1bit          */   ((1&0x1)<<7)   | \
/*XOSC_SPIN_S_2bit           */   ((1&0x3)<<8)   | \
/*XOSC_SYNEN_1bit            */   ((0&0x1)<<10)  | \
/*XOSC_SYS_OE_1bit           */   ((0&0x1)<<11)  | \
/*XOSC_TEST_OE_1bit          */   ((0&0x1)<<12)  | \
/*XOSC_TEST_S_2bit           */   ((0&0x3)<<13)

#define wla_con8_init_detect \
/*XOSC_EN_11v_1bit           */   ((0&0x1)<<0)   | \
/*XMDET_EN_1bit              */   ((0&0x1)<<1)   | \
/*XMDET_S_2bit               */   ((0&0x3)<<2)   | \
/*XOSC_AAC_EN_1bit           */   ((0&0x1)<<4)   | \
/*XOSC_AAC_S_2bit            */   ((0&0x3)<<5)   | \
/*XOSC_BIAS_EN_1bit          */   ((0&0x1)<<7)   | \
/*XOSC_BT_OE_1bit            */   ((0&0x1)<<8)   | \
/*XOSC_CK1X_OE_1bit          */   ((0&0x1)<<9)   | \
/*XOSC_CK2X_OE_1bit          */   ((0&0x1)<<10)  | \
/*XOSC_CK2X_S_3bit           */   ((0&0x7)<<11)  | \
/*XOSC_CKAIN_EN_1bit         */   ((1&0x1)<<14)  | \
/*XOSC_CKAIN_S_2bit          */   ((3&0x3)<<15)  | \
/*XOSC_CLS_S_5bit            */   ((0&0x1f)<<17) | \
/*XOSC_CRS_S_5bit            */   ((0&0x1f)<<22) | \
/*XOSC_EXT_EN_1bit           */   ((0&0x1)<<27)  | \
/*XOSC_GMBST_EN_1bit         */   ((0&0x1)<<28)  | \
/*XOSC_HCS_3bit              */   ((0&0x7)<<29)

#define wla_con9_init_detect \
/*XOSC_LDO_BYPASS_1bit       */   ((1&0x1)<<0)   | \
/*XOSC_LDO_S_4bit            */   ((0&0xf)<<1)   | \
/*XOSC_PMU_OE_1bit           */   ((0&0x1)<<5)   | \
/*XOSC_RESV_1bit             */   ((0&0x1)<<6)   | \
/*XOSC_SPIN_EN_1bit          */   ((0&0x1)<<7)   | \
/*XOSC_SPIN_S_2bit           */   ((0&0x3)<<8)   | \
/*XOSC_SYNEN_1bit            */   ((0&0x1)<<10)  | \
/*XOSC_SYS_OE_1bit           */   ((0&0x1)<<11)  | \
/*XOSC_TEST_OE_1bit          */   ((0&0x1)<<12)  | \
/*XOSC_TEST_S_2bit           */   ((0&0x3)<<13)

#define p11_clock_lrc24mcfg0 \
  /*LRC24M_EN_11v            */ ((0&0x1)<<0 )| \
  /*LRC24M_CKOE_L_11v        */ ((0&0x1)<<1 )| \
  /*LRC24M_CKOE_H_11v        */ ((0&0x1)<<2 )| \
  /*LRC24M_CAPS0_2_11v       */ ((lrc24m_caps&0x7)<<4 )| \
  /*LRC24M_IS0_1_11v         */ ((lrc24m_is&0x3)<<8 )| \
  /*LRC24M_RS0_3_11v         */ ((lrc24m_rs&0xf)<<12)| \
  /*LRC24M_LFSREN_11v        */ ((0&0x1)<<16)| \
  /*LRC24M_LFSR_CKSEL_11v    */ ((0&0x1)<<17)| \
  /*LRC24M_LFSR_RS_11v       */ ((0&0x1)<<18)| \
  /*LRC24M_TEST_EN_11v       */ ((0&0x1)<<20)| \
  /*LRC24M_TEST_S0_11v       */ ((0&0x1)<<21)

#define LRC24M_CAPS0_2_11v(x)     SFR(PMC_CLOCK->LRC24M_CON0,4,3,(x))
#define LRC24M_IS0_1_11v(x)       SFR(PMC_CLOCK->LRC24M_CON0,8,2,(x))
#define LRC24M_RS0_3_11v(x)       SFR(PMC_CLOCK->LRC24M_CON0,12,4,(x))
#define LRC24M_EN_11v(x)          SFR(PMC_CLOCK->LRC24M_CON0,0,1,(x))
#define LRC24M_CKOE_L_11v(x)      SFR(PMC_CLOCK->LRC24M_CON0,1,1,(x))
#define LRC24M_LFSREN(x)          SFR(PMC_CLOCK->LRC24M_CON0,16,1,(x))



#if COMPILE_CPU
#define XOSC_SFR_CONA JL_WLA->WLA_CON8
#define XOSC_SFR_CONB JL_WLA->WLA_CON9
#define XOSC_SFR_CONC JL_WLA->WLA_CON23

#define W_btosc_24m_cken(x)                      SFR(XOSC_SFR_CONC,0,1,(x))
#define W_btosc_48m_cken(x)                      SFR(XOSC_SFR_CONC,1,1,(x))
#else
#define XOSC_SFR_CONA PMC_CLOCK->XOSC_CON0
#define XOSC_SFR_CONB PMC_CLOCK->XOSC_CON1
#define XOSC_SFR_CONC PMC_CLOCK->CLKCFG_CON0

#define W_btosc_24m_cken(x)                      SFR(XOSC_SFR_CONC,1,1,(x))
#define W_btosc_48m_cken(x)                      SFR(XOSC_SFR_CONC,2,1,(x))
#endif

#define W_XMDET_EN_1(x)                          SFR(XOSC_SFR_CONA,1,1,(x))
#define W_XMDET_S_2(x)                           SFR(XOSC_SFR_CONA,2,2,(x))
#define W_XOSC_AAC_EN_1(x)                       SFR(XOSC_SFR_CONA,4,1,(x))
#define W_XOSC_AAC_S_2(x)                        SFR(XOSC_SFR_CONA,5,2,(x))
#define W_XOSC_BIAS_EN_1(x)                      SFR(XOSC_SFR_CONA,7,1,(x))
#define W_XOSC_BT_OE_1(x)                        SFR(XOSC_SFR_CONA,8,1,(x))
#define W_XOSC_CK1X_OE_1(x)                      SFR(XOSC_SFR_CONA,9,1,(x))
#define W_XOSC_CK2X_OE_1(x)                      SFR(XOSC_SFR_CONA,10,1,(x))
#define W_XOSC_CK2X_S_3(x)                       SFR(XOSC_SFR_CONA,11,3,(x))
#define W_XOSC_CKAIN_EN_1(x)                     SFR(XOSC_SFR_CONA,14,1,(x))
#define W_XOSC_CKAIN_S_2(x)                      SFR(XOSC_SFR_CONA,15,2,(x))
#define W_XOSC_CLS_S_5(x)                        SFR(XOSC_SFR_CONA,17,5,(x))
#define W_XOSC_CRS_S_5(x)                        SFR(XOSC_SFR_CONA,22,5,(x))
#define W_XOSC_EN_1(x)                           SFR(XOSC_SFR_CONA,0,1,(x))
#define W_XOSC_EXT_EN_1(x)                       SFR(XOSC_SFR_CONA,27,1,(x))
#define W_XOSC_GMBST_EN_1(x)                     SFR(XOSC_SFR_CONA,28,1,(x))
#define W_XOSC_HCS_3(x)                          SFR(XOSC_SFR_CONA,29,3,(x))
#define W_XOSC_LDO_BYPASS_1(x)                   SFR(XOSC_SFR_CONB,0,1,(x))
#define W_XOSC_LDO_S_4(x)                        SFR(XOSC_SFR_CONB,1,4,(x))
#define W_XOSC_PMU_OE_1(x)                       SFR(XOSC_SFR_CONB,5,1,(x))
#define W_XOSC_RESV_1(x)                         SFR(XOSC_SFR_CONB,6,1,(x))
#define W_XOSC_SPIN_EN_1(x)                      SFR(XOSC_SFR_CONB,7,1,(x))
#define W_XOSC_SPIN_S_2(x)                       SFR(XOSC_SFR_CONB,8,2,(x))
#define W_XOSC_SYNEN_1(x)                        SFR(XOSC_SFR_CONB,10,1,(x))
#define W_XOSC_SYS_OE_1(x)                       SFR(XOSC_SFR_CONB,11,1,(x))
#define W_XOSC_TEST_OE_1(x)                      SFR(XOSC_SFR_CONB,12,1,(x))
#define W_XOSC_TEST_S_2(x)                       SFR(XOSC_SFR_CONB,13,2,(x))

#define XOSC_CLS40_CRS40(x,y)    SFR(XOSC_SFR_CONA, 17, 10,(x|(y<<5)))


#define BTXOSC_INIT_FLAG_SUCCESS 1
#define BTXOSC_INIT_FLAG_NOTSURE 0

#define BTOSC_FREQ          24000000

#define PRECISION 1

#define GPC_MUL 7   //precision 0.5%  32*2^8=8192 1/8192=0.012%

#define GPCNT_NUM 1
#define GPCNT_INT 16

#define debug(x) JL_TIMER3->CNT=x

typedef enum {
    GPCNT_RC16M  =  3,
    GPCNT_LRC24M =  7,
    GPCNT_BTOSC  =  8,
    GPCNT_STD24M =  15,
} GPCNT_typedef;

typedef enum {
    P11_GPCNT_RC16M  =  1,
    P11_GPCNT_BTOSC  =  4,
    P11_GPCNT_LRC24M =  5,
    P11_GPCNT_STD24M =  8,
} GPCNT_typedef_P11;


void ic_btosc_init(void);
void lrc24m_init(void(*udly)(u32), u8 lrc24m_caps, u8 lrc24m_rs, u8 lrc24m_is);
void btosc_ctl_src(void);
void btosc_init_normal(void (*udly)(u32), u8 xosc_pin_mode, u8 xosc_ldo);
void btosc_init_ext_clk(void (*udly)(u32), void(*udly_margin)(u32), u8 xosc_pin_mode, u8 xosc_ldo);
unsigned int gpcnt_clk_cnt(u64 mul, GPCNT_typedef css_clk, GPCNT_typedef gss_clk);
unsigned char btxosc_init_check(void(*udly)(u32), u32 GSS_FREQ, u32 check_time_ms);

void open_xosc(void);


#endif  /*BOOT_XOSC_H*/
