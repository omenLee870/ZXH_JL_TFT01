#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".chgbox_det.data.bss")
#pragma data_seg(".chgbox_det.data")
#pragma const_seg(".chgbox_det.text.const")
#pragma code_seg(".chgbox_det.text")
#endif
#include "gpio.h"
#include "gpadc.h"
#include "asm/charge.h"
#include "asm/efuse.h"
/* #include "asm/power/p33.h" */
#include "system/event.h"
#include "app_config.h"
#include "chgbox_det.h"
#include "chgbox_box.h"
#include "chgbox_ctrl.h"
#include "system/timer.h"
#include "chargeIc_manage.h"
#include "screen_trans/smartbox_user_app.h"
#include "ui/ui_api.h"
#include "app_config.h"
#include "smartbox_info_manager.h"
#include "app_mode_manager/app_mode_manager.h"
#include "app_task.h"
#if CONFIG_WATCH_CASE_ENABLE
#include "jlui_app/ui_sys_param.h"
#endif

#if(TCFG_CHARGE_BOX_ENABLE &&!TCFG_CHARGE_IC_MANAGER_MODE_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[APPCHG_DET]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define LOW_LEVEL     0
#define HIGHT_LEVEL   1
#define DETECT_PERIOD   20//ms

//USB上下线检测时间配置
#define USB_DET_ONLINE_LEVEL    (HIGHT_LEVEL)//高电平表示在线
#define USB_DET_OFFLINE_TIME    (100/DETECT_PERIOD)
#define USB_DET_ONLINE_TIME     (100/DETECT_PERIOD)

//HALL开关检测时间配置
#define HALL_DET_OPEN_LEVEL     (HIGHT_LEVEL)//高电平表示开盖
#define HALL_DET_OPEN_TIME      (500/DETECT_PERIOD)
#define HALL_DET_CLOSE_TIME     (500/DETECT_PERIOD)

//升压检测电压及时间配置
#define LDO_SUCC_VOLTAGE        (2500)
#define LDO_DET_ON_TIME         (60/DETECT_PERIOD)
#define LDO_DET_OFF_TIME        (60/DETECT_PERIOD)

//满电检测时间
#define CHARGE_FULL_LEVEL       (HIGHT_LEVEL)//高电平表示充满
#define STAT_FULL_POWER_TIME    1//(1000/DETECT_PERIOD)

//给耳机充电电流限制
#define CURRENT_DETECT_PERIOD   (1000)
#define CHARGE_CURRENT_LIMIT    (200)//mA
#define CHARGE_CURRENT_RES      (500)//500mΩ

//充电温度限制
#define CHARGE_TEMP_DETECT_PERIOD   (10000)//温度检测周期
#define CHARGE_TEMP_ABNORMAL_LOW    (330)//温度异常的低值33k
#define CHARGE_TEMP_NORMAL_LOW      (250)//温度正常的低值25k
#define CHARGE_TEMP_NORMAL_HIGH      (53)//温度正常的低值5.3k
#define CHARGE_TEMP_ABNORMAL_HIGH    (43)//温度异常的低值4.3k
#define CHARGE_TEMP_AVG_COUNTS       (10)//取10次平均值

//充电舱欠压,不开机电压
#define POWER_ON_SHUTDOWN_VOLT  (2800)

//电池电压检测,进入退出低电检测电压及时间配置
#define POWER_DETECT_PERIOD     (10000)
#define LOWPOWER_ENTER_VOLTAGE  (3300)
#define LOWPOWER_EXIT_VOLTAGE   (3320)
#define BAT_AVE_COUNTS          (10) //检测n次
#define BAT_CUT_COUNTS          (2)  //首尾各去n个后计算均值
static u16 battery_value_tab[BAT_AVE_COUNTS];
static volatile u16 cur_bat_volt = 0;//当前充电盒子的电量
static volatile u8 battery_detect_skip = 0;//在USB插拔期间电压不稳定,不采集电压
static volatile u8 detect_init_ok;
static u16 cur_ear_curr, ear_static_current;//耳机端充电电流检测静态值

extern void delay_2ms(int cnt);
extern u8 get_cur_total_ad_ch(void);
extern void clr_wdt(void);
extern void user_charge_insert_detect();
extern void user_charge_remove_detect();
extern void charge_event_to_user(u8 event);
extern void p33_io_wakeup_edge(u32 gpio, P33_IO_WKUP_EDGE edge);

