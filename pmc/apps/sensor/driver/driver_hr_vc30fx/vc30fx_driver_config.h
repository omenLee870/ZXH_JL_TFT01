/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2022-09-30 09:50:56
 * @LastEditTime: 2024-03-26 17:51:50
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#ifndef __VC30Fx_DRIVER_CONFIG_H__
#define __VC30Fx_DRIVER_CONFIG_H__

/* wear default status select */
#define VC30Fx_WEAR_DFT_NONE (0)
#define VC30Fx_WEAR_DFT_DROP (1)
#define VC30Fx_WEAR_DFT_HOLD (2)
/* function enable select */
#define VC30Fx_FUNC_DISABLE (0)
#define VC30Fx_FUNC_ENABLE (1)

/* HARDWARE IRQ PIN CONFIGURATION */
#define VC30Fx_CFG_IRQPIN_PULSE_TIM (VC30Fx_IO_PULSE_TIM_400US)	/* vc30fx_core_com.h@VC30Fx_IO_PULSE_TIM */
#define VC30Fx_CFG_IRQPIN_PUSHPULL_3V3UP   (0) 	/* 3.3v,push-pull, up-edge */
#define VC30Fx_CFG_IRQPIN_PUSHPULL_3V3DOWN (1) 	/* 3.3v,push-pull, down-edge */
#define VC30Fx_CFG_IRQPIN_OPENDRAIN_1V8DOWN (2)	/* 1.8v,open-drain, down-edge */
#define VC30Fx_CFG_IRQPIN_MODE ( VC30Fx_CFG_IRQPIN_PUSHPULL_3V3UP )

/* set ps rate=fifo rate, while the dev is vc30fc */
#define VC30Fx_VC30FC_AUTO_UNDERCLOCKING (VC30Fx_FUNC_DISABLE) //默认不开启，30fc-12s使用ps-fifo同频

#define VC30Fx_FREQUENCY_MAX 20000
#define VC30Fx_FREQUENCY_MIN 19900
//#define VC30Fx_CFG_MCU_TICK_FREQUENCY 32000 // counter 32K
extern unsigned short int VC30Fx_CFG_MCU_TICK_FREQUENCY; /* extern unsigned short int VC30Fx_CFG_MCU_TICK_FREQUENCY; */
#define VC30Fx_CFG_MCU_TICK_MAX 0xffffff    // 24-bit counters
#define VC30Fx_CFG_CLK_CALC_DEVIATION 3
#define VC30Fx_CFG_CLK_CALC_GAPTIME 10 // interrupt gap times( 25hz_fifo_irq:20*40~0.4sec; 5hz_fifo_irq: 20*200 ~ 2sec; 1.25hz_fifo_irq: 10*800 ~ 8sec )

#define VC30Fx_CFG_CLK_CLAC_IRQGAP_EN (VC30Fx_FUNC_DISABLE)
/* interrupt run check timegap,dont write "(3/4)" !!!  */
/* 25hz(40ms~30ms), 50hz(20ms~15ms), 100hz(10ms~7ms) */
#define VC30Fx_IRQ_GAP_RATIO 3/4
/*  The event is triggered by a timer */
#define VC30Fx_CFG_IRQ_BY_TIMER_EN (VC30Fx_FUNC_DISABLE)

#define VC30Fx_CFG_DEFAULT_PPGGAIN_EN (1) /* 1-最小使用2倍增益, 0-最小使用单倍增益 */
/****************************************************************************
 * Configuration wear parameters
 ***************************************************************************/
