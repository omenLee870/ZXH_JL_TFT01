#include "app_config.h"
#include "sensor_driver.h"

#if TCFG_SENSOR_HUB && TCFG_ACCELER_MASTER_ENABLE

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_HUB_DRIVER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_ACCELER_MASTER_ENABLE && TCFG_ACCELER_P11_ENABLE
#error "TCFG_ACCELER_MASTER_ENABLE & TCFG_ACCELER_P11_ENABLE NOT OPEN IN SAME TIME!!!\n"
#endif


#define CONFIG_SENSOR_SLEEP_ENABLE   1

#define LOG(fmt,...)     printf("[sc7a20] " fmt "\n",##__VA_ARGS__)

/***使用驱动前请根据实际接线情况配置（7bit）IIC地址******/
/**SC7A20的SDO 脚接地：            0x18****************/
/**SC7A20的SDO 脚接电源：           0x19****************/
#define IIC_ADDR_W          (0x19U << 1 | 0x0)
#define IIC_ADDR_R          (0x19U << 1 | 0x1)
/*******************************************************/
static u8 sleep_mode = 0;
static sensor_info_t sensor_info = {
    .type = SENSOR_DRV_ACCELER,
    .name = "sc7a20",
    .range = {2, 4, 8, 16},
    .odr = {10, 25, 50, 100},
    .cbuffer = 0,
};
static s8 sensor_open(u16 range, u16 odr);
static const sensor_regs_t sensor_regs_array[] = {
    {0x57, 0x00},
    {0x1f, 0x00},
    {0x20, 0x3F},
    {0x21, 0x70},
    {0x23, 0x10},
    {0x24, 0x40},
    {0x2E, 0x8F},
    {0x22, 0x00},
};


static s8 sensor_check(void)
{
    u8 dev_id = 0;
    sensor_read(IIC_ADDR_R, 0x70, &dev_id, 1, 0);
    log_debug("[7a20]dev_id=%02x", dev_id);
    return ((dev_id == 0x28 || dev_id == 0x11)) ? RET_OK : RET_ERR;
}

static s8 sensor_read_config(void)
{
    u8 reg_val = 0;
    for (u8 i = 0; i < sizeof(sensor_regs_array) / sizeof(sensor_regs_t); i++) {
        if (sensor_read(IIC_ADDR_R, sensor_regs_array[i].addr, &reg_val, 1, 0) == RET_ERR) {
            return RET_ERR;
        }
        log_debug("[7a20]reg %02x = %02x", sensor_regs_array[i].addr, reg_val);
    }
    return RET_OK;
}

static s8 sensor_open(u16 range, u16 odr)
{
    if (sleep_mode) {
        u8 reg_val = 0;
        sensor_read(IIC_ADDR_R, 0x31, &reg_val, 1, 0);
        log_debug("[7a20]reg_val=%x wake=%x", reg_val, reg_val & 0x2a);
        if (!(reg_val & 0x2a)) {
            return RET_ERR;
        }
    }
    const u8 config_range[4] = {0x00, 0x10, 0x20, 0x30};
    const u8 config_odr[4]   = {0x2F, 0x3F, 0x4F, 0x5F};

    u8 acc_conf_odr   = 0x3F;  //默认 25hz
    u8 acc_conf_range = 0x10;  //默认 +-4g

    for (u8 i = 0; i < sizeof(sensor_regs_array) / sizeof(sensor_regs_t); i++) {
        if (sensor_write(IIC_ADDR_W, sensor_regs_array[i].addr, (u8 *)&sensor_regs_array[i].value, 1) == RET_ERR) {
            return RET_ERR;
        }
    }

    for (u8 i = 0; i < 4; i++) {
        if (sensor_info.range[i] == range) {
            acc_conf_range = config_range[i];
        }
        if (sensor_info.odr[i] == odr) {
            acc_conf_odr = config_odr[i];
        }
    }

    sensor_write(IIC_ADDR_W, 0x20, &acc_conf_odr, 1);
    sensor_write(IIC_ADDR_W, 0x23, &acc_conf_range, 1);
    sleep_mode = 0;
    return RET_OK;
}


static s8 sensor_close(void)
{
    u8 val = 0;
    return sensor_write(IIC_ADDR_W, 0x20, &val, 1);
}

static s8 sensor_read_fifo_data(void *arg, u16 *len)
{
    if (sleep_mode) {
        *len = 0;
        return RET_OK;
    }

    u8 fifo_len = 0;
    u8 temp_arry[6];
    axis_data_t *accel = arg;

    sensor_read(IIC_ADDR_R, 0x2f, &fifo_len, 1, 0);
    fifo_len = fifo_len & 0x1f;

    for (u8 i = 0; i < fifo_len; i++) {
        sensor_read(IIC_ADDR_R, 0xa8, temp_arry, 6, 0);
        accel[i].x = (((s16)((s16)temp_arry[1] * 256 + temp_arry[0])) >> 3);
        accel[i].y = -(((s16)((s16)temp_arry[3] * 256 + temp_arry[2])) >> 3);
        accel[i].z = (((s16)((s16)temp_arry[5] * 256 + temp_arry[4])) >> 3);
        log_debug("[7a20] xyz(%d,%d): %d %d %d", fifo_len, i, accel[i].x, accel[i].y,  accel[i].z);
    }
    fifo_len *= sizeof(axis_data_t);
    *len = fifo_len;
    return RET_OK;
}


#if CONFIG_SENSOR_SLEEP_ENABLE
static const sensor_regs_t sensor_wakeup_regs_array[] = {
    {0x20, 0x47},
    {0x23, 0x88},   //+-2g
    {0x21, 0x31},
    {0x22, 0x40},   //AOI中断on int1
    {0x25, 0x00},
    {0x24, 0x00},
    {0x30, 0x2a},   //x,y,z高事件或检测
    {0x32, 0x02},   //检测门限: 1-127, 值越小, 灵敏度越高
    {0x33, 0x00},
};

static s8 sensor_wakeup_enable(void)
{
    log_debug("[7a20]sensor_wakeup_enable");
    for (u8 i = 0; i < sizeof(sensor_wakeup_regs_array) / sizeof(sensor_regs_t); i++) {
        if (sensor_write(IIC_ADDR_W, sensor_wakeup_regs_array[i].addr, (u8 *)&sensor_wakeup_regs_array[i].value, 1) == RET_ERR) {
            return RET_ERR;
        }
    }
    sleep_mode = 1;
    return RET_OK;
}

#endif

REGISTER_SENSOR(sensor_acc) = {
    .info    = &sensor_info,
    .online  = sensor_check,
    .open    = sensor_open,
    .close   = sensor_close,
    .run     = sensor_read_fifo_data,
#if CONFIG_SENSOR_SLEEP_ENABLE
    .sleep   = sensor_wakeup_enable,
#endif
};


#endif
