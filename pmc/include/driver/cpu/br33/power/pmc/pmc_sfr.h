
#ifndef __PMC__
#define __PMC__

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

#ifdef PMU_SYSTEM
#define pmc_ram_base      0x7e40000
#define pmc_sfr_base      0x7e34000
#else
#define pmc_ram_base      0x7e40000
#define pmc_sfr_base      0x7e34000
#endif

#define __RW               volatile                 // read write
#define __RO               volatile const           // only read
#define __WO               volatile                 // only write

#define __u8               unsigned int             // u8  to u32 special for struct
#define __u16              unsigned int             // u16 to u32 special for struct
#define __u32              unsigned int

#define __s8(x)            char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define __s16(x)           short(x); short(reserved_1_##x)
#define __s32(x)           int(x)

#define map_adr(grp, adr)  ((64 * grp + adr) * 4)   // grp(0x0-0xff), adr(0x0-0x3f)
#define PMC_ACCESS(x)      (*(volatile u32 *)(pmc_base + x))
#define PMC_RAM(x)         (*(volatile u32 *)(pmc_ram_base + x))

#define pmc_map_adr(grp, adr)  (pmc_sfr_base + map_adr(grp, adr))
//===============================================================================//
//
//      typedef header file include
//
//===============================================================================//
#include "pmc_hal.h"

//===============================================================================//
//
//      sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for sfren
#define PMC_SFREN              ((PMC_SFREN_TypeDef    *) (pmc_map_adr(0x00, 0x00)))

//............. 0x0100 - 0x03ff............ for

//............. 0x0400 - 0x04ff............ for clock
#define PMC_RAND               ((JL_RAND_TypeDef    *) (pmc_map_adr(0x03, 0x00)))
#define PMC_CLOCK              ((PMC_CLOCK_TypeDef    *)(pmc_map_adr(0x04, 0x00)))
#define PMC_XOSC               ((JL_XOSC_TypeDef    *)pmc_map_adr(0x04, 0x18))

#define PMC_PWR_CON             PMC_CLOCK->PWR_CON
#define PMC_CLK_CON0            PMC_CLOCK->CLK_CON0

//............. 0x0500 - 0x05ff............ for xosc

//............. 0x0600 - 0x06ff............ for clkout
//#define PMC_CLKOUT             ((PMC_CLKOUT_TypeDef  *)(pmc_map_adr(0x06, 0x00)))

//............. 0x0500 - 0x05ff............ for xosc
#define PMC_DCDC               ((PMC_DCDC_TypeDef    *)(pmc_map_adr(0x05, 0x00)))

//............. 0x0700 - 0x07ff............ for pmc_memcfg
#define PMC_SYSTEM              ((PMC_SYSTEM_TypeDef    *)(pmc_map_adr(0x07, 0x00)))

// typedef struct {
//     __RW __u32 MCFG_RF1P;
//     __RW __u32 MCFG_RF2P;
//     __RW __u32 MCFG_RM1P;
//     __RW __u32 MCFG_RM2P;
//     __RW __u32 MCFG_VROM;
//     __RW __u32 MPWR_VROM;
// } PMC_MEMCFG_TypeDef;
//
// #define PMC_MEMCFG_BASE         (pmc_sfr_base + map_adr(0x07, 0x00))
// #define PMC_MEMCFG              ((PMC_MEMCFG_TypeDef *)PMC_MEMCFG_BASE)

//............. 0x0800 - 0x08ff............ for watch dog
//#define PMC_WDT                 ((PMC_WDT_TypeDef    *)(pmc_map_adr(0x08, 0x00)))

#define PMC_SIM_END             PMC_WDT->DUMMY

//............. 0x0900 - 0x0cff............ for lp timer
#define PMC_LPTMR0              ((PMC_LPTMR_TypeDef    *)(pmc_map_adr(0x09, 0x00)))
#define PMC_LPTMR1              ((PMC_LPTMR_TypeDef    *)(pmc_map_adr(0x0a, 0x00)))
//#define PMC_LPTMR2              ((PMC_LPTMR_TypeDef    *)(pmc_map_adr(0x0b, 0x00)))
//#define PMC_LPTMR3              ((PMC_LPTMR_TypeDef    *)(pmc_map_adr(0x0c, 0x00)))

//............. 0x0d00 - 0x0dff............ for irflt
#define PMC_GPCRC              ((JL_GPCRC_TypeDef    *)(pmc_map_adr(0x0d, 0x00)))

//............. 0x0e00 - 0x0eff............ for spi
#define PMC_SPI0                ((JL_SPI_TypeDef    *)pmc_map_adr(0x0e, 0x00))

