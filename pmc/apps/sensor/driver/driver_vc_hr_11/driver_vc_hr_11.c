
/*********************************************************************************************************
*               Copyright(c) 2018, Vcare Corporation. All rights reserved.
**********************************************************************************************************
* @file     module_heart_vc.c
* @brief
* @details
* @author
* @date
* @version  v1.6
*********************************************************************************************************
*/
#include "sdk_config.h"
#include "sensor_driver.h"
#include "vcHr11Hci.h"

#if CONFIG_SENSOR_DRIVER_ENABLE && TCFG_VCHR11S_ENABLE

#define LOG(fmt,...)       printf("[vcHr11] " fmt "\n",##__VA_ARGS__)

#define IIC_ADDR_W         (0x33<<1 | 0)
#define IIC_ADDR_R         (0x33<<1 | 1)


const uint32_t mcuOscData      = 30000;   // Timer clock frequency  Be used to adjust INT frequency
const uint16_t mcuI2cClock     = 400;     // MCU I2C clock frequency

static vcHr11_t vcHr11;

static s16             sensor_data[50 * 2];
static p11_cbuffer_t       sensor_cbuffer;
static cbuffer_child_t entry[1];//支持3个成员读取

static sensor_info_t sensor_info = {
    .type    = SENSOR_DRV_HR_SPO2,
    .name    = "lc11s",
    .odr     = {10, 25, 50, 100},
    .cbuffer = 0,
};



extern u64 lptmr1_get_pass_us(void);
extern u64 __lp_timer_get_cnt(u8 lptmr_x);
extern u32 lrc_get_avg_freq(void);

u32 vcHr11GetRtcCountFromMCU(void)
{
    static u64 cnt = 0;
    u32 pass_32bitcnt;

    if (lrc_get_avg_freq() == 0) {
        LOG("lrc_get_avg_freq is 0");
        return 0;
    }

    /* cnt += lptmr1_get_pass_us(); */

    cnt = lptmr1_get_pass_us();
    pass_32bitcnt = cnt / (1000000 / mcuOscData);
    pass_32bitcnt = pass_32bitcnt & (0xffffffff);
    LOG("cnt=%d %d ", pass_32bitcnt, (u32)lptmr1_get_pass_us());
    return pass_32bitcnt;
}

vcHr11Ret_t vcHr11WriteRegisters(uint8_t startAddress, uint8_t *pRegisters, uint8_t len)
{
    u8 ret = sensor_write(IIC_ADDR_W, startAddress, pRegisters, len);
    return ret == RET_OK ? VCHR11RET_ISOK : VCHR11RET_ISERR;
}


vcHr11Ret_t vcHr11ReadRegisters(uint8_t startAddress, uint8_t *pRegisters, uint8_t len)
{
    u8 ret = sensor_read(IIC_ADDR_R, startAddress, pRegisters, len, 0);
    return ret == RET_OK ? VCHR11RET_ISOK : VCHR11RET_ISERR;
}

static s8 sensor_check(void)
{
    u8  dev_id = 0;
    u32 vcHr11_addr = (u32)(&vcHr11);

    sensor_read(IIC_ADDR_R, VCREG0, &dev_id, 1, 0);

    if (sensor_info.cbuffer == NULL) {
        memcpy(sensor_info.range, &vcHr11_addr, sizeof(u32));
        sensor_info.cbuffer = &sensor_cbuffer;
        cbuf_mult_read_init(sensor_info.cbuffer, sensor_data, sizeof(sensor_data), 1, entry);
    }
    LOG("dev_id=%02x cbuf=%x vcHr11_addr=%x", dev_id, (u32)sensor_info.cbuffer, vcHr11_addr);
    return ((0x21 == dev_id || 0x29 == dev_id) && sensor_info.cbuffer) ? RET_OK : RET_ERR;
}

void cbuf_clear(p11_cbuffer_t *cbuffer);
static s8 sensor_open(u16 vcHr11WorkMode, u16 odr)
{
    vcHr11Ret_t ret = VCHR11RET_ISOK;
    vcHr11_t   *pVcHr11 = &vcHr11;

    LOG("mode=%d", vcHr11WorkMode);

    cbuf_clear(sensor_info.cbuffer);

    ret = vcHr11SoftReset(pVcHr11);
    ret = vcHr11StopSample(pVcHr11);

    if (VCWORK_MODE_POWER_OFF != vcHr11WorkMode) {

        pVcHr11->vcSampleRate = odr;
        pVcHr11->mcuOscValue  = mcuOscData;
        pVcHr11->mcuSclRate   = mcuI2cClock;
        pVcHr11->workMode     = vcHr11WorkMode;

        ret = vcHr11StartSample(pVcHr11);
    }
    return ret == VCHR11RET_ISOK ? RET_OK : RET_ERR;
}

static s8 sensor_close(void)
{
    vcHr11Ret_t ret = VCHR11RET_ISOK;
    vcHr11_t   *pVcHr11 = &vcHr11;

    ret = vcHr11SoftReset(pVcHr11);
    ret = vcHr11StopSample(pVcHr11);
    return ret == VCHR11RET_ISOK ? RET_OK : RET_ERR;
}




static s8 sensor_process(void *arg, u16 *len)
{
    u16 buf_len = 0;
    u8  ppgLength = 0;

    vcHr11GetSampleValues(&vcHr11, &ppgLength);
    LOG("oscFlg=%d ppgLength=%d readFlg=%d  wearStatus=%d", vcHr11.oscCheckFinishFlag, ppgLength, vcHr11.vcFifoReadFlag, vcHr11.wearStatus);

    if (vcHr11.vcFifoReadFlag && vcHr11.wearStatus) {
        vcHr11.vcFifoReadFlag = 0;
        if (ppgLength > 0) {
            cbuf_mult_write(sensor_info.cbuffer, vcHr11.sampleData.ppgValue, ppgLength * sizeof(u16));
        }
    }

    return RET_OK;
}


REGISTER_SENSOR(sensor_hr) = {
    .info    = &sensor_info,
    .online  = sensor_check,
    .open    = sensor_open,
    .close   = sensor_close,
    .run     = sensor_process,
};
#endif
