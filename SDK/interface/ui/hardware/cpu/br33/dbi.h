/* Copyright(C)
 * not free
 * All right reserved
 *
 * @file dbi.h
 * @brief DBI模块驱动API头文件。DBI模块为专门用于推屏的独立硬件模块，与硬件SPI无关。其配置仅在屏幕驱动的结构体中，推屏时钟的频率为内部通过推屏帧率计算而得，但时钟是由PLL进行分频而来，因此实际时钟频率仅会在分频组合允许的前提下尽可能接近所需的目标频率(实际测量的推屏时钟与计算并非绝对一致)。
 * @author zhuhaifang@zh-jieli.com
 * @version V201
 * @date 2023-01-10
 */

#ifndef __DBI_H__
#define __DBI_H__

#include "generic/typedef.h"
#include "os/os_api.h"
#include "gpio.h"
#include "asm/eva_sfr.h"
#include "dsi.h"

/*
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|                                                           DBI各接口硬件功能脚描述                                                                          |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
| 接口 | 位宽 | LCD_DRIVE_CONFIG                                                                        | 功能脚                                             |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  1   | SPI_3WIRE                                                                               | DBI_CSX、DBI_SCL、DBI_D0                           |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
| SPI  |  1   | SPI_4WIRE(SPI_MODE_BIDIR)                                                               | DBI_CSX、DBI_DCX、DBI_SCL、DBI_D0、DBI_D1          |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  1   | SPI_4WIRE(SPI_MODE_UNIDIR)                                                              | DBI_CSX、DBI_DCX、DBI_SCL、DBI_D0                  |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  2   | DSPI_SUBMODE0                                                                           | DBI_CSX、DBI_SCL、DBI_D0、DBI_D1                   |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
| DSPI |  2   | DSPI_SUBMODE1                                                                           | DBI_CSX、DBI_SCL、DBI_D0、DBI_D1                   |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  2   | DSPI_SUBMODE2                                                                           | DBI_CSX、DBI_SCL、DBI_D0、DBI_D1、DBI_D2           |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  1   | QSPI_A1D1                                                                               | DBI_CSX、DBI_SCL、DBI_D0                           |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
| QSPI |  2   | QSPI_A1D2、QSPI_A2D2                                                                    | DBI_CSX、DBI_SCL、DBI_D0、DBI_D1                   |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  4   | QSPI_A1D4、QSPI_A4D4、QSPI_RAMLESS                                                      | DBI_CSX、DBI_SCL、DBI_D0、DBI_D1、DBI_D2、DBI_D3   |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  8   | FORMAT_RGB565、FORMAT_RGB888                                                            | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D7  |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  9   | FORMAT_RGB666                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D8  |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
| MCU  |  16  | FORMAT_RGB565                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D15 |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  18  | FORMAT_RGB666                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D17 |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  24  | FORMAT_RGB888                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D23 |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  6   | FORMAT_RGB666                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D5  |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  8   | FORMAT_RGB565、FORMAT_RGB888、FORMAT_YUV422、FORMAT_YUV444、FORMAT_BT656、FORMAT_BT1120 | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D7  |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  9   | FORMAT_RGB666                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D8  |
| RGB  +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  16  | FORMAT_RGB565、FORMAT_YUV422、FORMAT_BT1120                                             | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D15 |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  18  | FORMAT_RGB666                                                                           | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D17 |
|      +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  24  | FORMAT_RGB888、FORMAT_YUV444                                                            | DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX、DBI_D0~DBI_D23 |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  1   | RGB565_DCS、RGB565_VDO、RGB666、RGB666_LOOSELY、RGB888                                  | DSI_MAP_CK_LANE、DSI_MAP_D0_LANE                   |
| MIPI +------+-----------------------------------------------------------------------------------------+----------------------------------------------------+
|      |  2   | RGB565_DCS、RGB565_VDO、RGB666、RGB666_LOOSELY、RGB888                                  | DSI_MAP_CK_LANE、DSI_MAP_D0_LANE、DSI_MAP_D1_LANE  |
+------+------+-----------------------------------------------------------------------------------------+----------------------------------------------------+

+-------------+------+---------+---------+---------+---------+
|        DBI_CSX、DBI_DCX、DBI_SCL、DBI_RWX功能定义          |
+-------------+------+---------+---------+---------+---------+
|    接口     | 子类 | DBI_CSX | DBI_DCX | DBI_SCL | DBI_RWX |
+-------------+------+---------+---------+---------+---------+
|SPI/DSPI/QSPI| N/A  |   CS    |   DC    |   SCL   |   N/A   |
+-------------+------+---------+---------+---------+---------+
|             | 8080 |   CS    |   RS    |   WR    |   RD    |
|     MCU     +------+---------+---------+---------+---------+
|             | 6800 |   CS    |   RS    |   E     |   R/W   |
+-------------+------+---------+---------+---------+---------+
|     RGB     | N/A  |  VSYNC  | HSYNC   |   DCLK  |   DEN   |
+-------------+------+---------+---------+---------+---------+
*/

