
#ifndef __LCDC_HAL_H__
#define __LCDC_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 SEG_IO_EN0;
    __RW __u32 SEG_IO_EN1;
    __RW __u32 SEG0_DAT;
    __RW __u32 SEG1_DAT;
    __RW __u32 SEG2_DAT;
    __RW __u32 SEG3_DAT;
    __RW __u32 SEG4_DAT;
    __RW __u32 SEG5_DAT;
} JL_LCDC_TypeDef;

#endif

