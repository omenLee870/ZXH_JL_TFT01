#ifndef __IIC_HW_H__
#define __IIC_HW_H__

#define MAX_HW_IIC_NUM                  8
#define P11_HW_IIC_NUM                  2 //p11 iic使能,及锁使能


#define     JL_IICX_TAB_LIST   JL_IIC0, \
                               JL_IIC1, \
                               JL_IIC2, \
                               JL_IIC3, \
                               JL_IIC4, \
                               JL_IIC5, \
                               PMC_IIC0, \
                               PMC_IIC1, \

#define     IICX_IRQ_IDX_TAB_LIST   IRQ_IIC0_IDX, \
                                    IRQ_IIC1_IDX, \
                                    IRQ_IIC2_IDX, \
                                    IRQ_IIC3_IDX, \
                                    IRQ_IIC4_IDX, \
                                    IRQ_IIC5_IDX, \
                                    -1, \
                                    -1, \

#define     IICX_ISR_FUNC_TAB_LIST  hw_iic_isr0, \
                                    hw_iic_isr1, \
                                    hw_iic_isr2, \
                                    hw_iic_isr3, \
                                    hw_iic_isr4, \
                                    hw_iic_isr5, \
                                    0, \
                                    0, \


#define     IICX_SCL_FUNC_TAB_LIST  PORT_FUNC_IIC0_SCL, \
                                    PORT_FUNC_IIC1_SCL, \
                                    PORT_FUNC_IIC2_SCL, \
                                    PORT_FUNC_IIC3_SCL, \
                                    PORT_FUNC_IIC4_SCL, \
                                    PORT_FUNC_IIC5_SCL, \
                                    0, \
                                    0, \

#define     IICX_SDA_FUNC_TAB_LIST  PORT_FUNC_IIC0_SDA, \
                                    PORT_FUNC_IIC1_SDA, \
                                    PORT_FUNC_IIC2_SDA, \
                                    PORT_FUNC_IIC3_SDA, \
                                    PORT_FUNC_IIC4_SDA, \
                                    PORT_FUNC_IIC5_SDA, \
                                    0, \
                                    0, \

enum {
    HW_IIC_0,
    HW_IIC_1,
    HW_IIC_2,
    HW_IIC_3,
    HW_IIC_4,
    HW_IIC_5,
#if defined(P11_HW_IIC_NUM)&&P11_HW_IIC_NUM
    HW_P11_IIC_0,//p11 init,pb
    HW_P11_IIC_1,//p11 init,pb
#endif

};

#endif

