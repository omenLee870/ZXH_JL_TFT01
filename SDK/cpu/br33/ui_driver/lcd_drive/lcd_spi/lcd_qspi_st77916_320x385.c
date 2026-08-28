
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
#if (TCFG_SPI_LCD_ENABLE && TCFG_LCD_QSPI_ST77916_320x385_ENABLE)

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
#define LCD_BLOCK_H 16
#define BUF_NUM     2

#define LCD_FORMAT OUTPUT_FORMAT_RGB565


static const struct dbi_param lcd_spi_st77916_param;


/*
** 初始化代码
*/
static const u8 lcd_spi_st77916_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0xf0, 0x28, _END_,
    _BEGIN_, 0xf2, 0x28, _END_,
    _BEGIN_, 0x7c, 0xd1, _END_,
    _BEGIN_, 0x80, 0x10, _END_,
    _BEGIN_, 0x83, 0xe0, _END_,
    _BEGIN_, 0x84, 0x61, _END_,
    _BEGIN_, 0xf2, 0x82, _END_,
    _BEGIN_, 0xf0, 0x00, _END_,
    _BEGIN_, 0xf0, 0x01, _END_,
    _BEGIN_, 0xf1, 0x01, _END_,

    /* #if (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB565) */
    /* _BEGIN_, 0x3a, 0x55, _END_, */
    /* #elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB666) */
    /* _BEGIN_, 0x3a, 0x66, _END_, */
    /* #elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB888) */
    /* _BEGIN_, 0x3a, 0x77, _END_, */
    /* #endif */
    _BEGIN_, 0xb0, 0x56, _END_,
    _BEGIN_, 0xb1, 0x4d, _END_,
    _BEGIN_, 0xb2, 0x24, _END_,

    _BEGIN_, 0xb4, 0x66, _END_,
    _BEGIN_, 0xb5, 0x44, _END_,
    _BEGIN_, 0xb6, 0x8b, _END_,
    _BEGIN_, 0xb7, 0x40, _END_,

    _BEGIN_, 0xb8, 0x05, _END_,
    _BEGIN_, 0xba, 0x00, _END_,
    _BEGIN_, 0xbb, 0x08, _END_,
    _BEGIN_, 0xbc, 0x08, _END_,
    _BEGIN_, 0xbd, 0x00, _END_,
    _BEGIN_, 0xc0, 0x80, _END_,

    _BEGIN_, 0xc1, 0x08, _END_,
    _BEGIN_, 0xc2, 0x35, _END_,
    _BEGIN_, 0xc3, 0x80, _END_,
    _BEGIN_, 0xc4, 0x08, _END_,
    _BEGIN_, 0xc5, 0x35, _END_,
    _BEGIN_, 0xc6, 0xa9, _END_,
    _BEGIN_, 0xc7, 0x41, _END_,
    _BEGIN_, 0xc8, 0x01, _END_,

    _BEGIN_, 0xc9, 0xa9, _END_,

    _BEGIN_, 0xca, 0x41, _END_,

    _BEGIN_, 0xcb, 0x01, _END_,
    _BEGIN_, 0xd0, 0xd1, _END_,
    _BEGIN_, 0xd1, 0x40, _END_,
    _BEGIN_, 0xd2, 0x81, _END_,
    _BEGIN_, 0xf5, 0x00, 0xa5, _END_,

    _BEGIN_, 0xdd, 0x49, _END_,
    _BEGIN_, 0xde, 0x49, _END_,
    _BEGIN_, 0xf1, 0x10, _END_,

    _BEGIN_, 0xf0, 0x00, _END_,
    _BEGIN_, 0xf0, 0x02, _END_,
    _BEGIN_, 0xe0, 0xf0, 0x0a, 0x11, 0x0c, 0x0b, 0x08, 0x3a, 0x54, 0x51, 0x29, 0x16, 0x15, 0x31, 0x34, _END_,
    _BEGIN_, 0xe1, 0xf0, 0x0a, 0x11, 0x0b, 0x0a, 0x07, 0x39, 0x43, 0x4e, 0x09, 0x15, 0x15, 0x2e, 0x34, _END_,

    _BEGIN_, 0xf0, 0x10, _END_,
    _BEGIN_, 0xf3, 0x10, _END_,
    _BEGIN_, 0xe0, 0x08, _END_,

    _BEGIN_, 0xe1, 0x00, _END_,
    _BEGIN_, 0xe2, 0x0b, _END_,
    _BEGIN_, 0xe3, 0x00, _END_,
    _BEGIN_, 0xe4, 0xe0, _END_,
    _BEGIN_, 0xe5, 0x06, _END_,
    _BEGIN_, 0xE6, 0x21, _END_,
    _BEGIN_, 0xE7, 0x10, _END_,
    _BEGIN_, 0xE8, 0x8A, _END_,
    _BEGIN_, 0xE9, 0x82, _END_,
    _BEGIN_, 0xEA, 0xE4, _END_,
    _BEGIN_, 0xEB, 0x00, _END_,
    _BEGIN_, 0xEC, 0x00, _END_,
    _BEGIN_, 0xED, 0x14, _END_,
    _BEGIN_, 0xEE, 0xFF, _END_,
    _BEGIN_, 0xEF, 0x00, _END_,
    _BEGIN_, 0xF8, 0xFF, _END_,
    _BEGIN_, 0xF9, 0x00, _END_,
    _BEGIN_, 0xFA, 0x00, _END_,
    _BEGIN_, 0xFB, 0x30, _END_,
    _BEGIN_, 0xFC, 0x00, _END_,
    _BEGIN_, 0xFD, 0x00, _END_,
    _BEGIN_, 0xFE, 0x00, _END_,
    _BEGIN_, 0xFF, 0x00, _END_,
    _BEGIN_, 0x60, 0x50, _END_,
    _BEGIN_, 0x61, 0x02, _END_,
    _BEGIN_, 0x62, 0x0B, _END_,
    _BEGIN_, 0x63, 0x50, _END_,
    _BEGIN_, 0x64, 0x04, _END_,
    _BEGIN_, 0x65, 0x0B, _END_,
    _BEGIN_, 0x66, 0x53, _END_,
    _BEGIN_, 0x67, 0x08, _END_,
    _BEGIN_, 0x68, 0x0B, _END_,
    _BEGIN_, 0x69, 0x53, _END_,
    _BEGIN_, 0x6A, 0x0A, _END_,
    _BEGIN_, 0x6B, 0x0B, _END_,
    _BEGIN_, 0x70, 0x50, _END_,
    _BEGIN_, 0x71, 0x01, _END_,
    _BEGIN_, 0x72, 0x0B, _END_,
    _BEGIN_, 0x73, 0x50, _END_,
    _BEGIN_, 0x74, 0x03, _END_,
    _BEGIN_, 0x75, 0x0B, _END_,
    _BEGIN_, 0x76, 0x53, _END_,
    _BEGIN_, 0x77, 0x07, _END_,
    _BEGIN_, 0x78, 0x0B, _END_,
    _BEGIN_, 0x79, 0x53, _END_,

    _BEGIN_, 0x7A, 0x09, _END_,
    _BEGIN_, 0x7B, 0x0B, _END_,
    _BEGIN_, 0x80, 0x58, _END_,
    _BEGIN_, 0x81, 0x00, _END_,
    _BEGIN_, 0x82, 0x04, _END_,
    _BEGIN_, 0x83, 0x03, _END_,
    _BEGIN_, 0x84, 0x0C, _END_,
    _BEGIN_, 0x85, 0x00, _END_,
    _BEGIN_, 0x86, 0x00, _END_,
    _BEGIN_, 0x87, 0x00, _END_,
    _BEGIN_, 0x88, 0x58, _END_,
    _BEGIN_, 0x89, 0x00, _END_,
    _BEGIN_, 0x8A, 0x06, _END_,
    _BEGIN_, 0x8B, 0x03, _END_,
    _BEGIN_, 0x8C, 0x0E, _END_,
    _BEGIN_, 0x8D, 0x00, _END_,
    _BEGIN_, 0x8E, 0x00, _END_,
    _BEGIN_, 0x8F, 0x00, _END_,
    _BEGIN_, 0x90, 0x58, _END_,
    _BEGIN_, 0x91, 0x00, _END_,
    _BEGIN_, 0x92, 0x08, _END_,
    _BEGIN_, 0x93, 0x03, _END_,
    _BEGIN_, 0x94, 0x10, _END_,
    _BEGIN_, 0x95, 0x00, _END_,
    _BEGIN_, 0x96, 0x00, _END_,
    _BEGIN_, 0x97, 0x00, _END_,
    _BEGIN_, 0x98, 0x58, _END_,
    _BEGIN_, 0x99, 0x00, _END_,
    _BEGIN_, 0x9A, 0x0A, _END_,
    _BEGIN_, 0x9B, 0x03, _END_,
    _BEGIN_, 0x9C, 0x12, _END_,
    _BEGIN_, 0x9D, 0x00, _END_,
    _BEGIN_, 0x9E, 0x00, _END_,
    _BEGIN_, 0x9F, 0x00, _END_,
    _BEGIN_, 0xA0, 0x58, _END_,
    _BEGIN_, 0xA1, 0x00, _END_,
    _BEGIN_, 0xA2, 0x03, _END_,
    _BEGIN_, 0xA3, 0x03, _END_,
    _BEGIN_, 0xA4, 0x0B, _END_,
    _BEGIN_, 0xA5, 0x00, _END_,
    _BEGIN_, 0xA6, 0x00, _END_,
    _BEGIN_, 0xA7, 0x00, _END_,
    _BEGIN_, 0xA8, 0x58, _END_,
    _BEGIN_, 0xA9, 0x00, _END_,
    _BEGIN_, 0xAA, 0x05, _END_,
    _BEGIN_, 0xAB, 0x03, _END_,
    _BEGIN_, 0xAC, 0x0D, _END_,
    _BEGIN_, 0xAD, 0x00, _END_,
    _BEGIN_, 0xAE, 0x00, _END_,
    _BEGIN_, 0xAF, 0x00, _END_,
    _BEGIN_, 0xB0, 0x58, _END_,
    _BEGIN_, 0xB1, 0x00, _END_,
    _BEGIN_, 0xB2, 0x07, _END_,
    _BEGIN_, 0xB3, 0x03, _END_,
    _BEGIN_, 0xB4, 0x0F, _END_,
    _BEGIN_, 0xB5, 0x00, _END_,
    _BEGIN_, 0xB6, 0x00, _END_,
    _BEGIN_, 0xB7, 0x00, _END_,
    _BEGIN_, 0xB8, 0x58, _END_,
    _BEGIN_, 0xB9, 0x00, _END_,
    _BEGIN_, 0xBA, 0x09, _END_,
    _BEGIN_, 0xBB, 0x03, _END_,
    _BEGIN_, 0xBC, 0x11, _END_,
    _BEGIN_, 0xBD, 0x00, _END_,
    _BEGIN_, 0xBE, 0x00, _END_,
    _BEGIN_, 0xBF, 0x00, _END_,
    _BEGIN_, 0xC0, 0x03, _END_,
    _BEGIN_, 0xC1, 0x12, _END_,
    _BEGIN_, 0xC2, 0xAA, _END_,
    _BEGIN_, 0xC3, 0x30, _END_,
    _BEGIN_, 0xC4, 0x21, _END_,
    _BEGIN_, 0xC5, 0xBB, _END_,
    _BEGIN_, 0xC6, 0x64, _END_,
    _BEGIN_, 0xC7, 0x55, _END_,
    _BEGIN_, 0xC8, 0x46, _END_,
    _BEGIN_, 0xC9, 0x77, _END_,
    _BEGIN_, 0xD0, 0x03, _END_,
    _BEGIN_, 0xD1, 0x12, _END_,
    _BEGIN_, 0xD2, 0xAA, _END_,
    _BEGIN_, 0xD3, 0x30, _END_,
    _BEGIN_, 0xD4, 0x21, _END_,
    _BEGIN_, 0xD5, 0xBB, _END_,
    _BEGIN_, 0xD6, 0x64, _END_,
    _BEGIN_, 0xD7, 0x55, _END_,
    _BEGIN_, 0xD8, 0x46, _END_,
    _BEGIN_, 0xD9, 0x77, _END_,
    _BEGIN_, 0xF3, 0x01, _END_,
    _BEGIN_, 0xF0, 0x00, _END_,
    _BEGIN_, 0x21, 0x00, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x3A, 0x55, _END_,
