
#ifndef __SPI_HAL_H__
#define __SPI_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 BAUD;
    __RW __u32 BUF;
    __WO __u32 ADR;
    __RW __u32 CNT;
    __RW __u32 CON1;
    __RW __u8  ENCCON ;
    __WO __u16 ENCKEY ;
    __WO __u16 ENCADR ;
    __RW __u32 REG;
    __WO __u32 FIFO;
} JL_SPI_TypeDef;

#endif

