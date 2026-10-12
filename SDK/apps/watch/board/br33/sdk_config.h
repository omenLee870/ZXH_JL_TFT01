/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/

#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#include "jlstream_node_cfg.h"

// ------------JLStudio Auto generate------------
#define PROJECT_CONFIG_NAME                       ""               // 默认配置为空，非默认配置为配置名称
// ------------JLStudio Auto generate------------

// ------------板级配置.json------------
#define TCFG_SUB_CASE_TYPE                        SUB_CASE_TYPE_DASHBOARD // 子项目

#define TCFG_DEBUG_UART_ENABLE                    1                // 调试串口
#if TCFG_DEBUG_UART_ENABLE
#define TCFG_DEBUG_UART_TX_PIN                    IO_PORTA_12      // 输出IO
#define TCFG_DEBUG_UART_BAUDRATE                  1000000          // 波特率
#endif // TCFG_DEBUG_UART_ENABLE

#define TCFG_EXCEPTION_LOG_ENABLE                 1                // 打印异常信息
#define TCFG_EXCEPTION_RESET_ENABLE               0                // 异常自动复位
#define TCFG_CONFIG_DEBUG_RECORD_ENABLE           0                // 异常存flash
#define TCFG_EXCEPTION_CODE_AT_RAM                1                // 异常代码放RAM

#define TCFG_CFG_TOOL_ENABLE                      0                // FW编辑、在线调音
#if TCFG_CFG_TOOL_ENABLE
#define TCFG_ONLINE_TX_PORT                       IO_PORT_DP       // 串口引脚TX
#define TCFG_ONLINE_RX_PORT                       IO_PORT_DM       // 串口引脚RX
#define TCFG_COMM_TYPE                            TCFG_USB_COMM    // 通信方式
#endif // TCFG_CFG_TOOL_ENABLE

