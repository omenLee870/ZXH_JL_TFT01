#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_mipi_st7701s_480x854.data.bss")
#pragma data_seg(".lcd_mipi_st7701s_480x854.data")
#pragma const_seg(".lcd_mipi_st7701s_480x854.text.const")
#pragma code_seg(".lcd_mipi_st7701s_480x854.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
** 注意：rgb屏需要一直推才能正常显示
*/
#if TCFG_LCD_MIPI_ST7701S_480x854_ENABLE


/* #define LCD_DRIVE_CONFIG                    MIPI_RGB565_VDO // VIDEO_MODE */
/* #define LCD_DRIVE_CONFIG                    MIPI_RGB666 */
/* #define LCD_DRIVE_CONFIG                    MIPI_RGB666_LOOSELY */
#define LCD_DRIVE_CONFIG                    MIPI_RGB888

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

static const u8 lcd_mipi_st7701s_480x854_cmd_list_poweron[] ALIGNED(4) = {
    _BEGIN_, 0x01, 0x00, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11, _END_,
    _BEGIN_, 0xD1, 0x11, _END_,
    _BEGIN_, 0x11, 0x00, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10, _END_,
    _BEGIN_, 0xC0, 0xE9, 0x03, _END_,
    _BEGIN_, 0xC1, 0x12, 0x02, _END_,
    _BEGIN_, 0xC2, 0x37, 0x08, _END_,
    _BEGIN_, 0xB0, 0x00, 0x0E, 0x15, 0x0F, 0x11, 0x08, 0x08, 0x08, 0x08, 0x23, 0x04, 0x13, 0x12, 0x2B, 0x34, 0x1F, _END_,
    _BEGIN_, 0xB1, 0x00, 0x0E, 0x95, 0x0F, 0x13, 0x07, 0x09, 0x08, 0x08, 0x22, 0x04, 0x10, 0x0E, 0x2C, 0x34, 0x1F, _END_,
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11, _END_,
    _BEGIN_, 0xB0, 0x4D, _END_,
    _BEGIN_, 0xB1, 0x13, _END_,
    _BEGIN_, 0xB2, 0x07, _END_,
    _BEGIN_, 0xB3, 0x80, _END_,
    _BEGIN_, 0xB5, 0x47, _END_,
    _BEGIN_, 0xB7, 0x85, _END_,
    _BEGIN_, 0xB8, 0x20, _END_,
    _BEGIN_, 0xC1, 0x78, _END_,
    _BEGIN_, 0xC2, 0x78, _END_,
    _BEGIN_, 0xD0, 0x88, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    _BEGIN_, 0xE0, 0x00, 0x00, 0x02, _END_,
    _BEGIN_, 0xE1, 0x0B, 0x00, 0x0D, 0x00, 0x0C, 0x00, 0x0E, 0x00, 0x00, 0x44, 0x44, _END_,
    _BEGIN_, 0xE2, 0x33, 0x33, 0x44, 0x44, 0x64, 0x00, 0x66, 0x00, 0x65, 0x00, 0x67, 0x00, 0x00, _END_,
    _BEGIN_, 0xE3, 0x00, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xE4, 0x44, 0x44, _END_,
    _BEGIN_, 0xE5, 0x0C, 0x78, 0x3C, 0xA0, 0x0E, 0x78, 0x3C, 0xA0, 0x10, 0x78, 0x3C, 0xA0, 0x12, 0x78, 0x3C, 0xA0, _END_,
    _BEGIN_, 0xE6, 0x00, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xE7, 0x44, 0x44, _END_,
    _BEGIN_, 0xE8, 0x0D, 0x78, 0x3C, 0xA0, 0x0F, 0x78, 0x3C, 0xA0, 0x11, 0x78, 0x3C, 0xA0, 0x13, 0x78, 0x3C, 0xA0, _END_,
    _BEGIN_, 0xEB, 0x02, 0x02, 0x39, 0x39, 0xEE, 0x44, 0x00, _END_,
    _BEGIN_, 0xEC, 0x00, 0x00, _END_,
    _BEGIN_, 0xED, 0xFF, 0xF1, 0x04, 0x56, 0x72, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x27, 0x65, 0x40, 0x1F, 0xFF, _END_,
    _BEGIN_, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00, _END_,
    _BEGIN_, 0x36, 0x00, _END_,
    _BEGIN_, 0x35, 0x00, _END_, // 开TE
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
};

static struct dbi_param lcd_mipi_st7701s_480x854 = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = DBI_INPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_MIPI,

    .buffer_num = 2,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 60,
    .rotate = LCD_ROTATE_90,

    .mipi = {
        .frame_timeout_ms = 33,

        .virtual_channel = 0,
        .display_mode = DSI_VIDEO_MODE,
        .color_code = OUT_FORMAT_CC(LCD_DRIVE_CONFIG),
        .color_sort = OUT_FORMAT_CS(LCD_DRIVE_CONFIG),
        .sync_mode = DSI_SYNC_PULSE,
        .clk_mode = DSI_CK_HS_LPDT,

        .bllp0_en = 1,
        .bllp1_en = 0,
        .bllp2_en = 1,
        .eotp_en = 1,

        .hsync = 8,
        .hbp = 84,
        .hact = 480,
        .hfp = 88,

        .vsync = 4,
        .vbp = 8,
        .vact = 854,
        .vfp = 20,
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


REGISTER_LCD_DEVICE(mipi_st7701s_480x854) = {
    .logo = "mipi_st7701s_480x854",

    .lcd_cmd = (void *) &lcd_mipi_st7701s_480x854_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_mipi_st7701s_480x854_cmd_list_poweron) / sizeof(lcd_mipi_st7701s_480x854_cmd_list_poweron[0]),
    .param  = (void *) &lcd_mipi_st7701s_480x854,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};



#endif