/*------------------------------------------------------------------------------------*/
/**@brief    电池电量检测
   @param    无
   @return   无
   @note     用于检测电池的电量,作出低电判断等
*/
/*------------------------------------------------------------------------------------*/
static int power_det_timer;
static void power_detect_func(void *priv)
{
    static u8 bat_cnt = 0;
    u16 ad_min, bat_volt_tmp;
    u8 i, j, k, low_flag;

    if (battery_detect_skip) {
        usr_timer_del(power_det_timer);
        power_det_timer = 0;
        bat_cnt = 0;
        return;
    }

    battery_value_tab[bat_cnt++] = adc_get_voltage(TCFG_BAT_DET_AD_CH) * 4; //注意电路分压

    if (bat_cnt == BAT_AVE_COUNTS) { //n次检测后排序
        for (i = 1; i < BAT_AVE_COUNTS; i++) {
            for (j = i; j > 0; j--) {
                if (battery_value_tab[j] < battery_value_tab[j - 1]) {
                    ad_min = battery_value_tab[j];
                    battery_value_tab[j] = battery_value_tab[j - 1];
                    battery_value_tab[j - 1] = ad_min;
                }
            }
        }
        bat_volt_tmp = 0;
        for (k = BAT_CUT_COUNTS; k < (BAT_AVE_COUNTS - BAT_CUT_COUNTS); k++) {
            bat_volt_tmp = battery_value_tab[k] + bat_volt_tmp;
        }
        bat_volt_tmp = (bat_volt_tmp / (BAT_AVE_COUNTS - (BAT_CUT_COUNTS * 2))); //均值
        //更新电池电量,USB在线时只能增,USB不在线是只能减
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            if (bat_volt_tmp > cur_bat_volt) {
                cur_bat_volt = bat_volt_tmp;
            }
        } else {
            if (bat_volt_tmp < cur_bat_volt) {
                cur_bat_volt = bat_volt_tmp;
            }
        }

        low_flag = adc_check_vbat_lowpower();

        log_info("vbat power: %dmV\n", cur_bat_volt);

        //低电与退出低电检测
        if (sys_info.lowpower_flag) {
            if ((cur_bat_volt > LOWPOWER_EXIT_VOLTAGE) && (low_flag == 0)) {
                sys_info.lowpower_flag = 0;
                app_chargebox_event_to_user(CHGBOX_EVENT_EXIT_LOWPOWER);
                log_info("Exit cbox lowpower\n");
            }
        } else {
            if (cur_bat_volt <= LOWPOWER_ENTER_VOLTAGE || low_flag) {
                sys_info.lowpower_flag = 1;
                app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_LOWPOWER);
                log_info("Enter cbox lowpower\n");
            }
        }
        usr_timer_del(power_det_timer);
        power_det_timer = 0;
        bat_cnt = 0;//清0，进行下一次统计
    }
}

static void power_detect_start(void *priv)
{
    if (!power_det_timer) {
        adc_check_vbat_lowpower();
        power_det_timer = usr_timer_add(NULL, power_detect_func, DETECT_PERIOD, 1);
    }
}

static void power_detect_init(void)
{
    sys_timer_add(NULL, power_detect_start, POWER_DETECT_PERIOD);
    power_det_timer = usr_timer_add(NULL, power_detect_func, DETECT_PERIOD, 1);
}

