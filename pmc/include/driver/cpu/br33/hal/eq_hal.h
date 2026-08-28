
#ifndef __EQ_HAL_H__
#define __EQ_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 DATAI_ADR;
    __RW __u32 DATAO_ADR;
    __RW __u32 DATA_LEN;
    __RW __u32 FLT_ADR;
    __RW __u32 BUF_MCTL;
    __RW __u32 BUF_MDAT;
} JL_EQ_TypeDef;

#endif