#define CONFIG_SPI_DATA_WIDTH                     4                // flash通信
#define CONFIG_SPI_MODE                           1                // flash模式
#define CONFIG_FLASH_SIZE                         0x1000000        // flash容量
#define TCFG_VM_SIZE                              32               // VM大小（K）
#define CONFIG_FLASH_DTR_EN                       0                // flash-dtr使能
#define CONFIG_FLASH_WPS_EN                       1                // flash-上电写保护使能
#define CONFIG_VOTP_SIZE                          256              // votp size（Byte）
#define TCFG_NORFLASH_DEV_ENABLE                  0                // 外挂flash使能
#define CONFIG_EXTERN_FLASH_SIZE                  0x0              // 外挂flash容量
#define TCFG_PSRAM_DEV_ENABLE                     1                // PSRAM 使能
#define TCFG_PSRAM_SIZE                           0x800000         // PSRAM容量
#define TCFG_NANDFLASH_DEV_ENABLE                 0                // NANDFLASH使能
#define TCFG_SD0_ENABLE                           0                // SD0使能
#define TCFG_SD0_SDC_TYPE                         0                // 类型设置
#define TCFG_SD0_DAT_MODE                         4                // 线数设置
#define TCFG_SD0_DET_MODE                         SD_CMD_DECT      // 检测方式
#define TCFG_SD0_CLK                              12000000         // SD时钟频率
#define TCFG_SD0_DET_IO                           NO_CONFIG_PORT   // 检测IO
#define TCFG_SD0_DET_IO_LEVEL                     0                // IO检测方式
#define TCFG_SD0_ALWAY_ONLINE_ENABLE              1                // 卡检测功耗优化
#define TCFG_SD0_POWER_SEL                        SD_PWR_SDPG      // SD卡电源
#define TCFG_SD0_CAN_OPERATE_MMC_CARD             0                // 支持MMC卡
#define TCFG_SD0_KEEP_CARD_AT_ACTIVE_STATUS       0                // 保持卡活跃状态
#define TCFG_SD0_POWER_PORT                       IO_PORTC_15      // SD卡GPIO电源0
#define TCFG_SD0_POWER_PORT1                      NO_CONFIG_PORT   // SD卡GPIO电源1
#define TCFG_SD0_POWER_PORT2                      NO_CONFIG_PORT   // SD卡GPIO电源2
#define TCFG_SD0_POWER_PORT_LEVEL                 0                // SD卡GPIO电源有效电平
#define TCFG_SD0_PORT_CMD                         IO_PORTD_07      // SD_PORT_CMD
#define TCFG_SD0_PORT_CLK                         IO_PORTD_06      // SD_PORT_CLK
#define TCFG_SD0_PORT_DA0                         IO_PORTD_05      // SD_PORT_DATA0
#define TCFG_SD0_PORT_DA1                         IO_PORTD_04      // SD_PORT_DATA1
#define TCFG_SD0_PORT_DA2                         IO_PORTD_03      // SD_PORT_DATA2
#define TCFG_SD0_PORT_DA3                         IO_PORTD_02      // SD_PORT_DATA3
#define TCFG_SD0_PORT_CS                          NO_CONFIG_PORT   // SD_PORT_CS
#define TCFG_SD0_PORT_WAKE                        NO_CONFIG_PORT   // SD_PORT_WAKE
#define TCFG_SD1_ENABLE                           1                // SD1使能
#define TCFG_SD1_SDC_TYPE                         1                // 类型设置
#define TCFG_SD1_DAT_MODE                         4                // 线数设置
#define TCFG_SD1_DET_MODE                         SD_CMD_DECT      // 检测方式
#define TCFG_SD1_CLK                              12000000         // SD时钟频率
#define TCFG_SD1_DET_IO                           NO_CONFIG_PORT   // 检测IO
#define TCFG_SD1_DET_IO_LEVEL                     0                // IO检测方式
#define TCFG_SD1_ALWAY_ONLINE_ENABLE              1                // 卡检测功耗优化
#define TCFG_SD1_POWER_SEL                        SD_PWR_NULL      // SD卡电源
#define TCFG_SD1_CAN_OPERATE_MMC_CARD             0                // 支持MMC卡
#define TCFG_SD1_KEEP_CARD_AT_ACTIVE_STATUS       0                // 保持卡活跃状态
#define TCFG_SD1_POWER_PORT                       IO_PORTC_08      // SD卡GPIO电源0
#define TCFG_SD1_POWER_PORT1                      IO_PORTC_07      // SD卡GPIO电源1
#define TCFG_SD1_POWER_PORT2                      IO_PORTC_06      // SD卡GPIO电源2
#define TCFG_SD1_POWER_PORT_LEVEL                 1                // SD卡GPIO电源有效电平
#define TCFG_SD1_PORT_CMD                         IO_PORTC_09      // SD_PORT_CMD
#define TCFG_SD1_PORT_CLK                         IO_PORTC_14      // SD_PORT_CLK
#define TCFG_SD1_PORT_DA0                         IO_PORTC_10      // SD_PORT_DATA0
#define TCFG_SD1_PORT_DA1                         IO_PORTC_11      // SD_PORT_DATA1
#define TCFG_SD1_PORT_DA2                         IO_PORTC_12      // SD_PORT_DATA2
#define TCFG_SD1_PORT_DA3                         IO_PORTC_13      // SD_PORT_DATA3
#define TCFG_SD1_PORT_CS                          NO_CONFIG_PORT   // SD_PORT_CS
#define TCFG_SD1_PORT_WAKE                        NO_CONFIG_PORT   // SD_PORT_WAKE
#define TCFG_SDPG_ALWAYS_ENABLE                   0                // SDPG使能
#define TCFG_SDPG_TIEUP_PORT                      NO_CONFIG_PORT   // SDPG绑定IO
#define FUSB_MODE                                 0                // USB工作模式
#define TCFG_USB_HOST_ENABLE                      1                // USB主机总开关
#define USB_H_MALLOC_ENABLE                       1                // 主机使用malloc
#define TCFG_USB_HOST_MOUNT_RESET                 40               // usb reset时间
#define TCFG_USB_HOST_MOUNT_TIMEOUT               50               // 握手超时时间
#define TCFG_USB_HOST_MOUNT_RETRY                 3                // 枚举失败重试次数
#define TCFG_UDISK_ENABLE                         1                // U盘使能
#define TCFG_HOST_UVC_ENABLE                      0                // 主机UVC使能
#define TCFG_HID_HOST_ENABLE                      0                // 主机HID使能
#define TCFG_HUB_HOST_ENABLE                      0                // 主机HUB使能
#define TCFG_HOST_AUDIO_ENABLE                    0                // 主机音频使能
#define TCFG_AOA_ENABLE                           0                // AOA使能
#define USB_MALLOC_ENABLE                         1                // 从机使用malloc
#define TCFG_USB_SLAVE_MSD_ENABLE                 1                // 读卡器使能
#define TCFG_USB_SLAVE_HID_ENABLE                 1                // HID使能
#define MSD_BLOCK_NUM                             1                // MSD缓存块数
#define USB_AUDIO_VERSION                         USB_AUDIO_VERSION_1_0 // UAC协议版本
#define TCFG_USB_SLAVE_AUDIO_SPK_ENABLE           1                // USB扬声器使能
#define SPK_AUDIO_RATE_NUM                        1                // SPK采样率列表
#define SPK_AUDIO_RES                             16               // SPK位宽1
#define SPK_AUDIO_RES_2                           0                // SPK位宽2
#define TCFG_USB_SLAVE_AUDIO_MIC_ENABLE           1                // USB麦克风使能
#define MIC_AUDIO_RATE_NUM                        1                // MIC采样率列表
#define MIC_AUDIO_RES                             16               // MIC位宽1
#define MIC_AUDIO_RES_2                           0                // MIC位宽2
#define TCFG_IIC_OFFSET_SOFT                      16               // 软件IIC偏移
#define TCFG_IIC_OFFSET_PMC                       6                // PMC IIC偏移
#define TCFG_IIC_OFFSET_HW                        0                // 硬件IIC偏移
#define TCFG_HW_I2C0_CLK_PORT                     IO_PORTB_02      // 硬件iic-0 CLK脚
#define TCFG_HW_I2C0_DAT_PORT                     IO_PORTB_03      // 硬件iic-0 DATA脚
#define TCFG_HW_I2C0_CLK                          400000           // 硬件iic-0波特率
#define TCFG_HW_I2C1_CLK_PORT                     IO_PORTD_12      // 硬件iic-1 CLK脚
#define TCFG_HW_I2C1_DAT_PORT                     IO_PORTE_00      // 硬件iic-1 DATA脚
#define TCFG_HW_I2C1_CLK                          400000           // 硬件iic-1波特率
#define TCFG_HW_I2C2_CLK_PORT                     NO_CONFIG_PORT   // 硬件iic-2 CLK脚
#define TCFG_HW_I2C2_DAT_PORT                     NO_CONFIG_PORT   // 硬件iic-2 DATA脚
#define TCFG_HW_I2C2_CLK                          400000           // 硬件iic-2波特率
#define TCFG_HW_I2C3_CLK_PORT                     NO_CONFIG_PORT   // 硬件iic-3 CLK脚
#define TCFG_HW_I2C3_DAT_PORT                     NO_CONFIG_PORT   // 硬件iic-3 DATA脚
#define TCFG_HW_I2C3_CLK                          400000           // 硬件iic-3波特率
#define TCFG_HW_I2C4_CLK_PORT                     NO_CONFIG_PORT   // 硬件iic-4 CLK脚
#define TCFG_HW_I2C4_DAT_PORT                     NO_CONFIG_PORT   // 硬件iic-4 DATA脚
#define TCFG_HW_I2C4_CLK                          400000           // 硬件iic-4波特率
#define TCFG_HW_I2C5_CLK_PORT                     NO_CONFIG_PORT   // 硬件iic-5 CLK脚
#define TCFG_HW_I2C5_DAT_PORT                     NO_CONFIG_PORT   // 硬件iic-5 DATA脚
#define TCFG_HW_I2C5_CLK                          400000           // 硬件iic-5波特率
#define TCFG_HW_PMC_I2C0_CLK_PORT                 IO_PORTD_10      // PMC iic-0 CLK脚
#define TCFG_HW_PMC_I2C0_DAT_PORT                 IO_PORTD_09      // PMC iic-0 DATA脚
#define TCFG_HW_PMC_I2C0_CLK                      400000           // PMC iic-0波特率
#define TCFG_HW_PMC_I2C1_CLK_PORT                 NO_CONFIG_PORT   // PMC iic-1 CLK脚
#define TCFG_HW_PMC_I2C1_DAT_PORT                 NO_CONFIG_PORT   // PMC iic-1 DATA脚
#define TCFG_HW_PMC_I2C1_CLK                      400000           // PMC iic-1波特率
#define TCFG_SW_I2C0_CLK_PORT                     NO_CONFIG_PORT   // 软件iic-0 CLK脚
#define TCFG_SW_I2C0_DAT_PORT                     NO_CONFIG_PORT   // 软件iic-0 DATA脚
#define TCFG_SW_I2C0_DELAY_CNT                    50               // iic-0 延时
#define TCFG_SW_I2C1_CLK_PORT                     NO_CONFIG_PORT   // 软件iic-1 CLK脚
#define TCFG_SW_I2C1_DAT_PORT                     NO_CONFIG_PORT   // 软件iic-1 DATA脚
#define TCFG_SW_I2C1_DELAY_CNT                    50               // iic-1 延时

