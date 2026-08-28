#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lptouch_key.data.bss")
#pragma data_seg(".lptouch_key.data")
#pragma const_seg(".lptouch_key.text.const")
#pragma code_seg(".lptouch_key.text")
#endif

#include "key_driver.h"
#include "lptouch_key.h"
#include "gpio.h"
#include "app_config.h"
#include "asm/power_interface.h"
#include "timer.h"
#include "jiffies.h"

#if TCFG_LPTOUCH_ENABLE

#define LOG_TAG             "[LPTOUCH]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#define SWIPE_STEP_MIN_MS       30      /* 相邻通道按压最小间隔(ms), 小于此值视为同时按压 */
#define SWIPE_STEP_MAX_MS       200     /* 相邻通道按压最大间隔(ms) */
#define SWIPE_TOTAL_MAX_MS      600     /* 首尾通道总时长上限(ms) */
#define SWIPE_COOLDOWN_MS       500     /* 滑动后冷却期, 抑制单键误报 */

/* 双指长按依次触发时间阈值(ms), 可按需调整 */
#define TWO_FINGER_HOLD_2S_MS   2000    /* 双指长按 2s → KEY_ACTION_HOLD_2SEC */
#define TWO_FINGER_HOLD_3S_MS   3000    /* 双指长按 3s → KEY_ACTION_HOLD_3SEC */
#define TWO_FINGER_HOLD_5S_MS   5000    /* 双指长按 5s → KEY_ACTION_HOLD_5SEC */
#define TWO_FINGER_COOLDOWN_MS  500     /* 双指抬起后冷却期, 抑制单键误报 */

/* 单指长按2s触发 HOLD_2SEC */
#define SINGLE_HOLD_2S_MS       2000

/* 滑动涉及的通道: ch3, ch4, ch5 */
#define SWIPE_CH_FIRST          3
#define SWIPE_CH_LAST           5

#define LPTOUCH_CH_TO_KEY_0                                              3                                //touch_key0对应通道3
#define LPTOUCH_CH_TO_KEY_1                                              4                                //touch_key1对应通道4
#define LPTOUCH_CH_TO_KEY_2                                              5                                //touch_key2对应通道5


struct key_driver_para lptouch_key_scan_para = {
    .last_key         = NO_KEY,
    .key_type         = KEY_DRIVER_TYPE_CTMU_TOUCH,
    .filter_time      = 0,            /* 底层已消抖 */
    .long_time        = 80,           /* 800ms 长按 */
    .hold_time        = (80 + 15),    /* 950ms 开始 HOLD */
    .click_delay_time = 40,           /* 400ms 多击窗口 */
    .scan_time        = 10,           /* 10ms 扫描周期 */
};

typedef struct {
    u8  is_pressed;          /* 当前是否按压中 */
    u8  fired_2s   : 1;     /* 单指 2s 事件已上报, 抬起时清除 */
    u32 press_time_ms;       /* 最近一次按压时刻 (jiffies_msec) */
} lptouch_ch_state_t;

static lptouch_ch_state_t g_ch_state[TOUCH_CH_MAX];  /* 8通道, 静态零初始化 */

typedef struct {
    u32 press_time[TOUCH_CH_MAX];       /* 各通道最近按压时刻 (仅 ch3/4/5 有效) */
    u8  reported     : 1;    /* 已上报滑动, 冷却期内抑制单键 */
    u32 clear_time;           /* 滑动上报时刻, 用于冷却期判断 */
} swipe_ctx_t;

static swipe_ctx_t g_swipe;

/* 双指长按状态 */
typedef struct {
    u8  active      : 1;    /* 当前处于双指状态 */
    u8  fired_2s    : 1;    /* 2s 事件已上报 */
    u8  fired_3s    : 1;    /* 3s 事件已上报 */
    u8  fired_5s    : 1;    /* 5s 事件已上报 */
    u32 start_ms;           /* 双指开始时刻 */
    u32 cooldown_ms;        /* 双指抬起时刻(0=未在冷却期) */
} two_finger_ctx_t;

