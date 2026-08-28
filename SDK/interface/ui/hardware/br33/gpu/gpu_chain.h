/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#ifndef __GPU_CHAIN_H__
#define __GPU_CHAIN_H__


/**
 * @brief GPU任务ID或控件ID，表示无效或不存在
 */
#define	JLGPU_ID_NONE	(-1)

/**
 * @brief GPU任务链状态枚举
 */
enum HEAD_STATUS {
    HEAD_STATUS_IDLE,   /**< 空闲状态，链表未就绪 */
    HEAD_STATUS_READY,  /**< 就绪状态，链表可渲染 */
    HEAD_STATUS_BUSY,   /**< 繁忙状态，链表在渲染 */
    HEAD_STATUS_USED,   /**< 已用状态，链表已渲染 */
};


/**
 * @brief GPU任务拓展信息结构体
 * @details 存储GPU任务的基本属性和变换矩阵信息
 */
typedef struct gpu_task_info {
    u32 task_id;    /**< 任务ID */
    u32 element_id; /**< 控件ID */

    u32 image_width: 11;  /**< 纹理实际宽度 */
    u32 image_height: 11; /**< 纹理实际高度 */
    u32 group: 10;        /**< 任务分组 */

    jl_path_t *path;      /**< 矢量信息 */

    jlgpu_matrix_t *matrix; /**< 变换矩阵 */
    jlgpu_matrix_t *path_m; /**< 矢量路径 */
    jlgpu_matrix_t *grad0m; /**< 渐变 0矩阵 */
    jlgpu_matrix_t *grad1m; /**< 渐变 1矩阵 */

    u32 rev1: 19;			/**< 预留位 */
    u32 free_cache: 1;      /**< 是否由GPU释放数据buf */
    u32 priority: 12;		/**< 优先级，值越大越上层 */

    u8 *clut_tab;  			/**< 颜色表 */
    void *custom_data;		/**< 自定义参数，用于拓展 */

} JLGPUTaskInfo_t, *pJLGPUTaskInfo_t;


/**
 * @brief GPU链表任务单元结构体
 * @details 任务链表中的节点单元，包含任务信息和指令地址
 */
typedef struct gpu_chain_task_unit {
    JLGPUTaskInfo_t info; /**< 任务信息 */
    u32 task_addr;        /**< 任务指令 */
} JLGPUChainTaskUnit_t, *pJLGPUChainTaskUnit_t;

#define jlgpu_chain_task_expand_info(taskp)			(&(taskp)->info)  /**< 获取GPU任务拓展信息指针 */
#define jlgpu_chain_task_task_id(taskp)				((taskp)->info.task_id)  /**< 获取GPU任务ID */
#define jlgpu_chain_task_element_id(taskp)			((taskp)->info.element_id)  /**< 获取GPU控件ID */
#define jlgpu_chain_task_image_width(taskp)			((taskp)->info.image_width)  /**< 获取GPU任务纹理实际宽度 */
#define jlgpu_chain_task_image_height(taskp)		((taskp)->info.image_height)  /**< 获取GPU任务纹理实际高度 */
#define jlgpu_chain_task_group(taskp)				((taskp)->info.group)  /**< 获取GPU任务分组序号 */
#define jlgpu_chain_task_free_cache(taskp)			((taskp)->info.free_cache)  /**< 获取GPU任务释放缓存标志 */
#define jlgpu_chain_task_priority(taskp)			((taskp)->info.priority)  /**< 获取GPU任务优先级 */
#define jlgpu_chain_task_path(taskp)				((taskp)->info.path)  /**< 获取GPU任务矢量路径 */
#define jlgpu_chain_task_custom_data(taskp)	        ((taskp)->info.custom_data)  /**< 获取GPU任务自定义数据 */
#define jlgpu_chain_task_tran_matrix(taskp)			((taskp)->info.matrix)  /**< 获取GPU任务变换矩阵 */
#define jlgpu_chain_task_path_matrix(taskp)			((taskp)->info.path_m)  /**< 获取GPU任务路径矩阵 */
#define jlgpu_chain_task_grad0_matrix(taskp)		((taskp)->info.grad0m)  /**< 获取GPU任务渐变0矩阵 */
#define jlgpu_chain_task_grad1_matrix(taskp)		((taskp)->info.grad1m)  /**< 获取GPU任务渐变1矩阵 */

