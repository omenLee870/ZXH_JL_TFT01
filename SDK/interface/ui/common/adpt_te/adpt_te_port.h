/**
 * @file    adpt_te_port.h
 * @brief   TE自适应同步模块 — 硬件移植接口 + 编译期配置
 *
 * 【移植指南】
 *   本文件是移植时唯一需要修改的头文件。请按以下步骤操作：
 *
 *   1. 在 "编译期配置宏" 区域设置所有宏参数（TE 边沿、超时、延迟等）
 *   2. 在你的平台代码中实现本文件声明的 13 个接口函数
 *   3. 在 TE GPIO ISR 中调用 adpt_te_isr_te_signal()
 *   4. 在 DMA 完成 ISR 中调用 adpt_te_isr_push_complete()
 *
 *   模块不管理显存缓冲区，不负责 DMA 启动。
 *   App 自行管理显存和推屏，模块只提供 TE 等待同步 + 自适应跳帧。
 *
 *   关于 TE 边沿：
 *     模块不关心 TE 信号是上升沿还是下降沿触发。App 自行配置 GPIO
 *     中断触发边沿，当 TE 信号到达时在 ISR 中调用 adpt_te_isr_te_signal()
 *     即可，模块内部视此次调用为一次有效的 TE 信号到达。
 */

#ifndef __ADPT_TE_PORT_H__
#define __ADPT_TE_PORT_H__

#include <stdint.h>
#include <stddef.h>

/*===========================================================================
 * 第一部分：编译期配置宏（移植者必须根据实际情况定义/修改）
 *===========================================================================*/

/* ─────────────────────────────────────────────
 * [1] TE 信号参数
 * ───────────────────────────────────────────── */

/* TE 信号默认周期（us），冷启动初始估计。60Hz = 16667us */
#ifndef ADPT_TE_PORT_DEFAULT_TE_PERIOD_US
#define ADPT_TE_PORT_DEFAULT_TE_PERIOD_US  16667
#endif

/* TE 信号最小周期（us），低于此值视为毛刺/干扰。200Hz max */
#ifndef ADPT_TE_PORT_MIN_TE_PERIOD_US
#define ADPT_TE_PORT_MIN_TE_PERIOD_US      5000
#endif

/* TE 信号最大周期（us），高于此值视为异常。20Hz min */
#ifndef ADPT_TE_PORT_MAX_TE_PERIOD_US
#define ADPT_TE_PORT_MAX_TE_PERIOD_US      50000
#endif

/* TE 超时阈值（us），wait_te 阻塞超过此时间 → 返回超时错误 */
#ifndef ADPT_TE_PORT_TE_TIMEOUT_US
#define ADPT_TE_PORT_TE_TIMEOUT_US         100000
#endif

/* ─────────────────────────────────────────────
 * [2] 推屏延迟（TE 到达后延迟指定时间再返回 wait_te）
 *     设为 0 表示不延迟。
 *     典型值：0~500 us，用于确保 LCD 内部已准备好接收数据。
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_PUSH_DELAY_US
#define ADPT_TE_PORT_PUSH_DELAY_US         0
#endif

/* ─────────────────────────────────────────────
 * [3] TE 相位（TE 信号到 LCD 扫描行 0 的时间差）
 *     0 = TE 与 LCD 第 0 行对齐（最常见）。
 *     非 0 = TE 信号在 LCD 已扫描若干行后触发。
 *     单位 us，取值 0 ~ (TE_period - 1)。
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_TE_PHASE_US
#define ADPT_TE_PORT_TE_PHASE_US           0
#endif

/* ─────────────────────────────────────────────
 * [4] 推屏安全余量百分比
 *     预测推屏耗时 × 此系数 vs 剩余安全窗口 → 决定是否直接推屏。
 *     115 = 1.15 倍（15% 安全余量），推荐范围 100~130。
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_PUSH_MARGIN_PCT
#define ADPT_TE_PORT_PUSH_MARGIN_PCT       115
#endif

/* ─────────────────────────────────────────────
 * [5] VBlank 安全写入窗口（us）
 *     TE 信号在 VBlank 开始时产生。VBlank 窗口是安全写入 GRAM
 *     的唯一时机，通常 1000~3000us。此参数作为安全写入的时间上限，
 *     推屏耗时应在此窗口内完成，否则数据可能写入可见区造成撕裂。
 *     典型值：2000（2ms）
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_VBLANK_WINDOW_US
#define ADPT_TE_PORT_VBLANK_WINDOW_US      2000
#endif

/* ─────────────────────────────────────────────
 * [6] EMA 平滑系数 α（Q8 定点格式，0~255）
 *     32 ≈ 0.125（推荐值，中等平滑度）
 *     64 ≈ 0.25 （响应更快，但抗噪性差）
 *     16 ≈ 0.0625（更平滑，但响应慢）
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_EMA_ALPHA
#define ADPT_TE_PORT_EMA_ALPHA             32
#endif

/* ─────────────────────────────────────────────
 * [7] LCD 复位超时（TE 信号长时间丢失 → 重新初始化 LCD）
 *
 *     当 wait_te 检测到 TE 连续丢失超过此时间，自动调用 LCD 复位回调。
 *     设为 0 表示禁用 LCD 复位功能。
 *     典型值：1000000（1秒），必须 > ADPT_TE_PORT_TE_TIMEOUT_US
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_LCD_RESET_TIMEOUT_US
#define ADPT_TE_PORT_LCD_RESET_TIMEOUT_US     0       /* 0=禁用 */
#endif

