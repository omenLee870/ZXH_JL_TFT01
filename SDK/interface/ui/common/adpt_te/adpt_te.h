/**
 * @file    adpt_te.h
 * @brief   TE（Tearing Effect）自适应同步模块 — 公共接口
 *          （adpt = adaptive 的缩写）
 *
 * ============================ 模块概述 ============================
 *
 * 本模块实现 LCD TE 信号的自适应同步，消除画面撕裂(Tearing Effect)。
 *
 * 模块职责非常纯粹 — 只做 TE 信号同步和自适应跳帧，不管理显存，
 * 不执行推屏操作。App 自行管理显存缓冲区，自行调用 lcd_draw_kistart()
 * 启动推屏，模块仅在 App 准备推屏时提供"等待 TE 信号到达"的阻塞式同步。
 *
 *
 * 四个核心接口：
 * ┌─────────────────────────────────┬────────────────────────────────────────┐
 * │ adpt_te_init()              │ 初始化模块，创建信号量                 │
 * │ adpt_te_isr_te_signal()     │ TE GPIO ISR 回调（App ISR 中调用）     │
 * │ adpt_te_wait_te()           │ 阻塞等待 TE 信号 + 延迟 → 推屏许可返回 │
 * │ adpt_te_isr_push_complete() │ DMA 完成 ISR 回调（App ISR 中调用）    │
 * └─────────────────────────────────┴────────────────────────────────────────┘
 *
 *
 * 应用层伪代码：
 * @code
 *   adpt_te_init(&cfg);
 *   adpt_te_start();
 *   while (1) {
 *       render_frame(my_buf);
 *       adpt_te_wait_te(0);      // 阻塞直到 TE 到达
 *       lcd_draw_kistart(my_buf, 0, W-1, 0, H-1);  // App 自行推屏
 *       // ... DMA 完成 ISR 中调用 adpt_te_isr_push_complete()
 *   }
 * @endcode
 *
 *
 * 自适应行为：
 *   - EMA 滤波测量 TE 信号周期（抗抖动）
 *   - EMA 预测推屏耗时（抗单次波动，用于同步决策引擎）
 *   - VBlank 安全窗口感知：推屏必须在下一个 VBlank 前完成
 *   - push_time > 0.9×TE → 自动跳过 1 个 TE
 *   - push_time > 1.5×TE → 自动跳过 2 个 TE
 *   - 智能判定：根据 TE 相位 + 预测推屏耗时 + VBlank 窗口，
 *     若安全窗口内则可立即推屏不等待
 *   - wait_te 超时 → 返回 -2，App 自行决定降级策略
 *   - TE 长期丢失 → 创建定时器周期性自动复位 LCD（若已注册）
 *
 *
 * 线程模型：
 *   App Task:   adpt_te_wait_te() — 阻塞在信号量上
 *   TE ISR:     adpt_te_isr_te_signal() — Post 信号量，测量周期
 *   DMA ISR:    adpt_te_isr_push_complete() — 记录推送耗时，计算跳帧
 *
 *
 * 关于 TE 边沿：
 *   模块不关心上升沿/下降沿。App 配置 GPIO 中断触发方式，
 *   在 ISR 中调用 adpt_te_isr_te_signal() 即表示一次有效 TE 到达。
 */

#ifndef __ADPT_TE_H__
#define __ADPT_TE_H__

#include <stdint.h>
#include "adpt_te_port.h"

/*===========================================================================
 * 类型定义（字段按 32→16→8 位排列以减小 padding）
 *===========================================================================*/

/** 模块状态 */
typedef enum {
    ADPT_TE_STATE_IDLE      = 0,   /* 空闲（未启动或 wait_te 返回后） */
    ADPT_TE_STATE_WAIT_TE   = 1,   /* App 正在 wait_te() 中阻塞等待 TE */
    ADPT_TE_STATE_PUSHING   = 2,   /* App 已拿到 TE，正在推屏中 */
    ADPT_TE_STATE_STOPPED   = 3,   /* 已停止 */
} adpt_te_state_t;

/** 自适应 TE 运行时信息（只读） */
typedef struct {
    uint32_t te_period_us;          /* EMA 平滑后的 TE 周期 */
    uint32_t te_period_min_us;      /* 观测到的最小周期 */
    uint32_t te_period_max_us;      /* 观测到的最大周期 */
    uint32_t te_period_raw_us;      /* 最近一次原始测量值 */
    uint32_t push_time_us;          /* 最近一次推送耗时（原始值） */
    uint32_t predicted_push_time_us; /* EMA 预测推屏耗时（用于决策引擎） */
    uint32_t te_count;              /* 累计 TE 触发次数 */
    uint32_t push_count;            /* 累计推送完成次数 */
    uint32_t timeout_count;         /* 累计 TE 超时次数 */
    uint32_t skip_count;            /* 累计跳帧次数 */
    uint32_t lcd_reset_count;       /* 累计 LCD 复位触发次数 */
    uint8_t  te_lost;               /* 1 = TE 信号丢失 */
    uint8_t  freq_stable;           /* 1 = TE 频率稳定（连续8次偏差<12.5%） */
    uint8_t  reserved[2];
} adpt_te_info_t;