#define jlgpu_chain_task_set_task_id(t, i)			jlgpu_chain_task_task_id(t) = (i)  /**< 设置GPU任务ID */
#define jlgpu_chain_task_set_element_id(t, i)		jlgpu_chain_task_element_id(t) = (i)  /**< 设置GPU控件ID */
#define jlgpu_chain_task_set_image_width(t, w)		jlgpu_chain_task_image_width(t) = (w)  /**< 设置GPU任务纹理实际宽度 */
#define jlgpu_chain_task_set_image_height(t, h)		jlgpu_chain_task_image_height(t) = (h)  /**< 设置GPU任务纹理实际高度 */
#define jlgpu_chain_task_set_group(t, g)			jlgpu_chain_task_group(t) = (g)  /**< 设置GPU任务分组序号 */
#define jlgpu_chain_task_set_free_cache(t, f)		jlgpu_chain_task_free_cache(t) = (f)  /**< 设置GPU任务释放缓存标志 */
#define jlgpu_chain_task_set_priority(t, p)			jlgpu_chain_task_priority(t) = (p)  /**< 设置GPU任务优先级 */
#define jlgpu_chain_task_set_path(t, p)				jlgpu_chain_task_path(t) = (p)  /**< 设置GPU任务矢量路径 */
#define jlgpu_chain_task_set_custom_data(t, d)		jlgpu_chain_task_custom_data(t) = (d)  /**< 设置GPU任务自定义数据 */
#define jlgpu_chain_task_set_tran_matrix(t, m)		jlgpu_chain_task_tran_matrix(t) = (m)  /**< 设置GPU任务变换矩阵 */
#define jlgpu_chain_task_set_path_matrix(t, m)		jlgpu_chain_task_path_matrix(t) = (m)  /**< 设置GPU任务路径矩阵 */
#define jlgpu_chain_task_set_grad0_matrix(t, m)		jlgpu_chain_task_grad0_matrix(t) = (m)  /**< 设置GPU任务渐变0矩阵 */
#define jlgpu_chain_task_set_grad1_matrix(t, m)		jlgpu_chain_task_grad1_matrix(t) = (m)  /**< 设置GPU任务渐变1矩阵 */


/**
 * @brief GPU链表结构头
 * @details 管理整个GPU任务链的头部信息，包含渲染区域、状态和任务指针
 */
typedef struct gpu_chain_task_head {
    struct rect win_rect;        /**< 渲染区域，默认全屏，启动渲染时会设置给GPU */
    struct rect gpu_rect;        /**< GPU区域，杰理UI特效使用 */
    struct rect lcd_rect;        /**< LCD区域，杰理UI特效使用 */
    enum HEAD_STATUS status;     /**< 链表状态 */

    void *render;	  /**< 渲染参数，如果有配置，运行任务链时会使用，否则使用默认的配置 */
    void *base_task;  /**< 首指令地址 */
    void *tail_task;  /**< 尾指令地址 */
} JLGPUChainHead_t, *pJLGPUChainHead_t;