static two_finger_ctx_t g_two_finger;

static void lptouch_swipe_detect(u8 ch);
static void lptouch_report_swipe(u8 direction);
static void lptouch_two_finger_fire(u8 event_type);

/* ============================================================
 *  lptouch_key_get_value: 获取当前按键值
 *
 *  供 key_driver_scan 以 scan_time 周期定时调用.
 *
 *  优先级: 滑动冷却 > 双指 > 单指
 *
 *  双指逻辑:
 *    1. >=2 通道按压 → 进入双指状态, 返回 NO_KEY 抑制单键
 *    2. 持续按压依次触发:
 *       2s → KEY_ACTION_HOLD_2SEC
 *       3s → KEY_ACTION_HOLD_3SEC
 *       5s → KEY_ACTION_HOLD_5SEC
 *       每个阈值仅触发一次 (fired 标志)
 *    3. 事件通过 key_event_handler 发送, 流经 multi_clicks_translate 时
 *       click_cnt=0, 不被拦截, 直接透传到消息表映射
 *    4. 双指抬起后进入冷却期, 防止残留单指误报
 * ============================================================ */
u8 lptouch_key_get_value(void)
{
    u32 now = jiffies_msec();

    if (g_swipe.reported) {
        if ((u32)jiffies_msec2offset(g_swipe.clear_time, now) < SWIPE_COOLDOWN_MS) {
            return NO_KEY;
        }
        g_swipe.reported = 0;
    }

    u8 pressed_cnt = 0;
    for (u8 ch = 0; ch < 8; ch++) {
        if (g_ch_state[ch].is_pressed) {
            pressed_cnt++;
        }
    }

    if (pressed_cnt >= 2) {
        if (!g_two_finger.active) {
            /* 首次检测到双指, 记录起始时间 */
            g_two_finger.active    = 1;
            g_two_finger.fired_2s  = 0;
            g_two_finger.fired_3s  = 0;
            g_two_finger.fired_5s  = 0;
            g_two_finger.start_ms  = now;

            /* 复位框架扫描状态: 避免先落下的单指被误判为单击 */
            lptouch_key_scan_para.last_key        = NO_KEY;
            lptouch_key_scan_para.filter_cnt      = 0;
            lptouch_key_scan_para.press_cnt       = 0;
            lptouch_key_scan_para.click_delay_cnt = 0;
            lptouch_key_scan_para.filter_value    = NO_KEY;
        }

        u32 elapsed = (u32)jiffies_msec2offset(g_two_finger.start_ms, now);

        /* 依次触发各时间阈值 (每个仅触发一次) */
        if (!g_two_finger.fired_5s && elapsed >= TWO_FINGER_HOLD_5S_MS) {
            lptouch_two_finger_fire(KEY_ACTION_HOLD_5SEC);
            g_two_finger.fired_5s = 1;
        } else if (!g_two_finger.fired_3s && elapsed >= TWO_FINGER_HOLD_3S_MS) {
            lptouch_two_finger_fire(KEY_ACTION_HOLD_3SEC);
            g_two_finger.fired_3s = 1;
        } else if (!g_two_finger.fired_2s && elapsed >= TWO_FINGER_HOLD_2S_MS) {
            lptouch_two_finger_fire(KEY_ACTION_HOLD_2SEC);
            g_two_finger.fired_2s = 1;
        }

        return NO_KEY;  /* 双指期间抑制单键上报 */
    }

    /* ---- 双指→抬起: 进入冷却期 ---- */
    if (g_two_finger.active) {
        g_two_finger.active      = 0;
        g_two_finger.cooldown_ms = now;

        /* 复位框架扫描状态: 避免残留按压被误判为单击 */
        lptouch_key_scan_para.last_key        = NO_KEY;
        lptouch_key_scan_para.filter_cnt      = 0;
        lptouch_key_scan_para.press_cnt       = 0;
        lptouch_key_scan_para.click_delay_cnt = 0;
        lptouch_key_scan_para.filter_value    = NO_KEY;
    }

    /* ---- 双指冷却期 ---- */
    if (g_two_finger.cooldown_ms) {
        if ((u32)jiffies_msec2offset(g_two_finger.cooldown_ms, now) < TWO_FINGER_COOLDOWN_MS) {
            return NO_KEY;
        }
        g_two_finger.cooldown_ms = 0;
    }

    /* ---- 单指: 返回首个按压通道键值, 并检测 2s 长按 ---- */
    if (pressed_cnt == 1) {
        for (u8 ch = 0; ch < 8; ch++) {
            if (g_ch_state[ch].is_pressed) {
                /* 单指持续按压 2s, 上报 HOLD_2SEC (仅一次) */
                if (!g_ch_state[ch].fired_2s &&
                    (u32)jiffies_msec2offset(g_ch_state[ch].press_time_ms, now) >= SINGLE_HOLD_2S_MS) {
                    g_ch_state[ch].fired_2s = 1;

                    struct key_event key = {0};
                    key.init  = 1;
                    key.type  = KEY_DRIVER_TYPE_CTMU_TOUCH;
                    key.event = KEY_ACTION_HOLD_2SEC;
                    key.value = LPTOUCH_CH_TO_KEY(ch);
                    key.tmr   = sys_timer_get_ms();
                    key_event_handler(&key);

                    /* 复位框架状态: 避免 HOLD_2SEC 事件被框架的 HOLD 计数器误判 */
                    lptouch_key_scan_para.last_key        = NO_KEY;
                    lptouch_key_scan_para.filter_cnt      = 0;
                    lptouch_key_scan_para.press_cnt       = 0;
                    lptouch_key_scan_para.click_delay_cnt = 0;
                    lptouch_key_scan_para.filter_value    = NO_KEY;
                }
                return LPTOUCH_CH_TO_KEY(ch);
            }
        }
    }

    return NO_KEY;
}

