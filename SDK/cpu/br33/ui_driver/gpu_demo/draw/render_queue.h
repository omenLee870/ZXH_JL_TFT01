/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    render_queue.h
 * @brief   GPU→LCD 渲染队列抽象层（声明）
 */

#ifndef __RENDER_QUEUE_H__
#define __RENDER_QUEUE_H__

#include "gpu_demo.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

#include "includes.h"
#include "gpu_demo_draw_cfg.h"

/* ── 队列命令类型 ── */
enum {
    LCD_CMD_SET_AREA = 0,  /* 设置推屏区域 (lcd_set_draw_area) */
    LCD_CMD_FLUSH    = 1,  /* 推屏: kistart 或 continue */
};

/* ── 队列元素 ── */
typedef struct {
    int  cmd;           /* LCD_CMD_SET_AREA / LCD_CMD_FLUSH */
    u8  *buf;           /* 显存 buffer 地址 (FLUSH时有效) */
    int  x_start;       /* 推屏 X 起始 */
    int  x_end;         /* 推屏 X 结束 */
    int  y_start;       /* 推屏 Y 起始 */
    int  y_end;         /* 推屏 Y 结束 */
    int  is_continue;   /* FLUSH时: 0=kistart首块, ≥1=continue续传 */
    int  is_ext_malloc; /* FLUSH时: 0=buf来自buffer_manager, 1=malloc */
    int  is_last_block; /* FLUSH时: 1=帧末块, lcd_task据此调push_complete */
} lcd_flush_msg_t;

/* ── 队列 API ── */
int render_queue_init(void);
int render_queue_push(const lcd_flush_msg_t *msg);
int render_queue_pend(lcd_flush_msg_t *msg, u32 timeout_ms);

#endif /* GPU_DEMO_ASYN_DRAW */
#endif /* __RENDER_QUEUE_H__ */
