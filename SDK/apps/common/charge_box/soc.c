#include "soc.h"
#include "system/includes.h"
#include "app_power_manage.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "asm/charge.h"
#include "ui_manage.h"
#include "tone_player.h"
#include "btstack/avctp_user.h"
#include "user_cfg.h"
#include "bt.h"
#include "asm/charge.h"
#include "ui/ui_api.h"
#include "../../include/key_event_deal.h"
#include "smartbox_user_app.h"
#include "gpadc.h"
#include "smartbox_info_manager.h"


#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[SOC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#define CHARGE_QUXIAN_TEST          0
#define LOW_POWER_READREAD          0//比记忆低重新校准
#define DISP_TEST_SOC_UI_FIRST      0//校准显示UI

#define SOC_SIMULATE    0//仿真测试
#if (defined CUSTOM_BATTERY_CURVE_MANAGEMENT) && CUSTOM_BATTERY_CURVE_MANAGEMENT


//  低电参数
#define LOW_POW_STORAGE_MODE    0//1   //  低电是否直接进仓储模式
#define LOWPOWER_V              3.550           //低电提示电压
#define POWEROFF_V              3.500           //低电关机电压
#define POWEROFF_V_OFFSET       0.1             //开机滞回电压差，避免低电反复开机
// #define POWEROFF_SOC            0.01            //低电百分比关机
#define POWERSHUTDOWN_V         3.400           //严重低电关机电压
#define LOWPOWER_CNT            3               //关机计数3*8S关机


//  充电曲线
#define CHARGE_PA_END_V         4.1//0.1//4.300           //恒流结束电压（估计）
#define CHARGE_PA_START_V       (LOWPOWER_V+0.1)//3.5//0.0//3.500           //恒流开始电压（估计）
#define CHARGE_PA_END_SOC       0.8//0.0//0.8             //恒流结束百分比（估计）
#define CHARGE_PA_START_SOC     0.0//0.0             //恒流开始百分比（估计）
#define CHARGE_PV_TIME          (1*25*60*1000)//(1.0*60*60*1000)    //恒压充电时间（估计）

#if (CHARGE_PV_TIME  == 0)
#define CHARGE_PA_START_A       250 //  恒压充电开始电流
#define CHARGE_PA_END_A         20  //50  //  恒压充电满电电流
#endif

//  滤波参数
#define CHR_SPEED               0.003             //充电电量追赶速度
#define DIS_SPEED               0.001             //放电电量追赶速度
#define DIS_K                   0.2             //放电滤波系数，越大越快
#define CHANGE_FIL_TIME         10000           //切换过滤不靠谱时间
#define SOC_CICLE_TIME          8000            //电量循环读取时间


// #define ERROR_SOC               1.0//0.3             //放电差异太大需要重新读
// #define ZERO_RE_READ            0//1            //放电状态下 电量0 % 是否重新读，关闭比较保险
// #define CHARG_ZERO_RE_READ      0//1//0//             //第一次充电是否重读电量,982给过来的信息概率不准不建议开启
//

// 校准
// #define charge_first_work_charge_out_trim_vbat   1//0//1
// #define NO_CHARGE_CNT       2//5//5次采样后才认为稳定
// #define     FIRST_START_INFO            /看VM序号实际配置
// #define     ALL_USR_CFG            34//看VM序号实际配置

// 放电曲线
// #define NEW_NUM_POINTS          3//11                   // 实际应用的电池曲线（放电）
static float cap_ocv[][2] = {
    {1.00, 4.1},//100%  //建议满电稍低一点，避免出现满电拔掉充电立即掉电的情况    {0.10, LOWPOWER_V},//10%
    {0.00, POWEROFF_V},//0%
};

void earphone_controller_in_poweroff_battery_capacity();

#define NEW_NUM_POINTS (ARRAY_SIZE(cap_ocv))
#if (CHARGE_PV_TIME)
int get_vbat_a_from_f98 = 0;
#else
extern int get_vbat_a_from_f98;
#endif

#if CHARGE_QUXIAN_TEST
static float vbat_val_test = 4.200;

static void change_vbat_val_test(void)
{
    vbat_val_test -= 0.001;
    r_printf("%s, test val:%f\n", __func__, vbat_val_test);
}
#endif  /* CHARGE_QUXIAN_TEST */


static void trig_vcb(void (*func)(void))
{
    log_info("os:%s func:%p", os_current_task(), func);
    func();
}

