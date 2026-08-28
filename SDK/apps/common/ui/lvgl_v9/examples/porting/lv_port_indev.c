#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lv_ui_core.data.bss")
#pragma data_seg(".lv_ui_core.data")
#pragma const_seg(".lv_ui_core.text.const")
#pragma code_seg(".lv_ui_core.text")
#endif

/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_ui.h"
#include "lv_port_indev.h"
#include "src/indev/lv_indev_private.h"
#include "src/misc/lv_timer_private.h"
#include "lvgl.h"
/*********************
 *      DEFINES
 *********************/
#define TOUCHPAD_USE_TIEMR 0
#define ABS(x) ((x)>0?(x):-(x))
#define BOOL_DEFINE_CONFLICT
#include "lcd/lcd_conf.h"
#include "key_driver.h"
#include "app_msg.h"
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

extern uint32_t timer_get_ms(void);
static void touchpad_init(void);
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(int32_t *x, int32_t *y);

static void mouse_init(void);
static void mouse_read(lv_indev_t *indev, lv_indev_data_t *data);
static bool mouse_is_pressed(void);
static void mouse_get_xy(int32_t *x, int32_t *y);

static void keypad_init(void);
static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data);
static void encoder_handler(void);

static void button_init(void);
static void button_read(lv_indev_t *indev, lv_indev_data_t *data);
static int8_t button_get_pressed_id(void);
static bool button_is_pressed(uint8_t id);

extern void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status);
/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t *indev_touchpad;
lv_indev_t *indev_mouse;
lv_indev_t *indev_keypad;
lv_indev_t *indev_encoder;
lv_indev_t *indev_button;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;
static int32_t g_first_touch_x = 0; //记录每次触摸首次按下坐标

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#if LV_USE_SIM_INERTIAL_SLIDE
static uint8_t touch_cnt = 0;            //连续触摸计数
static lv_indev_data_t g_touch_data;//模拟坐标
static uint8_t lv_indev_sim_status = 0;  //是否处于模拟滑动
static uint8_t lv_indev_sim_enable = 1;  //是否使能模拟滑动

struct lv_scroll_throw {
    short final_interval_x; //最后的x间隔
    short final_interval_y; //最后的y间隔
    float ratio;
};
static struct lv_scroll_throw lv_throw = {0};
#endif


bool lv_indev_set_touch_timer_check(void)
{
    if (indev_touchpad->read_timer) {
        return true;
    } else {
        return false;
    }
}


void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

#if !TOUCHPAD_USE_TIEMR
    lv_timer_del(indev_touchpad->read_timer);
    indev_touchpad->read_timer = NULL;
#endif
    /*------------------
     * Mouse
     * -----------------*/
#if 0
    /*Initialize your mouse if you have*/
    mouse_init();

    /*Register a mouse input device*/
    indev_mouse = lv_indev_create();
    lv_indev_set_type(indev_mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_mouse, mouse_read);

    /*Set cursor. For simplicity set a HOME symbol now.*/
    lv_obj_t *mouse_cursor = lv_image_create(lv_screen_active());
    lv_image_set_src(mouse_cursor, LV_SYMBOL_HOME);
    lv_indev_set_cursor(indev_mouse, mouse_cursor);
#endif
    /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();

    /*Register a keypad input device*/
    indev_keypad = lv_indev_create();
    lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_keypad, keypad_read);
    lv_timer_del(indev_keypad->read_timer);
    indev_keypad->read_timer = NULL;
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    indev_keypad->group = g;


    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/

    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read);
    lv_timer_del(indev_encoder->read_timer);
    indev_keypad->read_timer = NULL;


    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/

