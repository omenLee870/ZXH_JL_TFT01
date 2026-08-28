
#ifndef __MBIST_HAL_H__
#define __MBIST_HAL_H__

typedef struct {
    __RW __u32 CON;
    __RW __u32 SEL;
    __RW __u32 BEG;
    __RW __u32 END;
    __RW __u32 DAT_VLD0;
    __RW __u32 DAT_VLD1;
    __RW __u32 DAT_VLD2;
    __RW __u32 DAT_VLD3;
} JL_HMBIST_TypeDef;

#endif
