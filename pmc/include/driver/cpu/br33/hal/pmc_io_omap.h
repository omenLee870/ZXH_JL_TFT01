
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define PMC_FO_GP_OCH0        ((0 << 2)|BIT(1))
#define PMC_FO_GP_OCH1        ((1 << 2)|BIT(1))
#define PMC_FO_GP_OCH2        ((2 << 2)|BIT(1))
#define PMC_FO_GP_OCH3        ((3 << 2)|BIT(1))
#define PMC_FO_GP_OCH4        ((4 << 2)|BIT(1))
#define PMC_FO_GP_OCH5        ((5 << 2)|BIT(1))
#define PMC_FO_GP_OCH6        ((6 << 2)|BIT(1))
#define PMC_FO_GP_OCH7        ((7 << 2)|BIT(1))
#define PMC_FO_UART0_TX        ((8 << 2)|BIT(1)|BIT(0))
#define PMC_FO_UART1_TX        ((9 << 2)|BIT(1)|BIT(0))
#define PMC_FO_SPI0_CLK        ((10 << 2)|BIT(1)|BIT(0))
#define PMC_FO_SPI0_DA0        ((11 << 2)|BIT(1)|BIT(0))
#define PMC_FO_SPI0_DA1        ((12 << 2)|BIT(1)|BIT(0))
#define PMC_FO_SPI0_DA2        ((13 << 2)|BIT(1)|BIT(0))
#define PMC_FO_SPI0_DA3        ((14 << 2)|BIT(1)|BIT(0))
#define PMC_FO_IIC0_SCL        ((15 << 2)|BIT(1)|BIT(0))
#define PMC_FO_IIC0_SDA        ((16 << 2)|BIT(1)|BIT(0))
#define PMC_FO_IIC1_SCL        ((17 << 2)|BIT(1)|BIT(0))
#define PMC_FO_IIC1_SDA        ((18 << 2)|BIT(1)|BIT(0))
#define PMC_FO_DMIC_CLK        ((19 << 2)|BIT(1)|BIT(0))

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 PMC_PD0_OUT;
    __RW __u8 PMC_PD1_OUT;
    __RW __u8 PMC_PD2_OUT;
    __RW __u8 PMC_PD3_OUT;
    __RW __u8 PMC_PD4_OUT;
    __RW __u8 PMC_PD5_OUT;
    __RW __u8 PMC_PD6_OUT;
    __RW __u8 PMC_PD7_OUT;
    __RW __u8 PMC_PD8_OUT;
    __RW __u8 PMC_PD9_OUT;
    __RW __u8 PMC_PD10_OUT;
    __RW __u8 PMC_PD11_OUT;
    __RW __u8 PMC_PD12_OUT;
    __RW __u8 PMC_PE0_OUT;
    __RW __u8 PMC_PE1_OUT;
    __RW __u8 PMC_PE2_OUT;
    __RW __u8 PMC_PE3_OUT;
    __RW __u8 PMC_PE4_OUT;
    __RW __u8 PMC_PH0_OUT;
    __RW __u8 PMC_PH1_OUT;
    __RW __u8 PMC_PH2_OUT;
    __RW __u8 PMC_PH3_OUT;
    __RW __u8 PMC_PH4_OUT;
    __RW __u8 PMC_PH5_OUT;
    __RW __u8 PMC_PH6_OUT;
    __RW __u8 PMC_PH7_OUT;
    __RW __u8 PMC_PH8_OUT;
    __RW __u8 PMC_PP0_OUT;
    __RW __u8 PMC_PP1_OUT;
    __RW __u8 PMC_PP2_OUT;
} PMC_OMAP_TypeDef;

#define PMC_OMAP_BASE      (pmc_sfr_base + map_adr(0x19, 0x00))
#define PMC_OMAP           ((PMC_OMAP_TypeDef   *)PMC_OMAP_BASE)

#define GPIO_HAS_CROSSBAR(port)  _GPIO_HAS_CROSSBAR_##port

#define _GPIO_HAS_CROSSBAR_A 0
#define _GPIO_HAS_CROSSBAR_B 0
#define _GPIO_HAS_CROSSBAR_C 0
#define _GPIO_HAS_CROSSBAR_D 1
#define _GPIO_HAS_CROSSBAR_E 1
#define _GPIO_HAS_CROSSBAR_F 0
#define _GPIO_HAS_CROSSBAR_G 0
#define _GPIO_HAS_CROSSBAR_H 1
#define _GPIO_HAS_CROSSBAR_I 0
#define _GPIO_HAS_CROSSBAR_J 0
#define _GPIO_HAS_CROSSBAR_K 0

#define _GPIO_HAS_CROSSBAR_P 1
#define _GPIO_HAS_CROSSBAR_USB 0
#define _GPIO_HAS_CROSSBAR_R 0

