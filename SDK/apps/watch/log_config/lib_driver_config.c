#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_driver_config.data.bss")
#pragma data_seg(".lib_driver_config.data")
#pragma const_seg(".lib_driver_config.text.const")
#pragma code_seg(".lib_driver_config.text")
#endif
/*********************************************************************************************
    *   Filename        : lib_driver_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 14:58

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "debug.h"
#include "gpadc.h"
#include "asm/clock_hal.h"

// *INDENT-OFF*

#if TCFG_SD0_SD1_USE_THE_SAME_HW
const int sd0_sd1_use_the_same_hw = 1;
#else
const int sd0_sd1_use_the_same_hw = 0;
#endif

#if TCFG_SD0_KEEP_CARD_AT_ACTIVE_STATUS || TCFG_SD1_KEEP_CARD_AT_ACTIVE_STATUS
const int keep_card_at_active_status = 1;
#else
const int keep_card_at_active_status = 0;
#endif

#if TCFG_SD0_CAN_OPERATE_MMC_CARD || TCFG_SD1_CAN_OPERATE_MMC_CARD
const int sdx_can_operate_mmc_card = 1;
#else
const int sdx_can_operate_mmc_card = 0;
#endif

const int sdx_dev_cache_sector_num = 8;//卡驱动使用dma_malloc申请内存的扇区个数
#if(TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL) //系统时钟源选择
const int  clock_sys_src_use_lrc_hw = 1; //当使用lrc时timer.c需要特殊设置
#else
const int  clock_sys_src_use_lrc_hw = 0;
#endif
#ifdef TCFG_VAD_LOWPOWER_CLOCK
const char config_vad_lowpower_clock = TCFG_VAD_LOWPOWER_CLOCK;
#else
const char config_vad_lowpower_clock = 0;
#endif

#if defined(CONFIG_CPU_BR28)
const int config_otg_slave_detect_method_2 = 1;
#else
const int config_otg_slave_detect_method_2 = 0;
#endif

const int chip_is_extern_vdd = TCFG_DVDD_POWER_MODE;

//gpadc驱动开启 “电池电压”和“温度采集功能”

const u8 adc_vbat_ch_en = 1;
const u8 adc_vtemp_ch_en = 1;
const u32 lib_adc_clk_max = 500 * 1000;
const u8 gpadc_battery_mode = WEIGHTING_MODE; //使用IOVDD供电时,禁止使用 MEAN_FILTERING_MODE 模式


#define IS_VALID_EXT_VBAT_CH(ch) \
    ((ch) && (ch) != ADC_CH_PMU_VBAT_4 && (ch) != ADC_CH_PMU_VBAT_2)


// 默认：内部 1/4_VBAT
#define GPADC_POWER_CH        ADC_CH_PMU_VBAT_4
#define GPADC_POWER_DIV       4
#define GPADC_POWER_MODE      2   // VBAT
#define GPADC_EXTERN_TRIM     0


#if defined(TCFG_POWER_SUPPLY_MODE)

    #if (TCFG_POWER_SUPPLY_MODE == 0)

        #undef  GPADC_POWER_CH
        #undef  GPADC_POWER_DIV
        #undef  GPADC_POWER_MODE

        #define GPADC_POWER_CH    AD_CH_IOVDD
        #define GPADC_POWER_DIV   1
        #define GPADC_POWER_MODE  0

    #elif (TCFG_POWER_SUPPLY_MODE == 1)

        #undef  GPADC_POWER_CH
        #undef  GPADC_POWER_DIV
        #undef  GPADC_POWER_MODE

        #define GPADC_POWER_CH    ADC_CH_PMU_VPWR_4
        #define GPADC_POWER_DIV   4
        #define GPADC_POWER_MODE  1

    #endif

#else

    #if defined(TCFG_VBAT_CH) && IS_VALID_EXT_VBAT_CH(TCFG_VBAT_CH)

        // 外部分压 VBAT
        #undef  GPADC_POWER_CH
        #undef  GPADC_POWER_DIV
        #undef  GPADC_POWER_MODE

        #define GPADC_POWER_CH    TCFG_VBAT_CH
        #define GPADC_POWER_DIV   TCFG_BAT_EXT_DIV
        #define GPADC_POWER_MODE  3

    #else

        // 内部 1/2_VBAT
        #ifdef ADC_CH_PMU_VBAT_2

            #undef  GPADC_POWER_CH
            #undef  GPADC_POWER_DIV

            #define GPADC_POWER_CH    ADC_CH_PMU_VBAT_2
            #define GPADC_POWER_DIV   2

        #else

            // 内部 1/4_VBAT
            #undef  GPADC_POWER_CH
            #undef  GPADC_POWER_DIV

            #define GPADC_POWER_CH    ADC_CH_PMU_VBAT_4
            #define GPADC_POWER_DIV   4

        #endif

    #endif

#endif


const u32 gpadc_ch_power        = GPADC_POWER_CH;
const u8  gpadc_ch_power_div    = GPADC_POWER_DIV;
const u8  gpadc_power_supply_mode = GPADC_POWER_MODE;


const u16 gpadc_battery_trim_vddiom_voltage = 3300;
const u16 gpadc_battery_trim_voltage = 3700;

#if defined(TCFG_BAT_EX_ADJUST_ENABLE) && TCFG_BAT_EX_ADJUST_ENABLE
const u16 gpadc_extern_voltage_trim = 2500;
#else
const u16 gpadc_extern_voltage_trim = 0;
#endif


//sfc
#if (defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE)
const u8 norflash_ext_sfc_en = 1;//给gpu判断互斥
#else
const u8 norflash_ext_sfc_en = 0;
#endif

#if (CONFIG_FLASH_DTR_EN)
const u8 sfc0_dtr_mode_en = 1;
#else
const u8 sfc0_dtr_mode_en = 0;
#endif
const s8 sfc0_dtr_dummy_num = -1;//查看flash文档填入ini对应线宽的dummy
const u32 sfc0_dtr_clk_freq = 128000000;//flash文档*2
const u8 tzflash_write_protect_mode_en = 1;
#if (CONFIG_SPI_MODE == 2)
const u8 sfc0_continue_mode_en = 1;
#else
const u8 sfc0_continue_mode_en = 0;
#endif
/* 是否开启把vm配置项暂存到ram的功能 */
/* 具体使用方法和功能特性参考《项目帮助文档》的“11.4. 配置项管理 -VM配置项暂存RAM功能描述” */
const char vm_ram_storage_enable = FALSE;  // vm_ram_storage_enable会与在中断里面写到ram的功能冲突
// 在中断写vm时,会先暂存到ram. TRUE:在关机等操作前刷新到flash避免丢失数据; FALSE:关机时不刷新
const char vm_ram_storage_in_irq_enable = TRUE;  // vm_ram_storage_enable会与在中断里面写到ram的功能冲突

