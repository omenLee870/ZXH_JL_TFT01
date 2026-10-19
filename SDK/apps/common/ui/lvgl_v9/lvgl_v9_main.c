#if 0
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lv_ui_core.data.bss")
#pragma data_seg(".lv_ui_core.data")
#pragma const_seg(".lv_ui_core.text.const")
#pragma code_seg(".lv_ui_core.text")
#endif
#endif

#include "app_config.h"
#include "lv_conf.h"
#include "system/includes.h"
#include "lvgl.h"
#include "demos/benchmark/lv_demo_benchmark.h"
#include "demos/widgets/lv_demo_widgets.h"
/* #include "lv_port_disp.h" */
/* #include "lv_port_indev.h" */
/* #include "lv_port_fs.h" */
/* #include "lvgl_v8/lv_ui.h" */
#include "key_driver.h"
#include "event.h"
#include "rect.h"
#include "time.h"
#include "ui_key_remap.h"
/* #include "lcd/lcd_lvgl_api.h" */

#define LVGL_TASK_NAME 	"ui"
LV_IMG_DECLARE(img_cogwheel_argb);
LV_IMG_DECLARE(img_cogwheel_rgb);


#include "lvgl.h"

#include "lv_conf.h"
#include "system/includes.h"
#include "lvgl.h"
#include "demos/benchmark/lv_demo_benchmark.h"
#include "demos/widgets/lv_demo_widgets.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"
#include "lvgl_v8/lv_ui.h"
#include "key_driver.h"
#include "event.h"
#include "rect.h"
#include "time.h"
#include "ui_key_remap.h"
#include "lcd/lcd_lvgl_api.h"


#include "jlgpu_def.h"
#include "hwGpu.h"

#include "examples/porting/lv_port_disp.h"
#include "examples/porting/lv_port_indev.h"
#include "draw/jlgpu/lv_draw_jlgpu.h"

extern uint32_t timer_get_ms(void);
extern void lv_port_fs_init(void);

#define LVGL_TASK_NAME 	"ui"

void msleep(unsigned int ms);

static char lvgl_suspend_flag = 1;
static bool module_group_in_deal = false;
static bool module_msg_in_deal = false;
volatile static char lvgl_timerout_msg_remain_flag = 2;
static char lvgl_touch_msg_remain_cnt;
static char lvgl_key_msg_remain_cnt;
static char lvgl_touch_timer_status; //0:轮询定时器关闭  1:轮询定时器打开
static int lvgl_touch_timeout_id;

static u16 lvgl_timeout_id = 0;

int clock_lock(const char *name, u32 clk);

enum {
    UI_MSG_TOUCH = 1,
    UI_MSG_ENCODER,
    UI_MSG_KEY,
    UI_MSG_TIMER_TIMEOUT,
    UI_MSG_MODULE_CHANGE,
    UI_MSG_MODULE_CHANGE_REFRESH_NOW,
    UI_MSG_SUSPEND,
    UI_MSG_RESUME,
    UI_MSG_RPC_FUNC,
};
#if 1
struct touch_event {
    int32_t x;
    int32_t y;
    uint8_t status;
};
#endif
#if TCFG_TP_JD9165T_ENABLE && TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE
#include "examples/porting/lv_port_touch_queue.h"
static DEFINE_SPINLOCK(touch_queue_lock);
static struct lv_touch_queue touch_queue;
/** @brief UI 消费采集时的完整样本，不能把一次短按退化成“当前已经抬起”。 */
void lv_port_read_queued_touch(int32_t *x,int32_t *y,uint8_t *status,uint8_t *more,uint32_t *timestamp)
{
    struct lv_touch_sample sample;
    unsigned pending, fresh;
    static uint8_t previous_pressed;
    local_irq_disable();spin_lock(&touch_queue_lock);
    fresh=touch_queue.count!=0;
    lv_touch_queue_read(&touch_queue,&sample,more);
    pending=touch_queue.count;
    spin_unlock(&touch_queue_lock);local_irq_enable();
    *x=sample.x;*y=sample.y;*status=sample.pressed;
    /* 已缓存的报点使用采集时间；没有新报点时使用当前时间，让持续按住仍能触发长按。 */
    *timestamp=fresh?sample.sampled_ms:timer_get_ms();
    if (previous_pressed != sample.pressed) {
        unsigned now=timer_get_ms();
        printf("[TOUCH_UI] %s x=%d y=%d lag=%u q=%u t=%u\n",
               sample.pressed?"down":"up",sample.x,sample.y,
               now-sample.sampled_ms,pending,now);
        previous_pressed=sample.pressed;
    }
}
#endif

enum LVGL_Q_USER_TYPE {
    LVGL_Q_USER_TYPE_TOUCH = 0x10000,//从这个立即数开始是为了删消息池使用,不可更改
    LVGL_Q_USER_TYPE_KEY = 0x20000,
};

static struct key_event s_drop_key;
static struct key_event s_last_key;
static u16 lvgl_drop_key_hande_timeout_id;


void lv_port_get_touch_x_y_status(void *user_data, int32_t *x, int32_t *y, u8 *status)
{
    struct touch_event *e = (struct touch_event *)user_data;
    *x = e->x;
    *y = e->y;
    *status = e->status;
}