int lptouch_key_init(void)
{
    log_info("lptouch_key_init");
    register_key_for_hold(LPTOUCH_CH_TO_KEY(LPTOUCH_CH_TO_KEY_0));
    register_key_for_hold(LPTOUCH_CH_TO_KEY(LPTOUCH_CH_TO_KEY_1));
    register_key_for_hold(LPTOUCH_CH_TO_KEY(LPTOUCH_CH_TO_KEY_2));

    return 0;
}

/* ============================================================
 *  判断时间差是否在滑动范围内
 * ============================================================ */
static inline int in_swipe_range(u32 diff)
{
    return (diff >= SWIPE_STEP_MIN_MS && diff <= SWIPE_STEP_MAX_MS);
}

/* ============================================================
 *  lptouch_swipe_detect: 滑动检测
 *
 *  在 msys_touch_msg_handler 的 PRESS 分支中调用.
 *  检查 ch3/ch4/ch5 的按压时序是否构成滑动手势.
 *
 *  优先级:
 *    1. 三通道完整滑动 (ch3→ch4→ch5  / ch5→ch4→ch3 )
 *    2. 相邻两通道滑动 (ch3→ch4, ch4→ch5  / ch4→ch3, ch5→ch4 )
 *
 *  注意: 同一轮 PRESS 序列中仅上报一次滑动 (reported 标志).
 * ============================================================ */
