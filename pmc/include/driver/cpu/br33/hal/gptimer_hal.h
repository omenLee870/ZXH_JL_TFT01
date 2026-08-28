
#ifndef __GPTIMER_HAL_H__
#define __GPTIMER_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 CNT;
    __RW __u32 PRD;
    __RW __u32 PWM;
    __RW __u32 IRFLT;
} JL_GPTIMER_TypeDef;

#endif

