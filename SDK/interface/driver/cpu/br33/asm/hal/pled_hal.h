
#ifndef __PLED_HAL_H__
#define __PLED_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 BRI_PRD;
    __RW __u32 BRI_DUTY0;
    __RW __u32 BRI_DUTY1;
    __RW __u32 PRD_DIV;
    __RW __u32 DUTY0;
    __RW __u32 DUTY1;
    __RW __u32 DUTY2;
    __RW __u32 DUTY3;
    __RW __u32 CNT_CON;
    __RW __u32 CNT_DEC;
    __RO __u32 CNT_DATA;
    __WO __u32 CNT_SYNC;
    __RW __u32 SCON0;
    __RW __u32 SCON1;
    __RW __u32 OUT1_DIV;
    __RW __u32 OUT0_DIV;
    __RW __u32 ANA_CON0;
} JL_PLED_TypeDef;

#endif


