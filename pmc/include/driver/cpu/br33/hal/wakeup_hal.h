
#ifndef __WAKEUP_HAL_H__
#define __WAKEUP_HAL_H__

typedef struct {
    __RO __u32 WKUP_SRC;
    __RW __u32 IOWKUP_EN;
    __RW __u32 IOWKUP_EDGE;
    __RW __u32 IOWKUP_CLRPND;
    __RW __u32 IOWKUP_PND;
    __RW __u32 IOWKUP_EN2;
    __RW __u32 IOWKUP_EDGE2;
    __RW __u32 IOWKUP_CLRPND2;
    __RW __u32 IOWKUP_PND2;
    __RW __u32 IOWKUP_EN3;
    __RW __u32 IOWKUP_EDGE3;
    __RW __u32 IOWKUP_CLRPND3;
    __RW __u32 IOWKUP_PND3;
} JL_WAKEUP_TypeDef;

#endif


