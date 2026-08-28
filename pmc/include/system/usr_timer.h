#ifndef __USR_TIMER_H__
#define __USR_TIMER_H__

#define time_after(a,b)					((long)(b) - (long)(a) <= 0)
#define time_before(a,b)				time_after(b,a)

#define TIMER_ID_0_ERROR			1 // ID号0错误


u16 usr_timer_add(void *priv, void (*func)(void *priv), u32 msec, u8 priority);
void usr_timer_del(u16 t);

u16 usr_timeout_add(void *priv, void (*func)(void *priv), u32 msec, u8 priority);
void usr_timeout_del(u16 t);

int usr_timer_modify(u16 id, u32 msec);
int usr_timeout_modify(u16 id, u32 msec);

void usr_timer_schedule();
void usr_timer_dump(void);

int usr_timer_init();

u32 usr_timer_get_timeout();
#endif
