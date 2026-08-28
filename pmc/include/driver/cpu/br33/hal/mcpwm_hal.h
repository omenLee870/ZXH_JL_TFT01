
#ifndef __MCPWM_HAL_H__
#define __MCPWM_HAL_H__

typedef struct {
    __RW __u32 TMR0_CON;
    __RW __u32 TMR0_CNT;
    __RW __u32 TMR0_PR;
    __RW __u32 TMR1_CON;
    __RW __u32 TMR1_CNT;
    __RW __u32 TMR1_PR;
    __RW __u32 TMR2_CON;
    __RW __u32 TMR2_CNT;
    __RW __u32 TMR2_PR;
    __RW __u32 TMR3_CON;
    __RW __u32 TMR3_CNT;
    __RW __u32 TMR3_PR;
    __RW __u32 TMR4_CON;
    __RW __u32 TMR4_CNT;
    __RW __u32 TMR4_PR;
    __RW __u32 TMR5_CON;
    __RW __u32 TMR5_CNT;
    __RW __u32 TMR5_PR;
    __RW __u32 TMR6_CON;
    __RW __u32 TMR6_CNT;
    __RW __u32 TMR6_PR;
    __RW __u32 TMR7_CON;
    __RW __u32 TMR7_CNT;
    __RW __u32 TMR7_PR;
    __RW __u32 FPIN_CON;
    __RW __u32 CH0_CON0;
    __RW __u32 CH0_CON1;
    __RW __u32 CH0_CMPH;
    __RW __u32 CH0_CMPL;
    __RW __u32 CH1_CON0;
    __RW __u32 CH1_CON1;
    __RW __u32 CH1_CMPH;
    __RW __u32 CH1_CMPL;
    __RW __u32 CH2_CON0;
    __RW __u32 CH2_CON1;
    __RW __u32 CH2_CMPH;
    __RW __u32 CH2_CMPL;
    __RW __u32 CH3_CON0;
    __RW __u32 CH3_CON1;
    __RW __u32 CH3_CMPH;
    __RW __u32 CH3_CMPL;
    __RW __u32 CH4_CON0;
    __RW __u32 CH4_CON1;
    __RW __u32 CH4_CMPH;
    __RW __u32 CH4_CMPL;
    __RW __u32 CH5_CON0;
    __RW __u32 CH5_CON1;
    __RW __u32 CH5_CMPH;
    __RW __u32 CH5_CMPL;
    __RW __u32 CH6_CON0;
    __RW __u32 CH6_CON1;
    __RW __u32 CH6_CMPH;
    __RW __u32 CH6_CMPL;
    __RW __u32 CH7_CON0;
    __RW __u32 CH7_CON1;
    __RW __u32 CH7_CMPH;
    __RW __u32 CH7_CMPL;
    __RW __u32 MCPWM_CON0;
} JL_MCPWM_TypeDef;

#endif