static void lvgl_key_drop_key_handle_timeout_cb(void *p)
{
    int msg[2 + sizeof(struct key_event) / 4];
    msg[0] = UI_MSG_KEY;
    memcpy(&msg[1], &s_drop_key, sizeof(struct key_event));


    if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
        puts("lvgl_key_event_handler post failed 0\n");
        lvgl_drop_key_hande_timeout_id = sys_timeout_add_2_task(NULL, lvgl_key_drop_key_handle_timeout_cb, 30, LVGL_TASK_NAME);
    } else {
        ++lvgl_key_msg_remain_cnt;
        lvgl_drop_key_hande_timeout_id = 0;
    }
}


/*
 * down事件不发给UI,等到按键抬起后up事件拆分为2次事件发给UI
 * 这样做的好处是能区分出短按和长按,例如按键短按和长按分别有不同UI功能时
 * */

static int lvgl_key_event_handler(struct key_event *key)
{

    int msg_p[2 + sizeof(struct key_event) / 4];

    //编码器旋钮
    if (key->event == KEY_ACTION_RDEC_ROTATE) {
        msg_p[0] = UI_MSG_ENCODER;
        memcpy(&msg_p[1], key, sizeof(struct key_event));
        int key_value_test =  key->value;

        if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg_p), msg_p)) {
            puts("lvgl_key_event_handler post failed 4");
            goto  __retry;
        } else {
            s_last_key.value = key->value;
            ++lvgl_key_msg_remain_cnt;
        }

        return 1;//接管事件
    }

    if ((key->event == KEY_ACTION_UP) && (key->value != s_last_key.value)) {
        puts("lvgl_key_event_handler post failed 1\n");
        return 1;
    }

    if (key->event == KEY_ACTION_LVGL_DOWN) {
        //按下事件不发给UI
        s_last_key.value = key->value;
        s_last_key.event = key->event;
        return -EINVAL;
    }

    if (lvgl_drop_key_hande_timeout_id) {
        puts("lvgl_key_event_handler post failed 2\n");
        return 1;
    }

    if (lvgl_key_msg_remain_cnt > 2) {
        puts("lvgl_key_event_handler post failed 3\n");
        goto __retry;
    }


    if (key->event == KEY_ACTION_UP) {
        u8 event_split = 2;

        if (s_last_key.event == KEY_ACTION_HOLD || s_last_key.event == KEY_ACTION_LONG) {
            //长按后的抬起UI只响应一次up
            event_split = 1;
        }
        //按键抬起后才开始分发2次key事件,一次down 一次up
        s_last_key.event = key->event;
        for (int i = 0; i < event_split; i++) {
            msg_p[0] = UI_MSG_KEY;
            if (event_split == 2) {
                key->event = (i == 0) ? KEY_ACTION_LVGL_DOWN : KEY_ACTION_UP;
            }

            memcpy(&msg_p[1], key, sizeof(struct key_event));

            if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg_p), msg_p))   {
                puts("lvgl_key_event_handler post failed 4");
                goto  __retry;
            } else {
                s_last_key.value = key->value;
                ++lvgl_key_msg_remain_cnt;
            }
        }
    } else if (key->event == KEY_ACTION_LONG || key->event == KEY_ACTION_HOLD || key->event == KEY_ACTION_CLICK ||
               key->event == KEY_ACTION_DOUBLE_CLICK || key->event == KEY_ACTION_TRIPLE_CLICK) {

        msg_p[0] = UI_MSG_KEY;
        memcpy(&msg_p[1], key, sizeof(struct key_event));

        s_last_key.event = key->event;
        /*printf("lvgl key event->type:%d  key->event:%d  key->value:%d", key->type, key->event, key->value); */
        if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg_p), msg_p)) {
            puts("lvgl_key_event_handler post failed 4");
            goto  __retry;
        } else {
            s_last_key.value = key->value;
            ++lvgl_key_msg_remain_cnt;
        }
    }

    return 1;

__retry:

    if ((key->event == KEY_ACTION_UP) && (key->value == s_last_key.value)) {
        memcpy(&s_drop_key, key, sizeof(struct key_event));
        lvgl_drop_key_hande_timeout_id = sys_timeout_add(NULL, lvgl_key_drop_key_handle_timeout_cb, 30);
    }

    return 1;
}
void lv_set_touch_timer_status(uint8_t status)
{
    lvgl_touch_timer_status = status;
}

static void lv_set_timer_read_touch_mode(int32_t x, int32_t y, uint8_t status)
{
    struct touch_event event;
    event.x = x;
    event.y = y;
    event.status = status;
    lv_indev_timer_read_touch(&event);
    lv_indev_set_touch_timer_en(1);
}