int send_to_trig_vcb(void (*func)(void), char *taskname)
{
    if (0) {
        return 0;
    }
    log_info("send_to_trig_ui_vcb:%p", func);
    int argv[3];

    argv[0] = (int)trig_vcb;
    argv[1] = 1;
    argv[2] = (int)func;

    int ret = os_taskq_post_type(taskname, Q_CALLBACK, 3, argv);
    if (ret) {
        printf("post ret:%d \n", ret);
    }
    return 0;
}
int now_soc_trim_info;

//  获取上电信息
u8 get_first_start_info()
{
    int reset_sour = 0;
    int vmret = syscfg_read(FIRST_START_INFO, &reset_sour, sizeof(reset_sour));
    if (vmret == sizeof(reset_sour)) {
        g_printf("write_first_start_info 读VM成功: %d", reset_sour);
    }
    now_soc_trim_info = reset_sour;
    return reset_sour;
}

static u8 get_chr_cap(float ocv, float *soc)
{
    if (ocv < CHARGE_PA_START_V) {
        printf("%s %d", __func__, __LINE__);
        *soc = CHARGE_PA_START_SOC;
    } else if (ocv >= CHARGE_PA_END_V) {
#if(CHARGE_PV_TIME)
        *soc += (float)SOC_CICLE_TIME / (float)CHARGE_PV_TIME * (1.0 - CHARGE_PA_END_SOC);
        if (*soc > 1.0) {
            *soc = 1.0;
        }
        printf("%s %d", __func__, __LINE__);
        return 1;
#else
        *soc = CHARGE_PA_END_SOC + (CHARGE_PA_START_A - get_vbat_a_from_f98) * (1.0 - CHARGE_PA_END_SOC) / (CHARGE_PA_START_A - CHARGE_PA_END_A);
        printf("%s %d", __func__, __LINE__);
        return 0;
#endif
    } else {
        printf("%s %d", __func__, __LINE__);
        *soc = CHARGE_PA_START_SOC + (CHARGE_PA_END_SOC - CHARGE_PA_START_SOC) * (ocv - CHARGE_PA_START_V) / (CHARGE_PA_END_V - CHARGE_PA_START_V);
    }
    printf("%s %d", __func__, __LINE__);
    return 0 ;
}

static float get_ocv(float cap)
{
    if (cap >= cap_ocv[0][0]) {
        return cap_ocv[0][1];
    } else if (cap <= cap_ocv[NEW_NUM_POINTS - 1][0]) {
        return cap_ocv[NEW_NUM_POINTS - 1][1];
    } else {
        for (int i = 0; i < NEW_NUM_POINTS - 1; i++) {
            if (cap <= cap_ocv[i][0] && cap >= cap_ocv[i + 1][0]) {
                float ocv = cap_ocv[i][1] + (cap - cap_ocv[i][0]) * (cap_ocv[i + 1][1] - cap_ocv[i][1]) / (cap_ocv[i + 1][0] - cap_ocv[i][0]);
                return ocv;
            }
        }
    }
    return 0;
}

static float get_cap(float ocv)
{

    printf("%f %f %f %f", cap_ocv[0][1], cap_ocv[0][0], cap_ocv[NEW_NUM_POINTS - 1][1], cap_ocv[NEW_NUM_POINTS - 1][0]);
    if (ocv >= cap_ocv[0][1]) {
        return cap_ocv[0][0];
    } else if (ocv <= cap_ocv[NEW_NUM_POINTS - 1][1]) {
        return cap_ocv[NEW_NUM_POINTS - 1][0];
    } else {
        for (int i = 0; i < NEW_NUM_POINTS - 1; i++) {
            if (ocv <= cap_ocv[i][1] && ocv >= cap_ocv[i + 1][1]) {
                float cap = cap_ocv[i][0] + (ocv - cap_ocv[i][1]) * (cap_ocv[i + 1][0] - cap_ocv[i][0]) / (cap_ocv[i + 1][1] - cap_ocv[i][1]);
                return cap;
            }
        }
    }
    return 0;
}




static int vm_vbat_value = 0;
static void write_vm_vbat_value(int vm_value)
{
    if (vm_value < -1) {
        vm_value = -1;
    } else if (vm_value > 100) {
        vm_value = -1;
    }
    printf("%s %d", __func__, vm_value);
    syscfg_write(VM_BAT_PRESENT, &vm_value, sizeof(vm_value));
}

static int read_vm_vbat_value()
{
    int vm_value = -1;
    int ret = syscfg_read(VM_BAT_PRESENT, &vm_value, sizeof(vm_value));
    if (ret > 0) {
        if (vm_value < 0) {
            vm_value = -1;
        } else if (vm_value > 100) {
            vm_value = -1;
        }

        printf(">>>>>>>>> VM init val is %d", vm_vbat_value);
    } else {
        vm_value = -1;
    }

    // vm_vbat_value = vm_value;
    return vm_value;
}

