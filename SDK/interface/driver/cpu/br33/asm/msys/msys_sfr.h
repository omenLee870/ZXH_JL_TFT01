
#ifndef __MSYS__
#define __MSYS__

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

#define hs_base            0x7ce0000
// using by husb           0x7cd0000
#define ls_base            0x7cc0000

#define __RW               volatile       // read write
#define __RO               volatile const // only read
#define __WO               volatile       // only write

#define __u8               unsigned int   // u8  to u32 special for struct
#define __u16              unsigned int   // u16 to u32 special for struct
#define __u32              unsigned int

#define __s8(x)            char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define __s16(x)           short(x); short(reserved_1_##x)
#define __s32(x)           int(x)

#define map_adr(grp, adr)  ((64 * grp + adr) * 4)     // grp(0x0-0xff), adr(0x0-0x3f)
#define hs_map_adr(grp, adr)  (hs_base + map_adr(grp, adr))
#define ls_map_adr(grp, adr)  (ls_base + map_adr(grp, adr))
//===============================================================================//
//
//      typedef header file include
//
//===============================================================================//
#include "msys_hal.h"

//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//

#define JL_HEMU     ((JL_HEMU_TypeDef       *)(hs_map_adr(0x00, 0x00)))
#define JL_HMEMCFG  ((JL_COM_MEMCFG_TypeDef *)(hs_map_adr(0x01, 0x00)))
#define JL_HSBRST   ((JL_HSBRST_TypeDef     *)(hs_map_adr(0x02, 0x00)))
#define JL_HSBCLK   ((JL_HSBCLK_TypeDef     *)(hs_map_adr(0x02, 0x01)))
#define JL_MODE     ((JL_MODE_TypeDef       *)(hs_map_adr(0x03, 0x00)))
typedef struct {
    __RW __u8  ID;
    __RW __u8  STATUS;
    __RW __u8  PASS;
    __RW __u8  TERMINAL;
    __WO __u32 EN ;
    __WO __u32 RADR ;
    __WO __u32 RDAT ;
    __WO __u32 WADR ;
    __WO __u32 WDAT ;
    __WO __u32 INIT ;
} JL_JIBC_TypeDef;
#define JL_JIBC         ((JL_JIBC_TypeDef       *)(hs_map_adr(0x04, 0x00)))

#define JL_FSPI1        ((JL_SPI_TypeDef        *)(hs_map_adr(0x08, 0x00)))
#define JL_DCP          ((JL_DCP_TypeDef        *)(hs_map_adr(0x0a, 0x00)))

#define JL_EQ0          ((JL_EQ_TypeDef			*)(hs_map_adr(0x0c, 0x00)))
#define JL_SRC0         ((JL_SRC_TypeDef        *)(hs_map_adr(0x0d, 0x00)))
#define JL_SRC1         ((JL_SRC_TypeDef        *)(hs_map_adr(0x0e, 0x00)))

#define JL_SFC0_A2S0    ((JL_SFCA2S_TypeDef     *)(hs_map_adr(0x10, 0x00)))
#define JL_SFC1_A2S0    ((JL_SFCA2S_TypeDef     *)(hs_map_adr(0x11, 0x00)))
#define JL_SFC1         ((JL_SFC_TypeDef        *)(hs_map_adr(0x12, 0x00)))
#define JL_SFCDEC1      ((JL_SFCDEC_TypeDef     *)(hs_map_adr(0x12, 0x0a)))
#define JL_BOOTMMU1     ((JL_BOOTMMU_TypeDef    *)(hs_map_adr(0x12, 0x11)))

#define JL_EQ1          ((JL_EQ_TypeDef			*)(hs_map_adr(0x13, 0x00)))
#define JL_FIR          ((JL_FIR_TypeDef        *)(hs_map_adr(0x14, 0x00)))
#define JL_ECU          ((JL_ECU_TypeDef        *)(hs_map_adr(0x15, 0x00)))

//#define JL_FEC          ((JL_FEC_TypeDef        *)(hs_map_adr(0x16, 0x00)))
#define JL_GPCRC        ((JL_GPCRC_TypeDef      *)(hs_map_adr(0x17, 0x00)))

#define JL_VMEMCFG      ((JL_COM_MEMCFG_TypeDef *)(hs_map_adr(0x18, 0x00)))
#define JL_VDOCOM       ((JL_VDOCOM_TypeDef     *)(hs_map_adr(0x19, 0x00)))
//#define JL_GPU2D        ((JL_GPU2D_TypeDef      *)(hs_map_adr(0x20, 0x00)))
//#define JL_JPG          ((JL_JPG_TypeDef        *)(hs_map_adr(0x21, 0x00)))

