
#pragma once

#include "includes.h"
#include "sensor_driver.h"

// VC30FX设备类型定义
#define VC30FX_DEV_TYPE_HR       SENSOR_DRV_HR
#define VC30FX_DEV_TYPE_SPO2     SENSOR_DRV_SPO2
#define VC30FX_DEV_TYPE_HR_SPO2  SENSOR_DRV_HR_SPO2

// VC30FX原始数据结构
typedef struct {
    u16 ppg_data[3][64];     // 3个Slot的PPG数据
    u16 ps_data[3];          // 3个Slot的PS数据
    u16 bio_data[2];         // BIO内部/外部数据
    u16 temp_data[3];        // 温度数据(内部/外部/参考)
    u8  ppg_num;             // PPG数据数量
    u8  wear_status;         // 佩戴状态
    u8  slot_enable;         // Slot使能位图
} vc30fx_raw_data_t;

// VC30FX配置参数
typedef struct {
    u8 work_mode;            // 工作模式(HR/SPO2/HR_SPO2)
    u16 ppg_freq;            // PPG采样频率
    u8 ps_freq_div;          // PS分频系数
    u8 fifo_freq_div;        // FIFO分频系数
    u8 slot0_current;        // Slot0电流
    u8 slot0_resistance;     // Slot0电阻
    u8 slot1_current;        // Slot1电流
    u8 slot1_resistance;     // Slot1电阻
    u8 slot2_current;        // Slot2电流
    u8 slot2_resistance;     // Slot2电阻
} vc30fx_config_t;

// Driver层接口函数声明
s8 vc30fx_driver_open(u16 range, u16 odr);
s8 vc30fx_driver_close(void);
s8 vc30fx_driver_run(void *arg, u16 *len);
s8 vc30fx_driver_online(void);
s8 vc30fx_driver_sleep(void);

// Driver层数据获取接口
u16 vc30fx_driver_get_raw_data(vc30fx_raw_data_t *data);
u8 vc30fx_driver_get_wear_status(void);
void vc30fx_driver_irq_handler(void);