#define jlgpu_chain_head_status(head)				((head)->status)  /**< 获取链表状态 */
#define jlgpu_chain_head_render(head)				((pJLGPURenderCfg_t)(((pJLGPUChainHead_t)(head))->render))  /**< 获取链表渲染配置指针 */
#define jlgpu_chain_head_out_format(head)			jlgpu_render_out_fmt(jlgpu_chain_head_render(head))  /**< 获取链表输出格式 */
#define jlgpu_chain_head_win_rect(head)				(&(head)->win_rect)  /**< 获取链表渲染区域 */
#define jlgpu_chain_head_gpu_rect(head)				(&(head)->gpu_rect)  /**< 获取链表GPU区域 */
#define jlgpu_chain_head_lcd_rect(head)				(&(head)->lcd_rect)  /**< 获取链表LCD区域 */
#define jlgpu_chain_base_task_addr(head)			((pJLGPUChainHead_t)head)->base_task  /**< 获取链表首指令地址 */
#define jlgpu_chain_base_task_unit(head)			jlgpu_chain_task_get_unit(jlgpu_chain_base_task_addr(head))  /**< 获取链表首任务单元 */
#define jlgpu_chain_tail_task_addr(head)			((pJLGPUChainHead_t)head)->tail_task  /**< 获取链表尾指令地址 */
#define jlgpu_chain_tail_task_unit(head)			jlgpu_chain_task_get_unit(jlgpu_chain_tail_task_addr(head))  /**< 获取链表尾任务单元 */

#define jlgpu_chain_head_set_status(head, s)		jlgpu_chain_head_status(head) = (s)  /**< 设置链表状态 */
#define jlgpu_chain_head_set_render(head, r)		((head)->render) = (void*)(r)  /**< 设置链表渲染配置指针 */
#define jlgpu_chain_set_base_task(head, addr)		jlgpu_chain_base_task_addr(head) = (addr)  /**< 设置链表首指令地址 */
#define jlgpu_chain_set_tail_task(head, addr)		jlgpu_chain_tail_task_addr(head) = (addr)  /**< 设置链表尾指令地址 */

/**
 * @brief 初始化链表任务指针（首尾指令地址置空）
 */
#define jlgpu_chain_task_init(head) \
{ \
	jlgpu_chain_set_base_task(head, NULL); \
	jlgpu_chain_set_tail_task(head, NULL); \
}

/**
 * @brief 设置链表输出格式，参考 jlgpu_def.h 中 jlgpu_out_format_t
 */
#define jlgpu_chain_head_set_out_format(head, f) \
	jlgpu_render_set_out_fmt(jlgpu_chain_head_render(head), f)

/**
 * @brief 设置链表输出缓冲区
 */
#define jlgpu_chain_head_set_out_buffer(head, buf, rect, left, stride) \
	jlgpu_render_set_out_buf(jlgpu_chain_head_render(head), buf, rect, left, stride)

/**
 * @brief 设置链表渲染区域
 */
#define jlgpu_chain_head_set_win_rect(head, r) \
	memcpy(jlgpu_chain_head_win_rect(head), r, sizeof(struct rect));

/**
 * @brief 设置链表GPU区域
 */
#define jlgpu_chain_head_set_gpu_rect(head, r) \
	memcpy(jlgpu_chain_head_gpu_rect(head), r, sizeof(struct rect));

/**
 * @brief 设置链表LCD区域
 */
#define jlgpu_chain_head_set_lcd_rect(head, r) \
	memcpy(jlgpu_chain_head_lcd_rect(head), r, sizeof(struct rect));

/* 定义断点中断回调函数类型 */
typedef void (*jlgpu_bp_callback)(pJLGPUChainHead_t head, pJLGPUChainTaskUnit_t taskp);

/* 定义自定义数据释放回调 */
typedef void (*jlgpu_custom_data_free)(pJLGPUChainTaskUnit_t taskp, void *data);

/**
 * @brief 获取链表任务info长度
 * @return 任务信息结构体大小
 */
#define jlgpu_chain_task_info_len()		(sizeof(JLGPUTaskInfo_t)/*  >> 2 */)

/**
 * @brief 获取链表任务指令地址
 * @param unit 任务单元指针
 * @return 任务指令地址，如果unit为NULL则返回NULL
 */
static inline void *
jlgpu_chain_task_get_addr(pJLGPUChainTaskUnit_t unit)
{
    return (unit) ? ((void *)&unit->task_addr) : (NULL);
}

