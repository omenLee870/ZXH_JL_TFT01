#include "sdk_config.h"
#include "includes.h"
#include "main.h"
#include "usr_timer.h"
#include "init.h"
#include "battery_pmc.h"
#include "gpadc.h"

#define LOG_TAG_CONST       BATTERY_PMC
#define LOG_TAG             "[BAT_PMC]"
#include "generic/log.h"

enum {
    M2P_MSG_BATTERY_CONFIG = 0,
    M2P_MSG_BATTERY_REPORT,
    M2P_MSG_BATTERY_FULL_FLAG,
    M2P_MSG_BATTERY_CURVE_DATA,
    M2P_MSG_BATTERY_CHECK,
    M2P_MSG_BATTERY_CHARGE_ONLINE,
};
/* ==================== 检测参数 ==================== */
#define VBAT_DETECT_CNT       3      // 每次更新采集次数
#define VBAT_DETECT_INTERVAL  10     // 每次采集间隔 ms
#define VBAT_UPDATE_SLOW_TIME 60000L // 慢周期更新 ms (60s)
#define VBAT_UPDATE_FAST_TIME 10000L // 快周期更新 ms (10s)
#define LOW_POWER_WARN_TIME   (60 * 1000) // 低电提醒重复间隔 60s

/* ==================== 电池百分比计算模式 ==================== */
#define VBAT_PERCENT_MODE_OPEN_CIRCUIT_VOLTAGE      0  // 开路电压法
#define VBAT_PERCENT_MODE_CURRENT_INTEGRATING       1  // 安时积分法 (库仑计)
#define VBAT_PERCENT_MODE_SEL   VBAT_PERCENT_MODE_OPEN_CIRCUIT_VOLTAGE

/* ==================== 电量变化时存储到vm =================== */
#define abs(x) ((x)>0?(x):-(x))
#define BATTERY_CHANGE_SAVE		5				//电量变化超过5%时存储到vm
#define RECHECK_HOUR			24				//关机超过24小时，开机时校准
#define RECHECK_SOC				10				//误差超过10%电量，开机时校准

/* ==================== 安时积分参数 ==================== */
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
#define REAL_CHANGE_PER			85				//真实电流占比
#define PRED_CHANGE_PER			15				//模拟电流占比
#define BATTERY_CHARGE_K 		(1.0f)			//模拟充电电流系数
#define BATTERY_CAPACITY    	(200.0f) 		//电池总容量，单位mAh
#define BATTERY_RESISTOR    	(0.15f)   		//电池阻值
#define VBAT_UPDATE_SEC			8				//刷新间隔
#endif

/* ==================== 状态 ==================== */
enum {
    VBAT_NORMAL    = 0,
    VBAT_WARNING   = 1,
    VBAT_LOWPOWER  = 2,
};

/* ==================== 全局变量 ==================== */
static u8 battery_curve_max;
static struct battery_curve g_curve[16];
static u16 g_poweroff_voltage = 3400;
static u16 g_warning_voltage  = 3500;

static u16 vbat_slow_timer  = 0;
static u16 vbat_fast_timer  = 0;
static u16 lowpower_timer   = 0;

static u8  old_battery_level = 0xff;
static u16 cur_battery_voltage = 0;
static u8  cur_battery_level   = 0;
static u8  old_battery_percent = 0xff;
static u8  cur_battery_percent = 0;
static u8  cur_bat_st          = VBAT_NORMAL;
static u8  g_charge_online_flag = 0;
static u8  old_charge_online_flag = 0;
static u8  need_storage          = 0;  // 上报标志: 1=需要主系统写VM
static u8  last_present_k        = 0;
static u8  g_charge_poweron_en   = 0;

/* VM 数据(从 config 接收, 曲线接收完毕后使用) */
static u8  g_vm_percent = 0xff;
static u32 g_cur_ts     = 0;
static u32 g_vm_ts      = 0;

