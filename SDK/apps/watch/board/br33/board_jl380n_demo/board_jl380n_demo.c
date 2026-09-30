#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".board_config.data.bss")
#pragma data_seg(".board_config.data")
#pragma const_seg(".board_config.text.const")
#pragma code_seg(".board_config.text")
#endif


#include "app_config.h"

#ifdef CONFIG_BOARD_JL380N_DEMO

#include "system/includes.h"
#include "app_main.h"
#include "spi.h"
#include "asm/sdmmc.h"
#include "asm/spi_hw.h"
#include "asm/lpctmu_hw.h"
#include "asm/psram_api.h"
#include "asm/can.h"
#include "linein_dev.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_storage.h"
#include "usb/host/uvc_device.h"
#include "iic_api.h"
#include "include/norflash_sfc.h"
#include "fs/virfat_flash.h"
#include "alarm.h"
#include "data_storage.h"
#include "tp_api.h"
#include "gpadc.h"
#include "rdec_key.h"
#include "asm/espi.h"
#include "../sdk_config.c"
#include "nandflash.h"
#include "device/device.h"
#include "asm/sdmmc.h"
#include "bt.h"
#if TCFG_VIDEO0_ENABLE
#include "uvc_device.h"
#include "camera.h"
#include "video_info.h"
#include "fb.h"
#endif
#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/lcd/lcd_drive.h"
#endif
#if TCFG_EXT_WIFI_ENABLE
#include "wireless/wifi_dev.h"
#endif

#if TCFG_LTE_PHY_ENABLE
#include "lte_module.h"
#endif


#if (CONFIG_BT_MODE != BT_NORMAL) || TCFG_NORMAL_SET_DUT_MODE
#if ((TCFG_RDEC0_ECODEA_PORT==IO_PORT_DP) || (TCFG_RDEC0_ECODEA_PORT==IO_PORT_DM)) || \
	((TCFG_RDEC0_ECODEB_PORT==IO_PORT_DP) || (TCFG_RDEC0_ECODEB_PORT==IO_PORT_DM))
// 蓝牙测试会用到DPDM
#undef TCFG_RDEC_KEY_ENABLE
#define TCFG_RDEC_KEY_ENABLE					DISABLE_THIS_MOUDLE
#endif
#endif

#if TCFG_EXT_WIFI_ENABLE
WIFI_PLATFORM_DATA_BEGIN(wifi_data)
#if TCFG_AIC8800D40_ENABLE
.module = AIC8800D40,
#elif TCFG_RTL8733BS_ENABLE
.module = RTL8733BS,
#endif
.sdio_parm = 0 | 0 | SDIO_4_BIT_DATA | SDIO_DATA1_IRQ | SDIO_CLOCK_20M,
.wakeup_port = -1,//IO_PORTB_11,
.cs_port = -1,
.hd_level = 2,		//IO强驱等级，0:2p4mA   1:8p0mA  2:24p0mA  3:64p0mA


#if (TCFG_AIC8800D40_SDC_CH == 0)
.port = {
    TCFG_SD0_PORT_CMD,
    TCFG_SD0_PORT_CLK,
    TCFG_SD0_PORT_DA0,
    TCFG_SD0_PORT_DA1,
    TCFG_SD0_PORT_DA2,
    TCFG_SD0_PORT_DA3,
},
.power_port = TCFG_SD0_POWER_PORT,

#elif (TCFG_AIC8800D40_SDC_CH == 1)
.port = {
    TCFG_SD1_PORT_CMD,
    TCFG_SD1_PORT_CLK,
    TCFG_SD1_PORT_DA0,
    TCFG_SD1_PORT_DA1,
    TCFG_SD1_PORT_DA2,
    TCFG_SD1_PORT_DA3,
},
.power_port = TCFG_SD1_POWER_PORT,

#else
#error "TCFG_AIC8800D40_SDC_CH only support 0(SD0) or 1(SD1)"
#endif

WIFI_PLATFORM_DATA_END()

#endif //TCFG_EXT_WIFI_ENABLE

const struct wifi_platform_data * get_wifi_platform_data(void)
{
#if TCFG_EXT_WIFI_ENABLE
    return &wifi_data;
#else
    return NULL;
#endif
}


#if defined(TCFG_MULTI_CHIP_CONN_ENABLE) && TCFG_MULTI_CHIP_CONN_ENABLE

#include "jldtp/jldtp_manager.h"
#include "jldtp/spi_transport.h"
#include "jldtp/uart_transport.h"
#include "spi.h"

#if (TCFG_MULTI_CHIP_CONN_TYPE == CHIP_CONN_TYPE_SPI)
static jldtp_spi_config_t g_chip_conn_periph_cfg = {
    .spi      = HW_SPI1,
    .mode     = SPI_MODE_UNIDIR_2BIT,
    .clk_pin  = TCFG_SPI_CLK_PIN,
    .do_pin   = TCFG_SPI_DO_PIN,
    .di_pin   = TCFG_SPI_DI_PIN,
    .d2_pin   = 0xff,
    .d3_pin   = 0xff,
    .cs_pin   = TCFG_SPI_CS_PIN,
    .freq     = TCFG_SPI_BAUD,
    .irq_priority = 3,
};
#else
static jldtp_uart_config_t g_chip_conn_periph_cfg = {
    .tx_pin   = IO_PORTA_01,
    .rx_pin   = IO_PORTA_01,
    .baud_rate = 8000000,
    .irq_priority = 3,
};
#endif

const struct multi_chip_conn_config chip_conn_configs [] =  {
    {
        .role = JLDTP_SLAVE,
        .remote_chip_id = MULTI_CHIP_ID_1,
        .mtu_size = 1540,
        .periph_cfg = (void *) &g_chip_conn_periph_cfg,
    }
};
#endif // TCFG_MULTI_CHIP_CONN_ENABLE

#ifndef CONFIG_EXTRAM_CODE_ENABLE
#if TCFG_PSRAM_DEV_ENABLE
PSRAM_PLATFORM_DATA_BEGIN(psram_config)
.init_clk = TCFG_PSRAM_INIT_CLK,//hz
PSRAM_PLATFORM_DATA_END()
#endif//TCFG_PSRAM_DEV_ENABLE
#endif


#if TCFG_SD0_ENABLE
// TODO

#if TCFG_SD0_ALWAY_ONLINE_ENABLE
int custom_sd0_io_detect(void *data)
{
    return 1;
}
#endif

#if (TCFG_SD0_POWER_SEL == SD_PWR_SDPG)

#define sd0_power_config		sdpg_config

#elif defined(TCFG_SD0_POWER_PORT) && (TCFG_SD0_POWER_PORT!=NO_CONFIG_PORT)
#define SD0_POWER_ALONE		1
static u8 sd0_power_enable = 0xff;
void sd0_power_config(int enable)
{
    if (sd0_power_enable == enable) {
        return;
    }
    sd0_power_enable = enable;
    //TODO: 使用普通io供电
    if (enable) {
        /* printf("\n\n sd power enable \n\n"); */
#if TCFG_SD0_POWER_PORT_LEVEL
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_SD0_POWER_PORT), PORT_DRIVE_STRENGT_64p0mA);
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD0_POWER_PORT), PORT_OUTPUT_HIGH);
#else
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD0_POWER_PORT), PORT_OUTPUT_LOW);
#endif
    } else {
        /* printf("\n\n sd power disable \n\n"); */
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD0_POWER_PORT), PORT_HIGHZ);
    }
}


int sd0_power_status(void)
{
    if (sd0_power_enable == 0xff) {
        return SDPG_IO_HI_Z;
    }
    return sd0_power_enable;
}


#else
#define sd0_power_config(...)
#endif

