#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_main.data.bss")
#pragma data_seg(".app_main.data")
#pragma const_seg(".app_main.text.const")
#pragma code_seg(".app_main.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "gpadc.h"
#include "app_tone.h"
#include "gpio_config.h"
#include "app_main.h"
#include "asm/charge.h"
#include "update.h"
#include "app_power_manage.h"
#include "audio_config.h"
#include "app_charge.h"
#include "bt_profile_cfg.h"
#include "update_loader_download.h"
#include "idle.h"
#include "bt_tws.h"
#include "key_driver.h"
#include "user_cfg.h"
#include "app_default_msg_handler.h"
#include "app_music.h"
#include "fm.h"
#include "pc.h"
#include "linein.h"
#include "linein_dev.h"
#include "bt.h"
#include "rtc.h"
#include "record.h"
#include "usb/usb_task.h"
#include "usb/device/usb_stack.h"
#include "alarm.h"
#include "spdif.h"
#include "power_on.h"
#include "key/adkey.h"
#include "key/iokey.h"
#include "driver/trim.h"
#include "dev_manager.h"
#include "app_mode_update.h"
#include "app_version.h"
#include "sdfile.h"
#include "tp_api.h"
#include "data_storage.h"
#include "health_manager.h"
#include "clock_manager/clock_manager.h"
#include "product_test.h"
#include "chgbox_ctrl.h"
#include "mix_record_api.h"
#include "rcsp_mix_record.h"
#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/lcd/lcd_drive.h"
#endif
#include "app_video.h"
#include "video_photo.h"
#include "video_dec.h"

#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
#include "video_api.h"
#endif

#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

extern const u8 sfc0_dtr_mode_en;
extern u8 check_vbat_low_power(void);
extern struct app_mode *app_enter_photo_mode(int arg);
extern struct app_mode *app_enter_video_dec_mode(int arg);
extern struct app_mode *app_enter_video_rec_mode(int arg);
#define TASK_PRIO_BASE  20

