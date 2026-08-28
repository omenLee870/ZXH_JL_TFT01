
#ifndef __WWDG_HAL_H__
#define __WWDG_HAL_H__

typedef struct {
    __WO __u32 KEY;
    __RW __u32 CR;
    __RW __u32 CFG;
    __RO __u32 SR;
} JL_WWDG_TypeDef;

#endif