SDMMC_CARD_DECT_PLATFORM_DATA_BEGIN(sd0_dect_pdata) = {
    .detect_io = {
        TCFG_SD0_PORT_CMD,
        TCFG_SD0_PORT_CLK,
#if (TCFG_SD0_DET_MODE == SD_CLK_DECT)
        TCFG_SD0_PORT_CLK,
#elif (TCFG_SD0_DET_MODE == SD_PG_ADC_DECT)
        IO_PORT_SDPG,
#else
        TCFG_SD0_DET_IO,
#endif
    },
    .detect_io_level = TCFG_SD0_DET_IO_LEVEL,
#if ((TCFG_SD0_DET_MODE == SD_IO_ADC_DECT) || (TCFG_SD0_DET_MODE == SD_PG_ADC_DECT))
    .detect_less_ad = TCFG_SD0_DET_LESS_AD,
#endif
#if TCFG_SD0_ALWAY_ONLINE_ENABLE
    .detect_mode = SD_IO_LEV_DECT,
    .detect_func = custom_sd0_io_detect,
#else
    .detect_mode = TCFG_SD0_DET_MODE,
    .detect_func = sd_io_detect,
#endif
    .detect_cmd_sdc = TCFG_SD0_SDC_TYPE,
    .detect_io_private_operation = sd_dect_gpio_private_operation,
    SDMMC_CARD_DECT_PLATFORM_DATA_END()
};

SDMMC_CARD_PLATFORM_DATA_BEGIN(sd0_data) = {
    .port = {
        TCFG_SD0_PORT_CMD,
        TCFG_SD0_PORT_CLK,
        TCFG_SD0_PORT_DA0,
        TCFG_SD0_PORT_DA1,
        TCFG_SD0_PORT_DA2,
        TCFG_SD0_PORT_DA3,
    },
    .data_width = TCFG_SD0_DAT_MODE,
    .speed = TCFG_SD0_CLK,
#if (TCFG_SD0_POWER_SEL == SD_PWR_SDPG)
    .power = sdpg_config,
    .power_status = get_sdpg_status,
#elif defined(TCFG_SD0_POWER_PORT) && (TCFG_SD0_POWER_PORT!=NO_CONFIG_PORT)
    .power = sd0_power_config,
    .power_status = sd0_power_status,
#else
    .power = NULL,
    .power_status = NULL,
#endif
    .dect_pdata = &sd0_dect_pdata,
    .sdc    = TCFG_SD0_SDC_TYPE,
    .port_private_operation = sd_gpio_private_operation,
    SDMMC_CARD_PLATFORM_DATA_END()
};

#endif /* #if TCFG_SD0_ENABLE */

#if TCFG_SD1_ENABLE
// TODO

#if TCFG_SD1_ALWAY_ONLINE_ENABLE
int custom_sd1_io_detect(void *data)
{
    return 1;
}
#endif

#if (TCFG_SD1_POWER_SEL == SD_PWR_SDPG)

#define sd1_power_config		sdpg_config

#elif defined(TCFG_SD1_POWER_PORT) && (TCFG_SD1_POWER_PORT!=NO_CONFIG_PORT)
#define SD1_POWER_ALONE		1
static u8 sd1_power_enable = 0xff;
void sd1_power_config(int enable)
{
    if (sd1_power_enable == enable) {
        return;
    }
    sd1_power_enable = enable;
    //TODO: 使用普通io供电
    if (enable) {
        /* printf("\n\n sd power enable \n\n"); */
#if TCFG_SD1_POWER_PORT_LEVEL
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_SD1_POWER_PORT), PORT_DRIVE_STRENGT_64p0mA);
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD1_POWER_PORT), PORT_OUTPUT_HIGH);
#else
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD1_POWER_PORT), PORT_OUTPUT_LOW);
#endif
    } else {
        /* printf("\n\n sd power disable \n\n"); */
        gpio_set_mode(IO_PORT_SPILT(TCFG_SD1_POWER_PORT), PORT_HIGHZ);
    }
}


int sd1_power_status(void)
{
    if (sd1_power_enable == 0xff) {
        return SDPG_IO_HI_Z;
    }
    return sd1_power_enable;
}


#else
#define sd1_power_config(...)
#endif

SDMMC_CARD_DECT_PLATFORM_DATA_BEGIN(sd1_dect_pdata) = {
    .detect_io = {
        TCFG_SD1_PORT_CMD,
        TCFG_SD1_PORT_CLK,
#if (TCFG_SD1_DET_MODE == SD_CLK_DECT)
        TCFG_SD1_PORT_CLK,
#elif (TCFG_SD1_DET_MODE == SD_PG_ADC_DECT)
        IO_PORT_SDPG,
#else
        TCFG_SD1_DET_IO,
#endif
    },
    .detect_io_level = TCFG_SD1_DET_IO_LEVEL,
#if ((TCFG_SD1_DET_MODE == SD_IO_ADC_DECT) || (TCFG_SD1_DET_MODE == SD_PG_ADC_DECT))
    .detect_less_ad = TCFG_SD1_DET_LESS_AD,
#endif
#if TCFG_SD1_ALWAY_ONLINE_ENABLE
    .detect_mode = SD_IO_LEV_DECT,
    .detect_func = custom_sd1_io_detect,
#else
    .detect_mode = TCFG_SD1_DET_MODE,
    .detect_func = sd_io_detect,
#endif
    .detect_cmd_sdc = TCFG_SD1_SDC_TYPE,
    .detect_io_private_operation = sd_dect_gpio_private_operation,
    SDMMC_CARD_DECT_PLATFORM_DATA_END()
};

SDMMC_CARD_PLATFORM_DATA_BEGIN(sd1_data) = {
    .port = {
        TCFG_SD1_PORT_CMD,
        TCFG_SD1_PORT_CLK,
        TCFG_SD1_PORT_DA0,
        TCFG_SD1_PORT_DA1,
        TCFG_SD1_PORT_DA2,
        TCFG_SD1_PORT_DA3,
    },
    .data_width = TCFG_SD1_DAT_MODE,
    .speed = TCFG_SD1_CLK,
#if (TCFG_SD1_POWER_SEL == SD_PWR_SDPG)
    .power = sdpg_config,
    .power_status = get_sdpg_status,
#elif defined(TCFG_SD1_POWER_PORT) && (TCFG_SD1_POWER_PORT!=NO_CONFIG_PORT)
    .power = sd1_power_config,
    .power_status = sd1_power_status,
#else
    .power = NULL,
    .power_status = NULL,
#endif
    .dect_pdata = &sd1_dect_pdata,
    .sdc    = TCFG_SD1_SDC_TYPE,
    .port_private_operation = sd_gpio_private_operation,
    SDMMC_CARD_PLATFORM_DATA_END()
};

#endif /* #if TCFG_SD1_ENABLE */


