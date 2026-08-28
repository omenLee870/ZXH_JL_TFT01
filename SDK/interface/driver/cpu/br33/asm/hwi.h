#ifndef __HWI_H__
#define __HWI_H__


//=================================================
#define IRQ_EMUEXCPT_IDX        0
#define IRQ_EXCEPTION_IDX       1
#define IRQ_SYSCALL_IDX         2
#define IRQ_TICK_TMR_IDX        3
#define IRQ_TIME0_IDX           4
#define IRQ_TIME1_IDX           5
#define IRQ_TIME2_IDX           6
#define IRQ_TIME3_IDX           7
#define IRQ_TIME4_IDX           8
#define IRQ_TIME5_IDX           9

#define IRQ_ADV_TMR0_IDX        10

#define IRQ_UART0_IDX           11
#define IRQ_UART1_IDX           12
#define IRQ_UART2_IDX           13
#define IRQ_UART3_IDX           31

#define IRQ_IIC0_IDX            14
#define IRQ_IIC1_IDX            15
#define IRQ_IIC2_IDX            16
#define IRQ_IIC3_IDX            17
#define IRQ_IIC4_IDX            18
#define IRQ_IIC5_IDX            19

#define IRQ_SPI0_IDX            20
#define IRQ_SPI1_IDX            21
#define IRQ_CAN_IDX             22
#define IRQ_QDEC0_IDX           23
#define IRQ_QDEC1_IDX           24
#define IRQ_QDEC2_IDX           25

#define IRQ_WWDG_IDX            26
#define IRQ_GPCNT0_IDX          27
#define IRQ_GPCNT1_IDX          28
#define IRQ_GPCNT2_IDX          29

#define IRQ_LED_IDX             33
#define IRQ_PORT_IDX            34

#define IRQ_SPDIF_M_IDX         35
#define IRQ_SPDIF_S_IDX         36
#define IRQ_NFC_IDX             37


#define IRQ_P33_IDX             38   // P33_INT
#define IRQ_PINR_IDX            39   // PINR_INT
#define IRQ_PMU_TMR0_IDX        40   // P33_LPTMR0_INT
#define IRQ_LP_TMR1_IDX        41   // P33_LPTMR1_INT
#define IRQ_VLVD_INT_IDX        42
#define IRQ_WDT_INT_IDX         43
#define IRQ_DCV_OVP_INT_IDX     44
#define IRQ_LPCTMU_WKUP_IDX     45
#define IRQ_AVAD_INT_IDX        46
#define IRQ_PMC_PRP_INT0_IDX    47
#define IRQ_PMC_PRP_INT1_IDX    48
#define IRQ_SD0_BRK_IDX         49   // SDC0_BRK
#define IRQ_SD0_IDX             50   // SDC0
#define IRQ_EFU_AES_IDX         51
#define IRQ_UDMA0_IDX         	52
#define IRQ_UDMA1_IDX         	53
#define IRQ_UDMA2_IDX         	54
#define IRQ_UDMA3_IDX         	55
#define IRQ_UDMA4_IDX         	56
#define IRQ_UDMA5_IDX         	57
#define IRQ_UDMA6_IDX         	58
#define IRQ_UDMA7_IDX         	59

#define IRQ_SD1_BRK_IDX      	60   // XSDC0_BRK
#define IRQ_SD1_IDX         	61   // XSDC0

#define IRQ_USB_CTRL_IDX        62

#define IRQ_FIR_IDX             63
#define IRQ_SRC0_IDX            64   // SRC
#define IRQ_EQ0_IDX            	65   // EQ
#define IRQ_SRC1_IDX            66
#define IRQ_EQ1_IDX            	67

#define IRQ_XSPI_IDX            68
#define IRQ_FSPI1_IDX           69

#define IRQ_DCP_IDX             70
#define IRQ_XBUS_IDX            71
#define IRQ_IPU_VBUS_IDX        72
#define IRQ_ISC_IDX        		73
#define IRQ_REP_IDX        		74
#define IRQ_IMC_IDX        		75
#define IRQ_JPG_IDX             76
#define IRQ_JXENC_IDX      		77
#define IRQ_NCHENC_IDX     		78
#define IRQ_GPU_IDX             79
#define IRQ_GXDMA_IDX      		80
#define IRQ_DMM_IDX        		81
#define IRQ_DBI_IDX             82

#define IRQ_DSI_IDX             84
#define IRQ_VPU_BUS_IDX         85
#define IRQ_DPU_BUS_IDX         86

#define IRQ_SFC0_A2S0_IDX       87
#define IRQ_SFC1_A2S0_IDX       88

#define IRQ_USB_SOF_IDX         89

#define IRQ_ALNK_IDX        	90
#define IRQ_PLNK_IDX        	91
#define IRQ_ADC_IDX             92
#define IRQ_AUDIO_IDX           93   // DAC

#define IRQ_ALNK1_IDX        	95