int lcd_touch_interrupt_event(const char *tp_task_name, int32_t x, int32_t y, uint8_t status)
{
    int err;

    if (lvgl_suspend_flag) {
        return 0;
    }
#if TCFG_TP_JD9165T_ENABLE && TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE
    unsigned sampled_ms=timer_get_ms();
    local_irq_disable();spin_lock(&touch_queue_lock);
    unsigned count_before=touch_queue.count, merged_before=touch_queue.merged;
    err=lv_touch_queue_push(&touch_queue,x,y,status);
    if (!err && (touch_queue.count!=count_before || touch_queue.merged!=merged_before)) {
        touch_queue.items[(touch_queue.head+touch_queue.count-1)%LV_TOUCH_QUEUE_CAPACITY].sampled_ms=sampled_ms;
    }
    spin_unlock(&touch_queue_lock);local_irq_enable();
    /* 本板保留 LVGL 输入定时器，避免“关闭定时器”与新触摸同时发生而丢掉唤醒。
     * 定时器只读取缓存；不在 UI 线程轮询 I2C，也不会因此强制重绘屏幕。
     */
    return err;
#endif

    //fixme:低功耗模式
    /*lv_ui_auto_shut_down_re_run();*/

    if (status) {
        //检测到触摸,创建定时器启用轮询
        if (!lvgl_touch_timer_status) {//这个标志是用来防止多次发消息
            int msg[4];
            msg[0] = UI_MSG_TOUCH;
            msg[1] = (int)x;
            msg[2] = (int)y;
            msg[3] = (int)status;

            err = os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg);
            if (err) {
                printf("lvgl touch msg drop2 err = %d\n", err);
                return err;
            }
            lvgl_touch_timer_status = 1;
        }

        return 0;
    }

    return 0;

}

static void lvgl_timer_event_timeout(void *p)
{
    int err;
    int msg[2];
    msg[0] = UI_MSG_TIMER_TIMEOUT;
    lvgl_timeout_id = 0;
    err =  task_queue_post_event(LVGL_TASK_NAME,  msg, ARRAY_SIZE(msg));
    if (err) {
        printf("lvgl_timer_event_timeout post_ui_msg err=%d\n", err);
    } else {
        /*printf("lvgl post timeout msg ok.\r\n");*/
        lvgl_timerout_msg_remain_flag = 1;
    }
}


_WEAK_ int gui_scr_action_cb_ext(int page_id, int action)
{
    return 0;
}
void gui_scr_action_cb(int page_id, int action)
{
    for (const struct ui_screen_action_handler *p = ui_screen_action_handler_begin; p < ui_screen_action_handler_end; p++) {
        if (p->page_id == page_id) {
            p->onchange(action);
            break;
        }
    }
    gui_scr_action_cb_ext(page_id, action);
}

_WEAK_ int gui_msg_action_change_ext(int32_t msg_id, int access, void *data)
{
    return 0;
}
_WEAK_ void gui_msg_action_change_guider(int32_t msg_id, int access, void *data)
{
}

typedef enum {
    VALUE_INT = 1,
    VALUE_CHAR,
    VALUE_SHORT,
    VALUE_COORD,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_COLOR,
    VALUE_DATE,
    VALUE_POINT,
    VALUE_TIME,
    VALUE_ARRAY,
} gui_msg_data_type_t;

typedef union {
    int32_t value_int;
    lv_coord_t value_coord;
    bool value_bool;
    char *value_string;
    lv_color_t value_color;
    lv_calendar_date_t value_date;
    lv_point_t value_point;
    struct tm value_time;
    struct {
        void *ptr;
        int32_t len;
    } value_array;
} gui_msg_data_t;

static void gui_varable_reg_action_change(const struct ui_varable_reg_t *p, int access, gui_msg_data_t *data, gui_msg_data_type_t data_type)
{
    if (access == 0 && p->get_set_value_cb) {
        p->get_set_value_cb(0);
    }

    switch (data_type) {
    case VALUE_CHAR:
    case VALUE_BOOL:
        if (access == 0) {
            data->value_int = *(u8 *)p->ptr;
        } else if (access == 1) {
            *(u8 *)p->ptr = data->value_int;
        }
        break;
    case VALUE_SHORT:
        if (access == 0) {
            data->value_int = *(u16 *)p->ptr;
        } else if (access == 1) {
            *(u16 *)p->ptr = data->value_int;
        }
        break;
    case VALUE_INT:
        if (access == 0) {
            data->value_int = *(u32 *)p->ptr;
        } else if (access == 1) {
            *(u32 *)p->ptr = data->value_int;
        }
        break;
    case VALUE_COORD:
        if (access == 0) {
            data->value_int = *(lv_coord_t *)p->ptr;
        } else if (access == 1) {
            *(lv_coord_t *)p->ptr = data->value_int;
        }
        break;
    case VALUE_STRING:
        if (access == 0) {
            data->value_string = p->ptr;
        } else if (access == 1) {
            strcpy(p->ptr, (char *)data->value_string);
        }
        break;
    case VALUE_COLOR:
        if (access == 0) {
            data->value_color = *(lv_color_t *)p->ptr;
        } else if (access == 1) {
            *(lv_color_t *)p->ptr = data->value_color;
        }
        break;
    case VALUE_POINT:
        if (access == 0) {
            data->value_point = *(lv_point_t *)p->ptr;
        } else if (access == 1) {
            *(lv_point_t *)p->ptr = data->value_point;
        }
        break;

    case VALUE_DATE:
        if (access == 0) {
            data->value_date = *(lv_calendar_date_t *)p->ptr;
        } else if (access == 1) {
            *(lv_calendar_date_t *)p->ptr = data->value_date;
        }
        break;
    case VALUE_TIME:
        if (access == 0) {
            data->value_time = *(struct tm *)p->ptr;
        } else if (access == 1) {
            *(struct tm *)p->ptr = data->value_time;
        }
        break;
    case VALUE_ARRAY:
        if (access == 0) {
            data->value_array.ptr = p->ptr;
            data->value_array.len = p->len;
        } else if (access == 1) {
            memcpy(p->ptr, (char *)data->value_array.ptr, data->value_array.len);
        }
        break;
    }

    if (access == 1 && p->get_set_value_cb) {
        p->get_set_value_cb(1);
    }
}

