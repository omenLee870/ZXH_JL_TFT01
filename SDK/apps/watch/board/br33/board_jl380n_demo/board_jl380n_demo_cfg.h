#ifndef CONFIG_BOARD_JL380N_DEMO_CFG_H
#define CONFIG_BOARD_JL380N_DEMO_CFG_H

#include "board_jl380n_demo_global_build_cfg.h"

#ifdef CONFIG_BOARD_JL380N_DEMO

//*********************************************************************************//
//                                 配置开始                                        //
//*********************************************************************************//


//*********************************************************************************//
//                                 FLASH 配置                                      //
//*********************************************************************************//
//FLASH公共配置
#if TCFG_USER_BLE_ENABLE
#define TCFG_FLASH_ERASE_WAIT_BLE			ENABLE			//flash擦除前等待ble状态
#else
#define TCFG_FLASH_ERASE_WAIT_BLE			DISABLE        //flash擦除前等待ble状态
#endif

//外挂flash配置
#define TCFG_NORFLASH_SFC_DEV_ENABLE        TCFG_NORFLASH_DEV_ENABLE         //外挂flash驱动

#if TCFG_NORFLASH_SFC_DEV_ENABLE
#define TCFG_EX_FLASH_POWER_IO				IO_PORT_LDO28   // 外置flash电源脚
#define CONFIG_EX_FLASH_POWER_IO			LDO28   // 外置flash电源脚缩写: 普通IO(如PC08)/SDPG/LDO28/NULL(不接IO)，默认PC08供电
#define CONFIG_EX_FLASH_POWER_IO_CTRL		31	// LDO28供电时=电压值(如3.1V对应31)；其他IO供电时=控制电平(0:低电平 1:高电平)

// 双外挂 flash（第二个外挂 flash）使能开关
#define TCFG_NORFLASH_SFC_DEV1_ENABLE		0

#if TCFG_NORFLASH_SFC_DEV1_ENABLE
// 第二个 flash 的 CS 引脚（查硬件手册）
//   Port A: CS1=PC15 Port B: CS1=PB11
#define CONFIG_EX_FLASH_1_CS_PIN			PC15
// SPI 总线和端口组（必须与 flash0 的总线/端口一致）
#define CONFIG_EX_FLASH_1_SPI_BUS_PORT		1A
// 第二个 flash 的供电 IO（通常与 flash0 共享 ）
#define CONFIG_EX_FLASH_1_POWER_IO			CONFIG_EX_FLASH_POWER_IO
// 第二个 flash 的供电电平（通常与 flash0 共享 ）
#define CONFIG_EX_FLASH_1_POWER_IO_CTRL		CONFIG_EX_FLASH_POWER_IO_CTRL

// Flash0 文件配置（ [EX_FLASH_0] 段）
#define CONFIG_EX_FLASH_BIN_FILE				mode.bin
#define CONFIG_EX_FLASH_FLASH_ADDR				0
#define CONFIG_EX_FLASH_FLASH_SIZE				16M
#define CONFIG_EX_FLASH_FLASH_OPT				1
// Flash1 文件配置（ [EX_FLASH_1] 段）
#define CONFIG_EX_FLASH_1_BIN_FILE				ex_watch.bin
#define CONFIG_EX_FLASH_1_FLASH_ADDR			0
#define CONFIG_EX_FLASH_1_FLASH_SIZE			16M
#define CONFIG_EX_FLASH_1_FLASH_OPT			1
#endif
#endif//TCFG_NORFLASH_SFC_DEV_ENABLE

//NANDFLASH配置
#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
//电源配置
#define TCFG_EX_FLASH_POWER_IO				IO_PORTC_11   // 外置flash电源脚
#define CONFIG_EX_FLASH_POWER_IO			PC11 // NULL  // 外置flash电源脚。不接IO时填NULL或者屏蔽掉该宏定义
#define CONFIG_EX_FLASH_POWER_IO_CTRL		1	// 0:power_io低电平时供电;1：高电平时供电	(针对io口低电平控制mos管导通来供电的情况)

#define TCFG_FLASH_DEV_FLASH_READ_WIDTH     SPI_MODE_UNIDIR_4BIT//SPI_MODE_BIDIR_1BIT//
#define TCFG_FLASH_DEV_SPI_HW_NUM			2// 1: SPI1    2: SPI2
#define TCFG_FLASH_DEV_SPI_CS_PORT	    	IO_PORTC_01

// NANDFLASH驱动配置
#define TCFG_NANDFLASH_HEX_ID				0xc891	// flash ID, 十六进制值的字符串形式
#define TCFG_NANDFLASH_PLANE_NUMBER		    1
#define TCFG_NANDFLASH_ECC_MASK				0x30
#define TCFG_NANDFLASH_ECC_ERR				0x20
#define TCFG_NANDFLASH_PLANE_SEL			0
#define TCFG_NANDFLASH_WRITE_ENABLE_POS		0
#define TCFG_NANDFLASH_QUAD_MODE_DUM_NUM	1
#define TCFG_NANDFLASH_QUAD_MODE_QE			1
#define TCFG_NANDFLASH_BLOCK_NUMBER		    1024
#define TCFG_NANDFLASH_CAPACITY				(128 * 1024 * 1024)	// flash 总实际容量
#define TCFG_NANDFLASH_PAGE_NUM				64
#define TCFG_NANDFLASH_PAGE_SIZE			2048
#define TCFG_NANDFLASH_OOB_SIZE				64
#define TCFG_NANDFLASH_OOB_USER_OFFSET_0	0
#define TCFG_NANDFLASH_OOB_USER_OFFSET_1    0
#define TCFG_NANDFLASH_OOB_USER_SIZE_0		64
#define TCFG_NANDFLASH_OOB_USER_SIZE_1		0
#define TCFG_NANDFLASH_MAX_ERASE_CNT		100000	// 最大擦写次数，一般默认10万次

#if(!TCFG_PSRAM_DEV_ENABLE)
#error "need enable psram!!!"
#endif
#endif//TCFG_NANDFLASH_DEV_ENABLE

