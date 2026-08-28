#ifndef __LP_IPC_H__
#define __LP_IPC_H__

//===========================================================================//
//                              P2M MESSAGE TABLE                            //
//===========================================================================//
//==================power=============================
#define P2M_WKUP_SRC                                    P2M_MESSAGE_ACCESS(0)
#define P2M_WKUP_P_PND                                  P2M_MESSAGE_ACCESS(1)
#define P2M_WKUP_N_PND                                  P2M_MESSAGE_ACCESS(2)
#define P2M_AWKUP_P_PND                                 P2M_MESSAGE_ACCESS(3)
#define P2M_AWKUP_N_PND                                 P2M_MESSAGE_ACCESS(4)
#define P2M_WKUP_RTC                                    P2M_MESSAGE_ACCESS(5)
#define P2M_TIMEOUT_T0									P2M_MESSAGE_ACCESS(6)
#define P2M_TIMEOUT_T1									P2M_MESSAGE_ACCESS(7)
#define P2M_TIMEOUT_T2									P2M_MESSAGE_ACCESS(8)
#define P2M_TIMEOUT_T3									P2M_MESSAGE_ACCESS(9)
#define P2M_OSC_CNT0									P2M_MESSAGE_ACCESS(10)
#define P2M_OSC_CNT1									P2M_MESSAGE_ACCESS(11)
#define P2M_OSC_CNT2									P2M_MESSAGE_ACCESS(12)
#define P2M_OSC_CNT3									P2M_MESSAGE_ACCESS(13)
#define P2M_LPTMR_FLAG									P2M_MESSAGE_ACCESS(14)



//==================system===========================
#define P2M_MESSAGE_BANK_ADR_L                          P2M_MESSAGE_ACCESS(15)
#define P2M_MESSAGE_BANK_ADR_H                          P2M_MESSAGE_ACCESS(16)
#define P2M_MESSAGE_BANK_INDEX                          P2M_MESSAGE_ACCESS(17)
#define P2M_MESSAGE_BANK_ACK                            P2M_MESSAGE_ACCESS(18)
#define P2M_P11_HEAP_BEGIN_ADDR_L    					P2M_MESSAGE_ACCESS(19)
#define P2M_P11_HEAP_BEGIN_ADDR_H    					P2M_MESSAGE_ACCESS(20)
#define P2M_P11_HEAP_SIZE_L    							P2M_MESSAGE_ACCESS(21)
#define P2M_P11_HEAP_SIZE_H    							P2M_MESSAGE_ACCESS(22)
#define P2M_REPLY_SYNC_CMD                            	P2M_MESSAGE_ACCESS(23)

#define Q32S_MSG_Q_ADDR0								P2M_MESSAGE_ACCESS(24)
#define Q32S_MSG_Q_ADDR1                                P2M_MESSAGE_ACCESS(25)
#define Q32S_MSG_Q_ADDR2                                P2M_MESSAGE_ACCESS(26)
#define Q32S_MSG_Q_ADDR3                                P2M_MESSAGE_ACCESS(27)

#define MSYS_MSG_Q_ADDR0                                P2M_MESSAGE_ACCESS(28)
#define MSYS_MSG_Q_ADDR1                                P2M_MESSAGE_ACCESS(29)
#define MSYS_MSG_Q_ADDR2                                P2M_MESSAGE_ACCESS(30)
#define MSYS_MSG_Q_ADDR3                                P2M_MESSAGE_ACCESS(31)

#define Q32F_MSG_Q_ADDR0                             	P2M_MESSAGE_ACCESS(32)
#define Q32F_MSG_Q_ADDR1                             	P2M_MESSAGE_ACCESS(33)
#define Q32F_MSG_Q_ADDR2                             	P2M_MESSAGE_ACCESS(34)
#define Q32F_MSG_Q_ADDR3                             	P2M_MESSAGE_ACCESS(35)

//==================clock===========================
#define P2M_BTOSC_OK									P2M_MESSAGE_ACCESS(36)
#define P2M_STRC_FREQ0									P2M_MESSAGE_ACCESS(37)
#define P2M_STRC_FREQ1									P2M_MESSAGE_ACCESS(38)
#define P2M_STRC_FREQ2									P2M_MESSAGE_ACCESS(39)
#define P2M_STRC_FREQ3									P2M_MESSAGE_ACCESS(40)

//==================vad=============================
#define P2M_MESSAGE_VAD_CMD                             P2M_MESSAGE_ACCESS(41)
#define P2M_MESSAGE_VAD_CBUF_WPTR                       P2M_MESSAGE_ACCESS(42)

//==================lpctmu===========================
#define P2M_CTMU_CMD_ACK                                P2M_MESSAGE_ACCESS(43)

