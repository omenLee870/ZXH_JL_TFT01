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
    /* 修复: 原只填 [0]，[1][2] 隐式补 0(=KEY_UI_HOME)，长按/双击会误触发 HOME。
     * 显式补满，未定义的动作一律给 KEY_WATCH_NO_KEY。 */
    KEY_UI_SHORTCUT,  KEY_WATCH_NO_KEY,      KEY_WATCH_NO_KEY,
#if (JLUI_KEY_EVENT_MAX > 3)
    KEY_WATCH_NO_KEY,
#endif
};

const struct ui_jlui_key_remap_table jlui_key_watch_event_table[JLUI_KEY_EVENT_TABLE_MAX ] = {
    { .key_value = KEY_IO_NUM0,      .remap_table = key_io_num0_event_table },
    { .key_value = KEY_AD_NUM0,     .remap_table = key_ad_num0_event_table },
    { .key_value = KEY_AD_NUM1,      .remap_table = key_ad_num1_event_table },
    { .key_value = 0xff }
};


/* 安全取映射表项：索引超出本板表长或该项未定义时，统一返回 KEY_WATCH_NO_KEY */
#define KEY_REMAP_GET(tbl, idx)  (((idx) < JLUI_KEY_EVENT_MAX) ? (tbl)[(idx)] : (int)KEY_WATCH_NO_KEY)

/*将key_event进行映射成具体的按键事件*/
u16 jlui_key_event_remap(struct key_event *key, const struct ui_jlui_key_remap_table *table)
{
    /* 修复: 原为 -1(0xffff)，未匹配时会经 key_event_sent_to_ui_map 投递非法事件到 UI */
    u16 key_event = KEY_WATCH_NO_KEY;
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
        /* 修复: 原为 jlui_key_watch_event_table[i]，硬编码表名导致入参 table 不生效 */
        if (table[i].key_value == key->value) {
            if (table[i].remap_table) {
                const int *remap = table[i].remap_table;
                if (key->event == KEY_ACTION_CLICK) {
                    key_event = KEY_REMAP_GET(remap, 0);
                } else if (key->event == KEY_ACTION_LONG ||
                           key->event == KEY_ACTION_HOLD ||
                           key->event == KEY_ACTION_SINGLE_LONG) {
                    /* 修复: 长按类事件原先全落 else 被吞成 KEY_WATCH_NO_KEY，
                     * 导致长按 iokey 从 750ms 到抬手 UI 全程无响应 */
                    key_event = KEY_REMAP_GET(remap, 1);
                } else if (key->event == KEY_ACTION_DOUBLE_CLICK ||
                           key->event == KEY_ACTION_DOUBLE_LONG) {
                    key_event = KEY_REMAP_GET(remap, 2);
                } else if (key->event == KEY_ACTION_TRIPLE_CLICK ||
                           key->event == KEY_ACTION_TRIPLE_LONG ||
                           key->event == KEY_ACTION_TRIPLE_LONG_3SEC ||
                           key->event == KEY_ACTION_TRIPLE_LONG_8SEC) {
                    /* 修复: 原为 remap_table[3] 无边界保护，本板 JLUI_KEY_EVENT_MAX=3 时越界读 */
                    key_event = KEY_REMAP_GET(remap, 3);
                } else {
                    /* HOLD_2SEC/3SEC/5SEC/8SEC/16SEC、UP、NO_KEY 等：
                     * 更长的长按已由 LONG(750ms) 触发过一次，此处拦截避免重复投递；
                     * UP 若投递会导致抬手再触发一次。 */
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


