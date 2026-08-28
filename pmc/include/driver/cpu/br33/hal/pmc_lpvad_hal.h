
#ifndef __PMC_WDT_HAL_H__
#define __PMC_WDT_HAL_H__

typedef struct {
    __RW __u32 VAD_CON;
    __RW __u32 VAD_ACON0;
    __RW __u32 VAD_ACON1;
    __RW __u32 VAD_ACON2;
    __RW __u32 DVAD_CON0;
    __RW __u32 DVAD_CON1;
    __RW __u32 DVAD_CON2;
    __RW __u32 DVAD_CON3;
    __RW __u32 DMA_BADR;
    __RW __u32 DMA_LEN;
    __RW __u32 DMA_HPTR;
    __RW __u32 DMA_SPTR;
    __RW __u32 DMA_SPN;
    __RW __u32 DMA_SHN;
    __RW __u32 DIT_CON;
} PMC_LPVAD_TypeDef;


#endif

