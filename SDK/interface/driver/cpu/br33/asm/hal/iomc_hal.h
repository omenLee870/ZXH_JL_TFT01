
#ifndef __IOMC_HAL_H__
#define __IOMC_HAL_H__

typedef struct {
    __RW __u32 FSPG0_CON;
    __RW __u32 FSPG1_CON;
    __RW __u32 SPI_IOMC0;
    __RW __u32 SDC_IOMC0;
    __WO __u32 SYS_IOMC0;
    __RW __u32 SYS_IOMC1;
    __WO __u32 ASS_IOMC0;
    __WO __u32 WL_IOMC0;
    __RW __u32 ICH_IOMC0;
    __RW __u32 ICH_IOMC1;
    __RW __u32 ICH_IOMC2;
    __RW __u32 ICH_IOMC3;
    __RW __u32 ICH_IOMC4;
    __WO __u32 ICH_IOMC5;
    __WO __u32 ICH_IOMC6;
    __RW __u32 OCH_IOMC0;
    __RW __u32 OCH_IOMC1;
    __RW __u32 OCH_IOMC2;
    __WO __u32 OCH_IOMC3;
    __WO __u32 LCDPG_CON;
    __WO __u32 MTPG_CON;
    __RW __u32 SDPG_CON;
    __RW __u32 WAT_CON;
    __RW __u32 VPWRIO_CON;
    __RW __u32 VDO_IOMC;
} JL_IOMC_TypeDef;

typedef struct {
    __RW __u32 FSPG_CON;
    __RW __u32 IOMC0;
    __RW __u32 IOMC1;
} JL_SFC_IOMC_TypeDef;

#endif


