#ifndef  __GPTIMER_HW_H__
#define  __GPTIMER_HW_H__

//BR35
#include "cpu.h"
#include "gpio.h"
#include "gptimer_hw_v5.h"

typedef JL_GPTIMER_TypeDef    GPTIMER;
typedef JL_GPTIMER_TypeDef    JL_TIMER_TypeDef;
#define GPTIMER0    JL_TIMER0
#define GPTIMER1    JL_TIMER1
#define GPTIMER2    JL_TIMER2
#define GPTIMER3    JL_TIMER3
#define GPTIMER4    JL_TIMER4
#define GPTIMER5    JL_TIMER5
#define TIMER_MAX_NUM   6
#define TIMER_BASE_ADDR GPTIMER0
#define TIMER_OFFSET    (GPTIMER1 - GPTIMER0)

#define TIMER_CNT_SIZE  0xFFFFFFFF
#define TIMER_PRD_SIZE  0xFFFFFFFF
#define TIMER_PWM_SIZE  0xFFFFFFFF

#define IRQ_TIMEx_IDX_LIST  IRQ_TIME0_IDX, \
                            IRQ_TIME1_IDX, \
                            IRQ_TIME2_IDX, \
                            IRQ_TIME3_IDX, \
                            IRQ_TIME4_IDX, \
                            IRQ_TIME5_IDX, \

#define IRFLT_SRC_TABLE_LIST    \
    0,  /* lsb_clk */ \
    0,  /* lsb_clk */ \
    12, /* std_12m */ \
    24, /* std_24m */ \

typedef enum gptimerx : u8 {
    TIMER0 = 0,
    TIMER1,
    TIMER2,
    TIMER3,
    TIMER4,
    TIMER5,
    TIMERx, //传入此参数时,内部自动分配一个空闲TIMER
} timer_dev;

#define GPTIMER_EXTERN_USE  0b100000 //其他模块占用的timer, 不参与自动分配


#define GPTMR_SRC_RC16M        1
#define GPTMR_SRC_STRC32k      2
#define GPTMR_SRC_RC250K       3
#define GPTMR_SRC_PATCLK       4
#define GPTMR_SRC_HSB          5
#define GPTMR_SRC_LSB          6
#define GPTMR_SRC_STD12M       7
#define GPTMR_SRC_STD24M       8
#define GPTMR_SRC_STD48M       9
#define GPTMR_SRC_CLKOUT0     10
#define GPTMR_SRC_FLL_24M     11
#define GPTMR_SRC_OSC_24M     12
#define GPTMR_SRC_OSC_32K     14
#define GPTMR_SRC_WATCLK      13
#define GPTMR_SRC_TMR_CIN     15



#define GPTIMER_PND_CLR         (0b1<<14)

#define GPTIMER_CLK_SRC_RC16M       (GPTMR_SRC_RC16M   <<10)
#define GPTIMER_CLK_SRC_STRC32K     (GPTMR_SRC_STRC32k <<10)
#define GPTIMER_CLK_SRC_RC250K      (GPTMR_SRC_RC250K  <<10)
#define GPTIMER_CLK_SRC_PATCLK      (GPTMR_SRC_PATCLK  <<10)
#define GPTIMER_CLK_SRC_HSB         (GPTMR_SRC_HSB     <<10)
#define GPTIMER_CLK_SRC_LSB         (GPTMR_SRC_LSB     <<10)
#define GPTIMER_CLK_SRC_STD12M      (GPTMR_SRC_STD12M  <<10)
#define GPTIMER_CLK_SRC_STD24M      (GPTMR_SRC_STD24M  <<10)
#define GPTIMER_CLK_SRC_STD48M      (GPTMR_SRC_STD48M  <<10)
#define GPTIMER_CLK_SRC_CLKOUT0     (GPTMR_SRC_CLKOUT0 <<10)
#define GPTIMER_CLK_SRC_FLL_24M     (GPTMR_SRC_FLL_24M <<10)
#define GPTIMER_CLK_SRC_LRC         (GPTMR_SRC_FLL_24M <<10)//no lrc
#define GPTIMER_CLK_SRC_OSC_24M     (GPTMR_SRC_OSC_24M <<10)
#define GPTIMER_CLK_SRC_OSC_32K     (GPTMR_SRC_OSC_32K <<10)
#define GPTIMER_CLK_SRC_WATCLK      (GPTMR_SRC_WATCLK  <<10)
#define GPTIMER_CLK_SRC_TMR_CIN     (GPTMR_SRC_TMR_CIN <<10)

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


//以下宏定义给系统 timer 驱动使用
#define GPTIMER_TIMER_MODE      (0b1<<0)

// //延时
// #define GPTIMER_DLY             GPTIMER2
// #define GPTIMER_DLY_IRQ_INDEX   IRQ_TIME2_IDX

//sys_hi_timer
#define GPTIMER_SYS             GPTIMER5
#define GPTIMER_SYS_IRQ_INDEX   IRQ_TIME5_IDX
#define GPTIMER_SYS_START()     GPTIMER_SYS->CON |= GPTIMER_TIMER_MODE
#define GPTIMER_SYS_PAUSE()     GPTIMER_SYS->CON &= ~GPTIMER_TIMER_MODE
#define GPTIMER_SYS_EN_CHECK()  GPTIMER_SYS->CON & GPTIMER_TIMER_MODE
#define GPTIMER_SYS_CLR_PND()   GPTIMER_SYS->CON |= GPTIMER_PND_CLR

#define GPTIMER_SYS_INIT()      do{GPTIMER_SYS->CON = GPTIMER_PND_CLR|GPTIMER_CLK_SRC_STD12M|GPTIMER_CLK_DIV_4; \
                                GPTIMER_SYS->PRD = 0; \
                                GPTIMER_SYS->CNT = 0;}while(0) \

#define GPTIMER_SYS_CFG(src, div)      do{GPTIMER_SYS->CON = GPTIMER_PND_CLR|(src)|(div); \
                                GPTIMER_SYS->PRD = 0; \
                                GPTIMER_SYS->CNT = 0;}while(0) \

#define GPTIMER_SYS_GET_CNT     GPTIMER_SYS->CNT
#define GPTIMER_SYS_SET_CNT(x)  GPTIMER_SYS->CNT = (x)
#define GPTIMER_SYS_GET_PRD     GPTIMER_SYS->PRD
#define GPTIMER_SYS_SET_PRD(x)  GPTIMER_SYS->PRD = (x)-1
#define GPTIMER_SYS_CLK_SRC     12 //MHz单位
#define GPTIMER_SYS_CLK_DIV     2 // 右移两位 等效于 除以4

#endif

