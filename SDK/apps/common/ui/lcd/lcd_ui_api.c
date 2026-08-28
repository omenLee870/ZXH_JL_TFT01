#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_ui_api.data.bss")
#pragma data_seg(".lcd_ui_api.data")
#pragma const_seg(".lcd_ui_api.text.const")
#pragma code_seg(".lcd_ui_api.text")
#endif

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "res/mem_var.h"
#include "typedef.h"
#include "app_task.h"
#include "btstack/avctp_user.h"
#include "key_event_deal.h"
#include "app_main.h"
#include "ui_resource.h"
#include "gpu_draw.h"
#include "jlui/ui_page_manager.h"
#include "jlui/ui_page_switch.h"
#include "key/key_driver.h"
#include "clock_manager/clock_manager.h"
#include "jlui/ui_measure.h"
#include "app_mode_manager/app_mode_manager.h"
#include "app_task.h"
#if CONFIG_WATCH_CASE_ENABLE
#include "res_config.h"
#include "ui_sys_param.h"
#include "jlui_app/ui_app_effect.h"
#include "gif/gif.h"
#endif
#if TCFG_VIDEO_DIAL_ENABLE
#include "video_player.h"
#endif
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#include "chgbox_box.h"
#include "ui_bg_manage.h"
#include "smartbox_info_manager.h"
#endif

#if (defined CONFIG_JL_UI_ENABLE && CONFIG_JL_UI_ENABLE)

#define LOG_TAG_CONST       UI_TASK
#define LOG_TAG				"[UI_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CHAR_ENABLE
#include "debug.h"
extern u8 get_charge_online_flag(void);
extern u8 get_charge_full_flag(void);
extern void gif_play(int elm_id, int task_id);
extern int jlgpu_scheduler_async_gpu_cache_free_resident();
extern const int JLUI_MULTI_PAGE_OVERLAY_SUPPORT;
#if (TCFG_UI_ENABLE && (TCFG_SPI_LCD_ENABLE || TCFG_LCD_OLED_ENABLE) && (!TCFG_SIMPLE_LCD_ENABLE))

#define MEM_VAR_SIZE	(3*1024)

#define CURR_WINDOW_MAIN (0)
#define UI_NO_ARG		(-1)
#define UI_TASK_NAME	"ui"

REGISTER_UI_GLOBAL_STYLE(JL) /* 注册JL UI风格 */

enum {
    UI_MSG_OTHER,
    UI_MSG_KEY,
    UI_MSG_TOUCH,
    UI_MSG_SHOW,
    UI_MSG_HIDE,
    UI_TIME_TOUCH_RESUME,
    UI_TIME_TOUCH_SUPEND,
    UI_MSG_EXIT,
    UI_MSG_GIF_PLAY,
    UI_MSG_SHOW_MULTI_PAGE,
    UI_MSG_HIDE_MULTI_PAGE,
};
#define USER_MASK_TYPE       	(BIT(16)|BIT(17))//删消息池使用
#define GIF_TYPE_OFFSET			(20)			//GIF偏移

#define PAGE_SWITCH_MODE_NOT_LOOP	0			//卡片滑动不循环


struct ui_card_var {
    u16 move_en: 1;
    u16 slide_en: 1;
    u16 first_result: 6;
    u16 result: 6;

    s16	startx;
    s16 starty;
    s16 endx;
    s16 endy;
    s16 pos_x;
    s16 pos_y;

    s16	start_left;
    s16	start_pos_x;

    u32 page_in_touchdown;
};
struct ui_server_env {
    u8 init: 1;
    u8 key_lock : 1;
    u8 lcd_bl_idle_flag : 1;
    u8 lcd_need_keep_open: 1;
    u8 open_flag: 1;
    u8 ui_card_slide_dis: 1;
    u8 need_password: 1;  // 判断是否需要输入密码

    u8 ui_card_move_mode;
    u8 ui_page_slide_mode;
    u8 touch_msg_counter;
    u16 ui_auto_shut_down_timer;
    u16 timer_id;
    u16 shut_down_time;
    u32 mem_id[8];
    int (*touch_event_call)(void);
    int touch_event_interval;

    OS_SEM start_sem;
    struct ui_card_var card_var;
};


static struct ui_server_env __ui_display = {0};
#define __this (&__ui_display)
#define __card (&(__this->card_var))
static DEFINE_SPINLOCK(lock);
int ui_dial_manage_init();
u8 get_standby_watch_en();
int lcd_sleep_ctrl(u8 enter);
int ui_page_num();
void ui_key_set_send_event_cb(void (*cb)(int key_event));
int ui_core_clear_pagemove_flag();
int ui_cube_ontouch(struct element_touch_event *e);
int ui_hexagon_ontouch(struct element_touch_event *e);
int ui_reflection_ontouch(struct element_touch_event *e);
int ui_cube_reflection_ontouch(struct element_touch_event *e);
int ui_relate_init();
int ui_relate_uninit();
void ui_cube_rdec(int xoffset);
int ui_cube_status();
int ui_hexagon_status();
int ui_reflection_status();
int ui_cube_reflection_status();
void ui_page_set_busy(int busy);
void menu_list_rec_clr();
extern int dial_sel_exit();
extern int ui_page_switch_effect_stop();
//进入屏保
__attribute__((weak)) void ui_screen_saver(void *p)
{
    ui_backlight_close();
}

//退出屏保
__attribute__((weak)) void ui_screen_recover(u8 recover_cur_page)
{
    ui_backlight_open(recover_cur_page);
}

__attribute__((weak)) u8 get_screen_saver_status()
{
    return (!lcd_backlight_status());
}

__attribute__((weak)) void set_screen_saver_status(u8 status)
{

}

__attribute__((weak)) void auto_goto_dial_timer_del()
{

}

__attribute__((weak)) void ui_sysinfo_init()
{

}

__attribute__((weak)) void sys_param_init(void)
{

}

__attribute__((weak)) void lcd_bl_open()
{

}

__attribute__((weak)) u8 ui_message_filter_hook(const char *msg, va_list argptr)
{
    return 0;
}
/* __attribute__((weak)) int ui_id2remap(int id) */
/* { */
/* return id; */
/* } */
//映射id
int ui_id2remap(int id)
{
#if TCFG_UI_ENABLE
#if UI_UPGRADE_RES_ENABLE
    u8 rcsp_eflash_update_flag_get(void);
    u8 rcsp_eflash_flag_get(void);
    int dial_num = watch_get_items_num();
    if (ui_id2type(id) == CTRL_TYPE_WINDOW) {
        if (rcsp_eflash_flag_get() || rcsp_eflash_update_flag_get() || (dial_num == 0)) {
            log_info("id:0x%x remap to 0x%x\n", id, ID_WINDOW_UPGRADE);
            return ID_WINDOW_UPGRADE;
        }
    }
#endif
#endif /* #if TCFG_UI_ENABLE */
    return id;
}
u32 *get_ui_mem_id()
{
    return __this->mem_id;
}

u32 get_ui_mem_id_size()
{
    return sizeof(__this->mem_id) / sizeof(__this->mem_id[0]);
}

u8 get_ui_init_status()
{
    return __this->init;
}

int key_is_ui_takeover()
{
    return __this->key_lock;
}

void key_ui_takeover(u8 on)
{
    __this->key_lock = !!on;
}

u8 get_need_password()
{
    return __this->need_password;
}

void set_need_password(u8 flag)
{
    __this->need_password = flag;
}

static int post_ui_msg(int *msg, u8 len)
{
    int err = 0;
    int count = 0;

    if (!__this->init) {
        log_error("<%s> not init\n", __func__);
        return -1;
    }

__retry:
    err = os_taskq_post_type(UI_TASK_NAME, msg[0], len - 1, &msg[1]);

    if (cpu_in_irq() || cpu_irq_disabled()) {
        return err;
    }

    if (err) {
        log_error("<%s> os_error:%d\n", __func__, err); //在os_error.h中查找错误码含义

        if (!strcmp(os_current_task(), UI_TASK_NAME)) {
            return err;
        }
        if (count > 20) {
            return -1;
        }
        count++;
        os_time_dly(1);
        goto __retry;
    }
    return err;
}
int ui_page_memory_get(int id)
{
    int mem_id_num = (int)(sizeof(__this->mem_id) / sizeof(__this->mem_id[0]));
    if ((id <= 0) && (id > -1 * mem_id_num)) {
        if (__this->mem_id[7 + id]) {
            return __this->mem_id[7 + id];
        }
    }
    return 0;
}
int ui_page_memory_set(int id)
{
    if (__this->mem_id[7] != id) {
        for (int i = 1; i <= 7; i++) {
            __this->mem_id[i - 1] = __this->mem_id[i];
        }
        __this->mem_id[7] = id;
    }
    return id;
}
//=================================================================================//
// @brief: 显示主页 应用于非ui线程显示主页使用
//有针对ui线程进行处理，允许用于ui线程等同ui_show使用
//=================================================================================//
int ui_show_main(int id)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("<%s> ID:0x%x rets:%x\n", __func__, id, rets);

    static OS_SEM sem;
    int msg[8];
    int i;

    int mem_id = ui_page_memory_get(id);
    log_info("<%s>id:0x%x memid:0x%x \n", __func__, id, mem_id);
    if (mem_id) {
        id  = mem_id;
    } else {
        ui_page_memory_set(id);
    }

    auto_goto_dial_timer_del();

    u8 need_pend = 0;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    msg[0] = UI_MSG_SHOW;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }

    if (get_screen_saver_status()) {
        log_info("screen-saver ... ui_show_main_now\n");
        ui_screen_recover(0);
        __this->lcd_bl_idle_flag = 0;
    }


    return 0;
}