#define JL_MTAP         ((JL_MTAP_TypeDef       *)(hs_map_adr(0x23, 0x00)))


#define JL_XSPI         ((JL_XSPI_TypeDef       *)(hs_map_adr(0x24, 0x00)))
#define JL_XSPI_MMU     ((JL_XSPI_MMU_TypeDef   *)(hs_map_adr(0x25, 0x00)))
#define JL_MEMPWR       ((JL_COM_MEMPWR_TypeDef *)(hs_map_adr(0x26, 0x00)))
#define JL_SOARCFG      ((JL_COM_SOARCFG_TypeDef*)(hs_map_adr(0x27, 0x00)))

//#define JL_GXDEC        ((JL_GXDEC_TypeDef      *)(hs_map_adr(0x28, 0x00)))

//#define JL_JXENC        ((JL_GXDEC_TypeDef      *)(hs_map_adr(0x29, 0x00)))
//#define JL_NCHENC0      ((JL_GXDEC_TypeDef      *)(hs_map_adr(0x2a, 0x00)))
//#define JL_NCHENC1      ((JL_GXDEC_TypeDef      *)(hs_map_adr(0x2b, 0x00)))
//#define JL_NCHENC2      ((JL_GXDEC_TypeDef      *)(hs_map_adr(0x2c, 0x00)))
#define JL_MXP_CNT      ((JL_MXP_CNT_TypeDef    *)(hs_map_adr(0x2d, 0x00)))

//............. 0x3000 - 0x3fff............

//===============================================================================/
//      low speed sfr address define
//
//===============================================================================//

#define JL_LEMU         ((JL_LEMU_TypeDef       *)(ls_map_adr(0x00, 0x00)))
#define JL_PMU          ((JL_PMU_TypeDef        *)(ls_map_adr(0x01, 0x00)))
#define JL_EFUSE0       ((JL_EFUSE0_TypeDef     *)(ls_map_adr(0x02, 0x00)))
//#define JL_DCDC         ((JL_DCDC_TypeDef       *)(ls_map_adr(0x03, 0x00)))
#define JL_SPI0         ((JL_SPI_TypeDef        *)(ls_map_adr(0x04, 0x00)))
#define JL_SPI1         ((JL_SPI_TypeDef        *)(ls_map_adr(0x05, 0x00)))
#define JL_IIC0         ((JL_I2C_TypeDef        *)(ls_map_adr(0x06, 0x00)))
#define JL_IIC1         ((JL_I2C_TypeDef        *)(ls_map_adr(0x07, 0x00)))
#define JL_IIC2         ((JL_I2C_TypeDef        *)(ls_map_adr(0x08, 0x00)))
#define JL_IIC3         ((JL_I2C_TypeDef        *)(ls_map_adr(0x09, 0x00)))
#define JL_IIC4         ((JL_I2C_TypeDef        *)(ls_map_adr(0x0a, 0x00)))
#define JL_IIC5         ((JL_I2C_TypeDef        *)(ls_map_adr(0x0b, 0x00)))
#define JL_UART0        ((JL_UART_TypeDef       *)(ls_map_adr(0x0c, 0x00)))
#define JL_UART1        ((JL_UART_TypeDef       *)(ls_map_adr(0x0d, 0x00)))
#define JL_UART2        ((JL_UART_TypeDef       *)(ls_map_adr(0x0e, 0x00)))
#define JL_UART3        ((JL_UART_TypeDef       *)(ls_map_adr(0x35, 0x00)))
#define JL_UART2_DT     ((JL_UART_DT_TypeDef    *)(ls_map_adr(0x0f, 0x00)))

