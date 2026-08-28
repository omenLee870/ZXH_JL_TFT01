
#ifndef __CAN_HAL_H__
#define __CAN_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CMD0;
    __RW __u32 STATUS;
    __RW __u32 PND;
    __RW __u32 ACF;
    __RW __u32 AMF;
    __RW __u32 BTIME0;
    __RW __u32 BTIME1;
    __RW __u32 RESERVED0;
    __RW __u32 RESERVED1;
    __RW __u32 TIDH;
    __RW __u32 TIDL;
    __RW __u32 TXB1;
    __RW __u32 TXB2;
    __RW __u32 TXB3;
    __RW __u32 TXB4;
    __RW __u32 TXB5;
    __RW __u32 TXB6;
    __RW __u32 TXB7;
    __RW __u32 TXB8;
    __RW __u32 RIDH;
    __RW __u32 RIDL;
    __RW __u32 RXB1;
    __RW __u32 RXB2;
    __RW __u32 RXB3;
    __RW __u32 RXB4;
    __RW __u32 RXB5;
    __RW __u32 RXB6;
    __RW __u32 RXB7;
    __RW __u32 RXB8;
    __RW __u32 BAUDH;
    __RW __u32 CLKDIV;
} JL_CAN_TypeDef;

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CMD0;
    __RW __u32 STATUS;
    __RW __u32 PND;
    __RW __u32 IE;
    __RW __u32 RESERVED0;
    __RW __u32 BTIME0;
    __RW __u32 BTIME1;
    __RW __u32 RESERVED1;
    __RW __u32 RESERVED2;
    __RW __u32 RESERVED3;
    __RW __u32 ACL;
    __RW __u32 ECC;
    __RW __u32 EWLR;
    __RW __u32 RXERR;
    __RW __u32 TXERR;
    __RW __u32 FF;
    __RW __u32 BUF0;
    __RW __u32 BUF1;
    __RW __u32 BUF2;
    __RW __u32 BUF3;
    __RW __u32 BUF4;
    __RW __u32 BUF5;
    __RW __u32 BUF6;
    __RW __u32 BUF7;
    __RW __u32 BUF8;
    __RW __u32 BUF9;
    __RW __u32 BUF10;
    __RW __u32 BUF11;
    __RW __u32 RMC;
    __RW __u32 BAUDH;
    __RW __u32 CLKDIV;
} JL_ECAN_TypeDef;

typedef struct {
    /* 0   */    __RW __u32 CON;
    /* 1   */    __RW __u32 ADR;
    /* 2   */    __RW __u32 RXCNT;
    /* 3   */    __RO __u32 HRCNT;
} JL_CANDMA_TypeDef;

#endif


