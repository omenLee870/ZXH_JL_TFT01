#ifndef __BATTERY_PMC_H__
#define __BATTERY_PMC_H__

#include "typedef.h"
#include "main.h"

/**
 * @brief 电池曲线点
 */
struct battery_curve {
    u8  percent;    ///< 百分比 0-100
    u16 voltage;    ///< 电压 mV
};

/**
 * @brief 主系统→PMC 配置数据
 */
struct battery_config {
    u16 poweroff_voltage;               ///< 关机电压 mV
    u16 warning_voltage;                ///< 提醒电压 mV
    u16 cc_val;                         ///< 配置恒流电流 0xFFFF为无效值
    u16 cc_progi_volt;                  ///< 基准恒流PROGI电压 0xFFFF为无效值
    u8  vm_percent;                     ///< VM 存储的电量, 0xFF=无效
    u8  charge_poweron_en;              ///< 开机充电使能 0=关 1=开
    u32 cur_ts;                         ///< 当前 RTC UTC 时间(秒)
    u32 vm_ts;                          ///< VM 写入时的 RTC UTC 时间(秒)
} __attribute__((packed));

/**
 * @brief PMC→主系统 状态上报
 */
struct battery_report {
    u8  percent;        ///< 电量百分比 0-100
    u8  level;          ///< 电量等级 0-9
    u8  status;         ///< 0=正常 1=低电提醒 2=低电关机
    u8  charging;       ///< 0=未充电 1=充电中
    u8  need_storage;   ///< 1=需要主系统写VM, 0=无需
} __attribute__((packed));

/**
 * @brief BR33→PMC: 推送充满标志
 */
struct battery_full_flag {
    u8 is_full;             ///< 0=未满 1=已满
} __attribute__((packed));

u8 get_vbat_percent(void);
void battery_recv_config(struct battery_config *cfg);

#endif
