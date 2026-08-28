
#ifndef __PMC_UDMA_HAL_H__
#define __PMC_UDMA_HAL_H__

#include "udma_hal.h"

typedef struct {
    JL_UDMA_CH   PMC_UDMA_CH0;
    JL_DMAGEN_CH PMC_DMAGEN_CH0;
    JL_UDMA_CH   PMC_UDMA_CH1;
    JL_DMAGEN_CH PMC_DMAGEN_CH1;
    JL_UDMA_CH   PMC_UDMA_CH2;
    JL_DMAGEN_CH PMC_DMAGEN_CH2;
    JL_UDMA_CH   PMC_UDMA_CH3;
    JL_DMAGEN_CH PMC_DMAGEN_CH3;
} PMC_UDMA_TypeDef;


#endif