static u8  unit_cnt         = 0;
static u32 bat_voltage      = 0;
static u8  low_power_cnt    = 0;
static u8  low_voice_cnt    = 0;
static u8  power_normal_cnt = 0;
static u8  low_voice_first_flag = 1;//进入低电后先提醒一次

#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
static float current_soc     = 0.0f;
static u8   g_charge_full_flag = 0;
static u16  g_cc_val = 0xFFFF;
static u16  g_cc_progi_volt = 0xFFFF;
#endif

/* 前向声明 */
static void vbat_check_slow(void *priv);
static void vbat_check(void *priv);
static void vbat_timer_delete(void);
static void battery_report_to_main(u8 msg_type);

static u8 get_lvcmp_det(void)
{
    return ((P33_CON_GET(P3_ANA_READ) & BIT(1)) ? 1 : 0);
}

static u8 is_trickle_en(void)
{
    return ((P33_CON_GET(P3_CHG_CON0) & BIT(3)) ? 1 : 0);
}

u8 get_vbat_percent(void)
{
    return cur_battery_percent;
}
//将当前电量转换为1~9级发送给手机同步电量
u8  battery_value_to_phone_level(void)
{
    u8  battery_level = 0;
    u8 vbat_percent = get_vbat_percent();
    if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
        return 0;
    }
    battery_level = (vbat_percent - 5) / 10;
    return battery_level;
}

static u16 get_vbat_voltage(void)
{
    return gpadc_battery_get_voltage();
}

static u16 battery_calc_voltage(u16 percent)
{
    u8 i;
    u16 max, min, div_voltage, tmp_voltage;

    if (battery_curve_max < 2) {
        return 0;
    }

    for (i = 0; i < (battery_curve_max - 1); i++) {
        if (percent <= g_curve[i].percent) {
            return g_curve[i].voltage;
        }
        if (percent >= g_curve[i + 1].percent) {
            continue;
        }

        div_voltage = g_curve[i + 1].voltage - g_curve[i].voltage;
        min = g_curve[i].percent;
        max = g_curve[i + 1].percent;
        tmp_voltage = g_curve[i].voltage;
        tmp_voltage += ((percent - min) * div_voltage / (max - min));
        return tmp_voltage;
    }
    return g_curve[battery_curve_max - 1].voltage;
}

static u16 battery_calc_percent(u16 bat_val)
{
    u8 i, tmp_percent;
    u16 max, min, div_percent;

    if (battery_curve_max < 2) {
        return 0;
    }

    for (i = 0; i < (battery_curve_max - 1); i++) {
        if (bat_val <= g_curve[i].voltage) {
            return g_curve[i].percent;
        }
        if (bat_val >= g_curve[i + 1].voltage) {
            continue;
        }

        div_percent = g_curve[i + 1].percent - g_curve[i].percent;
        min = g_curve[i].voltage;
        max = g_curve[i + 1].voltage;
        tmp_percent = g_curve[i].percent;
        tmp_percent += (bat_val - min) * div_percent / (max - min);
        return tmp_percent;
    }
    return g_curve[battery_curve_max - 1].percent;
}


#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)

/* ==================== 安时积分法 (库仑计) ==================== */
static float vbat_predicted_current(u16 voltage, u16 soc)
{
    u16 ocv = battery_calc_voltage(soc);
    log_debug("%s ocv:%d soc:%d vol:%d ", __func__, ocv, soc, voltage);
    return (float)((int)voltage - (int)ocv) / BATTERY_RESISTOR;
}

