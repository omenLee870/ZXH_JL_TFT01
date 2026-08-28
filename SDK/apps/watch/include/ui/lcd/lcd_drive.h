#ifndef __LCD_DRIVER_H__
#define __LCD_DRIVER_H__

#include "os/os_type.h"
#include "dbi.h"
#include "asm/eva_sfr.h"

#ifdef Reset
#undef Reset
#endif

#ifndef SPI_LCD_DEBUG_ENABLE
#define SPI_LCD_DEBUG_ENABLE	0
#endif

#if (SPI_LCD_DEBUG_ENABLE == 0)
#define lcd_d(...)
#define lcd_w(...)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#elif (SPI_LCD_DEBUG_ENABLE == 1)
#define lcd_d(...)
#define lcd_w(fmt, ...)	printf("[LCD WARNING]: "fmt, ##__VA_ARGS__)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#else
#define lcd_d(fmt, ...)	printf("[LCD DEBUG]: "fmt, ##__VA_ARGS__)
#define lcd_w(fmt, ...)	printf("[LCD WARNING]: "fmt, ##__VA_ARGS__)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#endif


// 注意：以下配置组合为固定搭配，不可随意更改
// 切换配置使用上面的宏选择
#if defined(CONFIG_CPU_BR33)
#include "app_config.h"

// 兼容旧的命名
#define OUTPUT_FORMAT_RGB565 DBI_INPUT_FORMAT_RGB565
#define OUTPUT_FORMAT_RGB888 DBI_INPUT_FORMAT_RGB888

