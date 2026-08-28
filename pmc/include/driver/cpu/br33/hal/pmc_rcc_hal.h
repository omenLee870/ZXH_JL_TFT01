
#ifndef __PMC_RCC_HAL_H__
#define __PMC_RCC_HAL_H__

typedef struct {
    __RO __u32 RST_SRC              ;     // 5'd0
    __RW __u32 PWR_CON              ;     // 5'd1
    __RW __u32 ANA_CON0             ;
    __RW __u32 SRC_CKSEL            ;
    __RW __u32 SRC_CKDIV            ;
    __RW __u32 SRC_CON0             ;
    __RW __u32 SYS_CON0             ;
    __RW __u32 STD_CON0             ;

    __RW __u32 WKUP_EN              ;
    __RO __u32 WKUP_SRC             ;
    __RW __u32 WKUP_CON             ;
    __RW __u32 WKUP_reserved[5]     ;

    __RW __u32 PRP_CON0             ;
    __RW __u32 WL_CON0              ;
    __RW __u32 MSYS_CON             ;
    __RW __u32 PRP_reserved[5]      ;

    __RW __u32 XOSC_CON0            ;
    __RW __u32 XOSC_CON1            ;
    __RW __u32 XOSC_CON2            ;
    __RW __u32 XOSC_CON3            ;
    __RW __u32 XOSC_CON4            ;
    __RW __u32 XOSC_CON5            ;
    __RW __u32 XOSC_reserved[2]     ;

    __RW __u32 DPLL0_CON            ;
    __RW __u32 DPLL0_PREDIV0        ;
    __RW __u32 DPLL0_POSTDIV0       ;
    __RW __u32 DPLL0_PREDIV1        ;
    __RW __u32 DPLL0_POSTDIV1       ;
    __RW __u32 DPLL0_reserved[3]    ;

    __RW __u32 DPLL1_CON            ;
    __RW __u32 DPLL1_PREDIV0        ;
    __RW __u32 DPLL1_POSTDIV0       ;
    __RW __u32 DPLL1_PREDIV1        ;
    __RW __u32 DPLL1_POSTDIV1       ;
    __RW __u32 DPLL1_reserved[3]    ;

    __RW __u32 CLK_OUT0             ;
    __RW __u32 CLK_OUT1             ;
    __RW __u32 CLK_OUT2             ;
    __RW __u32 CLK_OUT3             ;
    __RW __u32 CLKOUT_reserved[4]   ;
} PMC_CLOCK_TypeDef;



#endif