//*********************************************************************************//
//                                 FLASH dev分区                                   //
//*********************************************************************************//
//分区配置
#if (!TCFG_UI_ENABLE)//不开UI时，不使用文件系统

#define TCFG_VIRFAT_FLASH_ENABLE					DISABLE		//虚拟文件系统

#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			DISABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		DISABLE 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			DISABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				DISABLE 	//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				DISABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		DISABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

#define TCFG_DATA_DEV_NAME         					"watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE	  			0x1D6000	//
#define TCFG_DATA_FLASH_DEV_PATY_SIZE	  			0x28000 	// 160K

#else//(!TCFG_UI_ENABLE)

//------------------------------------------------------------------------------------------
//ONLY_INSIDE_FLASH//只有内置flash，按8M规划
#if ((!TCFG_NORFLASH_DEV_ENABLE)&&(!TCFG_NANDFLASH_DEV_ENABLE)&&(CONFIG_FLASH_SIZE == 8*1024*1024))

#define TCFG_VIRFAT_FLASH_ENABLE					ENABLE		//虚拟文件系统
#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			ENABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		DISABLE 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			ENABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				DISABLE 	//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				ENABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		DISABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

//sdfile文件系统
#define TCFG_SDFILE_FLASH_DEV_PATY_SIZE	  			0x500000 // 5M
#define TCFG_SDFILE_FLASH_DEV_PATY_BASE	  			0x1fe000 // 2M - 8k开始 内置FLASH虚拟文件系统空间基础地址
//virfat
#define TCFG_VIRFAT_FLASH_DEV_PATY_BASE        		0x6FE000//(TCFG_MODE_INSERT_FLASH_BASE + TCFG_MODE_INSERT_FLASH_SIZE)
#define TCFG_VIRFAT_FLASH_DEV_PATY_SIZE        		0xD8000 //864K
//watch_data
#define TCFG_DATA_DEV_NAME         					"watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE				0x7D6000//(TCFG_WATCH_INSERT_FLASH_BASE + TCFG_WATCH_INSERT_FLASH_SIZE)
#define TCFG_DATA_FLASH_DEV_PATY_SIZE				0x28000 //160K

//------------------------------------------------------------------------------------------
//ONLY_INSIDE_FLASH//只有内置flash，按16M规划
//(***LVGL***)
#elif (CONFIG_LVGL_UI_ENABLE&&(!TCFG_NORFLASH_DEV_ENABLE)&&(!TCFG_NANDFLASH_DEV_ENABLE)&&(CONFIG_FLASH_SIZE == 16*1024*1024))

#define TCFG_VIRFAT_FLASH_ENABLE					ENABLE		//虚拟文件系统
#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			ENABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		DISABLE 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			ENABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				DISABLE 	//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				ENABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		DISABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

//sdfile文件系统
//注：尺寸极限！不能随意增大，工程生成的app_bin的大小为3M左右，若UI资源超过10M，请考虑增加FLASH容量或启用外部FLASH
#define TCFG_SDFILE_FLASH_DEV_PATY_SIZE             0x950000     // 9.3M
#define TCFG_SDFILE_FLASH_DEV_PATY_BASE             0x5af000     //内置FLASH虚拟文件系统空间基础地址
//virfat
#define TCFG_VIRFAT_FLASH_DEV_PATY_BASE        		0xeff000//(TCFG_MODE_INSERT_FLASH_BASE + TCFG_MODE_INSERT_FLASH_SIZE)
#define TCFG_VIRFAT_FLASH_DEV_PATY_SIZE        		0xd8000 //864K
//watch_data
#define TCFG_DATA_DEV_NAME         					"watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE				0xfd7000//(TCFG_WATCH_INSERT_FLASH_BASE + TCFG_WATCH_INSERT_FLASH_SIZE)
#define TCFG_DATA_FLASH_DEV_PATY_SIZE				0x28000 //160K

//------------------------------------------------------------------------------------------
//ONLY_INSIDE_FLASH//只有内置flash，按16M规划
//(***JLUI or others ***)
#elif ((!CONFIG_LVGL_UI_ENABLE)&&(!TCFG_NORFLASH_DEV_ENABLE)&&(!TCFG_NANDFLASH_DEV_ENABLE)&&(CONFIG_FLASH_SIZE == 16*1024*1024))

#define TCFG_VIRFAT_FLASH_ENABLE					ENABLE		//虚拟文件系统
#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			ENABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		DISABLE 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			ENABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				DISABLE 	//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				ENABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		DISABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

//sdfile文件系统
#define TCFG_SDFILE_FLASH_DEV_PATY_SIZE	  			0x700000 // 7M
#define TCFG_SDFILE_FLASH_DEV_PATY_BASE	  			0x300000 // 3M开始 内置FLASH虚拟文件系统空间基础地址
//virfat
#define TCFG_VIRFAT_FLASH_DEV_PATY_BASE        		0xa00000//(TCFG_MODE_INSERT_FLASH_BASE + TCFG_MODE_INSERT_FLASH_SIZE)
#define TCFG_VIRFAT_FLASH_DEV_PATY_SIZE        		0x5D7000 //864K
//watch_data
#define TCFG_DATA_DEV_NAME         					"watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE				0xfd7000//(TCFG_WATCH_INSERT_FLASH_BASE + TCFG_WATCH_INSERT_FLASH_SIZE)
#define TCFG_DATA_FLASH_DEV_PATY_SIZE				0x28000 //160K

//------------------------------------------------------------------------------------------
//EXT_NOR_FLASH//外挂NOR fals
//(***LVGL***)
#elif (CONFIG_LVGL_UI_ENABLE&&TCFG_NORFLASH_DEV_ENABLE)

#define TCFG_VIRFAT_FLASH_ENABLE					ENABLE		//虚拟文件系统
#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			DISABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		ENABLE	 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			DISABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				ENABLE 		//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				ENABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		ENABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