#define SPI_SUBMODE(config)             (((config)>>16)&0xf0)
#define SPI_WIRE(config)                (((config)>>16)&0x0f)
#define PIXEL_nPnT(config)              (((config))&0xe0)
#define PIXEL_nTnB(config)              (((config))&0x1f)
#define SPI_IF_MODE(config)             (((config)>>16)&0xff)
#define OUT_FORMAT(config)              (((config)>>8)&0xff)
#define OUT_FORMAT_CC(config)           (((config)>>12)&0xf)
#define OUT_FORMAT_CS(config)           (((config)>>8)&0xf)
#define PIXEL_TYPE(config)              (((config))&0xff)
#define LCD_CONFIG(mode, format, type)  (((mode)<<16) | ((format)<<8) | (type))
/////////////////////////////////////////////spi 3wire///////////////////////////////////////////////
#define SPI_3WIRE_RGB565_1T16B          LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T16B)
#define SPI_3WIRE_RGB565_1T8B           LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T8B)
#define SPI_3WIRE_RGB666_1T18B          LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T18B)
#define SPI_3WIRE_RGB666_1T9B           LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB666, PIXEL_1P2T|PIXEL_1T9B)
#define SPI_3WIRE_RGB666_1T6B           LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T6B)
#define SPI_3WIRE_RGB888_1T24B          LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T24B)
#define SPI_3WIRE_RGB888_1T12B          LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB888, PIXEL_1P2T|PIXEL_1T12B)
#define SPI_3WIRE_RGB888_1T8B           LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T8B)
/////////////////////////////////////////////spi 4wire///////////////////////////////////////////////
#define SPI_4WIRE_RGB565_1T16B          LCD_CONFIG(SPI_MODE|SPI_WIRE4, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T16B)
#define SPI_4WIRE_RGB666_1T18B          LCD_CONFIG(SPI_MODE|SPI_WIRE4, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T18B)
#define SPI_4WIRE_RGB888_1T24B          LCD_CONFIG(SPI_MODE|SPI_WIRE4, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T24B)
////////////////////////////////////////////dspi submode0///////////////////////////////////////////////
#define DSPI_SUBMODE0_RGB565_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE0, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T8B)
#define	DSPI_SUBMODE0_RGB666_1T9B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE0, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T9B)
#define DSPI_SUBMODE0_RGB666_1T6B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE0, FORMAT_RGB666, PIXEL_2P3T|PIXEL_1T6B)
#define DSPI_SUBMODE0_RGB888_1T12B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE0, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T12B)
#define	DSPI_SUBMODE0_RGB888_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE0, FORMAT_RGB888, PIXEL_2P3T|PIXEL_1T8B)
// 兼容旧的命名
#define DSPI_3WIRE_RGB565_1T8B          DSPI_SUBMODE0_RGB565_1T8B
#define	DSPI_3WIRE_RGB666_1T9B          DSPI_SUBMODE0_RGB666_1T9B
#define DSPI_3WIRE_RGB666_1T6B          DSPI_SUBMODE0_RGB666_1T6B
#define DSPI_3WIRE_RGB888_1T12B         DSPI_SUBMODE0_RGB888_1T12B
#define	DSPI_3WIRE_RGB888_1T8B          DSPI_SUBMODE0_RGB888_1T8B
////////////////////////////////////////////dspi submode1///////////////////////////////////////////////
#define	DSPI_SUBMODE1_RGB565_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE1, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T8B)
#define DSPI_SUBMODE1_RGB666_1T9B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE1, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T9B)
#define DSPI_SUBMODE1_RGB666_1T6B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE1, FORMAT_RGB666, PIXEL_2P3T|PIXEL_1T6B)
#define DSPI_SUBMODE1_RGB888_1T12B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE1, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T12B)
#define DSPI_SUBMODE1_RGB888_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE1, FORMAT_RGB888, PIXEL_2P3T|PIXEL_1T8B)
// 兼容旧的命名
#define	DSPI_4WIRE_RGB565_1T8B          DSPI_SUBMODE1_RGB565_1T8B
#define DSPI_4WIRE_RGB666_1T9B          DSPI_SUBMODE1_RGB666_1T9B
#define DSPI_4WIRE_RGB666_1T6B          DSPI_SUBMODE1_RGB666_1T6B
#define DSPI_4WIRE_RGB888_1T12B         DSPI_SUBMODE1_RGB888_1T12B
#define DSPI_4WIRE_RGB888_1T8B          DSPI_SUBMODE1_RGB888_1T8B
////////////////////////////////////////////dspi submode2///////////////////////////////////////////////
#define	DSPI_SUBMODE2_RGB565_1T16B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T16B)
#define	DSPI_SUBMODE2_RGB565_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB565, PIXEL_1P1T|PIXEL_1T8B)
#define DSPI_SUBMODE2_RGB666_1T18B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T18B)
#define DSPI_SUBMODE2_RGB666_1T9B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB666, PIXEL_1P1T|PIXEL_1T9B)
#define DSPI_SUBMODE2_RGB666_1T6B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB666, PIXEL_2P3T|PIXEL_1T6B)
#define DSPI_SUBMODE2_RGB888_1T24B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T24B)
#define DSPI_SUBMODE2_RGB888_1T12B      LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB888, PIXEL_1P1T|PIXEL_1T12B)
#define DSPI_SUBMODE2_RGB888_1T8B       LCD_CONFIG(DSPI_MODE|DSPI_SUBMODE2, FORMAT_RGB888, PIXEL_2P3T|PIXEL_1T8B)
//////////////////////////////////////////////////qspi//////////////////////////////////////////////////
// QSPI SDR/DDR Timing
#define QSPI_RGB565_A1D1_1T8B       LCD_CONFIG(QSPI_MODE|QSPI_A1D1, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T8B)
#define QSPI_RGB666_A1D1_1T8B       LCD_CONFIG(QSPI_MODE|QSPI_A1D1, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T8B)
#define QSPI_RGB888_A1D1_1T8B       LCD_CONFIG(QSPI_MODE|QSPI_A1D1, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T8B)
#define QSPI_RGB565_A1D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A1D2, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T4B)
#define QSPI_RGB666_A1D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A1D2, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T4B)
#define QSPI_RGB888_A1D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A1D2, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T4B)
#define QSPI_RGB565_A2D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A2D2, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T4B)
#define QSPI_RGB666_A2D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A2D2, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T4B)
#define QSPI_RGB888_A2D2_1T4B       LCD_CONFIG(QSPI_MODE|QSPI_A2D2, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T4B)
#define QSPI_RGB565_A1D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A1D4, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T2B)
#define QSPI_RGB666_A1D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A1D4, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB888_A1D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A1D4, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB565_A4D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A4D4, FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T2B)
#define QSPI_RGB666_A4D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A4D4, FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB888_A4D4_1T2B       LCD_CONFIG(QSPI_MODE|QSPI_A4D4, FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T2B)
// 兼容旧的命名
#define QSPI_RGB565_SUBMODE0_1T8B       QSPI_RGB565_A1D1_1T8B
#define QSPI_RGB666_SUBMODE0_1T8B       QSPI_RGB666_A1D1_1T8B
#define QSPI_RGB888_SUBMODE0_1T8B       QSPI_RGB888_A1D1_1T8B
#define QSPI_RGB565_SUBMODE1_1T2B       QSPI_RGB565_A1D4_1T2B
#define QSPI_RGB666_SUBMODE1_1T2B       QSPI_RGB666_A1D4_1T2B
#define QSPI_RGB888_SUBMODE1_1T2B       QSPI_RGB888_A1D4_1T2B
#define QSPI_RGB565_SUBMODE2_1T2B       QSPI_RGB565_A4D4_1T2B
#define QSPI_RGB666_SUBMODE2_1T2B       QSPI_RGB666_A4D4_1T2B
#define QSPI_RGB888_SUBMODE2_1T2B       QSPI_RGB888_A4D4_1T2B

