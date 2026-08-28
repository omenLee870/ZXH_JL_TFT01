#ifndef __INTERRUPT_HW__
#define __INTERRUPT_HW__


/*
gpcnt需要打断低功耗采样，优先即最高且不可屏蔽
 */

#define IRQ_EXCEPTION_IP   	    0
#define IRQ_M2P_IP   			2
#define IRQ_P33_IP   			0
#define IRQ_PINR_IP   			0
#define IRQ_SOFT0_IP       		0
#define IRQ_SOFT1_IP       		0
#define IRQ_WDT_IP   			0
#define IRQ_SPI_IP   			0
#define IRQ_UART0_IP   			0
#define IRQ_UART1_IP   			0
#define IRQ_IIC_IP   			0
#define IRQ_LP_TMR0_IP   		2
#define IRQ_LP_TMR1_IP   		0
#define IRQ_LP_TMR2_IP   		0
#define IRQ_LP_TMR3_IP   		0
#define IRQ_GP_TMR0_IP   		0
#define IRQ_GP_TMR1_IP   		0
#define IRQ_LP_CTM_IP   	    1
#define IRQ_LP_VAD_IP   		0
#define IRQ_GPCNT_IP   			3




#endif
