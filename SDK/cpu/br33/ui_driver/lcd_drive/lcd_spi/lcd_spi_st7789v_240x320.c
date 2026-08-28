/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
#if TCFG_LCD_SPI_ST7789V_240x320_ENABLE

#define LCD_DRIVE_CONFIG                    SPI_4WIRE_RGB565_1T16B
/* #define LCD_DRIVE_CONFIG                    SPI_4WIRE_RGB666_1T18B */
/* #define LCD_DRIVE_CONFIG                    SPI_4WIRE_RGB888_1T24B */

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"
#include "ui/lcd/lcd_drive.h"

/* sdk_config.h参考配置 */
/* #define TCFG_DBI_TYPE                             CONFIG_DBI_TYPE_SPI // 接口类型 */
/* #define TCFG_LCD_PIN_EN                           NO_CONFIG_PORT   // 电源控制 */
/* #define TCFG_LCD_PIN_RESET                        IO_PORTB_03      // 复位IO */
/* #define TCFG_LCD_PIN_BL                           IO_PORTB_02      // 背光控制 */
/* #define TCFG_LCD_PIN_BL_TYPE                      0                // 背光控制类型 */
/* #define TCFG_LCD_PIN_TE                           NO_CONFIG_PORT      // LCD TE */
/* #define TCFG_LCD_PIN_CS                           IO_PORTB_05      // LCD CS */
/* #define TCFG_LCD_PIN_SCL                          IO_PORTB_06      // LCD SCL */
/* #define TCFG_LCD_PIN_RWX                          NO_CONFIG_PORT   // LCD RWX */
/* #define TCFG_LCD_PIN_DC                           IO_PORTB_04   // LCD DCX */
/* #define TCFG_LCD_PIN_DATA_NUM                     1                // DATA数设置 */
/* #define TCFG_LCD_PIN_D0                           IO_PORTB_07      // SPI D0 */
/* #define TCFG_LCD_PIN_D1                           IO_PORTB_08      // SPI D1 */
/* #define TCFG_LCD_PIN_D2                           IO_PORTB_09      // SPI D2 */
/* #define TCFG_LCD_PIN_D3                           IO_PORTB_10      // SPI D3 */

#define SCR_X       0
#define SCR_Y       0
#define SCR_W       LCD_WIDTH
#define SCR_H       LCD_HEIGHT
#define LCD_W       LCD_WIDTH
#define LCD_H       LCD_HEIGHT
#define LCD_BLOCK_W LCD_WIDTH
#define LCD_BLOCK_H 20
#define BUF_NUM     2

#define TELINE 0x0000

static const u8 lcd_spi_st7789v_cmdlist[] ALIGNED(4) = {
    //----------------------------------------------------------------------------------------------------//
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,

    //------------------------------display and color format setting-------------
    _BEGIN_, 0x36, 0x00, _END_,
    _BEGIN_, 0x3a, 0x05, _END_,

    //--------------------------------ST7789V Frame rate setting-----------------
    _BEGIN_, 0xb2, 0x0c, 0x0c, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xb7, 0x35, _END_,

    //---------------------------------ST7789V Power setting---------------------
    _BEGIN_, 0xbb, 0x19, _END_,
    _BEGIN_, 0xc0, 0x2c, _END_,
    _BEGIN_, 0xc2, 0x01, _END_,
    _BEGIN_, 0xc3, 0x12, _END_,
    _BEGIN_, 0xc4, 0x20, _END_,
    _BEGIN_, 0xc6, 0x0f, _END_,
    _BEGIN_, 0xd0, 0xa4, 0xa1, _END_,

    //--------------------------------ST7789V gamma setting----------------------
    _BEGIN_, 0xe0, 0xd0, 0x04, 0x0d, 0x11, 0x13, 0x2b, 0x3f, 0x54, 0x4c, 0x18, 0x0d, 0x0b, 0x1f, 0x23, _END_,
    _BEGIN_, 0xe1, 0xd0, 0x04, 0x0c, 0x11, 0x13, 0x2c, 0x3f, 0x44, 0x51, 0x2f, 0x1f, 0x1f, 0x20, 0x23, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 50, _END_,
};


static void delay_2ms(int cnt)
{
    if (cnt * 2 > 10) {
        os_time_dly(cnt * 2 / 10 + 1);
    } else {
        mdelay(2 * cnt);
    }
}

struct dbi_param lcd_spi_st7789v_param = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = OUTPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,
    .fps = 60,
    .fps_constraint = FPS_CONSTRAINT_LESS_THAN,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_spi_st7789v_backlight_ctrl(u8 onoff)
{
    return 0;
}

/*
** 设置lcd进入睡眠
*/
static void lcd_spi_st7789v_entersleep(void)
{
    //TODO
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2); // delay 120ms
}

/*
** 设置lcd退出睡眠
*/
static void lcd_spi_st7789v_exitsleep(void)
{
    //TODO
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);   // delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
}

REGISTER_LCD_DEVICE(st7789v) = {
    .logo = "st7789v",
    .row_addr_align     = 1,
    .column_addr_align  = 1,

    .lcd_cmd = (void *) &lcd_spi_st7789v_cmdlist,
    .cmd_cnt = sizeof(lcd_spi_st7789v_cmdlist) / sizeof(lcd_spi_st7789v_cmdlist[0]),
    .param   = (void *) &lcd_spi_st7789v_param,

    .reset          = NULL, // 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,//lcd_spi_st7789v_backlight_ctrl,
    .entersleep     = lcd_spi_st7789v_entersleep,
    .exitsleep      = lcd_spi_st7789v_exitsleep,
};

#endif

