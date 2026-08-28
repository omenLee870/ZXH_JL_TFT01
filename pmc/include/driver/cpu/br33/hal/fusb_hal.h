
#ifndef __FUSB_HAL_H__
#define __FUSB_HAL_H__

typedef struct {
    __RW __u32 FADDR;
    __RW __u32 POWER;
    __RO __u32 INTRTX1;
    __RO __u32 INTRTX2;
    __RO __u32 INTRRX1;
    __RO __u32 INTRRX2;
    __RO __u32 INTRUSB;
    __RW __u32 INTRTX1E;
    __RW __u32 INTRTX2E;
    __RW __u32 INTRRX1E;
    __RW __u32 INTRRX2E;
    __RW __u32 INTRUSBE;
    __RO __u32 FRAME1;
    __RO __u32 FRAME2;
    __RO __u32 RESERVED14;
    __RW __u32 DEVCTL;
    __RO __u32 DONTTOUCH10_1b[0x1b - 0x10 + 1];
    __RW __u32 VHUB_FADDR1;
    __RW __u32 VHUB_FADDR2;
    __RO __u32 RESERVED1e_1f[0x1f - 0x1e + 1];
} JL_USB_SIE_TypeDef;

typedef struct {
    __RO __u32 RESERVED0;
    __RW __u32 CSR0;
    __RO __u32 RESERVED2_5[5 - 1];
    __RO __u32 COUNT0;
} JL_USB_EP0_TypeDef;

typedef struct {
    __RW __u32 TXMAXP;
    __RW __u32 TXCSR1;
    __RW __u32 TXCSR2;
    __RW __u32 RXMAXP;
    __RW __u32 RXCSR1;
    __RW __u32 RXCSR2;
    __RO __u32 RXCOUNT1;
    __RO __u32 RXCOUNT2;
    __RW __u32 TXTYPE;
    __RO __u32 TXINTERVAL;
    __RW __u32 RXTYPE;
    __RO __u32 RXINTERVAL;
} JL_USB_EP_TypeDef;

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 TXDLY_CON;
    __RW __u32 SOF_STA_CON;
    __RW __u32 EP0_ADR;
    __WO __u32 EP0_CNT;
    __WO __u32 EP1_CNT;
    __WO __u32 EP2_CNT;
    __WO __u32 EP3_CNT;
    __WO __u32 EP4_CNT;
    __WO __u32 EP5_CNT;
    __WO __u32 EP6_CNT;
    __RW __u32 EP1_MTX_CON0;
    __RW __u32 EP1_MTX_CON1;
    __RW __u32 EP1_MTX_DAT_ADR;
    __RW __u32 EP1_MTX_STA_ADR;
    __RW __u32 EP2_MTX_CON0;
    __RW __u32 EP2_MTX_CON1;
    __RW __u32 EP2_MTX_DAT_ADR;
    __RW __u32 EP2_MTX_STA_ADR;
    __RW __u32 EP3_MTX_CON0;
    __RW __u32 EP3_MTX_CON1;
    __RW __u32 EP3_MTX_DAT_ADR;
    __RW __u32 EP3_MTX_STA_ADR;
    __RW __u32 EP4_MTX_CON0;
    __RW __u32 EP4_MTX_CON1;
    __RW __u32 EP4_MTX_DAT_ADR;
    __RW __u32 EP4_MTX_STA_ADR;
    __RW __u32 EP5_MTX_CON0;
    __RW __u32 EP5_MTX_CON1;
    __RW __u32 EP5_MTX_DAT_ADR;
    __RW __u32 EP5_MTX_STA_ADR;
    __RW __u32 EP6_MTX_CON0;
    __RW __u32 EP6_MTX_CON1;
    __RW __u32 EP6_MTX_DAT_ADR;
    __RW __u32 EP6_MTX_STA_ADR;
    __RW __u32 EP1_MRX_CON0;
    __RW __u32 EP1_MRX_CON1;
    __RW __u32 EP1_MRX_DAT_ADR;
    __RW __u32 EP1_MRX_STA_ADR;
    __RW __u32 EP2_MRX_CON0;
    __RW __u32 EP2_MRX_CON1;
    __RW __u32 EP2_MRX_DAT_ADR;
    __RW __u32 EP2_MRX_STA_ADR;
    __RW __u32 EP3_MRX_CON0;
    __RW __u32 EP3_MRX_CON1;
    __RW __u32 EP3_MRX_DAT_ADR;
    __RW __u32 EP3_MRX_STA_ADR;
    __RW __u32 EP4_MRX_CON0;
    __RW __u32 EP4_MRX_CON1;
    __RW __u32 EP4_MRX_DAT_ADR;
    __RW __u32 EP4_MRX_STA_ADR;
    __RW __u32 EP5_MRX_CON0;
    __RW __u32 EP5_MRX_CON1;
    __RW __u32 EP5_MRX_DAT_ADR;
    __RW __u32 EP5_MRX_STA_ADR;
    __RW __u32 EP6_MRX_CON0;
    __RW __u32 EP6_MRX_CON1;
    __RW __u32 EP6_MRX_DAT_ADR;
    __RW __u32 EP6_MRX_STA_ADR;
    __RW __u32 VHUB_F1_EP0_ADR;
    __WO __u32 VHUB_F1_EP0_CNT;
    __RW __u32 VHUB_F2_EP0_ADR;
    __WO __u32 VHUB_F2_EP0_CNT;
} JL_USB_TypeDef;

#endif