#define TCFG_HW_SPI1_ENABLE                       1                // 硬件SPI1
#if TCFG_HW_SPI1_ENABLE
#define TCFG_HW_SPI1_PORT_CLK                     NO_CONFIG_PORT   // SPI1 CLK脚
#define TCFG_HW_SPI1_PORT_DO                      NO_CONFIG_PORT   // SPI1  DO脚
#define TCFG_HW_SPI1_PORT_DI                      NO_CONFIG_PORT   // SPI1  DI脚
#define TCFG_HW_SPI1_MODE                         SPI_MODE_BIDIR_1BIT // SPI1 模式
#define TCFG_HW_SPI1_ROLE                         SPI_ROLE_MASTER  // SPI1  ROLE
#define TCFG_HW_SPI1_BAUD                         24000000         // SPI1  时钟
#endif // TCFG_HW_SPI1_ENABLE

#define TCFG_HW_SPI2_ENABLE                       1                // 硬件SPI2
#if TCFG_HW_SPI2_ENABLE
#define TCFG_HW_SPI2_PORT_CLK                     IO_PORTC_00      // SPI2 CLK脚
#define TCFG_HW_SPI2_PORT_DO                      IO_PORTC_10      // SPI2  DO脚
#define TCFG_HW_SPI2_PORT_DI                      IO_PORTC_02      // SPI2  DI脚
#define TCFG_HW_SPI2_PORT_D2                      NO_CONFIG_PORT   // SPI2  D2脚
#define TCFG_HW_SPI2_PORT_D3                      NO_CONFIG_PORT   // SPI2  D3脚
#define TCFG_HW_SPI2_MODE                         SPI_MODE_BIDIR_1BIT // SPI2 模式
#define TCFG_HW_SPI2_ROLE                         SPI_ROLE_MASTER  // SPI2  ROLE
#define TCFG_HW_SPI2_BAUD                         32000000         // SPI2  时钟
#endif // TCFG_HW_SPI2_ENABLE

#define TCFG_CAN_ENABLE                           1                // CAN
#if TCFG_CAN_ENABLE
#define TCFG_CAN_MODE                             1                // CAN MODE
#define TCFG_CAN_RX_IO                            IO_PORTD_00      // RX脚
#define TCFG_CAN_TX_IO                            IO_PORTD_01      // TX脚
#endif // TCFG_CAN_ENABLE

#define TCFG_MOTO_PORT                            NO_CONFIG_PORT   // MOTO

#define TCFG_MULTI_CHIP_CONN_ENABLE               0                // 多芯片连接
#if TCFG_MULTI_CHIP_CONN_ENABLE
#define TCFG_MULTI_CHIP_CONN_TYPE                 CHIP_CONN_TYPE_SPI // MULTI_CHIP_CONN_TYPE
#define TCFG_SPI_CLK_PIN                          IO_PORTA_11      // SPI_CLK
#define TCFG_SPI_DO_PIN                           IO_PORTA_10      // SPI_DO
#define TCFG_SPI_DI_PIN                           IO_PORTA_09      // SPI_DI
#define TCFG_SPI_CS_PIN                           IO_PORTA_08      // SPI_CS
#define TCFG_SPI_BAUD                             16000000         // SPI 时钟
#endif // TCFG_MULTI_CHIP_CONN_ENABLE

#define TCFG_BT_PA_ENABLE                         0                // 蓝牙PA
#if TCFG_BT_PA_ENABLE
#define TCFG_BT_PA_PWR_IO                         IO_PORTD_04      // 电源脚
#define TCFG_BT_PA_PWR_IO_LEVEL                   1                // 电源脚有效电平
#define TCFG_BT_PA_RX_IO                          IO_PORTD_06      // RX脚
#define TCFG_BT_PA_TX_IO                          IO_PORTD_05      // TX脚
#endif // TCFG_BT_PA_ENABLE

#define TCFG_CLOCK_OSC_ADJUST_ENABLE              0                // 晶振电容调整
#if TCFG_CLOCK_OSC_ADJUST_ENABLE
#define TCFG_CLOCK_OSC_CAP                        7                // CAP
#define TCFG_CLOCK_OSC_CAP_FINE                   3                // CAP_FINE
#endif // TCFG_CLOCK_OSC_ADJUST_ENABLE
// ------------板级配置.json------------

// ------------功能配置.json------------
#define TCFG_APP_BT_EN                            1                // 蓝牙模式
#define TCFG_APP_MUSIC_EN                         0                // 音乐模式
#define TCFG_APP_PC_EN                            0                // PC模式
#define TCFG_MIC_EFFECT_ENABLE                    0                // 混响使能
#define TCFG_DEC_ID3_V2_ENABLE                    0                // ID3_V2
#define TCFG_DEC_ID3_V1_ENABLE                    0                // ID3_V1
#define FILE_DEC_REPEAT_EN                        0                // 无缝循环播放
#define FILE_DEC_DEST_PLAY                        0                // 指定时间播放
#define FILE_DEC_AB_REPEAT_EN                     0                // AB点复读
#define TCFG_DEC_DECRYPT_ENABLE                   0                // 加密文件播
#define TCFG_DEC_DECRYPT_KEY                      0x12345678       // 加密KEY
#define MUSIC_PLAYER_CYCLE_ALL_DEV_EN             0                // 循环播放模式是否循环所有设备
#define MUSIC_PLAYER_PLAY_FOLDER_PREV_FIRST_FILE_EN 0                // 切换文件夹播放时从第一首歌开始
#define TCFG_MUSIC_DEVICE_TONE_EN                 0                // 设备提示音
#define TWFG_APP_POWERON_IGNORE_DEV               4000             // 设备忽略时间（单位：ms）
#define TCFG_FIX_CLOCK_FREQ                       0                // 固定时钟频率
#define TCFG_MIX_RECORD_ENABLE                    0                // 本地混响录音使能
#define TCFG_RECORD_AUDIO_REPLAY_EN               0                // 本地录音回播使能
#define TCFG_AI_VOICE_ENABLE                      0                // AI VOICE录音使能
// ------------功能配置.json------------

// ------------按键配置.json------------
#define TCFG_LONG_PRESS_RESET_ENABLE              0                // 非按键长按复位配置
#if TCFG_LONG_PRESS_RESET_ENABLE
#define TCFG_LONG_PRESS_RESET_PORT                IO_PORTB_01      // 复位IO
#define TCFG_LONG_PRESS_RESET_TIME                8                // 复位时间
#define TCFG_LONG_PRESS_RESET_LEVEL               1                // 复位电平
#define TCFG_LONG_PRESS_RESET_INSIDE_PULL_UP_DOWN 0                // 使用IO内置上下拉
#endif // TCFG_LONG_PRESS_RESET_ENABLE

#define TCFG_IOKEY_ENABLE                         0                // IO按键配置

#define TCFG_ADKEY_ENABLE                         1                // AD按键配置

#define TCFG_QDEC_ENABLE                          0                // 旋转编码器配置

#define TCFG_LPTOUCH_ENABLE                       0                // 触摸按键配置