//............. 0x0f00 - 0x10ff............ for uart
#define PMC_UART0                ((JL_UART_TypeDef         *)(pmc_map_adr(0x0f, 0x00)))
#define PMC_UART1                ((JL_UART_TypeDef         *)(pmc_map_adr(0x10, 0x00)))

//............. 0x1100 - 0x11ff............ for iic
#define PMC_IIC0               ((JL_I2C_TypeDef    *)(pmc_map_adr(0x11, 0x00)))
#define PMC_IIC1               ((JL_I2C_TypeDef    *)(pmc_map_adr(0x1c, 0x00)))

//............. 0x1500 - 0x15ff............ for port
#define PMC_PORTD                ((JL_PORT_TypeDef    *)(pmc_map_adr(0x12, 0x00)))
#define PMC_PORTE                ((JL_PORT_TypeDef    *)(pmc_map_adr(0x13, 0x00)))
#define PMC_PORTH                ((JL_PORT_TypeDef    *)(pmc_map_adr(0x14, 0x00)))
#define PMC_PORTF                ((JL_PORT_TypeDef    *)(pmc_map_adr(0x29, 0x00)))
#define PMC_PORTP                ((JL_PORT_TypeDef    *)(pmc_map_adr(0x2a, 0x00)))

//............. 0x1500 - 0x15ff............ for iomc
#define PMC_IOMC                 ((JL_LP_IOMC_TypeDef *)(pmc_map_adr(0x15, 0x00)))

//............. 0x1600 - 0x17ff............ for lp ctmu
#define PMC_LPCTM0               ((JL_LPCTM_TypeDef    *)(pmc_map_adr(0x16, 0x00)))

// #define PMC_LPCTM1_BASE          (pmc_sfr_base + map_adr(0x14, 0x00))
// #define PMC_LPCTM1               ((PMC_LPCTM_TypeDef    *)PMC_LPCTM1_BASE)

//............. 0x1500 - 0x15ff............ for lpvad
#define PMC_LPVAD               ((PMC_LPVAD_TypeDef    *)(pmc_map_adr(0x18, 0x00)))

//............. 0x1600 - 0x17ff............ for crossbar

//............. 0x1800 - 0x19ff............ for gp timer
#define PMC_GPTMR0              ((JL_GPTIMER_TypeDef    *)(pmc_map_adr(0x1b, 0x00)))
#define PMC_GPTMR1              ((JL_GPTIMER_TypeDef    *)(pmc_map_adr(0x1b, 0x05)))

//............. 0x1a00 - 0x1aff............ for NFC
#define PMC_NFC               ((JL_NFC_TypeDef    *)(pmc_map_adr(0x1d, 0x00)))


//............. 0x1b00 - 0x1bff............ for RESLOCK
#define PMC_RESLOCK          ((PMC_RESLOCK_TypeDef *)(pmc_map_adr(0x1e, 0x00)))


//............. 0x1c00 - 0x1cff............ for lp_gpcnt0
#define PMC_GPCNT0               ((JL_GPCNT_TypeDef    *)(pmc_map_adr(0x1f, 0x00)))
#define PMC_GPCNT1               ((JL_GPCNT_TypeDef    *)(pmc_map_adr(0x1f, 0x02)))

