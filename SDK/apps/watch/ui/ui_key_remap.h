#ifndef __UI_KEY_REMAP_H
#define __UI_KEY_REMAP_H

#if ((defined CONFIG_LVGL_UI_ENABLE) && (CONFIG_LVGL_UI_ENABLE))
struct ui_lvgl_key_remap_table {
    u8 key_value;
    const int *remap_table;
};

extern const struct ui_lvgl_key_remap_table lvgl_key_watch_event_table[];

/* ***************************************************************************/
/**
 * \Brief :        将key_event进行映射成具体的按键事件
 *
 * \Param :        key
 * \Param :        table
 *
 * \Return :
 */
/* ***************************************************************************/
u16 ui_lvgl_key_event_remap(struct key_event *key, const struct ui_lvgl_key_remap_table *table);

#endif
/* ***************************************************************************/
/**
 * \Brief :        设置key_event拦截时候,所执行发送消息回调
 *
 * \Param :        cb 发送消息函数回调
 */
/* ***************************************************************************/
void ui_key_set_send_event_cb(void (*cb)(int key_event));

void lvgl_ui_key_set_send_event_cb(int (*cb)(struct key_event *key));

#endif