/**
 * @brief 获取链表任务结构地址
 * @param addr 任务指令地址
 * @return 任务单元指针，如果addr为NULL则返回NULL
 */
static inline pJLGPUChainTaskUnit_t
jlgpu_chain_task_get_unit(void *addr)
{
    return (addr) ? ((pJLGPUChainTaskUnit_t)((u32)addr - jlgpu_chain_task_info_len())) : (NULL);
}



/* @partA =================================<< 链表API >>================================= */
/**
 * @brief 创建GPU任务链表头
 * @return 创建成功返回链表头指针，失败返回NULL
 */
pJLGPUChainHead_t jlgpu_chain_create_head();

/**
 * @brief 删除GPU任务链表头，会释放掉所有GPU任务指令
 * @param head 链表头指针
 */
void jlgpu_chain_delete_head(pJLGPUChainHead_t head);

/**
 * @brief 将任务追加到链表头部
 * @param head 链表头指针
 * @param taskp 待追加的任务单元
 */
void jlgpu_chain_append_base_task(pJLGPUChainHead_t head, pJLGPUChainTaskUnit_t taskp);

/**
 * @brief 将任务追加到链表尾部
 * @param head 链表头指针
 * @param taskp 待追加的任务单元
 */
void jlgpu_chain_append_tail_task(pJLGPUChainHead_t head, pJLGPUChainTaskUnit_t taskp);

/**
 * @brief 获取链表任务的下一个任务
 * @param taskp 当前任务单元
 * @return 下一个任务单元指针，NULL为无下一个任务
 */
pJLGPUChainTaskUnit_t jlgpu_chain_task_get_next(pJLGPUChainTaskUnit_t taskp);

/**
 * @brief 设置链表中当前任务的下一个任务，原本任务全部向后移一个位置
 * @param curr 当前任务单元
 * @param next 下一个任务单元
 */
void jlgpu_chain_task_set_next(pJLGPUChainTaskUnit_t curr, pJLGPUChainTaskUnit_t next);

/**
 * @brief 创建GPU任务单元
 * @param param 任务参数配置
 * @return 创建成功返回任务单元指针，失败返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_create_task(pJLGPUTaskParam_t param);

/**
 * @brief 删除GPU任务单元
 * @param taskp 待删除的任务单元
 * @return 若有下一个任务，则返回下一个任务单元指针，否则返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_delete_task(pJLGPUChainTaskUnit_t taskp);

/**
 * @brief 创建并追加任务到链表
 * @param head 链表头指针
 * @param param 任务参数配置
 * @return 创建成功返回任务单元指针，失败返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_append_task(pJLGPUChainHead_t head, pJLGPUTaskParam_t param);

/**
 * @brief 将任务插入到链表，会根据taskp任务的优先级插入到指定位置
 * @param head 链表头指针
 * @param taskp 待插入的任务单元
 */
void jlgpu_chain_insert_task(pJLGPUChainHead_t head, pJLGPUChainTaskUnit_t taskp);

/**
 * @brief 使能或禁用任务
 * @param taskp 任务单元
 * @param enable 使能标志，非0使能，0禁用
 */
void jlgpu_chain_enable_task(pJLGPUChainTaskUnit_t taskp, int enable);

/**
 * @brief 清空链表中所有任务，只清空所有任务，不释放链表头
 * @param head 链表头指针
 */
void jlgpu_chain_clear_all_task(pJLGPUChainHead_t head);

/**
 * @brief 统计链表中任务数量
 * @param head 链表头指针
 * @return 任务数量
 */
int jlgpu_chain_count_all_task(pJLGPUChainHead_t head);

/**
 * @brief 对链表中所有任务根据优先级进行重新排序
 * @param head 链表头指针
 */
void jlgpu_chain_reorder_all_task(pJLGPUChainHead_t head);

