
#ifndef __UART_HAL_H__
#define __UART_HAL_H__

typedef struct {
    __RW __u16 TX_CON0;
    __RW __u16 TX_CON1;
    __RW __u16 RX_CON0;
    __RW __u16 RX_CON1;
    __RW __u16 CON2;
    __RW __u16 BAUD;
    __RW __u8  BUF;
    __RW __u32 OTCNT;
    __RW __u32 TXADR;
    __WO __u16 TXCNT;
    __RW __u32 RXSADR;
    __RW __u32 RXEADR;
    __RW __u32 RXCNT;
    __RO __u16 HRXCNT;
    __RO __u16 RX_ERR_CNT;
} JL_UART_TypeDef;

typedef struct {
    __RW __u16 TX_CON0;
    __RW __u16 TX_CON1;
    __RW __u16 RX_CON0;
    __RW __u16 RX_CON1;
    __RW __u16 CON2;
    __RW __u16 BAUD;
    __RW __u8  BUF;
} JL_UART_LITE_TypeDef;

#endif


