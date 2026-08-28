#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_qspi_co5300_466x466.data.bss")
#pragma data_seg(".lcd_qspi_co5300_466x466.data")
#pragma const_seg(".lcd_qspi_co5300_466x466.text.const")
#pragma code_seg(".lcd_qspi_co5300_466x466.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
** 注意：rgb屏需要一直推才能正常显示
*/
#if TCFG_LCD_QSPI_CO5300_466x466_ENABLE


#define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE1_1T2B

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

static const u8 lcd_qspi_co5300_466x466_cmd_list_poweron[] ALIGNED(4) = {
    //======================== CMD1 setting ============================
    _BEGIN_, 0xFE, 0x00, _END_,
    _BEGIN_, 0xC4, 0x80, _END_,                     //QSPI setting, MIPI remove
    _BEGIN_, 0x3A, 0x55, _END_,                     //Interface Pixel Format   16bit/pixel
    _BEGIN_, 0x35, 0x00, _END_,                     //TE ON
    _BEGIN_, 0x53, 0x20, _END_,
    _BEGIN_, 0x51, 0x00, _END_,                     //Write Display Brightness  MAX_VAL=0XFF
    _BEGIN_, 0x63, 0xFF, _END_,
    /* _BEGIN_, 0x2A, 0x00, 0x06, 0x01, 0xD7, _END_, */
    /* _BEGIN_, 0x2B, 0x00, 0x00, 0x01, 0xD1, _END_, */
    _BEGIN_, 0x11, _END_,                           //Sleep out
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,                           //Display on
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x51, 0xFF, _END_,                     //Write Display Brightness  MAX_VAL=0XFF
};

static struct dbi_param lcd_qspi_co5300_466x466 = {
    .scr_x    = SCR_X,
    .scr_y    = SCR_Y,
    .scr_w    = SCR_W,
    .scr_h    = SCR_H,
    .in_width  = SCR_W,
    .in_height = SCR_H,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .in_format = DBI_INPUT_FORMAT_RGB565,
    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = 2,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 60,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
        .read_pin_select = READ_FROM_DBI_D0,
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


REGISTER_LCD_DEVICE(qspi_co5300_466x466) = {
    .logo = "qspi_co5300_466x466",
    .row_addr_align    = 2,
    .column_addr_align = 2,

    .lcd_cmd = (void *) &lcd_qspi_co5300_466x466_cmd_list_poweron,
    .cmd_cnt = sizeof(lcd_qspi_co5300_466x466_cmd_list_poweron) / sizeof(lcd_qspi_co5300_466x466_cmd_list_poweron[0]),
    .param  = (void *) &lcd_qspi_co5300_466x466,

    .reset          = NULL,// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = NULL,//lcd_entersleep,
    .exitsleep      = NULL,//lcd_exitsleep,
};



#endif

