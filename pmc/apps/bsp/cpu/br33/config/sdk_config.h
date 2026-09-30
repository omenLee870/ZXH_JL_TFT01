#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef PMC_APP_CASE_WATCH
#include "msys_sdk_config.h"
#endif
#include "msys_pmc_app.h"

//==========================================================//
//                      串口打印使能开关                    //
//==========================================================//
#ifdef CONFIG_RELEASE_ENABLE
#define CONFIG_UART_DEBUG_ENABLE  				0
#else
#define CONFIG_UART_DEBUG_ENABLE  				1
#endif
#define CONFIG_DEBUG_UART_TX_PIN                IO_PORTD_01
#define CONFIG_DEBUG_UART_BAUD    		        1000000L

#define TCFG_EXCEPTION_CODE_AT_FLASH            1 // 异常代码放FLASH

//==========================================================//
//                  AEC数据导出UART配置                     //
// 参照 uartPcmSender.h 中 PCM_UART1_TX_PORT/BAUDRATE 模式  //
// 可配置为 PMC_UART0 或 PMC_UART1，注意:                   //
// 1、TX 引脚不要和 CONFIG_DEBUG_UART_TX_PIN 冲突           //
// 2、关闭q32s的串口打印使能                                //
//==========================================================//
#define CONFIG_AEC_UART_EXPORT_ENABLE        0               /* AEC数据导出总开关 */
#define CONFIG_AEC_UART_EXPORT_UART          PMC_UART1       /* 数据导出使用的UART实例 */
#define CONFIG_AEC_UART_EXPORT_TX_PIN        IO_PORTD_00     /* 数据导出TX引脚 */
#define CONFIG_AEC_UART_EXPORT_BAUD          1000000L        /* 数据导出波特率 */

//==========================================================//
//                      PMU 调试版本使能                    //
//==========================================================//
#ifdef CONFIG_RELEASE_ENABLE
#define CONFIG_PMU_DEBUG_ENABLE					0
#else
#define CONFIG_PMU_DEBUG_ENABLE					1
#endif


//==========================================================//
//                 		VAD使能开关                   		//
//==========================================================//
#if defined(PMC_APP_CASE_WATCH) && (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
#define CONFIG_VAD_ENABLE          				1
#else
#define CONFIG_VAD_ENABLE          				0
#endif


//==========================================================//
//                 		定时器支持(usrtimer)              	//
//==========================================================//
#define TIMER_POOL_NUM_CONFIG   				10 //同时存在的最大定时器数量



//==========================================================//
//                 		SENSOR驱动                      	//
//==========================================================//
//★ 运动健康相关开关默认关闭（交付态保持 0），需要时按功能逐项打开
//运动健康管理模块（总开关，0 = 关闭整条 sensor 链路）
#define CONFIG_SPORT_HEALTH_MANAGER_ENABLE	0

//common
#define CONFIG_SENSOR_DRIVER_ENABLE         CONFIG_SPORT_HEALTH_MANAGER_ENABLE
#define TCFG_SENSOR_HUB						CONFIG_SPORT_HEALTH_MANAGER_ENABLE

#define CONFIG_SPORT_HEALTH_MANAGER_DEBUG	0//数据模拟 GSENSOR BIT(0) HEART BIT(1) SPO2  BIT(2)（pmc_gsensor 心率血氧模拟打通）

#define CONFIG_SPORT_HEALTH_UPDATE_PERIOD	1000//pmc算法数据更新节奏ms，默认1s（与大核 TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS 对应）
#define CONFIG_SPORT_HEALTH_GSENSOR_DIR_PERIOD	1000//gsensor方向更新周期ms，默认1s（bedside用）
#define CONFIG_SENSOR_DRIVER_TIMER_PERIOD	200 //sensor FIFO取数周期ms，默认200

#define TCFG_SENSOR_IIC_HW_EN				CONFIG_SPORT_HEALTH_MANAGER_ENABLE //硬件iic使能

#define CONFIG_SENSOR_SLEEP_ENABLE        	0 //自动让SENSOR睡眠，需要SENSOR支持唤醒检测并IO中断唤醒MCU

//MODULE
#define TCFG_SENSOR_ALGO_GSENSOR_ENABLE		0
#define TCFG_SENSOR_ALGO_HEART_RATE_EN		0
#define TCFG_SENSOR_ALGO_SPO2_ENABLE		0
#define TCFG_SENSOR_DET_WRIST_ENABLE		0
#define TCFG_SENSOR_DET_WEAR_ENABLE			0

//sample
#define TCFG_SENSOR_HR_SAMPLE_ENABLE		0	//心率后台周期采数
#define TCFG_SENSOR_SPO2_SAMPLE_ENABLE		0	//血氧后台周期采数

