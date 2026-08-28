#include "app_config.h"

#if TCFG_VIDEO0_ENABLE

#include "asm/iic_hw.h"
#include "isp_dev.h"
#include "iic_api.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/device.h"
#include "app_config.h"

static hw_iic_dev iic_dev = HW_IIC_0;
static enum iic_state_enum iic_init_state = -1;
static u8 xs9950_reset_io = -1;
static u8 xs9950_power_io = -1;

#define XS9950_OUTPUT_WIDTH     1280
#define XS9950_OUTPUT_HEIGHT    720
#define XS9950_OUTPUT_FPS       25

#define XS9950_WRCMD 0x60
#define XS9950_RDCMD 0x61

#define COLOR_BAR_TEST_ENABLE   1

#define gpio_set_die(gpio, value)  			gpio_hw_set_die(IO_PORT_SPILT(gpio), value)
#define gpio_direction_output(gpio, value) 	gpio_hw_direction_output(IO_PORT_SPILT(gpio), value)
#define gpio_direction_input(gpio) 			gpio_hw_direction_input(IO_PORT_SPILT(gpio))

typedef struct {
    u16 addr;
    u8 value;
} sensor_reg_ini;

static const sensor_reg_ini xs9950_dvp_init[] = {
    // enVoClkEdge = NI_VO_CLK_EDGE_RISING;  // 上升沿采样
    // vo_clk = 148.5MHz
    {0x4135, 0x02},  // 此为默认值，BT656 D0-D7以及VO_HS/VS IO电平，bit1=0:1.8V，bit1=1:3.3V
    {0x101F, 0x00},  // 此为默认值，bit4=0: VCCA18_33 is 1.8V.  bit4=1:VCCA18_33 is 3.3V
    {0x4107, 0x40},  // 此为默认值，bit0=0:iic_rst_ls3v is 1.8V   bit0=1: iic_rst_ls3v is 3.3V

    {0x101F, 0x10}, // 此为默认值，bit4=0: VCCA18_33 is 1.8V.  bit4=1:VCCA18_33 is 3.3V
    {0x4107, 0x41},  // 此为默认值，bit0=0:iic_rst_ls3v is 1.8V   bit0=1: iic_rst_ls3v is 3.3V

    {0x4300, 0x05},
    {0x4300, 0x15},
    {0x4080, 0x07},
    {0x4119, 0x01},
    {0x0803, 0x00},
    {0x4020, 0x00},
    {0x080e, 0x00},
    {0x080e, 0x20},
    {0x080e, 0x28},
    {0x4020, 0x03},
    {0x0803, 0x0f},
    {0x0100, 0x35},
    {0x0104, 0x48},
    {0x0300, 0x3f},
    {0x0105, 0xe1},
    {0x0101, 0x42},
    {0x0102, 0x40},
    {0x0116, 0x3c},
    {0x0117, 0x23},
    {0x0333, 0x09},
    {0x0337, 0xd9},
    {0x0338, 0x0a},
    {0x01bf, 0x4e},
    {0x010e, 0x78},
    {0x010f, 0x92},
    {0x0110, 0x70},
    {0x0111, 0x40},
    {0x01e1, 0xff},
    {0x0314, 0x66},
    {0x0130, 0x10},
    {0x0315, 0x23},
    {0x0b64, 0x02},
    {0x01e2, 0x03},
    {0x0b55, 0x80},
    {0x0b56, 0x00},
    {0x0b59, 0x04},
    {0x0b5a, 0x01},
    {0x0b5c, 0x07},
    {0x0b5e, 0x05},
    {0x0b4b, 0x10},
    {0x0b4e, 0x05},
    {0x0b51, 0x21},
    {0x0b30, 0xbc},
    {0x0b31, 0x19},
    {0x0b15, 0x03},
    {0x0b16, 0x03},
    {0x0b17, 0x03},
    {0x0b07, 0x03},
    {0x0b08, 0x05},
    {0x0b1a, 0x10},
    {0x0158, 0x03},
    {0x0a88, 0x20},
    {0x0a61, 0x09},
    {0x0a62, 0x00},
    {0x0a63, 0x0e},
    {0x0a64, 0x00},
    {0x0a65, 0xfc},
    {0x0a67, 0xe5},
    {0x0a69, 0xef},
    {0x0a6b, 0x1b},
    {0x0a6d, 0x2f},
    {0x0a6f, 0x00},
    {0x0a71, 0xc2},
    {0x0a72, 0xff},
    {0x0a73, 0xd0},
    {0x0a74, 0xff},
    {0x0a75, 0x29},
    {0x0a77, 0x57},
    {0x0a78, 0x00},
    {0x0a79, 0x10},
    {0x0a7a, 0x00},
    {0x0a7b, 0xaa},
    {0x0a7d, 0xb2},
    {0x0a7f, 0x24},
    {0x0a80, 0x00},
    {0x0a81, 0x69},
    {0x0a82, 0x00},
    {0x0802, 0x02},
    {0x0501, 0x81},
    {0x0b74, 0xfc},
    {0x01dc, 0x01},
    {0x0804, 0x04},
    {0x4018, 0x01},
    {0x0b56, 0x01},
    {0x0b73, 0x02},
    {0x4210, 0x0c},
    {0x420b, 0x2f},
    {0x0504, 0x89},  // bit[4:7] free_run颜色, 取值范围:[0, 8]
    // {0x0507, 0x13},  // bit[5] 0:上升沿采集模式; 1:上下边沿采集模式;
    {0x0507, 0x0b},  // bit[5] 0:上升沿采集模式; 1:上下边沿采集模式;
    {0x0503, 0x00},
    {0x0502, 0x0c},  // bit4 bt data0-7 取反，改为data7-0
    {0x015a, 0x00},
    {0x015b, 0x24},
    {0x015c, 0x80},
    {0x015d, 0x16},
    {0x015e, 0xd0},
    {0x015f, 0x02},
    {0x0160, 0xee},
    {0x0161, 0x02},
    {0x0165, 0x00},
    {0x0166, 0x0f},
    {0x4030, 0x15},
    {0x4134, 0x0a},  // 增加BT输出驱动能力0x6 -> 0xa

    //复位MIPI信号, 使能TTL模式配置
    {0x0803, 0x0f},
    {0x4412, 0x01},
    {0x0803, 0x1f},
    //使能MIPI IO TTL
    {0x10e3, 0x04},
    //使能analog driver
    {0x10eb, 0xfd},

    {0x0800, 0x07},
    {0x0805, 0x07},
    {0x4200, 0x02},  // 0x4200=2：选择VINA，0x4200=0：选择VINB，x4200=4：选择VINC，0x4200=6：选择VIND
    {0x060b, 0x00},
    // {0x0111, 0x68}  // 适当调整这三个寄存器，也可增加清晰度和饱和度
    // {0x4202, 0x0e}  // 适当调整这三个寄存器，也可增加清晰度和饱和度
    // {0x4203, 0x04}  // 适当调整这三个寄存器，也可增加清晰度和饱和度
    // {0x0100, 0x38}  #
    // {0x01ce, 0x00}  // 调整自动曝光
};

