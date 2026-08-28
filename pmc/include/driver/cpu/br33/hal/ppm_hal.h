
#ifndef __PPM_HAL_H__
#define __PPM_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 STRAN_DAT;
    __RW __u32 STRAN_SET;
    __RW __u32 DMA_ADR;
    __RW __u32 PPM_RD;
} JL_PPM_TypeDef;

#endif