static void lptouch_swipe_detect(u8 ch)
{
    if (ch < SWIPE_CH_FIRST || ch > SWIPE_CH_LAST) {
        return;
    }

    /* 双指优先: >=2通道按压时跳过滑动检测 */
    u8 pressed = 0;
    u8 non_swipe_pressed = 0;
    for (u8 i = 0; i < 8; i++) {
        if (g_ch_state[i].is_pressed) {
            pressed++;
            if (i < SWIPE_CH_FIRST || i > SWIPE_CH_LAST) {
                non_swipe_pressed = 1;
            }
        }
    }
    if (pressed >= 2 && non_swipe_pressed) {
        return;
    }

    u32 now = jiffies_msec();
    g_swipe.press_time[ch] = now;

    u32 t3 = g_swipe.press_time[3];
    u32 t4 = g_swipe.press_time[4];
    u32 t5 = g_swipe.press_time[5];
    u8 reported = 0;

    /* ---- 优先级1: 三通道完整滑动 ---- */

    /*  ch3→ch4→ch5 */
    if (!reported && t3 && t4 && t5 && t3 < t4 && t4 < t5) {
        u32 d34 = t4 - t3;
        u32 d45 = t5 - t4;
        u32 d35 = t5 - t3;
        if (in_swipe_range(d34) && in_swipe_range(d45) &&
            d35 <= SWIPE_TOTAL_MAX_MS) {
            lptouch_report_swipe(1);  /* backward */
            reported = 1;
        }
    }

    /*  ch5→ch4→ch3 */
    if (!reported && t3 && t4 && t5 && t5 < t4 && t4 < t3) {
        u32 d54 = t4 - t5;
        u32 d43 = t3 - t4;
        u32 d53 = t3 - t5;
        if (in_swipe_range(d54) && in_swipe_range(d43) &&
            d53 <= SWIPE_TOTAL_MAX_MS) {
            lptouch_report_swipe(0);  /* forward */
            reported = 1;
        }
    }

    /* ---- 优先级2: 相邻两通道滑动 ---- */

    /*  ch3→ch4 */
    if (!reported && t3 && t4 && t3 < t4 && in_swipe_range(t4 - t3)) {
        lptouch_report_swipe(1);
        reported = 1;
    }

    /*  ch4→ch5 */
    if (!reported && t4 && t5 && t4 < t5 && in_swipe_range(t5 - t4)) {
        lptouch_report_swipe(1);
        reported = 1;
    }

    /*  ch4→ch3 */
    if (!reported && t3 && t4 && t4 < t3 && in_swipe_range(t3 - t4)) {
        lptouch_report_swipe(0);
        reported = 1;
    }

    /*  ch5→ch4 */
    if (!reported && t4 && t5 && t5 < t4 && in_swipe_range(t4 - t5)) {
        lptouch_report_swipe(0);
        reported = 1;
    }
}

/* ============================================================
 *  lptouch_report_swipe: 滑动上报
 *
 *  通过 key_event_handler 发送 KEY_ACTION_SWIPE 事件.
 *  (该事件类型在 multi_clicks_translate 中不被拦截, 直接透传)
 *
 *  同时:
 *    - 设置滑动冷却期标志, 抑制 get_value 返回单键值
 *    - 清除滑动通道的按压时间, 防止重复触发
 *    - 复位框架 key_driver_para 状态, 避免已按压通道在抬起时误报 CLICK
 * ============================================================ */
static void lptouch_report_swipe(u8 direction)
{
    struct key_event key = {0};
    key.init = 1;
    key.type = KEY_DRIVER_TYPE_CTMU_TOUCH;
    key.event = KEY_ACTION_SWIPE;
    key.value = (direction == 0) ? KEY_LPTOUCH_SWIPE_FORWARD
                : KEY_LPTOUCH_SWIPE_BACKWARD;
    key.tmr = sys_timer_get_ms();

    log_info("SWIPE %s", direction == 0 ? "FORWARD" : "BACKWARD");

    /* 通过框架上报: KEY_ACTION_SWIPE 不被 multi_clicks_translate 拦截 */
    key_event_handler(&key);

    /* 设置冷却期标志 */
    g_swipe.reported   = 1;
    g_swipe.clear_time = jiffies_msec();

    /* 清除滑动通道的按压时间 */
    g_swipe.press_time[3] = 0;
    g_swipe.press_time[4] = 0;
    g_swipe.press_time[5] = 0;

    /* 复位框架扫描参数: 避免滑动通道产生单击/长按误报 */
    lptouch_key_scan_para.last_key        = NO_KEY;
    lptouch_key_scan_para.filter_cnt      = 0;
    lptouch_key_scan_para.press_cnt       = 0;
    lptouch_key_scan_para.click_delay_cnt = 0;
    lptouch_key_scan_para.filter_value    = NO_KEY;
}

