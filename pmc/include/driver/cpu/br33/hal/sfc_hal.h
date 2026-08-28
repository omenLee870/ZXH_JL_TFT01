#ifndef __SFC_HAL_H__
#define __SFC_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 USER_CMD;
    __RW __u32 USER_CRM;
    __RW __u32 OFFSET_ADR;
    __RW __u32 CLK_CON;
    __RW __u32 DTR_CON;
    __WO __u32 DYC0_WR;
    __RO __u32 DYC0_RD;
    __RW __u32 DYC1;
} JL_SFC_TypeDef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 MASK;
    __WO __u32 KEY_ADDR;
    __WO __u32 KEY_DATL;
    __WO __u32 KEY_DATH;
    __RW __u32 NDEC_SADDR;
    __RW __u32 NDEC_EADDR;
} JL_SFCDEC_TypeDef;

typedef struct {
    __RW __u32 SADDR;
    __RW __u32 EADDR;
    __RW __u32 OFFSET_ADR;
} JL_MMU_SEC;

typedef struct  {
    __RW __u32 CON;
    JL_MMU_SEC MMU[2];
} JL_BOOTMMU_TypeDef;

typedef struct {
    __RW __u32 CON0;
    __WO __u32 CON1;
    __RW __u32 CON2;
    __RO __u32 ERR_ADDR;
} JL_SFCA2S_TypeDef;

#endif