/* 设置vm的ram内存缓存区总大小限制，0为不限制，非0为限制大小（单位/byte）。 */
const int  vm_ram_storage_limit_data_total_size  = 16 * 1024;

//gptimer
const u32 lib_gptimer_src_lsb_clk = 0; //时钟源选择lsb_clk, 单位:MHz
const u32 lib_gptimer_src_std_clk = 12 * 1000 * 1000; //时钟源选择std_x_clk, 单位:Hz
const u8 lib_gptimer_timer_mode_en = 1; //gptimer timer功能使能
const u8 lib_gptimer_pwm_mode_en = 1; //gptimer pwm功能使能
const u8 lib_gptimer_capture_mode_en = 1; //gptimer capture功能使能
const u8 lib_gptimer_auto_tid_en = 1; //gptimer_tid 内部自动分配使能
const u8 lib_gptimer_extern_use = GPTIMER_EXTERN_USE; //gptimer 模块外部已经占用, bit0=1表示timer0 外部占用，以此类推

const u32 lib_config_uart_flow_enable = 1;

// sram
const int lib_config_enable_sram_share = !!TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM;

//秘钥鉴权相关
#define     AUTH_multi_algorithm (1<<0)   //多算法授权
#define     AUTH_mkey_check      (1<<1)   //二级秘钥
#define     AUTH_sdk_chip_key    (1<<2)   //SDK秘钥校验

//需要对应的功能，就或上对应的宏定义，支持多种鉴权同时打开
const u32 lib_config_enable_auth_check = 0b0000
#if TCFG_VBAT_TRIM_EN || \
    TCFG_SMART_VOICE_ENABLE
        | AUTH_multi_algorithm
#endif
        ;

//CAN
const u16 config_can_dma_ram_malloc_size = 200; // can-dma内部申请缓存帧大小，申请地址内部ram，必须是IOCTL_CAN_SET_DMA_FRAMES的整数倍，建议小于300帧，实际申请大小config_can_dma_ram_malloc_size * 16 byte;
/**
 * @brief Log (Verbose/Debug/Info/Warn/Error/Char)
 */
/*-----------------------------------------------------------*/

REGISTER_LOG_TAG(FALSE, TRUE,  FALSE, TRUE,  TRUE,  FALSE, CLOCK)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, LP_TIMER)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, LRC)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, RCH)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, P33_MISC)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, P33)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, PMU)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, RTC)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, WKUP)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, SDFILE)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, CHARGE)

REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, DEBUG)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, PWM_LED)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, PWM)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, KEY)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, TMR)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, VM)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, TZFLASH)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, TRIM_VDD)

// Audio DAC 及其他
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, SYS_DAC)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, APP_EDET)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FM)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, CORE)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, CACHE)

// 低功耗与通讯外设
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LP_KEY)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, LP_NFC)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, SD)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, CAN)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, EAR_DETECT)
REGISTER_LOG_TAG(FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, TDM)
REGISTER_LOG_TAG(FALSE, FALSE, TRUE,  TRUE,  TRUE,  FALSE, USB)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, UART)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, IIC)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, SPI)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, GPIO)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, EXTI)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, GPTIMER)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, ADVTMR)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, PERI)
REGISTER_LOG_TAG(FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, GPADC)
