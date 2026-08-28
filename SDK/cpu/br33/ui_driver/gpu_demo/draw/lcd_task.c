/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    lcd_task.c
 * @brief   LCD 推屏任务 — 通过渲染队列接收 GPU 输出的 buffer，经 TE 同步后推到 LCD
 *
 * 流水线: [UI Task] → (GPU task chain) → [GPU Task] → (render_queue) → [LCD Task]
 *
 * DBI 推屏生命周期:
 *   lcd_task:
 *     1. render_queue_pend() 从队列取 buf 消息
 *     2. os_sem_pend(flush_sem) 等上次 DBI 推屏完成
 *     3. adpt_te_wait_te() 等 TE 同步 (首块)
 *     4. lcd_draw_kistart/continue 启动 DBI 推屏
 *                                           →
 *   DBI ISR (异步):
 *     lcd_flush_done_cb:
 *     5. set_buffer_idle / free(buf)  释放 buf
 *     6. if last_block: adpt_te_isr_push_complete()
 *     7. os_sem_post(flush_sem)       通知 lcd_task 推屏已完成
 */

#include "gpu_demo.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

#include "includes.h"
#include "buffer_manager.h"
#include "ui/lcd/lcd_drive.h"
#include "render_queue.h"
#include "gpu_demo_draw_cfg.h"
#include "adpt_te.h"
#include "ui_measure.h"

#define DBG(...)
/* #define DBG printf */

#define LCD_TASK_NAME   "lcd_task"
#define LCD_TASK_PRIO   9
#define LCD_TASK_STK    1024
#define LCD_TASK_Q_SZ   512

#define LCD_FLUSH_TIMEOUT_MS 100

/* ═══════════════════════════════════════════════════════════════════════
 *  LCD 任务上下文
 * ═══════════════════════════════════════════════════════════════════════ */
struct lcd_task_ctx {
    OS_SEM flush_sem;             /* DBI 推屏完成信号量 (ISR→task) */
    int    width;                 /* LCD 宽度 */
    int    height;                /* LCD 高度 */

    /* 当前推屏上下文 (在推屏前设置, ISR 回调中使用) */
    u8    *flush_buf;             /* 正在推屏的 buf 地址 */
    int    flush_is_ext;          /* buf 是否来自 malloc */
    int    flush_is_last;         /* 是否为帧末块 (用于 ISR 中调 push_complete) */
    OS_SEM frame_done_sem;        /* 帧推屏完成信号量 (ISR→gpu_task) */
};

static struct lcd_task_ctx g_lcd;

/* ── DBI 推屏完成 ISR 回调 ── */
static int lcd_flush_done_cb(void *p)
{
    u8  *buf     = g_lcd.flush_buf;
    int  is_ext  = g_lcd.flush_is_ext;
    int  is_last = g_lcd.flush_is_last;

    DBG("[LCD_TASK] flush done ISR: is_ext=%d buf=0x%x last=%d\n",
        is_ext, (u32)(uintptr_t)buf, is_last);

    if (buf) {
        if (!is_ext) {
            /* pool buf: 直接归还 buffer_manager (ISR 安全) */
            set_buffer_idle(NULL, buf);
        } else {
            DBG("[LCD_TASK] free ext buf: 0x%x\n", (u32)(uintptr_t)g_lcd.flush_buf);
            free(g_lcd.flush_buf);
        }
        /* ext_malloc buf: 保留 g_lcd.flush_buf/is_ext 由主循环 free */
        g_lcd.flush_buf     = NULL;
        g_lcd.flush_is_ext  = 0;
    }

    if (is_last) {
        g_lcd.flush_is_last = 0;
        adpt_te_isr_push_complete();
        os_sem_post(&g_lcd.frame_done_sem);  /* 通知 gpu_task 帧推屏完成 */
    }

    os_sem_post(&g_lcd.flush_sem);
    IO_DBI_LOW();
    return 0;
}

/* ── LCD 硬件初始化 ── */
static void lcd_hw_init(void)
{
    struct lcd_interface *lcd = lcd_get_hdl();

    ASSERT(lcd);
    ASSERT(lcd->init);
    ASSERT(lcd->get_screen_info);
    ASSERT(lcd->clear_screen);
    ASSERT(lcd->backlight_ctrl);

    extern const struct ui_devices_cfg ui_cfg_data;
    lcd->init((void *)&ui_cfg_data);

    struct lcd_info info;
    lcd->get_screen_info(&info);
    g_lcd.width  = info.width;
    g_lcd.height = info.height;

    lcd->clear_screen(0x000000, 0, info.width - 1, 0, info.height - 1);
    lcd->backlight_ctrl(100);

    /* 注册 DBI DMA 完成中断回调 */
    lcd_draw_set_callback(lcd_flush_done_cb);

    DBG("[LCD_TASK] hw init OK: %dx%d\n", info.width, info.height);
}