// ------------按键配置.json------------

// ------------电源配置.json------------
#define TCFG_CLOCK_OSC_HZ                         24000000         // 晶振频率
#define TCFG_LOWPOWER_POWER_SEL                   PWR_LDO15        // 电源模式
#define TCFG_DCDC_TYPE                            PWR_DCDC18_DCDC12 // DCDC类型
#define TCFG_LOWPOWER_OSC_TYPE                    OSC_TYPE_LRC     // 低功耗时钟源
#define TCFG_CLOCK_MODE                           CLOCK_MODE_ADAPTIVE // 时钟模式
#define TCFG_LOWPOWER_VDDIOM_LEVEL                VDDIOM_VOL_3P3V  // IOVDD
#define TCFG_LOWPOWER_VDDIOW_LEVEL                VDDIOW_VOL_2P6V  // 弱IOVDD
#define CONFIG_LVD_LEVEL                          2.5v             // LVD档位
#define TCFG_LOWPOWER_LOWPOWER_SEL                0                // 低功耗模式
#define TCFG_AUTO_POWERON_ENABLE                  1                // 上电自动开机
#define TCFG_DVDD_POWER_MODE                      CLOCK_DVDD_POWER_SELF // DVDD供电模式

#define TCFG_SYS_LVD_EN                           0                // 电池电量检测
#if TCFG_SYS_LVD_EN
#define TCFG_POWER_OFF_VOLTAGE                    3500             // 关机电压(mV)
#define TCFG_POWER_WARN_VOLTAGE                   3600             // 低电电压(mV)
#endif // TCFG_SYS_LVD_EN

#define TCFG_CHARGE_ENABLE                        0                // 充电配置
#if TCFG_CHARGE_ENABLE
#define TCFG_CHARGE_TRICKLE_MA                    CHARGE_TC_mA_20  // 涓流电流
#define TCFG_CHARGE_MA                            CHARGE_mA_200    // 恒流电流
#define TCFG_CHARGE_FULL_MA                       20               // 截止电流(mA)
#define TCFG_CHARGE_FULL_V                        CHARGE_FULL_V_4200 // 截止电压
#define TCFG_CHARGE_POWERON_ENABLE                1                // 开机充电
#define TCFG_CHARGE_OFF_POWERON_EN                1                // 拔出开机
#define TCFG_CHARGE_NVDC_EN                       1                // NVDC架构使能
#define TCFG_RECHARGE_ENABLE                      0                // 复充使能
#define TCFG_RECHARGE_VOLTAGE                     4000             // 复充电压(mV)
#define TCFG_LDOIN_PULLDOWN_EN                    1                // 下拉电阻开关
#define TCFG_LDOIN_PULLDOWN_LEV                   CHARGE_PULLDOWN_200K // 下拉电阻档位
#define TCFG_LDOIN_PULLDOWN_KEEP                  0                // 下拉电阻保持开关
#define TCFG_LDOIN_ON_FILTER_TIME                 100              // 入舱滤波时间(ms)
#define TCFG_LDOIN_OFF_FILTER_TIME                200              // 出舱滤波时间(ms)
#define TCFG_LDOIN_KEEP_FILTER_TIME               440              // 维持电压滤波时间(ms)
#endif // TCFG_CHARGE_ENABLE

#define TCFG_BATTERY_CURVE_ENABLE                 1                // 电池曲线配置

// ------------电源配置.json------------

