#include "app_config.h"
#include "sensor_driver.h"

#if TCFG_SENSOR_MMC5603_ENABLE

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_5603]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"



#define IIC_ADDR_W          (0x30 << 1 | 0x0)
#define IIC_ADDR_R          (0x30 << 1 | 0x1)

static axis_data_t sensor_data[50];
static cbuffer_t   sensor_cbuffer;

static sensor_info_t sensor_info = {
    .type    = SENSOR_DRV_MAGNETIC,
    .name    = "mmc5603",
    .range   = {2, 4, 8, 16},
    .odr     = {10, 25, 50, 100},
    .cbuffer = 0,
};

static s8 sensor_check(void)
{
    u8 dev_id = 0;
    sensor_read(IIC_ADDR_R, 0x39, &dev_id, 1, 0);

    // if(sensor_info.cbuffer == NULL){
    //     sensor_info.cbuffer = &sensor_cbuffer;
    //     cbuf_init(sensor_info.cbuffer, sensor_data, sizeof(sensor_data));
    // }
    log_info("dev_id=%02x cbuf=%x", dev_id, (u32)sensor_info.cbuffer);
    return ((dev_id == 0x10)) ? RET_OK : RET_ERR;
}


static s8 sensor_open(u16 range, u16 odr)
{
    u8 register_value ;
    s8 ret = RET_OK;

    /* Write 0x10 to register 0x1B, set RESET bit high */
    if (ret == RET_OK) {
        register_value = 0x10;
        ret = sensor_write(IIC_ADDR_W, 0x1B, &register_value, 1); //RESET
        /* Delay to finish the RESET operation */
        mdelay(1);
    }

    /* Write reg 0x1C, Set BW<1:0> = bandwith */
    if (ret == RET_OK) {
        register_value = odr / 75;
        ret = sensor_write(IIC_ADDR_W, 0x1C, &register_value, 1);
    }

    /* Write reg 0x1A, set ODR<7:0> = sampling_rate */
    if (ret == RET_OK) {
        register_value = odr;
        ret = sensor_write(IIC_ADDR_W, 0x1A, &register_value, 1);
    }

    /* Write reg 0x1B */
    /* Set Auto_SR_en bit '1', Enable the function of automatic set/reset */
    /* Set Cmm_freq_en bit '1', Start the calculation of the measurement period according to the ODR*/
    if (ret == RET_OK) {
        register_value = 0x80 | 0x20;
        ret = sensor_write(IIC_ADDR_W, 0x1B, &register_value, 1);
    }

    /* Write reg 0x1D */
    /* Set Cmm_en bit '1', Enter continuous mode */
    if (ret == RET_OK) {
        register_value = 0x10;
        ret = sensor_write(IIC_ADDR_W, 0x1D, &register_value, 1);
    }

    return ret;
}


static s8 sensor_close(void)
{
    u8 val = 0;
    return sensor_write(IIC_ADDR_W, 0x1D, &val, 1);
}

static s8 sensor_read_single_data(void *arg, u16 *len)
{
    u8  data_reg[6]  = {0};
    axis_data_t *mag = arg;
    ASSERT(arg);
    sensor_read(IIC_ADDR_R, 0x00, data_reg, 6, 0);

    mag[0].x  = (s16)(data_reg[0] << 8 | data_reg[1]);
    mag[0].y  = (s16)(data_reg[2] << 8 | data_reg[3]);
    mag[0].z  = (s16)(data_reg[4] << 8 | data_reg[5]);

    mag[0].x -= 32768;
    mag[0].y -= 32768;
    mag[0].z -= 32768;

    *len      = 6;

    // if((*len>0 ) && (sensor_info.cbuffer)){
    //     u32 wlen = cbuf_write(sensor_info.cbuffer, arg, *len);
    //     LOG("cbuf wlen=%d glen=%d",wlen,*len);
    // }
    log_debug("mag:%d,%d,%d", mag[0].x, mag[0].y, mag[0].z);

    return 1;
}

REGISTER_SENSOR(sensor_acc) = {
    .info    = &sensor_info,
    .online  = sensor_check,
    .open    = sensor_open,
    .close   = sensor_close,
    .run     = sensor_read_single_data,
};
#endif
