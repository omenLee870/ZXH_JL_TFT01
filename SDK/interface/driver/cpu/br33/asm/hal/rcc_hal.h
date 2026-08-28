
#ifndef __RCC_HAL_H__
#define __RCC_HAL_H__

typedef struct {
    __RW __u32 RST_SRC;
} JL_LSBRST_TypeDef;

typedef struct {
    __RW __u32 PWR_CON;

    __RW __u32 ANA_CON0;
    __RW __u32 SYS_CON0;
    __RW __u32 SYS_CON1;
    __RW __u32 LSB_SEL;
    __RW __u32 PCLK_SEL;
    __RW __u32 STD_CON0;

    __RW __u32 CLK_OUT0;
    __RW __u32 CLK_OUT1;
    __RW __u32 CLK_OUT2;
    __RW __u32 CLK_OUT3;
    __RW __u32 reserved[4];

    __RW __u32 PRP_CON0;
    __RW __u32 PRP_CON1;
    __RW __u32 PRP_CON2;
    __RW __u32 PRP_CON3;

// #ifdef BR33_FPGA_CODE_VERIFY
//     __RW __u32 PRP_TCON;
//     __RW __u32 STD_CON1;
//     __RW __u32 STD_CON2;
// #endif

} JL_LSBCLK_TypeDef;

typedef struct {
    __RW __u32 RST_SRC;
} JL_HSBRST_TypeDef;

typedef struct {
    __RW __u32 PWR_CON;
    __RW __u32 SYS_CON0;
    __RW __u32 HSB_DIV;
    __RW __u32 HSB_SEL;
    __RW __u32 PUCLK_ST;
} JL_HSBCLK_TypeDef;

#endif