enum DBI_INPUT_FORMAT {
    /* 0 */DBI_INPUT_FORMAT_ARGB8888,
    /* 1 */DBI_INPUT_FORMAT_ARGB8565,
    /* 2 */DBI_INPUT_FORMAT_RGB888,
    /* 3 */DBI_INPUT_FORMAT_RGB565,
    /* 4 */DBI_INPUT_FORMAT_YUV422_BT601,
    /* 5 */DBI_INPUT_FORMAT_YUV422_BT709,
    /* 6 */DBI_INPUT_FORMAT_YUV444_BT601,
    /* 7 */DBI_INPUT_FORMAT_YUV444_BT709,
    /* 8 */DBI_INPUT_FORMAT_RAW8,
};


//[DBI硬件IO定义,支持硬件级映射,禁止自行修改]
typedef enum {
    DBI_PORTA_07 = IO_PORTA_07,
    DBI_PORTA_08 = IO_PORTA_08,
    DBI_PORTA_09 = IO_PORTA_09,
    DBI_PORTA_10 = IO_PORTA_10,
    DBI_PORTA_11 = IO_PORTA_11,
    DBI_PORTA_12 = IO_PORTA_12,

    DBI_PORTB_00 = IO_PORTB_00,
    DBI_PORTB_01 = IO_PORTB_01,
    DBI_PORTB_02 = IO_PORTB_02,
    DBI_PORTB_03 = IO_PORTB_03,
    DBI_PORTB_04 = IO_PORTB_04,
    DBI_PORTB_05 = IO_PORTB_05,
    DBI_PORTB_06 = IO_PORTB_06,
    DBI_PORTB_07 = IO_PORTB_07,
    DBI_PORTB_08 = IO_PORTB_08,
    DBI_PORTB_09 = IO_PORTB_09,
    DBI_PORTB_10 = IO_PORTB_10,
    DBI_PORTB_11 = IO_PORTB_11,

    DBI_PORTG_00 = IO_PORTG_00,
    DBI_PORTG_01 = IO_PORTG_01,
    DBI_PORTG_02 = IO_PORTG_02,
    DBI_PORTG_03 = IO_PORTG_03,

    DBI_PORTC_00 = IO_PORTC_00,
    DBI_PORTC_01 = IO_PORTC_01,
    DBI_PORTC_02 = IO_PORTC_02,
    DBI_PORTC_03 = IO_PORTC_03,
    DBI_PORTC_04 = IO_PORTC_04,
    DBI_PORTC_05 = IO_PORTC_05,
    DBI_PORTC_06 = IO_PORTC_06,
    DBI_PORTC_07 = IO_PORTC_07,
    DBI_PORTC_08 = IO_PORTC_08,
} DBI_IO;

// <<<[lcd接口配置]>>>
#define SPI_MODE  (0<<4)
#define DSPI_MODE (1<<4)
#define QSPI_MODE (2<<4)

