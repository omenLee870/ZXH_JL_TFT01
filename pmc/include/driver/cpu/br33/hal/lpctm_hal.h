

#ifndef __LPCTM_HAL_H__
#define __LPCTM_HAL_H__

typedef struct {
    __RW __u32 CON0 ;
    __RW __u32 CHEN ;
    __RW __u32 CNUM ;
    __RW __u32 PPRD ;
    __RW __u32 DPRD ;
    __RW __u32 ECON ;
    __RW __u32 EXEN ;
    __RW __u32 CHIS ;
    __RW __u32 CLKC ;
    __WO __u32 WCON ;
    __RW __u32 ANA0 ;
    __RW __u32 ANA1 ;
    __RO __u32 RES  ;
    __RW __u32 DMA_START_ADR;
    __RW __u32 DMA_HALF_ADR;
    __RW __u32 DMA_END_ADR;
    __RW __u32 DMA_CON;
    __RW __u32 MSG_CON;
    __RO __u32 DMA_WADR;
    __RW __u32 SLEEP_CON;
    __RW __u32 PDEN;
} JL_LPCTM_TypeDef;


#endif



