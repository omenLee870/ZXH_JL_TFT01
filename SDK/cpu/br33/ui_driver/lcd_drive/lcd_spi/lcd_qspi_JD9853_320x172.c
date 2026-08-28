/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"
#include "ui/lcd/lcd_drive.h"
#include "ui/lcd/lcd_conf.h"
// #include "board_ac707n_demo/board_ac707n_demo_cfg.h"

/*
** 驱动代码的宏开关
*/
//<<<[4-fire屏 240x240]>>>//
#if (TCFG_SPI_LCD_ENABLE && TCFG_LCD_SPI_JD9853_320x172_ENABLE)

#define LCD_DRIVE_CONFIG					SPI_4WIRE_RGB565_1T8B


/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
** 包含imb头文件，imb输出格式定义在imb.h
*/
#include "includes.h"
#include "ui/ui_api.h"


#define SCR_X 0
#define SCR_Y 34
#define SCR_W LCD_WIDTH
#define SCR_H LCD_HEIGHT
#define LCD_W LCD_WIDTH
#define LCD_H LCD_HEIGHT
#define LCD_BLOCK_W 32//LCD_WIDTH
#define LCD_BLOCK_H 172//40
#define BUF_NUM 2
#define LCD_FORMAT OUTPUT_FORMAT_RGB565




/*
** 初始化代码
*/
#define MY 0     //Y反向（画面）
#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
#define MV 0    //90旋转（画面）
#define MX 1     //X反向（画面）
#else
#define MV 1     //90旋转（画面）
#define MX 1     //X反向（画面）
#endif
#define ML 0
#define BGR 0       //大小端颜色
#define MH 1
static const u8 lcd_qspi_jd9853_cmdlist[] ALIGNED(4) = {

    _BEGIN_, 0xDF, 0x98, 0x53, _END_,
    // _BEGIN_, 0xDE, 0x00, _END_,
    _BEGIN_, 0xB2, 0x23, _END_,
    _BEGIN_, 0xB7, 0x00, 0x47, 0x00, 0x6F, _END_, // 0x21 VGMP 4.3V ,0x49 VGSP 4.3V
    // _BEGIN_, 0x36, (MY<<7)|(MX<<6)|(MV<<5)|(ML<<4)|(BGR<<3)|(MH<<2), _END_,

    _BEGIN_, 0xBB, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0, _END_,

    _BEGIN_, 0xC0, 0x46, 0xA6,  _END_,
    _BEGIN_, 0xC1, 0x12, _END_,
    _BEGIN_, 0xC3, 0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77, _END_,
    _BEGIN_, 0xC4, 0x04, 0x00, 0xA0, 0x9E, 0x0A, 0xCA, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82, _END_, //第一个dat00=60Hz 01=53Hz 02=42Hz ，0xA0 LN=320  Line
    _BEGIN_, 0xC8, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, \
    0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, \
    0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, _END_,
    _BEGIN_, 0xD0, 0x04, 0x06, 0x6B, 0x0F, 0x00, _END_,
    _BEGIN_, 0xD7, 0x00, 0x30, _END_,
    _BEGIN_, 0xE6, 0x10, _END_,
    _BEGIN_, 0xDE, 0x01, _END_,
    _BEGIN_, 0xB7, 0x03, 0x13, 0xEF, 0x35, 0x35, _END_,
    _BEGIN_, 0xC1, 0x14, 0x15, 0xC0, _END_,
    _BEGIN_, 0xC2, 0x06, 0x3A, _END_,
    _BEGIN_, 0xC4, 0x72, 0x12, _END_,
    _BEGIN_, 0xBE, 0x00, _END_,
    _BEGIN_, 0xC5, 0x03, _END_,
    _BEGIN_, 0xBB, 0x05, 0x22, 0x22, 0x07, _END_,
    _BEGIN_, 0xDE, 0x02, _END_,
    _BEGIN_, 0xE5, 0x00, 0x02, 0x00, _END_,
    _BEGIN_, 0xE5, 0x01, 0x02, 0x00, _END_,
    _BEGIN_, 0xDE, 0x00, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0x3A, 0x05, _END_,//0x06=RGB666  0x05=RGB565
    _BEGIN_, 0x2A, 0x00, 0x00, 0x01, 0x3F, _END_, //0x00:Start_X=00   0xEF:End_X=240
    _BEGIN_, 0x2B, 0x00, 0x22, 0x00, 0xCD, _END_, //0x00:Start_Y=00   0x01,0x27:End_Y=296

#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
    _BEGIN_, 0x36, 0, _END_,
#else
    // _BEGIN_, 0x36, 0x64, _END_,
    _BEGIN_, 0x36, (MY << 7) | (MX << 6) | (MV << 5) | (ML << 4) | (BGR << 3) | (MH << 2), _END_,
#endif

    _BEGIN_, 0x11, _END_,                                              // exit sleep
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0xDE, 0x02, _END_,
    _BEGIN_, 0xE5, 0x00, 0x02, 0x00, _END_,
    _BEGIN_, 0xDE, 0x00, _END_,
    _BEGIN_, 0x29, _END_, /* Display ON (29h) */
    _BEGIN_, REGFLAG_DELAY, 1, _END_,

};


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
static int lcd_qspi_jd9853_power_ctrl(u8 onoff)
{
    printf("lcd_qspi_jd9853_power_ctrl %d\r\n", onoff);
    lcd_en_ctrl(onoff);
    return 0;
}

