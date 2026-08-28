
#ifndef __I2C_HAL_H__
#define __I2C_HAL_H__

typedef struct {
    __RW __u32 CON    ;
    __RW __u32 PND    ;
    __RW __u32 TX_BUF ;
    __RW __u32 TASK   ;
    __RO __u32 RX_BUF ;
    __RW __u32 ADDR   ;
    __RW __u32 BAUD   ;
    __RW __u32 TSU    ;
    __RW __u32 THD    ;
    __RO __u32 DBG    ;
    __RW __u32 TRIG_EN;
} JL_I2C_TypeDef;

#endif