#if TCFG_CAMERA_DVP_ENABLE
const struct camera_platform_data camera0_data = {
    .xclk_gpio      = TCFG_CAMERA_DVP_XCLK,
    .reset_gpio     = TCFG_CAMERA_DVP_RST,
    .pwdn_gpio      = TCFG_CAMERA_DVP_PWDN,
    .power_value    = 0,
    .interface      = 0,
    .dvp = {
#if (TCFG_CAMERA_DVP_GROUP == DVP_GROUP_A)
        .group_port  = DVP_GROUP_A,
        .pclk_gpio   = IO_PORTB_02,
        .vsync_gpio  = IO_PORTA_09,
        .hsync_gpio  = IO_PORTA_10,
        .data_gpio = {
            IO_PORTB_00,
            IO_PORTB_01,
            IO_PORTB_03,
            IO_PORTB_04,
            IO_PORTB_05,
            IO_PORTB_06,
            IO_PORTA_08,
            IO_PORTA_07,
            IO_PORTA_06,
            IO_PORTA_05,
        },
#elif (TCFG_CAMERA_DVP_GROUP == DVP_GROUP_B)
        .group_port  = DVP_GROUP_B,
        .pclk_gpio   = IO_PORTB_02,
        .vsync_gpio  = IO_PORTB_00,
        .hsync_gpio  = IO_PORTB_01,
        .data_gpio = {
            IO_PORTB_03,
            IO_PORTB_04,
            IO_PORTB_05,
            IO_PORTB_06,
            IO_PORTC_00,
            IO_PORTC_01,
            IO_PORTC_02,
            IO_PORTC_03,
            IO_PORTC_04,
            IO_PORTC_05,
        },
#elif (TCFG_CAMERA_DVP_GROUP == DVP_GROUP_C)
        .timing_mode = 3,
        .group_port  = DVP_GROUP_C,
        .pclk_gpio   = IO_PORTH_01,
        .vsync_gpio  = IO_PORTD_10,
        .hsync_gpio  = IO_PORTD_11,
        .data_gpio = {
            IO_PORTH_07,
            IO_PORTH_06,
            IO_PORTH_05,
            IO_PORTH_04,
            IO_PORTH_03,
            IO_PORTH_02,
            IO_PORTD_09,
            IO_PORTD_08,
        },
#elif (TCFG_CAMERA_DVP_GROUP == DVP_GROUP_D)
        .group_port  = DVP_GROUP_D,
        .pclk_gpio   = IO_PORTC_04,
        .vsync_gpio  = IO_PORTC_06,
        .hsync_gpio  = IO_PORTC_15,
        .data_gpio = {
            IO_PORTC_06,
            IO_PORTC_05,
            IO_PORTC_03,
            IO_PORTC_02,
            IO_PORTC_01,
            IO_PORTC_00,
            IO_PORTG_03,
            IO_PORTG_02,
            IO_PORTG_01,
            IO_PORTG_00,
        },
#else
#error "DVP GROUP ERROR"
#endif
    }
};

static const struct video_subdevice_data video0_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *) &camera0_data },
};
static const struct video_platform_data video0_data = {
    .data = video0_subdev_data,
    .num = ARRAY_SIZE(video0_subdev_data),
};
#endif

#if TCFG_CAMERA_SPI_ENABLE
const struct camera_platform_data camera0_data = {
    .xclk_gpio      = TCFG_CAMERA_SPI_MCLK,
    .reset_gpio     = -1,
    .pwdn_gpio      = TCFG_CAMERA_SPI_PWDN,
    .power_value    = 0,
    .interface      = 0,
    .dvp = {
        .group_port  = DVP_GROUP_C,
        .pclk_gpio   = TCFG_CAMERA_SPI_PCLK,
        .vsync_gpio  = -1,
        .hsync_gpio  = -1,
        .timing_mode = 3,
        .msb_position = 5,
        .reverse = 0,
        .bus_width = 2,
        .data_gpio = {
            TCFG_CAMERA_SPI_DATA0,
            TCFG_CAMERA_SPI_DATA1,
        },
    }
};

static const struct video_subdevice_data video0_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *) &camera0_data },
};
static const struct video_platform_data video0_data = {
    .data = video0_subdev_data,
    .num = ARRAY_SIZE(video0_subdev_data),
};
#endif


// *INDENT-OFF*
#if TCFG_LPCTMU_ENABLE
LPCTMU_PLATFORM_DATA_BEGIN(lpctmu_pdata)
#if LPCTMU_ANA_CFG_ADAPTIVE
    .aim_vol_delta              = 800,
    .aim_charge_khz             = 2500,
#else
    .hv_level                   = 3,
    .lv_level                   = 0,
    .cur_level                  = 7,
#endif
LPCTMU_PLATFORM_DATA_END();

LPCTMU_CFG_DATA_BEGIN(lpctmu_cfg)
    .ch_en = ((TCFG_LPCTMU_CH0_EN << 0) | \
              (TCFG_LPCTMU_CH1_EN << 1) | \
              (TCFG_LPCTMU_CH2_EN << 2) | \
              (TCFG_LPCTMU_CH3_EN << 3) | \
              (TCFG_LPCTMU_CH4_EN << 4)),
    .pdata = &lpctmu_pdata,
LPCTMU_CFG_DATA_END();

#endif


/************************** linein KEY ****************************/
#if TCFG_APP_LINEIN_EN
struct linein_dev_data linein_data = {
    .enable = TCFG_APP_LINEIN_EN,
    .port   = NO_CONFIG_PORT,
    .up     = 1,
    .down   = 0,
    .ad_channel = NO_CONFIG_PORT,
    .ad_vol = 0,
};
#endif

#if TCFG_UI_ENABLE

#if TCFG_SPI_LCD_ENABLE
const struct dbi_signal_remap develop_board_qspi_signal_remap_tab[] = {
    {DBI_CSX, TCFG_LCD_PIN_CS},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_SCL, TCFG_LCD_PIN_SCL},//SPI - SCL, MCU - WRX, RGB - DCLK
#if ((defined TCFG_LCD_PIN_RWX)&&(TCFG_LCD_PIN_RWX != NO_CONFIG_PORT))
    {DBI_RWX, TCFG_LCD_PIN_RWX},
#endif
#if ((defined TCFG_LCD_PIN_DC)&&(TCFG_LCD_PIN_DC != NO_CONFIG_PORT))
    {DBI_DCX, TCFG_LCD_PIN_DC},
#endif
	//data
    {DBI_D0, TCFG_LCD_PIN_D0},
#if ((defined TCFG_LCD_PIN_DATA_NUM)&&(TCFG_LCD_PIN_DATA_NUM > 1))
    {DBI_D1, TCFG_LCD_PIN_D1},
#if ((defined TCFG_LCD_PIN_DATA_NUM)&&(TCFG_LCD_PIN_DATA_NUM == 4))
    {DBI_D2, TCFG_LCD_PIN_D2},
    {DBI_D3, TCFG_LCD_PIN_D3},
#endif // TCFG_LCD_PIN_DATA_NUM == 4
#endif // TCFG_LCD_PIN_DATA_NUM > 1
};

const struct dbi_signal_remap rm69330_qspi_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTB_05},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_SCL, DBI_PORTB_06},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_D0, DBI_PORTB_07},
    {DBI_D1, DBI_PORTB_08},
    {DBI_DCX, DBI_PORTB_09},
};

const struct dbi_signal_remap dbi_qspi_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTB_05},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_SCL, DBI_PORTB_06},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_D0, DBI_PORTB_07},
    {DBI_D1, DBI_PORTB_08},
    {DBI_D2, DBI_PORTB_09},
    {DBI_D3, DBI_PORTB_10},
};

#if defined(LCD_BOARD_DIP) && LCD_BOARD_DIP
// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_8bit_rgb_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTC_05},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, DBI_PORTC_04},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, DBI_PORTC_02},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, DBI_PORTC_06},//MCU - RDX, RGB - DEN
    {DBI_D0, DBI_PORTB_06},
    {DBI_D1, DBI_PORTB_07},
    {DBI_D2, DBI_PORTB_08},
    {DBI_D3, DBI_PORTB_09},
    {DBI_D4, DBI_PORTA_09},
    {DBI_D5, DBI_PORTA_10},
    {DBI_D6, DBI_PORTA_11},
    {DBI_D7, DBI_PORTA_12},
};
#else
// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_8bit_rgb_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTB_05},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, DBI_PORTB_06},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, DBI_PORTB_07},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, DBI_PORTB_08},//MCU - RDX, RGB - DEN
    {DBI_D0, DBI_PORTB_09},
    {DBI_D1, DBI_PORTB_10},
    {DBI_D2, DBI_PORTB_11},
    {DBI_D3, DBI_PORTG_00},
    {DBI_D4, DBI_PORTG_01},
    {DBI_D5, DBI_PORTG_02},
    {DBI_D6, DBI_PORTG_03},
    {DBI_D7, DBI_PORTC_00},
};
#endif

// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_24bit_rgb_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTC_07},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, DBI_PORTC_05},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, DBI_PORTC_04},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, DBI_PORTC_08},//MCU - RDX, RGB - DEN

    {DBI_D0,  DBI_PORTB_11},//B0
    {DBI_D1,  DBI_PORTG_01},//B1
    {DBI_D2,  DBI_PORTG_02},//B2
    {DBI_D3,  DBI_PORTG_03},//B3
    {DBI_D4,  DBI_PORTC_00},//B4
    {DBI_D5,  DBI_PORTC_01},//B5
    {DBI_D6,  DBI_PORTC_02},//B6
    {DBI_D7,  DBI_PORTC_03},//B7

    {DBI_D8,  DBI_PORTB_03},//G0
    {DBI_D9,  DBI_PORTB_04},//G1
    {DBI_D10, DBI_PORTB_09},//G2
    {DBI_D11, DBI_PORTB_10},//G3
    {DBI_D12, DBI_PORTB_07},//G4
    {DBI_D13, DBI_PORTB_08},//G5
    {DBI_D14, DBI_PORTB_05},//G6
    {DBI_D15, DBI_PORTB_06},//G7

    {DBI_D16, DBI_PORTA_08},//R0
    {DBI_D17, DBI_PORTA_09},//R1
    {DBI_D18, DBI_PORTA_10},//R2
    {DBI_D19, DBI_PORTA_11},//R3
    {DBI_D20, DBI_PORTA_12},//R4
    {DBI_D21, DBI_PORTB_00},//R5
    {DBI_D22, DBI_PORTB_01},//R6
    {DBI_D23, DBI_PORTB_02},//R7
};

const struct dbi_signal_remap dbi_rgb_signal_remap_tab[] = {
    {DBI_CSX, TCFG_LCD_RGB_PIN_VSYNC},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, TCFG_LCD_RGB_PIN_HSYNC},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, TCFG_LCD_RGB_PIN_DCLK},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, TCFG_LCD_RGB_PIN_DEN},//MCU - RDX, RGB - DEN

    {DBI_D0,  TCFG_LCD_RGB_PIN_D0},
    {DBI_D1,  TCFG_LCD_RGB_PIN_D1},
    {DBI_D2,  TCFG_LCD_RGB_PIN_D2},
    {DBI_D3,  TCFG_LCD_RGB_PIN_D3},
    {DBI_D4,  TCFG_LCD_RGB_PIN_D4},
    {DBI_D5,  TCFG_LCD_RGB_PIN_D5},
    {DBI_D6,  TCFG_LCD_RGB_PIN_D6},
    {DBI_D7,  TCFG_LCD_RGB_PIN_D7},

#if (TCFG_LCD_RGB_PIN_DATA_NUM > 8)
#error "rgb gpio need set"
#endif
};

// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_8bit_mcu_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTB_05},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, DBI_PORTB_06},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, DBI_PORTB_07},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, DBI_PORTB_08},//MCU - RDX, RGB - DEN

    {DBI_D0,  DBI_PORTB_09},
    {DBI_D1,  DBI_PORTB_10},
    {DBI_D2,  DBI_PORTB_11},
    {DBI_D3,  DBI_PORTG_00},
    {DBI_D4,  DBI_PORTG_01},
    {DBI_D5,  DBI_PORTG_02},
    {DBI_D6,  DBI_PORTG_03},
    {DBI_D7,  DBI_PORTC_00},
};

// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_18bit_mcu_signal_remap_tab[] = {
    {DBI_CSX, DBI_PORTA_07},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, DBI_PORTA_09},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, DBI_PORTA_11},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, DBI_PORTB_00},//MCU - RDX, RGB - DEN

    {DBI_D0,  DBI_PORTB_02},
    {DBI_D1,  DBI_PORTB_04},
    {DBI_D2,  DBI_PORTB_06},
    {DBI_D3,  DBI_PORTB_08},
    {DBI_D4,  DBI_PORTB_10},
    {DBI_D5,  DBI_PORTG_00},
    {DBI_D6,  DBI_PORTG_02},
    {DBI_D7,  DBI_PORTC_00},

    {DBI_D8,  DBI_PORTC_02},
    {DBI_D9,  DBI_PORTC_04},
    {DBI_D10, DBI_PORTC_06},
    {DBI_D11, DBI_PORTA_08},
    {DBI_D12, DBI_PORTA_10},
    {DBI_D13, DBI_PORTA_12},
    {DBI_D14, DBI_PORTB_01},
    {DBI_D15, DBI_PORTB_03},

    {DBI_D16, DBI_PORTB_05},
    {DBI_D17, DBI_PORTB_07},
};

// DBI硬件支持映射的IO，请参考enum DBI_IO
const struct dbi_signal_remap dbi_mcu_signal_remap_tab[] = {
    {DBI_CSX, TCFG_LCD_MCU_PIN_CS},//SPI/MCU - DBI_CSX, RGB - VSYNC
    {DBI_DCX, TCFG_LCD_MCU_PIN_DC},//SPI/MCU - DBI_DCX, RGB - HSYNC
    {DBI_SCL, TCFG_LCD_MCU_PIN_RWX},//SPI - SCL, MCU - WRX, RGB - DCLK
    {DBI_RWX, TCFG_LCD_MCU_PIN_RDX},//MCU - RDX, RGB - DEN

    {DBI_D0,  TCFG_LCD_MCU_PIN_D0},
    {DBI_D1,  TCFG_LCD_MCU_PIN_D1},
    {DBI_D2,  TCFG_LCD_MCU_PIN_D2},
    {DBI_D3,  TCFG_LCD_MCU_PIN_D3},
    {DBI_D4,  TCFG_LCD_MCU_PIN_D4},
    {DBI_D5,  TCFG_LCD_MCU_PIN_D5},
    {DBI_D6,  TCFG_LCD_MCU_PIN_D6},
    {DBI_D7,  TCFG_LCD_MCU_PIN_D7},

#if (TCFG_LCD_MCU_PIN_DATA_NUM > 8)
    {DBI_D8,  TCFG_LCD_MCU_PIN_D8},
    {DBI_D9,  TCFG_LCD_MCU_PIN_D9},
    {DBI_D10, TCFG_LCD_MCU_PIN_D10},
    {DBI_D11, TCFG_LCD_MCU_PIN_D11},
    {DBI_D12, TCFG_LCD_MCU_PIN_D12},
    {DBI_D13, TCFG_LCD_MCU_PIN_D13},
    {DBI_D14, TCFG_LCD_MCU_PIN_D14},
    {DBI_D15, TCFG_LCD_MCU_PIN_D15},
#endif

#if (TCFG_LCD_MCU_PIN_DATA_NUM > 16)
    {DBI_D16, TCFG_LCD_MCU_PIN_D16},
    {DBI_D17, TCFG_LCD_MCU_PIN_D17},
#endif

#if (TCFG_LCD_MCU_PIN_DATA_NUM > 18)
#error "mcu gpio need set"
#endif
};

static void lcd_mipi_reset_enter(int pin_reset)
{
    gpio_set_mode(IO_PORT_SPILT(pin_reset), PORT_OUTPUT_LOW);
    os_time_dly(1);
}

static void lcd_mipi_reset_exit(int pin_reset)
{
    gpio_set_mode(IO_PORT_SPILT(pin_reset), PORT_OUTPUT_HIGH);
    os_time_dly(10);
}

static void board_lcd_highz_init(void)
{
	//仪表板子MIPI Lane和一些IO内绑在一起，使用时需要设为高阻
    gpio_set_mode(PORTB, PORT_PIN_5 | PORT_PIN_6 | PORT_PIN_7
                        | PORT_PIN_8 | PORT_PIN_9 | PORT_PIN_10, PORT_HIGHZ);

    gpio_set_mode(PORTG, PORT_PIN_0, PORT_HIGHZ);
}

