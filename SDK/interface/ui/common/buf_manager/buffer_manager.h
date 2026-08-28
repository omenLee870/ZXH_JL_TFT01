/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__


/* ------------------------------------------------------------------------------------*/
/*
buffer 管理使用说明：
1. 本模块管理的buf有五种状态，除了INIT为初始化状态，其他状态之间可自由转换
2. 使用本模块之前，须先用 buffer_manager_callback 函数设置 malloc 和 free 回调函数
3. 本模块采用句柄方式管理buf，允许多个句柄共存。每个句柄下管理多个具体buf
4. 通过 buffer_manager_init 初始化buf，它将一个大buf平均分成多个小buf进行管理
5. 通过 buffer_manager_free 释放buf管理句柄，注意它只释放模块资源，对于管理的大buf不会进行释放，因为它是外部申请并传入进来管理的
6. 允许通过 set_default_buffer_handler 设置一个默认句柄，使用其他API时，句柄参数传入NULL，实现不好传递句柄的场景。但仅允许一个
7. 内联函数 get_idle_buffer_by_handler 用于等待并获取句柄中的idle状态buf，实现pend挂起CPU，避免CPU被占用卡住
8. 内联函数 wait_buf_to_status 用于等待指定buf转换到指定状态，其他地方将buf转为指定状态时该函数退出
9. 其他API功能参考函数上面说明


注意：
1. 本模块只负责以句柄方式管理buf状态及切换，不会对buf本身做任何处理
 */
/* ------------------------------------------------------------------------------------*/


/* 缓存buf的状态定义 */
typedef enum buffer_status_def {
    BUFFER_STATUS_INIT,		// 初始
    BUFFER_STATUS_IDLE,		// 空闲
    BUFFER_STATUS_LOCK,		// 锁定
    BUFFER_STATUS_PEND,		// 等待
    BUFFER_STATUS_USED,		// 使用
} BUFFER_STATUS;


/* buffer管理API快捷调用 */
// 注册buffer管理模块的回调
#define buffer_manager_callback(malloc, free) \
	buffer_manager_set_memory_callback(malloc, free)

// 初始化buffer管理，创建buffer管理句柄
#define buffer_manager_init(hdl, buf, size, block_num) \
{ \
	int BM_i = 0; \
	int BM_buf_size = (size) / (block_num); \
	u32 BM_buf_list[3] = {0}; \
	for (BM_i = 0; BM_i < (block_num); BM_i++) { \
		BM_buf_list[BM_i] = (u32)((buf) + (BM_buf_size * BM_i)); \
	} \
	(hdl) = buffer_manager_init_handler(BM_buf_list, (block_num)); \
}

// 释放buffer管理资源和句柄
#define buffer_manager_free(buffer_handler) \
	buffer_manager_free_handler(buffer_handler)

// 打印buffer管理的buf状态
#define buffer_manager_dump(hdl) \
	buffer_manager_status_dump(hdl)

// 设置指定buffer的状态
#define set_buffer_status(buffer_handler, buf_addr, status) \
	buffer_manager_set_buf_status(buffer_handler, buf_addr, status)

// 获取指定buffer的状态
#define get_buffer_status(buffer_handler, buf_addr) \
	buffer_manager_get_buf_status(buffer_handler, buf_addr)

// 获取指定状态的buffer
#define get_buffer_by_status(buffer_handler, status) \
	buffer_manager_get_buf_by_status(buffer_handler, status)

// 获取目标状态的buffer，并修改为结果状态
#define change_buffer_status(buffer_handler, target, result) \
	buffer_manager_change_buf_status(buffer_handler, target, result)


// 设置默认句柄，可以设置一个默认句柄，该句柄将作为buffer管理的静态变量，外部无需每次操作都传入该句柄
// 需要注意的时，buffer_manager_free时，可以传入NULL来释放默认句柄，但大buffer还需外面主动释放
// 即：buffer管理只做状态管理，和自身资源的申请、释放，不管被管理的buffer申请和释放
#define set_default_buffer_handler(buffer_handler) \
	buffer_manager_set_default_handler(buffer_handler)




/* 快捷获取指定状态的buffer */
// 获取空闲状态的buffer
#define get_idle_buffer(buffer_handler) \
	get_buffer_by_status(buffer_handler, BUFFER_STATUS_IDLE)

// 获取锁定状态的buffer
#define get_lock_buffer(buffer_handler) \
	get_buffer_by_status(buffer_handler, BUFFER_STATUS_LOCK)

// 获取等待状态的buffer
#define get_pend_buffer(buffer_handler) \
	get_buffer_by_status(buffer_handler, BUFFER_STATUS_PEND)

// 获取使用状态的buffer
#define get_used_buffer(buffer_handler) \
	get_buffer_by_status(buffer_handler, BUFFER_STATUS_USED)




/* 快捷获取指定状态的buffer，并修改buffer状态 */
// 获取空闲状态的buffer，并修改为锁定状态
#define get_idle_buffer_to_lock(buffer_handler) \
	change_buffer_status(buffer_handler, BUFFER_STATUS_IDLE, BUFFER_STATUS_LOCK)

// 获取锁定状态的buffer，并修改为等待状态
#define get_lock_buffer_to_pend(buffer_handler) \
	change_buffer_status(buffer_handler, BUFFER_STATUS_LOCK, BUFFER_STATUS_PEND)

// 获取等待状态的buffer，并修改为使用状态
#define get_pend_buffer_to_used(buffer_handler) \
	change_buffer_status(buffer_handler, BUFFER_STATUS_PEND, BUFFER_STATUS_USED)