/* ─────────────────────────────────────────────
 * [8] 推屏耗时 EMA 平滑系数 α（Q8 定点格式，0~255）
 *     用于平滑预测推屏耗时，避免单次抖动影响同步决策。
 *     64 ≈ 0.25（响应较快，适合推屏波动场景）
 *     32 ≈ 0.125（中等平滑，与 TE 周期 EMA 一致）
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_PUSH_EMA_ALPHA
#define ADPT_TE_PORT_PUSH_EMA_ALPHA        64
#endif

/* ─────────────────────────────────────────────
 * [9] LCD 复位定时器间隔（ms）
 *     TE 丢失超过 lcd_reset_timeout_us 后，创建定时器定时触发 LCD 复位。
 *     设为 0 表示不在定时器中复位（禁用定时器复位功能）。
 *     典型值：100（100ms），即每 100ms 复位一次 LCD 直到 TE 恢复。
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_LCD_RESET_TIMER_MS
#define ADPT_TE_PORT_LCD_RESET_TIMER_MS     200
#endif

/* ─────────────────────────────────────────────
 * [10] TE 信号预测安全阈值（us）
 *      wait_te 在判定"不可立即推屏"后、进入信号量阻塞等待前，
 *      预测下一个 TE 信号的到达时间。若剩余等待时间 ≤ 此阈值，
 *      则跳过信号量等待，直接允许推屏，省去 sem_pend→ISR→sem_post 的开销。
 *
 *      设为 0 表示禁用预测：必须等到真实 TE 信号才允许推屏。
 *      设为 2000 表示若距下一个 TE ≤ 2ms，无需等待。
 *
 *      典型值：0（禁用）或 500~2000（轻度预测）
 *      注意：此值应显著小于 TE 周期（例如 60Hz=16667us 时设为 2000），
 *            过大的值会削弱 TE 同步效果，可能导致撕裂。
 * ───────────────────────────────────────────── */
#ifndef ADPT_TE_PORT_PREDICT_SAFE_US
#define ADPT_TE_PORT_PREDICT_SAFE_US        0       /* 0=禁用预测 */
#endif

/* ─────────────────────────────────────────────
 * [11] 调试开关
 *     ADPT_TE_DEBUG 未定义 → 日志输出编译为空（零开销）
 * ───────────────────────────────────────────── */
#define ADPT_TE_DEBUG

/*===========================================================================
 * 第二部分：LCD 复位回调类型（由应用层实现，通过 adpt_te_register_lcd_reset_cb 注册）
 *===========================================================================*/

/**
 * @brief   LCD 硬件复位回调函数类型
 * @note    当 TE 信号连续丢失超过 ADPT_TE_PORT_LCD_RESET_TIMEOUT_US 时，
 *          模块在 wait_te() 上下文中自动调用此回调。回调应完成：
 *            1. 复位 LCD 硬件（拉低 RESET 引脚）
 *            2. 重新发送 LCD 初始化命令序列
 *            3. 重新配置 TE 输出
 *          回调在 App 调用 wait_te() 的线程上下文中执行，允许耗时操作。
 */