//推屏使用有专门硬件模块,不是普通spi模块,io固定,根据屏幕驱动类似输出时序
LCD_SPI_PLATFORM_DATA_BEGIN(lcd_spi_data) = {
    .pin_reset = TCFG_LCD_PIN_RESET,
    .pin_en = TCFG_LCD_PIN_EN,
    .pin_en_ex = NO_CONFIG_PORT,
    .pin_te = TCFG_LCD_PIN_TE,
    .pin_bl = TCFG_LCD_PIN_BL,
	.board_pre_init = board_lcd_highz_init,
    // DBI io映射
#if (defined(TCFG_LCD_RGB_ST7701S_480x800_ENABLE) && TCFG_LCD_RGB_ST7701S_480x800_ENABLE) ||\
      (defined(TCFG_LCD_RGB_FL58202_800x480_ENABLE) && TCFG_LCD_RGB_FL58202_800x480_ENABLE) ||\
      (defined(TCFG_LCD_RGB_480x272_ENABLE) && TCFG_LCD_RGB_480x272_ENABLE)
#if (TCFG_DBI_TYPE != CONFIG_DBI_TYPE_RGB)
#error "DBI TYPE ERROR, is RGB"
#endif
    .dbi_hw = {
        .remap_tab = dbi_rgb_signal_remap_tab,
        .remap_tab_size = sizeof(dbi_rgb_signal_remap_tab) / sizeof(dbi_rgb_signal_remap_tab[0]),
        .rgb = {//任意gpio
#if ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==6)
            .data_bus = DATA_BUS_6BIT,
#elif ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==8)
            .data_bus = DATA_BUS_8BIT,
#elif ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==9)
            .data_bus = DATA_BUS_9BIT,
#elif ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==16)
            .data_bus = DATA_BUS_16BIT,
#elif ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==18)
            .data_bus = DATA_BUS_18BIT,
#elif ((TCFG_LCD_RGB_PIN_DATA_NUM/TCFG_LCD_RGB_PIN_DATA_CYCLE)==24)
            .data_bus = DATA_BUS_24BIT,
#else
#error "RGB data & cycle error"
#endif
            .use_soft_spi = TCFG_LCD_RGB_USE_SOFT_SPI,
            .soft_spi_cs = TCFG_LCD_RGB_PIN_SPI_CS,
            .soft_spi_scl = TCFG_LCD_RGB_PIN_SPI_SCL,
            .soft_spi_sda = TCFG_LCD_RGB_PIN_SPI_SDA,
        },
        .eva_link_mode = LINKAGE_MODE_DMM,
    }
#elif (defined(TCFG_LCD_MCU_JD5858_360x360_ENABLE) && TCFG_LCD_MCU_JD5858_360x360_ENABLE) ||\
      (defined(TCFG_LCD_MCU_NV3031A_240x320_ENABLE) && TCFG_LCD_MCU_NV3031A_240x320_ENABLE) ||\
      (defined(TCFG_LCD_MCU_ST7789V_240x320_ENABLE) && TCFG_LCD_MCU_ST7789V_240x320_ENABLE) ||\
      (defined(TCFG_LCD_MCU_ST7789V_240x320_ENABLE) && TCFG_LCD_MCU_ST7789V_240x320_ENABLE)
#if (TCFG_DBI_TYPE != CONFIG_DBI_TYPE_MCU)
#error "DBI TYPE ERROR, is MCU"
#endif
    .dbi_hw = {
        .remap_tab = dbi_mcu_signal_remap_tab,
        .remap_tab_size = sizeof(dbi_mcu_signal_remap_tab) / sizeof(dbi_mcu_signal_remap_tab[0]),
        .mcu = {
#if ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==6)
            .data_bus = DATA_BUS_6BIT,
#elif ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==8)
            .data_bus = DATA_BUS_8BIT,
#elif ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==9)
            .data_bus = DATA_BUS_9BIT,
#elif ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==16)
            .data_bus = DATA_BUS_16BIT,
#elif ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==18)
            .data_bus = DATA_BUS_18BIT,
#elif ((TCFG_LCD_MCU_PIN_DATA_NUM/TCFG_LCD_MCU_PIN_DATA_CYCLE)==24)
            .data_bus = DATA_BUS_24BIT,
#else
#error "MCU data & cycle error"
#endif
            .use_rd_pin = 1,
        },
		.eva_link_mode = LINKAGE_MODE_GPU,
    }
#elif (defined(TCFG_LCD_MIPI_ST7701S_480x854_ENABLE) && TCFG_LCD_MIPI_ST7701S_480x854_ENABLE) ||\
      (defined(TCFG_LCD_MIPI_ILI9881C_800x1280_ENABLE) && TCFG_LCD_MIPI_ILI9881C_800x1280_ENABLE) || \
	  (defined(TCFG_LCD_MIPI_CO5300_466x466_ENABLE) && TCFG_LCD_MIPI_CO5300_466x466_ENABLE) || \
	  (defined(TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE) && TCFG_LCD_MIPI_JD9165T_1024x600_ENABLE)
#if (TCFG_DBI_TYPE != CONFIG_DBI_TYPE_MIPI)
#error "DBI TYPE ERROR, is MIPI"
#endif
    .dbi_hw = {
        .mipi = {
            .lane_num = TCFG_MIPI_PIN_DATA_NUM,
            .x0_sel = TCFG_MIPI_PIN_X0,
            .x0_inv = TCFG_MIPI_X0_INV,
            .x1_sel = TCFG_MIPI_PIN_X1,
            .x1_inv = TCFG_MIPI_X1_INV,
            .x2_sel = TCFG_MIPI_PIN_X2,
            .x2_inv = TCFG_MIPI_X2_INV,
            // lcd_reset_enter -> LP11 -> lcd_reset_exit复位流程(部分MIPI屏需要按照该复位流程进行复位)
            /* .lcd_reset_pin = TCFG_LCD_PIN_RESET, */
            /* .lcd_reset_enter = lcd_mipi_reset_enter, */
            /* .lcd_reset_exit = lcd_mipi_reset_exit, */
        },
		.eva_link_mode = LINKAGE_MODE_DMM,
        .jstc_en = 1,
    }
#else
#if (TCFG_DBI_TYPE != CONFIG_DBI_TYPE_SPI)
#error "DBI TYPE ERROR, is SPI"
#endif
    .dbi_hw = {
        .remap_tab = develop_board_qspi_signal_remap_tab,
        .remap_tab_size = sizeof(develop_board_qspi_signal_remap_tab) / sizeof(develop_board_qspi_signal_remap_tab[0]),
#if CONFIG_LVGL_UI_ENABLE
		.eva_link_mode = LINKAGE_MODE_GPU,
#endif
    }
#endif
    LCD_SPI_PLATFORM_DATA_END()
};

const struct ui_devices_cfg ui_cfg_data = {
    .type = TFT_LCD,
    .private_data = (void *) &lcd_spi_data,
};
#endif /*TCFG_SPI_LCD_ENABLE*/

#endif /*TCFG_UI_ENABLE*/

const struct iic_master_config soft_iic_cfg_const[MAX_SOFT_IIC_NUM] = {
    //soft iic0
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_SW_I2C0_CLK_PORT,
        .sda_io = TCFG_SW_I2C0_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_SW_I2C0_DELAY_CNT,  //IIC通讯波特率 未使用
        .io_filter = 0,                        //软件iic无滤波器
    },
    //soft iic1
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_SW_I2C1_CLK_PORT,
        .sda_io = TCFG_SW_I2C1_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_SW_I2C1_DELAY_CNT,  //IIC通讯波特率 未使用
        .io_filter = 0,                        //软件iic无滤波器
    },
};