/*------------------------------------------------------------------------------------*/
/**@brief    usb上下线检测
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static int usb_det_timer;
static void usb_detect_func(void *priv)
{
    static u16 usb_in_det_cnt = 0;
    static u16 usb_out_det_cnt = 0;
#if TCFG_CHARGE_MOUDLE_OUTSIDE
    // extern void usb_det_wakeup_set_edge(u8 edge);
    // u8 io_level = gpio_read(TCFG_USB_ONLE_DET_IO);
    u8 io_level = (LDO5V_DET_GET() == 0) ? 0 : 1;
    log_info("io_level:%d\n", io_level);
#else
    u8 io_level = get_lvcmp_det();
#endif

    battery_detect_skip = 1;
    sys_info.life_cnt = 0;//状态变化时，清除休眠计时
    if (((USB_DET_ONLINE_LEVEL == LOW_LEVEL) && (!io_level)) || ((USB_DET_ONLINE_LEVEL == HIGHT_LEVEL) && io_level)) {//插入
        usb_in_det_cnt++;
        if (usb_in_det_cnt >= USB_DET_ONLINE_TIME) {
            log_info("%s %d detect_off", __func__, __LINE__);
            usr_timer_del(usb_det_timer);
            usb_det_timer = 0;
            usb_in_det_cnt = 0;
            usb_out_det_cnt = 0;
            battery_detect_skip = 0;
            if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
                sys_info.status[USB_DET] = STATUS_ONLINE;
                user_charge_insert_detect();
                app_chargebox_event_to_user(CHGBOX_EVENT_USB_IN);
                // charge_event_to_user(CHARGE_EVENT_CHARGE_START);

                // UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BATCHARGE);
                // UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_BATCHARGE, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_CHARGE);
                CS_UI_POPUP_PAGE(ID_WINDOW_BATCHARGE);
                log_info("usb online!");
#if (TCFG_CHARGE_MOUDLE_OUTSIDE)
                // usb_det_wakeup_set_edge(io_level);
#else
                LVCMP_EDGE_SEL(1);	//检测ldoin比vbat电压低的情况(充电仓给电池充满后会关断，此时电压会掉下来)
#endif
            }
        }
    } else {//拔出
        usb_out_det_cnt++;
        if (usb_out_det_cnt >= USB_DET_OFFLINE_TIME) {

            log_info("%s %d detect_off", __func__, __LINE__);
            usr_timer_del(usb_det_timer);
            usb_det_timer = 0;
            usb_in_det_cnt = 0;
            usb_out_det_cnt = 0;
            battery_detect_skip = 0;
            if (sys_info.status[USB_DET] == STATUS_ONLINE) {
                sys_info.localfull = 0; ///充满标志清0
                sys_info.status[USB_DET] = STATUS_OFFLINE;
                user_charge_remove_detect();
                app_chargebox_event_to_user(CHGBOX_EVENT_USB_OUT);
                charge_event_to_user(CHARGE_EVENT_CHARGE_CLOSE);
                // UI_WINDOW_PREEMPTION_POP(ID_WINDOW_BATCHARGE);
                if (2 == UI_WINDOW_BACK_SHOW(2)) {
                    UI_SHOW_WINDOW(ID_WINDOW_DEFAULT);
                }
                log_info("usb offline!");
#if (TCFG_CHARGE_MOUDLE_OUTSIDE)
                // usb_det_wakeup_set_edge(io_level);
#else
                LVCMP_EDGE_SEL(0);//拔出后重新检测插入
#endif
            }
        }
    }
}

void usb_wakeup_deal(void)
{
    log_info("%s %d", __func__, __LINE__);
    if (detect_init_ok && (!usb_det_timer)) {
        usb_det_timer = usr_timer_add(NULL, usb_detect_func, DETECT_PERIOD, 1);
    }
}
void usb_wakeup_callback(P33_IO_WKUP_EDGE edge)
{
    usb_wakeup_deal();
}
static void usb_detect_init(void)
{
    usb_det_timer = usr_timer_add(NULL, usb_detect_func, DETECT_PERIOD, 1);
}

/*------------------------------------------------------------------------------------*/
/**@brief    霍尔传感器开合盖检测
   @param    无
   @return   无
   @note     传感器上下线处理，设置状态、发送相关事件
*/
/*------------------------------------------------------------------------------------*/
static int hall_det_timer;
// extern void hall_det_wakeup_set_edge(u8 edge);
#if (TCFG_CHARGE_BOX_ENABLE && (TCFG_HALL_PORT != NO_CONFIG_PORT))
void hall_det_wakeup_set_edge(u8 edge)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("%s() rets: 0x%x edge:%d\n", __func__, rets, edge);
    p33_io_wakeup_edge(TCFG_HALL_PORT, edge ? FALLING_EDGE : RISING_EDGE);
}
#else
void hall_det_wakeup_set_edge(u8 edge)
{
}
#endif /* (TCFG_CHARGE_BOX_ENABLE && (TCFG_HALL_PORT != NO_CONFIG_PORT)) */

static void hall_detect_func(void *priv)
{
    static u16 hall_open_det_cnt = 0;
    static u16 hall_close_det_cnt = 0;
    u8 io_level = gpio_read(TCFG_HALL_PORT);
    log_info("hall io_level:%d\n", io_level);
    sys_info.life_cnt = 0;

    if (((HALL_DET_OPEN_LEVEL == LOW_LEVEL) && (!io_level)) || ((HALL_DET_OPEN_LEVEL == HIGHT_LEVEL) && io_level)) {
        hall_open_det_cnt++;
        hall_close_det_cnt = 0;//需要保持状态稳定中途IO状态切换应把之前状态计数清0
        if (hall_open_det_cnt >= HALL_DET_OPEN_TIME) {
            usr_timer_del(hall_det_timer);
            hall_det_timer = 0;
            hall_open_det_cnt = 0;
            hall_close_det_cnt = 0;
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
            if (lcd_sleep_status()) {
                ui_screen_recover(1);
            }
#endif
            if (sys_info.status[LID_DET] == STATUS_OFFLINE) {
                sys_info.status[LID_DET] = STATUS_ONLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_OPEN_LID);
                hall_det_wakeup_set_edge(io_level);
                log_info("hall open!\n");
                extern void ex_clid_trig();
                sbox_box_clid_status_set(LID_OPEN);
                ex_clid_trig();
            }
        }
    } else {
        hall_close_det_cnt++;
        hall_open_det_cnt = 0;//需要保持状态稳定中途IO状态切换应把之前状态计数清0
        if (hall_close_det_cnt >= HALL_DET_CLOSE_TIME) {
            usr_timer_del(hall_det_timer);
            hall_det_timer = 0;
            hall_open_det_cnt = 0;
            hall_close_det_cnt = 0;
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
            if (!lcd_sleep_status() && (app_get_current_mode_name() != APP_MODE_POWERON)) {//判断状态机的任务状态避免开机屏闪
                ui_screen_saver(NULL);
            }
#endif
            if (sys_info.status[LID_DET] == STATUS_ONLINE) {
                sys_info.status[LID_DET] = STATUS_OFFLINE;
                app_chargebox_event_to_user(CHGBOX_EVENT_CLOSE_LID);
                hall_det_wakeup_set_edge(io_level);
                log_info("hall close!\n");
                // app_chargebox_timer_del();
                extern void ex_clid_trig();
                sbox_box_clid_status_set(LID_CLOSE);
                ex_clid_trig();
            }
        }
    }
}


