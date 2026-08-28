/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    gpu_task.c
 * @brief   GPU 合成任务 — 接收 UI 任务投递的 GPU 任务链，启动硬件合成
 *
 * 流水线: [UI Task] → (GPU task chain) → [GPU Task] → (rendered buffer) → [LCD Task]
 *
 * 参考 gpu_demo_draw.c 的 gpu_demo_start_draw 实现分块渲染：
 *   - 根据 block_height 将页面垂直切分为多个分块
 *   - 每个分块：获取空闲 buf → GPU 合成 → 投递给 lcd_task 推屏
 *   - 首分块 is_continue=0 (lcd_task 需设 draw_area + 等 TE + kistart)
 *   - 后续分块 is_continue=1 (lcd_task 用 continue 续传)
 */

#include "gpu_demo.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

#include "includes.h"
#include "buffer_manager.h"
#include "ui/lcd/lcd_drive.h"
#include "render_queue.h"
#include "gpu_demo_draw_cfg.h"

#include "ui_measure.h"

#define DBG(...)
/* #define DBG printf */

#define GPU_TASK_NAME   "gpu_task"
#define GPU_TASK_PRIO   8
#define GPU_TASK_STK    2048
#define GPU_TASK_Q_SZ   256

/* ── 消息 (UI→GPU) ── */
enum {
    GPU_MSG_RENDER = 0,   /* msg[1]=head, 渲染区域从 head->win_rect 获取 */
};

/* ═══════════════════════════════════════════════════════════════════════
 *  GPU 任务上下文
 * ═══════════════════════════════════════════════════════════════════════ */
struct gpu_task_ctx {
    void *buffer_hdl;             /* 显存buf管理句柄 */
    u16   block_height;           /* 显存buf分块高度 */
    struct lcd_info info;         /* LCD屏幕信息    */
    OS_SEM gpu_done_sem;          /* GPU→UI: 渲染完成 (task post, task pend) */
};

static struct gpu_task_ctx g_ctx;

/* ── GPU 硬件初始化 ── */
static void gpu_hw_init(void)
{
    struct lcd_interface *lcd = lcd_get_hdl();

    ASSERT(lcd && lcd->get_screen_info && lcd->buffer_malloc);

    /* 获取屏幕信息 */
    lcd->get_screen_info(&g_ctx.info);

    /* 申请显存buf */
    u8  *buf;
    u32  len;
    lcd->buffer_malloc(&buf, &len);

    /* 初始化显存buf管理 */
    buffer_manager_init(g_ctx.buffer_hdl, buf, len, g_ctx.info.buf_num);

    /* 注册为默认句柄，lcd_task 可通过 NULL 访问 */
    set_default_buffer_handler(g_ctx.buffer_hdl);

    /* 计算分块高度 */
    g_ctx.block_height = len / g_ctx.info.buf_num / g_ctx.info.stride;

    /* GPU 硬件模块初始化 */
    jlgpu_module_init(g_ctx.info.width, g_ctx.info.height);

    /* 设置 GPU 窗口 */
    struct rect win;
    rect_left(&win)   = GPU_WIN_OFFSET;
    rect_top(&win)    = GPU_WIN_OFFSET;
    rect_width(&win)  = g_ctx.info.width;
    rect_height(&win) = g_ctx.info.height;
    jlgpu_set_win_rect(&win);

    printf("[GPU_TASK] hw init OK: %dx%d buf=%p len=%lu blk=%d\n",
           g_ctx.info.width, g_ctx.info.height,
           (void *)buf, (unsigned long)len, g_ctx.block_height);
}

/* ── 发送 SET_AREA 给 LCD 任务 ── */
static void gpu_send_set_area(int x_start, int x_end, int y_start, int y_end)
{
    lcd_flush_msg_t qmsg;
    memset(&qmsg, 0, sizeof(qmsg));
    qmsg.cmd       = LCD_CMD_SET_AREA;
    qmsg.x_start   = x_start;
    qmsg.x_end     = x_end;
    qmsg.y_start   = y_start;
    qmsg.y_end     = y_end;
    DBG("[GPU_TASK] queue push SET_AREA: x[%d,%d] y[%d,%d]\n",
        x_start, x_end, y_start, y_end);
    render_queue_push(&qmsg);
}

