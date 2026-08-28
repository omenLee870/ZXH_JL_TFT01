
#ifndef __PMC_IOMC_HAL_H__
#define __PMC_IOMC_HAL_H__

typedef struct {
    __RW __u32 OCH_IOMC0;
    __RW __u32 OCH_IOMC1;
    __RW __u32 OCH_IOMC2;
    __RW __u32 ICH_IOMC0;
    __RW __u32 ICH_IOMC1;
    __RW __u32 FSPG_CON;
    __RW __u32 SDPG_CON;
    __RW __u32 SYS_IOMC;
    __RW __u32 VPWRIO_CON;
    __RO __u32 P33_PORT;
} JL_LP_IOMC_TypeDef;


#endif