int ui_show_multi_page()
{
    static OS_SEM sem;
    int msg[8];
    int i;

    u8 need_pend = 0;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    msg[0] = UI_MSG_SHOW_MULTI_PAGE;
    msg[1] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[1] = (int)&sem;
    }

    if (!post_ui_msg(msg, 2)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }

    return 0;
}

int ui_hide_multi_page()
{
    static OS_SEM sem;
    int msg[8];
    int i;

    auto_goto_dial_timer_del();

    u8 need_pend = 0;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    msg[0] = UI_MSG_HIDE_MULTI_PAGE;
    msg[1] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[1] = (int)&sem;
    }

    if (!post_ui_msg(msg, 2)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }

    return 0;
}

//=================================================================================//
// @brief: 关闭主页 应用于非ui线程关闭使用
//有针对ui线程进行处理，允许用于ui线程等同ui_hide使用
//=================================================================================//
int ui_hide_main(int id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("__func__ %s %x\n", __func__, rets);


    static OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];

    u8 need_pend = 0;;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    /* if (!strcmp(os_current_task(), UI_TASK_NAME)) { */
    /*     if (CURR_WINDOW_MAIN == id) { */
    /*         id = ui_get_current_window_id(); */
    /*     } */
    /*     ui_hide(id); */
    /*     return 0; */
    /* } */
    msg[0] = UI_MSG_HIDE;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }
    return 0;
}

//=================================================================================//
// @brief: 关闭当前主页 灵活使用自动判断当前主页进行关闭
//用户可以不用关心当前打开的主页,特别适用于一个任务使用了多个主页的场景
//=================================================================================//
int ui_hide_curr_main()
{
    return ui_hide_main(CURR_WINDOW_MAIN);
}

//=================================================================================//
// @brief: 应用往ui发送消息，ui主页需要注册回调函数关闭当前主页
// //消息发送方法demo： UI_MSG_POST("test1:a=%4,test2:bcd=%4,test3:efgh=%4,test4:hijkl=%4", 1,2,3,4);
// 往test1、test2、test3、test4发送了字符为a、bcd、efgh、hijkl，附带变量为1、2、3、4
// 每次可以只发一个消息，也可以不带数据例如:UI_MSG_POST("test1")
//=================================================================================//
//

static int msg_argc(const char *msg)
{
    int ret = 0;
    int step = 0;

    if (*msg == '\0') {
        return 0;
    }

    while (*msg) {
        switch (step) {
        case 0:
            if (*msg == ':') {
                step = 1;
            }
            break;
        case 1:
            switch (*msg) {
            case '%':
                msg++;
                if (*msg >= '0' && *msg <= '9') {
                    ret++;
                } else if (*msg == 'p') {
                    ret++;
                }
                step = 0;
                break;
            default:
                break;
            }
            break;
        }
        msg++;
    }

    return ret;
}

int ui_server_msg_post(const char *msg, ...)
{
    int argv[9];
    argv[0] = UI_MSG_OTHER;
    argv[1] = (int)msg;
    va_list argptr;
    va_start(argptr, msg);
    int ret = msg_argc(msg);
    for (int i = 0; (i < 7 && i < ret) ; i++) {
        argv[i + 2] = va_arg(argptr, int);
    }
    va_end(argptr);
    return post_ui_msg(argv, 9);
}

//=================================================================================//
// @brief: 应用往ui发送key消息，由ui控件分配
//=================================================================================//

