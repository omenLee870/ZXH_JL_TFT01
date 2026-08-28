#ifndef	_IPC_SPIN_LOCK_H_
#define _IPC_SPIN_LOCK_H_

#include "typedef.h"
#include "gpio.h"

enum ipc_spin_lock_event {
    IPC_SPIN_LOCK_EVENT_USER0 = 0,//自定义事件名
    IPC_SPIN_LOCK_EVENT_USER1,
    IPC_SPIN_LOCK_EVENT_USER2,
    IPC_SPIN_LOCK_EVENT_USER3,
    IPC_SPIN_LOCK_EVENT_USER4,
    IPC_SPIN_LOCK_EVENT_USER5,
    IPC_SPIN_LOCK_EVENT_USER6,
    IPC_SPIN_LOCK_EVENT_USER7,
    IPC_SPIN_LOCK_EVENT_LP_MSG,
    IPC_SPIN_LOCK_EVENT_P11_GPIO,
    IPC_SPIN_LOCK_EVENT_RTC,
    IPC_SPIN_LOCK_EVENT_UART, //11
    IPC_SPIN_LOCK_EVENT_P11_IIC,//12
    IPC_SPIN_LOCK_EVENT_CBUF, //13
    IPC_SPIN_LOCK_EVENT_PMU,  //14
    IPC_SPIN_LOCK_EVENT_SFR,  //15
    IPC_SPIN_LOCK_EVENT_MAX,
};

void ipc_spin_lock_init();

void ipc_spin_lock(enum ipc_spin_lock_event event);//0~15
void ipc_spin_unlock(enum ipc_spin_lock_event event);//0~15
#endif

