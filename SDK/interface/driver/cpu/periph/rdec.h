#ifndef _RDEC_H
#define _RDEC_H

#include "asm/rdec_hw.h"

#define RDEC_FREQ   (1000*16)//预设采样频率
#define RDEC_TIME   10000 //预设定时模式定时周期 单位:us

typedef enum : u8 {
    RDEC_PHASE_1 = 0,
    RDEC_PHASE_2,
    RDEC_PHASE_4,
} rdec_phase_mode;

typedef enum : u8 {
    RDEC_WORK_AUTO = 0, //自动增减
    RDEC_WORK_TIMER, //定时模式
    RDEC_WORK_INTR, //中断模式
} rdec_work_mode;

typedef void (*rdec_irq_callback)(u32 id); //回调函数

struct rdec_config {
    u32 rdec_a; //编码器A相
    u32 rdec_b; //编码器B相
    rdec_phase_mode phase_mode; //编码器模式
    rdec_work_mode work_mode; //工作模式
    rdec_dev rdec_x;//硬件模块选择
    u8 irq_priority;
    rdec_irq_callback irq_cb;
};

u32 rdec_init(const struct rdec_config *cfg);
u32 rdec_deinit(u32 id);
u32 rdec_start(u32 id);
u32 rdec_pause(u32 id);
u32 rdec_resume(u32 id);
int rdec_get_value(u32 id);
void rdec_dump();

#endif