// ------------GUI配置.json------------
#define TCFG_UI_ENABLE                            1                // GUI配置
#if TCFG_UI_ENABLE
#define CONFIG_JL_UI_ENABLE                       0                // JLUI
#define CONFIG_LVGL_UI_ENABLE                     1                // LVGL
#define TCFG_LCD_TP_USE_SAME_PWR                  1                // 触摸和屏幕公用电源
#define TCFG_TP_PIN_PWR                           NO_CONFIG_PORT   // 电源控制
#define TCFG_TP_PIN_RESET                         IO_PORTB_01      // 复位IO
#define TCFG_TP_PIN_INT                           IO_PORTB_04      // 中断IO
#define TCFG_TP_PIN_IIC_CH                        0x0              // IIC通道
#define CONFIG_DBI_TYPE_SPI                       0                // SPI
#define CONFIG_DBI_TYPE_MIPI                      1                // MIPI
#define CONFIG_DBI_TYPE_RGB                       2                // RGB
#define CONFIG_DBI_TYPE_MCU                       3                // MCU
#define TCFG_DBI_TYPE                             CONFIG_DBI_TYPE_MIPI // 接口类型
#define TCFG_LCD_PIN_EN                           NO_CONFIG_PORT   // 电源控制
#define TCFG_LCD_PIN_RESET                        IO_PORTB_11      // 复位IO
#define TCFG_LCD_PIN_BL                           IO_PORTE_04      // 背光控制
#define TCFG_LCD_PIN_BL_TYPE                      0                // 背光控制类型
#define TCFG_LCD_PIN_TE                           NO_CONFIG_PORT   // LCD TE
#define TCFG_LCD_PIN_CS                           IO_PORTB_05      // LCD CS
#define TCFG_LCD_PIN_SCL                          IO_PORTB_06      // LCD SCL
#define TCFG_LCD_PIN_RWX                          NO_CONFIG_PORT   // LCD RWX
#define TCFG_LCD_PIN_DC                           NO_CONFIG_PORT   // LCD DCX
#define TCFG_LCD_PIN_DATA_NUM                     4                // DATA数设置
#define TCFG_LCD_PIN_D0                           IO_PORTB_07      // SPI D0
#define TCFG_LCD_PIN_D1                           IO_PORTB_08      // SPI D1
#define TCFG_LCD_PIN_D2                           IO_PORTB_09      // SPI D2
#define TCFG_LCD_PIN_D3                           IO_PORTB_10      // SPI D3
#define TCFG_LCD_MCU_PIN_CS                       IO_PORTB_08      // LCD MCU CS
#define TCFG_LCD_MCU_PIN_RDX                      IO_PORTB_11      // LCD MCU RDX
#define TCFG_LCD_MCU_PIN_RWX                      IO_PORTB_06      // LCD MCU RWX
#define TCFG_LCD_MCU_PIN_DC                       IO_PORTB_07      // LCD MCU DCX
#define TCFG_LCD_MCU_PIN_DATA_NUM                 8                // DATA数设置
#define TCFG_LCD_MCU_PIN_DATA_CYCLE               1                // CYCLE数设置
#define TCFG_LCD_MCU_PIN_D0                       IO_PORTB_05      // MCU D0
#define TCFG_LCD_MCU_PIN_D1                       IO_PORTB_04      // MCU D1
#define TCFG_LCD_MCU_PIN_D2                       IO_PORTB_03      // MCU D2
#define TCFG_LCD_MCU_PIN_D3                       IO_PORTB_02      // MCU D3
#define TCFG_LCD_MCU_PIN_D4                       IO_PORTB_01      // MCU D4
#define TCFG_LCD_MCU_PIN_D5                       IO_PORTB_00      // MCU D5
#define TCFG_LCD_MCU_PIN_D6                       IO_PORTA_12      // MCU D6
#define TCFG_LCD_MCU_PIN_D7                       IO_PORTA_11      // MCU D7
#define TCFG_LCD_MCU_PIN_D8                       NO_CONFIG_PORT   // MCU D8
#define TCFG_LCD_MCU_PIN_D9                       NO_CONFIG_PORT   // MCU D9
#define TCFG_LCD_MCU_PIN_D10                      NO_CONFIG_PORT   // MCU D10
#define TCFG_LCD_MCU_PIN_D11                      NO_CONFIG_PORT   // MCU D11
#define TCFG_LCD_MCU_PIN_D12                      NO_CONFIG_PORT   // MCU D12
#define TCFG_LCD_MCU_PIN_D13                      NO_CONFIG_PORT   // MCU D13
#define TCFG_LCD_MCU_PIN_D14                      NO_CONFIG_PORT   // MCU D14
#define TCFG_LCD_MCU_PIN_D15                      NO_CONFIG_PORT   // MCU D15
#define TCFG_LCD_MCU_PIN_D16                      NO_CONFIG_PORT   // MCU D16
#define TCFG_LCD_MCU_PIN_D17                      NO_CONFIG_PORT   // MCU D17
#define TCFG_LCD_RGB_PIN_VSYNC                    IO_PORTB_08      // LCD RGB VSYNC
#define TCFG_LCD_RGB_PIN_HSYNC                    IO_PORTB_11      // LCD RGB HSYNC
#define TCFG_LCD_RGB_PIN_DCLK                     IO_PORTB_06      // LCD RGB DCLK
#define TCFG_LCD_RGB_PIN_DEN                      IO_PORTB_07      // LCD RGB DEN
#define TCFG_LCD_RGB_USE_SOFT_SPI                 0                // 使用软件SPI
#define TCFG_LCD_RGB_PIN_SPI_CS                   IO_PORTC_08      // LCD RGB SPI CS
#define TCFG_LCD_RGB_PIN_SPI_SCL                  IO_PORTC_05      // LCD RGB SPI SCL
#define TCFG_LCD_RGB_PIN_SPI_SDA                  IO_PORTC_07      // LCD RGB SPI SDA
#define TCFG_LCD_RGB_PIN_DATA_NUM                 8                // DATA数设置
#define TCFG_LCD_RGB_PIN_DATA_CYCLE               1                // CYCLE数设置
#define TCFG_LCD_RGB_PIN_D0                       IO_PORTB_05      // RGB D0
#define TCFG_LCD_RGB_PIN_D1                       IO_PORTB_04      // RGB D1
#define TCFG_LCD_RGB_PIN_D2                       IO_PORTB_03      // RGB D2
#define TCFG_LCD_RGB_PIN_D3                       IO_PORTB_02      // RGB D3
#define TCFG_LCD_RGB_PIN_D4                       IO_PORTB_01      // RGB D4
#define TCFG_LCD_RGB_PIN_D5                       IO_PORTB_00      // RGB D5
#define TCFG_LCD_RGB_PIN_D6                       IO_PORTA_12      // RGB D6
#define TCFG_LCD_RGB_PIN_D7                       IO_PORTA_11      // RGB D7
#define TCFG_MIPI_PIN_DATA_NUM                    2                // DATA数设置
#define TCFG_MIPI_PIN_X0                          DSI_MAP_CK_LANE  // MIPI X0
#define TCFG_MIPI_X0_INV                          0                // X0翻转DPDN
#define TCFG_MIPI_PIN_X1                          DSI_MAP_D1_LANE  // MIPI X1
#define TCFG_MIPI_X1_INV                          0                // X1翻转DPDN
#define TCFG_MIPI_PIN_X2                          DSI_MAP_D0_LANE  // MIPI X2
#define TCFG_MIPI_X2_INV                          0                // X2翻转DPDN
#endif // TCFG_UI_ENABLE
// ------------GUI配置.json------------

// ------------SENSOR配置.json------------
#define TCFG_VIDEO0_ENABLE                        1                // VIDEO0配置
#if TCFG_VIDEO0_ENABLE
#define TCFG_CAMERA_SPI_ENABLE                    0                // SPI
#define TCFG_CAMERA_DVP_ENABLE                    1                // DVP
#define TCFG_CAMERA_SPI_VCC                       IO_PORT_LDO28    // CAM_VCC
#define TCFG_CAMERA_SPI_PWDN                      IO_PORTH_06      // PWDN
#define TCFG_CAMERA_SPI_MCLK                      IO_PORTH_00      // MCLK
#define TCFG_CAMERA_SPI_PCLK                      IO_PORTH_01      // PCLK
#define TCFG_CAMERA_SPI_SCL                       IO_PORTH_04      // SCL
#define TCFG_CAMERA_SPI_SDA                       IO_PORTH_05      // SDA
#define TCFG_CAMERA_SPI_DATA0                     IO_PORTH_03      // DATA0
#define TCFG_CAMERA_SPI_DATA1                     IO_PORTH_02      // DATA1
#define TCFG_CAMERA_DVP_VCC                       IO_PORT_LDO28    // CAM_VCC
#define TCFG_CAMERA_DVP_PWDN                      NO_CONFIG_PORT   // PWDN
#define TCFG_CAMERA_DVP_RST                       IO_PORTH_00      // RST
#define TCFG_CAMERA_DVP_XCLK                      NO_CONFIG_PORT   // XCLK
#define TCFG_CAMERA_DVP_SCL                       IO_PORTD_11      // SCL
#define TCFG_CAMERA_DVP_SDA                       IO_PORTD_10      // SDA
#define TCFG_CAMERA_DVP_GROUP                     DVP_GROUP_C      // GROUP
#endif // TCFG_VIDEO0_ENABLE

#define TCFG_GSENSOR_ENABLE                       0                // Gsensor配置
#if TCFG_GSENSOR_ENABLE
#define TCFG_GSENSOR_PIN_PWR                      NO_CONFIG_PORT   // 电源控制
#define TCFG_GSENSOR_PIN_INT                      IO_PORTD_08      // 中断IO
#define TCFG_GSENSOR_PIN_IIC_CH                   0x6              // IIC通道
#endif // TCFG_GSENSOR_ENABLE
// ------------SENSOR配置.json------------

// ------------蓝牙配置.json------------
#define TCFG_BT_NAME_SEL_BY_AD_ENABLE             0                // 蓝牙名(AD采样)

