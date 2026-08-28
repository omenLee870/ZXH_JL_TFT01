
#ifndef __SYSPLL_HAL_H__
#define __SYSPLL_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 NR;
//  __RW __u32 CON3;
//  __RW __u32 CON4;
//  __RW __u32 CON9;
//  __RW __u32 CON10;
//  __RW __u32 CON11;
    __RW __u32 DSM;
    __RW __u32 INTF0;
    __RW __u32 INTF1;
    __RW __u32 SSC;
    __RW __u32 DMIN;
    __RW __u32 DMAX;
    __RW __u32 DSTP;
    __RW __u32 TRIM_CON0;
    __RW __u32 TRIM_CON1;
    __RW __u32 TRIM_PND;
    __RW __u32 FRQ_CNT;
    __RW __u32 FRC_SCA;
} JL_SYSPLL_TypeDef;

#endif


