#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".battery_level.data.bss")
#pragma data_seg(".battery_level.data")
#pragma const_seg(".battery_level.text.const")
#pragma code_seg(".battery_level.text")
#endif
#include "system/includes.h"
#include "battery_manager.h"
#include "app_power_manage.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "app_charge.h"
#include "asm/charge.h"
#include "app_tone.h"
#include "gpadc.h"
#include "btstack/avctp_user.h"
#include "user_cfg.h"
#include "bt_tws.h"
#include "idle.h"
#include "ui/ui_api.h"
#include "rcsp.h"
#include "power/power_manage.h"
#include "power/lp_msg.h"
#include "rtc.h"
#include "timestamp/timestamp.h"
#if RCSP_ADV_EN
#include "ble_rcsp_adv.h"
#endif

#define LOG_TAG             "[BATTERY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

enum {
    M2P_MSG_BATTERY_CONFIG = 0,
    M2P_MSG_BATTERY_REPORT,
    M2P_MSG_BATTERY_FULL_FLAG,
    M2P_MSG_BATTERY_CURVE_DATA,
    M2P_MSG_BATTERY_CHECK,
    M2P_MSG_BATTERY_CHARGE_ONLINE,
};

enum {
    VBAT_NORMAL = 0,
    VBAT_WARNING,
    VBAT_LOWPOWER,
} VBAT_STATUS;

/* 电池曲线点  */
struct battery_curve_q32f {
    u8  percent;
    u16 voltage;
} __attribute__((packed));

struct battery_q32f_config {
    u16 poweroff_voltage;
    u16 warning_voltage;
    u16 cc_val;
    u16 cc_progi_volt;
    u8  vm_percent;
    u8  charge_poweron_en;
    u32 cur_ts;
    u32 vm_ts;
} __attribute__((packed));

struct battery_curve {
    u8 percent;
    u16 voltage;
};

union battery_data {
    u32 raw_data;
    struct {
        u8 reserved;
        u16 voltage;
        u8 percent;
    } __attribute__((packed)) data;
};

struct battery_q32f_report {
    u8  percent;
    u8  level;
    u8  status;
    u8  charging;
    u8  need_storage;
} __attribute__((packed));

/* ==================== 全局状态 ==================== */
static u8  old_battery_level      = 9;
static u8  cur_battery_level      = 0;
static u8  old_battery_percent    = 0;
static u8  cur_battery_percent    = 0;
static u8  tws_sibling_bat_level  = 0xff;
static u8  tws_sibling_bat_percent_level = 0xff;
static u8  cur_bat_st             = VBAT_NORMAL;  /* 0=正常 1=警告 2=低电 */
static u8  cur_charging           = 0;
static u8  old_charging           = 0;
static u8  old_charge_online_flag = 0xff;
static u32 g_timestamp            = 0;

struct battery_info {
    u32 time_stamp;
    u8  percent;
};

static u8 battery_curve_max;
static struct battery_curve *battery_curve_p;
static struct battery_info vm_info;

extern void clr_wdt(void);
/* ==================== TWS 电量同步 ==================== */

#if TCFG_USER_TWS_ENABLE
u8 get_tws_sibling_bat_level(void)
{
    return tws_sibling_bat_level & 0x7f;
}

u8 get_tws_sibling_bat_persent(void)
{
    return tws_sibling_bat_percent_level;
}

void app_power_set_tws_sibling_bat_level(u8 vbat, u8 percent)
{
    tws_sibling_bat_level = vbat;
    tws_sibling_bat_percent_level = percent;
    batmgr_send_msg(POWER_EVENT_SYNC_TWS_VBAT_LEVEL, 0);
    log_info("set_sibling_bat_level: %d, %d\n", vbat, percent);
}

static void set_tws_sibling_bat_level(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;
    if (rx) {
        app_power_set_tws_sibling_bat_level(data[0], data[1]);
    }
}

REGISTER_TWS_FUNC_STUB(vbat_sync_stub) = {
    .func_id = TWS_FUNC_ID_VBAT_SYNC,
    .func    = set_tws_sibling_bat_level,
};