void gui_msg_action_change(int32_t msg_id, int access, void *data, gui_msg_data_type_t data_type)
{
    ASSERT(!(data == NULL && (access == 0 || access == 1)), "gui_msg_action_change data==NULL");

    char found = 0;
    for (const struct ui_module_event_handler *p = ui_module_event_handler_begin; p < ui_module_event_handler_end; p++) {
        if (p->msg_id == msg_id) {
            p->onchange(access, data);
            found = 1;
            break;
        }
    }

    if (!found) {
        for (const struct ui_varable_reg_t *p = ui_varable_reg_tab_begin; p < ui_varable_reg_tab_end; p++) {
            if (p->msg_id == msg_id) {
                gui_varable_reg_action_change(p, access, data, data_type);
                found = 1;
                break;
            }
        }
    }

    gui_msg_action_change_ext(msg_id, access, data);

    if (!found) {
        gui_msg_action_change_guider(msg_id, access, data);
    }
}


////////
_WEAK_ int gui_msg_send(int32_t msg_id, void *value, int32_t len)
{
    puts("MUST DEFINE gui_msg_send function !!!");
    return 0;
}
////////
static struct list_head list_head = LIST_HEAD_INIT(list_head);
static struct list_head group_list_head = LIST_HEAD_INIT(group_list_head);
static spinlock_t list_spinlock;
static spinlock_t group_list_spinlock;

enum LVGL_MODULE_MSG_TYPE {
    LVGL_MODULE_MSG_CONST_OR_VALUE = 0x10000,//从这个立即数开始是为了删消息池使用,不可更改
    LVGL_MODULE_MSG_PTR = 0x20000,
    LVGL_MODULE_MSG_GROUP = 0x40000,
};

typedef struct {
    struct list_head entry;
    u32 msg_id;
    u32 msg_len;
    enum LVGL_MODULE_MSG_TYPE msg_type;
    char data[];
} lvgl_module_msg_t;

typedef struct {
    struct list_head entry;
    struct list_head head;
    spinlock_t spinlock;
    u32 group_id;
} lvgl_module_msg_group_t;

int lvgl_module_msg_send_global_ptr(unsigned int msg_id, const void *ptr, unsigned int len, char refr_now)
{
    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_send_global_ptr_or_value suspended \n");
        return -1;
    }


    if (!strcmp(os_current_task(), LVGL_TASK_NAME)) {
        int error = 0;
        if (!module_msg_in_deal) {
            module_msg_in_deal = true;
            if (gui_msg_send(msg_id, (void *)ptr, len)) {
                printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, msg_id);
                error = -1;
            }
            module_msg_in_deal = false;
            return error;
        }
    }


    int err;
    int msg[5];
    msg[0] = refr_now ? UI_MSG_MODULE_CHANGE_REFRESH_NOW : UI_MSG_MODULE_CHANGE;
    msg[1] = LVGL_MODULE_MSG_CONST_OR_VALUE;
    msg[2] = msg_id;
    msg[3] = (int)ptr;
    msg[4] = len;

    err = os_taskq_del_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_CONST_OR_VALUE | msg_id);
    if (err == 0) {
        printf("lvgl_module_msg_send_global_ptr_or_value[%u] del\n", msg_id);
    }
    err =  os_taskq_post_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_CONST_OR_VALUE | msg_id, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_module_msg_send_global_ptr_or_value err=%d\n", err);
    }
    return err;

}
int lvgl_module_msg_send_string(unsigned int msg_id, const char *string, char refr_now)
{
    return lvgl_module_msg_send_global_ptr(msg_id, (const void *)string, 4, refr_now);
}
int lvgl_module_msg_send_value(unsigned int msg_id, unsigned int value, char refr_now)
{
    return lvgl_module_msg_send_global_ptr(msg_id, (const void *)value, 4, refr_now);
}
void *lvgl_module_msg_get_ptr(unsigned int msg_id, unsigned int len)
{
    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_get_ptr suspended \n");
        return NULL;
    }

    lvgl_module_msg_t *p = malloc(sizeof(lvgl_module_msg_t) + len);
    LV_ASSERT_MALLOC(p);
    p->msg_id = msg_id;
    p->msg_len = len;
    return p->data;
}