// 获取使用状态的buffer，并修改为空闲状态
#define get_used_buffer_to_idle(buffer_handler) \
	change_buffer_status(buffer_handler, BUFFER_STATUS_USED, BUFFER_STATUS_IDLE)




/* 快捷设置指定buffer的状态 */
// 设置buffer状态为空闲
#define set_buffer_idle(buffer_handler, buf) \
	set_buffer_status(buffer_handler, buf, BUFFER_STATUS_IDLE)

// 设置buffer状态为锁定
#define set_buffer_lock(buffer_handler, buf) \
	set_buffer_status(buffer_handler, buf, BUFFER_STATUS_LOCK)

// 设置buffer状态为等待
#define set_buffer_pend(buffer_handler, buf) \
	set_buffer_status(buffer_handler, buf, BUFFER_STATUS_PEND)

// 设置buffer状态为使用
#define set_buffer_used(buffer_handler, buf) \
	set_buffer_status(buffer_handler, buf, BUFFER_STATUS_USED)



// 等待buffer转为idle状态
#define wait_buf_to_idle(hdl, buf, sem, timeout) \
	wait_buf_to_status(hdl, buf, sem, timeout, BUFFER_STATUS_IDLE)

// 等待buffer转为lock状态
#define wait_buf_to_lock(hdl, buf, sem, timeout) \
	wait_buf_to_status(hdl, buf, sem, timeout, BUFFER_STATUS_LOCK)

// 等待buffer转为pend状态
#define wait_buf_to_pend(hdl, buf, sem, timeout) \
	wait_buf_to_status(hdl, buf, sem, timeout, BUFFER_STATUS_PEND)

// 等待buffer转为used状态
#define wait_buf_to_used(hdl, buf, sem, timeout) \
	wait_buf_to_status(hdl, buf, sem, timeout, BUFFER_STATUS_USED)



// buffer管理原生API，不建议直接调用原生API，
// 防止后续库升级导致调用方法改变，使用时请调用重封装的快捷调用
int buffer_manager_set_default_handler(void *buffer_hdl);

void buffer_manager_set_memory_callback(void *(*malloc)(int size, u32 ram_type, u32 module_type), void (*free)(void *p, u32 ram_type, u32 module_type));

void *buffer_manager_init_handler(u32 buf_list[], int buf_num);

void buffer_manager_free_handler(void *buffer_hdl);

int buffer_manager_set_buf_status(void *buffer_hdl, void *buf, int status);

int buffer_manager_get_buf_status(void *buffer_hdl, void *buf);

void *buffer_manager_get_buf_by_status(void *buffer_hdl, int status);

void *buffer_manager_change_buf_status(void *buffer_hdl, int target, int result);

void *buffer_manager_status_dump(void *buffer_hdl);

int buffer_manager_append_node(void *buffer_hdl, u8 *buf_addr);

int buffer_manager_remove_node(void *buffer_hdl, u8 *buf_addr);


/* 快捷调用：追加/删除buf节点 */
#define buffer_manager_add_node(hdl, buf) \
	buffer_manager_append_node(hdl, (u8 *)(buf))

#define buffer_manager_del_node(hdl, buf) \
	buffer_manager_remove_node(hdl, (u8 *)(buf))


/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_idle_buffer_by_handler 从句柄获取idle状态的buf，异步等待，有时间限制
 *
 * @Params hdl buffer管理句柄
 * @Params sem 外部定义的信号量，用于pend挂起当前线程，避免死等CPU被卡住
 * @Params timeout 超时时间，超过此时间会抛出一个超时打印，打印出句柄内所有buf状态
 * @Params count 超时次数，0时死等，其他超时超过此次数时，会跳出等待循环，此时可能未成功获取buf
 *
 * @return NULL 获取idle buf失败，其他 idle状态的buf
 */
/* ------------------------------------------------------------------------------------*/
static inline void *get_idle_buffer_by_handler(void *hdl, OS_SEM *sem, int timeout, int count)
{
    void *idle_buf = NULL;
    unsigned int time_start = jiffies_msec();
    unsigned int time_count = 0;

    do {
        idle_buf = get_idle_buffer_to_lock(hdl); // 尝试获取idle_buf
        if (!idle_buf && sem) {
            os_sem_pend(sem, 1); // 如果当前没有idle_buf，释放CPU给其他逻辑使用
        }

        unsigned int time_run = jiffies_msec();
        if (jiffies_msec2offset(time_start, time_run) >= timeout) {
            buffer_manager_dump(hdl); // 超时没获取到idle_buf，打印所有buf状态
            time_start = time_run;

            if (count && (++time_count >= count)) {
                break; // 如果超时次数超过限制，强制退出
            }
        }
    } while (!idle_buf);

    return idle_buf;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief wait_buf_to_status 等待句柄中的指定buf到转换到指定状态
 *
 * @Params hdl buf管理句柄
 * @Params buf 需等待转换状态的buf地址
 * @Params sem 外部定义的信号量，避免CPU被卡住
 * @Params timeout 超时时间，超过本时间时会打印一次hdl中所有buf状态
 * @Params s 目标状态，当buf转换到本状态时，本函数退出
 */
/* ------------------------------------------------------------------------------------*/
static inline void wait_buf_to_status(void *hdl, void *buf, OS_SEM *sem, int timeout, BUFFER_STATUS s)
{
    unsigned int time_start = jiffies_msec();

    do {
        if ((get_buffer_status(hdl, buf) != s) && sem) {
            os_sem_pend(sem, 1); // 挂起当前任务，释放CPU
        }
        unsigned int time_run = jiffies_msec();
        if (jiffies_msec2offset(time_start, time_run) > timeout) {
            buffer_manager_dump(hdl);
            time_start = time_run;
        }
    } while (get_buffer_status(hdl, buf) != s);
}


#endif


