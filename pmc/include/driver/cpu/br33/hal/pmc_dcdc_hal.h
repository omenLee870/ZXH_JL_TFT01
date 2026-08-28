
#ifndef __PMC_DCDC_HAL_H__
#define __PMC_DCDC_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 PRD0;
    __RW __u32 INI0;
    __RW __u32 PRD1;
    __RW __u32 INI1;
    __RW __u32 PRD2;
    __RW __u32 INI2;
} PMC_DCDC_TypeDef;

#endif