void tws_sync_bat_level(void)
{
#if TCFG_BT_DISPLAY_BAT_ENABLE
    u8 battery_level = cur_battery_level;
#if CONFIG_DISPLAY_DETAIL_BAT
    u8 percent_level = get_vbat_percent();
#else
    u8 percent_level = get_self_battery_level() * 10 + 10;
#endif
    if (get_charge_online_flag()) {
        percent_level |= BIT(7);
    }
    u8 data[2];
    data[0] = battery_level;
    data[1] = percent_level;
    tws_api_send_data_to_sibling(data, 2, TWS_FUNC_ID_VBAT_SYNC);
    log_info("tws_sync_bat_level: %d,%d\n", battery_level, percent_level);
#endif
}
#endif

static int app_power_event_handler(int *msg)
{
    int ret = false;

#if (TCFG_SYS_LVD_EN == 1)
    switch (msg[0]) {
    case POWER_EVENT_POWER_NORMAL:
        break;
    case POWER_EVENT_POWER_WARNING:
        play_tone_file(get_tone_files()->low_power);
#ifdef CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE
        UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_LOW_POWER_TIPS, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_CHARGE);
#endif
        break;
    case POWER_EVENT_POWER_LOW:
        r_printf(" POWER_EVENT_POWER_LOW");
#if TCFG_APP_BT_EN
#if RCSP_ADV_EN
        adv_tws_both_in_charge_box(1);
#endif
        if (!app_in_mode(APP_MODE_IDLE)) {
            sys_enter_soft_poweroff(POWEROFF_NORMAL);
        } else {
            power_set_soft_poweroff();
        }
#else
        app_goto_mode(APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
#endif
        break;
#if TCFG_APP_BT_EN
    case POWER_EVENT_SYNC_TWS_VBAT_LEVEL:
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
        }
        break;
    case POWER_EVENT_POWER_CHANGE:
        log_info("POWER_EVENT_POWER_CHANGE\n");
        if (!app_in_mode(APP_MODE_BT)) {
            break;
        }
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_tws_state() & TWS_STA_ESCO_OPEN) {
                break;
            }
            tws_sync_bat_level();
        }
#endif
        bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
#endif
        RCSP_UPDATE(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_BATTERY));
        break;
    case POWER_EVENT_POWER_CHARGE:
        break;
#if TCFG_CHARGE_ENABLE
    case CHARGE_EVENT_LDO5V_OFF:
        //充电拔出时重新初始化检测定时器
        vbat_check_init();
        break;
#endif
    default:
        break;
    }
#endif

    return ret;
}
APP_MSG_HANDLER(bat_level_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = app_power_event_handler,
};

static u16 get_vbat_voltage(void)
{
    return gpadc_battery_get_voltage();
}

u16 get_vbat_value(void)
{
    return get_vbat_voltage();
}

u8 get_vbat_percent(void)
{
    return cur_battery_percent;
}

bool get_vbat_need_shutdown(void)
{
    if ((get_vbat_voltage() <= app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
        return TRUE;
    }
    return FALSE;
}

//获取自身的电量
u8  get_self_battery_level(void)
{
    return cur_battery_level;
}

#if TCFG_USER_TWS_ENABLE
u8 get_cur_battery_level(void)
{
    u8 bat_lev = tws_sibling_bat_level & (~BIT(7));
    if (bat_lev == 0x7f) {
        return cur_battery_level;
    }
#if (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LOWER)
    return cur_battery_level < bat_lev ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_HIGHER)
    return cur_battery_level < bat_lev ? bat_lev : cur_battery_level;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LEFT)
    return tws_api_get_local_channel() == 'L' ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_RIGHT)
    return tws_api_get_local_channel() == 'R' ? cur_battery_level : bat_lev;
#else
    return cur_battery_level;
#endif
}
#else
u8 get_cur_battery_level(void)
{
    return cur_battery_level;
}
#endif