#define TCFG_USER_BT_CLASSIC_ENABLE               1                // 经典蓝牙开关
#define TCFG_BT_PAGE_TIMEOUT                      8                // 单次回连时间(s)
#define TCFG_BT_POWERON_PAGE_TIME                 30               // 开机回连超时(s)
#define TCFG_BT_TIMEOUT_PAGE_TIME                 120              // 超距断开回连超时(s)
#define TCFG_DUAL_CONN_INQUIRY_SCAN_TIME          120              // 开机可被发现时间 (s)
#define TCFG_AUTO_SHUT_DOWN_TIME                  0                // 无连接关机时间(s)
#define TCFG_BT_VOL_SYNC_ENABLE                   1                // 音量同步
#define TCFG_BT_MUSIC_INFO_ENABLE                 1                // 歌曲信息显示
#define TCFG_BT_DISPLAY_BAT_ENABLE                0                // 电量显示
#define TCFG_BT_INBAND_RING                       1                // 手机铃声
#define TCFG_BT_PHONE_NUMBER_ENABLE               0                // 来电报号
#define TCFG_BT_SUPPORT_AAC                       0                // AAC
#define TCFG_BT_MSBC_EN                           1                // MSBC
#define TCFG_BT_SBC_BITPOOL                       38               // sbcBitPool
#define TCFG_AAA_BITRATE                          320000           // AAC码率
#define TCFG_BT_SUPPORT_HFP                       1                // HFP
#define TCFG_BT_SUPPORT_AVCTP                     1                // AVRCP
#define TCFG_BT_SUPPORT_A2DP                      1                // A2DP
#define TCFG_BT_SUPPORT_HID                       1                // HID
#define TCFG_BT_SUPPORT_SPP                       1                // SPP
#define TCFG_BT_SUPPORT_PNP                       1                // PNP
#define TCFG_BT_SUPPORT_PBAP                      0                // PBAP
#define TCFG_BT_SUPPORT_MAP                       1                // MAP
#define TCFG_BT_SUPPORT_OPP                       0                // OPP
#define TCFG_BT_SUPPORT_LHDC                      0                // LHDC
#define TCFG_BT_SUPPORT_LDAC                      0                // LDAC
#define TCFG_BT_BACKGROUND_ENABLE                 1                // 蓝牙后台
#define TCFG_BT_SUPPORT_PBAP_LIST                 1                // pbap单条查询
#define TCFG_BT_CALL_PHONE_BY_WATCH               1                // 通话拦截
#define TCFG_USER_EMITTER_ENABLE                  0                // 发射器开关
#define CONFIG_BT_MODE                            1                // 模式选择
#define TCFG_NORMAL_SET_DUT_MODE                  0                // NORMAL模式下使能DUT测试

#define TCFG_BT_SNIFF_ENABLE                      1                // sniff
#if TCFG_BT_SNIFF_ENABLE
#define TCFG_SNIFF_CHECK_TIME                     5                // 检测时间
#define CONFIG_LRC_WIN_SIZE                       400              // LRC窗口初值
#define CONFIG_LRC_WIN_STEP                       400              // LRC窗口步进
#define CONFIG_OSC_WIN_SIZE                       400              // OSC窗口初值
#define CONFIG_OSC_WIN_STEP                       400              // OSC窗口步进
#endif // TCFG_BT_SNIFF_ENABLE

#define TCFG_USER_BLE_ENABLE                      1                // BLE
#if TCFG_USER_BLE_ENABLE
#define TCFG_BT_BLE_TX_POWER                      5                // 最大发射功率
#define TCFG_BT_BLE_BREDR_SAME_ADDR               1                // 与2.1同地址
#define TCFG_BLE_BRIDGE_EDR_ENALBE                1                // CTKD一键连接
#define TCFG_USER_BLE_CTRL_BREDR_EN               0                // BLE控制EDR
#define TCFG_BLE_ADV_DYNAMIC_SWITCH               0                // BLE广播动态调整
#endif // TCFG_USER_BLE_ENABLE

#define TCFG_BT_AI_ENABLE                         1                // AI配置
#if TCFG_BT_AI_ENABLE
#define TCFG_BT_AI_SEL_PROTOCOL                   RCSP_MODE_EN     // AI协议选择
#endif // TCFG_BT_AI_ENABLE
// ------------蓝牙配置.json------------

// ------------网络配置.json------------
#define TCFG_BT_NET_ENABLE                        1                // 蓝牙共享网络

#define TCFG_EXT_WIFI_ENABLE                      1                // 外挂WIFI
#if TCFG_EXT_WIFI_ENABLE
#define TCFG_AIC8800D40_ENABLE                    1                // AIC8800D40
#define TCFG_AIC8800D40_SDC_CH                    0                // SDC通道
#define TCFG_EXT_WIFI_MODE                        0x4              // MODE（bit0-AP; bit1-STA; bit2-P2P）
#define TCFG_EXT_WIFI_AP_SSID                     "JL380N-AP"      // AP SSID
#define TCFG_EXT_WIFI_AP_PWD                      "12345678"       // AP PWD
#define TCFG_EXT_WIFI_STA_SSID                    "JL380N-STA"     // STA SSID
#define TCFG_EXT_WIFI_STA_PWD                     "12345678"       // STA PWD
#define TCFG_EXT_WIFI_P2P_SSID                    "JL380N-P2P"     // P2P SSID
#define TCFG_EXT_WIFI_RST_PIN                     NO_CONFIG_PORT   // RST PIN
#endif // TCFG_EXT_WIFI_ENABLE

#define TCFG_UART_NET_ENABLE                      0                // UART网卡
#if TCFG_UART_NET_ENABLE
#define TCFG_UART_NETIF_TX_PIN                    NO_CONFIG_PORT   // TX
#define TCFG_UART_NETIF_RX_PIN                    NO_CONFIG_PORT   // RX
#define TCFG_UART_NETIF_BAUD                      4000000          // 波特率
#endif // TCFG_UART_NET_ENABLE

#define TCFG_LTE_PHY_ENABLE                       1                // 4G网卡
#if TCFG_LTE_PHY_ENABLE
#define TCFG_HOST_WIRELESS_ENABLE                 1                // USB网卡
#endif // TCFG_LTE_PHY_ENABLE

#define TCFG_SPI_NET_ENABLE                       0                // SPI网卡

// ------------网络配置.json------------

