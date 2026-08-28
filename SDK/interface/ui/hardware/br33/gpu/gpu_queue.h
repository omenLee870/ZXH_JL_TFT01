#ifndef __GPU_QUEUE_H__
#define __GPU_QUEUE_H__



/**
 * @brief GPU队列任务单元结构体
 * @details 用于帧渲染队列的任务节点
 */
typedef struct gpu_queue_task_unit {
    struct gpu_queue_task_unit *next; /**< 下一个任务单元 */
    void *task_addr;                  /**< 任务地址 */
} JLGPUQueueTaskUnit_t, *pJLGPUQueueTaskUnit_t;



#endif