static const sensor_reg_ini xs9950_format_init[] = {
    {0x0627, 0x14},
    {0x010c, 0x00},
    {0x0800, 0x05},
    {0x0805, 0x05},
    {0x0b50, 0x08},
    {0x0e08, 0x00},
    {0x010d, 0x40},
    {0x010c, 0x01},
    {0x0121, 0x7a},
    {0x0122, 0x6b},
    {0x0130, 0x10},
    {0x01a9, 0x00},
    {0x01aa, 0x04},
    {0x0156, 0x00},
    {0x0157, 0x08},
    {0x0105, 0xc1},  // AHD制式如发现颜色饱和度异常，可选择开关
    {0x0101, 0x42},
    {0x0102, 0x40},
    {0x0116, 0x3c},
    {0x0117, 0x23},  // AHD制式如发现颜色饱和度异常，可选择打开，并微调0x117
    {0x01e2, 0x03},
    {0x420b, 0x21},
    {0x0106, 0x80},
    {0x0107, 0x00},
    {0x0108, 0x80},
    {0x0109, 0x00},
    {0x010a, 0x20},
    {0x010b, 0x00},
    {0x011d, 0x17},
    {0x0e08, 0x01},
    {0x0a60, 0x04},
    {0x0a5c, 0xf6},
    {0x0a5d, 0xc0},
    {0x0a5e, 0x2d},
    {0x0a5f, 0x1b},
    {0x0156, 0x50},
    {0x0157, 0x07},
    {0x011d, 0x17},
    {0x0156, 0x00},
    {0x0157, 0x08},
    {0x0503, 0x00},
    {0x015a, 0x8b},
    {0x015b, 0x0e},
    {0x015c, 0x80},
    {0x015d, 0x16},
    {0x015e, 0xd0},
    {0x015f, 0x02},
    {0x0160, 0xee},
    {0x0161, 0x02},
    {0x0165, 0x40},
    {0x0166, 0x0f},
    {0x0A00, 0xFD},
    {0x0A01, 0xFF},
    {0x0A02, 0x00},
    {0x0A03, 0x00},
    {0x0A04, 0x04},
    {0x0A05, 0x00},
    {0x0A06, 0x01},
    {0x0A07, 0x00},
    {0x0A08, 0xFB},
    {0x0A09, 0xFF},
    {0x0A0A, 0xFE},
    {0x0A0B, 0xFF},
    {0x0A0C, 0x07},
    {0x0A0D, 0x00},
    {0x0A0E, 0x03},
    {0x0A0F, 0x00},
    {0x0A10, 0xF7},
    {0x0A11, 0xFF},
    {0x0A12, 0xFA},
    {0x0A13, 0xFF},
    {0x0A14, 0x0B},
    {0x0A15, 0x00},
    {0x0A16, 0x0A},
    {0x0A17, 0x00},
    {0x0A18, 0xF3},
    {0x0A19, 0xFF},
    {0x0A1A, 0xF1},
    {0x0A1B, 0xFF},
    {0x0A1C, 0x0F},
    {0x0A1D, 0x00},
    {0x0A1E, 0x18},
    {0x0A1F, 0x00},
    {0x0A20, 0xEE},
    {0x0A21, 0xFF},
    {0x0A22, 0xDC},
    {0x0A23, 0xFF},
    {0x0A24, 0x14},
    {0x0A25, 0x00},
    {0x0A26, 0x39},
    {0x0A27, 0x00},
    {0x0A28, 0xEB},
    {0x0A29, 0xFF},
    {0x0A2A, 0x98},
    {0x0A2B, 0xFF},
    {0x0A2C, 0x16},
    {0x0A2D, 0x00},
    {0x0A2E, 0x45},
    {0x0A2F, 0x01},
    {0x0A30, 0xEA},
    {0x0A31, 0x01},
    {0x0A60, 0x01},
    {0x0158, 0x01},
};