// ------------升级配置.json------------
#define TCFG_UPDATE_ENABLE                        1                // 升级选择
#if TCFG_UPDATE_ENABLE
#define TCFG_TEST_BOX_ENABLE                      0                // 测试盒串口升级
#define TCFG_UPDATE_UART_IO_EN                    0                // 普通io串口升级
#define TCFG_UPDATE_UART_ROLE                     0                // 串口升级主从机选择
#define TCFG_DEV_UPDATE_EN                        0                // 设备升级（USB、SD卡）
#define TCFG_COMPELTE_UFW_NAND_EN                 0                // 升级文件存放本地触发升级（nandflash）
#define TCFG_COMPELTE_UFW_NOR_EN                  0                // 升级文件存放本地触发升级（norflash升级）
#define TCFG_COMPELTE_UFW_LC_EN                   0                // 升级文件存放本地触发升级（内置flash升级）
#define TCFG_NORFLASH_UPDATE_EN                   0                // norflash_flash升级，目前功能没用到
#define TCFG_UPDATE_RESOURCE_EN                   1                // 升级资源（除代码以外的其他部分）
#define TCFG_TEST_BOX_WIRELESS_EN                 0                // 测试盒无线升级
#define TCFG_APP_UPDATE_EN                        0                // APP升级
#define TCFG_HTTP_UPDATE_ENABLE                   1                // HTTP升级
#define TCFG_CHARGESTORE_PORT                     IO_PORTA_01      // 串口通信IO
#endif // TCFG_UPDATE_ENABLE
// ------------升级配置.json------------

// ------------音频配置.json------------
#define TCFG_AUDIO_GLOBAL_SAMPLE_RATE             44100            // 全局采样率
#define TCFG_AUDIO_DAC_MODE                       DAC_MODE_DIFF    // 输出模式
#define TCFG_AUDIO_DAC_CONNECT_MODE               DAC_OUTPUT_MONO_L // 声道配置
#define TCFG_DAC_PERFORMANCE_MODE                 DAC_MODE_HIGH_PERFORMANCE // 性能模式
#define TCFG_DAC_POWER_MODE                       0                // 输出功率
#define TCFG_DAC_POWER_LEVEL                      0x3              // 音频供电档位
#define TCFG_AUDIO_VCM_CAP_EN                     0x1              // VCM电容
#define TCFG_AUDIO_FILTER_SELECT                  AUDIO_FILTER_BALANCE // Filter配置
#define TCFG_AUDIO_DAC_BUFFER_TIME_MS             50               // DMA长度(ms)
#define TCFG_AUDIO_DAC_IO_ENABLE                  0                // DAC_IO
#define TCFG_AUDIO_DAC_HP_PA_ISEL0                4                // PA_ISEL0
#define TCFG_AUDIO_DAC_HP_PA_ISEL1                4                // PA_ISEL1
#define TCFG_AUDIO_DAC_LP_PA_ISEL0                2                // PA_ISEL0
#define TCFG_AUDIO_DAC_LP_PA_ISEL1                2                // PA_ISEL1
#define TCFG_AUDIO_L_CHANNEL_GAIN                 0x03             // L Channel
#define TCFG_AUDIO_R_CHANNEL_GAIN                 0x03             // R Channel
#define TCFG_AUDIO_RL_CHANNEL_GAIN                0x03             // RL Channel
#define TCFG_AUDIO_DIGITAL_GAIN                   0                // Digital Gain
#define TCFG_AUDIO_DAC_PA_PORT                    IO_PORTH_08      // IO选择
#define TCFG_AUDIO_DAC_PA_MUTE_LEVEL              0                // MUTE

#define TCFG_AUDIO_ADC_ENABLE                     1                // ADC配置
#if TCFG_AUDIO_ADC_ENABLE
#define TCFG_AUDIO_MIC_LDO_VSEL                   4                // MICLDO电压
#define TCFG_ADC_PERFORMANCE_MODE                 ADC_MODE_HIGH_PERFORMANCE // 性能模式
#define TCFG_ADC_LOW_LATENCY_FRAME_SIZE           0                // ADC低延时点数
#define TCFG_ADC0_ENABLE                          1                // 使能
#define TCFG_ADC0_MODE                            1                // 模式
#define TCFG_ADC0_AIN_SEL                         32               // 输入端口
#define TCFG_ADC0_BIAS_SEL                        48               // 供电端口
#define TCFG_ADC0_BIAS_RSEL                       12               // MIC BIAS上拉电阻挡位
#define TCFG_ADC0_POWER_IO                        0                // IO供电选择
#define TCFG_ADC0_DCC_EN                          1                // DCC使能
#define TCFG_ADC0_DCC_LEVEL                       5                // DCC 截止频率
#define TCFG_ADC0_USE_CASE                        1                // 使用场景
#define TCFG_ADC0_DIGITAL_GAIN                    0                // 数字增益
#define TCFG_ADC1_ENABLE                          1                // 使能
#define TCFG_ADC1_MODE                            1                // 模式
#define TCFG_ADC1_AIN_SEL                         64               // 输入端口
#define TCFG_ADC1_BIAS_SEL                        48               // 供电端口
#define TCFG_ADC1_BIAS_RSEL                       12               // MIC BIAS上拉电阻挡位
#define TCFG_ADC1_POWER_IO                        0                // IO供电选择
#define TCFG_ADC1_DCC_EN                          1                // DCC使能
#define TCFG_ADC1_DCC_LEVEL                       5                // DCC 截止频率
#define TCFG_ADC1_USE_CASE                        1                // 使用场景
#define TCFG_ADC1_DIGITAL_GAIN                    0                // 数字增益
#define TCFG_ADC2_ENABLE                          1                // 使能
#define TCFG_ADC2_MODE                            1                // 模式
#define TCFG_ADC2_AIN_SEL                         32               // 输入端口
#define TCFG_ADC2_BIAS_SEL                        48               // 供电端口
#define TCFG_ADC2_BIAS_RSEL                       12               // MIC BIAS上拉电阻挡位
#define TCFG_ADC2_POWER_IO                        0                // IO供电选择
#define TCFG_ADC2_DCC_EN                          1                // DCC使能
#define TCFG_ADC2_DCC_LEVEL                       5                // DCC 截止频率
#define TCFG_ADC2_USE_CASE                        1                // 使用场景
#define TCFG_ADC2_DIGITAL_GAIN                    0                // 数字增益
#define TCFG_ADC3_ENABLE                          1                // 使能
#define TCFG_ADC3_MODE                            0                // 模式
#define TCFG_ADC3_AIN_SEL                         32               // 输入端口
#define TCFG_ADC3_BIAS_SEL                        8                // 供电端口
#define TCFG_ADC3_BIAS_RSEL                       12               // MIC BIAS上拉电阻挡位
#define TCFG_ADC3_POWER_IO                        0                // IO供电选择
#define TCFG_ADC3_DCC_EN                          1                // DCC使能
#define TCFG_ADC3_DCC_LEVEL                       5                // DCC 截止频率
#define TCFG_ADC3_USE_CASE                        1                // 使用场景
#define TCFG_ADC3_DIGITAL_GAIN                    0                // 数字增益
#define TCFG_LINEIN0_ENABLE                       1                // 使能
#define TCFG_LINEIN0_MODE                         0                // 模式
#define TCFG_LINEIN0_AIN_SEL                      1                // 输入端口
#define TCFG_LINEIN0_DCC_EN                       1                // DCC使能
#define TCFG_LINEIN0_DCC_LEVEL                    5                // DCC截止频率
#define TCFG_LINEIN1_ENABLE                       1                // 使能
#define TCFG_LINEIN1_MODE                         0                // 模式
#define TCFG_LINEIN1_AIN_SEL                      2                // 输入端口
#define TCFG_LINEIN1_DCC_EN                       1                // DCC使能
#define TCFG_LINEIN1_DCC_LEVEL                    5                // DCC截止频率
#define TCFG_LINEIN2_ENABLE                       1                // 使能
#define TCFG_LINEIN2_MODE                         0                // 模式
#define TCFG_LINEIN2_AIN_SEL                      4                // 输入端口
#define TCFG_LINEIN2_DCC_EN                       1                // DCC使能
#define TCFG_LINEIN2_DCC_LEVEL                    5                // DCC截止频率
#define TCFG_LINEIN3_ENABLE                       1                // 使能
#define TCFG_LINEIN3_MODE                         0                // 模式
#define TCFG_LINEIN3_AIN_SEL                      8                // 输入端口
#define TCFG_LINEIN3_DCC_EN                       1                // DCC使能
#define TCFG_LINEIN3_DCC_LEVEL                    5                // DCC截止频率
#endif // TCFG_AUDIO_ADC_ENABLE

