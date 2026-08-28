
#ifndef __PMC_SOARCFG_HAL_H__
#define __PMC_SOARCFG_HAL_H__

typedef struct {
    __RW __u32 SOAR_SEL[16];
    __RW __u32 SOAR_CEB[16];
    __RW __u32 SOAR_CLK[16];
} PMC_SOARCFG_TypeDef;


#endif