#if 0
    /*------------------
     * Button
     * -----------------*/

    /*Initialize your button if you have*/
    button_init();

    /*Register a button input device*/
    indev_button = lv_indev_create();
    lv_indev_set_type(indev_button, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(indev_button, button_read);

    /*Assign buttons to points on the screen*/
    static const lv_point_t btn_points[2] = {
        {10, 10},   /*Button 0 -> x:10; y:10*/
        {40, 100},  /*Button 1 -> x:40; y:100*/
    };
    lv_indev_set_button_points(indev_button, btn_points);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/
void lv_indev_timer_read_touch(void *user_data)
{
    lv_timer_t timer;
    timer.user_data = indev_touchpad;
    indev_touchpad->user_data = user_data;
    lv_indev_read_timer_cb(&timer);
}

void lv_indev_set_touch_timer_en(int en)
{
    if (en) {
        if (indev_touchpad->read_timer == NULL) {
            indev_touchpad->read_timer = lv_timer_create(lv_indev_read_timer_cb, LV_DEF_REFR_PERIOD, indev_touchpad);
        }
    } else {
        if (indev_touchpad->read_timer) {
            lv_timer_del(indev_touchpad->read_timer);
            indev_touchpad->read_timer = NULL;
        }
        void lv_set_touch_timer_status(uint8_t status);
        lv_set_touch_timer_status(0);
    }
}

void lv_indev_timer_read_key(void *user_data)
{
    lv_timer_t timer;
    timer.user_data = indev_keypad;
    indev_keypad->user_data = user_data;
    lv_indev_read_timer_cb(&timer);
}

#if LV_USE_SIM_INERTIAL_SLIDE
void lv_set_sim_inertial_slide_status(uint8_t status)
{
    lv_indev_sim_enable = status;
}
#endif

_WEAK_ void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status)
{

}

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
#if TOUCHPAD_USE_TIEMR

    static int32_t last_x = 0;
    static int32_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    if (touchpad_is_pressed()) {
        touchpad_get_xy(&last_x, &last_y);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;

#else

    uint8_t status;
    static uint32_t touch_time = 0; //记录滑动抬起后的时刻
    static uint8_t g_prev_touch_status = 0; //上一轮触摸状态

#if LV_USE_SIM_INERTIAL_SLIDE
    lv_indev_data_t touch_data;
#endif

    /*Save the pressed coordinates and the state*/
    if (indev_drv->read_timer == NULL) {
        /* putchar('I'); */
        extern void lv_port_get_touch_x_y_status(void *user_data, int32_t *x, int32_t *y, uint8_t *status);
        lv_port_get_touch_x_y_status(indev_drv->user_data, &data->point.x, &data->point.y, &status);
#if LV_USE_SIM_INERTIAL_SLIDE
        if (lv_indev_sim_enable) {
            touch_data.point.x = data->point.x;
            touch_data.point.y = data->point.y;
        }
#endif
    } else {
        /* putchar('T'); */
#if LV_USE_SIM_INERTIAL_SLIDE

        if (!lv_indev_sim_enable) {
            get_touch_x_y_status(&data->point.x, &data->point.y, &status);
            if (status) {
                data->state = LV_INDEV_STATE_PR;
                touch_time = timer_get_ms();
            } else {
                data->state = LV_INDEV_STATE_REL;
                //检测到抬起,延时删除轮询定时器
                if (timer_get_ms() - touch_time > 600 && touch_time != 0) {
                    lv_indev_set_touch_timer_en(0);
                }
            }
            return;
        }

        get_touch_x_y_status(&touch_data.point.x, &touch_data.point.y, &status);

        //处于模拟惯性滑动且没有再按下:模拟坐标启用
        if (lv_indev_sim_status && !status) {
            lv_throw.ratio += 0.2f; //模拟加速
            g_touch_data.point.x += lv_throw.final_interval_x * lv_throw.ratio; //g_touch_data lvgl内部读取
            g_touch_data.point.y += lv_throw.final_interval_y * lv_throw.ratio;

            if (g_touch_data.point.x > LCD_WIDTH || g_touch_data.point.x < 0 || \
                g_touch_data.point.y > LCD_HEIGHT || g_touch_data.point.y < 0 || lv_throw.ratio > 2) {

                g_touch_data.point.x = (g_touch_data.point.x >= 0 && g_touch_data.point.x <= LCD_WIDTH) ? \
                                       g_touch_data.point.x : (g_touch_data.point.x < 0 ? 0 : LCD_WIDTH);
                g_touch_data.point.y = (g_touch_data.point.y >= 0 && g_touch_data.point.y <= LCD_HEIGHT) ? \
                                       g_touch_data.point.y : (g_touch_data.point.y < 0 ? 0 : LCD_HEIGHT);
                g_touch_data.state = LV_INDEV_STATE_REL;

                lv_indev_sim_status = 0;
                lv_throw.ratio = 1.0f;
                touch_time = timer_get_ms();
            }

            data->point.x = g_touch_data.point.x;
            data->point.y = g_touch_data.point.y;
            data->state = g_touch_data.state;

            return;
        }//if(lv_indev_sim_status && status){

        if (!status) { //touch抬起
            if (timer_get_ms() - touch_time > 1000 && touch_time != 0) {
                lv_indev_set_touch_timer_en(0); //抬起后超过1s关闭定时器
            }

            if (touch_cnt < 2 || (ABS(lv_throw.final_interval_y) < 20 && ABS(lv_throw.final_interval_x) < 10)) {
                touch_cnt = 0;
                goto _GET_DATA_DONE;
            }


            //惯性滑动生效
            lv_indev_sim_status = 1;

            touch_cnt = 0;

            //lvgl坐标
            data->point.x = touch_data.point.x + lv_throw.final_interval_x;
            data->point.y = touch_data.point.y + lv_throw.final_interval_y;
            data->state = LV_INDEV_STATE_PR;//设置为按下（模拟）

            //模拟坐标
            g_touch_data.point.x = data->point.x;
            g_touch_data.point.y = data->point.y;
            g_touch_data.state = data->state;

            return;
        } else {//touch按下
            lv_indev_sim_status = 0;
            lv_throw.ratio = 1.0f;
            touch_time = timer_get_ms();
        }

        //处于按下状态,累加触摸连续次数,计算最后触摸距离
        touch_cnt++;
        lv_throw.final_interval_x = touch_data.point.x - g_touch_data.point.x;
        lv_throw.final_interval_y = touch_data.point.y - g_touch_data.point.y;
    }

_GET_DATA_DONE:
    //lvgl坐标
    data->point.x = touch_data.point.x;
    data->point.y = touch_data.point.y;
    if (status) {
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    //模拟坐标
    g_touch_data.point.x = data->point.x;
    g_touch_data.point.y = data->point.y;
    g_touch_data.state = data->state;

#else

        get_touch_x_y_status(&data->point.x, &data->point.y, &status);
    }
    // T0: 首次触摸按下，记录坐标
    if (status && !g_prev_touch_status)
    {
        g_first_touch_x = data->point.x;
    }
    g_prev_touch_status = status;
    if (status)
    {
        data->state = LV_INDEV_STATE_PR;
        touch_time = timer_get_ms();
    } else
    {
        data->state = LV_INDEV_STATE_REL;
        //检测到抬起,延时删除轮询定时器
        if (timer_get_ms() - touch_time > 600 && touch_time != 0) {
            lv_indev_set_touch_timer_en(0);
        }
    }
#endif

#endif
}

