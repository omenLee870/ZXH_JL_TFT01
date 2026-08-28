
#ifndef __HACC_HAL_H__
#define __HACC_HAL_H__

typedef struct {
    __WO __u32 FIFO;
    __RW __u32 REG;
} JL_CRC_TypeDef;

typedef struct {
    __RO __u32 R64L;
    __RO __u32 R64H;
} JL_RAND_TypeDef;

typedef struct {
    __RW __u8  ENCCON ;
    __WO __u16 ENCKEY ;
    __WO __u16 ENCADR ;
} JL_PERIENC_TypeDef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 POL;
    __WO __u32 INIT;
    __RW __u32 REG;
} JL_GPCRC_TypeDef;

#endif

