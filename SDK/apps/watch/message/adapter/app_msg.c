#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_msg.data.bss")
#pragma data_seg(".app_msg.data")
#pragma const_seg(".app_msg.text.const")
#pragma code_seg(".app_msg.text")
#endif
#include "app_msg.h"
#include "key_driver.h"
#include "app_task.h"

#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* 非 IDLE GOTO_MODE 置位；power_on 通过 consume 读取并清除 */
static volatile u8 s_goto_mode_pending;

int app_goto_mode_consume_pending(void)
{
    if (s_goto_mode_pending) {
        s_goto_mode_pending = 0;
        return 1;
    }
    return 0;
}

int app_send_message(int _msg, int arg)
{
    int msg[2];
    msg[0] = _msg;
    msg[1] = arg;
    if (_msg == APP_MSG_GOTO_MODE) {
        if (arg != APP_MODE_IDLE) {
            s_goto_mode_pending = 1;
        }
    }
    int ret = os_taskq_post_type("app_core", MSG_FROM_APP, 2, msg);
    if (ret) {
        log_error("%s send err  %x\n", __FUNCTION__, ret);
    }
    return ret;
}

int app_send_message2(int _msg, int arg1, int arg2)
{
    int msg[3];
    msg[0] = _msg;
    msg[1] = arg1;
    msg[2] = arg2;
    if (_msg == APP_MSG_GOTO_MODE) {
        if (arg1 != APP_MODE_IDLE) {
            s_goto_mode_pending = 1;
        }
    }
    int ret = os_taskq_post_type("app_core", MSG_FROM_APP, 3, msg);
    if (ret) {
        log_error("%s send err  %x\n", __FUNCTION__, ret);
    }
    return ret;

}

int app_send_message_from(int from, int argc, int *msg)
{
    int ret = os_taskq_post_type("app_core", from, (argc + 3) / 4, msg);
    if (ret) {
        log_error("%s send err  %x\n", __FUNCTION__, ret);
    }
    return ret;
}


int app_key_event_remap(const struct key_remap_table *table, int *_event)
{
    struct key_event *key = (struct key_event *)_event;

    for (int i = 0; table[i].key_value != 0xff; i++) {
        if (table[i].key_value == key->value) {
            if (table[i].remap_table) {
                return table[i].remap_table[key->event];
            }
            if (table[i].remap_func) {
                return table[i].remap_func((int *)key);
            }
            break;
        }
    }

    return 0;
}
