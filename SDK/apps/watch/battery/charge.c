#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".charge.data.bss")
#pragma data_seg(".charge.data")
#pragma const_seg(".charge.text.const")
#pragma code_seg(".charge.text")
#endif
#include "app_config.h"
#include "asm/charge.h"
#include "asm/power_interface.h"
#include "system/event.h"
#include "system/includes.h"
#include "app_action.h"
#include "asm/wdt.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "bt_tws.h"
#include "usb/otg.h"
#include "bt_common.h"
#include "battery_manager.h"
#include "app_charge.h"
#include "dual_bank_updata_api.h"
#include "gpadc.h"
#include "ui/ui_api.h"
#include "jlui_app/ui_sys_param.h"

#define LOG_TAG_CONST       APP_CHARGE
#define LOG_TAG             "[APP_CHARGE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_CHARGE_ENABLE

static u8 charge_full_flag = 0;

#if TCFG_RECHARGE_ENABLE
static int recharge_timer = 0;

static void recharge_check(void *priv)
{
    if ((adc_get_voltage(AD_CH_PMU_VBAT) * AD_CH_PMU_VBAT_DIV) < TCFG_RECHARGE_VOLTAGE) {
        sys_timer_del(recharge_timer);
        recharge_timer = 0;
        charge_start();
    }
}

static void recharge_detect_start(void)
{
    if (recharge_timer == 0) {
        recharge_timer = sys_timer_add(NULL, recharge_check, 10000);
    }
}

static void recharge_detect_close(void)
{
    if (recharge_timer) {
        sys_timer_del(recharge_timer);
        recharge_timer = 0;
    }
}
#endif

u8 get_charge_full_flag(void)
{
    return charge_full_flag;
}

static void charge_start_deal(void)
{
    log_info("%s\n", __FUNCTION__);

    power_set_mode(PWR_LDO15);

    batmgr_send_msg(BAT_MSG_CHARGE_START, 0);
}

static void charge_full_deal(void)
{
    const struct app_charge_handler *handler;

    log_info("%s\n", __func__);

    charge_full_flag = 1;

    battery_q32f_push_full_flag();

    charge_close();

#if TCFG_RECHARGE_ENABLE
    recharge_detect_start();
#endif
    for_each_app_charge_handler(handler) {
        handler->handler(CHARGE_EVENT_CHARGE_FULL, 0);
    }

    if (get_charge_poweron_en() == 0) {
        /* power_set_soft_poweroff(); */
    } else {
        batmgr_send_msg(BAT_MSG_CHARGE_FULL, 0);
    }
}

static void charge_close_deal(void)
{
    log_info("%s\n", __FUNCTION__);

    /* power_set_mode(TCFG_LOWPOWER_POWER_SEL); */

    batmgr_send_msg(BAT_MSG_CHARGE_CLOSE, 0);
}


void app_charge_power_off_keep_mode()
{
    //兼容一些充电仓5v输出慢的时候会导致无法充电的问题
    if (get_lvcmp_det()) {
        log_info("...charge ing...\n");
        cpu_reset();
    }

    log_info("get_charge_online_flag:%d %d\n", get_charge_online_flag(), get_ldo5v_online_hw());
    if (get_ldo5v_online_hw() && get_charge_online_flag()) {
        power_set_soft_poweroff();
    } else {
        charge_check_and_set_pinr(1);
#if TCFG_CHARGE_OFF_POWERON_EN
        cpu_reset();
#else
        power_set_soft_poweroff();
#endif
    }
}

static void ldo5v_keep_softoff(void)
{
    log_info("%s\n", __func__);

    if (!app_in_mode(APP_MODE_IDLE)) {
        sys_enter_soft_poweroff(POWEROFF_POWER_KEEP);
    } else {
        app_charge_power_off_keep_mode();
    }
}

/*ldoin电压大于拔出电压(0.6V左右)且小于VBat电压时调用该函数进行一些错误提示或其他处理*/
void ldo5v_keep_deal(void)
{
    int abandon = 0;
    const struct app_charge_handler *handler;

    log_info("%s\n", __func__);
#if TCFG_RECHARGE_ENABLE
    recharge_detect_close();
#endif


    //插入交换
    batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);

    charge_check_and_set_pinr(0);

    for_each_app_charge_handler(handler) {
        abandon += handler->handler(CHARGE_EVENT_LDO5V_KEEP, 0);
    }

#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
    //升级过程中,不执行关机操作
    if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
        return;
    }