// SPI线数标志(clk + dat)
#define SPI_WIRE3     0
#define SPI_WIRE4     1

// DSPI子模式标志
#define DSPI_SUBMODE0    0
#define DSPI_SUBMODE1    1
#define DSPI_SUBMODE2    2

// QSPI的子模式标志,A1D1表示地址1 ch,数据1 ch,依此类推
#define QSPI_A1D1     0 //0x02 or 0x42
#define QSPI_A1D2     1 //0xa2 or 0xe2
#define QSPI_A2D2     2 //0x22 or 0x62
#define QSPI_A1D4     3 //0x32 or 0x72
#define QSPI_A4D4     4 //0x12 or 0x52
#define QSPI_RAMLESS  5 //0xde

// LCD时序标志(根据LCD数据手册配置，常用时序均已组合到LCD_DRIVE_CONFIG宏，在LCD驱动文件配置)
#define PIXEL_1P1T (0<<5)
#define PIXEL_1P2T (1<<5)
#define PIXEL_1P3T (2<<5)
#define PIXEL_2P3T (3<<5)

#define PIXEL_1T2B  1
#define PIXEL_1T4B  3
#define PIXEL_1T6B  5
#define PIXEL_1T8B  7
#define PIXEL_1T9B  8
#define PIXEL_1T12B 11
#define PIXEL_1T16B 15
#define PIXEL_1T18B 17
#define PIXEL_1T24B 23

#define FORMAT_RGB565 0
#define FORMAT_RGB666 1
#define FORMAT_RGB888 2
#define FORMAT_YUV444 3
#define FORMAT_YUV422 4
#define FORMAT_BT656  5
#define FORMAT_BT1120 6

#define SPI_MODE_UNIDIR  0//半双工，d0分时发送接收
#define SPI_MODE_BIDIR   1//全双工，d0发送、d1接收

enum {
    CMD_MODE,
};

enum {
    CMD_8BIT,
    CMD_16BIT,
    CMD_24BIT,
};

// DBI TE
typedef enum {
    TE_SYNC_DISABLE,
    TE_SYNC_RISING,
    TE_SYNC_FALLING,
} TE_SYNC_MODE;

// DBI IO
struct dbi_signal_remap {
    u8 signal;
    DBI_IO gpio;
};

// MCU、RGB总线位宽
typedef enum {
    DATA_BUS_6BIT,  // Serial-RGB666(3cycle)
    DATA_BUS_8BIT,  // Serial-RGB565(2cycle)、Serial-RGB888(3cycle)
    DATA_BUS_9BIT,  // Serial-RGB666(2cycle)
    DATA_BUS_16BIT, // Parallel-RGB565(1cycle)
    DATA_BUS_18BIT, // Parallel-RGB666(1cycle)
    DATA_BUS_24BIT, // Parallel-RGB888(1cycle)
} DATA_BUS;