int ui_key_msg_post(int key)
{
    u8 count = 0;
    int msg[8];

    msg[0] = UI_MSG_KEY;
    msg[1] = key;
    /* touch_msg_counter++; */
    return post_ui_msg(msg, 2);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief key_event_sent_to_ui_map 按键事件映射为ui事件
 *
 * @param key
 */
/* ------------------------------------------------------------------------------------*/
void key_event_sent_to_ui_map(int key_event)
{
    if (key_event == KEY_UI_TRIPLE_CLICK) {
#if  (defined TCFG_CHARGE_BOX_ENABLE && TCFG_CHARGE_BOX_ENABLE)
        app_chargebox_event_to_user(CHGBOX_EVENT_EXCHANGE_MAC);
#endif
    } else if (key_event != KEY_WATCH_NO_KEY) {
        if (get_screen_saver_status()) {
            ui_screen_recover(1);
            return;
        } else {
            ui_auto_shut_down_re_run();
        }
        ui_key_msg_post(key_event);
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_key_event_deal_app 应用层处理ui按键事件的地方
 *
 * @param key_event
 *
 * @return true 应用层接管 false  ui框架处理
 */
/* ------------------------------------------------------------------------------------*/
int ui_key_event_deal_app(int key_event)
{
    /* log_info("%s key_event:%d", __func__, key_event); */
    if (app_in_mode(APP_MODE_IDLE)) {
        return true;
    }
#if TCFG_PC_MODE_LOCK_UI_ENABLE
    if (app_in_mode(APP_MODE_PC)) {
        return true;
    }
#endif
    /* 如果在表盘选择功能，按键时先退回表盘 */
    extern int dial_sel_state();
    extern void dial_sel_free(struct element * elm, int touch_x, int touch_y);
    if (dial_sel_state() && (key_event != KEY_UI_PLUS) && (key_event != KEY_UI_MINUS)) {
        dial_sel_free(NULL, -1, -1);
        return true;
    }

    int ret = false;
    if (key_event == KEY_UI_HOME) {
        ui_show_menu_page();
        ret = true;
    } else if (key_event == KEY_UI_SHORTCUT) {
        ui_show_shortcut_key();
        ret = false;
    } else if (key_event == KEY_UI_POWEROFF) {
        ui_key_shutdown_or_reboot(1);
        ret = true;
    } else if (key_event == KEY_UI_MENU_LIST) {
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
        ui_key_shutdown_or_reboot(1);
        ret = true;
#else
        ret = ui_show_menu_list();
        if (ret == false) {
            ui_card_anim_next();
        }
        ret = true;
#endif
    } else if (key_event == KEY_UI_PLUS) {
        if ((ui_cube_status()) && (ui_card_get_move_mode() == PAGE_MOVE_MODE_CUBE)) {
            ui_cube_rdec(20);
            ret = true;
        }
    } else if (key_event == KEY_UI_MINUS) {
        if ((ui_cube_status()) && (ui_card_get_move_mode() == PAGE_MOVE_MODE_CUBE)) {
            ui_cube_rdec(-20);
            ret = true;
        }
    } else {
        ret = false;
    }
    return  ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 应用层默认处理ui按键事件的地方,位于ui框架处理按键事件之后
 *
 * @param key_event
 *
 */
/* ------------------------------------------------------------------------------------*/
void ui_default_key_event_deal_app(int key_event)
{
    if (key_event == KEY_UI_PLUS) {
        ui_show_plus();
    } else if (key_event == KEY_UI_MINUS) {
        ui_show_minus();
    }
}


// 触摸限流降频
#define TCFG_UI_TOUCH_LIMIT_EN			0
// 动画降频
#define TCFG_UI_ANIM_LIMIT_EN			0

#define TCFG_UI_LIMIT_ENABLE			(TCFG_UI_TOUCH_LIMIT_EN || TCFG_UI_ANIM_LIMIT_EN)

#if TCFG_UI_LIMIT_ENABLE

#define UI_LIMIT_TMR_MS			(100) // 定时器间隔
#define UI_LIMIT_CHECK_CNT		(5) // 检测间隔
#define UI_LIMIT_CHECK_MIN		(1)
#define UI_LIMIT_CHECK_MAX		(UI_LIMIT_CHECK_CNT - 1)

#define UI_TOUCH_LIMIT_ABANDON_SETUP		(20)

struct ui_limit {
    u8  block_percent; 	// 拥堵百分比，值越大越拥堵
    u8  block_cnt_once;	// 每次增加的拥堵比
    u8  cnt_task;
    u8  cnt_irq;
    u16 timer_task;
    u16 timer_irq;
#if TCFG_UI_TOUCH_LIMIT_EN
    u8  touch_abandon_cnt;
#endif
#if TCFG_UI_ANIM_LIMIT_EN
    u8  anim_interval_min;
    u8  anim_interval_max;
#endif
};
static struct ui_limit *p_ui_limit = NULL;

static void ui_limit_task_deal(void *priv)
{
    spin_lock(&lock);
    if (p_ui_limit) {
        p_ui_limit->cnt_task ++;
        p_ui_limit->timer_task = sys_timeout_add_2_task(NULL, ui_limit_task_deal, UI_LIMIT_TMR_MS, "app_core");
        /* printf("ui_limit task deal\n"); */
    }
    spin_unlock(&lock);
}

static void ui_limit_irq_deal(void *priv)
{
    spin_lock(&lock);
    if (p_ui_limit) {
        p_ui_limit->cnt_irq ++;
        if (p_ui_limit->cnt_irq >= UI_LIMIT_CHECK_CNT) {
            u8 old_percent = p_ui_limit->block_percent;
            if (p_ui_limit->cnt_task <= UI_LIMIT_CHECK_MIN) { // 阻塞
                p_ui_limit->block_percent += p_ui_limit->block_cnt_once;
                if (p_ui_limit->block_percent > 100) {
                    p_ui_limit->block_percent = 100;
                }
            } else if (p_ui_limit->cnt_task >= UI_LIMIT_CHECK_MAX) { // 缓解
                if (p_ui_limit->block_percent > p_ui_limit->block_cnt_once) {
                    p_ui_limit->block_percent -= p_ui_limit->block_cnt_once;
                } else {
                    p_ui_limit->block_percent = 0;
                }
            }
#if TCFG_UI_ANIM_LIMIT_EN
            if (old_percent != p_ui_limit->block_percent) {
                u8 anim_interval = p_ui_limit->anim_interval_min + (p_ui_limit->anim_interval_max - p_ui_limit->anim_interval_min) * p_ui_limit->block_percent / 100;
                ui_anim_set_interval(anim_interval);
                /* printf("ui_anim switch:%d \n", anim_interval); */
            }
#endif
            /* printf("ui_limit taskcnt:%d, percent:%d \n", p_ui_limit->cnt_task, p_ui_limit->block_percent); */
            p_ui_limit->cnt_irq = 0;
            p_ui_limit->cnt_task = 0;
        }
    }
    spin_unlock(&lock);
}

static void ui_limit_release(void)
{
    if (p_ui_limit) {
        /* printf("ui_limit release\n"); */
        if (p_ui_limit->timer_task) {
            sys_timeout_del(p_ui_limit->timer_task);
            p_ui_limit->timer_task = 0;
        }
        if (p_ui_limit->timer_irq) {
            sys_hi_timer_del(p_ui_limit->timer_irq);
            p_ui_limit->timer_irq = 0;
        }
        free(p_ui_limit);
        p_ui_limit = NULL;
    }
}
static void ui_limit_create(void)
{
    if (p_ui_limit == NULL) {
        p_ui_limit = zalloc(sizeof(struct ui_limit));
        ASSERT(p_ui_limit);
        p_ui_limit->timer_task = sys_timeout_add_2_task(NULL, ui_limit_task_deal, UI_LIMIT_TMR_MS, "app_core");
        p_ui_limit->timer_irq = sys_hi_timer_add(NULL, ui_limit_irq_deal, UI_LIMIT_TMR_MS);
        u32 wdt_ms = 4000; //wdt_get_time()
        p_ui_limit->block_cnt_once = 100 / (wdt_ms / (UI_LIMIT_TMR_MS * UI_LIMIT_CHECK_CNT) - 1);
        /* printf("ui_limit create, once:%d \n", p_ui_limit->block_cnt_once); */
#if TCFG_UI_ANIM_LIMIT_EN
#if (defined TCFG_LCD_TE_USED_PEND && TCFG_LCD_TE_USED_PEND)
        p_ui_limit->anim_interval_min = lcd_get_te_frame_period_us() * 80 / 100 / 1000;
#else
        p_ui_limit->anim_interval_min = 13;
#endif
        p_ui_limit->anim_interval_max = 100;
        ui_anim_set_interval(p_ui_limit->anim_interval_min);
        /* printf("ui_anim min:%d \n", p_ui_limit->anim_interval_min); */
#endif
    }
}

static void ui_limit_lcd_sleep_enter(void)
{
    // 灭屏
    spin_lock(&lock);
    ui_limit_release();
    spin_unlock(&lock);
}

static void ui_limit_lcd_sleep_exit(void)
{
    // 亮屏
    spin_lock(&lock);
    ui_limit_create();
    spin_unlock(&lock);
}

REGISTER_LCD_SLEEP_HEADLER(ui_limit_by_lcd_sleep) = {
    .name = "ui_limit",
    .enter = ui_limit_lcd_sleep_enter,
    .exit = ui_limit_lcd_sleep_exit,
};


#if TCFG_UI_TOUCH_LIMIT_EN

static void ui_touch_limit_release(void)
{
    if (p_ui_limit) {
        /* printf("ui_touch release\n"); */
        p_ui_limit->touch_abandon_cnt = 0;
    }
}
static void ui_touch_limit_create(void)
{
}
static int ui_touch_limit_is_abandon(void)
{
    if (p_ui_limit && p_ui_limit->block_percent) {
        p_ui_limit->touch_abandon_cnt += UI_TOUCH_LIMIT_ABANDON_SETUP;
        if (p_ui_limit->touch_abandon_cnt <= p_ui_limit->block_percent) {
            /* printf("ui_touch abandon\n"); */
            return true;
        }
        if (p_ui_limit->touch_abandon_cnt > 100) {
            p_ui_limit->touch_abandon_cnt = 0;
        }
        /* printf("ui_touch ok\n"); */
    }
    return false;
}
#endif /* #if TCFG_UI_TOUCH_LIMIT_EN */

#endif /* #if TCFG_UI_LIMIT_ENABLE */


//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post(struct touch_event *event)
{
    int msg[8];
    int i = 0;
    int err;

    ui_auto_shut_down_re_run();
    if (get_screen_saver_status()) {
        ui_screen_recover(1);
        return 0;
    }

#if TCFG_UI_TOUCH_LIMIT_EN
    spin_lock(&lock);
    if (event->event == ELM_EVENT_TOUCH_MOVE) {
        ui_touch_limit_create();
        if (ui_touch_limit_is_abandon()) {
            spin_unlock(&lock);
            return 0;
        }
    } else {
        ui_touch_limit_release();
    }
    spin_unlock(&lock);
#endif /* #if TCFG_UI_TOUCH_LIMIT_EN */

    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)NULL;
    memcpy(&msg[2], event, sizeof(struct touch_event));

    while (1) {
        spin_lock(&lock);
        __this->touch_msg_counter++;
        spin_unlock(&lock);
        err = post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
        if (err) {
            log_error("post_ui_msg err! %d, %d, %d\n", event->event, event->x, event->y);
            spin_lock(&lock);
            __this->touch_msg_counter--;
            spin_unlock(&lock);

            /* if ((event->event == ELM_EVENT_TOUCH_DOWN) || (event->event == ELM_EVENT_TOUCH_UP)) { */
            /*     os_time_dly(1); */
            /*     continue; */
            /* } */

            return -1;
        }
        break;
    }

    return 0;
}


//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post_withcallback(struct touch_event *event, void (*cb)(u8 finish))
{
    int msg[8];
    int i = 0;
    int err;

    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)cb;
    memcpy(&msg[2], event, sizeof(struct touch_event));
    if (__this->touch_msg_counter < 16) {
        spin_lock(&lock);
        __this->touch_msg_counter++;
        spin_unlock(&lock);
        err = post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
        if (err) {
            log_error("post_ui_msg err! %d, %d, %d\n", event->event, event->x, event->y);
            spin_lock(&lock);
            __this->touch_msg_counter--;
            spin_unlock(&lock);
            return -1;
        }
    } else {
        log_info("touch msg drop! %d, %d, %d\n", event->event, event->x, event->y);
        return -1;
    }
    return 0;
}



const char *str_substr_iter(const char *str, char delim, int *iter)
{
    const char *substr;
    ASSERT(str != NULL);
    substr = str + *iter;
    if (*substr == '\0') {
        return NULL;
    }
    for (str = substr; *str != '\0'; str++) {
        (*iter)++;
        if (*str == delim) {
            break;
        }
    }
    return substr;
}


int do_msg_handler(const char *msg, va_list *pargptr, int (*handler)(const char *, u32))
{
    int ret = 0;
    int width;
    int step = 0;
    u32 arg = 0x5678;
    int m[16];
    char *t = (char *)&m[3];
    va_list argptr = *pargptr;

    if (*msg == '\0') {
        handler((const char *)' ', 0);
        return 0;
    }

    while (*msg && *msg != ',') {
        switch (step) {
        case 0:
            if (*msg == ':') {
                step = 1;
            }
            break;
        case 1:
            switch (*msg) {
            case '%':
                msg++;
                if (*msg >= '0' && *msg <= '9') {
                    if (*msg == '1') {
                        arg = va_arg(argptr, int) & 0xff;
                    } else if (*msg == '2') {
                        arg = va_arg(argptr, int) & 0xffff;
                    } else if (*msg == '4') {
                        arg = va_arg(argptr, int);
                    }
                } else if (*msg == 'p') {
                    arg = va_arg(argptr, int);
                }
                m[2] = arg;
                handler((char *)&m[3], m[2]);
                t = (char *)&m[3];
                step = 0;
                break;
            case '=':
                *t = '\0';
                break;
            case ' ':
                break;
            default:
                *t++ = *msg;
                break;
            }
            break;
        }
        msg++;
    }
    *pargptr = argptr;
    return ret;
}


int ui_message_handler(int id, const char *msg, va_list argptr)
{

    int iter = 0;
    const char *str;
    const struct uimsg_handl *handler;
    struct window *window = (struct window *)ui_core_get_element_by_id(id);
    if (!window || !window->private_data) {
        return -EINVAL;
    }
    handler = (const struct uimsg_handl *)window->private_data;
    while ((str = str_substr_iter(msg, ',', &iter)) != NULL) {
        for (; handler->msg != NULL; handler++) {
            if (!memcmp(str, handler->msg, strlen(handler->msg))) {
                do_msg_handler(str + strlen(handler->msg), &argptr, handler->handler);
                break;
            }
        }
    }

    return 0;
}

void ui_touch_timer_delete()
{
    if (!__ui_display.init) {
        return;
    }

    int msg[8];
    msg[0] = UI_TIME_TOUCH_SUPEND;
    post_ui_msg(msg, 1);

#if UI_WATCH_RES_ENABLE
    local_irq_disable();
    if (!__ui_display.timer_id) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
#endif

}


void ui_touch_timer_start()
{
    if (!__ui_display.init) {
        return;
    }

    int msg[8];
    msg[0] = UI_TIME_TOUCH_RESUME;
    post_ui_msg(msg, 1);

    local_irq_disable();
    if (__ui_display.timer_id) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
}

void ui_set_touch_event(int (*touch_event)(void), int interval)
{
    __ui_display.touch_event_call = touch_event;
    __ui_display.touch_event_interval = interval;
}



u8 get_ui_open_flag()
{
    return __this->open_flag;
}

void set_ui_open_flag(u8 flag)
{
    __this->open_flag = !!flag;
}

void set_lcd_keep_open_flag(u8 flag)
{
    __this->lcd_need_keep_open = !!flag;
}
extern u8 get_charge_online_flag(void);
void ui_backlight_open(u8 recover_cur_page)
{

    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    __this->lcd_bl_idle_flag = 0;
    if (!__ui_display.init) {
        return;
    }

    if (__this->open_flag) {
        return;
    }

    log_info("__func__ %s %x\n", __func__, rets);
    if (get_screen_saver_status()) {

        __this->open_flag = 1;
        /* UI_MSG_POST("ui_lp_cb:exit=%4", 1); */
        /* sys_key_event_enable();//关闭按键 */
        /* ui_auto_shut_down_enable(); // 如果唤醒后的界面有disable，这里会把它重新打开 */
        //lcd_sleep_ctrl(0);//屏幕退出低功耗
#if UI_WATCH_RES_ENABLE
        if (recover_cur_page) {
            ui_auto_shut_down_enable();
            u8 poweron_password = 0;
            int ret = syscfg_read(USER_PASSWORD_ON, &poweron_password, sizeof(poweron_password));
            if ((ret == sizeof(poweron_password)) && poweron_password) {
                set_need_password(1);
                UI_SHOW_WINDOW(ID_WINDOW_DIAL);
            }
#if TCFG_CHARGE_ENABLE
            else if (get_charge_online_flag()) {
                UI_SHOW_WINDOW(ID_WINDOW_BATCHARGE);
            }
#elif TCFG_CHARGE_BOX_ENABLE //彩屏仓 仓充电
            else if (sbox_box_charging_get()) {
                UI_SHOW_WINDOW(ID_WINDOW_BATCHARGE);
            }
#endif
            else {
                ui_show_main(0);//恢复当前ui
            }
        } else {
            u8 poweron_password = 0;
            int ret = syscfg_read(USER_PASSWORD_ON, &poweron_password, sizeof(poweron_password));
            if ((ret == sizeof(poweron_password)) && poweron_password) {
                set_need_password(1);
            }
        }
#endif
        //启用灵动岛功能的时候，亮屏显示
        if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT && TCFG_UI_ENABLE_SMARTWIN) {
            UI_SHOW_MULTI_PAGE();
        }
        ui_touch_timer_start();
    }
}

void ui_backlight_close(void)
{

    if (!__ui_display.init) {
        return;
    }


    __this->open_flag = 0;

    if (!lcd_sleep_status()) {
        /* UI_MSG_POST("ui_lp_cb:enter=%4", 0); */
        /* sys_key_event_disable();//关闭按键 */
#if UI_WATCH_RES_ENABLE
        ui_hide_curr_main();//关闭当前页面
#endif
        if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
            UI_HIDE_MULTI_PAGE();
        }

        ui_auto_shut_down_disable();
        ui_touch_timer_delete();
    }

}

