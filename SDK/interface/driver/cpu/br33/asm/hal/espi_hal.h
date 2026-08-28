
#ifndef __ESPI_HAL_H__
#define __ESPI_HAL_H__

typedef struct {
    __RW __u32 KICK ;
    __RW __u32 CON0 ;
    __RW __u32 PCON ;
    __RW __u32 FCON ;
    __RW __u32 DCON0;
    __RW __u32 DCON1;
    __RW __u32 DCON2;
    __RW __u32 DCON3;
    __RW __u32 DCON4;
    __RO __u32 PND  ;
    __RW __u32 IE   ;
    __RW __u32 CLR  ;

    __RW __u32 ADDR ;
    __RW __u32 DLEN ;
    __RW __u32 TXBUF0;
    __RW __u32 TXBUF1;

    __RW __u32 RXBUF0;
    __RW __u32 RXBUF1;

    __RW __u32 CCON ;
    __RW __u32 TCON ;
    __RW __u32 INST ;
    __RW __u32 ALBY ;

    __RW __u32 WCCON;
    __RW __u32 WTCON;
    __RW __u32 WINST;
    __RW __u32 WALBY;

    __RW __u32 WPCCON;
    __RW __u32 WPTCON;
    __RW __u32 WPINST;
    __RW __u32 WPALBY;
} JL_ESPI_TypeDef;

typedef struct {
    __RW __u32 CON  ;
} JL_ESPI_CK_TypeDef;

#endif