#define VC30Fx_CFG_WEAR_DETECTION (VC30Fx_FUNC_ENABLE)
#define VC30Fx_CFG_WEAR_DEFAULT_STA (VC30Fx_WEAR_DFT_HOLD)
#define VC30Fx_CFG_WEAR_BIO_EN (VC30Fx_FUNC_ENABLE)
#define VC30Fx_CFG_WEAR_ALG_EN (VC30Fx_FUNC_DISABLE)
#define VC30Fx_CFG_WEAR_ACC_EN (VC30Fx_FUNC_DISABLE)
#define VC30Fx_CFG_WEAR_HOLD_CNT (1)
#define VC30Fx_CFG_WEAR_DROP_CNT (3)
#define VC30Fx_CFG_WEAR_BIOALGO_EN (VC30Fx_FUNC_ENABLE/* VC30Fx_FUNC_DISABLE */) /* enable-30FS调用算法的活体识别方式,disable-驱动的绝对阈值（需要标定） */
/* sensor serial default wear param */
#define VC30Fx_WEAR_PARAM_ENVMIN 0xff
#define VC30Fx_WEAR_PARAM_ENVMAX 0xff
#define VC30Fx_WEAR_PARAM_PSTH 0xB400     /* 16bit:0xB400~46080 , 8bit:0xB4~180 */
#define VC30Fx_WEAR_PARAM_PS_DELTA 0x3000 /* 16bit:0x3000~12288 , 8bit:0x30~48 */
#define VC30Fx_WEAR_PARAM_BIO_DELTA 20   /* bio0/1,建议取测试变化量的45%作为参数设置 */
//unsigned short VC30Fx_WEAR_PARAM_PS_DELTA = 0x5000;
//unsigned short VC30Fx_WEAR_PARAM_BIO_DELTA = 200;

//#define VC30Fx_WEAR_PARAM_BIO_ABS_HOLD 200 /* 80% */
//#define VC30Fx_WEAR_PARAM_BIO_ABS_DROP 160 /* 50% */
extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_HOLD; /* extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_HOLD;  */
extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_DROP; /* extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_DROP;  */

#define VC30Fx_WEAR_PARAM_PS_CURRENT 0x3F
#define VC30Fx_WEAR_PARAM_PS_RESISTANCE 13
#define VC30Fx_WEAR_PARAM_PS_GAIN 1

#if VC30Fx_CFG_DEFAULT_PPGGAIN_EN
#define VC30Fx_PPG_CURT       0x40
#define VC30Fx_PPG_RESISTANCE 9 /* 9 ~ 4.48M */
#define VC30Fx_PPG_GAIN       1
/* Set the maximum current parameter in FAT1/FAT2 work_mode */
#define VC30Fx_FAT_GREEN_MAX_CURT 0x7f /* 0-0x7F */
#define VC30Fx_FAT_RED_MAX_CURT   0x7f /* 0-0x7F */
#define VC30Fx_FAT_IR_MAX_CURT    0x7f /* 0-0x7F */
#define VC30Fx_FAT_RESISTANCE     11   /* 11~8.96M */
#define VC30Fx_FAT_GAIN           1
#else
#define VC30Fx_PPG_CURT       0x40
#define VC30Fx_PPG_RESISTANCE 11 /* 11 ~ 4.48M */
#define VC30Fx_PPG_GAIN       0
/* Set the maximum current parameter in FAT1/FAT2 work_mode */
#define VC30Fx_FAT_GREEN_MAX_CURT 0x7f /* 0-0x7F */
#define VC30Fx_FAT_RED_MAX_CURT   0x7f /* 0-0x7F */
#define VC30Fx_FAT_IR_MAX_CURT    0x7f /* 0-0x7F */
#define VC30Fx_FAT_RESISTANCE     13   /*13~8.96M */
#define VC30Fx_FAT_GAIN           0
#endif

/****************************************************************************
 * PPG AUTO ADJUST PARAMETERS
 ***************************************************************************/
#define VC30Fx_PPG_MIN_DEFT_THVAL (0x6)
#define VC30Fx_PPG_MAX_DEFT_THVAL (0xA) /*0xA~0xB*/
#define VC30Fx_PPG_SPO2_MIN_DEFT_THVAL (0x09)
#define VC30Fx_PPG_SPO2_MAX_DEFT_THVAL (0x0A) /* 0x0A~0x0B */
#define VC30Fx_ADJ_I_INCREASE 22 // 1.4 << 4 = 22.4//1.4f
#define VC30Fx_ADJ_I_DECREASE 11 // 0.7 << 4 = 11.2//0.7f
#define VC30Fx_ADJ_I_STEP_MAX 8
#define VC30Fx_ADJ_I_STEP_MIN 1
#define VC30Fx_ADJ_I_STEP_DFT 4 // default step

/* fix 8bit pre_ppg */
#define VC30Fx_PPG_CONV_BIT4(data) (data >> (4))

#endif /* __VC30Fx_DRIVER_CONFIG_H__ */
