/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    render_queue.c
 * @brief   GPU→LCD 渲染队列抽象层（实现）
 *
 * 通过 gpu_demo_draw_cfg.h 中的 GPU_QUEUE_USE_OS_Q 运行时选择底层实现：
 *   - 1 = 使用 OS 自带消息队列 (os_taskq)
 *   - 0 = 自定义环形缓冲 + 信号量 + 自旋锁
 */

#include "render_queue.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

/* ═══════════════════════════════════════════════════════════════════════
 *  自定义环形队列 (GPU_QUEUE_USE_OS_Q=0 时使用)
 * ═══════════════════════════════════════════════════════════════════════ */

#include "spinlock.h"
#include "os/os_api.h"

#define RQ_DEPTH  8

DEFINE_SPINLOCK(rq_spinlock);

struct render_queue_custom_ctx {
    OS_SEM           sem;
    lcd_flush_msg_t  ring[RQ_DEPTH];
    volatile u8      head;
    volatile u8      tail;
    volatile u8      count;
};

static struct render_queue_custom_ctx rq_cts;

static int rq_custom_init(void)
{
    memset(&rq_cts, 0, sizeof(rq_cts));
    os_sem_create(&rq_cts.sem, 0);
    return 0;
}

static int rq_custom_push(const lcd_flush_msg_t *msg)
{
    spin_lock(&rq_spinlock);

    if (rq_cts.count >= RQ_DEPTH) {
        spin_unlock(&rq_spinlock);
        printf("[RENDER_Q] push overflow! count=%d\n", rq_cts.count);
        return -1;
    }

    memcpy(&rq_cts.ring[rq_cts.head], msg, sizeof(lcd_flush_msg_t));
    rq_cts.head = (rq_cts.head + 1) % RQ_DEPTH;
    rq_cts.count++;

    spin_unlock(&rq_spinlock);

    os_sem_post(&rq_cts.sem);
    return 0;
}

static int rq_custom_pend(lcd_flush_msg_t *msg, u32 timeout_ms)
{
    int err = os_sem_pend(&rq_cts.sem, timeout_ms);
    if (err == OS_TIMEOUT) {
        return -1;
    }

    spin_lock(&rq_spinlock);

    if (rq_cts.count == 0) {
        spin_unlock(&rq_spinlock);
        return -1;
    }

    memcpy(msg, &rq_cts.ring[rq_cts.tail], sizeof(lcd_flush_msg_t));
    rq_cts.tail  = (rq_cts.tail + 1) % RQ_DEPTH;
    rq_cts.count--;

    spin_unlock(&rq_spinlock);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  OS 任务队列 (GPU_QUEUE_USE_OS_Q=1 时使用)
 * ═══════════════════════════════════════════════════════════════════════ */

static int rq_os_init(void)
{
    return 0;
}

static int rq_os_push(const lcd_flush_msg_t *msg)
{
    int m[9];
    m[0] = msg->cmd;
    m[1] = (int)(u32)msg->buf;
    m[2] = msg->x_start;
    m[3] = msg->x_end;
    m[4] = msg->y_start;
    m[5] = msg->y_end;
    m[6] = msg->is_continue;
    m[7] = msg->is_ext_malloc;
    m[8] = msg->is_last_block;

    return os_taskq_post_type("lcd_task", 0, 9, m);
}

static int rq_os_pend(lcd_flush_msg_t *msg, u32 timeout_ms)
{
    int m[32] = {0};

    (void)timeout_ms;

    if (os_taskq_pend(NULL, m, ARRAY_SIZE(m)) != OS_TASKQ) {
        return -1;
    }

    msg->cmd           = m[1];
    msg->buf           = (u8 *)(u32)m[2];
    msg->x_start       = m[3];
    msg->x_end         = m[4];
    msg->y_start       = m[5];
    msg->y_end         = m[6];
    msg->is_continue   = m[7];
    msg->is_ext_malloc = m[8];
    msg->is_last_block = m[9];

    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  公开 API — 运行时分支
 * ═══════════════════════════════════════════════════════════════════════ */

int render_queue_init(void)
{
    if (GPU_QUEUE_USE_OS_Q) {
        return rq_os_init();
    }
    return rq_custom_init();
}

int render_queue_push(const lcd_flush_msg_t *msg)
{
    if (GPU_QUEUE_USE_OS_Q) {
        return rq_os_push(msg);
    }
    return rq_custom_push(msg);
}

int render_queue_pend(lcd_flush_msg_t *msg, u32 timeout_ms)
{
    if (GPU_QUEUE_USE_OS_Q) {
        return rq_os_pend(msg, timeout_ms);
    }
    return rq_custom_pend(msg, timeout_ms);
}

#endif /* GPU_DEMO_ASYN_DRAW */
