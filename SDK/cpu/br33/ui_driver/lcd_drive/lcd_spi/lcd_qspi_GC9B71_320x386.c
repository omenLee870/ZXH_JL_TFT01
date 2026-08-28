
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
#if (TCFG_SPI_LCD_ENABLE && TCFG_LCD_QSPI_GC9B71_320x386_ENABLE)


// QSPI
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE0_1T8B */
#define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE1_1T2B
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE2_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_SUBMODE0_1T8B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_SUBMODE1_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB666_SUBMODE2_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_SUBMODE0_1T8B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_SUBMODE1_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB888_SUBMODE2_1T2B */


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

#define LCD_FORMAT DBI_INPUT_FORMAT_RGB565


struct dbi_param lcd_spi_gc9b71_param;


/*
** 初始化代码
*/
static const u8 lcd_spi_gc9b71_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0xfe, _END_,
    _BEGIN_, 0xef, _END_,
    _BEGIN_, 0x80, 0x11, _END_,
    _BEGIN_, 0x81, 0x70, _END_,
    _BEGIN_, 0x82, 0x09, _END_,
    _BEGIN_, 0x83, 0x03, _END_,
    _BEGIN_, 0x84, 0x62, _END_,
    _BEGIN_, 0x89, 0x18, _END_,
    _BEGIN_, 0x8a, 0x40, _END_,
    _BEGIN_, 0x8b, 0x0a, _END_,

#if (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB565)
    _BEGIN_, 0x3a, 0x55, _END_,
#elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB666)
    _BEGIN_, 0x3a, 0x66, _END_,
#elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB888)
    _BEGIN_, 0x3a, 0x77, _END_,
#endif
    _BEGIN_, 0x36, 0x40, _END_,
    _BEGIN_, 0xec, 0x07, _END_,
    _BEGIN_, 0x74, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, _END_,

    _BEGIN_, 0x98, 0x3e, _END_,
    _BEGIN_, 0x99, 0x3e, _END_,
    _BEGIN_, 0xa1, 0x01, 0x04, _END_,
    _BEGIN_, 0xa2, 0x01, 0x04, _END_,

    _BEGIN_, 0xcb, 0x02, _END_,
    _BEGIN_, 0x7c, 0xb6, 0x24, _END_,
    _BEGIN_, 0xac, 0x74, _END_,
    _BEGIN_, 0xf6, 0x80, _END_,
    _BEGIN_, 0xb5, 0x09, 0x09, _END_,
    _BEGIN_, 0xeb, 0x01, 0x81, _END_,

    _BEGIN_, 0x60, 0x38, 0x06, 0x13, 0x56, _END_,
    _BEGIN_, 0x63, 0x38, 0x08, 0x13, 0x56, _END_,
    _BEGIN_, 0x61, 0x3b, 0x1b, 0x58, 0x38, _END_,
    _BEGIN_, 0x62, 0x3b, 0x1b, 0x58, 0x38, _END_,
    _BEGIN_, 0x64, 0x38, 0x0a, 0x73, 0x16, 0x13, 0x56, _END_,
    _BEGIN_, 0x66, 0x38, 0x0b, 0x73, 0x17, 0x13, 0x56, _END_,
    _BEGIN_, 0x68, 0x00, 0x0b, 0x22, 0x0b, 0x22, 0x1c, 0x1c, _END_,
    _BEGIN_, 0x69, 0x00, 0x0b, 0x26, 0x0b, 0x26, 0x1c, 0x1c, _END_,

    _BEGIN_, 0x6a, 0x15, 0x00, _END_,

    _BEGIN_, 0x6e, 0x08, 0x02, 0x1a, 0x00, 0x12, 0x12, 0x11, 0x11, 0x14, 0x14, 0x13, 0x13, 0x04, 0x19, 0x1e, 0x1d, 0x1d, 0x1e, 0x19, 0x04, 0x0b, 0x0b, 0x0c, 0x0c, 0x09, 0x09, 0x0a, 0x0a, 0x00, 0x1a, 0x01, 0x07, _END_,

    _BEGIN_, 0x6c, 0xcc, 0x0c, 0xcc, 0x84, 0xcc, 0x04, 0x50, _END_,
    _BEGIN_, 0x7d, 0x72, _END_,
    _BEGIN_, 0x70, 0x02, 0x03, 0x09, 0x07, 0x09, 0x03, 0x09, 0x07, 0x09, 0x03, _END_,
    _BEGIN_, 0x90, 0x06, 0x06, 0x05, 0x06, _END_,
    _BEGIN_, 0x93, 0x45, 0xff, 0x00, _END_,

    _BEGIN_, 0xc3, 0x15, _END_,
    _BEGIN_, 0xc4, 0x36, _END_,
    _BEGIN_, 0xc9, 0x3d, _END_,

    _BEGIN_, 0xf0, 0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29, _END_,
    _BEGIN_, 0xf2, 0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29, _END_,
    _BEGIN_, 0xf1, 0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f, _END_,
    _BEGIN_, 0xf3, 0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f, _END_,

    _BEGIN_, 0xf9, 0x30, _END_,
    _BEGIN_, 0xbe, 0x11, _END_,
    _BEGIN_, 0xfb, 0x00, 0x00, _END_,

    _BEGIN_, 0x84, 0x32, _END_,// B4 EN
    _BEGIN_, 0xB4, 0x0A, _END_,// TE Width
    _BEGIN_, 0x35, 0x00, _END_,		// 开启TE
    _BEGIN_, 0x44, 0x00, 0xa0, _END_,		// TE configure


    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    /* _BEGIN_, 0x2c, 0x00, 0x00, 0x00, 0x00, _END_, */
    /* _BEGIN_, 0x2c, 0x00, 0x00, 0x00, 0x00, _END_, */
    /* _BEGIN_, REGFLAG_DELAY, 120, _END_, */
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
static int lcd_spi_gc9b71_power_ctrl(u8 onoff)
{
    lcd_en_ctrl(onoff);
    return 0;
}


