
#ifndef __PPS_HAL_H__
#define __PPS_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 CNT;
    __RW __u32 CFG;
    __RW __u32 VTRIM;
    __RW __u32 TX_BASE;
    __RW __u32 RX_BASE;
} JL_PPS_TypeDef;

#endif

