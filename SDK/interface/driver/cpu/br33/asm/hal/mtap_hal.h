
#ifndef __MTAP_HAL_H__
#define __MTAP_HAL_H__

typedef struct {
    __RW __u32 MBISTPG_CTL ;
    __RO __u32 MBISTPG_SO  ;
    __RO __u32 MBISTPG_GO  ;
    __RO __u32 MBISTPG_DONE;
    __RW __u32 MBISTPG_SEL ;
} JL_MTAP_TypeDef;


#endif