#if CONFIG_LVGL_UI_ENABLE
    _BEGIN_, 0x44, 0x00, 0x01, _END_,		// TE configure
#endif
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    /* _BEGIN_, 0x2c, 0x00, 0x00, 0x00, 0x00, _END_, */
    /* _BEGIN_, 0x2c, 0x00, 0x00, 0x00, 0x00, _END_, */
    /* _BEGIN_, REGFLAG_DELAY, 120, _END_, */
};


extern void lcd_bl_ctrl(u8 val);
extern struct lcd_spi_platform_data *lcd_get_platform_data();

static void delay_2ms(int cnt)
{
    if (cnt * 2 > 10) {
        os_time_dly(cnt * 2 / 10 + 1);
    } else {
        mdelay(2 * cnt);
    }
}

static int lcd_spi_st77916_backlight_ctrl(u8 percent)
{
#if ((defined TCFG_PT_JL_TEST_ENABLE)&&TCFG_PT_JL_TEST_ENABLE)
    lcd_bl_ctrl(percent);
#else
    lcd_bl_ctrl(!percent);
#endif
    return 0;
}

/*
** lcd电源控制
*/
static int lcd_spi_st77916_power_ctrl(u8 onoff)
{
    lcd_en_ctrl(onoff);
    return 0;
}


