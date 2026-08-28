/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
#if TCFG_LCD_MCU_D54E4PA7551B_XP26_400x240_ENABLE


/* #define LCD_DRIVE_CONFIG                    MCU_8BITS_RGB565 */
#define LCD_DRIVE_CONFIG                    MCU_8BITS_RGB666
/* #define LCD_DRIVE_CONFIG                    MCU_8BITS_RGB888 */

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "includes.h"
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
#define LCD_BLOCK_H 20
#define BUF_NUM     2

#define TELINE 0x0000

/*
8位接口
16bit PAP8BIT  RGB565 TWO_CYCLE   PORT_8BITS 8bitsx2
18bit PAP8BIT  RGB666 THREE_CYCLE PORT_6BITS 6bitsx3 DB[7:2]

9位接口
16bit RGB565 TWO_CYCLE 8bitsx2
18bit RGB666 TWO_CYCLE 9bitsx2

16位接口
16bit PAP16BIT 小端  RGB565 ONE_CYCLE 16bitsx1
16bit PAP16BIT 大端  RGB565 TWO_CYCLE 8bitsx2
18bit PAP16BIT 大端  RGB666 THREE_CYCLE 6bitsx3 DB[7:2]

18位接口
18bit PAP16BIT 小端  RGB666 ONE_CYCLE 18bitsx1
16bit PAP16BIT 大端  RGB565 TWO_CYCLE PORT_8BITS 8bitsx2
16bit PAP16BIT 小端  RGB565 ONE_CYCLE 16bitsx1


IM0 IM1
 L   L		18bit
 L   H		16bit
 H   L		9bit
 H   H		8bit


reg 00H = 0x0a
reg 01H = 0x04
reg 10H = 0xf0
reg 11H = 0x90


RGB565:       R7|R6|R5|R4|R3|G7|G6|G5|G4|G3|G2|B7|B6|B5|B4|B3
RGB666: R7|R6|R5|R4|R3|R2|G7|G6|G5|G4|G3|G2|B7|B6|B5|B4|B3|B2
*/

#define PORT08          (0<<0)
#define PORT09          (1<<0)
#define PORT16          (2<<0)
#define PORT18          (3<<0)
#define PORT_MASK        0x03

#define PIXEL16         (0<<2)
#define PIXEL18         (1<<2)
#define PIXEL_MASK       0x04

#define RGB565          (0<<3)
#define RGB666          (1<<3)
#define RGB_MASK         0x08

#define CYCLE1          (0<<4)
#define CYCLE2          (1<<4)
#define CYCLE3          (2<<4)
#define CYCLE_MASK       0x30


#define PORT08_PIXEL16_RGB565_CYCLE2    (PORT08|PIXEL16|RGB565|CYCLE2)
#define PORT08_PIXEL18_RGB666_CYCLE3    (PORT08|PIXEL18|RGB666|CYCLE3)

#define PORT09_PIXEL16_RGB565_CYCLE2    (PORT09|PIXEL16|RGB565|CYCLE2)
#define PORT09_PIXEL18_RGB666_CYCLE2    (PORT09|PIXEL18|RGB666|CYCLE2)

#define PORT16_PIXEL16_RGB565_CYCLE1    (PORT16|PIXEL16|RGB565|CYCLE1)
#define PORT16_PIXEL16_RGB565_CYCLE2    (PORT16|PIXEL16|RGB565|CYCLE2)
#define PORT16_PIXEL18_RGB666_CYCLE3    (PORT16|PIXEL18|RGB666|CYCLE3)

#define PORT18_PIXEL18_RGB666_CYCLE1    (PORT18|PIXEL18|RGB666|CYCLE1)
#define PORT18_PIXEL16_RGB565_CYCLE1    (PORT18|PIXEL16|RGB565|CYCLE1)
#define PORT18_PIXEL16_RGB565_CYCLE2    (PORT18|PIXEL16|RGB565|CYCLE2)

#if OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB565
/* #define DISP_MODE  PORT08_PIXEL16_RGB565_CYCLE2 // 8位 */
#define DISP_MODE  PORT16_PIXEL16_RGB565_CYCLE1 // 16位
#elif OUT_FORMAT(LCD_DRIVE_CONFIG) == FORMAT_RGB666
#define DISP_MODE  PORT09_PIXEL18_RGB666_CYCLE2 // 9位
/* #define DISP_MODE  PORT18_PIXEL18_RGB666_CYCLE1 // 18位 */
#else // RGB888
#error "Current LCD Driver dont supprt RGB888!!!"
#endif