// QSPI RAMLESS Timing
#define QSPI_RGB565_RAMLESS_1T2B        LCD_CONFIG(QSPI_MODE|QSPI_RAMLESS,  FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T2B)
#define QSPI_RGB666_RAMLESS_1T2B        LCD_CONFIG(QSPI_MODE|QSPI_RAMLESS,  FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB888_RAMLESS_1T2B        LCD_CONFIG(QSPI_MODE|QSPI_RAMLESS,  FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T2B)
// QSPI FT2388 Timing
#define QSPI_RGB565_FT2388_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_FT2388,  FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T2B)
#define QSPI_RGB666_FT2388_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_FT2388,  FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB888_FT2388_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_FT2388,  FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T2B)
// QSPI SD3302 Timing
#define QSPI_RGB565_SD3302_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_SD3302,  FORMAT_RGB565, PIXEL_1P2T|PIXEL_1T2B)
#define QSPI_RGB666_SD3302_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_SD3302,  FORMAT_RGB666, PIXEL_1P3T|PIXEL_1T2B)
#define QSPI_RGB888_SD3302_1T2B         LCD_CONFIG(QSPI_MODE|QSPI_SD3302,  FORMAT_RGB888, PIXEL_1P3T|PIXEL_1T2B)
//////////////////////////////////////////////////RGB//////////////////////////////////////////////////
#define RGB_RGB565                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB565, 0)
#define RGB_RGB666                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB666, 0)
#define RGB_RGB888                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_RGB888, 0)
#define RGB_YUV444                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_YUV444, 0)
#define RGB_YUV422                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_YUV422, 0)
#define RGB_BT656                       LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_BT656, 0)
#define RGB_BT1120                      LCD_CONFIG(SPI_MODE|SPI_WIRE3, FORMAT_BT1120, 0)
//////////////////////////////////////////////////MCU//////////////////////////////////////////////////
#define MCU_RGB565                      LCD_CONFIG(0, FORMAT_RGB565, 0)
#define MCU_RGB666                      LCD_CONFIG(0, FORMAT_RGB666, 0)
#define MCU_RGB888                      LCD_CONFIG(0, FORMAT_RGB888, 0)
//////////////////////////////////////////////////MIPI//////////////////////////////////////////////////
#define MIPI_RGB565_DCS                 LCD_CONFIG(0, (DSI_CC_RGB565 << 4) | DSI_CS_RGB565_DCS, 0)
#define MIPI_RGB565_VDO                 LCD_CONFIG(0, (DSI_CC_RGB565 << 4) | DSI_CS_RGB565_VDM, 0)
#define MIPI_RGB666                     LCD_CONFIG(0, (DSI_CC_RGB666 << 4) | DSI_CS_RGB666, 0)
#define MIPI_RGB666_LOOSELY             LCD_CONFIG(0, (DSI_CC_RGB666_LOOSELY << 4) | DSI_CS_RGB666_LOOSELY, 0)
#define MIPI_RGB888                     LCD_CONFIG(0, (DSI_CC_RGB888 << 4) | DSI_CS_RGB888, 0)
#endif // #if defined(CONFIG_CPU_BR33)
//////////////////////////////lcd mode end///////////////////////////
//~~~~~~~~~~~~~~~~~~~~~~~屏驱相关的参数和结构体~~~~~~~~~~~~~~~~~~~~~~~~~~//
// 屏幕初始化代码延时标志