int lvgl_module_msg_send_ptr(void *ptr, char refr_now)
{
    if (ptr == NULL) {
        return -1;
    }

    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_send_ptr suspended \n");
        free(ptr);
        return -1;
    }

    lvgl_module_msg_t *p = (lvgl_module_msg_t *)ptr - 1;

    if (!strcmp(os_current_task(), LVGL_TASK_NAME)) {
        int error = 0;
        if (!module_msg_in_deal) {
            module_msg_in_deal = true;
            if (gui_msg_send(p->msg_id, (void *)p->data, p->msg_len)) {
                printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
                error = -1;
            }
            free(p);
            module_msg_in_deal = false;
            return error;
        }
    }

    spin_lock(&list_spinlock);
    list_add_tail(&p->entry, &list_head);
    spin_unlock(&list_spinlock);

    int err;
    int msg[3];
    msg[0] = refr_now ? UI_MSG_MODULE_CHANGE_REFRESH_NOW : UI_MSG_MODULE_CHANGE;
    msg[1] = LVGL_MODULE_MSG_PTR;
    msg[2] = (int)p;

    err = os_taskq_del_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_PTR | p->msg_id);
    if (err == 0) {
        printf("lvgl_module_msg[%d] del\n", p->msg_id);

        //同时把链表里面的节点内存释放掉
        struct list_head *pos, *node;
        lvgl_module_msg_t *oldp;
        char find = 0;
        spin_lock(&list_spinlock);
        list_for_each_safe(pos, node, &list_head) {
            oldp = list_entry(pos, lvgl_module_msg_t, entry);
            if (oldp && oldp->msg_id == p->msg_id) {
                list_del(&oldp->entry);
                spin_unlock(&list_spinlock);
                free(oldp);
                find = 1;
                break;
            }
        }
        ASSERT(find, "%s list_del failed!\n", __FUNCTION__);
        if (!find) {
            spin_unlock(&list_spinlock);
        }

    }
    err =  os_taskq_post_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_PTR | p->msg_id, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_module_msg_send err=%d\n", err);

        spin_lock(&list_spinlock);
        list_del(&p->entry);
        spin_unlock(&list_spinlock);
        free(p);
    }
    return err;
}


void *lvgl_module_group_create(unsigned int group_id)
{
    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_group_create suspended \n");
        return NULL;
    }

    lvgl_module_msg_group_t *pgroup = malloc(sizeof(lvgl_module_msg_group_t));
    LV_ASSERT_MALLOC(pgroup);

    spin_lock_init(&pgroup->spinlock);
    INIT_LIST_HEAD(&pgroup->head);
    pgroup->group_id = group_id;

    spin_lock(&group_list_spinlock);
    list_add_tail(&pgroup->entry, &group_list_head);
    spin_unlock(&group_list_spinlock);

    return pgroup;

}

int lvgl_module_msg_group_add_global_ptr(void *group, unsigned int msg_id, const void *ptr, unsigned int len)
{
    if (group == NULL) {
        return -1;
    }

    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_group_add_global_ptr_or_value suspended \n");
        return -1;
    }

    lvgl_module_msg_group_t *pgroup = group;

    lvgl_module_msg_t *p = malloc(sizeof(lvgl_module_msg_t) + 4);
    LV_ASSERT_MALLOC(p);
    p->msg_type = LVGL_MODULE_MSG_CONST_OR_VALUE;
    p->msg_id = msg_id;
    p->msg_len = len;
    *(int *)p->data = (int)ptr;

    spin_lock(&pgroup->spinlock);
    list_add_tail(&p->entry, &pgroup->head);
    spin_unlock(&pgroup->spinlock);

    return 0;
}
int lvgl_module_msg_group_add_value(void *group, unsigned int msg_id, unsigned int value)
{
    return lvgl_module_msg_group_add_global_ptr(group, msg_id, (void *)value, 4);
}

int lvgl_module_msg_group_add_string(void *group, unsigned int msg_id, const char *string)
{
    return lvgl_module_msg_group_add_global_ptr(group, msg_id, (const void *)string, 4);
}

void *lvgl_module_msg_group_add_ptr(void *group, unsigned int msg_id, unsigned int len)
{
    if (group == NULL) {
        return NULL;
    }

    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_group_add_ptr suspended \n");
        return NULL;
    }

    lvgl_module_msg_group_t *pgroup = group;

    lvgl_module_msg_t *p = malloc(sizeof(lvgl_module_msg_t) + len);
    LV_ASSERT_MALLOC(p);
    p->msg_type = LVGL_MODULE_MSG_PTR;
    p->msg_id = msg_id;
    p->msg_len = len;

    spin_lock(&pgroup->spinlock);
    list_add_tail(&p->entry, &pgroup->head);
    spin_unlock(&pgroup->spinlock);

    return p->data;
}

