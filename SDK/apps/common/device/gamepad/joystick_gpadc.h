/**@file        joystick_gpadc.h
  * @brief      本机左右摇杆 GPADC 采集（br33）
  * @details    用 GPADC + DMA 连续采集 4 路摇杆电位器，换算成 XInput 用的
  *             s16 轴值（-32768~32767，0 居中）。
  *             本模块是 gamepad_input 汇聚层的一个输入源（GAMEPAD_SRC_LOCAL）：
  *             初始化时向汇聚层注册取数函数，之后由汇聚层按 USB 帧驱动；
  *             没被选中时不会被调用，ADC 采集也会自然停下来。
  * @note       本模块与 SDK 原有的 lib/driver/gpadc/gpadc.c 硬件互斥
  *             （同一个 IRQ_GPADC_IDX、同一组 ADC 寄存器），不能同时使用。
  */
#ifndef __JOYSTICK_GPADC_H__
#define __JOYSTICK_GPADC_H__

#include "typedef.h"

/**@brief   初始化摇杆采集：复用 4 个 IO 到 GPADC、配置并启动 DMA
  * @note    需在 USB 从机启动之前调用。重复调用无副作用。
  * @return 0 成功，非 0 失败
  */
int joystick_init(void);

/**@brief   取 4 个轴的当前值（已换算、已过死区）
  * @note    可在中断上下文调用：只读 4 个 u16 并做整数换算，不加锁。
  *          允许传入 NULL 表示不需要该轴。
  * @param[out] *lx 左摇杆 X，-32768~32767，0 居中
  * @param[out] *ly 左摇杆 Y
  * @param[out] *rx 右摇杆 X
  * @param[out] *ry 右摇杆 Y
  * @return 无
  */
void joystick_get_axis(s16 *lx, s16 *ly, s16 *rx, s16 *ry);

/**@brief   触发下一轮 ADC 转换
  * @note    GPADC 的 DMA 不会自动循环触发（CONT=0 / DISCEN=0），每轮序列都要
  *          软件触发一次。约定在「上一轮数据被取走之后」调用，调用方按自己的
  *          采样节奏来（本项目跟随 USB 帧）。内部自带"上一轮未完成则跳过"的
  *          保护，重复调用安全。可在中断上下文调用。
  * @return 无
  */
void joystick_scan(void);

/**@brief   低功耗挂起/恢复时停/开 DAC 采样，由电源管理钩子调用
  * @return 无
  */
void joystick_suspend(void);
void joystick_resume(void);

/**@brief   联调用：打印 4 路原始值（不要在中断里调）
  * @return 无
  */
void joystick_dump_raw(void);

#endif  /* __JOYSTICK_GPADC_H__ */