u8 check_bat_vm_is_vaild()
{
    if (-1 == read_vm_vbat_value()) {
        return 0;
    } else {
        return 1;
    }
}


static float disp_soc;
static void soc_calculate_loop(void *p);
static void low_enter_poweroff();
static void force_poweroff(void *p);
// #include "lvgl.h"

int get_curr_soc()
{
    return sbox_battery_box_get();
}


static u8 reread = 0;


static int soc_trim_id = 0;

static int app_is_sleep = 0;
static u16 app_sleep_check_timer = 0;
void app_sleep_check()
{
    int value = 1;
    /* struct lp_target *p; */
    // list_for_each_lp_target(p){
    //     value = 1 & (p->is_idle());
    //     putchar('*');
    // }
    int lcd_sleep_flag = true;

#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
    lcd_sleep_flag = lcd_sleep_status();
#endif

    if (value && lcd_sleep_flag && (sbox_ble_connect_flag_get() == 0) && !sbox_box_charging_get()) {
        ++app_is_sleep;
        putchar('o');
        if (app_is_sleep > TCFG_AUTO_POWER_OFF_TIME) {
            putchar('P');
            // app_chargebox_api_shutdown();
            // power_event_to_user(POWER_EVENT_POWER_LOW);
            // set_sys_soft_shutdown_flag(1);
            // app_chargebox_api_shutdown();
            sys_timer_del(app_sleep_check_timer);
            // soft_poweroff_mode(1);  ///强制关机
            sys_enter_soft_poweroff(POWEROFF_NORMAL);
        }
    } else {
        // putchar('@');
        app_is_sleep = 0;
    }
}

void soc_check_init()
{
    r_printf("soc_check_init reset ok");
#if CHARGE_QUXIAN_TEST
    sys_timer_add(NULL, change_vbat_val_test, 2000);
#endif  /* CHARGE_QUXIAN_TEST */
    get_first_start_info();
    //  读取各个电量信息
#if CHARGE_QUXIAN_TEST
    float current_707_vbat = vbat_val_test;//(float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4)/1000.0;
    r_printf("current_707_vbat init:%f\n", (float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4) / 1000.0);
#else
    float current_707_vbat = (float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4) / 1000.0;
#endif  /* CHARGE_QUXIAN_TEST */
    float current_982_vbat = current_707_vbat;//(float)(adc_get_value_f98())/1000.0;
    u8 charging = (sbox_box_charging_get() /*|| (nocharge_cnt<NO_CHARGE_CNT)*/);

    //  读取VM的电量
    vm_vbat_value = read_vm_vbat_value();
    r_printf("init vm_vbat_value %d", vm_vbat_value);

    //  无效电量，看时机校准
    if (vm_vbat_value == -1) {
        r_printf("init vm_vbat_value invail");
    } else {
        //  初始电量
        disp_soc = (float)vm_vbat_value / 100.0;
        r_printf("init disp_soc:%f", disp_soc);

        r_printf("arry : %d", NEW_NUM_POINTS);
#if LOW_POWER_READREAD


        //  非充电状态，比记忆低，重新读取
        if (!charging && (get_cap(current_707_vbat) < disp_soc - 0.1)) {
            g_printf(">>>>no charge vbat low low :%f, %f", disp_soc, get_cap(current_707_vbat));
            disp_soc = get_cap(current_707_vbat);
        }
#endif
        //  范围控制
        disp_soc = SOC_CLAMP(-1.0, disp_soc, 101.0);
        vm_vbat_value = (int)(disp_soc * 100.0);
        if (vm_vbat_value < 1) {
            vm_vbat_value += 1;
        }
        vm_vbat_value = SOC_CLAMP(0, vm_vbat_value, 100);
        sbox_battery_box_set(vm_vbat_value);
        write_vm_vbat_value(sbox_battery_box_get());
    }

    g_printf(">>>>set_soc_first_work c %d init %d mapsoc:%f 701:%f 982:%f", charging, sbox_battery_box_get(), disp_soc, current_707_vbat, current_982_vbat);

#if SOC_SIMULATE
    if (!soc_trim_id) {
        soc_trim_id = sys_timer_add(NULL, soc_calculate_loop, 10);
    }
#else
    if (!soc_trim_id) {
        soc_trim_id = sys_timer_add(NULL, soc_calculate_loop, (vm_vbat_value == -1) ? 500 : SOC_CICLE_TIME);
    }
#endif

    if (app_sleep_check_timer == 0) {
        app_sleep_check_timer = sys_timer_add(NULL, app_sleep_check, 60000);//检查是否进入低功耗
    }

}