/** 初始化配置（无 reserved 填充，字段天然对齐） */
typedef struct {
    uint32_t   default_te_period_us; /* TE 默认周期（us），冷启动初值 */
    uint32_t   te_timeout_us;        /* TE 超时阈值（us），wait_te 等待上限 */
    uint32_t   lcd_reset_timeout_us; /* LCD 复位超时（us），0=禁用 */
    uint32_t   ema_alpha;            /* EMA 平滑系数 0~255 */
    uint32_t   push_delay_us;        /* TE 到达后延迟返回的 us 数，0=不延迟 */
    uint32_t   te_phase_us;          /* TE 信号到 LCD 行0 的时间差（us），默认 0 */
    uint32_t   push_margin_pct;      /* 推屏安全余量百分比（100~130），默认 115 */
    uint32_t   vblank_window_us;     /* VBlank 安全写入窗口（us），默认 2000 */
    uint32_t   push_ema_alpha;       /* 推屏耗时 EMA 平滑系数（0~255），默认 64 */
    uint32_t   lcd_reset_timer_ms;   /* LCD 复位定时器间隔（ms），0=直接在 wait_te 调用回调 */
    uint32_t   predict_safe_us;      /* TE 预测安全阈值（us），0=禁用预测必须等真实 TE，>0=距下个TE≤此值时跳过等待 */
} adpt_te_config_t;

/*===========================================================================
 * 核心 API（4 个）
 *===========================================================================*/

/**
 * @brief   初始化 TE 自适应同步模块
 * @param   pconfig  配置参数。传 NULL 使用编译期宏默认值
 * @return  0=成功，-1=已初始化，-2=信号量创建失败
 */
int adpt_te_init(const adpt_te_config_t *pconfig);

/**
 * @brief   TE 信号中断回调（在 TE GPIO ISR 中调用）
 * @note    极短执行路径：更新 EMA → 判断跳帧 → Post 信号量
 *          调用即认为一次有效的 TE 信号到达
 */
void adpt_te_isr_te_signal(void);

/**
 * @brief   等待 TE 信号（阻塞式，使用信号量）
 * @param   extra_timeout_us  额外增加的等待时间（us），0 = 仅使用配置中的 te_timeout_us
 * @return   0 = TE 到达 或 安全窗口内可直接推屏 或 预测等待（可开始推屏）
 *          -1 = 模块未启动
 *          -2 = 超时（TE 未在 timeout 内到达）
 * @note    阻塞期间 CPU 不空转（信号量 Pend）。
 *          返回 0 的三种情况：
 *          1. 安全推屏窗口：当前在 VBlank 窗口内/扫描安全区内，可立即推屏；
 *          2. 预测等待：predict_safe_us > 0 且距下一个 TE ≤ predict_safe_us，
 *             跳过信号量等待直接返回；
 *          3. TE 信号到达：阻塞等待到真实 TE 信号。
 *          TE 到达后若配置了 push_delay_us，会额外延时再返回。
 */
int adpt_te_wait_te(unsigned long extra_timeout_us);

#define adpt_te_wait_sync() adpt_te_wait_te(ADPT_TE_PORT_PUSH_DELAY_US)

/**
 * @brief   推屏完成中断回调（在 DMA 完成 ISR 中调用）
 * @note    记录推送耗时 → 计算下次需要跳过的 TE 数 → 状态切回 IDLE
 */
void adpt_te_isr_push_complete(void);

/*===========================================================================
 * 辅助 API
 *===========================================================================*/

/** @brief 反初始化，删除信号量并复位所有状态 */
void adpt_te_deinit(void);

/** @brief 启动 TE 同步（开始接受 TE 信号），返回 0=成功 */
int adpt_te_start(void);

/** @brief 停止 TE 同步（若有 App 在 wait_te 中，会被超时唤醒） */
void adpt_te_stop(void);

/** @brief 获取当前模块状态 */
adpt_te_state_t adpt_te_get_state(void);

/** @brief 获取运行时统计信息 */
void adpt_te_get_info(adpt_te_info_t *pinfo);

/**
 * @brief   注册 LCD 硬件复位回调
 * @param   cb  回调函数指针，传 NULL 取消注册
 * @note    当 TE 连续丢失超过 lcd_reset_timeout_us 时，
 *          在 wait_te() 上下文中自动调用此回调。
 */
void adpt_te_register_lcd_reset_cb(adpt_te_lcd_reset_cb_t cb);

/**
 * @brief   Dump 全部内部状态到日志（调试用）
 * @note    通过 adpt_te_port_log() 输出，无 DEBUG 宏时为空操作。
 *          在临界区内执行，保证打印内容一致性。
 */
void adpt_te_dump(void);

#endif /* __ADPT_TE_H__ */