/*返回本次触摸首次按下的x坐标*/
int32_t lv_port_get_first_touch_x(void)
{
    return g_first_touch_x;
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    /*Your code comes here*/

    int32_t x = 0;
    int32_t y = 0;
    uint8_t status;
    get_touch_x_y_status(&x, &y, &status);

    if (status) {
        return true;
    } else {
        return false;
    }

}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(int32_t *x, int32_t *y)
{
    /*Your code comes here*/
    uint8_t status;
    get_touch_x_y_status(x, y, &status);
}

/*------------------
 * Mouse
 * -----------------*/

/*Initialize your mouse*/
static void mouse_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void mouse_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the mouse button is pressed or released*/
    if (mouse_is_pressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*Return true is the mouse button is pressed*/
static bool mouse_is_pressed(void)
{
    /*Your code comes here*/

    return false;
}

/*Get the x and y coordinates if the mouse is pressed*/
static void mouse_get_xy(int32_t *x, int32_t *y)
{
    /*Your code comes here*/

    (*x) = 0;
    (*y) = 0;
}

/*------------------
 * Keypad
 * -----------------*/

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
}

//=======================================================//
// LVGL按键值重新映射函数:
// 用户可以在custom.c中实现该函数把LVGL的按键重新映射
// key_value为映射前的键值，key_remap为映射后的键值
//=======================================================//
_WEAK_ void lvgl_key_value_remap(u8 key_value, uint32_t *key_remap)
{
    return;
}

_WEAK_ int ui_scr_key_event_handler_ext(struct key_event *event)
{
    //返回1时，则将按键信息交给外部处理
    //默认是所有的非DOWN和UP事件都会发回给app
    return 1;
}


