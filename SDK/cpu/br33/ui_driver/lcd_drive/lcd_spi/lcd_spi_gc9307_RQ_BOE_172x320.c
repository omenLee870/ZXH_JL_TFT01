
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
#if (TCFG_SPI_LCD_ENABLE && TCFG_LCD_GC9307_172X320)
#define LCD_DRIVE_CONFIG				SPI_4WIRE_RGB565_1T8B


/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"

//  GRAM区是240（宽）*320（高）
//  相对位置：
// RAM-X1 --- LCD-X1 ------ LCD-X2 ----- RAM-X2
// (0,0) -34- (34,0) -172- (206,0) -34- (240,0)
//
//  LCD的范围:
// (34,0)   (206,0)
// (34,320) (206,320)


#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define SCR_X       34
#define SCR_Y       0
#else
#define SCR_X       0
#define SCR_Y       34
#endif

//  UI刷新区

#define SCR_W       320//LCD_WIDTH // 320
#define SCR_H       172//LCD_HEIGHT // 172
//  屏区
#define LCD_W       320//(LCD_WIDTH)// 320//172 //LCD_HEIGHT//172//(LCD_WIDTH) // 目前刷新区域超出LCD会ASSERT，临时加大宽度解决，但会导致双buf申请偏大
#define LCD_H       172//(LCD_HEIGHT)// 172 //LCD_WIDTH//320 (LCD_WIDTH) //LCD_HEIGHT
#define LCD_BLOCK_W 32//LCD_WIDTH
#define LCD_BLOCK_H 172
#define BUF_NUM     2

#define LCD_FORMAT OUTPUT_FORMAT_RGB565


struct dbi_param lcd_spi_gc9307_param;

#define MY 0     //Y反向（画面）

#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define MV 0    //
#define MX 1     //
#else
#define MV 1     //90旋转（画面）
#define MX 0     //X反向（画面）
#endif

#define ML 0
#define BGR 1       //大小端颜色
#define MH 0

// Frame Rate = 47.62KHz/(136*(RTN1+4)+RTN2)）
#define DINV  0x0//0-15
#define RTN1  0x1//0-15
#define RTN2   0x72 //0-255
/*
** 初始化代码
*/
static const u8 lcd_cmd_t_nv9307[] ALIGNED(4) = {
    // _BEGIN_, 0x01, _END_,				// soft reset
    // _BEGIN_, REGFLAG_DELAY, 120, _END_,	// delay 120ms
    // _BEGIN_, 0x11, _END_,				// sleep out
    // _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0xfe, _END_,
    _BEGIN_, 0xef, _END_,

    _BEGIN_, 0x36, (MY << 7) | (MX << 6) | (MV << 5) | (ML << 4) | (BGR << 3) | (MH << 2), _END_,
    _BEGIN_, 0x3a, 0x05, _END_,

    _BEGIN_, 0x85, 0xC0, _END_,
    _BEGIN_, 0x86, 0x98, _END_,
    _BEGIN_, 0x87, 0x28, _END_,
    _BEGIN_, 0x89, 0x33, _END_,
    _BEGIN_, 0x8b, 0x84, _END_,
    _BEGIN_, 0x8d, 0x3b, _END_,
    _BEGIN_, 0x8e, 0x0f, _END_,
    _BEGIN_, 0x8f, 0x70, _END_,

    _BEGIN_, 0xe8, (DINV << 4) | (RTN1), RTN2, _END_,
    _BEGIN_, 0xec, 0x57, 0x07, 0xff, _END_,
    _BEGIN_, 0xed, 0x18, 0x09, _END_,

    //_BEGIN_, 0xc3, 0x29, _END_,
    //_BEGIN_, 0xc4, 0x45, _END_,       ///vcom烧录了
    _BEGIN_, 0xc9, 0x10, _END_,

    _BEGIN_, 0xff, 0x61, _END_,

    _BEGIN_, 0x99, 0x3a, _END_,
    _BEGIN_, 0x9d, 0x43, _END_,
    _BEGIN_, 0x98, 0x3e, _END_,
    _BEGIN_, 0x9c, 0x4b, _END_,

    _BEGIN_, 0xF0, 0x06, 0x08, 0x08, 0x06, 0x05, 0x1d, _END_,
    _BEGIN_, 0xF2, 0x00, 0x01, 0x09, 0x07, 0x04, 0x23, _END_,
    _BEGIN_, 0xF1, 0x3b, 0x68, 0x66, 0x36, 0x35, 0x2f, _END_,
    _BEGIN_, 0xF3, 0x37, 0x6a, 0x66, 0x37, 0x35, 0x35, _END_,

    _BEGIN_, 0xFA, 0x80, 0x0f, _END_,
    _BEGIN_, 0xBE, 0x11, _END_,     //source bias


    _BEGIN_, 0xCB, 0x02, _END_,
    _BEGIN_, 0xCD, 0x22, _END_,
    _BEGIN_, 0x9B, 0xFF, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x44, 0x00, 0x1a, _END_,

    //_BEGIN_, 0x2B,0x00,0x00,0x00,0xAB, _END_,
    //_BEGIN_, 0x2A,0x00,0x00,0x01,0x3F, _END_,

    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,
    //_BEGIN_, 0x2c, _END_,

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
static int lcd_spi_gc9307_power_ctrl(u8 onoff)
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
    delay_2ms(120 / 2);	// delay 120ms
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(50 / 2);	// delay 120ms
#if 0

    // 把TE设置成高阻态，降低功耗
    //extern struct lcd_platform_data *lcd_get_platform_data();
    // struct lcd_platform_data *lcd_dat = lcd_get_platform_data();
    //if (lcd_dat->pin_te != NO_CONFIG_PORT) {
    //    gpio_set_pull_up(lcd_dat->pin_te, 0);
    //    gpio_set_pull_down(lcd_dat->pin_te, 0);
    //    gpio_direction_input(lcd_dat->pin_te);
    //    gpio_set_die(lcd_dat->pin_te, 0);
    //}
#endif
}

