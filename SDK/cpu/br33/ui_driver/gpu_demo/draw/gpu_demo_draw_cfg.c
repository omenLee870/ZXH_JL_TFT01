/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    gpu_demo_draw_cfg.c
 * @brief   GPU→LCD 流水线 Demo 配置常量定义
 */

#include "gpu_demo_draw_cfg.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

/** @brief 1=OS任务队列, 0=自定义环形缓冲 */
const int GPU_QUEUE_USE_OS_Q = 1;

/** @brief 1=整帧buffer, 0=分块buffer */
const int GPU_USED_FRAME_BUF = 0;

/** @brief 1=GPU+DBI完全异步(不等待flush_sem), 0=半同步 */
const int GPU_USED_UI_HW_ASYNC = 1;

#endif /* GPU_DEMO_ASYN_DRAW */
