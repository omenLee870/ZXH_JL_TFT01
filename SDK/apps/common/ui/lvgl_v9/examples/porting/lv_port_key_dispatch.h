#ifndef LV_PORT_KEY_DISPATCH_H
#define LV_PORT_KEY_DISPATCH_H

#include "lvgl.h"

struct key_event;
bool lvgl_key_event_consumed(struct key_event *event);

/* 页面捷径先消费原始 UI 消息，普通按键再进入 LVGL 输入检查。 */
static inline void lv_port_key_dispatch(lv_indev_t *indev, struct key_event *event)
{
    if (lvgl_key_event_consumed(event)) {
        lv_indev_reset(indev, NULL);
        return;
    }
    lv_indev_set_user_data(indev, event);
    lv_indev_read(indev);
}

#endif