#define JL_SD0          ((JL_SDC_TypeDef        *)(ls_map_adr(0x10, 0x00)))
#define JL_CAN          ((JL_CAN_TypeDef        *)(ls_map_adr(0x11, 0x00)))
#define JL_ECAN         ((JL_ECAN_TypeDef       *)(ls_map_adr(0x11, 0x00)))
#define JL_CANDMA       ((JL_CANDMA_TypeDef     *)(ls_map_adr(0x12, 0x00)))
#define JL_NFC          ((JL_NFC_TypeDef        *)(ls_map_adr(0x13, 0x00)))
#define JL_TCRM         ((JL_TCRM_TypeDef       *)(ls_map_adr(0x14, 0x00)))
#define JL_ADV_TIMER0   ((JL_ADV_TIMER_TypeDef  *)(ls_map_adr(0x15, 0x00)))
#define JL_TIMER0       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*0)))
#define JL_TIMER1       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*1)))
#define JL_TIMER2       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*2)))
#define JL_TIMER3       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*3)))
#define JL_TIMER4       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*4)))
#define JL_TIMER5       ((JL_GPTIMER_TypeDef    *)(ls_map_adr(0x16, 5*5)))
#define JL_GPCNT0       ((JL_GPCNT_TypeDef      *)(ls_map_adr(0x17, 2*0)))
#define JL_GPCNT1       ((JL_GPCNT_TypeDef      *)(ls_map_adr(0x17, 2*1)))
#define JL_GPCNT2       ((JL_GPCNT_TypeDef      *)(ls_map_adr(0x17, 2*2)))
#define JL_GPCNT3       ((JL_GPCNT_TypeDef      *)(ls_map_adr(0x17, 2*3)))
#define JL_PCNT         ((JL_PCNT_TypeDef       *)(ls_map_adr(0x18, 0x00)))
#define JL_QDEC0        ((JL_QDEC_TypeDef       *)(ls_map_adr(0x19, 0x00)))
#define JL_QDEC1        ((JL_QDEC_TypeDef       *)(ls_map_adr(0x1a, 0x00)))
#define JL_QDEC2        ((JL_QDEC_TypeDef       *)(ls_map_adr(0x1b, 0x00)))
#define JL_RAND         ((JL_RAND_TypeDef       *)(ls_map_adr(0x1c, 0x00)))
#define JL_WWDG         ((JL_WWDG_TypeDef       *)(ls_map_adr(0x1d, 0x00)))
#define JL_PLED         ((JL_PLED_TypeDef       *)(ls_map_adr(0x1e, 0x00)))
#define JL_WAKEUP       ((JL_WAKEUP_TypeDef     *)(ls_map_adr(0x1f, 0x00)))

#define JL_PORTA        ((JL_PORT_TypeDef       *)(ls_map_adr(0x28, 0x00)))
#define JL_PORTB        ((JL_PORT_TypeDef       *)(ls_map_adr(0x29, 0x00)))
#define JL_PORTC        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2a, 0x00)))
#define JL_PORTD        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2b, 0x00)))
#define JL_PORTE        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2c, 0x00)))
// #define JL_PORTF        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2d, 0x00))) //pmc io
#define JL_PORTG        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2e, 0x00)))
#define JL_PORTH        ((JL_PORT_TypeDef       *)(ls_map_adr(0x2f, 0x00)))
#define JL_PORTS        ((JL_PORT_TypeDef       *)(ls_map_adr(0x30, 0x00)))
// #define JL_PORTP        ((JL_PORT_TypeDef       *)(ls_map_adr(0x31, 0x00))) //pmc io
#define JL_PORTT        ((JL_PORT_TypeDef       *)(ls_map_adr(0x32, 0x00)))
#define JL_PORTUSB     ((JL_PORT_TypeDef       *)(ls_map_adr(0x33, 0x00)))//兼容旧芯片
#define JL_PORTHUSB     ((JL_PORT_TypeDef       *)(ls_map_adr(0x33, 0x00)))
#define JL_XSDC0        ((JL_SDC_TypeDef        *)(ls_map_adr(0x34, 0x00)))
#define JL_SD1          JL_XSDC0

#define JL_IOMC         ((JL_IOMC_TypeDef       *)(ls_map_adr(0x3e, 0x00)))

#define JL_SPDIF        ((JL_SPDIF_TypeDef      *)(ls_map_adr(0x46, 0x00)))
#define JL_WL_UART      ((JL_UART_LITE_TypeDef  *)(ls_map_adr(0x47, 0x00)))

#define JL_LSBRST       ((JL_LSBRST_TypeDef     *)(ls_map_adr(0x48, 0x00)))
#define JL_LSBCLK       ((JL_LSBCLK_TypeDef     *)(ls_map_adr(0x48, 0x01)))
#define JL_XOSC         PMC_XOSC
#define JL_SYSPLL       ((JL_SYSPLL_TypeDef     *)(ls_map_adr(0x49, 0x00)))
#define JL_XPLL         ((JL_XPLL_TypeDef       *)(ls_map_adr(0x4a, 0x00)))
//#define JL_WLA          ((JL_WLA_TypeDef        *)(ls_map_adr(0x4b, 0x00)))
#define JL_LMEMCFG      ((JL_COM_MEMCFG_TypeDef *)(ls_map_adr(0x4c, 0x00)))
#define JL_UDMA         ((JL_UDMA_TypeDef       *)(ls_map_adr(0x4d, 0x00)))
//#define                 ((                      *)(ls_map_adr(0x4e, 0x00)))
//#define                 ((                  	*)(ls_map_adr(0x4f, 0x00)))

#define husb_base(i)    (0x7cd0000 + 0x1000*i)
#define JL_HUSBPLL      ((JL_SYSPLL_TypeDef     *)husb_base(0))



#endif