typedef void (*adpt_te_lcd_reset_cb_t)(void);

/*===========================================================================
 * 第三部分：平台层必须实现的接口（由移植者实现，共 13 个）
 *
 * 所有函数必须线程安全（可能在 ISR 和 App 任务上下文中被调用）。
 *
 *   1.  adpt_te_port_get_time_us       — us 时间戳
 *   2.  adpt_te_port_delay_us          — us 延时
 *   3.  adpt_te_port_sem_create        — 创建信号量
 *   4.  adpt_te_port_sem_delete        — 删除信号量
 *   5.  adpt_te_port_sem_pend          — 等待信号量（带超时）
 *   6.  adpt_te_port_sem_post          — 发送信号量（ISR安全）
 *   7.  adpt_te_port_enter_critical    — 进入临界区
 *   8.  adpt_te_port_exit_critical     — 退出临界区
 *   9.  adpt_te_port_log               — 日志输出
 *  10.  adpt_te_port_timer_create      — 创建 LCD 复位定时器
 *  11.  adpt_te_port_timer_delete      — 删除 LCD 复位定时器
 *===========================================================================*/

/* ── 时间 ── */

/** @brief 获取系统当前时间戳（us），返回 unsigned long 类型，允许溢出回绕 */
unsigned long adpt_te_port_get_time_us(void);

/**
 * @brief   微秒级延时（用于推屏延迟）
 * @param   us  延时微秒数（通常 0~500）
 * @note    实现可用硬件定时器延时或软件空循环。
 *          此函数在 App 线程上下文中调用，非 ISR。
 */
void adpt_te_port_delay_us(unsigned long us);

/* ── 信号量（用于 wait_te 阻塞等待）── */

/**
 * @brief   创建计数信号量（初始值 = 0）
 * @return  信号量句柄，NULL 表示失败
 * @note    uCOS-II: OSSemCreate(0)
 */
uint8_t adpt_te_port_sem_create(void);

/**
 * @brief   删除信号量
 * @param   sem   adpt_te_port_sem_create() 的返回值
 */
void adpt_te_port_sem_delete();

/**
 * @brief   等待信号量（阻塞，带超时）
 * @param   sem         信号量句柄
 * @param   timeout_ms  超时时间（ms），0 = 永久等待
 * @return  1 = 获取到信号量，0 = 超时
 * @note    uCOS-II: OSSemPend(sem, timeout_ms / OS_TICKS_PER_SEC, &err)
 *          超时时 err == OS_ERR_TIMEOUT 返回 0，否则返回 1
 */
int adpt_te_port_sem_pend(unsigned long timeout_ms);

/**
 * @brief   发送信号量（ISR 安全，在 TE GPIO ISR 中调用）
 * @param   sem   信号量句柄
 * @note    uCOS-II: OSSemPost(sem)，ISR 中可直接调用
 */
void adpt_te_port_sem_post();

/* ── 临界区 ── */

/** @brief 进入临界区，返回保存的状态 */
unsigned long adpt_te_port_enter_critical(void);

/** @brief 退出临界区 */
void adpt_te_port_exit_critical(unsigned long key);

/* ── 日志 ── */

/** @brief 日志输出（未定义 ADPT_TE_DEBUG 时为空函数） */
void adpt_te_port_log(const char *fmt, ...);

/* ── LCD 复位定时器 ── */

/**
 * @brief   创建 LCD 复位定时器（单次或周期，由移植层决定）
 * @param   timeout_ms  定时器超时时间（ms）
 * @param   callback    定时器到期回调
 * @return  定时器 ID（> 0），0 表示创建失败
 * @note    回调中应完成 LCD 硬件复位和初始化。
 *          若定时器是单次的，每次触发后需重新创建。
 */
uint16_t adpt_te_port_timer_create(unsigned long timeout_ms, void (*callback)(void));

/**
 * @brief   删除 LCD 复位定时器
 * @param   timer_id  adpt_te_port_timer_create() 返回的 ID
 */
void adpt_te_port_timer_delete(uint16_t timer_id);

#endif /* __ADPT_TE_PORT_H__ */
