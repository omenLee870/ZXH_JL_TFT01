#ifndef __TIMER_H__
#define __TIMER_H__

#include "typedef.h"
#include "p11_gptimer.h"

#define USR_TMR     GPTIMER0


void timer_suspend(void);
void timer_resume(u32 usec);
void timer_stop(void);
void timer_run(u32 period_ms);
void timer_init();
_INLINE_ u32 timer_get_jiffies();
_INLINE_ u64 timer_get_sys_tick_time_us(void);
void timer_set_clock_source(u32 clk);
void timer_dump();

#endif