#define DIRECTION_HORI  0//400x240
#define DIRECTION_VERT  1//240x400
#define DIRECTION       DIRECTION_HORI

static const u8 lcd_mcu_d54e4pa7551b_xp26_cmdlist[] ALIGNED(4) = {
    _BEGIN_, 0x68, 0x07, _END_,
    _BEGIN_, 0x69, 0x80, _END_,

    //8bit
#if (DISP_MODE == PORT08_PIXEL16_RGB565_CYCLE2)
    _BEGIN_, 0x01, 0x02, _END_,  //8bit+8bit 2 times
#elif (DISP_MODE == PORT08_PIXEL18_RGB666_CYCLE3)
    _BEGIN_, 0x01, 0x04, _END_,  //6bit+6bit+6bit 3 times DB[7..2]
#elif (DISP_MODE == PORT09_PIXEL16_RGB565_CYCLE2)
    //9bit
    _BEGIN_, 0x01, 0x02, _END_,  //8bit+8bit 2 times
#elif (DISP_MODE == PORT09_PIXEL18_RGB666_CYCLE2)
    _BEGIN_, 0x01, 0x00, _END_,  //9bit+9bit 2 times
#elif (DISP_MODE == PORT16_PIXEL16_RGB565_CYCLE1)
    //16bit
    _BEGIN_, 0x01, 0x02, _END_,  //16bit 1 times
#elif (DISP_MODE == PORT16_PIXEL16_RGB565_CYCLE2)
    _BEGIN_, 0x01, 0x03, _END_,  //8bit+8bit 2 times
#elif (DISP_MODE == PORT16_PIXEL18_RGB666_CYCLE3)
    _BEGIN_, 0x01, 0x05, _END_,  //6bit+6bit+6bit 3 times DB[7..2]
#elif (DISP_MODE == PORT18_PIXEL18_RGB666_CYCLE1)
    //18bit
    _BEGIN_, 0x01, 0x00, _END_,  //18bit 1 times
#elif (DISP_MODE == PORT18_PIXEL16_RGB565_CYCLE2)
    _BEGIN_, 0x01, 0x02, _END_,  //8bit+8bit 2times
#elif (DISP_MODE == PORT18_PIXEL16_RGB565_CYCLE1)
    _BEGIN_, 0x01, 0x03, _END_,  //16bit 1 times
#endif

    _BEGIN_, 0x08, 0x00, _END_,
    _BEGIN_, 0x10, 0xF0, _END_,
    _BEGIN_, 0x11, 0x90, _END_,
    _BEGIN_, 0x12, 0x01, _END_,
    _BEGIN_, 0x1A, 0xef, _END_,
    _BEGIN_, 0x1B, 0x00, _END_,
    _BEGIN_, 0x1C, 0x00, _END_,

    /*
    screen
    1|2|3
    4|5|6
    7|8|9
    */
#if (DIRECTION == DIRECTION_VERT)
    _BEGIN_, 0x1D, 0x04, _END_,  //3->2->1->6->5->4->9->8->7
    //_BEGIN_, 0x1D,0x00, _END_, //1->2->3->4->5->6->7->8->9
    //_BEGIN_, 0x1D,0x02, _END_, //7->8->9->4->5->6->1->2->3
    //_BEGIN_, 0x1D,0x06, _END_, //9->8->7->6->5->4->3->2->1
#elif (DIRECTION == DIRECTION_HORI)
    //_BEGIN_, 0x1D,0x01, _END_, //1->4->7->2->5->8->3->6->9
    //_BEGIN_, 0x1D,0x05, _END_, //3->6->9->2->5->8->1->4->7
    _BEGIN_, 0x1D, 0x03, _END_,  //7->4->1->8->5->2->9->6->3
    //_BEGIN_, 0x1D,0x07, _END_, //9->6->3->8->5->2->7->4->1
#endif
    _BEGIN_, 0x1E, 0x50, _END_,
    _BEGIN_, 0x30, 0x81, _END_,  //_BEGIN_, 0x30,0x80, _END_,
    _BEGIN_, 0x33, 0x02, _END_,
    _BEGIN_, 0x42, 0x07, _END_,
    _BEGIN_, 0x43, 0x01, _END_,
    _BEGIN_, 0x44, 0x04, _END_,
    _BEGIN_, 0x45, 0x08, _END_,
    _BEGIN_, 0x46, 0x06, _END_,
    _BEGIN_, 0x48, 0x01, _END_,
    _BEGIN_, 0x4A, 0x01, _END_,
    _BEGIN_, 0x4B, 0xA8, _END_,
    _BEGIN_, 0x4C, 0x07, _END_,
    _BEGIN_, 0x5A, 0x00, _END_,
    _BEGIN_, 0x62, 0x21, _END_,
    _BEGIN_, 0x65, 0x02, _END_,
    _BEGIN_, 0x66, 0x00, _END_,
    _BEGIN_, 0x6A, 0x02, _END_,
    _BEGIN_, 0x81, 0x44, _END_,
    _BEGIN_, 0x82, 0x15, _END_,
    _BEGIN_, 0x83, 0x25, _END_,
    _BEGIN_, 0x84, 0x10, _END_,
    _BEGIN_, 0x85, 0x25, _END_,
    _BEGIN_, 0x86, 0x17, _END_,
    _BEGIN_, 0x90, 0x25, _END_,
    _BEGIN_, 0x91, 0x04, _END_,
    _BEGIN_, 0x92, 0x15, _END_,
    _BEGIN_, 0x31, 0x01, _END_,
    _BEGIN_, 0x3E, 0x01, _END_,
};


