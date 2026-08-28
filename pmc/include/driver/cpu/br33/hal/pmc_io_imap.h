
//===============================================================================//
//
//      input IO define
//
//===============================================================================//
#define PMC_PD0_IN  1
#define PMC_PD1_IN  2
#define PMC_PD2_IN  3
#define PMC_PD3_IN  4
#define PMC_PD4_IN  5
#define PMC_PD5_IN  6
#define PMC_PD6_IN  7
#define PMC_PD7_IN  8
#define PMC_PD8_IN  9
#define PMC_PD9_IN  10
#define PMC_PD10_IN  11
#define PMC_PD11_IN  12
#define PMC_PD12_IN  13
#define PMC_PE0_IN  14
#define PMC_PE1_IN  15
#define PMC_PE2_IN  16
#define PMC_PE3_IN  17
#define PMC_PE4_IN  18
#define PMC_PH0_IN  19
#define PMC_PH1_IN  20
#define PMC_PH2_IN  21
#define PMC_PH3_IN  22
#define PMC_PH4_IN  23
#define PMC_PH5_IN  24
#define PMC_PH6_IN  25
#define PMC_PH7_IN  26
#define PMC_PH8_IN  27
#define PMC_PP0_IN  28
#define PMC_PP1_IN  29
#define PMC_PP2_IN  30

//===============================================================================//
//
//      function input select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 PMC_FI_GP_ICH0;
    __RW __u8 PMC_FI_GP_ICH1;
    __RW __u8 PMC_FI_GP_ICH2;
    __RW __u8 PMC_FI_GP_ICH3;
    __RW __u8 PMC_FI_GP_ICH4;
    __RW __u8 PMC_FI_GP_ICH5;
    __RW __u8 PMC_FI_UART0_RX;
    __RW __u8 PMC_FI_UART1_RX;
    __RW __u8 PMC_FI_SPI0_CLK;
    __RW __u8 PMC_FI_SPI0_DA0;
    __RW __u8 PMC_FI_SPI0_DA1;
    __RW __u8 PMC_FI_SPI0_DA2;
    __RW __u8 PMC_FI_SPI0_DA3;
    __RW __u8 PMC_FI_IIC0_SCL;
    __RW __u8 PMC_FI_IIC0_SDA;
    __RW __u8 PMC_FI_IIC1_SCL;
    __RW __u8 PMC_FI_IIC1_SDA;
    __RW __u8 PMC_FI_DMIC_DAT;
} PMC_IMAP_TypeDef;

#define PMC_IMAP_BASE      (pmc_sfr_base + map_adr(0x1a, 0x00))
#define PMC_IMAP           ((PMC_IMAP_TypeDef   *)PMC_IMAP_BASE)