extern void app_chargebox_timer_add(void);

static void app_chargebox_timer_add_call(void)
{
    log_info("func:%s , cpu_in_irq():%d  , cpu_irq_disabled():%d \n", __func__, cpu_in_irq(), cpu_irq_disabled());
    if (cpu_in_irq()) {
        int msg[3];
        msg[0] = (int)app_chargebox_timer_add;
        msg[1] = 0;
        os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    } else {
        app_chargebox_timer_add();
    }
}
void hall_wakeup_deal(P33_IO_WKUP_EDGE edge)
{
    // app_chargebox_timer_add();
    app_chargebox_timer_add_call();
    if (detect_init_ok && (!hall_det_timer)) {
        hall_det_timer = usr_timer_add(NULL, hall_detect_func, DETECT_PERIOD, 1);
    }
}

static void hall_detect_init(void)
{
    hall_det_timer = usr_timer_add(NULL, hall_detect_func, DETECT_PERIOD, 1);
}

#if TCFG_LDO_DET_ENABLE
/*------------------------------------------------------------------------------------*/
/**@brief    LDO升压成功检测
   @param    无
   @return   无
   @note     用于检测ldo升压是否正常
*/
/*------------------------------------------------------------------------------------*/
static int ldo_det_timer;
static void ldo_detect_func(void *priv)
{
    static u16 ldo_on_detect_cnt = 0;
    static u16 ldo_off_detect_cnt = 0;
    u32 ldo_voltage = adc_get_voltage(TCFG_CHG_LDO_DET_AD_CH) * 23 / 13;
    log_info("%s %d", __func__, ldo_voltage);
    if (ldo_voltage >= LDO_SUCC_VOLTAGE) {
        ldo_on_detect_cnt++;
        if (ldo_on_detect_cnt >= LDO_DET_ON_TIME) {
            usr_timer_del(ldo_det_timer);
            ldo_det_timer = 0;
            ldo_on_detect_cnt = 0;
            ldo_off_detect_cnt = 0;
            if (sys_info.status[LDO_DET] == STATUS_OFFLINE) {
                sys_info.status[LDO_DET] = STATUS_ONLINE;
                log_info("LDO ON\n");
            }
        }
    } else {
        if (ldo_voltage < LDO_SUCC_VOLTAGE) { //注意分压
            ldo_off_detect_cnt++;
            if (ldo_off_detect_cnt >= LDO_DET_OFF_TIME) {
                usr_timer_del(ldo_det_timer);
                ldo_det_timer = 0;
                ldo_on_detect_cnt = 0;
                ldo_off_detect_cnt = 0;
                if (sys_info.status[LDO_DET] == STATUS_ONLINE) {
                    sys_info.status[LDO_DET] = STATUS_OFFLINE;
                    log_info("LDO OFF\n");
                }
            }
        }
    }
}

//提供升压使能和关闭的时候调用下
void ldo_wakeup_deal(void *priv)
{
    if (detect_init_ok && (!ldo_det_timer)) {
        ldo_det_timer = usr_timer_add(NULL, ldo_detect_func, DETECT_PERIOD, 1);
    }
}

static void ldo_detect_init(void)
{
    sys_timer_add(NULL, ldo_wakeup_deal, 10000);//10s起来检测一次
    ldo_det_timer = usr_timer_add(NULL, ldo_detect_func, DETECT_PERIOD, 1);
}
#endif