#define P2M_MASSAGE_CTMU_CH0_L_RES                                         44
#define P2M_MASSAGE_CTMU_CH0_H_RES                                         45
#define P2M_CTMU_CH0_L_RES                              P2M_MESSAGE_ACCESS(0 * 2 + P2M_MASSAGE_CTMU_CH0_L_RES)
#define P2M_CTMU_CH0_H_RES                              P2M_MESSAGE_ACCESS(0 * 2 + P2M_MASSAGE_CTMU_CH0_H_RES)
#define P2M_CTMU_CH1_L_RES                              P2M_MESSAGE_ACCESS(1 * 2 + P2M_MASSAGE_CTMU_CH0_L_RES)
#define P2M_CTMU_CH1_H_RES                              P2M_MESSAGE_ACCESS(1 * 2 + P2M_MASSAGE_CTMU_CH0_H_RES)
#define P2M_CTMU_CH2_L_RES                              P2M_MESSAGE_ACCESS(2 * 2 + P2M_MASSAGE_CTMU_CH0_L_RES)
#define P2M_CTMU_CH2_H_RES                              P2M_MESSAGE_ACCESS(2 * 2 + P2M_MASSAGE_CTMU_CH0_H_RES)
#define P2M_CTMU_CH3_L_RES                              P2M_MESSAGE_ACCESS(3 * 2 + P2M_MASSAGE_CTMU_CH0_L_RES)
#define P2M_CTMU_CH3_H_RES                              P2M_MESSAGE_ACCESS(3 * 2 + P2M_MASSAGE_CTMU_CH0_H_RES)
#define P2M_CTMU_CH4_L_RES                              P2M_MESSAGE_ACCESS(4 * 2 + P2M_MASSAGE_CTMU_CH0_L_RES)
#define P2M_CTMU_CH4_H_RES                              P2M_MESSAGE_ACCESS(4 * 2 + P2M_MASSAGE_CTMU_CH0_H_RES)


//===========================================================================//
//                              M2P MESSAGE TABLE                            //
//===========================================================================//
//==================power=============================
#define M2P_LRC_PRD                                     M2P_MESSAGE_ACCESS(0)
#define M2P_WVDD                                		M2P_MESSAGE_ACCESS(1)
#define M2P_WVDD2                                		M2P_MESSAGE_ACCESS(2)
#define M2P_LRC_FEQ0									M2P_MESSAGE_ACCESS(3)
#define M2P_LRC_FEQ1									M2P_MESSAGE_ACCESS(4)
#define M2P_LRC_FEQ2									M2P_MESSAGE_ACCESS(5)
#define M2P_LRC_FEQ3									M2P_MESSAGE_ACCESS(6)
#define M2P_VDDIO_KEEP                                  M2P_MESSAGE_ACCESS(7)
#define M2P_LRC_KEEP                                    M2P_MESSAGE_ACCESS(8)
#define M2P_RCH_FEQ_L                                   M2P_MESSAGE_ACCESS(9)
#define M2P_RCH_FEQ_H                                   M2P_MESSAGE_ACCESS(10)
#define M2P_MEM_CONTROL                                 M2P_MESSAGE_ACCESS(11)
#define M2P_XOSC24M_KEEP                                M2P_MESSAGE_ACCESS(12)
#define M2P_CTMU_KEEP									M2P_MESSAGE_ACCESS(13)
#define M2P_RTC_KEEP                                    M2P_MESSAGE_ACCESS(14)
#define M2P_SF_MODE										M2P_MESSAGE_ACCESS(15)
#define M2P_DCV_MODE									M2P_MESSAGE_ACCESS(16)
#define M2P_LIGHT_PDOWN_DVDD_VOL						M2P_MESSAGE_ACCESS(17)
#define M2P_LRC24M_MODE									M2P_MESSAGE_ACCESS(18)
#define M2P_RC16M_FRE_MHZ                               M2P_MESSAGE_ACCESS(19)
#define M2P_LP_AVAD_EN                                  M2P_MESSAGE_ACCESS(20)
#define M2P_DCDC12_CLK_SEL                              M2P_MESSAGE_ACCESS(21)
#define M2P_DCDC12_ULP_LOW_VOL                          M2P_MESSAGE_ACCESS(22)
#define M2P_DCDC12_ULP_HIGH_VOL                         M2P_MESSAGE_ACCESS(23)
#define M2P_DCDC12_ULP_NOW_VOL                          M2P_MESSAGE_ACCESS(24)
#define M2P_SVDDB_RAM_KEEP                              M2P_MESSAGE_ACCESS(25)
#define M2P_SVDDC_RAM_KEEP                              M2P_MESSAGE_ACCESS(26)
#define M2P_MSYS_MODE                                   M2P_MESSAGE_ACCESS(27)
#define M2P_PMC_MODE                                    M2P_MESSAGE_ACCESS(28)
#define M2P_BT_MODE                                     M2P_MESSAGE_ACCESS(29)

