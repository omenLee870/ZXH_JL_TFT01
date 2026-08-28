#ifndef __CPU_H__
#define __CPU_H__

#include "br33.h"
#include "pmc_q32dsp_csfr.h"
#include "typedef.h"

///<---- cmac use
#define 	CHIP_NAME   	"BR33"

#define    	BT_SFR_ADR              (wl_base + map_adr(0x02, 0x00))
#define    	BT_WL_ARB_CON1          (*(volatile unsigned long *)(BT_SFR_ADR + 0x03*4))

#define    BT_PHCOM_CNT            (*(volatile unsigned long *)(BT_SFR_ADR + 0x1b*4))
#define    BT_LP_CON               (*(volatile unsigned long *)(BT_SFR_ADR + 0x1c*4))


#define CONFIG_SPINLOCK_ENABLE 0

#define OS_CORE_AFFINITY_ENABLE     CONFIG_OS_AFFINITY_ENABLE //app_cfg.mk中定义, Modify Me: 0: 自由分配, 1: 固定核

extern const int CONFIG_CPU_UNMASK_IRQ_ENABLE;
///屏蔽的优先级, < N的优先级不可以响应
#define CPU_IRQ_IPMASK_LEVEL   				6

#define  CPU_TASK_CLR(a)
#define  CPU_TASK_SW(a) 		\
    do { \
        pmc_q32DSP(a)->ILAT_SET |= BIT(3-a); \
    } while (0)


#define  CPU_INT_NESTING 	2

#define CORE_IDLE_TICK_TIMER_PERIOD         4000 //10 ~ 16000 ms


#define CPU_CRITICAL_ENTER()  \
     do { \
         local_irq_disable(); \
     }while(0)


#define CPU_CRITICAL_EXIT() \
     do { \
         local_irq_enable(); \
     }while(0)


static inline u32 rand32()
{
    return JL_RAND->R64L;
}

static inline int current_cpu_id(void)
{
    u32 num;
    asm volatile("%0 = cnum" : "=r"(num) :);
    return num;
}

static inline int cpu_in_irq()
{
    int flag;
    __asm__ volatile("%0 = icfg" : "=r"(flag));
    return flag & 0xff;
}

static inline int cpu_irq_disabled()
{
    int flag;
    int ret;

    __asm__ volatile("%0 = icfg" : "=r"(flag));
    ret = ((flag & 0x300) != 0x300);

    return ret;
}
#endif