#endif

    if (get_charge_poweron_en() == 0) {
        if (abandon == 0) {
            ldo5v_keep_softoff();
        }
    } else {
        batmgr_send_msg(BAT_MSG_CHARGE_ERR, 0);
    }
}

void charge_ldo5v_in_deal(void)
{
    int abandon = 0;
    const struct app_charge_handler *handler;

    log_info("%s\n", __FUNCTION__);
#if TCFG_RECHARGE_ENABLE
    recharge_detect_close();
#endif

    //插入交换
    batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);

    charge_full_flag = 0;

    battery_q32f_push_full_flag();

    charge_check_and_set_pinr(0);

    for_each_app_charge_handler(handler) {
        abandon += handler->handler(CHARGE_EVENT_LDO5V_IN, 0);
    }

#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
    //升级过程中,不执行充电插入关机流程
    if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
        return;
    }
#endif

    if (get_charge_poweron_en() == 0) {
        if (!app_in_mode(APP_MODE_IDLE)) {
            if (abandon == 0) {
                sys_enter_soft_poweroff(POWEROFF_RESET);
            }
        } else {
            charge_start();
            wdt_init(WDT_32S);
            log_info("set wdt to 32s!\n");
            goto _check_reset;
        }
    } else {
        charge_start();
        goto _check_reset;
    }
    return;

_check_reset:
    //防止耳机低电时,插拔充电有几率出现关机不充电问题
    if (app_var.goto_poweroff_flag) {
        cpu_reset();
    }
}

void charge_ldo5v_off_deal(void)
{
    int abandon = 0;
    int off_type = LDO5V_OFF_TYPE_NORMAL_ON;
    bool lowpower_flag = FALSE, is_bt_mode, is_idle_mode;
    const struct app_charge_handler *handler;

    log_info("%s\n", __FUNCTION__);

    //拨出交换
    batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);

    charge_full_flag = 0;

    battery_q32f_push_full_flag();

    charge_close();
    power_set_mode(TCFG_LOWPOWER_POWER_SEL);

#if TCFG_RECHARGE_ENABLE
    recharge_detect_close();
#endif
    batmgr_send_msg(BAT_MSG_CHARGE_LDO5V_OFF, 0);

    charge_check_and_set_pinr(1);

#if TCFG_SYS_LVD_EN
    lowpower_flag = get_vbat_need_shutdown();
#endif
    is_bt_mode = app_in_mode(APP_MODE_BT);
    is_idle_mode = app_in_mode(APP_MODE_IDLE);

    if ((get_charge_poweron_en() == 0)) {
        wdt_init(WDT_4S);
        log_info("set wdt to 4s!\n");
#if TCFG_CHARGE_OFF_POWERON_EN
        log_info("ldo5v off,task switch to BT\n");
        if (app_var.goto_poweroff_flag == 0) {
            if (!is_bt_mode) {
                if (lowpower_flag == FALSE) {
                    off_type = LDO5V_OFF_TYPE_NORMAL_ON;//正常拔出开机
                } else {
                    log_info("ldo5v off,lowpower,need enter softpoweroff\n");
                    off_type = LDO5V_OFF_TYPE_LOWPOWER_OFF;//拔出低电关机
                }
            }
        }
#else //TCFG_CHARGE_OFF_POWERON_EN
        log_info("ldo5v off,enter softpoweroff\n");
        off_type = LDO5V_OFF_TYPE_NORMAL_OFF;//正常拔出关机
#endif
    } else {
        if (is_idle_mode && (app_var.goto_poweroff_flag == 0)) {
#if TCFG_CHARGE_OFF_POWERON_EN
            log_info("ldo5v off,task switch to BT\n");
            if (lowpower_flag == FALSE) {
                off_type = LDO5V_OFF_TYPE_NORMAL_ON;//正常拔出开机
            } else {
                log_info("ldo5v off,lowpower,need enter softpoweroff\n");
                off_type = LDO5V_OFF_TYPE_LOWPOWER_OFF;//拔出低电关机
            }
#else
            off_type = LDO5V_OFF_TYPE_NORMAL_OFF;//正常拔出关机
#endif
        }
    }

    for_each_app_charge_handler(handler) {
        abandon += handler->handler(CHARGE_EVENT_LDO5V_OFF, off_type);
    }

    if (abandon) {
        return;
    }

    switch (off_type) {
    case LDO5V_OFF_TYPE_NORMAL_OFF:
    case LDO5V_OFF_TYPE_LOWPOWER_OFF:
        power_set_soft_poweroff();
        break;
    case LDO5V_OFF_TYPE_CHARGESTORE_OFF:
        if (is_bt_mode) {
            sys_enter_soft_poweroff(POWEROFF_NORMAL);
        } else {
            power_set_soft_poweroff();
        }
        break;
    case LDO5V_OFF_TYPE_NORMAL_ON:
#if 1//idle充电，需要拔出开机先power_on
        if (app_in_mode(APP_MODE_IDLE)) {
            app_var.play_poweron_tone = 1;
            app_send_message(APP_MSG_GOTO_MODE, APP_MODE_POWERON);
        }
#else
        app_var.play_poweron_tone = 0;
        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
#endif//直接开机
        break;
    }
}