/* ── 任务入口 ── */
static void lcd_task(void *p)
{
    os_sem_create(&g_lcd.flush_sem, 1);
    os_sem_create(&g_lcd.frame_done_sem, 1);   /* 首帧无需等待，直接通过 */

    lcd_hw_init();

    render_queue_init();

    adpt_te_init(NULL);
    adpt_te_start();

    while (1) {
        lcd_flush_msg_t qmsg;

        /* ── ① 挂起等待队列消息 ── */
        if (render_queue_pend(&qmsg, 0) != 0) {
            continue;
        }
        DBG("[LCD_TASK] queue pend: cmd=%d buf=%p x[%d,%d] y[%d,%d] cont=%d ext=%d last=%d\n",
            qmsg.cmd, (void *)qmsg.buf,
            qmsg.x_start, qmsg.x_end, qmsg.y_start, qmsg.y_end,
            qmsg.is_continue, qmsg.is_ext_malloc, qmsg.is_last_block);

        /* ── ② 等待上一次推屏完成 + 释放 ext buf ── */
        if (qmsg.cmd == LCD_CMD_FLUSH) {
            if (os_sem_pend(&g_lcd.flush_sem, LCD_FLUSH_TIMEOUT_MS) == OS_TIMEOUT) {
                printf("[LCD_TASK] flush timeout! buf=0x%08lx\n",
                       (unsigned long)(uintptr_t)qmsg.buf);
                continue;
            }
        }

        /* 释放上一次 ext_malloc buf (ISR 中保留, 此处 safe free) */
        /* if (g_lcd.flush_buf && g_lcd.flush_is_ext) { */
        /* DBG("[LCD_TASK] free ext buf: 0x%x\n", (u32)(uintptr_t)g_lcd.flush_buf); */
        /* free(g_lcd.flush_buf); */
        /* g_lcd.flush_buf    = NULL; */
        /* g_lcd.flush_is_ext = 0; */
        /* } */

        switch (qmsg.cmd) {

        case LCD_CMD_SET_AREA:
            DBG("[LCD_TASK] SET_AREA: x[%d,%d] y[%d,%d]\n",
                qmsg.x_start, qmsg.x_end, qmsg.y_start, qmsg.y_end);
            lcd_set_draw_area(qmsg.x_start, qmsg.x_end,
                              qmsg.y_start, qmsg.y_end);
            break;

        case LCD_CMD_FLUSH: {
            int te_ret;

            DBG("[LCD_TASK] FLUSH: buf=%p x[%d,%d] y[%d,%d] cont=%d last=%d\n",
                (void *)qmsg.buf,
                qmsg.x_start, qmsg.x_end, qmsg.y_start, qmsg.y_end,
                qmsg.is_continue, qmsg.is_last_block);

            /* TE 同步 (仅首分块) */
            if (!qmsg.is_continue) {
                DBG("[LCD TASK] FLUSH: @@@@@ --> Wati Te Sync start!\n");
                te_ret = adpt_te_wait_te(0);
                if (te_ret == -2) {
                    printf("[LCD_TASK] TE timeout, push anyway\n");
                }
                DBG("[LCD TASK] FLUSH: @@@@@ --> Wati Te Sync end!\n");
            }

            /* 记录推屏上下文 (供 ISR 使用) */
            g_lcd.flush_buf      = qmsg.buf;
            g_lcd.flush_is_ext   = qmsg.is_ext_malloc;
            g_lcd.flush_is_last  = qmsg.is_last_block;

            /* 启动 DBI 推屏 */
            IO_DBI_HIGH();
            if (!qmsg.is_continue) {
                lcd_draw_kistart(qmsg.buf,
                                 qmsg.x_start, qmsg.x_end,
                                 qmsg.y_start, qmsg.y_end);
            } else {
                lcd_draw_continue(qmsg.buf,
                                  qmsg.x_start, qmsg.x_end,
                                  qmsg.y_start, qmsg.y_end);
            }
            break;
        }
        default:
            break;
        }
    }
}

/* ── 初始化 ── */
void lcd_task_init(void)
{
    memset(&g_lcd, 0, sizeof(g_lcd));

    int err = os_task_create(lcd_task, NULL, LCD_TASK_PRIO,
                             LCD_TASK_STK, LCD_TASK_Q_SZ, LCD_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("[LCD_TASK] create failed! err=%d\n", err);
    }
}

/* ── 帧完成同步 ── */

/**
 * @brief 等待当前帧全部推屏完成 (阻塞调用, 仅首分块调用方)
 *
 * 一帧的所有分块都经 DBI 推屏完毕 (最末块的 DBI ISR 触发) 后,
 * 本函数返回 0。调用方 (ui_task) 应在新帧渲染前调用以确保
 * 上一帧不会因显存复用而花屏。
 *
 * @param timeout_ms  超时时间 (ms), 0=死等
 * @return 0=帧推屏完成, OS_TIMEOUT=超时
 */
int lcd_wait_frame_done(u32 timeout_ms)
{
    return os_sem_pend(&g_lcd.frame_done_sem, timeout_ms);
}

#endif /* GPU_DEMO_ASYN_DRAW */
