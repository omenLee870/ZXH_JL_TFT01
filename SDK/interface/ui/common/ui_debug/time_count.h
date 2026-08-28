/* ------------------------------------------------------------------------------------*/
/**
 * @file time_count.h
 *
 * @brief 时间探针，用于粗略记录程序执行时间
 *
 * 使用方法：
 * 1. 在程序开始执行的位置调用 time_count_start；
 * 2. 在程序结束执行的位置调用 time_count_end；
 * 3. 保证 start 和 end 时传入的 id 一致、模块会自动在程序结束位置打印出执行时间，单位 us；
 * 4. 若打印出 Error 信息，说明 start 和 end 时传入的 id 不一致；
 * 5. 此方法仅用于粗略统计程序执行时间，非精确时间。可用于性能调试参考。
 * 注意：
 * 本模块接口仅应用层可以调用，库里调用会导致宏开关无法关闭，造成资源浪费。
 * 每start一个时间，会malloc 20byte空间占用，end时会释放
 *
 * @author
 *
 * @version V1.0.0
 *
 * @date 2025-03-18
 *
 * Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __TIME_COUNT_H__
#define __TIME_COUNT_H__

// 程序计数功能使能
#define TIME_COUNT_ENABLE	0


#if (defined TIME_COUNT_ENABLE && TIME_COUNT_ENABLE)

#define time_count_start(id) time_count_pushing(id)

#define time_count_end(id, pop) \
{                                                                     \
    if (pop) {                                                        \
        printf("%s(), %d, time count id: %d, runtime: %u us\n",          \
                __func__, __LINE__, id, time_count_popping(id, pop)); \
    } else {                                                          \
        time_count_popping(id, pop);                                  \
    }                                                                 \
}

#define time_count_dump() time_count_dumping()
#define time_count_clean(c) time_count_cleanup(c)

#else

#define time_count_start(id)
#define time_count_end(id, pop)
#define time_count_dump()
#define time_count_clean(c)

#endif


typedef enum {
    TIME_COUNT_CLR_END,  /* 仅清理已结束节点 */
    TIME_COUNT_CLR_ALL,  /* 清理所有节点 */
} timeCountClr_t;


/* ------------------------------------------------------------------------------------*/
/**
 * @brief time_count_pushing 将当前时间压入计时栈中
 *
 * @Params id 为当前时间命名的id，用于计时出栈时计算运行时间
 *
 * @return 当前时间
 */
/* ------------------------------------------------------------------------------------*/
u32 time_count_pushing(u16 id);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief time_count_popping 统计指定ID的程序段运行时间
 *
 * @Params id  需要统计时间的ID号
 * @Params pop true=出栈并释放, false=仅计算不出栈
 *
 * @return 指定ID的程序段运行时间(us)，ID未入栈则返回0
 */
/* ------------------------------------------------------------------------------------*/
u32 time_count_popping(u16 id, u8 pop);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief time_count_dumping 打印链表中所有计时节点的信息表格
 *
 * @note  打印格式: id | push_us | elapsed_us(当前已运行时间) | status(运行状态)
 *         使用 jiffies_usec2offset 计算运行时间以处理定时器回绕
 */
/* ------------------------------------------------------------------------------------*/
void time_count_dumping(void);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief time_count_cleanup 清理计时链表中指定状态的节点
 *
 * @Params flag TIME_COUNT_CLR_END  仅清理已结束节点(count_end==true)
 *              TIME_COUNT_CLR_ALL  清理全部节点(含运行中)
 */
/* ------------------------------------------------------------------------------------*/
void time_count_cleanup(timeCountClr_t flag);


#endif // !__TIME_COUNT_H__


