#include "sdk_config.h"
#include "sensor_driver.h"

#if 1
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#pragma const_seg(".sensor_const")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#pragma const_seg(".sensor_const")
#endif

#define LOG_TAG_CONST       SENSOR_DRIVER
#define LOG_TAG     		"[SENSOR_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"

#if CONFIG_SENSOR_DRIVER_ENABLE && TCFG_SC7A20_ENABLE

/***使用驱动前请根据实际接线情况配置（7bit）IIC地址******/
/**SC7A20的SDO 脚接地：            0x18****************/
/**SC7A20的SDO 脚接电源：           0x19****************/
#define IIC_ADDR_W          (0x19U << 1 | 0x0)
#define IIC_ADDR_R          (0x19U << 1 | 0x1)
/*******************************************************/
static axis_data_t accel_data[50];  //accel data buffer
static p11_cbuffer_t  accel_cbuffer;
static cbuffer_child_t entry[3];//支持3个成员读取
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

    if (sensor_info.cbuffer == NULL) {
        sensor_info.cbuffer = &accel_cbuffer;
        cbuf_mult_read_init(sensor_info.cbuffer, accel_data, sizeof(accel_data), 3, entry);
    }
    log_info("[SC7A20]dev_id=%02x cbuf=%x", dev_id, (u32)sensor_info.cbuffer);
    return ((dev_id == 0x28 || dev_id == 0x11) && sensor_info.cbuffer) ? RET_OK : RET_ERR;
}

static s8 sensor_read_config(void)
{
    u8 reg_val = 0;
    for (u8 i = 0; i < sizeof(sensor_regs_array) / sizeof(sensor_regs_t); i++) {
        if (sensor_read(IIC_ADDR_R, sensor_regs_array[i].addr, &reg_val, 1, 0) == RET_ERR) {
            return RET_ERR;
        }
        log_debug("reg %02x = %02x", sensor_regs_array[i].addr, reg_val);
    }
    return RET_OK;
}

static s8 sensor_open(u16 range, u16 odr)
{
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
    return RET_OK;
}


static s8 sensor_close(void)
{
    u8 val = 0;
    return sensor_write(IIC_ADDR_W, 0x20, &val, 1);
}

static s8 sensor_read_fifo_data(void *arg, u16 *len)
{
    u8 fifo_len = 0;
    u8 temp_arry[6];
    axis_data_t *accel = arg;

    sensor_read(IIC_ADDR_R, 0x2f, &fifo_len, 1, 0);
    fifo_len = fifo_len & 0x1f;
    log_debug("%s fifo:%d\n", __func__, fifo_len);
    for (u8 i = 0; i < fifo_len; i++) {
        sensor_read(IIC_ADDR_R, 0xa8, temp_arry, 6, 0);
        accel[i].x = (((s16)((s16)temp_arry[1] * 256 + temp_arry[0])) >> 3);
        accel[i].y = -(((s16)((s16)temp_arry[3] * 256 + temp_arry[2])) >> 3);
        accel[i].z = (((s16)((s16)temp_arry[5] * 256 + temp_arry[4])) >> 3);

        /* printf(" xyz(%d,%d): %d %d %d", fifo_len, i, accel[i].x, accel[i].y,  accel[i].z); */
    }
    log_debug(" xyz(%d): %d %d %d", fifo_len,  accel[0].x, accel[0].y,  accel[0].z);
    fifo_len *= sizeof(axis_data_t);

    if ((fifo_len > 0) && (sensor_info.cbuffer)) {

        if (!cbuf_mult_is_write_able(sensor_info.cbuffer, fifo_len)) {
            cbuf_mult_read_alloc_len_updata(sensor_info.cbuffer, 0, fifo_len);
            cbuf_mult_read_alloc_len_updata(sensor_info.cbuffer, 1, fifo_len);
            log_debug("cbuf update");
        }
        u32 wlen = cbuf_mult_write(sensor_info.cbuffer, arg, fifo_len);
        log_debug("cbuf wlen=%d glen=%d", wlen, fifo_len);
    }
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
    log_info("sensor_wakeup_enable");
    for (u8 i = 0; i < sizeof(sensor_wakeup_regs_array) / sizeof(sensor_regs_t); i++) {
        if (sensor_write(IIC_ADDR_W, sensor_wakeup_regs_array[i].addr, (u8 *)&sensor_wakeup_regs_array[i].value, 1) == RET_ERR) {
            return RET_ERR;
        }
    }
    return RET_OK;
}

__attribute__((weak)) s8 driver_sc7a20_is_wake(void)
{
    u8 reg_val = 0;
    if (sensor_read(IIC_ADDR_R, 0x31, &reg_val, 1, 0) == RET_OK) {
        log_debug("reg_val=%x wake=%x", reg_val, reg_val & 0x2a);
        if (reg_val & 0x2a) {
            sensor_open(4, 25);
        } else {
            return RET_ERR;
        }
    }
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