#define TCFG_VIRFAT_FLASH_DEV_PATY_BASE             0x000000
#define TCFG_VIRFAT_FLASH_DEV_PATY_SIZE             0x400000
// sdfile:  mode.bin
#define TCFG_SDFILE_FLASH_DEV_PATY_BASE             0x400000
#define TCFG_SDFILE_FLASH_DEV_PATY_SIZE             0xBD8000
// watch_data:  160K
#define TCFG_DATA_DEV_NAME                         "watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE               0xFD8000        // 16M - 160K
#define TCFG_DATA_FLASH_DEV_PATY_SIZE               0x28000         // 160K


//------------------------------------------------------------------------------------------
//EXT_NAND_FLASH//外挂NAND falsh
#elif (TCFG_NANDFLASH_DEV_ENABLE)

#define TCFG_VIRFAT_FLASH_ENABLE					DISABLE		//虚拟文件系统
#define TCFG_VIRFAT_INSERT_FLASH_ENABLE  			DISABLE		//内置FLASH虚拟文件系统
#define TCFG_VIRFAT_EXT_FLASH_ENABLE  	    		DISABLE	 	//外挂FLASH虚拟文件系统

#define TCFG_SDFILE_INSERT_FLASH_ENABLE  			DISABLE		//内置只读文件系统
#define TCFG_SDFILE_EXTERN_FLASH_ENABLE				DISABLE 	//外挂只读文件系统

#define TCFG_DATA_STORAGE_FDB_ENABLE  				ENABLE		//用户数据存FDB
#define TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE  		ENABLE		//用户数据存FDB&外挂flash
#define TCFG_DATA_STORAGE_VM_ENABLE   				DISABLE		//用户数据存VM

#define TCFG_NANDFLASH_UI_FAT_ENABLE 				ENABLE		//nandflash ui_fat分区使能(与virfat互斥，可以超过32M)
#define TCFG_NANDFLASH_FAT_ENABLE 					ENABLE		//nandflash fat分区使能
#define TCFG_NANDFLASH_UNUSED_FILE_SYSTEM			DISABLE		//不挂文件系统
//sdfile文件系统
#define TCFG_SDFILE_FLASH_DEV_PATY_SIZE	  			0x580000  // 5.5M
#define TCFG_SDFILE_FLASH_DEV_PATY_BASE	  			0x17e000
//uifat/virfat
#define TCFG_NANDFLASH_UI_FAT_LOGO					"UI_FAT"
#define TCFG_UI_FAT_FLASH_DEV_PATY_BASE				0x00000000
#define TCFG_UI_FAT_FLASH_DEV_PATY_SIZE				0x02000000							//32M	(使用virfat时不能超过32M，使用ui_fat没有限制)
//当TCFG_VIRFAT_INSERT_FLASH_ENABLE 或 TCFG_VIRFAT_EXT_FLASH_ENABLE使能时有效
#define TCFG_VIRFAT_FLASH_DEV_PATY_BASE             0x6FE000
#define TCFG_VIRFAT_FLASH_DEV_PATY_SIZE        		0xD8000 							//32M	(使用virfat时不能超过32M，使用ui_fat没有限制)
// nandflash 分区 fat16 区域地址和大小，与ini工具文件对应，即ini文件中，fat16文件的XX_ADR参数与BASE保持参数一致
#define TCFG_NANDFLASH_FAT_ROOT						"fat_nand"							// fat16 分区的设备名(存音乐)
#define TCFG_NANDFLASH_FAT_BASE						TCFG_UI_FAT_FLASH_DEV_PATY_SIZE	    // fat16 分区起始地址
#define TCFG_NANDFLASH_FAT_SIZE						(90 * 1024 * 1024)					// fat16 分区容量大小
//watch_data
#define TCFG_DATA_DEV_NAME         					"watch_data"
#define TCFG_DATA_FLASH_DEV_PATY_BASE				(TCFG_NANDFLASH_FAT_SIZE+TCFG_NANDFLASH_FAT_BASE)
#define TCFG_DATA_FLASH_DEV_PATY_SIZE				0x28000 //160K

// #if( TCFG_VIRFAT_FLASH_ENABLE && TCFG_NANDFLASH_UI_FAT_ENABLE)
// #error"config error,only enable one"
// #endif
#if(TCFG_VIRFAT_INSERT_FLASH_ENABLE &&  TCFG_VIRFAT_EXT_FLASH_ENABLE)
#error"config error,only enable one"
#endif

//------------------------------------------------------------------------------------------
#else
#error "FLASH_DEV_PART_CONFIG_ERROR !!!"
#endif

#endif//(!TCFG_UI_ENABLE)

#define TCFG_DATA_STORAGE_ENABLE      (TCFG_DATA_STORAGE_FDB_ENABLE || TCFG_DATA_STORAGE_VM_ENABLE ||TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE)

//*********************************************************************************//
//                                 SRAM 配置                                       //
//*********************************************************************************//
// 配置虚拟内存的大小(1MB <= 建议值 <=2MB)
//   增大虚拟内存可以缓解内存碎片问题,但同时维护虚拟内存的页表总大小也会占用更多的内存
#define TCFG_MMU_MEMORY_SIZE                        (2 * 1024 * 1024)
// 配置任务栈的总内存大小(建议按"任务数 * 单任务最大栈内存"配置)
//   由于每个任务的栈大小不同,所以此处设置的是估算值
#define TCFG_TOTAL_STACK_HEAP_SIZE                  (64 * (4 * 1024))
#if(TCFG_MMU_MEMORY_SIZE <= (256 * 1024))
#error"TCFG_MMU_MEMORY_SIZE CONFIG ERROR"
#endif
#if(TCFG_TOTAL_STACK_HEAP_SIZE <= (4 * 1024))
#error"TCFG_TOTAL_STACK_HEAP_SIZE CONFIG ERROR"
#endif

//*********************************************************************************//
//                                 PSRAM 配置                                      //
//*********************************************************************************//
#define TCFG_PSRAM_INIT_CLK 			 (400* 1000000)	//400Mhz	//内封跑400M
// #define TCFG_PSRAM_INIT_CLK				 (200*1000000)	//200Mhz		//顶板外挂考虑降频