int lvgl_module_msg_group_send(void *group, char refr_now)
{
    if (group == NULL) {
        return -1;
    }

    lvgl_module_msg_group_t *pgroup = group;

    if (lvgl_suspend_flag) {
        printf("lvgl_module_msg_group_send suspended \n");

        spin_lock(&group_list_spinlock);
        list_del(&pgroup->entry);
        spin_unlock(&group_list_spinlock);

        lvgl_module_msg_t *p, *n;
        list_for_each_entry_safe(p, n, &pgroup->head, entry) {
            free(p);
        }

        free(pgroup);

        return -1;
    }

    if (!strcmp(os_current_task(), LVGL_TASK_NAME)) {
        if (!module_group_in_deal) {
            module_group_in_deal = true;

            spin_lock(&group_list_spinlock);
            list_del(&pgroup->entry);
            spin_unlock(&group_list_spinlock);

            lvgl_module_msg_t *p, *n;
            int error = 0;
            list_for_each_entry_safe(p, n, &pgroup->head, entry) {
                if (p->msg_type == LVGL_MODULE_MSG_CONST_OR_VALUE) {
                    if (gui_msg_send(p->msg_id, (void *)(*(int *)p->data), p->msg_len)) {
                        printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
                        error = -1;
                    }
                } else if (p->msg_type == LVGL_MODULE_MSG_PTR) {
                    if (gui_msg_send(p->msg_id, (void *)p->data, p->msg_len)) {
                        printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
                        error = -1;
                    }
                }
                free(p);
            }
            free(pgroup);

            module_group_in_deal = false;
            return error;
        }
    }

    int err;
    int msg[3];
    msg[0] = refr_now ? UI_MSG_MODULE_CHANGE_REFRESH_NOW : UI_MSG_MODULE_CHANGE;
    msg[1] = LVGL_MODULE_MSG_GROUP;
    msg[2] = (int)pgroup;

    err = os_taskq_del_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_GROUP | pgroup->group_id);
    if (err == 0) {
        printf("lvgl_module_msg[%d] del\n", pgroup->group_id);

        //同时把组链表里面的节点内存释放掉
        struct list_head *pos, *node;
        lvgl_module_msg_group_t *oldg;
        char find = 0;
        spin_lock(&group_list_spinlock);
        list_for_each_safe(pos, node, &group_list_head) {
            oldg = list_entry(pos, lvgl_module_msg_group_t, entry);
            if (oldg && oldg->group_id == pgroup->group_id) {
                list_del(&oldg->entry);
                spin_unlock(&group_list_spinlock);

                lvgl_module_msg_t *oldp;
                //同时把链表里面的节点内存释放掉,链表已经脱钩,不需要保护
                list_for_each_safe(pos, node, &oldg->head) {
                    oldp = list_entry(pos, lvgl_module_msg_t, entry);
                    list_del(&oldp->entry);
                    free(oldp);
                }

                free(oldg);
                find = 1;
                break;
            }
        }
        ASSERT(find, "%s list_del failed!\n", __FUNCTION__);
        if (!find) {
            spin_unlock(&group_list_spinlock);
        }

    }
    err =  os_taskq_post_type(LVGL_TASK_NAME, Q_MSG | LVGL_MODULE_MSG_GROUP | pgroup->group_id, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_module_group_send err=%d\n", err);

        spin_lock(&group_list_spinlock);
        list_del(&pgroup->entry);
        spin_unlock(&group_list_spinlock);

        lvgl_module_msg_t *p, *n;
        list_for_each_entry_safe(p, n, &pgroup->head, entry) {
            free(p);
        }

        free(pgroup);
    }
    return err;
}

int lvgl_module_varable_reg_send(unsigned int msg_id, char refr_now)
{
    for (const struct ui_varable_reg_t *p = ui_varable_reg_tab_begin; p < ui_varable_reg_tab_end; p++) {
        if (p->msg_id == msg_id) {
            lvgl_module_msg_send_global_ptr(msg_id, p->ptr, p->len, refr_now);
            break;
        }
    }

    return 0;
}


int lvgl_module_varable_reg_group_add(void *group, unsigned int msg_id)
{
    for (const struct ui_varable_reg_t *p = ui_varable_reg_tab_begin; p < ui_varable_reg_tab_end; p++) {
        if (p->msg_id == msg_id) {
            lvgl_module_msg_group_add_global_ptr(group, msg_id, p->ptr, p->len);
            break;
        }
    }

    return 0;
}

int lvgl_module_varable_reg_group_send(void *group, char refr_now)
{
    return lvgl_module_msg_group_send(group, refr_now);
}

static void lvgl_module_msg_deal(int *msg)
{
    if (msg[1] == LVGL_MODULE_MSG_CONST_OR_VALUE) {
        if (gui_msg_send(msg[2], (void *)msg[3], msg[4])) {
            printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, msg[2]);
        }

    } else if (msg[1] == LVGL_MODULE_MSG_PTR) {
        lvgl_module_msg_t *p = (lvgl_module_msg_t *)msg[2];
        if (gui_msg_send(p->msg_id, (void *)p->data, p->msg_len)) {
            printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
        }
        spin_lock(&list_spinlock);
        list_del(&p->entry);
        spin_unlock(&list_spinlock);
        free(p);
    } else if (msg[1] == LVGL_MODULE_MSG_GROUP) {
        lvgl_module_msg_group_t *pgroup = (lvgl_module_msg_group_t *)msg[2];

        spin_lock(&group_list_spinlock);
        list_del(&pgroup->entry);
        spin_unlock(&group_list_spinlock);

        lvgl_module_msg_t *p, *n;
        list_for_each_entry_safe(p, n, &pgroup->head, entry) {
            if (p->msg_type == LVGL_MODULE_MSG_CONST_OR_VALUE) {
                if (gui_msg_send(p->msg_id, (void *)(*(int *)p->data), p->msg_len)) {
                    printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
                }
            } else if (p->msg_type == LVGL_MODULE_MSG_PTR) {
                if (gui_msg_send(p->msg_id, (void *)p->data, p->msg_len)) {
                    printf("%s[%d] gui msg_id[%d] had unsubscribe\r\n", __func__, __LINE__, p->msg_id);
                }
            }
            free(p);
        }
        free(pgroup);
    }
}

static void lvgl_rpc_post_func_deal(int msg[])
{
    if (msg[1] == 0) {
        ((void (*)(void))msg[2])();
    } else if (msg[1] == 1) {
        ((void (*)(int))msg[2])(msg[3]);
    } else if (msg[1] == 2) {
        ((void (*)(int, int))msg[2])(msg[3], msg[4]);
    } else if (msg[1] == 3) {
        ((void (*)(int, int, int))msg[2])(msg[3], msg[4], msg[5]);
    } else if (msg[1] == 4) {
        ((void (*)(int, int, int, int))msg[2])(msg[3], msg[4], msg[5], msg[6]);
    } else if (msg[1] == 5) {
        ((void (*)(int, int, int, int, int))msg[2])(msg[3], msg[4], msg[5], msg[6], msg[7]);
    }
}