struct dbi_hw_param {
    const struct dbi_signal_remap *remap_tab;
    u8 eva_link_mode;
    u8 jstc_en; //显存压缩
    u8 remap_tab_size;
    struct mcu_hw_param {
        DATA_BUS data_bus;
        u8 use_rd_pin; // 8080总线是否使用RD(读使能)脚：1-接RD脚并初始化，0-不接RD脚省IO不初始化
    } mcu;
    struct rgb_hw_param {
        DATA_BUS data_bus;
        u8 use_soft_spi;
        int soft_spi_cs;
        int soft_spi_scl;
        int soft_spi_sda;
    } rgb;
    struct mipi_hw_param {
        u16 lane_num: 2;
        u16 reserved: 2;
        u16 x0_sel: 3;
        u16 x0_inv: 1;
        u16 x1_sel: 3;
        u16 x1_inv: 1;
        u16 x2_sel: 3;
        u16 x2_inv: 1;
        int lcd_reset_pin;
        void (*lcd_reset_enter)(int pin_reset);
        void (*lcd_reset_exit)(int pin_reset);
    } mipi;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 屏幕驱动接口类型定义，根据屏幕的接口类型在屏驱内配置使用
 */
/* ------------------------------------------------------------------------------------*/
typedef enum lcd_type_cfg {
    LCD_TYPE_SPI,	        // SPI屏
    LCD_TYPE_SPI_RAMLESS,	// SPI RAMLESS屏
    LCD_TYPE_MCU,	        // MCU屏
    LCD_TYPE_RGB,	        // RGB屏
    LCD_TYPE_MIPI,	        // MIPI屏
    LCD_TYPE_MAX,
} LCD_TYPE;


typedef enum {
    SDR_TYPE,
    DDR_TYPE1,
    DDR_TYPE2,
} DDR_TYPE;

typedef enum  {
    HSYNC_POLARITY_RISING,
    HSYNC_POLARITY_FALLING,
} HSYNC_POLARITY;

typedef enum  {
    VSYNC_POLARITY_RISING,
    VSYNC_POLARITY_FALLING,
} VSYNC_POLARITY;

typedef enum  {
    DEN_POLARITY_RISING,
    DEN_POLARITY_FALLING,
} DEN_POLARITY;

typedef enum {
    READ_FROM_DBI_D0,
    READ_FROM_DBI_D1,
    READ_FROM_DBI_D2,
    READ_FROM_DBI_D3,
} READ_PIN_SEL;

typedef enum {
    CLOCK_POLARITY_IDLE_LOW,  // 空闲时为低电平
    CLOCK_POLARITY_IDLE_HIGH, // 空闲时为高电平
} CLOCK_POLARITY;

typedef enum {
    CLOCK_PHASE_LEADING_EDGE, //时钟前沿,即第一个边沿,数据被采样
    CLOCK_PHASE_TRAILING_EDGE,//时钟后沿,即第二个边沿,数据被采样
} CLOCK_PHASE;

typedef enum {
    MCU_BUS_8080,
    MCU_BUS_6800,
} MCU_BUS;

/* 帧率约束类型 */
typedef enum {
    FPS_CONSTRAINT_CLOSEST = 0,   // 与目标帧率所对应的频率差值绝对值最小（可大可小）
    FPS_CONSTRAINT_LESS_THAN,     // 必须小于目标帧率
    FPS_CONSTRAINT_GREATER_THAN   // 必须大于目标帧率
} FPS_CONSTRAINT;

/* 频率约束类型 */
typedef enum {
    FREQ_CONSTRAINT_CLOSEST = 0,   // 与目标频率差值绝对值最小（可大可小）
    FREQ_CONSTRAINT_LESS_THAN,     // 必须小于目标频率
    FREQ_CONSTRAINT_GREATER_THAN   // 必须大于目标频率
} FREQ_CONSTRAINT;

struct dbi_param {
    // 显示区域相关(用于配置推屏区域，可不推全屏)
    int scr_x;
    int scr_y;
    int scr_w;
    int scr_h;

    // lcd配置
    int lcd_width;
    int lcd_height;
    LCD_TYPE lcd_type;	// LCD接口类型：SPI, SPI Ramless, MCU, RGB

    // 显存配置
    int buffer_num;
    int buffer_size;

    // dbi模块的输入，与imb模块一起用时，也是imb模块的输出
    int in_width;
    int in_height;
    int in_format;
    int in_stride;
    int rotate;

    // debug模式(推新屏时，可使能debug模式，先推纯色测试)
    int debug_mode_en;
    int debug_mode_color;

    // 帧率配置(用于计算DBI模块时钟频率，但实际推屏帧率仅会接近配置，并非100%一致)
    // 注意: fps和freq为0无效，不为0生效，fps和freq同时生效的情况下会优先使用fps
    int fps;//单位：Hz
    FPS_CONSTRAINT fps_constraint;