/* 代码跑PSRAM */
#if TCFG_PSRAM_DEV_ENABLE
// #define CONFIG_EXTRAM_CODE_ENABLE
#ifdef CONFIG_EXTRAM_CODE_ENABLE
#define CONFIG_EXTRAM_ENTRY_ADDRESS      0x14000100       // 程序入口地址
#define CONFIG_EXTRAM_TYPE               1                // 外部ram类型 [1:psram | 2:sdram | 3:ddr1]
#define CONFIG_EXTRAM_CLK                144000000        // 外部ram时钟

#if TCFG_PMC_CODE_SIZE
#error "if defined CONFIG_EXTRAM_CODE_ENABLE, must ensure TCFG_PMC_CODE_SIZE = 0 !!!"
#endif

#define __SDRAM_SIZE__                   TCFG_PSRAM_SIZE  // 外部ram大小
// NOTE:ram的其它专有配置需要到isd_config_rule.c中配置(如PSRAM_PORT/PSRAM_MODE)
#if (__SDRAM_SIZE__ == 0)
#error "if defined CONFIG_EXTRAM_CODE_ENABLE, must ensure __SDRAM_SIZE__ > 0 !!!"
#endif
#else
#define __SDRAM_SIZE__                   0
#endif
#endif // #if TCFG_PSRAM_DEV_ENABLE

//*********************************************************************************//
//                               CACHE配置                                       //
//*********************************************************************************//
#if 0//((!TCFG_PSRAM_DEV_ENABLE) && (!TCFG_NORMAL_SET_DUT_MODE))
#define TCFG_FREE_ICACHE_WAY				2 // n*8KB //max is 2
#define TCFG_FREE_DCACHE_WAY				3 // n*8KB //max is 3
#define TCFG_DCACHE_RUN_BT_STATIC_RAM		DISABLE // 蓝牙静态ram放dcache
#define TCFG_DCACHE_RUN_GPU_BUF				DISABLE // GPU动态BUF优先使用dcache
#define TCFG_ICACHE_RUN_GPU_BUF				DISABLE // GPU动态BUF使用icache
#define TCFG_ICACHE_DYNAMIC_SWITCH			ENABLE // icache动态切换，低功耗的时候用回cache功能
#endif

//*********************************************************************************//
//                                 tp   配置                                      //
//*********************************************************************************//
#if TCFG_UI_ENABLE
#define TCFG_TOUCH_PANEL_ENABLE 				1
#else
#define TCFG_TOUCH_PANEL_ENABLE 				0
#endif
#define TCFG_TP_CST816D_ENABLE 					0
#define TCFG_TP_FT3X68_ENABLE 					0
#define TCFG_TP_JD9165T_ENABLE 					1
#define TCFG_TP_SLEEP_EN                        1
#define TCFG_TP_INT_IO      					TCFG_TP_PIN_INT //TP中断脚
#define TCFG_TP_RESET_IO    					TCFG_TP_PIN_RESET //TP复位脚

//*********************************************************************************//
//                                 LCD   配置                                      //
//*********************************************************************************//

#if TCFG_UI_ENABLE
#define TCFG_SPI_LCD_ENABLE 					1//spi lcd开关
#else
#define TCFG_SPI_LCD_ENABLE 					0//spi lcd开关
#endif

#define TCFG_LCD_QSPI_ICNA3310B_466x466_ENABLE 	0
#define TCFG_LCD_QSPI_GC9B71_320x386_ENABLE  	0
#define TCFG_LCD_QSPI_ST77916_320x385_ENABLE 	0
#define TCFG_LCD_QSPI_SH8501A_240x296_ENABLE 	0
#define TCFG_LCD_QSPI_SH8601A_454x454_ENABLE 	0
#define TCFG_LCD_QSPI_RAMLESS_ST77903_400x400_ENABLE 	0
#define TCFG_LCD_QSPI_RM69330_454x454_ENABLE 	0
#define TCFG_LCD_MCU_JD5858_360x360_ENABLE 		0
#define TCFG_LCD_MCU_D54E4PA7551B_XP26_400x240_ENABLE 	0
#define TCFG_LCD_RGB_ST7789V_240x240_ENABLE 	0
#define TCFG_LCD_QSPI_DTR_ST7801N_390x450_ENABLE 		0
#define TCFG_LCD_SPI_ST7789V_240x320_ENABLE     0
#define TCFG_LCD_RGB_480x272_ENABLE 			0
#define TCFG_LCD_RGB_ST7701S_480x800_ENABLE 	0
#define TCFG_LCD_QSPI_CO5300_466x466_ENABLE 	0
#define TCFG_LCD_MIPI_CO5300_466x466_ENABLE 	0
#define TCFG_LCD_MIPI_ST7701S_480x854_ENABLE 	0
#define TCFG_LCD_MIPI_ILI9881C_800x1280_ENABLE 	0
#define TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE   1
#define TCFG_LCD_RGB_FL58202_800x480_ENABLE     0
#define TCFG_LCD_MCU_NV3031A_240x320_ENABLE     0
#define TCFG_LCD_MCU_ST7789V_240x320_ENABLE     0


#define TCFG_LCD_MATCH_MODE 					0

#define LCD_MATCH_BY_LOGO   					0 //通过屏驱logo来匹配
#define LCD_LOGO            					"null"
#define TCFG_LCD_TE_USED_PEND 					1

#if (defined TCFG_NANDFLASH_DEV_ENABLE && TCFG_NANDFLASH_DEV_ENABLE)
#define TCFG_LCD_TE_IO      					IO_PORTA_00//TCFG_LCD_PIN_TE
#else
#define TCFG_LCD_TE_IO      					TCFG_LCD_PIN_TE
#endif

#define TCFG_LCD_TE_SYNC_HW_ENABLE				0//硬件te同步
#define TCFG_LCD_TE_QUICK_SYNC                  1//te优化策略
#define TCFG_LCD_BL_IO							TCFG_LCD_PIN_BL
#define TCFG_BACKLIGHT_PWM_MODE 				TCFG_LCD_PIN_BL_TYPE// 0-on/off, 1-pwmled(no support), 2-mcpwm
#define TCFG_UI_SHUT_DOWN_TIME					ENABLE // 自动息屏