/*任务列表 */
const struct task_info task_info_table[] = {
    {"write_sd0",            0 + TASK_PRIO_BASE,      0,  2048,  1024  },
    {"save_sd_card",            0 + TASK_PRIO_BASE,      0,  2048,  1024  },
    {"rep_recv_0",            0 + TASK_PRIO_BASE,      0,  2048,  1024  },
    {"dynamic_huffman",            0 + TASK_PRIO_BASE,      0,  2048,  1024  },
#if TCFG_PAY_ALIOS_ENABLE
    {"app_core",            0 + TASK_PRIO_BASE,      0,  2048,  1024  },
#else
    {"app_core",            0 + TASK_PRIO_BASE,      0,  768,  768},
#endif
#if TCFG_PAY_TRANSITCODE_ENABLE
    //乘车码应用使用完要kill 任务 对栈占用厉害
    {"transitcode",         1,      0,  2048 + 2048 + 2048 + 1024,  512 },
#endif
    {"btctrler",            4 + TASK_PRIO_BASE,     0,   512,   512 },
    {"btencry",             1 + TASK_PRIO_BASE,     0,   512,   128 },
#if (BT_FOR_APP_EN)
    {"btstack",             3 + TASK_PRIO_BASE,     0,   1024,  256 },
#else
    {"btstack",             3 + TASK_PRIO_BASE,     0,   768,   256 },
#endif
    {"jlstream",            3 + TASK_PRIO_BASE,     0,  768,   128 },
    {"jlstream_0",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_1",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_2",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_3",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_4",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_5",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_6",          6 + TASK_PRIO_BASE,     0,  768,   0 },
    {"jlstream_7",          6 + TASK_PRIO_BASE,     0,  768,   0 },

#if (TCFG_BT_SUPPORT_LHDC)
    {"a2dp_dec",            7 + TASK_PRIO_BASE,     0,  512 + 256,   0 },
#else
    {"a2dp_dec",            7 + TASK_PRIO_BASE,     0,  512,   0 },
#endif
    {"file_dec",            5 + TASK_PRIO_BASE,     0,  640,   0 },
    {"file_cache",          5 + TASK_PRIO_BASE,     0,  512,   0 },
    {"write_file",		    5 + TASK_PRIO_BASE,	   0,  512,   0 },


    /* 混响任务优先级要高 */
    {"mic_effect1",         7 + TASK_PRIO_BASE,     0,  768,   0 },
    {"mic_effect2",         7 + TASK_PRIO_BASE,     0,  768,   0 },

    /*
     *为了防止dac buf太大，通话一开始一直解码，
     *导致编码输入数据需要很大的缓存，这里提高编码的优先级
     */
    {"audio_enc",           7 + TASK_PRIO_BASE,     0,   768,   128 },
    {"aec",					2 + TASK_PRIO_BASE,	   1,   768,   128 },

    {"aec_dbg",				3 + TASK_PRIO_BASE,	   0,   512,   128 },
    {"update",				1 + TASK_PRIO_BASE,	   0,   512,   0   },
    {"tws_ota",				2 + TASK_PRIO_BASE,	   0,   256,   0   },
    {"tws_ota_msg",			2 + TASK_PRIO_BASE,	   0,   256,   128 },
    {"dw_update",		 	2 + TASK_PRIO_BASE,	   0,   512,   128 },
    {"aud_capture",         4 + TASK_PRIO_BASE,     0,   512,   256 },
    {"data_export",         5 + TASK_PRIO_BASE,     0,   512,   256 },
    {"anc",                 3 + TASK_PRIO_BASE,     0,   512,   128 },
    {"pmu_task",            6 + TASK_PRIO_BASE,      0,  512,   512 },
    {"dac",                 2 + TASK_PRIO_BASE,     0,   256,   128 },

#if RCSP_MODE
    {"rcsp",		    	4 + TASK_PRIO_BASE,	   0,   768,   128 },
#if RCSP_FILE_OPT
    {"rcsp_file_bs",		    	1 + TASK_PRIO_BASE,	   0,   768,   128 },
#endif
#endif
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    {"kws",                 2 + TASK_PRIO_BASE,     0,   256,   64  },
#endif
    {"usb_stack",          	1 + TASK_PRIO_BASE,     0,   512,   128 },
    {"usb_test",          	1 + TASK_PRIO_BASE,     0,   512,   128 },
#if (BT_AI_SEL_PROTOCOL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN))
    {"app_proto",           2 + TASK_PRIO_BASE,     0,   768,   64  },
#endif
#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
    {"ui",                  3 + TASK_PRIO_BASE,     0,   1024 * 8,  512  },
    {"gpu",                 8 + TASK_PRIO_BASE,     0,   1408,  512  },
    {"lcd_init",           	4 + TASK_PRIO_BASE,     0,	256,    0  },
    {"tp_init",           	5 + TASK_PRIO_BASE,     0,  256,    0  },
#elif (defined CONFIG_JL_UI_ENABLE && CONFIG_JL_UI_ENABLE)
    {"ui",                  3 + TASK_PRIO_BASE,     0,   1408,  512  },
    {"gpu",                 8 + TASK_PRIO_BASE,     0,   1408,  512  },
    {"lcd_init",           	4 + TASK_PRIO_BASE,     0,	256,    0  },
    {"tp_init",           	5 + TASK_PRIO_BASE,     0,  256,    0  },
    {"lcd",             	8 + TASK_PRIO_BASE,     0,  256,    64 },
#endif
#if (TCFG_DEV_MANAGER_ENABLE)
    {"dev_mg",           	3 + TASK_PRIO_BASE,     0,   256,   32 },
#endif
    {"audio_vad",           1 + TASK_PRIO_BASE,     1,   512,   128 },
#if TCFG_KEY_TONE_EN
    {"key_tone",            5 + TASK_PRIO_BASE,     0,   256,   32  },
#endif
#if (BT_AI_SEL_PROTOCOL & TUYA_DEMO_EN)
    {"user_deal",           7 + TASK_PRIO_BASE,     0,   512,   512 }, //定义线程 tuya任务调度
    {"dw_update",           2 + TASK_PRIO_BASE,     0,   256,   128 },
#endif
#if(TCFG_UPDATE_UART_IO_EN)
    {"uart_update",	        1 + TASK_PRIO_BASE,	   0,   512,   128	},
#endif
#if (USER_FILE_UPDATE_V2_EN)
    {"ex_f_update",			1 + TASK_PRIO_BASE,	    1,  512,   0    },
#endif
    {"periph_demo",         3 + TASK_PRIO_BASE,     0,   512,   0 },
    {"CVP_RefTask",	        4 + TASK_PRIO_BASE,	   0,   256,   128	},
    {"touch_task",			9 + TASK_PRIO_BASE,     0,   512,   0  },
#if (TCFG_SPORT_HEALTH_ENABLE)
    {"health_manager",		3 + TASK_PRIO_BASE,	0,	512, 128},
#endif
    {"aud_adc_demo",            1 + TASK_PRIO_BASE,     0,  512,   128 },
#if PRODUCT_TEST_ENABLE
    {"pt",					1 + TASK_PRIO_BASE,	    0,  512,	  128  },
#endif
#if TCFG_CHARGE_IC_MANAGER_MODE_ENABLE
    {"sbox_uart_task",       5 + TASK_PRIO_BASE,     0,  256,  256 },
#endif
#if (TCFG_DEV_MANAGER_ENABLE)
    {"ftran_back",		    1 + TASK_PRIO_BASE,	    0,  512,	  0  },
#endif
#if TCFG_UI_AVRCP_MUSIC_BG_ENABLE
    {"bip_deal",           		2 + TASK_PRIO_BASE,      0,  512,   512},
#endif
#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    {"sdmmc",        	    1,     0,   512,   128 },
#endif
#if TCFG_NETWORK_ENABLE
    {"tcpip_thread",    11,  1,  1024,  256},
#if TCFG_EXT_WIFI_ENABLE
    {"sdio_irq_thread",    8,  1,  1024,  256},
    {"ext_wifi_app_task",    10,  1,  1024,  64},
#if TCFG_AIC8800D40_ENABLE
    {"sdio_rx_task",	9, 	1,	2048,	256},
    {"fhost_cntrl_task",    9,  1,  2048,   256},
    {"fhost_rx_task",    9,  1,  2048,   256},
    {"fhost_wpa_task",    8,  1,  2048,   256},
    {"fhost_tx_task",    8,  1,  2048,   256},
    {"cli_cmd_task",    5,  1,  2048,   256},
    {"apm_staloss_task",    8,  1,  2048,  256},
#elif TCFG_RTL8733BS_ENABLE
    /* RTL8733BS WiFi driver threads */
    {"usb_recv_task",           7,     1,  1024,  0},
    {"p2p_main_thread",         8,     1,  1024,  256},
    {"p2p_polling_client",      8,     1,  1024,  0},
    {"p2p_polling_status",      8,     1,  1024,  0},
    {"__timer_check",           7,     1,  1024,  0},
    {"tasklet",                 10,     1,  1024,  0},
    {"wpa_auth_eloop_task",     9,     1,  2048,  0},
    {"RTW_XMIT_THREAD",         10,     1,  1024,  0},
    {"RTW_CMD_THREAD",          8,     1,  1024,  0},
    {"RTWHALXT",                10,     1,  1024,  0},
    {"RTW_MP_THREAD",           9,     1,  1024,  0},
    {"TLW_MP_THREAD",           9,     1,  1024,  0},
    {"RTW_RECV_THREAD",         8,     1,  1024,  0},
    {"RTW_EVENT_THREAD",        8,     1,  1024,  0},
    {"RTW_LBK_THREAD",          9,     1,  1024,  0},
    {"woal_reassoc_service",    7,     1,  1024,  0},
    {"RtmpTimerTask",           7,     1,  2048,  0},
    {"RtmpMlmeTask",            7,     1,  2048,  0},
    {"RtmpCmdQTask",            7,     1,  2048,  0},
    {"TLW_XMIT_THREAD",         7,     1,  1024,  0},
    {"TLW_HAL_XMIT",            7,     1,  1024,  0},
    {"TLW_CMD_THREAD",          7,     1,  1024,  0},
#endif
#endif
#endif
#if TCFG_LTE_PHY_ENABLE
    {"lte_rx_task",          TASK_PRIO_BASE,   0,   256,     0 },
#endif
#if TCFG_VSTREAM_TRANSFER
    {"vstream_pkg_send_task",   2,      0,  512,   64},
    {"vstream_recv_task",       2,      0,  512,   1024},
#endif
#if TCFG_DASHBOARD_HARDWARE_TEST_ENABLE
    {"gps_test",            1 + TASK_PRIO_BASE,     0,  512,   128 },
#endif
    {0, 0},
};