static unsigned char wrXS9950Reg(u16 regID, unsigned char regDat)
{
    u8 addr[2];
    addr[0] = regID >> 8;
    addr[1] = regID & 0xFF;
    int ret = i2c_master_write_nbytes_to_device_reg(iic_dev, XS9950_WRCMD, addr, 2, &regDat, 1);
    return (ret < 0) ? 0 : 1;
}

static unsigned char rdXS9950Reg(u16 regID, unsigned char *regDat)
{
    u8 addr[2];
    addr[0] = regID >> 8;
    addr[1] = regID & 0xFF;
    int ret = i2c_master_read_nbytes_from_device_reg(iic_dev, XS9950_WRCMD, addr, 2, regDat, 1);
    return (ret < 0) ? 0 : 1;
}

static void XS9950_config_sensor(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    // 1. 跑第一组初始化
    for (i = 0; i < ARRAY_SIZE(xs9950_dvp_init); i++) {
        wrXS9950Reg(xs9950_dvp_init[i].addr, xs9950_dvp_init[i].value);
    }

    // 2. 跑第二组格式初始化
    for (i = 0; i < ARRAY_SIZE(xs9950_format_init); i++) {
        wrXS9950Reg(xs9950_format_init[i].addr, xs9950_format_init[i].value);
    }

#if COLOR_BAR_TEST_ENABLE   //接入摄像头会覆盖彩条
    // wrXS9950Reg(0x0507, 0x13); // 彩条使能
    wrXS9950Reg(0x0507, 0x0b); // 彩条使能
    wrXS9950Reg(0x0503, 0x00); // 彩条制式5:1080p25
    wrXS9950Reg(0x0504, 0x89); // 彩条颜色
#endif

    *format = SEN_IN_FORMAT_UYVY;
    printf("XS9950 DVP Config OK\n");
}

