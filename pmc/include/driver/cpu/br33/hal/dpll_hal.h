
#ifndef __DPLL_HAL_H__
#define __DPLL_HAL_H__

typedef struct {
    __RW __u32 DPLL_CON     ;
    __RW __u32 DPLL_PREDIV0 ;
    __RW __u32 DPLL_POSTDIV0;
    __RW __u32 DPLL_PREDIV1 ;
    __RW __u32 DPLL_POSTDIV1;
} JL_DPLL_TypeDef;

#endif


