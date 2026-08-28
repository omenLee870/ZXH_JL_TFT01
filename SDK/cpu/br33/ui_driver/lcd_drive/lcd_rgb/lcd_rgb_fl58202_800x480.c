#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_rgb_fl58202_800x480.data.bss")
#pragma data_seg(".lcd_rgb_fl58202_800x480.data")
#pragma const_seg(".lcd_rgb_fl58202_800x480.text.const")
#pragma code_seg(".lcd_rgb_fl58202_800x480.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
** 注意：rgb屏需要一直推才能正常显示
*/
#if TCFG_LCD_RGB_FL58202_800x480_ENABLE


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

static const u8 lcd_rgb_fl58202_cmd_list_poweron[] ALIGNED(4) = {
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

        .hsync = 4,
        .hbp = 18,
        .hact = 800,
        .hfp = 8,

        .vsync = 4,
        .vbp = 8,
        .vact = 480,
        .vfp = 8,
    },

    .debug_mode_en = false,
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
    .logo = "rgb_fl58202_800x480",

    .lcd_cmd = (void *) &lcd_rgb_fl58202_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_rgb_fl58202_cmd_list_poweron) / sizeof(lcd_rgb_fl58202_cmd_list_poweron[0]),
    .param  = (void *) &rgb_param,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};



#endif

