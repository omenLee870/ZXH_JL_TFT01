
#ifndef __PMC_LPTMR_HAL_H__
#define __PMC_LPTMR_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 PRD;
    __RW __u32 RSC;
    __RO __u32 CNT;
} PMC_LPTMR_TypeDef;


#endif