int read_bat_info_vm(struct battery_info *info)
{
    int ret = syscfg_read(VM_BATTERY_INFO, info, sizeof(struct battery_info));
    if (ret != sizeof(struct battery_info)) {
        return -1;
    }
    return 0;
}

void bat_info_init_from_vm(void)
{
    int ret = read_bat_info_vm(&vm_info);
    if (ret) {
        vm_info.time_stamp = 0;
        vm_info.percent = 0xFF;
    }
}

void update_bat_info_vm()
{
    struct sys_time time;
    rtc_read_time(&time);
    u32 timestamp = timestamp_mytime_2_utc_sec(&time);
    struct battery_info info = {
        .time_stamp = timestamp,
        .percent = cur_battery_percent,
    };
    syscfg_write(VM_BATTERY_INFO, &info, sizeof(struct battery_info));
}

void bat_info_storage_task_sync()
{
    int argv[3];
    argv[0] = (int)update_bat_info_vm;
    argv[1] = 0;
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, argv);
    if (ret) {
        log_error("vbat change post ret:%d \n", ret);
    }
}

/**
 * @brief 充满状态变化时, 主动推送充满标志给 Q32F
 * @param is_full 1=已充满 0=未满
 */
#if TCFG_CHARGE_ENABLE
void battery_q32f_push_full_flag(void)
{
    u8 buf[2];
    buf[0] = M2P_MSG_BATTERY_FULL_FLAG;
    buf[1] = get_charge_full_flag();
    m2p_post_msg_q32f(MSG_BATTERY, 0, buf, sizeof(buf));
}
#endif
/**
 * @brief 充电状态变化时, 推送在线标志给 q32f
 * @param online 1=充电在线 0=未充电
 */
void battery_q32f_push_charge_online(u8 online)
{
    if (online != old_charge_online_flag) {
        old_charge_online_flag = online;
        u8 buf[2];
        buf[0] = M2P_MSG_BATTERY_CHARGE_ONLINE;
        buf[1] = online;
        m2p_post_msg_q32f(MSG_BATTERY, 0, buf, sizeof(buf));
    }
}

static void battery_q32f_msg_handler(void *priv, u8 *buf, u32 len)
{
    if (len < 1 + sizeof(struct battery_q32f_report)) {
        return;
    }

    if (buf[0] != M2P_MSG_BATTERY_REPORT) {
        return;
    }

    struct battery_q32f_report *rpt = (struct battery_q32f_report *)(buf + 1);

    /* 更新全局变量 */
    cur_battery_percent = rpt->percent;
    cur_battery_level   = rpt->level;
    cur_charging        = rpt->charging;

    log_info("rpt: P:%d%% Lv:%d St:%d CHG:%d stor:%d",
             rpt->percent, rpt->level, rpt->status, rpt->charging, rpt->need_storage);

    /* 根据 q32f 上报的状态发出 BR33 内部事件 */
    switch (rpt->status) {
    case VBAT_LOWPOWER:
        cur_bat_st = VBAT_LOWPOWER;
        log_info("POWER_EVENT_POWER_LOW");
        batmgr_send_msg(POWER_EVENT_POWER_LOW, 0);
        break;
    case VBAT_WARNING:
        cur_bat_st = VBAT_WARNING;
        log_info("Low Power Warning");
        batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
        break;
    case VBAT_NORMAL:
    default:
        cur_bat_st = VBAT_NORMAL;
        log_info("Power Normal");
        batmgr_send_msg(POWER_EVENT_POWER_NORMAL, 0);
        break;
    }

    /* q32f 上报 need_storage 标志, 触发VM存储 */
    if (rpt->need_storage) {
        bat_info_storage_task_sync();
    }

    /* 充电状态变化 */
    if (old_charging != cur_charging) {
        old_charging = cur_charging;
        batmgr_send_msg(POWER_EVENT_POWER_CHARGE, 0);
        batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);
    }

    /* 电量/百分比变化 */
    if ((cur_battery_level != old_battery_level) ||
        (cur_battery_percent != old_battery_percent)) {
        batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);
        old_battery_level   = cur_battery_level;
        old_battery_percent = cur_battery_percent;
    }

    UI_MSG_POST("batcharge:process=%4", cur_battery_percent);
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_BATTERY, battery_q32f_msg_handler);

