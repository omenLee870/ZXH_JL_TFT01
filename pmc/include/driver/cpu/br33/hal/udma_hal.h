
#ifndef __UDMA_HAL_H__
#define __UDMA_HAL_H__

typedef struct {
    __RW __u32 SADR;
    __RW __u32 DADR;
    __RW __u32 CON0;
    __RW __u32 CON1;
} JL_UDMA_CH;

typedef struct {
    __RW __u32 CON0;
    __RW __u32 SEL0;
    __RW __u32 SEL1;
    __RO __u32 RESERVED[1];
} JL_DMAGEN_CH;

typedef struct {
    JL_UDMA_CH   JL_UDMA_CH0;
    JL_DMAGEN_CH JL_DMAGEN_CH0;
    JL_UDMA_CH   JL_UDMA_CH1;
    JL_DMAGEN_CH JL_DMAGEN_CH1;
    JL_UDMA_CH   JL_UDMA_CH2;
    JL_DMAGEN_CH JL_DMAGEN_CH2;
    JL_UDMA_CH   JL_UDMA_CH3;
    JL_DMAGEN_CH JL_DMAGEN_CH3;
    JL_UDMA_CH   JL_UDMA_CH4;
    JL_DMAGEN_CH JL_DMAGEN_CH4;
    JL_UDMA_CH   JL_UDMA_CH5;
    JL_DMAGEN_CH JL_DMAGEN_CH5;
    JL_UDMA_CH   JL_UDMA_CH6;
    JL_DMAGEN_CH JL_DMAGEN_CH6;
    JL_UDMA_CH   JL_UDMA_CH7;
    JL_DMAGEN_CH JL_DMAGEN_CH7;
} JL_UDMA_TypeDef;

#endif