/*
** 设置lcd退出睡眠
*/
static void lcd_exitsleep(void)
{
    r_printf("LCD进入工作..................");

    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(120 / 2);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
}


static u32 lcd_spi_gc9307_read_id()
{
    printf("##########################################################%s, %d\n", __FUNCTION__, __LINE__);
    u8 id1, id2, id3;

    lcd_read_cmd(0xda, &id1, 1);	// Read ID1
    lcd_read_cmd(0xdb, &id2, 1);	// Read ID2
    lcd_read_cmd(0xdc, &id3, 1);	// Read ID3

    return ((id1 << 16) | (id2 << 8) | id3);
}

int lcd_get_scanline()
{
    u8 scanline[2] = {0};
    lcd_read_cmd(0x45, scanline, 2);	// Read ID1

    return (scanline[1] | (((u16)(scanline[0] & 1) << 8)));
}

struct dbi_param lcd_spi_gc9307_param = {


#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
    .scr_x    = 34,//SCR_X,
    .scr_y	  = 0,//SCR_Y,
    .scr_w	  = 320,//172,//320,// SCR_W,
    .scr_h	  = 172,//320,//172,//SCR_H,

    .in_width  = 320,//SCR_W,
    .in_height = 172,//SCR_H,
    .in_format = LCD_FORMAT,


    .lcd_width  = 172,//LCD_W,
    .lcd_height = 320,//LCD_H,
#else
    .scr_x    = 0,//SCR_X,
    .scr_y	  = 34,//SCR_Y,
    .scr_w	  = 320,//172,//320,// SCR_W,
    .scr_h	  = 172,//320,//172,//SCR_H,

    .in_width  = 320,//SCR_W,
    .in_height = 172,//SCR_H,
    .in_format = LCD_FORMAT,


    .lcd_width  = 320,//LCD_W,
    .lcd_height = 172,//LCD_H,

#endif


    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 60,

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

REGISTER_LCD_DEVICE(gc9307) = {
    .logo = "gc9307",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_cmd_t_nv9307,
    .cmd_cnt = sizeof(lcd_cmd_t_nv9307) / sizeof(lcd_cmd_t_nv9307[0]),
    .param   = (void *) &lcd_spi_gc9307_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,// 这款是LCD屏幕，需设置TCFG_BACKLIGHT_PWM_MODE=2调整背光
    .power_ctrl = lcd_spi_gc9307_power_ctrl,
    .entersleep = lcd_entersleep,
    .exitsleep = lcd_exitsleep,
    .read_id = lcd_spi_gc9307_read_id,
    .lcd_id = 0x000000,
};


#endif




