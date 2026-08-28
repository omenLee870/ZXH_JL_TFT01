#ifndef   OS_API_H
#define   OS_API_H


#ifdef __cplusplus
extern "C"
{
#endif




#include "generic/typedef.h"
#include "os/os_cpu.h"
#include "os/os_error.h"
#include "os/os_type.h"


typedef void *TaskHandle_t;






/* --------------------------------------------------------------------------*/
/**
 * @brief reserved
 */
/* ----------------------------------------------------------------------------*/
void os_init(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief reserved
 */
/* ----------------------------------------------------------------------------*/
void os_start(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief reserved
 */
/* ----------------------------------------------------------------------------*/
void os_init_tick(int);

/* --------------------------------------------------------------------------*/
/**
 * @brief 创建任务
 *
 * @param task 任务回调函数
 * @param p_arg 传递给任务回调函数的参数
 * @param prio 任务的优先级
 * @param stksize 任务的堆栈大小, 单位(u32)
 * @param qsize 任务的queue大小，单位(byte)
 * @param name 任务名 (名字长度不能超过configMAX_TASK_NAME_LEN字节)
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_create(void (*task)(void *p_arg),
                   void *p_arg,
                   u8 prio,
                   u32 stksize,
                   int qsize,
                   const char *name);

int os_task_create_affinity_core(void (*task)(void *p_arg),
                                 void *p_arg,
                                 u8 prio,
                                 u32 stksize,
                                 int qsize,
                                 const char *name,
                                 u8 core);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前任务名
 *
 * @return 当前任务名
 */
/* ----------------------------------------------------------------------------*/
const char *os_current_task();

/* --------------------------------------------------------------------------*/
/**
 * @brief 删除任务
 *
 * @param name 任务名
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del_req(const char *name);

/* --------------------------------------------------------------------------*/
/**
 * @brief 响应任务删除请求，标记资源已经释放，可以删除当前任
 *
 * @param name 任务名，任务自己可以用OS_TASK_SELF
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del_res(const char *name);

/* --------------------------------------------------------------------------*/
/**
 * @brief 删除任务
 *
 * @param name 任务名
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del(const char *name);


/* --------------------------------------------------------------------------*/
/**
 * @brief 延时。中断函数或者关闭系统总中断的情况下不能调用此函数
 *
 * @param time_tick 延时时间
 */
/* ----------------------------------------------------------------------------*/
void os_time_dly(int time_tick);


/* --------------------------------------------------------------------------*/
/**
 * @brief 发送Q_USER类型taskq
 *
 * @param name 任务名
 * @param argc 后面传入的参数的个数。发送的最大参数个数限制为8个int类型
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post(const char *name, int argc, ...);

/* --------------------------------------------------------------------------*/
/**
 * @brief 非阻塞方式查询taskq
 *
 * @param argc 最大可获取的queue长度，单位(int)
 * @param argv 存放queue的buf
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_accept(int argc, int *argv);

/* --------------------------------------------------------------------------*/
/**
 * @brief 阻塞方式获取taskq
 *
 * @param fmt 保留，传NULL
 * @param argv 存放queue的buf
 * @param argc 最大可获取的queue长度，单位(int)
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_pend(const char *fmt, int *argv, int argc);
// int os_task_pend(const char *fmt, int *argv, int argc);
// int __os_taskq_pend(int *argv, int argc, int tick);

/* --------------------------------------------------------------------------*/
/**
 * @brief 超时方式获取taskq
 *
 * @param fmt 保留，传NULL
 * @param argv 存放queue的buf
 * @param argc 最大可获取的queue长度，单位(int)
 * @param tick 超时时间, 单位: tick(10ms)
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_pend_timeout(const char *fmt, int *argv, int argc, int tick);


/* --------------------------------------------------------------------------*/
/**
 * @brief 发送指定类型的taskq
 *
 * @param name 任务名
 * @param type queue类型
 * @param argc 后面传入的参数的个数
 * @param argv
 *
 * @return 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_type(const char *name, int type, int argc, int *argv);


/* --------------------------------------------------------------------------*/
/**
 * @brief 发送Q_MSG类型的taskq
 *
 * @param name 任务名
 * @param argc 后面参数的个数
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_msg(const char *name, int argc, ...);


/* --------------------------------------------------------------------------*/
/**
 * @brief 发送Q_EVENT类型的taskq
 *
 * @param name 任务名
 * @param argc 后面参数的个数
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_event(const char *name, int argc, ...);


/* --------------------------------------------------------------------------*/
/**
 * @brief 删除指定类型的taskq
 *
 * @param name 任务名
 * @param type taskq的类型
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_del_type(const char *name, int type);

/* --------------------------------------------------------------------------*/
/**
 * @brief 清除所有taskq
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_flush(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 创建信号量
 *
 * @param sem 信号量
 * @param int 初始计数值
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_create(OS_SEM *, int);

/* --------------------------------------------------------------------------*/
/**
 * @brief 非阻塞方式查询信号量
 *
 * @param sem 信号量
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_accept(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 阻塞方式获取信号量
 *
 * @param sem 信号量
 * @param timeout 等待时长，0表示一直等待
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_pend(OS_SEM *, int timeout);

/* --------------------------------------------------------------------------*/
/**
 * @brief 发送信号量
 *
 * @param sem 信号量
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_post(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 信号量删除
 *
 * @param sem 信号量
 * @param block 保留
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_del(OS_SEM *, int block);

/* --------------------------------------------------------------------------*/
/**
 * @brief 信号量设置
 *
 * @param sem 信号量
 * @param cnt 计数值
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_set(OS_SEM *, u16 cnt);

/* --------------------------------------------------------------------------*/
/**
 * @brief 信号量类型是否queueQUEUE_TYPE_COUNTING_SEMAPHORE
 *
 * @param true:信号量匹配, fail:信号量不匹配
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_valid(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 判断信号量是否可用
 *
 * @param sem 信号量
 *
 * @reutrn 可用数量
 */
/* ----------------------------------------------------------------------------*/
int os_sem_query(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 创建互斥量
 *
 * @param mutex 互斥量
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_create(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 非阻塞方式查询互斥量
 *
 * @param mutex:互斥量
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_accept(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 阻塞方式查询互斥量
 *
 * @param mutex 互斥量
 * @param timeout 等待时间，0表示一直等待
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_pend(OS_MUTEX *, int timeout);

/* --------------------------------------------------------------------------*/
/**
 * @brief 发送斥量
 *
 * @param mutex 互斥量
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_post(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief 删除斥量
 *
 * @param mutex 互斥量
 * @param block 保留
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_del(OS_MUTEX *, int block);

/* --------------------------------------------------------------------------*/
/**
 * @brief 互斥量类型是否queueQUEUE_TYPE_MUTEX
 *
 * @param true:互斥量匹配, fail:互斥量不匹配
 *
 * @reutrn 错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_valid(OS_MUTEX *);



/**
 *  @brief enum kill_mode
 *  used by thread_kill
 */
enum kill_mode {
    KILL_WAIT,		/*!< WAIT */
    KILL_REQ,		/*!< REQUEST	*/
    KILL_FORCE,		/*!< FORCE ON */
};


/*----------------------------------------------------------------------------*/
/**@brief  创建thread_fork_kill线程
   @return 0: 成功
   @return other: enum OS_ERROR_CODES
   @note   该线程用来Kill掉其他线程，其任务优先级最高
*/
/*----------------------------------------------------------------------------*/
int thread_fork_init(void);

/*----------------------------------------------------------------------------*/
/**@brief  创建一个线程（扩展接口）
   @param[in] condition: 线程开始运行的前提条件（条件回调返回非0值开始运行）
   @param[in] thread_name: 标识此线程的字符串
   @param[in] prio: 优先级
   @param[in] stk_size: 堆栈空间的大小（以四个字节为一个单位）
   @param[in] q_size: 消息队列空间的大小（以四个字节为一个单位）
   @param[out] pid: 保存新线程的pid
   @param[in] func: 入口功能函数
   @param[in] parm: 入口函数的传入参数
   @param[in] thread_complete_cb: 线程结束回调
   @param[in] thread_complete_cb_parm: 线程结束回调的传入参数
   @return 0: 成功
   @return -1: 参数错误
   @return other: enum OS_ERROR_CODES
   @note   由于系统靠thread_name区分不同任务，所以请保证thread_name的唯一
   @note   replace pthread_create function
*/
/*----------------------------------------------------------------------------*/
int thread_rpc(int (*condition)(void), const char *thread_name, int prio, int stk_size, u32 q_size, int *pid, void (*func)(void *), void *parm, void (*thread_complete_cb)(void *), void *thread_complete_cb_parm);

/*----------------------------------------------------------------------------*/
/**@brief  创建一个线程
   @param[in] thread_name: 标识此线程的字符串
   @param[in] prio: 优先级
   @param[in] stk_size: 堆栈空间的大小（以四个字节为一个单位）
   @param[in] q_size: 消息队列空间的大小（以四个字节为一个单位）
   @param[out] pid: 保存新线程的pid
   @param[in] func: 入口功能函数
   @param[in] parm: 入口函数的传入参数
   @return 0: 成功
   @return -1: 参数错误
   @return other: enum OS_ERROR_CODES
   @note   由于系统靠thread_name区分不同任务，所以请保证thread_name的唯一
   @note   replace pthread_create function
*/
/*----------------------------------------------------------------------------*/
int thread_fork(const char *thread_name, int prio, int stk_size, u32 q_size, int *pid, void (*func)(void *), void *parm);

int thread_fork_multiple(const char *thread_prefix_name, int prio, int stk_size, u32 q_size, int *pid, void (*func)(void *), void *parm);

/*----------------------------------------------------------------------------*/
/**@brief  杀死一个线程
   @param[in]  pid: 指向需要杀死的pid号的指针
   @param[in]  mode: enum kill_mode
   @return 0: 成功
   @return -1: 参数错误
   @return other: enum OS_ERROR_CODES
   @note   推荐使用KILL_WAIT
   @note   replace pthread_kill function
*/
/*----------------------------------------------------------------------------*/
void thread_kill(volatile int *pid, enum kill_mode mode);

/*----------------------------------------------------------------------------*/
/**@brief  获取正在运行的线程是否有其他线程请求自己杀死
   @return 0: 没有杀死请求
   @return 1: 有杀死请求
   @note
*/
/*----------------------------------------------------------------------------*/
int thread_kill_req(void);


/*struct os_msg *os_message_create(int size);

int os_message_receive(struct os_msg **msg, int block_time);

int os_message_send(const char *task_name, struct os_msg *msg, int msgflg);

int os_message_delete(struct os_msg *msg);*/



int os_q_create(OS_QUEUE *pevent, /*void **start, */QS size);

int os_q_del(OS_QUEUE *pevent, u8 opt);

int os_q_flush(OS_QUEUE *pevent);

int os_q_pend(OS_QUEUE *pevent, int timeout, void *msg);

int os_q_post(OS_QUEUE *pevent, void *msg);

int os_q_query(OS_QUEUE *pevent);

int os_q_valid(OS_QUEUE *pevent);

int task_queue_post_event(const char *name, void *data, int len);

void *os_task_get_handle(const char *name);

void os_suspend_other_core(void);

void os_resume_other_core(void);

void os_system_info_output(void);

u8 os_idle_percentage(void);


#ifdef __cplusplus
}
#endif
#endif