/*------------------------------------------------------------------------------------*/
/**@brief    舱电池充满检测
   @param    无
   @return   无
   @note     用于检测是否充满电池
*/
/*------------------------------------------------------------------------------------*/
static int usb_charge_full_det_timer;
static void usb_charge_full_detect_func(void *priv)
{
    static u16 charge_full_det_cnt = 0;
    u8 io_level = 0;

    if (chargebox_get_charge_en() == 0) {
        goto __detect_del;
    }

    if ((sys_info.status[USB_DET] == STATUS_OFFLINE) || sys_info.localfull) {
        goto __detect_del;
    }

#if TCFG_CHARGE_MOUDLE_OUTSIDE
    // io_level = gpio_read(TCFG_CHARGE_FULL_DET_IO);
    extern bool is_device_charging();
    io_level = is_device_charging();
#else
    io_level = CHARGE_FULL_FLAG_GET() && LVCMP_DET_GET();
#endif
    if (((CHARGE_FULL_LEVEL == LOW_LEVEL) && (!io_level)) || ((CHARGE_FULL_LEVEL == HIGHT_LEVEL) && io_level)) {
        charge_full_det_cnt++;
        if (charge_full_det_cnt > STAT_FULL_POWER_TIME) {
            sys_info.localfull = 1; ///注意清0动作在usb拔出动作那里
            app_chargebox_event_to_user(CHGBOX_EVENT_LOCAL_FULL);
            log_info("Usb charge is Full\n");
            goto __detect_del;
        }
    } else {
        charge_full_det_cnt = 0;
        goto __detect_del;
    }
    return;
__detect_del:
    usr_timer_del(usb_charge_full_det_timer);
    usb_charge_full_det_timer = 0;
    charge_full_det_cnt = 0;
#if (!TCFG_CHARGE_MOUDLE_OUTSIDE)
    if (sys_info.localfull == 0) {
        CHARGE_EDGE_DETECT_EN(1);
        CHARGE_LEVEL_DETECT_EN(1);
    }
#endif
}

void usb_charge_full_wakeup_deal(void)
{
    if (detect_init_ok && (!usb_charge_full_det_timer)) {
        usb_charge_full_det_timer = usr_timer_add(NULL, usb_charge_full_detect_func, DETECT_PERIOD, 1);
    }
}

static void usb_charge_full_detect_init(void)
{
    usb_charge_full_det_timer = usr_timer_add(NULL, usb_charge_full_detect_func, DETECT_PERIOD, 1);
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机充电电流检测
   @param    无
   @return   无
   @note     用于检测给耳机充电电流是否正常
*/
/*------------------------------------------------------------------------------------*/
#if TCFG_CURRENT_LIMIT_ENABLE
static int current_det_slow_timer, current_det_fast_timer;
static void ear_current_detect_func(void *priv)
{
    static u8 current_cnt = 0;
    static u16 ear_current = 0;
    if (sys_info.current_limit) {
        goto __ear_current_end;
    }
    current_cnt++;
    ear_current += adc_get_voltage(TCFG_CURRENT_DET_AD_CH) * 1000 / CHARGE_CURRENT_RES;
    if (current_cnt < 5) {
        return;
    }
    ear_current = ear_current / current_cnt;
    if (ear_current > ear_static_current) {
        ear_current = ear_current - ear_static_current;
    } else {
        ear_current = 0;
    }
    cur_ear_curr = ear_current;
    if (ear_current > CHARGE_CURRENT_LIMIT) {
        sys_info.current_limit = 1;
        app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_CURRENT_PROTECT);
        log_error("ear_current over limit: %d\n", ear_current);
    }
    log_info("ear_current: %d mA\n", ear_current);
__ear_current_end:
    current_cnt = 0;
    ear_current = 0;
    usr_timer_del(current_det_fast_timer);
    current_det_fast_timer = 0;
}

static void ear_current_detect_start(void *priv)
{
    if (sys_info.current_limit) {
        usr_timer_del(current_det_slow_timer);
        current_det_slow_timer = 0;
        return;
    }
    if (!current_det_fast_timer) {
        current_det_fast_timer = usr_timer_add(NULL, ear_current_detect_func, DETECT_PERIOD, 1);
    }
}

void ear_current_detect_enable(u8 en)
{
    cur_ear_curr = 0;
    if (en == 0) {
        if (current_det_slow_timer) {
            sys_timer_del(current_det_slow_timer);
            current_det_slow_timer = 0;
        }
        if (current_det_fast_timer) {
            usr_timer_del(current_det_fast_timer);
            current_det_fast_timer = 0;
        }
    } else {
        if (sys_info.current_limit) {
            return;
        }
        if (!current_det_slow_timer) {
            current_det_slow_timer = sys_timer_add(NULL, ear_current_detect_start, CURRENT_DETECT_PERIOD);
        }
        if (!current_det_fast_timer) {
            current_det_fast_timer = usr_timer_add(NULL, ear_current_detect_func, DETECT_PERIOD, 1);
        }
    }
}

#endif