/* ── 发送 FLUSH 给 LCD 任务 (通过渲染队列) ── */
static void gpu_post_to_lcd(void *buf, int x_start, int x_end,
                            int y_start, int y_end, int is_continue,
                            int is_ext_malloc, int is_last_block)
{
    lcd_flush_msg_t qmsg;
    memset(&qmsg, 0, sizeof(qmsg));
    qmsg.cmd           = LCD_CMD_FLUSH;
    qmsg.buf           = (u8 *)buf;
    qmsg.x_start       = x_start;
    qmsg.x_end         = x_end;
    qmsg.y_start       = y_start;
    qmsg.y_end         = y_end;
    qmsg.is_continue   = is_continue;
    qmsg.is_ext_malloc = is_ext_malloc;
    qmsg.is_last_block = is_last_block;
    DBG("[GPU_TASK] queue push FLUSH: buf=%p x[%d,%d] y[%d,%d] cont=%d ext=%d last=%d\n",
        (void *)buf, x_start, x_end, y_start, y_end, is_continue, is_ext_malloc, is_last_block);
    render_queue_push(&qmsg);
}

/* ── 获取渲染 buf (异步模式下可能 malloc) ──
 *   return 非 NULL buf   is_ext_malloc 指示 buf 是否来自 malloc
 */
static u8 *gpu_acquire_buf(int draw_w, int draw_h, int *is_ext_malloc)
{
    u8 *buf;

    *is_ext_malloc = 0;

    /* 优先从 buffer_manager 获取空闲 buf */
    if (g_ctx.buffer_hdl) {
        buf = get_idle_buffer_to_lock(g_ctx.buffer_hdl);
        if (buf) {
            return buf;
        }
    }

    /* 全异步模式：无空闲 buf 时 malloc */
    if (GPU_USED_UI_HW_ASYNC) {
        int size = draw_w * draw_h * 2;     /* RGB565: 每像素 2 字节 */
        buf = (u8 *)malloc(size);
        if (buf) {
            *is_ext_malloc = 1;
            DBG("[GPU_TASK] malloc ext buf: %p, %d bytes\n", (void *)buf, size);
        }
        return buf;
    }

    /* 半同步模式：死等 */
    {
        u32 t_start = jiffies_msec();
        do {
            buf = get_idle_buffer_to_lock(g_ctx.buffer_hdl);
            if (!buf) {
                u32 t_now = jiffies_msec();
                if (jiffies_msec2offset(t_start, t_now) > 500) {
                    buffer_manager_status_dump(g_ctx.buffer_hdl);
                    t_start = t_now;
                }
            }
        } while (!buf);
        return buf;
    }
}

/* ── 归还 buf: 半同步模式归还到 pool, 异步模式 pool buf 归 pool 外部 buf free ── */
static void gpu_release_buf(u8 *buf, int is_ext_malloc)
{
    if (g_ctx.buffer_hdl && !is_ext_malloc) {
        set_buffer_idle(g_ctx.buffer_hdl, buf);
    }
    /* is_ext_malloc buf: lcd_task 负责 free */
}

