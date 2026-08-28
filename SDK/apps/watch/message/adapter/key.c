#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key.data.bss")
#pragma data_seg(".key.data")
#pragma const_seg(".key.text.const")
#pragma code_seg(".key.text")
#endif
#include "app_msg.h"
#include "key_driver.h"
#include "system/timer.h"
#include "sdk_config.h"

#define LOG_TAG_CONST       KEY_EVENT
#define LOG_TAG             "[KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

struct key_hold {
    u8 send_2s_msg;
    u8 send_3s_msg;
    u8 send_5s_msg;
    u8 send_8s_msg;
    u8 send_16s_msg;
    u32 start_time;    // 长按起始时间
    u32 last_check_time; // 上次检查时间
    u8 hold_2s_sent;   // 2秒事件是否已发送
    u8 hold_3s_sent;   // 3秒事件是否已发送
    u8 hold_5s_sent;   // 5秒事件是否已发送
    u8 hold_8s_sent;   // 8秒事件是否已发送
    u8 hold_16s_sent;  // 16秒事件是否已发送
};

#define KEY_HOLD_MAX 3		//需更多按键自行拓展
static struct key_hold key_hold_hdl[KEY_HOLD_MAX];
static u8 key_value_registered[KEY_HOLD_MAX] = {NO_KEY, NO_KEY, NO_KEY};
static int registered_count = 0;

// 提供给各按键驱动调用的注册函数
int register_key_for_hold(u8 key_value)
{
    if (registered_count >= KEY_HOLD_MAX) {
        log_debug("[register_key_for_hold]error: too many keys registered\n");
        return -1;
    }

    // 检查是否已注册
    for (int i = 0; i < registered_count; i++) {
        if (key_value_registered[i] == key_value) {
            return i;  // 已注册，返回索引
        }
    }

    // 新注册
    key_value_registered[registered_count] = key_value;
    log_debug("[register_key_for_hold]: registered key_value=%d at index=%d\n",
              key_value, registered_count);
    return registered_count++;
}