/**
 * @brief 将电量初始化配置下发到 q32f
 */
static void send_battery_config_to_q32f(void)
{
    struct battery_q32f_config cfg;

    cfg.poweroff_voltage = app_var.poweroff_tone_v;
    cfg.warning_voltage  = app_var.warning_tone_v;
#if TCFG_CHARGE_ENABLE
    cfg.cc_val           = get_charge_constant_current_val();
    cfg.cc_progi_volt    = get_charge_constant_current_progi_volt();
    cfg.charge_poweron_en  = get_charge_poweron_en();
#else
    cfg.cc_val           = 0xFFFF;
    cfg.cc_progi_volt    = 0xFFFF;
    cfg.charge_poweron_en  = 0;
#endif
    cfg.vm_percent         = vm_info.percent;
    cfg.cur_ts             = g_timestamp;
    cfg.vm_ts         = vm_info.time_stamp;
    u8 buf[1 + sizeof(cfg)];
    buf[0] = M2P_MSG_BATTERY_CONFIG;
    memcpy(&buf[1], &cfg, sizeof(cfg));
    m2p_post_msg_q32f(MSG_BATTERY, 0, buf, sizeof(buf));
    log_debug("send cfg to q32f: poff=%dmV warn=%dmV vm_p=%d cur_ts=%d vm_ts=%d cc_val:0x%x cc_progi_volt:0x%x",
              cfg.poweroff_voltage, cfg.warning_voltage, cfg.vm_percent, cfg.cur_ts, cfg.vm_ts, cfg.cc_val, cfg.cc_progi_volt);
}

static void send_battery_curve_to_q32f(void)
{
    int i, g;
    u8 curve_cnt;
    u8 buf[26];

    if (battery_curve_p == NULL) {
        log_error("battery_curve not init\n");
        return;
    }

    curve_cnt = battery_curve_max;
    if (curve_cnt > 16) {
        curve_cnt = 16;
    }

    for (g = 0; g < 2; g++) {
        u8 cnt = (curve_cnt > (g + 1) * 8) ? 8 : (curve_cnt > g * 8 ? curve_cnt - g * 8 : 0);
        if (cnt == 0) {
            break;
        }

        /* 检查下一批是否为空, 是则标记当前为最后一批 */
        u8 next_cnt = (curve_cnt > (g + 2) * 8) ? 8 : (curve_cnt > (g + 1) * 8 ? curve_cnt - (g + 1) * 8 : 0);
        u8 last_flag = (next_cnt == 0) ? 0x80 : 0;

        buf[0] = M2P_MSG_BATTERY_CURVE_DATA;
        buf[1] = cnt | last_flag;
        for (i = 0; i < cnt; i++) {
            u8 idx = g * 8 + i;
            buf[2 + i * 3]     = battery_curve_p[idx].percent;
            buf[2 + i * 3 + 1] = (u8)(battery_curve_p[idx].voltage & 0xFF);
            buf[2 + i * 3 + 2] = (u8)(battery_curve_p[idx].voltage >> 8);
        }
        m2p_post_msg_q32f(MSG_BATTERY, 0, buf, (u32)(2 + cnt * 3));
    }
    log_debug("send curve to q32f: cnt=%d", curve_cnt);
}

static void send_battery_check_to_q32f(void)
{
    u8 buf[1];
    buf[0] = M2P_MSG_BATTERY_CHECK;
    m2p_post_msg_q32f(MSG_BATTERY, 0, buf, sizeof(buf));
    log_info("send check to q32f");
}