/* ============================================================
 *  lptouch_two_finger_fire: 双指长按事件上报
 *
 *  通过 key_event_handler 发送指定 event_type 事件,
 *  键值为 KEY_LPTOUCH_TWO_FINGER.
 *
 *  event_type 为 KEY_ACTION_HOLD_XSEC (2s/3s/5s),
 *  该事件流经 multi_clicks_translate 时:
 *    - 不属于 CLICK/LONG/HOLD/UP 中的任何一种
 *    - click_cnt 为 0 (双指期间 get_value 始终返回 NO_KEY)
 *    - 直接透传到上层消息表映射
 *
 *  每次调用复位框架扫描参数, 防止双指过程中框架累积的
 *  press_cnt 在双指结束后触发意外事件.
 * ============================================================ */
static void lptouch_two_finger_fire(u8 event_type)
{
    struct key_event key = {0};
    key.init  = 1;
    key.type  = KEY_DRIVER_TYPE_CTMU_TOUCH;
    key.event = event_type;
    key.value = KEY_LPTOUCH_TWO_FINGER;
    key.tmr   = sys_timer_get_ms();

    log_info("TWO_FINGER_HOLD %s",
             event_type == KEY_ACTION_HOLD_2SEC ? "2s" :
             event_type == KEY_ACTION_HOLD_3SEC ? "3s" :
             event_type == KEY_ACTION_HOLD_5SEC ? "5s" : "?");

    key_event_handler(&key);

    /* 不在此处设 fired 标志由调用者按各自阈值独立管理 */

    /* 复位框架状态: 避免双指期间累积的 press_cnt 产生副作用 */
    lptouch_key_scan_para.last_key        = NO_KEY;
    lptouch_key_scan_para.filter_cnt      = 0;
    lptouch_key_scan_para.press_cnt       = 0;
    lptouch_key_scan_para.click_delay_cnt = 0;
    lptouch_key_scan_para.filter_value    = NO_KEY;
}

/* ============================================================
 *  msys_touch_msg_handler: PMC → 主系统 触摸消息处理
 *
 *  接收来自PMC 的 MSG_CTMU 类型原始触摸事件.
 *
 *  逻辑:
 *    1. 更新每通道按压状态 (g_ch_state), 供 get_value() 读取
 *    2. PRESS 时触发滑动检测
 *    3. 不再做单击/长按/多击识别  由 key_driver 框架完成
 * ============================================================ */
static void msys_touch_msg_handler(void *priv, u8 *buf, u32 len)
{
    if (len < sizeof(touch_msg_t)) {
        return;
    }

    touch_msg_t *msg = (touch_msg_t *)buf;
    u8 ch = msg->channel;
    if (ch >= 8) {
        return;
    }

    u32 now = jiffies_msec();

    if (msg->event == TOUCH_EVENT_PRESS) {
        g_ch_state[ch].is_pressed    = 1;
        g_ch_state[ch].press_time_ms = now;
        g_ch_state[ch].fired_2s      = 0;   /* 新一次按压, 重置 2s 事件标志 */
        lptouch_swipe_detect(ch);
    } else {
        g_ch_state[ch].is_pressed = 0;
        g_ch_state[ch].fired_2s   = 0;       /* 抬起时清除 */
        if (ch >= SWIPE_CH_FIRST && ch <= SWIPE_CH_LAST) {
            g_swipe.press_time[ch] = 0;
        }
    }
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_CTMU, msys_touch_msg_handler);

REGISTER_KEY_OPS(lptouch_key) = {
    .idle_query_en    = 1,
    .param            = &lptouch_key_scan_para,
    .get_value        = lptouch_key_get_value,
    .key_init         = lptouch_key_init,
};

#endif  /* TCFG_LPTOUCH_ENABLE */
