#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_mipi_co5300_466x466.data.bss")
#pragma data_seg(".lcd_mipi_co5300_466x466.data")
#pragma const_seg(".lcd_mipi_co5300_466x466.text.const")
#pragma code_seg(".lcd_mipi_co5300_466x466.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
** 注意：rgb屏需要一直推才能正常显示
*/
#if TCFG_LCD_MIPI_CO5300_466x466_ENABLE


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

#define SCR_X       6
#define SCR_Y       0
#define SCR_W       LCD_WIDTH
#define SCR_H       LCD_HEIGHT
#define LCD_W       LCD_WIDTH
#define LCD_H       LCD_HEIGHT
#define LCD_BLOCK_W LCD_WIDTH
#define LCD_BLOCK_H 16
#define BUF_NUM     2

#define MIXED_PACKET_TYPE 0 /* 混合包类型,每条指令可以指定不同的包类型 */

#if MIXED_PACKET_TYPE == 0
static const u8 lcd_mipi_co5300_466x466_cmd_list_poweron[] ALIGNED(4) = {
    //======================== CMD1 setting ============================
    _BEGIN_, 0xFE, 0x00, _END_,
    _BEGIN_, 0xC2, 0x03, _END_,                     //VIDEO MODE
    _BEGIN_, 0x35, 0x00, _END_,                     //TE ON
    /* _BEGIN_, 0x53, 0x20, _END_, */
    /* _BEGIN_, 0x51, 0x00, _END_,                     //Write Display Brightness  MAX_VAL=0XFF */
    _BEGIN_, 0x63, 0xFF, _END_,
    _BEGIN_, 0x2A, 0x00, 0x06, 0x01, 0xD7, _END_,
    _BEGIN_, 0x2B, 0x00, 0x00, 0x01, 0xD1, _END_,
    _BEGIN_, 0x3A, 0x77, _END_,                     //Interface Pixel Format   24bit/pixel
    _BEGIN_, 0x51, 0xFF, _END_,                     //Write Display Brightness  MAX_VAL=0XFF
    _BEGIN_, 0x11, _END_,                           //Sleep out
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,                           //Display on

};
#else
/*
 * 如何判断使用哪种包类型, 主要跟驱动IC有关, 有些驱动IC所有类型都会兼容, 遇到不兼容的就需要按照以下范围来区分不同的包类型
 *
 * 寄存器范围  说明
 * 0x00–0x0F   复位 + 读状态/ID（标准查询）
 * 0x10–0x3F   核心控制区：睡眠、显示ON/OFF、GRAM窗口、MADCTL、像素格式、Gamma、滚动、TE
 * 0x40–0x4F   TE/扫描线相关
 * 0x50–0x5F   亮度/CABC/省电
 * 0x60–0x9F   大部分未分配/厂商自定义,一般走Generic
 * 0xA0–0xAF   DDB 读取（标准）
 * 0xB0–0xFF   厂商私有命令空间,走Generic，不走DCS
*/
static const u8 lcd_mipi_co5300_466x466_cmd_list_poweron[] ALIGNED(4) = {
    //======================== CMD1 setting ============================
    _BEGIN_, PACKET_GENERIC_BTA, 0xFE, 0x00, _END_,
    _BEGIN_, PACKET_GENERIC_BTA, 0xC2, 0x03, _END_,                     //VIDEO MODE
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x35, 0x00, _END_,                     //TE ON
    /* _BEGIN_, PACKET_DCS_WITH_BTA, 0x53, 0x20, _END_, */
    /* _BEGIN_, PACKET_DCS_WITH_BTA, 0x51, 0x00, _END_,                     //Write Display Brightness  MAX_VAL=0XFF */
    _BEGIN_, PACKET_GENERIC_BTA, 0x63, 0xFF, _END_,
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x2A, 0x00, 0x06, 0x01, 0xD7, _END_,
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x2B, 0x00, 0x00, 0x01, 0xD1, _END_,
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x3A, 0x77, _END_,                     //Interface Pixel Format   24bit/pixel
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x51, 0xFF, _END_,                     //Write Display Brightness  MAX_VAL=0XFF
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x11, _END_,                           //Sleep out
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, PACKET_DCS_WITH_BTA, 0x29, _END_,                           //Display on
    _BEGIN_, REGFLAG_DELAY, 10, _END_,
};
#endif

static struct dbi_param lcd_mipi_co5300_466x466 = {
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

    .mipi = {
        .frame_timeout_ms = 33,
#if MIXED_PACKET_TYPE == 0
        .packet_category = BOTH_DCS_PACKET_AND_GENERIC_PACKET_AUTO,
#else
        .packet_category = BOTH_DCS_PACKET_AND_GENERIC_PACKET_MANUAL,
#endif
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

        .hsync = 32,
        .hbp = 40,
        .hact = 466,
        .hfp = 10,

        .vsync = 4,
        .vbp = 12,
        .vact = 466,
        .vfp = 14,
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


REGISTER_LCD_DEVICE(mipi_co5300_466x466) = {
    .logo = "mipi_co5300_466x466",

    .lcd_cmd = (void *) &lcd_mipi_co5300_466x466_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_mipi_co5300_466x466_cmd_list_poweron) / sizeof(lcd_mipi_co5300_466x466_cmd_list_poweron[0]),
    .param  = (void *) &lcd_mipi_co5300_466x466,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};

#endif