// #define TCFG_LCD_TP_USE_SAME_PWR				ENABLE // lcd和tp 使用相同的电源

#if TCFG_LCD_TP_USE_SAME_PWR
#undef TCFG_TP_SLEEP_EN
#define TCFG_TP_SLEEP_EN                        DISABLE
#define LCD_POWER_DOWN_EN  						ENABLE
#define TP_POWER_DOWN_EN  						ENABLE
#endif
#define TCFG_GPU_DOUBLE_TASK_ENABLE             ENABLE

#define TCFG_LCD_BUF_IN_STATIC_RAM_ENABLE		DISABLE//ENABLE		//屏幕显存buf使用静态ram，可以优化帧率
#define TCFG_LCD_BUF_DYNAMIC_LINE_ENABLE		DISABLE		//屏幕显存高度动态计算，复杂场景下可以减少屏幕需要的ram

#define TCFG_PC_MODE_LOCK_UI_ENABLE				ENABLE		//PC模式锁定UI

#define TCFG_LCD_CLK_DRIVE_STRENGT				PORT_DRIVE_STRENGT_64p0mA//lcdclk强驱，抗干扰
//*********************************************************************************//
//                                 MOTO 配置                                      //
//*********************************************************************************//
#define TCFG_MOTO_PWM_IO 						TCFG_MOTO_PORT

//*********************************************************************************//
//                                 rdec_key 配置                                      //
//*********************************************************************************//
#define TCFG_RDEC_KEY_ENABLE					TCFG_QDEC_ENABLE//是否使能RDEC按键
//RDEC0配置

//*********************************************************************************//
//                                 充电 配置 (补充sdk_config_h)                                     //
//*********************************************************************************//
#ifndef   TCFG_CHARGE_NVDC_EN
#define   TCFG_CHARGE_NVDC_EN  TCFG_CHARGE_ENABLE
#endif

#define   TCFG_CHANEG_DONT_ENTER_LOW_POWER		ENABLE	//充电不进低功耗

//*********************************************************************************//
//                                 看门狗配置                                     //
//*********************************************************************************//
#define   WDT_APP_INIT_TIME						WDT_16S			//开机初始化的看门狗时间，适当给长一些
#define   WDT_APP_RUN_TIME						WDT_LRC_4S		//程序正常运行的看门狗时间，用LRC看门狗准一点


//*********************************************************************************//
//                                 ANC配置                                         //
//*********************************************************************************//
#define TCFG_AUDIO_ANC_ENABLE				CONFIG_ANC_ENABLE		//ANC总使能,根据global_bulid_cfg板级定义
// #define TCFG_ANC_TOOL_DEBUG_ONLINE 			DISABLE_THIS_MOUDLE		//ANC工具蓝牙spp调试
#define TCFG_ANC_EXPORT_RAM_EN				DISABLE_THIS_MOUDLE		//ANCdebug数据释放RAM使能
#if TCFG_ANC_EXPORT_RAM_EN
#define TCFG_AUDIO_CVP_CODE_AT_RAM			DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_AAC_CODE_AT_RAM			DISABLE_THIS_MOUDLE
#endif/*TCFG_ANC_EXPORT_RAM_EN*/

/*
 *系统音量类型选择
 *软件数字音量是指纯软件对声音进行运算后得到的
 *硬件数字音量是指dac内部数字模块对声音进行运算后输出
 */
#define VOL_TYPE_DIGITAL		0	//软件数字音量(调节解码输出数据的音量)
#define VOL_TYPE_ANALOG			1	//(暂未支持)硬件模拟音量
#define VOL_TYPE_AD				2	//(暂未支持)联合音量(模拟数字混合调节)
#define VOL_TYPE_DIGITAL_HW		3  	//硬件数字音量(调节DAC模块的硬件音量)
/*注意:ANC使能情况下使用软件数字音量*/
#if TCFG_AUDIO_ANC_ENABLE
//#define SYS_VOL_TYPE            VOL_TYPE_DIGITAL
#else
//#define SYS_VOL_TYPE            VOL_TYPE_DIGITAL_HW
#endif/*TCFG_AUDIO_ANC_ENABLE*/
/*
 *通话的时候使用数字音量
 *0：通话使用和SYS_VOL_TYPE一样的音量调节类型
 *1：通话使用数字音量调节，更加平滑
 */
#define TCFG_CALL_USE_DIGITAL_VOLUME		0

//第三方清晰语音开发使能
// #define TCFG_CVP_DEVELOP_ENABLE             DISABLE_THIS_MOUDLE

/*通话降噪模式配置*/
// #define CVP_ANS_MODE	0	[>传统降噪<]
// #define CVP_DNS_MODE	1	[>神经网络降噪<]
// #define TCFG_AUDIO_CVP_NS_MODE				CVP_ANS_MODE

/*
 * ENC(双mic降噪)配置
 * 双mic降噪包括DMS_NORMAL和DMS_FLEXIBLE，在使能TCFG_AUDIO_DUAL_MIC_ENABLE
 * 的前提下，根据具体需求，选择对应的DMS模式
 */
/*ENC(双mic降噪)使能*/
// #define TCFG_AUDIO_DUAL_MIC_ENABLE			DISABLE_THIS_MOUDLE

/*DMS模式选择*/
// #define DMS_NORMAL		1	//普通双mic降噪(mic距离固定)
// #define DMS_FLEXIBLE	2	//适配mic距离不固定且距离比较远的情况，比如头戴式话务耳机
// #define TCFG_AUDIO_DMS_SEL					DMS_NORMAL

/*ENC双mic配置主mic副mic对应的mic port*/
// #define DMS_MASTER_MIC0		0 //mic0是主mic
// #define DMS_MASTER_MIC1		1 //mic1是主mic
// #define TCFG_AUDIO_DMS_MIC_MANAGE			DMS_MASTER_MIC0
/*双mic降噪/单麦mic降噪 DUT测试模式，配合设备测试mic频响和(双mic)降噪量*/