static int app_charge_event_handler(int *msg)
{
    int ret = false;
    u8 otg_status = 0;

    switch (msg[0]) {
    case CHARGE_EVENT_CHARGE_START:
#if TCFG_UI_ENABLE
        if (get_ui_sys_param(bed_light)) {//床头时钟
            /* UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BEDSIDE_WATCH); */
            /* UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_BEDSIDE_WATCH, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_CHARGE); */
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_BEDSIDE_WATCH);
        } else {
#if 0
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BATCHARGE);
            UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_BATCHARGE, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_CHARGE);
#else
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_BATCHARGE);
#endif
        }
#endif
        charge_start_deal();
        break;
    case CHARGE_EVENT_CHARGE_CLOSE:
        charge_close_deal();

#if TCFG_UI_ENABLE
        if (get_ui_sys_param(bed_light)) {
            /* UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BEDSIDE_WATCH);		 */
        } else {
            /*充满后退出充电页面*/
#if 0
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BATCHARGE);
            //退出页面后，如果没有记录页面，则回表盘
            if (!ui_return_page_id()) {
                UI_SHOW_WINDOW(ID_WINDOW_DIAL);
            }
#else
            //拔出的时候再显示一次电量页面
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_BATCHARGE);
#endif
        }
#endif
        break;
    case CHARGE_EVENT_CHARGE_FULL:
        charge_full_deal();
        break;
    case CHARGE_EVENT_LDO5V_KEEP:
        ldo5v_keep_deal();
        break;
    case CHARGE_EVENT_LDO5V_IN:

#if (TCFG_APP_PC_EN &&TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)
        void pc_charge_usb_detect_start();
        pc_charge_usb_detect_start();
#endif//(TCFG_APP_PC_EN &&TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)

#if ((TCFG_OTG_MODE & OTG_SLAVE_MODE) && (TCFG_OTG_MODE & OTG_CHARGE_MODE))
        while (1) {
            otg_status = usb_otg_online(0);
            if (otg_status != OTG_STATE_DISCONN_MODE) {
                break;
            }
            os_time_dly(2);
        }
        if (otg_status == OTG_STATE_SLAVE_MODE) {
            set_charge_poweron_en(1);
        }
#endif

        if (get_charge_poweron_en() || (otg_status != OTG_STATE_SLAVE_MODE)) {
            charge_ldo5v_in_deal();
        }
        break;
    case CHARGE_EVENT_LDO5V_OFF:
#if TCFG_UI_ENABLE
        if (get_ui_sys_param(bed_light)) {
            /*拔出时退出床头时钟*/
#if 0
            UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BEDSIDE_WATCH);
#else
            if (ui_get_current_window_id() == ID_WINDOW_BEDSIDE_WATCH) {
                int page_ret = UI_WINDOW_BACK_SHOW(0);
                if (page_ret == 2) {
                    UI_SHOW_WINDOW(ID_WINDOW_DIAL);
                }
            }
#endif
        } else {
            /* UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BATCHARGE); */
        }
#endif
#if ((TCFG_OTG_MODE & OTG_SLAVE_MODE) && (TCFG_OTG_MODE & OTG_CHARGE_MODE))
        while (1) {
            otg_status = usb_otg_online(0);
            if (otg_status != OTG_STATE_DISCONN_MODE) {
                break;
            }
            os_time_dly(2);
        }
#endif
        if (get_charge_poweron_en() || (otg_status != OTG_STATE_SLAVE_MODE)) {
            charge_ldo5v_off_deal();
        }

#if (TCFG_APP_PC_EN &&TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)
        void pc_charge_usb_detect_stop();
        pc_charge_usb_detect_stop();
#endif//(TCFG_APP_PC_EN &&TCFG_PC_MODE_USB_DETECT_DURING_CHARGING)


        break;
    default:
        break;
    }

    return ret;
}

APP_MSG_HANDLER(bat_charge_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = app_charge_event_handler,
};

#else

u8 get_charge_full_flag(void)
{
    return 0;
}

#endif

