#ifndef _CLOCK_MANAGER_
#define _CLOCK_MANAGER_

#include "typedef.h"
// #include "asm/clock.h"


// int clk_set(const char *name, int clk);
/* --------------------------------------------------------------------------*/
/**
 * @brief clock_alloc，此函数会触发时钟频率设置
 *
 * @param name
 * @param clk
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_alloc(const char *name, u32 clk);


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_free，此函数会触发时钟频率设置
 *
 * @param name
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_free(char *name);


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_dump
 */
/* ----------------------------------------------------------------------------*/
void clock_manager_dump(void);


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_refurbish，当CPU运行代码有改变时候，建议调用此函数刷新时钟频率
 */
/* ----------------------------------------------------------------------------*/
void clock_refurbish(void);


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_lock_deal
 *
 * @param name: owner name
 * @param clk: lock_freq
 * @param check: check is locked
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_lock_deal(const char *name, u32 clk, u8 check);
int clock_lock(const char *name, u32 clk); // 检查是否已经锁定
int clock_force_lock(const char *name, u32 clk); // 不检查，强制设定时钟频率

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_unlock
 *
 * @param name: owner name
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_unlock(char *name);

#endif //_CLOCK_MANAGER_
