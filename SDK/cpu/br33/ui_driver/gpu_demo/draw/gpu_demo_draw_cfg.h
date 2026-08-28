/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/**
 * @file    gpu_demo_draw_cfg.h
 * @brief   GPU→LCD 流水线 Demo 统一配置（常量开关）
 */

#ifndef __GPU_DEMO_DRAW_CFG_H__
#define __GPU_DEMO_DRAW_CFG_H__

#include "gpu_demo.h"

#if (defined GPU_DEMO_ASYN_DRAW && GPU_DEMO_ASYN_DRAW)

/** @brief GPU→LCD 渲染队列实现: 1=OS任务队列, 0=自定义环形缓冲 */
extern const int GPU_QUEUE_USE_OS_Q;

/** @brief 整帧 buffer 模式: 1=使用整帧buffer, 0=使用分块buffer */
extern const int GPU_USED_FRAME_BUF;

/** @brief UI 硬件全异步模式: 1=GPU+DBI完全异步(不等待flush_sem), 0=半同步 */
extern const int GPU_USED_UI_HW_ASYNC;

#endif /* GPU_DEMO_ASYN_DRAW */
#endif /* __GPU_DEMO_DRAW_CFG_H__ */