#if TCFG_UI_SHUT_DOWN_TIME
void ui_set_shut_down_time(u16 time)
{
    __ui_display.shut_down_time = time;
}
int ui_get_shut_down_time()
{
    return  __ui_display.shut_down_time;
}
#endif

void ui_auto_shut_down_enable(void)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
#if TCFG_UI_SHUT_DOWN_TIME
    if (!__ui_display.init) {
        return;
    }
#if TCFG_USER_BT_CLASSIC_ENABLE
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        log_info("%s call_status:%d", __func__, bt_get_call_status());
        return;
    }
#endif
    if (__this->lcd_need_keep_open) {
        log_info("need keep open");
        return;
    }
    if (__this->ui_auto_shut_down_timer == 0) {
        if (__ui_display.shut_down_time == 0) {
            __ui_display.shut_down_time = 10;
        }
        if (__ui_display.shut_down_time > 20) {
            /* __ui_display.shut_down_time = 20; */
        }
        log_info("[%s]time:%d rets:0x%x\n", __func__, __ui_display.shut_down_time, rets);
        __this->ui_auto_shut_down_timer = sys_timeout_add(NULL, ui_screen_saver, __ui_display.shut_down_time * 1000);
    }
#endif
}

u8 ui_auto_shut_down_disable(void)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
#if TCFG_UI_SHUT_DOWN_TIME
    if (__this->ui_auto_shut_down_timer) {
        log_info("[%s]rets:0x%x", __func__, rets);
        sys_timeout_del(__this->ui_auto_shut_down_timer);
        __this->ui_auto_shut_down_timer = 0;
        return true;
    }
    return false;
#endif
    return false;
}

void ui_auto_shut_down_modify(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (__this->ui_auto_shut_down_timer) {
        sys_timer_modify(__this->ui_auto_shut_down_timer, __ui_display.shut_down_time * 1000);
    }
#endif
}

void ui_auto_shut_down_re_run(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (__this->ui_auto_shut_down_timer) {
        sys_timer_re_run(__this->ui_auto_shut_down_timer);
    }
#endif
}

static u8 lcd_bl_idle_query(void)
{
    return __this->lcd_bl_idle_flag;
}
REGISTER_LP_TARGET(lcd_backlight_lp_target) = {
    .name = "lcd_backlight",
    .is_idle = lcd_bl_idle_query,
};


#if(1||CONFIG_UI_STYLE == STYLE_JL_WTACH_NEW)
extern int g_cur_left;
//=================================================================================//
// @brief: 卡片滑动模式管理，用于左侧侧边栏、负一屏等效果
//	限制首页右滑、所有页面右滑和最后一页左滑
//=================================================================================//

int ui_page_manager_mode_set(u8 mode)
{
    __this->ui_page_slide_mode = mode;
    return  __this->ui_page_slide_mode;
}
int ui_page_manager_mode_get(void)
{
    return __this->ui_page_slide_mode;
}
//=================================================================================//
// @brief: 卡片管理开关
//=================================================================================//

enum ui_card_run_type ui_card_get_status()
{
    struct ui_effect_module *effmod_hd = ui_effect_get_handle_by_style(ui_card_get_move_mode());
    /* log_info("%s mode:%d hd:0x%x",__func__,ui_card_get_move_mode(),(u32)effmod_hd); */
    if (effmod_hd && effmod_hd->get_status) {
        if (effmod_hd->get_status()) {
            return UI_CARD_RUN_MULTI_PAGE;
        }
    }
    if (__card->slide_en) {
        return UI_CARD_RUN_SINGLE_PAGE;
    }
    return UI_CARD_RUN_NULL;
}