#ifndef REGFLAG_DELAY
#define REGFLAG_DELAY_FLAG  0xff5aa5ff
#define REGFLAG_DELAY       ((REGFLAG_DELAY_FLAG>>24)&0xff),((REGFLAG_DELAY_FLAG>>16)&0xff),((REGFLAG_DELAY_FLAG>>8)&0xff),(REGFLAG_DELAY_FLAG&0xff)
#endif

#ifndef REGFLAG_CONFIRM
#define REGFLAG_CONFIRM_FLAG  0xff5bb5ff
#define REGFLAG_CONFIRM       ((REGFLAG_CONFIRM_FLAG>>24)&0xff),((REGFLAG_CONFIRM_FLAG>>16)&0xff),((REGFLAG_CONFIRM_FLAG>>8)&0xff),(REGFLAG_CONFIRM_FLAG&0xff)
#endif


// 区分屏幕初始化代码开始和结束的标志
#define BEGIN_FLAG          0x12345678
#define END_FLAG            0x87654321
#define _BEGIN_             ((BEGIN_FLAG>>24)&0xff),((BEGIN_FLAG>>16)&0xff),((BEGIN_FLAG>>8)&0xff),(BEGIN_FLAG&0xff)
#define _END_               ((END_FLAG>>24)&0xff),((END_FLAG>>16)&0xff),((END_FLAG>>8)&0xff),(END_FLAG&0xff)


struct lcd_drive {
    char *logo; //屏驱logo

    u8 column_addr_align; //列对齐(默认为1)
    u8 row_addr_align;    //行对齐(默认为1)

    u8 *lcd_cmd; // 初始化命令列表
    int cmd_cnt;

    // 显示形状描述
    u16 radius; // 圆角半径
    u32 fill_argb; // 不显示区域填充色

    // 配置参数
    void *param;

    // 应用层函数
    void (*reset)(void); /* 复位函数 */
    int (*backlight_ctrl)(u8);
    int (*power_ctrl)(u8);
    void (*entersleep)(void);
    void (*exitsleep)(void);
    u32(*read_id)(void);

    u32 lcd_id; //屏幕id
};

#define REGISTER_LCD_DEVICE(lcd) \
	const struct lcd_drive lcd sec(.lcd_device_info) __attribute__((used))

extern struct lcd_drive lcd_device_begin[];
extern struct lcd_drive lcd_device_end[];



struct lcd_platform_data {
    int  pin_reset;
    int  pin_en;
    int  pin_en_ex;
    int  pin_bl;
    int  pin_te;
    void (*board_pre_init)(void);  //板级相关初始化
    struct dbi_hw_param dbi_hw;
};


#ifndef KHz
#define KHz	(1000L)
#endif

// #ifndef MHz
// #define MHz	(10001000L)
// #endif

// enum LCD_COLOR {
//     LCD_COLOR_RGB888,
//     LCD_COLOR_RGB565,
//     LCD_COLOR_MONO,
// };
// 使用 dbi 模块定义的颜色格式
#define LCD_COLOR_RGB888	OUTPUT_FORMAT_RGB888
#define LCD_COLOR_RGB565	OUTPUT_FORMAT_RGB565

enum LCD_IF {
    LCD_SPI,
    LCD_MCU,
    LCD_RGB,
    LCD_EMI,
};

struct lcd_info {
    u16 width;
    u16 height;
    u16 stride;
    u16 radius;
    u32 fill_argb;
    u8 rotate;
    u8 fps;
    u8 color_format;
    u8 interface;
    u8 col_align;
    u8 row_align;
    u8 buf_num;
    u8 bl_status;
    u8 *buffer;
    int buffer_size;
};

struct lcd_interface {
    void (*init)(void *);
    void (*get_screen_info)(struct lcd_info *info);
    void (*buffer_malloc)(u8 **buf, u32 *size);
    void (*buffer_free)(u8 *buf);
    void (*draw)(u8 *buf, int xstart, int xend, int ystart, int yend);
    void (*draw_continue)(u8 *buf, int xstart, int xend, int ystart, int yend);
    void (*set_draw_area)(u16 xs, u16 xe, u16 ys, u16 ye);
    void (*clear_screen)(u32 color, int xstart, int xend, int ystart, int yend);
    int (*backlight_ctrl)(u8 on);
    int (*power_ctrl)(u8 on);
    void (*draw_page)(u8 *buf, u8 page_star, u8 page_len);//刷新页(点阵屏)
};

extern struct lcd_interface lcd_interface_begin[];
extern struct lcd_interface lcd_interface_end[];

