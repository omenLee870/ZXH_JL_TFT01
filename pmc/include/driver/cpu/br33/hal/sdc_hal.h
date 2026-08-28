
#ifndef __SDC_HAL_H__
#define __SDC_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __WO __u32 CPTR;
    __WO __u32 DPTR;
    __RW __u32 CTU_CON;
    __WO __u32 CTU_CNT;
    __RW __u32 BRK;
} JL_SDC_TypeDef, JL_SD_TypeDef;

#endif