static void vbat_calculate_loop(void *priv)
{
    u8 curr_percent_k;
    u16 current_vbat  = get_vbat_voltage();
    u16 ocv2soc = battery_calc_percent(current_vbat);
    float I = 0.0f;

    if (!get_lvcmp_det()) {
        I = vbat_predicted_current(current_vbat, current_soc);
        current_soc += I * VBAT_UPDATE_SEC * 100.0f / 3600.0f / BATTERY_CAPACITY;
    } else {
        if (g_cc_val != 0xFFFF) {
            float I1 = vbat_predicted_current(current_vbat, current_soc) * BATTERY_CHARGE_K;
            float I2;
            if (is_trickle_en()) {
                I2 = adc_get_voltage(AD_CH_PMU_PROGI) * g_cc_val / g_cc_progi_volt / 10.0f;
            } else {
                I2 = adc_get_voltage(AD_CH_PMU_PROGI) * g_cc_val / g_cc_progi_volt;
            }
            I = (REAL_CHANGE_PER * I2 + PRED_CHANGE_PER * I1) / 100.0f;
            log_debug("change:I1 :%d I2:%d I:%d ", (u32)(abs(I1)), (u32)(abs(I2)), (u32)(abs(I)));
        } else {
            I = vbat_predicted_current(current_vbat, current_soc);
        }
        current_soc += I * VBAT_UPDATE_SEC * 100.0f / 3600.0f / BATTERY_CAPACITY;

        if (!g_charge_full_flag) {//未充满
            if (current_soc > 99.99f) {
                current_soc = 99.99f;
            }
        } else {
            current_soc += 1.0f;//快速逼近100%
        }
    }
    //soc边界处理
    if (current_soc < 0)  {
        current_soc = 0;
    }
    if (current_soc > 100) {
        current_soc = 100;
    }

    //soc2percent
    curr_percent_k = (u8)current_soc;

    if (g_charge_online_flag) {
        if (curr_percent_k < last_present_k) {
            curr_percent_k = last_present_k;
        }
    } else {
        if (curr_percent_k >= last_present_k) {
            curr_percent_k = last_present_k;
        }
    }

    cur_battery_percent = curr_percent_k;

    if (abs(cur_battery_percent - last_present_k) >= BATTERY_CHANGE_SAVE) {
        last_present_k = cur_battery_percent;
        need_storage = 1;
        battery_report_to_main(M2P_MSG_BATTERY_REPORT);
        need_storage = 0;
    }
    log_debug("V:%dmV I:%dmA SOC:%d OCV2SOC:%d last:%d CHG:%d lvcmp:%d curr_k:%d",
              current_vbat, (u32)(abs(I)), (u32)(current_soc), ocv2soc, last_present_k, g_charge_online_flag, get_lvcmp_det(), curr_percent_k);
}

#endif /* VBAT_PERCENT_MODE_SEL */

static void battery_report_to_main(u8 msg_type)
{
    u8 buf[1 + 5]; // msg_type + percent + level + status + charging + need_storage
    buf[0] = msg_type;
    buf[1] = cur_battery_percent;
    buf[2] = cur_battery_level;
    buf[3] = cur_bat_st;
    buf[4] = g_charge_online_flag;
    buf[5] = need_storage;
    log_info("battery_report_to_main type=%d P=%d%% Lv=%d St=%d CHG=%d stor=%d",
             msg_type, cur_battery_percent, cur_battery_level, cur_bat_st, g_charge_online_flag, need_storage);
    p2m_post_msg(MSG_BATTERY, 0, buf, sizeof(buf));
}

static bool battery_init_vm_check(u16 vpercent)
{
    last_present_k =  vpercent;
    if (g_vm_percent == 0xFF) {
        log_info("%s vm not find", __func__);
        return true;
    }
    if ((g_cur_ts - g_vm_ts) > (u32)(RECHECK_HOUR * 60 * 60)) {  //超过半小时
        log_info("%s time need reset", __func__);
        return true;
    }

    if (abs(vpercent - g_vm_percent) > RECHECK_SOC) { //误差超过10%
        log_info("%s vbat percent more than 10per", __func__);
        return true;
    }
    cur_battery_percent = g_vm_percent;
    log_info("%s used vm info:%d\n ", __func__, g_vm_percent);
    return false;
}

