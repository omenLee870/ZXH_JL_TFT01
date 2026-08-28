
#ifndef __EFUSE_HAL_H__
#define __EFUSE_HAL_H__

typedef struct {
    __RW __u32 EFU_CON;
    __RO __u32 EFU_RDAT;
    __RW __u32 STROBE_CON;
    __RW __u32 ADDR_CON  ;
    __RW __u32 DAT_ADDR  ;
    __RW __u32 DAT_LEN   ;
    __RW __u32 IV_ADDR   ;
    __RW __u32 AES_CON   ;
    __RW __u32 EFU_SW_EN ;
    __RW __u32 SAFE_CON  ;
    __RW __u32 SAFE_KEY  ;
} JL_EFUSE0_TypeDef;

#endif


