/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
#if TCFG_LCD_MCU_ST7789V_240x320_ENABLE


#define LCD_DRIVE_CONFIG                    MCU_RGB565
/* #define LCD_DRIVE_CONFIG                    MCU_RGB666 */
/* #define LCD_DRIVE_CONFIG                    MCU_RGB888 */

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
#include "ui/ui_api.h"
#include "ui/lcd/lcd_drive.h"


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


static const u8 lcd_mcu_st7789v_cmdlist[] ALIGNED(4) = {
    /* _BEGIN_, 0x01, _END_,				// soft reset */
    /* _BEGIN_, REGFLAG_DELAY, 120, _END_,	// delay 120ms */
    _BEGIN_, 0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    // _BEGIN_, 0x36, 0x60, _END_, // 旧: MX|MV 旋转90°横屏(320x240)
    _BEGIN_, 0x36, 0x00, _END_, // MADCTL: 原生竖屏(物理240x320→显示240x320)
    _BEGIN_, 0x3A, 0x05, _END_,
    _BEGIN_, 0xB7, 0x74, _END_,
    _BEGIN_, 0xBB, 0x1E, _END_,
    _BEGIN_, 0xC0, 0x2C, _END_,
    _BEGIN_, 0xC2, 0x01, _END_,
    _BEGIN_, 0xC3, 0x19, _END_,
    _BEGIN_, 0xC6, 0x0F, _END_,
    _BEGIN_, 0xD0, 0xA7, _END_,
    _BEGIN_, 0xD0, 0xA4, 0xA1, _END_,
    _BEGIN_, 0xD6, 0xA1, _END_,
    _BEGIN_, 0xE0, 0xF0, 0x08, 0x0E, 0x09, 0x08, 0x16, 0x33, 0x43, 0x4A, 0x38, 0x14, 0x14, 0x2E, 0x32, _END_,
    _BEGIN_, 0xE1, 0xF0, 0x0A, 0x10, 0x0A, 0x09, 0x04, 0x33, 0x33, 0x49, 0x0A, 0x16, 0x15, 0x2C, 0x30, _END_,
    _BEGIN_, 0x21, _END_,
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 20, _END_,
    /* _BEGIN_, 0X2C, _END_, */
    /* _BEGIN_, REGFLAG_DELAY, 120, _END_, */
};


static void delay_2ms(int cnt)
{
    if (cnt * 2 > 10) {
        os_time_dly(cnt * 2 / 10 + 1);
    } else {
        mdelay(2 * cnt);
    }
}

struct dbi_param lcd_mcu_st7789v_param = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = OUTPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_MCU,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,
    .fps = 60,
    // .rotate = LCD_ROTATE_90,

    .mcu = {
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_mcu_st7789v_backlight_ctrl(u8 onoff)
{
    return 0;
}


/*
** 设置lcd进入睡眠
*/
static void lcd_mcu_st7789v_entersleep(void)
{
    //TODO
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2); // delay 120ms
}

/*
** 设置lcd退出睡眠
*/
static void lcd_mcu_st7789v_exitsleep(void)
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

    .lcd_cmd = (void *) &lcd_mcu_st7789v_cmdlist,
    .cmd_cnt = sizeof(lcd_mcu_st7789v_cmdlist) / sizeof(lcd_mcu_st7789v_cmdlist[0]),
    .param   = (void *) &lcd_mcu_st7789v_param,

    .reset          = NULL, // 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,//lcd_mcu_st7789v_backlight_ctrl,
    .entersleep     = lcd_mcu_st7789v_entersleep,
    .exitsleep      = lcd_mcu_st7789v_exitsleep,
};



#endif