float current_701_vbat;
float current_982_vbat;
float map_soc;
static int last_change_time = 0;

static void soc_calculate_loop(void *p)
{
    r_printf("Entering soc_calculate_loop\n");

    // 读取各个电量信息
#if CHARGE_QUXIAN_TEST
    current_701_vbat = vbat_val_test;
    r_printf("Testing mode: current_701_vbat: %f\n", (float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4) / 1000.0);
#else
    current_701_vbat = (float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4) / 1000.0;
    r_printf("Normal mode: current_701_vbat: %f\n", current_701_vbat);
#endif  /* CHARGE_QUXIAN_TEST */

    current_982_vbat = current_701_vbat;
    u8 charging = sbox_box_charging_get();

    r_printf("Charging status: %d\n", charging);

    if (current_701_vbat < 2.0) {
        r_printf("Error: 701 VBAT below threshold\n");
        return;
    }

    if (charging && current_982_vbat < 2.0) {
        r_printf("Error: 982 VBAT below threshold while charging\n");
        return;
    }

    // 严重低电
    if (!charging && (current_701_vbat < POWERSHUTDOWN_V)) {
        earphone_controller_in_poweroff_battery_capacity();
        g_printf("Critical low battery: %f\n", current_701_vbat);
        vm_vbat_value = 0;
        sbox_battery_box_set(vm_vbat_value);
        write_vm_vbat_value(vm_vbat_value);
        force_poweroff(NULL);
        return;
    }

    // 记录变化时间点
    static s8 state = -1;
    if (state != charging || last_change_time == 0) {
        last_change_time = jiffies_msec();
        state = charging;
        r_printf("State change detected: charging = %d, time = %d\n", charging, last_change_time);
    }

#if DISP_TEST_SOC_UI_FIRST
    if (vm_vbat_value == -1) {
        static u8 firstsoc_disp = 1;
        if (firstsoc_disp) {
            firstsoc_disp = 0;
            send_to_trig_vcb(soc_disp, "ui");
            r_printf("First SOC display triggered\n");
        }
    }
#endif

    // 过滤不靠谱时间
    r_printf("Current time: %d, Last change time: %d\n", jiffies_msec(), last_change_time);
    if ((u32)jiffies_msec2offset(last_change_time, jiffies_msec()) < CHANGE_FIL_TIME) {
        r_printf("Time filter active, skipping calculation\n");
        return;
    }
    r_printf("Passed time filter\n");

    // 无效电量，需要重新校准
    if (vm_vbat_value == -1) {
        r_printf("Invalid battery value detected, starting recalibration\n");

        if (!charging) {
            r_printf("Device is not charging, proceeding with recalibration\n");

            // 修改定时器
            sys_timer_modify(soc_trim_id, SOC_CICLE_TIME);
            r_printf("Timer modified with SOC_CICLE_TIME: %d\n", SOC_CICLE_TIME);

            // 获取当前电量
            disp_soc = get_cap(current_701_vbat);
            r_printf("Calculated SOC from voltage: %f\n", disp_soc);

            // 限制 SOC 范围
            disp_soc = SOC_CLAMP(-1.0, disp_soc, 101.0);
            r_printf("Clamped SOC: %f\n", disp_soc);

            // 转换为整数
            vm_vbat_value = (int)(disp_soc * 100.0);
            r_printf("Converted SOC to integer value: %d\n", vm_vbat_value);

            // 确保电量值不低于 1
            if (vm_vbat_value < 1) {
                vm_vbat_value += 1;
                r_printf("Adjusted SOC to minimum threshold: %d\n", vm_vbat_value);
            }

            // 再次限制范围
            vm_vbat_value = SOC_CLAMP(0, vm_vbat_value, 100);
            r_printf("Final clamped SOC value: %d\n", vm_vbat_value);

            // 更新电池信息
            sbox_battery_box_set(vm_vbat_value);
            write_vm_vbat_value(vm_vbat_value);
            r_printf("Recalibrated SOC saved: %d\n", vm_vbat_value);
        } else {
            r_printf("Device is charging, skipping recalibration\n");
        }

        return;
    }


    // 低电关机
    static int low_cnt = 0;
    if (!charging && (current_701_vbat < POWEROFF_V)) {
        g_printf("Low battery detected: %f\n", current_701_vbat);
        low_cnt++;
    }
    if (charging) {
        low_cnt = 0;
    }
    if (low_cnt > LOWPOWER_CNT) {
        g_printf("Low power shutdown triggered\n");
        vm_vbat_value = 0;
        sbox_battery_box_set(vm_vbat_value);
        write_vm_vbat_value(vm_vbat_value);
        low_enter_poweroff();
        return;
    }

    // 计算电量
    r_printf("Calculating SOC\n");
    if (charging) {
        map_soc = disp_soc;
        u8 ret = get_chr_cap(current_982_vbat, &map_soc);
        if (ret) {
            disp_soc = map_soc;
            r_printf("Charging: SOC updated to %f\n", disp_soc);
        }
    } else {
        map_soc = get_cap(current_701_vbat);
        r_printf("Discharging: Calculated SOC = %f\n", map_soc);
    }

    // 追赶
    r_printf("Adjusting SOC\n");
    if (map_soc > disp_soc && charging) {
        if (disp_soc < 0.01) {
            disp_soc += 0.02;
        }
        disp_soc += CHR_SPEED;
        r_printf("Charging: SOC increased to %f\n", disp_soc);
    }
    if (map_soc < disp_soc && !charging) {
        disp_soc -= (disp_soc - map_soc) < DIS_SPEED ? DIS_SPEED : (disp_soc - map_soc) * DIS_K;
        r_printf("Discharging: SOC decreased to %f\n", disp_soc);
    }

    // 转化整形
    r_printf("Clamping SOC\n");
    disp_soc = SOC_CLAMP(-1.0, disp_soc, 101.0);
    vm_vbat_value = (int)(disp_soc * 100.0);
    if (vm_vbat_value < 1) {
        vm_vbat_value += 1;
    }
    vm_vbat_value = SOC_CLAMP(0, vm_vbat_value, 100);
    sbox_battery_box_set(vm_vbat_value);

    // 电量变化保存
    static u8 lastboxbat = -3;
    if (lastboxbat != sbox_battery_box_get()) {
        lastboxbat = sbox_battery_box_get();
        write_vm_vbat_value(lastboxbat);
        UI_MSG_POST("batcharge:process=%4",  lastboxbat);
        r_printf("Battery value saved: %d\n", lastboxbat);
    }

    r_printf("Charge: %d, SOC: %.2f, 701: %.2f, Map: %.2f, 982: %.2f\n",
             charging, disp_soc * 100.0, current_701_vbat, map_soc, current_982_vbat);

    r_printf("Exiting soc_calculate_loop\n");
}