#define TCFG_AEC_TOOL_ONLINE_ENABLE               0                // 手机APP在线调试
#define TCFG_AUDIO_CVP_SYNC                       0                // 通话上行同步
#define TCFG_AUDIO_DMS_DUT_ENABLE                 1                // 通话产测
#define TCFG_ESCO_DL_CVSD_SR_USE_16K              1                // ESCO通话全局采样率
#define TCFG_AUDIO_SMS_TDE_STEREO_REF_ENABLE      0                // 1mic立体声回音消除
#define TCFG_AUDIO_SMS_SEL                        SMS_DEFAULT      // 1mic算法选择
#define TCFG_MUSIC_PLC_TYPE                       1                // PLC类型选择
#define TCFG_KWS_VOICE_RECOGNITION_ENABLE         0                // 关键词检测KWS
#define TCFG_KWS_MIC_CHA_SELECT                   AUDIO_ADC_MIC_0  // 麦克风选择
#define TCFG_SMART_VOICE_ENABLE                   0                // 离线语音识别
#define TCFG_SMART_VOICE_USE_AEC                  0                // 回音消除使能
#define TCFG_SMART_VOICE_MIC_CH_SEL               AUDIO_ADC_MIC_0  // 麦克风选择
#define TCFG_AUDIO_ASR_ALGO_SELECT                0                // 算法选择
#define TCFG_AUDIO_KWS_LANGUAGE_SEL               KWS_CH           // 模式选择
#define TCFG_DEC_SBC_ENABLE                       1                // SBC
#define TCFG_DEC_MSBC_ENABLE                      1                // MSBC
#define TCFG_DEC_WAV_ENABLE                       1                // WAV（倒车提示音）
#define TCFG_DEC_MP3_ENABLE                       0                // MP3
#define TCFG_DEC_AAC_ENABLE                       0                // AAC
#define TCFG_DEC_OPUS_ENABLE                      0                // OPUS
#define TCFG_OPUS_DEC_SUPPORT_TYPE                1                // OPUS支持解码类型
#define TCFG_OPUS_FRAME_LEN                       40               // 帧长(byte)
#define TCFG_ENC_OPUS_TYPE                        2                // OPUS封装
#define TCFG_DEC_F2A_ENABLE                       0                // F2A
#define TCFG_DEC_WTG_ENABLE                       1                // WTG
#define TCFG_DEC_MTY_ENABLE                       0                // MTY
#define TCFG_DEC_WTS_ENABLE                       0                // WTS
#define TCFG_DEC_JLA_ENABLE                       0                // JLA
#define TCFG_DEC_STREAM_MP3_ENABLE                0                // STREAM MP3
#define TCFG_ENC_MSBC_ENABLE                      1                // MSBC
#define TCFG_ENC_CVSD_ENABLE                      1                // CVSD
#define TCFG_ENC_JLA_ENABLE                       0                // JLA
#define TCFG_ENC_SBC_ENABLE                       1                // SBC
#define TCFG_ENC_PCM_ENABLE                       0                // PCM
#define TCFG_ENC_OPUS_ENABLE                      0                // OPUS
#define TCFG_ENC_SPEEX_ENABLE                     0                // SPEEX
#define TCFG_ENC_MP3_ENABLE                       0                // MP3
#define TCFG_ENC_MP3_TYPE                         0                // MP3格式选择
#define TCFG_ENC_ADPCM_ENABLE                     0                // ADPCM
#define TCFG_ENC_ADPCM_TYPE                       0                // ADPCM格式选择
#define ALINK0_SAMPLE_RATE                        0                // Sample Rate
#define ALINK0_CH_NUM                             2                // Channels
#define ALINK0_WORK_MODE                          1                // Work Mode
#define ALINK0_CLK_MODE                           0                // Clk Mode
#define ALINK0_BIT_WIDTH                          -1               // Bit Width
#define ALINK0_IO_DRIVE_STRENGTH_MODE             255              // Drive Strength
#define ALINK1_SAMPLE_RATE                        0                // Sample Rate
#define ALINK1_CH_NUM                             2                // Channels
#define ALINK1_WORK_MODE                          1                // Work Mode
#define ALINK1_CLK_MODE                           0                // Clk Mode
#define ALINK1_BIT_WIDTH                          -1               // Bit Width
#define ALINK1_IO_DRIVE_STRENGTH_MODE             255              // Drive Strength

#define TCFG_AUDIO_AUX_ANALOG_PATH_ENABLE         0                // 模拟AUX配置
#if TCFG_AUDIO_AUX_ANALOG_PATH_ENABLE
#define TCFG_AUDIO_AUX_CH                         3                // 通道使能
#define TCFG_AUDIO_AUX0_AIN                       AUX_AIN_PORT1    // 输入选择
#define TCFG_AUDIO_AUX0_GAIN_BOOST                0                // 前级增益
#define TCFG_AUDIO_AUX1_AIN                       AUX_AIN_PORT0    // 输入选择
#define TCFG_AUDIO_AUX1_GAIN_BOOST                0                // 前级增益
#endif // TCFG_AUDIO_AUX_ANALOG_PATH_ENABLE

#define TCFG_DATA_EXPORT_UART_TX_PORT             IO_PORT_DM       // 串口发送引脚
#define TCFG_DATA_EXPORT_UART_BAUDRATE            2000000          // 串口波特率
#define TCFG_AUDIO_DATA_EXPORT_TYPE               0                // 串口导出使能
// ------------音频配置.json------------
#endif
