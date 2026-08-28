/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __GPU_CHAIN_MULT_H__
#define __GPU_CHAIN_MULT_H__

#include "gpu_chain.h"


/**
 * @brief 多链任务链表头列表单元
 * @details 存储单个任务链头指针，供多链容器管理
 */
typedef struct gpu_head_list {
    pJLGPUChainHead_t head;   /**< 任务链头指针 */
    // int identity;
} JLGPUMultHeadList_t, *pJLGPUMultHeadList_t;

/**
 * @brief GPU多链管理结构体
 * @details 管理多个GPU任务链的容器
 */
typedef struct gpu_mult_chain {
    struct list_head list;            /**< 链表节点 */
    JLGPUMultHeadList_t head_list[3];
    pJLGPUMultHeadList_t curr_head;
    pJLGPUMultHeadList_t last_head;
    pJLGPUChainHead_t head;        /**< GPU任务链头 */
    struct rect win_rect;      /**< 渲染区域 */
    struct rect gpu_rect;      /**< GPU区域 */
    struct rect lcd_rect;      /**< LCD区域 */
    int identity;                     /**< 任务链ID */
} JLGPUMultTaskList_t, *pJLGPUMultTaskList_t;


#define mult_chain_list(mult)        (&((mult)->list))                         /**< 链表节点成员，内部访问 */
#define mult_chain_id(mult)          (((pJLGPUMultTaskList_t)mult)->identity)  /**< 获取 GPU任务链表ID */
#define mult_chain_head(mult)	     (((pJLGPUMultTaskList_t)mult)->head)      /**< 获取 GPU任务链表头 */
#define mult_chain_curr_head(mult)	 (((pJLGPUMultTaskList_t)mult)->curr_head) /**< 获取 GPU任务链表头 */
#define mult_chain_last_head(mult)	 (((pJLGPUMultTaskList_t)mult)->last_head) /**< 获取 GPU任务链表头 */

#define mult_chain_set_id(mult, id)    mult_chain_id(mult) = (id)         /**< 设置 GPU任务链表ID */
#define mult_chain_set_head(mult, h)   mult_chain_head(mult) = (h)        /**< 设置 GPU任务链表头 */
#define mult_chain_set_curr_head(m, h) mult_chain_curr_head(m) = (h)        /**< 设置 GPU任务链表当前头 */
#define mult_chain_set_last_head(m, h) mult_chain_last_head(m) = (h)        /**< 设置 GPU任务链表上一个头 */


/**
 * @brief 创建多链单元句柄
 * @param head 任务链头
 * @param id   任务链ID
 * @return 创建成功返回句柄指针，失败返回NULL
 */
pJLGPUMultTaskList_t jlgpu_mult_chain_handler_create(pJLGPUChainHead_t head, u32 id);

/**
 * @brief 获取多链中任务链数量
 * @param mult 多链头指针
 * @return 任务链数量
 */
int jlgpu_mult_chain_get_head_number(pJLGPUMultTaskList_t mult);

/**
 * @brief 获取多链中所有任务链ID表
 * @param mult 多链头指针
 * @param index_tab 任务链ID数组
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_get_head_table(pJLGPUMultTaskList_t mult, int *index_tab);

/**
 * @brief 向多链追加任务链
 * @param mult 多链头指针
 * @param head 待追加的任务链头
 * @param id 待追加任务链ID
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_append_head(pJLGPUMultTaskList_t mult, pJLGPUChainHead_t head, u32 id);

/**
 * @brief 从多链删除任务链
 * @param mult 多链头指针
 * @param head 待删除的任务链头
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_delete_head(pJLGPUMultTaskList_t mult, pJLGPUChainHead_t head);

/**
 * @brief 清空多链中所有任务链
 * @param mult 多链头指针
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_clear_head(pJLGPUMultTaskList_t mult);

/**
 * @brief 影子复制多链
 * @param src 源多链
 * @param dst 目标多链
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_shadow_head(pJLGPUMultTaskList_t src, pJLGPUMultTaskList_t dst);

/**
 * @brief 根据ID获取任务链头
 * @param mult 多链头指针
 * @param id 任务链ID
 * @return 找到返回任务链头指针，未找到返回NULL
 */
pJLGPUChainHead_t jlgpu_mult_chain_get_head_by_id(pJLGPUMultTaskList_t mult, u32 id);

/**
 * @brief 根据ID设置任务链头
 * @param mult 多链头指针
 * @param id 任务链ID
 * @param new_head 新任务链头
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_set_head_by_id(pJLGPUMultTaskList_t mult, u32 id, pJLGPUChainHead_t new_head);

/**
 * @brief 根据ID删除任务链
 * @param mult 多链头指针
 * @param id 任务链ID
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_del_head_by_id(pJLGPUMultTaskList_t mult, u32 id);

/**
 * @brief 根据ID设置任务链矩形区域
 * @param mult 多链头指针
 * @param id 任务链ID
 * @param page_r 页面区域
 * @param gpu_r GPU区域
 * @param lcd_r LCD区域
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_set_head_rect_by_id(pJLGPUMultTaskList_t mult, u32 id, struct rect *page_r, struct rect *gpu_r, struct rect *lcd_r);

/**
 * @brief 根据ID获取任务链矩形区域
 * @param mult 多链头指针
 * @param id 任务链ID
 * @param page_r 页面区域输出
 * @param gpu_r GPU区域输出
 * @param lcd_r LCD区域输出
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_mult_chain_get_head_rect_by_id(pJLGPUMultTaskList_t mult, u32 id, struct rect *page_r, struct rect *gpu_r, struct rect *lcd_r);

/**
 * @brief 根据ID获取空闲任务链头
 * @param mult 多链头指针
 * @param id 任务链ID
 * @param cur_head 当前任务链头
 * @return 找到返回空闲任务链头指针，未找到返回NULL
 */
pJLGPUChainHead_t jlgpu_mult_chain_set_head_idle(pJLGPUMultTaskList_t mult, u32 id, pJLGPUChainHead_t cur_head);


/**
 * @brief 打印多链信息
 * @param mult 多链头指针
 */
void jlgpu_mult_chain_dump(pJLGPUMultTaskList_t mult);


#endif