/*------------------------------------------------------------------------------------*/
/**@brief    温度检测
   @param    无
   @return   无
   @note     用于检测电池温度是否正常
*/
/*------------------------------------------------------------------------------------*/
#if TCFG_TEMPERATURE_ENABLE
static int temperature_timer;
static void charge_temperatue_detect(void *priv)
{
    static u8 detect_cnt = 0;
    static u32 adc_totle = 0;
    u32 adc_avg, ntc_res;
    adc_totle += adc_get_value(TCFG_CHARGE_TEMP_AD_CH);
    detect_cnt++;
    if (detect_cnt >= CHARGE_TEMP_AVG_COUNTS) {
        adc_avg = adc_totle / detect_cnt;
        ntc_res = TCFG_RES_UP * adc_avg / (1024 - adc_avg);
        if (adc_avg > 0x3e0) {
            detect_cnt = 0;
            adc_totle = 0;
            usr_timer_del(temperature_timer);
            temperature_timer = 0;
            return;
        }
        log_info("ntc_ad: %x, ntc_res: %d\n", adc_avg,  ntc_res);
        if (sys_info.temperature_limit == 0) {
            if ((ntc_res > CHARGE_TEMP_ABNORMAL_LOW) || (ntc_res < CHARGE_TEMP_ABNORMAL_HIGH)) {
                sys_info.temperature_limit = 1;
                app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_TEMP_PROTECT);
            }
        } else {
            if ((ntc_res > CHARGE_TEMP_NORMAL_HIGH) && (ntc_res < CHARGE_TEMP_NORMAL_LOW)) {
                sys_info.temperature_limit = 0;
                app_chargebox_event_to_user(CHGBOX_EVENT_EXIT_TEMP_PROTECT);
            }
        }
        usr_timer_del(temperature_timer);
        temperature_timer = 0;
        detect_cnt = 0;
        adc_totle = 0;
    }
}

static void charge_temperatue_detect_start(void *priv)
{
    if (!temperature_timer) {
        temperature_timer = usr_timer_add(NULL, charge_temperatue_detect, DETECT_PERIOD, 1);//10ms
    }
}

static void charge_temperatue_detect_init(void)
{
    sys_timer_add(NULL, charge_temperatue_detect_start, CHARGE_TEMP_DETECT_PERIOD);
    temperature_timer = usr_timer_add(NULL, charge_temperatue_detect, DETECT_PERIOD, 1);//20ms
}
#endif

/*------------------------------------------------------------------------------------*/
/**@brief    获取当前仓电池电量
   @param    无
   @return   电量
   @note
*/
/*------------------------------------------------------------------------------------*/
u16 get_vbat_voltage(void)
{
    if (cur_bat_volt > 4200) {
        return 4200;
    } else {
        return cur_bat_volt;
    }
}


/*------------------------------------------------------------------------------------*/
/**@brief    获取当前给耳机充电的充电电流
   @param    无
   @return   电流mA
   @note
*/
/*------------------------------------------------------------------------------------*/
u16 get_ear_current(void)
{
    return cur_ear_curr;
}


