#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_rgb_st7701s_480x800.data.bss")
#pragma data_seg(".lcd_rgb_st7701s_480x800.data")
#pragma const_seg(".lcd_rgb_st7701s_480x800.text.const")
#pragma code_seg(".lcd_rgb_st7701s_480x800.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
** 注意：rgb屏需要一直推才能正常显示
*/
#if TCFG_LCD_RGB_ST7701S_480x800_ENABLE


#define LCD_DRIVE_CONFIG                    RGB_RGB888

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "system/includes.h"
#include "ui/ui_api.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"



#define SCR_X       0
#define SCR_Y       0
#define SCR_W       LCD_WIDTH
#define SCR_H       LCD_HEIGHT
#define LCD_W       LCD_WIDTH
#define LCD_H       LCD_HEIGHT
#define LCD_BLOCK_W LCD_WIDTH
#define LCD_BLOCK_H 16
#define BUF_NUM     2

static const u8 lcd_rgb_st7701s_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0x01, _END_, // soft reset. st7701s硬复位后发现还是可能出现不显示，加软复位命令可以解决.
    _BEGIN_, REGFLAG_DELAY, 10, _END_,

    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    //---------------------------Bank0 Setting---------------------------//
    //------------Display Control setting------------//
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10, _END_,
    _BEGIN_, 0xC0, 0x63, 0x00, _END_,
    _BEGIN_, 0xC1, 0x11, 0x02, _END_,
    _BEGIN_, 0xC2, 0x31, 0x08, _END_,
    _BEGIN_, 0xC3, 0x10, _END_,
    _BEGIN_, 0xCC, 0x10, _END_,
    //------------Gamma Cluster Setting--------------//
    _BEGIN_, 0xB0, 0x40, 0x01, 0x46, 0x0D, 0x13, 0x09, 0x05, 0x09, 0x09, 0x1B, 0x07, 0x15, 0x12, 0x4C, 0x10, 0xC8, _END_,
    _BEGIN_, 0xB1, 0x40, 0x02, 0x86, 0x0D, 0x13, 0x09, 0x05, 0x09, 0x09, 0x1F, 0x07, 0x15, 0x12, 0x15, 0x19, 0x08, _END_,
    //------------- End Gamma Setting--------------------//
    //------------End Display Control setting------------//
    //-----------------------Bank0 Setting End------------------------//


    //---------------------- Bank1 Setting----------------------------//
    //----------- Power Control Registers Initial -------- //
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11, _END_,
    _BEGIN_, 0xB0, 0x50, _END_,
    //--------------- Vcom Setting------------------ //
    _BEGIN_, 0xB1, 0x68, _END_,
    //------------- End Vcom Setting---------------- //
    _BEGIN_, 0xB2, 0x07, _END_,
    _BEGIN_, 0xB3, 0x80, _END_,
    _BEGIN_, 0xB5, 0x47, _END_,
    _BEGIN_, 0xB7, 0x85, _END_,
    _BEGIN_, 0xB8, 0x21, _END_,
    _BEGIN_, 0xB9, 0x10, _END_,
    _BEGIN_, 0xC1, 0x78, _END_,
    _BEGIN_, 0xC2, 0x78, _END_,
    _BEGIN_, 0xD0, 0x88, _END_,
    //-----End Power Control Registers Initial ------//
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    //----------------GIP Setting--------------------//
    _BEGIN_, 0xE0, 0x00, 0x00, 0x02, _END_,
    _BEGIN_, 0xE1, 0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, _END_,
    _BEGIN_, 0xE3, 0x00, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xE4, 0x44, 0x44, _END_,
    _BEGIN_, 0xE5, 0x0E, 0x2D, 0xA0, 0xA0, 0x10, 0x2D, 0xA0, 0xA0, 0x0A, 0x2D, 0xA0, 0xA0, 0x0C, 0x2D, 0xA0, 0xA0, _END_,
    _BEGIN_, 0xE6, 0x00, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xE7, 0x44, 0x44, _END_,
    _BEGIN_, 0xE8, 0x0D, 0x2D, 0xA0, 0xA0, 0x0F, 0x2D, 0xA0, 0xA0, 0x09, 0x2D, 0xA0, 0xA0, 0x0B, 0x2D, 0xA0, 0xA0, _END_,
    _BEGIN_, 0xEB, 0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40, _END_,
    _BEGIN_, 0xEC, 0x02, 0x01, _END_,
    _BEGIN_, 0xED, 0xAB, 0x89, 0x76, 0x54, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0x45, 0x67, 0x98, 0xBA, _END_,
    //---------------- End GIP Setting------------------- //
    //--------- Power Control Registers Initial End-------//
    //-------------------------- Bank1 Setting--------------------------- //
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00, _END_,

    _BEGIN_, 0x3A, 0x77, _END_,

    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
};

static struct dbi_param rgb_param = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = DBI_INPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_RGB,

    .buffer_num = 2,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 60,

    .rgb = {
        .hsync_polarity = HSYNC_POLARITY_FALLING,
        .vsync_polarity = VSYNC_POLARITY_FALLING,
        .den_polarity = DEN_POLARITY_RISING,
        .clock_polarity = CLOCK_POLARITY_IDLE_LOW,
        .clock_phase = CLOCK_PHASE_LEADING_EDGE,

        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .continue_frames = 1,

        .hsync = 1,
        .hbp = 15,
        .hact = 480,
        .hfp = 2,

        .vsync = 2,
        .vbp = 16,
        .vact = 800,
        .vfp = 1,
    },

    .debug_mode_en = false,
    /* .debug_mode_color = 0xffff11, */
    .debug_mode_color = 0xffff00,
};

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/


/*
** 设置lcd进入睡眠
*/
static void lcd_entersleep(void)
{
    //TODO
}

/*
** 设置lcd退出睡眠
*/
static void lcd_exitsleep(void)
{
    //TODO
}


REGISTER_LCD_DEVICE(rgb) = {
    .logo = "rgb_st7701s",

    .lcd_cmd = (void *) &lcd_rgb_st7701s_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_rgb_st7701s_cmd_list_poweron) / sizeof(lcd_rgb_st7701s_cmd_list_poweron[0]),
    .param  = (void *) &rgb_param,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};



#endif

