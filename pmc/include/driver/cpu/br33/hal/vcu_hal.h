
#ifndef __VCU_HAL_H__
#define __VCU_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 EMU_CON;
    __RW __u32 EMU_MSG;
    __RW __u32 EMU_ID;
    __RW __u32 CMD_CON;
    __RW __u32 CMD_BEG;
    __RW __u32 CMD_END;
    __RW __u32 CNT_RACK;
    __RW __u32 CNT_RNAK;
} JL_VCU_TypeDef;

#endif