/*------------------------------------------------------------------------------------*/
/**@brief    充电舱检测初始化
   @param    无
   @return   无
   @note     初始化电池电量检测、5v升压、升压检测、usb在线检测、满电检测
             的IO初始化。检测usb是否在线、电池的初始电量情况
*/
/*------------------------------------------------------------------------------------*/
void chargebox_det_init(void)
{
    log_info("func:%s line:%d", __func__, __LINE__);
    //IO init
    u16 status_init_cnt = 0;
    /* u16 shut_down_cnt = 0; */
    u16 i = 0;
    u8 io_level;
    u16 val;
    u32 volt_sum = 0;
    /* u32 ntc_sum = 0; */
    /* u32 curr_sum = 0; */
    /* u8 total_ad_ch = 0; */

    sys_info.localfull = 0;
    sys_info.temperature_limit = 0;
    sys_info.status[USB_DET] = STATUS_OFFLINE;
    sys_info.status[LID_DET] = STATUS_OFFLINE;

    //电池电量检测初始化
#if (TCFG_BAT_DET_AD_CH != AD_CH_PMU_VBAT)
    gpio_set_mode(IO_PORT_SPILT(TCFG_BAT_DET_IO), PORT_HIGHZ);
    adc_add_sample_ch(TCFG_BAT_DET_AD_CH);
    adc_set_sample_freq(AD_CH_LDOREF, 2);
#else
    adc_set_sample_period(AD_CH_PMU_VBAT, 2);
    adc_set_sample_period(AD_CH_LDOREF, 2);
#endif

    //耳机充电电流检测初始化
#if TCFG_CURRENT_LIMIT_ENABLE
    gpio_set_mode(IO_PORT_SPILT(TCFG_CURRENT_DET_IO), PORT_HIGHZ);
    adc_add_sample_ch(TCFG_CURRENT_DET_AD_CH);
#endif

    //温度检测初始化
#if TCFG_TEMPERATURE_ENABLE
    gpio_set_pull_down(TCFG_CHARGE_TEMP_IO, 0);
    gpio_set_pull_up(TCFG_CHARGE_TEMP_IO, !TCFG_CHARGE_EXTERN_UP_ENABLE);
    gpio_direction_input(TCFG_CHARGE_TEMP_IO);
    gpio_set_die(TCFG_CHARGE_TEMP_IO, 0);
    adc_add_sample_ch(TCFG_CHARGE_TEMP_AD_CH);
#endif

    //hall开合盖检测初始化
    // gpio_set_direction(TCFG_HALL_PORT, 1);
    // gpio_set_die(TCFG_HALL_PORT, 1);
    // gpio_set_pull_down(TCFG_HALL_PORT, 0);
    // gpio_set_pull_up(TCFG_HALL_PORT, 0);
    gpio_set_mode(IO_PORT_SPILT(TCFG_HALL_PORT), PORT_INPUT_FLOATING);

#if TCFG_LDO_DET_ENABLE
    //5v升压检测初始化
    gpio_set_mode(IO_PORT_SPILT(TCFG_CHG_LDO_DET_IO), PORT_HIGHZ);
    adc_add_sample_ch(TCFG_CHG_LDO_DET_AD_CH);
#endif

    //usb在线检测IO初始化
#if TCFG_CHARGE_MOUDLE_OUTSIDE
    // gpio_set_direction(TCFG_USB_ONLE_DET_IO, 1);
    // gpio_set_die(TCFG_USB_ONLE_DET_IO, 1);
    // gpio_set_pull_down(TCFG_USB_ONLE_DET_IO, 0);
    // gpio_set_pull_up(TCFG_USB_ONLE_DET_IO, 0);
    gpio_set_mode(IO_PORT_SPILT(TCFG_USB_ONLE_DET_IO), PORT_INPUT_FLOATING);
    //满电检测IO初始化
    // gpio_set_mode(IO_PORT_SPILT(TCFG_CHARGE_FULL_DET_IO), PORT_INPUT_PULLUP_10K);
#endif

    // total_ad_ch = get_cur_total_ad_ch();//ad总通道数
    sys_info.init_ok = 1;

    //检测hall是否开盖
    status_init_cnt = 0;
    for (i = 0; i < 20; i++) {
        io_level = gpio_read(TCFG_HALL_PORT);
        if (((HALL_DET_OPEN_LEVEL == LOW_LEVEL) && (!io_level)) || ((HALL_DET_OPEN_LEVEL == HIGHT_LEVEL) && io_level)) {
            sys_info.status[LID_DET] = STATUS_ONLINE;
            sbox_box_clid_status_set(LID_OPEN);
        } else {
            sys_info.status[LID_DET] = STATUS_OFFLINE;
            sbox_box_clid_status_set(LID_CLOSE);
            break;
        }
    }
    hall_det_wakeup_set_edge(io_level);

    //检测usb是否在线
    status_init_cnt = 0;
    for (i = 0; i < 20; i++) {
#if TCFG_CHARGE_MOUDLE_OUTSIDE
        // io_level = gpio_read(TCFG_USB_ONLE_DET_IO);
        // io_level = adc_get_voltage(ADC_CH_PMU_VPWR_4) * 4;
        io_level = (LDO5V_DET_GET() == 0) ? 0 : 1;
        extern void set_charge_online_flag(u8 flag);
        set_charge_online_flag(io_level);
#else
        io_level = get_lvcmp_det();
#endif
        if (((USB_DET_ONLINE_LEVEL == LOW_LEVEL) && (!io_level)) || ((USB_DET_ONLINE_LEVEL == HIGHT_LEVEL) && io_level)) {
            status_init_cnt++;
            sys_info.status[USB_DET] = STATUS_ONLINE;
            user_charge_insert_detect();
        } else {
            sys_info.status[USB_DET] = STATUS_OFFLINE;
            user_charge_remove_detect();
            break;
        }
    }
#if (TCFG_CHARGE_MOUDLE_OUTSIDE)
    // usb_det_wakeup_set_edge(io_level);
#else
    LVCMP_EDGE_SEL(io_level);
#endif

#if TCFG_CHARGE_MOUDLE_OUTSIDE
    //usb在线且默认充电开启时判断是否充满
    if ((sys_info.status[USB_DET] == STATUS_ONLINE) && chargebox_get_charge_en()) {
        status_init_cnt = 0;
        for (i = 0; i < 20; i++) {
            // io_level = gpio_read(TCFG_CHARGE_FULL_DET_IO);
            extern bool is_device_charging();
            io_level = is_device_charging();
            if (((CHARGE_FULL_LEVEL == LOW_LEVEL) && (!io_level)) || ((CHARGE_FULL_LEVEL == HIGHT_LEVEL) && io_level)) {
                status_init_cnt++;
                sys_info.localfull = 1;
            } else {
                sys_info.localfull = 0;
                break;
            }
        }
    }
#endif

    //检测是否处于低电量状态
    for (i = 0; i < 5; i++) {
        clr_wdt();
        // delay_2ms(2);//等待所有通道采完
        val = adc_get_voltage(TCFG_BAT_DET_AD_CH) * 4; //注意电路分压
        volt_sum += val;
        log_info("Volt:%d", val);

#if TCFG_TEMPERATURE_ENABLE
        val = adc_get_value(TCFG_CHARGE_TEMP_AD_CH);
        ntc_sum += val;
#endif

#if TCFG_CURRENT_LIMIT_ENABLE
        val = adc_get_value(TCFG_CURRENT_DET_AD_CH) * 1000 / CHARGE_CURRENT_RES;
        curr_sum += val;
#endif
    }

    cur_bat_volt = volt_sum / 5; //电池电压初始值
    if (cur_bat_volt < LOWPOWER_ENTER_VOLTAGE) {//是否小于低电电压
        sys_info.lowpower_flag = 1;
        //判断是否小于关机电压,直接关机
        if ((cur_bat_volt < POWER_ON_SHUTDOWN_VOLT) && (sys_info.status[USB_DET] == STATUS_OFFLINE)) {
            if (sys_info.wireless_wakeup == 0) {
                sys_info.init_ok = 0;
            }
        }
    }

#if TCFG_CURRENT_LIMIT_ENABLE
    ear_static_current = curr_sum / 5;
    log_info("ear_static_current: %d mA\n", ear_static_current);
#endif

#if TCFG_TEMPERATURE_ENABLE
    u32 ntc_res, adc_avg;
    adc_avg = ntc_sum / 5;
    if (adc_avg < 0x3e0) {
        ntc_res = TCFG_RES_UP * adc_avg / (1024 - adc_avg);
        if ((ntc_res < CHARGE_TEMP_ABNORMAL_HIGH) || (ntc_res > CHARGE_TEMP_ABNORMAL_LOW)) {
            sys_info.temperature_limit = 1;
            if (chargebox_get_charge_en()) {
                chargebox_charge_close();
            }
        }
    } else {
        log_info("mabe NTC not connect!\n");
    }
#endif

    power_detect_init();
    usb_detect_init();
    hall_detect_init();
    usb_charge_full_detect_init();
#if TCFG_LDO_DET_ENABLE
    ldo_detect_init();
#endif
#if TCFG_TEMPERATURE_ENABLE
    charge_temperatue_detect_init();
#endif

    log_info("usb status: %d\n", sys_info.status[USB_DET]);
    log_info("lid status: %d\n", sys_info.status[LID_DET]);
    log_info("ldo status: %d\n", sys_info.status[LDO_DET]);
    log_info("localfull : %d\n", sys_info.localfull);
    log_info("lowpower_flag: %d\n", sys_info.lowpower_flag);
    log_info("sys_info.init_ok: %d\n", sys_info.init_ok);
    log_info("power_on volt: %d\n", cur_bat_volt);
    log_info("temperature status: %d\n", sys_info.temperature_limit);
    detect_init_ok = 1;
}