#define DEEP_STANDBY  0

/*
** 设置lcd进入睡眠
*/
static void lcd_spi_gc9b71_entersleep(void)
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
static void lcd_spi_gc9b71_exitsleep(void)
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

    lcd_init(&lcd_spi_gc9b71_param);
#else
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
#endif
}

static u32 lcd_spi_gc9b71_read_id()
{
    printf("##########################################################%s, %d\n", __FUNCTION__, __LINE__);
    u8 id1, id2, id3;

    lcd_read_cmd(0xda, &id1, 1);	// Read ID1
    lcd_read_cmd(0xdb, &id2, 1);	// Read ID2
    lcd_read_cmd(0xdc, &id3, 1);	// Read ID3

    return ((id1 << 16) | (id2 << 8) | id3);
}

struct dbi_param lcd_spi_gc9b71_param = {
    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,

    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = DBI_INPUT_FORMAT_RGB565,



    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 48,

    .te_offset_line = 128,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
        .read_pin_select = READ_FROM_DBI_D0,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

REGISTER_LCD_DEVICE(gc9b71) = {
    .logo = "gc9b71",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_spi_gc9b71_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_spi_gc9b71_cmd_list_poweron) / sizeof(lcd_spi_gc9b71_cmd_list_poweron[0]),
    .param   = (void *) &lcd_spi_gc9b71_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,// 这款是LCD屏幕，需设置TCFG_BACKLIGHT_PWM_MODE=2调整背光
    .power_ctrl = lcd_spi_gc9b71_power_ctrl,
    .entersleep = lcd_spi_gc9b71_entersleep,
    .exitsleep = lcd_spi_gc9b71_exitsleep,
    .read_id = lcd_spi_gc9b71_read_id,
    .lcd_id = 0x000000,
};


#endif


