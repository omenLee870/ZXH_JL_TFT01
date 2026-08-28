
#ifndef __PMC_WDT_HAL_H__
#define __PMC_WDT_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 KEY;
    __RW __u32 DUMMY;
} PMC_WDT_TypeDef;


#endif