//service
#define TCFG_SENSOR_SERVICE_MOTION_ENABLE	TCFG_SENSOR_ALGO_GSENSOR_ENABLE
#define TCFG_SENSOR_SERVICE_WRITE_ENABLE	(TCFG_SENSOR_ALGO_GSENSOR_ENABLE&&TCFG_SENSOR_DET_WRIST_ENABLE)//抬腕检测（jl_motion_run 内嵌 wrist_raise）
#define TCFG_SENSOR_SERVICE_VC_HR_SPO2_EN	0	//VCHR11 旧方案专用；本项目 HR/SPO2/wear 均由 VC30FX 提供
#define TCFG_SENSOR_SERVICE_WEAR_ENABLE		TCFG_SENSOR_DET_WEAR_ENABLE

//hal
#define TCFG_SENSOR_HAL_ACCELER_ENABLE		TCFG_SENSOR_ALGO_GSENSOR_ENABLE
#define TCFG_SENSOR_HAL_VCHR11_ENABLE		0

//driver
#define TCFG_SC7A20_ENABLE                	TCFG_SENSOR_ALGO_GSENSOR_ENABLE

#define TCFG_MMC5603_ENABLE               	0

/* HR 与 SPO2 共用 VC30FX 一颗传感器：任一算法开启都要把驱动编进来，不能只跟 HR
 * ——否则「只开血氧」时驱动缺失、上层 service 仍在，链路静默失效 */
#define TCFG_VC30fx_ENABLE               	(TCFG_SENSOR_ALGO_HEART_RATE_EN || TCFG_SENSOR_ALGO_SPO2_ENABLE)
#define TCFG_VCHR11S_ENABLE               	0
#define TCFG_HRS3602_ENABLE               	0
#define TCFG_HR_SENSOR_READ_BY_INT        	0

/*==========================================================*/
/*          心率传感器取数方式（二选一）                     */
/*  0 = 定时器取数（默认）：小核 usr_timer 按 FIFO 周期轮询  */
/*  1 = IO 中断取数：VC30FX INT 引脚下降沿唤醒（当前无实现） */
/*==========================================================*/
#define TCFG_HR_SENSOR_SAMPLE_BY_IO_INT   	0


/*==========================================================*/
/*        SENSOR 代码段放置（RAM / Flash）配置               */
/*  段名只有 .sensor_code.L1(RAM) / .sensor_code.L2(Flash)， */
/*    L0 = 全部 RAM                                          */
/*    L1 = 取数/算法接口 RAM，初始化 Flash                   */
/*    L2 = 取数接口 RAM，初始化/算法 Flash（默认档）          */
/*    L3 = 全部 Flash                                        */
/*==========================================================*/

/* gsensor 链路（含 sensor common 4 文件）代码段放置档位 */
#define TCFG_SENSOR_GSENSOR_LEVEL          2

/* hrsensor 链路代码段放置：0 = 全部 Flash（默认）；1 = 全部 RAM
 * vc30fx 驱动+算法/sensor_hal_vc30fx/sensor_service_hr_vc30fx/
 * shm_algo_heart_rate/shm_algo_spo2；全 RAM 约增 13KB code
 */
#define TCFG_SENSOR_HR_CODE_IN_RAM        	0


//==========================================================//
//                 		soft iic                        	//
//==========================================================//
#define TCFG_SW_I2C0_CLK_PORT      IO_PORTB_02  //spft IIC CLK
#define TCFG_SW_I2C0_DAT_PORT      IO_PORTB_01  //spft IIC DAT
#define TCFG_SW_I2C0_DELAY_CNT     1           //软件IIC延时参数，影响通讯时钟频率



//========================================================== //
//                 		hw  iic                   	        //
//==========================================================//
#define PMC_HW_IIC_EN			    CONFIG_SPORT_HEALTH_MANAGER_ENABLE
#define PMC_HW_IIC_SCL				IO_PORTD_10
#define PMC_HW_IIC_SDA				IO_PORTD_09
#define	PMC_HW_IIC_FREQ				(400*1000)
#define PMC_HW_IIC_PU_EN			1

//===========================================================//
//                     lptocuh 配置                          //
//===========================================================//
#if defined(PMC_APP_CASE_WATCH) && (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
#define TCFG_LPTOUCH_ENABLE         1   //触摸总开关
#else
#define TCFG_LPTOUCH_ENABLE         0   //触摸总开关
#endif
#define LPTOUCH_CH0_ENABLE          0   //LPTOUCH_CH0使能，固定引脚: PD01
#define LPTOUCH_CH1_ENABLE          0   //LPTOUCH_CH1使能，固定引脚: PD03
#define LPTOUCH_CH2_ENABLE          0   //LPTOUCH_CH2使能，固定引脚: PD05
#define LPTOUCH_CH3_ENABLE          1   //LPTOUCH_CH3使能，固定引脚: PD06
#define LPTOUCH_CH4_ENABLE          1   //LPTOUCH_CH4使能，固定引脚: PD08
#define LPTOUCH_CH5_ENABLE          1   //LPTOUCH_CH5使能，固定引脚: PD09
#define LPTOUCH_CH6_ENABLE          0   //LPTOUCH_CH6使能，固定引脚: PD11
#define LPTOUCH_CH7_ENABLE          0   //LPTOUCH_CH7使能，固定引脚: PE03


#endif