//MIC通道配置
// #if TCFG_AUDIO_DUAL_MIC_ENABLE
//#define TCFG_AUDIO_ADC_MIC_CHA				(AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1)
// #else
//#define TCFG_AUDIO_ADC_MIC_CHA				AUDIO_ADC_MIC_0
// #endif[>TCFG_AUDIO_DUAL_MIC_ENABLE<]

/*MIC模式配置:单端隔直电容模式/差分隔直电容模式/单端省电容模式*/
// #if TCFG_AUDIO_ANC_ENABLE
/*注意:ANC使能情况下，使用差分mic*/
// #define TCFG_AUDIO_MIC_MODE					AUDIO_MIC_CAP_DIFF_MODE
// #define TCFG_AUDIO_MIC1_MODE				AUDIO_MIC_CAP_DIFF_MODE
// #else
// #define TCFG_AUDIO_MIC_MODE					AUDIO_MIC_CAP_MODE
// #define TCFG_AUDIO_MIC1_MODE				AUDIO_MIC_CAP_MODE
// #endif[>TCFG_AUDIO_ANC_ENABLE<]

/*
 *>>MIC电源管理:根据具体方案，选择对应的mic供电方式
 *(1)如果是多种方式混合，则将对应的供电方式或起来即可，比如(MIC_PWR_FROM_GPIO | MIC_PWR_FROM_MIC_BIAS)
 *(2)如果使用固定电源供电(比如dacvdd)，则配置成DISABLE_THIS_MOUDLE
 */
#define MIC_PWR_FROM_GPIO		(1UL << 0)	//使用普通IO输出供电
#define MIC_PWR_FROM_MIC_BIAS	(1UL << 1)	//使用内部mic_ldo供电(有上拉电阻可配)
#define MIC_PWR_FROM_MIC_LDO	(1UL << 2)	//使用内部mic_ldo供电
//配置MIC电源
// #define TCFG_AUDIO_MIC_PWR_CTL				MIC_PWR_FROM_MIC_BIAS


/*提示音叠加配置*/
#define TCFG_WAV_TONE_MIX_ENABLE			DISABLE
#define TCFG_MP3_TONE_MIX_ENABLE			DISABLE
#define TCFG_WTG_TONE_MIX_ENABLE			DISABLE
#define TCFG_WTS_TONE_MIX_ENABLE			ENABLE



//*********************************************************************************//
//                         Spatial Audio Effect 空间音效配置                       //
//*********************************************************************************//
// #define TCFG_AUDIO_SPATIAL_EFFECT_ENABLE           	DISABLE_THIS_MOUDLE
// #define TCFG_TWS_SPATIAL_AUDIO_AS_CHANNEL   		'L'

/*独立任务里面跑空间音效*/
#define TCFG_AUDIO_EFFECT_TASK_EBABLE               ENABLE_THIS_MOUDLE

/*空间音效在线调试*/
// #define TCFG_SPATIAL_EFFECT_ONLINE_ENABLE           DISABLE_THIS_MOUDLE

/*空间音频传感器是否在解码任务读传感器数据*/
#define TCFG_SENSOR_DATA_READ_IN_DEC_TASK           DISABLE_THIS_MOUDLE

/*传感器数据读取的频率间隔，单位ms*/
#define TCFG_SENSOR_DATA_READ_INTERVAL              20

/*空间音频独立EQ使能*/
#define TCFG_SPATIAL_EFFECT_EQ_ENABLE               DISABLE_THIS_MOUDLE

/*陀螺仪数据导出配置:支持BT_SPP\UART载体导出*/
#define SENSOR_DATA_EXPORT_USE_UART 				1
#define SENSOR_DATA_EXPORT_USE_SPP 	    			2
#define TCFG_SENSOR_DATA_EXPORT_ENABLE				DISABLE_THIS_MOUDLE

//*********************************************************************************//
//                                  IIS 配置                                     //
//*********************************************************************************//
#define TCFG_IIS_ENABLE                     DISABLE_THIS_MOUDLE

#define TCFG_IIS_MODE                       (0)     // 0:master  1:slave

#define TCFG_AUDIO_INPUT_IIS                (ENABLE && TCFG_IIS_ENABLE)
#define TCFG_IIS_INPUT_DATAPORT_SEL         ALINK_CH1

#define TCFG_AUDIO_OUTPUT_IIS               (DISABLE && TCFG_IIS_ENABLE)
#define TCFG_IIS_OUTPUT_DATAPORT_SEL        ALINK_CH0

#define TCFG_IIS_SAMPLE_RATE                (16000L)


//*********************************************************************************//
//                                   CAN配置                                       //
//*********************************************************************************//
#if TCFG_CAN_ENABLE
#define TCFG_CANOPEN_ENABLE                 TCFG_CAN_ENABLE
#define TCFG_CAN_STM_ENABLE                 0
#define TCFG_CAN_LOM_ENABLE                 0
#define TCFG_CAN_BAUD_BRP                   7
#define TCFG_CAN_BAUD_SJW                   3
#define TCFG_CAN_BAUD_SAM                   1
#define TCFG_CAN_BAUD_PTS_PBS1              2
#define TCFG_CAN_BAUD_PBS2                  1
#if TCFG_CAN_MODE == 1 //enhanced can
#define TCFG_CAN_FILTER_MODE                4
#define TCFG_CAN_FILTER_BLACKLIST_EN        0
#define TCFG_CAN_FILTER_ACC_ID0             0x1F02E000
#define TCFG_CAN_FILTER_ACC_ID1             0x00000605
#define TCFG_CAN_FILTER_ACC_RTR0            0
#define TCFG_CAN_FILTER_ACC_RTR1            0
#define TCFG_CAN_FILTER_MASK_ID0            0x1FFFFFFF
#define TCFG_CAN_FILTER_MASK_ID1            0x1FFFFFFF
#define TCFG_CAN_FILTER_MASK_RTR0           1
#define TCFG_CAN_FILTER_MASK_RTR1           1
#else
#define TCFG_CAN_FILTER_MODE                2
#define TCFG_CAN_FILTER_BLACKLIST_EN        0
#define TCFG_CAN_FILTER_ACC_ID0             0x654
#define TCFG_CAN_FILTER_MASK_ID0            0x7FF
#endif
#endif


