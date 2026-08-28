#ifndef __ADV_TIMER_HAL_H__
#define __ADV_TIMER_HAL_H__

typedef struct {
    __RW __u32 CON0;
} JL_TCRM_TypeDef;

typedef struct {
    __RW __u32  CR1;
    __RW __u32  CR2;
    __RW __u32  SMCR;
    __RW __u32  DIER;
    __RW __u32  SR;
    __WO __u32  EGR;
    __RW __u32  CCMR1;
    __RW __u32  CCMR2;
    __RW __u32  CCMR3;
    __RW __u32  CCER;
    __RW __u32  CNT;
    __RW __u32  PSC;
    __RW __u32  ARR;
    __RW __u32  RCR;
    __RW __u32  CCR1;
    __RW __u32  CCR2;
    __RW __u32  CCR3;
    __RW __u32  CCR4;
    __RW __u32  CCR5;
    __RW __u32  CCR6;
    __RW __u32  BDTR;
    __RW __u32  DCR;
    __RW __u32  DMAR;
    __RW __u32  AF1;
    __RW __u32  AF2;
} JL_ADV_TIMER_TypeDef;

#endif