void vbat_check_init(void)
{
    u8 tmp[128] = {0};
    int i;
    u16 battery_0, battery_100;
    union battery_data battery_data_t;

    //初始化电池曲线
    if (battery_curve_p == NULL) {
        memset(tmp, 0x00, sizeof(tmp));
        int ret = syscfg_read(CFG_BATTERY_CURVE_ID, tmp, sizeof(tmp));
        if (ret > 0) {
            battery_curve_max = ret / sizeof(battery_data_t);
        } else {
            battery_curve_max = 2;
        }
        battery_curve_p = malloc(battery_curve_max * sizeof(struct battery_curve));
        ASSERT(battery_curve_p, "malloc battery_curve err!");
        if (ret < 0) {
            log_error("battery curve id, ret: %d\n", ret);
            battery_0 = app_var.poweroff_tone_v;
#if TCFG_CHARGE_ENABLE
            //防止部分电池充不了这么高电量，充满显示未满的情况
            battery_100 = (get_charge_full_value() - 100);
#else
            battery_100 = 4100;
#endif
            battery_curve_p[0].percent = 0;
            battery_curve_p[0].voltage = battery_0;
            battery_curve_p[1].percent = 100;
            battery_curve_p[1].voltage = battery_100;
            log_info("percent: %d, voltage: %d mV", 0, battery_curve_p[0].voltage);
            log_info("percent: %d, voltage: %d mV", 100, battery_curve_p[1].voltage);
        } else {
            for (i = 0; i < battery_curve_max; i++) {
                memcpy(&battery_data_t.raw_data,
                       &tmp[i * sizeof(battery_data_t)], sizeof(battery_data_t));
                battery_curve_p[i].percent = battery_data_t.data.percent;
                battery_curve_p[i].voltage = battery_data_t.data.voltage;
                log_info("percent: %d, voltage: %d mV\n",
                         battery_curve_p[i].percent, battery_curve_p[i].voltage);
            }
        }
        struct sys_time time;
        rtc_read_time(&time);
        g_timestamp = timestamp_mytime_2_utc_sec(&time);
        bat_info_init_from_vm();
        send_battery_config_to_q32f();
        send_battery_curve_to_q32f();
    }
#if TCFG_CHARGE_ENABLE
    /*更新满电信息*/
    battery_q32f_push_full_flag();
    /*更新充电在线标志*/
    battery_q32f_push_charge_online(get_charge_online_flag());
#endif
    send_battery_check_to_q32f();
}

bool vbat_is_low_power(void)
{
    return (cur_bat_st != VBAT_NORMAL);
}

void check_power_on_voltage(void)
{
#if(TCFG_SYS_LVD_EN == 1)

    u16 val = 0;
    u8 normal_power_cnt = 0;
    u8 low_power_cnt = 0;

    while (1) {
        clr_wdt();
        val = get_vbat_voltage();
        printf("vbat: %d\n", val);
        if ((val < app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
            low_power_cnt++;
            normal_power_cnt = 0;
            if (low_power_cnt > 10) {
                /* ui_update_status(STATUS_POWERON_LOWPOWER); */
                os_time_dly(100);
                /* log_info("power on low power , enter softpoweroff!\n");*/
                power_set_soft_poweroff();
            }
        } else {
            normal_power_cnt++;
            low_power_cnt = 0;
            if (normal_power_cnt > 10) {
                vbat_check_init();
                return;
            }
        }
    }
#endif
}

u8 check_vbat_low_power(void)
{
    int ret = 0;
#if(TCFG_SYS_LVD_EN == 1)
    u16 val = 0;
    u8 normal_power_cnt = 0;
    u8 low_power_cnt = 0;

    while (1) {
        clr_wdt();
        val = get_vbat_voltage();
        printf("vbat: %d\n", val);
        if ((val < app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
            low_power_cnt++;
            normal_power_cnt = 0;
            if (low_power_cnt > 10) {
                /* ui_update_status(STATUS_POWERON_LOWPOWER); */
                os_time_dly(100);
                ret = 1;
                /* printf("%s low_power",__func__); */
                return ret;
            }
        } else {
            normal_power_cnt++;
            low_power_cnt = 0;
            if (normal_power_cnt > 10) {
                /* printf("%s power_on",__func__); */
                ret = 0;
                return ret;
            }
        }
    }
#endif
    return ret;
}