const struct iic_master_config hw_iic_cfg_const[MAX_HW_IIC_NUM] = {
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C0_CLK_PORT,
        .sda_io = TCFG_HW_I2C0_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C0_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic0
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C1_CLK_PORT,
        .sda_io = TCFG_HW_I2C1_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C1_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic1
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C2_CLK_PORT,
        .sda_io = TCFG_HW_I2C2_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C2_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic2
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C3_CLK_PORT,
        .sda_io = TCFG_HW_I2C3_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C3_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic3
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C4_CLK_PORT,
        .sda_io = TCFG_HW_I2C4_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C4_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic4
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_I2C5_CLK_PORT,
        .sda_io = TCFG_HW_I2C5_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C5_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//iic5
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_PMC_I2C0_CLK_PORT,
        .sda_io = TCFG_HW_PMC_I2C0_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_PMC_I2C0_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//pmc iic0
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_HW_PMC_I2C1_CLK_PORT,
        .sda_io = TCFG_HW_PMC_I2C1_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_5K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_PMC_I2C1_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },//pmc iic1
};

#if TCFG_CAN_ENABLE
CAN_PLATFORM_DATA_BEGIN(can_data)
    .can_mode               = TCFG_CAN_MODE,
    .rx_pin                 = TCFG_CAN_RX_IO,
    .tx_pin                 = TCFG_CAN_TX_IO,
    .stm                    = TCFG_CAN_STM_ENABLE,
    .lom                    = TCFG_CAN_LOM_ENABLE,
    .baudrate = {
       .brp                 = TCFG_CAN_BAUD_BRP,
       .sjw                 = TCFG_CAN_BAUD_SJW,
       .sam                 = TCFG_CAN_BAUD_SAM,
       .tseg2               = TCFG_CAN_BAUD_PBS2,
       .tseg1               = TCFG_CAN_BAUD_PTS_PBS1,
    },
    .filter = {
        .filter_mode        = TCFG_CAN_FILTER_MODE,
        .blacklist_en       = TCFG_CAN_FILTER_BLACKLIST_EN,
#if TCFG_CAN_MODE
        .accept_id0         = TCFG_CAN_FILTER_ACC_ID0,
        .accept_id1         = TCFG_CAN_FILTER_ACC_ID1,
        .accept_rtr0        = TCFG_CAN_FILTER_ACC_RTR0,
        .accept_rtr1        = TCFG_CAN_FILTER_ACC_RTR1,
        .mask_id0           = TCFG_CAN_FILTER_MASK_ID0,
        .mask_id1           = TCFG_CAN_FILTER_MASK_ID1,
        .mask_rtr0          = TCFG_CAN_FILTER_MASK_RTR0,
        .mask_rtr1          = TCFG_CAN_FILTER_MASK_RTR1,
#else
        .accept_id0         = TCFG_CAN_FILTER_ACC_ID0,
        .mask_id0           = TCFG_CAN_FILTER_MASK_ID0,
#endif
	},
    .priority               = 5,
    .cpu_id                 = 0,
CAN_PLATFORM_DATA_END()
#endif

#if (TCFG_HW_SPI1_ENABLE || TCFG_HW_SPI2_ENABLE)
const struct spi_platform_data spix_p_data[HW_SPI_MAX_NUM] = {
    {
        //spi0
    },
#if SUPPORT_SPI1
    {
        //spi1
        .port = {
            TCFG_HW_SPI1_PORT_CLK, //clk any io
            TCFG_HW_SPI1_PORT_DO, //do any io
            TCFG_HW_SPI1_PORT_DI, //di any io
            0xff, //d2 any io
            0xff, //d3 any io
            0xff, //cs any io(主机不操作cs)
        },
        .role = TCFG_HW_SPI1_ROLE,//SPI_ROLE_MASTER,
        .clk  = TCFG_HW_SPI1_BAUD,
        .mode = TCFG_HW_SPI1_MODE,//SPI_MODE_BIDIR_1BIT,//SPI_MODE_UNIDIR_2BIT,
        .bit_mode = SPI_FIRST_BIT_MSB,
        .cpol = 0,//clk level in idle state:0:low,  1:high
#if (TCFG_HW_SPI1_BAUD == 32000000)
        .cpha = 2,//0:s_rise,u_fall  1:s_fall,u_rise  2:s_fall,u_fall  3:s_rise,u_rise
#else
        .cpha = 0,//0:s_rise,u_fall  1:s_fall,u_rise  2:s_fall,u_fall  3:s_rise,u_rise
#endif
        .ie_en = 0, //ie enbale:0:disable,  1:enable
        .irq_priority = 3,
        .spi_isr_callback = NULL,  //spi isr callback
    },
#endif
#if SUPPORT_SPI2
    {
        //spi2
		.port = {
			TCFG_HW_SPI2_PORT_CLK, //clk any io
			TCFG_HW_SPI2_PORT_DO, //D0
			TCFG_HW_SPI2_PORT_DI, //DI/D1
			TCFG_HW_SPI2_PORT_D2, //d2
			TCFG_HW_SPI2_PORT_D3, //d3
			0xff, //cs any io(主机不操作cs)
        },
        .role = TCFG_HW_SPI2_ROLE,//SPI_ROLE_MASTER,
        .clk  = TCFG_HW_SPI2_BAUD,
        .mode = TCFG_HW_SPI2_MODE,//SPI_MODE_BIDIR_1BIT,//SPI_MODE_UNIDIR_2BIT,
        .bit_mode = SPI_FIRST_BIT_MSB,
        .cpol = 0,//clk level in idle state:0:low,  1:high
#if (TCFG_HW_SPI2_BAUD == 32000000)
        .cpha = 2,//0:s_rise,u_fall  1:s_fall,u_rise  2:s_fall,u_fall  3:s_rise,u_rise
#else
        .cpha = 0,//0:s_rise,u_fall  1:s_fall,u_rise  2:s_fall,u_fall  3:s_rise,u_rise
#endif
        .ie_en = 0, //ie enbale:0:disable,  1:enable
        .irq_priority = 3,
        .spi_isr_callback = NULL,  //spi isr callback
    },
#endif
};
#endif
//--------------------------------------------------------------------------------------
//内置flash

#if TCFG_SDFILE_INSERT_FLASH_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_inside_dev_mode_data)
.path			= (const u8 *)"mnt/sdfile/app/MODE",
 .start_addr	= TCFG_SDFILE_FLASH_DEV_PATY_BASE,
  .size     	= TCFG_SDFILE_FLASH_DEV_PATY_SIZE,
   NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_VIRFAT_INSERT_FLASH_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_inside_dev_data)
.path           = (const u8 *)"mnt/sdfile/app/FATFSI",
 .start_addr    = TCFG_VIRFAT_FLASH_DEV_PATY_BASE,
  .size     	= TCFG_VIRFAT_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_DATA_STORAGE_FDB_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_inside_watch_data_dev_data)
.start_addr     =TCFG_DATA_FLASH_DEV_PATY_BASE,
 .size     		=TCFG_DATA_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
//--------------------------------------------------------------------------------------
//这里是外挂norflash分区的配置
#if TCFG_NORFLASH_SFC_DEV_ENABLE
SFC_SPI_PLATFORM_DATA_BEGIN(sfc_spi_data)
.sfc_data_width  = SFC_DATA_WIDTH_4,
 .sfc_read_mode  = SFC_RD_IO,
#if TCFG_NORFLASH_SFC_DEV1_ENABLE
	.sfc_cs1_en = 1,
#endif
SFC_SPI_PLATFORM_DATA_END()

//fat分区(watch)
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_sfc_dev_data)
.sfc_spi_pdata   = &sfc_spi_data,
 .start_addr     = TCFG_VIRFAT_FLASH_DEV_PATY_BASE,
  .size          = TCFG_VIRFAT_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