int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...)
{
    int err;

    if (lvgl_suspend_flag) {
        printf("lvgl_rpc_post suspended \n");
        return -1;
    }

    int msg[8];

    msg[0] = UI_MSG_RPC_FUNC;
    msg[1] = argc;
    msg[2] = (int)rpc_func;

    ASSERT(argc <= ARRAY_SIZE(msg) - 3, "%s argc too long!\n", __FUNCTION__);

    va_list argptr;
    va_start(argptr, argc);
    for (int i = 0; i < argc; i++) {
        msg[i + 3] = va_arg(argptr, int);
    }
    va_end(argptr);


_RETRY:
    err = os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_rpc_post_func err=%d\n", err);
        msleep(20);
        goto _RETRY;
    }
    return 0;
}



/**
 * @brief    lvgl 暂停函数
 * @param:   state: 1:开 0:关
 * @param:   cb: 执行成功的回调函数
 * @return:  none
 **/
void lvgl_ui_task_suspend_resume(uint8_t state)
{
    OS_SEM sem_wait;
    int err;
    int msg[2];
    u8 is_need_wait = 1;
    msg[0] = state ? UI_MSG_RESUME : UI_MSG_SUSPEND;
    msg[1] = (int)&sem_wait;

    if (!strcmp(os_current_task(), LVGL_TASK_NAME)) {
        //当前如果不在lvgl线程则需要等待
        is_need_wait = 0;
        msg[1] = 0;
    }


    os_sem_create(&sem_wait, 0);
    err =  os_taskq_post_type(LVGL_TASK_NAME, Q_MSG, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_ui_start_stop post err=%d\n", err);
    } else {
        if (is_need_wait) {
            err = os_sem_pend(&sem_wait, 300);
            ASSERT(err != OS_TIMEOUT, "%s %s no response\n", __FUNCTION__, state ? "RESUME" : "SUSPEND");
        }
    }
    os_sem_del(&sem_wait, OS_DEL_ALWAYS);
}

u8 lvgl_ui_is_suspended(void)
{
    return lvgl_suspend_flag;
}

_WEAK_ void jl_gui_init(void)
{
#if LV_USE_MONKEY != 0
    /*Create pointer monkey test*/
    lv_monkey_config_t monkey_pointer_config;
    monkey_pointer_config.type = LV_INDEV_TYPE_POINTER;
    monkey_pointer_config.period_range.min = 20;
    monkey_pointer_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_pointer_config), true);

    /*Create keypad monkey test*/
    lv_monkey_config_t monkey_keypad_config;
    monkey_keypad_config.type = LV_INDEV_TYPE_KEYPAD;
    monkey_keypad_config.period_range.min = 20;
    monkey_keypad_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_keypad_config), true);

    /*Create encoder monkey test*/
    lv_monkey_config_t monkey_encode_config;
    monkey_encode_config.type = LV_INDEV_TYPE_ENCODER;
    monkey_encode_config.period_range.min = 20;
    monkey_encode_config.period_range.max = 20;
    monkey_encode_config.input_range.min = -5;
    monkey_encode_config.input_range.max = 5;
    lv_monkey_set_enable(lv_monkey_create(&monkey_encode_config), true);
#endif

#if LV_USE_DEMO_BENCHMARK
    lv_demo_benchmark();
#elif LV_USE_DEMO_WIDGETS
    lv_demo_widgets();
#else
    puts("\r\n\r\n LVGL APP NOT DEFINE...");
#endif
}