#define REGISTER_LCD_INTERFACE(lcd) \
    static const struct lcd_interface lcd sec(.lcd_if_info) __attribute__((used))

struct lcd_interface *lcd_get_hdl();

#define LCD_SPI_PLATFORM_DATA_BEGIN(data) \
		const struct lcd_platform_data data

#define LCD_SPI_PLATFORM_DATA_END() \


/**
 * @brief lcd bl引脚输出控制
 *
 * @param val 电平输出值
 *
 */
void lcd_bl_ctrl(u8 val);


/**
 * @brief lcd en引脚输出控制
 *
 * @param val 电平输出值
 *
 */
void lcd_en_ctrl(u8 val);


/*
 * Description: 获取 LCD 背光状态
 * Returns    : 0 背光熄灭
 * 				1 背光点亮
 */
int lcd_backlight_status();


/*
 * Description: 获取 LCD sleep状态
 * Returns    : 0 sleep out
 * 				1 sleep in
 */
int lcd_sleep_status();


// int lcd_drv_backlight_ctrl(u8 percent);
// int lcd_drv_power_ctrl(u8 on);
int lcd_backlight_ctrl(u8 on);


/*
 * Description: 发送屏幕初始化指令
 * @param cmd_list 初始化命令地址
 * @param cmd_cnt 初始化命令长度
 */
void lcd_drv_cmd_list(u8 *cmd_list, int cmd_cnt);


/*
 * Description: LCD 设备初始化
 *
 * Arguments  : 匹配模式， 屏驱LOGO
 *
 * Returns    : 屏设备句柄
 *
 * Notes      : 1、当只有一个屏驱时，不进行匹配，直接返回该屏驱句柄
 *
 *              2、两种匹配方式 LOGO 或者 ID
 *
 *              3、若存在屏设备句柄时直接返回
 */
struct lcd_drive *lcd_drv_get_hdl(u8 mode, const char *logo);


/*
 * Description: LCD 设备初始化
 *
 * Arguments  : 匹配模式， 屏驱ID
 *
 * Returns    : 屏设备句柄
 */
struct lcd_drive *lcd_drv_get_hdl_by_id();


/*
 * Description: LCD 设备初始化
 *
 * Arguments  : 匹配模式， 屏驱LOGO
 *
 * Returns    : 屏设备句柄
 */
struct lcd_drive *lcd_drv_get_hdl_by_logo(const char *logo);


/*
 * Description: 获取 LCD 设备信息
 *
 * Arguments  : *info LCD 设备信息缓存结构体，根据结构体内容赋值即可
 *
 * Returns    : 0 获取成功
 */
int lcd_drv_get_info(void *info);


/*
 * Description: 获取向 LCD 设备发送一帧数据所需时间(us)
 *
 * Returns    : 发送一帧数据的时间
 */
extern u32 lcd_get_spi_frame_period_us(void);


/*
 * Description: 获取 LCD 设备刷新一行所需时间(us)
 *
 * Returns    : 刷新一行的时间
 */
extern u8 lcd_get_te_line_period_us(void);


/*
 * Description: 获取 LCD 设备刷新一帧所需时间(us)
 *
 * Returns    : 刷新一帧的时间
 */
extern u32 lcd_get_te_frame_period_us(void);


/*
 * Description: 获取 LCD 设备的宽度
 *
 * Returns    : LCD 宽度
 */
extern int lcd_get_screen_width();


/*
 * Description: 获取 LCD 设备的高度
 *
 * Returns    : LCD 高度
 */
extern int lcd_get_screen_height();


// 两毫秒延时
extern void delay_2ms(int cnt);

/*************************************************************************/
struct lcd_sleep_headler {
    char *name;
    void (*enter)(void);
    void (*exit)(void);
};

#define REGISTER_LCD_SLEEP_HEADLER(target) \
            const struct lcd_sleep_headler target sec(.lcd_sleep_headler)

extern const struct lcd_sleep_headler lcd_sleep_ctrl_headler_begin[];
extern const struct lcd_sleep_headler lcd_sleep_ctrl_headler_end[];

#define list_for_each_lcd_sleep_headler(p) \
    for (p = (struct lcd_sleep_headler *)lcd_sleep_ctrl_headler_begin; p < lcd_sleep_ctrl_headler_end; p++)
/*************************************************************************/

#endif

