/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
#if TCFG_LCD_MCU_NV3031A_240x320_ENABLE


#define LCD_DRIVE_CONFIG                    MCU_RGB565

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

//========== nv3031a+INx 1.3" initial setting ==========//
static const u8 lcd_mcu_nv3031a_cmdlist[] ALIGNED(4) = {
    _BEGIN_, 0xFD, 0x06, 0x08, _END_,
    _BEGIN_, 0x61, 0x07, 0x07, _END_,
    _BEGIN_, 0x73, 0x70, _END_,
    _BEGIN_, 0x73, 0x00, _END_,
    _BEGIN_, 0x61, 0x07, 0x07, _END_,
    _BEGIN_, 0x62, 0x00, 0x44, 0x40, _END_,
    _BEGIN_, 0x65, 0x08, 0x10, 0x21, _END_,
    _BEGIN_, 0x66, 0x08, 0x10, 0x21, _END_,
    _BEGIN_, 0x67, 0x21, 0x40, _END_,
    _BEGIN_, 0x68, 0x70, 0x37, 0x2A, 0x16, _END_,
    _BEGIN_, 0xB1, 0x0F, 0x02, 0x00, _END_,
    _BEGIN_, 0xB4, 0x01, _END_,
    _BEGIN_, 0xB5, 0x02, 0x02, 0x0A, 0x14, _END_,
    _BEGIN_, 0xB6, 0x44, 0x01, 0x9F, 0x00, 0x02, _END_,
    _BEGIN_, 0xE6, 0x00, 0xFF, _END_,
    _BEGIN_, 0xE7, 0x01, 0x04, 0x03, 0x03, 0x00, 0x12, _END_,
    _BEGIN_, 0xE8, 0x00, 0x70, 0x00, _END_,
    _BEGIN_, 0xEC, 0x52, _END_,
    _BEGIN_, 0xDF, 0x11, _END_,
    _BEGIN_, 0x62, 0x00, 0x44, 0x40, _END_,
    _BEGIN_, 0xE0, 0x07, 0x07, 0x0a, 0x13, 0x0e, 0x11, 0x11, 0x19, _END_,

    _BEGIN_, 0xe3, 0x12, 0x0f, 0x11, 0x0e, 0x10, 0x09, 0x05, 0x06, _END_,
    _BEGIN_, 0xe1, 0x2a, 0x71, _END_,
    _BEGIN_, 0xe4, 0x6b, 0x2a, _END_,

    _BEGIN_, 0xe2, 0x00, 0x13, 0x17, 0x31, 0x36, 0x3f, _END_,
    _BEGIN_, 0xe5, 0x3f, 0x36, 0x32, 0x17, 0x13, 0x00, _END_,
    _BEGIN_, 0xf6, 0x01, 0x30, 0x00, 0x00, _END_,
    _BEGIN_, 0xf1, 0x01, 0x21, 0x22, _END_,
    _BEGIN_, 0xfd, 0xfa, 0xfc, _END_,
    _BEGIN_, 0x3a, 0x55, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x36, 0x00, _END_,
    _BEGIN_, 0x11, _END_,

    _BEGIN_, REGFLAG_DELAY, 0x0f, _END_,
    //DISP ON
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 0x05, _END_,
    _BEGIN_, 0x2c, _END_,
    /* _BEGIN_, 0x23, _END_,  */
    // _BEGIN_, 0x55, 2, _END_,
};


struct dbi_param lcd_mcu_nv3031a_param = {
    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = OUTPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_MCU,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,
    .fps = 60,

    .mcu = {
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
    },

    .debug_mode_en = false,
    .debug_mode_color = 0x0000ff,
};

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_mcu_nv3031a_backlight_ctrl(u8 onoff)
{
    return 0;
}


/*
** 设置lcd进入睡眠
*/
static void lcd_mcu_nv3031a_entersleep(void)
{
    //TODO
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2);	// delay 120ms
}

/*
** 设置lcd退出睡眠
*/
static void lcd_mcu_nv3031a_exitsleep(void)
{
    //TODO
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
}




REGISTER_LCD_DEVICE(nv3031a) = {
    .logo = "nv3031a",
    .row_addr_align 	= 1,
    .column_addr_align	= 1,

    .lcd_cmd = (void *) &lcd_mcu_nv3031a_cmdlist,
    .cmd_cnt = sizeof(lcd_mcu_nv3031a_cmdlist) / sizeof(lcd_mcu_nv3031a_cmdlist[0]),
    .param   = (void *) &lcd_mcu_nv3031a_param,

    .reset 			= NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,//lcd_mcu_nv3031a_backlight_ctrl,
    .entersleep 	= lcd_mcu_nv3031a_entersleep,
    .exitsleep 		= lcd_mcu_nv3031a_exitsleep,
};



#endif //endif TCFG_LCD_MCU_NV3031A_ENABLE


