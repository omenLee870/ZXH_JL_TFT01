#ifndef _IIC_HW_H_
#define _IIC_HW_H_

#include "typedef.h"
#include "gpio.h"


#define MAX_HW_IIC_NUM                  2
#define     JL_IICX_TAB_LIST   PMC_IIC0, \
                               PMC_IIC1, \

#define     IICX_IRQ_IDX_TAB_LIST   IRQ_IIC0_IDX, \
                                    IRQ_IIC1_IDX, \

#define     IICX_ISR_FUNC_TAB_LIST  hw_iic_isr0, \
                                    hw_iic_isr1, \


#define     IICX_SCL_FO_TAB_LIST  PMC_FO_IIC0_SCL, \
                                  PMC_FO_IIC1_SCL, \

#define     IICX_SDA_FO_TAB_LIST  PMC_FO_IIC0_SDA, \
                                  PMC_FO_IIC1_SDA, \

#define     IICX_SCL_FI_TAB_LIST  PFI_IIC0_SCL, \
                                  PFI_IIC1_SCL, \

#define     IICX_SDA_FI_TAB_LIST  PFI_IIC0_SDA, \
                                  PFI_IIC1_SDA, \

#define     IICX_IPC_SPIN_LOCK_EVENT_TAB_LIST  IPC_SPIN_LOCK_EVENT_P11_IIC, \
                                               IPC_SPIN_LOCK_EVENT_P11_IIC, \

enum {
    HW_IIC_0,
    HW_IIC_1,
};


#endif

