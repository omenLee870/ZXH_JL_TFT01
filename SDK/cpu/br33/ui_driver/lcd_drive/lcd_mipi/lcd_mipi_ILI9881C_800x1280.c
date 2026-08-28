#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_mipi_ili9881c_800x1280.data.bss")
#pragma data_seg(".lcd_mipi_ili9881c_800x1280.data")
#pragma const_seg(".lcd_mipi_ili9881c_800x1280.text.const")
#pragma code_seg(".lcd_mipi_ili9881c_800x1280.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
#if defined(TCFG_LCD_MIPI_ILI9881C_800x1280_ENABLE) && TCFG_LCD_MIPI_ILI9881C_800x1280_ENABLE


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

static const u8 lcd_mipi_ili9881c_800x1280_cmd_list_poweron[] ALIGNED(4) = {

    _BEGIN_, 0xFF, 0x98, 0x81, 0x03, _END_,//PAGE3
    _BEGIN_, 0x01, 0x00, _END_,
    _BEGIN_, 0x02, 0x00, _END_,
    _BEGIN_, 0x03, 0x53, _END_,
    _BEGIN_, 0x04, 0x53, _END_,
    _BEGIN_, 0x05, 0x13, _END_,
    _BEGIN_, 0x06, 0x04, _END_,
    _BEGIN_, 0x07, 0x02, _END_,
    _BEGIN_, 0x08, 0x02, _END_,
    _BEGIN_, 0x09, 0x00, _END_,
    _BEGIN_, 0x0a, 0x00, _END_,
    _BEGIN_, 0x0b, 0x00, _END_,
    _BEGIN_, 0x0c, 0x00, _END_,
    _BEGIN_, 0x0d, 0x00, _END_,
    _BEGIN_, 0x0e, 0x00, _END_,
    _BEGIN_, 0x0f, 0x00, _END_,
    _BEGIN_, 0x10, 0x00, _END_,
    _BEGIN_, 0x11, 0x00, _END_,
    _BEGIN_, 0x12, 0x00, _END_,
    _BEGIN_, 0x13, 0x00, _END_,
    _BEGIN_, 0x14, 0x00, _END_,
    _BEGIN_, 0x15, 0x00, _END_,
    _BEGIN_, 0x16, 0x00, _END_,
    _BEGIN_, 0x17, 0x00, _END_,
    _BEGIN_, 0x18, 0x00, _END_,
    _BEGIN_, 0x19, 0x00, _END_,
    _BEGIN_, 0x1a, 0x00, _END_,
    _BEGIN_, 0x1b, 0x00, _END_,
    _BEGIN_, 0x1c, 0x00, _END_,
    _BEGIN_, 0x1d, 0x00, _END_,
    _BEGIN_, 0x1e, 0xc0, _END_,
    _BEGIN_, 0x1f, 0x00, _END_,
    _BEGIN_, 0x20, 0x02, _END_,
    _BEGIN_, 0x21, 0x09, _END_,
    _BEGIN_, 0x22, 0x00, _END_,
    _BEGIN_, 0x23, 0x00, _END_,
    _BEGIN_, 0x24, 0x00, _END_,
    _BEGIN_, 0x25, 0x00, _END_,
    _BEGIN_, 0x26, 0x00, _END_,
    _BEGIN_, 0x27, 0x00, _END_,
    _BEGIN_, 0x28, 0x55, _END_,
    _BEGIN_, 0x29, 0x03, _END_,
    _BEGIN_, 0x2a, 0x00, _END_,
    _BEGIN_, 0x2b, 0x00, _END_,
    _BEGIN_, 0x2c, 0x00, _END_,
    _BEGIN_, 0x2d, 0x00, _END_,
    _BEGIN_, 0x2e, 0x00, _END_,
    _BEGIN_, 0x2f, 0x00, _END_,
    _BEGIN_, 0x30, 0x00, _END_,
    _BEGIN_, 0x31, 0x00, _END_,
    _BEGIN_, 0x32, 0x00, _END_,
    _BEGIN_, 0x33, 0x00, _END_,
    _BEGIN_, 0x34, 0x00, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x36, 0x00, _END_,
    _BEGIN_, 0x37, 0x00, _END_,
    _BEGIN_, 0x38, 0x3C, _END_,
    _BEGIN_, 0x39, 0x00, _END_,
    _BEGIN_, 0x3a, 0x00, _END_,
    _BEGIN_, 0x3b, 0x00, _END_,
    _BEGIN_, 0x3c, 0x00, _END_,
    _BEGIN_, 0x3d, 0x00, _END_,
    _BEGIN_, 0x3e, 0x00, _END_,
    _BEGIN_, 0x3f, 0x00, _END_,
    _BEGIN_, 0x40, 0x00, _END_,
    _BEGIN_, 0x41, 0x00, _END_,
    _BEGIN_, 0x42, 0x00, _END_,
    _BEGIN_, 0x43, 0x00, _END_,
    _BEGIN_, 0x44, 0x00, _END_,
    _BEGIN_, 0x45, 0x00, _END_,

    _BEGIN_, 0x50, 0x01, _END_,
    _BEGIN_, 0x51, 0x23, _END_,
    _BEGIN_, 0x52, 0x45, _END_,
    _BEGIN_, 0x53, 0x67, _END_,
    _BEGIN_, 0x54, 0x89, _END_,
    _BEGIN_, 0x55, 0xab, _END_,
    _BEGIN_, 0x56, 0x01, _END_,
    _BEGIN_, 0x57, 0x23, _END_,
    _BEGIN_, 0x58, 0x45, _END_,
    _BEGIN_, 0x59, 0x67, _END_,
    _BEGIN_, 0x5a, 0x89, _END_,
    _BEGIN_, 0x5b, 0xab, _END_,
    _BEGIN_, 0x5c, 0xcd, _END_,
    _BEGIN_, 0x5d, 0xef, _END_,

    _BEGIN_, 0x5e, 0x01, _END_,
    _BEGIN_, 0x5f, 0x0A, _END_, //FW_CGOUT_L[1] RESE_ODD
    _BEGIN_, 0x60, 0x02, _END_, //FW_CGOUT_L[2] VSSG_ODD
    _BEGIN_, 0x61, 0x02, _END_, //FW_CGOUT_L[3] VSSG_ODD
    _BEGIN_, 0x62, 0x08, _END_, //FW_CGOUT_L[4] STV2_ODD
    _BEGIN_, 0x63, 0x15, _END_, //FW_CGOUT_L[5] VDD2_ODD
    _BEGIN_, 0x64, 0x14, _END_, //FW_CGOUT_L[6] VDD1_ODD
    _BEGIN_, 0x65, 0x02, _END_, //FW_CGOUT_L[7]
    _BEGIN_, 0x66, 0x11, _END_, //FW_CGOUT_L[8] CK11
    _BEGIN_, 0x67, 0x10, _END_, //FW_CGOUT_L[9] CK9
    _BEGIN_, 0x68, 0x02, _END_, //FW_CGOUT_L[10]
    _BEGIN_, 0x69, 0x0F, _END_, //FW_CGOUT_L[11] CK7
    _BEGIN_, 0x6a, 0x0E, _END_, //FW_CGOUT_L[12] CK5
    _BEGIN_, 0x6b, 0x02, _END_, //FW_CGOUT_L[13]
    _BEGIN_, 0x6c, 0x0D, _END_, //FW_CGOUT_L[14] CK3
    _BEGIN_, 0x6d, 0x0C, _END_, //FW_CGOUT_L[15] CK1
    _BEGIN_, 0x6e, 0x06, _END_, //FW_CGOUT_L[16] STV1_ODD
    _BEGIN_, 0x6f, 0x02, _END_, //FW_CGOUT_L[17]
    _BEGIN_, 0x70, 0x02, _END_, //FW_CGOUT_L[18]
    _BEGIN_, 0x71, 0x02, _END_, //FW_CGOUT_L[19]
    _BEGIN_, 0x72, 0x02, _END_, //FW_CGOUT_L[20]
    _BEGIN_, 0x73, 0x02, _END_, //FW_CGOUT_L[21]
    _BEGIN_, 0x74, 0x02, _END_, //FW_CGOUT_L[22]

    _BEGIN_, 0x75, 0x0A, _END_, //BW_CGOUT_L[1]   RESE_ODD
    _BEGIN_, 0x76, 0x02, _END_, //BW_CGOUT_L[2]   VSSG_ODD
    _BEGIN_, 0x77, 0x02, _END_, //BW_CGOUT_L[3]   VSSG_ODD
    _BEGIN_, 0x78, 0x06, _END_, //BW_CGOUT_L[4]   STV2_ODD
    _BEGIN_, 0x79, 0x15, _END_, //BW_CGOUT_L[5]   VDD2_ODD
    _BEGIN_, 0x7a, 0x14, _END_, //BW_CGOUT_L[6]   VDD1_ODD
    _BEGIN_, 0x7b, 0x02, _END_, //BW_CGOUT_L[7]
    _BEGIN_, 0x7c, 0x10, _END_, //BW_CGOUT_L[8]   CK11
    _BEGIN_, 0x7d, 0x11, _END_, //BW_CGOUT_L[9]   CK9
    _BEGIN_, 0x7e, 0x02, _END_, //BW_CGOUT_L[10]
    _BEGIN_, 0x7f, 0x0C, _END_, //BW_CGOUT_L[11]  CK7
    _BEGIN_, 0x80, 0x0D, _END_, //BW_CGOUT_L[12]  CK5
    _BEGIN_, 0x81, 0x02, _END_, //BW_CGOUT_L[13]
    _BEGIN_, 0x82, 0x0E, _END_, //BW_CGOUT_L[14]  CK3
    _BEGIN_, 0x83, 0x0F, _END_, //BW_CGOUT_L[15]  CK1
    _BEGIN_, 0x84, 0x08, _END_, //BW_CGOUT_L[16]  STV1_ODD
    _BEGIN_, 0x85, 0x02, _END_, //BW_CGOUT_L[17]
    _BEGIN_, 0x86, 0x02, _END_, //BW_CGOUT_L[18]
    _BEGIN_, 0x87, 0x02, _END_, //BW_CGOUT_L[19]
    _BEGIN_, 0x88, 0x02, _END_, //BW_CGOUT_L[20]
    _BEGIN_, 0x89, 0x02, _END_, //BW_CGOUT_L[21]
    _BEGIN_, 0x8A, 0x02, _END_, //BW_CGOUT_L[22]

    _BEGIN_, 0xFF, 0x98, 0x81, 0x04, _END_, //PAGE4
    // _BEGIN_, 0x00, 0x00, _END_, //3L  打开此行
    // _BEGIN_, 0x2F, 0x01, _END_, //BIST
    _BEGIN_, 0x3B, 0xC0, _END_, //ILI4003D sel
    _BEGIN_, 0x6C, 0x15, _END_,
    _BEGIN_, 0x6E, 0x30, _END_, //VGH 16V
    _BEGIN_, 0x6F, 0x55, _END_, //Pump ratio VGH=VSPX4 VGL=VSNX4
    _BEGIN_, 0x3A, 0x24, _END_,
    _BEGIN_, 0x8D, 0x1F, _END_,
    _BEGIN_, 0x87, 0xBA, _END_,
    _BEGIN_, 0x26, 0x76, _END_,
    _BEGIN_, 0xB2, 0xD1, _END_,
    _BEGIN_, 0xB5, 0x07, _END_,
    _BEGIN_, 0x35, 0x1F, _END_,
    _BEGIN_, 0x88, 0x0B, _END_,
    _BEGIN_, 0x21, 0x30, _END_,

    _BEGIN_, 0xFF, 0x98, 0x81, 0x01, _END_,//PAGE1
    _BEGIN_, 0x22, 0x0A, _END_,
    _BEGIN_, 0x31, 0x09, _END_,
    _BEGIN_, 0x40, 0x33, _END_,
    _BEGIN_, 0x53, 0x37, _END_,
    _BEGIN_, 0x55, 0x38, _END_,
    _BEGIN_, 0x50, 0x95, _END_,
    _BEGIN_, 0x51, 0x95, _END_,
    _BEGIN_, 0x60, 0x30, _END_,

    _BEGIN_, 0xA0, 0x0F, _END_, //VP255Gamma P
    _BEGIN_, 0xA1, 0x17, _END_, //VP251
    _BEGIN_, 0xA2, 0x22, _END_, //VP247
    _BEGIN_, 0xA3, 0x19, _END_, //VP243
    _BEGIN_, 0xA4, 0x15, _END_, //VP239
    _BEGIN_, 0xA5, 0x28, _END_, //VP231
    _BEGIN_, 0xA6, 0x1C, _END_, //VP219
    _BEGIN_, 0xA7, 0x1C, _END_, //VP203
    _BEGIN_, 0xA8, 0x78, _END_, //VP175
    _BEGIN_, 0xA9, 0x1C, _END_, //VP144
    _BEGIN_, 0xAA, 0x28, _END_, //VP111
    _BEGIN_, 0xAB, 0x69, _END_, //VP80
    _BEGIN_, 0xAC, 0x1A, _END_, //VP52
    _BEGIN_, 0xAD, 0x19, _END_, //VP36
    _BEGIN_, 0xAE, 0x4B, _END_, //VP24
    _BEGIN_, 0xAF, 0x22, _END_, //VP16
    _BEGIN_, 0xB0, 0x2A, _END_, //VP12
    _BEGIN_, 0xB1, 0x4B, _END_, //VP8
    _BEGIN_, 0xB2, 0x6B, _END_, //VP4
    _BEGIN_, 0xB3, 0x3F, _END_, //VP0

    _BEGIN_, 0xB7, 0x03, _END_, //mipi 2lane

    _BEGIN_, 0xC0, 0x01, _END_, //VN255 GAMMA N
    _BEGIN_, 0xC1, 0x17, _END_, //VN251
    _BEGIN_, 0xC2, 0x22, _END_, //VN247
    _BEGIN_, 0xC3, 0x19, _END_, //VN243
    _BEGIN_, 0xC4, 0x15, _END_, //VN239
    _BEGIN_, 0xC5, 0x28, _END_, //VN231
    _BEGIN_, 0xC6, 0x1C, _END_, //VN219
    _BEGIN_, 0xC7, 0x1D, _END_, //VN203
    _BEGIN_, 0xC8, 0x78, _END_, //VN175
    _BEGIN_, 0xC9, 0x1C, _END_, //VN144
    _BEGIN_, 0xCA, 0x28, _END_, //VN111
    _BEGIN_, 0xCB, 0x69, _END_, //VN80
    _BEGIN_, 0xCC, 0x1A, _END_, //VN52
    _BEGIN_, 0xCD, 0x19, _END_, //VN36
    _BEGIN_, 0xCE, 0x4B, _END_, //VN24
    _BEGIN_, 0xCF, 0x22, _END_, //VN16
    _BEGIN_, 0xD0, 0x2A, _END_, //VN12
    _BEGIN_, 0xD1, 0x4B, _END_, //VN8
    _BEGIN_, 0xD2, 0x6B, _END_, //VN4
    _BEGIN_, 0xD3, 0x3F, _END_, //VN0

    _BEGIN_, 0xFF, 0x98, 0x81, 0x00, _END_, //PAGE0
    // _BEGIN_, 0x35, 0x00, _END_, //TE OUT
    _BEGIN_, 0x11, 0x00, _END_, //sleep out
    _BEGIN_, REGFLAG_DELAY, 100, _END_,
    _BEGIN_, 0x29, 0x00, _END_, //display on
    _BEGIN_, REGFLAG_DELAY, 100, _END_,

};

static struct dbi_param mipi_ili9881c_800x1280_param = {
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

    /* .fps = 60, // psram@200MHz */
    .fps = 45, // psram@100MHz

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
        .hbp = 48,
        .hact = 800,
        .hfp = 52,

        .vsync = 6,
        .vbp = 16,
        .vact = 1280,
        .vfp = 15,
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


REGISTER_LCD_DEVICE(mipi_ili9881c_800x1280) = {
    .logo = "mipi_ili9881c_800x1280",

    .lcd_cmd = (void *) &lcd_mipi_ili9881c_800x1280_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_mipi_ili9881c_800x1280_cmd_list_poweron) / sizeof(lcd_mipi_ili9881c_800x1280_cmd_list_poweron[0]),
    .param  = (void *) &mipi_ili9881c_800x1280_param,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};



#endif