/* ==================== 消息处理 (接收主系统配置) ==================== */
static void battery_main_msg_handler(void *priv, u8 *msg, u32 len)
{
    switch (msg[0]) {
    case M2P_MSG_BATTERY_CONFIG:
        battery_recv_config((struct battery_config *)(msg + 1));
        break;

    case M2P_MSG_BATTERY_FULL_FLAG:
        struct battery_full_flag *flag = (struct battery_full_flag *)(msg + 1);
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
        g_charge_full_flag = flag->is_full;
#endif
        log_info("full flag: %d", flag->is_full);
        if (flag->is_full && !g_charge_poweron_en) {
            vbat_timer_delete();
        }
        break;

    case M2P_MSG_BATTERY_CURVE_DATA:
        u8 cnt     = msg[1] & 0x7F;
        u8 is_last = msg[1] & 0x80;
        u8 i;
        for (i = 0; i < cnt && battery_curve_max < 16; i++) {
            g_curve[battery_curve_max].percent = msg[2 + i * 3];
            g_curve[battery_curve_max].voltage = (u16)msg[2 + i * 3 + 1]
                                                 | (u16)msg[2 + i * 3 + 2] << 8;
            battery_curve_max++;
        }
        if (is_last) {
            cur_battery_voltage = get_vbat_voltage();
            u16 tmp_percent = battery_calc_percent(cur_battery_voltage);
            u8 ret = battery_init_vm_check(tmp_percent);
            if (ret) {
                cur_battery_percent = tmp_percent;
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
                current_soc  = cur_battery_percent;
#endif
                log_info("%s %d", __func__, cur_battery_percent);
                cur_battery_level = battery_value_to_phone_level();
            }
        }
        break;

    case M2P_MSG_BATTERY_CHARGE_ONLINE:
        g_charge_online_flag = msg[1];
        log_info("charge online: %d", g_charge_online_flag);
        break;

    case M2P_MSG_BATTERY_CHECK:
        if (vbat_slow_timer == 0) {
            vbat_slow_timer = usr_timer_add(NULL, vbat_check_slow, VBAT_UPDATE_FAST_TIME, 0);
        } else {
            usr_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
        }
        if (vbat_fast_timer == 0) {
            vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_INTERVAL, 0);
        }
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
        usr_timer_add(NULL, vbat_calculate_loop, VBAT_UPDATE_SEC * 1000, 0);
#endif
        log_info("battery timer started");
        break;
    }
}
REGISTER_M2P_MSG_HANDLER(0, MSG_BATTERY, battery_main_msg_handler);

void battery_recv_config(struct battery_config *cfg)
{
    g_poweroff_voltage = cfg->poweroff_voltage;
    g_warning_voltage  = cfg->warning_voltage;
    battery_curve_max  = 0;
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
    g_cc_val           = cfg->cc_val;
    g_cc_progi_volt    = cfg->cc_progi_volt;
#endif
    /* 记录 VM 数据, 待曲线接收完毕后使用 */
    g_vm_percent = cfg->vm_percent;
    g_cur_ts     = cfg->cur_ts;
    g_vm_ts      = cfg->vm_ts;

#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_CURRENT_INTEGRATING)
    g_charge_poweron_en = cfg->charge_poweron_en;
#endif

    log_info("cfg: poff=%dmV warn=%dmV vm_p=%d cur_ts=%d vm_ts=%d poweron=%d",
             g_poweroff_voltage, g_warning_voltage,
             g_vm_percent, g_cur_ts, g_vm_ts, g_charge_poweron_en);
}

static void vbat_timer_delete(void)
{
    if (vbat_slow_timer) {
        usr_timer_del(vbat_slow_timer);
        vbat_slow_timer = 0;
    }
    if (vbat_fast_timer) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }
}

static void power_warning_timer(void *priv)
{
    battery_report_to_main(M2P_MSG_BATTERY_REPORT);
}

static void vbat_check_slow(void *priv)
{
    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_INTERVAL, 0);
    }
    if (g_charge_online_flag) {
        usr_timer_modify(vbat_slow_timer, VBAT_UPDATE_SLOW_TIME);
    } else {
        usr_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
    }
}