/* ── 任务入口 ── */
static void gpu_task(void *p)
{
    int ret, msg[32] = {0};

    /* GPU 硬件初始化 */
    gpu_hw_init();

    /* UI→GPU 同步信号量: GPU 渲染完后 post, UI 在创建新帧前 pend */
    os_sem_create(&g_ctx.gpu_done_sem, 0);

    /*
     * LCD→GPU 同步信号量声明 (实现在 lcd_task.c):
     * lcd_wait_frame_done() 由 gpu_task 在渲染前调用, 等待上一帧推屏完成。
     */
    extern int lcd_wait_frame_done(u32 timeout_ms);

    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ || ret == OS_TIMEOUT) {
            continue;
        }
        DBG("[GPU_TASK] pend: msg[0]=%d\n", msg[0]);

        switch (msg[0]) {
        case GPU_MSG_RENDER: {
            pJLGPUChainHead_t head = (pJLGPUChainHead_t)(u32)msg[1];
            struct rect area;
            int block_height;
            int block_number;
            int i;

            if (!head) {
                break;
            }

            /*
             * LCD→GPU 同步: 等待上一帧所有分块推屏完毕再启动本次渲染,
             * 防止 GPU 渲染覆盖仍被 DBI 硬件使用的显存 buffer。
             * 首帧 frame_done_sem 初始值=1 直接通过。
             */
            lcd_wait_frame_done(0);

            memcpy(&area, jlgpu_chain_head_win_rect(head), sizeof(area));

            block_height = g_ctx.block_height;

            DBG("[GPU_TASK] recv UI: head=%p area=(%d,%d %dx%d) blk=%d\n",
                (void *)head, area.left, area.top, area.width, area.height,
                block_height);

            /* 计算分块数 */
            block_number = (area.height % block_height)
                           ? ((area.height / block_height) + 1)
                           : (area.height / block_height);

            /* 先发送 SET_AREA 设置整个帧的推屏区域 */
            gpu_send_set_area(area.left, area.left + area.width - 1,
                              area.top,  area.top + area.height - 1);

            IO_REDRAW_HIGH();
            for (i = 0; i < block_number; i++) {
                int draw_height;
                struct rect draw_rect;
                u8  *idle_buf;
                int  is_ext_malloc;

                /* 当前分块高度 */
                if ((i + 1) * block_height < area.height) {
                    draw_height = block_height;
                } else {
                    draw_height = area.height - (i * block_height);
                }

                /* 本次 GPU 合成区域 (相对于 area 左上角) */
                rect_set_val(&draw_rect, area.left,
                             area.top + i * block_height,
                             area.width, draw_height);

                /* 获取渲染 buf */
                idle_buf = gpu_acquire_buf(area.width, draw_height,
                                           &is_ext_malloc);
                if (!idle_buf) {
                    DBG("[GPU_TASK] acquire buf failed! block=%d\n", i);
                    continue;
                }

                /* 清空显存残留 */
                memset(idle_buf, 0x00, area.width * draw_height * 2);

                /* 设置 GPU 输出 → 启动合成 */
                jlgpu_set_task_list_out_buf(head, idle_buf, &draw_rect, 0, 0);
                jlgpu_task_list_run(head);

                /* buffer_manager 管理的 buf 标记为等待推屏 */
                if (!is_ext_malloc) {
                    set_buffer_pend(g_ctx.buffer_hdl, idle_buf);
                }

                /* 计算推屏坐标并投递给 LCD 任务 */
                {
                    int x_start = area.left;
                    int x_end   = area.left + area.width - 1;
                    int y_start = area.top + i * block_height;
                    int y_end   = y_start + draw_height - 1;
                    int is_last = (i == block_number - 1);

                    gpu_post_to_lcd(idle_buf,
                                    x_start, x_end, y_start, y_end,
                                    i /* 0=first block → kistart */,
                                    is_ext_malloc,
                                    is_last);
                }
            }

            /* 帧渲染结束, 释放任务链 */
            jlgpu_delete_task_list_head(head);
            IO_REDRAW_LOW();

            /*
             * GPU→UI 同步: 通知 UI 当前帧已渲染完毕，可以准备下一帧。
             * UI 通过 gpu_wait_render_done() 阻塞在此信号量上。
             */
            os_sem_post(&g_ctx.gpu_done_sem);
            break;
        }
        default:
            break;
        }
    }
}

/* ── 初始化 ── */
void gpu_task_init(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));

    int err = os_task_create(gpu_task, NULL, GPU_TASK_PRIO,
                             GPU_TASK_STK, GPU_TASK_Q_SZ, GPU_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("[GPU_TASK] create failed! err=%d\n", err);
    }
}

/**
 * @brief 等待 GPU 渲染完当前一帧 (GPU→UI 同步)
 *
 * UI 任务调用此函数阻塞等待，GPU 线程渲染完一帧所有分块后 post 信号量唤醒 UI。
 * 实现 UI → GPU → LCD 异步闭环: UI 等 GPU, GPU 等 LCD。
 *
 * @param timeout_ms  超时时间 (ms), 0=死等
 * @return 0=GPU完成, OS_TIMEOUT=超时
 */
int gpu_wait_render_done(u32 timeout_ms)
{
    return os_sem_pend(&g_ctx.gpu_done_sem, timeout_ms);
}

#endif /* GPU_DEMO_ASYN_DRAW */
