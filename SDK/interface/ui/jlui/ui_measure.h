/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_measure.h
 *
 * @brief 杰理UI性能测试API
 *
 * @author
 *
 * @version V1.0.0
 *
 * @date 2024-06-13
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_MEASURE_H__
#define __UI_MEASURE_H__

#include "generic/typedef.h"
#include "gpio.h"
// #include "asm/gpio_hw.h"


// UI各功能测试模块使能
#define UI_MEASURE_ENABLE	0


#define PORT(i,a,b) (u16)((i<<8)|((a-'A')<<4)|(b))


// 各测试模块IO定义
#define IO_GPU      IO_PORTB_00 //PORT(6,'B',0)
#define IO_DBI      IO_PORTB_01 //PORT(6,'B',1)
#define IO_MELD     //IO_PORTB_03 //PORT(6,'B',2)
#define IO_REDRAW	IO_PORTB_05

#define IO_FRAME     IO_PORTB_04//PORT(0,'B',4)
#define IO_FLASH    //PORT(1,'B',5)
#define IO_CPU      IO_PORTB_04//PORT(6,'B',6)
#define IO_INERTIA  IO_PORTB_05//PORT(6,'B',6)


#define LOW         0
#define HIGH        1


// extern int gpio_hw_direction_output(const enum gpio_port port, u32 pin, const int value);

#define MEASURE_IO_SET(io, level)	gpio_hw_direction_output(IO_PORT_SPILT(io), (level))


#if (defined UI_MEASURE_ENABLE && UI_MEASURE_ENABLE)
// 开测试功能时，使用宏定义函数拉IO
#define IO_FRAME_HIGH()		MEASURE_IO_SET(IO_FRAME, HIGH)
#define IO_FRAME_LOW()		MEASURE_IO_SET(IO_FRAME, LOW)

#define IO_REDRAW_HIGH()	MEASURE_IO_SET(IO_REDRAW, HIGH)
#define IO_REDRAW_LOW()		MEASURE_IO_SET(IO_REDRAW, LOW)

#define IO_FLASH_HIGH()		MEASURE_IO_SET(IO_FLASH, HIGH)
#define IO_FLASH_LOW()		MEASURE_IO_SET(IO_FLASH, LOW)

#define IO_GPU_HIGH()		MEASURE_IO_SET(IO_GPU, HIGH)
#define IO_GPU_LOW()		MEASURE_IO_SET(IO_GPU, LOW)

#define IO_DBI_HIGH()		MEASURE_IO_SET(IO_DBI, HIGH)
#define IO_DBI_LOW()		MEASURE_IO_SET(IO_DBI, LOW)

#define IO_CPU_HIGH()		MEASURE_IO_SET(IO_CPU, HIGH)
#define IO_CPU_LOW()		MEASURE_IO_SET(IO_CPU, LOW)

#define IO_MELD_HIGH()		MEASURE_IO_SET(IO_MELD, HIGH)
#define IO_MELD_LOW()		MEASURE_IO_SET(IO_MELD, LOW)

#define IO_INERTIA_HIGH()		MEASURE_IO_SET(IO_INERTIA, HIGH)
#define IO_INERTIA_LOW()		MEASURE_IO_SET(IO_INERTIA, LOW)

#else
// 不开测试功能时，用空的宏，不影响源代码
#define IO_FRAME_HIGH()
#define IO_FRAME_LOW()

#define IO_REDRAW_HIGH()
#define IO_REDRAW_LOW()

#define IO_FLASH_HIGH()
#define IO_FLASH_LOW()

#define IO_GPU_HIGH()
#define IO_GPU_LOW()

#define IO_DBI_HIGH()
#define IO_DBI_LOW()

#define IO_CPU_HIGH()
#define IO_CPU_LOW()

#define IO_MELD_HIGH()
#define IO_MELD_LOW()

#endif


void ui_io_set(u16 io, u8 level);

#endif
