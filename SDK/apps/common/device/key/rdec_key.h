#ifndef RDEC_KEY_H
#define RDEC_KEY_H

#include "typedef.h"
#include "key_driver.h"
#include "gpio.h"
#include "rdec.h"

/*目前rdec模块是1个*/
#define CONFIG_RDEC_KEY_MAX_NUM  1

struct rdec_device {
    u8 index;                           //编码器序号
    u32 sin_portA; 	                    //采样信号端口A
    enum gpio_mode sin_portA_io_mode;
    u32 sin_portB; 	                    //采样信号端口B
    enum gpio_mode sin_portB_io_mode;
    int scan_time;                      //扫描频率, 单位: ms
    rdec_phase_mode phase_mode;         //编码器分辨率
    int (*get_value)();                 //rdec获取单位时间内转动步数值 目前步数值是8位符号数
};

struct rdec_platform_data {
    u8 enable;                      //是否使能rdec
    u8 num; 	                    //rdec数量
    const struct rdec_device *rdec; //rdec参数表
};


int rdec_key_init(void);

#endif

