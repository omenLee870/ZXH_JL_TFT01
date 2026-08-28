/**@file  		pmc_hw.h
* @brief        hw sfr layer
* @date     	2024-05-16
* @version    	V1.0
* @copyright  	Copyright(c)2010-2031  JIELI
* @details		该文件对p11 clock相关寄存器封装
 */
#ifndef __PMC_CLOCK_HW_H__
#define __PMC_CLOCK_HW_H__

//
//
//				clock system
//
//
//
/**************************************************************/

enum PMC_SYS_CLK_TABLE {
    PMC_SYS_CLK_RC16M = 0,
    PMC_SYS_CLK_RC250K,
    PMC_SYS_CLK_LRC_OSC,
    PMC_SYS_CLK_BTOSC_24M,
    PMC_SYS_CLK_BTOSC_48M,
    PMC_SYS_CLK_LRC24M,
    PMC_SYS_CLK_CLK_X2,
    PMC_SYS_CLK_TEST,
};

#define PMC_SYS_CLK_SEL(x) 			SFR(PMC_CLOCK->SRC_CKSEL, 0, 3, x)

#endif