#define DEEP_STANDBY  0

//#define     APP_IO_DEBUG_0(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
//#define     APP_IO_DEBUG_1(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}


/*
** 设置lcd进入睡眠
*/
static void lcd_spi_st77916_entersleep(void)
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
static void lcd_spi_st77916_exitsleep(void)
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

    lcd_init(&lcd_spi_st77916_param);
#else
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
#endif
}

static u32 lcd_spi_st77916_read_id()
{
    /* printf("##########################################################%s, %d\n", __FUNCTION__, __LINE__); */
    u8 id1, id2, id3;

    lcd_read_cmd(0xda, &id1, 1);	// Read ID1
    lcd_read_cmd(0xdb, &id2, 1);	// Read ID2
    lcd_read_cmd(0xdc, &id3, 1);	// Read ID3

    return ((id1 << 16) | (id2 << 8) | id3);
}

static const struct dbi_param lcd_spi_st77916_param = {
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

    .fps = 55,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
        .read_pin_select = READ_FROM_DBI_D1,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

REGISTER_LCD_DEVICE(st77916) = {
    .logo = "st77916",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_spi_st77916_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_spi_st77916_cmd_list_poweron) / sizeof(lcd_spi_st77916_cmd_list_poweron[0]),
    .param   = (void *) &lcd_spi_st77916_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    //.backlight_ctrl = lcd_spi_st77916_backlight_ctrl,// 这款是LCD屏幕，需设置TCFG_BACKLIGHT_PWM_MODE=2调整背光
    .backlight_ctrl = NULL, //背光改由公共路径 lcd_drv_backlight_ctrl_base() + TCFG_BACKLIGHT_PWM_MODE=2(mcpwm), 旧实现 lcd_bl_ctrl(!percent) 会在 percent>0 时把 PB2 拉低, 导致 VCIEN 不点亮。
    .power_ctrl = lcd_spi_st77916_power_ctrl,
    .entersleep = lcd_spi_st77916_entersleep,
    .exitsleep = lcd_spi_st77916_exitsleep,
    .read_id = lcd_spi_st77916_read_id,
    .lcd_id = 0x000000,
};


#endif