/* UI页面事件处理函数 */
static int ui_scr_key_event_handler(struct key_event *key, u8 long_handled)
{

#if 1
    //有注册页面注册函数
    gui_scr_t *act_screen = gui_scr_get_act();
    if (!act_screen) {
        if (key->event == KEY_ACTION_UP && long_handled) {
            app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
            return 1;
        }
        //当前显示是LVGL官方demo，将按键全部由LVGL接收，不转发给外部
        return 0;
    }
    static int key_down_act_screen_id = -1;

    if (key->event == KEY_ACTION_LVGL_DOWN || key->event == KEY_ACTION_LONG) {
        //down 和 长按时记录当前页面ID
        key_down_act_screen_id = act_screen->id;
    }
    for (const struct ui_key_event_handler *p = ui_key_event_handler_begin; p < ui_key_event_handler_end; p++) {
        if (p->page_id == key_down_act_screen_id) {
            if (key->event == KEY_ACTION_LVGL_DOWN || key->event == KEY_ACTION_UP) {
                if (key->event == KEY_ACTION_UP && long_handled) {
                    app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
                    return 1;
                }
                return 0;
            } else {
                //页面已切换，说明当前事件是按键按下时所在页面的残留事件，丢弃不处理
                if (act_screen->id != key_down_act_screen_id) {
                    return 0;
                }
                //将非UP、DOWN事件交由已注册页面的按键回调函数中判断app是否处理(返回1则处理），如果处理，则交由app处理
                if (p->key_onchange(key)) {
                    app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
                    return 1;
                }
            }
        }
    }

    //若用户未注册页面注册函数，则交由统一的按键处理函数判断是否处理非UP和DOWN按键
    if (key->event == KEY_ACTION_LVGL_DOWN || key->event == KEY_ACTION_UP) {
        if (key->event == KEY_ACTION_UP && long_handled) {
            app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
            return 1;
        }
        return 0;
    } else {
        if (ui_scr_key_event_handler_ext(key)) {
            app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
            return 1;
        }
    }
#endif
    return 0;
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static u8 long_handled = 0;
    struct key_event *key_e = (struct key_event *)indev_drv->user_data;

    if (key_e->event == KEY_ACTION_LONG || key_e->event == KEY_ACTION_HOLD) {
        long_handled = 1;
    }

    if (key_e->event == KEY_ACTION_UP && long_handled) {
        ui_scr_key_event_handler(key_e, long_handled);
        long_handled = 0;
        lv_indev_reset(indev_drv, NULL);
        return;
    }

    if (ui_scr_key_event_handler(key_e, long_handled)) {
        //若ui_scr_key_event_handler(key)返回1,按键事件发给app_core处理,UI不处理
        return;
    }

    if (key_e->event != KEY_ACTION_UP && key_e->event != KEY_ACTION_LVGL_DOWN && key_e->event != KEY_ACTION_LONG && key_e->event != KEY_ACTION_HOLD) {
        //LVGL按键内部不处理非UP、DOWN事件
        return;
    }
    uint32_t key = LV_KEY_ENTER;
    lvgl_key_value_remap(key_e->value, &key) ;  //若用户没有添加LVGL按键映射，则只配置LV_KEY_ENTER键

    switch (key_e->event) {
    case KEY_ACTION_UP:
        data->state = LV_INDEV_STATE_REL;
        if (key_e->value == KEY_IO_WATCH_ENCODER) {
            encoder_state = LV_INDEV_STATE_REL;
        }
        break;
    case KEY_ACTION_LONG:
    case KEY_ACTION_HOLD:
    case KEY_ACTION_LVGL_DOWN:
        data->state = LV_INDEV_STATE_PR;
        if (key_e->value == KEY_IO_WATCH_ENCODER) {
            encoder_state = LV_INDEV_STATE_PR;
        }
        break;
    }

    data->key = key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
    /*Your code comes here*/

    return 0;
}

/*------------------
 * Encoder
 * -----------------*/
void lv_indev_timer_read_encoder(void *user_data)
{
    lv_timer_t timer;
    timer.user_data = indev_encoder;
    indev_encoder->user_data = user_data;
    lv_indev_read_timer_cb(&timer);
}

/*Initialize your encoder*/
static void encoder_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    struct key_event *key_e = (struct key_event *)indev_drv->user_data;

    int key_value = key_e->value;
    data->enc_diff += key_value;
    data->state = encoder_state;
    ui_scr_key_event_handler(key_e, 0);
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/

    encoder_diff += 0;
    encoder_state = LV_INDEV_STATE_RELEASED;
}

/*------------------
 * Button
 * -----------------*/

/*Initialize your buttons*/
static void button_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the button*/
static void button_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{

    static uint8_t last_btn = 0;

    /*Get the pressed button's ID*/
    int8_t btn_act = button_get_pressed_id();

    if (btn_act >= 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_btn = btn_act;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Save the last pressed button's ID*/
    data->btn_id = last_btn;
}

/*Get ID  (0, 1, 2 ..) of the pressed button*/
static int8_t button_get_pressed_id(void)
{
    uint8_t i;

    /*Check to buttons see which is being pressed (assume there are 2 buttons)*/
    for (i = 0; i < 2; i++) {
        /*Return the pressed button's ID*/
        if (button_is_pressed(i)) {
            return i;
        }
    }

    /*No button pressed*/
    return -1;
}

/*Test if `id` button is pressed or not*/
static bool button_is_pressed(uint8_t id)
{

    /*Your code comes here*/

    return false;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
