#include "app_config.h"
#include "ui/ui_api.h"

#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE

extern int topbar_battery_status_update_cb(const char *type, uint32_t argc);
extern int window_bt_status_update_cb(const char *type, uint32_t argc);
extern int do_msg_handler(const char *msg, va_list *pargptr, int (*handler)(const char *, u32));
extern int bt_emitter_status_handler(const char *type, u32 arg);

static const struct uimsg_handl ui_global_msg_handler[] = {
    { "bt_status", window_bt_status_update_cb },
    { "topbar_battery", topbar_battery_status_update_cb },
#if TCFG_USER_EMITTER_ENABLE
    { "bt_emitter_status",  bt_emitter_status_handler     }, /* 蓝牙状态 */
#endif
    {NULL, NULL},
};

int ui_message_filter_hook(const char *msg, va_list argptr)
{
    int iter = 0;
    const char *str;

    u8 ret = false;
    const struct uimsg_handl *handler = ui_global_msg_handler;
    while ((str = str_substr_iter(msg, ',', &iter)) != NULL) {
        for (; handler->msg != NULL; handler++) {
            if (!memcmp(str, handler->msg, strlen(handler->msg))) {
                do_msg_handler(str + strlen(handler->msg), &argptr, handler->handler);
                ret = true;
                break;
            }

        }
    }
    return ret;

}
#endif
