
#ifndef __LEDC_HAL_H__
#define __LEDC_HAL_H__

typedef struct {
    __RW __u32 CLK;
} JL_LEDCK_TypeDef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 FD;
    __RW __u32 LP;
    __RW __u32 TIX;
    __RW __u32 RSTX;
    __WO __u32 ADR;
} JL_LEDC_TypeDef;

#endif

