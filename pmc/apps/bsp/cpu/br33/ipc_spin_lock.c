#include "includes.h"
#include "ipc_spin_lock.h"

static volatile int ipc_cpu_lock_cnt[CPU_CORE_NUM];

void ipc_spin_lock_init()
{
    for (u8 i = 0; i < 16; i++) {
        PMC_RESLOCK->LOCK[i] = 0;
    }

    memset((void *)ipc_cpu_lock_cnt, 0, sizeof(ipc_cpu_lock_cnt));
}

AT(.ipc_spin_lock.text.cache.L1)
__attribute__((noinline))
void ipc_spin_lock(enum ipc_spin_lock_event event)//0~15
{
    local_irq_disable();
    ASSERT(event <= 15);
    event = IPC_SPIN_LOCK_EVENT_USER0;
    if (0 == ipc_cpu_lock_cnt[core_num()]++) {
        while (PMC_RESLOCK->LOCK[event]);
    }
}
AT(.ipc_spin_lock.text.cache.L1)
__attribute__((noinline))
void ipc_spin_unlock(enum ipc_spin_lock_event event)//0~15
{
    ASSERT(event <= 15);
    event = IPC_SPIN_LOCK_EVENT_USER0;
    if (--ipc_cpu_lock_cnt[core_num()] == 0) {
        PMC_RESLOCK->LOCK[event] = 0;
    }
    local_irq_enable();
}