//  开机前电压判断
void usr_check_power_on_voltage(void)
{
    float current_vbat;
    current_vbat = (float)(adc_get_voltage(AD_CH_PMU_VBAT) * 4) / 1000.0;
    g_printf("usr_check_power_on_voltage current_vbat %f ", current_vbat);
    if (current_vbat <= POWEROFF_V + POWEROFF_V_OFFSET) {
        extern void sbox_storage_mode_enter();
        sbox_storage_mode_enter();
    }
}

static void force_poweroff(void *p)
{
    g_printf("%s %d", __func__, __LINE__);
    sbox_battery_box_set(0);
    write_vm_vbat_value(0);

#if LOW_POW_STORAGE_MODE
    extern void sbox_storage_mode_enter();
    sbox_storage_mode_enter();
#else
    // soft_poweroff_mode(1);  ///强制关机
    sys_enter_soft_poweroff(0);
#endif
}

static void low_enter_poweroff()
{
    g_printf("%s %d", __func__, __LINE__);
    sbox_battery_box_set(0);
    write_vm_vbat_value(0);
    // ui_poweroff_enter();
    sys_timer_add(NULL, force_poweroff, 5000);
}

//  升级代码、上电复位、清
void write_first_start_info(int reset_sour)
{

    y_printf("write_first_start_info : %d", reset_sour);

    // if(reset_sour == 200){
    //     //  设置成无效状态
    //     // box_info.box_bat = vm_vbat_value = -1;
    //     // write_vm_vbat_value(box_info.box_bat);
    //     // sys_timer_modify(soc_trim_id, 100);
    // }

    int ret = 0;
    int retry = 10;
    do {
        ret = syscfg_write(FIRST_START_INFO, &reset_sour, sizeof(reset_sour));
        y_printf("retry:%d", retry);
        os_time_dly(1);
    } while (ret <= 0 && retry);
    now_soc_trim_info = reset_sour;
}

#endif //#if (defined CUSTOM_BATTERY_CURVE_MANAGEMENT) && CUSTOM_BATTERY_CURVE_MANAGEMENT

