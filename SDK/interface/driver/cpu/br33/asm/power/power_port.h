#ifndef __POWER_PORT_H__
#define __POWER_PORT_H__

//
//
//					FLASH PIN
//
//
//
//*****************************************************************************/
#define GET_SFC_PORT()		((PMC_IOMC->SYS_IOMC & BIT(0)) ? 1:0)

#if 0 //flash 引脚在pmc
/******************************************************************************/
#define     _PORT(p)            JL_PORT##p
#define     _PORT_IN(p,b)       P##p##b##_IN
#define     _PORT_OUT(p,b)      JL_OMAP->P##p##b##_OUT

/****************************spi boot *****************************************/
// #define     SPI_PORT(p)         _PORT(p)
// #define     SPI0_FUNC_OUT(p,b)  _PORT_OUT(p,b)
// #define     SPI0_FUNC_IN(p,b)   _PORT_IN(p,b)
// | func\port |  A   | B    |
// |-----------|------|------|
// | VCC       | FSPG |  |
// | CS        | PD3  |  |
// | CLK       | PD0  |  |
// | DO(D0)    | PD1  |  |
// | DI(D1)    | PD2  |  |
// | WP(D2)    | PA5  |  |
// | HOLD(D3)  | PA6  |  |


//FSPG define bit
#define     FSPG_A              0   //FSPG output bit
#define     FSPG_CS_EN          1   //FSPG CS connect enable bit
#define     FSPG_HD0            2   //FSPG HD0
#define     FSPG_HD1            3   //FSPG HD1
#define     FSPG_OE             4   //FSPG output enable bit
#define     FSPG_PD             5   //FSPG pull down enable bit
#define     FSPG_PD1            6   //FSPG pull down enable bit
#define     FSPG_18V            7   //flash supply power domain 1: DCVDD 1.8V; 0: IOVDD;

////////////////////////////////////////////////////////////////////////////////
//group a
// #define     PORT_SPI0_CSA       F
// #define     SPI0_CSA            0
//
// #define     PORT_SPI0_CLKA      F
// #define     SPI0_CLKA           4
//
// #define     PORT_SPI0_DOA       F
// #define     SPI0_DOA            5
//
// #define     PORT_SPI0_DIA       F
// #define     SPI0_DIA            1
//
// #define     PORT_SPI0_D2A       F
// #define     SPI0_D2A            2
//
// #define     PORT_SPI0_D3A       F
// #define     SPI0_D3A            3


//#define		SPI0_PWR_A		IO_PORTD_04
#define		SPI0_CS_A		IO_PORTF_00
#define 	SPI0_CLK_A		IO_PORTF_04
#define 	SPI0_DO_D0_A	IO_PORTF_05
#define 	SPI0_DI_D1_A	IO_PORTF_01
#define 	SPI0_WP_D2_A	IO_PORTF_02
#define 	SPI0_HOLD_D3_A	IO_PORTF_03

////////////////////////////////////////////////////////////////////////////////
//group b
// #define     PORT_SPI0_CSB       F
// #define     SPI0_CSB            3
//
// #define     PORT_SPI0_CLKB      F
// #define     SPI0_CLKB           1
//
// #define     PORT_SPI0_DOB       F
// #define     SPI0_DOB            0
//
// #define     PORT_SPI0_DIB       F
// #define     SPI0_DIB            4
//
// #define     PORT_SPI0_D2B       F
// #define     SPI0_D2B            5
//
// #define     PORT_SPI0_D3B       F
// #define     SPI0_D3B            2

//#define       SPI0_PWR_B		IO_PORTD_04
#define     SPI0_CS_B		IO_PORTF_03
#define     SPI0_CLK_B		IO_PORTF_01
#define     SPI0_DO_D0_B	IO_PORTF_00
#define     SPI0_DI_D1_B	IO_PORTF_04
#define     SPI0_WP_D2_B	IO_PORTF_05
#define     SPI0_HOLD_D3_B	IO_PORTF_02
#endif

////////////////////////////////////////////////////////////////////////////////

//
//
//					PSRAM
//
//
//
//*****************************************************************************/
#define		PSRAM_CS		IO_PORTC_06
#define		PSRAM_CLK		IO_PORTC_05
#define		PSRAM_D0A		IO_PORTC_09
#define		PSRAM_D1A		IO_PORTC_07
#define		PSRAM_D2A		IO_PORTC_08
#define		PSRAM_D3A		IO_PORTC_04

#define		PINR_DEFAULT_IO IO_PORTB_07

#define		MCLR_PORT		IO_PORTB_06

#endif
