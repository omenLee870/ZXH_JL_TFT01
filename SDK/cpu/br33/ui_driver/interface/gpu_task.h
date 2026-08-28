#ifndef __PUBLIC_GPU_TASK_H__
#define __PUBLIC_GPU_TASK_H__


/* ------------------------------------------------------------------------------------*/
/**
 * @brief GPU 任务名
 */
/* ------------------------------------------------------------------------------------*/
#define GPU_TASK_NAME	"gpu"


/* ------------------------------------------------------------------------------------*/
/**
 * @brief GPU任务消息类型
 */
/* ------------------------------------------------------------------------------------*/
typedef enum {
    GPU_MSG_OTHER,
    GPU_MSG_DRAW,
    GPU_MSG_DUMP,
    GPU_MSG_DRAW_LIST,
} gpu_task_msg_t;


/* ------------------------------------------------------------------------------------*/
/**
 * @brief GPU刷新方式标志
 */
/* ------------------------------------------------------------------------------------*/
typedef enum {
    GPU_SYNC_REDRAW,	// 同步/串行刷新
    GPU_ASYN_REDRAW,	// 异步/并行刷新
} gpu_redraw_mode_t;

int jlgpu_scheduler_async_free_buf(void *p);
void jlgpu_scheduler_set_redraw_mode(struct draw_context *dc, gpu_redraw_mode_t mode);
void *jlgpu_scheduler_create_mult_task_list(struct draw_context *dc);
void jlgpu_scheduler_delete_mult_task_list(struct draw_context *dc);
void *jlgpu_scheduler_draw_mult_task_list(struct draw_context *dc);
void jlgpu_scheduler_init(void *(*malloc)(int, u32, u32), void (*free)(void *, u32, u32), u16 lcd_width, u16 lcd_height);
void jlgpu_scheduler_free();
int jlgpu_scheduler_wait_sync();
int jlgpu_scheduler_async_free_jpeg_res(void *p);
int jlgpu_scheduler_get_redraw_mode();

// 影子任务链切换
#include "gpu_port.h"
pJLGPUMultTaskList_t jlgpu_mult_task_list_get_common_root();
pJLGPUMultTaskList_t jlgpu_mult_task_list_switch_root(pJLGPUMultTaskList_t mult);
int jlgpu_mult_task_list_check(pJLGPUMultTaskList_t mult);
pJLGPUMultTaskList_t jlgpu_mult_task_list_sync_shadow(pJLGPUMultTaskList_t mult);


#endif