//............. 0x2000 - 0x20ff............ for lp_fft
typedef struct {
    __RW __u32 WLA_CON0 ;
    __RW __u32 WLA_CON1 ;
    __RW __u32 WLA_CON2 ;
    __RW __u32 WLA_CON3 ;
    __RW __u32 WLA_CON4 ;
    __RW __u32 WLA_CON5 ;
    __RW __u32 WLA_CON6 ;
    __RW __u32 WLA_CON7 ;
    __RW __u32 WLA_CON8 ;
    __RW __u32 WLA_CON9 ;
    __RW __u32 WLA_CON10;
    __RW __u32 WLA_CON11;
    __RW __u32 WLA_CON12;
    __RW __u32 WLA_CON13;
    __RW __u32 WLA_CON14;
    __RW __u32 WLA_CON15;
    __RW __u32 WLA_CON16;
    __RW __u32 WLA_CON17;
    __RW __u32 WLA_CON18;
    __RW __u32 WLA_CON19;
    __RW __u32 WLA_CON20;
    __RW __u32 WLA_CON21;
    __RW __u32 WLA_CON22;
    __RW __u32 WLA_CON23;
    __RW __u32 WLA_CON24;
    __RW __u32 WLA_CON25;
    __RW __u32 WLA_CON26;
    __RW __u32 WLA_CON27;
    __RW __u32 WLA_CON28;
    __RW __u32 WLA_CON29;
    __RW __u32 WLA_CON30;
    __RW __u32 WLA_CON31;
    __RW __u32 WLA_CON32;
    __RW __u32 WLA_CON33;
    __RW __u32 WLA_CON34;
    __RW __u32 WLA_CON35;
    __RW __u32 WLA_CON36;
    __RW __u32 WLA_CON37;
    __RW __u32 WLA_CON38;
    __RW __u32 WLA_CON39;
    __RW __u32 WLA_CON40;
    __RW __u32 WLA_CON41;
    __RW __u32 WLA_CON42;
    __RW __u32 WLA_CON43;
    __RW __u32 WLA_CON44;
    __RW __u32 WLA_CON45;
    __RW __u32 WLA_CON46;
    __RW __u32 WLA_CON47;
    __RW __u32 WLA_CON48;
    __RW __u32 WLA_CON49;
    __RW __u32 WLA_CON50;
    __RW __u32 WLA_CON51;
    __RW __u32 WLA_CON52;
    __RW __u32 WLA_CON53;
    __RW __u32 WLA_CON54;
    __RW __u32 WLA_CON55;
    __RW __u32 WLA_CON56;
    __RW __u32 WLA_CON57;
    __RW __u32 WLA_CON58;
    __RW __u32 WLA_CON59;
    __RW __u32 WLA_CON60;
    __RW __u32 WLA_CON61;
    __WO __u32 WO_CON0;
    __RO __u32 RO_CON0;
} JL_WLA_TypeDef;

#define JL_WLA               ((JL_WLA_TypeDef    *)(pmc_map_adr(0x20, 0x00)))
#define PMC_WWDG             ((JL_WWDG_TypeDef   *)(pmc_map_adr(0x21, 0x00)))
#define JL_PMC_PEMU          ((JL_PEMU_TypeDef   *)(pmc_map_adr(0x22, 0x00)))

#define PMC_FFT               ((PMC_FFT_TypeDef    *)(pmc_map_adr(0x23, 0x00)))

//............. 0x2400 - 0x24ff............ for pmc wlclk
#define PMC_PSCLK        ((PMC_PSCLK_TypeDef   *)(pmc_map_adr(0x24, 0x00)))

//............. 0x2500 - 0x26ff............ for pmc udma
#define PMC_UDMA         ((PMC_UDMA_TypeDef   *)(pmc_map_adr(0x25, 0x00)))

//............. 0x2700 - 0x27ff............ for pmc adc
#define PMC_ADC         ((JL_ADC_TypeDef   *)(pmc_map_adr(0x27, 0x00)))

//............. 0x2800 - 0x28ff............ for pmc soar cfg
#define PMC_SOARCFG         ((PMC_SOARCFG_TypeDef   *)(pmc_map_adr(0x28, 0x00)))

//............. 0x2b00 - 0x2bff............ for pmc sfc a2s
#define PMC_SFC0_A2S1        ((JL_SFCA2S_TypeDef     *)(pmc_map_adr(0x2b, 0x00)))

//............. 0x2c00 - 0x2cff............ for pmc sfc
#define PMC_SFC0            ((JL_SFC_TypeDef        *)(pmc_map_adr(0x2c, 0x00)))
#define PMC_SFCDEC0         ((JL_SFCDEC_TypeDef     *)(pmc_map_adr(0x2c, 0x0a)))
#define PMC_BOOTMMU0        ((JL_BOOTMMU_TypeDef    *)(pmc_map_adr(0x2c, 0x11)))

//............. 0x2d00 - 0x2dff............ for pmc fspi0
#define PMC_FSPI0            ((JL_SPI_TypeDef        *)(pmc_map_adr(0x2d, 0x00)))

//#define    JL_SFC       PMC_SFC0
//#define    JL_SFCDEC    PMC_SFCDEC0
//#define    JL_BOOTMMU   PMC_BOOTMMU0
//#define    JL_SPI       PMC_FSPI0

//............. 0x2e00 - 0x2eff............ for pmc fspi0
#define PMC_PCNT            ((JL_PCNT_TypeDef        *)(pmc_map_adr(0x2e, 0x00)))

//............. 0x3000 - 0x3fff............ for pmc soar cfg
#define PMC_AES               ((PMC_AES_TypeDef    *)(pmc_map_adr(0x30, 0x00)))

#define JL_FEC                ((JL_FEC_TypeDef     *)(pmc_map_adr(0x31,0x00)))


#endif

