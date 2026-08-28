
#define __SPI_LCD_DRIVER_H
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"


/*
** 驱动代码的宏开关
*/
//<<<[4-fire屏 240x240]>>>//
#if defined(TCFG_LCD_NB3030_172X320) && TCFG_LCD_NB3030_172X320

#define LCD_DRIVE_CONFIG				QSPI_RGB565_SUBMODE1_1T2B//SPI_4WIRE_RGB565_1T16B//	SPI_4WIRE_RGB565_1T8B


/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"


#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define SCR_X       34
#define SCR_Y       0
#else
#define SCR_X       0
#define SCR_Y       34
#endif

#define SCR_W       LCD_WIDTH
#define SCR_H       LCD_HEIGHT
#define LCD_W       LCD_WIDTH
#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define LCD_H       (LCD_WIDTH)//临时规避
#else
#define LCD_H       LCD_HEIGHT
#endif
#define LCD_BLOCK_W LCD_WIDTH
#define LCD_BLOCK_H 20
#define BUF_NUM     2

#define LCD_FORMAT OUTPUT_FORMAT_RGB565


struct dbi_param lcd_spi_nb3030_param;

#define MY 0     //Y反向（画面）

#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define MV 0    //90旋转（画面）
#define MX 0//1     //X反向（画面）
#else
#define MV 1     //90旋转（画面）
#define MX 1//0     //X反向（画面）
#endif