//==================system===========================
#define M2P_SYNC_CMD                                  	M2P_MESSAGE_ACCESS(30)
#define M2P_WDT_SYNC                                   	M2P_MESSAGE_ACCESS(31)
#define M2P_WAIT_RELEASE								M2P_MESSAGE_ACCESS(32)

//==================clock===========================
#define M2P_LRC24M_CFG0                                 M2P_MESSAGE_ACCESS(35)
#define M2P_LRC24M_CFG1                                 M2P_MESSAGE_ACCESS(36)
#define M2P_BTOSC_CFG0                                  M2P_MESSAGE_ACCESS(37)
#define M2P_BTOSC_CFG1                                  M2P_MESSAGE_ACCESS(38)

#define M2P_LRC24M_FEQ0                                 M2P_MESSAGE_ACCESS(39)
#define M2P_LRC24M_FEQ1                                 M2P_MESSAGE_ACCESS(40)
#define M2P_LRC24M_FEQ2                                 M2P_MESSAGE_ACCESS(41)
#define M2P_LRC24M_FEQ3                                 M2P_MESSAGE_ACCESS(42)

//==================vad=============================
#define M2P_MESSAGE_VAD_CMD                             M2P_MESSAGE_ACCESS(43)
#define M2P_MESSAGE_VAD_CBUF_RPTR                       M2P_MESSAGE_ACCESS(44)
#define M2P_MESSAGE_VAD_PEND                            M2P_MESSAGE_ACCESS(45)

//==================lpctmu===========================
/*触摸所有通道配置*/
#define M2P_CTMU_CMD  									M2P_MESSAGE_ACCESS(50)
#define M2P_CTMU_CH_ENABLE								M2P_MESSAGE_ACCESS(51)
#define M2P_CTMU_CH_WAKEUP_EN					        M2P_MESSAGE_ACCESS(52)
#define M2P_CTMU_SCAN_TIME          					M2P_MESSAGE_ACCESS(53)
#define M2P_CTMU_LOWPOER_SCAN_TIME     					M2P_MESSAGE_ACCESS(54)

//==================bt===========================
#define M2P_BT_INIT     								M2P_MESSAGE_ACCESS(55)
#define M2P_BT_RF_DBG     								M2P_MESSAGE_ACCESS(56)

//==================gpadc===========================
#define M2P_VBAT_VOL_BUF_IN 							M2P_MESSAGE_ACCESS(57)
#define M2P_VBAT_VOL_BUF_BEGIN							M2P_MESSAGE_ACCESS(58)//这里占40bytes
#define M2P_VBAT_ADC_CH_SEL 							M2P_MESSAGE_ACCESS(98)
#define M2P_VBAT_ADC_CH_DIV 							M2P_MESSAGE_ACCESS(99)
#define M2P_VBAT_ADC_TRIM_EN 							M2P_MESSAGE_ACCESS(100)
#define M2P_VBAT_ADC_TRIM_DG 							M2P_MESSAGE_ACCESS(101)
#define M2P_VBAT_ADC_TRIM_VOL 							M2P_MESSAGE_ACCESS(102)
#define M2P_PROGI_VOL_L      							M2P_MESSAGE_ACCESS(103)
#define M2P_PROGI_VOL_H      							M2P_MESSAGE_ACCESS(104)

//==================aes==========================
#define M2P_AES128     								    M2P_MESSAGE_ACCESS(105)
#define M2P_CCM_V3     								    M2P_MESSAGE_ACCESS(106)

//===========================================================================//
//                              P2P MESSAGE TABLE                            //
//===========================================================================//
#define P2P_F2S_CMD										P2P_MESSAGE_ACCESS(0)
#define P2P_S2F_CMD										P2P_MESSAGE_ACCESS(1)

//==================aes==========================
#define P2P_AES128     								    P2P_MESSAGE_ACCESS(10)
#define P2P_CCM_V3     								    P2P_MESSAGE_ACCESS(11)

enum {
    F2S_CMD_SUSPEND = 1,
    F2S_CMD_RESUME,
    F2S_CMD_POWER_RECOVER,
};

enum {
    S2F_CMD_SUSPEND = 1,
    S2F_CMD_RESUME,
};

#define S_M2P_INT_OFFSET_CMAC		0
#define S_M2P_INT_OFFSET_START		1
#define S_M2P_INT_OFFSET_END		8
#define F_M2P_INT_OFFSET_START		S_M2P_INT_OFFSET_END
#define F_M2P_INT_OFFSET_END		16