int ui_in_effect()
{
    if (ui_card_get_status()) {
        return true;
    } else {
        return false;
    }
}

void ui_card_run_stop(void)
{
    struct ui_effect_module *effmod_hd = ui_effect_get_handle_by_style(ui_card_get_move_mode());
    /* log_info("%s mode:%d hd:0x%x",__func__,ui_card_get_move_mode(),(u32)effmod_hd); */
    if (effmod_hd && effmod_hd->uninit) {
        effmod_hd->uninit();
    }

    if (__card->slide_en) {
        ui_page_anim_stop();
        __card->slide_en = false;
    }
}

#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
u8 is_card_enable()
{
    return __this->ui_card_slide_dis;
}
#endif

void ui_card_enable()
{
    __this->ui_card_slide_dis = false;
}
void ui_card_disable()
{
    __this->ui_card_slide_dis = true;
}

u8 ui_card_get_move_mode()
{
    return __this->ui_card_move_mode;
}

void ui_card_set_move_mode(u8 mode)    // 设置卡片滑动动画
{
    void jlui_malloc_ram_info_clr(u8 clr);
    jlui_malloc_ram_info_clr(0x2);
    __this->ui_card_move_mode = mode;
}

static void ui_card_anim_start(int curr_win)
{
}

static void ui_card_anim_stop(int curr_win)
{
    __card->slide_en = false;
}

//#define UI_CARD_MOVE_MODE  3
int ui_card_ontouch(struct element_touch_event *e)
{
    /* 如果在菜单返回划屏特效，直接接管触摸消息 */
    extern int ui_return_page_effect_touch(struct element_touch_event * e);
    extern int menu_enter_app_state();
    if (ui_return_page_effect_touch(e) || menu_enter_app_state()) {
        return true;
    }
    int page_switch_mode = ui_page_manager_mode_get();
    if (__this->ui_card_slide_dis) {
        log_debug("<%s> ui_card disable\n", __func__);
        __card->page_in_touchdown = 0;
        if (__card->slide_en) {
            ui_page_anim_stop();
            __card->slide_en = false;
        }
        return ui_event_ontouch(e);
    }
    struct ui_page *page = ui_page_search(ui_get_current_window_id());
    int ui_card_page_num = get_ui_page_list_total_num();
    /* log_info("%s page_num:%d", __func__, ui_card_page_num); */
    extern int dial_sel_state();
    if ((!page) || (ui_card_page_num < 2) || (dial_sel_state())) {
        log_debug("<%s>page_not_search(page_id:0x%x) or page_num:%d \n", __func__, ui_get_current_window_id(), ui_card_page_num);
        if (__card->slide_en) {
            ui_page_anim_stop();
            __card->slide_en = false;
        }

        __card->page_in_touchdown = 0;
        return ui_event_ontouch(e);
    }
    int effect_ret = 0;
    int xoffset, yoffset;
    struct ui_effect_module *effmod_hd = ui_effect_get_handle_by_style(ui_card_get_move_mode());
    if (effmod_hd && effmod_hd->ontouch) {
        switch (e->event) {
        case ELM_EVENT_TOUCH_ENERGY:
            effect_ret = effmod_hd->ontouch(e);
            break;
        case ELM_EVENT_TOUCH_DOWN:
            ui_page_manager_mode_set(0);
            __card->startx = e->pos.x;
            __card->starty = e->pos.y;
            effect_ret = effmod_hd->ontouch(e);
            break;
        case ELM_EVENT_TOUCH_UP:
            effect_ret = effmod_hd->ontouch(e);
            break;
        case ELM_EVENT_TOUCH_MOVE:
            xoffset = e->pos.x - __card->pos_x;
            yoffset = e->pos.y - __card->pos_y;
            __card->pos_x = e->pos.x;
            __card->pos_y = e->pos.y;
            __card->result = get_direction(__card->startx, __card->starty, e->pos.x, e->pos.y);
            if ((__card->first_result == DIRECTION_NOT_INIT) &&
                (__card->result > DIRECTION_NONE)) {
                __card->first_result = __card->result;
            }
            if (__card->result != __card->first_result) {
                __card->startx = e->pos.x;
                __card->starty = e->pos.y;
                __card->first_result = __card->result;
            }
            __card->move_en = ui_page_move_callback_run();
            //左滑侧边栏
#if(TCFG_UI_ENABLE_LEFT_MENU)
            if (ui_get_current_window_id() == ID_WINDOW_DIAL && (ui_page_num() == 1)) {
                if (__card->result == DIRECTION_RIGHT) {
                    ui_page_manager_mode_set(1);
                } else {
                    ui_page_manager_mode_set(0);
                }
            } else {
                /* ui_page_manager_mode_set(0); */
            }
#endif
            int direction_threshold = 0;
            if ((abs(e->pos.x - __card->startx) > 15) || (abs(e->pos.y - __card->starty) > 15)) {
                direction_threshold = 1;
            }
            /* log_info("%s (%d %d)(%d %d)",__func__,e->pos.x,e->pos.y,__card->startx,__card->starty); */
            page_switch_mode = ui_page_manager_mode_get();
            /* log_info("%s %d page:%x page_num:%d result:%d mode:%d", __func__, __LINE__, ui_get_current_window_id(), ui_page_num(), __card->result, page_switch_mode); */

            if (!ui_card_get_status() && (__card->move_en != 0) && direction_threshold && ((__card->result == DIRECTION_LEFT) || (__card->result == DIRECTION_RIGHT)) && ((__card->first_result == DIRECTION_LEFT) || (__card->first_result == DIRECTION_RIGHT))) {
                if (page_switch_mode)  {  //禁止右滑
                } else { //cube滑动
                    effect_ret = effmod_hd->ontouch(e);
                }
            } else if (ui_card_get_status()) {
                effect_ret = effmod_hd->ontouch(e);
            }
            break;
        }
        //特效接管
        if (effect_ret || ui_card_get_status()) {
            return 0;
        } else {
            return ui_event_ontouch(e);
        }
    }



    struct element *p;
    struct element *curr_elm = 0;

    /*log_debug("<%s>event(%d)pos(%d,%d)\n", __func__, e->event, e->pos.x, e->pos.y);*/
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        log_char('D');

        ui_page_manager_mode_set(0);
        ui_page_down(ui_get_current_window_id(), ui_card_get_move_mode());
        if (__card->slide_en) {
            ui_page_anim_stop();
#if 0 // 按下后页面还原
            __card->slide_en = false;
            ui_page_move_stop(ui_get_current_window_id(), ui_card_get_move_mode());
#else // 按下后从当前位置继续开始
            __card->slide_en = true;
            __card->pos_x = e->pos.x;
            __card->pos_y = e->pos.y;
            __card->startx = e->pos.x;
            __card->starty = e->pos.y;
            __card->start_left = g_cur_left;
            __card->start_pos_x = __card->startx;
            break;
#endif
        }
        __card->page_in_touchdown = ui_get_current_window_id();
        list_for_each_child_element(p, ui_core_get_root()) {
            if (!ui_multi_page_check_id(p->id)) {
                ASSERT(p->id == __card->page_in_touchdown);
            }
        }

        __card->pos_x = e->pos.x;
        __card->pos_y = e->pos.y;
        __card->startx = e->pos.x;
        __card->starty = e->pos.y;
        __card->first_result = DIRECTION_NOT_INIT;
        __card->result = DIRECTION_NOT_INIT;
        curr_elm = ui_core_get_element_by_id(__card->page_in_touchdown);
        g_cur_left = curr_elm->css.left;

        __card->start_left = g_cur_left;
        __card->start_pos_x = __card->startx;
        /* log_info("id:0x%x, start:%d,%d \n", __card->page_in_touchdown, __card->start_left, __card->start_pos_x); */

        break;

    case ELM_EVENT_TOUCH_MOVE:
        IO_FRAME_HIGH();

        if (__card->page_in_touchdown != page->id) {
            return 0;
        }

#if TCFG_PAY_ALIOS_ENABLE
        if (ui_get_current_window_id() == ID_WINDOW_ALIPAY) {
            extern void alipay_component_show();
            alipay_component_show();
        }
#endif
        log_char('M');
        xoffset = e->pos.x - __card->pos_x;
        yoffset = e->pos.y - __card->pos_y;
        __card->pos_x = e->pos.x;
        __card->pos_y = e->pos.y;

        {
            int offset = e->pos.x - __card->start_pos_x;
            int target = __card->start_left + offset;
            xoffset = target - g_cur_left;
        }

        __card->result = get_direction(__card->startx, __card->starty, e->pos.x, e->pos.y);

        if ((__card->first_result == DIRECTION_NOT_INIT) &&
            (__card->result > DIRECTION_NONE)) {
            __card->first_result = __card->result;
        }

        if (__card->result != __card->first_result) {
            __card->startx = e->pos.x;
            __card->starty = e->pos.y;
            __card->first_result = __card->result;
        }

        __card->move_en = ui_page_move_callback_run();

//左滑侧边栏
#if(TCFG_UI_ENABLE_LEFT_MENU)
        if (ui_get_current_window_id() == ID_WINDOW_DIAL && (ui_page_num() == 1)) {
            if (__card->result == DIRECTION_LEFT) {
                ui_page_manager_mode_set(1);
            } else if (__card->result == DIRECTION_RIGHT) {
                ui_page_manager_mode_set(1);
            } else {
                ui_page_manager_mode_set(0);
            }
        } else {
            /* ui_page_manager_mode_set(0); */
        }
        page_switch_mode = ui_page_manager_mode_get();
#endif
        /* log_info("move_en:%d x_offset:%d result:%d first_result:%d page_switch_mode:%d slide_en:%d g_cur_left:%d", */
        /* __card->move_en, xoffset, __card->result, __card->first_result, page_switch_mode, __card->slide_en, g_cur_left); */
#if PAGE_SWITCH_MODE_NOT_LOOP
        ui_page_manager_mode_set(SLIDE_MODE_NOT_LOOP);
        page_switch_mode = ui_page_manager_mode_get();
#endif
        if ((__card->move_en != 0) && (xoffset != 0) && ((__card->result == DIRECTION_LEFT) || (__card->result == DIRECTION_RIGHT)) && ((__card->first_result == DIRECTION_LEFT) || (__card->first_result == DIRECTION_RIGHT))) {
            if (page_switch_mode) {//左滑侧边栏
                struct ui_page *first_page =  ui_page_get_first();
                int first_win = first_page->id;
                int last_win = ui_page_prev(first_win);
                int curr_win = ui_get_current_window_id();
                int prev_win = ui_page_prev(curr_win);
                int next_win = ui_page_next(curr_win);

                if ((page_switch_mode == SLIDE_MODE_NOT_RIGHT_ALL) && (__card->first_result == DIRECTION_RIGHT)) { //所有页面禁止右滑,单向循环
                    __card->slide_en = false;
                } else if ((page_switch_mode == SLIDE_MODE_NOT_RIGHT_FIRST) && (__card->first_result == DIRECTION_RIGHT) && (curr_win == first_win)) { //首页禁止右滑
                    __card->slide_en = false;
                } else if ((page_switch_mode == SLIDE_MODE_NOT_LOOP) && (((g_cur_left + xoffset > 0) && (curr_win == first_win)) || ((g_cur_left + xoffset < 0) && (next_win == first_win)))) {
                    xoffset = -g_cur_left;
                    __card->slide_en = true;
                    ui_page_move(ui_get_current_window_id(), xoffset, yoffset,  ui_card_get_move_mode());
                } else { //正常滑动
                    __card->slide_en = true;
                    ui_page_move(ui_get_current_window_id(), xoffset, yoffset,  ui_card_get_move_mode());

                }

                curr_elm = ui_core_get_element_by_id(ui_get_current_window_id());
                if ((__card->slide_en == false) && (curr_elm->css.left != 0)) {
                    __card->slide_en = true; //屏幕未居中时走自动居中流程,防止卡住
                }
                if (__card->slide_en) {
                    return 0;
                }
            } else {

                __card->slide_en = true;
                ui_page_move(ui_get_current_window_id(), xoffset, yoffset, ui_card_get_move_mode());

                return 0;
            }
        }
        break;

    case ELM_EVENT_TOUCH_L_MOVE:
    case ELM_EVENT_TOUCH_R_MOVE:
        /* case ELM_EVENT_TOUCH_U_MOVE: */
        /* case ELM_EVENT_TOUCH_D_MOVE: */
        if (page) {
            return 0;
        }
        break;

    case ELM_EVENT_TOUCH_UP:
        log_char('U');
        if (__card->slide_en) {
            if (e->has_energy == 0) {
                ui_page_set_anim_run_time(300, 100); // 设置过渡动画最大最小时间
                struct ui_page_anim_info anim_info = {0};
                anim_info.curr_win = ui_get_current_window_id();
                anim_info.mode = ui_card_get_move_mode();
                anim_info.dir = __card->first_result;
                /* anim_info.e = e; */
                anim_info.anim_start = ui_card_anim_start;
                anim_info.anim_stop = ui_card_anim_stop;
                int ret = ui_page_anim_auto(&anim_info);
                if (ret) {
                    __card->slide_en = 0;
                }
            }
        } else {
            ui_page_anim_stop();
            ui_page_move_stop(ui_get_current_window_id(), ui_card_get_move_mode());
        }
        break;
    case ELM_EVENT_TOUCH_ENERGY:
        if (__card->slide_en) {
            ui_page_set_anim_run_time(300, 100); // 设置过渡动画最大最小时间
            struct ui_page_anim_info anim_info = {0};
            anim_info.curr_win = ui_get_current_window_id();
            anim_info.mode = ui_card_get_move_mode();
            anim_info.dir = __card->first_result;
            anim_info.e = e;
            anim_info.anim_start = ui_card_anim_start;
            anim_info.anim_stop = ui_card_anim_stop;
            int ret = ui_page_anim_auto(&anim_info);
            if (ret) {
                __card->slide_en = 0;
            }
        }
        break;
    }
    if (__card->slide_en) {
        log_debug("<%s> silde_en\n", __func__);
        return 0;
    }
    return ui_event_ontouch(e);
}

