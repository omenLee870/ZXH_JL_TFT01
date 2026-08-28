
#ifndef __XPLL_HAL_H__
#define __XPLL_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 NR;
    __RW __u32 DSM;
    __RW __u32 INTF0;
    __RW __u32 INTF1;
    __RW __u32 SSC;
    __RW __u32 DMAX;
    __RW __u32 DMIN;
    __RW __u32 DSTP;
    __RW __u32 XSPI;
    __RW __u32 DLLCON;
    __RW __u32 FFCON;
    __RW __u32 PCON;
    __RW __u32 ALDOCON;
} JL_XPLL_TypeDef;

#endif


