
#ifndef __ADC_HAL_H__
#define __ADC_HAL_H__

typedef struct {
    __RW __u32 ADC_CON0;
    __RW __u32 ADC_CON1;
    __RW __u32 ADC_CON2;
    __RW __u32 ADC_CON3;
    __RW __u32 ADC_CHLS0;
    __RW __u32 ADC_CHLS1;
    __RW __u32 ADC_CHLS2;
    __RW __u32 ADC_CHLS3;
    __RW __u32 ADC_SPTS0;
    __RW __u32 ADC_SPTS1;
    __RW __u32 DMA_BADR;
    __RW __u32 DMA_BUFLEN;
    __RW __u32 DMA_WPTR;
    __RO __u32 ADC_RDATA;
    __RW __u32 WDG_TH;
    __RW __u32 WDG_CON;
    __RW __u32 ANA_CON0;
    __RW __u32 ANA_CON1;
    __RW __u32 ANA_CON2;
    __RW __u32 ANA_CON3;
    __RW __u32 ANA_CON4;
    __RO __u32 ANA_CON5;
    __RO __u32 ANA_CON6;
} JL_ADC_TypeDef;
#endif