APP_VAR app_var;

volatile int g_run_func_test = 0;

__attribute__((weak))
int eSystemConfirmStopStatus(void)
{
    /* 系统进入在未来时间里，无任务超时唤醒，可根据用户选择系统停止，
     * 或者系统定时唤醒(100ms)，或自己指定唤醒时间
     * return:
     *   1:Endless Sleep
     *   0:100 ms wakeup
     *   other: x ms wakeup
     */
    return 0;

}

__attribute__((used))
int *__errno()
{
    static int err;
    return &err;
}

void app_var_init(void)
{
    app_var.play_poweron_tone = 1;
}


u8 get_power_on_status(void)
{
#if TCFG_ADKEY_ENABLE
    return is_adkey_press_down();
#endif

#if TCFG_IOKEY_ENABLE
    return is_iokey_press_down();
#endif

    return 0;
}


void check_power_on_key(void)
{
#if PRODUCT_TEST_ENABLE
    u8 pt_result;
    int r = syscfg_read(PT_TEST_RESULT, (u8 *)&pt_result, sizeof(pt_result));
    if (r <= 0) {
        log_info("vm read pt_test_result faild\n");
        return;
    }
#endif

    u32 delay_10ms_cnt = 0;
    while (1) {
        wdt_clear();
        os_time_dly(1);

        if (get_power_on_status()) {
            putchar('+');
            delay_10ms_cnt++;
            if (delay_10ms_cnt > 70) {
                app_var.poweron_reason = SYS_POWERON_BY_KEY;
                return;
            }
        } else {
            log_info("enter softpoweroff\n");
            delay_10ms_cnt = 0;
            app_var.poweroff_reason = SYS_POWEROFF_BY_KEY;
            power_set_soft_poweroff();
        }
    }
}