    int freq;//单位：Hz
    FREQ_CONSTRAINT freq_constraint;

    int max_write_freq; // 单位: Hz, 0:无效, >0:生效, 若生效上述的fps和freq都会受限制
    int max_read_freq;  // 单位: Hz, 0:无效, >0:生效

    u8 high_precision_freq_en; // 是否启用高精度的时钟频率(使用单独的Pll作为时钟源)

    u16 te_offset_line; // TE相对于原点的偏移行数

    // 以下为三种屏驱动相关配置，out_format为屏幕的像素格式类型
    struct spi_param {
        int spi_mode;
        int pixel_type;
        int out_format;
        int spi_dat_mode;
        READ_PIN_SEL read_pin_select;
        struct qspi {
            u8 write_cmd;
            u8 read_cmd;
            u8 a1d1_cmd;//default value 0x02(sdr)/0x42(ddr)
            u8 a1d2_cmd;//default value 0xa2(sdr)/0xe2(ddr)
            u8 a2d2_cmd;//default value 0x22(sdr)/0x62(ddr)
            u8 a1d4_cmd;//default value 0x32(sdr)/0x72(ddr)
            u8 a4d4_cmd;//default value 0x12(sdr)/0x52(ddr)
            DDR_TYPE ddr_type;
        } qspi;
        struct ramless {
            u32 frame_sync_cmd;//ramless cmd
            u32 porch_sync_cmd;//ramless cmd
            u32 line_sync_cmd;//ramless cmd
            u16 hsync;
            u16 hbp;    //hori back porch
            u16 hact;   //hori actual
            u16 hfp;    //hori front porch
            u16 vsync;
            u16 vbp;    //vert back porch
            u16 vact;   //vert actual line
            u16 vfp;    //vert front porch
        } ramless;
        u8 caset_cmd;//Column Address Set, default value 0x2a
        u8 paset_cmd;//Page Address Set, default value 0x2b
        u8 ramwr_cmd;//Memory Write Start, default value 0x2c
        u8 ramwrc_cmd;//Memory Write Continue, default valus 0x3c
        int (*custom_set_draw_area)(int (*lcd_write_cmd_cb)(u32, u8 *, u32), int, int, int, int);
        CLOCK_POLARITY clk_pol; // 时钟极性
    } spi;

    struct mcu_param {
        MCU_BUS bus_type;
        int out_format;
        u8 caset_cmd;//Column Address Set, default value 0x2a
        u8 paset_cmd;//Page Address Set, default value 0x2b
        u8 ramwr_cmd;//Memory Write Start, default value 0x2c
        u8 ramwrc_cmd;//Memory Write Continue, default valus 0x3c
        int (*custom_set_draw_area)(int (*lcd_write_cmd_cb)(u32, u8 *, u32), int, int, int, int);
    } mcu;

    struct rgb_param {
        u8 out_format;
        u8 continue_frames;

        u32 hsync_polarity: 1;
        u32 vsync_polarity: 1;
        u32 den_polarity: 1;
        u32 clock_polarity: 1;
        u32 clock_phase: 1;
        u32 blank_mode: 1;
        u32 rb_swap: 1;
        u32 big_end: 1;
        u32 yuv_limit: 1;

        u16 hsync;
        u16 hbp;
        u16 hact;
        u16 hfp;

        u16 vsync;
        u16 vbp;
        u16 vact;
        u16 vfp;
    } rgb;

    struct mipi_param {
        DSI_PACKET_CATEGORY packet_category;
        DSI_DISPLAY_MODE display_mode;
        DSI_COLOR_CODE color_code;
        DSI_COLOR_SORT color_sort;
        DSI_SYNC_MODE sync_mode;
        u8 virtual_channel : 2; // 虚拟通道, 一般默认为0
        u8 bllp0_en : 1;
        u8 bllp1_en : 1;
        u8 bllp2_en : 1;
        u8 eotp_en : 1;
        u8 clk_mode : 2;