#endif

struct ui_msg {
    struct list_head entry;
    struct touch_event touch;
    void(*msg_hook)(u8);
};

struct ui_msg msg_handle;

static void jlui_show_page(void *p)
{
    log_info("@@@@@@@@@@@ JLUI SHOW MAIN PAGE...\n");
    ui_show(PAGE_2);
    /* wdt_enable(); */
#if 0
    ui_page_add(PAGE_1);
    ui_page_add(PAGE_2);
    ui_page_add(PAGE_3);
#endif
}
static void jlui_sec_printf()
{
    log_info("jiffies_msec:%d", (int)jiffies_msec());
    wdt_clear();
    /* void jlui_malloc_ram_info_dump(); */
    /* jlui_malloc_ram_info_dump(); */
}

int ui_gif_msg_type_get()
{
    u32 msg_type = 0;
    ASSERT((UI_MSG_GIF_PLAY <= 0xf) && (UI_MSG_GIF_PLAY > 0X4));
    //			只拼4bit的空间						特征值
    msg_type |= (UI_MSG_GIF_PLAY << GIF_TYPE_OFFSET) | USER_MASK_TYPE;
    return msg_type;
}


int ui_gif_play_post(u16 timer_id, u32 msec, u32 delay, u32 elm_id, u32 task_id)
{
    OS_SEM sem;
    int msg[8];
    u8 need_pend = 0;

    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }

    msg[0] = ui_gif_msg_type_get() | (timer_id);
    msg[1] = timer_id;
    msg[2] = msec;
    msg[3] = delay;
    msg[4] = elm_id;
    msg[5] = task_id;
    msg[6] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[6] = (int)&sem;
    }

    if (!post_ui_msg(msg, 7)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }
    return 0;
}
void ui_hide_current_window_deal(u32 window)
{
    if (__card->slide_en) {
        ui_page_anim_stop();
        ui_page_set_busy(0);
        ui_core_clear_pagemove_flag();
        __card->slide_en = false;
    }
    ui_card_run_stop();
    dial_sel_exit();
    ui_page_switch_effect_stop();
    u32 hide_window  = window ? window : ui_get_current_window_id();
    if (hide_window) {
        ui_hide(hide_window);
        //如果还有其他页面
        if (ui_get_current_window_id()) {
            ui_hide(ui_get_current_window_id());
        }
    }
    ui_disp_line_buf_uninit();				//释放绘图buf
    ui_gpu_buf_release();
}

static int ui_multi_page_show(int id)
{
    struct element *elm = ui_core_get_element_by_id(id);
    if (elm) {
        return 0;
    }

    int ret = window_init(id);
    if (!ret) {
        struct element *elm = ui_core_get_element_by_id(id);
        if (!elm) {
            return -EINVAL;
        }
        elm->dc->refresh = false;
        ui_core_show(elm, true);
        elm->dc->refresh = true;
    }
    return ret;
}

static int ui_multi_page_hide(int id)
{
    struct element *elm = ui_core_get_element_by_id(id);
    if (!elm) {
        return 0;
    }

    ui_hide(id);

    return 0;
}


