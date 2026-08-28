
#ifndef __XSPI_HAL_H__
#define __XSPI_HAL_H__

typedef struct {
    __RW __u32 CON   ;
    __RW __u32 DCON1 ;
    __RW __u32 DCON2 ;
    __RW __u32 DCON3 ;
    __RW __u32 DCON4 ;
    __RW __u32 ST    ;
    __RW __u32 STCLR ;

    __RW __u32 DLEN  ;
    __RW __u32 ADDR  ;
    __RW __u32 DAT   ;

    __RW __u32 PSMK  ;
    __RW __u32 PSMA  ;
    __RW __u32 POLLT ;

    __RW __u32 LPT   ;

    __RW __u32 CCON  ;
    __RW __u32 TCON  ;
    __RW __u32 INST  ;
    __RW __u32 ALTER ;

    __RW __u32 WPCCON;
    __RW __u32 WPTCON;
    __RW __u32 WPINST;
    __RW __u32 WPALTE;

    __RW __u32 WCCON ;
    __RW __u32 WTCON ;
    __RW __u32 WINST ;
    __RW __u32 WALTER;

    __RW __u32 HLCON ;
    __RW __u32 IOCTL ;

    __RW __u32 DLLCON0;
    __RW __u32 DLLCON1;
} JL_XSPI_TypeDef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 BEG_ADDR;
    __RW __u32 END_ADDR;
    __RW __u32 OFF_ADDR;
} JL_XSPI_MMU_TypeDef;



#endif


