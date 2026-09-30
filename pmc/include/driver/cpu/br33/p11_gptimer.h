#ifndef  __GPTIMER_H__
#define  __GPTIMER_H__

#include "typedef.h"
#include "power/pmc/pmc_sfr.h"

enum gptimerx {
    GPTIMER0 = 0,
    GPTIMER1,
};
#define P11_TIMER_MAX_NUM   2

//以下宏定义给 p11_gptimer 驱动使用
#define GPTIMER_PND_CLR         (0b1<<14)

#define GPTIMER_CLK_SRC_RC250K  	(0b0000<<10)
#define GPTIMER_CLK_SRC_RC16M   	(0b0001<<10)
#define GPTIMER_CLK_SRC_BTOSC_24M   (0b0010<<10)
#define GPTIMER_CLK_SRC_LRC     	(0b0011<<10)
#define GPTIMER_CLK_SRC_RTC_OSC		(0b0100<<10)
#define GPTIMER_CLK_SRC_STD12M  	(0b0101<<10)
#define GPTIMER_CLK_SRC_PATCLK  	(0b0110<<10)
#define GPTIMER_CLK_SRC_TEST_CLK  	(0b0111<<10)
#define GPTIMER_CLK_SRC_LRC_24M  	(0b1000<<10)
#define GPTIMER_CLK_SRC_IO_MUXIN  	(0b1111<<10)

#define GPTIMER_CLK_DIV_1       (0b0000<<4)
#define GPTIMER_CLK_DIV_4       (0b0001<<4)
#define GPTIMER_CLK_DIV_16      (0b0010<<4)
#define GPTIMER_CLK_DIV_64      (0b0011<<4)
#define GPTIMER_CLK_DIV_2       (0b0100<<4)
#define GPTIMER_CLK_DIV_8       (0b0101<<4)
#define GPTIMER_CLK_DIV_32      (0b0110<<4)
#define GPTIMER_CLK_DIV_128     (0b0111<<4)
#define GPTIMER_CLK_DIV_256     (0b1000<<4)
#define GPTIMER_CLK_DIV_1024    (0b1001<<4)
#define GPTIMER_CLK_DIV_4096    (0b1010<<4)
#define GPTIMER_CLK_DIV_16384   (0b1011<<4)
#define GPTIMER_CLK_DIV_512     (0b1100<<4)
#define GPTIMER_CLK_DIV_2048    (0b1101<<4)
#define GPTIMER_CLK_DIV_8192    (0b1110<<4)
#define GPTIMER_CLK_DIV_32768   (0b1111<<4)

#define GPTIMER_PWM_OUT_EN      (0b1<<8)
#define GPTIMER_TIMER_MODE      (0b1<<0)

#define GPTIMERx(TIMERx)	        (PMC_GPTMR0 + TIMERx * (PMC_GPTMR1 - PMC_GPTMR0))
#define GPTIMER_IRQ_INDEX(TIMERx)   (IRQ_GP_TMR0_IDX + TIMERx)
#define GPTIMER_START(TIMERx)       (GPTIMERx(TIMERx)->CON |= GPTIMER_TIMER_MODE)
#define GPTIMER_PAUSE(TIMERx)       (GPTIMERx(TIMERx)->CON &= ~GPTIMER_TIMER_MODE)
#define GPTIMER_EN_CHECK(TIMERx)    (GPTIMERx(TIMERx)->CON & GPTIMER_TIMER_MODE)
#define GPTIMER_PWM_EN(TIMERx)      (GPTIMERx(TIMERx)->CON |= GPTIMER_PWM_OUT_EN)
#define GPTIMER_CLR_PND(TIMERx)     (GPTIMERx(TIMERx)->CON |= GPTIMER_PND_CLR)

#define GPTIMER_CLK_SRC(x)  (x) //MHz单位
#define GPTIMER_CLK_DIV     2 // 右移两位 等效于 除以4
#define GPTIMER_INIT(TIMERx, CLK_SRC)    do{ \
					                GPTIMERx(TIMERx)->CON = GPTIMER_PND_CLR|CLK_SRC|GPTIMER_CLK_DIV_4; \
								    GPTIMERx(TIMERx)->PRD = 0; \
									GPTIMERx(TIMERx)->CNT = 0; \
								    }while(0)


#define GPTIMER_SET_PERIOD_US(TIMERx, period_us)		(GPTIMERx(TIMERx)->PRD = ((period_us * GPTIMER_CLK_SRC(12)) >> GPTIMER_CLK_DIV) - 1)
#define GPTIMER_GET_CNT(TIMERx)		(GPTIMERx(TIMERx)->CNT)
#define GPTIMER_SET_CNT(TIMERx, x)  (GPTIMERx(TIMERx)->CNT = (x))
#define GPTIMER_GET_PRD(TIMERx)		(GPTIMERx(TIMERx)->PRD)
#define GPTIMER_SET_PRD(TIMERx, x)  (GPTIMERx(TIMERx)->PRD = (x) - 1)

#endif