static void vbat_check(void *priv)
{
    u16 tmp_percent;
    bat_voltage += get_vbat_voltage();
    unit_cnt++;
    if (unit_cnt < VBAT_DETECT_CNT) {
        return;
    }
    unit_cnt = 0;

    //更新电池电压,以及电池百分比,还有电池等级
    cur_battery_voltage = bat_voltage / VBAT_DETECT_CNT;
    bat_voltage = 0;
    tmp_percent = battery_calc_percent(cur_battery_voltage);
#if (VBAT_PERCENT_MODE_SEL == VBAT_PERCENT_MODE_OPEN_CIRCUIT_VOLTAGE)
    if (g_charge_online_flag) {
        if (tmp_percent > cur_battery_percent) {
            cur_battery_percent++;
        }
    } else {
        if (tmp_percent < cur_battery_percent) {
            cur_battery_percent--;
        }
    }
    if (abs(cur_battery_percent - last_present_k) >= BATTERY_CHANGE_SAVE) {
        last_present_k = cur_battery_percent;
        need_storage = 1;   // 通知主系统写VM
    }
#endif
    cur_battery_level = battery_value_to_phone_level();
    if (!g_charge_online_flag) {
        if (cur_battery_voltage <= g_poweroff_voltage) {
            low_power_cnt++;
            low_voice_cnt    = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_LOWPOWER;
            if (low_power_cnt >= 6) {
                log_info("*** LOW POWER ***");
                low_power_cnt = 0;
                battery_report_to_main(M2P_MSG_BATTERY_REPORT);
                if (lowpower_timer) {
                    usr_timer_del(lowpower_timer);
                    lowpower_timer = 0;
                }
                usr_timer_del(vbat_fast_timer);
                vbat_fast_timer = 0;
            }
        } else if (cur_battery_voltage <= g_warning_voltage) {
            low_voice_cnt++;
            low_power_cnt    = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_WARNING;
            log_info("** Low Power, Please Charge! **");
            if ((low_voice_first_flag && low_voice_cnt > 1) || //第一次进低电10s后报一次
                (!low_voice_first_flag && low_voice_cnt >= 5)) {
                low_voice_first_flag = 0;
                low_voice_cnt = 0;
                if (lowpower_timer == 0) {
                    battery_report_to_main(M2P_MSG_BATTERY_REPORT);
                    lowpower_timer = usr_timer_add(NULL, power_warning_timer, LOW_POWER_WARN_TIME, 0);
                }
            }
        } else {
            power_normal_cnt++;
            low_voice_cnt = 0;
            low_power_cnt = 0;
            if (power_normal_cnt >= 2) {
                if (cur_bat_st != VBAT_NORMAL) {
                    log_info("[Normal Power]");
                    cur_bat_st = VBAT_NORMAL;
                    battery_report_to_main(M2P_MSG_BATTERY_REPORT);
                    if (lowpower_timer) {
                        usr_timer_del(lowpower_timer);
                        lowpower_timer = 0;
                    }
                }
            }
        }
    } else {
        low_voice_cnt    = 0;
        low_power_cnt    = 0;
        power_normal_cnt = 0;
        cur_bat_st = VBAT_NORMAL;
        if (lowpower_timer) {
            usr_timer_del(lowpower_timer);
            lowpower_timer = 0;
        }
    }

    if (cur_bat_st != VBAT_LOWPOWER) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }

    if ((cur_battery_level != old_battery_level) ||
        (cur_battery_percent != old_battery_percent) ||
        (g_charge_online_flag != old_charge_online_flag) ||
        (need_storage)) {
        battery_report_to_main(M2P_MSG_BATTERY_REPORT);
        old_battery_level      = cur_battery_level;
        old_battery_percent    = cur_battery_percent;
        old_charge_online_flag = g_charge_online_flag;
        need_storage = 0;   // 清除标志
    }
    log_debug("V:%dmV P:%d%% Lv:%d St:%d CHG:%d",
              cur_battery_voltage, cur_battery_percent, cur_battery_level, cur_bat_st, g_charge_online_flag);
}

