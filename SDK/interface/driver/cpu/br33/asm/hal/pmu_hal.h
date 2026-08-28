
#ifndef __PMU_HAL_H__
#define __PMU_HAL_H__
typedef struct {
    __RW __u32 PMU_CON;
    __RW __u32 PMU_STA;
    __RW __u32 WKUP_SRC;
    __RW __u32 WKUP_EN;
    __RO __u32 INT_SRC;
    __RW __u32 DCV_VOL_CON0;
    __RW __u32 P2M_KEY;
} JL_PMU_TypeDef;

#endif