//exsdfile分区(mode)
#if TCFG_SDFILE_EXTERN_FLASH_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_sfc_dev_data_sdfile)
.sfc_spi_pdata   = &sfc_spi_data,
 .start_addr     = TCFG_SDFILE_FLASH_DEV_PATY_BASE,
  .size          = TCFG_SDFILE_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
	//exsdfile分区(mode)
#if TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_sfc_dev_watch_data)
	.sfc_spi_pdata  = &sfc_spi_data,
	.start_addr     = TCFG_DATA_FLASH_DEV_PATY_BASE,
	.size           = TCFG_DATA_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
#endif
//--------------------------------------------------------------------------------------
//这里是外挂nandflash分区的配置
#if TCFG_NANDFLASH_DEV_ENABLE
/* nandflash驱动配置 */
NANDFLASH_DEV_PLATFORM_DATA_BEGIN(nandflash_dev_data) = {
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
    .spi_read_width = TCFG_FLASH_DEV_FLASH_READ_WIDTH,//flash读数据的线宽
    .start_addr     = 0,
    .size           = TCFG_NANDFLASH_CAPACITY,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spix_p_data[1],
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spix_p_data[2],
#endif
};
#if TCFG_NANDFLASH_UI_FAT_ENABLE
//ui_fat使用
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(ui_fat_dev_data)
	.start_addr     = TCFG_UI_FAT_FLASH_DEV_PATY_BASE,
	.size           = TCFG_UI_FAT_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
#if TCFG_NANDFLASH_FAT_ENABLE
/* nandflash fat16 文件系统配置，多分区时使用 */
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(fat_nand_dev_data)
	.start_addr     = TCFG_NANDFLASH_FAT_BASE,
	.size           = TCFG_NANDFLASH_FAT_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
#if  TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(nandflash_sfc_dev_watch_data)
	.start_addr     = TCFG_DATA_FLASH_DEV_PATY_BASE,
	.size           = TCFG_DATA_FLASH_DEV_PATY_SIZE,
	NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif
extern const struct device_operations nandflash_ftl_ops;
extern const struct device_operations nandflash_sfc_ftl_ops;
extern const struct device_operations nandflash_fs_ftl_ops;
#endif	// TCFG_NANDFLASH_DEV_ENABLE


const u32 g_res_nor_unencry_start_addr = TCFG_SDFILE_FLASH_DEV_PATY_BASE;   //资源分区分界地址，用于设备选择和偏移换算

#if TCFG_HOST_UVC_ENABLE
UVC_PLATFORM_DATA_BEGIN(uvc_data)
    .width = 640,
    .height = 480,
    .fps = 25,
    .mem_size = 100 * 1024,
    .timeout = 500,//ms
UVC_PLATFORM_DATA_END()

static const struct video_subdevice_data video0_subdev_data[] = {
    { VIDEO_TAG_UVC, (void *)&uvc_data },
};
static const struct video_platform_data video0_data = {
    .data = video0_subdev_data,
    .num = ARRAY_SIZE(video0_subdev_data),
};
#endif

#if TCFG_LTE_PHY_ENABLE
extern const struct device_operations lte_module_dev_ops;
LTE_MODULE_DATA_BEGIN(lte_module_data)
	.name = (u8 *)"usb_rndis",
LTE_MODULE_DATA_END()
#endif

REGISTER_DEVICES(device_table) = {
#if TCFG_EXT_WIFI_ENABLE
    { "wifi",  &wifi_dev_ops, (void *) &wifi_data},
#endif
#if TCFG_SD0_ENABLE
    { "sd0", 	&sd_dev_ops,	(void *) &sd0_data},
#endif
#if TCFG_SD1_ENABLE
    { "sd1", 	&sd_dev_ops,	(void *) &sd1_data},
#endif

#if TCFG_CAN_ENABLE
    { "can", &can_dev_ops, (void *)&can_data },
#endif

#if TCFG_CAMERA_DVP_ENABLE
    { "video0",  NULL, (void *)&video0_data},
#endif
#if TCFG_CAMERA_SPI_ENABLE
    { "video0",  NULL, (void *)&video0_data},
#endif
#if TCFG_APP_LINEIN_EN
    { "linein",  &linein_dev_ops, (void *) &linein_data},
#endif

#if TCFG_UDISK_ENABLE
    { "udisk0",   &mass_storage_ops, NULL},
#endif
#if TCFG_HOST_UVC_ENABLE
    {"uvc0", &uvc_dev_ops, (void *)&uvc_data},
    { "video0.*",  &video_dev_ops, (void *)&video0_data },
#endif
#if TCFG_NANDFLASH_DEV_ENABLE
    {"nand_flash",   &nandflash_dev_ops, (void *) &nandflash_dev_data},
    {"nandflash_ftl",   &nandflash_ftl_ops, 	"nand_flash"},
#endif
#if TCFG_NANDFLASH_FAT_ENABLE
	// nandflash fat16区
	{TCFG_NANDFLASH_FAT_ROOT, &nandflash_sfc_ftl_ops,	(void *)&fat_nand_dev_data},
#endif
#if TCFG_NANDFLASH_UI_FAT_ENABLE
	//ui fat
	{TCFG_NANDFLASH_UI_FAT_LOGO, &nandflash_sfc_ftl_ops,	(void *)&ui_fat_dev_data},
#endif
#if TCFG_VIRFAT_FLASH_ENABLE
#if  TCFG_NANDFLASH_DEV_ENABLE &&(!TCFG_VIRFAT_INSERT_FLASH_ENABLE)
    //虚拟文件系统对接jl sdfile fat 文件系统设备入口,往下对接文件系统，往上对接物理设备
    { "virfat_flash", 	&virfat_flash_dev_ops,	(void *)"nandflash_ftl"},
#else
    //虚拟文件系统对接jl sdfile fat 文件系统设备入口,往下对接文件系统，往上对接物理设备
    { "virfat_flash", 	&virfat_flash_dev_ops,	(void *)"res_nor"},
#endif
#endif

//res_nor 是物理设备入口
#if TCFG_VIRFAT_INSERT_FLASH_ENABLE
    //使用内置flash  跑ui表盘
    { "res_nor",   &inside_norflash_fs_dev_ops, (void *) &norflash_norfs_inside_dev_data},
#elif (TCFG_VIRFAT_EXT_FLASH_ENABLE &&  (!TCFG_NANDFLASH_DEV_ENABLE))
    //使用外挂flash  跑ui表盘
    { "res_nor",   &norflash_sfc_fs_dev_ops, (void *) &norflash_sfc_dev_data},
#endif//TCFG_VIRFAT_INSERT_FLASH_ENABLE

#if TCFG_SDFILE_INSERT_FLASH_ENABLE
    //内置只读文件系统
    { "res_nor_mode",   &inside_norflash_fs_dev_ops, (void *) &norflash_norfs_inside_dev_mode_data},
#elif  TCFG_SDFILE_EXTERN_FLASH_ENABLE
	//外挂只读文件系统
    { "res_nor_mode",   &norflash_sfc_fs_dev_ops, (void *) &norflash_sfc_dev_data_sdfile},
#endif

#if TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE
	//用户数据放外挂
#if TCFG_NANDFLASH_DEV_ENABLE
	{ TCFG_DATA_DEV_NAME,   & nandflash_fs_ftl_ops, (void *) &nandflash_sfc_dev_watch_data},
#else
	{ TCFG_DATA_DEV_NAME,   &norflash_sfc_fs_dev_ops, (void *) &norflash_sfc_dev_watch_data},
#endif
#elif TCFG_DATA_STORAGE_FDB_ENABLE
	//用户数据放内置
    { TCFG_DATA_DEV_NAME,   &inside_norflash_fs_dev_ops, (void *) &norflash_norfs_inside_watch_data_dev_data},
#endif
#if TCFG_UI_ENABLE && TCFG_VIDEO0_ENABLE
    {"fb5",&fb_dev_ops,NULL},
    {"fb6",&fb_dev_ops,NULL},
    {"fb7",&fb_dev_ops,NULL},
    {"fb_out",&fb_out_dev_ops,NULL},
#endif

#if TCFG_LTE_PHY_ENABLE
    { "lte",  &lte_module_dev_ops, (void *) &lte_module_data},
#endif
};