//*********************************************************************************//
//                                  g-sensor配置                                   //
//*********************************************************************************//
// #define TCFG_GSENSOR_ENABLE                       0     //gSensor使能
#define TCFG_DA230_EN                       DISABLE
#define TCFG_SC7A20_EN                      DISABLE
#define TCFG_STK8321_EN                     DISABLE
#define TCFG_IRSENSOR_ENABLE                DISABLE
#define TCFG_JSA1221_ENABLE                 DISABLE
#define TCFG_GSENOR_USER_IIC_TYPE           DISABLE     //0:软件IIC  1:硬件IIC

//*********************************************************************************//
//                            传感器与运动健康 配置                                //
//*********************************************************************************//
//运动健康总开关
#define TCFG_SPORT_HEALTH_ENABLE			DISABLE
#define TCFG_SENSOR_HUB_TASK_ENABLE			DISABLE
//各模块开关
#define TCFG_SPORT_HEALTH_ALGO_GSENSOR		ENABLE				//gsensor算法
#define TCFG_SPORT_HEALTH_SPORT				ENABLE				//运动
#define TCFG_SPORT_HEALTH_DAILY_ACTIVE		ENABLE				//日常活动
#define TCFG_SPORT_HEALTH_SLEEP				DISABLE				//睡眠
#define TCFG_SPORT_HEALTH_HEART_RATE		DISABLE				//心率
#define TCFG_SPORT_HEALTH_BLOOD_OXYGEN		DISABLE				//血氧
#define TCFG_SPORT_HEALTH_DET_MENSE			ENABLE				//女性健康监测

//SENSOR HUB总开关
#define TCFG_SENSOR_HUB						DISABLE
//P11 sensorhub开关
#define TCFG_SENSOR_HUB_P11					DISABLE
//common
#define TCFG_ALGO_STEP_COUNETER				DISABLE
#define TCFG_ACCELER_SLEEP_ENABLE  			DISABLE
//p11
#define	TCFG_ACCELER_P11_ENABLE 			DISABLE
#define	TCFG_GYRO_P11_ENABLE 				DISABLE
#define	TCFG_MAGNETIC_P11_ENABLE          	DISABLE
#define	TCFG_VCHR11_P11_ENABLE          	DISABLE
#define	TCFG_HRS3602_P11_ENABLE          	DISABLE

//master
#define TCFG_ACCELER_MASTER_ENABLE			DISABLE
#define TCFG_GYRO_MASTER_ENABLE				DISABLE
#define	TCFG_MAGNETIC_MASTER_ENABLE         DISABLE
#define	TCFG_VCHR11_MASTER_ENABLE          	DISABLE
#define	TCFG_HRS3602_MASTER_ENABLE          DISABLE

//gsensor配置
#define TCFG_SENSOR_SC7A20_ENABLE			DISABLE

//msensor配置
#define TCFG_SENSOR_MMC5603_ENABLE			DISABLE


//*********************************************************************************//
//                                  pay 配置                                       //
//*********************************************************************************//
#define TCFG_PAY_ALIOS_ENABLE			    0

#define TCFG_PAY_TRANSITCODE_ENABLE         0 // 乘车码
#define TCFG_TRANSIT_USE_PSRAM              0 // 乘车码使用psram,需使能psram

#define TCFG_PAY_ALIOS_WAY_T_HEAD			1 // 平头哥
#define TCFG_PAY_ALIOS_WAY_SEL				TCFG_PAY_ALIOS_WAY_T_HEAD

#if (TCFG_PAY_ALIOS_WAY_SEL==TCFG_PAY_ALIOS_WAY_T_HEAD)
#define TCFG_PAY_ALIOS_PRODUCT_MODEL        ""
#define TCFG_PAY_ALIOS_COMPANY_NAME         "" //需要客户申请
#define ALIPAY_SE_FW_V2_0                   1  //SE版本固件为2.0设置为1,否则设置为0
#define ALIPAY_SE_USE_RESET_PIN             0  //置1加密芯片采用reset管脚复位进低功耗，置0 上下电进低功耗
#define SE_POWER_GPIO                       0  //使用GPIO口给SE芯片电源脚供电

#endif

//*********************************************************************************//
//                                  触摸配置                                       //
//*********************************************************************************//
#define TCFG_LPCTMU_ENABLE                  0   //总开关
#define TCFG_LPCTMU_CH0_EN                  0   //IO_PORTB_00
#define TCFG_LPCTMU_CH1_EN                  1   //IO_PORTB_01
#define TCFG_LPCTMU_CH2_EN                  0   //IO_PORTB_02
#define TCFG_LPCTMU_CH3_EN                  0   //IO_PORTB_03
#define TCFG_LPCTMU_CH4_EN                  0   //IO_PORTB_04


//*********************************************************************************//
//                                  demo配置                                       //
//*********************************************************************************//
#define JLUI_DEMO_ENABLE					0 /*! 使能杰理UI DEMO开关 */

#define GPU_DEMO_ENABLE						0 /*! 使能GPU DEMO开关 */

//*********************************************************************************//
//                                 Find my 配置                                      //
//*********************************************************************************//
#define TCFG_FINDMY_ENABLE 					0

//*********************************************************************************//
//                                 emitter配置                                      //
//*********************************************************************************//
#if TCFG_USER_EMITTER_ENABLE
#ifdef TCFG_BT_SUPPORT_HFP_AG
#undef TCFG_BT_SUPPORT_HFP_AG
#endif
#define TCFG_BT_SUPPORT_HFP_AG 				1 // HFP AG
#endif