static void delay_2ms(int cnt)
{
    if (cnt * 2 > 10) {
        os_time_dly(cnt * 2 / 10 + 1);
    } else {
        mdelay(2 * cnt);
    }
}

static void lcd_mcu_d54e4pa7551b_xp26_set_draw_area(void (*lcd_write_cmd_cb)(u32 cmd, u8 *buf, u32 len), int xstart, int xend, int ystart, int yend)
{
    u8 param_buf[1];

#if (DIRECTION == DIRECTION_VERT)
    param_buf[0] = xstart;
    lcd_write_cmd_cb(0x13, param_buf, 1);

    param_buf[0] = xend;
    lcd_write_cmd_cb(0x16, param_buf, 1);

    param_buf[0] = ystart;
    lcd_write_cmd_cb(0x14, param_buf, 1);

    param_buf[0] = ystart >> 8;
    lcd_write_cmd_cb(0x15, param_buf, 1);

    param_buf[0] = yend;
    lcd_write_cmd_cb(0x17, param_buf, 1);

    param_buf[0] = yend >> 8;
    lcd_write_cmd_cb(0x18, param_buf, 1);
#else
    param_buf[0] = xend;
    lcd_write_cmd_cb(0x13, param_buf, 1);

    param_buf[0] = xstart;
    lcd_write_cmd_cb(0x16, param_buf, 1);

    param_buf[0] = yend;
    lcd_write_cmd_cb(0x14, param_buf, 1);

    param_buf[0] = yend >> 8;
    lcd_write_cmd_cb(0x15, param_buf, 1);

    param_buf[0] = ystart;
    lcd_write_cmd_cb(0x17, param_buf, 1);

    param_buf[0] = ystart >> 8;
    lcd_write_cmd_cb(0x18, param_buf, 1);
#endif

    param_buf[0] = 0x01;
    lcd_write_cmd_cb(0x19, param_buf, 1);
}

struct dbi_param lcd_mcu_d54e4pa7551b_xp26_param = {
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
        .ramwr_cmd = 0x03,
        .ramwrc_cmd = 0x03,
        .custom_set_draw_area = lcd_mcu_d54e4pa7551b_xp26_set_draw_area,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_mcu_d54e4pa7551b_xp26_backlight_ctrl(u8 onoff)
{
    return 0;
}


/*
** 设置lcd进入睡眠
*/
static void lcd_mcu_d54e4pa7551b_xp26_entersleep(void)
{
    //TODO
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(120 / 2);	// delay 120ms
}

/*
** 设置lcd退出睡眠
*/
static void lcd_mcu_d54e4pa7551b_xp26_exitsleep(void)
{
    //TODO
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
}




REGISTER_LCD_DEVICE(d54e4pa7551b_xp26) = {
    .logo = "d54e4pa7551b_xp26",
    .row_addr_align 	= 1,
    .column_addr_align	= 1,

    .lcd_cmd = (void *) &lcd_mcu_d54e4pa7551b_xp26_cmdlist,
    .cmd_cnt = sizeof(lcd_mcu_d54e4pa7551b_xp26_cmdlist) / sizeof(lcd_mcu_d54e4pa7551b_xp26_cmdlist[0]),
    .param   = (void *) &lcd_mcu_d54e4pa7551b_xp26_param,

    .reset 			= NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,//lcd_mcu_d54e4pa7551b_xp26_backlight_ctrl,
    .entersleep 	= lcd_mcu_d54e4pa7551b_xp26_entersleep,
    .exitsleep 		= lcd_mcu_d54e4pa7551b_xp26_exitsleep,
};



#endif




