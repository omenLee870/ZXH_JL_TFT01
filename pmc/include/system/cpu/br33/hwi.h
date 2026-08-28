#ifndef __ASS_Q32S_HWI__
#define __ASS_Q32S_HWI__

//*********************************************************************************//
// Module name : hwi.h                                                             //
// Description : q32DSP interrupt head file                                        //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//


#define IRQ_BPCTL_IDX   	    0
#define IRQ_EXCEPTION_IDX   	1
#define IRQ_SYSCALL_IDX         2
#define IRQ_TICK_TMR_IDX        3

#define IRQ_UART0_IDX   		4
#define IRQ_UART1_IDX   		5
#define IRQ_IIC0_IDX   			6
#define IRQ_IIC1_IDX   			7
#define IRQ_LP_TMR0_IDX   		8
#define IRQ_LP_TMR1_IDX   		9
#define IRQ_GP_TMR0_IDX   	    10   // GPTMR0
#define IRQ_GP_TMR1_IDX   	    11
#define IRQ_PMC_GPCNT0_IDX 		12
#define IRQ_PMC_GPCNT1_IDX 		13
#define IRQ_UDMA0_IDX   		14
#define IRQ_UDMA1_IDX   		15
#define IRQ_UDMA2_IDX   		16
#define IRQ_UDMA3_IDX   		17

#define IRQ_FFT_IDX             18
#define IRQ_WWDG_IDX            19
#define IRQ_LP_VAD_IDX   		20
#define IRQ_AES_IDX         	21
#define IRQ_GPADC_IDX           22
#define IRQ_LCTM0_IDX   	    23
#define IRQ_LPSPI0_IDX   		24
#define IRQ_FSPI0_IDX  		    25
#define IRQ_SFC0_A2S1_IDX       26


#define IRQ_P33_IDX   			39
#define IRQ_PINR_IDX   			40
#define IRQ_TOPND0_IDX   		41
#define IRQ_TOPND1_IDX   		42
#define IRQ_VLVD_IDX   		    43
#define IRQ_WDT_IDX   			44
#define IRQ_DCV_OVP_IDX			45
#define IRQ_LPCTMU_IDX			46
#define IRQ_AVAD_IDX   		    47
#define IRQ_M2P_INT0_IDX		48
#define IRQ_M2P_INT1_IDX		49
#define IRQ_M2P_INT2_IDX		50
#define IRQ_M2P_INT3_IDX		51
#define IRQ_M2P_INT4_IDX		52
#define IRQ_M2P_INT5_IDX		53
#define IRQ_M2P_INT6_IDX		54
#define IRQ_M2P_INT7_IDX		55
#define IRQ_M2P_INT8_IDX		56
#define IRQ_M2P_INT9_IDX		57
#define IRQ_M2P_INT10_IDX		58
#define IRQ_M2P_INT11_IDX		59
#define IRQ_M2P_INT12_IDX		60
#define IRQ_M2P_INT13_IDX		61
#define IRQ_M2P_INT14_IDX		62
#define IRQ_M2P_INT15_IDX		63

#define IRQ_PRP_SWI0_IDX        64
#define IRQ_PRP_SWI1_IDX        65
#define IRQ_PRP_SWI2_IDX        66
#define IRQ_PRP_SWI3_IDX        67
#define IRQ_PRP_SWI4_IDX        68
#define IRQ_PRP_SWI5_IDX        69
#define IRQ_PRP_SWI6_IDX        70
#define IRQ_PRP_SWI7_IDX        71
#define IRQ_PRP_SWI8_IDX        72
#define IRQ_PRP_SWI9_IDX        73
#define IRQ_PRP_SWI10_IDX       74
#define IRQ_PRP_SWI11_IDX       75
#define IRQ_PRP_SWI12_IDX       76
#define IRQ_PRP_SWI13_IDX       77
#define IRQ_PRP_SWI14_IDX       78
#define IRQ_PRP_SWI15_IDX       79

#define IRQ_RP_CNT_IDX          80
#define IRQ_CMAC_SOFT_IDX       81
#define IRQ_BT_BLERX_IDX        82
#define IRQ_BT_EVENT_IDX        83
#define IRQ_BT_TIMEBASE_IDX     84
#define IRQ_BT_CLKN_IDX         85
#define IRQ_BREDR_IDX           86
#define IRQ_BLE_RXMCH_IDX       87
#define IRQ_SRC_SYNC_IDX        88
#define IRQ_BREDR_RXMCH_IDX     89
#define IRQ_BTRAM_DBG_IDX       90
#define IRQ_SYNC_GETMSG_IDX     91
#define IRQ_TXHA_IDX            92

#define IRQ_PACKET_IDX          96
#define IRQ_TIMEBASE_IDX        97
#define IRQ_CMAC_TSK_IDX        98

#define IRQ_SOFT0_IDX			120
#define IRQ_SOFT1_IDX			121
#define IRQ_SOFT2_IDX           122
#define IRQ_SOFT3_IDX           123
#define IRQ_SOFT4_IDX           124
#define IRQ_SOFT5_IDX           125
#define IRQ_SOFT6_IDX           126
#define IRQ_SOFT7_IDX           127

#define MAX_IRQ_ENTRY_NUM       128

//---------------------------------------------//
// interrupt install
//---------------------------------------------//

void reg_set_ip(unsigned char index, unsigned char dat);
void bit_set_ie(unsigned char index);
void bit_clr_ie(unsigned char index);
void bit_set_swi(unsigned char index);
void bit_clr_swi(unsigned char index);

void request_irq(u8 index, u8 priority, void (*handler)(void), u8 cpu_id);
#define irq_unmask_set(a,b,c)

//---------------------------------------------//
// core_num
//---------------------------------------------//

static inline int core_num(void)
{
    u32 num;
    asm volatile("%0 = cnum" : "=r"(num) :);
    return num;
}

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
//---------------------------------------------//
// interrupt enable
//---------------------------------------------//

#define ENABLE_INT enable_int
#define enable_int()  local_irq_enable()

#define DISABLE_INT disable_int
#define disable_int() local_irq_disable()

#define ___interrupt 	__attribute__((interrupt("")))


void local_irq_disable();
void local_irq_enable();

void irq_disable_core();
void irq_enable_core();

#define P33_IE_ENABLE()     \
    bit_set_ie(IRQ_P33_IDX)

#define P33_IE_DISABLE()     \
    bit_clr_ie(IRQ_P33_IDX)


//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif
