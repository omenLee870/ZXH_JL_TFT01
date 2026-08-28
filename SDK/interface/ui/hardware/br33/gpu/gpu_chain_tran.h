/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __GPU_CHAIN_TRAN_H__
#define __GPU_CHAIN_TRAN_H__

#include "gpu_chain.h"


/**
 * @brief 复制创建任务链表
 * @param new_head 新链表头
 * @param head 源链表头
 * @param matrix 变换矩阵
 * @param group 任务分组
 * @return 创建成功返回新链表头指针，失败返回NULL
 */
pJLGPUChainHead_t jlgpu_task_list_copy_create(pJLGPUChainHead_t new_head, pJLGPUChainHead_t head, gpu_matrix_t *matrix, int group);

/**
 * @brief 带背景色复制创建任务链表
 * @param backcolor 背景色
 * @param head 源链表头
 * @param matrix 变换矩阵
 * @param group 任务分组
 * @return 创建成功返回新链表头指针，失败返回NULL
 */
pJLGPUChainHead_t jlgpu_task_list_copy_create_with_backcolor(u32 backcolor, pJLGPUChainHead_t head, gpu_matrix_t *matrix, int group);

/**
 * @brief 根据分组更新变换矩阵
 * @param new_head 新链表头
 * @param head 源链表头
 * @param group 任务分组
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_update_matrix_by_group(pJLGPUChainHead_t new_head, pJLGPUChainHead_t head, int group);

/**
 * @brief 销毁任务链表副本
 * @param head 链表头
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_destroy(pJLGPUChainHead_t head);

/**
 * @brief 根据分组销毁任务链表副本
 * @param head 链表头
 * @param group 任务分组
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_destroy_by_group(pJLGPUChainHead_t head, int group);

/**
 * @brief 根据分组乘以变换矩阵
 * @param head 链表头
 * @param matrix 变换矩阵
 * @param group 任务分组
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_mul_matrix_by_group(pJLGPUChainHead_t head, gpu_matrix_t *matrix, int group);

/**
 * @brief 根据分组使能或禁用任务
 * @param head 链表头
 * @param group 任务分组
 * @param enable 使能标志，非0使能，0禁用
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_enable_by_group(pJLGPUChainHead_t head, int group, int enable);

/**
 * @brief 获取任务链表指令数量
 * @param head 链表头
 * @return 指令数量
 */
int jlgpu_task_list_copy_get_instruction_num(pJLGPUChainHead_t head);

/**
 * @brief 调整任务链表分组顺序
 * @param head 链表头
 * @param order_tab 顺序表
 * @param order_tab_size 顺序表大小
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_group_order_adjust(pJLGPUChainHead_t head, u8 *order_tab, int order_tab_size);

/**
 * @brief 重置任务链表分组全局透明度
 * @param head 链表头
 * @param order_tab 顺序表
 * @param global_alpha_tab 全局透明度表
 * @param tab_size 表大小
 * @return 成功返回0，失败返回错误码
 */
int jlgpu_task_list_copy_group_global_alpha_reset(pJLGPUChainHead_t head, u8 *order_tab, u8 *global_alpha_tab, int tab_size);


#endif