/**
 * @brief 设置链表中所有任务的使能状态
 * @param head 链表头指针
 * @param enable 使能标志，非0使能，0禁用
 */
void jlgpu_chain_set_all_task_enable(pJLGPUChainHead_t head, int enable);

/**
 * @brief 复制创建任务单元
 * @param taskp 源任务单元
 * @param shared_cache 是否共享缓存，非0共享，0不共享
 * @return 创建成功返回新任务单元指针，失败返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_copy_create_task(pJLGPUChainTaskUnit_t taskp, uint8_t shared_cache);

/**
 * @brief 复制创建链表头
 * @param head 源链表头
 * @param copy_task 是否复制任务，非0复制，0不复制
 * @return 创建成功返回新链表头指针，失败返回NULL
 */
pJLGPUChainHead_t jlgpu_chain_copy_create_head(pJLGPUChainHead_t head, uint8_t copy_task);


/* @partB =================================<< 任务API >>================================= */
/**
 * @brief 根据ID查找任务
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 * @return 找到返回任务单元指针，未找到返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_search_task_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id);

/**
 * @brief 根据ID使能或禁用任务
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 * @param enable 使能标志，非0使能，0禁用
 */
void jlgpu_chain_enable_task_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id, int enable);

/**
 * @brief 根据ID删除任务
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 */
void jlgpu_chain_delete_task_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id);

/**
 * @brief 根据ID创建并追加任务
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 * @param param 任务参数配置
 * @return 创建成功返回任务单元指针，失败返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_append_task_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id, pJLGPUTaskParam_t param);

/**
 * @brief 根据ID更新任务
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 * @param param 任务参数配置
 * @return 更新成功返回任务单元指针，失败返回NULL
 */
pJLGPUChainTaskUnit_t jlgpu_chain_update_task_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id, pJLGPUTaskParam_t param);

/**
 * @brief 根据ID设置任务优先级
 * @param head 链表头指针
 * @param task_id 任务ID
 * @param elm_id 控件ID
 * @param priority 优先级
 * @param reorder 是否重新排序，非0重新排序，0不排序
 */
void jlgpu_chain_set_task_prior_by_id(pJLGPUChainHead_t head, u32 task_id, u32 elm_id, u32 priority, int reorder);

/**
 * @brief 根据ID清理任务，清理elm_id相同，但task_id不同且task_id按顺序递增的任务
 * @param head 链表头指针
 * @param elm_id 控件ID
 * @param task_index 待清理的任务ID起始索引
 */
void jlgpu_chain_clean_up_task_by_id(pJLGPUChainHead_t head, u32 elm_id, u8 task_index);

/**
 * @brief 渲染任务链表
 * @param head 链表头指针
 */
void jlgpu_chain_render_head(pJLGPUChainHead_t head);

/**
 * @brief 设置GPU断点中断回调，当GPU运行到断点任务时会调用此回调
 * @param cb GPU断点中断回调函数
 */
void jlgpu_chain_set_breakpoint_cb(jlgpu_bp_callback cb);

/**
 * @brief 设置GPU释放自定义数据回调，当GPU指令释放时，如果回调和自定义参数存在则调用
 * @param cb GPU自定义数据释放函数
 */
void jlgpu_chain_set_custom_data_free_cb(jlgpu_custom_data_free cb);

/* @partC =================================<< 调试API >>================================= */
/**
 * @brief 打印任务链表头信息
 * @param head 链表头指针
 */
void jlgpu_chain_dump_head(pJLGPUChainHead_t head);

/**
 * @brief 打印链表任务单元
 * @param task 任务单元指针
 * @param count 打印数量
 */
void jlgpu_chain_dump_task_unit(pJLGPUChainTaskUnit_t task, int count);

/**
 * @brief 打印链表所有任务
 * @param head 链表头指针
 * @param func 调用函数名
 * @param line 调用行号
 */
void jlgpu_chain_dump_all_task(pJLGPUChainHead_t head, const char *func, int line);



#endif



