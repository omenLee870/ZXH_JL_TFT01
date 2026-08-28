
#ifndef __SPDIF_HAL_H__
#define __SPDIF_HAL_H__

typedef struct {
    __RW __u32(SM_CLK);
    __RW __u32(SM_CON);
    __RW __u32(SM_ABADR);
    __RW __u32(SM_ABUFLEN);
    __RW __u32(SM_AHPTR);
    __RW __u32(SM_ASPTR);
    __RW __u32(SM_ASHN);
    __RW __u32(SM_APNS);
    __RW __u32(SM_IBADR);
    __RW __u32(SM_IBUFLEN);
    __RW __u32(SM_IHPTR);
    __RW __u32(SS_CON);
    __RO __u32(SS_SR_CNT);
    __RW __u32(SS_IO_CON);
    __RW __u32(SS_DMA_CON);
    __RW __u32(SS_DMA_LEN);
    __WO __u32(SS_DAT_ADR);
    __WO __u32(SS_INF_ADR);
    __RO __u32(SS_CSB0);
    __RO __u32(SS_CSB1);
    __RO __u32(SS_CSB2);
    __RO __u32(SS_CSB3);
    __RO __u32(SS_CSB4);
    __RO __u32(SS_CSB5);
    __RW __u32(SS_ANA_CON);
} JL_SPDIF_TypeDef;

#endif


