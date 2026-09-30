#ifndef _SENSOR_SEG_H_
#define _SENSOR_SEG_H_

#include "sdk_config.h"

/*
 * sensor 代码段标注宏（函数级）
 *
 * 段名只有两个：
 *   .sensor_code.L1  →  p11_ram1  (RAM)
 *   .sensor_code.L2  →  code0     (Flash)
 * TCFG_SENSOR_GSENSOR_LEVEL 是配置档位，不是段名。
 *
 * 函数级 AT() 优先级高于文件级 #pragma code_seg。
 * 每个函数显式标注 COLD / FIFO / ALGO 三类之一：
 *   COLD  初始化/enable/set/close/check/printf   L0:RAM    L1~L3:Flash
 *   FIFO  轮询/定时器/IIC/调度/取数公共接口      L0~L2:RAM L3:Flash
 *   ALGO  算法本体与算法调度（1s 路径）          L0/L1:RAM L2/L3:Flash
 */

/* COLD：初始化/设置类 */
#if TCFG_SENSOR_GSENSOR_LEVEL == 0
#define AT_SENSOR_COLD
#else
#define AT_SENSOR_COLD      AT(.sensor_code.L2)
#endif

/* FIFO：轮询/定时器/IIC/调度/取数路径公共接口 */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
#define AT_SENSOR_FIFO
#else
#define AT_SENSOR_FIFO      AT(.sensor_code.L2)
#endif

/* ALGO：算法与算法调度 */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 1
#define AT_SENSOR_ALGO
#else
#define AT_SENSOR_ALGO      AT(.sensor_code.L2)
#endif

/* gsensor 侧旧宏名，保留为别名 */
#define AT_GSENSOR_COLD     AT_SENSOR_COLD
#define AT_GSENSOR_FIFO     AT_SENSOR_FIFO
#define AT_GSENSOR_ALGO     AT_SENSOR_ALGO

/*
 * sensor FIFO 缓冲余量（编译期配置，静态 RAM 分配）
 *
 * REQ-002：gsensor / hrsensor 的 FIFO 缓存 buf 需预留 1.2 ~ 1.5 倍余量，
 * 默认 1.2 倍（NUM/DEN = 12/10），需要时可改为 15/10（1.5 倍）。
 * 必须用编译期宏实现：余量按静态数组尺寸落地，运行期扩容不可行。
 *
 * 用法：static axis_data_t accel_data[SENSOR_FIFO_DEPTH(50)];
 * 各 driver 只写基准帧数，余量系数统一在此调整（禁魔鬼数字）。
 */
#ifndef SENSOR_FIFO_RESERVE_NUM
#define SENSOR_FIFO_RESERVE_NUM     12  /* 余量分子，默认 12（1.2 倍） */
#endif
#ifndef SENSOR_FIFO_RESERVE_DEN
#define SENSOR_FIFO_RESERVE_DEN     10  /* 余量分母，默认 10（1.2 倍） */
#endif

/* 向上取整：base * NUM / DEN，避免整数截断导致余量不足 */
#define SENSOR_FIFO_DEPTH(base) \
    (((base) * SENSOR_FIFO_RESERVE_NUM + SENSOR_FIFO_RESERVE_DEN - 1) / SENSOR_FIFO_RESERVE_DEN)

#endif /* _SENSOR_SEG_H_ */
