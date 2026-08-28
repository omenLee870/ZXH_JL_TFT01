#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "msys_sdk_config.h"
#include "msys_pmc_app.h"

//==========================================================//
//                      串口打印使能开关                    //
//==========================================================//
#define CONFIG_UART_DEBUG_ENABLE  				1
#define CONFIG_DEBUG_UART_TX_PIN                IO_PORTD_01
#define CONFIG_DEBUG_UART_BAUD    		        1000000L

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
#define CONFIG_PMU_DEBUG_ENABLE					1


//==========================================================//
//                 		VAD使能开关                   		//
//==========================================================//
#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
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
//运动健康管理模块
#define CONFIG_SPORT_HEALTH_MANAGER_ENABLE	0

//common
#define CONFIG_SENSOR_DRIVER_ENABLE         CONFIG_SPORT_HEALTH_MANAGER_ENABLE
#define TCFG_SENSOR_HUB						CONFIG_SPORT_HEALTH_MANAGER_ENABLE
#define CONFIG_SPORT_HEALTH_MANAGER_DEBUG	0//BIT(0)|BIT(1)|BIT(2)//数据模拟 GSENSOR BIT(0) HEART BIT(1) SPO2  BIT(2)
#define TCFG_SENSOR_IIC_HW_EN				1 //硬件iic使能
#define CONFIG_SENSOR_SLEEP_ENABLE        	0 //自动让SENSOR睡眠，需要SENSOR支持唤醒检测并IO中断唤醒MCU
//MODULE
#define TCFG_SENSOR_ALGO_GSENSOR_ENABLE		1
#define TCFG_SENSOR_ALGO_HEART_RATE_EN		0
#define TCFG_SENSOR_ALGO_SPO2_ENABLE		0
#define TCFG_SENSOR_DET_WRIST_ENABLE		0
#define TCFG_SENSOR_DET_WEAR_ENABLE			0
//service
#define TCFG_SENSOR_SERVICE_MOTION_ENABLE	TCFG_SENSOR_ALGO_GSENSOR_ENABLE
#define TCFG_SENSOR_SERVICE_WRITE_ENABLE	0//(TCFG_SENSOR_ALGO_GSENSOR_ENABLE&&TCFG_SENSOR_DET_WRIST_ENABLE)	//缺算法
#define TCFG_SENSOR_SERVICE_VC_HR_SPO2_EN	(TCFG_SENSOR_ALGO_HEART_RATE_EN	||TCFG_SENSOR_ALGO_SPO2_ENABLE)
#define TCFG_SENSOR_SERVICE_WEAR_ENABLE		TCFG_SENSOR_DET_WEAR_ENABLE
//hal
#define TCFG_SENSOR_HAL_ACCELER_ENABLE		TCFG_SENSOR_ALGO_GSENSOR_ENABLE
#define TCFG_SENSOR_HAL_VCHR11_ENABLE		(TCFG_SENSOR_ALGO_HEART_RATE_EN	||TCFG_SENSOR_ALGO_SPO2_ENABLE)

//driver
#define TCFG_SC7A20_ENABLE                	TCFG_SENSOR_ALGO_GSENSOR_ENABLE

#define TCFG_MMC5603_ENABLE               	0

#define TCFG_VC30fx_ENABLE               	0
#define TCFG_VCHR11S_ENABLE               	0
#define TCFG_HRS3602_ENABLE               	0
#define TCFG_HR_SENSOR_READ_BY_INT        	0


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
#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_AI_GLASSES)
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