static void lvgl_main_task(void *priv)
{
    clock_lock("ui_show", clk_get_max_frequency());
    lv_init();
    lv_tick_set_cb(&timer_get_ms);
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();

    lvgl_ui_key_set_send_event_cb(lvgl_key_event_handler);
    lvgl_suspend_flag = 0;

    jl_gui_init();

    /* RPC 消息: 类型占 1 槽, 数据 8 槽 (argc=5: rpc+5 vararg) 需 9 槽, 8 槽会丢第 5 个 vararg */
    int msg[12] = {0};
    int ret;
    lvgl_touch_timer_status = 0;
    char lvgl_task_pend_timeout_refr_flag = 0;
    u32 time_till_next;
    u8 timeout = 3;

    //刷新第一帧不需要事件触发
    time_till_next = lv_timer_handler();
    printf("@@@@@@@@@@@ time_till_next: %d\n", time_till_next);
    if (time_till_next != 0xFFFFFFFF) {
        sys_hi_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next);
    }


    while (1) {
        ret = os_taskq_pend_timeout(NULL, msg, ARRAY_SIZE(msg), timeout); //超时是为了模型改变，但未触发刷屏，设置最大延时30毫秒
        /* printf("%s() %d, ret: %d\n", __FUNCTION__, __LINE__, ret); */
        if (ret != OS_TASKQ && ret != OS_TIMEOUT) {
            printf("lvgl_main_task os_taskq_pend err=%d %d\n", ret, msg[0]);
            continue;
        }


        if (ret == OS_TIMEOUT) {
            if (lvgl_task_pend_timeout_refr_flag == 0) {
                /*puts("ui task pend timeout, nothing happend\r\n");*/
                continue; //如果没有模型改变消息，不需要刷新屏幕
            } else {
                puts("ui task pend timeout, module change happend\r\n");
            }
            lvgl_task_pend_timeout_refr_flag = 0;
        } else if (msg[1] == UI_MSG_TOUCH) {
            lv_set_timer_read_touch_mode((u16)msg[2], (u16)msg[3], (u8)msg[4]);
        } else if (msg[1] == UI_MSG_KEY) {
            --lvgl_key_msg_remain_cnt;
//fixme:低功耗模式
#if 0
            if (lv_get_screen_saver_status()) {
                //唤醒lvgl
                lvgl_ui_task_suspend_resume(1);
                continue;
            } else {
                lv_ui_auto_shut_down_re_run();
            }
            if (lvgl_suspend_flag) {
                continue;
            }
#endif
            /*printf("lvgl_key_msg_remain_cnt = %d\r\n", lvgl_key_msg_remain_cnt);*/
            struct key_event *e = (struct key_event *)&msg[2];
            lv_indev_timer_read_key(e);
        } else if (msg[1] == UI_MSG_ENCODER) {
            --lvgl_key_msg_remain_cnt;
            struct key_event *e = (struct key_event *)&msg[2];
            lv_indev_timer_read_encoder(e);
        } else if (msg[1] == UI_MSG_MODULE_CHANGE || msg[1] == UI_MSG_MODULE_CHANGE_REFRESH_NOW) {

            lvgl_module_msg_deal(&msg[1]);

            if (msg[1] == UI_MSG_MODULE_CHANGE) {

                if (timer_get_ms() - lv_get_timer_handler_start_time_ms() < 20) {//对于不需要立刻刷屏的模型改变的消息,限制最大间隔20毫秒刷屏一次
                    /*printf("UI_MSG_MODULE_CHANGE DELAY");*/
                    lvgl_task_pend_timeout_refr_flag = 1;
                    continue;
                } else {
                    /*printf("UI_MSG_MODULE_CHANGE REFRESH NOW");*/
                }
            }

        } else if (msg[1] == UI_MSG_TIMER_TIMEOUT) {
            lvgl_timerout_msg_remain_flag = 2; //标记LVGL定时器超时消息已经消耗掉
        } else if (msg[1] == UI_MSG_SUSPEND) {
            timeout = 0;
            lvgl_suspend_flag = 1;

            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            continue;
        } else if (msg[1] == UI_MSG_RESUME) {
            timeout = 3;
            lvgl_suspend_flag = 0;
            //fixme:低功耗模式
            /*lv_screen_recover();*/
            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
        } else if (msg[1] == UI_MSG_RPC_FUNC) {
            lvgl_rpc_post_func_deal(&msg[1]);
        }

        if (lvgl_suspend_flag) {
            time_till_next = 0xFFFFFFFF;
        } else {
            time_till_next = lv_timer_handler();
        }

        if (time_till_next == 0) {//没有时间释放CPU了
            /*printf("lvgl time_till_next == 0\r\n");*/
        }

        if (lvgl_key_msg_remain_cnt <= 0) { //如果按键事件还没消耗完，利用按键事件来调度运行，不需要超时事件来调度, LVGL任务会持续运转
            /*printf("lvgl time_till_next %d ms, lvgl_timerout_msg_remain_flag=%d\r\n", time_till_next,lvgl_timerout_msg_remain_flag); */
            if (time_till_next != 0xFFFFFFFF) {//如果LVGL内部有注册超时定时器
                if (lvgl_timerout_msg_remain_flag == 2) { //定时器超时消息已经被消耗掉
                    lvgl_timerout_msg_remain_flag = 0;
                    /* printf("lvgl add timeout msg %d\r\n",time_till_next); */
                    lvgl_timeout_id = sys_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next < 3 ? 3 : time_till_next); //FIX_ME:系统未支持1ms时间超时释放cpu
                } else if (lvgl_timerout_msg_remain_flag == 1) { //超时函数已跑起，并且post消息成功，但是消息还在队列内未消耗
                    lvgl_timerout_msg_remain_flag = 0;
                    if (os_taskq_del_type(LVGL_TASK_NAME, Q_EVENT)) {
                        //puts("lvgl_timeout_event_del err\r\n"); //有可能由于超时定时器和lvgl任务不同gpu导致lvgl_timerout_msg_remain_flag执行乱序,不过不会造成不良影响
                    }
                    /* printf("lvgl add timeout msg %d\r\n", time_till_next); */
                    lvgl_timeout_id = sys_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next < 3 ? 3 : time_till_next); //FIX_ME:系统未支持1ms时间超时释放cpu
                } else { //超时函数未跑起
                    if (lvgl_timeout_id) {
                        sys_timer_modify(lvgl_timeout_id, time_till_next < 3 ? 3 : time_till_next); //有可能由于超时函数已经跑起修改不成功,但是也没所谓,  主要担心修改到其他ID的注册定时器
                    }
                }
            }
        }

    } // while (1)
}

int lvgl_ui_init(void)
{
    puts("lvgl9_main_task_init \n\n");
    return task_create(lvgl_main_task, NULL, LVGL_TASK_NAME);
}