/*
** lcd背光控制
** 考虑到手表应用lcd背光控制需要更灵活自由，可能需要pwm调光，随时亮灭等
** 因此内部不操作lcd背光，全部由外部自行控制
*/
static int lcd_qspi_jd9853_backlight_ctrl(u8 onoff)
{
    lcd_bl_ctrl(onoff);
    return 0;
}

#define DEEP_STANDBY  1
/*
** 设置lcd进入睡眠
*/
static void lcd_qspi_jd9853_entersleep(void)
{
    printf("%s\n", __func__);
    lcd_write_cmd(0x28, NULL, 0);
    delay_2ms(60);
    lcd_write_cmd(0x10, NULL, 0);
    delay_2ms(60);
    struct lcd_platform_data *lcd_dat = lcd_get_platform_data();
    if (lcd_dat && lcd_dat->pin_te != NO_CONFIG_PORT) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_te), PORT_HIGHZ);
    }
    // if (lcd_dat->pin_cs != NO_CONFIG_PORT) {
    //     gpio_set_pull_up(lcd_dat->pin_cs, 0);
    //     gpio_set_pull_down(lcd_dat->pin_cs, 0);
    //     gpio_direction_input(lcd_dat->pin_cs);
    //     gpio_set_die(lcd_dat->pin_cs, 0);
    // }
    // if (lcd_dat->pin_reset != NO_CONFIG_PORT) {
    //     gpio_set_pull_up(lcd_dat->pin_reset, 0);
    //     gpio_set_pull_down(lcd_dat->pin_reset, 0);
    //     gpio_direction_input(lcd_dat->pin_reset);
    //     gpio_set_die(lcd_dat->pin_reset, 0);
    // }
    // if (lcd_dat->pin_bl != NO_CONFIG_PORT) {
    //     gpio_set_pull_up(lcd_dat->pin_bl, 0);
    //     gpio_set_pull_down(lcd_dat->pin_bl, 0);
    //     gpio_direction_input(lcd_dat->pin_bl);
    //     gpio_set_die(lcd_dat->pin_bl, 0);
    // }
}

/*
** 设置lcd退出睡眠
*/
static void lcd_qspi_jd9853_exitsleep(void)
{
#if DEEP_STANDBY
    struct lcd_platform_data *lcd_dat = (struct lcd_platform_data *)lcd_get_platform_data();
    if (lcd_dat && (lcd_dat->pin_reset != NO_CONFIG_PORT)) {
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_LOW);
        delay_2ms(4);	// delay >5ms
        gpio_set_mode(IO_PORT_SPILT(lcd_dat->pin_reset), PORT_OUTPUT_HIGH);
        delay_2ms(4);	// delay >5ms
    }
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(25);
    lcd_write_cmd(0x29, NULL, 0);
    delay_2ms(10);

    extern struct dbi_param lcd_qspi_jd9853_param;
    lcd_init(&lcd_qspi_jd9853_param);
    lcd_drv_cmd_list(lcd_qspi_jd9853_cmdlist, sizeof(lcd_qspi_jd9853_cmdlist) / sizeof(lcd_qspi_jd9853_cmdlist[0]));
#else
    lcd_write_cmd(0x11, NULL, 0);
    delay_2ms(5);	// delay 120ms
    lcd_write_cmd(0x29, NULL, 0);
#endif
}
//读取LCD驱动标识
static u32 lcd_qspi_jd9853_read_id()
{
    u8 id[3];
    lcd_read_cmd(0x04, id, sizeof(id));
    return (id[0] << 16 | id[1] << 8 | id[2]);
}
//0x6063992
// 配置参数
struct dbi_param lcd_qspi_jd9853_param = {

#if DRAW_X_DIV_EN && DRAW_BUF_ROTATE
    .scr_x    = 34,//SCR_X,
    .scr_y	  = 0,//SCR_Y,
    .scr_w	  = 320,//172,//320,// SCR_W,
    .scr_h	  = 172,//320,//172,//SCR_H,

    .in_width  = 320,//SCR_W,
    .in_height = 172,//SCR_H,
    .in_format = LCD_FORMAT,


    .lcd_width  = 172,//LCD_W,
    .lcd_height = 320,//LCD_H,
#else

    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,

    // imb配置相关
    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = LCD_FORMAT,    //OUTPUT_FORMAT_RGB565,//-1,

    // 屏幕相关
    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

#endif
    .lcd_type   = LCD_TYPE_SPI,

    .buffer_num  = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,
    .fps = 80,  //60,

    .spi = {
        .spi_mode	= SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .spi_dat_mode = SPI_MODE_UNIDIR,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0x00FF00,
};


REGISTER_LCD_DEVICE(jd9853) = {
    .logo = "jd9853",
    .row_addr_align		= 1,
    .column_addr_align	= 1,

    .radius		= SCR_W / 2,
    .fill_argb	= 0xffffffff,

    .lcd_cmd = (void *) &lcd_qspi_jd9853_cmdlist,
    .cmd_cnt = sizeof(lcd_qspi_jd9853_cmdlist) / sizeof(lcd_qspi_jd9853_cmdlist[0]),
    .param   = (void *) &lcd_qspi_jd9853_param,

    .reset			= NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,//lcd_qspi_jd9853_backlight_ctrl,
    .power_ctrl     = lcd_qspi_jd9853_power_ctrl,
    .entersleep     = lcd_qspi_jd9853_entersleep,
    .exitsleep      = lcd_qspi_jd9853_exitsleep,
    .read_id = lcd_qspi_jd9853_read_id,
    .lcd_id = 0x4c0000,
};



#endif
