/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    ui_task.c
 * @brief   UI 渲染任务 — 定时触发，轮换 DEMO 页面，生成 GPU 任务链投递给 GPU 任务
 *
 * 流水线: [UI Task] → (GPU task chain) → [GPU Task] → (rendered buffer) → [LCD Task]
 */

#include "gpu_demo.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

#include "includes.h"

#include "ui_measure.h"

#define DBG(...)
/* #define DBG printf */

#define UI_TASK_NAME    "ui_task"
#define UI_TASK_PRIO    7
#define UI_TASK_STK     2048
#define UI_TASK_Q_SZ    256

/* ── 定时 + 页面轮换 ── */
#define UI_REFR_MS      1000    /* 定时刷新间隔 (ms) */
#define UI_TICK_PER_PAGE 2      /* 每个页面持续的 tick 数 */

/* 页面尺寸 (需与 ui_pipeline_demo.c 中 PAGE_W/PAGE_H 一致) */
#define PAGE_W          466
#define PAGE_H          466

/* 背景填充色 */
#define UI_BG_COLOR     ARGB8888(0xff, 0x40, 0x40, 0x40)

/* ── 消息类型 ── */
enum {
    UI_MSG_RENDER = 0,
};

/* ── 页面绘制函数 (定义在 ui_pipeline_demo.c) ── */
extern pJLGPUChainHead_t draw_page_1(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t draw_page_2(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t draw_page_3(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t draw_page_4(pJLGPUChainHead_t head, struct rect *screen);

/* ── gpu_task 渲染完成同步接口 (定义在 gpu_task.c) ──
 *   UI 任务调用此函数阻塞等待，GPU 渲染完一帧后 post 唤醒 UI。 */
extern int gpu_wait_render_done(u32 timeout_ms);

/*
 * 页面函数指针类型: 传入 head + screen, 返回 draw_head (可能 == head)
 */
typedef pJLGPUChainHead_t (*draw_page_fn_t)(pJLGPUChainHead_t, struct rect *);

/*
 * 页面表: 按其顺序循环切换。
 * 增删页面只需修改此数组, 无需改动 ui_task() 主循环。
 */
static const draw_page_fn_t page_table[] = {
    draw_page_1,
    draw_page_2,
    draw_page_3,
    draw_page_4,
};
#define PAGE_COUNT  ((int)(sizeof(page_table) / sizeof(page_table[0])))

/*
 * 页面函数指针类型: 传入 head + screen, 返回 draw_head (可能 == head)
 */
typedef pJLGPUChainHead_t (*draw_page_fn_t)(pJLGPUChainHead_t, struct rect *);

/*
 * 页面表: 按其顺序循环切换。
 * 增删页面只需修改此数组, 无需改动 ui_task() 主循环。
 */
static const draw_page_fn_t page_table[] = {
    /* draw_page_1, */
    /* draw_page_2, */
    /* draw_page_3, */
    draw_page_4,
};
#define PAGE_COUNT  ((int)(sizeof(page_table) / sizeof(page_table[0])))

static int  tick_count;
static int  page_index;
static u32  refr_timer_id;

/* ── 创建背景填充任务 (清空显存残留数据) ── */
static void create_bg_task(pJLGPUChainHead_t head, struct rect *screen)
{
    u8 a = ARGB8888_A(UI_BG_COLOR);
    u8 r = ARGB8888_R(UI_BG_COLOR);
    u8 g = ARGB8888_G(UI_BG_COLOR);
    u8 b = ARGB8888_B(UI_BG_COLOR);

    jlgpu_task_fill_param_init(0, 0, a, r, g, b);
    jlgpu_task_fill_set_draw(rect_left(screen), rect_top(screen),
                             rect_width(screen), rect_height(screen));
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen),
                             rect_width(screen), rect_height(screen));
    jlgpu_update_task_by_id(head, 0, 0, &task_param);
}

/* ── 定时器回调 — 触发 UI 渲染 ── */
static void ui_refr_timer(void *p)
{
    int msg[2] = {UI_MSG_RENDER, 0};
    int err = os_taskq_post_type(UI_TASK_NAME, msg[0], 2, msg);
    DBG("[UI_TASK] timer post: msg=%d err=%d\n", msg[0], err);
}

/* ── 任务入口 ── */
static void ui_task(void *p)
{
    int ret, msg[32] = {0};
    struct rect screen;

    /* 页面区域 */
    screen.left   = 0;
    screen.top    = 0;
    screen.width  = PAGE_W;
    screen.height = PAGE_H;

    /* 创建定时器，间隔 UI_REFR_MS 触发渲染 */
    refr_timer_id = sys_timer_add(NULL, ui_refr_timer, UI_REFR_MS);

    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        DBG("[UI_TASK] pend: msg[0]=%d page=%d tick=%d\n",
            msg[0], page_index, tick_count);

        switch (msg[0]) {
        case UI_MSG_RENDER: {
            pJLGPUChainHead_t draw_head;
            pJLGPUChainHead_t head;

            /*
             * GPU→UI 同步: 等待 GPU 渲染完当前一帧后再准备下一帧,
             * 避免 GPU 渲染使用到的显存 buf 在 GPU 完成前被新帧覆盖。
             * 首次进入 gpu_done_sem=0 → 首帧的 GPU post 后后续帧才能通过。
             */
            if (tick_count > 0) {
                gpu_wait_render_done(1000);
            }

            IO_FRAME_HIGH();
            /* 页面轮换 */
            if (++tick_count >= UI_TICK_PER_PAGE) {
                tick_count = 0;
                page_index = (PAGE_COUNT > 0)
                             ? ((page_index + 1) % PAGE_COUNT) : 0;
            }

            /* 创建 GPU 任务链 */
            head = jlgpu_create_task_list_head();
            jlgpu_set_task_list_out_format(head, JLGPU_OUT_FORMAT_RGB565);

            /* 设置任务链渲染区域 (gpu_task 从此读取分块依据) */
            jlgpu_chain_head_set_win_rect(head, &screen);

            /* 背景填充 */
            create_bg_task(head, &screen);

            /* 调用当前页面: 数组越界+空指针双防护 */
            if (PAGE_COUNT > 0 && page_index < PAGE_COUNT
                && page_table[page_index]) {
                draw_head = page_table[page_index](head, &screen);
            } else {
                draw_head = NULL;
            }

            if (!draw_head) {
                draw_head = head;
            }

            /* jlgpu_chain_dump_all_task(draw_head, __func__, __LINE__); */
            /* 投递给 gpu_task: 仅传 head 指针，渲染区域从 head->win_rect 获取 */
            {
                int m[2];
                m[0] = (int)(u32)draw_head;
                m[1] = 0;
                DBG("[UI_TASK] post to gpu: head=%p\n", (void *)(u32)m[0]);
                os_taskq_post_type("gpu_task", UI_MSG_RENDER, 1, m);
            }
            IO_FRAME_LOW();
            break;
        }
        default:
            break;
        }
    }
}

/* ── 初始化 ── */
void ui_task_init(void)
{
    tick_count  = 0;
    page_index  = 0;
    refr_timer_id = 0;

    int err = os_task_create(ui_task, NULL, UI_TASK_PRIO,
                             UI_TASK_STK, UI_TASK_Q_SZ, UI_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("[UI_TASK] create failed! err=%d\n", err);
    }
}

#endif /* GPU_DEMO_ASYN_DRAW */