#if ((TCFG_SD0_ENABLE && TCFG_SD0_ALWAY_ONLINE_ENABLE) || \
     (TCFG_SD1_ENABLE && TCFG_SD1_ALWAY_ONLINE_ENABLE))

extern int sdx_dev_entry_lowpower(const char *sdx_name);

static int sdx_entry_lowpower(int param)
{
    sdx_dev_entry_lowpower("sd0");
    sdx_dev_entry_lowpower("sd1");
    return 0;
}
static int sd0_entry_lowpower(int param)
{
    sdmmc_dev_enter_lowpower("sd0");
    return 0;
}
static int sd1_entry_lowpower(int param)
{
    sdmmc_dev_enter_lowpower("sd1");
    return 0;
}
static void sdx_sleep_callback(void)
{
    int argv[3];
    u32 sdx_lp_fun = 0;
#if (defined(SD0_POWER_ALONE) && (SD0_POWER_ALONE))
    if (sd0_power_enable && (sd0_power_enable != 0xff)) {
        sdx_lp_fun ++;
        argv[0] = (int)sd0_entry_lowpower;
    }
#endif
#if (defined(SD1_POWER_ALONE) && (SD1_POWER_ALONE))
    if (sd1_power_enable && (sd1_power_enable != 0xff)) {
        sdx_lp_fun ++;
        argv[0] = (int)sd1_entry_lowpower;
    }
#endif
    if (sdx_lp_fun == 0) {
        return;
    } else if (sdx_lp_fun == 2) {
        argv[0] = (int)sdx_entry_lowpower;
    }
    argv[1] = 1;
    /* argv[2] = ; */
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
    if (ret) {
        printf("post ret:%d \n", ret);
    }
}
#endif /*#if TCFG_SD_ALWAY_ONLINE_ENABLE*/

#if ((TCFG_SD0_ENABLE && (!TCFG_SD0_ALWAY_ONLINE_ENABLE)) || \
     (TCFG_SD1_ENABLE && (!TCFG_SD1_ALWAY_ONLINE_ENABLE)))


void sdmmc_dev_dect_scan_timer_modify(char *sdx, u32 modify_time);
void sniff_hook(u32 slot, u8 num, u8 first_conn, int t_sniff)
{
    if (num) {
        if (first_conn) {
            /* printf("..%d..\n", t_sniff); */
#if (TCFG_SD0_ENABLE && (!TCFG_SD0_ALWAY_ONLINE_ENABLE))
            sdmmc_dev_dect_scan_timer_modify("sd0", t_sniff - 12);
#endif
#if (TCFG_SD1_ENABLE && (!TCFG_SD1_ALWAY_ONLINE_ENABLE))
            sdmmc_dev_dect_scan_timer_modify("sd1", t_sniff - 12);
#endif
        }
    } else {
        /* printf("..%d..\n", t_sniff); */
#if (TCFG_SD0_ENABLE && (!TCFG_SD0_ALWAY_ONLINE_ENABLE))
        sdmmc_dev_dect_scan_timer_modify("sd0", t_sniff - 3);
#endif
#if (TCFG_SD1_ENABLE && (!TCFG_SD1_ALWAY_ONLINE_ENABLE))
        sdmmc_dev_dect_scan_timer_modify("sd1", t_sniff - 3);
#endif
    }
}
#endif


void board_init()
{

    board_power_init();

    gpadc_init();

#if TCFG_APP_FM_EN
    y_printf(">> Func:%s, Line:%d, call: fm_dev_init Func!\n", __func__, __LINE__);
    fm_dev_init((void *)(&fm_dev_data));
#endif

#if TCFG_RDEC_KEY_ENABLE
    rdec_key_init();
#endif

#if TCFG_LPCTMU_ENABLE
    lpctmu_init(&lpctmu_cfg);
#endif


#if TCFG_PAY_ALIOS_ENABLE
    extern void alipay_upay_init(void);
    alipay_upay_init();
#endif /* #if TCFG_PAY_ALIOS_ENABLE */

#if (TCFG_SD0_ENABLE && TCFG_SD0_ALWAY_ONLINE_ENABLE)
    //TODO
    // sd0_power_config(4);
#endif
#if (TCFG_SD1_ENABLE && TCFG_SD1_ALWAY_ONLINE_ENABLE)
    sd1_power_config(4);
#endif
#ifndef CONFIG_EXTRAM_CODE_ENABLE
#if TCFG_PSRAM_DEV_ENABLE
	psram_init(&psram_config);
#endif
#endif

#if TCFG_MULTI_CHIP_CONN_ENABLE
	jldtp_manager_init(chip_conn_configs, ARRAY_SIZE(chip_conn_configs));
#endif

#if TCFG_BT_PA_ENABLE
    bt_pa_init();
#endif
}


/*-----------------------------------------------------------------------
 *进入、退出低功耗函数回调状态，函数单核操作、关中断，请勿做耗时操作
 *
 */
#include "iokey.h"
#include "irkey.h"
#include "adkey.h"
#include "gpio_config.h"



void board_sleep_enter_callback()
{
    /* 此函数禁止添加打印 */
    putchar('<');
}


void board_sleep_exit_callback()
{
    putchar('>');

#if ((TCFG_SD0_ENABLE && TCFG_SD0_ALWAY_ONLINE_ENABLE && SD0_POWER_ALONE) || \
     (TCFG_SD1_ENABLE && TCFG_SD1_ALWAY_ONLINE_ENABLE && SD1_POWER_ALONE))

    sdx_sleep_callback();
#endif
}

////关机回调执行顺序
//power_soff_callback()->do_platform_uninitcall()->board_poweroff_uninit->gpio_config_soft_poweroff()

//关机的注册的段
void board_poweroff_uninit()
{

    //用户进行自己模块的关闭操作

#if TCFG_SD0_ENABLE
     sdmmc_dev_enter_lowpower("sd0");
#endif
#if TCFG_SD1_ENABLE
    sdmmc_dev_enter_lowpower("sd1");
#endif

#if (TCFG_SD0_ENABLE && TCFG_SD0_ALWAY_ONLINE_ENABLE)
// TODO
//  sd0_power_config(0);
#endif
#if (TCFG_SD1_ENABLE && TCFG_SD1_ALWAY_ONLINE_ENABLE)
    sd1_power_config(0);
#endif

#if TCFG_NORFLASH_DEV_ENABLE
	extern void ex_norflash_sfc_poweroff(void);
	ex_norflash_sfc_poweroff();
#endif
#if TCFG_NANDFLASH_DEV_ENABLE
	extern void nandflash_poweroff(int priv);
	struct device*dev = (struct device*)dev_open("nandflash_ftl",NULL);
	while((int)atomic_read(&dev->ref)){
		dev_close((void*)dev);
	}
	nandflash_poweroff(0);
#endif
#ifndef CONFIG_EXTRAM_CODE_ENABLE
#if TCFG_PSRAM_DEV_ENABLE
	extern void  psram_power_off();
	psram_power_off();
#endif
#endif
}
/* platform_uninitcall(board_poweroff_uninit); */



//设置关机保持状态的io
void board_gpio_config_soft_poweroff(u32 *gpio_config)
{
//设置后关机io会保持原来状态
//    PORT_PROTECT(IO_PORTB_01);
//    PORT_PROTECT(IO_PORTB_01);
}







#endif
