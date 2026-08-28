#include "app_config.h"
#include "system/includes.h"
#include "app_msg.h"
#include "app_mode.h"
#include "app_main.h"
#include "key_driver.h"
#include "ui_key_remap.h"
#include "ui_api.h"
#include "jlui_app/ui_style.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI_KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/***********************************************************************************
					JL框架
***********************************************************************************/
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE)) ||(defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
struct ui_key_remap_handler {
    void (*send_event_cb)(int key_event);
};
struct ui_jlui_key_remap_table {
    u8 key_value;
    const int *remap_table;
};
static struct ui_key_remap_handler hdl;
#define __this      (&hdl)


/*lvgl只需要down up事件*/
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
#define JLUI_KEY_EVENT_MAX 4
#else
#define JLUI_KEY_EVENT_MAX 3
#endif
#define JLUI_KEY_EVENT_TABLE_MAX 6

static const int key_io_num0_event_table[JLUI_KEY_EVENT_MAX] = {
    KEY_UI_HOME,      KEY_UI_POWEROFF,       KEY_UI_MENU_LIST,
};

static const int key_ad_num0_event_table[JLUI_KEY_EVENT_MAX] = {
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
    KEY_UI_HOME,     KEY_UI_POWEROFF,        KEY_UI_MENU_LIST, KEY_UI_TRIPLE_CLICK,
#else
    KEY_UI_HOME,     KEY_UI_POWEROFF,        KEY_UI_MENU_LIST,
#endif
};

static const int key_ad_num1_event_table[JLUI_KEY_EVENT_MAX] = {
    KEY_UI_SHORTCUT,
};

const struct ui_jlui_key_remap_table jlui_key_watch_event_table[JLUI_KEY_EVENT_TABLE_MAX ] = {
    { .key_value = KEY_IO_NUM0,      .remap_table = key_io_num0_event_table },
    { .key_value = KEY_AD_NUM0,     .remap_table = key_ad_num0_event_table },
    { .key_value = KEY_AD_NUM1,      .remap_table = key_ad_num1_event_table },
    { .key_value = 0xff }
};


/*将key_event进行映射成具体的按键事件*/
u16 jlui_key_event_remap(struct key_event *key, const struct ui_jlui_key_remap_table *table)
{
    u16 key_event = -1;
    int key_value;

    if (key->event == KEY_ACTION_RDEC_ROTATE) {    // 旋转编码器
        key_value = key->value;
        if (key_value > 0) {
            key_event = KEY_UI_PLUS;     // 正转
        } else if (key_value < 0) {
            key_event = KEY_UI_MINUS;    // 反转
        }
        return key_event;
    }

    for (int i = 0; table[i].key_value != 0xff; i++) {
        if (jlui_key_watch_event_table[i].key_value == key->value) {
            if (table[i].remap_table) {
                if (key->event == KEY_ACTION_CLICK) {
                    key_event = table[i].remap_table[0];
                } else if (key->event == KEY_ACTION_HOLD) {
                    key_event = table[i].remap_table[1];
                } else if (key->event == KEY_ACTION_DOUBLE_CLICK) {
                    key_event = table[i].remap_table[2];
                } else if (key->event == KEY_ACTION_TRIPLE_CLICK) {
                    key_event = table[i].remap_table[3];
                } else {
                    key_event = KEY_WATCH_NO_KEY;
                }
            }
            break;
        }
    }
    return key_event;
}

void ui_key_set_send_event_cb(void (*cb)(int key_event))
{
    if (cb) {
        __this->send_event_cb = cb;
    }
}

static int ui_key_event_prob_handle(void *arg)
{
    struct key_event *key = (struct key_event *)arg;
    u16 key_event;
    log_debug("[%s] key->event:%d, key->value:%d", __func__, key->event, key->value);
    if (app_get_current_mode_name() == APP_MODE_UPDATE ||
        app_get_current_mode_name() == APP_MODE_RCSP) {
        log_info("%s return", __func__);
        return 0; // 传输的时候不处理
    }
    key_event = jlui_key_event_remap(key, jlui_key_watch_event_table);

    if (__this->send_event_cb) {
        __this->send_event_cb(key_event);
        return 1; /*接管事件*/
    }

    return 0; /*不接管事件*/
}
/*拦截key_driver产生的key_event*/
REGISTER_KEY_DET_CALLBACK(jlui_key) = {
    .name = NULL,
    .arg = NULL,
    .cb_deal = ui_key_event_prob_handle,
};


#endif
/***********************************************************************************
					lvgl
***********************************************************************************/

//LVGL框架
struct lvgl_key_remap_handler {
    int (*send_event_cb)(struct key_event *event);
};

static struct lvgl_key_remap_handler lv_hdl;
#define __lv_this      (&lv_hdl)

void lvgl_ui_key_set_send_event_cb(int (*cb)(struct key_event *key))
{
    if (cb) {
        __lv_this->send_event_cb = cb;
    }
}

#if ((defined CONFIG_LVGL_UI_ENABLE) && (CONFIG_LVGL_UI_ENABLE))

enum {
    KEY_IO_NUM0_DOWN,
    KEY_IO_NUM0_UP,
    KEY_AD_NUM0_DOWN,
    KEY_AD_NUM0_UP,
    KEY_AD_NUM1_DOWN,
    KEY_AD_NUM1_UP,
    KEY_AD_NUM2_DOWN,
    KEY_AD_NUM2_UP,
};
enum {
    LV_KEY_UI_MINUS,
    LV_KEY_UI_PLUS,
};


/*lvgl只需要down up事件*/
#define LVGL_KEY_EVENT_MAX 2
#define LVGL_KEY_EVENT_TABLE_MAX 6

static const int key_io_num0_event_table[LVGL_KEY_EVENT_MAX] = {
    KEY_IO_NUM0_DOWN,      KEY_IO_NUM0_UP,
};

static const int key_ad_num0_event_table[LVGL_KEY_EVENT_MAX] = {
    KEY_AD_NUM0_DOWN,      KEY_AD_NUM0_UP,
};

static const int key_ad_num1_event_table[LVGL_KEY_EVENT_MAX] = {
    KEY_AD_NUM1_DOWN,      KEY_AD_NUM1_UP,
};

static const int key_ad_num2_event_table[LVGL_KEY_EVENT_MAX] = {
    KEY_AD_NUM2_DOWN,      KEY_AD_NUM2_UP,
};

const struct ui_lvgl_key_remap_table lvgl_key_watch_event_table[LVGL_KEY_EVENT_TABLE_MAX] = {
    { .key_value = KEY_IO_NUM0,      .remap_table = key_io_num0_event_table },
    { .key_value = KEY_AD_NUM0,      .remap_table = key_ad_num0_event_table },
    { .key_value = KEY_AD_NUM1,      .remap_table = key_ad_num1_event_table },
    { .key_value = KEY_AD_NUM2,      .remap_table = key_ad_num2_event_table },
    { .key_value = 0xff }
};


/*将key_event进行映射成具体的按键事件*/
u16 ui_lvgl_key_event_remap(struct key_event *key, const struct ui_lvgl_key_remap_table *table)
{
    u16 key_event = -1;
    int key_value;

    if (key->event == KEY_ACTION_RDEC_ROTATE) {// 旋转编码器
        key_value = key->value;
        if (key_value > 0) {
            key_event = LV_KEY_UI_PLUS;     // 正转
        } else if (key_value < 0) {
            key_event = LV_KEY_UI_MINUS;    // 反转
        }
        return key_event;
    }

    for (int i = 0; table[i].key_value != 0xff; i++) {
        if (lvgl_key_watch_event_table[i].key_value == key->value) {
            if (table[i].remap_table) {
                if (key->event == KEY_ACTION_LVGL_DOWN) {
                    key_event = table[i].remap_table[0];
                } else if (key->event == KEY_ACTION_UP) {
                    key_event = table[i].remap_table[1];
                } else {
                    key_event = KEY_WATCH_NO_KEY;
                }
            }
            break;
        }
    }
    return key_event;
}

u16 lvgl_key_event_remap_default(struct key_event *key)
{

    return ui_lvgl_key_event_remap(key, lvgl_key_watch_event_table);
}


static int ui_key_event_prob_handle(void *arg)
{
    struct key_event *key = (struct key_event *)arg;
    log_debug("[%s] key->event:%d, key->value:%d", __func__, key->event, key->value);

    if (__lv_this->send_event_cb) {
        return (__lv_this->send_event_cb(key));
        //return 1; /*接管事件*/
    }
    return 0; /*不接管事件*/
}
/*拦截key_driver产生的key_event*/
REGISTER_KEY_DET_CALLBACK(lvgl_key) = {
    .name = NULL,
    .arg = NULL,
    .cb_deal = ui_key_event_prob_handle,
};

#endif