        u8 frame_timeout_ms;   // unit:ms max:33ms

        u16 hsync;
        u16 hbp;
        u16 hact;
        u16 hfp;
        u16 hdmy_pix;   // burst mode才用到

        u16 vsync;
        u16 vbp;
        u16 vact;
        u16 vfp;
    } mipi;
};


struct dbi_ramless_callback_param {
    int lcd_buff_update;
    u32 lcd_buff_old;
    u32 lcd_buff_new;
};

enum DBI_EVA_LINKAGE_MODE {
    LINKAGE_MODE_GPU = 0x55,
    LINKAGE_MODE_DMM,
};

enum {
    LCD_ROTATE_0 = 0,
    LCD_MIRROR,
    LCD_FLIP,
    LCD_ROTATE_90,
    LCD_ROTATE_180,
    LCD_ROTATE_270,
    LCD_ROTATE_90_MIRROR,
    LCD_ROTATE_90_FLIP,
};



/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_init DBI模块初始化(注意：需要操作DBI或从DBI获取信息时，必须先初始化)
 *
 * @param param DBI配置句柄
 * @param hw_param DBI硬件相关参数（DBI IO）
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int lcd_init(struct dbi_param *param, struct dbi_hw_param *hw_param);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_write_cmd DBI模块向LCD指定地址写数据
 *
 * @param cmd 待操作的LCD命令地址
 * @param buf 数据缓存buf（如果不需要写数据，可传入NULL）
 * @param len 数据缓存长度（如果不写数据，需将len配置为0）
 */
/* ------------------------------------------------------------------------------------*/
int lcd_write_cmd(u32 cmd, u8 *buf, u32 len);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_read_cmd DBI模块从LCD指定地址读数据
 *
 * @param cmd 待操作的LCD命令地址
 * @param buf 数据缓存buf（输出）
 * @param len 读取的数据长度（需与buf匹配，不可超过buf大小）
 */
/* ------------------------------------------------------------------------------------*/
int lcd_read_cmd(u32 cmd, u8 *buf, u32 len);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_draw_area 设置LCD显示区域（2A, 2B命令）
 *
 * @param xstart X起始像素坐标
 * @param xend   X结束像素坐标
 * @param ystart Y起始像素坐标
 * @param yend   Y结束像素坐标
 */
/* ------------------------------------------------------------------------------------*/
void lcd_set_draw_area(int xstart, int xend, int ystart, int yend);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_draw DBI硬件写显示数据到LCD, 非阻塞接口，与lcd_set_draw_area配合使用
 *
 * @param buf     待写出数据
 * @param xstart  X起始像素坐标
 * @param xend    X结束像素坐标
 * @param ystart  Y起始像素坐标
 * @param yend    Y结束像素坐标
 */
/* ------------------------------------------------------------------------------------*/
void lcd_draw(u8 *buf, int xstart, int xend, int ystart, int yend);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_draw_continue DBI硬件以续传方式写显示数据到LCD, 非阻塞接口，与lcd_set_draw_area、lcd_draw配合使用
 *
 * @param buf     待写出数据
 * @param xstart  X起始像素坐标
 * @param xend    X结束像素坐标
 * @param ystart  Y起始像素坐标
 * @param yend    Y结束像素坐标
 */
/* ------------------------------------------------------------------------------------*/
void lcd_draw_continue(u8 *buf, int xstart, int xend, int ystart, int yend);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_clear 纯色填充LCD指定区域
 *
 * @param color  待填充的RGB888颜色值
 * @param xstart 待填充的X起始像素坐标
 * @param xend   待填充的X结束像素坐标
 * @param ystart 待填充的Y起始像素坐标
 * @param yend   待填充的Y结束像素坐标
 */
/* ------------------------------------------------------------------------------------*/
void lcd_clear(u32 color, int xstart, int xend, int ystart, int yend);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_ctrl_pin_func 配置LCD控制脚操作函数
 *
 * @param te_stat TE脚控制函数
 */
/* ------------------------------------------------------------------------------------*/
void lcd_set_ctrl_pin_func(int (*te_stat)());


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_te_wait_cb 配置TE信号等待回调函数，如果注册，等待TE时将调用回调进行等待
 *
 * @Params te_wait 应用层实现的TE等待函数
 */
/* ------------------------------------------------------------------------------------*/
void lcd_set_te_wait_cb(void (*te_wait)());

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_te_win_calculate TE窗口计算
 *
 * @Params lcd_fps	屏幕实际帧率
 * @Params min_fps	软件刷新一帧最低帧率
 * @Params *te_dly	输出：TE延迟推送
 * @Params *te_wth	输出：TE可用窗口
 */
/* ------------------------------------------------------------------------------------*/
int lcd_te_win_calculate(float lcd_fps, float min_fps, int *te_dly, int *te_wth);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_align 设置LCD对齐参数
 *
 * @param row_align    设置行对齐参数
 * @param column_align 设置列对齐参数
 */
/* ------------------------------------------------------------------------------------*/
void lcd_set_align(u8 row_align, u8 column_align);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_get_align 设置LCD对齐参数
 *
 * @param row_align    获取行对齐参数
 * @param column_align 获取列对齐参数
 */
/* ------------------------------------------------------------------------------------*/
void lcd_get_align(u8 *row_align, u8 *column_align);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_check_align 检查LCD刷屏区域参数是否对齐
 *
 * @param xstart X起始像素坐标
 * @param xend   X结束像素坐标
 * @param ystart Y起始像素坐标
 * @param yend   Y结束像素坐标
 */
/* ------------------------------------------------------------------------------------*/
int lcd_check_align(int xstart, int xend, int ystart, int yend);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_wait_busy 等待lcd空闲
 * 注意：本函数一般在推屏buffer释放前调用，等待推屏结束
 */
/* ------------------------------------------------------------------------------------*/
void lcd_wait_busy();

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_draw_kistart 无等待推屏接口
 */
/* ------------------------------------------------------------------------------------*/
void lcd_draw_kistart(uint8_t *buf, int xstart, int xend, int ystart, int yend);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_draw_set_callback 设置推屏结束回调, lcd_draw、lcd_draw_continue推屏结束时调用该回调
 */
/* ------------------------------------------------------------------------------------*/
int  lcd_draw_set_callback(int (*callback)(void *priv));

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_te_trig_kistart_set_busy 设置LCD繁忙状态，需TE中断调用lcd_draw_kistart接口推屏，推屏结束后会自动消除繁忙状态,
 * 另外附加作用是打印调试接口，如果是kistart是由TE来引导的,那么wait lcd busy就是正常的, 不用打印出来
 */
/* ------------------------------------------------------------------------------------*/
void lcd_te_trig_kistart_set_busy(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_param 设置LCD模式
 */
/* ------------------------------------------------------------------------------------*/
void lcd_set_param(u8 mode, int priv);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_get_param 获取LCD模式
 */
/* ------------------------------------------------------------------------------------*/
int lcd_get_param(u8 mode);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief dbi_get_actual_fps 获取实际帧率
 */
/* ------------------------------------------------------------------------------------*/
float lcd_get_actual_fps();

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_get_input_format_bpp 获取输入格式字节数(bytes per pixel)
 */
/* ------------------------------------------------------------------------------------*/
u8 lcd_get_input_format_bpp(int in_format);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_set_te_detect_gpio 设置te脚
 */
/* ------------------------------------------------------------------------------------*/
int lcd_set_te_detect_gpio(u16 gpio, TE_SYNC_MODE sync_mode, u32 dly, u32 wth);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_enter_sleep 进入睡眠状态
 */
/* ------------------------------------------------------------------------------------*/
void lcd_enter_sleep();

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lcd_exit_sleep 退出睡眠状态
 */
/* ------------------------------------------------------------------------------------*/
void lcd_exit_sleep();


#endif