static void ui_task(void *p)
{
    int top_page_num;
    log_info("<%s> init\n", __func__);

    clock_lock("ui_show", clk_get_max_frequency());

    int msg[32];
    int ret;
    mem_var_init(3 * 1024, false);
    /*表盘管理初始化*/
    ui_dial_manage_init();
    /*内核初始化*/
    ui_framework_init(p);
    /*gpu绘图加速初始化*/
    jlui_gpu_blend_init();
#if (defined TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
    /*背景管理初始化*/
    ui_csbg_res_check();
#endif
    /*卡片管理初始化*/
    ui_page_init();
    /*设置参数初始化*/
    sys_param_init();
    ui_sysinfo_init();
    /*注册按键事件*/
    ui_key_set_send_event_cb(key_event_sent_to_ui_map);
    struct ui_style style;
    style.file = RES_PATH"JL/JL.sty";
    ret =  ui_set_style_file(&style);
    if (ret) {
        log_error("ui task fail!\n");
        return;
    }
    __this->init = 1;
    os_sem_post(&(__this->start_sem));

    /*debug使用*/
    /* sys_timeout_add(NULL, jlui_show_page, 1500); */
    /* sys_timer_add(NULL, jlui_sec_printf, 1000); */

    //添加自动熄屏定时器
    ui_auto_shut_down_enable();
    //注册tp扫描定时器
    if (__this->touch_event_call && __this->touch_event_interval) {
        __this->timer_id = sys_timer_add((void *)NULL, (void *)__this->touch_event_call, __this->touch_event_interval);
    }

#if TCFG_UI_LIMIT_ENABLE
    ui_limit_create();
#endif

#if (defined JLUI_DEMO_ENABLE && JLUI_DEMO_ENABLE)
    extern int jlui_demo_init(void);
    jlui_demo_init();
#endif

    /* sys_timeout_add(NULL,memtest_start,10*1000); */
    struct touch_event *touch;
    struct element_touch_event t;
    struct element_key_event e = {0};

    struct ui_msg ui_msg_move;
    INIT_LIST_HEAD(&msg_handle.entry);
    u8 touch_move_msg = 0;
    u32 time_start = 0;
    u32 time_end = 0;

    while (1) {
        time_end = jiffies_msec();
        if (time_start && time_end) {
            int run_time = time_end - time_start;
            int fps = run_time ? (1000 / run_time) : 0;
            log_debug("run_time:%d fps:%d ", run_time, fps);
        }
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            time_start  = 0;
            continue;
        }
        time_start = jiffies_msec();

        if (msg[0] == UI_MSG_TOUCH) {
            touch = (struct touch_event *)&msg[2];
            if (touch->event == ELM_EVENT_TOUCH_MOVE) {
                touch_move_msg++;
                ui_msg_move.msg_hook = (void (*)(u8))msg[1];
                memcpy((u8 *)&ui_msg_move.touch, (u8 *)&msg[2], sizeof(struct touch_event));
            } else {
                if (touch_move_msg) {
                    struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                    ui_msg->msg_hook = ui_msg_move.msg_hook;
                    memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                    list_add_tail(&ui_msg->entry, &msg_handle.entry);
                    touch_move_msg = 0;
                }
                struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                ui_msg->msg_hook = (void (*)(u8))msg[1];
                memcpy((u8 *)&ui_msg->touch, (u8 *)&msg[2], sizeof(struct touch_event));
                list_add_tail(&ui_msg->entry, &msg_handle.entry);
            }
            spin_lock(&lock);
            __this->touch_msg_counter--;
            spin_unlock(&lock);
            if (__this->touch_msg_counter) {
                continue;
            } else {
                if (touch_move_msg) {
                    struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                    ui_msg->msg_hook = ui_msg_move.msg_hook;
                    memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                    list_add_tail(&ui_msg->entry, &msg_handle.entry);
                    touch_move_msg = 0;
                }

                while (!list_empty(&msg_handle.entry)) {
                    struct ui_msg *ui_msg = list_first_entry(&msg_handle.entry, struct ui_msg, entry);
                    touch = (struct touch_event *)&ui_msg->touch;
                    t.event = touch->event;
                    t.pos.x = touch->x;
                    t.pos.y = touch->y;
                    t.has_energy = touch->has_energy;
                    if (ui_msg->msg_hook) {
                        ((void(*)(u8))ui_msg->msg_hook)(0);
                    }
                    ui_card_ontouch(&t);
                    /* ui_event_ontouch(&t); */
                    if (ui_msg->msg_hook) {
                        ((void(*)(u8))ui_msg->msg_hook)(1);
                    }
                    list_del(&ui_msg->entry);
                    free(ui_msg);
                }
                continue;
            }
        } else {//其他消息直接处理
            if (touch_move_msg) {
                struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                ui_msg->msg_hook = ui_msg_move.msg_hook;
                memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                list_add_tail(&ui_msg->entry, &msg_handle.entry);
                touch_move_msg = 0;
            }
            while (!list_empty(&msg_handle.entry)) {
                struct ui_msg *ui_msg = list_first_entry(&msg_handle.entry, struct ui_msg, entry);
                touch = (struct touch_event *)&ui_msg->touch;
                t.event = touch->event;
                t.pos.x = touch->x;
                t.pos.y = touch->y;
                t.has_energy = touch->has_energy;
                if (ui_msg->msg_hook) {
                    ((void(*)(u8))ui_msg->msg_hook)(0);
                }
                ui_card_ontouch(&t);
                /* ui_event_ontouch(&t); */
                if (ui_msg->msg_hook) {
                    ((void(*)(u8))ui_msg->msg_hook)(1);
                }
                list_del(&ui_msg->entry);
                free(ui_msg);
            }
        }

        u8 msg_type = (msg[0] & USER_MASK_TYPE) ? ((msg[0] >> GIF_TYPE_OFFSET) & 0xff) : (msg[0] & 0xff);
        switch (/* msg[0] */ msg_type) { //action
        case UI_MSG_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        case UI_MSG_OTHER:
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
            int ret = ui_message_filter_hook((const char *)msg[1], (void *)&msg[2]);
            if (ret) {
                break;
            }
#endif
            if (ui_get_current_window_id() > 0) {
                ui_message_handler(ui_get_current_window_id(), (const char *)msg[1], (void *)&msg[2]);
            }
#if TCFG_UI_ENABLE_SMARTWIN
            struct window *window = (struct window *)ui_core_get_element_by_id(ID_WINDOW_SMARTWIN);
            if (window) {
                ui_message_handler(ID_WINDOW_SMARTWIN, (const char *)msg[1], (void *)&msg[2]);
            }
#endif
            break;
        case UI_MSG_KEY:
            e.value = msg[1];
            int key_event_map  = 0;
            /* 菜单返回划屏过程中，不响应按键事件。避免划屏中按按键跳转导致死机 */
            extern int ui_return_page_effect_in_move();
            extern int menu_enter_app_state();
            if (ui_return_page_effect_in_move() || menu_enter_app_state()) {
                break;
            }
            /*key lock可以使消息直接发往底层，用于部分页面接管按键，退出页面时记得解锁*/
            if (!__this->key_lock) {
                key_event_map  = ui_key_event_deal_app(e.value);
            }
            /*若应用层不接管，则发给ui框架*/
            if (!key_event_map) {
                ret = ui_event_onkey(&e);
            }
            /*若ui框架不接管，则跑默认按键处理*/
            if (!ret && !key_event_map) {
                ui_default_key_event_deal_app(e.value);
            }
            break;
        case UI_MSG_TOUCH:
            touch = (struct touch_event *)&msg[2];
            t.event = touch->event;
            t.pos.x = touch->x;
            t.pos.y = touch->y;
            t.has_energy = touch->has_energy;
            if (msg[1]) {
                ((void(*)(u8))msg[1])(0);
            }
            ui_card_ontouch(&t);
            /* ui_event_ontouch(&t); */
            if (msg[1]) {
                ((void(*)(u8))msg[1])(1);
            }

            break;
        case UI_MSG_SHOW:
            int bl_ctrl = 0;
            if (lcd_sleep_status()) {
                lcd_sleep_ctrl(0);//退出低功耗
                bl_ctrl = 1;
            }
            //防止没调用ui_msg_hide
            ui_hide_current_window_deal(0);
#if CONFIG_WATCH_CASE_ENABLE || CONFIG_SERIAL_HMI_DISPLAY_CASE_ENABLE
            if (msg[1] == ID_WINDOW_DIAL) {
#if TCFG_UI_MENU_LIST_ENABLE
                menu_list_rec_clr();
#endif
                ui_return_page_effect_set_en(0);
            }
            UI_WINDOW_BACK_PUSH(msg[1]);
            if (!__this->ui_card_slide_dis) { //从菜单进入被添加到卡片的页面时，不清除记录，用于退出
                if (ui_page_search(msg[1]) || (msg[1] == ID_WINDOW_POWER_ON)) {
                    UI_WINDOW_BACK_CLEAN(); // 卡片
                    if (msg[1] != ID_WINDOW_POWER_ON) {
                        UI_WINDOW_BACK_PUSH(msg[1]);
                    }
                }
            } else {
                /*关闭卡片功能时，在表盘清除页面记录*/
                if (msg[1] == ID_WINDOW_DIAL) {
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
                    // UI_WINDOW_BACK_CLEAN(); // 卡片
#else
                    UI_WINDOW_BACK_CLEAN(); // 卡片
#endif
                    UI_WINDOW_BACK_PUSH(msg[1]);
                }
            }

#endif
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
            __this->lcd_bl_idle_flag = 0;//???
#endif
            ui_relate_init();
            msg[1] =  ui_id2remap(msg[1]);

            ui_show(msg[1]);
            if (bl_ctrl) {
                extern void lcd_wait_busy();//等推屏完成，非同步的 */
                lcd_wait_busy();
                lcd_bl_open();
            }


            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_MSG_HIDE:
            ui_hide_current_window_deal(msg[1]);
            void malloc_list_status_printf_all(int index, int count);
            /* malloc_list_status_printf_all(0,200); */
            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_MSG_SHOW_MULTI_PAGE:
            top_page_num = ui_multi_page_get_id_by_prio(-1, NULL);
            if (top_page_num > 0) {
                int *top_page_tab = (int *)malloc(top_page_num * sizeof(int));
                ASSERT(top_page_tab);
                ui_multi_page_get_id_by_prio(-1, top_page_tab);
                int i;
                for (i = 0; i < top_page_num; i++) {
                    ui_multi_page_show(top_page_tab[i]);
                }
                free(top_page_tab);
            }

            if (msg[1]) {
                os_sem_post((OS_SEM *)msg[1]);
            }
            break;
        case UI_MSG_HIDE_MULTI_PAGE:
            top_page_num = ui_multi_page_get_id_by_prio(-1, NULL);
            if (top_page_num > 0) {
                int *top_page_tab = (int *)malloc(top_page_num * sizeof(int));
                ASSERT(top_page_tab);
                ui_multi_page_get_id_by_prio(-1, top_page_tab);
                int i;
                for (i = 0; i < top_page_num; i++) {
                    ui_multi_page_hide(top_page_tab[i]);
                }
                free(top_page_tab);
            }
#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
            if (JLUI_MULTI_PAGE_OVERLAY_SUPPORT) {
                jlgpu_scheduler_async_gpu_cache_free_resident();
            }
#endif
            if (msg[1]) {
                os_sem_post((OS_SEM *)msg[1]);
            }
            break;
        case UI_MSG_GIF_PLAY:
            u16 timer_id = (u16)msg[1];
            u32 msec = msg[2];
            u32 gif_delay = msg[3];
            u32 elm_id = msg[4];
            u32 task_id = msg[5];
            u32 delay = jiffies_msec() - msec;
            int pagemove = ui_page_get_busy();

            if (!pagemove && (delay < gif_delay)) {
                gif_play(elm_id, task_id);
            } else {
                printf("drop gif frame ! pagemove %d, delay %d, gif_delay %d\n", pagemove, delay, gif_delay);
            }
            if (msg[6]) {
                os_sem_post((OS_SEM *)msg[6]);
            }
            break;
        case UI_TIME_TOUCH_RESUME:
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))
            if (!lcd_backlight_status()) {
                /* set_backlight_time(get_backlight_time_item());//防止在息屏状态下跳转到PC模式后不亮屏 */
            }
