/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"

/*
** 驱动代码的宏开关
*/
//<<<[qspi屏 400x400]>>>//
#if TCFG_LCD_QSPI_JD9161C_480x480_ENABLE


#define LCD_DRIVE_CONFIG                    QSPI_RGB565_RAMLESS_1T2B

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"

/*
 * 推QSPI RAMLESS屏有以下配置需要打开
 * <<sdk_config.h>>
 * #define TCFG_PSRAM_DEV_ENABLE 1 //需要内置PSRAM的封装
 * #define TCFG_PSRAM_SIZE 0x200000 //内置PSRAM容量
 * <<lib_jlui_config.c>>
 * const int JLUI_GPU_DMA_TO_PSRAM = 1
 * const int JLUI_LCD_RAMLESS_ENABLE = 1
 * <<board_ac707n_demo_cfg.h>>
 * #define TCFG_LCD_QSPI_JD9161C_480x480_ENABLE 1 // 关闭其他屏驱
 * #define TCFG_LCD_TE_USED_PEND 0
 * */

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

/*
** 初始化代码
*/
static const u8 lcd_qspi_jd9161c_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0xDF, 0x90, 0x68, 0xF8, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xDE, 0x00, _END_,
    _BEGIN_, 0xB2, 0x00, 0x4B, _END_,
    _BEGIN_, 0xB7, 0x00, 0x51, 0x00, 0xA1, 0x00, _END_,
    _BEGIN_, 0xBB, 0x65, 0xE4, 0xEC, 0xBC, 0xFF, 0x6D, 0xAD, 0xCB, 0xFF, _END_,
    _BEGIN_, 0xC3, 0x04, 0x04, 0x0C, 0x10, 0x10, 0x10, 0x0C, 0x10, 0x10, 0x10, 0x10, 0xB9, 0x0C, 0xB9, _END_,
    _BEGIN_, 0xC4, 0x00, 0xF0, 0xF0, 0xFF, 0x0E, 0x0B, _END_,
    _BEGIN_, 0xC5, 0x55, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xC9, 0x01, _END_,
    _BEGIN_, 0xC8, 0x70, 0x64, 0x6F, 0x4F, 0x52, 0x43, 0x4B, 0x36, 0x50, 0x4B, 0x4A, 0x66, 0x54, 0x5C, 0x4F, 0x4D, 0x3E, 0x33, 0x03, 0x70, 0x64, 0x6F, 0x4F, 0x52, 0x43, 0x4B, 0x36, 0x50, 0x4B, 0x4A, 0x66, 0x54, 0x5C, 0x4F, 0x4D, 0x3E, 0x33, 0x03, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xD0, 0x10, 0x00, 0x0A, 0x08, 0x06, 0x04, 0x17, 0x37, 0x1F, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, _END_,
    _BEGIN_, 0xD1, 0x11, 0x01, 0x0B, 0x09, 0x07, 0x05, 0x17, 0x37, 0x1F, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xD4, 0x10, 0x00, 0x00, 0x03, 0x60, 0x08, 0x20, 0x00, 0x00, 0x06, 0x98, 0x00, 0x00, 0xEE, 0x21, 0xA0, 0x00, 0x06, 0x98, 0x72, 0x0A, 0x06, 0x98, 0x00, 0x01, _END_,
    _BEGIN_, 0xD5, 0x04, 0x10, 0x90, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x60, 0x00, 0xF8, 0x50, 0x00, 0x02, 0x04, 0x00, 0x60, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xDE, 0x02, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0xB7, 0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x0F, 0x23, 0x20, 0x20, 0x20, 0x20, _END_,
    _BEGIN_, 0xDE, 0x00, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0x3A, 0x50, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0x35, _END_,
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 250, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,

    /* BIST模式(显示红色) */
    /* _BEGIN_, 0xc2, 0x21,_END_, */
    /* _BEGIN_, REGFLAG_DELAY, 10, _END_, */
    /* _BEGIN_, 0xc2, 0x23,_END_, */
    /* _BEGIN_, REGFLAG_DELAY, 10, _END_, */
};




/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_qspi_jd9161c_backlight_ctrl(u8 percent)
{
    if (percent) {

    } else {

    }

    return 0;
}


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
static int lcd_qspi_jd9161c_power_ctrl(u8 onoff)
{
    lcd_en_ctrl(onoff);
    return 0;
}



/*
** 设置lcd进入睡眠
*/
static void lcd_qspi_jd9161c_entersleep(void)
{
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2); // delay 120ms
}



/*
** 设置lcd退出睡眠
*/
static void lcd_qspi_jd9161c_exitsleep(void)
{
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);   // delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
}


struct dbi_param lcd_qspi_jd9161c_param = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,

    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = LCD_FORMAT,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .lcd_type = LCD_TYPE_SPI_RAMLESS,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 52,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
        .qspi_cmd = {
            .write_cmd = 0xde,
            .read_cmd = 0xdd,
        },
        .ramless = {
            .frame_sync_cmd = 0xde006100,
            .porch_sync_cmd = 0xde006000,
            .line_sync_cmd = 0xde006000,

            .hsync = 20,
            .hbp = 20,
            .hact = LCD_W,
            .hfp = 20,

            .vsync = 4,
            .vbp = 8,
            .vact = LCD_H,
            .vfp = 20,
        },
    },

    .debug_mode_en = false,
    .debug_mode_color = 0x00ff00,
};

REGISTER_LCD_DEVICE(jd9161c) = {
    .logo = "jd9161c",
    .row_addr_align    = 1,
    .column_addr_align = 1,

    .lcd_cmd = (void *) &lcd_qspi_jd9161c_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_qspi_jd9161c_cmd_list_poweron) / sizeof(lcd_qspi_jd9161c_cmd_list_poweron[0]),
    .param   = (void *) &lcd_qspi_jd9161c_param,

    .reset = NULL,  // 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL, //lcd_qspi_jd9161c_backlight_ctrl,
    .power_ctrl = lcd_qspi_jd9161c_power_ctrl,
    .entersleep = lcd_qspi_jd9161c_entersleep,
    .exitsleep = lcd_qspi_jd9161c_exitsleep,
};


#endif






