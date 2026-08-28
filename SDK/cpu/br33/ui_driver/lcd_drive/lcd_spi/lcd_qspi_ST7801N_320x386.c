#define __SPI_LCD_DRIVER_H
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"

// TP 使用CST816D

/*
** 驱动代码的宏开关
*/
//<<<[qspi屏 320x386]>>>//
#if (TCFG_SPI_LCD_ENABLE && TCFG_LCD_QSPI_DTR_ST7801N_390x450_ENABLE)

#define QSPI_SDR_TYPE   0
#define QSPI_DDR_TYPE1  1
#define QSPI_DDR_TYPE2  2
#define CURR_DDR_TYPE   QSPI_DDR_TYPE2

// QSPI SDR/DDR
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_A1D1_1T8B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_A1D1_1T8B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_A1D1_1T8B */

/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_A1D2_1T4B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_A1D2_1T4B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_A1D2_1T4B */

/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_A2D2_1T4B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_A2D2_1T4B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_A2D2_1T4B */

#define LCD_DRIVE_CONFIG                    QSPI_RGB565_A1D4_1T2B
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_A1D4_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_A1D4_1T2B */

/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_A4D4_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_A4D4_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_A4D4_1T2B */



/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"


#define SCR_X       0
#define SCR_Y       0
#define SCR_W       LCD_WIDTH
#define SCR_H       LCD_HEIGHT
#define LCD_W       LCD_WIDTH
#define LCD_H       LCD_HEIGHT
#define LCD_BLOCK_W LCD_WIDTH
#define LCD_BLOCK_H 20
#define BUF_NUM     2

#define LCD_FORMAT OUTPUT_FORMAT_RGB565


struct dbi_param lcd_spi_st7801n_param;


/*
** 初始化代码
*/
static const u8 lcd_spi_st7801n_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,

#if CURR_DDR_TYPE == QSPI_DDR_TYPE1
    _BEGIN_, 0x40, 0x02, _END_,
    _BEGIN_, 0x62, 0x07, 0x46, _END_, // MSPI_DDR_EN=1
    _BEGIN_, 0x40, 0x00, _END_,
#endif

    _BEGIN_, 0x53, 0x20, _END_,
    _BEGIN_, 0x51, 0xff, _END_,
    _BEGIN_, 0x35, 0x00, _END_,

#if (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB565)
    _BEGIN_, 0x3a, 0x55, _END_,
#elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB666)
    _BEGIN_, 0x3a, 0x66, _END_,
#elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB888)
    _BEGIN_, 0x3a, 0x77, _END_,
#endif

    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
};


extern struct lcd_spi_platform_data *lcd_get_platform_data();

static void delay_2ms(int cnt)
{
    if (cnt * 2 > 10) {
        os_time_dly(cnt * 2 / 10 + 1);
    } else {
        mdelay(2 * cnt);
    }
}

/*
** lcd电源控制
*/
static int lcd_spi_st7801n_power_ctrl(u8 onoff)
{
    lcd_en_ctrl(onoff);
    return 0;
}


#define DEEP_STANDBY  0

/*
** 设置lcd进入睡眠
*/
static void lcd_spi_st7801n_entersleep(void)
{
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2);	// delay 120ms

#if DEEP_STANDBY
    u8 dstb = 0x01;
    lcd_write_cmd(0x4f, &dstb, sizeof(dstb));
    delay_2ms(102 / 2);	// delay 120ms
    struct lcd_platform_data *lcd_dat = lcd_get_platform_data();
    if (lcd_dat && lcd_dat->pin_te != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_te), PORT_HIGHZ);
    }
#endif

}



/*
** 设置lcd退出睡眠
*/
static void lcd_spi_st7801n_exitsleep(void)
{
#if DEEP_STANDBY
    struct lcd_platform_data *lcd_dat = (struct lcd_platform_data *)lcd_get_platform_data();
    if (lcd_dat && (lcd_dat->pin_reset != NO_CONFIG_PORT)) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_LOW);
        delay_2ms(4);	// delay >5ms
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_HIGH);
        delay_2ms(4);	// delay >5ms
    }

    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);
    lcd_write_cmd(0x29, NULL, 0);
    delay_2ms(5);

    lcd_init(&lcd_spi_st7801n_param);
#else
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
#endif
}

static u32 lcd_spi_st7801n_read_id()
{
    printf("##########################################################%s, %d\n", __FUNCTION__, __LINE__);
    u8 id1, id2, id3;

    lcd_read_cmd(0xda, &id1, 1);	// Read ID1
    lcd_read_cmd(0xdb, &id2, 1);	// Read ID2
    lcd_read_cmd(0xdc, &id3, 1);	// Read ID3

    return ((id1 << 16) | (id2 << 8) | id3);
}

struct dbi_param lcd_spi_st7801n_param = {
    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,

    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = LCD_FORMAT,


    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

#if CURR_DDR_TYPE == QSPI_SDR_TYPE
    .fps = 50,
#else
    .fps = 60,
#endif
    .fps_constraint = FPS_CONSTRAINT_LESS_THAN, // 帧率约束

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
        .read_pin_select = READ_FROM_DBI_D0,
        .qspi = {
#if CURR_DDR_TYPE == QSPI_SDR_TYPE
            .ddr_type = SDR_TYPE,
#elif CURR_DDR_TYPE == QSPI_DDR_TYPE1
            .ddr_type = DDR_TYPE1,
#else //if CURR_DDR_TYPE == QSPI_DDR_TYPE2
            .ddr_type = DDR_TYPE2,
#endif
        }
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

REGISTER_LCD_DEVICE(st7801n) = {
    .logo = "st7801n",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_spi_st7801n_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_spi_st7801n_cmd_list_poweron) / sizeof(lcd_spi_st7801n_cmd_list_poweron[0]),
    .param   = (void *) &lcd_spi_st7801n_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,// 这款是LCD屏幕，需设置TCFG_BACKLIGHT_PWM_MODE=2调整背光
    .power_ctrl = lcd_spi_st7801n_power_ctrl,
    .entersleep = lcd_spi_st7801n_entersleep,
    .exitsleep = lcd_spi_st7801n_exitsleep,
    .read_id = lcd_spi_st7801n_read_id,
    .lcd_id = 0x000000,
};


#endif