#endif
            if (lcd_sleep_status()) {
                lcd_sleep_ctrl(false);
                /* lcd_backlight_ctrl(true); */
            }
            set_screen_saver_status(0);
            if (__ui_display.timer_id) {
                break;
            }
            if (__ui_display.touch_event_call && __ui_display.touch_event_interval) {
                __ui_display.timer_id = sys_timer_add((void *)NULL, (void *)__ui_display.touch_event_call, __ui_display.touch_event_interval); //注册按键扫描定时器
            }
            break;
        case UI_TIME_TOUCH_SUPEND:
            if (__ui_display.timer_id) {
                sys_timer_del(__ui_display.timer_id);
                __ui_display.timer_id = 0;
            }
            if (!get_standby_watch_en()) {
#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
                // lcd_clear(0, 0, lcd_get_screen_width() - 1, 0, lcd_get_screen_height() - 1);
#else
                lcd_clear(0, 0, lcd_get_screen_width() - 1, 0, lcd_get_screen_height() - 1);
#endif
                lcd_sleep_ctrl(true);
                ui_relate_uninit();
                __this->lcd_bl_idle_flag = 1;
                /* malloc_list_status_printf_all(200); */
                //lcd_backlight_ctrl(false);
            }
            set_screen_saver_status(1);
            break;
        default:
            break;
        }
    }
}


int lcd_ui_init(void *arg)
{
    int err = 0;

    /* clock_alloc("ui_clk", 159 * 1000000UL); */

    mem_var_init(MEM_VAR_SIZE, false);

    os_sem_create(&(__this->start_sem), 0);
    err = task_create(ui_task, arg, UI_TASK_NAME);
    os_sem_pend(&(__this->start_sem), 0);

    if (err) {
        log_error("<%s> error:%d\n", __func__, err);
    } else {
        log_info("<%s> succ\n", __func__);
    }
    return err;
}

static void lcd_poweroff_uninit()
{
#if TCFG_UI_ENABLE
    if (get_ui_init_status()) {               // 判断是否初始化了ui
        if (lcd_sleep_status() == 0) {
            UI_HIDE_CURR_WINDOW();
            lcd_sleep_ctrl(1);
            struct lcd_interface *lcd = lcd_get_hdl();
            if (lcd->power_ctrl) {
                lcd->power_ctrl(false);
            }
        }
    }
#endif
}
platform_uninitcall(lcd_poweroff_uninit);


#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
/**
 * @brief 彩屏仓，重新切换背景图片时候
 *      先hide页面 -> 先资源释放 加载新资源 -> show原来页面
 *
 */
void csbg_reload_handler(void)
{
    int cur_id;
    log_info("<%s>", __func__);

    /*参考UI_MSG_HIDE消息处理 隐藏当前页面*/
    if (__card->slide_en) {
        ui_page_anim_stop();
        ui_page_set_busy(0);
        ui_core_clear_pagemove_flag();
        __card->slide_en = false;
    }

    cur_id = UI_GET_WINDOW_ID();
    if (cur_id != -1) {
        ui_hide(cur_id);
    }

    if (UI_GET_WINDOW_ID() != -1) {
        ui_hide(UI_GET_WINDOW_ID());
    }

    log_info("cur_id:%x", cur_id);

    /*更新图片资源*/
    csbg_change_handler(CSBG_TYPE_WALLPAPER);

    /*参考UI_MSG_SHOW消息处理 隐藏当前页面*/
    if (__card->slide_en) {
        ui_page_anim_stop();
        ui_page_set_busy(0);
        ui_core_clear_pagemove_flag();
        __card->slide_en = false;
    }

    if (cur_id == -1) {
        goto __end;
    }

    ui_show(cur_id);

__end:
    return;
}

static int nowcardpage = 0;

int ui_page_set_curr_deal(int curr_id)
{
    log_info("%s %d", __func__, __LINE__);
    nowcardpage = curr_id;
    return 0;
}

bool cpc_is_card_page(int mem_id)
{
    extern u32 ui_show_page_list[20];
    for (int i = 0; i < ARRAY_SIZE(ui_show_page_list); i++) {
        if (ui_show_page_list[i] == mem_id) {
            return true;
        }
    }
    return false;
}

void cpc_go_to_history_card()
{
    ui_card_enable();
    if (nowcardpage != 0) {
        UI_WINDOW_BACK_CLEAN(); // 卡片
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(nowcardpage);
        return;
    }

    UI_WINDOW_BACK_CLEAN(); // 卡片
    UI_HIDE_CURR_WINDOW();
    UI_SHOW_WINDOW(ID_WINDOW_MUSIC_PLAYER);
}
#else//watch

int ui_page_set_curr_deal(int curr_id)
{
#if TCFG_VIDEO_DIAL_ENABLE
    if (curr_id == ID_WINDOW_DIAL) {
        extern video_player_handle_t _dial_video_player;
        if (_dial_video_player) {
            video_player_resume(_dial_video_player);
        }
    }
#endif
    UI_WINDOW_BACK_CLEAN(); // 卡片
    UI_WINDOW_BACK_PUSH(curr_id);
    return 0;
}
#endif

#else

int key_is_ui_takeover()
{
    return 0;
}

void key_ui_takeover(u8 on)
{

}

int ui_key_msg_post(int key)
{
    return 0;
}


int ui_touch_msg_post(struct touch_event *event)
{
    return 0;
}

int ui_touch_msg_post_withcallback(struct touch_event *event, void (*cb)(u8 finish))
{
    return 0;
}

void ui_touch_timer_delete()
{

}

void ui_touch_timer_start()
{

}

void ui_backlight_ctrl(u8 lcd_open_flag)
{

}

void ui_auto_shut_down_re_run(void)
{

}

#endif

#endif

