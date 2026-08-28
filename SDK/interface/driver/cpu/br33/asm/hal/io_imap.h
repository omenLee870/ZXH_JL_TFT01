
#ifndef __IO_IMAP_H__
#define __IO_IMAP_H__

//===============================================================================//
//
//      input IO define
//
//===============================================================================//
#define PA0_IN  1
#define PA1_IN  2
#define PA2_IN  3
#define PA3_IN  4
#define PA4_IN  5
#define PA5_IN  6
#define PA6_IN  7
#define PA7_IN  8
#define PA8_IN  9
#define PA9_IN  10
#define PA10_IN  11
#define PA11_IN  12
#define PA12_IN  13
#define PA13_IN  14
#define PB0_IN  15
#define PB1_IN  16
#define PB2_IN  17
#define PB3_IN  18
#define PB4_IN  19
#define PB5_IN  20
#define PB6_IN  21
#define PB7_IN  22
#define PB8_IN  23
#define PB9_IN  24
#define PB10_IN  25
#define PB11_IN  26
#define PB12_IN  27 // 没有该io
#define PC0_IN  28
#define PC1_IN  29
#define PC2_IN  30
#define PC3_IN  31
#define PC4_IN  32
#define PC5_IN  33
#define PC6_IN  34
#define PC7_IN  35
#define PC8_IN  36
#define PC9_IN  37
#define PC10_IN  38
#define PC11_IN  39
#define PC12_IN  40
#define PC13_IN  41
#define PC14_IN  42
#define PC15_IN  43
#define PD0_IN  44
#define PD1_IN  45
#define PD2_IN  46
#define PD3_IN  47
#define PD4_IN  48
#define PD5_IN  49
#define PD6_IN  50
#define PD7_IN  51
#define PD8_IN  52
#define PD9_IN  53
#define PD10_IN  54
#define PD11_IN  55
#define PD12_IN  56
#define PE0_IN  57
#define PE1_IN  58
#define PE2_IN  59
#define PE3_IN  60
#define PE4_IN  61
#define PG0_IN  62
#define PG1_IN  63
#define PG2_IN  64
#define PG3_IN  65
#define PH0_IN  66
#define PH1_IN  67
#define PH2_IN  68
#define PH3_IN  69
#define PH4_IN  70
#define PH5_IN  71
#define PH6_IN  72
#define PH7_IN  73
#define PH8_IN  74
#define USBDP_IN  75
#define USBDM_IN  76
#define PT0_IN  77
#define PT1_IN  78
#define PT2_IN  79
#define PT3_IN  80
#define PT4_IN  81
#define PXx_IN_MAX  82

//===============================================================================//
//
//      function input select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 FI_GP_ICH0;
    __RW __u8 FI_GP_ICH1;
    __RW __u8 FI_GP_ICH2;
    __RW __u8 FI_GP_ICH3;
    __RW __u8 FI_GP_ICH4;
    __RW __u8 FI_GP_ICH5;
    __RW __u8 FI_GP_ICH6;
    __RW __u8 FI_GP_ICH7;
    __RW __u8 FI_SPI0_CLK;
    __RW __u8 FI_SPI0_DA0;
    __RW __u8 FI_SPI0_DA1;
    __RW __u8 FI_SPI0_DA2;
    __RW __u8 FI_SPI0_DA3;
    __RW __u8 FI_SPI1_CLK;
    __RW __u8 FI_SPI1_DA0;
    __RW __u8 FI_SPI1_DA1;
    __RW __u8 FI_SPI1_DA2;
    __RW __u8 FI_SPI1_DA3;
    __RW __u8 FI_IIC0_SCL;
    __RW __u8 FI_IIC0_SDA;
    __RW __u8 FI_IIC1_SCL;
    __RW __u8 FI_IIC1_SDA;
    __RW __u8 FI_IIC2_SCL;
    __RW __u8 FI_IIC2_SDA;
    __RW __u8 FI_IIC3_SCL;
    __RW __u8 FI_IIC3_SDA;
    __RW __u8 FI_IIC4_SCL;
    __RW __u8 FI_IIC4_SDA;
    __RW __u8 FI_IIC5_SCL;
    __RW __u8 FI_IIC5_SDA;
    __RW __u8 FI_UART0_RX;
    __RW __u8 FI_UART1_RX;
    __RW __u8 FI_UART2_RX;
    __RW __u8 FI_UART3_RX;
    __RW __u8 FI_SD0_CMD;
    __RW __u8 FI_SD0_DA0;
    __RW __u8 FI_SD0_DA1;
    __RW __u8 FI_SD0_DA2;
    __RW __u8 FI_SD0_DA3;
    __RW __u8 FI_CAN_RX;
    __RW __u8 FI_QDEC0_SIN0;
    __RW __u8 FI_QDEC0_SIN1;
    __RW __u8 FI_QDEC1_SIN0;
    __RW __u8 FI_QDEC1_SIN1;
    __RW __u8 FI_QDEC2_SIN0;
    __RW __u8 FI_QDEC2_SIN1;
    __RW __u8 FI_NFC_RX;
    __RW __u8 FI_ALNK0_MCLK;
    __RW __u8 FI_ALNK0_LRCK;
    __RW __u8 FI_ALNK0_SCLK;
    __RW __u8 FI_ALNK0_DAT0;
    __RW __u8 FI_ALNK0_DAT1;
    __RW __u8 FI_ALNK0_DAT2;
    __RW __u8 FI_ALNK0_DAT3;
    __RW __u8 FI_ALNK1_MCLK;
    __RW __u8 FI_ALNK1_LRCK;
    __RW __u8 FI_ALNK1_SCLK;
    __RW __u8 FI_ALNK1_DAT0;
    __RW __u8 FI_ALNK1_DAT1;
    __RW __u8 FI_ALNK1_DAT2;
    __RW __u8 FI_ALNK1_DAT3;
    __RW __u8 FI_DBI_TE;
    __RW __u8 FI_TOTAL;
} JL_IMAP_TypeDef;

#define JL_IMAP_BASE      (ls_base + map_adr(0x3a, 0x00))
#define JL_IMAP           ((JL_IMAP_TypeDef   *)JL_IMAP_BASE)
#endif