__attribute__((weak))
u8 get_charge_online_flag(void)
{
    return 0;
}

/*充电拔出,CPU软件复位, 不检测按键，直接开机*/
static void app_poweron_check(int update)
{
#if (CONFIG_BT_MODE == BT_NORMAL)
    if (!update && cpu_reset_by_soft()) {
        app_var.play_poweron_tone = 0;
        return;
    }


#if (TCFG_CHARGE_ENABLE && TCFG_CHARGE_OFF_POWERON_EN)
    if (is_ldo5v_wakeup()) {
#if TCFG_CHARGE_OFF_POWERON_EN
        app_var.play_poweron_tone = 0;
        app_var.poweron_reason = SYS_POWERON_BY_OUT_BOX;
        return;
#else
        //拔出关机
        power_set_soft_poweroff();
#endif
    }
#endif

#if TCFG_AUTO_POWERON_ENABLE
    return;
#endif

    check_power_on_key();

#endif
}



__attribute__((weak))
void board_init()
{

}

__attribute__((weak))
void arch_trim()
{

}

static void app_version_check()
{
    puts("=================Version===============\n");
    for (char *version = __VERSION_BEGIN; version < __VERSION_END;) {
        version += 4;
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");
}


#if TCFG_APP_RTC_EN
static struct sys_time def_sys_time = {  //初始化系统时间
    .year = 2025,
    .month = 10,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

void rtc_event_isr(const struct rtc_event_t *ev)    //闹钟回调函数测试
{
    if (ev->event & BIT(RTC_ALARM_EVENT)) {
        alm_wakeup_isr();
    }

    if (ev->event &  BIT(RTC_ALARM_EVENT)) {

    }

    if (ev->event == BIT(RTC_WKUP_EVENT)) {   //task 定时唤醒trim、仅触发定时trim事件才关机
        printf("rtc_trim_clk and enter soff");
        power_set_soft_poweroff();
    }
}

struct rtc_dev_platform_data rtc_dev_config = {   //RTC初始化结构体
    .default_sys_time = &def_sys_time,   //配置默认系统时钟
    .cbfun = rtc_event_isr,
    //rtc闹钟回调函数
};
#endif

static struct app_mode *app_task_init()
{
    app_var_init();
    app_version_check();

    /* sdfile_init(); */
    /* syscfg_tools_init(); */
    do_early_initcall();
    board_init();

    tzflash_change_mode();//dtr(clk),continue,wps

    do_platform_initcall();

#if (defined TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE) && TCFG_COLOR_SCREEN_CHARGING_CASE_ENABLE
#if(TCFG_CHARGE_BOX_ENABLE)
    /* clock_add_set(CHARGE_BOX_CLK); */
    chgbox_init_app();
    static u8 flag = 0;
    extern int send_to_trig_vcb(void (*func)(void), char *taskname);
    extern void soc_check_init();
    if (flag == 0) {
        flag = 1;
        send_to_trig_vcb(soc_check_init, "app_core");
    }
#endif
#endif

    tzflash_dump();

    cfg_file_parse(0);
    key_driver_init();

    do_initcall();
    do_module_initcall();
    do_late_initcall();

    dev_manager_init();

#if TCFG_APP_RTC_EN
    rtc_init(&rtc_dev_config);
    alarm_init();
#endif

    int update = 0;
    if (CONFIG_UPDATE_ENABLE) {
        update = update_result_deal();
    }

    app_var.start_time = jiffies_msec();

    int msg[4] = { MSG_FROM_APP, APP_MSG_GOTO_MODE, 0, 0 };
#if (!TCFG_CHARGE_OFF_POWERON_EN)
    if (get_charge_online_flag()) {
#if(TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
        msg[2] = APP_MODE_IDLE;
        msg[3] = IDLE_MODE_CHARGE;
    } else {
        msg[2] = APP_MODE_POWERON;
#if(TCFG_SYS_LVD_EN == 1)
        check_power_on_voltage();
#endif
        app_poweron_check(update);
        app_send_message(APP_MSG_POWER_ON, 0);
    }
#else//关机插入后先进power on 开bt

    //vpwr在线，vpwr供电
    if (get_charge_online_flag()) {
#if(TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
        if (check_vbat_low_power()) { //vbat浮空
            //进idle,不允许开蓝牙和退出充电UI
            msg[2] = APP_MODE_IDLE;
            msg[3] = IDLE_MODE_CHARGE;
        } else {
            //进power_on,允许开机
            msg[2] = APP_MODE_POWERON;
            /* app_poweron_check(update); */
            app_send_message(APP_MSG_POWER_ON, 0);
        }
    } else { //vpwr未插入，检测电量
        msg[2] = APP_MODE_POWERON;
        check_power_on_voltage();
        app_poweron_check(update);
        app_send_message(APP_MSG_POWER_ON, 0);
    }
#endif

#if TCFG_CHARGE_ENABLE
    set_charge_event_flag(1);
#endif

#if TCFG_DYNAMIC_SWITCHING_IOVDDM_ENABLE
    miovdd_adaptive_adjustment();
#endif

    arch_trim();

#if (defined TCFG_UI_ENABLE && TCFG_UI_ENABLE)
#if GPU_PORT_DEMO_ENABLE
    extern int gpu_port_demo_task(void);
    gpu_port_demo_task();
#elif defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE
    extern void gpu_demo_run(void);
    gpu_demo_run();
#else
#if (defined CONFIG_JL_UI_ENABLE && CONFIG_JL_UI_ENABLE)
    // JLUI 框架 依赖文件系统 在dev挂载后才能初始化
    UI_INIT((void *)&ui_cfg_data);
    /* wdt_disable(); */
#endif
#endif
#endif

#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
    int lvgl_ui_init(void);
    lvgl_ui_init();
#endif



#if TCFG_TOUCH_PANEL_ENABLE
    // tp和lcd共电源情况下，tp初始化需要放lcd初始化之后
    TP_INIT();
    /* wdt_close(); */
#endif

#if TCFG_DATA_STORAGE_ENABLE
    data_small_file_init();
#endif
#if TCFG_SPORT_HEALTH_ENABLE
    sport_health_manager_init();
#endif

    //程序正常运行看门狗时间
    wdt_init(WDT_APP_RUN_TIME);

#if PRODUCT_TEST_ENABLE
    extern int product_test_open();
    app_var.play_poweron_tone  = 0;
    app_var.wtone_volume = 100;
    product_test_open();
#endif

    struct app_mode *mode;
    mode = app_mode_switch_handler(msg);
    ASSERT(mode != NULL);

    clock_refurbish();

    return mode;
}

static int app_core_get_message(int *msg, int max_num)
{
    while (1) {
        int res = os_taskq_pend(NULL, msg, max_num);
        if (res != OS_TASKQ) {
            continue;
        }
        if (msg[0] & Q_MSG) {
            return 1;
        }
    }
    return 0;
}

static int g_mode_switch_arg;
static int g_mode_switch_msg[2];

static void retry_goto_mode_in_irq(void *_arg)
{
    if (g_mode_switch_msg[0] == APP_MSG_GOTO_MODE) {
        app_send_message2(g_mode_switch_msg[0], g_mode_switch_msg[1], g_mode_switch_arg);
    } else {
        app_send_message(g_mode_switch_msg[0], g_mode_switch_arg);
    }
}




int app_task_switch_to(u8 app_task, int priv)
{
    int ret = app_send_message2(APP_MSG_GOTO_MODE, app_task, priv);
    if (!ret) {
        if (app_get_current_mode_name()) {
            app_push_mode(app_get_current_mode_name());
        }
        return TRUE;
    }
    return FALSE;
}



int app_task_switch_back()
{
    int app_task = app_pop_mode();
    if (app_task != 0xff) {
        return app_task_switch_to(app_task, 0);
    } else {
        return app_task_switch_to(APP_MODE_BT, 0);
    }
}



u8 idle_sub_mode_get();
struct app_mode *app_mode_switch_handler(int *msg)
{
    int arg;
    struct app_mode *next_mode;

    if (msg[0] != MSG_FROM_APP) {
        return NULL;
    }

    /* 录像过程中不允许切换模式(按键)，防止按键映射变更导致无法停止录像;
     * 但 USB 插入等系统事件应优先处理: 先停止录像，再切到 PC 模式 */
#if TCFG_VIDEO0_ENABLE && (TCFG_CAMERA_SPI_ENABLE || TCFG_CAMERA_DVP_ENABLE)
    if (get_camera_status() == CAMERA_STATE_VIDEO) {
        if (msg[1] == APP_MSG_GOTO_MODE && msg[2] == APP_MODE_PC) {
            log_info("app_mode_switch_handler: PC mode via USB, stopping record first\n");
            camera_api_stop_rec_photo();
            /* fall through to normal mode switch below */
        } else {
            log_info("app_mode_switch_handler: video recording active, reject mode switch\n");
            return NULL;
        }
    }
#endif


    switch (msg[1]) {
    case APP_MSG_GOTO_MODE:
        arg = msg[3];
        log_info("APP_MSG_GOTO_MODE, CURR_TASK = %d,NEXT_TASK= %d \n", app_get_current_mode_name(), msg[2]);
        /*判断当前是否已经处于GOTO MODE希望切换的任务，如果是则不反复切换*/
        if (app_get_current_mode() && app_get_current_mode()->name == msg[2]) {
            if (app_get_current_mode_name() != APP_MODE_IDLE) {
                return NULL;
            }
            if (idle_sub_mode_get() == arg) {
                return NULL;
            }
        }
        next_mode = app_get_mode_by_name(msg[2]);
        break;
    case APP_MSG_GOTO_NEXT_MODE:
        arg = msg[2];
        next_mode = app_get_next_mode();
        break;
    default:
        return NULL;
    }

    if (!next_mode) {
        log_error("APP_MSG_GOTO_MODE target not found: %d\n", msg[2]);
        return NULL;
    }

    g_mode_switch_arg = arg;
    g_mode_switch_msg[0] = msg[1];
    g_mode_switch_msg[1] = msg[2];

#if TCFG_APP_BT_EN && TCFG_BT_BACKGROUND_ENABLE
    if (!bt_check_already_initializes()) {          //如果是后台使能蓝牙还没初始化，需要先记录切换的模式，等到蓝牙初始化完成之后再切回去
        struct app_mode *curr_mode = app_get_current_mode();
        if (next_mode->name != APP_MODE_BT && next_mode->name != APP_MODE_POWERON && next_mode->name != APP_MODE_IDLE) {
            u8 delayed_mode = next_mode->name;
            bt_background_set_switch_mode(delayed_mode);
            if (curr_mode && curr_mode->name == APP_MODE_POWERON) {
                next_mode =  app_get_mode_by_name(APP_MODE_BT);
                if (!next_mode) {
                    log_error("APP_MODE_BT target not found while delaying mode %d\n", delayed_mode);
                    return NULL;
                }
            } else {
                return NULL;
            }
        }
    }
#endif

    /*
     * 循环检查下一个模式是否可以进入
     */
    while (next_mode) {
        if (app_try_enter_mode(next_mode, arg)) {
            break;
        }
        next_mode = app_next_mode(next_mode);
    }
    if (!next_mode) {
        log_error("no enterable mode for target %d\n", msg[2]);
        return NULL;
    }
#if TCFG_MIX_RECORD_ENABLE
    //切换模式前关闭混合录音
    if (get_mix_recorder_status()) {
        mix_recorder_stop();
    }
    if (rcsp_mix_recorder_get_status()) {
        rcsp_mix_recorder_stop();
    }
#endif // TCFG_MIX_RECORD_ENABLE
    /*
     * 等待当前模式退出
     */
    if (!app_try_exit_curr_mode()) {
        sys_hi_timeout_add(NULL, retry_goto_mode_in_irq, 100);
        return NULL;
    }

    return next_mode;
}

int app_get_message(int *msg, int max_num, const struct key_remap_table *key_table)
{
    const struct app_msg_handler *handler;

    app_core_get_message(msg, max_num);

    if (msg[0] == MSG_FROM_KEY && key_table) {
        /*
         * 按键消息映射成当前模式的消息
         */
        struct app_mode *mode = app_get_current_mode();
        if (mode) {
            int key_msg = app_key_event_remap(key_table, msg + 1);
            if (key_msg == APP_MSG_NULL) {
                return 1;
            }
            if (mode->name == APP_MODE_BT) { //蓝牙模式 判断是否需要tws同步按键消息
#if TCFG_USER_TWS_ENABLE
                bt_tws_key_msg_sync(key_msg);
#else
                msg[0] = MSG_FROM_APP;
                msg[1] = key_msg;
#endif
            } else {
                msg[0] = MSG_FROM_APP;
                msg[1] = key_msg;
            }
        }
    }

    //消息截获,返回1表示中断消息分发
    int abandon = 0;
    for_each_app_msg_prob_handler(handler) {
        if (handler->from == msg[0]) {
            abandon = handler->handler(msg + 1);
            if (abandon) {
                break;
            }
        }
    }
    if (abandon) {
        return 0;
    }
    return 1;
}


void mem_printf(void *priv)
{
    mem_stats();
}


static void app_task_loop(void *p)
{
    struct app_mode *mode;

    printf("app_task_loop\n");

    /* sys_timer_add(NULL, mem_printf, 1000); */
    mode = app_task_init();

#if CONFIG_FINDMY_INFO_ENABLE || (BT_AI_SEL_PROTOCOL & REALME_EN)
#if (VFS_ENABLE == 1)
    if (mount(NULL, "mnt/sdfile", "sdfile", 0, NULL)) {
        log_debug("sdfile mount succ");
    } else {
        log_debug("sdfile mount failed!!!");
    }
#if BT_AI_SEL_PROTOCOL & REALME_EN
    int update = 0;
    u32 realme_breakpoint = 0;
    if (CONFIG_UPDATE_ENABLE) {
        update = update_result_deal();
        extern int realme_check_upgrade_area(int update);
        realme_check_upgrade_area(update);
    }
#endif
#endif /* #if (VFS_ENABLE == 1) */
#endif

    while (1) {
        if (!mode) {
            log_error("app_task_loop got NULL mode, fallback to idle\n");
            mode = app_get_mode_by_name(APP_MODE_IDLE);
            if (!mode) {
                os_time_dly(1);
                continue;
            }
        }
        app_set_current_mode(mode);
        audio_digital_vol_default_init();

//需要根据具体开发需求选择是否开启切mode也需要缓存flash
//1.需要注意的缓存到flash需要较长一段时间（1~3s），特别是从蓝牙后台模式切换到蓝牙前台模式，会出现用户明显感知卡顿现象，因此需要具体需求选择性开启。
#if 0
        //如果开启了VM配置项暂存RAM功能则在每次切模式的时候保存数据到vm_flash,避免丢失数据
        if (get_vm_ram_storage_enable()) {
            vm_flush2flash();
        }
#endif //#if 0
//

        switch (mode->name) {
        case APP_MODE_IDLE:
            mode = app_enter_idle_mode(g_mode_switch_arg);
            break;
        case APP_MODE_POWERON:
            mode = app_enter_poweron_mode(g_mode_switch_arg);
            break;
#if TCFG_APP_BT_EN
        case APP_MODE_BT:
            log_info("APP_MODE_BT \n");
            mode = app_enter_bt_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_MUSIC_EN
        case APP_MODE_MUSIC:
            log_info("APP_MODE_MUSIC \n");
            mode = app_enter_music_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_FM_EN
        case APP_MODE_FM:
            log_info("APP_MODE_FM \n");
            mode = app_enter_fm_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_RECORD_EN
        case APP_MODE_RECORD:
            log_info("APP_MODE_RECORD \n");
            mode = app_enter_record_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_LINEIN_EN
        case APP_MODE_LINEIN:
            log_info("APP_MODE_LINEIN \n");
            mode = app_enter_linein_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_RTC_EN
        case APP_MODE_RTC:
            log_info("APP_MODE_RTC \n");
            mode = app_enter_rtc_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_PC_EN
        case APP_MODE_PC:
            log_info("APP_MODE_PC \n");
            mode = app_enter_pc_mode(g_mode_switch_arg);
            break;
#endif
        case APP_MODE_UPDATE:
            log_info("APP_MODE_UPDATE \n");
            mode = app_enter_update_mode(g_mode_switch_arg);
            break;

#if (RCSP_MODE)
        case APP_MODE_RCSP:
            extern struct app_mode *app_enter_rcsp_mode(int arg);
            log_info("APP_MODE_RCSP \n");
            mode = app_enter_rcsp_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_VIDEO_EN
        case APP_MODE_VIDEO:
            log_info("APP_MODE_VIDEO \n");
            mode = app_enter_video_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_VIDEO_REC_EN
        case APP_MODE_VIDEO_REC:
            log_info("APP_MODE_VIDEO_REC \n");
            mode = app_enter_video_rec_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_PHOTO_EN
        case APP_MODE_PHOTO:
            log_info("APP_MODE_PHOTO \n");
            mode = app_enter_photo_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_VIDEO_DEC_EN
        case APP_MODE_VIDEO_DEC:
            log_info("APP_MODE_VIDEO_DEC \n");
            mode = app_enter_video_dec_mode(g_mode_switch_arg);
            break;
#endif
        default:
            break;
        }
    }

}

void app_main()
{
#if (defined TCFG_UI_ENABLE && TCFG_UI_ENABLE)
    printf("UI FRAME INIT: %s\n", (TCFG_UI_ENABLE ? (CONFIG_LVGL_UI_ENABLE ? "LVGL" : (CONFIG_JL_UI_ENABLE ? "JLUI" : "NULL")) : "NULL"));
#endif

#ifdef CONFIG_CXX_SUPPORT
    void cpp_run_init(void);
    cpp_run_init();
#endif

#if (defined TCFG_UI_ENABLE && TCFG_UI_ENABLE)
    extern const struct ui_devices_cfg ui_cfg_data;
#if (defined CONFIG_LVGL_UI_ENABLE && CONFIG_LVGL_UI_ENABLE)
    // GPU DEMO
    /* extern int jlgpu_demo_task_start(char *name, void *arg); */
    /* jlgpu_demo_task_start("ui", (void *)&ui_cfg_data); */
#endif
#endif

    task_create(app_task_loop, NULL, "app_core");

    os_start(); //no return
    while (1) {
        asm("idle");
    }
}