static struct key_hold *get_key_hold(u8 key_value)
{
    for (int i = 0; i < registered_count; i++) {
        if (key_value_registered[i] == key_value) {
            return &key_hold_hdl[i];
        }
    }
    return NULL;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 多击按键事件解析与转换处理
 *         支持单击、双击、多连击、连击后长按、长按抬手定时触发（2s/3s/5s/8s/16s）等组合按键识别，
 *         将底层原始按键动作统一翻译成上层可直接使用的标准化按键事件码
 *
 * @param[in] key：按键事件结构体指针，携带底层原始按键动作类型（单击、长按起始、长按持续、按键抬起）以及对应物理键值
 *
 * @return 0：不拦截原始按键事件，原始事件继续向上层传递
 *         1：拦截本次原始按键事件，上层不再接收该底层原始按键消息
 */
/* ----------------------------------------------------------------------------*/
static int multi_clicks_translate(struct key_event *key)
{
    static u8 click_cnt = 0;
    static u8 notify_value = 0xff;
    static u32 hold_start_time = 0;
    // 普通长按标志位（单击后长按）
    static u8 send_2s = 0, send_3s = 0, send_5s = 0, send_8s = 0, send_16s = 0;
    // 多击长按标志位（双击后长按）
    static u8 send_double_hold_3s = 0, send_double_hold_8s = 0;
    static u8 hold_click_cnt = 0;   //进入长按时的点击次数,UP时用于区分"多击+长按"(click_cnt在LONG后会被末尾逻辑清零)

    struct key_hold *hold = get_key_hold(key->value);

    /* 触摸滑动 / 触摸定时长按等"已定型"事件直接透传, 不参与连击状态机.
     * 这些事件来自 lptouch 触摸键, 与普通按键(如 iokey)的 click_cnt/notify_value
     * 无关. 若走末尾逻辑会清零这些共享静态状态, 导致持续滑动期间 iokey 的
     * 单击/多击(依赖 CLICK→NO_KEY 之间的 click_cnt 状态)被误清而丢失. */
    if (key->event == KEY_ACTION_SWIPE ||
        key->event == KEY_ACTION_HOLD_2SEC ||
        key->event == KEY_ACTION_HOLD_3SEC ||
        key->event == KEY_ACTION_HOLD_5SEC) {
        return 0;
    }

    if (key->event == KEY_ACTION_LONG) {
        if (click_cnt > 0 && key->value != notify_value) {
            //换键长按:前一键连击未结算(NO_KEY已被驱动层清零),补发结算,新键当普通长按
            //避免残留click_cnt把新键长按误判为多击+长按(如左键双击+右键长按2秒本该录像却失效)
            struct key_event settle = *key;
            settle.value = notify_value;
            settle.event = (click_cnt >= 2) ? (KEY_ACTION_DOUBLE_CLICK + (click_cnt - 2)) : KEY_ACTION_CLICK;
            app_send_message_from(MSG_FROM_KEY, sizeof(settle), (int *)&settle);
            click_cnt = 0;
            hold_click_cnt = 0;
        } else {
            if (click_cnt) {
                key->event = KEY_ACTION_SINGLE_LONG + (click_cnt - 1);
            }
            hold_click_cnt = click_cnt;   //同键多击+长按:保存,UP时据此区分
        }

        hold_start_time = jiffies;

        send_2s = send_3s = send_5s = send_8s = send_16s = 0;
        send_double_hold_3s = send_double_hold_8s = 0;

        if (hold) {
            hold->start_time = jiffies;
            hold->send_2s_msg = 0;
            hold->send_3s_msg = 0;
            hold->send_5s_msg = 0;
            hold->send_8s_msg = 0;
            hold->send_16s_msg = 0;
        }

        notify_value = key->value;
        return 0;  // 继续上报 LONG 事件
    } else if (key->event == KEY_ACTION_HOLD) {
        if (!hold) {
            return 1;  // 无 hold 结构体，拦截
        }

        // 计算当前长按时间（补偿触发 long 的时差）
        int time_msec = jiffies_offset_to_msec(hold->start_time, jiffies)
                        + (key->long_time) * 10;

        // ===== 多击长按（双击及以上）=====
        if (hold_click_cnt >= 2) {
            log_debug("[hold_click_cnt]: multi-click hold, hold_click_cnt=%d\n", hold_click_cnt);

            // 三击长按
            if (hold_click_cnt == 2) {
                if (time_msec >= 3000 && !send_double_hold_3s) {
                    key->event = KEY_ACTION_TRIPLE_LONG_3SEC;
                    log_debug("triple click + hold 3s triggered");
                    send_double_hold_3s = 1;
                    return 0;  // 立即上报事件
                } else if (time_msec >= 8000 && !send_double_hold_8s) {
                    key->event = KEY_ACTION_TRIPLE_LONG_8SEC;
                    log_debug("triple click + hold 8s triggered");
                    send_double_hold_8s = 1;
                    return 0;
                }
            }
            // 多击长按未达到阈值，拦截原始 HOLD 事件
            return 1;
        }

        // ===== 普通长按（单击）=====
        else {
            // 普通长按定时事件
            /* if (time_msec >= 16000 && !send_16s) { */
            /* key->event = KEY_ACTION_HOLD_16SEC; */
            /* log_debug("hold 16s detected"); */
            /* send_16s = 1; */
            /* return 0; */
            /* } else if (time_msec >= 8000 && !send_8s) { */
            /* key->event = KEY_ACTION_HOLD_8SEC; */
            /* log_debug("hold 8s detected"); */
            /* send_8s = 1; */
            /* return 0; */
            /* } else  */
            if (time_msec >= 5000 && !send_5s) {
                key->event = KEY_ACTION_HOLD_5SEC;
                log_debug("hold 5s detected");
                send_5s = 1;
                return 0;
            } else if (time_msec >= 3000 && !send_3s) {
                key->event = KEY_ACTION_HOLD_3SEC;
                log_debug("hold 3s detected");
                send_3s = 1;
                return 0;
            } else if (time_msec >= 2000 && !send_2s) {
                key->event = KEY_ACTION_HOLD_2SEC;
                log_debug("hold 2s detected");
                send_2s = 1;
                return 0;
            }
        }

        // 未达到任何阈值，拦截原始 HOLD 事件
        return 1;
    } else if (key->event == KEY_ACTION_UP) {
        if (hold_start_time != 0) {
            int time_msec = jiffies_offset_to_msec(hold_start_time, jiffies)
                            + (key->long_time) * 10;
            log_debug("total hold duration: %d ms\n", time_msec);
            //长按8s和长按16s在抬起时结算
            if (time_msec >= 16000) {
                key->event = KEY_ACTION_HOLD_16SEC;
                log_debug("hold 16s detected");
            } else if (time_msec >= 8000) {
                key->event = KEY_ACTION_HOLD_8SEC;
                log_debug("hold 8s detected");
            }

            // 仅清除状态，不再修改事件（长按事件已在 HOLD 阶段处理）
            hold_start_time = 0;
            hold_click_cnt = 0;
            send_2s = send_3s = send_5s = send_8s = send_16s = 0;
            send_double_hold_3s = send_double_hold_8s = 0;
            click_cnt = 0;
            notify_value = NO_KEY;

            if (hold) {
                hold->send_2s_msg = 0;
                hold->send_3s_msg = 0;
                hold->send_5s_msg = 0;
                hold->send_8s_msg = 0;
                hold->send_16s_msg = 0;
                hold->start_time = 0;
            }
        }
        return 0;  // 继续上报 UP 事件
    } else {
        // 其他事件清除状态
        hold_start_time = 0;
        hold_click_cnt = 0;
        send_2s = send_3s = send_5s = send_8s = send_16s = 0;
        send_double_hold_3s = send_double_hold_8s = 0;

        if (hold) {
            hold->send_2s_msg = 0;
            hold->send_3s_msg = 0;
            hold->send_5s_msg = 0;
            hold->send_8s_msg = 0;
            hold->send_16s_msg = 0;
            hold->start_time = 0;
        }
    }

    // ===== 原有单击/多击逻辑保持不变 =====
    if (key->event == KEY_ACTION_CLICK) {
        if (key->value != notify_value) {
            //连击窗口内换键:先把上一个键的连击结算补发出去,
            //否则会被新键覆盖且其NO_KEY已被驱动层清零而彻底丢失
            if (notify_value != NO_KEY && click_cnt > 0) {
                struct key_event settle = *key;
                settle.value = notify_value;
                if (click_cnt >= 2) {
                    settle.event = KEY_ACTION_DOUBLE_CLICK + (click_cnt - 2);
                } else {
                    settle.event = KEY_ACTION_CLICK;
                }
                app_send_message_from(MSG_FROM_KEY, sizeof(settle), (int *)&settle);
            }

            click_cnt = 1;
            notify_value = key->value;
            //换键时前一键长按状态作废:单键驱动下前键UP会丢失,残留的hold状态会导致下次该键UP误触发时长档
            hold_start_time = 0;
            hold_click_cnt = 0;
            send_2s = send_3s = send_5s = send_8s = send_16s = 0;
            send_double_hold_3s = send_double_hold_8s = 0;
            if (hold) {
                hold->send_2s_msg = 0;
                hold->send_3s_msg = 0;
                hold->send_5s_msg = 0;
                hold->send_8s_msg = 0;
                hold->send_16s_msg = 0;
                hold->start_time = 0;
            }
        } else {
            if (click_cnt < 5) {
                click_cnt++;
            }
        }
        return 1;  // 拦截单击，等待 NO_KEY
    }

    if (key->event == KEY_ACTION_NO_KEY) {
        if (click_cnt >= 2) {
            key->event = KEY_ACTION_DOUBLE_CLICK + (click_cnt - 2);
        } else {
            key->event = KEY_ACTION_CLICK;
        }
        key->value = notify_value;
        click_cnt = 0;
        notify_value = NO_KEY;
    } else if (key->event > KEY_ACTION_CLICK) {
        if (key->event != KEY_ACTION_UP && key->event != KEY_ACTION_LVGL_DOWN) {
            click_cnt = 0;
            notify_value = NO_KEY;
        }
    }

    return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @brief 联合按键判断
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 *
 * @return 0：不拦截按键事件
 *         1：拦截按键事件
 */
/* ----------------------------------------------------------------------------*/
static int combination_key_translate(struct key_event *key)
{
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件过滤、检测和发送
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 */
/* ----------------------------------------------------------------------------*/
void key_event_handler(struct key_event *key)
{
    const struct key_callback *p;

    if (multi_clicks_translate(key)) {
        return;
    }
    if (combination_key_translate(key)) {
        return;
    }
    if (key->event == KEY_ACTION_NO_KEY) {
        return;
    }

    //外部需要格外做的处理流程，请通过注册的形式在此处回调
    list_for_each_key_callback(p) {
        if (p->cb_deal == NULL) {
            continue;
        }
        if (p->cb_deal(key)) {
            return;
        }
    }
    app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
}


