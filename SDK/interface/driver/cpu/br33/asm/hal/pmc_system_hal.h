
#ifndef __PMC_SYSTEM_HAL_H__
#define __PMC_SYSTEM_HAL_H__

typedef struct {
    __RW __u32 P2M_INT_IE;
    __RW __u32 P2M_INT_SET;
    __RW __u32 P2M_INT_CLR;
    __RO __u32 P2M_INT_PND;
    __RW __u32 P2M_CLK_CON0;
    __RW __u32 M2P_INT_IE;
    __RW __u32 M2P_INT_SET;
    __RW __u32 M2P_INT_CLR;
    __RO __u32 M2P_INT_PND;
    __RW __u32 PMC_SYS_CON0;
    __RW __u32 PMC_SYS_CON1;
    __RW __u32 P2P_INT_CON0;
    __RW __u32 PMC_PRP_CON0;
    __RW __u32 PMC_PRP_CON1;
    __RW __u32 PMC_PRP_CON2;
    __RW __u32 P33_MSYS_CON0;
    __RW __u32 DBG_CON0;
    __RW __u32 PMU_KEY;
} PMC_SYSTEM_TypeDef;

#endif