static s32 XS9950_ID_check(void)
{
    u8 pid = 0, ver = 0;
    for (int i = 0; i < 5; i++) {
        rdXS9950Reg(0x40F0, &pid);
        rdXS9950Reg(0x40F1, &ver);
        if (pid == 0x99 && ver == 0x50) {
            printf("XS9950 Sensor ID : 0x%x %x\n", pid, ver);
            return 0;
        }
    }
    return -1;
}

static void XS9950_reset(u8 isp_dev)
{
    printf("XS9950 DVP Reset, reset_io:%x\n", xs9950_reset_io);
    if (xs9950_power_io != (u8) - 1) {
        gpio_direction_output(xs9950_power_io, 1);
    }
    if (xs9950_reset_io != (u8) - 1) {
        gpio_direction_output(xs9950_reset_io, 0);
        os_time_dly(1);
        gpio_direction_output(xs9950_reset_io, 1);
    }
}

static s32 XS9950_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio, char *iic_name)
{
    if (iic_init_state == -1) {
        struct iic_master_config iic_config = {
            .role = IIC_MASTER,
            .scl_io = TCFG_CAMERA_DVP_SCL,
            .sda_io = TCFG_CAMERA_DVP_SDA,
            .io_mode = PORT_INPUT_PULLUP_10K,
            .hdrive = PORT_DRIVE_STRENGT_2p4mA,
            .master_frequency = 100000,
            .io_filter = 1,
        };
        iic_init_state = iic_init(iic_dev, &iic_config);
        xs9950_reset_io = (u8)_reset_gpio;
        xs9950_power_io = (u8)_power_gpio;
    }

    if (iic_init_state != IIC_OK) {
        return -1;
    }
    XS9950_reset(isp_dev);

    if (0 != XS9950_ID_check()) {
        printf("-------not XS9950------\n\n");
        iic_deinit(iic_dev);
        iic_init_state = -1;
        return -1;
    }
    printf("-------hello XS9950------\n\n");

    return 0;
}

static s32 XS9950_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    XS9950_config_sensor(isp_dev, width, height, format, frame_freq);

    return 0;
}

u16 XS9950_dvp_rd_reg(u8 isp_dev, u16 addr)
{
    u8 val;
    rdXS9950Reg(addr, &val);
    return val;
}

void XS9950_dvp_wr_reg(u8 isp_dev, u16 addr, u16 val)
{
    wrXS9950Reg(addr, (u8)val);
}


REGISTER_CAMERA(XS9950) = {
    .logo               =   "XS9950",
    .isp_dev            =   ISP_DEV_NONE,
    .in_format          =   SEN_IN_FORMAT_UYVY,
    .out_format         =   ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_BT656,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B | SEN_MBUS_PCLK_SAMPLE_RISING,
    .fps                =   XS9950_OUTPUT_FPS,
    .sen_size           =   {XS9950_OUTPUT_WIDTH, XS9950_OUTPUT_HEIGHT},
    .ops                =   {
        .sensor_check   =   XS9950_check,
        .init           =   XS9950_init,
        .set_size_fps   =   NULL,
        .power_ctrl     =   NULL,
        .write_reg      =   XS9950_dvp_wr_reg,
        .read_reg       =   XS9950_dvp_rd_reg,
    }
};

#endif