#define ML 0
#define BGR 0//1       //大小端颜色
#define MH 0
/*
** 初始化代码
*/
static const u8 lcd_cmd_t_nb3030[] ALIGNED(4) = {
    _BEGIN_, 0xfd, 0x06, 0x08, _END_,
    _BEGIN_, 0x61, 0x07, 0x07, _END_,
    _BEGIN_, 0x73, 0x70, _END_,
    _BEGIN_, 0x73, 0x00, _END_,
    _BEGIN_, 0x62, 0x00, 0x44, 0x40, _END_,
    _BEGIN_, 0x63, 0x41, 0x07, 0x12, 0x12, _END_,
    _BEGIN_, 0x64, 0x37, _END_,
    _BEGIN_, 0x65, 0x09, 0x17, 0x21, _END_,
    _BEGIN_, 0x66, 0x09, 0x17, 0x21, _END_,
    _BEGIN_, 0x67, 0x20, 0x40, _END_,
    _BEGIN_, 0x68, 0x90, 0x4c, 0x1d, 0x26, _END_,
    _BEGIN_, 0xb1, 0x0f, 0x02, 0x01, _END_,
    _BEGIN_, 0xb4, 0x01, _END_,
    _BEGIN_, 0xb5, 0x02, 0x02, 0x0a, 0x014, _END_,
// #if (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB565)
//     _BEGIN_, 0x3A, 0x55, _END_,
// #elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB666)
//     _BEGIN_, 0x3A, 0x66, _END_,
// #elif (OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB888)
//     _BEGIN_, 0x3A, 0x77, _END_,
// #endif

    _BEGIN_, 0xb6, 0x04, 0x01, 0x9f, 0x00, 0x02, _END_,
    _BEGIN_, 0xdf, 0x11, _END_,
    _BEGIN_, 0xe2, 0x00, 0x02, 0x01, 0x29, 0x2e, 0x3f, _END_,
    _BEGIN_, 0xe5, 0x3f, 0x2e, 0x29, 0x01, 0x01, 0x00, _END_,
    _BEGIN_, 0xe1, 0x0b, 0x52, _END_,
    _BEGIN_, 0xe4, 0x57, 0x0b, _END_,

    _BEGIN_, 0xe0, 0x06, 0x06, 0x10, 0x0e, 0x0f, 0x0e, 0x10, 0x16, _END_,
    _BEGIN_, 0xe3, 0x15, 0x14, 0x0f, 0x0f, 0x0e, 0x10, 0x06, 0x05, _END_,
    _BEGIN_, 0xE6, 0x00, 0xff, _END_,
    _BEGIN_, 0xe7, 0x01, 0x04, 0x03, 0x03, 0x00, 0x12, _END_,

    _BEGIN_, 0xe8, 0x00, 0x70, 0x00, _END_,
    _BEGIN_, 0xec, 0x52, _END_,
    _BEGIN_, 0xf1, 0x01, 0x01, 0x02, _END_,
    _BEGIN_, 0xf6, 0x01, 0x30, 0x00, 0x00, _END_,
    _BEGIN_, 0xfd, 0xfa, 0xfc, _END_,
    _BEGIN_, 0x3a, 0x55, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x36, (MY << 7) | (MX << 6) | (MV << 5) | (ML << 4) | (BGR << 3) | (MH << 2), _END_,
    _BEGIN_, 0x37, 0, 34, _END_,
    _BEGIN_, 0x11, _END_,
    _BEGIN_, 0x21, _END_,
    _BEGIN_, REGFLAG_DELAY, 200, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 20, _END_,
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
static int lcd_spi_power_ctrl(u8 onoff)
{
    lcd_en_ctrl(onoff);
    return 0;
}


/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_backlight_ctrl(u8 onoff)
{
    lcd_bl_ctrl(onoff);
    return 0;
}


/*
** 设置lcd进入睡眠
*/
static void lcd_entersleep(void)
{
    r_printf("LCD进入睡眠..................");



    lcd_write_cmd(0x28, NULL, 0);
    delay_2ms(50);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(60);
    // u8 dstb = 0x01;
    // u8 data = 0x10;

    // lcd_write_cmd(0x4f, &dstb, sizeof(dstb));
    // delay_2ms(102 / 2); // delay 120ms
    // // lcd_write_cmd(0x10, NULL, 0);
    // struct lcd_platform_data *lcd_dat = lcd_get_platform_data();
    //if (lcd_dat->pin_te != NO_CONFIG_PORT) {
    //     gpio_set_pull_up(lcd_dat->pin_te, 0);
    //     gpio_set_pull_down(lcd_dat->pin_te, 0);
    //    gpio_direction_input(lcd_dat->pin_te);
    //    gpio_set_die(lcd_dat->pin_te, 0);
    //}
    // delay_2ms(120 / 2);	// delay 120ms

}



/*
** 设置lcd退出睡眠
*/
static void lcd_exitsleep(void)
{
    y_printf("%s\n", __func__);
    // lcd_custom_reset();
#if DEEP_STANDBY
    // struct lcd_platform_data *lcd_dat = lcd_get_platform_data();
    // if (lcd_dat && lcd_dat->pin_reset) {
    //     gpio_direction_output(lcd_dat->pin_reset, 0);
    //     delay_2ms(4);	// delay >5ms
    //     gpio_direction_output(lcd_dat->pin_reset, 1);
    //     delay_2ms(4);	// delay >5ms
    // }
    /* u8 data = 0x29; */
    lcd_write_cmd(0x11, NULL, 1);
    delay_2ms(5);
    lcd_write_cmd(0x29, NULL, 0);
    delay_2ms(5);

    // extern struct imd_param lcd_spi_sh8601a_param;
    // lcd_init(&lcd_spi_sh8601a_param);
    // lcd_drv_cmd_list(lcd_cmd_list_sleepout, sizeof(lcd_cmd_list_sleepout) / sizeof(lcd_cmd_list_sleepout[0]));
#else
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
#endif
}


static u32 lcd_spi_nb3030_read_id()
{
    printf("##########################################################%s, %d\n", __FUNCTION__, __LINE__);
    u8 id1, id2, id3;

    lcd_read_cmd(0xda, &id1, 1);	// Read ID1
    lcd_read_cmd(0xdb, &id2, 1);	// Read ID2
    lcd_read_cmd(0xdc, &id3, 1);	// Read ID3

    return ((id1 << 16) | (id2 << 8) | id3);
}

struct dbi_param lcd_spi_nb3030_param = {
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

    .fps = 60,  //太高会闪屏

    /* .fps = 40, */

    .te_offset_line = 128,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

REGISTER_LCD_DEVICE(nb3030) = {
    .logo = "nb3030",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_cmd_t_nb3030,
    .cmd_cnt = sizeof(lcd_cmd_t_nb3030) / sizeof(lcd_cmd_t_nb3030[0]),
    .param   = (void *) &lcd_spi_nb3030_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,// 这款是LCD屏幕，需设置TCFG_BACKLIGHT_PWM_MODE=2调整背光
    .power_ctrl = lcd_spi_power_ctrl,
    .entersleep = lcd_entersleep,
    .exitsleep = lcd_exitsleep,
    .read_id = lcd_spi_nb3030_read_id,
    .lcd_id = 0x000000,
};


#endif




