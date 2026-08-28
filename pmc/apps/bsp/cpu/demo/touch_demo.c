/*==============================================================================
 * touch_demo.c  LPCTMU 触摸按键检测完整 demo
 *
 * 功能:
 *   1. 覆写 lib 层的弱函数 lpctmu_res_deal, 实现按压/释放检测
 *   2. 动态基线跟踪(一阶 IIR 低通滤波)
 *   3. 消抖防误触
 *   4. 通过 p2m_post_msg(MSG_CTMU) 将触摸事件发送给主系统
 *============================================================================*/

#include "includes.h"
#include "lpctmu.h"
#include "lp_msg.h"

/* 通道按压各异性: 不同通道的灵敏度/基线不同时, 可在此单独配置 */
typedef struct {
    u16 press_threshold;    /* 按下阈值: baseline - val > threshold 判定为按压 */
    u16 release_hyst;       /* 释放迟滞: baseline - val < hyst 判定为释放   */
    u16 baseline_init;      /* 初始基线值(0 = 自动从第一次采样学习)         */
} touch_ch_cfg_t;

static const touch_ch_cfg_t ch_cfg[LPCTMU_CH_SIZE] = {
    [0] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [1] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [2] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [3] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [4] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [5] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [6] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
    [7] = { .press_threshold = 200, .release_hyst = 100, .baseline_init = 0 },
};

/* 基线 IIR 滤波系数: alpha = 1/2^N, N 越大跟踪越慢但越抗干扰 */
#define BASELINE_ALPHA_SHIFT    7                   /* 1/128 ≈ 0.008 */

/* 消抖: 需连续 N 次满足条件才切换状态 (scan_time=20ms 时 3 次 = 60ms) */
#define DEBOUNCE_CNT_MAX        3

/* 事件类型 */
#define TOUCH_EVENT_PRESS       1
#define TOUCH_EVENT_RELEASE     0

/* 调试打印开关 */
#define TOUCH_DEBUG             CONFIG_UART_DEBUG_ENABLE

#if TOUCH_DEBUG
#define touch_log(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#else
#define touch_log(fmt, ...)
#endif

typedef enum {
    TOUCH_STATE_IDLE,           /* 空闲(未按压) */
    TOUCH_STATE_DEBOUNCE_PRESS, /* 消抖中 → 按下的过渡态 */
    TOUCH_STATE_PRESSED,        /* 已按压 */
    TOUCH_STATE_DEBOUNCE_REL,   /* 消抖中 → 释放的过渡态 */
} touch_state_e;

typedef struct {
    u16             baseline;       /* 动态基线 */
    touch_state_e   state;          /* 当前状态 */
    u8              debounce_cnt;   /* 消抖计数器 */
    u16             last_val;       /* 上一次采样值(用于斜率判断) */
} touch_ctx_t;

static touch_ctx_t g_touch_ctx[LPCTMU_CH_SIZE];

/*===========================================================================
 * 触摸事件消息体
 *===========================================================================*/

typedef struct {
    u8 channel;     /* 通道号 0~7 */
    u8 event;       /* TOUCH_EVENT_PRESS / TOUCH_EVENT_RELEASE */
    u16 value;      /* 当前采样值(可选, 方便主系统做手势识别) */
} touch_msg_t;

void touch_report(u8 ch, u8 event, u16 val);

/*===========================================================================
 * 核心: lpctmu_res_deal 覆写
 *
 * 本函数在 ISR 上下文中被调用, 每条通道每次扫描都会调用一次.
 *===========================================================================*/

void lpctmu_res_deal(u32 ch, u32 res)
{
    if (ch >= LPCTMU_CH_SIZE) {
        return;
    }

    touch_ctx_t *ctx = &g_touch_ctx[ch];
    const touch_ch_cfg_t *cfg = &ch_cfg[ch];
    u16 val = (u16)res;
    s16 delta;

    /* ---------- 1. 基线初始化(首次采样) ---------- */
    if (ctx->baseline == 0) {
        ctx->baseline = val;
        ctx->state = TOUCH_STATE_IDLE;
        touch_log("touch ch%u init baseline = %u\n", ch, val);
        return;
    }

    /* ---------- 2. 基线跟踪 ---------- */
    if (ctx->state == TOUCH_STATE_IDLE) {
        /*
         * 一阶 IIR 低通滤波, 仅在空闲态跟踪基线.
         * 跟踪很慢(alpha = 1/128), 所以即使偶尔有手指误入,
         * 也不会把基线拉到按压水平.
         */
        s16 diff = (s16)val - (s16)ctx->baseline;
        ctx->baseline += diff >> BASELINE_ALPHA_SHIFT;
    }
    /* 按压期间基线不更新, 避免把按压电平当新基线 */

    /* ---------- 3. 计算偏差 ---------- */
    delta = (s16)ctx->baseline - (s16)val;  /* 按压时 val 下降, delta > 0 */

    ctx->last_val = val;

    /* ---------- 4. 状态机 ---------- */
    switch (ctx->state) {

    /* ---- 空闲 → 消抖按下 ---- */
    case TOUCH_STATE_IDLE:
        if (delta > cfg->press_threshold) {
            if (++ctx->debounce_cnt >= DEBOUNCE_CNT_MAX) {
                ctx->state        = TOUCH_STATE_PRESSED;
                ctx->debounce_cnt = 0;

                touch_log(">>> TOUCH PRESS  ch%u  val=%u  baseline=%u  delta=%d\n",
                          ch, val, ctx->baseline, delta);
                touch_report(ch, TOUCH_EVENT_PRESS, val);
            }
        } else {
            ctx->debounce_cnt = 0;
        }
        break;

    /* ---- 按压 → 消抖释放 ---- */
    case TOUCH_STATE_PRESSED:
        if (delta < cfg->release_hyst) {
            if (++ctx->debounce_cnt >= DEBOUNCE_CNT_MAX) {
                ctx->state        = TOUCH_STATE_IDLE;
                ctx->debounce_cnt = 0;
                /*
                 * 释放后立即用当前值初始化基线,
                 * 避免环境导致的基线漂移造成误触发.
                 */
                ctx->baseline     = val;

                touch_log("<<< TOUCH RELEASE ch%u  val=%u  baseline=%u  delta=%d\n",
                          ch, val, ctx->baseline, delta);
                touch_report(ch, TOUCH_EVENT_RELEASE, val);
            }
        } else {
            ctx->debounce_cnt = 0;
        }
        break;

    default:
        break;
    }
}

/*===========================================================================
 * 事件上报: p2m_post_msg
 *
 * 注意: 本函数在 ISR 上下文调用,
 *       ack 必须为 0 (不允许在中断里发 ack 消息)
 *===========================================================================*/

void touch_report(u8 ch, u8 event, u16 val)
{
    touch_msg_t msg = {
        .channel = ch,
        .event   = event,
        .value   = val,
    };

    int ret = p2m_post_msg(MSG_CTMU, 0, &msg, sizeof(msg));
    if (ret != MSG_NO_ERROR) {
        touch_log("touch: p2m_post_msg fail! ret=%d\n", ret);
    }
}