#define IRQ_P2M_SOFT_INT0_IDX   100
#define IRQ_P2M_SOFT_INT1_IDX   101
#define IRQ_P2M_SOFT_INT2_IDX   102
#define IRQ_P2M_SOFT_INT3_IDX   103
#define IRQ_P2M_SOFT_INT4_IDX   104
#define IRQ_P2M_SOFT_INT5_IDX   105
#define IRQ_P2M_SOFT_INT6_IDX   106
#define IRQ_P2M_SOFT_INT7_IDX   107
#define IRQ_P2M_SOFT_INT8_IDX   108
#define IRQ_P2M_SOFT_INT9_IDX   109
#define IRQ_P2M_SOFT_INT10_IDX  110
#define IRQ_P2M_SOFT_INT11_IDX  111
#define IRQ_P2M_SOFT_INT12_IDX  112
#define IRQ_P2M_SOFT_INT13_IDX  113
#define IRQ_P2M_SOFT_INT14_IDX  114
#define IRQ_P2M_SOFT_INT15_IDX  115

#define IRQ_FFT_IDX             116
#define IRQ_ANPU_IDX            117


#define IRQ_SYNC_IDX            IRQ_PMC_PRP_INT0_IDX
#define IRQ_SYNC_GETMSG_IDX     IRQ_PMC_PRP_INT1_IDX


#define IRQ_SOFT0_IDX           120
#define IRQ_SOFT1_IDX           121
#define IRQ_SOFT2_IDX           122
#define IRQ_SOFT3_IDX           123
#define IRQ_SOFT4_IDX           124
#define IRQ_SOFT5_IDX           125
#define IRQ_SOFT6_IDX           126
#define IRQ_SOFT7_IDX           127

#define     USB_SOFT_IRQ    IRQ_SOFT2_IDX

#define MAX_IRQ_ENTRY_NUM       128


#define IRQ_GPADC_IDX           (MAX_IRQ_ENTRY_NUM + 18)


//=================================================

//=================================================
void interrupt_init();

/* --------------------------------------------------------------------------*/
/**
 * @brief 中断注册函数
 *
 * @param index 中断号
 * @param priority 优先级，范围0-6可用
 * @param handler 中断服务函数
 * @param cpu_id 相应中断服务函数的CPU
 */
/* ----------------------------------------------------------------------------*/

typedef void (*soft_irq_func)(void);

soft_irq_func get_irq_handler(u32 index);

void request_irq(u8 index, u8 priority, void (*handler)(void), u8 cpu_id);

void unrequest_irq(u8 index);

void reg_set_ip(unsigned char index, unsigned char priority, u8 cpu_id);

/* --------------------------------------------------------------------------*/
/**
 * @brief 设置不可屏蔽中断（不可屏蔽中断不区分优先级）
 * cpu多核同步默认使用0，其他使用1。
 *
 * @param index 中断号
 * @param priority 不可屏蔽优先级，范围 0、1 （CPU_IRQ_IPMASK_LEVEL == 6）
 * @param cpu_id 相应中断服务函数的CPU
 */
/* ----------------------------------------------------------------------------*/
void irq_unmask_set(u8 index, u8 priority, u8 cpu_id);

/* --------------------------------------------------------------------------*/
/**
 * @brief 取消不可屏蔽中断
 * cpu多核同步默认使用0，其他使用1。
 *
 * @param index 中断号
 * @param priority 不可屏蔽优先级，范围 0、1、2、3、4、5
 * @param cpu_id 相应中断服务函数的CPU
 */
/* ----------------------------------------------------------------------------*/
void irq_unmask_unset(u8 index, u8 priority, u8 cpu_id);

void bit_clr_ie(unsigned char index);
void bit_set_ie(unsigned char index);
bool irq_read(u32 index);

#define irq_disable(x)  bit_clr_ie(x)
#define irq_enable(x)   bit_set_ie(x)

void unmask_enter_critical(void);
void unmask_exit_critical(void);

//---------------------------------------------//
// low power waiting
//---------------------------------------------//
__attribute__((always_inline))
static inline void lp_waiting(int *ptr, int pnd, int cpd, char inum)
{
#if 0
    q32DSP(core_num())->IWKUP_NUM = inum;
    while (!(*ptr & pnd)) {
        asm volatile("idle");
    }
    *ptr |= cpd;
#else
    int con;
    q32DSP(core_num())->IWKUP_NUM = inum;
    asm volatile(
        " goto 2f                      \n\t"
        " 1:                           \n\t"
        " idle                         \n\t"
        " 2:                           \n\t"
        " %0 = [%1]                    \n\t"
        " rep 1 {                      \n\t"  // disable_bpu
        "     if((%0 & %2)==0) goto 1b \n\t"
        " }                            \n\t"
        :"=&r"(con)
        :"r"(ptr), "r"(pnd), "0"(con)
        :
    );
    *ptr = con | cpd;
#endif
}

//---------------------------------------------//
// interrupt cli/sti
//---------------------------------------------//

static inline int int_cli(void)
{
    int msg;
    asm volatile("cli %0" : "=r"(msg) :);
    return msg;
}

static inline void int_sti(int msg)
{
    asm volatile("sti %0" :: "r"(msg));
}

#ifdef IRQ_TIME_COUNT_EN
void irq_handler_enter(int irq);

void irq_handler_exit(int irq);

void irq_handler_times_dump();
#else

#define irq_handler_enter(irq)      do { }while(0)
#define irq_handler_exit(irq)       do { }while(0)
#define irq_handler_times_dump()    do { }while(0)

#endif


#endif