/*
 *  Must Sync to P11 code
 */
enum {
    M2P_CMAC_INDEX = 0, ///int 0
    M2P_CMAC_RFU_INDEX,

    M2P_LP_INDEX = S_M2P_INT_OFFSET_START * 2,	// int0
    M2P_PF_INDEX,
    M2P_LLP_INDEX,	// int1
    M2P_P33_INDEX,
    M2P_SF_INDEX,	// int2
    M2P_CTMU_INDEX,
    M2P_CCMD_INDEX,	// int3       //common cmd
    M2P_WDT_INDEX,
    M2P_SYNC_INDEX,	// int4
    M2P_APP_INDEX,
    M2P_WKUP_INDEX,	// int5

    M2P_VAD_INDEX = F_M2P_INT_OFFSET_START * 2, // int8
    M2P_F_APP_INDEX,
};

#define M2P_IDX_MASK_S		(0x0000ffff)
#define M2P_IDX_MASK_F		(0xffff0000)
#define M2P_IDX_MASK_POWER	(BIT(M2P_LP_INDEX) | BIT(M2P_PF_INDEX) | BIT(M2P_LLP_INDEX))



#define P2M_INT_OFFSET_SYSTEM_START		0
#define P2M_INT_OFFSET_SYSTEM_END		12
#define P2M_INT_OFFSET_BT_START			P2M_INT_OFFSET_SYSTEM_END
#define P2M_INT_OFFSET_BT_END			16

enum {
    P2M_LP_INDEX = P2M_INT_OFFSET_SYSTEM_START,	// int0
    P2M_WK_MSYS_INDEX,
    P2M_LLP_INDEX,	// int1
    P2M_WK_INDEX,
    P2M_WDT_INDEX,	// int2
    P2M_LP_INDEX2,
    P2M_CTMU_INDEX,	// int3
    P2M_CTMU_POWUP,
    P2M_REPLY_CCMD_INDEX,	// int4  //reply common cmd
    P2M_VAD_INDEX,
    P2M_USER_INDEX,	// int5
    P2M_BANK_INDEX,
    P2M_REPLY_SYNC_INDEX,	// int6
    P2M_APP_INDEX,
    P2M_OSC_INDEX,	// int7

    P2M_BT_ESCO_INDEX = P2M_INT_OFFSET_BT_START * 2,	// int12
    P2M_BT_RFU_INDEX,	// int12
    P2M_CMAC_INDEX,		// int13
    P2M_CMAC_RFU_INDEX,	// int13
};

#define P2M_IDX_MASK_SYSTEM		(0x00ffffff)
#define P2M_IDX_MASK_BT			(0xff000000)

enum {
    P2M_PRP_INT_UART0,
    P2M_PRP_INT_UART1,
    P2M_PRP_INT_IIC0,
    P2M_PRP_INT_IIC1,
    P2M_PRP_INT_LPTMR0,
    P2M_PRP_INT_LPTMR1,
    P2M_PRP_INT_GPTMR0,
    P2M_PRP_INT_GPTMR1,

    P2M_PRP_INT_GPCNT0,
    P2M_PRP_INT_GPCNT1,
    P2M_PRP_INT_UDMA0,
    P2M_PRP_INT_UDMA1,
    P2M_PRP_INT_UDMA2,
    P2M_PRP_INT_UDMA3,
    P2M_PRP_INT_FFT,
    P2M_PRP_INT_WWDG,

    P2M_PRP_INT_LPVAD,
    P2M_PRP_INT_AES,
    P2M_PRP_INT_GPADC,
    P2M_PRP_INT_LCTM0,
    P2M_PRP_INT_LPSPI0,
    P2M_PRP_INT_FSPI0,
    P2M_PRP_INT_SFC0_A2S1,
    P2M_PRP_INT_TXHA,

    P2M_PRP_INT_BT_BLERX,
    P2M_PRP_INT_BT_EVENT,
    P2M_PRP_INT_BT_TIMEBASE,
    P2M_PRP_INT_BT_CLKN,
    P2M_PRP_INT_BT_BREDR,
    P2M_PRP_INT_SRC_SYNC,
    P2M_PRP_INT_BREDR_RXMCH,
    P2M_PRP_INT_SYNC_GETMSG,
};

enum {
    CLOSE_P33_INTERRUPT = 1,
    OPEN_P33_INTERRUPT,
    LOWPOWER_PREPARE,
    M2P_SPIN_LOCK,
    M2P_SPIN_UNLOCK,
    P2M_SPIN_LOCK,
    P2M_SPIN_UNLOCK,

};

enum {
    MSG_CLOCK_LRC24M_OK = 1,
};

#include "power/lp_msg.h"

#endif