//*********************************************************************************//
//                                 视频配置                                      //
//*********************************************************************************//
//
#define TCFG_VIDEO_DIAL_ENABLE 				0//视频表盘
//uvc
#define TCFG_UVC_SHOW_ENABLE 				0//UVC显示
#define UVC_JPG_DATA_WRITE2SD           	0//UVC jpg 数据写入SD卡，需要开启SD卡
//camera
#define TCFG_CAMERA_MANAGER_ENABLE			0//相机、相册、摄像头功能
#define TCFG_CAMERA_DEV_BF30A2				0//摄像头驱动bf30a2
#if TCFG_VIDEO_DIAL_ENABLE || TCFG_CAMERA_MANAGER_ENABLE
#define TCFG_APP_VIDEO_EN               	1
#endif

// 视频流传输 (对讲/视频通话)
#define TCFG_VSTREAM_TRANSFER           0   // 视频流传输总开关
#define TCFG_JPEG_MAX_SIZE              (6*1024)  // JPEG 最大帧大小 6KB

//*********************************************************************************//
//                                 USB配置                                      //
//*********************************************************************************//
// #define TCFG_OTG_MODE					0
//插入充电时打开usb检测，拔出关闭检测
#define TCFG_PC_MODE_USB_DETECT_DURING_CHARGING		(TCFG_APP_PC_EN&&TCFG_CHARGE_ENABLE)// ENABLE
//*********************************************************************************//
//                                  产测配置                                       //
//*********************************************************************************//
#define PRODUCT_TEST_ENABLE					DISABLE
#define PT_IO_TX_PIN					    IO_PORT_DM
#define PT_IO_RX_PIN					    IO_PORT_DP
#define PT_CHECK_OT                         10000

#define PT_GPIO_ENABLE						DISABLE
#define PT_GSENSOR_ENABLE					DISABLE
#define PT_MOTOR_ENABLE						DISABLE
#define PT_SD_ENABLE						DISABLE
#define PT_HR_ENABLE						DISABLE
#define PT_SPEAKER_MIC_ENABLE				DISABLE

#define PT_GPIO_CHECK_LCD_TP				DISABLE // 使用GPIO检查LCD_TP



#define TCFG_PT_JL_TEST_ENABLE				DISABLE //JL生产部开发板测试
#define TCFG_DASHBOARD_HARDWARE_TEST_ENABLE 0  // 仪表开发板测试

#if TCFG_PT_JL_TEST_ENABLE || TCFG_DASHBOARD_HARDWARE_TEST_ENABLE

#undef TCFG_SENSOR_HUB
#define TCFG_SENSOR_HUB					1

#undef TCFG_ACCELER_MASTER_ENABLE
#define TCFG_ACCELER_MASTER_ENABLE		1

#undef TCFG_SENSOR_SC7A20_ENABLE
#define TCFG_SENSOR_SC7A20_ENABLE		1

#undef  TCFG_UI_SHUT_DOWN_TIME
#define TCFG_UI_SHUT_DOWN_TIME			0


#endif//TCFG_PT_JL_TEST_ENABLE
//*********************************************************************************//
//                                 网络配置                                        //
//*********************************************************************************//
#define USER_SUPPORT_PROFILE_PAN    		TCFG_BT_NET_ENABLE //开启pan服务
#define TCFG_NETWORK_ENABLE                 (TCFG_BT_NET_ENABLE || TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE || TCFG_LTE_PHY_ENABLE)  // 网络能力
#define TCFG_WIFI_NET_ENABLE                (TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE)  // WIFI类型网络(外挂WIFI/UART网卡/SPI网卡)


#define TCFG_AI_INTERACTION_ENABLE          DISABLE // APP版科大讯飞

#define TCFG_IFLYTEK_ENABLE                 DISABLE // 科大讯飞网络版
#define TCFG_IFLYTEK_USE_PSRAM              DISABLE // 科大讯飞网络版使用psram

#if TCFG_IFLYTEK_ENABLE
#define TCFG_IFLYTEK_APP_SECRET             ""
#define TCFG_IFLYTEK_APP_KEY                ""
#define TCFG_IFLYTEK_APP_ID                 ""
#endif


// 网络投屏
#define TCFG_NET_SCR_ENABLE                 1  // WiFi投屏
#define TCFG_BT_H264_SCR_ENABLE             0  // 蓝牙H264投屏

#if TCFG_NET_SCR_ENABLE
#define NET_SCR_PORT    	                2230
#define CONFIG_NET_PROTOCOL_TCP             0  //网络投屏协议选择
#define CONFIG_NET_PROTOCOL_UDP             1
#define CTP_CTRL_PORT   	                3333
#define CDP_CTRL_PORT   	                2228
#define SCR_FRAME_TYPE   	                10   //2: jpeg, 10:jljpeg
#endif

#if TCFG_BT_H264_SCR_ENABLE
// 分辨率
#define SM_CFG_DEFAULT_WIDTH    800
#define SM_CFG_DEFAULT_HEIGHT   464
// 帧率
#define SM_CFG_DEFAULT_FPS      10
// 流媒体MTU（设备端配置值，实际协商后的 MTU 通过 stream_media_core_get_mtu() 获取）
#define SM_CFG_STREAM_BLE_MTU      	500
#define SM_CFG_STREAM_SPP_MTU      	2048
// 码率，bps
#define SM_CFG_STREAM_BITRATE    300000
// H264 IP间隔
#define SM_CFG_IP_INTERVAL_FRAMES    20
#endif

// 网络配置检查
#if TCFG_IFLYTEK_ENABLE || TCFG_PAY_TRANSITCODE_ENABLE || TCFG_NET_SCR_ENABLE
#if !TCFG_NETWORK_ENABLE
#error "need NET"
#endif
#endif

//*********************************************************************************//
//                                 固件端应用配置                                  //
//*********************************************************************************//
#ifdef BUILD_BSP_PROJ_ENABLE
#define TCFG_FW_APP_ENABLE					ENABLE
#else
#define TCFG_FW_APP_ENABLE					DISABLE
#endif

//*********************************************************************************//
//                                 配置结束                                        //
//*********************************************************************************//



#endif //CONFIG_BOARD_JL707N_DEMO
#endif //CONFIG_BOARD_JL707N_DEMO_CFG_H