#if(defined CUSTOM_BATTERY_CURVE_MANAGEMENT && CUSTOM_BATTERY_CURVE_MANAGEMENT)
u16 earphone_has_shutdown = 0;

void hall_simple_detect(void *p)
{
    static u16 hall_open_det_cnt = 0;
    static u16 hall_close_det_cnt = 0;
    u8 io_level = gpio_read(TCFG_HALL_PORT);
    log_info("hall io_level:%d\n", io_level);
    sys_info.life_cnt = 0;

    if (((HALL_DET_OPEN_LEVEL == LOW_LEVEL) && (!io_level)) || ((HALL_DET_OPEN_LEVEL == HIGHT_LEVEL) && io_level)) {
        hall_open_det_cnt++;
        if (hall_open_det_cnt >= HALL_DET_OPEN_TIME) {
            //确认是开盖了
            // usr_timer_del(hall_simple_detect_det_timer);
            // hall_simple_detect_det_timer = 0;
        } else {
            hall_close_det_cnt++;
            if (hall_close_det_cnt >= HALL_DET_CLOSE_TIME) {
                //确认是合盖了
                // usr_timer_del(hall_simple_detect_det_timer);
                // hall_simple_detect_det_timer = 0;
                //检测耳机是否在舱
                // while (earphone_exist_poweron()) {
                //     send_poweroff_cmd_to_earphone(which_earphone_is_inbox());
                // }
                //给在舱耳机发送关机指令
                extern u8 app_chargebox_api_send_shutdown(void);
                app_chargebox_api_send_shutdown();
                earphone_has_shutdown = 1;
            }
        }
    }
}

// void hall_simple_detect_timer_create() {
//     if (!hall_simple_detect_det_timer) {
//         hall_simple_detect_det_timer = usr_timer_add(NULL, hall_simple_detect, DETECT_PERIOD, 1);
//     }
// }
// 定义一个函数，用于在耳机控制器电源关闭时检测电池容量
void earphone_controller_in_poweroff_battery_capacity()
{
    int i = HALL_DET_OPEN_TIME;
    while (!earphone_has_shutdown) {
        hall_simple_detect(NULL);
        i--;
        os_time_dly(DETECT_PERIOD * 2);
        wdt_clear();
    }

    // hall_simple_detect_timer_create();
}

#endif
#else

void earphone_controller_in_poweroff_battery_capacity()
{

}
#endif
