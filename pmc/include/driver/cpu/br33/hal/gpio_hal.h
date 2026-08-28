
#ifndef __GPIO_HAL_H__
#define __GPIO_HAL_H__

typedef struct {
    __RO __u32 IN;
    __RW __u32 OUT;
    __RW __u32 DIR;
    __RW __u32 DIE;
    __RW __u32 DIEH;
    __RW __u32 PU;
    __RW __u32 PD;
    __RW __u32 HD;
    __RW __u32 SPL;
    __RW __u32 CON;  // usb phy only/hvt io only
    __RW __u32 CON1; // dpddr io only
    __RW __u32 SEL;  // pmc io only
    __RW __u32 OUT_BSR;
    __RW __u32 DIR_BSR;
    __RW __u32 DIE_BSR;
    __RW __u32 DIEH_BSR;
    __RW __u32 PUL_BSR;
    __RW __u32 PUH_BSR;
    __RW __u32 PDL_BSR;
    __RW __u32 PDH_BSR;
    __RW __u32 HDL_BSR;
    __RW __u32 HDH_BSR;
    __RW __u32 SPL_BSR;
    __RW __u32 CON_BSR;   // usb phy only/hvt io only
    __RW __u32 CON1_BSR;  // dpddr io only
    __RW __u32 SEL_BSR;   // pmc io only
} JL_PORT_TypeDef;

#endif


